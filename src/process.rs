use crate::{
    cli::{OutputProcessArgs, PackageArgs, ProcessArgs, TestArgs, TestCommand},
    config,
    engine::EngineInfo,
    error::AppError,
};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
#[cfg(unix)]
use std::os::unix::process::CommandExt;
use std::{
    collections::BTreeMap,
    ffi::{OsStr, OsString},
    fs,
    io::Read,
    path::{Component, Path, PathBuf},
    process::{Command, ExitStatus, Stdio},
    sync::mpsc::{self, Receiver, RecvTimeoutError},
    thread,
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

const CAPTURE_LIMIT: usize = 1024 * 1024;
const MAX_REPORT_BYTES: u64 = 16 * 1024 * 1024;
const MAX_AUTOMATION_TESTS: usize = 50_000;
const MAX_LIST_LOG_BYTES: u64 = 16 * 1024 * 1024;
const MAX_LOG_FILES: usize = 20;
const MAX_LOG_SCAN_BYTES: u64 = 8 * 1024 * 1024;
const MAX_LOG_LINE_SCALARS: usize = 4_000;
const MAX_ARTIFACT_ENTRIES: usize = 20;
const MAX_MATERIALIZE_ENTRIES: usize = 100_000;
const MAX_MATERIALIZE_FILES: usize = 10_000;
const MAX_MATERIALIZE_DEPTH: usize = 32;

#[derive(Debug, Serialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Invocation {
    pub executable: PathBuf,
    pub arguments: Vec<String>,
    pub working_directory: PathBuf,
    pub environment: BTreeMap<String, String>,
    pub executes_project_code: bool,
}

impl Invocation {
    fn command(&self) -> Command {
        let mut command = Command::new(&self.executable);
        command
            .args(&self.arguments)
            .current_dir(&self.working_directory)
            .stdin(Stdio::null());
        command.envs(&self.environment);
        #[cfg(unix)]
        command.process_group(0);
        command
    }
}

#[derive(Debug)]
struct Capture {
    status: ExitStatus,
    duration: Duration,
    stdout: Vec<u8>,
    stderr: Vec<u8>,
    stdout_truncated: bool,
    stderr_truncated: bool,
}

impl Capture {
    fn combined_text(&self) -> String {
        format!(
            "{}\n{}",
            String::from_utf8_lossy(&self.stdout),
            String::from_utf8_lossy(&self.stderr)
        )
    }
}

pub fn build_invocation(
    project: &Path,
    engine: &EngineInfo,
    args: &ProcessArgs,
    timeout: Option<u64>,
) -> Result<Value, AppError> {
    let project = canonical_project(project)?;
    let invocation = Invocation {
        executable: engine.ubt.clone(),
        arguments: vec![
            format!("{}Editor", project_stem(&project)?),
            "Mac".into(),
            "Development".into(),
            format!("-Project={}", project.display()),
            "-NoUBTMakefiles".into(),
            "-WaitMutex".into(),
            "-utf8output".into(),
        ],
        working_directory: project_root(&project),
        environment: dotnet_environment(engine),
        executes_project_code: true,
    };
    if args.dry_run {
        return Ok(preview(&project, "build", invocation, &[]));
    }
    let id = operation_id();
    let (summary, capture) = execute_pipeline(
        &project,
        &id,
        "build",
        &invocation,
        Duration::from_secs(timeout.unwrap_or(1800)),
        &[],
    )?;
    require_process_success(summary, &capture)
}

pub fn test_invocation(
    project: &Path,
    engine: &EngineInfo,
    args: &TestArgs,
    timeout: Option<u64>,
) -> Result<Value, AppError> {
    let project = canonical_project(project)?;
    match &args.command {
        TestCommand::List {
            filter,
            limit,
            dry_run,
        } => test_list(
            &project,
            engine,
            filter.as_deref(),
            *limit,
            *dry_run,
            timeout,
        ),
        TestCommand::Run {
            filter,
            report,
            dry_run,
        } => test_run(
            &project,
            engine,
            filter,
            report.as_deref(),
            *dry_run,
            timeout,
        ),
    }
}

fn test_list(
    project: &Path,
    engine: &EngineInfo,
    filter: Option<&str>,
    limit: u16,
    dry_run: bool,
    timeout: Option<u64>,
) -> Result<Value, AppError> {
    if let Some(filter) = filter {
        validate_list_filter(filter)?;
    }
    let id = operation_id();
    let ue_log = operation_directory(project, &id).join("automation-list.log");
    let invocation = automation_invocation(project, engine, "Automation List;Quit", None, &ue_log);
    if dry_run {
        return Ok(preview(
            project,
            "test-list",
            invocation,
            &[("automation-list", ue_log)],
        ));
    }
    fs::create_dir_all(operation_directory(project, &id)).map_err(process_io)?;
    let (summary, capture) = execute_pipeline(
        project,
        &id,
        "test-list",
        &invocation,
        Duration::from_secs(timeout.unwrap_or(1800)),
        &[("automation-list", ue_log.clone())],
    )?;
    let summary = require_process_success(summary, &capture)?;
    let all = parse_automation_list(&ue_log)?;
    let query = filter.unwrap_or("").to_ascii_lowercase();
    let matched = all
        .into_iter()
        .filter(|name| query.is_empty() || name.to_ascii_lowercase().contains(&query))
        .collect::<Vec<_>>();
    let total = matched.len();
    let items = matched
        .into_iter()
        .take(limit as usize)
        .map(|id| json!({"id": id}))
        .collect::<Vec<_>>();
    Ok(json!({
        "count": items.len(),
        "total": total,
        "scope": if query.is_empty() { "all automation tests".to_owned() } else { format!("automation tests containing {}", filter.unwrap_or("")) },
        "items": items,
        "operation": summary["operation"],
        "artifacts": summary["artifacts"]
    }))
}

fn test_run(
    project: &Path,
    engine: &EngineInfo,
    filter: &str,
    report: Option<&Path>,
    dry_run: bool,
    timeout: Option<u64>,
) -> Result<Value, AppError> {
    validate_run_filter(filter)?;
    let id = operation_id();
    let report = match report {
        Some(path) => safe_output_path(path, false, project, engine)?,
        None => operation_directory(project, &id).join("automation-report"),
    };
    let ue_log = operation_directory(project, &id).join("automation-run.log");
    let command = format!("Automation RunTests {filter}");
    let invocation = automation_invocation(project, engine, &command, Some(&report), &ue_log);
    if dry_run {
        return Ok(preview(
            project,
            "test-run",
            invocation,
            &[("automation-report", report), ("automation-log", ue_log)],
        ));
    }
    prepare_empty_directory(&report)?;
    fs::create_dir_all(operation_directory(project, &id)).map_err(process_io)?;
    let (mut summary, capture) = execute_pipeline(
        project,
        &id,
        "test-run",
        &invocation,
        Duration::from_secs(timeout.unwrap_or(1800)),
        &[
            ("automation-report", report.clone()),
            ("automation-log", ue_log.clone()),
        ],
    )?;
    let parsed = if !report.join("index.json").is_file() && automation_log_has_no_matches(&ue_log)?
    {
        ParsedAutomationReport {
            matched: 0,
            failed: 0,
            not_run: 0,
            in_process: 0,
            totals: json!({"matched":0,"succeeded":0,"succeededWithWarnings":0,"failed":0,"notRun":0,"inProcess":0,"warnings":0,"errors":0,"durationMs":0,"complete":true}),
            notable: Vec::new(),
        }
    } else {
        parse_automation_report(&report).map_err(|error| {
            summary["operation"]["status"] = json!("failed");
            let _ = persist_operation(project, &summary);
            error.with_bridge_diagnostics(None, None, Some(summary.clone()))
        })?
    };
    summary["totals"] = parsed.totals.clone();
    if report.join("index.json").is_file() {
        summary["artifacts"]
            .as_array_mut()
            .unwrap()
            .push(artifact_summary(
                "automation-report-index",
                &report.join("index.json"),
            )?);
    }
    let failed = parsed.matched == 0
        || parsed.failed > 0
        || parsed.not_run > 0
        || parsed.in_process > 0
        || !capture.status.success();
    summary["operation"]["status"] = json!(if failed { "failed" } else { "passed" });
    persist_operation(project, &summary)?;
    if failed {
        let reason = if parsed.matched == 0 {
            "automation_no_matches"
        } else if !capture.status.success() {
            "process_failed"
        } else {
            "automation_failed"
        };
        return Err(pipeline_error(
            reason,
            format!(
                "automation matched {}, failed {}, not run {}, in process {}",
                parsed.matched, parsed.failed, parsed.not_run, parsed.in_process
            ),
            summary,
        ));
    }
    Ok(json!({
        "operation": summary["operation"],
        "totals": summary["totals"],
        "tests": parsed.notable,
        "artifacts": summary["artifacts"]
    }))
}

fn automation_invocation(
    project: &Path,
    engine: &EngineInfo,
    automation_command: &str,
    report: Option<&Path>,
    ue_log: &Path,
) -> Invocation {
    let mut arguments = vec![
        project.display().to_string(),
        "-unattended".into(),
        "-nop4".into(),
        "-nosplash".into(),
        "-nullrhi".into(),
        "-NoSound".into(),
        "-Multiprocess".into(),
        format!("-ExecCmds={automation_command}"),
        "-TestExit=Automation Test Queue Empty".into(),
        format!("-abslog={}", ue_log.display()),
    ];
    if let Some(report) = report {
        arguments.push(format!("-ReportOutputPath={}", report.display()));
    }
    Invocation {
        executable: engine.editor_cmd.clone(),
        arguments,
        working_directory: project_root(project),
        environment: BTreeMap::new(),
        executes_project_code: true,
    }
}

pub fn cook_invocation(
    project: &Path,
    engine: &EngineInfo,
    args: &OutputProcessArgs,
    timeout: Option<u64>,
) -> Result<Value, AppError> {
    let project = canonical_project(project)?;
    let output = safe_output_path(&args.output, false, &project, engine)?;
    let cooked_source = project_root(&project).join("Saved/Cooked/Mac");
    let invocation = uat_cook_invocation(&project, engine);
    if args.dry_run {
        return Ok(preview(
            &project,
            "cook",
            invocation,
            &[("cooked-output", output), ("cooked-source", cooked_source)],
        ));
    }
    let id = operation_id();
    let (summary, capture) = execute_pipeline(
        &project,
        &id,
        "cook",
        &invocation,
        Duration::from_secs(timeout.unwrap_or(3600)),
        &[("cooked-source", cooked_source.clone())],
    )?;
    let mut summary = require_process_success(summary, &capture)?;
    let postprocess = (|| {
        validate_project_generated_path(&cooked_source, &project)?;
        let staging = unique_sibling(
            output.parent().unwrap_or(Path::new(".")),
            ".magi-cook-stage",
        )?;
        fs::create_dir(&staging).map_err(process_io)?;
        if let Err(error) = copy_cooked_tree(&cooked_source, &staging)
            .and_then(|_| artifact_summary("cooked-output", &staging).map(|_| ()))
        {
            let _ = fs::remove_dir_all(&staging);
            return Err(error);
        }
        commit_fresh_directory(&staging, &output)?;
        summary["artifacts"]
            .as_array_mut()
            .expect("receipt artifacts")
            .push(artifact_summary("cooked-output", &output)?);
        Ok::<(), AppError>(())
    })();
    if let Err(error) = postprocess {
        summary["operation"]["status"] = json!("failed");
        persist_operation(&project, &summary)?;
        return Err(pipeline_error(
            "artifact_copy_failed",
            error.message,
            summary,
        ));
    }
    persist_operation(&project, &summary)?;
    Ok(summary)
}

fn uat_cook_invocation(project: &Path, engine: &EngineInfo) -> Invocation {
    let arguments = vec![
        "BuildCookRun".into(),
        format!("-project={}", project.display()),
        "-nop4".into(),
        "-utf8output".into(),
        "-platform=Mac".into(),
        "-clientconfig=Development".into(),
        "-cook".into(),
    ];
    Invocation {
        executable: engine.uat.clone(),
        arguments,
        working_directory: project_root(project),
        environment: dotnet_environment(engine),
        executes_project_code: true,
    }
}

pub fn package_invocation(
    project: &Path,
    engine: &EngineInfo,
    args: &PackageArgs,
    timeout: Option<u64>,
) -> Result<Value, AppError> {
    let project = canonical_project(project)?;
    let output = safe_output_path(&args.output, args.force, &project, engine)?;
    let staging = unique_sibling(
        output.parent().unwrap_or(Path::new(".")),
        &format!(
            ".magi-package-stage-{}",
            output
                .file_name()
                .and_then(OsStr::to_str)
                .unwrap_or("output")
        ),
    )?;
    let invocation = uat_invocation(&project, engine, &staging, true);
    if args.dry_run {
        return Ok(preview(
            &project,
            "package",
            invocation,
            &[("package-output", output)],
        ));
    }
    let id = operation_id();
    let (summary, capture) = execute_pipeline(
        &project,
        &id,
        "package",
        &invocation,
        Duration::from_secs(timeout.unwrap_or(3600)),
        &[],
    )?;
    let mut summary = match require_process_success(summary, &capture) {
        Ok(summary) => summary,
        Err(error) => {
            let _ = fs::remove_dir_all(&staging);
            return Err(error);
        }
    };
    let postprocess = (|| {
        let staged_artifact = artifact_summary("package-output", &staging)?;
        if staged_artifact["fileCount"].as_u64() == Some(0) {
            return Err(process_error(
                "artifact_empty",
                format!("{} produced no files", staging.display()),
            ));
        }
        config::atomic_write(
            &staging.join(".magi-unreal-axi-package.json"),
            serde_json::to_string(&json!({"managed":true,"project":project}))
                .map_err(process_json)?
                .as_bytes(),
        )
        .map_err(process_io)?;
        if let Some((backup, cleanup_error)) = commit_managed_directory(&staging, &output)? {
            let warnings = summary["totals"]["warnings"].as_u64().unwrap_or(0);
            summary["totals"]["warnings"] = json!(warnings.saturating_add(1));
            summary["artifacts"]
                .as_array_mut()
                .expect("receipt artifacts")
                .push(json!({
                    "kind":"package-backup-retained",
                    "path":backup,
                    "exists":backup.exists(),
                    "cleanupError":cleanup_error
                }));
        }
        summary["artifacts"]
            .as_array_mut()
            .expect("receipt artifacts")
            .push(artifact_summary("package-output", &output)?);
        Ok::<(), AppError>(())
    })();
    if let Err(error) = postprocess {
        let _ = fs::remove_dir_all(&staging);
        summary["operation"]["status"] = json!("failed");
        persist_operation(&project, &summary)?;
        return Err(pipeline_error(
            "package_postprocess_failed",
            error.message,
            summary,
        ));
    }
    persist_operation(&project, &summary)?;
    Ok(summary)
}

fn uat_invocation(project: &Path, engine: &EngineInfo, output: &Path, package: bool) -> Invocation {
    let mut arguments = vec![
        "BuildCookRun".into(),
        format!("-project={}", project.display()),
        "-nop4".into(),
        "-utf8output".into(),
        "-platform=Mac".into(),
        "-clientconfig=Development".into(),
        "-build".into(),
        "-cook".into(),
        "-stage".into(),
    ];
    if package {
        arguments.extend(["-package".into(), "-pak".into()]);
    }
    arguments.extend([
        "-archive".into(),
        format!("-archivedirectory={}", output.display()),
    ]);
    Invocation {
        executable: engine.uat.clone(),
        arguments,
        working_directory: project_root(project),
        environment: dotnet_environment(engine),
        executes_project_code: true,
    }
}

fn preview(
    project: &Path,
    kind: &str,
    invocation: Invocation,
    artifacts: &[(&str, PathBuf)],
) -> Value {
    json!({
        "dryRun": true,
        "operation": {
            "kind": kind,
            "project": project,
            "platform": "Mac",
            "configuration": "Development",
            "executesProjectCode": invocation.executes_project_code
        },
        "invocation": invocation,
        "artifacts": artifacts.iter().map(|(kind, path)| json!({"kind":kind,"path":path,"exists":path.exists()})).collect::<Vec<_>>()
    })
}

fn execute_pipeline(
    project: &Path,
    id: &str,
    kind: &str,
    invocation: &Invocation,
    timeout: Duration,
    output_artifacts: &[(&str, PathBuf)],
) -> Result<(Value, Capture), AppError> {
    let capture = match capture_process(invocation, timeout) {
        Ok(capture) => capture,
        Err(error) => {
            let log = retain_pipeline_log(
                project,
                id,
                kind,
                invocation,
                None,
                Some(error.message.as_bytes()),
                false,
            )?;
            let summary = operation_summary(
                project,
                id,
                kind,
                "failed",
                (None, Duration::ZERO),
                invocation,
                SummaryData {
                    totals: json!({"errors":0,"warnings":0,"complete":false}),
                    artifacts: vec![artifact_summary("log", &log)?],
                },
            );
            persist_operation(project, &summary)?;
            return Err(error.with_bridge_diagnostics(None, None, Some(summary)));
        }
    };
    let log = retain_pipeline_log(
        project,
        id,
        kind,
        invocation,
        Some((&capture.stdout, capture.stdout_truncated)),
        Some(&capture.stderr),
        capture.stderr_truncated,
    )?;
    let (errors, warnings) = diagnostic_totals(&capture.combined_text());
    let mut artifacts = vec![artifact_summary("log", &log)?];
    artifacts.extend(
        output_artifacts
            .iter()
            .map(|(kind, path)| json!({"kind":kind,"path":path,"exists":path.exists()})),
    );
    let up_to_date = kind == "build" && is_up_to_date(&capture.combined_text());
    let status = if !capture.status.success() {
        "failed"
    } else if up_to_date {
        "up_to_date"
    } else {
        "passed"
    };
    let summary = operation_summary(
        project,
        id,
        kind,
        status,
        (capture.status.code(), capture.duration),
        invocation,
        SummaryData {
            totals: json!({"errors":errors,"warnings":warnings,"complete":true}),
            artifacts,
        },
    );
    persist_operation(project, &summary)?;
    Ok((summary, capture))
}

fn require_process_success(summary: Value, capture: &Capture) -> Result<Value, AppError> {
    if capture.status.success() {
        Ok(summary)
    } else {
        Err(pipeline_error(
            "process_failed",
            format!("process exited with code {:?}", capture.status.code()),
            summary,
        ))
    }
}

fn capture_process(invocation: &Invocation, timeout: Duration) -> Result<Capture, AppError> {
    let mut child = invocation
        .command()
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|error| {
            process_error(
                "process_start_failed",
                format!("cannot start {}: {error}", invocation.executable.display()),
            )
        })?;
    let pid = child.id();
    let stdout = child.stdout.take().map(spawn_reader);
    let stderr = child.stderr.take().map(spawn_reader);
    let started = Instant::now();
    let deadline = started + timeout;
    let mut status = None;
    while status.is_none() && Instant::now() < deadline {
        status = child.try_wait().map_err(process_io)?;
        if status.is_none() {
            thread::sleep(Duration::from_millis(20));
        }
    }
    let mut timed_out = status.is_none();
    if timed_out {
        terminate_process_group(pid, &mut child);
        status = child.try_wait().ok().flatten();
    }
    let mut stdout = receive_reader(stdout, deadline);
    let mut stderr = receive_reader(stderr, deadline);
    if stdout.timed_out || stderr.timed_out {
        timed_out = true;
        terminate_process_group(pid, &mut child);
        stdout.retry_after_kill();
        stderr.retry_after_kill();
    }
    if timed_out {
        return Err(process_error(
            "process_timeout",
            format!(
                "{} exceeded timeout of {} seconds",
                invocation.executable.display(),
                timeout.as_secs()
            ),
        ));
    }
    if let Some(error) = stdout.error.or(stderr.error) {
        return Err(process_error("capture_failed", error));
    }
    Ok(Capture {
        status: status
            .ok_or_else(|| process_error("process_wait_failed", "missing exit status"))?,
        duration: started.elapsed(),
        stdout: stdout.bytes,
        stderr: stderr.bytes,
        stdout_truncated: stdout.truncated,
        stderr_truncated: stderr.truncated,
    })
}

type ReaderMessage = std::io::Result<(Vec<u8>, bool)>;

fn spawn_reader(reader: impl Read + Send + 'static) -> Receiver<ReaderMessage> {
    let (sender, receiver) = mpsc::sync_channel(1);
    thread::spawn(move || {
        let _ = sender.send(read_bounded(reader));
    });
    receiver
}

struct ReaderResult {
    receiver: Option<Receiver<ReaderMessage>>,
    bytes: Vec<u8>,
    truncated: bool,
    error: Option<String>,
    timed_out: bool,
}

impl ReaderResult {
    fn retry_after_kill(&mut self) {
        let Some(receiver) = self.receiver.take() else {
            return;
        };
        match receiver.recv_timeout(Duration::from_millis(500)) {
            Ok(Ok((bytes, truncated))) => {
                self.bytes = bytes;
                self.truncated = truncated;
                self.timed_out = false;
            }
            Ok(Err(error)) => self.error = Some(error.to_string()),
            Err(_) => {}
        }
    }
}

fn receive_reader(receiver: Option<Receiver<ReaderMessage>>, deadline: Instant) -> ReaderResult {
    let Some(receiver) = receiver else {
        return ReaderResult {
            receiver: None,
            bytes: Vec::new(),
            truncated: false,
            error: None,
            timed_out: false,
        };
    };
    let remaining = deadline.saturating_duration_since(Instant::now());
    match receiver.recv_timeout(remaining) {
        Ok(Ok((bytes, truncated))) => ReaderResult {
            receiver: None,
            bytes,
            truncated,
            error: None,
            timed_out: false,
        },
        Ok(Err(error)) => ReaderResult {
            receiver: None,
            bytes: Vec::new(),
            truncated: false,
            error: Some(error.to_string()),
            timed_out: false,
        },
        Err(RecvTimeoutError::Timeout) => ReaderResult {
            receiver: Some(receiver),
            bytes: Vec::new(),
            truncated: true,
            error: None,
            timed_out: true,
        },
        Err(RecvTimeoutError::Disconnected) => ReaderResult {
            receiver: None,
            bytes: Vec::new(),
            truncated: false,
            error: Some("child output reader disconnected".into()),
            timed_out: false,
        },
    }
}

fn read_bounded(mut reader: impl Read) -> ReaderMessage {
    let mut bytes = Vec::new();
    let mut buffer = [0_u8; 8192];
    let mut truncated = false;
    loop {
        let count = reader.read(&mut buffer)?;
        if count == 0 {
            break;
        }
        let remaining = CAPTURE_LIMIT.saturating_sub(bytes.len());
        bytes.extend_from_slice(&buffer[..count.min(remaining)]);
        truncated |= count > remaining;
    }
    Ok((bytes, truncated))
}

fn terminate_process_group(pid: u32, child: &mut std::process::Child) {
    signal_process_group(pid, "-TERM");
    thread::sleep(Duration::from_millis(100));
    signal_process_group(pid, "-KILL");
    let _ = child.kill();
    let _ = child.wait();
}

#[cfg(unix)]
fn signal_process_group(pid: u32, signal: &str) {
    let _ = Command::new("/bin/kill")
        .args([OsString::from(signal), OsString::from(format!("-{pid}"))])
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status();
}

#[cfg(not(unix))]
fn signal_process_group(_pid: u32, _signal: &str) {}

pub fn run(invocation: &Invocation, timeout: Duration) -> Result<Value, AppError> {
    let capture = capture_process(invocation, timeout)?;
    let root = cache_root().join("process");
    fs::create_dir_all(&root).map_err(process_io)?;
    let path = root.join(format!("{}.log", operation_id()));
    write_log(
        &path,
        "process",
        "process",
        invocation,
        Some((&capture.stdout, capture.stdout_truncated)),
        Some((&capture.stderr, capture.stderr_truncated)),
    )?;
    if !capture.status.success() {
        return Err(process_error(
            "process_failed",
            format!(
                "process exited with code {:?}; log: {}",
                capture.status.code(),
                path.display()
            ),
        ));
    }
    Ok(
        json!({"process":{"status":"passed","code":capture.status.code(),"durationMs":capture.duration.as_millis(),"logPath":path,"executesProjectCode":invocation.executes_project_code}}),
    )
}

struct SummaryData {
    totals: Value,
    artifacts: Vec<Value>,
}

fn operation_summary(
    project: &Path,
    id: &str,
    kind: &str,
    status: &str,
    process: (Option<i32>, Duration),
    invocation: &Invocation,
    data: SummaryData,
) -> Value {
    let (code, duration) = process;
    let SummaryData { totals, artifacts } = data;
    json!({
        "operation": {
            "id": id,
            "kind": kind,
            "status": status,
            "project": project,
            "platform": "Mac",
            "configuration": "Development",
            "exitCode": code,
            "durationMs": duration.as_millis(),
            "executesProjectCode": invocation.executes_project_code
        },
        "totals": totals,
        "artifacts": artifacts,
        "process": {"status":status,"code":code,"durationMs":duration.as_millis(),"executesProjectCode":invocation.executes_project_code}
    })
}

pub fn view_local_operation(project: &Path, id: &str) -> Result<Value, AppError> {
    if !valid_operation_id(id) {
        return Err(AppError::usage(
            "invalid_operation_id",
            "process operation id is invalid",
            "use operation view proc-<id>",
        ));
    }
    let path = operation_directory(&canonical_project(project)?, id).join("summary.json");
    let bytes = fs::read(&path).map_err(|_| {
        process_error(
            "operation_not_found",
            format!("process operation {id} was not found for selected project"),
        )
    })?;
    serde_json::from_slice(&bytes)
        .map_err(|error| process_error("operation_malformed", error.to_string()))
}

fn persist_operation(project: &Path, summary: &Value) -> Result<(), AppError> {
    let id = summary["operation"]["id"]
        .as_str()
        .ok_or_else(|| process_error("operation_serialization_failed", "operation id missing"))?;
    let bytes = serde_json::to_vec_pretty(summary).map_err(process_json)?;
    config::atomic_write(
        &operation_directory(project, id).join("summary.json"),
        &bytes,
    )
    .map_err(process_io)?;
    config::atomic_write(&pipeline_root(project).join("latest.json"), &bytes).map_err(process_io)
}

fn retain_pipeline_log(
    project: &Path,
    id: &str,
    kind: &str,
    invocation: &Invocation,
    stdout: Option<(&[u8], bool)>,
    stderr: Option<&[u8]>,
    stderr_truncated: bool,
) -> Result<PathBuf, AppError> {
    let path = pipeline_root(project)
        .join("Logs")
        .join(format!("{id}-{kind}.log"));
    write_log(
        &path,
        id,
        kind,
        invocation,
        stdout,
        stderr.map(|bytes| (bytes, stderr_truncated)),
    )?;
    Ok(path)
}

fn write_log(
    path: &Path,
    id: &str,
    kind: &str,
    invocation: &Invocation,
    stdout: Option<(&[u8], bool)>,
    stderr: Option<(&[u8], bool)>,
) -> Result<(), AppError> {
    let mut bytes = format!(
        "operation_id={id}\nkind={kind}\nexecutable={}\narguments={:?}\n",
        invocation.executable.display(),
        invocation.arguments
    )
    .into_bytes();
    if let Some((stdout, truncated)) = stdout {
        bytes.extend_from_slice(format!("stdout_truncated={truncated}\n").as_bytes());
        bytes.extend_from_slice(stdout);
        bytes.push(b'\n');
    }
    if let Some((stderr, truncated)) = stderr {
        bytes.extend_from_slice(format!("stderr_truncated={truncated}\n").as_bytes());
        bytes.extend_from_slice(stderr);
        bytes.push(b'\n');
    }
    config::atomic_write(path, &bytes).map_err(process_io)
}

pub fn latest_log(project: &Path, lines: u16, bytes: u32) -> Result<Value, AppError> {
    let project = canonical_project(project)?;
    let files = log_files(&project)?;
    let Some(path) = files.first() else {
        return Ok(
            json!({"count":0,"total":0,"scope":"latest managed process log for selected project","lines":[]}),
        );
    };
    let content = read_log_bounded(path, bytes as usize)?;
    let all_lines = sanitized_lines(&content.bytes);
    let total = all_lines.len();
    let start = total.saturating_sub(lines as usize);
    let items = all_lines[start..]
        .iter()
        .enumerate()
        .map(|(index, text)| json!({"number":start + index + 1,"text":text}))
        .collect::<Vec<_>>();
    Ok(json!({
        "count": items.len(),
        "total": total,
        "scope": "latest managed process log for selected project",
        "log": log_identity(path),
        "truncated": content.truncated || start > 0,
        "lines": items
    }))
}

pub fn search_logs(project: &Path, query: &str, limit: u16) -> Result<Value, AppError> {
    if !(1..=1_000).contains(&query.chars().count()) || query.chars().any(char::is_control) {
        return Err(AppError::usage(
            "invalid_log_query",
            "log query must contain 1..1000 printable characters",
            "use log search <literal>",
        ));
    }
    let project = canonical_project(project)?;
    let query_lower = query.to_lowercase();
    let mut scanned = 0_u64;
    let mut total = 0_usize;
    let mut items = Vec::new();
    let mut scan_complete = true;
    for path in log_files(&project)? {
        let remaining = MAX_LOG_SCAN_BYTES.saturating_sub(scanned);
        if remaining == 0 {
            scan_complete = false;
            break;
        }
        let bound = usize::try_from(remaining.min(CAPTURE_LIMIT as u64)).unwrap_or(CAPTURE_LIMIT);
        let content = read_log_bounded(&path, bound)?;
        scanned += content.read_bytes as u64;
        scan_complete &= !content.truncated;
        let identity = log_identity(&path);
        for (index, line) in sanitized_lines(&content.bytes).into_iter().enumerate() {
            if line.to_lowercase().contains(&query_lower) {
                total += 1;
                if items.len() < limit as usize {
                    items.push(json!({
                        "operationId": identity["operationId"],
                        "operation": identity["operation"],
                        "path": path,
                        "line": index + 1,
                        "text": line
                    }));
                }
            }
        }
    }
    Ok(json!({
        "count":items.len(),
        "total":total,
        "scope":"newest 20 managed project logs; at most 8 MiB scanned",
        "query":query,
        "scannedBytes":scanned,
        "scanComplete":scan_complete,
        "items":items
    }))
}

struct BoundedFile {
    bytes: Vec<u8>,
    read_bytes: usize,
    truncated: bool,
}

fn read_log_bounded(path: &Path, limit: usize) -> Result<BoundedFile, AppError> {
    let metadata = fs::symlink_metadata(path).map_err(process_io)?;
    if metadata.file_type().is_symlink() || !metadata.is_file() {
        return Err(process_error(
            "log_path_unsafe",
            "managed log must be a regular non-symlink file",
        ));
    }
    let file = fs::File::open(path).map_err(process_io)?;
    let mut bytes = Vec::new();
    file.take(limit as u64 + 1)
        .read_to_end(&mut bytes)
        .map_err(process_io)?;
    let truncated = bytes.len() > limit;
    bytes.truncate(limit);
    Ok(BoundedFile {
        read_bytes: bytes.len(),
        bytes,
        truncated,
    })
}

fn log_files(project: &Path) -> Result<Vec<PathBuf>, AppError> {
    let directory = pipeline_root(project).join("Logs");
    if !directory.exists() {
        return Ok(Vec::new());
    }
    if fs::symlink_metadata(&directory)
        .map_err(process_io)?
        .file_type()
        .is_symlink()
    {
        return Err(process_error(
            "log_path_unsafe",
            "managed log directory is a symlink",
        ));
    }
    let mut files = fs::read_dir(directory)
        .map_err(process_io)?
        .filter_map(Result::ok)
        .filter_map(|entry| {
            let file_type = entry.file_type().ok()?;
            let path = entry.path();
            (file_type.is_file() && path.extension().and_then(OsStr::to_str) == Some("log"))
                .then_some(path)
        })
        .collect::<Vec<_>>();
    files.sort_by(|left, right| right.file_name().cmp(&left.file_name()));
    files.truncate(MAX_LOG_FILES);
    Ok(files)
}

fn log_identity(path: &Path) -> Value {
    let stem = path.file_stem().and_then(OsStr::to_str).unwrap_or("");
    let mut parts = stem.splitn(4, '-');
    let operation_id = match (parts.next(), parts.next(), parts.next(), parts.next()) {
        (Some("proc"), Some(time), Some(pid), Some(_)) => format!("proc-{time}-{pid}"),
        _ => stem.to_owned(),
    };
    let operation = stem
        .strip_prefix(&format!("{operation_id}-"))
        .unwrap_or("unknown");
    json!({"operationId":operation_id,"operation":operation,"path":path})
}

fn sanitized_lines(bytes: &[u8]) -> Vec<String> {
    sanitize_terminal(&String::from_utf8_lossy(bytes))
        .lines()
        .map(|line| line.chars().take(MAX_LOG_LINE_SCALARS).collect())
        .collect()
}

fn sanitize_terminal(text: &str) -> String {
    enum Escape {
        None,
        Start,
        Csi,
        Osc,
        OscEnd,
    }
    let mut state = Escape::None;
    let mut output = String::new();
    for character in text.chars() {
        match state {
            Escape::None if character == '\u{1b}' => state = Escape::Start,
            Escape::Start if character == '[' => state = Escape::Csi,
            Escape::Start if character == ']' => state = Escape::Osc,
            Escape::Start => state = Escape::None,
            Escape::Csi if ('@'..='~').contains(&character) => state = Escape::None,
            Escape::Csi => {}
            Escape::Osc if character == '\u{7}' => state = Escape::None,
            Escape::Osc if character == '\u{1b}' => state = Escape::OscEnd,
            Escape::Osc => {}
            Escape::OscEnd if character == '\\' => state = Escape::None,
            Escape::OscEnd => state = Escape::Osc,
            Escape::None
                if (character.is_control() && !matches!(character, '\n' | '\t'))
                    || (0x80..=0x9f).contains(&(character as u32)) => {}
            Escape::None => output.push(character),
        }
    }
    output
}

#[derive(Debug, Deserialize)]
struct AutomationReport {
    succeeded: u64,
    #[serde(rename = "succeededWithWarnings")]
    succeeded_with_warnings: u64,
    failed: u64,
    #[serde(rename = "notRun")]
    not_run: u64,
    #[serde(rename = "inProcess")]
    in_process: u64,
    #[serde(rename = "totalDuration")]
    total_duration: f64,
    tests: Vec<AutomationTest>,
}

#[derive(Debug, Deserialize)]
struct AutomationTest {
    #[serde(rename = "fullTestPath")]
    full_test_path: String,
    state: String,
    warnings: u64,
    errors: u64,
}

struct ParsedAutomationReport {
    matched: u64,
    failed: u64,
    not_run: u64,
    in_process: u64,
    totals: Value,
    notable: Vec<Value>,
}

fn automation_log_has_no_matches(path: &Path) -> Result<bool, AppError> {
    let metadata = fs::metadata(path).map_err(process_io)?;
    if metadata.len() > MAX_LIST_LOG_BYTES {
        return Err(process_error(
            "automation_log_too_large",
            "automation log exceeds 16 MiB",
        ));
    }
    Ok(fs::read_to_string(path)
        .map_err(process_io)?
        .contains("LogAutomationCommandLine: Error: No automation tests matched"))
}

fn parse_automation_report(directory: &Path) -> Result<ParsedAutomationReport, AppError> {
    let path = directory.join("index.json");
    let metadata = fs::symlink_metadata(&path).map_err(|_| {
        process_error(
            "automation_report_missing",
            format!("{} is missing", path.display()),
        )
    })?;
    if metadata.file_type().is_symlink() || !metadata.is_file() {
        return Err(process_error(
            "automation_report_invalid",
            "automation index must be a regular non-symlink file",
        ));
    }
    if metadata.len() > MAX_REPORT_BYTES {
        return Err(process_error(
            "automation_report_too_large",
            "automation index exceeds 16 MiB",
        ));
    }
    let bytes = fs::read(&path).map_err(process_io)?;
    let bytes = bytes.strip_prefix(&[0xef, 0xbb, 0xbf]).unwrap_or(&bytes);
    let report: AutomationReport = serde_json::from_slice(bytes)
        .map_err(|error| process_error("automation_report_invalid", error.to_string()))?;
    if report.tests.len() > MAX_AUTOMATION_TESTS
        || !report.total_duration.is_finite()
        || report.total_duration < 0.0
    {
        return Err(process_error(
            "automation_report_invalid",
            "automation report exceeds test bound or has invalid duration",
        ));
    }
    let matched = report.tests.len() as u64;
    if report.succeeded
        + report.succeeded_with_warnings
        + report.failed
        + report.not_run
        + report.in_process
        != matched
    {
        return Err(process_error(
            "automation_report_totals_mismatch",
            "automation totals do not equal test count",
        ));
    }
    let mut states = [0_u64; 5];
    let mut warning_count = 0_u64;
    let mut error_count = 0_u64;
    let mut notable = Vec::new();
    for test in &report.tests {
        if test.full_test_path.is_empty() || test.full_test_path.chars().count() > 1_024 {
            return Err(process_error(
                "automation_report_invalid",
                "automation test path is empty or too long",
            ));
        }
        let state = test.state.to_ascii_lowercase();
        let index = match state.as_str() {
            "success" => 0,
            "successwithwarnings" | "success with warnings" => 1,
            "fail" | "failed" => 2,
            "notrun" | "not run" => 3,
            "inprocess" | "in process" => 4,
            _ => {
                return Err(process_error(
                    "automation_report_invalid",
                    format!("unknown automation state {}", test.state),
                ));
            }
        };
        states[index] += 1;
        warning_count += test.warnings;
        error_count += test.errors;
        if (index != 0 || test.warnings > 0 || test.errors > 0) && notable.len() < 50 {
            notable.push(json!({
                "id":test.full_test_path,
                "state":test.state,
                "warnings":test.warnings,
                "errors":test.errors
            }));
        }
    }
    if states
        != [
            report.succeeded,
            report.succeeded_with_warnings,
            report.failed,
            report.not_run,
            report.in_process,
        ]
    {
        return Err(process_error(
            "automation_report_totals_mismatch",
            "automation states do not match declared totals",
        ));
    }
    Ok(ParsedAutomationReport {
        matched,
        failed: report.failed,
        not_run: report.not_run,
        in_process: report.in_process,
        totals: json!({
            "matched":matched,
            "succeeded":report.succeeded,
            "succeededWithWarnings":report.succeeded_with_warnings,
            "failed":report.failed,
            "notRun":report.not_run,
            "inProcess":report.in_process,
            "warnings":warning_count,
            "errors":error_count,
            "durationMs":report.total_duration * 1000.0,
            "complete":true
        }),
        notable,
    })
}

fn parse_automation_list(path: &Path) -> Result<Vec<String>, AppError> {
    let metadata = fs::metadata(path).map_err(|_| {
        process_error(
            "automation_list_missing",
            format!("{} is missing", path.display()),
        )
    })?;
    if metadata.len() > MAX_LIST_LOG_BYTES {
        return Err(process_error(
            "automation_list_too_large",
            "automation list log exceeds 16 MiB",
        ));
    }
    let text = fs::read_to_string(path)
        .map_err(|error| process_error("automation_list_invalid", error.to_string()))?;
    let marker = "LogAutomationCommandLine: Display:";
    let mut declared = None;
    let mut tests = Vec::new();
    for line in text.lines() {
        let Some(payload) = line.split_once(marker).map(|(_, payload)| payload.trim()) else {
            continue;
        };
        if let Some(number) = payload
            .strip_prefix("Found ")
            .and_then(|value| value.strip_suffix(" Automation Tests"))
        {
            declared = number.parse::<usize>().ok();
            continue;
        }
        if declared.is_some()
            && let Some(name) = payload
                .strip_prefix('\'')
                .and_then(|value| value.strip_suffix('\''))
        {
            if name.is_empty() || name.chars().count() > 1_024 {
                return Err(process_error(
                    "automation_list_invalid",
                    "automation list contains invalid test name",
                ));
            }
            tests.push(name.to_owned());
            if tests.len() > MAX_AUTOMATION_TESTS {
                return Err(process_error(
                    "automation_list_too_large",
                    "automation list exceeds test bound",
                ));
            }
        }
    }
    let declared = declared.ok_or_else(|| {
        process_error(
            "automation_list_invalid",
            "automation list count marker is missing",
        )
    })?;
    tests.sort();
    tests.dedup();
    if tests.len() != declared {
        return Err(process_error(
            "automation_list_invalid",
            format!(
                "automation declared {declared} tests but listed {}",
                tests.len()
            ),
        ));
    }
    Ok(tests)
}

fn validate_run_filter(filter: &str) -> Result<(), AppError> {
    if !(1..=512).contains(&filter.chars().count())
        || filter
            .chars()
            .any(|character| !(character.is_ascii_alphanumeric() || " .*/:_-".contains(character)))
    {
        return Err(AppError::usage(
            "unsafe_test_filter",
            "--filter must contain 1..512 safe automation-name characters",
            "use letters, digits, spaces, '.', '*', '/', ':', '_', and '-'",
        ));
    }
    Ok(())
}

fn validate_list_filter(filter: &str) -> Result<(), AppError> {
    if !(1..=512).contains(&filter.chars().count()) || filter.chars().any(char::is_control) {
        return Err(AppError::usage(
            "invalid_test_filter",
            "--filter must contain 1..512 printable characters",
            "use project test list --filter <literal>",
        ));
    }
    Ok(())
}

fn safe_output_path(
    path: &Path,
    allow_managed_nonempty: bool,
    project: &Path,
    engine: &EngineInfo,
) -> Result<PathBuf, AppError> {
    if path.as_os_str().is_empty() || path.components().any(|part| part == Component::ParentDir) {
        return Err(AppError::usage(
            "invalid_output_path",
            "output path must be non-empty and cannot contain '..'",
            "provide an explicit output directory",
        ));
    }
    let candidate = canonical_future_path(path)?;
    let project_root = project_root(project).canonicalize().map_err(process_io)?;
    let engine_root = engine.root.canonicalize().map_err(process_io)?;
    let home = std::env::var_os("HOME")
        .map(PathBuf::from)
        .and_then(|path| path.canonicalize().ok());
    if candidate.parent().is_none()
        || candidate == project_root
        || candidate == engine_root
        || home.as_ref().is_some_and(|home| candidate == *home)
        || candidate.starts_with(&project_root)
        || project_root.starts_with(&candidate)
        || candidate.starts_with(&engine_root)
        || engine_root.starts_with(&candidate)
    {
        return Err(process_error(
            "unsafe_output_path",
            "output must be separate from filesystem, home, project, and engine roots",
        ));
    }
    if candidate.exists() {
        let metadata = fs::symlink_metadata(&candidate).map_err(process_io)?;
        if metadata.file_type().is_symlink() || !metadata.is_dir() {
            return Err(process_error(
                "unsafe_output_path",
                "output must be a real directory, not a file or symlink",
            ));
        }
        let nonempty = fs::read_dir(&candidate)
            .map_err(process_io)?
            .next()
            .is_some();
        if nonempty {
            if !allow_managed_nonempty {
                return Err(process_error(
                    "output_not_empty",
                    format!("{} is not empty", candidate.display()),
                ));
            }
            if !candidate.join(".magi-unreal-axi-package.json").is_file() {
                return Err(process_error(
                    "output_not_managed",
                    "--force only replaces a CLI-managed package destination",
                ));
            }
        }
    }
    Ok(candidate)
}

fn canonical_future_path(path: &Path) -> Result<PathBuf, AppError> {
    let absolute = if path.is_absolute() {
        path.to_path_buf()
    } else {
        std::env::current_dir().map_err(process_io)?.join(path)
    };
    let mut existing = absolute.as_path();
    let mut missing = Vec::<OsString>::new();
    while !existing.exists() {
        let name = existing.file_name().ok_or_else(|| {
            process_error(
                "invalid_output_path",
                "output path has no existing ancestor",
            )
        })?;
        missing.push(name.to_os_string());
        existing = existing.parent().ok_or_else(|| {
            process_error(
                "invalid_output_path",
                "output path has no existing ancestor",
            )
        })?;
    }
    if fs::symlink_metadata(existing)
        .map_err(process_io)?
        .file_type()
        .is_symlink()
    {
        return Err(process_error(
            "unsafe_output_path",
            "output path existing ancestor is a symlink",
        ));
    }
    let mut canonical = existing.canonicalize().map_err(process_io)?;
    for name in missing.into_iter().rev() {
        canonical.push(name);
    }
    Ok(canonical)
}

fn prepare_empty_directory(path: &Path) -> Result<(), AppError> {
    if path.exists() {
        let metadata = fs::symlink_metadata(path).map_err(process_io)?;
        if metadata.file_type().is_symlink() || !metadata.is_dir() {
            return Err(process_error(
                "automation_report_path",
                "automation report path must be a real directory",
            ));
        }
        if fs::read_dir(path).map_err(process_io)?.next().is_some() {
            return Err(process_error(
                "automation_report_not_empty",
                "automation report directory must be empty",
            ));
        }
    } else {
        fs::create_dir_all(path).map_err(process_io)?;
    }
    Ok(())
}

fn artifact_summary(kind: &str, path: &Path) -> Result<Value, AppError> {
    let metadata = fs::symlink_metadata(path).map_err(process_io)?;
    if metadata.file_type().is_symlink() {
        return Err(process_error(
            "artifact_symlink_refused",
            format!("artifact {} is a symlink", path.display()),
        ));
    }
    if metadata.is_file() {
        return Ok(json!({"kind":kind,"path":path,"exists":true,"sizeBytes":metadata.len()}));
    }
    if !metadata.is_dir() {
        return Err(process_error(
            "artifact_invalid",
            format!("artifact {} is not a file or directory", path.display()),
        ));
    }
    let files = validate_materialization_tree(path)?;
    let mut total_bytes = 0_u64;
    let mut entries = Vec::new();
    for (file, relative) in &files {
        total_bytes = total_bytes.saturating_add(fs::metadata(file).map_err(process_io)?.len());
        if entries.len() < MAX_ARTIFACT_ENTRIES {
            entries.push(relative.display().to_string());
        }
    }
    entries.sort();
    Ok(json!({
        "kind":kind,
        "path":path,
        "exists":true,
        "fileCount":files.len(),
        "totalBytes":total_bytes,
        "inventoryComplete":true,
        "entries":entries
    }))
}

fn copy_cooked_tree(source: &Path, destination: &Path) -> Result<(), AppError> {
    let files = validate_materialization_tree(source)?;
    if files.is_empty() {
        return Err(process_error(
            "artifact_empty",
            "cooked source produced no files",
        ));
    }
    for (source_path, relative) in files {
        let target = destination.join(relative);
        if let Some(parent) = target.parent() {
            fs::create_dir_all(parent).map_err(process_io)?;
        }
        fs::copy(source_path, target).map_err(process_io)?;
    }
    Ok(())
}

fn validate_materialization_tree(source: &Path) -> Result<Vec<(PathBuf, PathBuf)>, AppError> {
    let metadata = fs::symlink_metadata(source).map_err(process_io)?;
    if metadata.file_type().is_symlink() || !metadata.is_dir() {
        return Err(process_error(
            "artifact_invalid",
            format!("cooked source {} is not a real directory", source.display()),
        ));
    }
    let mut stack = vec![(source.to_path_buf(), PathBuf::new(), 0_usize)];
    let mut files = Vec::new();
    let mut entries = 0_usize;
    while let Some((directory, relative, depth)) = stack.pop() {
        if depth > MAX_MATERIALIZE_DEPTH {
            return Err(process_error(
                "artifact_limit_exceeded",
                "cooked source exceeds maximum depth",
            ));
        }
        for entry in fs::read_dir(&directory).map_err(process_io)? {
            let entry = entry.map_err(process_io)?;
            entries += 1;
            if entries > MAX_MATERIALIZE_ENTRIES {
                return Err(process_error(
                    "artifact_limit_exceeded",
                    "cooked source exceeds maximum entries",
                ));
            }
            let file_type = entry.file_type().map_err(process_io)?;
            let rel = relative.join(entry.file_name());
            if file_type.is_symlink() || (!file_type.is_dir() && !file_type.is_file()) {
                return Err(process_error(
                    "artifact_invalid",
                    format!(
                        "cooked source contains unsafe entry {}",
                        entry.path().display()
                    ),
                ));
            }
            if file_type.is_dir() {
                stack.push((entry.path(), rel, depth + 1));
            } else {
                if files.len() >= MAX_MATERIALIZE_FILES {
                    return Err(process_error(
                        "artifact_limit_exceeded",
                        "cooked source exceeds maximum files",
                    ));
                }
                files.push((entry.path(), rel));
            }
        }
    }
    Ok(files)
}

fn diagnostic_totals(text: &str) -> (u64, u64) {
    let mut errors = 0;
    let mut warnings = 0;
    for line in text.lines() {
        let line = line.to_ascii_lowercase();
        let xcode_api_selector = line.contains(":error:]") && line.contains("error domain=");
        errors += u64::from(
            !xcode_api_selector && (line.contains("error:") || line.contains(": error ")),
        );
        warnings += u64::from(line.contains("warning:") || line.contains(": warning "));
    }
    (errors, warnings)
}

fn unique_sibling(parent: &Path, prefix: &str) -> Result<PathBuf, AppError> {
    let stamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos();
    let path = parent.join(format!("{prefix}.{}.{stamp}", std::process::id()));
    if path.exists() {
        Err(process_error(
            "temporary_path_collision",
            path.display().to_string(),
        ))
    } else {
        Ok(path)
    }
}

fn validate_project_generated_path(path: &Path, project: &Path) -> Result<(), AppError> {
    let root = project_root(project);
    let relative = path
        .strip_prefix(&root)
        .map_err(|_| process_error("artifact_invalid", "cooked path is outside project"))?;
    let mut current = root.to_path_buf();
    for component in relative.components() {
        current.push(component.as_os_str());
        let metadata = fs::symlink_metadata(&current).map_err(process_io)?;
        if metadata.file_type().is_symlink() || !metadata.is_dir() {
            return Err(process_error(
                "artifact_invalid",
                format!(
                    "generated path {} is not a real directory",
                    current.display()
                ),
            ));
        }
    }
    Ok(())
}

fn commit_fresh_directory(staging: &Path, output: &Path) -> Result<(), AppError> {
    if output.exists() {
        if fs::read_dir(output).map_err(process_io)?.next().is_some() {
            return Err(process_error(
                "output_not_empty",
                format!("{} is not empty", output.display()),
            ));
        }
        fs::remove_dir(output).map_err(process_io)?;
    }
    fs::rename(staging, output).map_err(process_io)
}

fn is_up_to_date(text: &str) -> bool {
    let text = text.to_ascii_lowercase();
    text.contains("target is up to date")
        || text.contains("no actions to execute")
        || text.contains("nothing to build")
}

fn project_stem(project: &Path) -> Result<&str, AppError> {
    project
        .file_stem()
        .and_then(OsStr::to_str)
        .filter(|name| !name.is_empty())
        .ok_or_else(|| {
            AppError::usage(
                "invalid_project_name",
                "project filename must be non-empty UTF-8",
                "rename project descriptor",
            )
        })
}

fn canonical_project(project: &Path) -> Result<PathBuf, AppError> {
    project.canonicalize().map_err(process_io)
}

fn project_root(project: &Path) -> PathBuf {
    project.parent().unwrap_or(project).to_path_buf()
}

fn operation_id() -> String {
    format!(
        "proc-{}-{}",
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos(),
        std::process::id()
    )
}

fn valid_operation_id(id: &str) -> bool {
    id.starts_with("proc-")
        && id.len() <= 128
        && id
            .chars()
            .all(|character| character.is_ascii_alphanumeric() || character == '-')
}

fn commit_managed_directory(
    staging: &Path,
    output: &Path,
) -> Result<Option<(PathBuf, String)>, AppError> {
    let backup = unique_sibling(
        output.parent().unwrap_or(Path::new(".")),
        ".magi-package-backup",
    )?;
    let had_output = output.exists();
    if had_output {
        fs::rename(output, &backup).map_err(process_io)?;
    }
    if let Err(error) = fs::rename(staging, output) {
        if had_output && let Err(rollback_error) = fs::rename(&backup, output) {
            return Err(process_error(
                "package_commit_rollback_failed",
                format!(
                    "cannot install staged package: {error}; prior package remains at {}; cannot restore it: {rollback_error}",
                    backup.display()
                ),
            ));
        }
        return Err(process_io(error));
    }
    if had_output && let Err(error) = fs::remove_dir_all(&backup) {
        return Ok(Some((backup, error.to_string())));
    }
    Ok(None)
}
fn cache_root() -> PathBuf {
    std::env::var_os("HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from("."))
        .join("Library/Caches/magi-unreal-axi")
}

fn pipeline_root(project: &Path) -> PathBuf {
    let canonical = project
        .canonicalize()
        .unwrap_or_else(|_| project.to_path_buf());
    let hash = format!(
        "{:x}",
        Sha256::digest(canonical.to_string_lossy().as_bytes())
    );
    cache_root().join("pipeline").join(hash)
}

fn operation_directory(project: &Path, id: &str) -> PathBuf {
    pipeline_root(project).join("Operations").join(id)
}

fn dotnet_environment(engine: &EngineInfo) -> BTreeMap<String, String> {
    BTreeMap::from([(
        "DOTNET_ROOT".into(),
        engine.dotnet_root.display().to_string(),
    )])
}

fn pipeline_error(reason: &'static str, message: impl Into<String>, summary: Value) -> AppError {
    process_error(reason, message).with_bridge_diagnostics(
        summary["totals"]["errors"].as_u64(),
        summary["totals"]["warnings"].as_u64(),
        Some(summary),
    )
}

fn process_error(reason: &'static str, message: impl Into<String>) -> AppError {
    AppError::operational(
        "process",
        reason,
        message,
        "magi-unreal-axi log latest --project <path>",
    )
}

fn process_io(error: std::io::Error) -> AppError {
    process_error("process_io_failed", error.to_string())
}

fn process_json(error: serde_json::Error) -> AppError {
    process_error("operation_serialization_failed", error.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn run_filter_rejects_console_injection() {
        assert!(validate_run_filter("MagiUnrealAXI.M6").is_ok());
        assert!(validate_run_filter("MagiUnrealAXI.M6;Quit").is_err());
        assert!(validate_run_filter("line\nbreak").is_err());
        assert!(validate_run_filter(&"a".repeat(513)).is_err());
    }

    #[test]
    fn automation_list_parser_requires_exact_count() {
        let temp = tempdir().unwrap();
        let path = temp.path().join("list.log");
        fs::write(
            &path,
            "x LogAutomationCommandLine: Display: Found 2 Automation Tests\n\
x LogAutomationCommandLine: Display: \t'B.Test'\n\
x LogAutomationCommandLine: Display: \t'A.Test'\n",
        )
        .unwrap();
        assert_eq!(parse_automation_list(&path).unwrap(), ["A.Test", "B.Test"]);
        fs::write(
            &path,
            "x LogAutomationCommandLine: Display: Found 2 Automation Tests\n\
x LogAutomationCommandLine: Display: \t'A.Test'\n",
        )
        .unwrap();
        assert!(parse_automation_list(&path).is_err());
    }

    #[test]
    fn real_automation_report_shape_is_normalized() {
        let temp = tempdir().unwrap();
        fs::write(
            temp.path().join("index.json"),
            r#"{"devices":[],"reportCreatedOn":"now","succeeded":1,"succeededWithWarnings":0,"failed":0,"notRun":0,"inProcess":0,"totalDuration":0.25,"tests":[{"testDisplayName":"Pass","fullTestPath":"Magi.Pass","tags":[],"state":"Success","warnings":0,"errors":0,"entries":[],"artifacts":[]}]}"#,
        )
        .unwrap();
        let parsed = parse_automation_report(temp.path()).unwrap();
        assert_eq!(parsed.matched, 1);
        assert_eq!(parsed.totals["succeeded"], 1);
        assert_eq!(parsed.totals["durationMs"], 250.0);
    }

    #[test]
    fn automation_report_rejects_inconsistent_totals() {
        let temp = tempdir().unwrap();
        fs::write(
            temp.path().join("index.json"),
            r#"{"succeeded":1,"succeededWithWarnings":0,"failed":0,"notRun":0,"inProcess":0,"totalDuration":0,"tests":[]}"#,
        )
        .unwrap();
        assert!(parse_automation_report(temp.path()).is_err());
    }

    #[test]
    fn terminal_sanitizer_removes_csi_osc_and_controls() {
        assert_eq!(
            sanitize_terminal("ok\u{1b}[31mred\u{1b}[0m\u{1b}]0;title\u{7}\u{1}"),
            "okred"
        );
    }

    #[test]
    fn diagnostic_counts_and_up_to_date_are_normalized() {
        assert_eq!(
            diagnostic_totals("a.cpp: error: bad\nb.cpp: warning: caution"),
            (1, 1)
        );
        assert_eq!(
            diagnostic_totals(
                "iOSSimulator: [SimServiceContext sharedServiceContextForDeveloperDir:error:] returned nil (Error Domain=DVTCoreSimulatorAdditionsErrorDomain Code=3)"
            ),
            (0, 0)
        );
        assert!(is_up_to_date("Target is up to date"));
    }
}
