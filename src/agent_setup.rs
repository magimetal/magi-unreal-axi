use crate::{cli::AgentSetupArgs, config, error::AppError, project};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::{
    env, fs, io,
    path::{Path, PathBuf},
};

const SKILL: &str = include_str!("../skills/magi-unreal-axi/SKILL.md");
const MANAGED_SUFFIX: &str = " agent context --format json";

pub fn install(args: &AgentSetupArgs) -> Result<Value, AppError> {
    let home = env::var_os("HOME")
        .map(PathBuf::from)
        .ok_or_else(|| error("home_unavailable", "HOME is not set"))?;
    let all = !args.claude && !args.codex && !args.opencode;
    let mut plan = SetupPlan::default();
    if all || args.claude {
        plan.claude = Some(preflight_claude(&home)?);
        plan.skills.push(preflight_skill(
            &home,
            &home.join(".claude/skills/magi-unreal-axi/SKILL.md"),
        )?);
    }
    if all || args.codex {
        plan.skills.push(preflight_skill(
            &home,
            &home.join(".agents/skills/magi-unreal-axi/SKILL.md"),
        )?);
    }
    if all || args.opencode {
        plan.skills.push(preflight_skill(
            &home,
            &home.join(".config/opencode/skills/magi-unreal-axi/SKILL.md"),
        )?);
    }
    let mut changed = false;
    let mut targets = Vec::new();
    let mut writes = Vec::<(PathBuf, Vec<u8>)>::new();
    if let Some(claude) = plan.claude {
        changed |= claude.write;
        if claude.write {
            writes.push((claude.path, claude.bytes));
        }
        targets.push(json!({"name":"claude","skill":"installed","ambientContext":"SessionStart"}));
    }
    for skill in plan.skills {
        changed |= !skill.writes.is_empty();
        writes.extend(skill.writes);
    }
    if all || args.codex {
        targets.push(json!({"name":"codex","skill":"installed","ambientContext":"n/a","reason":"no documented command session-start hook; official Agent Skill integration installed"}));
    }
    if all || args.opencode {
        targets.push(json!({"name":"opencode","skill":"installed","ambientContext":"n/a","reason":"ambient plugins require JavaScript/TypeScript, prohibited by this distribution"}));
    }
    let references = writes
        .iter()
        .map(|(path, bytes)| (path.as_path(), bytes.as_slice()))
        .collect::<Vec<_>>();
    if !references.is_empty() {
        config::atomic_write_batch(&references)
            .map_err(|error| io_error(references[0].0, error))?;
    }
    Ok(json!({"changed":changed,"skill":"magi-unreal-axi","targets":targets}))
}

#[derive(Default)]
struct SetupPlan {
    claude: Option<ClaudePlan>,
    skills: Vec<SkillPlan>,
}
struct SkillPlan {
    writes: Vec<(PathBuf, Vec<u8>)>,
}
struct ClaudePlan {
    path: PathBuf,
    bytes: Vec<u8>,
    write: bool,
}

fn preflight_skill(home: &Path, path: &Path) -> Result<SkillPlan, AppError> {
    reject_symlink_ancestors(home, path)?;
    let sidecar = path.with_file_name("SKILL.md.magi-unreal-axi.sha256");
    reject_symlink_ancestors(home, &sidecar)?;
    let existing = read_target(path)?;
    let recorded = read_target(&sidecar)?
        .map(|bytes| {
            String::from_utf8(bytes).map_err(|_| error("malformed_sidecar", "sidecar is not UTF-8"))
        })
        .transpose()?;
    if let Some(value) = recorded.as_deref()
        && (value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()))
    {
        return Err(error(
            "malformed_sidecar",
            &format!("invalid sidecar {}", sidecar.display()),
        ));
    }
    let expected = format!("{:x}", Sha256::digest(SKILL.as_bytes()));
    let mut writes = Vec::new();
    match existing {
        None => {
            writes.push((path.to_owned(), SKILL.as_bytes().to_vec()));
        }
        Some(bytes) if bytes == SKILL.as_bytes() => {}
        Some(bytes) => {
            let hash = format!("{:x}", Sha256::digest(&bytes));
            if recorded.as_deref() != Some(hash.as_str()) {
                return Err(error(
                    "skill_modified",
                    &format!("refusing to overwrite modified skill {}", path.display()),
                ));
            }
            writes.push((path.to_owned(), SKILL.as_bytes().to_vec()));
        }
    }
    if recorded.as_deref() != Some(expected.as_str()) {
        writes.push((sidecar, expected.into_bytes()));
    }
    Ok(SkillPlan { writes })
}

fn read_target(path: &Path) -> Result<Option<Vec<u8>>, AppError> {
    config::validate_existing_target(path).map_err(|error| io_error(path, error))?;
    match fs::read(path) {
        Ok(bytes) => Ok(Some(bytes)),
        Err(error) if error.kind() == io::ErrorKind::NotFound => Ok(None),
        Err(error) => Err(io_error(path, error)),
    }
}
fn preflight_claude(home: &Path) -> Result<ClaudePlan, AppError> {
    let path = home.join(".claude/settings.json");
    reject_symlink_ancestors(home, &path)?;
    config::validate_existing_target(&path).map_err(|error| io_error(&path, error))?;
    let source = match fs::read_to_string(&path) {
        Ok(text) => text,
        Err(error) if error.kind() == io::ErrorKind::NotFound => "{}".into(),
        Err(error) => return Err(io_error(&path, error)),
    };
    let mut root: Value =
        serde_json::from_str(&source).map_err(|error| config_error(&path, error.to_string()))?;
    let object = root
        .as_object_mut()
        .ok_or_else(|| config_error(&path, "root must be an object".into()))?;
    let hooks = object.entry("hooks").or_insert_with(|| json!({}));
    let sessions = hooks
        .as_object_mut()
        .ok_or_else(|| config_error(&path, "hooks must be an object".into()))?
        .entry("SessionStart")
        .or_insert_with(|| json!([]));
    let list = sessions
        .as_array_mut()
        .ok_or_else(|| config_error(&path, "hooks.SessionStart must be an array".into()))?;
    let command = format!(
        "{}{}",
        shell_quote(&hook_executable(&path)?),
        MANAGED_SUFFIX
    );
    remove_managed_hooks(list);
    list.push(json!({"matcher":"","hooks":[{"type":"command","command":command,"timeout":5}]}));
    let bytes =
        serde_json::to_vec_pretty(&root).map_err(|error| config_error(&path, error.to_string()))?;
    Ok(ClaudePlan {
        write: source.as_bytes() != bytes.as_slice(),
        path,
        bytes,
    })
}

fn hook_executable(path: &Path) -> Result<String, AppError> {
    let current = env::current_exe()
        .and_then(|path| path.canonicalize())
        .map_err(|error| io_error(path, error))?;
    if current
        .file_name()
        .is_some_and(|name| name == "magi-unreal-axi")
        && let Some(paths) = env::var_os("PATH")
    {
        for directory in env::split_paths(&paths) {
            let candidate = directory.join("magi-unreal-axi");
            if candidate.canonicalize().is_ok_and(|path| path == current) {
                return Ok("magi-unreal-axi".into());
            }
        }
    }
    current.to_str().map(str::to_owned).ok_or_else(|| {
        error(
            "executable_path_invalid",
            "current executable path is not UTF-8",
        )
    })
}

fn remove_managed_hooks(list: &mut Vec<Value>) {
    list.retain_mut(|entry| {
        let Some(object) = entry.as_object_mut() else {
            return true;
        };
        if object.get("matcher").and_then(Value::as_str) != Some("") {
            return true;
        }
        let Some(hooks) = object.get_mut("hooks").and_then(Value::as_array_mut) else {
            return true;
        };
        hooks.retain(|hook| !is_managed_hook(hook));
        !hooks.is_empty() || object.len() != 2
    });
}

fn is_managed_hook(value: &Value) -> bool {
    let Some(hook) = value.as_object() else {
        return false;
    };
    matches!(hook.len(), 2 | 3)
        && hook.get("type").and_then(Value::as_str) == Some("command")
        && hook
            .get("command")
            .and_then(Value::as_str)
            .is_some_and(|command| command.ends_with(MANAGED_SUFFIX))
        && hook
            .keys()
            .all(|key| matches!(key.as_str(), "type" | "command" | "timeout"))
}

fn shell_quote(value: &str) -> String {
    if value == "magi-unreal-axi" {
        value.into()
    } else {
        format!("'{}'", value.replace('\'', "'\\''"))
    }
}

pub fn context(cwd: &Path) -> Result<Value, AppError> {
    let selected = project::select(None, cwd)?;
    let project = selected.as_ref().map(|path| {
        let plugin = path.parent().is_some_and(|root| root.join("Plugins/MagiUnrealAXI/MagiUnrealAXI.uplugin").is_file());
        json!({"name":path.file_stem().and_then(|name| name.to_str()).unwrap_or("project"),"path":path,"pluginInstalled":plugin})
    });
    let mut value = json!({"magiUnrealAxi":{"scope":cwd,"project":project,"next":if selected.is_some() { "magi-unreal-axi project doctor" } else { "magi-unreal-axi --project <path> project view" }}});
    if serde_json::to_vec(&value).is_ok_and(|bytes| bytes.len() >= 400) {
        value["magiUnrealAxi"]["project"]
            .as_object_mut()
            .map(|project| project.remove("path"));
        value["magiUnrealAxi"]
            .as_object_mut()
            .map(|root| root.remove("scope"));
    }
    Ok(value)
}

fn reject_symlink_ancestors(home: &Path, path: &Path) -> Result<(), AppError> {
    if !path.starts_with(home) {
        return Err(error("path_escape", "agent integration path escapes HOME"));
    }
    let mut current = Some(path);
    while let Some(candidate) = current {
        if fs::symlink_metadata(candidate).is_ok_and(|metadata| metadata.file_type().is_symlink()) {
            return Err(AppError::operational(
                "agent_setup",
                "symlink_refused",
                format!("refusing symlink path {}", candidate.display()),
                "replace symlink with a real user-local path",
            ));
        }
        if candidate == home {
            break;
        }
        current = candidate.parent();
    }
    Ok(())
}

fn error(reason: &'static str, message: &str) -> AppError {
    AppError::operational(
        "agent_setup",
        reason,
        message,
        "set HOME to a writable user directory",
    )
}

fn config_error(path: &Path, message: String) -> AppError {
    AppError::operational(
        "agent_setup",
        "malformed_config",
        format!("{}: {message}", path.display()),
        "repair config JSON and retry",
    )
}

fn io_error(path: &Path, error: io::Error) -> AppError {
    AppError::operational(
        "agent_setup",
        "io_failed",
        format!("{}: {error}", path.display()),
        "check permissions and retry",
    )
}
