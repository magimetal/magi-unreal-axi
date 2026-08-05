use crate::{config::ResolvedConfig, error::AppError, project};
use serde::Serialize;
use serde_json::Value;
use std::{
    collections::BTreeSet,
    fs,
    path::{Path, PathBuf},
};

#[derive(Clone, Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct EngineInfo {
    pub root: PathBuf,
    pub version: String,
    pub changelist: u64,
    pub editor: PathBuf,
    pub editor_cmd: PathBuf,
    pub ubt: PathBuf,
    pub uat: PathBuf,
    pub dotnet_root: PathBuf,
}

pub fn resolve(
    config: &ResolvedConfig,
    selected_project: Option<&Path>,
) -> Result<Option<EngineInfo>, AppError> {
    if let Some(path) = config.engine.as_deref() {
        return validate(path).map(Some);
    }
    if let Some(project_path) = selected_project {
        for ancestor in project_path.ancestors().skip(1) {
            if ancestor.join("Engine/Build/Build.version").is_file() {
                return validate(ancestor).map(Some);
            }
        }
    }
    if let Some(project_path) = selected_project {
        let descriptor = project::descriptor(project_path)?;
        if let Some(association) = descriptor
            .get("EngineAssociation")
            .and_then(Value::as_str)
            .filter(|value| !value.is_empty())
        {
            if let Some(path) = registered_engine(association) {
                return validate(&path).map(Some);
            }
            let conventional = discovery_root().join(format!("UE_{association}"));
            if conventional.exists() {
                return validate(&conventional).map(Some);
            }
            let matching = discover()?
                .into_iter()
                .filter(|engine| engine.version.starts_with(association))
                .collect::<Vec<_>>();
            return match matching.len() {
                0 => Ok(None),
                1 => Ok(matching.into_iter().next()),
                _ => Err(engine_error(
                    "ambiguous_engine",
                    format!("multiple engines match association {association}"),
                )),
            };
        }
    }
    let engines = discover()?;
    Ok(match engines.len() {
        0 => None,
        1 => engines.into_iter().next(),
        _ => {
            return Err(AppError::operational(
                "engine",
                "ambiguous_engine",
                "multiple Unreal installations found without a resolvable EngineAssociation",
                "magi-unreal-axi --engine <path> engine view",
            ));
        }
    })
}

fn registered_engine(association: &str) -> Option<PathBuf> {
    let home = std::env::var_os("HOME")?;
    let path =
        PathBuf::from(home).join("Library/Application Support/Epic/UnrealEngine/Install.ini");
    let text = fs::read_to_string(path).ok()?;
    text.lines()
        .filter_map(|line| line.split_once('='))
        .find_map(|(key, value)| (key.trim() == association).then(|| PathBuf::from(value.trim())))
}

pub fn discover() -> Result<Vec<EngineInfo>, AppError> {
    let root = discovery_root();
    let mut candidates = BTreeSet::new();
    if let Ok(entries) = fs::read_dir(&root) {
        for entry in entries.flatten() {
            if entry.file_name().to_string_lossy().starts_with("UE_") {
                candidates.insert(entry.path());
            }
        }
    }
    let mut engines = candidates
        .into_iter()
        .filter_map(|path| validate(&path).ok())
        .collect::<Vec<_>>();
    engines.sort_by(|left, right| {
        right
            .version
            .cmp(&left.version)
            .then_with(|| left.root.cmp(&right.root))
    });
    Ok(engines)
}

fn discovery_root() -> PathBuf {
    std::env::var_os("MAGI_UNREAL_ENGINE_DISCOVERY_ROOT")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from("/Users/Shared/Epic Games"))
}

pub fn validate(root: &Path) -> Result<EngineInfo, AppError> {
    if root.as_os_str().is_empty() {
        return Err(engine_error(
            "empty_engine_selector",
            "engine path is empty",
        ));
    }
    let root = root.canonicalize().map_err(|error| {
        engine_error(
            "engine_not_found",
            format!("cannot resolve {}: {error}", root.display()),
        )
    })?;
    let version_path = root.join("Engine/Build/Build.version");
    let version_text = fs::read_to_string(&version_path).map_err(|error| {
        engine_error(
            "invalid_engine",
            format!("cannot read {}: {error}", version_path.display()),
        )
    })?;
    let metadata: Value = serde_json::from_str(&version_text).map_err(|error| {
        engine_error(
            "invalid_engine",
            format!("malformed {}: {error}", version_path.display()),
        )
    })?;
    let major = metadata
        .get("MajorVersion")
        .and_then(Value::as_u64)
        .ok_or_else(|| engine_error("invalid_engine", "Build.version lacks MajorVersion"))?;
    let minor = metadata
        .get("MinorVersion")
        .and_then(Value::as_u64)
        .ok_or_else(|| engine_error("invalid_engine", "Build.version lacks MinorVersion"))?;
    let patch = metadata
        .get("PatchVersion")
        .and_then(Value::as_u64)
        .unwrap_or(0);
    let changelist = metadata
        .get("Changelist")
        .and_then(Value::as_u64)
        .unwrap_or(0);
    let info = EngineInfo {
        version: format!("{major}.{minor}.{patch}"),
        changelist,
        editor: root.join("Engine/Binaries/Mac/UnrealEditor"),
        editor_cmd: root.join("Engine/Binaries/Mac/UnrealEditor-Cmd"),
        ubt: root.join("Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool"),
        uat: root.join("Engine/Build/BatchFiles/RunUAT.sh"),
        dotnet_root: root.join("Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"),
        root,
    };
    for (name, path) in [
        ("UnrealEditor", &info.editor),
        ("UnrealEditor-Cmd", &info.editor_cmd),
        ("UnrealBuildTool", &info.ubt),
        ("RunUAT", &info.uat),
    ] {
        if !is_executable(path) {
            return Err(engine_error(
                "invalid_engine",
                format!("{name} missing or not executable at {}", path.display()),
            ));
        }
    }
    let dotnet = info.dotnet_root.join("dotnet");
    if !is_executable(&dotnet) {
        return Err(engine_error(
            "invalid_engine",
            format!(
                "bundled dotnet missing or not executable at {}",
                dotnet.display()
            ),
        ));
    }
    Ok(info)
}

#[cfg(unix)]
fn is_executable(path: &Path) -> bool {
    use std::os::unix::fs::PermissionsExt;
    path.metadata()
        .is_ok_and(|metadata| metadata.is_file() && metadata.permissions().mode() & 0o111 != 0)
}

#[cfg(not(unix))]
fn is_executable(path: &Path) -> bool {
    path.is_file()
}

fn engine_error(reason: &'static str, message: impl Into<String>) -> AppError {
    AppError::operational(
        "engine",
        reason,
        message,
        "magi-unreal-axi --engine <path> engine view",
    )
}
