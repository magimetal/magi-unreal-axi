# Contract evidence ledger

Certified cell: UE 5.8.1, changelist `56057345`, macOS arm64. Support certification is complete through P1.5.

## Confirmed

- Rust 2024 CLI emits TOON by default, strict JSON on request, structured exit 0/1/2 errors, recursive Unicode truncation, and bounded child output.
- Plugin source is embedded in release binary. Setup and bridge lifecycle remain explicit, hash-managed, authenticated, loopback-only, project/session-bound, serial, and game-thread-dispatched.
- Canonical catalog contains 79 records. Current full-file SHA-256 is `a1f1906449ba158584f4b07f0adc0cccb4dba27df12f371e04aadb88452aae8f`; generation and every handshake enforce Rust/C++ parity.
- Catalog validation covers closed bounded schemas plus mutation idempotency, save behavior, allowed editor states, transaction behavior, reversibility, and required modules. Mutation receipts bind exact project/editor/operation/target identity and verify declared safety/persistence metadata with operation-specific readback.
- P1.0 made generated catalog metadata authoritative for native mutation classification, preflight, receipt/readback routing, save-policy enforcement, tri-state capability availability, and startup fail-closed validation. P1.1–P1.5 implement and certify bounded Blueprint, SCS, interaction, widget, AI navigation, Blackboard, Behavior Tree, Character animation binding, Animation Blueprint authoring, and runtime observation operations.
- Failed Blueprint compile receipts are non-atomic and recoverable through live or offline `operation view`. Dirty invalid assets report changed objects, revisions, diagnostics, dirty packages, and no saved packages; already-invalid clean assets may truthfully report unchanged persistence.
- M3 authenticates exact token/project/session/process/version, bounds frames/queue/deadlines, protects runtime files, removes discovery state, and does not disclose token.
- M4 reads use stable canonical Unreal identities, deterministic projection-bound pagination, canonical revisions, and stale-cursor rejection.
- M5 proves natural-key actor idempotency, conflict revisions, explicit save, force/dry-run deletion, structured receipts, restart persistence, receipt recovery, and concurrent serialization/busy behavior.
- M6 exposes exactly `asset.create_input_action`, `asset.create_input_mapping_context`, `asset.save`, `blueprint.view`, `blueprint.compile`, `component.list`, `component.view`, `component.add`, `component.update`, `component.remove`, `level.settings`, `level.set_game_mode`, `play.start`, `play.status`, `play.observe`, `play.input`, `play.screenshot`, and `play.stop`.
- M6 fixture proves input asset creation/save, persistent component and GameMode mutation, canonical revision stability across restart, destructive component removal persistence, valid Blueprint compile/repeat/save/restart, and invalid Blueprint graph/node diagnostics. P1.1 authoring passes its separate integrated gate.
- `play.input` response waits for next-tick `play.observe` readback and records accepted/before/after/changed evidence. PIE observation reports world/session/actor state. Stop is synchronous and verified stopped before success.
- Screenshot output is confined to project screenshot directory, rejects path/symlink escape, validates rendered PNG signature/pixels, reports bounded dimensions, and never emits image bytes.
- M7 release CLI proves exact side-effect-free pipeline previews, UBT pass/repeat normalization, deterministic automation listing, authoritative pass/zero-match reports, cook-only materialization from canonical `Saved/Cooked/Mac`, Blueprint-only packaged `.app` output, transactional managed destination replacement, structured build/cook/package failures, bounded logs, and durable local operation readback.
- M8 proves preserving/idempotent Claude/Codex/OpenCode setup, compact Claude SessionStart context, representative agent jobs, checksum-bound allowlisted archive installation, ad-hoc codesign verification, its then-current full 27/27 automation inventory, save/restart persistence, and retained-evidence token non-disclosure. C++ source fixture is build/editor proof; Blueprint-only fixture is certified cook/package target.
- Agent setup uses selected `HOME` only for Claude settings/skill, Codex skill, and OpenCode skill installation. M8 clean-install certification isolates that configuration HOME, then restores actual macOS account HOME for Unreal lifecycle because Unreal resolves user settings/runtime paths through macOS account APIs.
- Current Rust baseline is Rust 1.88 with 55 library tests, 6 xtask tests, and 44 real-binary integration tests = 105 tests.

## Evidence

- M0: `~/Library/Caches/magi-unreal-axi/m0/evidence.YHvFl0`
- M3 native: `~/Library/Caches/magi-unreal-axi/m3/native/evidence.ukTM4g`
- M3 live: `~/Library/Caches/magi-unreal-axi/m3/live/evidence.wHfLa2`
- M4 native: `~/Library/Caches/magi-unreal-axi/m4/native/evidence.o1lLDJ`
- M4 live: `~/Library/Caches/magi-unreal-axi/m4/live/evidence.T6rRB8`
- M5 native: `~/Library/Caches/magi-unreal-axi/m5/native/evidence.wVNcWF`
- M5 live: `~/Library/Caches/magi-unreal-axi/m5/live/evidence.2qcbtW`
- M6 native: `~/Library/Caches/magi-unreal-axi/m6/native/evidence.T25zpG`
- M6 live: `~/Library/Caches/magi-unreal-axi/m6/live/evidence.m9J5dO`
- M7 live: `~/Library/Caches/magi-unreal-axi/m7/live/evidence.Ekd7HQ`
- M8 hardened exact-artifact clean install: `~/Library/Caches/magi-unreal-axi/m8/live/evidence.8v8UFv`
- M8 agent evaluation: `~/Library/Caches/magi-unreal-axi/m8/agent-evaluation/run.Gy9dcQ/evidence`
- Post-fix 15/15 automation: `~/Library/Caches/magi-unreal-axi/read-fixture-fix.KGV2FR/evidence`
- P1.0 native: `~/Library/Caches/magi-unreal-axi/p1.0/native/evidence.zLZ1or`
- P1.0 live: `~/Library/Caches/magi-unreal-axi/p1.0/live/evidence.FRJY3A`
- P1.1 integrated native/live: `~/Library/Caches/magi-unreal-axi/p1.1/native/evidence.CUX9X7`
- P1.2 integrated native/live: `~/Library/Caches/magi-unreal-axi/p1.2/native/evidence.iY67AH` (outer gate evidence; live certification wrapped by native gate).
- P1.3 integrated native/live: `~/Library/Caches/magi-unreal-axi/p1.3/native/evidence.1kyPg7` (27/27 automation, UI state loop, cook/package, IoStore, and Asset Registry).
- P1.4 integrated native/live: `~/Library/Caches/magi-unreal-axi/p1.4/native/evidence.to16qz` (33/33 automation, AI navigation loop/reset, universal plugin, `.app`, IoStore, and Asset Registry).
- P1.5 integrated native/live: `~/Library/Caches/magi-unreal-axi/p1.5/native/latest` (all nine certified operation cells, 79-record catalog hash `a1f1906449ba158584f4b07f0adc0cccb4dba27df12f371e04aadb88452aae8f`, owned seed, exact Skeleton, compile/restart/no-op, real PIE idle-moving-idle, 38/38 automation with five zero-warning P1.5 tests and an exact 10-warning prior-regression baseline, universal plugin, Blueprint-only `.app`/IoStore/Asset Registry, receipts, source integrity, and token gates).

## Remaining

- M6–M8 have no acceptance unknowns for certified cell. Historical evaluated artifact hash remains distinct from release checksum in `SHA256SUMS`.
- P1 remains active. P1.0–P1.5 are complete; P1.6 has not started.

## N/A

- MCP, Node.js, Python, HTTP, WebSocket, JSON-RPC, arbitrary shell/Python/console escape hatches: prohibited and absent.
- Remote bridge configuration: intentionally unavailable.
- Arbitrary Blueprint graph mutation remains unavailable; P1.1 and P1.2 expose only closed, allowlisted authoring and observation operations.

## Reproduction

```sh
cargo fmt --check
cargo test --all-targets --all-features --locked
cargo clippy --all-targets --all-features --locked -- -D warnings
cargo build --release --locked
cargo run --locked --bin xtask -- capabilities check
cargo run --locked --bin xtask -- guidance check
cargo run --locked --bin xtask -- release check
./tests/unreal/certify-m0.sh
./tests/unreal/certify-m3.sh
./tests/unreal/certify-m3-live.sh
./tests/unreal/certify-m4.sh
./tests/unreal/certify-m4-live.sh
./tests/unreal/certify-m5.sh
./tests/unreal/certify-m5-live.sh
./tests/unreal/certify-m6.sh
./tests/unreal/certify-m6-live.sh
./tests/unreal/certify-m7-live.sh
./tests/unreal/certify-p1.0.sh
./tests/unreal/certify-p1.0-live.sh
./tests/unreal/certify-p1.1.sh
./tests/unreal/certify-p1.2.sh
./tests/unreal/certify-p1.3.sh
./tests/unreal/certify-p1.4.sh
./tests/unreal/certify-p1.5.sh
./tests/unreal/certify-m8-live.sh target/release/magi-unreal-axi-0.1.0-macos-arm64.tar.gz
```
