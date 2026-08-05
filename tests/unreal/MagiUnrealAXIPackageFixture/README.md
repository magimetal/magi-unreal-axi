# M7 package fixture

Blueprint-only disposable project for installed-engine cook/package certification. It intentionally has no `Modules`, `Source`, or editor plugin dependency. `certify-m7-live.sh` copies it to cache workspace, verifies cook output copied from `Saved/Cooked/Mac`, and verifies packaged macOS `.app` output.
