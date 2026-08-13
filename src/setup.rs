use crate::{
    cli::{PluginCommand, PluginMutationArgs},
    config,
    engine::EngineInfo,
    error::AppError,
    process::{self, Invocation},
    project,
};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::{
    collections::BTreeMap,
    fs,
    path::{Path, PathBuf},
    time::{Duration, SystemTime, UNIX_EPOCH},
};

const PAYLOAD_VERSION: &str = env!("CARGO_PKG_VERSION");
const MANIFEST_NAME: &str = ".magi-unreal-axi.manifest.json";
const SUPPORTED_ENGINE: &str = "5.8.1";
const SUPPORTED_CHANGELIST: u64 = 56_057_345;
const PAYLOAD: &[(&str, &[u8])] = &[
    (
        "MagiUnrealAXI.uplugin",
        include_bytes!("../plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin"),
    ),
    (
        "Source/MagiUnrealAXI/MagiUnrealAXI.Build.cs",
        include_bytes!("../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/MagiUnrealAXI.Build.cs"),
    ),
    (
        "Source/MagiUnrealAXI/Public/MagiUnrealAXI.h",
        include_bytes!("../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Public/MagiUnrealAXI.h"),
    ),
    (
        "Source/MagiUnrealAXI/Public/MagiAxiCatalog.generated.h",
        include_bytes!(
            "../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Public/MagiAxiCatalog.generated.h"
        ),
    ),
    (
        "Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp",
        include_bytes!("../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp"),
    ),
    (
        "Source/MagiUnrealAXI/Private/MagiBlueprintAuthoring.inl",
        include_bytes!(
            "../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiBlueprintAuthoring.inl"
        ),
    ),
    (
        "Source/MagiUnrealAXI/Private/MagiWidgetAuthoring.inl",
        include_bytes!(
            "../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiWidgetAuthoring.inl"
        ),
    ),
    (
        "Source/MagiUnrealAXI/Private/MagiAiNavigation.inl",
        include_bytes!("../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiAiNavigation.inl"),
    ),
    (
        "Source/MagiUnrealAXI/Private/MagiAnimationAuthoring.inl",
        include_bytes!(
            "../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiAnimationAuthoring.inl"
        ),
    ),
];

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
struct Manifest {
    payload_version: String,
    source_hash: String,
    engine_version: String,
    engine_changelist: u64,
    files: BTreeMap<String, String>,
    descriptor_entry_before: Value,
    descriptor_entry_installed: Value,
}

pub fn execute(
    command: &PluginCommand,
    project_path: &Path,
    engine: Option<&EngineInfo>,
) -> Result<Value, AppError> {
    let root = project_path
        .parent()
        .ok_or_else(|| setup_error("invalid_project", "project has no parent"))?;
    let destination = safe_destination(root)?;
    match command {
        PluginCommand::Status => status(&destination, engine),
        PluginCommand::Install(args)
        | PluginCommand::Update(args)
        | PluginCommand::Repair(args) => {
            let engine = engine.ok_or_else(|| {
                AppError::operational(
                    "engine",
                    "engine_not_found",
                    "plugin build requires a validated Unreal engine",
                    "magi-unreal-axi --engine <path> setup plugin install",
                )
            })?;
            install(args, project_path, &destination, engine)
        }
        PluginCommand::Uninstall(args) => uninstall(args, project_path, &destination),
    }
}

fn status(destination: &Path, engine: Option<&EngineInfo>) -> Result<Value, AppError> {
    if !destination.exists() {
        return Ok(
            json!({"plugin": {"installed": false, "managed": false, "compatible": false, "path": destination}, "help": ["magi-unreal-axi setup plugin install --dry-run"]}),
        );
    }
    reject_symlink(destination)?;
    let manifest = read_manifest(destination).ok();
    let managed = manifest
        .as_ref()
        .is_some_and(|manifest| tree_matches(destination, manifest).unwrap_or(false));
    let compatible = managed
        && manifest
            .as_ref()
            .zip(engine)
            .is_some_and(|(manifest, engine)| {
                supported_engine(engine)
                    && manifest.payload_version == PAYLOAD_VERSION
                    && manifest.source_hash == source_hash()
                    && manifest.engine_version == engine.version
                    && manifest.engine_changelist == engine.changelist
            });
    Ok(
        json!({"plugin": {"installed": true, "managed": managed, "compatible": compatible, "version": manifest.as_ref().map(|manifest| &manifest.payload_version), "engineVersion": manifest.as_ref().map(|manifest| &manifest.engine_version), "path": destination}}),
    )
}

fn install(
    args: &PluginMutationArgs,
    project_path: &Path,
    destination: &Path,
    engine: &EngineInfo,
) -> Result<Value, AppError> {
    ensure_supported_engine(engine)?;
    let current_manifest = destination
        .exists()
        .then(|| read_manifest(destination).ok())
        .flatten();
    let current_matches = current_manifest
        .as_ref()
        .is_some_and(|manifest| tree_matches(destination, manifest).unwrap_or(false));
    let payload_matches = current_manifest.as_ref().is_some_and(|manifest| {
        manifest.payload_version == PAYLOAD_VERSION
            && manifest.source_hash == source_hash()
            && manifest.engine_version == engine.version
            && manifest.engine_changelist == engine.changelist
    });
    let (descriptor, before_entry, installed_entry) = enabled_descriptor(project_path)?;
    let descriptor_changed = before_entry != installed_entry;
    let descriptor_already_enabled = !descriptor_changed;
    let original_descriptor_entry = current_manifest
        .as_ref()
        .filter(|manifest| before_entry == manifest.descriptor_entry_installed)
        .map(|manifest| manifest.descriptor_entry_before.clone())
        .unwrap_or_else(|| before_entry.clone());
    if current_matches && payload_matches && descriptor_already_enabled {
        return Ok(
            json!({"plugin": {"changed": false, "installed": true, "managed": true, "compatible": true, "version": PAYLOAD_VERSION, "path": destination}, "projectDescriptor": {"changed": false, "path": project_path}}),
        );
    }
    if destination.exists() && !current_matches && !args.force {
        return Err(AppError::operational(
            "setup",
            "plugin_modified",
            format!("{} is unmanaged or modified", destination.display()),
            "re-run with --force to preserve it as backup",
        ));
    }
    let invocation = build_plugin_invocation(engine, args.dry_run)?;
    if args.dry_run {
        return Ok(
            json!({"dryRun": true, "plugin": {"wouldChange": true, "path": destination, "preserveBackup": destination.exists() && !current_matches}, "build": invocation, "projectDescriptor": {"wouldChange": descriptor_changed, "path": project_path}}),
        );
    }

    let build_root = invocation.working_directory.clone();
    let package = build_root.join("package");
    if let Err(error) = process::run(&invocation, Duration::from_secs(600)) {
        let _ = fs::remove_dir_all(&build_root);
        return Err(error);
    }
    if let Err(error) = validate_package(&package) {
        let _ = fs::remove_dir_all(&build_root);
        return Err(error);
    }
    let parent = destination.parent().unwrap();
    fs::create_dir_all(parent).map_err(setup_io)?;
    let stage = unique_sibling(parent, ".MagiUnrealAXI.stage")?;
    copy_tree(&package, &stage)?;
    let manifest = Manifest {
        payload_version: PAYLOAD_VERSION.into(),
        source_hash: source_hash(),
        engine_version: engine.version.clone(),
        engine_changelist: engine.changelist,
        files: tree_hashes(&stage)?,
        descriptor_entry_before: original_descriptor_entry,
        descriptor_entry_installed: installed_entry,
    };
    write_manifest(&stage, &manifest)?;
    let _ = fs::remove_dir_all(&build_root);

    let old = if destination.exists() {
        let path = unique_sibling(parent, ".MagiUnrealAXI.previous")?;
        fs::rename(destination, &path).map_err(setup_io)?;
        Some(path)
    } else {
        None
    };
    if let Err(error) = fs::rename(&stage, destination) {
        if let Some(old) = old.as_ref() {
            let _ = fs::rename(old, destination);
        }
        let _ = fs::remove_dir_all(&stage);
        return Err(setup_io(error));
    }
    if descriptor_changed && let Err(error) = write_descriptor(project_path, &descriptor) {
        let _ = fs::remove_dir_all(destination);
        if let Some(old) = old.as_ref() {
            let _ = fs::rename(old, destination);
        }
        return Err(error);
    }
    let backup = match old {
        Some(old) if !current_matches => Some(old),
        Some(old) => {
            fs::remove_dir_all(old).map_err(setup_io)?;
            None
        }
        None => None,
    };
    Ok(
        json!({"plugin": {"changed": true, "installed": true, "managed": true, "compatible": true, "version": PAYLOAD_VERSION, "engineVersion": engine.version, "path": destination, "backup": backup}, "projectDescriptor": {"changed": descriptor_changed, "path": project_path}}),
    )
}

fn uninstall(
    args: &PluginMutationArgs,
    project_path: &Path,
    destination: &Path,
) -> Result<Value, AppError> {
    if !destination.exists() {
        return Ok(
            json!({"plugin": {"changed": false, "installed": false, "path": destination}, "projectDescriptor": {"changed": false, "path": project_path}}),
        );
    }
    reject_symlink(destination)?;
    let manifest = read_manifest(destination).ok();
    let matches = manifest
        .as_ref()
        .is_some_and(|manifest| tree_matches(destination, manifest).unwrap_or(false));
    if !matches && !args.force {
        return Err(AppError::operational(
            "setup",
            "plugin_modified",
            format!("{} is unmanaged or modified", destination.display()),
            "re-run with --force to preserve it as backup",
        ));
    }
    if args.dry_run {
        let descriptor_would_change = manifest.as_ref().is_some_and(|manifest| {
            manifest.descriptor_entry_before != manifest.descriptor_entry_installed
        });
        return Ok(
            json!({"dryRun": true, "plugin": {"wouldChange": true, "path": destination, "preserveBackup": !matches}, "projectDescriptor": {"wouldChange": descriptor_would_change, "path": project_path}}),
        );
    }
    let parent = destination.parent().unwrap();
    let removed = unique_sibling(parent, ".MagiUnrealAXI.removed")?;
    fs::rename(destination, &removed).map_err(setup_io)?;
    let descriptor_changed = if let Some(manifest) = manifest.as_ref() {
        match restore_descriptor(project_path, manifest) {
            Ok(changed) => changed,
            Err(error) => {
                let _ = fs::rename(&removed, destination);
                return Err(error);
            }
        }
    } else {
        false
    };
    if matches {
        fs::remove_dir_all(&removed).map_err(setup_io)?;
        Ok(
            json!({"plugin": {"changed": true, "installed": false, "path": destination}, "projectDescriptor": {"changed": descriptor_changed, "path": project_path}}),
        )
    } else {
        Ok(
            json!({"plugin": {"changed": true, "installed": false, "path": destination, "backup": removed}, "projectDescriptor": {"changed": descriptor_changed, "path": project_path}}),
        )
    }
}

fn build_plugin_invocation(engine: &EngineInfo, dry_run: bool) -> Result<Invocation, AppError> {
    let cache = std::env::var_os("HOME")
        .map(PathBuf::from)
        .ok_or_else(|| {
            setup_error(
                "home_unavailable",
                "HOME is required for canonical plugin build workspace",
            )
        })?
        .join("Library/Caches/magi-unreal-axi/plugin-build");
    let root = if dry_run {
        cache.join("<build-id>")
    } else {
        fs::create_dir_all(&cache).map_err(setup_io)?;
        let cache = cache.canonicalize().map_err(setup_io)?;
        if cache.starts_with("/tmp") || cache.starts_with("/private/tmp") {
            return Err(setup_error(
                "unsafe_build_workspace",
                "plugin build workspace resolves under /tmp",
            ));
        }
        let root = unique_sibling(&cache, "build")?;
        let source = root.join("source/MagiUnrealAXI");
        fs::create_dir_all(&source).map_err(setup_io)?;
        write_embedded_source(&source)?;
        root
    };
    let source = root.join("source/MagiUnrealAXI");
    Ok(Invocation {
        executable: engine.uat.clone(),
        arguments: vec![
            "BuildPlugin".into(),
            "-NoCompileUAT".into(),
            format!("-Plugin={}", source.join("MagiUnrealAXI.uplugin").display()),
            format!("-Package={}", root.join("package").display()),
            "-HostPlatforms=Mac".into(),
            "-TargetPlatforms=Mac".into(),
        ],
        working_directory: root,
        environment: BTreeMap::from([(
            "DOTNET_ROOT".into(),
            engine.dotnet_root.display().to_string(),
        )]),
        executes_project_code: false,
    })
}

fn validate_package(package: &Path) -> Result<(), AppError> {
    for path in [
        package.join("MagiUnrealAXI.uplugin"),
        package.join("Binaries/Mac/libUnrealEditor-MagiUnrealAXI.dylib"),
    ] {
        if !path.is_file() {
            return Err(setup_error(
                "plugin_build_incomplete",
                format!("BuildPlugin did not produce {}", path.display()),
            ));
        }
    }
    Ok(())
}

fn ensure_supported_engine(engine: &EngineInfo) -> Result<(), AppError> {
    if supported_engine(engine) {
        Ok(())
    } else {
        Err(AppError::operational(
            "setup",
            "unsupported_engine",
            format!(
                "plugin setup supports UE {SUPPORTED_ENGINE} changelist {SUPPORTED_CHANGELIST}; selected {} changelist {}",
                engine.version, engine.changelist
            ),
            "select certified UE 5.8.1 installation",
        ))
    }
}
fn supported_engine(engine: &EngineInfo) -> bool {
    engine.version == SUPPORTED_ENGINE && engine.changelist == SUPPORTED_CHANGELIST
}

fn safe_destination(project_root: &Path) -> Result<PathBuf, AppError> {
    let canonical_root = project_root.canonicalize().map_err(setup_io)?;
    reject_symlink(project_root)?;
    let plugins = canonical_root.join("Plugins");
    if plugins.exists() {
        reject_symlink(&plugins)?;
        let plugins = plugins.canonicalize().map_err(setup_io)?;
        if !plugins.starts_with(&canonical_root) {
            return Err(AppError::operational(
                "security",
                "path_escape",
                format!("{} escapes project", plugins.display()),
                "remove symlinked Plugins path",
            ));
        }
        Ok(plugins.join("MagiUnrealAXI"))
    } else {
        Ok(plugins.join("MagiUnrealAXI"))
    }
}

fn reject_symlink(path: &Path) -> Result<(), AppError> {
    if fs::symlink_metadata(path).is_ok_and(|metadata| metadata.file_type().is_symlink()) {
        Err(AppError::operational(
            "security",
            "symlink_refused",
            format!("refusing symlink path {}", path.display()),
            "replace symlink with a real project-local path",
        ))
    } else {
        Ok(())
    }
}

fn write_embedded_source(root: &Path) -> Result<(), AppError> {
    for (relative, bytes) in PAYLOAD {
        let path = root.join(relative);
        fs::create_dir_all(path.parent().unwrap()).map_err(setup_io)?;
        fs::write(path, bytes).map_err(setup_io)?;
    }
    Ok(())
}

fn copy_tree(source: &Path, destination: &Path) -> Result<(), AppError> {
    reject_symlink(source)?;
    fs::create_dir(destination).map_err(setup_io)?;
    for entry in fs::read_dir(source).map_err(setup_io)? {
        let entry = entry.map_err(setup_io)?;
        let from = entry.path();
        reject_symlink(&from)?;
        let to = destination.join(entry.file_name());
        if from.is_dir() {
            copy_tree(&from, &to)?;
        } else {
            fs::copy(from, to).map_err(setup_io)?;
        }
    }
    Ok(())
}

fn write_manifest(root: &Path, manifest: &Manifest) -> Result<(), AppError> {
    fs::write(
        root.join(MANIFEST_NAME),
        serde_json::to_vec_pretty(manifest)
            .map_err(|error| setup_error("manifest_failed", error.to_string()))?,
    )
    .map_err(setup_io)
}
fn read_manifest(root: &Path) -> Result<Manifest, AppError> {
    let path = root.join(MANIFEST_NAME);
    serde_json::from_slice(&fs::read(&path).map_err(setup_io)?)
        .map_err(|error| setup_error("manifest_malformed", format!("{}: {error}", path.display())))
}
fn tree_matches(root: &Path, manifest: &Manifest) -> Result<bool, AppError> {
    Ok(tree_hashes(root)? == manifest.files)
}
fn tree_hashes(root: &Path) -> Result<BTreeMap<String, String>, AppError> {
    fn visit(
        root: &Path,
        directory: &Path,
        output: &mut BTreeMap<String, String>,
    ) -> Result<(), AppError> {
        for entry in fs::read_dir(directory).map_err(setup_io)? {
            let path = entry.map_err(setup_io)?.path();
            reject_symlink(&path)?;
            if path.is_dir() {
                visit(root, &path, output)?;
            } else if path.file_name().is_none_or(|name| name != MANIFEST_NAME) {
                let relative = path
                    .strip_prefix(root)
                    .map_err(|error| setup_error("path_escape", error.to_string()))?
                    .to_string_lossy()
                    .into_owned();
                output.insert(relative, digest(&fs::read(path).map_err(setup_io)?));
            }
        }
        Ok(())
    }
    let mut output = BTreeMap::new();
    visit(root, root, &mut output)?;
    Ok(output)
}

fn enabled_descriptor(path: &Path) -> Result<(Value, Value, Value), AppError> {
    let mut descriptor = project::descriptor(path)?;
    let plugins = descriptor
        .as_object_mut()
        .ok_or_else(|| setup_error("descriptor_malformed", "descriptor root must be object"))?
        .entry("Plugins")
        .or_insert_with(|| json!([]))
        .as_array_mut()
        .ok_or_else(|| setup_error("descriptor_malformed", "Plugins must be array"))?;
    let index = plugins
        .iter()
        .position(|entry| entry.get("Name").and_then(Value::as_str) == Some("MagiUnrealAXI"));
    let before = index
        .map(|index| plugins[index].clone())
        .unwrap_or(Value::Null);
    let mut installed = before.as_object().cloned().unwrap_or_default();
    installed.insert("Name".into(), json!("MagiUnrealAXI"));
    installed.insert("Enabled".into(), json!(true));
    let installed = Value::Object(installed);
    if let Some(index) = index {
        plugins[index] = installed.clone();
    } else {
        plugins.push(installed.clone());
    }
    Ok((descriptor, before, installed))
}

fn restore_descriptor(path: &Path, manifest: &Manifest) -> Result<bool, AppError> {
    let mut descriptor = project::descriptor(path)?;
    let plugins = descriptor
        .get_mut("Plugins")
        .and_then(Value::as_array_mut)
        .ok_or_else(|| setup_error("descriptor_changed", "descriptor Plugins array is missing"))?;
    let index = plugins
        .iter()
        .position(|entry| entry.get("Name").and_then(Value::as_str) == Some("MagiUnrealAXI"))
        .ok_or_else(|| {
            setup_error(
                "descriptor_changed",
                "managed plugin descriptor entry is missing",
            )
        })?;
    if plugins[index] != manifest.descriptor_entry_installed {
        return Err(AppError::operational(
            "setup",
            "descriptor_changed",
            "managed plugin descriptor entry was modified",
            "restore descriptor entry before uninstall",
        ));
    }
    if manifest.descriptor_entry_before == manifest.descriptor_entry_installed {
        return Ok(false);
    }
    if manifest.descriptor_entry_before.is_null() {
        plugins.remove(index);
    } else {
        plugins[index] = manifest.descriptor_entry_before.clone();
    }
    write_descriptor(path, &descriptor)?;
    Ok(true)
}
fn write_descriptor(path: &Path, descriptor: &Value) -> Result<(), AppError> {
    config::atomic_write(
        path,
        &serde_json::to_vec_pretty(descriptor)
            .map_err(|error| setup_error("descriptor_write_failed", error.to_string()))?,
    )
    .map_err(setup_io)
}

fn unique_sibling(parent: &Path, prefix: &str) -> Result<PathBuf, AppError> {
    let stamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos();
    let path = parent.join(format!("{prefix}.{}.{stamp}", std::process::id()));
    if path.exists() {
        Err(setup_error(
            "temporary_path_collision",
            format!("{} already exists", path.display()),
        ))
    } else {
        Ok(path)
    }
}
fn source_hash() -> String {
    let mut hash = Sha256::new();
    for (path, bytes) in PAYLOAD {
        hash.update(path.as_bytes());
        hash.update(bytes);
    }
    format!("{:x}", hash.finalize())
}
fn digest(bytes: &[u8]) -> String {
    format!("{:x}", Sha256::digest(bytes))
}
fn setup_io(error: std::io::Error) -> AppError {
    setup_error("setup_io_failed", error.to_string())
}
fn setup_error(reason: &'static str, message: impl Into<String>) -> AppError {
    AppError::operational(
        "setup",
        reason,
        message,
        "magi-unreal-axi setup plugin status",
    )
}

#[cfg(test)]
mod tests {
    use super::PAYLOAD;
    use std::collections::BTreeSet;
    #[test]
    fn embedded_payload_contains_every_local_magi_inl_include() {
        let source =
            include_str!("../plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp");
        let embedded: BTreeSet<_> = PAYLOAD
            .iter()
            .map(|(path, _)| path.rsplit('/').next().unwrap())
            .collect();
        let includes: BTreeSet<_> = source
            .lines()
            .filter_map(|line| line.trim().strip_prefix("#include \"")?.strip_suffix("\""))
            .filter(|name| name.starts_with("Magi") && name.ends_with(".inl"))
            .collect();
        assert!(!includes.is_empty());
        assert!(
            includes.is_subset(&embedded),
            "missing embedded includes: {:?}",
            includes.difference(&embedded).collect::<Vec<_>>()
        );
    }
}
