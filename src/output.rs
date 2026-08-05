use crate::{cli::Format, error::AppError};
use serde_json::{Value, json};

pub const SEMANTIC_STRING_LIMIT: usize = 1_000;

pub fn render(mut value: Value, format: Format, full: bool) -> Result<String, AppError> {
    if !full && truncate_recursive(&mut value) {
        append_help(
            &mut value,
            "Re-run with --full to retrieve complete semantic strings",
        );
    }
    let rendered = match format {
        Format::Json => serde_json::to_string(&value).map_err(|error| error.to_string()),
        Format::Toon => toon_format::encode_default(&value).map_err(|error| error.to_string()),
    };
    rendered.map_err(|error| {
        AppError::operational(
            "output",
            "serialization_failed",
            format!("cannot serialize structured output: {error}"),
            "magi-unreal-axi --format json",
        )
    })
}

pub fn truncate_recursive(value: &mut Value) -> bool {
    match value {
        Value::String(text) => {
            let count = text.chars().count();
            if count <= SEMANTIC_STRING_LIMIT {
                false
            } else {
                let preview: String = text.chars().take(SEMANTIC_STRING_LIMIT).collect();
                *text = format!("{preview}… [truncated; original scalar count: {count}]");
                true
            }
        }
        Value::Array(values) => {
            let mut changed = false;
            for item in values {
                changed |= truncate_recursive(item);
            }
            changed
        }
        Value::Object(values) => {
            let mut changed = false;
            for (key, item) in values {
                if matches!(key.as_str(), "inputSchema" | "outputSchema") {
                    continue;
                }
                changed |= truncate_recursive(item);
            }
            changed
        }
        _ => false,
    }
}

fn append_help(value: &mut Value, suggestion: &str) {
    let Value::Object(object) = value else { return };
    match object.get_mut("help") {
        Some(Value::Array(items)) => items.push(json!(suggestion)),
        Some(_) => {}
        None => {
            object.insert("help".into(), json!([suggestion]));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn truncates_unicode_recursively_and_reports_original_size() {
        let mut value = json!({"nested": [{"text": "🦀".repeat(1001)}]});
        assert!(truncate_recursive(&mut value));
        let text = value["nested"][0]["text"].as_str().unwrap();
        assert_eq!(text.matches('🦀').count(), 1000);
        assert!(text.ends_with("[truncated; original scalar count: 1001]"));
    }
    #[test]
    fn canonical_schema_strings_are_never_truncated() {
        let schema = "x".repeat(4_096);
        let mut value = json!({"outputSchema": schema});
        assert!(!truncate_recursive(&mut value));
        assert_eq!(value["outputSchema"].as_str().unwrap().len(), 4_096);
    }
}
