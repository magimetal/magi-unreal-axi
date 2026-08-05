# Changelog

## Unreleased

- Added Rust 2024 AXI kernel, offline home, project and UE engine discovery, and explicit plugin setup foundation.
- Expanded generated catalog from 16 to 34 records; current Rust/C++ parity hash is `b1888d3416a0873e31b1b600f6c84a7e01cdce982fedc8088ab42e8b76c3b506`.
- Added game-thread editor, level, actor, asset, component, world-settings, Blueprint, and PIE operations with canonical revisions, bounded schemas, deterministic pagination, and receipt identity/safety validation.
- Added M5 safe mutations: level create/open/save, natural-key actor spawn, revision-checked transform update/delete, explicit force/dry-run guards, postcondition receipts, and operation lookup.
- Completed M6 input asset creation/save, component lifecycle, GameMode assignment, Blueprint inspect/compile diagnostics, and PIE start/status/observe/input/screenshot/stop.
- Deferred `play.input` completion to next-tick observation readback, bounded screenshots to verified PNG artifacts, and made play stop synchronous with stopped-state verification.
- Certified M6 on UE 5.8.1/56057345 arm64 through save/restart persistence, deterministic movement/interaction/reset, operation receipt lookup, rendered screenshot, Blueprint success/failure, and source fixture project build.
- Added and live-certified M7 typed build/test/cook/package pipelines, authoritative automation reports, cook-only artifact materialization, transactional protected package destinations, bounded logs, and durable process summaries on UE 5.8.1/56057345 arm64. Blueprint authoring remains deferred; agent integration and release were assigned to M8.
- Completed M8 agent setup/guidance and release workflow for Claude Code, Codex, and OpenCode; setup is preserving, managed, path-repairing, fail-closed, and idempotent. Certified representative agent evaluation, exact-archive clean installation, ad-hoc codesign/checksum packaging, full 15/15 MagiUnrealAXI automation, save/restart persistence, and token non-disclosure on UE 5.8.1/macOS arm64.
