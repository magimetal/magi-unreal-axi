use crate::error::AppError;
use serde_json::{Map, Value, json};
use std::collections::HashSet;

#[allow(clippy::collapsible_if)]
mod generated {
    include!("capability_generated.rs");
}
pub use generated::*;

const CATALOG_TEXT: &str = include_str!("../capabilities/catalog.json");

pub fn records() -> Vec<Value> {
    let mut records: Vec<Value> =
        serde_json::from_str(CATALOG_TEXT).expect("generated catalog is valid");
    debug_assert_eq!(records.len(), CATALOG_COUNT);
    records.sort_by(|left, right| left["id"].as_str().cmp(&right["id"].as_str()));
    records
}

pub fn find(id: &str) -> Option<Value> {
    records().into_iter().find(|record| record["id"] == id)
}

pub fn search(query: &str, limit: usize) -> Value {
    let terms = query
        .split_whitespace()
        .map(str::to_ascii_lowercase)
        .collect::<Vec<_>>();
    let mut matches = records()
        .into_iter()
        .filter_map(|record| {
            let id = record["id"].as_str()?;
            let domain = record["domain"].as_str()?;
            let summary = record["summary"].as_str()?;
            let id_lower = id.to_ascii_lowercase();
            let domain_lower = domain.to_ascii_lowercase();
            let summary_lower = summary.to_ascii_lowercase();
            let score = terms.iter().try_fold(0_u32, |score, term| {
                let term_score = if id_lower == *term {
                    100
                } else if id_lower.contains(term) {
                    20
                } else if domain_lower.contains(term) {
                    10
                } else if summary_lower.contains(term) {
                    5
                } else {
                    return None;
                };
                Some(score + term_score)
            })?;
            Some((
                score,
                id.to_owned(),
                json!({"id":id,"domain":domain,"summary":summary,"available":true}),
            ))
        })
        .collect::<Vec<_>>();
    matches.sort_by(|left, right| right.0.cmp(&left.0).then(left.1.cmp(&right.1)));
    let total = matches.len();
    let items = matches
        .into_iter()
        .take(limit)
        .map(|(_, _, item)| item)
        .collect::<Vec<_>>();
    json!({
        "count": items.len(),
        "total": total,
        "scope": "generated capability catalog",
        "items": items,
        "nextCursor": null,
    })
}

pub fn describe(id: &str) -> Result<Value, AppError> {
    let record = find(id).ok_or_else(|| {
        AppError::usage(
            "unknown_capability",
            format!("unknown capability `{id}"),
            "magi-unreal-axi capability search <query>",
        )
    })?;
    let mut capability = record;
    let object = capability
        .as_object_mut()
        .expect("catalog capability record is an object");
    for field in [
        "inputSchema",
        "outputSchema",
        "verification",
        "engineSupport",
    ] {
        let value = object
            .get(field)
            .cloned()
            .expect("catalog capability field exists");
        object.insert(
            field.to_owned(),
            Value::String(serde_json::to_string(&value).expect("schema serializes")),
        );
    }
    Ok(
        json!({"capability": capability, "runtime": {"available": true, "catalogHash": CATALOG_HASH}}),
    )
}

pub fn validate_input(id: &str, args: Value) -> Result<Value, AppError> {
    if find(id).is_none() {
        return Err(AppError::usage(
            "unknown_capability",
            format!("unknown capability `{id}`"),
            "magi-unreal-axi capability search <query>",
        ));
    }
    if !id.starts_with("asset.create_input_")
        && !matches!(
            id,
            "asset.save"
                | "blueprint.compile"
                | "blueprint.view"
                | "component.add"
                | "component.list"
                | "component.remove"
                | "component.update"
                | "component.view"
                | "level.set_game_mode"
                | "level.settings"
                | "play.input"
                | "play.observe"
                | "play.screenshot"
                | "play.start"
                | "play.status"
                | "play.stop"
        )
    {
        validate_generated_input(id, &args).map_err(|message| input_error(id, message))?;
    }
    let object = args
        .as_object()
        .ok_or_else(|| input_error(id, "input must be a JSON object"))?;
    match id {
        "editor.status" | "level.current" => require_keys(id, object, &[], &[])?,
        "level.list" | "actor.list" | "asset.list" => validate_list_input(id, object)?,
        "actor.view" | "asset.view" | "capability.describe" | "operation.view" => {
            require_keys(id, object, &["id"], &["id"])?;
            bounded_string(id, object, "id", 512)?;
        }
        "capability.search" => {
            require_keys(id, object, &["query"], &["query", "limit"])?;
            bounded_string(id, object, "query", 256)?;
            if let Some(limit) = object.get("limit") {
                bounded_integer(id, "limit", limit, 1, 50)?;
            }
        }
        "actor.delete" => {
            require_keys(id, object, &["id"], &["id", "force", "dryRun"])?;
            bounded_string(id, object, "id", 512)?;
        }
        "actor.spawn" => {
            require_keys(
                id,
                object,
                &["levelId", "class", "agentKey"],
                &[
                    "levelId", "class", "agentKey", "label", "location", "rotation", "scale",
                ],
            )?;
            bounded_string(id, object, "levelId", 512)?;
            bounded_string(id, object, "class", 512)?;
            bounded_string(id, object, "agentKey", 256)?;
        }
        "actor.update_transform" => {
            require_keys(
                id,
                object,
                &["id"],
                &["id", "location", "rotation", "scale"],
            )?;
            bounded_string(id, object, "id", 512)?;
            if !["location", "rotation", "scale"]
                .iter()
                .any(|field| object.contains_key(*field))
            {
                return Err(input_error(id, "at least one transform group is required"));
            }
        }
        "level.create" | "level.open" | "level.save" => {
            require_keys(id, object, &["path"], &["path"])?;
            bounded_string(id, object, "path", 512)?;
        }
        "asset.create_input_action" => {
            require_keys(id, object, &["path", "valueType"], &["path", "valueType"])?;
            bounded_string(id, object, "path", 512)?;
            if !matches!(object["valueType"].as_str(), Some("Boolean" | "Axis1D")) {
                return Err(input_error(id, "valueType must be Boolean or Axis1D"));
            }
        }
        "asset.create_input_mapping_context" => {
            require_keys(id, object, &["path", "mappings"], &["path", "mappings"])?;
            bounded_string(id, object, "path", 512)?;
            let mappings = object["mappings"]
                .as_array()
                .ok_or_else(|| input_error(id, "mappings must be an array"))?;
            if mappings.len() > 100 {
                return Err(input_error(id, "mappings exceeds 100 items"));
            }
            for mapping in mappings {
                let mapping = mapping
                    .as_object()
                    .ok_or_else(|| input_error(id, "mapping must be object"))?;
                require_keys(id, mapping, &["actionId", "key"], &["actionId", "key"])?;
                bounded_string(id, mapping, "actionId", 512)?;
                bounded_string(id, mapping, "key", 128)?;
            }
        }
        "asset.save" | "blueprint.compile" | "component.remove" | "component.view" => {
            require_keys(id, object, &["id"], &["id", "force", "dryRun"])?;
            bounded_string(id, object, "id", 512)?;
        }
        "blueprint.view" => {
            require_keys(id, object, &["id"], &["id"])?;
            bounded_string(id, object, "id", 512)?;
        }
        "component.add" => {
            require_keys(
                id,
                object,
                &["actorId", "class", "name"],
                &["actorId", "class", "name", "location"],
            )?;
            for field in ["actorId", "class", "name"] {
                bounded_string(id, object, field, 512)?;
            }
        }
        "component.list" => {
            require_keys(
                id,
                object,
                &["actorId"],
                &["actorId", "limit", "cursor", "fields"],
            )?;
            bounded_string(id, object, "actorId", 512)?;
            validate_list_input(id, object)?;
        }
        "component.update" => {
            require_keys(id, object, &["id", "location"], &["id", "location"])?;
            bounded_string(id, object, "id", 512)?;
        }
        "level.set_game_mode" => {
            require_keys(
                id,
                object,
                &["levelId", "gameModeClass"],
                &["levelId", "gameModeClass"],
            )?;
            bounded_string(id, object, "levelId", 512)?;
            bounded_string(id, object, "gameModeClass", 512)?;
        }
        "level.settings" => {
            require_keys(id, object, &[], &["levelId"])?;
            if object.contains_key("levelId") {
                bounded_string(id, object, "levelId", 512)?;
            }
        }
        "play.start" => require_keys(id, object, &[], &[])?,
        "play.status" => {
            require_keys(id, object, &[], &["sessionId"])?;
            if object.contains_key("sessionId") {
                bounded_string(id, object, "sessionId", 128)?;
            }
        }
        "play.input" => {
            require_keys(
                id,
                object,
                &["sessionId", "key", "event"],
                &["sessionId", "key", "event"],
            )?;
            bounded_string(id, object, "sessionId", 128)?;
            bounded_string(id, object, "key", 128)?;
            if !matches!(object["event"].as_str(), Some("pressed" | "released")) {
                return Err(input_error(id, "event must be pressed or released"));
            }
        }
        "play.observe" | "play.stop" => {
            require_keys(id, object, &["sessionId"], &["sessionId"])?;
            bounded_string(id, object, "sessionId", 128)?;
        }
        "play.screenshot" => {
            require_keys(id, object, &["sessionId"], &["sessionId", "path"])?;
            bounded_string(id, object, "sessionId", 128)?;
            if object.contains_key("path") {
                bounded_string(id, object, "path", 512)?;
            }
        }
        _ => return Err(input_error(id, "capability is not executable")),
    }
    Ok(Value::Object(object.clone()))
}

pub fn execute_local(id: &str, args: &Value) -> Option<Result<Value, AppError>> {
    let object = args.as_object().expect("validated capability input");
    match id {
        "capability.search" => Some(Ok(search(
            object["query"].as_str().expect("validated query"),
            object.get("limit").and_then(Value::as_u64).unwrap_or(50) as usize,
        ))),
        "capability.describe" => Some(describe(object["id"].as_str().expect("validated id"))),
        _ => None,
    }
}

pub fn validate_output(id: &str, result: Value) -> Result<Value, AppError> {
    validate_generated_output(id, &result).map_err(|message| output_error(id, message))?;
    let object = result
        .as_object()
        .ok_or_else(|| output_error(id, "result must be an object"))?;
    match id {
        "editor.status" => {
            require_output_string(id, object, "state")?;
            require_output_string(id, object, "projectId")?;
            require_output_u64(id, object, "editorPid")?;
            require_output_string(id, object, "levelId")?;
            require_output_string(id, object, "pie")?;
            require_output_u64(id, object, "dirtyPackageCount")?;
        }
        "level.current" => {
            let level = object
                .get("level")
                .and_then(Value::as_object)
                .ok_or_else(|| output_error(id, "result.level must be an object"))?;
            for field in ["id", "name", "worldType", "revision"] {
                require_output_string(id, level, field)?;
            }
            if !level.get("persistent").is_some_and(Value::is_boolean) {
                return Err(output_error(id, "result.level.persistent must be boolean"));
            }
            require_output_string(id, object, "scope")?;
        }
        "level.list" | "actor.list" | "asset.list" | "capability.search" => {
            validate_list_output(id, object)?
        }
        "actor.view" => {
            for field in [
                "id",
                "actorGuid",
                "levelId",
                "label",
                "class",
                "objectPath",
                "revision",
            ] {
                require_output_string(id, object, field)?;
            }
            for field in ["location", "rotation", "scale"] {
                let value = object
                    .get(field)
                    .and_then(Value::as_array)
                    .ok_or_else(|| output_error(id, format!("result.{field} must be an array")))?;
                if value.len() != 3 || !value.iter().all(Value::is_number) {
                    return Err(output_error(
                        id,
                        format!("result.{field} must contain three numbers"),
                    ));
                }
            }
        }
        "asset.view" => {
            for field in [
                "id",
                "packagePath",
                "objectPath",
                "name",
                "class",
                "revision",
            ] {
                require_output_string(id, object, field)?;
            }
        }
        "capability.describe"
        | "operation.view"
        | "level.create"
        | "level.open"
        | "level.save"
        | "actor.spawn"
        | "actor.update_transform"
        | "actor.delete" => {}
        "asset.create_input_action" => {
            for field in ["id", "class", "valueType", "revision"] {
                require_output_string(id, object, field)?;
            }
            for field in ["changed"] {
                if !object.get(field).is_some_and(Value::is_boolean) {
                    return Err(output_error(id, format!("result.{field} must be boolean")));
                }
            }
            for field in ["dirtyPackages", "savedPackages"] {
                if !object.get(field).is_some_and(Value::is_array) {
                    return Err(output_error(id, format!("result.{field} must be array")));
                }
            }
        }
        "asset.create_input_mapping_context" => {
            for field in ["id", "class", "revision"] {
                require_output_string(id, object, field)?;
            }
            require_output_u64(id, object, "mappingCount")?;
            if !object.get("changed").is_some_and(Value::is_boolean) {
                return Err(output_error(id, "result.changed must be boolean"));
            }
        }
        "asset.save" => {
            for field in ["id", "revision"] {
                require_output_string(id, object, field)?;
            }
            if !object.get("changed").is_some_and(Value::is_boolean) {
                return Err(output_error(id, "result.changed must be boolean"));
            }
        }
        "component.view" => {
            for field in ["id", "actorId", "name", "class", "revision"] {
                require_output_string(id, object, field)?;
            }
            if !object.get("scene").is_some_and(Value::is_boolean) {
                return Err(output_error(id, "result.scene must be boolean"));
            }
        }
        "component.list" => validate_list_output(id, object)?,
        "component.add" | "component.update" | "component.remove" => {
            require_output_string(id, object, "id")?;
            require_output_string(id, object, "revision")?;
        }
        "level.set_game_mode" => {
            require_output_string(id, object, "levelId")?;
            require_output_string(id, object, "revision")?;
        }
        "level.settings" => {
            for field in ["levelId", "gameModeClass", "defaultPawnClass", "revision"] {
                require_output_string(id, object, field)?;
            }
        }
        "blueprint.view" | "blueprint.compile" | "play.start" | "play.status" | "play.input"
        | "play.observe" | "play.screenshot" | "play.stop" => {
            require_output_string(id, object, "revision")?;
        }
        _ => return Err(output_error(id, "unknown result contract")),
    }
    Ok(result)
}

fn validate_list_input(id: &str, object: &Map<String, Value>) -> Result<(), AppError> {
    if id == "component.list" {
        require_keys(
            id,
            object,
            &["actorId"],
            &["actorId", "limit", "cursor", "fields"],
        )?;
    } else {
        require_keys(id, object, &[], &["limit", "cursor", "fields"])?;
    }
    if let Some(limit) = object.get("limit") {
        bounded_integer(id, "limit", limit, 1, 100)?;
    }
    if object.get("cursor").is_some() {
        bounded_string(id, object, "cursor", 256)?;
    }
    if let Some(fields) = object.get("fields") {
        let fields = fields
            .as_array()
            .ok_or_else(|| input_error(id, "fields must be an array"))?;
        if fields.is_empty() || fields.len() > 4 {
            return Err(input_error(id, "fields must contain 1 to 4 values"));
        }
        let mut seen = HashSet::new();
        for field in fields {
            let field = field
                .as_str()
                .ok_or_else(|| input_error(id, "field names must be strings"))?;
            let allowed: &[&str] = match id {
                "actor.list" => &["id", "label", "class", "levelId"],
                "asset.list" => &["id", "name", "class", "packagePath"],
                "level.list" => &["id", "name", "worldType", "persistent"],
                "component.list" => &["id", "name", "class", "scene"],
                _ => &[],
            };
            if !allowed.contains(&field) || !seen.insert(field) {
                return Err(input_error(
                    id,
                    format!("invalid or duplicate field `{field}`"),
                ));
            }
        }
    }
    Ok(())
}
fn validate_list_output(id: &str, object: &Map<String, Value>) -> Result<(), AppError> {
    let count = require_output_u64(id, object, "count")?;
    let total = require_output_u64(id, object, "total")?;
    let items = object
        .get("items")
        .and_then(Value::as_array)
        .ok_or_else(|| output_error(id, "result.items must be an array"))?;
    require_output_string(id, object, "scope")?;
    if count as usize != items.len() || count > total || count > 100 {
        return Err(output_error(id, "list count and total are inconsistent"));
    }
    if !object.get("nextCursor").is_some_and(|value| {
        value.is_null() || value.as_str().is_some_and(|text| text.len() <= 256)
    }) {
        return Err(output_error(
            id,
            "nextCursor must be null or a bounded string",
        ));
    }
    if !items
        .iter()
        .all(|item| item.as_object().is_some_and(|item| item.contains_key("id")))
    {
        return Err(output_error(id, "every projected list item requires id"));
    }
    if id != "capability.search" {
        let revision = require_output_string(id, object, "revision")?;
        if revision.len() != 64 || !revision.bytes().all(|byte| byte.is_ascii_hexdigit()) {
            return Err(output_error(
                id,
                "list revision must be a SHA-256 hex value",
            ));
        }
    }
    let mut previous: Option<&str> = None;
    for item in items {
        let item = item
            .as_object()
            .ok_or_else(|| output_error(id, "list items must be objects"))?;
        if let Some(item_id) = item.get("id").and_then(Value::as_str) {
            if previous.is_some_and(|prior| prior >= item_id) {
                return Err(output_error(id, "list item IDs must be unique and ordered"));
            }
            previous = Some(item_id);
        }
    }
    Ok(())
}

fn require_keys(
    id: &str,
    object: &Map<String, Value>,
    required: &[&str],
    allowed: &[&str],
) -> Result<(), AppError> {
    if let Some(key) = object.keys().find(|key| !allowed.contains(&key.as_str())) {
        return Err(input_error(id, format!("unknown input field `{key}`")));
    }
    if let Some(key) = required.iter().find(|key| !object.contains_key(**key)) {
        return Err(input_error(
            id,
            format!("missing required input field `{key}`"),
        ));
    }
    Ok(())
}

fn bounded_string(
    id: &str,
    object: &Map<String, Value>,
    field: &str,
    maximum: usize,
) -> Result<(), AppError> {
    let value = object
        .get(field)
        .and_then(Value::as_str)
        .ok_or_else(|| input_error(id, format!("{field} must be a string")))?;
    if value.is_empty() || value.chars().count() > maximum {
        return Err(input_error(id, format!("{field} exceeds allowed length")));
    }
    Ok(())
}

fn bounded_integer(
    id: &str,
    field: &str,
    value: &Value,
    minimum: u64,
    maximum: u64,
) -> Result<(), AppError> {
    if !value
        .as_u64()
        .is_some_and(|number| (minimum..=maximum).contains(&number))
    {
        return Err(input_error(
            id,
            format!("{field} must be between {minimum} and {maximum}"),
        ));
    }
    Ok(())
}

fn require_output_string<'a>(
    id: &str,
    object: &'a Map<String, Value>,
    field: &str,
) -> Result<&'a str, AppError> {
    object
        .get(field)
        .and_then(Value::as_str)
        .ok_or_else(|| output_error(id, format!("result.{field} must be a string")))
}

fn require_output_u64(id: &str, object: &Map<String, Value>, field: &str) -> Result<u64, AppError> {
    object
        .get(field)
        .and_then(Value::as_u64)
        .ok_or_else(|| output_error(id, format!("result.{field} must be a non-negative integer")))
}

fn input_error(id: &str, message: impl Into<String>) -> AppError {
    AppError::usage(
        "invalid_capability_input",
        format!("{id}: {}", message.into()),
        format!("magi-unreal-axi capability describe {id}"),
    )
}

fn output_error(id: &str, message: impl Into<String>) -> AppError {
    AppError::operational(
        "capability",
        "invalid_editor_output",
        format!("{id}: {}", message.into()),
        "magi-unreal-axi editor status",
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn catalog_is_sorted_unique_and_complete() {
        let ids = records()
            .into_iter()
            .map(|record| record["id"].as_str().unwrap().to_owned())
            .collect::<Vec<_>>();
        let mut sorted = ids.clone();
        sorted.sort();
        sorted.dedup();
        assert_eq!(ids, sorted);
        assert_eq!(ids.len(), CATALOG_COUNT);
    }

    #[test]
    fn search_is_compact_and_deterministic() {
        let result = search("actor", 50);
        assert_eq!(result["count"], 5);
        assert_eq!(result["items"][0]["id"], "actor.delete");
        assert!(result["items"][0].get("inputSchema").is_none());
    }

    #[test]
    fn generated_invalid_output_fixtures_are_rejected() {
        let fixtures: Vec<Value> = serde_json::from_str(include_str!(
            "../capabilities/output-invalid.generated.json"
        ))
        .unwrap();
        assert!(fixtures.len() > CATALOG_COUNT);
        for fixture in fixtures {
            let id = fixture["operation"].as_str().unwrap();
            assert!(
                validate_output(id, fixture["result"].clone()).is_err(),
                "{id} accepted generated invalid output"
            );
        }
    }

    #[test]
    fn generated_list_contract_rejects_missing_identity_and_unknown_fields() {
        let base = json!({
            "count":1,"total":1,"scope":"/Game","revision":"0".repeat(64),
            "items":[{"label":"Actor"}],"nextCursor":null
        });
        assert!(validate_output("actor.list", base).is_err());
        let extra = json!({
            "count":1,"total":1,"scope":"/Game","revision":"0".repeat(64),
            "items":[{"id":"/Game/L#guid","unknown":true}],"nextCursor":null
        });
        assert!(validate_output("actor.list", extra).is_err());
    }

    #[test]
    fn invalid_fields_fail_before_transport() {
        let error = validate_input("actor.list", json!({"fields":["label","label"]})).unwrap_err();
        assert_eq!(error.reason, "invalid_capability_input");
    }

    #[test]
    fn component_list_accepts_required_actor_scope() {
        let input = json!({"actorId":"/Game/Map#guid","limit":10,"fields":["id","name"]});
        assert_eq!(
            validate_input("component.list", input.clone()).unwrap(),
            input
        );
    }
}
