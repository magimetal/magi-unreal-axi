# Verification status

Certified cell: Unreal Engine 5.8.1, changelist `56057345`, macOS arm64. M7 independent review: CLEAN. M8 hardened exact-artifact gate: PASS.

## Successful gates

```sh
cargo fmt --check
cargo test --locked
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
```

Rust final: 31 unit plus 42 real-binary integration tests = 73 tests (`cargo test --locked`, verified locally). Catalog check validates 34 records, generated static Rust/C++ schemas and registries, mutation safety metadata, receipt identity fields, and hash `b1888d3416a0873e31b1b600f6c84a7e01cdce982fedc8088ab42e8b76c3b506`.

## Evidence paths

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
- M8 hardened exact-archive clean install: `~/Library/Caches/magi-unreal-axi/m8/live/latest`
- M8 agent evaluation: `~/Library/Caches/magi-unreal-axi/m8/agent-evaluation/run.Gy9dcQ/evidence`
- Post-fix full automation: `~/Library/Caches/magi-unreal-axi/read-fixture-fix.KGV2FR/evidence`

## M3–M5 regression status

- M3 native/live: adversarial framing, exact authentication, failure cap, queue/deadline behavior, lifecycle ownership, dirty-stop refusal, token non-disclosure, process exit, and discovery cleanup pass.
- M4 native/live: two native catalog/pagination tests pass; all typed reads plus generic execution preserve canonical IDs/revisions, bounded pagination, hash parity, read-only inventory, and clean lifecycle.
- M5 native/live: three mutation tests pass; level/actor mutation, unsafe-state matrix, idempotency, conflict handling, explicit save, two restarts, exact deletion, receipt journal, ambiguity recovery, and concurrency pass.

## M6 native certification

`certify-m6.sh` packages plugin, builds source-backed fixture, compiles all Blueprints, and runs six native tests: three M6 component/world-settings/play-receipt/Blueprint-diagnostic contracts plus three M5 mutation regressions. Result: 6/6, zero failures.

Evidence: `~/Library/Caches/magi-unreal-axi/m6/native/evidence.T25zpG`.

## M6 live certification

Release CLI plus packaged plugin completed full fixture loop:

1. Created two input actions and mapping context; repeated mapping was no-op; explicit asset saves persisted all three assets.
2. Added component, verified it through typed and generic component lists, updated it, assigned GameMode with expected revision, explicitly saved level, restarted, and matched canonical level/actor/component/settings revisions and values.
3. Enforced component remove force/dry-run/readback, saved, restarted, and proved absence.
4. Compiled valid Blueprint, repeated as stable no-op, saved/restarted with same revision, and returned structured invalid-Blueprint diagnostic containing graph, node GUID, and node title.
5. Started PIE, observed structured world/session/actors, sent deterministic W/E input, and completed each input only after next-tick observation readback. Movement changed exactly 100 units; interaction changed exact tag/location once.
6. Verified receipts and `operation.view` for play start/input/screenshot/stop with exact project/editor/operation/target, transaction, reversibility, persistence, and matched readback.
7. Wrote rendered PNG under bounded screenshot root, reported positive dimensions within 1–16384, verified PNG signature and nonblank pixels, and emitted no binary image data.
8. Stopped PIE synchronously with stopped-state receipt, started second session, proved deterministic reset, then stopped cleanly.
9. Built fixture project through `project build` with exit 0, preserved unrelated sentinel, retained exact inventory, removed discovery/token, and found no token leak.

Evidence: `~/Library/Caches/magi-unreal-axi/m6/live/evidence.m9J5dO`.

M6 acceptance is complete. C++ source fixture remains build/editor proof only; M7 certifies pipeline normalization and Blueprint-only cook/package target.

## M7 live certification

Release CLI completed exact side-effect-free dry-runs; source fixture build and repeat; deterministic three-test and empty automation lists; authoritative three-test pass and zero-match failure; blueprint-only cook materialization; packaged macOS `.app`; unmanaged overwrite refusal; structured live build/cook/package failures; bounded project logs; and durable operation readback.

Cook copies a bounded, symlink-safe inventory from canonical project `Saved/Cooked/Mac` only after successful UAT. Package writes to sibling staging, verifies artifacts, adds CLI management marker, then commits destination. Hermetic tests prove failed cook cannot export stale project artifacts, unsafe cooked trees cannot partially materialize, failed package preserves existing managed output, and postprocessing failure keeps child process status truthful.

Evidence: `~/Library/Caches/magi-unreal-axi/m7/live/evidence.Ekd7HQ`.

M7 acceptance is complete. UE 5.8.1/macOS arm64 support is certified through M8.

## M8 certification

`~/Library/Caches/magi-unreal-axi/m8/live/latest` points to retained hardened exact-artifact evidence. Gate binds archive to adjacent `SHA256SUMS`, allowlists complete inventory and rejects links/path traversal, verifies macOS arm64 and ad-hoc codesign, proves isolated-HOME agent setup, installs matching managed plugin, runs project build plus full 15/15 `MagiUnrealAXI` automation, completes editor health/read/mutation/save/restart persistence, safely uninstalls plugin, records artifact/source/workflow identity, and proves retained evidence excludes captured bridge token.

After concrete `UInputAction` fixture correction, source fixture built and full MagiUnrealAXI automation passed 15/15 with zero warnings/errors at `~/Library/Caches/magi-unreal-axi/read-fixture-fix.KGV2FR/evidence`. This is build/editor automation proof, not source game-target package proof.

Representative agent evaluation jobs 1–8 are complete at `~/Library/Caches/magi-unreal-axi/m8/agent-evaluation/run.Gy9dcQ/evidence`; full findings and token estimates are in `docs/agent-evaluation.md`. Jobs 2–7 succeeded for intent, job 1 was partial orientation, job 8 retained pre-fix 14/15 automation plus C++ game-target arm64 link failure/no package, and distinct Blueprint-only job 8b packaged 37 files totaling 1,389,331,062 bytes.

Release workflow builds and packages once, passes exact downloaded archive to approved self-hosted UE 5.8.1/macOS arm64 certification, then publishes with checksum and GitHub provenance. Binary uses ad-hoc codesign, not Developer ID signing or notarization.

## M8 completion

Representative jobs, full Rust/release/security gates, post-fix automation, and hardened exact-artifact live certification pass. Release checksum remains authoritative in adjacent `SHA256SUMS`; ad-hoc signing is not Developer ID signing or notarization.
