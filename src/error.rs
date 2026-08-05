use serde_json::{Value, json};

#[derive(Debug, Default)]
struct ErrorDetails {
    dirty_package_count: Option<u64>,
    dirty_packages: Option<Vec<String>>,
    operation_id: Option<Box<str>>,
    error_count: Option<u64>,
    warning_count: Option<u64>,
    diagnostics: Option<Value>,
    receipt: Option<Value>,
}

#[derive(Debug)]
pub struct AppError {
    pub kind: &'static str,
    pub reason: &'static str,
    pub code: u8,
    pub message: String,
    pub help: String,
    pub retryable: bool,
    details: Option<Box<ErrorDetails>>,
}

impl AppError {
    pub fn operational(
        kind: &'static str,
        reason: &'static str,
        message: impl Into<String>,
        help: impl Into<String>,
    ) -> Self {
        Self {
            kind,
            reason,
            code: 1,
            message: message.into(),
            help: help.into(),
            retryable: false,
            details: None,
        }
    }

    pub fn with_bridge_details(
        mut self,
        retryable: bool,
        dirty_package_count: Option<u64>,
        dirty_packages: Option<Vec<String>>,
    ) -> Self {
        self.retryable = retryable;
        if dirty_package_count.is_some() || dirty_packages.is_some() {
            let details = self
                .details
                .get_or_insert_with(|| Box::new(ErrorDetails::default()));
            details.dirty_package_count = dirty_package_count;
            details.dirty_packages = dirty_packages;
        }
        self
    }

    pub fn with_operation_id(mut self, operation_id: String) -> Self {
        self.details
            .get_or_insert_with(|| Box::new(ErrorDetails::default()))
            .operation_id = Some(operation_id.into_boxed_str());
        self
    }
    pub fn with_bridge_diagnostics(
        mut self,
        error_count: Option<u64>,
        warning_count: Option<u64>,
        diagnostics: Option<Value>,
    ) -> Self {
        if error_count.is_some() || warning_count.is_some() || diagnostics.is_some() {
            let details = self
                .details
                .get_or_insert_with(|| Box::new(ErrorDetails::default()));
            details.error_count = error_count;
            details.warning_count = warning_count;
            details.diagnostics = diagnostics;
        }
        self
    }

    pub fn with_receipt(mut self, receipt: Value) -> Self {
        self.details
            .get_or_insert_with(|| Box::new(ErrorDetails::default()))
            .receipt = Some(receipt);
        self
    }

    pub fn receipt(&self) -> Option<&Value> {
        self.details
            .as_ref()
            .and_then(|details| details.receipt.as_ref())
    }

    pub fn operation_id(&self) -> Option<&str> {
        self.details
            .as_ref()
            .and_then(|details| details.operation_id.as_deref())
    }

    pub fn usage(
        reason: &'static str,
        message: impl Into<String>,
        help: impl Into<String>,
    ) -> Self {
        Self {
            kind: "usage",
            reason,
            code: 2,
            message: message.into(),
            help: help.into(),
            retryable: false,
            details: None,
        }
    }

    pub fn envelope(&self) -> Value {
        let mut error = json!({
            "type": self.kind,
            "reason": self.reason,
            "code": self.code,
            "message": self.message,
            "help": self.help,
            "retryable": self.retryable
        });
        if let Some(details) = &self.details {
            if let Some(count) = details.dirty_package_count {
                error["dirtyPackageCount"] = json!(count);
            }
            if let Some(packages) = &details.dirty_packages {
                error["dirtyPackages"] = json!(packages);
            }
            if let Some(operation_id) = &details.operation_id {
                error["operationId"] = json!(operation_id);
            }
            if let Some(count) = details.error_count {
                error["errorCount"] = json!(count);
            }
            if let Some(count) = details.warning_count {
                error["warningCount"] = json!(count);
            }
            if let Some(diagnostics) = &details.diagnostics {
                error["diagnostics"] = diagnostics.clone();
            }
            if let Some(receipt) = &details.receipt {
                error["receipt"] = receipt.clone();
            }
        }
        json!({"error": error})
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn envelope_preserves_bridge_details() {
        let error = AppError::operational("bridge", "busy", "editor busy", "retry")
            .with_bridge_details(true, Some(3), Some(vec!["A.uasset".into()]))
            .with_operation_id("operation-1".into());
        assert_eq!(error.envelope()["error"]["retryable"], true);
        assert_eq!(error.envelope()["error"]["dirtyPackageCount"], 3);
        assert_eq!(error.envelope()["error"]["dirtyPackages"][0], "A.uasset");
        assert_eq!(error.envelope()["error"]["operationId"], "operation-1");
    }

    #[test]
    fn envelope_preserves_failed_receipt() {
        let receipt = json!({"operationId":"operation-1","state":"failed"});
        let error = AppError::operational(
            "bridge",
            "blueprint_compile_failed",
            "compile failed",
            "inspect",
        )
        .with_receipt(receipt.clone());
        assert_eq!(error.envelope()["error"]["receipt"], receipt);
    }
}
