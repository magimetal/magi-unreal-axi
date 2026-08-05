use crate::{
    cli::{Cli, Format},
    error::AppError,
};
use std::{
    fs::{self, OpenOptions},
    io::Write,
    path::{Path, PathBuf},
};

#[cfg(unix)]
use std::os::unix::fs::{MetadataExt, PermissionsExt};

pub fn validate_existing_target(path: &Path) -> std::io::Result<bool> {
    let metadata = match fs::symlink_metadata(path) {
        Ok(metadata) => metadata,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => return Ok(false),
        Err(error) => return Err(error),
    };
    if !metadata.file_type().is_file() {
        return Err(std::io::Error::other(
            "existing target is not a regular file",
        ));
    }
    // SAFETY: getuid has no preconditions and only reads process credentials.
    if metadata.uid() != unsafe { libc::getuid() } {
        return Err(std::io::Error::other(
            "existing target is not owned by current user",
        ));
    }
    Ok(true)
}

#[derive(Clone, Debug)]
pub struct ResolvedConfig {
    pub engine: Option<PathBuf>,
    pub editor: Option<u32>,
    pub timeout_seconds: Option<u64>,
    pub format: Format,
}
#[derive(Default)]
struct FileConfig {
    engine: Option<PathBuf>,
    editor: Option<u32>,
    timeout_seconds: Option<u64>,
    format: Option<Format>,
}

pub fn resolve(cli: &Cli, project: Option<&Path>) -> Result<ResolvedConfig, AppError> {
    let env_engine = if cli.engine.is_none() {
        env_nonempty("MAGI_UNREAL_ENGINE")?.map(PathBuf::from)
    } else {
        None
    };
    let env_editor = if cli.editor.is_none() {
        env_nonempty("MAGI_UNREAL_EDITOR")?
            .map(|value| parse_u32_env("MAGI_UNREAL_EDITOR", &value))
            .transpose()?
    } else {
        None
    };
    let env_timeout = if cli.timeout.is_none() {
        env_nonempty("MAGI_UNREAL_TIMEOUT")?
            .map(|value| parse_timeout("MAGI_UNREAL_TIMEOUT", &value))
            .transpose()?
    } else {
        None
    };
    let env_format = if cli.format.is_none() {
        env_nonempty("MAGI_UNREAL_FORMAT")?
            .map(|value| parse_format_env(&value))
            .transpose()?
    } else {
        None
    };

    let needs_files = cli.engine.is_none() && env_engine.is_none()
        || cli.editor.is_none() && env_editor.is_none()
        || cli.timeout.is_none() && env_timeout.is_none()
        || cli.format.is_none() && env_format.is_none();
    let (project_file, global_file) = if needs_files {
        (
            project
                .map(|path| read_file_config(&path.parent().unwrap().join(".magi/unreal-axi.toml")))
                .transpose()?,
            global_config_path()
                .map(|path| read_file_config(&path))
                .transpose()?,
        )
    } else {
        (None, None)
    };

    let from_project = project_file.unwrap_or_default();
    let from_global = global_file.unwrap_or_default();
    Ok(ResolvedConfig {
        engine: cli
            .engine
            .clone()
            .or(env_engine)
            .or(from_project.engine)
            .or(from_global.engine),
        editor: cli
            .editor
            .or(env_editor)
            .or(from_project.editor)
            .or(from_global.editor),
        timeout_seconds: cli
            .timeout
            .or(env_timeout)
            .or(from_project.timeout_seconds)
            .or(from_global.timeout_seconds),
        format: cli
            .format
            .or(env_format)
            .or(from_project.format)
            .or(from_global.format)
            .unwrap_or(Format::Toon),
    })
}

fn read_file_config(path: &Path) -> Result<FileConfig, AppError> {
    let text = match fs::read_to_string(path) {
        Ok(text) => text,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => {
            return Ok(FileConfig::default());
        }
        Err(error) => return Err(config_error("config_unreadable", path, error.to_string())),
    };
    let value: toml::Value = toml::from_str(&text)
        .map_err(|error| config_error("config_malformed", path, error.to_string()))?;
    let table = value
        .as_table()
        .ok_or_else(|| config_error("config_malformed", path, "root must be a TOML table"))?;
    let engine = table
        .get("engine")
        .map(|value| {
            value
                .as_str()
                .filter(|value| !value.is_empty())
                .map(PathBuf::from)
                .ok_or_else(|| {
                    config_error(
                        "config_value_invalid",
                        path,
                        "engine must be a non-empty string",
                    )
                })
        })
        .transpose()?;
    let editor = table
        .get("editor")
        .map(|value| {
            value
                .as_integer()
                .and_then(|value| u32::try_from(value).ok())
                .filter(|value| *value > 0)
                .ok_or_else(|| {
                    config_error(
                        "config_value_invalid",
                        path,
                        "editor must be a positive PID",
                    )
                })
        })
        .transpose()?;
    let timeout_seconds = table
        .get("timeout")
        .map(|value| {
            value
                .as_integer()
                .and_then(|value| u64::try_from(value).ok())
                .filter(|value| (1..=86_400).contains(value))
                .ok_or_else(|| {
                    config_error(
                        "config_value_invalid",
                        path,
                        "timeout must be 1..=86400 seconds",
                    )
                })
        })
        .transpose()?;
    let format = table
        .get("format")
        .map(|value| match value.as_str() {
            Some("toon") => Ok(Format::Toon),
            Some("json") => Ok(Format::Json),
            _ => Err(config_error(
                "config_value_invalid",
                path,
                "format must be toon or json",
            )),
        })
        .transpose()?;
    Ok(FileConfig {
        engine,
        editor,
        timeout_seconds,
        format,
    })
}

fn config_error(reason: &'static str, path: &Path, message: impl std::fmt::Display) -> AppError {
    AppError::operational(
        "project",
        reason,
        format!("config {}: {message}", path.display()),
        format!("fix {}", path.display()),
    )
}

fn env_nonempty(name: &'static str) -> Result<Option<String>, AppError> {
    match std::env::var(name) {
        Ok(value) if value.is_empty() => Err(AppError::operational(
            "project",
            "empty_environment_value",
            format!("{name} is empty"),
            format!("unset {name} or provide a value"),
        )),
        Ok(value) => Ok(Some(value)),
        Err(std::env::VarError::NotPresent) => Ok(None),
        Err(error) => Err(AppError::operational(
            "project",
            "invalid_environment_value",
            format!("{name}: {error}"),
            format!("unset {name}"),
        )),
    }
}

fn parse_u32_env(name: &str, value: &str) -> Result<u32, AppError> {
    value
        .parse::<u32>()
        .ok()
        .filter(|value| *value > 0)
        .ok_or_else(|| {
            AppError::operational(
                "project",
                "invalid_environment_value",
                format!("{name} must be a positive PID"),
                format!("fix or unset {name}"),
            )
        })
}

fn parse_timeout(name: &str, value: &str) -> Result<u64, AppError> {
    value
        .parse::<u64>()
        .ok()
        .filter(|value| (1..=86_400).contains(value))
        .ok_or_else(|| {
            AppError::operational(
                "project",
                "invalid_environment_value",
                format!("{name} must be 1..=86400 seconds"),
                format!("fix or unset {name}"),
            )
        })
}

fn parse_format_env(value: &str) -> Result<Format, AppError> {
    match value {
        "toon" => Ok(Format::Toon),
        "json" => Ok(Format::Json),
        _ => Err(AppError::operational(
            "output",
            "invalid_environment_value",
            "MAGI_UNREAL_FORMAT must be toon or json",
            "fix or unset MAGI_UNREAL_FORMAT",
        )),
    }
}

pub fn global_config_path() -> Option<PathBuf> {
    std::env::var_os("XDG_CONFIG_HOME")
        .map(PathBuf::from)
        .or_else(|| std::env::var_os("HOME").map(|home| PathBuf::from(home).join(".config")))
        .map(|root| root.join("magi-unreal-axi/config.toml"))
}

pub fn atomic_write(path: &Path, bytes: &[u8]) -> std::io::Result<()> {
    atomic_write_batch(&[(path, bytes)])
}

pub fn atomic_write_batch(writes: &[(&Path, &[u8])]) -> std::io::Result<()> {
    let mut staged = Vec::with_capacity(writes.len());
    let mut backups = Vec::with_capacity(writes.len());
    let mut committed = Vec::with_capacity(writes.len());
    let result = (|| {
        for (path, bytes) in writes {
            let parent = path
                .parent()
                .ok_or_else(|| std::io::Error::other("path has no parent"))?;
            fs::create_dir_all(parent)?;
            let index = staged.len();
            let tmp = parent.join(format!(
                ".{}.{}.{}.tmp",
                path.file_name().unwrap_or_default().to_string_lossy(),
                std::process::id(),
                index
            ));
            let mut file = OpenOptions::new().write(true).create_new(true).open(&tmp)?;
            staged.push((tmp, path.to_path_buf()));
            #[cfg(unix)]
            {
                let mode = fs::metadata(path)
                    .map(|metadata| metadata.permissions().mode() & 0o777)
                    .unwrap_or(0o600);
                file.set_permissions(fs::Permissions::from_mode(mode))?;
            }
            file.write_all(bytes)?;
            file.sync_all()?;
        }
        for (index, (_, path)) in staged.iter().enumerate() {
            if fs::symlink_metadata(path)
                .map(|metadata| !metadata.file_type().is_dir())
                .unwrap_or(false)
            {
                let backup = unique_backup_path(path, index)?;
                fs::rename(path, &backup)?;
                backups.push((backup, path.clone()));
            }
        }
        for (tmp, path) in &staged {
            fs::rename(tmp, path)?;
            committed.push(path.clone());
        }
        Ok(())
    })();
    if result.is_err() {
        for path in committed.iter().rev() {
            let _ = fs::remove_file(path);
        }
        for (backup, path) in backups.iter().rev() {
            let _ = fs::rename(backup, path);
        }
    }
    for (tmp, _) in &staged {
        let _ = fs::remove_file(tmp);
    }
    for (backup, _) in &backups {
        let _ = fs::remove_file(backup);
    }
    result
}

fn unique_backup_path(path: &Path, index: usize) -> std::io::Result<PathBuf> {
    let parent = path
        .parent()
        .ok_or_else(|| std::io::Error::other("path has no parent"))?;
    let name = path.file_name().unwrap_or_default().to_string_lossy();
    for attempt in 0..100 {
        let backup = parent.join(format!(
            ".{name}.{}.{}.{}.bak",
            std::process::id(),
            index,
            attempt
        ));
        match OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&backup)
        {
            Ok(file) => {
                drop(file);
                fs::remove_file(&backup)?;
                return Ok(backup);
            }
            Err(error) if error.kind() == std::io::ErrorKind::AlreadyExists => {}
            Err(error) => return Err(error),
        }
    }
    Err(std::io::Error::new(
        std::io::ErrorKind::AlreadyExists,
        "could not allocate unique backup path",
    ))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rolls_back_prior_commit_when_later_commit_fails() {
        let directory = tempfile::tempdir().unwrap();
        let first = directory.path().join("first");
        let second = directory.path().join("second");
        fs::write(&first, b"old-first").unwrap();
        fs::create_dir(&second).unwrap();
        #[cfg(unix)]
        fs::set_permissions(&first, fs::Permissions::from_mode(0o640)).unwrap();

        let result = atomic_write_batch(&[(&first, b"new-first"), (&second, b"new-second")]);
        assert!(result.is_err());
        assert_eq!(fs::read(&first).unwrap(), b"old-first");
        assert!(fs::metadata(&second).unwrap().is_dir());
        #[cfg(unix)]
        assert_eq!(
            fs::metadata(&first).unwrap().permissions().mode() & 0o777,
            0o640
        );
        let debris = fs::read_dir(directory.path())
            .unwrap()
            .map(|entry| entry.unwrap().file_name().to_string_lossy().into_owned())
            .filter(|name| name.contains(".tmp") || name.contains(".bak"))
            .collect::<Vec<_>>();
        assert!(debris.is_empty(), "debris: {debris:?}");
    }
}
