use crate::error::AppError;
use serde_json::Value;
use std::{
    fs,
    path::{Path, PathBuf},
};

const MAX_DESCRIPTOR_BYTES: u64 = 1024 * 1024;

pub fn select(cli_project: Option<&Path>, cwd: &Path) -> Result<Option<PathBuf>, AppError> {
    if cli_project.is_some_and(|path| path.as_os_str().is_empty()) {
        return Err(AppError::usage(
            "empty_project_selector",
            "--project cannot be empty",
            "magi-unreal-axi --project <path>",
        ));
    }
    if let Some(selected) = cli_project {
        return resolve_selected(selected).map(Some);
    }
    let env_project = match std::env::var_os("MAGI_UNREAL_PROJECT") {
        Some(value) if value.is_empty() => {
            return Err(AppError::operational(
                "project",
                "empty_environment_value",
                "MAGI_UNREAL_PROJECT is empty",
                "unset MAGI_UNREAL_PROJECT or provide a project path",
            ));
        }
        value => value.map(PathBuf::from),
    };
    if let Some(selected) = env_project {
        return resolve_selected(&selected).map(Some);
    }
    let mut current = Some(cwd);
    while let Some(directory) = current {
        let projects = projects_in(directory)?;
        match projects.len() {
            0 => current = directory.parent(),
            1 => return Ok(projects.into_iter().next()),
            _ => {
                return Err(AppError::operational(
                    "project",
                    "ambiguous_project",
                    format!("multiple .uproject files in {}", directory.display()),
                    "magi-unreal-axi --project <path>",
                ));
            }
        }
    }
    Ok(None)
}

fn resolve_selected(path: &Path) -> Result<PathBuf, AppError> {
    let canonical = path.canonicalize().map_err(|error| {
        AppError::operational(
            "project",
            "project_not_found",
            format!("cannot resolve {}: {error}", path.display()),
            "magi-unreal-axi --project <path> project doctor",
        )
    })?;
    if canonical.is_dir() {
        let projects = projects_in(&canonical)?;
        return match projects.len() {
            1 => Ok(projects.into_iter().next().unwrap()),
            0 => Err(AppError::operational(
                "project",
                "project_not_found",
                format!("no .uproject in {}", canonical.display()),
                "magi-unreal-axi --project <path>.uproject",
            )),
            _ => Err(AppError::operational(
                "project",
                "ambiguous_project",
                format!("multiple .uproject files in {}", canonical.display()),
                "magi-unreal-axi --project <path>.uproject",
            )),
        };
    }
    if canonical
        .extension()
        .is_some_and(|extension| extension == "uproject")
    {
        descriptor(&canonical)?;
        Ok(canonical)
    } else {
        Err(AppError::operational(
            "project",
            "invalid_project_path",
            format!("{} is not a .uproject", canonical.display()),
            "magi-unreal-axi --project <path>.uproject",
        ))
    }
}

fn projects_in(directory: &Path) -> Result<Vec<PathBuf>, AppError> {
    let mut projects = Vec::new();
    let entries = fs::read_dir(directory).map_err(|error| {
        AppError::operational(
            "project",
            "directory_unreadable",
            format!("cannot read {}: {error}", directory.display()),
            "check directory permissions",
        )
    })?;
    for entry in entries {
        let path = entry
            .map_err(|error| {
                AppError::operational(
                    "project",
                    "directory_unreadable",
                    error.to_string(),
                    "check directory permissions",
                )
            })?
            .path();
        if path
            .extension()
            .is_some_and(|extension| extension == "uproject")
        {
            projects.push(path.canonicalize().map_err(|error| {
                AppError::operational(
                    "project",
                    "project_not_found",
                    error.to_string(),
                    "magi-unreal-axi project doctor",
                )
            })?);
        }
    }
    projects.sort();
    Ok(projects)
}

pub fn descriptor(path: &Path) -> Result<Value, AppError> {
    let metadata = fs::metadata(path)
        .map_err(|error| descriptor_error(path, "descriptor_unreadable", error))?;
    if metadata.len() > MAX_DESCRIPTOR_BYTES {
        return Err(AppError::operational(
            "project",
            "descriptor_too_large",
            format!("{} exceeds 1 MiB descriptor limit", path.display()),
            "reduce descriptor size",
        ));
    }
    let text = fs::read_to_string(path)
        .map_err(|error| descriptor_error(path, "descriptor_unreadable", error))?;
    serde_json::from_str(&text).map_err(|error| {
        AppError::operational(
            "project",
            "descriptor_malformed",
            format!("malformed {}: {error}", path.display()),
            "fix .uproject JSON",
        )
    })
}

fn descriptor_error(path: &Path, reason: &'static str, error: impl std::fmt::Display) -> AppError {
    AppError::operational(
        "project",
        reason,
        format!("{}: {error}", path.display()),
        "magi-unreal-axi project doctor",
    )
}
