use crate::{
    agent_setup, bridge, capability,
    cli::{
        ActorCommand, AssetCommand, BlueprintCommand, CapabilityCommand, Cli, CommandKind,
        ComponentCommand, EditorCommand, EngineCommand, Format, LevelCommand, ListArgs, LogCommand,
        OperationCommand, PlayCommand, ProjectCommand, SetupCommand,
    },
    config, engine,
    error::AppError,
    output, process, project, setup,
};
use clap::{
    Parser,
    error::{ContextKind, ErrorKind},
};
use serde_json::{Value, json};
use std::{ffi::OsString, io::Read, path::Path, process::ExitCode, time::Duration};

pub fn run<I, T>(args: I) -> ExitCode
where
    I: IntoIterator<Item = T>,
    T: Into<OsString> + Clone,
{
    let args = args.into_iter().map(Into::into).collect::<Vec<OsString>>();
    let parse_format = format_from_raw_args(&args);
    let cli = match Cli::try_parse_from(args) {
        Ok(cli) => cli,
        Err(error)
            if matches!(
                error.kind(),
                ErrorKind::DisplayHelp | ErrorKind::DisplayVersion
            ) =>
        {
            print!("{error}");
            return ExitCode::SUCCESS;
        }
        Err(error) => {
            let message = error
                .to_string()
                .lines()
                .next()
                .unwrap_or("invalid command")
                .to_owned();
            return emit(
                AppError::usage("invalid_arguments", message, clap_correction(&error)).envelope(),
                parse_format,
                false,
                2,
            );
        }
    };
    if matches!(
        &cli.command,
        Some(CommandKind::Agent {
            command: crate::cli::AgentCommand::Context
        })
    ) {
        let cwd = std::env::current_dir().map_err(|error| {
            AppError::operational(
                "context",
                "cwd_unavailable",
                error.to_string(),
                "select a readable working directory",
            )
        });
        return match cwd.and_then(|path| agent_setup::context(&path)) {
            Ok(value) => emit(value, parse_format, cli.full, 0),
            Err(error) => emit(error.envelope(), parse_format, cli.full, error.code),
        };
    }
    if let Some(CommandKind::Setup {
        command: SetupCommand::Agents(args),
    }) = &cli.command
    {
        return match agent_setup::install(args) {
            Ok(value) => emit(value, parse_format, cli.full, 0),
            Err(error) => emit(error.envelope(), parse_format, cli.full, error.code),
        };
    }

    let context = match prepare(&cli) {
        Ok(context) => context,
        Err(error) => return emit(error.envelope(), parse_format, cli.full, error.code),
    };
    let format = context.resolved.format;
    match execute(&cli, &context) {
        Ok(value) => emit(value, format, cli.full, 0),
        Err(error) => emit(error.envelope(), format, cli.full, error.code),
    }
}

fn clap_correction(error: &clap::Error) -> String {
    for kind in [
        ContextKind::SuggestedSubcommand,
        ContextKind::SuggestedArg,
        ContextKind::SuggestedValue,
        ContextKind::SuggestedCommand,
    ] {
        if let Some(suggestion) = error.get(kind) {
            return format!("replace invalid input with `{suggestion}`; magi-unreal-axi --help");
        }
    }
    "magi-unreal-axi --help".to_owned()
}

fn emit(value: Value, format: Format, full: bool, code: u8) -> ExitCode {
    match output::render(value, format, full) {
        Ok(text) => {
            println!("{text}");
            ExitCode::from(code)
        }
        Err(error) => {
            let fallback = serde_json::to_string(&error.envelope())
                .unwrap_or_else(|_| "{\"error\":{\"type\":\"output\",\"code\":1}}".into());
            println!("{fallback}");
            ExitCode::from(1)
        }
    }
}

struct Context {
    cwd: std::path::PathBuf,
    project: Option<std::path::PathBuf>,
    resolved: config::ResolvedConfig,
}

fn prepare(cli: &Cli) -> Result<Context, AppError> {
    let cwd = std::env::current_dir().map_err(|error| {
        AppError::operational(
            "project",
            "cwd_unavailable",
            error.to_string(),
            "select a readable working directory",
        )
    })?;
    let project = project::select(cli.project.as_deref(), &cwd)?;
    let resolved = config::resolve(cli, project.as_deref())?;
    if cli.verbose {
        eprintln!(
            "magi-unreal-axi: project={} engine={}",
            project
                .as_ref()
                .map_or("none".into(), |path| path.display().to_string()),
            resolved
                .engine
                .as_ref()
                .map_or("auto".into(), |path| path.display().to_string())
        );
    }
    Ok(Context {
        cwd,
        project,
        resolved,
    })
}

fn execute(cli: &Cli, context: &Context) -> Result<Value, AppError> {
    let selected_project = context.project.as_deref();
    let resolved = &context.resolved;
    let cwd = &context.cwd;
    match &cli.command {
        None => home(cwd, selected_project, resolved),
        Some(CommandKind::Project { command }) => match command {
            None | Some(ProjectCommand::View) => project_view(require_project(selected_project)?),
            Some(ProjectCommand::Doctor) => {
                project_doctor(require_project(selected_project)?, resolved)
            }
            Some(ProjectCommand::Build(args)) => {
                let project = require_project(selected_project)?;
                let engine = require_engine(engine::resolve(resolved, Some(project))?)?;
                process::build_invocation(project, &engine, args, resolved.timeout_seconds)
            }
            Some(ProjectCommand::Test(args)) => {
                let project = require_project(selected_project)?;
                let engine = require_engine(engine::resolve(resolved, Some(project))?)?;
                process::test_invocation(project, &engine, args, resolved.timeout_seconds)
            }
            Some(ProjectCommand::Cook(args)) => {
                let project = require_project(selected_project)?;
                let engine = require_engine(engine::resolve(resolved, Some(project))?)?;
                process::cook_invocation(project, &engine, args, resolved.timeout_seconds)
            }
            Some(ProjectCommand::Package(args)) => {
                let project = require_project(selected_project)?;
                let engine = require_engine(engine::resolve(resolved, Some(project))?)?;
                process::package_invocation(project, &engine, args, resolved.timeout_seconds)
            }
        },
        Some(CommandKind::Editor { command }) => {
            let project = require_project(selected_project)?;
            match command {
                EditorCommand::Status => bridge::status(
                    project,
                    resolved.editor,
                    std::time::Duration::from_secs(resolved.timeout_seconds.unwrap_or(30)),
                ),
                EditorCommand::Describe => bridge::describe(
                    project,
                    resolved.editor,
                    std::time::Duration::from_secs(resolved.timeout_seconds.unwrap_or(30)),
                ),
                EditorCommand::Stop => bridge::stop(
                    project,
                    resolved.editor,
                    std::time::Duration::from_secs(resolved.timeout_seconds.unwrap_or(30)),
                ),
                EditorCommand::Start => {
                    let plugin = project.parent().unwrap().join("Plugins/MagiUnrealAXI");
                    if !plugin.join("MagiUnrealAXI.uplugin").is_file()
                        || !plugin
                            .join("Binaries/Mac/libUnrealEditor-MagiUnrealAXI.dylib")
                            .is_file()
                    {
                        return Err(AppError::operational(
                            "setup",
                            "plugin_not_ready",
                            "compatible built MagiUnrealAXI plugin is not installed",
                            "magi-unreal-axi setup plugin install",
                        ));
                    }
                    let engine = require_engine(engine::resolve(resolved, Some(project))?)?;
                    bridge::start(
                        project,
                        &engine,
                        resolved.editor,
                        std::time::Duration::from_secs(resolved.timeout_seconds.unwrap_or(120)),
                    )
                }
            }
        }
        Some(CommandKind::Log { command }) => match command {
            LogCommand::Latest { lines, bytes } => {
                process::latest_log(require_project(selected_project)?, *lines, *bytes)
            }
            LogCommand::Search { query, limit } => {
                process::search_logs(require_project(selected_project)?, query, *limit)
            }
        },
        Some(CommandKind::Setup {
            command: SetupCommand::Agents(args),
        }) => agent_setup::install(args),
        Some(CommandKind::Setup {
            command: SetupCommand::Plugin { command },
        }) => {
            let project = require_project(selected_project)?;
            let engine = engine::resolve(resolved, Some(project))?;
            setup::execute(command, project, engine.as_ref())
        }
        Some(CommandKind::Agent { .. }) => {
            unreachable!("agent context handled before configuration")
        }
        Some(CommandKind::Engine { command }) => match command {
            None | Some(EngineCommand::List) => engine_list(resolved),
            Some(EngineCommand::View) => {
                let engine = require_engine(engine::resolve(resolved, selected_project)?)?;
                serde_json::to_value(engine).map_err(output_error)
            }
        },
        Some(CommandKind::Capability { command }) => match command {
            CapabilityCommand::Search { query, limit } => {
                let args = capability::validate_input(
                    "capability.search",
                    json!({"query":query,"limit":limit}),
                )?;
                let mut output = capability::execute_local("capability.search", &args)
                    .expect("search is local")?;
                if let Some(project) = selected_project
                    && let Some(live) = bridge::runtime_availability(
                        project,
                        resolved.editor,
                        Duration::from_secs(resolved.timeout_seconds.unwrap_or(30)),
                    )?
                {
                    capability::apply_runtime_availability(&mut output, &live);
                }
                capability::validate_output("capability.search", output)
            }
            CapabilityCommand::Describe { id } => {
                let args = capability::validate_input("capability.describe", json!({"id":id}))?;
                let mut output = capability::execute_local("capability.describe", &args)
                    .expect("describe is local")?;
                if let Some(project) = selected_project
                    && let Some(live) = bridge::runtime_availability(
                        project,
                        resolved.editor,
                        Duration::from_secs(resolved.timeout_seconds.unwrap_or(30)),
                    )?
                {
                    capability::apply_runtime_availability(&mut output, &live);
                }
                capability::validate_output("capability.describe", output)
            }
            CapabilityCommand::Execute {
                id,
                input_json,
                input_file,
                expected_revision,
                idempotency_key,
            } => {
                let input = read_capability_input(input_json.as_deref(), input_file.as_deref())?;
                execute_capability_with_options(
                    id,
                    input,
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: expected_revision.clone(),
                        idempotency_key: idempotency_key.clone(),
                    },
                )
            }
        },
        Some(CommandKind::Level { command }) => match command {
            LevelCommand::Current => {
                execute_capability("level.current", json!({}), selected_project, resolved)
            }
            LevelCommand::List(args) => {
                execute_capability("level.list", list_input(args), selected_project, resolved)
            }
            LevelCommand::Create(args) => execute_capability_with_options(
                "level.create",
                json!({"path":args.path}),
                selected_project,
                resolved,
                bridge::ExecutionOptions {
                    expected_revision: args.expected_revision.clone(),
                    idempotency_key: args.idempotency_key.clone(),
                },
            ),
            LevelCommand::Open(args) => execute_capability_with_options(
                "level.open",
                json!({"path":args.path}),
                selected_project,
                resolved,
                bridge::ExecutionOptions {
                    expected_revision: args.expected_revision.clone(),
                    idempotency_key: args.idempotency_key.clone(),
                },
            ),
            LevelCommand::Save(args) => execute_capability_with_options(
                "level.save",
                json!({"path":args.path}),
                selected_project,
                resolved,
                bridge::ExecutionOptions {
                    expected_revision: args.expected_revision.clone(),
                    idempotency_key: args.idempotency_key.clone(),
                },
            ),
            LevelCommand::Settings { level_id } => execute_capability(
                "level.settings",
                level_id
                    .as_ref()
                    .map_or_else(|| json!({}), |id| json!({"levelId": id})),
                selected_project,
                resolved,
            ),
            LevelCommand::SetGameMode {
                level_id,
                game_mode_class,
                expected_revision,
            } => {
                require_revision("level.set_game_mode", expected_revision.as_deref())?;
                execute_capability_with_options(
                    "level.set_game_mode",
                    json!({"levelId":level_id,"gameModeClass":game_mode_class}),
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: expected_revision.clone(),
                        idempotency_key: None,
                    },
                )
            }
        },
        Some(CommandKind::Actor { command }) => match command {
            ActorCommand::List(args) => {
                execute_capability("actor.list", list_input(args), selected_project, resolved)
            }
            ActorCommand::View { id } => {
                execute_capability("actor.view", json!({"id":id}), selected_project, resolved)
            }
            ActorCommand::Spawn(args) => {
                let mut input = serde_json::Map::from_iter([
                    ("levelId".to_owned(), json!(args.level_id)),
                    ("class".to_owned(), json!(args.class_path)),
                    ("agentKey".to_owned(), json!(args.agent_key)),
                ]);
                if let Some(label) = &args.label {
                    input.insert("label".into(), json!(label));
                }
                if let Some(location) = &args.location {
                    input.insert("location".into(), parse_location(&Some(location.clone()))?);
                }
                execute_capability_with_options(
                    "actor.spawn",
                    Value::Object(input),
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: None,
                        idempotency_key: args.idempotency_key.clone(),
                    },
                )
            }
            ActorCommand::UpdateTransform(args) if args.expected_revision.is_none() => {
                Err(AppError::usage(
                    "expected_revision_required",
                    "actor.update_transform requires --expected-revision",
                    "re-read actor.view, then pass --expected-revision <revision>",
                ))
            }
            ActorCommand::UpdateTransform(args) => execute_capability_with_options(
                "actor.update_transform",
                json!({"id":args.id,"location":parse_location(&Some(args.location.clone()))?}),
                selected_project,
                resolved,
                bridge::ExecutionOptions {
                    expected_revision: args.expected_revision.clone(),
                    idempotency_key: None,
                },
            ),
            ActorCommand::Delete(args) => {
                if !args.dry_run && args.expected_revision.is_none() {
                    return Err(AppError::usage(
                        "expected_revision_required",
                        "actor.delete requires --expected-revision unless --dry-run",
                        "re-read actor.view, then pass --expected-revision <revision>",
                    ));
                }
                if !args.force && !args.dry_run {
                    return Err(AppError::usage(
                        "force_required",
                        "actor.delete requires --force or --dry-run",
                        "re-run with --force or --dry-run",
                    ));
                }
                execute_capability_with_options(
                    "actor.delete",
                    json!({"id":args.id,"force":args.force,"dryRun":args.dry_run}),
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: args.expected_revision.clone(),
                        idempotency_key: None,
                    },
                )
            }
        },
        Some(CommandKind::Operation {
            command: OperationCommand::View { id },
        }) => {
            let project = require_project(selected_project)?;
            if id.starts_with("proc-") {
                process::view_local_operation(project, id)
            } else {
                execute_capability(
                    "operation.view",
                    json!({"id":id}),
                    selected_project,
                    resolved,
                )
            }
        }
        Some(CommandKind::Asset { command }) => match command {
            AssetCommand::List(args) => {
                execute_capability("asset.list", list_input(args), selected_project, resolved)
            }
            AssetCommand::View { id } => {
                execute_capability("asset.view", json!({"id":id}), selected_project, resolved)
            }
            AssetCommand::Save(args) => {
                require_revision("asset.save", args.expected_revision.as_deref())?;
                execute_capability_with_options(
                    "asset.save",
                    json!({"id":args.id}),
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: args.expected_revision.clone(),
                        idempotency_key: None,
                    },
                )
            }
        },
        Some(CommandKind::Component { command }) => match command {
            ComponentCommand::List { actor_id, args } => {
                let mut input = list_input(args);
                input["actorId"] = json!(actor_id);
                execute_capability("component.list", input, selected_project, resolved)
            }
            ComponentCommand::View { id } => execute_capability(
                "component.view",
                json!({"id":id}),
                selected_project,
                resolved,
            ),
            ComponentCommand::Add(args) => {
                let mut input =
                    json!({"actorId":args.actor_id,"class":args.class_path,"name":args.name});
                if let Some(location) = &args.location {
                    input["location"] = parse_location(&Some(location.clone()))?;
                }
                require_revision("component.add", args.expected_revision.as_deref())?;
                execute_capability_with_options(
                    "component.add",
                    input,
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: args.expected_revision.clone(),
                        idempotency_key: None,
                    },
                )
            }
            ComponentCommand::Update(args) => {
                require_revision("component.update", args.expected_revision.as_deref())?;
                execute_capability_with_options(
                    "component.update",
                    json!({"id":args.id,"location":parse_location(&Some(args.location.clone()))?}),
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: args.expected_revision.clone(),
                        idempotency_key: None,
                    },
                )
            }
            ComponentCommand::Remove(args) => {
                if !args.force && !args.dry_run {
                    return Err(AppError::usage(
                        "force_required",
                        "component.remove requires --force or --dry-run",
                        "re-run with --force or --dry-run",
                    ));
                }
                if !args.dry_run {
                    require_revision("component.remove", args.expected_revision.as_deref())?;
                }
                execute_capability_with_options(
                    "component.remove",
                    json!({"id":args.id,"force":args.force,"dryRun":args.dry_run}),
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: args.expected_revision.clone(),
                        idempotency_key: None,
                    },
                )
            }
        },
        Some(CommandKind::Blueprint { command }) => match command {
            BlueprintCommand::View { id } => execute_capability(
                "blueprint.view",
                json!({"id":id}),
                selected_project,
                resolved,
            ),
            BlueprintCommand::Compile(args) => {
                require_revision("blueprint.compile", args.expected_revision.as_deref())?;
                execute_capability_with_options(
                    "blueprint.compile",
                    json!({"id":args.id}),
                    selected_project,
                    resolved,
                    bridge::ExecutionOptions {
                        expected_revision: args.expected_revision.clone(),
                        idempotency_key: None,
                    },
                )
            }
        },
        Some(CommandKind::Play { command }) => {
            let (id, input) = match command {
                PlayCommand::Start => ("play.start", json!({})),
                PlayCommand::Status { session_id } => (
                    "play.status",
                    session_id
                        .as_ref()
                        .map_or_else(|| json!({}), |id| json!({"sessionId":id})),
                ),
                PlayCommand::Observe { session_id } => {
                    ("play.observe", json!({"sessionId":session_id}))
                }
                PlayCommand::Stop { session_id } => ("play.stop", json!({"sessionId":session_id})),
                PlayCommand::Input {
                    session_id,
                    key,
                    event,
                } => (
                    "play.input",
                    json!({"sessionId":session_id,"key":key,"event":match event { crate::cli::PlayEvent::Pressed => "pressed", crate::cli::PlayEvent::Released => "released" }}),
                ),
                PlayCommand::Screenshot { session_id, path } => {
                    let mut input = json!({"sessionId":session_id});
                    if let Some(path) = path {
                        input["path"] = json!(path);
                    }
                    ("play.screenshot", input)
                }
            };
            execute_capability(id, input, selected_project, resolved)
        }
    }
}
fn require_revision(operation: &str, revision: Option<&str>) -> Result<(), AppError> {
    if revision.is_some_and(|value| !value.is_empty()) {
        Ok(())
    } else {
        Err(AppError::usage(
            "expected_revision_required",
            format!("{operation} requires --expected-revision"),
            "re-read target, then pass --expected-revision <revision>",
        ))
    }
}
fn execute_capability(
    id: &str,
    input: Value,
    selected_project: Option<&Path>,
    resolved: &config::ResolvedConfig,
) -> Result<Value, AppError> {
    execute_capability_with_options(
        id,
        input,
        selected_project,
        resolved,
        bridge::ExecutionOptions::default(),
    )
}

fn execute_capability_with_options(
    id: &str,
    input: Value,
    selected_project: Option<&Path>,
    resolved: &config::ResolvedConfig,
    options: bridge::ExecutionOptions,
) -> Result<Value, AppError> {
    let args = capability::validate_input(id, input.clone())?;
    if matches!(
        id,
        "asset.save"
            | "blueprint.compile"
            | "component.add"
            | "component.update"
            | "level.set_game_mode"
    ) && options.expected_revision.is_none()
    {
        return Err(AppError::usage(
            "expected_revision_required",
            format!("{id} requires --expected-revision"),
            "re-read target, then pass --expected-revision <revision>",
        ));
    }
    if id == "component.remove"
        && !input
            .get("dryRun")
            .and_then(Value::as_bool)
            .unwrap_or(false)
        && options.expected_revision.is_none()
    {
        return Err(AppError::usage(
            "expected_revision_required",
            "component.remove requires --expected-revision unless --dry-run",
            "re-read component.view, then pass --expected-revision <revision>",
        ));
    }
    let response = match capability::execute_local(id, &args) {
        Some(result) => result?,
        None => bridge::capability(
            require_project(selected_project)?,
            resolved.editor,
            id,
            args,
            options,
            Duration::from_secs(resolved.timeout_seconds.unwrap_or(30)),
        )?,
    };
    if capability::find(id).is_some_and(|record| record["mutates"] == true) {
        let object = response.as_object().ok_or_else(|| {
            AppError::operational(
                "capability",
                "invalid_editor_output",
                "mutation response must contain result and receipt",
                "magi-unreal-axi operation view <id>",
            )
        })?;
        let result = object.get("result").cloned().ok_or_else(|| {
            AppError::operational(
                "capability",
                "invalid_editor_output",
                "mutation response lacks result",
                "magi-unreal-axi operation view <id>",
            )
        })?;
        let validated = capability::validate_output(id, result)?;
        return Ok(json!({"result": validated, "receipt": object["receipt"]}));
    }
    capability::validate_output(id, response)
}

fn parse_location(value: &Option<String>) -> Result<Value, AppError> {
    let text = value.as_deref().ok_or_else(|| {
        AppError::usage(
            "invalid_location",
            "location is required",
            "use --location X,Y,Z",
        )
    })?;
    let values = text
        .split(',')
        .map(|part| {
            part.parse::<f64>().map_err(|_| {
                AppError::usage(
                    "invalid_location",
                    "location must be X,Y,Z",
                    "use --location X,Y,Z",
                )
            })
        })
        .collect::<Result<Vec<_>, _>>()?;
    if values.len() != 3 {
        return Err(AppError::usage(
            "invalid_location",
            "location must contain three numbers",
            "use --location X,Y,Z",
        ));
    }
    Ok(json!(values))
}

fn list_input(args: &ListArgs) -> Value {
    let mut input = serde_json::Map::from_iter([("limit".to_owned(), json!(args.limit))]);
    if let Some(cursor) = &args.cursor {
        input.insert("cursor".into(), json!(cursor));
    }
    if !args.fields.is_empty() {
        input.insert("fields".into(), json!(args.fields));
    }
    Value::Object(input)
}

fn read_capability_input(
    input_json: Option<&str>,
    input_file: Option<&Path>,
) -> Result<Value, AppError> {
    const MAX_INPUT: u64 = 8 * 1024 * 1024;
    let text = if let Some(text) = input_json {
        text.to_owned()
    } else if let Some(path) = input_file {
        let mut text = String::new();
        if path == Path::new("-") {
            std::io::stdin()
                .take(MAX_INPUT + 1)
                .read_to_string(&mut text)
                .map_err(capability_input_io)?;
        } else {
            std::fs::File::open(path)
                .map_err(capability_input_io)?
                .take(MAX_INPUT + 1)
                .read_to_string(&mut text)
                .map_err(capability_input_io)?;
        }
        if text.len() as u64 > MAX_INPUT {
            return Err(AppError::usage(
                "invalid_capability_input",
                "capability input exceeds 8 MiB",
                "provide a smaller JSON object",
            ));
        }
        text
    } else {
        "{}".to_owned()
    };
    serde_json::from_str(&text).map_err(|error| {
        AppError::usage(
            "invalid_capability_input",
            format!("capability input is not valid JSON: {error}"),
            "provide one JSON object",
        )
    })
}

fn capability_input_io(error: std::io::Error) -> AppError {
    AppError::usage(
        "invalid_capability_input",
        format!("capability input could not be read: {error}"),
        "provide a readable JSON input file",
    )
}

fn home(
    cwd: &Path,
    selected_project: Option<&Path>,
    resolved: &config::ResolvedConfig,
) -> Result<Value, AppError> {
    let bin = collapsed_executable();
    let project_value = match selected_project {
        Some(path) => {
            let descriptor = project::descriptor(path)?;
            json!({"found": true, "name": path.file_stem().and_then(|name| name.to_str()).unwrap_or("project"), "path": path, "engineAssociation": descriptor.get("EngineAssociation")})
        }
        None => json!({"found": false, "scope": cwd, "help": "magi-unreal-axi --project <path>"}),
    };
    let engine_value = match engine::resolve(resolved, selected_project) {
        Ok(Some(info)) => json!({"resolved": true, "version": info.version, "root": info.root}),
        Ok(None) => {
            json!({"resolved": false, "help": "magi-unreal-axi --engine <path> engine view"})
        }
        Err(error) if resolved.engine.is_none() => {
            json!({"resolved": false, "reason": error.reason, "help": error.help})
        }
        Err(error) => return Err(error),
    };
    let plugin_installed = selected_project.is_some_and(|path| {
        path.parent().is_some_and(|root| {
            root.join("Plugins/MagiUnrealAXI/MagiUnrealAXI.uplugin")
                .is_file()
        })
    });
    let editor_value = match selected_project {
        Some(path) => bridge::status(
            path,
            resolved.editor,
            std::time::Duration::from_secs(resolved.timeout_seconds.unwrap_or(30)),
        )?
        .get("editor")
        .cloned()
        .unwrap_or_else(|| json!({"state":"stopped"})),
        None => json!({"state":"stopped"}),
    };
    Ok(json!({
        "bin": bin,
        "description": "Build, inspect, run, and safely automate the current Unreal project",
        "project": project_value,
        "engine": engine_value,
        "plugin": {"installed": plugin_installed},
        "editor": editor_value,
        "help": if selected_project.is_some() { json!(["magi-unreal-axi project doctor", "magi-unreal-axi setup plugin status"]) } else { json!(["magi-unreal-axi --project <path> project view"]) }
    }))
}

fn project_view(path: &Path) -> Result<Value, AppError> {
    let descriptor = project::descriptor(path)?;
    Ok(
        json!({"project": {"name": path.file_stem().and_then(|name| name.to_str()).unwrap_or("project"), "path": path, "engineAssociation": descriptor.get("EngineAssociation"), "descriptor": descriptor}}),
    )
}

fn project_doctor(path: &Path, resolved: &config::ResolvedConfig) -> Result<Value, AppError> {
    let descriptor = project::descriptor(path)?;
    let engine = engine::resolve(resolved, Some(path));
    let plugin_path = path
        .parent()
        .unwrap()
        .join("Plugins/MagiUnrealAXI/MagiUnrealAXI.uplugin");
    let checks = json!([
        {"name": "descriptor", "passed": true, "path": path},
        {"name": "engine", "passed": engine.as_ref().is_ok_and(|value| value.is_some())},
        {"name": "plugin", "passed": plugin_path.is_file(), "path": plugin_path}
    ]);
    Ok(
        json!({"healthy": engine.as_ref().is_ok_and(|value| value.is_some()) && plugin_path.is_file(), "project": path, "engineAssociation": descriptor.get("EngineAssociation"), "checks": checks, "help": ["magi-unreal-axi setup plugin status", "magi-unreal-axi engine view"]}),
    )
}

fn engine_list(resolved: &config::ResolvedConfig) -> Result<Value, AppError> {
    let engines = if resolved.engine.is_some() {
        engine::resolve(resolved, None)?
            .into_iter()
            .collect::<Vec<_>>()
    } else {
        engine::discover()?
    };
    Ok(
        json!({"count": engines.len(), "total": engines.len(), "scope": "local validated Unreal installations", "items": engines, "help": if engines.is_empty() { json!(["magi-unreal-axi --engine <path> engine view"]) } else { json!(["magi-unreal-axi engine view"]) }}),
    )
}

fn require_project(project: Option<&Path>) -> Result<&Path, AppError> {
    project.ok_or_else(|| {
        AppError::operational(
            "project",
            "project_not_found",
            "no Unreal project selected",
            "magi-unreal-axi --project <path> project view",
        )
    })
}

fn require_engine(engine: Option<engine::EngineInfo>) -> Result<engine::EngineInfo, AppError> {
    engine.ok_or_else(|| {
        AppError::operational(
            "engine",
            "engine_not_found",
            "no validated Unreal engine resolved",
            "magi-unreal-axi --engine <path> engine view",
        )
    })
}

fn collapsed_executable() -> String {
    let executable = std::env::current_exe()
        .map(|path| path.display().to_string())
        .unwrap_or_else(|_| "magi-unreal-axi".into());
    std::env::var_os("HOME")
        .and_then(|home| {
            executable
                .strip_prefix(home.to_string_lossy().as_ref())
                .map(|rest| format!("~{rest}"))
        })
        .unwrap_or(executable)
}

fn format_from_raw_args(args: &[OsString]) -> Format {
    args.iter()
        .enumerate()
        .find_map(|(index, arg)| {
            let arg = arg.to_str()?;
            if arg == "--format" {
                args.get(index + 1)?.to_str().and_then(parse_format)
            } else {
                arg.strip_prefix("--format=").and_then(parse_format)
            }
        })
        .or_else(format_from_environment)
        .unwrap_or(Format::Toon)
}

fn format_from_environment() -> Option<Format> {
    std::env::var("MAGI_UNREAL_FORMAT")
        .ok()
        .and_then(|value| parse_format(&value))
}

fn parse_format(value: &str) -> Option<Format> {
    match value {
        "toon" => Some(Format::Toon),
        "json" => Some(Format::Json),
        _ => None,
    }
}

fn output_error(error: serde_json::Error) -> AppError {
    AppError::operational(
        "output",
        "serialization_failed",
        error.to_string(),
        "magi-unreal-axi --format json",
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use clap::CommandFactory;

    #[test]
    fn clap_schema_is_valid() {
        Cli::command().debug_assert();
    }

    #[test]
    fn global_format_parses_before_and_after_subcommands() {
        assert_eq!(
            Cli::try_parse_from(["magi-unreal-axi", "--format", "json", "project", "view"])
                .unwrap()
                .format,
            Some(Format::Json)
        );
        assert_eq!(
            Cli::try_parse_from(["magi-unreal-axi", "project", "view", "--format", "json"])
                .unwrap()
                .format,
            Some(Format::Json)
        );
    }
}
