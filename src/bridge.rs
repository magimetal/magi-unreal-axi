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
}

#[derive(Debug, Deserialize)]
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
    if response.status == "ok" {
        let mut result = response.result;
        if requires_receipt(operation) {
            let receipt = response
                .receipt
                .as_ref()
                .ok_or_else(|| outcome_unknown(&id))?;
            validate_receipt(operation, &id, receipt, &result, discovery)?;
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
        "blueprint_compile_failed" => "blueprint_compile_failed",
        _ => "operation_failed",
    };
    if operation == "blueprint.compile"
        && error.kind == "blueprint_compile_failed"
        && metadata(operation).and_then(|value| value.failure_receipt) == Some("preserved-dirty")
    {
        let receipt = response
            .receipt
            .as_ref()
            .ok_or_else(|| outcome_unknown(&id))?;
        validate_failed_receipt(operation, &id, &args, options, receipt, &error, discovery)?;
        let app_error = AppError::operational(
            "bridge",
            "blueprint_compile_failed",
            error.message.clone(),
            "inspect `magi-unreal-axi operation view <id>` before retrying",
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
        .with_operation_id(id.clone())
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
        app_error.with_operation_id(id)
    } else {
        app_error
    })
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
    let valid_revision =
        |value: &str| value.len() == 64 && value.bytes().all(|byte| byte.is_ascii_hexdigit());
    let changed = before_revision != observed_revision;
    if receipt.operation_id != id
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

fn validate_receipt(
    operation: &str,
    id: &str,
    receipt: &Receipt,
    result: &Value,
    discovery: &Discovery,
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
    let result_target = if !capability_record.target_fields.is_empty() {
        capability_record
            .target_fields
            .iter()
            .map(|field| result[*field].as_str().unwrap_or_default())
            .collect::<Vec<_>>()
            .join("#")
    } else {
        String::new()
    };
    let result_changed = result["changed"]
        .as_bool()
        .ok_or_else(|| outcome_unknown(id))?;
    let result_revision = result["revision"]
        .as_str()
        .ok_or_else(|| outcome_unknown(id))?;
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
        || target.chars().count() > 1024
        || readback.is_empty()
        || readback.chars().count() > 128
        || readback != expected_readback
        || !matched
        || receipt.changed != result_changed
        || receipt.revision != result_revision
        || receipt.dirty_packages != result_dirty
        || receipt.saved_packages != result_saved
        || receipt.transaction != expected_transaction
        || receipt.reversibility != expected_reversibility
        || receipt.persistence != expected_persistence
        || receipt.revision.chars().count() != 64
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
    if let Some(observed) = verification.get("observedRevision").and_then(Value::as_str) {
        if observed != result_revision {
            return Err(outcome_unknown(id));
        }
    } else if operation != "play.screenshot" {
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
        if !result["accepted"].as_bool().unwrap_or(false)
            || before.len() != 64
            || after.len() != 64
            || result_revision != after
            || observed != after
            || result_changed != (before != after)
            || (result["event"] == "pressed" && before == after)
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
    let (session, discovery) = match live(project, pid) {
        Ok(value) => value,
        Err(error)
            if operation == "operation.view"
                && matches!(error.reason, "editor_not_found" | "stale_editor") =>
        {
            return journal_find(project, args["id"].as_str().unwrap_or(""))?.ok_or(error);
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
            {
                journal_receipt(project, receipt)?;
            }
            Ok(value)
        }
        Err(error) => {
            if let Some(receipt) = error.receipt() {
                journal_receipt(project, receipt)?;
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
fn journal_receipt(project: &Path, receipt: &Value) -> Result<(), AppError> {
    let directory = project_directory(project)?;
    secure_directory(&runtime_root()?)?;
    secure_directory(&directory)?;
    let path = directory.join("operation-journal-v1.json");
    let mut records = if path.exists() {
        serde_json::from_slice::<Vec<Value>>(&read_bounded(&path, MAX_RESPONSE as u64)?)
            .unwrap_or_default()
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

    fn receipt_fixture(operation: &str, result: Value, matched: bool) -> Receipt {
        let target = if operation == "play.input" {
            format!(
                "{}#{}#{}",
                result["sessionId"].as_str().unwrap_or(""),
                result["key"].as_str().unwrap_or(""),
                result["event"].as_str().unwrap_or("")
            )
        } else {
            result["id"]
                .as_str()
                .or_else(|| result["levelId"].as_str())
                .unwrap_or("target")
                .to_owned()
        };
        let revision = result["revision"]
            .as_str()
            .unwrap_or(&"a".repeat(64))
            .to_owned();
        let readback = if operation.starts_with("actor.") {
            "actor.view"
        } else if operation.starts_with("component.") {
            "component.view"
        } else if operation == "play.input" {
            "play.observe"
        } else {
            "level.current"
        };
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
            reversibility: if matches!(operation, "actor.delete" | "component.remove") {
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
                &fake_discovery(0)
            )
            .is_ok()
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
                &fake_discovery(0)
            )
            .is_err()
        );
    }
    #[test]
    fn validate_receipt_requires_later_pressed_revision() {
        let result = json!({"sessionId":"s","key":"W","event":"pressed","accepted":true,"changed":true,"beforeRevision":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","afterRevision":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","revision":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","dirtyPackages":[],"savedPackages":[]});
        let receipt = receipt_fixture("play.input", result.clone(), true);
        assert!(
            validate_receipt("play.input", "id", &receipt, &result, &fake_discovery(0)).is_ok()
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
