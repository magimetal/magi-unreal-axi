#[cfg(target_os = "macos")]
mod m3_fake_bridge {
    use super::*;
    use sha2::{Digest, Sha256};
    use std::os::unix::fs::PermissionsExt;
    use std::{
        io::{Read, Write},
        net::{TcpListener, TcpStream},
        thread,
    };

    const VERSION: &str = env!("CARGO_PKG_VERSION");
    const TOKEN: &str = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    const CATALOG_HASH: &str = magi_unreal_axi::capability::CATALOG_HASH;

    fn process_start(pid: u32) -> String {
        let mut info = std::mem::MaybeUninit::<libc::proc_bsdinfo>::zeroed();
        let size = std::mem::size_of::<libc::proc_bsdinfo>();
        let read = unsafe {
            libc::proc_pidinfo(
                pid as libc::c_int,
                libc::PROC_PIDTBSDINFO,
                0,
                info.as_mut_ptr().cast(),
                size as libc::c_int,
            )
        };
        assert_eq!(read, size as libc::c_int);
        let info = unsafe { info.assume_init() };
        format!("{}:{}", info.pbi_start_tvsec, info.pbi_start_tvusec)
    }

    fn frame(stream: &mut TcpStream) -> Value {
        let mut length = [0; 4];
        stream.read_exact(&mut length).unwrap();
        let mut data = vec![0; u32::from_le_bytes(length) as usize];
        stream.read_exact(&mut data).unwrap();
        serde_json::from_slice(&data).unwrap()
    }

    fn send(stream: &mut TcpStream, value: Value) {
        let data = serde_json::to_vec(&value).unwrap();
        stream
            .write_all(&(data.len() as u32).to_le_bytes())
            .unwrap();
        stream.write_all(&data).unwrap();
    }

    fn install_record(
        harness: &Harness,
        project: &Path,
        pid: u32,
        process_start: &str,
        extra: Value,
        token: &str,
    ) -> PathBuf {
        let root = harness.home.join("Library/Caches/magi-unreal-axi");
        let project_id = format!(
            "sha256:{:x}",
            Sha256::digest(project.canonicalize().unwrap().to_str().unwrap().as_bytes())
        );
        let project_dir = root.join(project_id.trim_start_matches("sha256:"));
        let session = project_dir.join(pid.to_string());
        fs::create_dir_all(&session).unwrap();
        for path in [&root, &project_dir, &session] {
            fs::set_permissions(path, fs::Permissions::from_mode(0o700)).unwrap();
        }
        let mut discovery = json!({
            "protocol": 1, "pluginVersion": VERSION, "pid": pid, "processStart": process_start,
            "projectPath": project.canonicalize().unwrap(), "projectId": project_id,
            "engineVersion": "5.8.1", "host": "127.0.0.1", "port": 0,
            "sessionNonce": "abcdef0123456789abcdef0123456789", "startedAt": "m3"
        });
        discovery
            .as_object_mut()
            .unwrap()
            .extend(extra.as_object().cloned().unwrap_or_default());
        fs::write(
            session.join("bridge-v1.json"),
            serde_json::to_vec(&discovery).unwrap(),
        )
        .unwrap();
        fs::write(session.join("token"), token).unwrap();
        for name in ["bridge-v1.json", "token"] {
            fs::set_permissions(session.join(name), fs::Permissions::from_mode(0o600)).unwrap();
        }
        session
    }

    fn live_bridge(
        harness: &Harness,
        project: &Path,
        token: &str,
    ) -> (PathBuf, thread::JoinHandle<()>) {
        live_bridge_with_handshake(harness, project, token, None, None)
    }

    fn live_bridge_with_handshake(
        harness: &Harness,
        project: &Path,
        token: &str,
        handshake_process_start: Option<&str>,
        expected_token: Option<&str>,
    ) -> (PathBuf, thread::JoinHandle<()>) {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let port = listener.local_addr().unwrap().port();
        let pid = std::process::id();
        let process_start = process_start(pid);
        let session = install_record(
            harness,
            project,
            pid,
            &process_start,
            json!({"port": port}),
            token,
        );
        let expected_token = expected_token.unwrap_or(token).to_owned();
        let handshake_process_start = handshake_process_start
            .map(str::to_owned)
            .unwrap_or_else(|| process_start.clone());
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let handshake = frame(&mut stream);
            if handshake["token"] != expected_token {
                return;
            }
            send(
                &mut stream,
                json!({"protocol":1,"status":"ok","pluginVersion":VERSION,"pid":pid,"processStart":handshake_process_start,"sessionNonce":"abcdef0123456789abcdef0123456789","catalogHash":CATALOG_HASH}),
            );
            if handshake_process_start != process_start {
                return;
            }
            let request = frame(&mut stream);
            send(
                &mut stream,
                json!({"protocol":1,"id":request["id"],"status":"ok","result":{"state":"ready","projectId":"sha256:fixture","editorPid":pid,"levelId":"/Game/Maps/Fixture","pie":"stopped","dirtyPackageCount":0}}),
            );
        });
        (session, worker)
    }

    fn status(harness: &Harness, project: &Path) -> Output {
        harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project.to_str().unwrap(),
                "editor",
                "status",
                "--format",
                "json",
                "--timeout",
                "2",
            ],
        )
    }

    #[test]
    fn m3_real_binary_authenticated_editor_status() {
        let harness = Harness::new();
        let project = harness.project("M3", json!({}));
        let (session, worker) = live_bridge(&harness, &project, TOKEN);
        let output = status(&harness, &project);
        worker.join().unwrap();
        assert!(
            output.status.success(),
            "{}",
            String::from_utf8_lossy(&output.stdout)
        );
        assert_eq!(json_output(&output)["editor"]["state"], "ready");
        assert_eq!(
            fs::metadata(&session).unwrap().permissions().mode() & 0o777,
            0o700
        );
        assert_eq!(
            fs::metadata(session.join("token"))
                .unwrap()
                .permissions()
                .mode()
                & 0o777,
            0o600
        );
    }

    #[test]
    fn m3_wrong_token_fails_closed_without_token_output() {
        let harness = Harness::new();
        let project = harness.project("M3WrongToken", json!({}));
        let (_session, worker) = live_bridge_with_handshake(
            &harness,
            &project,
            "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210",
            None,
            Some(TOKEN),
        );
        let output = status(&harness, &project);
        worker.join().unwrap();
        assert_eq!(output.status.code(), Some(1));
        assert_eq!(json_output(&output)["error"]["reason"], "handshake_failed");
        let text = format!(
            "{}{}",
            String::from_utf8_lossy(&output.stdout),
            String::from_utf8_lossy(&output.stderr)
        );
        assert!(!text.contains("fedcba9876543210"));
    }
    #[test]
    fn m3_wrong_handshake_identity_fails_closed() {
        let harness = Harness::new();
        let project = harness.project("M3Handshake", json!({}));
        let (_session, worker) =
            live_bridge_with_handshake(&harness, &project, TOKEN, Some("wrong-session"), None);
        let output = status(&harness, &project);
        worker.join().unwrap();
        assert_eq!(
            json_output(&output)["error"]["reason"],
            "handshake_rejected"
        );
    }

    #[test]
    fn m3_wrong_project_and_version_discovery_rejected() {
        for (name, extra, reason) in [
            (
                "project",
                json!({"projectId":"sha256:wrong"}),
                "wrong_project",
            ),
            (
                "version",
                json!({"pluginVersion":"wrong"}),
                "incompatible_editor",
            ),
        ] {
            let harness = Harness::new();
            let project = harness.project(name, json!({}));
            install_record(
                &harness,
                &project,
                std::process::id(),
                &process_start(std::process::id()),
                extra,
                TOKEN,
            );
            assert_eq!(
                json_output(&status(&harness, &project))["error"]["reason"],
                reason
            );
        }
    }

    #[test]
    fn m3_stale_incompatible_record_does_not_block_live_editor() {
        let harness = Harness::new();
        let project = harness.project("M3Mixed", json!({}));
        install_record(
            &harness,
            &project,
            std::process::id() + 100_000,
            "stale",
            json!({"protocol":99, "pluginVersion":"old"}),
            TOKEN,
        );
        let (_session, worker) = live_bridge(&harness, &project, TOKEN);
        let output = status(&harness, &project);
        worker.join().unwrap();
        assert!(output.status.success());
        assert_eq!(json_output(&output)["editor"]["state"], "ready");
    }

    fn operation_bridge(
        harness: &Harness,
        project: &Path,
        result: Value,
    ) -> (std::sync::mpsc::Receiver<Value>, thread::JoinHandle<()>) {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let pid = std::process::id();
        let process_start = process_start(pid);
        install_record(
            harness,
            project,
            pid,
            &process_start,
            json!({"port":listener.local_addr().unwrap().port()}),
            TOKEN,
        );
        let (sender, receiver) = std::sync::mpsc::channel();
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let handshake = frame(&mut stream);
            assert_eq!(handshake["token"], TOKEN);
            send(
                &mut stream,
                json!({"protocol":1,"status":"ok","pluginVersion":VERSION,"pid":pid,"processStart":process_start,"sessionNonce":"abcdef0123456789abcdef0123456789","catalogHash":CATALOG_HASH}),
            );
            let request = frame(&mut stream);
            sender.send(request.clone()).unwrap();
            send(
                &mut stream,
                json!({"protocol":1,"id":request["id"],"status":"ok","result":result}),
            );
        });
        (receiver, worker)
    }

    #[test]
    fn m4_typed_and_generic_actor_list_share_validated_request_path() {
        for generic in [false, true] {
            let harness = Harness::new();
            let project = harness.project("M4ActorList", json!({}));
            let result = json!({
                "count":0,"total":0,"scope":"/Game/Maps/Fixture","items":[],
                "nextCursor":null,"revision":"0".repeat(64)
            });
            let (request, worker) = operation_bridge(&harness, &project, result);
            let project_text = project.to_str().unwrap();
            let args = if generic {
                vec![
                    "--project",
                    project_text,
                    "capability",
                    "execute",
                    "actor.list",
                    "--input-json",
                    "{\"limit\":1,\"fields\":[\"id\",\"label\"]}",
                    "--format",
                    "json",
                ]
            } else {
                vec![
                    "--project",
                    project_text,
                    "actor",
                    "list",
                    "--limit",
                    "1",
                    "--fields",
                    "id,label",
                    "--format",
                    "json",
                ]
            };
            let output = harness.command(project.parent().unwrap(), &args);
            worker.join().unwrap();
            assert!(
                output.status.success(),
                "{}",
                String::from_utf8_lossy(&output.stdout)
            );
            assert!(output.stderr.is_empty());
            assert_eq!(json_output(&output)["count"], 0);
            let request = request.recv().unwrap();
            assert_eq!(request["operation"], "actor.list");
            assert_eq!(request["args"], json!({"limit":1,"fields":["id","label"]}));
        }
    }

    #[test]
    fn m4_catalog_hash_mismatch_stops_before_operation_frame() {
        let harness = Harness::new();
        let project = harness.project("M4Hash", json!({}));
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let pid = std::process::id();
        let process_start = process_start(pid);
        install_record(
            &harness,
            &project,
            pid,
            &process_start,
            json!({"port":listener.local_addr().unwrap().port()}),
            TOKEN,
        );
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let _ = frame(&mut stream);
            send(
                &mut stream,
                json!({"protocol":1,"status":"ok","pluginVersion":VERSION,"pid":pid,"processStart":process_start,"sessionNonce":"abcdef0123456789abcdef0123456789","catalogHash":"wrong"}),
            );
            let mut byte = [0_u8; 1];
            assert_eq!(stream.read(&mut byte).unwrap(), 0);
        });
        let output = harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project.to_str().unwrap(),
                "level",
                "current",
                "--format",
                "json",
            ],
        );
        worker.join().unwrap();
        assert_eq!(output.status.code(), Some(1));
        assert_eq!(json_output(&output)["error"]["reason"], "catalog_mismatch");
        assert!(output.stderr.is_empty());
    }

    #[test]
    fn m4_capability_discovery_is_offline_compact_and_complete() {
        let harness = Harness::new();
        let search = harness.command(
            &harness.root,
            &["capability", "search", "actor", "--format", "json"],
        );
        assert!(search.status.success());
        let search = json_output(&search);
        assert_eq!(search["count"], 7);
        assert!(search["items"][0].get("inputSchema").is_none());
        assert_eq!(search["items"][0]["availability"], "unknown");
        assert_eq!(search["items"][0]["reasons"][0]["code"], "editor_offline");

        let describe = harness.command(
            &harness.root,
            &["capability", "describe", "actor.list", "--format", "json"],
        );
        assert!(describe.status.success());
        let describe = json_output(&describe);
        assert_eq!(describe["capability"]["id"], "actor.list");
        assert!(
            describe["capability"]["inputSchema"]
                .as_str()
                .is_some_and(|schema| schema.contains("object"))
        );
        assert_eq!(describe["runtime"]["catalogHash"], CATALOG_HASH);
        assert_eq!(describe["runtime"]["availability"], "unknown");
        assert_eq!(describe["runtime"]["reasons"][0]["code"], "editor_offline");
    }

    #[test]
    fn m5_delete_without_force_fails_before_bridge_connection() {
        let harness = Harness::new();
        let project = harness.project("M5DeleteGuard", json!({}));
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        listener.set_nonblocking(true).unwrap();
        let pid = std::process::id();
        install_record(
            &harness,
            &project,
            pid,
            &process_start(pid),
            json!({"port":listener.local_addr().unwrap().port()}),
            TOKEN,
        );
        let output = harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project.to_str().unwrap(),
                "actor",
                "delete",
                "/Game/M5#00000000-0000-0000-0000-000000000001",
                "--expected-revision",
                &"0".repeat(64),
                "--format",
                "json",
            ],
        );
        assert_eq!(output.status.code(), Some(2));
        assert_eq!(json_output(&output)["error"]["reason"], "force_required");
        assert!(output.stderr.is_empty());
        assert_eq!(
            listener.accept().unwrap_err().kind(),
            std::io::ErrorKind::WouldBlock
        );
    }

    #[test]
    fn m5_ambiguous_disconnect_resolves_by_operation_view() {
        let harness = Harness::new();
        let project = harness.project("M5Disconnect", json!({}));
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let pid = std::process::id();
        let process_start = process_start(pid);
        install_record(
            &harness,
            &project,
            pid,
            &process_start,
            json!({"port":listener.local_addr().unwrap().port()}),
            TOKEN,
        );
        let worker = thread::spawn(move || {
            let (mut first, _) = listener.accept().unwrap();
            assert_eq!(frame(&mut first)["token"], TOKEN);
            send(
                &mut first,
                json!({"protocol":1,"status":"ok","pluginVersion":VERSION,"pid":pid,"processStart":process_start,"sessionNonce":"abcdef0123456789abcdef0123456789","catalogHash":CATALOG_HASH}),
            );
            let mutation = frame(&mut first);
            assert_eq!(mutation["operation"], "actor.spawn");
            let operation_id = mutation["id"].as_str().unwrap().to_owned();
            drop(first);

            let (mut second, _) = listener.accept().unwrap();
            assert_eq!(frame(&mut second)["token"], TOKEN);
            send(
                &mut second,
                json!({"protocol":1,"status":"ok","pluginVersion":VERSION,"pid":pid,"processStart":process_start,"sessionNonce":"abcdef0123456789abcdef0123456789","catalogHash":CATALOG_HASH}),
            );
            let lookup = frame(&mut second);
            assert_eq!(lookup["operation"], "operation.view");
            assert_eq!(lookup["args"]["id"], operation_id);
            let receipt = json!({
                "operationId":operation_id,
                "operation":"actor.spawn",
                "state":"completed",
                "projectId":"sha256:fixture",
                "editorPid":pid,
                "target":"/Game/M5#00000000-0000-0000-0000-000000000001",
                "changed":true,
                "transaction":"atomic",
                "reversibility":"source-control",
                "dirtyPackages":["/Game/M5"],
                "savedPackages":[],
                "revision":"0".repeat(64),
                "persistence":"dirty",
                "verification":{"readback":"actor.view","target":"/Game/M5#00000000-0000-0000-0000-000000000001","matched":true,"exists":true}
            });
            send(
                &mut second,
                json!({"protocol":1,"id":lookup["id"],"status":"ok","result":receipt}),
            );
        });

        let project_text = project.to_str().unwrap();
        let mutation = harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project_text,
                "--timeout",
                "2",
                "actor",
                "spawn",
                "--level",
                "/Game/M5",
                "--class",
                "/Script/Engine.StaticMeshActor",
                "--agent-key",
                "disconnect-proof",
                "--format",
                "json",
            ],
        );
        assert_eq!(mutation.status.code(), Some(1));
        let mutation = json_output(&mutation);
        assert_eq!(mutation["error"]["reason"], "outcome_unknown");
        let operation_id = mutation["error"]["operationId"].as_str().unwrap();
        assert!(
            mutation["error"]["help"]
                .as_str()
                .unwrap()
                .contains(operation_id)
        );

        let lookup = harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project_text,
                "--timeout",
                "2",
                "operation",
                "view",
                operation_id,
                "--format",
                "json",
            ],
        );
        worker.join().unwrap();
        assert!(
            lookup.status.success(),
            "{}",
            String::from_utf8_lossy(&lookup.stdout)
        );
        assert_eq!(json_output(&lookup)["operationId"], operation_id);
    }

    #[test]
    fn p10_live_describe_merges_runtime_availability() {
        let harness = Harness::new();
        let project = harness.project("P10Availability", json!({}));
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let pid = std::process::id();
        let process_start = process_start(pid);
        install_record(
            &harness,
            &project,
            pid,
            &process_start,
            json!({"port":listener.local_addr().unwrap().port()}),
            TOKEN,
        );
        let native = magi_unreal_axi::capability::CAPABILITY_METADATA
            .iter()
            .filter(|metadata| metadata.execution == "native")
            .map(|metadata| {
                if metadata.id == "blueprint.compile" {
                    json!({"operation":metadata.id,"availability":"unavailable","reasons":[{"code":"missing_module","subject":"KismetCompiler","message":"Required module KismetCompiler is not loaded"}]})
                } else {
                    json!({"operation":metadata.id,"availability":"available","reasons":[]})
                }
            })
            .collect::<Vec<_>>();
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            assert_eq!(frame(&mut stream)["token"], TOKEN);
            send(
                &mut stream,
                json!({"protocol":1,"status":"ok","pluginVersion":VERSION,"pid":pid,"processStart":process_start,"sessionNonce":"abcdef0123456789abcdef0123456789","catalogHash":CATALOG_HASH}),
            );
            let request = frame(&mut stream);
            assert_eq!(request["operation"], "bridge.describe");
            send(
                &mut stream,
                json!({"protocol":1,"id":request["id"],"status":"ok","result":{"protocol":1,"catalogHash":CATALOG_HASH,"operations":[],"nativeOperations":native}}),
            );
        });
        let output = harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project.to_str().unwrap(),
                "capability",
                "describe",
                "blueprint.compile",
                "--format",
                "json",
            ],
        );
        worker.join().unwrap();
        assert!(
            output.status.success(),
            "{}",
            String::from_utf8_lossy(&output.stdout)
        );
        let output = json_output(&output);
        assert_eq!(output["runtime"]["availability"], "unavailable");
        assert_eq!(output["runtime"]["reasons"][0]["code"], "missing_module");
    }

    #[test]
    fn p10_failed_compile_receipt_renders_and_recovers_offline() {
        let harness = Harness::new();
        let project = harness.project("P10CompileFailure", json!({}));
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let pid = std::process::id();
        let process_start = process_start(pid);
        let session = install_record(
            &harness,
            &project,
            pid,
            &process_start,
            json!({"port":listener.local_addr().unwrap().port()}),
            TOKEN,
        );
        let project_id = format!(
            "sha256:{:x}",
            Sha256::digest(project.canonicalize().unwrap().to_str().unwrap().as_bytes())
        );
        let before = "a".repeat(64);
        let before_server = before.clone();
        let target = "/Game/BP.BP";
        let worker = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            assert_eq!(frame(&mut stream)["token"], TOKEN);
            send(
                &mut stream,
                json!({"protocol":1,"status":"ok","pluginVersion":VERSION,"pid":pid,"processStart":process_start,"sessionNonce":"abcdef0123456789abcdef0123456789","catalogHash":CATALOG_HASH}),
            );
            let request = frame(&mut stream);
            assert_eq!(request["operation"], "blueprint.compile");
            assert_eq!(request["expectedRevision"], before_server);
            let operation_id = request["id"].as_str().unwrap();
            let diagnostics = json!([{"severity":"error","message":"invalid graph","graph":"/Game/BP.BP:Graph","nodeGuid":"00000000-0000-0000-0000-000000000001","nodeTitle":"Broken"}]);
            let receipt = json!({
                "operationId":operation_id,"operation":"blueprint.compile","state":"failed",
                "projectId":project_id,"editorPid":pid,"target":target,"changed":false,
                "transaction":"non-atomic","reversibility":"source-control",
                "dirtyPackages":["/Game/BP"],"savedPackages":[],"revision":before_server,"persistence":"dirty",
                "verification":{"readback":"blueprint.view","target":target,"matched":true,
                    "beforeRevision":before_server,"observedRevision":before_server,"observedStatus":"error",
                    "failureType":"blueprint_compile_failed","errorCount":1,"warningCount":0,
                    "diagnostics":diagnostics,"changedObjects":[]}
            });
            send(
                &mut stream,
                json!({"protocol":1,"id":operation_id,"status":"error",
                "error":{"type":"blueprint_compile_failed","message":"Blueprint compile failed","retryable":false,
                    "dirtyPackageCount":1,"dirtyPackages":["/Game/BP"],"errorCount":1,"warningCount":0,"diagnostics":diagnostics},
                "receipt":receipt}),
            );
        });
        let project_text = project.to_str().unwrap();
        let output = harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project_text,
                "blueprint",
                "compile",
                target,
                "--expected-revision",
                &before,
                "--format",
                "json",
            ],
        );
        worker.join().unwrap();
        assert_eq!(output.status.code(), Some(1));
        let output = json_output(&output);
        assert_eq!(output["error"]["reason"], "blueprint_compile_failed");
        assert_eq!(output["error"]["retryable"], false);
        let receipt = &output["error"]["receipt"];
        assert_eq!(receipt["state"], "failed");
        assert_eq!(receipt["transaction"], "non-atomic");
        assert_eq!(receipt["persistence"], "dirty");
        assert_eq!(receipt["savedPackages"], json!([]));
        assert_eq!(receipt["dirtyPackages"], json!(["/Game/BP"]));
        assert_eq!(receipt["revision"], before);
        assert_eq!(receipt["verification"]["beforeRevision"], before);
        assert_eq!(receipt["verification"]["observedRevision"], before);
        assert_eq!(receipt["verification"]["observedStatus"], "error");
        assert_eq!(receipt["verification"]["changedObjects"], json!([]));
        assert_eq!(
            receipt["verification"]["diagnostics"][0]["severity"],
            "error"
        );
        assert_eq!(
            receipt["verification"]["failureType"],
            "blueprint_compile_failed"
        );
        let operation_id = output["error"]["operationId"].as_str().unwrap();
        fs::remove_dir_all(session).unwrap();
        let lookup = harness.command(
            project.parent().unwrap(),
            &[
                "--project",
                project_text,
                "operation",
                "view",
                operation_id,
                "--format",
                "json",
            ],
        );
        assert!(
            lookup.status.success(),
            "{}",
            String::from_utf8_lossy(&lookup.stdout)
        );
        assert_eq!(json_output(&lookup), output["error"]["receipt"]);
    }
}
use serde_json::{Value, json};
use std::{
    fs,
    path::{Path, PathBuf},
    process::{Command, Output},
    time::{Duration, Instant},
};
use tempfile::TempDir;

struct Harness {
    _temp: TempDir,
    root: PathBuf,
    home: PathBuf,
    engine_discovery_root: PathBuf,
}

impl Harness {
    fn new() -> Self {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path().join("work");
        let home = temp.path().join("home");
        let engine_discovery_root = temp.path().join("engines");
        fs::create_dir_all(&root).unwrap();
        fs::create_dir_all(&home).unwrap();
        fs::create_dir_all(&engine_discovery_root).unwrap();
        Self {
            _temp: temp,
            root,
            home,
            engine_discovery_root,
        }
    }

    fn command(&self, cwd: &Path, args: &[&str]) -> Output {
        Command::new(env!("CARGO_BIN_EXE_magi-unreal-axi"))
            .args(args)
            .current_dir(cwd)
            .env_clear()
            .env("HOME", &self.home)
            .env("XDG_CONFIG_HOME", self.home.join("config"))
            .env("PATH", "/usr/bin:/bin")
            .env(
                "MAGI_UNREAL_ENGINE_DISCOVERY_ROOT",
                &self.engine_discovery_root,
            )
            .output()
            .expect("binary runs")
    }

    fn project(&self, name: &str, extra: Value) -> PathBuf {
        let project = self.root.join(format!("{name}.uproject"));
        let mut descriptor = json!({"FileVersion": 3, "EngineAssociation": "5.8"});
        descriptor
            .as_object_mut()
            .unwrap()
            .extend(extra.as_object().cloned().unwrap_or_default());
        fs::write(&project, serde_json::to_vec_pretty(&descriptor).unwrap()).unwrap();
        project
    }

    fn fake_engine(&self, ubt_script: &str) -> PathBuf {
        self.fake_engine_at(self.root.join("FakeEngine"), ubt_script)
    }

    fn fake_pipeline_engine(&self, ubt: &str, editor: &str, uat: &str) -> PathBuf {
        let root = self.fake_engine(ubt);
        executable(&root.join("Engine/Binaries/Mac/UnrealEditor-Cmd"), editor);
        executable(&root.join("Engine/Build/BatchFiles/RunUAT.sh"), uat);
        root
    }

    fn fake_engine_at(&self, root: PathBuf, ubt_script: &str) -> PathBuf {
        fs::create_dir_all(root.join("Engine/Build/BatchFiles")).unwrap();
        fs::create_dir_all(root.join("Engine/Binaries/Mac")).unwrap();
        fs::create_dir_all(root.join("Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64")).unwrap();
        fs::create_dir_all(root.join("Engine/Binaries/DotNET/UnrealBuildTool")).unwrap();
        fs::write(
            root.join("Engine/Build/Build.version"),
            r#"{"MajorVersion":5,"MinorVersion":8,"PatchVersion":1,"Changelist":56057345}"#,
        )
        .unwrap();
        for relative in [
            "Engine/Binaries/Mac/UnrealEditor",
            "Engine/Binaries/Mac/UnrealEditor-Cmd",
        ] {
            executable(&root.join(relative), "#!/bin/sh\nexit 0\n");
        }
        executable(
            &root.join("Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64/dotnet"),
            "#!/bin/sh\nexit 0\n",
        );
        executable(
            &root.join("Engine/Build/BatchFiles/RunUAT.sh"),
            r##"#!/bin/sh
plugin=""
package=""
for arg in "$@"; do
  case "$arg" in
    -Plugin=*) plugin=${arg#-Plugin=} ;;
    -Package=*) package=${arg#-Package=} ;;
  esac
done
[ -n "$plugin" ] && [ -n "$package" ] || exit 2
mkdir -p "$package/Binaries/Mac"
cp -R "$(dirname "$plugin")/." "$package/"
printf fake > "$package/Binaries/Mac/libUnrealEditor-MagiUnrealAXI.dylib"
"##,
        );
        executable(
            &root.join("Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool"),
            ubt_script,
        );
        root
    }
}

fn executable(path: &Path, content: &str) {
    fs::write(path, content).unwrap();
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        fs::set_permissions(path, fs::Permissions::from_mode(0o755)).unwrap();
    }
}

fn json_output(output: &Output) -> Value {
    serde_json::from_slice(&output.stdout).unwrap_or_else(|error| {
        panic!(
            "invalid JSON ({error}): {}",
            String::from_utf8_lossy(&output.stdout)
        )
    })
}

#[test]
fn outside_project_home_is_hermetic_and_explicit() {
    let harness = Harness::new();
    let output = harness.command(&harness.root, &["--format", "json"]);
    assert!(output.status.success());
    assert!(output.stderr.is_empty());
    let value = json_output(&output);
    assert_eq!(value["project"]["found"], false);
    assert!(
        value["project"]["help"]
            .as_str()
            .unwrap()
            .contains("--project")
    );
}

#[test]
fn usage_errors_follow_toon_default_and_selected_json() {
    let harness = Harness::new();
    let toon = harness.command(&harness.root, &["--not-a-real-flag"]);
    assert_eq!(toon.status.code(), Some(2));
    assert!(toon.stderr.is_empty());
    assert!(String::from_utf8_lossy(&toon.stdout).starts_with("error:\n"));

    let json = harness.command(
        &harness.root,
        &["project", "view", "--format", "json", "--not-a-real-flag"],
    );
    assert_eq!(json.status.code(), Some(2));
    assert_eq!(json_output(&json)["error"]["code"], 2);

    let typo = harness.command(&harness.root, &["projec", "--format", "json"]);
    assert_eq!(typo.status.code(), Some(2));
    assert!(
        json_output(&typo)["error"]["help"]
            .as_str()
            .unwrap()
            .contains("project")
    );
}

#[test]
fn help_and_version_bypass_poisoned_environment() {
    let harness = Harness::new();
    for arg in ["--help", "--version"] {
        let output = Command::new(env!("CARGO_BIN_EXE_magi-unreal-axi"))
            .arg(arg)
            .current_dir(&harness.root)
            .env_clear()
            .env("MAGI_UNREAL_PROJECT", "")
            .env("MAGI_UNREAL_ENGINE", "")
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "{}",
            String::from_utf8_lossy(&output.stderr)
        );
        assert!(output.stderr.is_empty());
    }
}

#[test]
fn explicit_project_outranks_empty_environment_and_ambiguity_is_error() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    let explicit = Command::new(env!("CARGO_BIN_EXE_magi-unreal-axi"))
        .args([
            "--project",
            project.to_str().unwrap(),
            "project",
            "view",
            "--format",
            "json",
        ])
        .current_dir(&harness.root)
        .env_clear()
        .env("HOME", &harness.home)
        .env("MAGI_UNREAL_PROJECT", "")
        .output()
        .unwrap();
    assert!(
        explicit.status.success(),
        "{}",
        String::from_utf8_lossy(&explicit.stdout)
    );

    harness.project("Other", json!({}));
    let ambiguous = harness.command(&harness.root, &["project", "view", "--format", "json"]);
    assert_eq!(ambiguous.status.code(), Some(1));
    assert_eq!(
        json_output(&ambiguous)["error"]["reason"],
        "ambiguous_project"
    );
}

#[test]
fn malformed_project_config_is_actionable_operational_error() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    fs::create_dir_all(harness.root.join(".magi")).unwrap();
    fs::write(harness.root.join(".magi/unreal-axi.toml"), "engine = [").unwrap();
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "project",
            "view",
            "--format",
            "json",
        ],
    );
    assert_eq!(output.status.code(), Some(1));
    let error = json_output(&output);
    assert_eq!(error["error"]["reason"], "config_malformed");
    assert!(
        error["error"]["message"]
            .as_str()
            .unwrap()
            .contains("unreal-axi.toml")
    );
}

#[test]
fn long_descriptor_strings_truncate_and_full_restores_them() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({"Description": "x".repeat(1001)}));
    let args = [
        "--project",
        project.to_str().unwrap(),
        "project",
        "view",
        "--format",
        "json",
    ];
    let truncated = json_output(&harness.command(&harness.root, &args));
    assert!(
        truncated["project"]["descriptor"]["Description"]
            .as_str()
            .unwrap()
            .contains("original scalar count: 1001")
    );
    assert!(
        truncated["help"]
            .as_array()
            .unwrap()
            .iter()
            .any(|value| value.as_str().unwrap().contains("--full"))
    );

    let full = json_output(&harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "project",
            "view",
            "--format",
            "json",
            "--full",
        ],
    ));
    assert_eq!(
        full["project"]["descriptor"]["Description"]
            .as_str()
            .unwrap()
            .len(),
        1001
    );
}

#[test]
fn embedded_plugin_install_is_dry_run_safe_idempotent_and_preserves_descriptor() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({"Unrelated": {"keep": true}}));
    let engine = harness.fake_engine("#!/bin/sh\nexit 0\n");
    let prefix = [
        "--project",
        project.to_str().unwrap(),
        "--engine",
        engine.to_str().unwrap(),
        "setup",
        "plugin",
    ];
    let dry = harness.command(
        &harness.root,
        &[
            prefix[0],
            prefix[1],
            prefix[2],
            prefix[3],
            prefix[4],
            prefix[5],
            "install",
            "--dry-run",
            "--format",
            "json",
        ],
    );
    assert!(
        dry.status.success(),
        "{}",
        String::from_utf8_lossy(&dry.stdout)
    );
    assert!(!harness.root.join("Plugins").exists());
    assert_eq!(json_output(&dry)["projectDescriptor"]["wouldChange"], true);

    let install = harness.command(
        &harness.root,
        &[
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], "install",
            "--format", "json",
        ],
    );
    assert!(
        install.status.success(),
        "{}",
        String::from_utf8_lossy(&install.stdout)
    );
    assert_eq!(json_output(&install)["plugin"]["changed"], true);
    assert_eq!(json_output(&install)["projectDescriptor"]["changed"], true);
    assert!(
        harness
            .root
            .join("Plugins/MagiUnrealAXI/Binaries/Mac/libUnrealEditor-MagiUnrealAXI.dylib")
            .is_file()
    );
    let descriptor: Value = serde_json::from_slice(&fs::read(&project).unwrap()).unwrap();
    assert_eq!(descriptor["Unrelated"]["keep"], true);
    assert_eq!(descriptor["Plugins"][0]["Name"], "MagiUnrealAXI");

    let again = harness.command(
        &harness.root,
        &[
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], "install",
            "--format", "json",
        ],
    );
    assert!(again.status.success());
    assert_eq!(json_output(&again)["plugin"]["changed"], false);
    assert_eq!(json_output(&again)["projectDescriptor"]["changed"], false);
}

#[test]
fn plugin_modified_tree_is_refused_and_forced_copy_is_backed_up() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    let engine = harness.fake_engine("#!/bin/sh\nexit 0\n");
    let project_arg = project.to_str().unwrap();
    let engine_arg = engine.to_str().unwrap();
    assert!(
        harness
            .command(
                &harness.root,
                &[
                    "--project",
                    project_arg,
                    "--engine",
                    engine_arg,
                    "setup",
                    "plugin",
                    "install",
                    "--format",
                    "json"
                ]
            )
            .status
            .success()
    );
    fs::write(harness.root.join("Plugins/MagiUnrealAXI/user.txt"), "keep").unwrap();
    let refused = harness.command(
        &harness.root,
        &[
            "--project",
            project_arg,
            "--engine",
            engine_arg,
            "setup",
            "plugin",
            "update",
            "--format",
            "json",
        ],
    );
    assert_eq!(refused.status.code(), Some(1));
    assert_eq!(json_output(&refused)["error"]["reason"], "plugin_modified");
    let forced = harness.command(
        &harness.root,
        &[
            "--project",
            project_arg,
            "--engine",
            engine_arg,
            "setup",
            "plugin",
            "update",
            "--force",
            "--format",
            "json",
        ],
    );
    assert!(
        forced.status.success(),
        "{}",
        String::from_utf8_lossy(&forced.stdout)
    );
    let backup = json_output(&forced)["plugin"]["backup"]
        .as_str()
        .unwrap()
        .to_owned();
    assert_eq!(
        fs::read_to_string(Path::new(&backup).join("user.txt")).unwrap(),
        "keep"
    );
    let uninstall = harness.command(
        &harness.root,
        &[
            "--project",
            project_arg,
            "setup",
            "plugin",
            "uninstall",
            "--format",
            "json",
        ],
    );
    assert!(uninstall.status.success());
    let restored: Value = serde_json::from_slice(&fs::read(&project).unwrap()).unwrap();
    assert!(restored["Plugins"].as_array().unwrap().is_empty());
}

#[test]
fn forced_modified_uninstall_restores_managed_descriptor_and_preserves_backup() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    let engine = harness.fake_engine("#!/bin/sh\nexit 0\n");
    let prefix = [
        "--project",
        project.to_str().unwrap(),
        "--engine",
        engine.to_str().unwrap(),
        "setup",
        "plugin",
    ];
    assert!(
        harness
            .command(
                &harness.root,
                &[
                    prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], "install",
                    "--format", "json"
                ]
            )
            .status
            .success()
    );
    fs::write(
        harness.root.join("Plugins/MagiUnrealAXI/user.txt"),
        "preserve",
    )
    .unwrap();
    let output = harness.command(
        &harness.root,
        &[
            prefix[0],
            prefix[1],
            prefix[2],
            prefix[3],
            prefix[4],
            prefix[5],
            "uninstall",
            "--force",
            "--format",
            "json",
        ],
    );
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stdout)
    );
    let value = json_output(&output);
    let backup = PathBuf::from(value["plugin"]["backup"].as_str().unwrap());
    assert_eq!(
        fs::read_to_string(backup.join("user.txt")).unwrap(),
        "preserve"
    );
    let descriptor: Value = serde_json::from_slice(&fs::read(project).unwrap()).unwrap();
    assert!(descriptor["Plugins"].as_array().unwrap().is_empty());
}

#[test]
fn project_config_format_and_invalid_editor_selector_follow_contract() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    fs::create_dir_all(harness.root.join(".magi")).unwrap();
    fs::write(
        harness.root.join(".magi/unreal-axi.toml"),
        "format = \"json\"\n",
    )
    .unwrap();
    let output = harness.command(
        &harness.root,
        &["--project", project.to_str().unwrap(), "project", "view"],
    );
    assert!(output.status.success());
    assert_eq!(json_output(&output)["project"]["name"], "Game");
    let invalid = harness.command(&harness.root, &["--editor", "0", "--format", "json"]);
    assert_eq!(invalid.status.code(), Some(2));
    assert_eq!(json_output(&invalid)["error"]["type"], "usage");
}

#[cfg(unix)]
#[test]
fn plugin_setup_rejects_symlinked_plugins_directory() {
    use std::os::unix::fs::symlink;
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    let engine = harness.fake_engine("#!/bin/sh\nexit 0\n");
    let external = harness.home.join("external");
    fs::create_dir(&external).unwrap();
    symlink(&external, harness.root.join("Plugins")).unwrap();
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "setup",
            "plugin",
            "install",
            "--format",
            "json",
        ],
    );
    assert_eq!(output.status.code(), Some(1));
    assert_eq!(json_output(&output)["error"]["reason"], "symlink_refused");
    assert!(fs::read_dir(external).unwrap().next().is_none());
}

#[test]
fn fake_engine_dry_run_has_exact_build_contract_and_dotnet_root() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    let engine = harness.fake_engine("#!/bin/sh\nexit 0\n");
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "build",
            "--dry-run",
            "--format",
            "json",
        ],
    );
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stdout)
    );
    let invocation = &json_output(&output)["invocation"];
    assert!(
        invocation["executable"]
            .as_str()
            .unwrap()
            .ends_with("UnrealBuildTool")
    );
    assert_eq!(invocation["arguments"][0], "GameEditor");
    assert_eq!(invocation["arguments"][1], "Mac");
    assert!(
        invocation["environment"]["DOTNET_ROOT"]
            .as_str()
            .unwrap()
            .ends_with("10.0/mac-arm64")
    );
}

#[test]
fn process_timeout_is_structured_and_child_output_never_reaches_axi_stdout() {
    let harness = Harness::new();
    let project = harness.project("Game", json!({}));
    let engine = harness.fake_engine(
        "#!/bin/sh\necho CHILD-NOISE\n(sh -c \"trap '' TERM; /bin/sleep 5\") &\nexit 0\n",
    );
    let started = Instant::now();
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "--timeout",
            "1",
            "project",
            "build",
            "--format",
            "json",
        ],
    );
    assert_eq!(output.status.code(), Some(1));
    assert!(output.stderr.is_empty());
    let value = json_output(&output);
    assert_eq!(value["error"]["reason"], "process_timeout");
    assert!(!String::from_utf8_lossy(&output.stdout).contains("CHILD-NOISE"));
    assert!(
        started.elapsed() < Duration::from_secs(3),
        "timeout took {:?}",
        started.elapsed()
    );
}

#[test]
fn pre_enabled_plugin_descriptor_is_reported_unchanged() {
    let harness = Harness::new();
    let project = harness.project(
        "Enabled",
        json!({"Plugins":[{"Name":"MagiUnrealAXI","Enabled":true,"Keep":"value"}]}),
    );
    let before = fs::read(&project).unwrap();
    let engine = harness.fake_engine("#!/bin/sh\nexit 0\n");
    let args = [
        "--project",
        project.to_str().unwrap(),
        "--engine",
        engine.to_str().unwrap(),
        "setup",
        "plugin",
        "install",
        "--format",
        "json",
    ];
    let install = harness.command(&harness.root, &args);
    assert!(install.status.success());
    assert_eq!(json_output(&install)["projectDescriptor"]["changed"], false);
    assert_eq!(fs::read(&project).unwrap(), before);

    let uninstall = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "setup",
            "plugin",
            "uninstall",
            "--format",
            "json",
        ],
    );
    assert!(uninstall.status.success());
    assert_eq!(
        json_output(&uninstall)["projectDescriptor"]["changed"],
        false
    );
    assert_eq!(fs::read(&project).unwrap(), before);
}

#[test]
fn engine_discovery_is_hermetic_and_ambiguity_is_structured() {
    let harness = Harness::new();
    let empty = harness.command(&harness.home, &["engine", "list", "--format", "json"]);
    assert!(empty.status.success());
    assert_eq!(json_output(&empty)["count"], 0);

    harness.fake_engine_at(
        harness.engine_discovery_root.join("UE_A"),
        "#!/bin/sh\nexit 0\n",
    );
    harness.fake_engine_at(
        harness.engine_discovery_root.join("UE_B"),
        "#!/bin/sh\nexit 0\n",
    );
    let ambiguous = harness.command(&harness.home, &["engine", "view", "--format", "json"]);
    assert_eq!(ambiguous.status.code(), Some(1));
    assert_eq!(
        json_output(&ambiguous)["error"]["reason"],
        "ambiguous_engine"
    );
}

#[test]
fn fake_ubt_receives_exact_build_argv_cwd_and_dotnet_root() {
    let harness = Harness::new();
    let project = harness.project("Exact", json!({}));
    let capture = harness.root.join("ubt-capture.txt");
    let script = format!(
        "#!/bin/sh\n{{ printf 'cwd=%s\\n' \"$PWD\"; printf 'arg=%s\\n' \"$@\"; printf 'dotnet=%s\\n' \"$DOTNET_ROOT\"; }} > '{}'\n",
        capture.display()
    );
    let engine = harness.fake_engine(&script);
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "build",
            "--format",
            "json",
        ],
    );
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stdout)
    );
    let lines = fs::read_to_string(capture).unwrap();
    assert_eq!(
        lines.lines().collect::<Vec<_>>(),
        vec![
            format!("cwd={}", harness.root.canonicalize().unwrap().display()),
            "arg=ExactEditor".to_owned(),
            "arg=Mac".to_owned(),
            "arg=Development".to_owned(),
            format!("arg=-Project={}", project.canonicalize().unwrap().display()),
            "arg=-NoUBTMakefiles".to_owned(),
            "arg=-WaitMutex".to_owned(),
            "arg=-utf8output".to_owned(),
            format!(
                "dotnet={}",
                engine
                    .canonicalize()
                    .unwrap()
                    .join("Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64")
                    .display()
            ),
        ]
    );
}

#[cfg(unix)]
#[test]
fn descriptor_write_failure_rolls_back_new_plugin_install() {
    use std::os::unix::fs::PermissionsExt;

    let harness = Harness::new();
    let project = harness.project("Rollback", json!({"Keep":true}));
    let engine = harness.fake_engine("#!/bin/sh\nexit 0\n");
    fs::create_dir_all(harness.root.join("Plugins")).unwrap();
    let before = fs::read(&project).unwrap();
    fs::set_permissions(&harness.root, fs::Permissions::from_mode(0o555)).unwrap();
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "setup",
            "plugin",
            "install",
            "--format",
            "json",
        ],
    );
    fs::set_permissions(&harness.root, fs::Permissions::from_mode(0o755)).unwrap();
    assert_eq!(output.status.code(), Some(1));
    assert_eq!(fs::read(&project).unwrap(), before);
    assert!(!harness.root.join("Plugins/MagiUnrealAXI").exists());
}

#[test]
fn m7_all_pipeline_dry_runs_are_exact_and_side_effect_free() {
    let harness = Harness::new();
    let project = harness.project("M7Dry", json!({}));
    let engine = harness.fake_pipeline_engine(
        "#!/bin/sh\nexit 0\n",
        "#!/bin/sh\nexit 0\n",
        "#!/bin/sh\nexit 0\n",
    );
    let output = harness.home.join("m7-output");
    let cases = [
        vec!["project", "build", "--dry-run"],
        vec!["project", "test", "list", "--dry-run"],
        vec![
            "project",
            "test",
            "run",
            "--filter",
            "MagiUnrealAXI.M6",
            "--dry-run",
        ],
        vec![
            "project",
            "cook",
            "--output",
            output.to_str().unwrap(),
            "--dry-run",
        ],
        vec![
            "project",
            "package",
            "--output",
            output.to_str().unwrap(),
            "--dry-run",
        ],
    ];
    for command in cases {
        let mut args = vec![
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
        ];
        args.extend(command);
        args.extend(["--format", "json"]);
        let result = harness.command(&harness.root, &args);
        assert!(
            result.status.success(),
            "{}",
            String::from_utf8_lossy(&result.stdout)
        );
        let value = json_output(&result);
        assert_eq!(value["dryRun"], true);
        assert_eq!(value["invocation"]["executesProjectCode"], true);
        assert!(value["invocation"]["executable"].is_string());
        assert!(value["invocation"]["arguments"].is_array());
    }
    assert!(!output.exists());
    assert!(
        !harness
            .home
            .join("Library/Caches/magi-unreal-axi/pipeline")
            .exists()
    );
}

#[test]
fn m7_test_list_is_deterministic_and_zero_is_success() {
    let harness = Harness::new();
    let project = harness.project("M7List", json!({}));
    let editor = r#"#!/bin/sh
log=""
for arg in "$@"; do case "$arg" in -abslog=*) log=${arg#-abslog=};; esac; done
mkdir -p "$(dirname "$log")"
cat > "$log" <<'EOF'
x LogAutomationCommandLine: Display: Found 3 Automation Tests
x LogAutomationCommandLine: Display: 	'MagiUnrealAXI.M6.Second'
x LogAutomationCommandLine: Display: 	'Other.Test'
x LogAutomationCommandLine: Display: 	'MagiUnrealAXI.M6.First'
EOF
"#;
    let engine = harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", editor, "#!/bin/sh\nexit 0\n");
    let base = [
        "--project",
        project.to_str().unwrap(),
        "--engine",
        engine.to_str().unwrap(),
        "project",
        "test",
        "list",
        "--format",
        "json",
    ];
    let mut matching = base.to_vec();
    matching.extend(["--filter", "MagiUnrealAXI.M6"]);
    let output = harness.command(&harness.root, &matching);
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stdout)
    );
    let value = json_output(&output);
    assert_eq!(value["count"], 2);
    assert_eq!(value["total"], 2);
    assert_eq!(value["items"][0]["id"], "MagiUnrealAXI.M6.First");
    assert_eq!(value["items"][1]["id"], "MagiUnrealAXI.M6.Second");

    let mut empty = base.to_vec();
    empty.extend(["--filter", "No.Such.Test"]);
    let output = harness.command(&harness.root, &empty);
    assert!(output.status.success());
    let value = json_output(&output);
    assert_eq!(value["count"], 0);
    assert_eq!(value["total"], 0);
    assert_eq!(value["items"], json!([]));
}

#[test]
fn m7_automation_reports_pass_fail_and_zero_match_truthfully() {
    let harness = Harness::new();
    let project = harness.project("M7Report", json!({}));
    let editor = r#"#!/bin/sh
report=""; log=""; command=""
for arg in "$@"; do
  case "$arg" in
    -ReportOutputPath=*) report=${arg#-ReportOutputPath=} ;;
    -abslog=*) log=${arg#-abslog=} ;;
    -ExecCmds=*) command=${arg#-ExecCmds=} ;;
  esac
done
mkdir -p "$report" "$(dirname "$log")"
printf 'automation log\n' > "$log"
case "$command" in
  *NoMatch*) body='{"succeeded":0,"succeededWithWarnings":0,"failed":0,"notRun":0,"inProcess":0,"totalDuration":0,"tests":[]}' ;;
  *Failure*) body='{"succeeded":0,"succeededWithWarnings":0,"failed":1,"notRun":0,"inProcess":0,"totalDuration":0.1,"tests":[{"fullTestPath":"Magi.Failure","state":"Fail","warnings":0,"errors":1}]}' ;;
  *) body='{"devices":[],"succeeded":1,"succeededWithWarnings":0,"failed":0,"notRun":0,"inProcess":0,"totalDuration":0.1,"tests":[{"testDisplayName":"Pass","fullTestPath":"Magi.Pass","state":"Success","warnings":0,"errors":0,"entries":[]}]}' ;;
esac
printf '%s\n' "$body" > "$report/index.json"
"#;
    let engine = harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", editor, "#!/bin/sh\nexit 0\n");
    let invoke = |filter: &str| {
        harness.command(
            &harness.root,
            &[
                "--project",
                project.to_str().unwrap(),
                "--engine",
                engine.to_str().unwrap(),
                "project",
                "test",
                "run",
                "--filter",
                filter,
                "--format",
                "json",
            ],
        )
    };
    let passed = invoke("Magi.Pass");
    assert!(
        passed.status.success(),
        "{}",
        String::from_utf8_lossy(&passed.stdout)
    );
    let passed = json_output(&passed);
    assert_eq!(passed["totals"]["matched"], 1);
    assert_eq!(passed["totals"]["succeeded"], 1);
    assert!(
        passed["artifacts"]
            .as_array()
            .unwrap()
            .iter()
            .any(|item| item["kind"] == "automation-report-index")
    );

    let failed = invoke("Magi.Failure");
    assert_eq!(failed.status.code(), Some(1));
    let failed = json_output(&failed);
    assert_eq!(failed["error"]["reason"], "automation_failed");
    assert_eq!(failed["error"]["diagnostics"]["totals"]["failed"], 1);
    assert_eq!(
        failed["error"]["diagnostics"]["process"]["status"],
        "passed"
    );

    let empty = invoke("Magi.NoMatch");
    assert_eq!(empty.status.code(), Some(1));
    let empty = json_output(&empty);
    assert_eq!(empty["error"]["reason"], "automation_no_matches");
    assert_eq!(empty["error"]["diagnostics"]["totals"]["matched"], 0);
    assert_eq!(empty["error"]["diagnostics"]["process"]["status"], "passed");
}

#[test]
fn m7_unsafe_test_filter_fails_before_editor_spawn() {
    let harness = Harness::new();
    let project = harness.project("M7Filter", json!({}));
    let marker = harness.home.join("editor-started");
    let editor = format!("#!/bin/sh\nprintf started > '{}'\n", marker.display());
    let engine =
        harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", &editor, "#!/bin/sh\nexit 0\n");
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "test",
            "run",
            "--filter",
            "Magi;Quit",
            "--format",
            "json",
        ],
    );
    assert_eq!(output.status.code(), Some(2));
    assert_eq!(
        json_output(&output)["error"]["reason"],
        "unsafe_test_filter"
    );
    assert!(!marker.exists());
}

#[test]
fn m7_uat_outputs_are_verified_protected_and_never_retried() {
    let harness = Harness::new();
    let project = harness.project("M7Uat", json!({}));
    let counter = harness.home.join("uat-count");
    let capture = harness.home.join("uat-args");
    let uat = format!(
        r#"#!/bin/sh
count=0; [ ! -f '{counter}' ] || count=$(cat '{counter}')
count=$((count + 1)); printf '%s' "$count" > '{counter}'
printf '%s\n' "$@" > '{capture}'
out=""
for arg in "$@"; do case "$arg" in -archivedirectory=*) out=${{arg#-archivedirectory=}};; esac; done
case "$out" in *fail*) exit 7;; esac
if [ -n "$out" ]; then mkdir -p "$out/Mac"; printf artifact > "$out/Mac/Game.app"; else mkdir -p Saved/Cooked/Mac; printf artifact > Saved/Cooked/Mac/Game.app; fi
"#,
        counter = counter.display(),
        capture = capture.display()
    );
    let engine = harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", "#!/bin/sh\nexit 0\n", &uat);
    let cook = harness.home.join("cook-output");
    let package = harness.home.join("package-output");
    let command = |family: &str, output: &Path, force: bool| {
        let mut args = vec![
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            family,
            "--output",
            output.to_str().unwrap(),
            "--format",
            "json",
        ];
        if force {
            args.push("--force");
        }
        harness.command(&harness.root, &args)
    };
    let cooked = command("cook", &cook, false);
    assert!(
        cooked.status.success(),
        "{}",
        String::from_utf8_lossy(&cooked.stdout)
    );
    assert_eq!(json_output(&cooked)["operation"]["kind"], "cook");
    let cooked_args = fs::read_to_string(&capture).unwrap();
    assert_eq!(
        cooked_args.lines().collect::<Vec<_>>(),
        vec![
            "BuildCookRun".to_string(),
            format!("-project={}", project.canonicalize().unwrap().display()),
            "-nop4".to_string(),
            "-utf8output".to_string(),
            "-platform=Mac".to_string(),
            "-clientconfig=Development".to_string(),
            "-cook".to_string(),
        ]
    );
    assert_eq!(
        fs::read_to_string(cook.join("Game.app")).unwrap(),
        "artifact"
    );
    let packaged = command("package", &package, false);
    assert!(
        packaged.status.success(),
        "{}",
        String::from_utf8_lossy(&packaged.stdout)
    );
    assert!(package.join(".magi-unreal-axi-package.json").is_file());
    let package_args = fs::read_to_string(&capture).unwrap();
    assert!(package_args.contains("-package"));
    assert!(package_args.contains("-pak"));
    assert!(
        json_output(&packaged)["artifacts"]
            .as_array()
            .unwrap()
            .iter()
            .all(|artifact| !artifact["path"]
                .as_str()
                .unwrap_or("")
                .contains(".magi-package-stage-"))
    );

    fs::write(package.join("stale.txt"), "stale").unwrap();
    let before = fs::read_to_string(&counter)
        .unwrap()
        .parse::<u32>()
        .unwrap();
    let forced = command("package", &package, true);
    assert!(forced.status.success());
    assert!(!package.join("stale.txt").exists());
    assert_eq!(
        fs::read_to_string(&counter)
            .unwrap()
            .parse::<u32>()
            .unwrap(),
        before + 1
    );
    assert!(fs::read_dir(&harness.home).unwrap().all(|entry| {
        !entry
            .unwrap()
            .file_name()
            .to_string_lossy()
            .starts_with(".magi-package-backup")
    }));

    let unmanaged = harness.home.join("unmanaged");
    fs::create_dir(&unmanaged).unwrap();
    fs::write(unmanaged.join("sentinel"), "keep").unwrap();
    let before = fs::read_to_string(&counter).unwrap();
    let refused = command("package", &unmanaged, true);
    assert_eq!(refused.status.code(), Some(1));
    assert_eq!(
        json_output(&refused)["error"]["reason"],
        "output_not_managed"
    );
    assert_eq!(fs::read_to_string(&counter).unwrap(), before);
    assert_eq!(
        fs::read_to_string(unmanaged.join("sentinel")).unwrap(),
        "keep"
    );

    let failed_output = harness.home.join("fail-package");
    let before = fs::read_to_string(&counter)
        .unwrap()
        .parse::<u32>()
        .unwrap();
    let failed = command("package", &failed_output, false);
    assert_eq!(failed.status.code(), Some(1));
    assert_eq!(
        fs::read_to_string(&counter)
            .unwrap()
            .parse::<u32>()
            .unwrap(),
        before + 1
    );
    let failed = json_output(&failed);
    assert_eq!(failed["error"]["reason"], "process_failed");
    assert!(
        failed["error"]["diagnostics"]["artifacts"]
            .as_array()
            .unwrap()
            .iter()
            .any(|item| item["kind"] == "log")
    );
}

#[test]
fn m7_failed_cook_does_not_export_stale_saved_artifacts() {
    let harness = Harness::new();
    let project = harness.project("M7CookFail", json!({}));
    let cooked = harness.root.join("Saved/Cooked/Mac");
    fs::create_dir_all(&cooked).unwrap();
    fs::write(cooked.join("stale.bin"), "stale").unwrap();
    let counter = harness.home.join("cook-count");
    let uat = format!(
        "#!/bin/sh\nprintf called >> '{}'\nexit 7\n",
        counter.display()
    );
    let engine = harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", "#!/bin/sh\nexit 0\n", &uat);
    let output = harness.home.join("failed-cook-output");
    let result = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "cook",
            "--output",
            output.to_str().unwrap(),
            "--format",
            "json",
        ],
    );
    assert_eq!(result.status.code(), Some(1));
    let error = json_output(&result);
    assert_eq!(error["error"]["reason"], "process_failed");
    assert_eq!(
        error["error"]["diagnostics"]["operation"]["status"],
        "failed"
    );
    assert_eq!(error["error"]["diagnostics"]["process"]["status"], "failed");
    assert!(!output.exists());
    assert_eq!(
        fs::read_to_string(cooked.join("stale.bin")).unwrap(),
        "stale"
    );
    assert_eq!(fs::read_to_string(counter).unwrap(), "called");

    let id = error["error"]["diagnostics"]["operation"]["id"]
        .as_str()
        .unwrap();
    let view = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "operation",
            "view",
            id,
            "--format",
            "json",
        ],
    );
    assert!(view.status.success());
    assert_eq!(json_output(&view)["operation"]["status"], "failed");
}

#[cfg(unix)]
#[test]
fn m7_unsafe_cooked_tree_fails_after_successful_process_without_partial_output() {
    let harness = Harness::new();
    let project = harness.project("M7CookUnsafe", json!({}));
    let uat = "#!/bin/sh\nmkdir -p Saved/Cooked/Mac\nprintf safe > Saved/Cooked/Mac/safe.bin\nln -s /etc/passwd Saved/Cooked/Mac/escape\n";
    let engine = harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", "#!/bin/sh\nexit 0\n", uat);
    let output = harness.home.join("unsafe-cook-output");
    let result = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "cook",
            "--output",
            output.to_str().unwrap(),
            "--format",
            "json",
        ],
    );
    assert_eq!(result.status.code(), Some(1));
    let error = json_output(&result);
    assert_eq!(error["error"]["reason"], "artifact_copy_failed");
    assert_eq!(
        error["error"]["diagnostics"]["operation"]["status"],
        "failed"
    );
    assert_eq!(error["error"]["diagnostics"]["process"]["status"], "passed");
    assert!(!output.exists());

    let id = error["error"]["diagnostics"]["operation"]["id"]
        .as_str()
        .unwrap();
    let view = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "operation",
            "view",
            id,
            "--format",
            "json",
        ],
    );
    let view = json_output(&view);
    assert_eq!(view["operation"]["status"], "failed");
    assert_eq!(view["process"]["status"], "passed");
}

#[test]
fn m7_failed_package_preserves_final_destinations_and_cleans_staging() {
    let harness = Harness::new();
    let project = harness.project("M7PackageFail", json!({}));
    let counter = harness.home.join("package-count");
    let uat = format!(
        r#"#!/bin/sh
printf x >> '{counter}'
out=""
for arg in "$@"; do case "$arg" in -archivedirectory=*) out=${{arg#-archivedirectory=}};; esac; done
mkdir -p "$out/Mac"
printf partial > "$out/Mac/partial.bin"
exit 7
"#,
        counter = counter.display()
    );
    let engine = harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", "#!/bin/sh\nexit 0\n", &uat);
    let fresh = harness.home.join("fresh-package");
    let existing = harness.home.join("managed-package");
    fs::create_dir(&existing).unwrap();
    fs::write(
        existing.join(".magi-unreal-axi-package.json"),
        "marker-original",
    )
    .unwrap();
    fs::write(existing.join("sentinel.bin"), "sentinel-original").unwrap();

    let run = |output: &Path, force: bool| {
        let mut args = vec![
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "package",
            "--output",
            output.to_str().unwrap(),
            "--format",
            "json",
        ];
        if force {
            args.push("--force");
        }
        harness.command(&harness.root, &args)
    };
    assert_eq!(run(&fresh, false).status.code(), Some(1));
    assert!(!fresh.exists());
    assert_eq!(run(&existing, true).status.code(), Some(1));
    assert_eq!(
        fs::read_to_string(existing.join(".magi-unreal-axi-package.json")).unwrap(),
        "marker-original"
    );
    assert_eq!(
        fs::read_to_string(existing.join("sentinel.bin")).unwrap(),
        "sentinel-original"
    );
    assert_eq!(fs::read_to_string(counter).unwrap(), "xx");
    assert!(fs::read_dir(&harness.home).unwrap().all(|entry| {
        !entry
            .unwrap()
            .file_name()
            .to_string_lossy()
            .starts_with(".magi-package-stage-")
    }));
}

#[test]
fn m7_empty_package_artifact_records_operation_failure_but_process_success() {
    let harness = Harness::new();
    let project = harness.project("M7PackageEmpty", json!({}));
    let uat = r#"#!/bin/sh
out=""
for arg in "$@"; do case "$arg" in -archivedirectory=*) out=${arg#-archivedirectory=};; esac; done
mkdir -p "$out"
"#;
    let engine = harness.fake_pipeline_engine("#!/bin/sh\nexit 0\n", "#!/bin/sh\nexit 0\n", uat);
    let output = harness.home.join("empty-package");
    let result = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "package",
            "--output",
            output.to_str().unwrap(),
            "--format",
            "json",
        ],
    );
    assert_eq!(result.status.code(), Some(1));
    let error = json_output(&result);
    assert_eq!(error["error"]["reason"], "package_postprocess_failed");
    assert_eq!(
        error["error"]["diagnostics"]["operation"]["status"],
        "failed"
    );
    assert_eq!(error["error"]["diagnostics"]["process"]["status"], "passed");
    assert!(!output.exists());

    let id = error["error"]["diagnostics"]["operation"]["id"]
        .as_str()
        .unwrap();
    let view = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "operation",
            "view",
            id,
            "--format",
            "json",
        ],
    );
    let view = json_output(&view);
    assert_eq!(view["operation"]["status"], "failed");
    assert_eq!(view["process"]["status"], "passed");
}

#[test]
fn m7_failed_build_has_totals_log_and_local_operation_summary() {
    let harness = Harness::new();
    let project = harness.project("M7BuildFail", json!({}));
    let engine = harness.fake_engine(
        "#!/bin/sh\nprintf 'Game.cpp: error: broken\nGame.cpp: warning: caution\n'\nexit 3\n",
    );
    let output = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "build",
            "--format",
            "json",
        ],
    );
    assert_eq!(output.status.code(), Some(1));
    assert!(output.stderr.is_empty());
    let error = json_output(&output);
    assert_eq!(error["error"]["diagnostics"]["totals"]["errors"], 1);
    assert_eq!(error["error"]["diagnostics"]["totals"]["warnings"], 1);
    let id = error["error"]["diagnostics"]["operation"]["id"]
        .as_str()
        .unwrap();
    let view = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "operation",
            "view",
            id,
            "--format",
            "json",
        ],
    );
    assert!(view.status.success());
    assert_eq!(json_output(&view)["operation"]["id"], id);
}

#[test]
fn m7_logs_are_project_scoped_bounded_sanitized_and_empty_is_success() {
    let harness = Harness::new();
    let project = harness.project("M7Logs", json!({}));
    let empty = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "log",
            "latest",
            "--format",
            "json",
        ],
    );
    assert!(empty.status.success());
    assert_eq!(json_output(&empty)["count"], 0);

    let engine =
        harness.fake_engine("#!/bin/sh\nprintf '\\033[31mneedle\\033[0m\\001 visible\\n'\n");
    let build = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "--engine",
            engine.to_str().unwrap(),
            "project",
            "build",
            "--format",
            "json",
        ],
    );
    assert!(build.status.success());
    let latest = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "log",
            "latest",
            "--lines",
            "10",
            "--bytes",
            "65536",
            "--format",
            "json",
        ],
    );
    assert!(latest.status.success());
    let latest_text = serde_json::to_string(&json_output(&latest)["lines"]).unwrap();
    assert!(latest_text.contains("needle"));
    assert!(!latest_text.contains("\\u001b"));
    assert!(!latest_text.contains("\\u0001"));
    let search = harness.command(
        &harness.root,
        &[
            "--project",
            project.to_str().unwrap(),
            "log",
            "search",
            "needle",
            "--format",
            "json",
        ],
    );
    assert!(search.status.success());
    assert_eq!(json_output(&search)["count"], 1);
    assert!(json_output(&search)["scannedBytes"].as_u64().unwrap() <= 8 * 1024 * 1024);
}

#[test]
fn m8_agent_setup_preserves_deduplicates_repairs_and_invokes_context() {
    let harness = Harness::new();
    let project = harness.project("AgentSetup", json!({}));
    fs::create_dir_all(harness.home.join(".claude")).unwrap();
    fs::write(
        harness.home.join(".claude/settings.json"),
        serde_json::to_vec_pretty(&json!({
            "permissions":{"allow":["Read"]},
            "hooks":{"SessionStart":[
                {"matcher":"","hooks":[{"type":"command","command":"/stale/magi-unreal-axi agent context --format json"},{"type":"command","command":"echo preserve-sibling"}]},
                {"matcher":"compact","hooks":[{"type":"command","command":"echo keep"}]}
            ]}
        }))
        .unwrap(),
    )
    .unwrap();

    let first = harness.command(&harness.root, &["setup", "agents", "--format", "json"]);
    assert!(
        first.status.success(),
        "{}",
        String::from_utf8_lossy(&first.stdout)
    );
    let first_value = json_output(&first);
    assert_eq!(first_value["changed"], true);
    assert_eq!(first_value["targets"].as_array().unwrap().len(), 3);
    for path in [
        ".claude/skills/magi-unreal-axi/SKILL.md",
        ".agents/skills/magi-unreal-axi/SKILL.md",
        ".config/opencode/skills/magi-unreal-axi/SKILL.md",
    ] {
        let installed = fs::read_to_string(harness.home.join(path)).unwrap();
        assert_eq!(
            installed,
            include_str!("../skills/magi-unreal-axi/SKILL.md")
        );
    }

    let settings_path = harness.home.join(".claude/settings.json");
    let first_settings = fs::read(&settings_path).unwrap();
    let settings: Value = serde_json::from_slice(&first_settings).unwrap();
    assert_eq!(settings["permissions"]["allow"][0], "Read");
    let sessions = settings["hooks"]["SessionStart"].as_array().unwrap();
    assert_eq!(sessions.len(), 3);
    assert!(sessions.iter().any(|entry| {
        entry["hooks"][0]["command"]
            .as_str()
            .is_some_and(|command| command == "echo keep")
    }));
    assert!(sessions.iter().any(|entry| {
        entry["hooks"].as_array().is_some_and(|hooks| {
            hooks
                .iter()
                .any(|hook| hook["command"] == "echo preserve-sibling")
        })
    }));
    let command = sessions
        .iter()
        .find_map(|entry| {
            entry["hooks"][0]["command"]
                .as_str()
                .filter(|command| command.ends_with(" agent context --format json"))
        })
        .expect("managed SessionStart hook");
    assert!(!command.contains("/stale/"));
    assert!(
        sessions
            .iter()
            .all(|entry| { entry["hooks"][0].get("magiUnrealAxi").is_none() })
    );

    let hook = Command::new("/bin/sh")
        .args(["-c", command])
        .current_dir(&harness.root)
        .env_clear()
        .env("HOME", &harness.home)
        .env("PATH", "/usr/bin:/bin")
        .output()
        .unwrap();
    assert!(
        hook.status.success(),
        "{}",
        String::from_utf8_lossy(&hook.stdout)
    );
    assert!(hook.stderr.is_empty());
    assert!(
        hook.stdout.len() < 400,
        "context exceeds conservative 400-token byte bound"
    );
    let context: Value = serde_json::from_slice(&hook.stdout).unwrap();
    assert_eq!(
        context["magiUnrealAxi"]["project"]["path"].as_str(),
        project.canonicalize().unwrap().to_str()
    );
    let nested = (0..24).fold(harness.root.clone(), |path, index| {
        path.join(format!("evaluation-context-segment-{index}"))
    });
    fs::create_dir_all(&nested).unwrap();
    let long_context = Command::new("/bin/sh")
        .args(["-c", command])
        .current_dir(nested)
        .env_clear()
        .env("HOME", &harness.home)
        .env("PATH", "/usr/bin:/bin")
        .output()
        .unwrap();
    assert!(long_context.status.success());
    assert!(long_context.stdout.len() < 400);
    assert_eq!(
        serde_json::from_slice::<Value>(&long_context.stdout).unwrap()["magiUnrealAxi"]["project"]
            ["name"],
        "AgentSetup"
    );

    let second = harness.command(&harness.root, &["setup", "agents", "--format", "json"]);
    assert!(second.status.success());
    assert_eq!(json_output(&second)["changed"], false);
    assert_eq!(fs::read(settings_path).unwrap(), first_settings);
}

#[cfg(unix)]
#[test]
fn m8_agent_setup_refuses_modified_skill_without_partial_writes_and_preserves_mode() {
    use std::os::unix::fs::PermissionsExt;
    let harness = Harness::new();
    let skill = harness.home.join(".agents/skills/magi-unreal-axi/SKILL.md");
    fs::create_dir_all(skill.parent().unwrap()).unwrap();
    fs::write(&skill, "user-authored").unwrap();
    let refused = harness.command(
        &harness.root,
        &[
            "setup",
            "agents",
            "--codex",
            "--opencode",
            "--format",
            "json",
        ],
    );
    assert_eq!(refused.status.code(), Some(1));
    assert_eq!(json_output(&refused)["error"]["reason"], "skill_modified");
    assert!(
        !harness
            .home
            .join(".config/opencode/skills/magi-unreal-axi/SKILL.md")
            .exists()
    );

    let settings = harness.home.join(".claude/settings.json");
    fs::create_dir_all(settings.parent().unwrap()).unwrap();
    fs::write(&settings, "{}").unwrap();
    fs::set_permissions(&settings, fs::Permissions::from_mode(0o640)).unwrap();
    let installed = harness.command(
        &harness.root,
        &["setup", "agents", "--claude", "--format", "json"],
    );
    assert!(installed.status.success());
    assert_eq!(
        fs::metadata(settings).unwrap().permissions().mode() & 0o777,
        0o640
    );

    let fresh = Harness::new();
    let created = fresh.command(
        &fresh.root,
        &["setup", "agents", "--claude", "--format", "json"],
    );
    assert!(created.status.success());
    assert_eq!(
        fs::metadata(fresh.home.join(".claude/settings.json"))
            .unwrap()
            .permissions()
            .mode()
            & 0o777,
        0o600
    );
}

#[cfg(unix)]
#[test]
fn m8_agent_setup_stages_all_targets_and_repairs_owned_sidecars() {
    use std::os::unix::fs::PermissionsExt;

    let harness = Harness::new();
    let blocked = harness.home.join(".config/opencode/skills/magi-unreal-axi");
    fs::create_dir_all(&blocked).unwrap();
    fs::set_permissions(&blocked, fs::Permissions::from_mode(0o500)).unwrap();
    let failed = harness.command(&harness.root, &["setup", "agents", "--format", "json"]);
    fs::set_permissions(&blocked, fs::Permissions::from_mode(0o700)).unwrap();
    assert_eq!(failed.status.code(), Some(1));
    for path in [
        ".claude/settings.json",
        ".claude/skills/magi-unreal-axi/SKILL.md",
        ".agents/skills/magi-unreal-axi/SKILL.md",
        ".config/opencode/skills/magi-unreal-axi/SKILL.md",
    ] {
        assert!(!harness.home.join(path).exists(), "partial write: {path}");
    }

    let repaired = Harness::new();
    let first = repaired.command(
        &repaired.root,
        &["setup", "agents", "--codex", "--format", "json"],
    );
    assert!(first.status.success());
    let sidecar = repaired
        .home
        .join(".agents/skills/magi-unreal-axi/SKILL.md.magi-unreal-axi.sha256");
    fs::remove_file(&sidecar).unwrap();
    let repair = repaired.command(
        &repaired.root,
        &["setup", "agents", "--codex", "--format", "json"],
    );
    assert!(repair.status.success());
    assert_eq!(json_output(&repair)["changed"], true);
    assert_eq!(fs::read_to_string(&sidecar).unwrap().len(), 64);
    let repeat = repaired.command(
        &repaired.root,
        &["setup", "agents", "--codex", "--format", "json"],
    );
    assert!(repeat.status.success());
    assert_eq!(json_output(&repeat)["changed"], false);

    fs::write(&sidecar, "invalid").unwrap();
    let malformed = repaired.command(
        &repaired.root,
        &["setup", "agents", "--codex", "--format", "json"],
    );
    assert_eq!(malformed.status.code(), Some(1));
    assert_eq!(
        json_output(&malformed)["error"]["reason"],
        "malformed_sidecar"
    );
}

#[test]
fn m8_agent_setup_targets_combine_and_malformed_claude_config_fails_closed() {
    let harness = Harness::new();
    fs::create_dir_all(harness.root.join(".magi")).unwrap();
    fs::write(harness.root.join(".magi/unreal-axi.toml"), "engine = [").unwrap();
    let selected = harness.command(
        &harness.root,
        &[
            "setup",
            "agents",
            "--codex",
            "--opencode",
            "--format",
            "json",
        ],
    );
    assert!(selected.status.success());
    let selected = json_output(&selected);
    assert_eq!(selected["targets"].as_array().unwrap().len(), 2);
    assert!(!harness.home.join(".claude/settings.json").exists());
    assert!(
        harness
            .home
            .join(".agents/skills/magi-unreal-axi/SKILL.md")
            .is_file()
    );
    assert!(
        harness
            .home
            .join(".config/opencode/skills/magi-unreal-axi/SKILL.md")
            .is_file()
    );

    fs::create_dir_all(harness.home.join(".claude")).unwrap();
    fs::write(harness.home.join(".claude/settings.json"), "{").unwrap();
    let malformed = harness.command(
        &harness.root,
        &["setup", "agents", "--claude", "--format", "json"],
    );
    assert_eq!(malformed.status.code(), Some(1));
    assert_eq!(
        json_output(&malformed)["error"]["reason"],
        "malformed_config"
    );
    assert!(
        !harness
            .home
            .join(".claude/skills/magi-unreal-axi/SKILL.md")
            .exists()
    );
}
