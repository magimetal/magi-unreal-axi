use crate::error::AppError;
use serde_json::{Map, Value, json};
use sha2::{Digest, Sha256};
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
            let (availability, reasons) = if record["execution"] == "local" {
                ("available", json!([]))
            } else {
                (
                    "unknown",
                    json!([{"code":"editor_offline","subject":"","message":"No authenticated matching editor is available"}]),
                )
            };
            Some((
                score,
                id.to_owned(),
                json!({"id":id,"domain":domain,"summary":summary,"availability":availability,"reasons":reasons}),
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
    let (availability, reasons) = if capability["execution"] == "local" {
        ("available", json!([]))
    } else {
        (
            "unknown",
            json!([{"code":"editor_offline","subject":"","message":"No authenticated matching editor is available"}]),
        )
    };
    Ok(
        json!({"capability": capability, "runtime": {"availability":availability,"reasons":reasons,"catalogHash": CATALOG_HASH}}),
    )
}

pub fn apply_runtime_availability(output: &mut Value, live: &Value) {
    if let Some(items) = output.get_mut("items").and_then(Value::as_array_mut) {
        for item in items {
            let Some(id) = item.get("id").and_then(Value::as_str) else {
                continue;
            };
            if capability_metadata(id).is_some_and(|metadata| metadata.execution == "native")
                && let Some(runtime) = live.get(id)
            {
                item["availability"] = runtime["availability"].clone();
                item["reasons"] = runtime["reasons"].clone();
            }
        }
        return;
    }
    let Some(id) = output["capability"]["id"].as_str() else {
        return;
    };
    if capability_metadata(id).is_some_and(|metadata| metadata.execution == "native")
        && let Some(runtime) = live.get(id)
    {
        output["runtime"]["availability"] = runtime["availability"].clone();
        output["runtime"]["reasons"] = runtime["reasons"].clone();
    }
}

pub fn validate_input(id: &str, args: Value) -> Result<Value, AppError> {
    if find(id).is_none() {
        return Err(AppError::usage(
            "unknown_capability",
            format!("unknown capability `{id}`"),
            "magi-unreal-axi capability search <query>",
        ));
    }
    validate_generated_input(id, &args).map_err(|message| input_error(id, message))?;
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
            bounded_string(id, object, "id", 1024)?;
        }
        "blueprint.graph_view" => {
            require_keys(
                id,
                object,
                &["blueprintId"],
                &["blueprintId", "graphId", "limit", "cursor"],
            )?;
            bounded_string(id, object, "blueprintId", 512)?;
            if object.contains_key("graphId") {
                bounded_string(id, object, "graphId", 1024)?;
            }
            if object.contains_key("cursor") {
                bounded_string(id, object, "cursor", 256)?;
                let cursor = object["cursor"].as_str().unwrap();
                validate_graph_cursor_input(id, cursor)?;
            }
        }
        "blueprint.create" => {
            require_keys(
                id,
                object,
                &["path", "parentClass"],
                &["path", "parentClass"],
            )?;
            bounded_string(id, object, "path", 512)?;
            bounded_string(id, object, "parentClass", 64)?;
        }
        "blueprint.event_ensure" => {
            require_keys(
                id,
                object,
                &["blueprintId", "graphId", "agentKey", "event"],
                &[
                    "blueprintId",
                    "graphId",
                    "agentKey",
                    "event",
                    "variableGuid",
                    "interfaceId",
                ],
            )?;
            bounded_string(id, object, "blueprintId", 512)?;
            bounded_string(id, object, "graphId", 1024)?;
            bounded_string(id, object, "agentKey", 128)?;
            bounded_string(id, object, "event", 32)?;
            match object["event"].as_str() {
                Some("component.begin_overlap") => {
                    if !object
                        .get("variableGuid")
                        .and_then(Value::as_str)
                        .is_some_and(canonical_guid)
                        || object.contains_key("interfaceId")
                    {
                        return Err(input_error(
                            id,
                            "component.begin_overlap requires one canonical variableGuid",
                        ));
                    }
                }
                Some("interface.interact") => {
                    if !object.contains_key("interfaceId") || object.contains_key("variableGuid") {
                        return Err(input_error(
                            id,
                            "interface.interact requires one interfaceId",
                        ));
                    }
                }
                Some("actor.begin_play" | "input.key_e") => {
                    if object.contains_key("variableGuid") || object.contains_key("interfaceId") {
                        return Err(input_error(
                            id,
                            "event identity fields do not apply to this intent",
                        ));
                    }
                }
                _ => return Err(input_error(id, "event intent is not allowlisted")),
            }
        }
        "blueprint.node_ensure" => {
            require_keys(
                id,
                object,
                &["blueprintId", "graphId", "agentKey", "node"],
                &["blueprintId", "graphId", "agentKey", "node", "interfaceId"],
            )?;
            bounded_string(id, object, "blueprintId", 512)?;
            bounded_string(id, object, "graphId", 1024)?;
            bounded_string(id, object, "agentKey", 128)?;
            bounded_string(id, object, "node", 64)?;
            if (object["node"] == "interface.message_interact")
                != object.contains_key("interfaceId")
            {
                return Err(input_error(
                    id,
                    "interface.message_interact requires one interfaceId",
                ));
            }
        }
        "blueprint.pin_default_set" => {
            require_keys(
                id,
                object,
                &["blueprintId", "pinId", "value"],
                &["blueprintId", "pinId", "value"],
            )?;
            bounded_string(id, object, "blueprintId", 512)?;
            bounded_string(id, object, "pinId", 2048)?;
        }
        "blueprint.pin_connect" => {
            require_keys(
                id,
                object,
                &["blueprintId", "sourcePinId", "targetPinId"],
                &["blueprintId", "sourcePinId", "targetPinId"],
            )?;
            for field in ["blueprintId", "sourcePinId", "targetPinId"] {
                bounded_string(id, object, field, 2048)?;
            }
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
            if let Some(path) = object.get("path").and_then(Value::as_str) {
                bounded_string(id, object, "path", 512)?;
                if path.contains(['/', '\\'])
                    || !path.ends_with(".png")
                    || path.chars().any(char::is_control)
                {
                    return Err(input_error(
                        id,
                        "screenshot path must be a control-free PNG filename",
                    ));
                }
            }
        }
        "blueprint.interface_create" => {
            require_keys(id, object, &["path", "function"], &["path", "function"])?;
            bounded_string(id, object, "path", 512)?;
        }
        "blueprint.interface_view" => {
            require_keys(id, object, &["id"], &["id"])?;
            bounded_string(id, object, "id", 512)?;
        }
        "blueprint.interface_ensure" => {
            require_keys(
                id,
                object,
                &["blueprintId", "interfaceId"],
                &["blueprintId", "interfaceId"],
            )?;
            bounded_string(id, object, "blueprintId", 512)?;
            bounded_string(id, object, "interfaceId", 512)?;
        }
        "blueprint.scs_view" => {
            require_keys(id, object, &["blueprintId"], &["blueprintId"])?;
            bounded_string(id, object, "blueprintId", 512)?;
        }
        "blueprint.scs_component_ensure" => {
            require_keys(
                id,
                object,
                &["blueprintId", "name", "class"],
                &["blueprintId", "name", "class", "parent"],
            )?;
            bounded_string(id, object, "blueprintId", 512)?;
            bounded_string(id, object, "name", 128)?;
            if object
                .get("parent")
                .and_then(Value::as_str)
                .is_some_and(|value| !canonical_guid(value))
            {
                return Err(input_error(
                    id,
                    "parent must be a canonical lowercase VariableGuid",
                ));
            }
        }
        "blueprint.scs_component_update" => {
            if !object
                .get("variableGuid")
                .and_then(Value::as_str)
                .is_some_and(canonical_guid)
            {
                return Err(input_error(
                    id,
                    "variableGuid must be canonical lowercase GUID",
                ));
            }
            if ![
                "location",
                "rotation",
                "scale",
                "collisionEnabled",
                "collisionProfile",
                "generateOverlapEvents",
                "simulatePhysics",
                "gravityEnabled",
                "massOverride",
                "boxExtent",
                "sphereRadius",
            ]
            .iter()
            .any(|field| object.contains_key(*field))
            {
                return Err(input_error(
                    id,
                    "at least one SCS component field is required",
                ));
            }
        }
        "blueprint.scs_component_remove" => {
            if !object
                .get("variableGuid")
                .and_then(Value::as_str)
                .is_some_and(canonical_guid)
            {
                return Err(input_error(
                    id,
                    "variableGuid must be canonical lowercase GUID",
                ));
            }
        }
        "play.component_observe" => {
            if !object
                .get("variableGuid")
                .and_then(Value::as_str)
                .is_some_and(canonical_guid)
            {
                return Err(input_error(
                    id,
                    "variableGuid must be canonical lowercase GUID",
                ));
            }
        }
        "widget.create" => {
            require_keys(
                id,
                object,
                &["path", "rootName", "rootClass"],
                &["path", "rootName", "rootClass"],
            )?;
            let path = object["path"]
                .as_str()
                .ok_or_else(|| input_error(id, "path must be a string"))?;
            if !valid_package_path(path) {
                return Err(input_error(
                    id,
                    "path must be a canonical /Game package path",
                ));
            }
            if !object["rootName"].as_str().is_some_and(valid_widget_name) {
                return Err(input_error(id, "rootName is invalid"));
            }
            if object["rootClass"] != "VerticalBox" {
                return Err(input_error(id, "rootClass must be VerticalBox"));
            }
        }
        "widget.tree_view" => {
            require_keys(id, object, &["blueprintId"], &["blueprintId"])?;
            if !object["blueprintId"].as_str().is_some_and(valid_object_id) {
                return Err(input_error(id, "blueprintId is invalid"));
            }
        }
        "widget.child_ensure" => {
            require_keys(
                id,
                object,
                &["blueprintId", "parentWidgetId", "name", "class"],
                &["blueprintId", "parentWidgetId", "name", "class"],
            )?;
            let blueprint = object["blueprintId"]
                .as_str()
                .ok_or_else(|| input_error(id, "blueprintId must be a string"))?;
            let parent = object["parentWidgetId"]
                .as_str()
                .ok_or_else(|| input_error(id, "parentWidgetId must be a string"))?;
            if !valid_object_id(blueprint) || !valid_widget_id(blueprint, parent) {
                return Err(input_error(id, "widget identity is invalid"));
            }
            if !object["name"].as_str().is_some_and(valid_widget_name)
                || object["class"] != "TextBlock"
            {
                return Err(input_error(id, "name or class is invalid"));
            }
        }
        "widget.property_set" => {
            require_keys(
                id,
                object,
                &["blueprintId", "widgetId", "property"],
                &[
                    "blueprintId",
                    "widgetId",
                    "property",
                    "text",
                    "visibility",
                    "enabled",
                ],
            )?;
            let blueprint = object["blueprintId"]
                .as_str()
                .ok_or_else(|| input_error(id, "blueprintId must be a string"))?;
            let widget = object["widgetId"]
                .as_str()
                .ok_or_else(|| input_error(id, "widgetId must be a string"))?;
            let property = object["property"]
                .as_str()
                .ok_or_else(|| input_error(id, "property must be a string"))?;
            if !valid_object_id(blueprint) || !valid_widget_id(blueprint, widget) {
                return Err(input_error(id, "widget identity is invalid"));
            }
            let fields = ["text", "visibility", "enabled"];
            if fields
                .iter()
                .filter(|field| object.contains_key(**field))
                .count()
                != 1
                || !object.contains_key(property)
                || !fields.contains(&property)
            {
                return Err(input_error(
                    id,
                    "exactly matching property payload is required",
                ));
            }
            match property {
                "text" => {
                    if object["text"]
                        .as_str()
                        .is_none_or(|value| value.chars().count() > 256)
                    {
                        return Err(input_error(id, "text exceeds 256 characters"));
                    }
                }
                "visibility" => {
                    if !matches!(
                        object["visibility"].as_str(),
                        Some("Visible" | "Hidden" | "Collapsed")
                    ) {
                        return Err(input_error(id, "visibility is invalid"));
                    }
                }
                "enabled" => {
                    if !object["enabled"].is_boolean() {
                        return Err(input_error(id, "enabled must be boolean"));
                    }
                }
                _ => unreachable!(),
            }
        }
        "widget.event_ensure" => {
            require_keys(
                id,
                object,
                &["blueprintId", "agentKey", "event", "actions"],
                &["blueprintId", "agentKey", "event", "actions"],
            )?;
            let blueprint = object["blueprintId"]
                .as_str()
                .ok_or_else(|| input_error(id, "blueprintId must be a string"))?;
            if !valid_object_id(blueprint)
                || !object["agentKey"].as_str().is_some_and(valid_agent_key)
                || object["event"] != "activate"
            {
                return Err(input_error(id, "event identity is invalid"));
            }
            let actions = object["actions"]
                .as_array()
                .ok_or_else(|| input_error(id, "actions must be an array"))?;
            if !(1..=3).contains(&actions.len()) {
                return Err(input_error(id, "actions must contain 1 to 3 items"));
            }
            let mut previous: Option<(u8, &str)> = None;
            let mut seen = HashSet::new();
            for action in actions {
                let action = action
                    .as_object()
                    .ok_or_else(|| input_error(id, "action must be an object"))?;
                let kind = action["kind"]
                    .as_str()
                    .ok_or_else(|| input_error(id, "action kind is required"))?;
                let target = action["targetWidgetId"]
                    .as_str()
                    .ok_or_else(|| input_error(id, "targetWidgetId is required"))?;
                let rank = match kind {
                    "text.set" => 0,
                    "enabled.set" => 1,
                    "visibility.set" => 2,
                    _ => return Err(input_error(id, "action kind is invalid")),
                };
                if !valid_widget_id(blueprint, target) || !seen.insert((kind, target)) {
                    return Err(input_error(id, "action target is invalid or duplicated"));
                }
                let payload = match kind {
                    "text.set" => {
                        action.get("text").is_some_and(|value| {
                            value
                                .as_str()
                                .is_some_and(|text| text.chars().count() <= 256)
                        }) && !action.contains_key("enabled")
                            && !action.contains_key("visibility")
                    }
                    "enabled.set" => {
                        action.get("enabled").is_some_and(Value::is_boolean)
                            && !action.contains_key("text")
                            && !action.contains_key("visibility")
                    }
                    "visibility.set" => {
                        action
                            .get("visibility")
                            .and_then(Value::as_str)
                            .is_some_and(|value| {
                                matches!(value, "Visible" | "Hidden" | "Collapsed")
                            })
                            && !action.contains_key("text")
                            && !action.contains_key("enabled")
                    }
                    _ => false,
                };
                if !payload || previous.is_some_and(|prior| (rank, target) <= prior) {
                    return Err(input_error(
                        id,
                        "action payload or canonical ordering is invalid",
                    ));
                }
                previous = Some((rank, target));
            }
        }
        "widget.viewport_ensure" => {
            require_keys(
                id,
                object,
                &[
                    "hostBlueprintId",
                    "widgetBlueprintId",
                    "agentKey",
                    "inputKey",
                    "zOrder",
                ],
                &[
                    "hostBlueprintId",
                    "widgetBlueprintId",
                    "agentKey",
                    "inputKey",
                    "zOrder",
                ],
            )?;
            if !object["hostBlueprintId"]
                .as_str()
                .is_some_and(valid_object_id)
                || !object["widgetBlueprintId"]
                    .as_str()
                    .is_some_and(valid_object_id)
                || !object["agentKey"].as_str().is_some_and(valid_agent_key)
                || object["inputKey"] != "E"
                || object["zOrder"] != 0
            {
                return Err(input_error(id, "viewport input is invalid"));
            }
        }
        "play.ui_observe" => {
            require_keys(
                id,
                object,
                &["sessionId", "widgetBlueprintId", "widgetIds"],
                &["sessionId", "widgetBlueprintId", "widgetIds"],
            )?;
            let blueprint = object["widgetBlueprintId"]
                .as_str()
                .ok_or_else(|| input_error(id, "widgetBlueprintId must be a string"))?;
            if !valid_object_id(blueprint) {
                return Err(input_error(id, "widgetBlueprintId is invalid"));
            }
            let ids = object["widgetIds"]
                .as_array()
                .ok_or_else(|| input_error(id, "widgetIds must be an array"))?;
            let mut previous = None;
            for value in ids {
                let widget = value
                    .as_str()
                    .ok_or_else(|| input_error(id, "widgetIds must contain strings"))?;
                if !valid_widget_id(blueprint, widget)
                    || previous.is_some_and(|prior| prior >= widget)
                {
                    return Err(input_error(
                        id,
                        "widgetIds must be exact, unique, and ordered",
                    ));
                }
                previous = Some(widget);
            }
        }
        "navigation.bounds_ensure"
        | "navigation.build"
        | "navigation.status"
        | "navigation.path_query"
        | "blackboard.create"
        | "blackboard.key_ensure"
        | "blackboard.view"
        | "behavior_tree.create"
        | "behavior_tree.node_ensure"
        | "behavior_tree.connect"
        | "behavior_tree.view"
        | "ai.controller_configure"
        | "ai.pawn_configure"
        | "play.ai_target_set"
        | "play.ai_observe"
        | "animation_blueprint.create"
        | "animation.character_configure"
        | "animation.character_view"
        | "animation.graph_view"
        | "animation.state_ensure"
        | "animation.state_machine_ensure"
        | "animation.transition_ensure"
        | "animation.variable_ensure"
        | "play.animation_observe" => {}
        _ => {
            return Err(input_error(
                id,
                "capability input contract is not executable",
            ));
        }
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
    validate_output_for_request(id, &Value::Null, result)
}

pub fn validate_output_for_request(
    id: &str,
    request: &Value,
    result: Value,
) -> Result<Value, AppError> {
    validate_generated_output(id, &result).map_err(|message| output_error(id, message))?;
    validate_canonical_revisions(id, &result)?;
    let object = result
        .as_object()
        .ok_or_else(|| output_error(id, "result must be an object"))?;
    match id {
        "widget.create"
        | "widget.tree_view"
        | "widget.child_ensure"
        | "widget.property_set"
        | "widget.event_ensure"
        | "widget.viewport_ensure"
        | "play.ui_observe" => validate_widget_output(id, request, object)?,
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
        "blueprint.graph_view" => {
            validate_graph_view_output(id, object)?;
            if !request.is_null() {
                validate_graph_view_page(id, request, object)?;
            }
        }
        "blueprint.view"
        | "blueprint.compile"
        | "blueprint.create"
        | "blueprint.event_ensure"
        | "blueprint.node_ensure"
        | "blueprint.pin_default_set"
        | "blueprint.pin_connect"
        | "blueprint.interface_create"
        | "blueprint.interface_view"
        | "blueprint.interface_ensure"
        | "blueprint.scs_view"
        | "blueprint.scs_component_ensure"
        | "blueprint.scs_component_update"
        | "blueprint.scs_component_remove"
        | "play.start"
        | "play.status"
        | "play.input"
        | "play.observe"
        | "play.component_observe"
        | "play.screenshot"
        | "play.stop"
        | "navigation.bounds_ensure"
        | "navigation.build"
        | "navigation.status"
        | "navigation.path_query"
        | "blackboard.create"
        | "blackboard.key_ensure"
        | "blackboard.view"
        | "behavior_tree.create"
        | "behavior_tree.node_ensure"
        | "behavior_tree.connect"
        | "behavior_tree.view"
        | "ai.controller_configure"
        | "ai.pawn_configure"
        | "play.ai_target_set"
        | "play.ai_observe" => {
            require_output_string(id, object, "revision")?;
        }
        "animation_blueprint.create" => validate_animation_create(id, request, object)?,
        "animation.character_configure"
        | "animation.state_ensure"
        | "animation.state_machine_ensure"
        | "animation.transition_ensure"
        | "animation.variable_ensure" => validate_animation_mutation(id, request, object)?,
        "animation.character_view" => validate_animation_character_view(id, request, object)?,
        "animation.graph_view" => validate_animation_graph(id, request, object)?,
        "play.animation_observe" => validate_animation_observation(id, request, object)?,
        _ => return Err(output_error(id, "unknown result contract")),
    }
    Ok(result)
}

fn validate_animation_create(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    validate_animation_mutation_evidence(id, object)?;
    for field in [
        "animationBlueprintId",
        "skeletonId",
        "generatedClass",
        "animGraphId",
        "rootNodeId",
    ] {
        require_nonempty_output(id, object, field)?;
    }
    let blueprint = object["animationBlueprintId"].as_str().unwrap_or_default();
    if object["generatedClass"] != json!(format!("{blueprint}_C")) {
        return Err(output_error(
            id,
            "generated class is not bound to Animation Blueprint",
        ));
    }
    if !request.is_null() {
        let path = request["path"]
            .as_str()
            .ok_or_else(|| output_error(id, "request.path is missing"))?;
        let name = path
            .rsplit_once('/')
            .map(|(_, name)| name)
            .filter(|name| !name.is_empty())
            .ok_or_else(|| output_error(id, "request.path is invalid"))?;
        if blueprint != format!("{path}.{name}")
            || object.get("skeletonId") != request.get("skeletonId")
        {
            return Err(output_error(
                id,
                "create output does not match path or Skeleton request",
            ));
        }
    }
    Ok(())
}

fn validate_animation_mutation(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    validate_animation_mutation_evidence(id, object)?;
    let identity_fields: &[&str] = match id {
        "animation.character_configure" => &[
            "characterBlueprintId",
            "meshComponentId",
            "skeletalMeshId",
            "skeletonId",
            "animationBlueprintId",
            "animClass",
        ],
        "animation.state_machine_ensure" => &[
            "animationBlueprintId",
            "stateMachineId",
            "stateMachineGraphId",
            "entryNodeId",
            "name",
        ],
        "animation.state_ensure" => &[
            "animationBlueprintId",
            "stateMachineId",
            "stateId",
            "stateGraphId",
            "resultNodeId",
            "sequencePlayerNodeId",
            "name",
            "sequenceId",
            "skeletonId",
        ],
        "animation.transition_ensure" => &[
            "animationBlueprintId",
            "stateMachineId",
            "transitionId",
            "transitionGraphId",
            "resultNodeId",
            "variableGetterNodeId",
            "comparisonNodeId",
            "fromStateId",
            "toStateId",
            "expression",
        ],
        "animation.variable_ensure" => &[
            "animationBlueprintId",
            "variableId",
            "bindingId",
            "name",
            "type",
            "source",
            "updateGraphId",
            "eventNodeId",
            "ownerNodeId",
            "velocityNodeId",
            "planarSpeedNodeId",
            "setterNodeId",
        ],
        _ => unreachable!(),
    };
    for field in identity_fields {
        require_nonempty_output(id, object, field)?;
    }
    let request_fields: &[&str] = match id {
        "animation.character_configure" => &[
            "characterBlueprintId",
            "skeletalMeshId",
            "animationBlueprintId",
        ],
        "animation.state_machine_ensure" => &["animationBlueprintId", "name"],
        "animation.state_ensure" => &[
            "animationBlueprintId",
            "stateMachineId",
            "name",
            "sequenceId",
        ],
        "animation.transition_ensure" => &[
            "animationBlueprintId",
            "stateMachineId",
            "fromStateId",
            "toStateId",
            "expression",
        ],
        "animation.variable_ensure" => &["animationBlueprintId", "name", "type", "source"],
        _ => unreachable!(),
    };
    validate_animation_request_fields(id, request, object, request_fields)?;
    if id == "animation.character_configure" {
        let animation_blueprint = object["animationBlueprintId"].as_str().unwrap_or_default();
        if object["animClass"] != json!(format!("{animation_blueprint}_C")) {
            return Err(output_error(
                id,
                "AnimClass is not generated by requested Animation Blueprint",
            ));
        }
    }
    if id == "animation.state_ensure" && object["initial"] != json!(object["name"] == "idle") {
        return Err(output_error(
            id,
            "state initial flag must follow state name",
        ));
    }
    Ok(())
}

fn validate_animation_mutation_evidence(
    id: &str,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    if !object.get("changed").is_some_and(Value::is_boolean) {
        return Err(output_error(id, "result.changed must be boolean"));
    }
    for field in ["dirtyPackages", "savedPackages"] {
        if !object.get(field).is_some_and(Value::is_array) {
            return Err(output_error(id, format!("result.{field} must be array")));
        }
    }
    if !object
        .get("revision")
        .and_then(Value::as_str)
        .is_some_and(canonical_revision)
    {
        return Err(output_error(id, "result.revision is not canonical"));
    }
    Ok(())
}

fn validate_animation_character_view(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    for field in ["characterBlueprintId", "meshComponentId", "animationMode"] {
        require_nonempty_output(id, object, field)?;
    }
    validate_animation_request_fields(id, request, object, &["characterBlueprintId"])?;
    let mesh_bound = object["skeletalMeshId"].is_string();
    if mesh_bound != object["skeletonId"].is_string() {
        return Err(output_error(
            id,
            "SkeletalMesh and Skeleton evidence must be paired",
        ));
    }
    match object["animationBlueprintId"].as_str() {
        Some(animation_blueprint) => {
            if object["animationMode"] != "AnimationBlueprint"
                || object["animClass"] != json!(format!("{animation_blueprint}_C"))
            {
                return Err(output_error(
                    id,
                    "Animation Blueprint mode, asset, and generated class are incoherent",
                ));
            }
        }
        None => {
            if !object["animClass"].is_null() {
                return Err(output_error(
                    id,
                    "AnimClass requires an Animation Blueprint identity",
                ));
            }
        }
    }
    Ok(())
}

fn validate_animation_request_fields(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
    fields: &[&str],
) -> Result<(), AppError> {
    if request.is_null() {
        return Ok(());
    }
    for field in fields {
        if object.get(*field) != request.get(*field) {
            return Err(output_error(
                id,
                format!("result.{field} must match request"),
            ));
        }
    }
    Ok(())
}

fn require_nonempty_output<'a>(
    id: &str,
    object: &'a Map<String, Value>,
    field: &str,
) -> Result<&'a str, AppError> {
    object
        .get(field)
        .and_then(Value::as_str)
        .filter(|value| !value.is_empty())
        .ok_or_else(|| output_error(id, format!("result.{field} must be nonempty string")))
}

fn validate_animation_graph(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    validate_animation_request_fields(id, request, object, &["animationBlueprintId"])?;
    for field in [
        "animationBlueprintId",
        "skeletonId",
        "generatedClass",
        "animGraphId",
        "rootNodeId",
        "revision",
    ] {
        require_nonempty_output(id, object, field)?;
    }
    let blueprint = object["animationBlueprintId"].as_str().unwrap_or_default();
    if object["generatedClass"] != json!(format!("{blueprint}_C")) {
        return Err(output_error(
            id,
            "generated class is not bound to Animation Blueprint",
        ));
    }
    let mut identities = HashSet::new();
    for field in ["animGraphId", "rootNodeId"] {
        insert_animation_identity(
            id,
            object[field].as_str().unwrap_or_default(),
            field,
            &mut identities,
        )?;
    }
    let variables = object["variables"]
        .as_array()
        .ok_or_else(|| output_error(id, "variables must be array"))?;
    for variable in variables {
        validate_animation_row(
            id,
            variable,
            &mut identities,
            &[
                "variableId",
                "bindingId",
                "updateGraphId",
                "eventNodeId",
                "ownerNodeId",
                "velocityNodeId",
                "planarSpeedNodeId",
                "setterNodeId",
            ],
        )?;
    }
    let machines = object["stateMachines"]
        .as_array()
        .ok_or_else(|| output_error(id, "stateMachines must be array"))?;
    if let Some(machine) = machines.first() {
        let machine = machine
            .as_object()
            .ok_or_else(|| output_error(id, "state machine must be object"))?;
        for field in ["stateMachineId", "stateMachineGraphId", "entryNodeId"] {
            validate_identity(id, machine, field, &mut identities)?;
        }
        let states = machine["states"]
            .as_array()
            .ok_or_else(|| output_error(id, "states must be array"))?;
        let mut state_ids = HashSet::new();
        let mut previous_state_rank = None;
        let mut idle_state_id = None;
        for state in states {
            let state = state
                .as_object()
                .ok_or_else(|| output_error(id, "state must be object"))?;
            let rank = match state["name"].as_str() {
                Some("idle") => 0,
                Some("moving") => 1,
                _ => return Err(output_error(id, "state name is not canonical")),
            };
            if previous_state_rank.is_some_and(|previous| previous >= rank)
                || state["skeletonId"] != object["skeletonId"]
            {
                return Err(output_error(
                    id,
                    "states must be a unique ordered subset on root Skeleton",
                ));
            }
            previous_state_rank = Some(rank);
            for field in [
                "stateId",
                "stateGraphId",
                "resultNodeId",
                "sequencePlayerNodeId",
            ] {
                validate_identity(id, state, field, &mut identities)?;
            }
            let state_id = state["stateId"].as_str().unwrap_or_default();
            if !state_ids.insert(state_id) || state["initial"] != json!(rank == 0) {
                return Err(output_error(
                    id,
                    "state identity or initial semantics are invalid",
                ));
            }
            if rank == 0 {
                idle_state_id = Some(state["stateId"].clone());
            }
        }
        if machine["initialStateId"] != idle_state_id.unwrap_or(Value::Null) {
            return Err(output_error(
                id,
                "initialStateId must select idle when idle exists",
            ));
        }
        let transitions = machine["transitions"]
            .as_array()
            .ok_or_else(|| output_error(id, "transitions must be array"))?;
        let mut previous_transition_rank = None;
        for transition in transitions {
            let transition = transition
                .as_object()
                .ok_or_else(|| output_error(id, "transition must be object"))?;
            for field in [
                "transitionId",
                "transitionGraphId",
                "resultNodeId",
                "variableGetterNodeId",
                "comparisonNodeId",
            ] {
                validate_identity(id, transition, field, &mut identities)?;
            }
            let from = states
                .iter()
                .find(|state| state["stateId"] == transition["fromStateId"])
                .and_then(|state| state["name"].as_str())
                .unwrap_or_default();
            let to = states
                .iter()
                .find(|state| state["stateId"] == transition["toStateId"])
                .and_then(|state| state["name"].as_str())
                .unwrap_or_default();
            let rank = match (
                from,
                to,
                transition["expression"].as_str().unwrap_or_default(),
            ) {
                ("idle", "moving", "Speed > 10") => 0,
                ("moving", "idle", "Speed <= 10") => 1,
                _ => {
                    return Err(output_error(
                        id,
                        "transition does not match a canonical directional rule",
                    ));
                }
            };
            if previous_transition_rank.is_some_and(|previous| previous >= rank) {
                return Err(output_error(
                    id,
                    "transitions must be a unique ordered subset",
                ));
            }
            previous_transition_rank = Some(rank);
        }
    }
    Ok(())
}

fn validate_animation_row(
    id: &str,
    value: &Value,
    identities: &mut HashSet<String>,
    fields: &[&str],
) -> Result<(), AppError> {
    let row = value
        .as_object()
        .ok_or_else(|| output_error(id, "animation row must be object"))?;
    for field in fields {
        validate_identity(id, row, field, identities)?;
    }
    Ok(())
}

fn validate_identity(
    id: &str,
    row: &Map<String, Value>,
    field: &str,
    identities: &mut HashSet<String>,
) -> Result<(), AppError> {
    let value = row.get(field).and_then(Value::as_str).unwrap_or_default();
    insert_animation_identity(id, value, field, identities)
}

fn insert_animation_identity(
    id: &str,
    value: &str,
    field: &str,
    identities: &mut HashSet<String>,
) -> Result<(), AppError> {
    if value.is_empty() || !identities.insert(value.to_owned()) {
        return Err(output_error(
            id,
            format!("result.{field} identity is empty or duplicated"),
        ));
    }
    Ok(())
}

fn validate_animation_observation(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    validate_animation_request_fields(
        id,
        request,
        object,
        &[
            "sessionId",
            "characterId",
            "animationBlueprintId",
            "stateMachineId",
        ],
    )?;
    let weights = object["stateWeights"]
        .as_array()
        .ok_or_else(|| output_error(id, "stateWeights must be array"))?;
    if weights.len() != 2 || weights[0]["name"] != "idle" || weights[1]["name"] != "moving" {
        return Err(output_error(id, "stateWeights must be ordered idle,moving"));
    }
    let mut ids = HashSet::new();
    for row in weights {
        validate_identity(
            id,
            row.as_object()
                .ok_or_else(|| output_error(id, "state weight must be object"))?,
            "stateId",
            &mut ids,
        )?;
    }
    let idle_weight = weights[0]["weight"].as_f64().unwrap_or_default();
    let moving_weight = weights[1]["weight"].as_f64().unwrap_or_default();
    if ((idle_weight + moving_weight) - 1.0).abs() > 0.001 {
        return Err(output_error(
            id,
            "state weights must sum to one within 0.001",
        ));
    }
    let selected = usize::from(moving_weight > idle_weight);
    if object["activeStateId"] != weights[selected]["stateId"]
        || object["activeStateName"] != weights[selected]["name"]
    {
        return Err(output_error(
            id,
            "active state does not match selected weight row",
        ));
    }
    let transition = object["activeTransition"]
        .as_object()
        .ok_or_else(|| output_error(id, "activeTransition must be object"))?;
    let active = transition["active"]
        .as_bool()
        .ok_or_else(|| output_error(id, "transition active must be boolean"))?;
    for field in [
        "transitionId",
        "fromStateId",
        "toStateId",
        "elapsedFraction",
    ] {
        if active == transition[field].is_null() {
            return Err(output_error(
                id,
                "transition nullable fields are inconsistent",
            ));
        }
    }
    if active
        && !((transition["fromStateId"] == weights[0]["stateId"]
            && transition["toStateId"] == weights[1]["stateId"])
            || (transition["fromStateId"] == weights[1]["stateId"]
                && transition["toStateId"] == weights[0]["stateId"]))
    {
        return Err(output_error(
            id,
            "transition direction does not reference state weights",
        ));
    }
    Ok(())
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
fn validate_widget_output(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    let empty_request = Map::new();
    let request = request.as_object().unwrap_or(&empty_request);
    let string = |field: &str| require_output_string(id, object, field);
    let exact = |fields: &[&str]| {
        object
            .keys()
            .find(|key| !fields.contains(&key.as_str()))
            .map_or(Ok(()), |key| {
                Err(output_error(id, format!("unknown output field `{key}`")))
            })
    };
    let revision = || {
        let value = string("revision")?;
        if !canonical_revision(value) {
            return Err(output_error(id, "revision is not canonical"));
        }
        Ok(())
    };
    let arrays = || {
        for field in ["dirtyPackages", "savedPackages"] {
            if !object.get(field).is_some_and(Value::is_array) {
                return Err(output_error(id, format!("result.{field} must be an array")));
            }
        }
        Ok(())
    };
    let blueprint = match id {
        "widget.create" => {
            exact(&[
                "blueprintId",
                "generatedClass",
                "rootWidgetId",
                "rootName",
                "rootClass",
                "changed",
                "dirtyPackages",
                "savedPackages",
                "revision",
            ])?;
            let path = request
                .get("path")
                .and_then(Value::as_str)
                .ok_or_else(|| output_error(id, "request.path is missing"))?;
            let short = path
                .rsplit('/')
                .next()
                .filter(|v| !v.is_empty())
                .ok_or_else(|| output_error(id, "request.path is invalid"))?;
            let blueprint = format!("{path}.{short}");
            if string("blueprintId")? != blueprint
                || string("generatedClass")? != format!("{blueprint}_C")
                || string("rootWidgetId")?
                    != format!(
                        "{blueprint}#widget:{}",
                        request["rootName"].as_str().unwrap_or("")
                    )
                || string("rootName")? != request["rootName"].as_str().unwrap_or("")
                || string("rootClass")? != "VerticalBox"
                || !object.get("changed").is_some_and(Value::is_boolean)
            {
                return Err(output_error(id, "create output does not match request"));
            }
            arrays()?;
            revision()?;
            return Ok(());
        }
        "widget.child_ensure" => {
            exact(&[
                "blueprintId",
                "widgetId",
                "parentWidgetId",
                "name",
                "class",
                "changed",
                "dirtyPackages",
                "savedPackages",
                "revision",
            ])?;
            let blueprint = request
                .get("blueprintId")
                .and_then(Value::as_str)
                .ok_or_else(|| output_error(id, "request.blueprintId is missing"))?;
            let name = request.get("name").and_then(Value::as_str).unwrap_or("");
            for (field, expected) in [
                ("blueprintId", blueprint),
                (
                    "parentWidgetId",
                    request["parentWidgetId"].as_str().unwrap_or(""),
                ),
                ("name", name),
                ("class", "TextBlock"),
            ] {
                if string(field)? != expected {
                    return Err(output_error(id, "child output does not match request"));
                }
            }
            if string("widgetId")? != format!("{blueprint}#widget:{name}")
                || !object.get("changed").is_some_and(Value::is_boolean)
            {
                return Err(output_error(id, "child identity is invalid"));
            }
            arrays()?;
            revision()?;
            return Ok(());
        }
        "widget.property_set" => {
            exact(&[
                "blueprintId",
                "widgetId",
                "property",
                "text",
                "visibility",
                "enabled",
                "changed",
                "dirtyPackages",
                "savedPackages",
                "revision",
            ])?;
            for field in ["blueprintId", "widgetId", "property"] {
                if string(field)? != request.get(field).and_then(Value::as_str).unwrap_or("") {
                    return Err(output_error(id, "property output does not match request"));
                }
            }
            let property = string("property")?;
            if !object.contains_key(property)
                || ["text", "visibility", "enabled"]
                    .iter()
                    .any(|field| *field != property && object.contains_key(*field))
            {
                return Err(output_error(id, "property payload is not exact"));
            }
            if !object.get("changed").is_some_and(Value::is_boolean) {
                return Err(output_error(id, "result.changed must be boolean"));
            }
            arrays()?;
            revision()?;
            return Ok(());
        }
        "widget.event_ensure" => {
            exact(&[
                "blueprintId",
                "eventId",
                "agentKey",
                "event",
                "actions",
                "changed",
                "dirtyPackages",
                "savedPackages",
                "revision",
            ])?;
            for field in ["blueprintId", "agentKey", "event"] {
                if string(field)? != request.get(field).and_then(Value::as_str).unwrap_or("") {
                    return Err(output_error(id, "event output does not match request"));
                }
            }
            let blueprint = string("blueprintId")?;
            let agent = string("agentKey")?;
            if string("eventId")? != format!("{blueprint}#event:{agent}")
                || object.get("actions") != request.get("actions")
                || !object.get("changed").is_some_and(Value::is_boolean)
            {
                return Err(output_error(id, "event identity or actions mismatch"));
            }
            arrays()?;
            revision()?;
            return Ok(());
        }
        "widget.viewport_ensure" => {
            exact(&[
                "hostBlueprintId",
                "widgetBlueprintId",
                "viewportId",
                "graphId",
                "inputKey",
                "zOrder",
                "widgetRevision",
                "changed",
                "dirtyPackages",
                "savedPackages",
                "revision",
            ])?;
            let host = request
                .get("hostBlueprintId")
                .and_then(Value::as_str)
                .unwrap_or("");
            let agent = request
                .get("agentKey")
                .and_then(Value::as_str)
                .unwrap_or("");
            if string("hostBlueprintId")? != host
                || string("widgetBlueprintId")?
                    != request["widgetBlueprintId"].as_str().unwrap_or("")
                || string("inputKey")? != "E"
                || object["zOrder"] != 0
                || string("viewportId")? != format!("{host}#viewport:{agent}")
                || string("graphId")?.is_empty()
                || !canonical_revision(string("widgetRevision")?)
                || !object.get("changed").is_some_and(Value::is_boolean)
            {
                return Err(output_error(id, "viewport output does not match request"));
            }
            arrays()?;
            revision()?;
            return Ok(());
        }
        "widget.tree_view" => request
            .get("blueprintId")
            .and_then(Value::as_str)
            .ok_or_else(|| output_error(id, "request.blueprintId is missing"))?,
        "play.ui_observe" => {
            exact(&[
                "sessionId",
                "widgetBlueprintId",
                "instanceId",
                "inViewport",
                "widgets",
                "revision",
            ])?;
            if string("sessionId")? != request["sessionId"].as_str().unwrap_or("")
                || string("widgetBlueprintId")?
                    != request["widgetBlueprintId"].as_str().unwrap_or("")
                || string("instanceId")?.is_empty()
                || !object.get("inViewport").is_some_and(Value::is_boolean)
            {
                return Err(output_error(id, "observe identity is invalid"));
            }
            let ids = request["widgetIds"]
                .as_array()
                .ok_or_else(|| output_error(id, "request.widgetIds is missing"))?;
            let widgets = object["widgets"]
                .as_array()
                .ok_or_else(|| output_error(id, "result.widgets must be an array"))?;
            if ids.len() != widgets.len() {
                return Err(output_error(id, "observe widget count mismatch"));
            }
            for (wanted, row) in ids.iter().zip(widgets) {
                let row = row
                    .as_object()
                    .ok_or_else(|| output_error(id, "observe widget must be object"))?;
                let name = row["name"].as_str().unwrap_or("");
                if row.get("widgetId") != Some(wanted)
                    || row.get("widgetId")
                        != Some(&json!(format!(
                            "{}#widget:{name}",
                            request["widgetBlueprintId"].as_str().unwrap_or("")
                        )))
                    || row["class"] == "VerticalBox" && !row["text"].is_null()
                    || row["class"] == "TextBlock" && !row["text"].is_string()
                {
                    return Err(output_error(id, "observe widget identity or text invalid"));
                }
            }
            revision()?;
            return Ok(());
        }
        _ => unreachable!(),
    };
    exact(&[
        "blueprintId",
        "generatedClass",
        "rootWidgetId",
        "count",
        "total",
        "scope",
        "widgets",
        "events",
        "revision",
    ])?;
    if string("blueprintId")? != blueprint
        || string("generatedClass")? != format!("{blueprint}_C")
        || string("scope")? != blueprint
        || string("rootWidgetId")?
            != object["widgets"]
                .as_array()
                .and_then(|v| v.first())
                .and_then(|v| v["widgetId"].as_str())
                .unwrap_or("")
    {
        return Err(output_error(id, "tree identity mismatch"));
    }
    let widgets = object["widgets"]
        .as_array()
        .ok_or_else(|| output_error(id, "result.widgets must be array"))?;
    if object["count"] != json!(widgets.len())
        || object["total"] != json!(widgets.len())
        || widgets.is_empty()
    {
        return Err(output_error(id, "tree count mismatch"));
    }
    let mut names = HashSet::new();
    let mut ids = HashSet::new();
    let mut child_indices = HashSet::new();
    let mut prior_child_name: Option<&str> = None;
    for (row_index, widget) in widgets.iter().enumerate() {
        let widget = widget
            .as_object()
            .ok_or_else(|| output_error(id, "widget must be object"))?;
        let name = widget["name"].as_str().unwrap_or("");
        let wid = widget["widgetId"].as_str().unwrap_or("");
        if !names.insert(name) || !ids.insert(wid) || wid != format!("{blueprint}#widget:{name}") {
            return Err(output_error(id, "tree widget identity invalid"));
        }
        if row_index == 0 {
            if widget["class"] != "VerticalBox"
                || !widget["parentWidgetId"].is_null()
                || widget["index"] != 0
                || !widget["text"].is_null()
            {
                return Err(output_error(id, "tree root invalid"));
            }
        } else {
            let index = widget["index"]
                .as_u64()
                .ok_or_else(|| output_error(id, "child index must be integer"))?;
            if widget["class"] != "TextBlock"
                || widget["parentWidgetId"] != widgets[0]["widgetId"]
                || !child_indices.insert(index)
                || index >= (widgets.len() - 1) as u64
                || !widget["text"].is_string()
                || prior_child_name.is_some_and(|prior| prior >= name)
            {
                return Err(output_error(id, "tree child invalid"));
            }
            prior_child_name = Some(name);
        }
    }
    if child_indices.len() != widgets.len() - 1
        || !(0..(widgets.len() - 1) as u64).all(|index| child_indices.contains(&index))
    {
        return Err(output_error(
            id,
            "child indices must be contiguous physical slots",
        ));
    }
    let events = object["events"]
        .as_array()
        .ok_or_else(|| output_error(id, "result.events must be array"))?;
    let mut prior = None;
    for event in events {
        let event = event
            .as_object()
            .ok_or_else(|| output_error(id, "event must be object"))?;
        let event_id = event["eventId"].as_str().unwrap_or("");
        if prior.is_some_and(|p| p >= event_id)
            || !event_id.starts_with(&format!("{blueprint}#event:"))
        {
            return Err(output_error(id, "events not ordered or derived"));
        }
        prior = Some(event_id);
        for action in event["actions"]
            .as_array()
            .ok_or_else(|| output_error(id, "event actions must be array"))?
        {
            if !ids.contains(action["targetWidgetId"].as_str().unwrap_or("")) {
                return Err(output_error(id, "event target missing from tree"));
            }
        }
    }
    revision()
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
fn validate_graph_view_output(id: &str, object: &Map<String, Value>) -> Result<(), AppError> {
    let count = require_output_u64(id, object, "count")?;
    let total = require_output_u64(id, object, "total")?;
    let items = object
        .get("items")
        .and_then(Value::as_array)
        .ok_or_else(|| output_error(id, "result.items must be an array"))?;
    require_output_string(id, object, "blueprintId")?;
    require_output_string(id, object, "scope")?;
    if count as usize != items.len() || count > total || count > 100 {
        return Err(output_error(
            id,
            "graph page count and total are inconsistent",
        ));
    }
    if !object.get("nextCursor").is_some_and(|value| {
        value.is_null()
            || value
                .as_str()
                .is_some_and(|text| !text.is_empty() && text.len() <= 256)
    }) {
        return Err(output_error(
            id,
            "nextCursor must be null or a bounded string",
        ));
    }
    let revision = require_output_string(id, object, "revision")?;
    if revision.len() != 64 || !revision.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err(output_error(
            id,
            "graph revision must be a SHA-256 hex value",
        ));
    }
    let mut row_kind = None;
    let mut previous = None;
    for item in items {
        let item = item
            .as_object()
            .ok_or_else(|| output_error(id, "graph items must be objects"))?;
        let (kind, identity) = if let Some(identity) = item.get("graphId").and_then(Value::as_str) {
            ("graph", identity)
        } else if let Some(identity) = item.get("nodeId").and_then(Value::as_str) {
            ("node", identity)
        } else {
            return Err(output_error(id, "graph items require graphId or nodeId"));
        };
        if row_kind.is_some_and(|expected| expected != kind) {
            return Err(output_error(
                id,
                "graph page cannot mix graph and node rows",
            ));
        }
        if previous.is_some_and(|prior| prior >= identity) {
            return Err(output_error(
                id,
                "graph item identities must be unique and ordered",
            ));
        }
        row_kind = Some(kind);
        previous = Some(identity);
    }
    Ok(())
}
fn validate_graph_cursor_input(id: &str, cursor: &str) -> Result<(), AppError> {
    let mut parts = cursor.split('.');
    let version = parts.next();
    let snapshot = parts.next();
    let offset = parts.next();
    if version != Some("v1") || parts.next().is_some() {
        return Err(input_error(
            id,
            "cursor must use v1.<64 hex snapshot>.<offset>",
        ));
    }
    let snapshot = snapshot.ok_or_else(|| input_error(id, "cursor snapshot is missing"))?;
    if snapshot.len() != 64
        || !snapshot
            .bytes()
            .all(|byte| byte.is_ascii_digit() || (b'a'..=b'f').contains(&byte))
    {
        return Err(input_error(
            id,
            "cursor snapshot must be 64 lowercase ASCII hex characters",
        ));
    }
    let offset = offset.ok_or_else(|| input_error(id, "cursor offset is missing"))?;
    if (offset.len() > 1 && offset.starts_with('0'))
        || offset.is_empty()
        || !offset.bytes().all(|byte| byte.is_ascii_digit())
    {
        return Err(input_error(
            id,
            "cursor offset must be canonical nonnegative decimal",
        ));
    }
    offset
        .parse::<u64>()
        .map_err(|_| input_error(id, "cursor offset is out of range"))?;
    Ok(())
}
fn parse_graph_cursor<'a>(id: &str, cursor: &'a str) -> Result<(&'a str, u64), AppError> {
    let mut parts = cursor.split('.');
    let version = parts.next();
    let snapshot = parts.next();
    let offset = parts.next();
    if version != Some("v1") || parts.next().is_some() {
        return Err(output_error(
            id,
            "cursor must use v1.<64 hex snapshot>.<offset>",
        ));
    }
    let snapshot = snapshot.ok_or_else(|| output_error(id, "cursor snapshot is missing"))?;
    if snapshot.len() != 64
        || !snapshot
            .bytes()
            .all(|byte| byte.is_ascii_digit() || (b'a'..=b'f').contains(&byte))
    {
        return Err(output_error(
            id,
            "cursor snapshot must be 64 lowercase ASCII hex characters",
        ));
    }
    let offset = offset.ok_or_else(|| output_error(id, "cursor offset is missing"))?;
    if (offset.len() > 1 && offset.starts_with('0'))
        || offset.is_empty()
        || !offset.bytes().all(|byte| byte.is_ascii_digit())
    {
        return Err(output_error(
            id,
            "cursor offset must be canonical nonnegative decimal",
        ));
    }
    let offset = offset
        .parse::<u64>()
        .map_err(|_| output_error(id, "cursor offset is out of range"))?;
    Ok((snapshot, offset))
}

fn canonical_row(fields: &[&str]) -> String {
    fields
        .iter()
        .map(|field| format!("{}:{}", field.encode_utf16().count(), field))
        .collect()
}

fn graph_cursor_snapshot(revision: &str, scope: &str, kind: &str) -> String {
    format!(
        "{:x}",
        Sha256::digest(canonical_row(&[revision, scope, kind]).as_bytes())
    )
}

fn canonical_guid(value: &str) -> bool {
    value.len() == 36
        && value.as_bytes().iter().enumerate().all(|(index, byte)| {
            if matches!(index, 8 | 13 | 18 | 23) {
                *byte == b'-'
            } else {
                byte.is_ascii_digit() || (b'a'..=b'f').contains(byte)
            }
        })
}

fn graph_identity(blueprint_id: &str, kind: &str, identity: &str) -> bool {
    matches!(
        kind,
        "interface" | "ubergraph" | "function" | "macro" | "delegate_signature" | "other"
    ) && identity
        .strip_prefix(&format!("{blueprint_id}#graph:{kind}:"))
        .is_some_and(canonical_guid)
}

fn node_identity(graph_id: &str, identity: &str) -> bool {
    identity
        .strip_prefix(&format!("{graph_id}#node:"))
        .is_some_and(canonical_guid)
}

fn encoded_pin_name(value: &str) -> bool {
    canonical_pin_encoding(value)
}

fn canonical_pin_encoding(value: &str) -> bool {
    let bytes = value.as_bytes();
    let mut index = 0;
    if bytes.is_empty() {
        return false;
    }
    while index < bytes.len() {
        if bytes[index].is_ascii_alphanumeric() || matches!(bytes[index], b'-' | b'_' | b'.' | b'~')
        {
            index += 1;
        } else if bytes[index] == b'%'
            && index + 2 < bytes.len()
            && bytes[index + 1].is_ascii_hexdigit()
            && bytes[index + 2].is_ascii_hexdigit()
            && (bytes[index + 1].is_ascii_digit() || bytes[index + 1].is_ascii_uppercase())
            && (bytes[index + 2].is_ascii_digit() || bytes[index + 2].is_ascii_uppercase())
        {
            index += 3;
        } else {
            return false;
        }
    }
    true
}

fn encode_pin_name(value: &str) -> String {
    value.bytes().fold(String::new(), |mut encoded, byte| {
        if byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_' | b'.' | b'~') {
            encoded.push(byte as char);
        } else {
            encoded.push('%');
            encoded.push_str(&format!("{byte:02X}"));
        }
        encoded
    })
}

fn pin_identity_exact(node_id: &str, identity: &str, direction: &str, name: &str) -> bool {
    identity == format!("{node_id}#pin:{direction}:{}", encode_pin_name(name))
}

fn pin_identity(node_id: &str, identity: &str) -> bool {
    let Some(suffix) = identity.strip_prefix(&format!("{node_id}#pin:")) else {
        return false;
    };
    suffix
        .strip_prefix("input:")
        .or_else(|| suffix.strip_prefix("output:"))
        .is_some_and(encoded_pin_name)
}

fn link_identity(graph_id: &str, identity: &str) -> bool {
    let Some((node_id, _)) = identity.split_once("#pin:") else {
        return false;
    };
    node_identity(graph_id, node_id) && pin_identity(node_id, identity)
}

fn validate_graph_view_page(
    id: &str,
    request: &Value,
    object: &Map<String, Value>,
) -> Result<(), AppError> {
    let blueprint_id = request
        .get("blueprintId")
        .and_then(Value::as_str)
        .ok_or_else(|| output_error(id, "request blueprintId is missing"))?;
    if object.get("blueprintId").and_then(Value::as_str) != Some(blueprint_id) {
        return Err(output_error(
            id,
            "response blueprintId does not match request",
        ));
    }
    let graph_id = request.get("graphId").and_then(Value::as_str);
    let expected_scope = graph_id.unwrap_or(blueprint_id);
    if object.get("scope").and_then(Value::as_str) != Some(expected_scope) {
        return Err(output_error(id, "response scope does not match request"));
    }
    let input_cursor = request
        .get("cursor")
        .and_then(Value::as_str)
        .map(|cursor| parse_graph_cursor(id, cursor))
        .transpose()?;
    let expected_kind = if graph_id.is_some() {
        "nodes"
    } else {
        "graphs"
    };
    let revision = require_output_string(id, object, "revision")?;
    let expected_snapshot = graph_cursor_snapshot(revision, expected_scope, expected_kind);
    if input_cursor
        .as_ref()
        .is_some_and(|(snapshot, _)| *snapshot != expected_snapshot)
    {
        return Err(output_error(
            id,
            "input cursor snapshot does not match requested graph snapshot",
        ));
    }
    let input_cursor = request
        .get("cursor")
        .and_then(Value::as_str)
        .map(|cursor| parse_graph_cursor(id, cursor))
        .transpose()?;
    let current_offset = input_cursor.map_or(0, |(_, offset)| offset);
    let requested_limit = request.get("limit").and_then(Value::as_u64).unwrap_or(100);
    let count = require_output_u64(id, object, "count")?;
    let total = require_output_u64(id, object, "total")?;
    if input_cursor.is_some() && (total == 0 || current_offset >= total) {
        return Err(output_error(
            id,
            "input cursor must point within nonempty graph result",
        ));
    }
    let items = object
        .get("items")
        .and_then(Value::as_array)
        .ok_or_else(|| output_error(id, "result.items must be an array"))?;
    if count > requested_limit
        || count as usize != items.len()
        || current_offset > total
        || count > total - current_offset
    {
        return Err(output_error(
            id,
            "graph page count or offset is inconsistent",
        ));
    }
    if total > current_offset && count == 0 {
        return Err(output_error(id, "nonterminal graph page cannot be empty"));
    }
    let mut previous = None;
    for item in items {
        let item = item
            .as_object()
            .ok_or_else(|| output_error(id, "graph items must be objects"))?;
        let identity = if let Some(identity) = item.get("graphId").and_then(Value::as_str) {
            let kind = item.get("kind").and_then(Value::as_str).unwrap_or("");
            if graph_id.is_some() || !graph_identity(blueprint_id, kind, identity) {
                return Err(output_error(
                    id,
                    "graph identity is not bound to requested Blueprint",
                ));
            }
            identity
        } else if let Some(identity) = item.get("nodeId").and_then(Value::as_str) {
            let graph = graph_id
                .ok_or_else(|| output_error(id, "node row requires requested graph scope"))?;
            if !node_identity(graph, identity) {
                return Err(output_error(
                    id,
                    "node identity is not bound to requested graph",
                ));
            }
            let mut previous_pin = None;
            for pin in item
                .get("pins")
                .and_then(Value::as_array)
                .ok_or_else(|| output_error(id, "node pins must be an array"))?
            {
                let pin = pin
                    .as_object()
                    .ok_or_else(|| output_error(id, "pins must be objects"))?;
                let pin_id = pin
                    .get("pinId")
                    .and_then(Value::as_str)
                    .ok_or_else(|| output_error(id, "pinId is missing"))?;
                if !pin_identity(identity, pin_id) {
                    return Err(output_error(
                        id,
                        "pin identity is not bound to requested node",
                    ));
                }
                if let Some(prior) = previous_pin
                    && prior >= pin_id
                {
                    return Err(output_error(
                        id,
                        format!(
                            "pin identities must be unique and ordered: `{prior}` before `{pin_id}`"
                        ),
                    ));
                }
                previous_pin = Some(pin_id);
                let direction = pin
                    .get("direction")
                    .and_then(Value::as_str)
                    .ok_or_else(|| output_error(id, "pin direction is missing"))?;
                let name = pin
                    .get("name")
                    .and_then(Value::as_str)
                    .ok_or_else(|| output_error(id, "pin name is missing"))?;
                if !matches!(direction, "input" | "output")
                    || !pin_identity_exact(identity, pin_id, direction, name)
                {
                    return Err(output_error(
                        id,
                        "pin identity does not match direction and name",
                    ));
                }
                let mut previous_link = None;
                for link in pin
                    .get("links")
                    .and_then(Value::as_array)
                    .ok_or_else(|| output_error(id, "pin links must be an array"))?
                {
                    let link = link
                        .as_str()
                        .ok_or_else(|| output_error(id, "pin links must be strings"))?;
                    if !link_identity(graph, link) {
                        return Err(output_error(id, "pin link is not bound to requested graph"));
                    }
                    if previous_link.is_some_and(|prior| prior >= link) {
                        return Err(output_error(id, "pin links must be unique and ordered"));
                    }
                    previous_link = Some(link);
                }
            }
            identity
        } else {
            return Err(output_error(id, "graph items require graphId or nodeId"));
        };
        if previous.is_some_and(|prior| prior >= identity) {
            return Err(output_error(
                id,
                "graph item identities must be unique and ordered",
            ));
        }
        previous = Some(identity);
    }
    let next = object
        .get("nextCursor")
        .ok_or_else(|| output_error(id, "result.nextCursor is missing"))?;
    let terminal = current_offset.checked_add(count) == Some(total);
    if terminal {
        if !next.is_null() {
            return Err(output_error(
                id,
                "terminal graph page must have null nextCursor",
            ));
        }
    } else {
        let next = next
            .as_str()
            .ok_or_else(|| output_error(id, "nonterminal graph page requires nextCursor"))?;
        let (snapshot, next_offset) = parse_graph_cursor(id, next)?;
        if snapshot != expected_snapshot {
            return Err(output_error(
                id,
                "output cursor snapshot does not match requested graph snapshot",
            ));
        }
        if next_offset != current_offset + count {
            return Err(output_error(id, "nextCursor offset does not advance page"));
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

fn valid_widget_name(value: &str) -> bool {
    let bytes = value.as_bytes();
    !bytes.is_empty()
        && bytes.len() <= 64
        && bytes[0].is_ascii_alphabetic()
        && bytes[1..]
            .iter()
            .all(|byte| byte.is_ascii_alphanumeric() || *byte == b'_')
}

fn valid_agent_key(value: &str) -> bool {
    !value.is_empty()
        && value.len() <= 128
        && value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'.' | b'_' | b':' | b'-'))
}

fn valid_object_id(value: &str) -> bool {
    let Some((package, object)) = value.rsplit_once('.') else {
        return false;
    };
    let Some(path) = package.strip_prefix("/Game/") else {
        return false;
    };
    !path.is_empty()
        && !path.contains('.')
        && !path.contains("//")
        && !path
            .split('/')
            .any(|part| part.is_empty() || part == "." || part == "..")
        && object == path.rsplit('/').next().unwrap_or("")
        && object
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'_' | b'-'))
}

fn valid_package_path(value: &str) -> bool {
    let Some(path) = value.strip_prefix("/Game/") else {
        return false;
    };
    !path.is_empty()
        && !path.contains('.')
        && !path.contains("//")
        && !path
            .split('/')
            .any(|part| part.is_empty() || part == "." || part == "..")
}

fn valid_widget_id(blueprint: &str, widget: &str) -> bool {
    widget
        .strip_prefix(&format!("{blueprint}#widget:"))
        .is_some_and(valid_widget_name)
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

pub(crate) fn canonical_revision(value: &str) -> bool {
    value.len() == 64
        && value
            .bytes()
            .all(|byte| byte.is_ascii_digit() || (b'a'..=b'f').contains(&byte))
}

fn validate_canonical_revisions(id: &str, value: &Value) -> Result<(), AppError> {
    let invalid = match value {
        Value::Object(object) => object.iter().any(|(key, value)| {
            (matches!(
                key.as_str(),
                "revision" | "beforeRevision" | "afterRevision" | "observedRevision"
            ) && !value.as_str().is_some_and(canonical_revision))
                || validate_canonical_revisions(id, value).is_err()
        }),
        Value::Array(values) => values
            .iter()
            .any(|value| validate_canonical_revisions(id, value).is_err()),
        _ => false,
    };
    if invalid {
        return Err(output_error(
            id,
            "canonical revision must be exactly 64 lowercase hexadecimal characters",
        ));
    }
    Ok(())
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
    fn canonical_revision_rejects_uppercase_and_non_hex() {
        for revision in ["A".repeat(64), "g".repeat(64)] {
            let result = json!({"state":"stopped","sessionId":null,"worldId":null,"levelId":null,"playerCount":0,"revision":revision});
            assert!(validate_output("play.status", result).is_err());
        }
    }

    #[test]
    fn search_is_compact_and_deterministic() {
        let result = search("actor", 50);
        assert_eq!(result["count"], 7);
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
        let fixtures: Vec<Value> =
            serde_json::from_str(include_str!("../capabilities/input-invalid.generated.json"))
                .unwrap();
        assert!(!fixtures.is_empty());
        for fixture in fixtures {
            let id = fixture["operation"].as_str().unwrap();
            assert!(
                validate_generated_input(id, &fixture["args"]).is_err(),
                "{id} accepted generated invalid input"
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
    fn graph_view_rejects_inconsistent_pagination_and_ordering() {
        let empty = json!({
            "blueprintId":"/Game/BP.BP","count":0,"total":0,"scope":"/Game/BP.BP",
            "revision":"0".repeat(64),"items":[],"nextCursor":null
        });
        let mut count_mismatch = empty.clone();
        count_mismatch["count"] = json!(1);
        assert!(validate_output("blueprint.graph_view", count_mismatch).is_err());
        let mut non_hex_revision = empty.clone();
        non_hex_revision["revision"] = json!("z".repeat(64));
        assert!(validate_output("blueprint.graph_view", non_hex_revision).is_err());
        let mut unordered = empty;
        unordered["count"] = json!(2);
        unordered["total"] = json!(2);
        unordered["items"] = json!([
            {"graphId":"z","kind":"ubergraph","name":"Z","nodeCount":0},
            {"graphId":"a","kind":"ubergraph","name":"A","nodeCount":0}
        ]);
        assert!(validate_output("blueprint.graph_view", unordered).is_err());

        let revision = "0".repeat(64);
        let graph_a = "/Game/BP.BP#graph:ubergraph:00000000-0000-0000-0000-000000000001";
        let graph_b = "/Game/BP.BP#graph:ubergraph:00000000-0000-0000-0000-000000000002";
        let snapshot = graph_cursor_snapshot(&revision, "/Game/BP.BP", "graphs");
        let other_snapshot = "b".repeat(64);
        let request = json!({"blueprintId":"/Game/BP.BP","limit":1});
        let page = json!({
            "blueprintId":"/Game/BP.BP","count":1,"total":2,"scope":"/Game/BP.BP",
            "revision":revision,"items":[{"graphId":graph_a,"kind":"ubergraph","name":"A","nodeCount":0}],
            "nextCursor":format!("v1.{snapshot}.1")
        });
        assert!(
            validate_output_for_request("blueprint.graph_view", &request, page.clone()).is_ok()
        );
        let mut wrong_blueprint = page.clone();
        wrong_blueprint["blueprintId"] = json!("/Game/Other.BP");
        assert!(
            validate_output_for_request("blueprint.graph_view", &request, wrong_blueprint).is_err()
        );
        let mut foreign_graph = page.clone();
        foreign_graph["items"][0]["graphId"] =
            json!("/Game/Other.BP#graph:ubergraph:00000000-0000-0000-0000-000000000001");
        assert!(
            validate_output_for_request("blueprint.graph_view", &request, foreign_graph).is_err()
        );

        let node_id = format!("{graph_a}#node:00000000-0000-0000-0000-000000000010");
        let linked_node = format!("{graph_a}#node:00000000-0000-0000-0000-000000000011");
        let node_request = json!({"blueprintId":"/Game/BP.BP","graphId":graph_a,"limit":1});
        let node_page = json!({
            "blueprintId":"/Game/BP.BP","count":1,"total":1,"scope":graph_a,
            "revision":revision,"items":[{
                "nodeId":node_id,"class":"/Script/BlueprintGraph.K2Node_Event","title":"BeginPlay","x":0,"y":0,
                "pins":[{"pinId":format!("{node_id}#pin:output:then"),"name":"then","direction":"output","type":"exec","defaultValue":"","links":[format!("{linked_node}#pin:input:execute")]}]
            }],"nextCursor":null
        });
        assert!(
            validate_output_for_request("blueprint.graph_view", &node_request, node_page.clone())
                .is_ok()
        );
        let mut unordered_pins = node_page.clone();
        unordered_pins["items"][0]["pins"]
            .as_array_mut()
            .unwrap()
            .push(json!({
                "pinId":format!("{node_id}#pin:input:A"),"name":"A","direction":"input",
                "type":"exec","defaultValue":"","links":[]
            }));
        assert!(
            validate_output_for_request("blueprint.graph_view", &node_request, unordered_pins)
                .is_err()
        );
        let mut duplicate_links = node_page.clone();
        let link = duplicate_links["items"][0]["pins"][0]["links"][0].clone();
        duplicate_links["items"][0]["pins"][0]["links"]
            .as_array_mut()
            .unwrap()
            .push(link);
        assert!(
            validate_output_for_request("blueprint.graph_view", &node_request, duplicate_links)
                .is_err()
        );
        let mut foreign_pin = node_page;
        foreign_pin["items"][0]["pins"][0]["pinId"] =
            json!(format!("{linked_node}#pin:output:then"));
        assert!(
            validate_output_for_request("blueprint.graph_view", &node_request, foreign_pin)
                .is_err()
        );

        let mut wrong_scope = page.clone();
        wrong_scope["scope"] = json!("/Game/Other.BP");
        assert!(
            validate_output_for_request("blueprint.graph_view", &request, wrong_scope).is_err()
        );
        let node_request = json!({"blueprintId":"/Game/BP.BP","graphId":"a","limit":1});
        assert!(
            validate_output_for_request("blueprint.graph_view", &node_request, page.clone())
                .is_err()
        );

        let mut malformed_output_cursor = page.clone();
        malformed_output_cursor["nextCursor"] = json!("v1.bad.1");
        assert!(
            validate_output_for_request("blueprint.graph_view", &request, malformed_output_cursor)
                .is_err()
        );
        let malformed_input = json!({"blueprintId":"/Game/BP.BP","limit":1,"cursor":"v1.bad.1"});
        assert!(
            validate_output_for_request("blueprint.graph_view", &malformed_input, page.clone())
                .is_err()
        );
        let second_page = json!({
            "blueprintId":"/Game/BP.BP","count":1,"total":2,"scope":"/Game/BP.BP",
            "revision":"0".repeat(64),
            "items":[{"graphId":graph_b,"kind":"ubergraph","name":"B","nodeCount":0}],
            "nextCursor":null
        });
        let empty_page = json!({
            "blueprintId":"/Game/BP.BP","count":0,"total":0,"scope":"/Game/BP.BP",
            "revision":"0".repeat(64),"items":[],"nextCursor":null
        });
        let empty_cursor =
            json!({"blueprintId":"/Game/BP.BP","limit":1,"cursor":format!("v1.{snapshot}.0")});
        assert!(
            validate_output_for_request("blueprint.graph_view", &empty_cursor, empty_page).is_err()
        );

        let second_request =
            json!({"blueprintId":"/Game/BP.BP","limit":1,"cursor":format!("v1.{snapshot}.1")});
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &second_request,
                second_page.clone()
            )
            .is_ok()
        );

        let mut empty_nonterminal = second_page.clone();
        empty_nonterminal["count"] = json!(0);
        empty_nonterminal["items"] = json!([]);
        empty_nonterminal["nextCursor"] = json!(format!("v1.{snapshot}.1"));
        assert!(
            validate_output_for_request("blueprint.graph_view", &second_request, empty_nonterminal)
                .is_err()
        );
        let mut next_offset_mismatch = page.clone();
        next_offset_mismatch["nextCursor"] = json!(format!("v1.{snapshot}.2"));
        assert!(
            validate_output_for_request("blueprint.graph_view", &request, next_offset_mismatch)
                .is_err()
        );
        let mut premature_null = page.clone();
        premature_null["nextCursor"] = Value::Null;
        assert!(
            validate_output_for_request("blueprint.graph_view", &request, premature_null).is_err()
        );
        let mut nonterminal_cursor = second_page.clone();
        nonterminal_cursor["nextCursor"] = json!(format!("v1.{snapshot}.2"));
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &second_request,
                nonterminal_cursor
            )
            .is_err()
        );
        let mut changed_snapshot = second_page;
        changed_snapshot["total"] = json!(3);
        changed_snapshot["nextCursor"] = json!(format!("v1.{other_snapshot}.2"));
        assert!(
            validate_output_for_request("blueprint.graph_view", &second_request, changed_snapshot)
                .is_err()
        );
        let malformed_cursor = validate_input(
            "blueprint.graph_view",
            json!({"blueprintId":"/Game/BP.BP","cursor":"v1.bad.01"}),
        )
        .unwrap_err();
        assert_eq!(malformed_cursor.reason, "invalid_capability_input");
        let uppercase_cursor = validate_input(
            "blueprint.graph_view",
            json!({"blueprintId":"/Game/BP.BP","cursor":format!("v1.{}.0", "A".repeat(64))}),
        )
        .unwrap_err();
        assert_eq!(uppercase_cursor.reason, "invalid_capability_input");

        let pin_graph = graph_a;
        let pin_node = format!("{pin_graph}#node:00000000-0000-0000-0000-000000000010");
        let pin_request = json!({"blueprintId":"/Game/BP.BP","graphId":pin_graph,"limit":1});
        let pin_page = |pin_id: String, name: &str, direction: &str| json!({"blueprintId":"/Game/BP.BP","count":1,"total":1,"scope":pin_graph,"revision":"0".repeat(64),"items":[{"nodeId":pin_node,"class":"C","title":"N","x":0,"y":0,"pins":[{"pinId":pin_id,"name":name,"direction":direction,"type":"exec","defaultValue":"","links":[]}]}],"nextCursor":null});
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &pin_request,
                pin_page(format!("{pin_node}#pin:output:%2541"), "%41", "output")
            )
            .is_ok()
        );
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &pin_request,
                pin_page(format!("{pin_node}#pin:output:%41"), "%41", "output")
            )
            .is_err()
        );
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &pin_request,
                pin_page(format!("{pin_node}#pin:output:%FF"), "ÿ", "output")
            )
            .is_err()
        );
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &pin_request,
                pin_page(format!("{pin_node}#pin:input:%2541"), "%41", "output")
            )
            .is_err()
        );
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &pin_request,
                pin_page(format!("{pin_node}#pin:output:%C3%BC"), "ü", "output")
            )
            .is_ok()
        );
        assert!(
            validate_output_for_request(
                "blueprint.graph_view",
                &pin_request,
                pin_page(format!("{pin_node}#pin:output:%c3%bc"), "ü", "output")
            )
            .is_err()
        );
    }

    #[test]
    fn invalid_fields_fail_before_transport() {
        let error = validate_input("actor.list", json!({"fields":["label","label"]})).unwrap_err();
        assert_eq!(error.reason, "invalid_capability_input");
    }

    #[test]
    fn screenshot_filename_controls_fail_before_transport() {
        for path in [
            "claimed.png\0suffix.png",
            "claimed\u{85}.png",
            "claimed\u{9f}.png",
        ] {
            let error = validate_input(
                "play.screenshot",
                json!({"sessionId":"m6-pie-1","path":path}),
            )
            .unwrap_err();
            assert_eq!(error.reason, "invalid_capability_input");
        }
    }

    #[test]
    fn component_list_accepts_required_actor_scope() {
        let input = json!({"actorId":"/Game/Map#guid","limit":10,"fields":["id","name"]});
        assert_eq!(
            validate_input("component.list", input.clone()).unwrap(),
            input
        );
    }
    #[test]
    fn animation_mutations_bind_request_and_state_semantics() {
        let revision = "0".repeat(64);
        let create_request = json!({"path":"/Game/ABP","skeletonId":"/Game/Skeleton.Skeleton"});
        let create = json!({
            "animationBlueprintId":"/Game/ABP.ABP","skeletonId":"/Game/Skeleton.Skeleton",
            "generatedClass":"/Game/ABP.ABP_C","animGraphId":"anim-graph","rootNodeId":"root",
            "changed":true,"dirtyPackages":["/Game/ABP"],"savedPackages":[],"revision":revision
        });
        assert!(
            validate_output_for_request(
                "animation_blueprint.create",
                &create_request,
                create.clone()
            )
            .is_ok()
        );
        let mut wrong_skeleton = create;
        wrong_skeleton["skeletonId"] = json!("/Game/Other.Other");
        assert!(
            validate_output_for_request(
                "animation_blueprint.create",
                &create_request,
                wrong_skeleton
            )
            .is_err()
        );

        let character_request = json!({
            "characterBlueprintId":"/Game/Char.Char","skeletalMeshId":"/Game/Mesh.Mesh",
            "animationBlueprintId":"/Game/ABP.ABP"
        });
        let character = json!({
            "characterBlueprintId":"/Game/Char.Char","meshComponentId":"mesh",
            "skeletalMeshId":"/Game/Mesh.Mesh","skeletonId":"/Game/Skeleton.Skeleton",
            "animationMode":"AnimationBlueprint","animationBlueprintId":"/Game/ABP.ABP",
            "animClass":"/Game/ABP.ABP_C","changed":true,"dirtyPackages":["/Game/Char"],
            "savedPackages":[],"revision":"0".repeat(64)
        });
        assert!(
            validate_output_for_request(
                "animation.character_configure",
                &character_request,
                character.clone()
            )
            .is_ok()
        );
        let mut wrong_class = character;
        wrong_class["animClass"] = json!("/Game/Other.Other_C");
        assert!(
            validate_output_for_request(
                "animation.character_configure",
                &character_request,
                wrong_class
            )
            .is_err()
        );

        let state_request = json!({
            "animationBlueprintId":"/Game/ABP.ABP","stateMachineId":"machine",
            "name":"idle","sequenceId":"/Game/Idle.Idle"
        });
        let state = json!({
            "animationBlueprintId":"/Game/ABP.ABP","stateMachineId":"machine","stateId":"idle-state",
            "stateGraphId":"idle-graph","resultNodeId":"idle-result","sequencePlayerNodeId":"idle-player",
            "name":"idle","sequenceId":"/Game/Idle.Idle","skeletonId":"/Game/Skeleton.Skeleton",
            "initial":true,"changed":true,"dirtyPackages":["/Game/ABP"],"savedPackages":[],
            "revision":"0".repeat(64)
        });
        assert!(
            validate_output_for_request("animation.state_ensure", &state_request, state.clone())
                .is_ok()
        );
        let mut wrong_initial = state;
        wrong_initial["initial"] = json!(false);
        assert!(
            validate_output_for_request("animation.state_ensure", &state_request, wrong_initial)
                .is_err()
        );
    }

    #[test]
    fn animation_graph_enforces_skeleton_topology_and_direction() {
        let request = json!({"animationBlueprintId":"/Game/ABP.ABP"});
        let graph = json!({
            "animationBlueprintId":"/Game/ABP.ABP","skeletonId":"/Game/Skeleton.Skeleton",
            "generatedClass":"/Game/ABP.ABP_C","animGraphId":"anim-graph","rootNodeId":"root",
            "variables":[{
                "variableId":"speed-variable","bindingId":"speed-binding","name":"Speed","type":"float",
                "source":"owner_planar_speed","updateGraphId":"update-graph","eventNodeId":"update-event",
                "ownerNodeId":"owner","velocityNodeId":"velocity","planarSpeedNodeId":"planar-speed",
                "setterNodeId":"speed-setter"
            }],
            "stateMachines":[{
                "stateMachineId":"machine","stateMachineGraphId":"machine-graph","entryNodeId":"entry",
                "name":"locomotion","initialStateId":"idle-state",
                "states":[
                    {"stateId":"idle-state","stateGraphId":"idle-graph","resultNodeId":"idle-result",
                     "sequencePlayerNodeId":"idle-player","name":"idle","sequenceId":"/Game/Idle.Idle",
                     "skeletonId":"/Game/Skeleton.Skeleton","initial":true},
                    {"stateId":"moving-state","stateGraphId":"moving-graph","resultNodeId":"moving-result",
                     "sequencePlayerNodeId":"moving-player","name":"moving","sequenceId":"/Game/Moving.Moving",
                     "skeletonId":"/Game/Skeleton.Skeleton","initial":false}
                ],
                "transitions":[
                    {"transitionId":"idle-moving","transitionGraphId":"idle-moving-graph",
                     "resultNodeId":"idle-moving-result","variableGetterNodeId":"idle-moving-getter",
                     "comparisonNodeId":"idle-moving-compare","fromStateId":"idle-state",
                     "toStateId":"moving-state","expression":"Speed > 10"},
                    {"transitionId":"moving-idle","transitionGraphId":"moving-idle-graph",
                     "resultNodeId":"moving-idle-result","variableGetterNodeId":"moving-idle-getter",
                     "comparisonNodeId":"moving-idle-compare","fromStateId":"moving-state",
                     "toStateId":"idle-state","expression":"Speed <= 10"}
                ]
            }],
            "revision":"0".repeat(64)
        });
        assert!(
            validate_output_for_request("animation.graph_view", &request, graph.clone()).is_ok()
        );
        let mut wrong_skeleton = graph.clone();
        wrong_skeleton["stateMachines"][0]["states"][1]["skeletonId"] = json!("/Game/Other.Other");
        assert!(
            validate_output_for_request("animation.graph_view", &request, wrong_skeleton).is_err()
        );
        let mut wrong_direction = graph.clone();
        wrong_direction["stateMachines"][0]["transitions"][0]["expression"] = json!("Speed <= 10");
        assert!(
            validate_output_for_request("animation.graph_view", &request, wrong_direction).is_err()
        );
        let mut duplicate_identity = graph.clone();
        duplicate_identity["stateMachines"][0]["states"][1]["resultNodeId"] = json!("idle-result");
        assert!(
            validate_output_for_request("animation.graph_view", &request, duplicate_identity)
                .is_err()
        );
        let mut moving_only = graph.clone();
        moving_only["stateMachines"][0]["states"] =
            json!([moving_only["stateMachines"][0]["states"][1].clone()]);
        moving_only["stateMachines"][0]["initialStateId"] = Value::Null;
        moving_only["stateMachines"][0]["transitions"] = json!([]);
        assert!(validate_output_for_request("animation.graph_view", &request, moving_only).is_ok());
        let mut reverse_only = graph.clone();
        reverse_only["stateMachines"][0]["transitions"] =
            json!([reverse_only["stateMachines"][0]["transitions"][1].clone()]);
        assert!(
            validate_output_for_request("animation.graph_view", &request, reverse_only).is_ok()
        );
        let character_request = json!({"characterBlueprintId":"/Game/Char.Char"});
        let character_view = json!({
            "characterBlueprintId":"/Game/Char.Char","meshComponentId":"mesh",
            "skeletalMeshId":"/Game/Mesh.Mesh","skeletonId":"/Game/Skeleton.Skeleton",
            "animationMode":"AnimationBlueprint","animationBlueprintId":"/Game/ABP.ABP",
            "animClass":"/Game/ABP.ABP_C","revision":"0".repeat(64)
        });
        assert!(
            validate_output_for_request(
                "animation.character_view",
                &character_request,
                character_view.clone()
            )
            .is_ok()
        );
        let mut partial_null = character_view;
        partial_null["skeletonId"] = Value::Null;
        assert!(
            validate_output_for_request(
                "animation.character_view",
                &character_request,
                partial_null
            )
            .is_err()
        );
    }

    #[test]
    fn animation_observation_binds_exact_request_and_runtime_evidence() {
        let request = json!({"sessionId":"session","characterId":"/Game/Char.Char","animationBlueprintId":"/Game/ABP.ABP","stateMachineId":"machine"});
        let result = json!({
            "sessionId":"session","characterId":"/Game/Char.Char","meshComponentId":"mesh-component",
            "skeletalMeshId":"/Game/Mesh.Mesh","skeletonId":"/Game/Skeleton.Skeleton",
            "animationBlueprintId":"/Game/ABP.ABP","animClass":"/Game/ABP.ABP_C",
            "animationInstanceId":"animation-instance","stateMachineId":"machine",
            "speed":0.0,"ownerPlanarSpeed":0.0,
            "stateWeights":[{"stateId":"idle-state","name":"idle","weight":1.0},{"stateId":"moving-state","name":"moving","weight":0.0}],
            "activeStateId":"idle-state","activeStateName":"idle",
            "activeTransition":{"active":false,"transitionId":null,"fromStateId":null,"toStateId":null,"elapsedFraction":null},
            "revision":"0".repeat(64)
        });
        assert!(
            validate_output_for_request("play.animation_observe", &request, result.clone()).is_ok()
        );
        let mut wrong_session = result.clone();
        wrong_session["sessionId"] = json!("wrong");
        assert!(
            validate_output_for_request("play.animation_observe", &request, wrong_session).is_err()
        );
        let mut wrong_active = result.clone();
        wrong_active["activeStateId"] = json!("moving-state");
        assert!(
            validate_output_for_request("play.animation_observe", &request, wrong_active).is_err()
        );
        let mut inconsistent_transition = result;
        inconsistent_transition["activeTransition"]["transitionId"] = json!("idle-moving");
        assert!(
            validate_output_for_request(
                "play.animation_observe",
                &request,
                inconsistent_transition
            )
            .is_err()
        );
    }
}
