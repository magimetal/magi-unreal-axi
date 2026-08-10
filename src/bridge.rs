use crate::{
    capability::{self, CATALOG_HASH},
    engine::EngineInfo,
    error::AppError,
};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
#[cfg(unix)]
use std::os::unix::{
    fs::{MetadataExt, OpenOptionsExt, PermissionsExt},
    io::AsRawFd,
    process::CommandExt,
};
use std::{
    fs::{self, File, OpenOptions},
    io::{Read, Write},
    net::{Ipv4Addr, SocketAddrV4, TcpStream},
    path::{Path, PathBuf},
    process::{Child, Command, Stdio},
    thread,
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

const MAX_REQUEST: usize = 8 * 1024 * 1024;
const MAX_RESPONSE: usize = 16 * 1024 * 1024;
const MAX_DISCOVERY: u64 = 16 * 1024;
const PROTOCOL: u32 = 1;
const VERSION: &str = env!("CARGO_PKG_VERSION");

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct Discovery {
    protocol: u32,
    plugin_version: String,
    pid: u32,
    #[serde(default)]
    process_start: String,
    project_path: String,
    project_id: String,
    engine_version: String,
    host: String,
    port: u16,
    session_nonce: String,
    started_at: String,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct HandshakeResponse {
    protocol: u32,
    status: String,
    plugin_version: String,
    pid: u32,
    process_start: String,
    session_nonce: String,
    catalog_hash: String,
}

#[derive(Clone, Debug, Default)]
pub struct ExecutionOptions {
    pub expected_revision: Option<String>,
    pub idempotency_key: Option<String>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct Receipt {
    pub operation_id: String,
    pub operation: String,
    pub state: String,
    pub project_id: String,
    pub editor_pid: u32,
    pub target: String,
    pub changed: bool,
    pub transaction: String,
    pub reversibility: String,
    pub dirty_packages: Vec<String>,
    pub saved_packages: Vec<String>,
    pub revision: String,
    pub persistence: String,
    pub verification: Value,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
struct OperationResponse {
    protocol: u32,
    id: String,
    status: String,
    #[serde(default)]
    result: Value,
    #[serde(default)]
    receipt: Option<Receipt>,
    #[serde(default)]
    error: Option<BridgeOperationError>,
    #[serde(default, rename = "operationId")]
    operation_id: Option<String>,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct BridgeOperationError {
    #[serde(rename = "type")]
    kind: String,
    message: String,
    retryable: bool,
    #[serde(default)]
    dirty_package_count: Option<u64>,
    #[serde(default)]
    dirty_packages: Option<Vec<String>>,
    #[serde(default)]
    error_count: Option<u64>,
    #[serde(default)]
    warning_count: Option<u64>,
    #[serde(default)]
    diagnostics: Option<Value>,
    #[serde(default)]
    current_revision: Option<String>,
}
const MAX_DIRTY_PACKAGES: usize = 100;
const MAX_DIRTY_PACKAGE_NAME: usize = 256;

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct Owner {
    pid: u32,
    process_start: String,
    project_id: String,
    session_nonce: String,
    executable: String,
}

fn runtime_root() -> Result<PathBuf, AppError> {
    std::env::var_os("HOME")
        .map(PathBuf::from)
        .map(|home| home.join("Library/Caches/magi-unreal-axi"))
        .ok_or_else(|| bridge_error("home_unavailable", "HOME is required for editor discovery"))
}

fn project_identity(project: &Path) -> Result<(PathBuf, String, String), AppError> {
    let canonical = project.canonicalize().map_err(io_error)?;
    let text = canonical
        .to_str()
        .ok_or_else(|| bridge_error("project_path_encoding", "project path is not UTF-8"))?
        .to_owned();
    let hash = format!("{:x}", Sha256::digest(text.as_bytes()));
    Ok((canonical, format!("sha256:{hash}"), hash))
}

fn project_directory(project: &Path) -> Result<PathBuf, AppError> {
    let (_, _, hash) = project_identity(project)?;
    Ok(runtime_root()?.join(hash))
}

fn io_error(error: std::io::Error) -> AppError {
    AppError::operational(
        "bridge",
        "bridge_io_failed",
        error.to_string(),
        "magi-unreal-axi editor status",
    )
}
fn bridge_error(reason: &'static str, message: impl Into<String>) -> AppError {
    AppError::operational("bridge", reason, message, "magi-unreal-axi editor status")
}

fn outcome_unknown(id: &str) -> AppError {
    AppError::operational(
        "bridge",
        "outcome_unknown",
        "mutation outcome is unknown after request transmission",
        format!("inspect `magi-unreal-axi operation view {id}` before retrying"),
    )
    .with_operation_id(id.to_owned())
}

fn metadata(operation: &str) -> Option<&'static capability::CapabilityMetadata> {
    capability::capability_metadata(operation)
}

fn is_mutation(operation: &str) -> bool {
    operation == "editor.stop" || metadata(operation).is_some_and(|value| value.mutates)
}

fn requires_receipt(operation: &str) -> bool {
    metadata(operation).is_some_and(|value| value.mutates)
}

#[cfg(unix)]
fn validate_private(path: &Path, directory: bool) -> Result<(), AppError> {
    let metadata = fs::symlink_metadata(path).map_err(io_error)?;
    let expected_mode = if directory { 0o700 } else { 0o600 };
    let correct_type = if directory {
        metadata.is_dir()
    } else {
        metadata.is_file()
    };
    if metadata.file_type().is_symlink()
        || !correct_type
        || metadata.uid() != unsafe { libc::geteuid() }
        || metadata.permissions().mode() & 0o777 != expected_mode
    {
        return Err(AppError::operational(
            "security",
            "insecure_runtime_path",
            format!(
                "{} must be current-user {} mode {:o}",
                path.display(),
                if directory { "directory" } else { "file" },
                expected_mode
            ),
            "remove insecure discovery state and restart editor",
        ));
    }
    Ok(())
}
#[cfg(not(unix))]
fn validate_private(_path: &Path, _directory: bool) -> Result<(), AppError> {
    Err(bridge_error(
        "unsupported_platform",
        "bridge is certified only on macOS arm64",
    ))
}

fn read_bounded(path: &Path, limit: u64) -> Result<Vec<u8>, AppError> {
    let file = File::open(path).map_err(io_error)?;
    let mut bytes = Vec::new();
    file.take(limit + 1)
        .read_to_end(&mut bytes)
        .map_err(io_error)?;
    if bytes.len() as u64 > limit {
        return Err(bridge_error(
            "runtime_file_too_large",
            format!("{} exceeds {limit} bytes", path.display()),
        ));
    }
    Ok(bytes)
}

fn records(
    project: &Path,
    selected_pid: Option<u32>,
) -> Result<Vec<(PathBuf, Discovery)>, AppError> {
    let directory = project_directory(project)?;
    if !directory.exists() {
        return Ok(Vec::new());
    }
    validate_private(&runtime_root()?, true)?;
    validate_private(&directory, true)?;
    let (canonical, expected_id, _) = project_identity(project)?;
    let canonical = canonical.to_str().unwrap();
    let mut found = Vec::new();
    for entry in fs::read_dir(&directory).map_err(io_error)? {
        let entry = entry.map_err(io_error)?;
        if !entry.file_type().map_err(io_error)?.is_dir() {
            continue;
        }
        if selected_pid
            .is_some_and(|pid| entry.file_name() != std::ffi::OsStr::new(&pid.to_string()))
        {
            continue;
        }
        let session = entry.path();
        validate_private(&session, true)?;
        let path = session.join("bridge-v1.json");
        if !path.exists() {
            continue;
        }
        validate_private(&path, false)?;
        if path.metadata().map_err(io_error)?.len() > MAX_DISCOVERY {
            return Err(bridge_error(
                "discovery_too_large",
                path.display().to_string(),
            ));
        }
        let discovery: Discovery = serde_json::from_slice(&read_bounded(&path, MAX_DISCOVERY)?)
            .map_err(|error| {
                bridge_error(
                    "malformed_discovery",
                    format!("{}: {error}", path.display()),
                )
            })?;
        if !process_matches(&discovery) {
            found.push((session, discovery));
            continue;
        }
        if discovery.project_path != canonical || discovery.project_id != expected_id {
            return Err(bridge_error(
                "wrong_project",
                format!("discovery {} identifies another project", path.display()),
            ));
        }
        if discovery.protocol != PROTOCOL
            || discovery.plugin_version != VERSION
            || discovery.engine_version.is_empty()
        {
            return Err(bridge_error(
                "incompatible_editor",
                format!(
                    "editor {} has protocol {} plugin {} engine {}",
                    discovery.pid,
                    discovery.protocol,
                    discovery.plugin_version,
                    discovery.engine_version
                ),
            ));
        }
        if discovery.host != "127.0.0.1"
            || discovery.port == 0
            || discovery.session_nonce.len() != 32
            || discovery.started_at.is_empty()
        {
            return Err(bridge_error(
                "malformed_discovery",
                format!("invalid endpoint/session fields in {}", path.display()),
            ));
        }
        found.push((session, discovery));
    }
    found.sort_by_key(|(_, discovery)| discovery.pid);
    Ok(found)
}

#[cfg(target_os = "macos")]
fn process_start_identity(pid: u32) -> Option<String> {
    let mut info = std::mem::MaybeUninit::<libc::proc_bsdinfo>::zeroed();
    let size = std::mem::size_of::<libc::proc_bsdinfo>();
    // SAFETY: buffer points to writable proc_bsdinfo storage of exactly the supplied size.
    let read = unsafe {
        libc::proc_pidinfo(
            pid as libc::c_int,
            libc::PROC_PIDTBSDINFO,
            0,
            info.as_mut_ptr().cast(),
            size as libc::c_int,
        )
    };
    if read != size as libc::c_int {
        return None;
    }
    // SAFETY: proc_pidinfo reported that it initialized the complete proc_bsdinfo buffer.
    let info = unsafe { info.assume_init() };
    (info.pbi_pid == pid).then(|| format!("{}:{}", info.pbi_start_tvsec, info.pbi_start_tvusec))
}
#[cfg(not(target_os = "macos"))]
fn process_start_identity(_pid: u32) -> Option<String> {
    None
}

fn process_matches(discovery: &Discovery) -> bool {
    process_start_identity(discovery.pid).as_deref() == Some(&discovery.process_start)
}

enum Selection {
    None,
    Stale,
    Live(PathBuf, Box<Discovery>),
}
fn select(project: &Path, selected_pid: Option<u32>) -> Result<Selection, AppError> {
    let all = records(project, selected_pid)?;
    let matching = all
        .into_iter()
        .filter(|(_, discovery)| selected_pid.is_none_or(|pid| discovery.pid == pid))
        .collect::<Vec<_>>();
    if matching.is_empty() {
        return Ok(Selection::None);
    }
    let mut live = matching
        .into_iter()
        .filter(|(_, discovery)| process_matches(discovery))
        .collect::<Vec<_>>();
    match live.len() {
        0 => Ok(Selection::Stale),
        1 => {
            let (path, discovery) = live.pop().unwrap();
            Ok(Selection::Live(path, Box::new(discovery)))
        }
        _ => Err(bridge_error(
            "ambiguous_editor",
            "multiple matching live editors; select --editor <pid>",
        )),
    }
}

fn remaining(deadline: Instant) -> Result<Duration, AppError> {
    deadline
        .checked_duration_since(Instant::now())
        .ok_or_else(|| {
            bridge_error(
                "bridge_deadline_exceeded",
                "editor bridge deadline exceeded",
            )
        })
}

fn deadline_error(error: std::io::Error) -> AppError {
    if matches!(
        error.kind(),
        std::io::ErrorKind::TimedOut | std::io::ErrorKind::WouldBlock
    ) {
        bridge_error(
            "bridge_deadline_exceeded",
            "editor bridge deadline exceeded",
        )
    } else {
        io_error(error)
    }
}

fn read_frame(stream: &mut TcpStream, max: usize, deadline: Instant) -> Result<Vec<u8>, AppError> {
    let mut length = [0_u8; 4];
    for byte in &mut length {
        stream
            .set_read_timeout(Some(remaining(deadline)?))
            .map_err(io_error)?;
        stream
            .read_exact(std::slice::from_mut(byte))
            .map_err(deadline_error)?;
    }
    let size = u32::from_le_bytes(length) as usize;
    if size == 0 || size > max {
        return Err(bridge_error(
            "frame_too_large",
            format!("invalid frame size {size}; maximum {max}"),
        ));
    }
    let mut data = vec![0; size];
    let mut offset = 0;
    while offset < data.len() {
        stream
            .set_read_timeout(Some(remaining(deadline)?))
            .map_err(io_error)?;
        let read = stream.read(&mut data[offset..]).map_err(deadline_error)?;
        if read == 0 {
            return Err(bridge_error(
                "malformed_response",
                "response ended before frame completed",
            ));
        }
        offset += read;
    }
    std::str::from_utf8(&data)
        .map_err(|_| bridge_error("malformed_response", "response is not UTF-8"))?;
    Ok(data)
}

fn write_frame(stream: &mut TcpStream, value: &Value, deadline: Instant) -> Result<(), AppError> {
    let data = serde_json::to_vec(value)
        .map_err(|error| bridge_error("serialization_failed", error.to_string()))?;
    if data.is_empty() || data.len() > MAX_REQUEST {
        return Err(bridge_error("frame_too_large", "request exceeds 8 MiB"));
    }
    let mut frame = Vec::with_capacity(4 + data.len());
    frame.extend_from_slice(&(data.len() as u32).to_le_bytes());
    frame.extend_from_slice(&data);
    let mut offset = 0;
    while offset < frame.len() {
        stream
            .set_write_timeout(Some(remaining(deadline)?))
            .map_err(io_error)?;
        let written = stream.write(&frame[offset..]).map_err(deadline_error)?;
        if written == 0 {
            return Err(io_error(std::io::Error::new(
                std::io::ErrorKind::WriteZero,
                "bridge write made no progress",
            )));
        }
        offset += written;
    }
    Ok(())
}

fn token(session: &Path) -> Result<String, AppError> {
    let path = session.join("token");
    validate_private(&path, false)?;
    let bytes = read_bounded(&path, 64)?;
    if bytes.len() != 64 || !bytes.iter().all(u8::is_ascii_hexdigit) {
        return Err(bridge_error(
            "insecure_token",
            "token must be exactly 64 hexadecimal bytes",
        ));
    }
    String::from_utf8(bytes).map_err(|_| bridge_error("insecure_token", "token is not UTF-8"))
}

fn exchange_selected_args(
    session: &Path,
    discovery: &Discovery,
    operation: &str,
    args: Value,
    options: &ExecutionOptions,
    deadline: Instant,
) -> Result<Value, AppError> {
    let secret = token(session)?;
    let address = SocketAddrV4::new(Ipv4Addr::LOCALHOST, discovery.port);
    let mut stream =
        TcpStream::connect_timeout(&address.into(), remaining(deadline)?).map_err(|error| {
            if matches!(
                error.kind(),
                std::io::ErrorKind::TimedOut | std::io::ErrorKind::WouldBlock
            ) {
                bridge_error(
                    "bridge_deadline_exceeded",
                    "editor bridge deadline exceeded",
                )
            } else {
                io_error(error)
            }
        })?;
    write_frame(
        &mut stream,
        &json!({"protocol":PROTOCOL,"kind":"handshake","token":secret,"projectId":discovery.project_id,"pid":discovery.pid,"processStart":discovery.process_start,"sessionNonce":discovery.session_nonce,"cliVersion":VERSION,"deadlineMs":remaining(deadline)?.as_millis().min(u128::from(u32::MAX)) as u32}),
        deadline,
    )?;
    let handshake_bytes = read_frame(&mut stream, MAX_RESPONSE, deadline).map_err(|error| {
        if error.reason == "bridge_deadline_exceeded" {
            error
        } else {
            bridge_error("handshake_failed", "editor rejected bridge handshake")
        }
    })?;
    let handshake: HandshakeResponse = serde_json::from_slice(&handshake_bytes)
        .map_err(|error| bridge_error("handshake_failed", error.to_string()))?;
    if handshake.protocol != PROTOCOL
        || handshake.status != "ok"
        || handshake.plugin_version != VERSION
        || handshake.pid != discovery.pid
        || handshake.process_start != discovery.process_start
        || handshake.session_nonce != discovery.session_nonce
    {
        return Err(bridge_error(
            "handshake_rejected",
            "editor handshake identity did not match discovery",
        ));
    }
    if handshake.catalog_hash != CATALOG_HASH {
        return Err(AppError::operational(
            "bridge",
            "catalog_mismatch",
            "installed plugin capability catalog does not match CLI",
            "magi-unreal-axi setup plugin update",
        ));
    }
    let id = format!(
        "{}-{}",
        std::process::id(),
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos()
    );
    let mut envelope = json!({"protocol":PROTOCOL,"id":id,"operation":operation,"args":args.clone(),"deadlineMs":remaining(deadline)?.as_millis().min(u128::from(u32::MAX)) as u32});
    if let Some(value) = &options.expected_revision {
        envelope["expectedRevision"] = json!(value);
    }
    if let Some(value) = &options.idempotency_key {
        envelope["idempotencyKey"] = json!(value);
    }
    write_frame(&mut stream, &envelope, deadline)?;
    let response_bytes = read_frame(&mut stream, MAX_RESPONSE, deadline).map_err(|error| {
        if is_mutation(operation) {
            outcome_unknown(&id)
        } else {
            error
        }
    })?;
    let response: OperationResponse = serde_json::from_slice(&response_bytes).map_err(|error| {
        if is_mutation(operation) {
            outcome_unknown(&id)
        } else {
            bridge_error("malformed_response", error.to_string())
        }
    })?;
    if response.protocol != PROTOCOL || response.id != id {
        return Err(if is_mutation(operation) {
            outcome_unknown(&id)
        } else {
            bridge_error(
                "mismatched_response",
                "response protocol or operation ID did not match",
            )
        });
    }
    let operation_id = response.operation_id.as_deref().unwrap_or(&id).to_owned();
    if response.operation_id.is_some()
        && (options.idempotency_key.is_none()
            || operation_id.is_empty()
            || operation_id.len() > 128)
    {
        return Err(outcome_unknown(&id));
    }
    if response.status == "ok" {
        let mut result = response.result;
        if requires_receipt(operation) {
            let receipt = response
                .receipt
                .as_ref()
                .ok_or_else(|| outcome_unknown(&id))?;
            validate_receipt_with_replay(
                operation,
                &operation_id,
                receipt,
                &result,
                &args,
                discovery,
                options.idempotency_key.is_some(),
            )?;
            result = json!({"result": result, "receipt": receipt});
        }
        if operation == "editor.stop" {
            let object = result.as_object_mut().ok_or_else(|| outcome_unknown(&id))?;
            object.insert("operationId".into(), json!(id));
        }
        return Ok(result);
    }
    if response.status != "error" {
        return Err(if operation == "editor.stop" {
            outcome_unknown(&id)
        } else {
            bridge_error("malformed_response", "response has unknown status")
        });
    }
    let error = response.error.ok_or_else(|| {
        if operation == "editor.stop" {
            outcome_unknown(&id)
        } else {
            bridge_error("malformed_response", "error response lacks error object")
        }
    })?;
    if matches!(error.kind.as_str(), "invalid_input" | "stale_cursor") {
        return Err(AppError::usage(
            if error.kind == "stale_cursor" {
                "stale_cursor"
            } else {
                "invalid_capability_input"
            },
            error.message,
            if error.kind == "stale_cursor" {
                format!(
                    "rerun `magi-unreal-axi {}` without --cursor",
                    operation.replace('.', " ")
                )
            } else {
                format!("magi-unreal-axi capability describe {operation}")
            },
        ));
    }
    if error.kind == "conflict" {
        let current = error
            .current_revision
            .as_deref()
            .unwrap_or("unknown")
            .chars()
            .take(128)
            .collect::<String>();
        return Err(AppError::usage(
            "conflict",
            format!("{} currentRevision={current}", error.message),
            "re-read the target with `magi-unreal-axi actor view <id>`, then retry with --expected-revision <currentRevision>",
        ));
    }
    let reason = match error.kind.as_str() {
        "unsafe_editor_state" => "unsafe_editor_state",
        "busy" => "busy",
        "timeout" => "timeout",
        "unsupported" => "unsupported",
        "not_found" => "not_found",
        "outcome_unknown" => "outcome_unknown",
        _ => "operation_failed",
    };
    if matches!(
        operation,
        "blueprint.create"
            | "blueprint.interface_create"
            | "blueprint.interface_ensure"
            | "blueprint.scs_component_ensure"
            | "blueprint.scs_component_update"
            | "blueprint.scs_component_remove"
            | "blueprint.event_ensure"
            | "blueprint.node_ensure"
            | "blueprint.pin_default_set"
            | "blueprint.pin_connect"
            | "widget.create"
            | "widget.child_ensure"
            | "widget.property_set"
            | "widget.event_ensure"
            | "widget.viewport_ensure"
    ) && matches!(error.kind.as_str(), "operation_failed" | "outcome_unknown")
    {
        let receipt = response
            .receipt
            .as_ref()
            .ok_or_else(|| outcome_unknown(&id))?;
        validate_failed_atomic_receipt(
            operation,
            &operation_id,
            &args,
            options,
            receipt,
            &error,
            discovery,
        )?;
        let app_error = AppError::operational(
            "bridge",
            if error.kind == "outcome_unknown" {
                "outcome_unknown"
            } else {
                "operation_failed"
            },
            error.message.clone(),
            format!("inspect `magi-unreal-axi operation view {operation_id}` before retrying"),
        )
        .with_bridge_details(
            error.retryable,
            error.dirty_package_count,
            error.dirty_packages.clone(),
        )
        .with_operation_id(operation_id.clone())
        .with_receipt(serde_json::to_value(receipt).map_err(|_| outcome_unknown(&id))?);
        return Err(app_error);
    }
    if operation == "navigation.build"
        && error.kind == "navigation_build_failed"
        && metadata(operation).and_then(|value| value.failure_receipt) == Some("terminal-ticket")
    {
        let receipt = response
            .receipt
            .as_ref()
            .ok_or_else(|| outcome_unknown(&id))?;
        validate_failed_terminal_ticket_receipt(&operation_id, &args, receipt, &error, discovery)?;
        let app_error = AppError::operational(
            "bridge",
            "navigation_build_failed",
            error.message.clone(),
            format!("inspect `magi-unreal-axi operation view {operation_id}` before retrying"),
        )
        .with_bridge_details(
            error.retryable,
            error.dirty_package_count,
            error.dirty_packages.clone(),
        )
        .with_operation_id(receipt.operation_id.clone())
        .with_receipt(serde_json::to_value(receipt).map_err(|_| outcome_unknown(&id))?);
        return Err(app_error);
    }
    if operation == "blueprint.compile"
        && error.kind == "blueprint_compile_failed"
        && metadata(operation).and_then(|value| value.failure_receipt) == Some("preserved-dirty")
    {
        let receipt = response
            .receipt
            .as_ref()
            .ok_or_else(|| outcome_unknown(&id))?;
        validate_failed_receipt(
            operation,
            &operation_id,
            &args,
            options,
            receipt,
            &error,
            discovery,
        )?;
        let app_error = AppError::operational(
            "bridge",
            "blueprint_compile_failed",
            error.message.clone(),
            format!("inspect `magi-unreal-axi operation view {operation_id}` before retrying"),
        )
        .with_bridge_details(
            error.retryable,
            error.dirty_package_count,
            error.dirty_packages.clone(),
        )
        .with_bridge_diagnostics(
            error.error_count,
            error.warning_count,
            error.diagnostics.clone(),
        )
        .with_operation_id(receipt.operation_id.clone())
        .with_receipt(serde_json::to_value(receipt).map_err(|_| outcome_unknown(&id))?);
        return Err(app_error);
    }

    let app_error = AppError::operational(
        "bridge",
        reason,
        error.message,
        "magi-unreal-axi editor status",
    )
    .with_bridge_details(
        error.retryable,
        error.dirty_package_count,
        error.dirty_packages.map(|packages| {
            packages
                .into_iter()
                .take(MAX_DIRTY_PACKAGES)
                .map(|package| package.chars().take(MAX_DIRTY_PACKAGE_NAME).collect())
                .collect()
        }),
    )
    .with_bridge_diagnostics(error.error_count, error.warning_count, error.diagnostics);
    Err(if is_mutation(operation) {
        app_error.with_operation_id(operation_id)
    } else {
        app_error
    })
}

fn validate_failed_atomic_receipt(
    operation: &str,
    id: &str,
    args: &Value,
    options: &ExecutionOptions,
    receipt: &Receipt,
    error: &BridgeOperationError,
    discovery: &Discovery,
) -> Result<(), AppError> {
    let record = metadata(operation).ok_or_else(|| outcome_unknown(id))?;
    let verification = receipt
        .verification
        .as_object()
        .ok_or_else(|| outcome_unknown(id))?;
    let target = match operation {
        "blueprint.create" => {
            let path = args["path"].as_str().ok_or_else(|| outcome_unknown(id))?;
            format!("{path}.{}", path.rsplit('/').next().unwrap_or_default())
        }
        "widget.create" => {
            let path = args["path"].as_str().ok_or_else(|| outcome_unknown(id))?;
            format!("{path}.{}", path.rsplit('/').next().unwrap_or_default())
        }
        "widget.child_ensure" => {
            let blueprint = args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?;
            let name = args["name"].as_str().ok_or_else(|| outcome_unknown(id))?;
            format!("{blueprint}#{blueprint}#widget:{name}")
        }
        "widget.property_set" => format!(
            "{}#{}#{}",
            args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["widgetId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["property"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?
        ),
        "widget.event_ensure" => {
            let blueprint = args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?;
            let agent = args["agentKey"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?;
            format!("{blueprint}#{blueprint}#event:{agent}")
        }
        "widget.viewport_ensure" => format!(
            "{}#viewport:{}",
            args["hostBlueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["agentKey"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?
        ),
        "widget.tree_view" => args["blueprintId"]
            .as_str()
            .ok_or_else(|| outcome_unknown(id))?
            .to_owned(),
        "blueprint.event_ensure" | "blueprint.node_ensure" => format!(
            "{}#{}#{}",
            args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["graphId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["agentKey"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?
        ),
        "blueprint.pin_default_set" => format!(
            "{}#{}",
            args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["pinId"].as_str().ok_or_else(|| outcome_unknown(id))?
        ),
        "blueprint.pin_connect" => format!(
            "{}#{}#{}",
            args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["sourcePinId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["targetPinId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?
        ),
        "blueprint.interface_create" => {
            let path = args["path"].as_str().ok_or_else(|| outcome_unknown(id))?;
            format!("{path}.{}", path.rsplit('/').next().unwrap_or_default())
        }
        "blueprint.interface_ensure" => format!(
            "{}#{}",
            args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["interfaceId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?
        ),
        "blueprint.scs_component_ensure" => format!(
            "{}#scs-name:{}",
            args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["name"].as_str().ok_or_else(|| outcome_unknown(id))?
        ),
        "blueprint.scs_component_update" | "blueprint.scs_component_remove" => format!(
            "{}#scs:{}",
            args["blueprintId"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?,
            args["variableGuid"]
                .as_str()
                .ok_or_else(|| outcome_unknown(id))?
        ),
        _ => return Err(outcome_unknown(id)),
    };
    let before = verification["beforeRevision"]
        .as_str()
        .ok_or_else(|| outcome_unknown(id))?;
    let observed = verification["observedRevision"]
        .as_str()
        .ok_or_else(|| outcome_unknown(id))?;
    let dirty = error
        .dirty_packages
        .as_ref()
        .ok_or_else(|| outcome_unknown(id))?;
    let unknown = error.kind == "outcome_unknown";
    let valid = capability::canonical_revision;
    let absent_revision = format!("{:x}", Sha256::digest(format!("{target}\nabsent")));
    let expected_before = if matches!(operation, "blueprint.create" | "blueprint.interface_create")
    {
        absent_revision.clone()
    } else if operation == "widget.create" {
        if before == absent_revision || (before == observed && !receipt.changed) {
            before.to_owned()
        } else {
            return Err(outcome_unknown(id));
        }
    } else {
        options
            .expected_revision
            .clone()
            .ok_or_else(|| outcome_unknown(id))?
    };
    if receipt.operation_id != id
        || receipt.operation != operation
        || receipt.state != if unknown { "outcome_unknown" } else { "failed" }
        || receipt.project_id != discovery.project_id
        || receipt.editor_pid != discovery.pid
        || receipt.target != target
        || verification["target"].as_str() != Some(target.as_str())
        || verification["readback"].as_str() != record.readback
        || verification["matched"].as_bool() != Some(!unknown)
        || receipt.changed != (before != observed)
        || (!unknown && before != observed)
        || receipt.revision != observed
        || !valid(before)
        || before != expected_before
        || !valid(observed)
        || receipt.dirty_packages != *dirty
        || error.dirty_package_count != Some(dirty.len() as u64)
        || !receipt.saved_packages.is_empty()
        || receipt.persistence
            != if dirty.is_empty() {
                "unchanged"
            } else {
                "dirty"
            }
        || receipt.transaction != record.transaction_behavior
        || receipt.reversibility != record.reversibility
        || verification["observedStatus"].as_str() != Some("error")
        || error.retryable == unknown
    {
        return Err(outcome_unknown(id));
    }
    if operation == "blueprint.create"
        && (verification["requestPath"] != args["path"]
            || verification["requestParentClass"] != args["parentClass"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "blueprint.interface_create"
        && (verification["requestPath"] != args["path"]
            || verification["requestFunction"] != args["function"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "blueprint.interface_ensure"
        && (verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestInterfaceId"] != args["interfaceId"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "blueprint.scs_component_ensure"
        && (verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestName"] != args["name"]
            || verification["requestClass"] != args["class"]
            || verification["requestParent"] != args["parent"])
    {
        return Err(outcome_unknown(id));
    }
    if matches!(
        operation,
        "blueprint.scs_component_update" | "blueprint.scs_component_remove"
    ) && (verification["requestBlueprintId"] != args["blueprintId"]
        || verification["requestVariableGuid"] != args["variableGuid"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "blueprint.scs_component_remove"
        && (verification["requestForce"] != args["force"]
            || verification["requestDryRun"] != args["dryRun"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "blueprint.scs_component_update" {
        for (argument, request) in [
            ("location", "requestLocation"),
            ("rotation", "requestRotation"),
            ("scale", "requestScale"),
            ("collisionEnabled", "requestCollisionEnabled"),
            ("collisionProfile", "requestCollisionProfile"),
            ("generateOverlapEvents", "requestGenerateOverlapEvents"),
            ("simulatePhysics", "requestSimulatePhysics"),
            ("gravityEnabled", "requestGravityEnabled"),
            ("massOverride", "requestMassOverride"),
            ("boxExtent", "requestBoxExtent"),
            ("sphereRadius", "requestSphereRadius"),
        ] {
            if args.get(argument).is_some() && verification[request] != args[argument] {
                return Err(outcome_unknown(id));
            }
        }
    }
    if matches!(
        operation,
        "blueprint.event_ensure" | "blueprint.node_ensure"
    ) {
        let intent = if operation == "blueprint.event_ensure" {
            "event"
        } else {
            "node"
        };
        if verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestGraphId"] != args["graphId"]
            || verification["requestAgentKey"] != args["agentKey"]
            || verification["requestIntent"] != args[intent]
            || verification.get("requestVariableGuid") != args.get("variableGuid")
            || verification.get("requestInterfaceId") != args.get("interfaceId")
        {
            return Err(outcome_unknown(id));
        }
    }
    if operation == "blueprint.pin_default_set" {
        let value = args["value"]
            .as_object()
            .ok_or_else(|| outcome_unknown(id))?;
        if verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestPinId"] != args["pinId"]
            || verification["requestValueType"] != value["type"]
            || verification["requestValue"] != value["value"]
        {
            return Err(outcome_unknown(id));
        }
    }
    if operation == "blueprint.pin_connect"
        && (verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestSourcePinId"] != args["sourcePinId"]
            || verification["requestTargetPinId"] != args["targetPinId"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "widget.create"
        && (verification["requestPath"] != args["path"]
            || verification["requestRootName"] != args["rootName"]
            || verification["requestRootClass"] != args["rootClass"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "widget.child_ensure"
        && (verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestParentWidgetId"] != args["parentWidgetId"]
            || verification["requestName"] != args["name"]
            || verification["requestClass"] != args["class"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "widget.property_set"
        && (verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestWidgetId"] != args["widgetId"]
            || verification["requestProperty"] != args["property"]
            || verification.get("requestText") != args.get("text")
            || verification.get("requestVisibility") != args.get("visibility")
            || verification.get("requestEnabled") != args.get("enabled"))
    {
        return Err(outcome_unknown(id));
    }
    if operation == "widget.event_ensure"
        && (verification["requestBlueprintId"] != args["blueprintId"]
            || verification["requestAgentKey"] != args["agentKey"]
            || verification["requestIntent"] != args["event"]
            || verification["requestActions"] != args["actions"])
    {
        return Err(outcome_unknown(id));
    }
    if operation == "widget.viewport_ensure"
        && (verification["requestHostBlueprintId"] != args["hostBlueprintId"]
            || verification["requestWidgetBlueprintId"] != args["widgetBlueprintId"]
            || verification["requestAgentKey"] != args["agentKey"]
            || verification["requestInputKey"] != args["inputKey"]
            || verification["requestZOrder"] != args["zOrder"])
    {
        return Err(outcome_unknown(id));
    }
    Ok(())
}

fn validate_failed_terminal_ticket_receipt(
    id: &str,
    args: &Value,
    receipt: &Receipt,
    error: &BridgeOperationError,
    discovery: &Discovery,
) -> Result<(), AppError> {
    let verification = receipt
        .verification
        .as_object()
        .ok_or_else(|| outcome_unknown(id))?;
    let level = args["levelId"]
        .as_str()
        .ok_or_else(|| outcome_unknown(id))?;
    let ticket = verification["ticketId"]
        .as_str()
        .ok_or_else(|| outcome_unknown(id))?;
    let observed = verification["observedRevision"]
        .as_str()
        .ok_or_else(|| outcome_unknown(id))?;
    let dirty = error
        .dirty_packages
        .as_ref()
        .ok_or_else(|| outcome_unknown(id))?;
    let allowed = [
        "target",
        "readback",
        "matched",
        "ticketId",
        "levelId",
        "requestLevelId",
        "observedRevision",
        "observedStatus",
        "terminal",
        "failureType",
        "failureMessage",
    ];
    if verification
        .keys()
        .any(|key| !allowed.contains(&key.as_str()))
        || receipt.operation_id != id
        || receipt.operation != "navigation.build"
        || verification.get("matched") != Some(&json!(true))
        || verification["target"].as_str() != Some(ticket)
        || verification["levelId"].as_str() != Some(level)
        || receipt.state != "failed"
        || receipt.project_id != discovery.project_id
        || receipt.editor_pid != discovery.pid
        || receipt.target != ticket
        || verification["terminal"] != json!(true)
        || verification["failureType"].as_str() != Some(error.kind.as_str())
        || verification["readback"].as_str()
            != metadata("navigation.build").and_then(|value| value.readback)
        || verification.get("requestLevelId") != args.get("levelId")
        || verification["observedStatus"].as_str() != Some("failed")
        || verification["failureType"].as_str() != Some(error.kind.as_str())
        || verification["failureMessage"].as_str() != Some(error.message.as_str())
        || !capability::canonical_revision(observed)
        || receipt.revision != observed
        || error.retryable
        || receipt.changed
        || !receipt.dirty_packages.is_empty()
        || !dirty.is_empty()
        || error.dirty_package_count != Some(0)
        || !receipt.saved_packages.is_empty()
        || receipt.persistence != "unchanged"
        || receipt.transaction != "non-atomic"
        || receipt.reversibility != "none"
    {
        return Err(outcome_unknown(id));
    }
    Ok(())
}

fn validate_failed_receipt(
    operation: &str,
    id: &str,
    args: &Value,
    options: &ExecutionOptions,
    receipt: &Receipt,
    error: &BridgeOperationError,
    discovery: &Discovery,
) -> Result<(), AppError> {
    let metadata = metadata(operation).ok_or_else(|| outcome_unknown(id))?;
    let verification = receipt
        .verification
        .as_object()
        .ok_or_else(|| outcome_unknown(id))?;
    let allowed_verification = [
        "target",
        "readback",
        "matched",
        "beforeRevision",
        "observedRevision",
        "observedStatus",
        "failureType",
        "errorCount",
        "warningCount",
        "diagnostics",
        "changedObjects",
    ];
    if verification
        .keys()
        .any(|key| !allowed_verification.contains(&key.as_str()))
    {
        return Err(outcome_unknown(id));
    }
    let target = args
        .get("id")
        .and_then(Value::as_str)
        .ok_or_else(|| outcome_unknown(id))?;
    let dirty = error
        .dirty_packages
        .as_ref()
        .filter(|packages| packages.len() <= MAX_DIRTY_PACKAGES)
        .ok_or_else(|| outcome_unknown(id))?;
    let observed_revision = verification
        .get("observedRevision")
        .and_then(Value::as_str)
        .ok_or_else(|| outcome_unknown(id))?;
    let before_revision = verification
        .get("beforeRevision")
        .and_then(Value::as_str)
        .ok_or_else(|| outcome_unknown(id))?;
    let changed_objects = verification
        .get("changedObjects")
        .and_then(Value::as_array)
        .filter(|objects| objects.len() <= MAX_DIRTY_PACKAGES)
        .ok_or_else(|| outcome_unknown(id))?;
    let changed_objects = changed_objects
        .iter()
        .map(Value::as_str)
        .collect::<Option<Vec<_>>>()
        .ok_or_else(|| outcome_unknown(id))?;
    let valid_revision = capability::canonical_revision;
    let changed = before_revision != observed_revision;
    if error.retryable
        || receipt.operation_id != id
        || receipt.operation != operation
        || receipt.state != "failed"
        || receipt.project_id != discovery.project_id
        || receipt.editor_pid != discovery.pid
        || receipt.target != target
        || receipt.transaction != metadata.transaction_behavior
        || receipt.reversibility != metadata.reversibility
        || receipt.dirty_packages != *dirty
        || error.dirty_package_count != Some(dirty.len() as u64)
        || dirty
            .iter()
            .any(|package| package.chars().count() > MAX_DIRTY_PACKAGE_NAME)
        || !receipt.saved_packages.is_empty()
        || receipt.revision != observed_revision
        || !valid_revision(&receipt.revision)
        || receipt.changed != changed
        || changed_objects != if changed { vec![target] } else { Vec::new() }
        || receipt.persistence
            != if dirty.is_empty() {
                "unchanged"
            } else {
                "dirty"
            }
        || verification.get("target").and_then(Value::as_str) != Some(target)
        || verification.get("readback").and_then(Value::as_str) != metadata.readback
        || verification.get("matched").and_then(Value::as_bool) != Some(true)
        || options.expected_revision.as_deref() != Some(before_revision)
        || !valid_revision(before_revision)
        || !valid_revision(observed_revision)
        || verification.get("observedStatus").and_then(Value::as_str) != Some("error")
        || verification.get("failureType").and_then(Value::as_str)
            != Some("blueprint_compile_failed")
        || verification.get("errorCount").and_then(Value::as_u64) != error.error_count
        || verification.get("warningCount").and_then(Value::as_u64) != error.warning_count
        || verification.get("diagnostics") != error.diagnostics.as_ref()
    {
        return Err(outcome_unknown(id));
    }
    Ok(())
}

#[cfg_attr(not(test), allow(dead_code))]
fn validate_receipt(
    operation: &str,
    id: &str,
    receipt: &Receipt,
    result: &Value,
    args: &Value,
    discovery: &Discovery,
) -> Result<(), AppError> {
    validate_receipt_with_replay(operation, id, receipt, result, args, discovery, false)
}

fn validate_receipt_with_replay(
    operation: &str,
    id: &str,
    receipt: &Receipt,
    result: &Value,
    args: &Value,
    discovery: &Discovery,
    _replayable: bool,
) -> Result<(), AppError> {
    let capability_record = metadata(operation).ok_or_else(|| outcome_unknown(id))?;
    let verification = receipt
        .verification
        .as_object()
        .ok_or_else(|| outcome_unknown(id))?;
    let target = verification
        .get("target")
        .and_then(Value::as_str)
        .ok_or_else(|| outcome_unknown(id))?;
    let readback = verification
        .get("readback")
        .and_then(Value::as_str)
        .ok_or_else(|| outcome_unknown(id))?;
    let expected_readback = capability_record
        .readback
        .ok_or_else(|| outcome_unknown(id))?;
    let matched = verification
        .get("matched")
        .and_then(Value::as_bool)
        .ok_or_else(|| outcome_unknown(id))?;
    let result_target = match operation {
        "play.screenshot" => result["path"].as_str().unwrap_or_default().to_owned(),
        "navigation.bounds_ensure" => format!(
            "{}#nav-bounds:{}",
            args["levelId"].as_str().unwrap_or_default(),
            args["agentKey"].as_str().unwrap_or_default()
        ),
        "navigation.build" => result["ticketId"].as_str().unwrap_or_default().to_owned(),
        "blackboard.key_ensure" => format!(
            "{}#{}",
            args["blackboardId"].as_str().unwrap_or_default(),
            args["keyName"].as_str().unwrap_or_default()
        ),
        "behavior_tree.connect" => {
            let link_id = result["linkId"].as_str().unwrap_or_default();
            format!(
                "{}#{}",
                args["behaviorTreeId"].as_str().unwrap_or_default(),
                link_id
            )
        }
        "ai.controller_configure" => format!(
            "{}#ai-controller:{}",
            args["blueprintId"].as_str().unwrap_or_default(),
            args["behaviorTreeId"].as_str().unwrap_or_default()
        ),
        "ai.pawn_configure" => format!(
            "{}#ai-pawn",
            args["blueprintId"].as_str().unwrap_or_default()
        ),
        "play.ai_target_set" => format!(
            "{}#{}#{}#{}",
            args["sessionId"].as_str().unwrap_or_default(),
            args["pawnId"].as_str().unwrap_or_default(),
            args["keyName"].as_str().unwrap_or_default(),
            args["targetActorId"].as_str().unwrap_or_default()
        ),
        "behavior_tree.node_ensure" => format!(
            "{}#{}",
            args["behaviorTreeId"].as_str().unwrap_or_default(),
            args["nodeId"].as_str().unwrap_or_default()
        ),
        "blueprint.event_ensure" | "blueprint.node_ensure" => format!(
            "{}#{}#{}",
            args["blueprintId"].as_str().unwrap_or_default(),
            args["graphId"].as_str().unwrap_or_default(),
            args["agentKey"].as_str().unwrap_or_default()
        ),
        "blueprint.pin_default_set" => format!(
            "{}#{}",
            args["blueprintId"].as_str().unwrap_or_default(),
            args["pinId"].as_str().unwrap_or_default()
        ),
        "blueprint.pin_connect" => format!(
            "{}#{}#{}",
            args["blueprintId"].as_str().unwrap_or_default(),
            args["sourcePinId"].as_str().unwrap_or_default(),
            args["targetPinId"].as_str().unwrap_or_default()
        ),
        "blueprint.interface_ensure" => format!(
            "{}#{}",
            args["blueprintId"].as_str().unwrap_or_default(),
            args["interfaceId"].as_str().unwrap_or_default()
        ),
        "blueprint.scs_component_ensure" => format!(
            "{}#scs-name:{}",
            args["blueprintId"].as_str().unwrap_or_default(),
            args["name"].as_str().unwrap_or_default()
        ),
        "blueprint.scs_component_update" | "blueprint.scs_component_remove" => format!(
            "{}#scs:{}",
            args["blueprintId"].as_str().unwrap_or_default(),
            args["variableGuid"].as_str().unwrap_or_default()
        ),
        "widget.viewport_ensure" => result["viewportId"].as_str().unwrap_or_default().to_owned(),
        _ if !capability_record.target_fields.is_empty() => capability_record
            .target_fields
            .iter()
            .map(|field| result[*field].as_str().unwrap_or_default())
            .collect::<Vec<_>>()
            .join("#"),
        _ => String::new(),
    };
    let request_target_matches = if operation == "play.screenshot" {
        let session = args["sessionId"].as_str().unwrap_or_default();
        let requested_name = args["path"].as_str().unwrap_or("");
        let name = if requested_name.is_empty() {
            format!("{session}.png")
        } else {
            requested_name.to_owned()
        };
        let clean = !name.contains(['/', '\\'])
            && Path::new(&name)
                .file_name()
                .and_then(|value| value.to_str())
                == Some(name.as_str());
        let valid_name = clean && name.ends_with(".png") && !name.chars().any(char::is_control);
        let expected = Path::new(&discovery.project_path)
            .parent()
            .map(|parent| parent.join("Saved/MagiUnrealAXI/Screenshots").join(&name))
            .and_then(|path| path.to_str().map(str::to_owned));
        valid_name
            && result["sessionId"].as_str() == Some(session)
            && expected.as_deref() == result["path"].as_str()
            && target == result_target
            && receipt.target == result_target
            && verification.get("target").and_then(Value::as_str) == Some(result_target.as_str())
    } else {
        capability_record.target_fields.iter().all(|field| {
            args.get(*field)
                .is_none_or(|request_value| result.get(*field) == Some(request_value))
        })
    };
    let p11_request_matches = match operation {
        "navigation.bounds_ensure" => {
            result.get("levelId") == args.get("levelId")
                && result.get("agentKey") == args.get("agentKey")
                && result.get("location") == args.get("location")
                && result.get("extent") == args.get("extent")
                && verification.get("requestLevelId") == args.get("levelId")
                && verification.get("requestAgentKey") == args.get("agentKey")
                && verification.get("requestLocation") == args.get("location")
                && verification.get("requestExtent") == args.get("extent")
        }
        "navigation.build" => {
            result.get("levelId") == args.get("levelId")
                && verification.get("requestLevelId") == args.get("levelId")
        }
        "blackboard.create" => {
            result
                .get("blackboardId")
                .and_then(Value::as_str)
                .is_some_and(|id| {
                    args["path"].as_str().is_some_and(|path| {
                        path.rsplit('/')
                            .next()
                            .is_some_and(|name| id == format!("{path}.{name}"))
                    })
                })
                && verification.get("requestPath") == args.get("path")
        }
        "behavior_tree.create" => {
            result
                .get("behaviorTreeId")
                .and_then(Value::as_str)
                .is_some_and(|id| {
                    args["path"].as_str().is_some_and(|path| {
                        path.rsplit('/')
                            .next()
                            .is_some_and(|name| id == format!("{path}.{name}"))
                    })
                })
                && verification.get("requestPath") == args.get("path")
                && verification.get("requestBlackboardId") == args.get("blackboardId")
        }
        "blackboard.key_ensure" => {
            result.get("blackboardId") == args.get("blackboardId")
                && result.get("keyName") == args.get("keyName")
                && result.get("keyType") == args.get("keyType")
                && verification.get("requestBlackboardId") == args.get("blackboardId")
                && verification.get("requestKeyName") == args.get("keyName")
                && verification.get("requestKeyType") == args.get("keyType")
        }
        "behavior_tree.node_ensure" => {
            result.get("behaviorTreeId") == args.get("behaviorTreeId")
                && result.get("nodeId") == args.get("nodeId")
                && result.get("nodeType") == args.get("nodeType")
                && verification.get("requestBehaviorTreeId") == args.get("behaviorTreeId")
                && verification.get("requestNodeId") == args.get("nodeId")
                && verification.get("requestNodeType") == args.get("nodeType")
        }
        "behavior_tree.connect" => {
            result.get("behaviorTreeId") == args.get("behaviorTreeId")
                && result.get("parentNodeId") == args.get("parentNodeId")
                && result.get("childNodeId") == args.get("childNodeId")
                && result.get("childIndex") == args.get("childIndex")
                && result.get("linkId").and_then(Value::as_str)
                    == Some(&format!(
                        "{}->{}",
                        args["parentNodeId"].as_str().unwrap_or_default(),
                        args["childNodeId"].as_str().unwrap_or_default()
                    ))
                && verification.get("requestBehaviorTreeId") == args.get("behaviorTreeId")
                && verification.get("requestParentNodeId") == args.get("parentNodeId")
                && verification.get("requestChildNodeId") == args.get("childNodeId")
                && verification.get("requestChildIndex") == args.get("childIndex")
        }
        "ai.controller_configure" => {
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("behaviorTreeId") == args.get("behaviorTreeId")
                && result.get("semantic")
                    == Some(&Value::String("on_possess.run_behavior_tree".into()))
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestBehaviorTreeId") == args.get("behaviorTreeId")
        }
        "ai.pawn_configure" => {
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("controllerBlueprintId") == args.get("controllerBlueprintId")
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestControllerBlueprintId")
                    == args.get("controllerBlueprintId")
        }
        "play.ai_target_set" => {
            result.get("sessionId") == args.get("sessionId")
                && result.get("pawnId") == args.get("pawnId")
                && result.get("keyName") == args.get("keyName")
                && result.get("targetActorId") == args.get("targetActorId")
                && verification.get("requestSessionId") == args.get("sessionId")
                && verification.get("requestPawnId") == args.get("pawnId")
                && verification.get("requestKeyName") == args.get("keyName")
                && verification.get("requestTargetActorId") == args.get("targetActorId")
        }
        "blueprint.interface_ensure" => {
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("interfaceId") == args.get("interfaceId")
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestInterfaceId") == args.get("interfaceId")
        }
        "blueprint.scs_component_update" | "blueprint.scs_component_remove" => {
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("variableGuid") == args.get("variableGuid")
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestVariableGuid") == args.get("variableGuid")
                && (operation != "blueprint.scs_component_remove"
                    || (verification.get("requestForce") == args.get("force")
                        && verification.get("requestDryRun") == args.get("dryRun")))
        }
        "blueprint.create" => {
            let path = args.get("path").and_then(Value::as_str);
            let expected_id = path.and_then(|path| {
                path.rsplit_once('/')
                    .map(|(_, name)| format!("{path}.{name}"))
            });
            expected_id.as_deref() == result.get("blueprintId").and_then(Value::as_str)
                && verification.get("requestPath") == args.get("path")
                && verification.get("requestParentClass") == args.get("parentClass")
                && result.get("parentClass") == args.get("parentClass")
        }
        "blueprint.pin_default_set" => {
            let value = args.get("value").and_then(Value::as_object);
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("pinId") == args.get("pinId")
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestPinId") == args.get("pinId")
                && verification.get("requestValueType") == value.and_then(|v| v.get("type"))
                && verification.get("requestValue").and_then(Value::as_f64)
                    == value.and_then(|v| v.get("value")).and_then(Value::as_f64)
        }
        "blueprint.pin_connect" => {
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("sourcePinId") == args.get("sourcePinId")
                && result.get("targetPinId") == args.get("targetPinId")
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestSourcePinId") == args.get("sourcePinId")
                && verification.get("requestTargetPinId") == args.get("targetPinId")
        }
        "blueprint.interface_create" => {
            result.get("id").and_then(Value::as_str)
                == args
                    .get("path")
                    .and_then(Value::as_str)
                    .and_then(|path| {
                        path.rsplit_once('/')
                            .map(|(_, name)| format!("{path}.{name}"))
                    })
                    .as_deref()
                && verification.get("requestPath") == args.get("path")
                && verification.get("requestFunction") == args.get("function")
        }
        _ => true,
    };
    let p13_request_matches = match operation {
        "widget.create" => {
            let path = args.get("path").and_then(Value::as_str);
            let blueprint = path.and_then(|path| {
                path.rsplit_once('/')
                    .map(|(_, name)| format!("{path}.{name}"))
            });
            result.get("blueprintId").and_then(Value::as_str) == blueprint.as_deref()
                && result.get("rootName") == args.get("rootName")
                && result.get("rootClass") == args.get("rootClass")
                && verification.get("requestPath") == args.get("path")
                && verification.get("requestRootName") == args.get("rootName")
                && verification.get("requestRootClass") == args.get("rootClass")
        }
        "widget.child_ensure" => {
            let blueprint = args
                .get("blueprintId")
                .and_then(Value::as_str)
                .unwrap_or_default();
            let name = args.get("name").and_then(Value::as_str).unwrap_or_default();
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("widgetId").and_then(Value::as_str)
                    == Some(&format!("{blueprint}#widget:{name}"))
                && result.get("parentWidgetId") == args.get("parentWidgetId")
                && result.get("name") == args.get("name")
                && result.get("class") == args.get("class")
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestParentWidgetId") == args.get("parentWidgetId")
                && verification.get("requestName") == args.get("name")
                && verification.get("requestClass") == args.get("class")
        }
        "widget.property_set" => {
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("widgetId") == args.get("widgetId")
                && result.get("property") == args.get("property")
                && args
                    .get("property")
                    .and_then(Value::as_str)
                    .is_some_and(|property| result.get(property) == args.get(property))
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestWidgetId") == args.get("widgetId")
                && verification.get("requestProperty") == args.get("property")
                && verification.get("requestText") == args.get("text")
                && verification.get("requestVisibility") == args.get("visibility")
                && verification.get("requestEnabled") == args.get("enabled")
        }
        "widget.event_ensure" => {
            let blueprint = args
                .get("blueprintId")
                .and_then(Value::as_str)
                .unwrap_or_default();
            let agent = args
                .get("agentKey")
                .and_then(Value::as_str)
                .unwrap_or_default();
            result.get("blueprintId") == args.get("blueprintId")
                && result.get("eventId").and_then(Value::as_str)
                    == Some(&format!("{blueprint}#event:{agent}"))
                && result.get("agentKey") == args.get("agentKey")
                && result.get("event") == args.get("event")
                && result.get("actions") == args.get("actions")
                && verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestAgentKey") == args.get("agentKey")
                && verification.get("requestIntent") == args.get("event")
                && verification.get("requestActions") == args.get("actions")
        }
        "widget.viewport_ensure" => {
            let host = args
                .get("hostBlueprintId")
                .and_then(Value::as_str)
                .unwrap_or_default();
            let agent = args
                .get("agentKey")
                .and_then(Value::as_str)
                .unwrap_or_default();
            result.get("hostBlueprintId") == args.get("hostBlueprintId")
                && result.get("widgetBlueprintId") == args.get("widgetBlueprintId")
                && result.get("viewportId").and_then(Value::as_str)
                    == Some(&format!("{host}#viewport:{agent}"))
                && result
                    .get("graphId")
                    .and_then(Value::as_str)
                    .is_some_and(|value| !value.is_empty())
                && result.get("inputKey") == args.get("inputKey")
                && result.get("zOrder") == args.get("zOrder")
                && result
                    .get("widgetRevision")
                    .and_then(Value::as_str)
                    .is_some_and(capability::canonical_revision)
                && verification.get("requestHostBlueprintId") == args.get("hostBlueprintId")
                && verification.get("requestWidgetBlueprintId") == args.get("widgetBlueprintId")
                && verification.get("requestAgentKey") == args.get("agentKey")
                && verification.get("requestInputKey") == args.get("inputKey")
                && verification.get("requestZOrder") == args.get("zOrder")
        }
        _ => true,
    };
    let p12_update_fields_match = operation != "blueprint.scs_component_update"
        || [
            ("location", "requestLocation"),
            ("rotation", "requestRotation"),
            ("scale", "requestScale"),
            ("collisionEnabled", "requestCollisionEnabled"),
            ("collisionProfile", "requestCollisionProfile"),
            ("generateOverlapEvents", "requestGenerateOverlapEvents"),
            ("simulatePhysics", "requestSimulatePhysics"),
            ("gravityEnabled", "requestGravityEnabled"),
            ("massOverride", "requestMassOverride"),
            ("boxExtent", "requestBoxExtent"),
            ("sphereRadius", "requestSphereRadius"),
        ]
        .into_iter()
        .all(|(argument, request)| {
            args.get(argument)
                .is_none_or(|value| verification.get(request) == Some(value))
        });
    let legacy_request_matches = match operation {
        "blueprint.node_ensure" | "blueprint.event_ensure" => {
            verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestGraphId") == args.get("graphId")
                && verification.get("requestAgentKey") == args.get("agentKey")
                && verification.get("requestIntent")
                    == args.get(if operation == "blueprint.node_ensure" {
                        "node"
                    } else {
                        "event"
                    })
                && verification.get("requestVariableGuid") == args.get("variableGuid")
                && verification.get("requestInterfaceId") == args.get("interfaceId")
        }
        "blueprint.scs_component_remove" => {
            verification.get("requestBlueprintId") == args.get("blueprintId")
                && verification.get("requestVariableGuid") == args.get("variableGuid")
                && verification.get("requestForce") == args.get("force")
                && verification.get("requestDryRun") == args.get("dryRun")
        }
        _ => true,
    };
    let result_changed = result["changed"]
        .as_bool()
        .ok_or_else(|| outcome_unknown(id))?;
    let result_revision = result["revision"]
        .as_str()
        .ok_or_else(|| outcome_unknown(id))?;
    if !capability::canonical_revision(result_revision) {
        return Err(outcome_unknown(id));
    }

    let result_dirty = result["dirtyPackages"]
        .as_array()
        .map(|v| {
            v.iter()
                .filter_map(Value::as_str)
                .map(str::to_owned)
                .collect::<Vec<_>>()
        })
        .unwrap_or_default();
    let result_saved = result["savedPackages"]
        .as_array()
        .map(|v| {
            v.iter()
                .filter_map(Value::as_str)
                .map(str::to_owned)
                .collect::<Vec<_>>()
        })
        .unwrap_or_default();
    let catalog_transaction = capability_record.transaction_behavior;
    let expected_transaction = if result_changed {
        catalog_transaction
    } else {
        "none"
    };
    let expected_reversibility = capability_record.reversibility;
    let expected_persistence = if operation == "play.screenshot" || !result_saved.is_empty() {
        "saved"
    } else if !result_dirty.is_empty() {
        "dirty"
    } else {
        "unchanged"
    };
    if receipt.operation_id != id
        || receipt.operation != operation
        || receipt.state != "completed"
        || receipt.project_id != discovery.project_id
        || receipt.editor_pid != discovery.pid
        || target != result_target
        || receipt.target != target
        || target.is_empty()
        || !request_target_matches
        || !p11_request_matches
        || !legacy_request_matches
        || !p12_update_fields_match
        || !p13_request_matches
        || target.chars().count() > 8192
        || !matched
        || receipt.changed != result_changed
        || receipt.revision != result_revision
        || receipt.dirty_packages != result_dirty
        || receipt.saved_packages != result_saved
        || receipt.transaction != expected_transaction
        || receipt.reversibility != expected_reversibility
        || receipt.persistence != expected_persistence
        || !capability::canonical_revision(&receipt.revision)
        || readback.is_empty()
        || readback.chars().count() > 128
        || readback != expected_readback
        || receipt.dirty_packages.len() > MAX_DIRTY_PACKAGES
        || receipt.saved_packages.len() > MAX_DIRTY_PACKAGES
        || receipt
            .dirty_packages
            .iter()
            .any(|p| p.chars().count() > MAX_DIRTY_PACKAGE_NAME)
        || receipt
            .saved_packages
            .iter()
            .any(|p| p.chars().count() > MAX_DIRTY_PACKAGE_NAME)
    {
        return Err(outcome_unknown(id));
    }
    let observed = verification.get("observedRevision").and_then(Value::as_str);
    if operation == "play.screenshot" || operation.starts_with("widget.") {
        if observed != Some(result_revision) {
            return Err(outcome_unknown(id));
        }
    } else if let Some(observed) = observed {
        if observed != result_revision {
            return Err(outcome_unknown(id));
        }
    } else {
        return Err(outcome_unknown(id));
    }
    if operation == "play.input" {
        let before = result["beforeRevision"]
            .as_str()
            .ok_or_else(|| outcome_unknown(id))?;
        let after = result["afterRevision"]
            .as_str()
            .ok_or_else(|| outcome_unknown(id))?;
        let observed = verification
            .get("observedRevision")
            .and_then(Value::as_str)
            .ok_or_else(|| outcome_unknown(id))?;
        if !capability::canonical_revision(before)
            || !capability::canonical_revision(after)
            || !capability::canonical_revision(observed)
            || !result["accepted"].as_bool().unwrap_or(false)
            || result_revision != after
            || observed != after
            || result_changed != (before != after)
            || (result["event"] != "pressed" && result["event"] != "released")
            || (result["event"] == "released" && before != after)
            || verification.get("accepted").and_then(Value::as_bool) != Some(true)
            || verification.get("beforeRevision").and_then(Value::as_str) != Some(before)
            || verification.get("afterRevision").and_then(Value::as_str) != Some(after)
            || readback != "play.observe"
            || target.split('#').count() != 3
        {
            return Err(outcome_unknown(id));
        }
    }
    if readback != expected_readback {
        return Err(outcome_unknown(id));
    }
    Ok(())
}
fn surface_journal_failure(mut error: AppError, receipt: &Value, journal: AppError) -> AppError {
    let operation_id = receipt["operationId"]
        .as_str()
        .unwrap_or("unknown")
        .to_owned();
    let detail = format!("journal persistence failed: {}", journal.message);
    if error.reason == "journal_write_failed" {
        return error
            .with_operation_id(operation_id)
            .with_receipt(receipt.clone());
    }
    error.message = format!("{}; {detail}", error.message);
    error.help = format!(
        "{}; do not retry; inspect operation view {} and recover journal persistence",
        error.help, operation_id
    );
    error
        .with_operation_id(operation_id)
        .with_receipt(receipt.clone())
}

fn journal_completed_failure(receipt: &Value, journal: AppError) -> AppError {
    let operation_id = receipt["operationId"]
        .as_str()
        .unwrap_or("unknown")
        .to_owned();
    AppError::operational(
        "bridge",
        "journal_write_failed",
        format!(
            "mutation completed but journal persistence failed: {}",
            journal.message
        ),
        format!(
            "do not retry; inspect operation view {operation_id} and recover journal persistence"
        ),
    )
    .with_operation_id(operation_id)
    .with_receipt(receipt.clone())
}

fn live(project: &Path, pid: Option<u32>) -> Result<(PathBuf, Discovery), AppError> {
    match select(project, pid)? {
        Selection::Live(session, discovery) => Ok((session, *discovery)),
        Selection::Stale => Err(bridge_error(
            "stale_editor",
            "only stale editor discovery exists",
        )),
        Selection::None => Err(bridge_error(
            "editor_not_found",
            "no matching editor discovery exists",
        )),
    }
}

fn exchange_selected(
    session: &Path,
    discovery: &Discovery,
    operation: &str,
    deadline: Instant,
) -> Result<Value, AppError> {
    exchange_selected_args(
        session,
        discovery,
        operation,
        json!({}),
        &ExecutionOptions::default(),
        deadline,
    )
}

pub fn capability(
    project: &Path,
    pid: Option<u32>,
    operation: &str,
    args: Value,
    options: ExecutionOptions,
    timeout: Duration,
) -> Result<Value, AppError> {
    let journal_id =
        (operation == "operation.view").then(|| args["id"].as_str().unwrap_or("").to_owned());
    let (session, discovery) = match live(project, pid) {
        Ok(value) => value,
        Err(error)
            if operation == "operation.view"
                && matches!(error.reason, "editor_not_found" | "stale_editor") =>
        {
            return journal_find(project, journal_id.as_deref().unwrap_or(""))?.ok_or(error);
        }
        Err(error) => return Err(error),
    };
    let result = exchange_selected_args(
        &session,
        &discovery,
        operation,
        args,
        &options,
        Instant::now() + timeout,
    );
    match result {
        Ok(value) => {
            if is_mutation(operation)
                && let Some(receipt) = value.get("receipt")
                && let Err(journal) = journal_receipt(project, receipt)
            {
                return Err(journal_completed_failure(receipt, journal));
            }
            Ok(value)
        }
        Err(error) => {
            if operation == "operation.view" && error.reason == "not_found" {
                return journal_find(project, journal_id.as_deref().unwrap_or(""))?.ok_or(error);
            }
            if let Some(receipt) = error.receipt().cloned()
                && let Err(journal) = journal_receipt(project, &receipt)
            {
                return Err(surface_journal_failure(error, &receipt, journal));
            }
            Err(error)
        }
    }
}

pub fn status(project: &Path, pid: Option<u32>, timeout: Duration) -> Result<Value, AppError> {
    match select(project, pid)? {
        Selection::None => Ok(json!({"editor":{"state":"stopped"}})),
        Selection::Stale => {
            Ok(json!({"editor":{"state":"stale"},"help":["magi-unreal-axi editor start"]}))
        }
        Selection::Live(session, discovery) => {
            let status = exchange_selected(
                &session,
                &discovery,
                "editor.status",
                Instant::now() + timeout,
            )?;
            let status = capability::validate_output("editor.status", status)?;

            let state = status
                .get("state")
                .and_then(Value::as_str)
                .expect("validated editor status state");
            if !matches!(state, "starting" | "ready" | "stopping") {
                return Err(bridge_error(
                    "invalid_health",
                    "editor status reported unknown lifecycle state",
                ));
            }
            Ok(
                json!({"editor":{"state":state,"pid":discovery.pid,"processStart":discovery.process_start,"projectPath":discovery.project_path,"projectId":discovery.project_id,"pluginVersion":discovery.plugin_version,"engineVersion":discovery.engine_version,"ownership":if owner_matches(&session,&discovery){"launched"}else{"attached"},"level":status["levelId"],"pie":status["pie"],"dirtyPackages":status["dirtyPackageCount"]},"health":status}),
            )
        }
    }
}

pub fn runtime_availability(
    project: &Path,
    pid: Option<u32>,
    timeout: Duration,
) -> Result<Option<Value>, AppError> {
    let (session, discovery) = match select(project, pid)? {
        Selection::None | Selection::Stale => return Ok(None),
        Selection::Live(session, discovery) => (session, discovery),
    };
    let description = exchange_selected(
        &session,
        &discovery,
        "bridge.describe",
        Instant::now() + timeout,
    )?;
    let entries = description
        .get("nativeOperations")
        .and_then(Value::as_array)
        .filter(|entries| entries.len() <= capability::CATALOG_COUNT)
        .ok_or_else(|| {
            bridge_error(
                "malformed_availability",
                "bridge.describe lacks bounded native availability",
            )
        })?;
    let mut availability = serde_json::Map::new();
    for entry in entries {
        let operation = entry
            .get("operation")
            .and_then(Value::as_str)
            .filter(|operation| {
                capability::capability_metadata(operation)
                    .is_some_and(|metadata| metadata.execution == "native")
            })
            .ok_or_else(|| {
                bridge_error(
                    "malformed_availability",
                    "native availability operation is invalid",
                )
            })?;
        let state = entry
            .get("availability")
            .and_then(Value::as_str)
            .filter(|state| matches!(*state, "available" | "unavailable"))
            .ok_or_else(|| {
                bridge_error(
                    "malformed_availability",
                    "native availability state is invalid",
                )
            })?;
        let reasons = entry
            .get("reasons")
            .and_then(Value::as_array)
            .filter(|reasons| reasons.len() <= 16)
            .ok_or_else(|| {
                bridge_error(
                    "malformed_availability",
                    "native availability reasons are invalid",
                )
            })?;
        if (state == "available") != reasons.is_empty() {
            return Err(bridge_error(
                "malformed_availability",
                "native availability reasons do not match state",
            ));
        }
        for reason in reasons {
            for (field, maximum, allow_empty) in [
                ("code", 32, false),
                ("subject", 128, true),
                ("message", 512, false),
            ] {
                reason
                    .get(field)
                    .and_then(Value::as_str)
                    .filter(|text| {
                        text.chars().count() <= maximum && (allow_empty || !text.is_empty())
                    })
                    .ok_or_else(|| {
                        bridge_error(
                            "malformed_availability",
                            "availability reason field is invalid",
                        )
                    })?;
            }
        }
        if availability
            .insert(
                operation.to_owned(),
                json!({"availability":state,"reasons":reasons}),
            )
            .is_some()
        {
            return Err(bridge_error(
                "malformed_availability",
                "native availability operation is duplicated",
            ));
        }
    }
    let expected = capability::CAPABILITY_METADATA
        .iter()
        .filter(|metadata| metadata.execution == "native")
        .count();
    if availability.len() != expected {
        return Err(bridge_error(
            "malformed_availability",
            "native availability coverage is incomplete",
        ));
    }
    Ok(Some(Value::Object(availability)))
}

pub fn describe(project: &Path, pid: Option<u32>, timeout: Duration) -> Result<Value, AppError> {
    let (session, discovery) = live(project, pid)?;

    exchange_selected(
        &session,
        &discovery,
        "bridge.describe",
        Instant::now() + timeout,
    )
}

pub fn stop(project: &Path, pid: Option<u32>, timeout: Duration) -> Result<Value, AppError> {
    let deadline = Instant::now() + timeout;
    let selection = select(project, pid)?;
    let (session, discovery) = match selection {
        Selection::None | Selection::Stale => {
            return Ok(json!({"editor":{"state":"stopped","changed":false}}));
        }
        Selection::Live(session, discovery) => (session, discovery),
    };
    if !owner_matches(&session, &discovery) {
        return Err(AppError::operational(
            "editor_state",
            "external_editor_refused",
            format!("editor {} was not launched by this CLI", discovery.pid),
            "stop the editor manually or launch it with magi-unreal-axi",
        ));
    }
    let result = exchange_selected(&session, &discovery, "editor.stop", deadline)?;
    let operation_id = result
        .get("operationId")
        .and_then(Value::as_str)
        .expect("validated stop result includes operationId")
        .to_owned();
    loop {
        if !process_matches(&discovery) && !session.join("bridge-v1.json").exists() {
            return Ok(
                json!({"editor":{"state":"stopped","pid":discovery.pid,"changed":true,"ownership":"launched"},"result":result}),
            );
        }
        let sleep = remaining(deadline)
            .map_err(|_| outcome_unknown(&operation_id))?
            .min(Duration::from_millis(100));
        thread::sleep(sleep);
    }
}

pub fn start(
    project: &Path,
    engine: &EngineInfo,
    selected_pid: Option<u32>,
    timeout: Duration,
) -> Result<Value, AppError> {
    let deadline = Instant::now() + timeout;
    match select(project, selected_pid)? {
        Selection::Live(session, discovery) => loop {
            let health = exchange_selected(&session, &discovery, "bridge.health", deadline)?;
            if health.get("state").and_then(Value::as_str) == Some("ready") {
                return Ok(
                    json!({"editor":{"state":"ready","pid":discovery.pid,"changed":false,"ownership":if owner_matches(&session,&discovery){"launched"}else{"attached"}},"health":health}),
                );
            }
            if health.get("state").and_then(Value::as_str) == Some("stopping") {
                return Err(bridge_error(
                    "unsafe_editor_state",
                    "selected editor is stopping",
                ));
            }
            let sleep = remaining(deadline)?.min(Duration::from_millis(100));
            thread::sleep(sleep);
        },
        Selection::Stale if selected_pid.is_some() => {
            return Err(bridge_error(
                "stale_editor",
                "selected editor has only stale discovery",
            ));
        }
        Selection::None if selected_pid.is_some() => {
            return Err(bridge_error(
                "editor_not_found",
                "selected editor does not exist",
            ));
        }
        Selection::Stale | Selection::None => {}
    }
    let log_directory = runtime_root()?.join("editor");
    secure_directory(&runtime_root()?)?;
    secure_directory(&log_directory)?;
    let log_path = log_directory.join(format!(
        "{}-{}.log",
        std::process::id(),
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_millis()
    ));
    let log = OpenOptions::new()
        .create_new(true)
        .write(true)
        .mode(0o600)
        .open(&log_path)
        .map_err(io_error)?;
    let mut command = Command::new(&engine.editor);
    command
        .args([
            project.as_os_str(),
            "-unattended".as_ref(),
            "-nop4".as_ref(),
            "-nosplash".as_ref(),
        ])
        .current_dir(project.parent().unwrap())
        .stdin(Stdio::null())
        .stdout(Stdio::from(log.try_clone().map_err(io_error)?))
        .stderr(Stdio::from(log));
    #[cfg(unix)]
    command.process_group(0);
    let mut child = command.spawn().map_err(io_error)?;
    let mut owner_written = false;

    while remaining(deadline).is_ok() {
        if let Some(status) = child.try_wait().map_err(io_error)? {
            return Err(bridge_error(
                "editor_start_failed",
                format!("editor exited with {status}; log: {}", log_path.display()),
            ));
        }
        match select(project, Some(child.id())) {
            Ok(Selection::Live(session, discovery)) => {
                let health =
                    match exchange_selected(&session, &discovery, "bridge.health", deadline) {
                        Ok(health) => health,
                        Err(error) => {
                            terminate_child(&mut child);
                            return Err(error);
                        }
                    };
                if !owner_written {
                    if let Err(error) = write_owner(&session, &discovery) {
                        terminate_child(&mut child);
                        return Err(error);
                    }
                    owner_written = true;
                }
                if health.get("state").and_then(Value::as_str) == Some("ready") {
                    return Ok(
                        json!({"editor":{"state":"ready","pid":discovery.pid,"changed":true,"ownership":"launched","logPath":log_path},"health":health}),
                    );
                }
            }
            Ok(Selection::None | Selection::Stale) => {}
            Err(error) => {
                terminate_child(&mut child);
                return Err(error);
            }
        }
        thread::sleep(remaining(deadline)?.min(Duration::from_millis(200)));
    }
    terminate_child(&mut child);
    Err(bridge_error(
        "editor_start_timeout",
        format!(
            "editor {} did not become ready; log: {}",
            child.id(),
            log_path.display()
        ),
    ))
}

fn owner_matches(session: &Path, discovery: &Discovery) -> bool {
    let path = session.join("owner-v1.json");
    validate_private(&path, false).is_ok()
        && read_bounded(&path, MAX_DISCOVERY)
            .ok()
            .and_then(|bytes| serde_json::from_slice::<Owner>(&bytes).ok())
            .is_some_and(|owner| {
                owner.pid == discovery.pid
                    && owner.project_id == discovery.project_id
                    && owner.session_nonce == discovery.session_nonce
                    && owner.process_start == discovery.process_start
            })
}

fn write_owner(session: &Path, discovery: &Discovery) -> Result<(), AppError> {
    let owner = Owner {
        pid: discovery.pid,
        process_start: discovery.process_start.clone(),
        project_id: discovery.project_id.clone(),
        session_nonce: discovery.session_nonce.clone(),
        executable: std::env::current_exe()
            .map_err(io_error)?
            .display()
            .to_string(),
    };
    atomic_private_write(
        &session.join("owner-v1.json"),
        &serde_json::to_vec(&owner)
            .map_err(|error| bridge_error("owner_write_failed", error.to_string()))?,
    )
}

#[cfg(unix)]
fn secure_directory(path: &Path) -> Result<(), AppError> {
    fs::create_dir_all(path).map_err(io_error)?;
    fs::set_permissions(path, fs::Permissions::from_mode(0o700)).map_err(io_error)?;
    validate_private(path, true)
}

#[cfg(unix)]
fn atomic_private_write(path: &Path, bytes: &[u8]) -> Result<(), AppError> {
    let temporary = path.with_extension(format!(
        "{}.{}.tmp",
        std::process::id(),
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos()
    ));
    let mut file = OpenOptions::new()
        .create_new(true)
        .write(true)
        .mode(0o600)
        .open(&temporary)
        .map_err(io_error)?;
    file.write_all(bytes)
        .and_then(|_| file.sync_all())
        .map_err(io_error)?;
    fs::rename(&temporary, path).map_err(io_error)?;
    validate_private(path, false)
}
#[cfg(unix)]
fn lock_journal(directory: &Path) -> Result<File, AppError> {
    let path = directory.join("operation-journal-v1.lock");
    let file = OpenOptions::new()
        .create(true)
        .read(true)
        .write(true)
        .mode(0o600)
        .custom_flags(libc::O_NOFOLLOW | libc::O_CLOEXEC)
        .open(&path)
        .map_err(io_error)?;
    validate_private(&path, false)?;
    if unsafe { libc::flock(file.as_raw_fd(), libc::LOCK_EX) } != 0 {
        return Err(io_error(std::io::Error::last_os_error()));
    }
    Ok(file)
}

fn journal_receipt(project: &Path, receipt: &Value) -> Result<(), AppError> {
    let directory = project_directory(project)?;
    secure_directory(&runtime_root()?)?;
    secure_directory(&directory)?;
    journal_receipt_in_directory(&directory, receipt)
}

fn journal_receipt_in_directory(directory: &Path, receipt: &Value) -> Result<(), AppError> {
    let _lock = lock_journal(directory)?;
    let path = directory.join("operation-journal-v1.json");
    let mut records = if path.exists() {
        serde_json::from_slice::<Vec<Value>>(&read_bounded(&path, MAX_RESPONSE as u64)?)
            .map_err(|_| bridge_error("malformed_journal", "operation journal is malformed"))?
    } else {
        Vec::new()
    };
    records.retain(|entry| {
        entry
            .get("recordedAt")
            .and_then(Value::as_u64)
            .is_some_and(|time| now_secs().saturating_sub(time) <= 86_400)
    });
    records.push(json!({"recordedAt": now_secs(), "receipt": receipt}));
    if records.len() > 1024 {
        records.drain(..records.len() - 1024);
    }
    atomic_private_write(
        &path,
        &serde_json::to_vec(&records)
            .map_err(|error| bridge_error("journal_write_failed", error.to_string()))?,
    )
}
fn now_secs() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
fn journal_find(project: &Path, id: &str) -> Result<Option<Value>, AppError> {
    let path = project_directory(project)?.join("operation-journal-v1.json");
    if !path.exists() {
        return Ok(None);
    }
    let records: Vec<Value> = serde_json::from_slice(&read_bounded(&path, MAX_RESPONSE as u64)?)
        .map_err(|_| bridge_error("malformed_journal", "operation journal is malformed"))?;
    Ok(records.into_iter().rev().find_map(|entry| {
        (entry["receipt"]["operationId"] == id).then(|| entry["receipt"].clone())
    }))
}

#[cfg(unix)]
fn terminate_child(child: &mut Child) {
    signal_group(child.id(), "-TERM");
    thread::sleep(Duration::from_millis(250));
    signal_group(child.id(), "-KILL");
    let _ = child.kill();
    let _ = child.wait();
}

#[cfg(unix)]
fn signal_group(pid: u32, signal: &str) {
    let _ = Command::new("/bin/kill")
        .args([signal, &format!("-{pid}")])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status();
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::TcpListener;

    fn fake_discovery(port: u16) -> Discovery {
        Discovery {
            protocol: PROTOCOL,
            plugin_version: VERSION.to_owned(),
            pid: std::process::id(),
            process_start: process_start_identity(std::process::id()).unwrap(),
            project_path: "/fixture/Game.uproject".to_owned(),
            project_id: "sha256:fixture".to_owned(),
            engine_version: "5.8.1".to_owned(),
            host: "127.0.0.1".to_owned(),
            port,
            session_nonce: "0123456789abcdef0123456789abcdef".to_owned(),
            started_at: "fixture".to_owned(),
        }
    }

    fn private_session() -> tempfile::TempDir {
        let session = tempfile::tempdir().unwrap();
        fs::set_permissions(session.path(), fs::Permissions::from_mode(0o700)).unwrap();
        let token_path = session.path().join("token");
        fs::write(&token_path, "a".repeat(64)).unwrap();
        fs::set_permissions(&token_path, fs::Permissions::from_mode(0o600)).unwrap();
        session
    }
    #[test]
    fn deadline_expires_without_waiting_past_boundary() {
        assert!(remaining(Instant::now() - Duration::from_millis(1)).is_err());
    }
    #[test]
    fn process_identity_detects_reused_pid_metadata() {
        let mut discovery = fake_discovery(1);
        assert!(process_matches(&discovery));
        discovery.process_start.push_str("-different");
        assert!(!process_matches(&discovery));
    }

    #[test]
    fn handshake_process_identity_mismatch_fails_closed() {
        let listener = TcpListener::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
        let discovery = fake_discovery(listener.local_addr().unwrap().port());
        let expected_nonce = discovery.session_nonce.clone();
        let expected_pid = discovery.pid;
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let _: Value = serde_json::from_slice(
                &read_frame(
                    &mut stream,
                    MAX_REQUEST,
                    Instant::now() + Duration::from_secs(1),
                )
                .unwrap(),
            )
            .unwrap();
            write_frame(
                &mut stream,
                &json!({"protocol":PROTOCOL,"status":"ok","pluginVersion":VERSION,"pid":expected_pid,"processStart":"different","sessionNonce":expected_nonce,"catalogHash":CATALOG_HASH}),
                Instant::now() + Duration::from_secs(1),
            )
            .unwrap();
        });
        let session = private_session();
        let error = exchange_selected(
            session.path(),
            &discovery,
            "bridge.health",
            Instant::now() + Duration::from_secs(1),
        )
        .unwrap_err();
        assert_eq!(error.reason, "handshake_rejected");
        assert!(!error.message.contains(&"a".repeat(64)));
        worker.join().unwrap();
    }

    #[test]
    fn operation_response_id_mismatch_fails_closed() {
        let listener = TcpListener::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
        let discovery = fake_discovery(listener.local_addr().unwrap().port());
        let server_identity = discovery.clone();
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let _: Value = serde_json::from_slice(
                &read_frame(
                    &mut stream,
                    MAX_REQUEST,
                    Instant::now() + Duration::from_secs(1),
                )
                .unwrap(),
            )
            .unwrap();
            write_frame(
                &mut stream,
                &json!({"protocol":PROTOCOL,"status":"ok","pluginVersion":VERSION,"pid":server_identity.pid,"processStart":server_identity.process_start,"sessionNonce":server_identity.session_nonce,"catalogHash":CATALOG_HASH}),
                Instant::now() + Duration::from_secs(1),
            )
            .unwrap();
            let _: Value = serde_json::from_slice(
                &read_frame(
                    &mut stream,
                    MAX_REQUEST,
                    Instant::now() + Duration::from_secs(1),
                )
                .unwrap(),
            )
            .unwrap();
            write_frame(
                &mut stream,
                &json!({"protocol":PROTOCOL,"id":"wrong","status":"ok","result":{"state":"ready"}}),
                Instant::now() + Duration::from_secs(1),
            )
            .unwrap();
        });
        let session = private_session();
        let error = exchange_selected(
            session.path(),
            &discovery,
            "bridge.health",
            Instant::now() + Duration::from_secs(1),
        )
        .unwrap_err();
        assert_eq!(error.reason, "mismatched_response");
        worker.join().unwrap();
    }

    #[test]
    fn trickle_frame_cannot_extend_absolute_deadline() {
        let listener = TcpListener::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
        let address = listener.local_addr().unwrap();
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            for byte in [4_u8, 0, 0, 0, b'x', b'y', b'z', b'!'] {
                stream.write_all(&[byte]).unwrap();
                thread::sleep(Duration::from_millis(20));
            }
        });
        let mut stream = TcpStream::connect(address).unwrap();
        let started = Instant::now();
        let error = read_frame(
            &mut stream,
            MAX_RESPONSE,
            started + Duration::from_millis(55),
        )
        .unwrap_err();
        assert_eq!(error.reason, "bridge_deadline_exceeded");
        assert!(started.elapsed() < Duration::from_millis(150));
        worker.join().unwrap();
    }

    #[test]
    fn transmitted_stop_without_response_reports_outcome_unknown_id() {
        let listener = TcpListener::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
        let discovery = fake_discovery(listener.local_addr().unwrap().port());
        let server_identity = discovery.clone();
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let _ = read_frame(
                &mut stream,
                MAX_REQUEST,
                Instant::now() + Duration::from_secs(1),
            )
            .unwrap();
            write_frame(&mut stream, &json!({"protocol":PROTOCOL,"status":"ok","pluginVersion":VERSION,"pid":server_identity.pid,"processStart":server_identity.process_start,"sessionNonce":server_identity.session_nonce,"catalogHash":CATALOG_HASH}), Instant::now() + Duration::from_secs(1)).unwrap();
            let _ = read_frame(
                &mut stream,
                MAX_REQUEST,
                Instant::now() + Duration::from_secs(1),
            )
            .unwrap();
        });
        let session = private_session();
        let error = exchange_selected(
            session.path(),
            &discovery,
            "editor.stop",
            Instant::now() + Duration::from_secs(1),
        )
        .unwrap_err();
        assert_eq!(error.reason, "outcome_unknown");
        assert!(error.operation_id().is_some_and(|id| !id.is_empty()));
        worker.join().unwrap();
    }

    #[test]
    fn validate_p12_receipts_bind_blueprint_requests() {
        let discovery = fake_discovery(0);
        let revision = "a".repeat(64);
        let guid = "11111111-2222-3333-4444-555555555555";
        let result = json!({"blueprintId":"/Game/BP.BP","variableGuid":guid,"changed":false,"dryRun":true,"dirtyPackages":[],"revision":revision});
        let args =
            json!({"blueprintId":"/Game/BP.BP","variableGuid":guid,"force":false,"dryRun":true});
        let request = json!({"requestBlueprintId":"/Game/BP.BP","requestVariableGuid":guid,"requestForce":false,"requestDryRun":true});
        let mut receipt = receipt_fixture("blueprint.scs_component_remove", result.clone(), true);
        receipt.target = format!("/Game/BP.BP#scs:{guid}");
        receipt.verification["target"] = json!(receipt.target);
        receipt
            .verification
            .as_object_mut()
            .unwrap()
            .extend(request.as_object().unwrap().clone());
        assert!(
            validate_receipt(
                "blueprint.scs_component_remove",
                "id",
                &receipt,
                &result,
                &args,
                &discovery
            )
            .is_ok()
        );
        receipt.verification["requestBlueprintId"] = json!("wrong");
        assert!(
            validate_receipt(
                "blueprint.scs_component_remove",
                "id",
                &receipt,
                &result,
                &args,
                &discovery
            )
            .is_err()
        );
        let root_result = json!({"blueprintId":"/Game/BP.BP","variableGuid":guid,"changed":true,"dirtyPackages":["/Game/BP"],"revision":revision});
        let root_args = json!({"blueprintId":"/Game/BP.BP","name":"Root","class":"SceneComponent"});
        let mut root_receipt =
            receipt_fixture("blueprint.scs_component_ensure", root_result.clone(), true);
        root_receipt.target = "/Game/BP.BP#scs-name:Root".into();
        root_receipt.verification["target"] = json!(root_receipt.target);
        root_receipt.verification["requestBlueprintId"] = json!("/Game/BP.BP");
        root_receipt.verification["requestName"] = json!("Root");
        root_receipt.verification["requestClass"] = json!("SceneComponent");
        root_receipt.verification["requestParent"] = Value::Null;
        assert!(
            validate_receipt(
                "blueprint.scs_component_ensure",
                "id",
                &root_receipt,
                &root_result,
                &root_args,
                &discovery
            )
            .is_ok()
        );
    }

    #[test]
    fn validate_p13_receipts_bind_widget_semantics() {
        let revision = "a".repeat(64);
        let blueprint = "/Game/UI.UI";
        let child_args = json!({"blueprintId":blueprint,"parentWidgetId":format!("{blueprint}#widget:Root"),"name":"Label","class":"TextBlock"});
        let child_result = json!({"blueprintId":blueprint,"widgetId":format!("{blueprint}#widget:Label"),"parentWidgetId":child_args["parentWidgetId"],"name":"Label","class":"TextBlock","changed":true,"dirtyPackages":["/Game/UI"],"savedPackages":[],"revision":revision});
        let mut child_receipt = receipt_fixture("widget.child_ensure", child_result.clone(), true);
        child_receipt.verification["requestBlueprintId"] = child_args["blueprintId"].clone();
        child_receipt.verification["requestParentWidgetId"] = child_args["parentWidgetId"].clone();
        child_receipt.verification["requestName"] = child_args["name"].clone();
        child_receipt.verification["requestClass"] = child_args["class"].clone();
        assert!(
            validate_receipt(
                "widget.child_ensure",
                "id",
                &child_receipt,
                &child_result,
                &child_args,
                &fake_discovery(0)
            )
            .is_ok()
        );
        let mut missing_binding = child_receipt.clone();
        missing_binding
            .verification
            .as_object_mut()
            .unwrap()
            .remove("requestName");
        assert!(
            validate_receipt(
                "widget.child_ensure",
                "id",
                &missing_binding,
                &child_result,
                &child_args,
                &fake_discovery(0)
            )
            .is_err()
        );
        let property_args = json!({"blueprintId":blueprint,"widgetId":format!("{blueprint}#widget:Label"),"property":"text","text":"READY"});
        let property_result = json!({"blueprintId":blueprint,"widgetId":property_args["widgetId"],"property":"text","text":"READY","changed":true,"dirtyPackages":["/Game/UI"],"savedPackages":[],"revision":revision});
        let mut property_receipt =
            receipt_fixture("widget.property_set", property_result.clone(), true);
        property_receipt.verification["requestBlueprintId"] = property_args["blueprintId"].clone();
        property_receipt.verification["requestWidgetId"] = property_args["widgetId"].clone();
        property_receipt.verification["requestProperty"] = property_args["property"].clone();
        property_receipt.verification["requestText"] = property_args["text"].clone();
        assert!(
            validate_receipt(
                "widget.property_set",
                "id",
                &property_receipt,
                &property_result,
                &property_args,
                &fake_discovery(0)
            )
            .is_ok()
        );
        property_receipt.verification["requestText"] = json!("tampered");
        assert!(
            validate_receipt(
                "widget.property_set",
                "id",
                &property_receipt,
                &property_result,
                &property_args,
                &fake_discovery(0)
            )
            .is_err()
        );
        let event_args = json!({"blueprintId":blueprint,"agentKey":"main","event":"activate","actions":[{"kind":"text.set","targetWidgetId":format!("{blueprint}#widget:Label"),"text":"Go"}]});
        let event_result = json!({"blueprintId":blueprint,"eventId":format!("{blueprint}#event:main"),"agentKey":"main","event":"activate","actions":event_args["actions"],"changed":true,"dirtyPackages":["/Game/UI"],"savedPackages":[],"revision":revision});
        let mut event_receipt = receipt_fixture("widget.event_ensure", event_result.clone(), true);
        event_receipt.verification["requestBlueprintId"] = event_args["blueprintId"].clone();
        event_receipt.verification["requestAgentKey"] = event_args["agentKey"].clone();
        event_receipt.verification["requestIntent"] = event_args["event"].clone();
        event_receipt.verification["requestActions"] = event_args["actions"].clone();
        assert!(
            validate_receipt(
                "widget.event_ensure",
                "id",
                &event_receipt,
                &event_result,
                &event_args,
                &fake_discovery(0)
            )
            .is_ok()
        );
        let viewport_args = json!({"hostBlueprintId":blueprint,"widgetBlueprintId":"/Game/HUD.HUD","agentKey":"main","inputKey":"E","zOrder":0});
        let viewport_result = json!({"hostBlueprintId":blueprint,"widgetBlueprintId":"/Game/HUD.HUD","viewportId":format!("{blueprint}#viewport:main"),"graphId":"graph","inputKey":"E","zOrder":0,"widgetRevision":revision,"changed":true,"dirtyPackages":["/Game/UI"],"savedPackages":[],"revision":revision});
        let mut viewport_receipt =
            receipt_fixture("widget.viewport_ensure", viewport_result.clone(), true);
        viewport_receipt.target = viewport_result["viewportId"].as_str().unwrap().to_owned();
        viewport_receipt.verification["target"] = json!(viewport_receipt.target);
        viewport_receipt.verification["requestHostBlueprintId"] =
            viewport_args["hostBlueprintId"].clone();
        viewport_receipt.verification["requestWidgetBlueprintId"] =
            viewport_args["widgetBlueprintId"].clone();
        viewport_receipt.verification["requestAgentKey"] = viewport_args["agentKey"].clone();
        viewport_receipt.verification["requestInputKey"] = viewport_args["inputKey"].clone();
        viewport_receipt.verification["requestZOrder"] = viewport_args["zOrder"].clone();
        assert!(
            validate_receipt(
                "widget.viewport_ensure",
                "id",
                &viewport_receipt,
                &viewport_result,
                &viewport_args,
                &fake_discovery(0)
            )
            .is_ok()
        );
        assert!(
            capability::validate_output(
                "operation.view",
                serde_json::to_value(&viewport_receipt).unwrap()
            )
            .is_ok()
        );
        let screenshot_args = json!({"sessionId":"m6-pie-1","path":"ready-one.png"});
        let screenshot_result = json!({"sessionId":"m6-pie-1","path":"/fixture/Saved/MagiUnrealAXI/Screenshots/ready-one.png","width":640,"height":360,"format":"png","changed":true,"revision":revision});
        let screenshot_receipt =
            receipt_fixture("play.screenshot", screenshot_result.clone(), true);
        assert!(
            validate_receipt(
                "play.screenshot",
                "id",
                &screenshot_receipt,
                &screenshot_result,
                &screenshot_args,
                &fake_discovery(0)
            )
            .is_ok()
        );
        let nul_args = json!({"sessionId":"m6-pie-1","path":"claimed.png\0suffix.png"});
        assert!(
            validate_receipt(
                "play.screenshot",
                "id",
                &screenshot_receipt,
                &screenshot_result,
                &nul_args,
                &fake_discovery(0)
            )
            .is_err()
        );
        let mut wrong_session = screenshot_result.clone();
        wrong_session["sessionId"] = json!("other");
        assert!(
            validate_receipt(
                "play.screenshot",
                "id",
                &screenshot_receipt,
                &wrong_session,
                &screenshot_args,
                &fake_discovery(0)
            )
            .is_err()
        );
        let mut wrong_path = screenshot_result.clone();
        wrong_path["path"] = json!("/tmp/ready-one.png");
        assert!(
            validate_receipt(
                "play.screenshot",
                "id",
                &screenshot_receipt,
                &wrong_path,
                &screenshot_args,
                &fake_discovery(0)
            )
            .is_err()
        );
        let mut wrong_observed = screenshot_receipt.clone();
        wrong_observed.verification["observedRevision"] = json!("b".repeat(64));
        assert!(
            validate_receipt(
                "play.screenshot",
                "id",
                &wrong_observed,
                &screenshot_result,
                &screenshot_args,
                &fake_discovery(0)
            )
            .is_err()
        );
        let default_args = json!({"sessionId":"m6-pie-1"});
        let mut default_result = screenshot_result.clone();
        default_result["path"] = json!("/fixture/Saved/MagiUnrealAXI/Screenshots/m6-pie-1.png");
        let default_receipt = receipt_fixture("play.screenshot", default_result.clone(), true);
        assert!(
            validate_receipt(
                "play.screenshot",
                "id",
                &default_receipt,
                &default_result,
                &default_args,
                &fake_discovery(0)
            )
            .is_ok()
        );
        let mut altered = event_args;
        altered["actions"][0]["text"] = json!("Tampered");
        assert!(
            validate_receipt(
                "widget.event_ensure",
                "id",
                &event_receipt,
                &event_result,
                &altered,
                &fake_discovery(0)
            )
            .is_err()
        );
        let mut altered_viewport = viewport_result;
        altered_viewport["zOrder"] = json!(1);
        assert!(
            validate_receipt(
                "widget.viewport_ensure",
                "id",
                &viewport_receipt,
                &altered_viewport,
                &viewport_args,
                &fake_discovery(0)
            )
            .is_err()
        );
    }

    fn receipt_fixture(operation: &str, result: Value, matched: bool) -> Receipt {
        let capability = metadata(operation).unwrap();
        let target = capability
            .target_fields
            .iter()
            .map(|field| result[*field].as_str().unwrap_or_default())
            .collect::<Vec<_>>()
            .join("#");
        let revision = result["revision"]
            .as_str()
            .unwrap_or(&"a".repeat(64))
            .to_owned();
        let readback = capability.readback.unwrap();
        let mut verification = json!({"target":target,"readback":readback,"matched":matched,"observedRevision":revision});
        if operation == "play.input" {
            verification["accepted"] = json!(true);
            verification["beforeRevision"] = result["beforeRevision"].clone();
            verification["afterRevision"] = result["afterRevision"].clone();
        }
        let dirty_packages: Vec<String> = result["dirtyPackages"]
            .as_array()
            .map(|values| {
                values
                    .iter()
                    .filter_map(Value::as_str)
                    .map(str::to_owned)
                    .collect()
            })
            .unwrap_or_default();
        let saved_packages: Vec<String> = result["savedPackages"]
            .as_array()
            .map(|values| {
                values
                    .iter()
                    .filter_map(Value::as_str)
                    .map(str::to_owned)
                    .collect()
            })
            .unwrap_or_default();
        let persistence = if operation == "play.screenshot" || !saved_packages.is_empty() {
            "saved"
        } else if !dirty_packages.is_empty() {
            "dirty"
        } else {
            "unchanged"
        };
        Receipt {
            operation_id: "id".into(),
            operation: operation.into(),
            state: "completed".into(),
            project_id: "sha256:fixture".into(),
            editor_pid: std::process::id(),
            target,
            changed: result["changed"].as_bool().unwrap_or(false),
            transaction: if operation.starts_with("play.")
                || !result["changed"].as_bool().unwrap_or(false)
            {
                "none".into()
            } else {
                "atomic".into()
            },
            reversibility: if matches!(
                operation,
                "actor.delete" | "component.remove" | "blueprint.scs_component_remove"
            ) {
                "destructive".into()
            } else if operation.starts_with("play.") {
                "none".into()
            } else {
                "source-control".into()
            },
            dirty_packages,
            saved_packages,
            revision,
            persistence: persistence.into(),
            verification,
        }
    }
    #[test]
    fn validate_receipt_accepts_valid_generic_mutation() {
        let result = json!({"id":"actor","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let receipt = receipt_fixture("actor.update_transform", result.clone(), true);
        assert!(
            validate_receipt(
                "actor.update_transform",
                "id",
                &receipt,
                &result,
                &json!({"id":"actor"}),
                &fake_discovery(0)
            )
            .is_ok()
        );
    }

    #[test]
    fn validate_receipt_rejects_wrong_operation_and_state() {
        let result = json!({"id":"actor","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let discovery = fake_discovery(0);
        let args = json!({"id":"actor"});
        let receipt = receipt_fixture("actor.update_transform", result.clone(), true);
        let mut wrong_operation = receipt.clone();
        wrong_operation.operation = "actor.delete".into();
        assert!(
            validate_receipt(
                "actor.update_transform",
                "id",
                &wrong_operation,
                &result,
                &args,
                &discovery
            )
            .is_err()
        );
        let mut wrong_state = receipt;
        wrong_state.state = "failed".into();
        assert!(
            validate_receipt(
                "actor.update_transform",
                "id",
                &wrong_state,
                &result,
                &args,
                &discovery
            )
            .is_err()
        );
    }
    #[test]
    fn validate_receipt_rejects_safety_metadata_mismatch() {
        let result = json!({"id":"actor","changed":true,"dirtyPackages":["/Game/Level"],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let discovery = fake_discovery(0);
        let receipt = receipt_fixture("actor.update_transform", result.clone(), true);
        for field in ["transaction", "reversibility", "persistence"] {
            let mut malformed = receipt.clone();
            match field {
                "transaction" => malformed.transaction = "none".into(),
                "reversibility" => malformed.reversibility = "none".into(),
                "persistence" => malformed.persistence = "unchanged".into(),
                _ => unreachable!(),
            }
            assert!(
                validate_receipt(
                    "actor.update_transform",
                    "id",
                    &malformed,
                    &result,
                    &json!({"id":"actor"}),
                    &discovery
                )
                .is_err(),
                "{field} mismatch was accepted"
            );
        }
    }
    #[test]
    fn validate_receipt_rejects_unmatched_receipt() {
        let result = json!({"id":"actor","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let receipt = receipt_fixture("actor.update_transform", result.clone(), false);
        assert!(
            validate_receipt(
                "actor.update_transform",
                "id",
                &receipt,
                &result,
                &json!({"id":"actor"}),
                &fake_discovery(0)
            )
            .is_err()
        );
    }
    #[test]
    fn validate_receipt_rejects_discovery_identity_mismatch() {
        let result = json!({"id":"actor","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let receipt = receipt_fixture("actor.update_transform", result.clone(), true);
        let mut discovery = fake_discovery(0);
        discovery.project_id = "sha256:other".into();
        assert!(
            validate_receipt(
                "actor.update_transform",
                "id",
                &receipt,
                &result,
                &json!({"id":"actor"}),
                &discovery
            )
            .is_err()
        );
        let mut discovery = fake_discovery(0);
        discovery.pid += 1;
        assert!(
            validate_receipt(
                "actor.update_transform",
                "id",
                &receipt,
                &result,
                &json!({"id":"actor"}),
                &discovery
            )
            .is_err()
        );
    }
    #[test]
    fn validate_receipt_rejects_changed_parity_mismatch() {
        let result = json!({"id":"actor","changed":true,"dirtyPackages":[],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let mut receipt = receipt_fixture("actor.update_transform", result.clone(), true);
        receipt.changed = false;
        assert!(
            validate_receipt(
                "actor.update_transform",
                "id",
                &receipt,
                &result,
                &json!({"id":"actor"}),
                &fake_discovery(0)
            )
            .is_err()
        );
    }
    #[test]
    fn validate_receipt_binds_blueprint_ensure_semantic_request() {
        let result = json!({"blueprintId":"/Game/BP.BP","graphId":"graph","nodeId":"node","changed":true,"dirtyPackages":["/Game/BP"],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let mut receipt = receipt_fixture("blueprint.node_ensure", result.clone(), true);
        let verification = receipt.verification.as_object_mut().unwrap();
        verification.insert("requestBlueprintId".into(), json!("/Game/BP.BP"));
        verification.insert("requestGraphId".into(), json!("graph"));
        verification.insert("requestAgentKey".into(), json!("owned"));
        verification.insert("requestIntent".into(), json!("math.make_vector"));
        verification.insert("target".into(), json!("/Game/BP.BP#graph#owned"));
        receipt.target = "/Game/BP.BP#graph#owned".into();
        let args = json!({"blueprintId":"/Game/BP.BP","graphId":"graph","agentKey":"owned","node":"math.make_vector"});
        assert!(
            validate_receipt(
                "blueprint.node_ensure",
                "id",
                &receipt,
                &result,
                &args,
                &fake_discovery(0)
            )
            .is_ok()
        );
        let mut tampered = args;
        tampered["node"] = json!("actor.add_world_offset");
        assert!(
            validate_receipt(
                "blueprint.node_ensure",
                "id",
                &receipt,
                &result,
                &tampered,
                &fake_discovery(0)
            )
            .is_err()
        );
    }
    #[test]
    fn validate_receipt_accepts_truthful_pressed_revision() {
        let changed = json!({"sessionId":"s","key":"W","event":"pressed","accepted":true,"changed":true,"beforeRevision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","afterRevision":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","revision":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","dirtyPackages":[],"savedPackages":[]});
        let unchanged = json!({"sessionId":"s","key":"E","event":"pressed","accepted":true,"changed":false,"beforeRevision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","afterRevision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","dirtyPackages":[],"savedPackages":[]});
        for result in [changed, unchanged] {
            let receipt = receipt_fixture("play.input", result.clone(), true);
            assert!(
                validate_receipt(
                    "play.input",
                    "id",
                    &receipt,
                    &result,
                    &json!({"sessionId":"s","key":result["key"],"event":"pressed"}),
                    &fake_discovery(0)
                )
                .is_ok()
            );
        }
    }
    #[test]
    fn validate_receipt_rejects_request_target_mismatch() {
        let result = json!({"blueprintId":"/Game/BP.BP","sourcePinId":"source","targetPinId":"target","changed":true,"dirtyPackages":["/Game/BP"],"savedPackages":[],"revision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
        let receipt = receipt_fixture("blueprint.pin_connect", result.clone(), true);
        let args =
            json!({"blueprintId":"/Game/BP.BP","sourcePinId":"other","targetPinId":"target"});
        assert!(
            validate_receipt(
                "blueprint.pin_connect",
                "id",
                &receipt,
                &result,
                &args,
                &fake_discovery(0)
            )
            .is_err()
        );
        let before = "a".repeat(64);
        let target = "/Game/BP.BP#graph#owned";
        let discovery = fake_discovery(0);
        let atomic_args = json!({"blueprintId":"/Game/BP.BP","graphId":"graph","agentKey":"owned","node":"math.make_vector"});
        let atomic_receipt = Receipt {
            operation_id: "atomic-id".into(),
            operation: "blueprint.node_ensure".into(),
            state: "failed".into(),
            project_id: discovery.project_id.clone(),
            editor_pid: discovery.pid,
            target: target.into(),
            changed: false,
            transaction: "atomic".into(),
            reversibility: "source-control".into(),
            dirty_packages: vec![],
            saved_packages: vec![],
            revision: before.clone(),
            persistence: "unchanged".into(),
            verification: json!({
                "readback":"blueprint.graph_view","target":target,"matched":true,
                "beforeRevision":before,"observedRevision":before,"observedStatus":"error",
                "requestBlueprintId":"/Game/BP.BP","requestGraphId":"graph",
                "requestAgentKey":"owned","requestIntent":"math.make_vector"
            }),
        };
        let atomic_error = BridgeOperationError {
            kind: "operation_failed".into(),
            message: "rolled back".into(),
            retryable: true,
            dirty_package_count: Some(0),
            dirty_packages: Some(vec![]),
            error_count: None,
            warning_count: None,
            diagnostics: None,
            current_revision: None,
        };
        assert!(
            validate_failed_atomic_receipt(
                "blueprint.node_ensure",
                "atomic-id",
                &atomic_args,
                &ExecutionOptions {
                    expected_revision: Some(atomic_receipt.revision.clone()),
                    idempotency_key: None,
                },
                &atomic_receipt,
                &atomic_error,
                &discovery,
            )
            .is_ok()
        );
        assert!(
            validate_failed_atomic_receipt(
                "blueprint.node_ensure",
                "atomic-id",
                &atomic_args,
                &ExecutionOptions {
                    expected_revision: Some("b".repeat(64)),
                    idempotency_key: None,
                },
                &atomic_receipt,
                &atomic_error,
                &discovery,
            )
            .is_err()
        );
        let mut unknown_receipt = atomic_receipt.clone();
        unknown_receipt.state = "outcome_unknown".into();
        unknown_receipt.verification["matched"] = json!(false);
        let mut unknown_error = atomic_error.clone();
        let viewport_args = json!({"hostBlueprintId":"/Game/Host.Host","widgetBlueprintId":"/Game/HUD.HUD","agentKey":"main","inputKey":"E","zOrder":0});
        let mut viewport_receipt = atomic_receipt.clone();
        viewport_receipt.operation = "widget.viewport_ensure".into();
        viewport_receipt.target = "/Game/Host.Host#viewport:main".into();
        viewport_receipt.verification["target"] = json!(viewport_receipt.target);
        viewport_receipt.verification["readback"] = json!("blueprint.graph_view");
        viewport_receipt.verification["requestHostBlueprintId"] =
            viewport_args["hostBlueprintId"].clone();
        viewport_receipt.verification["requestWidgetBlueprintId"] =
            viewport_args["widgetBlueprintId"].clone();
        viewport_receipt.verification["requestAgentKey"] = viewport_args["agentKey"].clone();
        viewport_receipt.verification["requestInputKey"] = viewport_args["inputKey"].clone();
        viewport_receipt.verification["requestZOrder"] = viewport_args["zOrder"].clone();
        assert!(
            validate_failed_atomic_receipt(
                "widget.viewport_ensure",
                "atomic-id",
                &viewport_args,
                &ExecutionOptions {
                    expected_revision: Some(viewport_receipt.revision.clone()),
                    idempotency_key: None
                },
                &viewport_receipt,
                &atomic_error,
                &discovery
            )
            .is_ok()
        );
        viewport_receipt.verification["requestZOrder"] = json!(1);
        assert!(
            validate_failed_atomic_receipt(
                "widget.viewport_ensure",
                "atomic-id",
                &viewport_args,
                &ExecutionOptions {
                    expected_revision: Some(viewport_receipt.revision.clone()),
                    idempotency_key: None
                },
                &viewport_receipt,
                &atomic_error,
                &discovery
            )
            .is_err()
        );
        unknown_error.kind = "outcome_unknown".into();
        unknown_error.retryable = false;
        assert!(
            validate_failed_atomic_receipt(
                "blueprint.node_ensure",
                "atomic-id",
                &atomic_args,
                &ExecutionOptions {
                    expected_revision: Some(unknown_receipt.revision.clone()),
                    idempotency_key: None,
                },
                &unknown_receipt,
                &unknown_error,
                &discovery,
            )
            .is_ok()
        );
        unknown_receipt.transaction = "none".into();
        assert!(
            validate_failed_atomic_receipt(
                "blueprint.node_ensure",
                "atomic-id",
                &atomic_args,
                &ExecutionOptions {
                    expected_revision: Some(unknown_receipt.revision.clone()),
                    idempotency_key: None,
                },
                &unknown_receipt,
                &unknown_error,
                &discovery,
            )
            .is_err()
        );
    }
    #[test]
    fn validate_p14_receipts_bind_all_adversarial_requests_and_results() {
        let revision = "a".repeat(64);
        let cases = [
            (
                "navigation.bounds_ensure",
                json!({"levelId":"/Game/L","agentKey":"Nav","location":[1,2,3],"extent":[100,100,100],"boundsId":"/Game/L#nav-bounds:Nav","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"levelId":"/Game/L","agentKey":"Nav","location":[1,2,3],"extent":[100,100,100]}),
                "requestLevelId",
                "location",
            ),
            (
                "navigation.build",
                json!({"levelId":"/Game/L","ticketId":"ticket-1","state":"scheduled","changed":true,"revision":revision.clone()}),
                json!({"levelId":"/Game/L"}),
                "requestLevelId",
                "ticketId",
            ),
            (
                "blackboard.create",
                json!({"blackboardId":"/Game/BB.BB","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"path":"/Game/BB"}),
                "requestPath",
                "blackboardId",
            ),
            (
                "blackboard.key_ensure",
                json!({"blackboardId":"/Game/BB.BB","keyName":"Target","keyType":"Actor","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"blackboardId":"/Game/BB.BB","keyName":"Target","keyType":"Actor"}),
                "requestKeyName",
                "keyType",
            ),
            (
                "behavior_tree.create",
                json!({"behaviorTreeId":"/Game/BT.BT","blackboardId":"/Game/BB.BB","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"path":"/Game/BT","blackboardId":"/Game/BB.BB"}),
                "requestPath",
                "behaviorTreeId",
            ),
            (
                "behavior_tree.node_ensure",
                json!({"behaviorTreeId":"/Game/BT.BT","nodeId":"Root","nodeType":"sequence","keyName":null,"waitSeconds":null,"changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"behaviorTreeId":"/Game/BT.BT","nodeId":"Root","nodeType":"sequence"}),
                "requestNodeType",
                "nodeType",
            ),
            (
                "behavior_tree.connect",
                json!({"behaviorTreeId":"/Game/BT.BT","parentNodeId":"Root","childNodeId":"Move","childIndex":0,"linkId":"Root->Move","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"behaviorTreeId":"/Game/BT.BT","parentNodeId":"Root","childNodeId":"Move","childIndex":0}),
                "requestChildIndex",
                "childIndex",
            ),
            (
                "ai.controller_configure",
                json!({"blueprintId":"/Game/C.C","behaviorTreeId":"/Game/BT.BT","semantic":"on_possess.run_behavior_tree","changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"blueprintId":"/Game/C.C","behaviorTreeId":"/Game/BT.BT"}),
                "requestBehaviorTreeId",
                "semantic",
            ),
            (
                "ai.pawn_configure",
                json!({"blueprintId":"/Game/P.P","controllerBlueprintId":"/Game/C.C","typedDefaults":{"controllerClass":"/Game/C.C_C","autoPossessAI":"PlacedInWorldOrSpawned","maxWalkSpeed":600},"changed":false,"dirtyPackages":[],"savedPackages":[],"revision":revision.clone()}),
                json!({"blueprintId":"/Game/P.P","controllerBlueprintId":"/Game/C.C"}),
                "requestControllerBlueprintId",
                "controllerBlueprintId",
            ),
            (
                "play.ai_target_set",
                json!({"sessionId":"s","pawnId":"p","controllerId":"c","keyName":"Target","targetActorId":"a","targetLocation":[1,2,3],"changed":false,"restarted":false,"revision":revision.clone()}),
                json!({"sessionId":"s","pawnId":"p","keyName":"Target","targetActorId":"a"}),
                "requestTargetActorId",
                "targetActorId",
            ),
        ];
        for (operation, result, args, request_key, result_key) in cases {
            let mut receipt = receipt_fixture(operation, result.clone(), true);
            let target = match operation {
                "navigation.bounds_ensure" => "/Game/L#nav-bounds:Nav",
                "navigation.build" => "ticket-1",
                "blackboard.create" => "/Game/BB.BB",
                "blackboard.key_ensure" => "/Game/BB.BB#Target",
                "behavior_tree.create" => "/Game/BT.BT",
                "behavior_tree.node_ensure" => "/Game/BT.BT#Root",
                "behavior_tree.connect" => "/Game/BT.BT#Root->Move",
                "ai.controller_configure" => "/Game/C.C#ai-controller:/Game/BT.BT",
                "ai.pawn_configure" => "/Game/P.P#ai-pawn",
                "play.ai_target_set" => "s#p#Target#a",
                _ => unreachable!(),
            };
            receipt.target = target.into();
            if operation == "navigation.build" {
                receipt.transaction = "non-atomic".into();
                receipt.reversibility = "none".into();
            }
            receipt.verification["target"] = json!(target);
            for (key, value) in [
                ("requestLevelId", args.get("levelId")),
                ("requestAgentKey", args.get("agentKey")),
                ("requestLocation", args.get("location")),
                ("requestExtent", args.get("extent")),
                ("requestPath", args.get("path")),
                ("requestBlackboardId", args.get("blackboardId")),
                ("requestKeyName", args.get("keyName")),
                ("requestKeyType", args.get("keyType")),
                ("requestBehaviorTreeId", args.get("behaviorTreeId")),
                ("requestNodeId", args.get("nodeId")),
                ("requestNodeType", args.get("nodeType")),
                ("requestParentNodeId", args.get("parentNodeId")),
                ("requestChildNodeId", args.get("childNodeId")),
                ("requestChildIndex", args.get("childIndex")),
                ("requestBlueprintId", args.get("blueprintId")),
                (
                    "requestControllerBlueprintId",
                    args.get("controllerBlueprintId"),
                ),
                ("requestSessionId", args.get("sessionId")),
                ("requestPawnId", args.get("pawnId")),
                ("requestTargetActorId", args.get("targetActorId")),
            ] {
                if let Some(value) = value {
                    receipt.verification[key] = value.clone();
                }
            }
            let generated_validation = capability::validate_output(operation, result.clone());
            assert!(
                generated_validation.is_ok(),
                "{operation} generated output: {generated_validation:?}"
            );
            let validation = validate_receipt(
                operation,
                "id",
                &receipt,
                &result,
                &args,
                &fake_discovery(0),
            );
            assert!(validation.is_ok(), "{operation} valid: {validation:?}");
            let mut request_tampered = receipt.clone();
            request_tampered.verification[request_key] = json!("tampered");
            assert!(
                validate_receipt(
                    operation,
                    "id",
                    &request_tampered,
                    &result,
                    &args,
                    &fake_discovery(0)
                )
                .is_err(),
                "{operation} request"
            );
            let mut result_tampered = result.clone();
            result_tampered[result_key] = if result_key == "location" {
                json!([9, 9, 9])
            } else {
                json!("tampered")
            };
            assert!(
                validate_receipt(
                    operation,
                    "id",
                    &receipt,
                    &result_tampered,
                    &args,
                    &fake_discovery(0)
                )
                .is_err(),
                "{operation} result"
            );
            let mut target_tampered = receipt;
            target_tampered.target = "tampered".into();
            assert!(
                validate_receipt(
                    operation,
                    "id",
                    &target_tampered,
                    &result,
                    &args,
                    &fake_discovery(0)
                )
                .is_err(),
                "{operation} target"
            );
        }
    }
    #[test]
    fn validate_failed_terminal_ticket_receipt_rejects_ticket_level_message_revision_tampering() {
        let discovery = fake_discovery(0);
        let args = json!({"levelId":"/Game/L"});
        let revision = "a".repeat(64);
        let error = BridgeOperationError {
            kind: "navigation_build_failed".into(),
            message: "build failed".into(),
            retryable: false,
            dirty_package_count: Some(0),
            dirty_packages: Some(vec![]),
            error_count: None,
            warning_count: None,
            diagnostics: None,
            current_revision: None,
        };
        let mut receipt = Receipt {
            operation_id: "id".into(),
            operation: "navigation.build".into(),
            state: "failed".into(),
            project_id: discovery.project_id.clone(),
            editor_pid: discovery.pid,
            target: "ticket-1".into(),
            changed: false,
            transaction: "non-atomic".into(),
            reversibility: "none".into(),
            dirty_packages: vec![],
            saved_packages: vec![],
            revision: revision.clone(),
            persistence: "unchanged".into(),
            verification: json!({"target":"ticket-1","readback":"navigation.status","matched":true,"ticketId":"ticket-1","levelId":"/Game/L","requestLevelId":"/Game/L","observedRevision":revision,"observedStatus":"failed","terminal":true,"failureType":"navigation_build_failed","failureMessage":"build failed"}),
        };
        assert!(
            capability::validate_output("operation.view", serde_json::to_value(&receipt).unwrap())
                .is_ok()
        );
        assert!(
            validate_failed_terminal_ticket_receipt("id", &args, &receipt, &error, &discovery)
                .is_ok()
        );
        for (field, value) in [
            ("ticketId", json!("wrong")),
            ("levelId", json!("wrong")),
            ("failureMessage", json!("wrong")),
            ("observedRevision", json!("b".repeat(64))),
        ] {
            let original = receipt.verification[field].clone();
            receipt.verification[field] = value;
            assert!(
                validate_failed_terminal_ticket_receipt("id", &args, &receipt, &error, &discovery)
                    .is_err(),
                "{field}"
            );
            receipt.verification[field] = original;
        }
        receipt.verification["terminal"] = json!(false);
        assert!(
            validate_failed_terminal_ticket_receipt("id", &args, &receipt, &error, &discovery)
                .is_err()
        );
        receipt.verification["extra"] = json!(true);
        assert!(
            validate_failed_terminal_ticket_receipt("id", &args, &receipt, &error, &discovery)
                .is_err()
        );
    }

    fn failed_compile_fixture() -> (Receipt, BridgeOperationError, Value, ExecutionOptions) {
        let before = "a".repeat(64);
        let observed = before.clone();
        let diagnostics = json!([{"severity":"error","message":"invalid graph","graph":"/Game/BP.BP:Graph","nodeGuid":"00000000-0000-0000-0000-000000000001","nodeTitle":"Broken"}]);
        let target = "/Game/BP.BP";
        let receipt = Receipt {
            operation_id: "id".into(),
            operation: "blueprint.compile".into(),
            state: "failed".into(),
            project_id: "sha256:fixture".into(),
            editor_pid: std::process::id(),
            target: target.into(),
            changed: false,
            transaction: "non-atomic".into(),
            reversibility: "source-control".into(),
            dirty_packages: vec!["/Game/BP".into()],
            saved_packages: vec![],
            revision: observed.clone(),
            persistence: "dirty".into(),
            verification: json!({
                "readback":"blueprint.view","target":target,"matched":true,
                "beforeRevision":before,"observedRevision":observed,"observedStatus":"error",
                "failureType":"blueprint_compile_failed","errorCount":1,"warningCount":0,
                "diagnostics":diagnostics,"changedObjects":[]
            }),
        };
        let error = BridgeOperationError {
            kind: "blueprint_compile_failed".into(),
            message: "Blueprint compile failed".into(),
            retryable: false,
            dirty_package_count: Some(1),
            dirty_packages: Some(vec!["/Game/BP".into()]),
            error_count: Some(1),
            warning_count: Some(0),
            diagnostics: Some(diagnostics),
            current_revision: None,
        };
        (
            receipt,
            error,
            json!({"id":target}),
            ExecutionOptions {
                expected_revision: Some(before),
                idempotency_key: None,
            },
        )
    }

    #[test]
    fn validate_failed_compile_receipt_accepts_preserved_dirty_state() {
        let (receipt, error, args, options) = failed_compile_fixture();
        assert!(
            validate_failed_receipt(
                "blueprint.compile",
                "id",
                &args,
                &options,
                &receipt,
                &error,
                &fake_discovery(0)
            )
            .is_ok()
        );
    }

    #[test]
    fn validate_failed_compile_receipt_requires_canonical_operation_identity() {
        let (receipt, error, args, mut options) = failed_compile_fixture();
        options.idempotency_key = Some("compile-key".into());
        assert!(
            validate_failed_receipt(
                "blueprint.compile",
                "retry-id",
                &args,
                &options,
                &receipt,
                &error,
                &fake_discovery(0)
            )
            .is_err()
        );
        let mut canonical = receipt;
        canonical.operation_id = "retry-id".into();
        assert!(
            validate_failed_receipt(
                "blueprint.compile",
                "retry-id",
                &args,
                &options,
                &canonical,
                &error,
                &fake_discovery(0)
            )
            .is_ok()
        );
    }

    #[test]
    fn validate_failed_compile_receipt_accepts_unchanged_clean_state() {
        let (mut receipt, mut error, args, options) = failed_compile_fixture();
        receipt.dirty_packages.clear();
        receipt.persistence = "unchanged".into();
        error.dirty_package_count = Some(0);
        error.dirty_packages = Some(vec![]);
        assert!(
            validate_failed_receipt(
                "blueprint.compile",
                "id",
                &args,
                &options,
                &receipt,
                &error,
                &fake_discovery(0)
            )
            .is_ok()
        );
    }

    #[test]
    fn validate_failed_compile_receipt_rejects_false_atomicity_and_revision() {
        let (receipt, error, args, options) = failed_compile_fixture();
        let mut atomic = receipt.clone();
        atomic.transaction = "atomic".into();
        assert!(
            validate_failed_receipt(
                "blueprint.compile",
                "id",
                &args,
                &options,
                &atomic,
                &error,
                &fake_discovery(0)
            )
            .is_err()
        );

        let mut stale = options.clone();
        stale.expected_revision = Some("b".repeat(64));
        assert!(
            validate_failed_receipt(
                "blueprint.compile",
                "id",
                &args,
                &stale,
                &receipt,
                &error,
                &fake_discovery(0)
            )
            .is_err()
        );
    }
    #[test]
    fn validate_failed_compile_receipt_rejects_retryable_and_dirty_package_tampering() {
        let (mut receipt, mut error, args, options) = failed_compile_fixture();
        let discovery = fake_discovery(0);
        let valid = |receipt: &Receipt, error: &BridgeOperationError| {
            validate_failed_receipt(
                "blueprint.compile",
                "id",
                &args,
                &options,
                receipt,
                error,
                &discovery,
            )
        };

        error.retryable = true;
        assert!(
            valid(&receipt, &error).is_err(),
            "retryable compile failure"
        );
        error.retryable = false;

        receipt.dirty_packages = vec!["/Game/A".into(), "/Game/B".into()];
        error.dirty_packages = Some(receipt.dirty_packages.clone());
        error.dirty_package_count = Some(2);
        assert!(valid(&receipt, &error).is_ok(), "two-package fixture");

        let mut swapped = receipt.clone();
        swapped.dirty_packages.swap(0, 1);
        assert!(valid(&swapped, &error).is_err(), "swapped dirty packages");

        let mut extra = receipt.clone();
        extra.dirty_packages.push("/Game/C".into());
        assert!(valid(&extra, &error).is_err(), "extra dirty package");

        let mut absent = receipt.clone();
        absent.dirty_packages.pop();
        assert!(valid(&absent, &error).is_err(), "absent dirty package");

        let mut unknown = receipt;
        unknown.verification["extra"] = json!(true);
        assert!(
            valid(&unknown, &error).is_err(),
            "unknown verification field"
        );
    }

    #[test]
    fn protocol_fixtures_match_typed_client_contract() {
        let handshake: HandshakeResponse = serde_json::from_str(include_str!(
            "../protocol/fixtures/handshake-response-valid.json"
        ))
        .unwrap();
        assert_eq!(handshake.protocol, PROTOCOL);
        assert_eq!(handshake.plugin_version, VERSION);
        assert!(!handshake.process_start.is_empty());

        for fixture in [
            include_str!("../protocol/fixtures/health-response.json"),
            include_str!("../protocol/fixtures/describe-response.json"),
            include_str!("../protocol/fixtures/stop-response.json"),
            include_str!("../protocol/fixtures/stop-starting-error.json"),
        ] {
            let response: OperationResponse = serde_json::from_str(fixture).unwrap();
            assert_eq!(response.protocol, PROTOCOL);
            assert!(!response.id.is_empty());
        }

        for fixture in [
            include_str!("../protocol/fixtures/handshake-valid.json"),
            include_str!("../protocol/fixtures/health-request.json"),
            include_str!("../protocol/fixtures/describe-request.json"),
            include_str!("../protocol/fixtures/stop-request.json"),
        ] {
            let request: Value = serde_json::from_str(fixture).unwrap();
            assert_eq!(request["protocol"], PROTOCOL);
        }
    }
}

#[cfg(test)]
mod journal_safety_tests {
    use super::*;

    #[test]
    fn malformed_journal_fails_closed_without_resetting_history() {
        let directory = tempfile::tempdir().unwrap();
        fs::write(
            directory.path().join("operation-journal-v1.json"),
            b"not-json",
        )
        .unwrap();
        let error = journal_receipt_in_directory(directory.path(), &json!({"operationId":"new"}))
            .unwrap_err();
        assert_eq!(error.reason, "malformed_journal");
        assert_eq!(
            fs::read(directory.path().join("operation-journal-v1.json")).unwrap(),
            b"not-json"
        );
    }

    #[test]
    fn journal_write_failure_is_reported_without_replacing_receipt() {
        let directory = tempfile::tempdir().unwrap();
        fs::create_dir(directory.path().join("operation-journal-v1.json")).unwrap();
        assert_eq!(
            journal_receipt_in_directory(directory.path(), &json!({"operationId":"known"}))
                .unwrap_err()
                .reason,
            "bridge_io_failed"
        );
    }
}
