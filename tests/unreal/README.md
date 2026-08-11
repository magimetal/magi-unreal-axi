# Unreal certification fixtures

This directory is text-owned. Certification copies fixtures to canonical cache workspaces before Unreal touches them.

- `certify-m0.sh`: package/build/load and Blueprint compilation baseline.
- `certify-m3.sh`: native adversarial frames, authentication/failure cap, queue/deadline, and response identity.
- `certify-m3-live.sh`: release CLI lifecycle, token non-disclosure, dirty-stop refusal, clean stop, process exit, and teardown.
- `certify-m4.sh`: native catalog/schema/game-thread/stable-revision/stale-cursor contracts.
- `certify-m4-live.sh`: M4 reads, canonical identities, pagination, catalog parity, unchanged inventory, and cleanup.
- `certify-m5.sh`: native mutation, receipt-readback, and unsafe-state contracts.
- `certify-m5-live.sh`: mutation revisions, explicit save/restart persistence, deletion, journal fallback, concurrency, inventory, and token checks.
- `certify-m6.sh`: plugin and source fixture build, Blueprint compilation, three M6 native contracts, and three M5 mutation regressions.
- `certify-m6-live.sh`: 34-record catalog parity; input assets; components; GameMode; Blueprint diagnostics; deferred next-tick input readback; PIE observe/screenshot/synchronous stop/reset; receipt identity/safety metadata; save/restart persistence; fixture project build; inventory/token/cleanup checks.
- `certify-m7-live.sh`: exact pipeline dry-runs, build/repeat, automation list/run, blueprint-only cook/package, destination protection, structured failures, bounded logs, and durable operation readback.
- `p1.0-manifest.json`: certified engine cell, catalog identity, runtime limits, exact generated safety fields, native test inventory, and live assertion inventory.
- `certify-p1.0.sh`: P1.0 plugin/fixture build, `CompileAllBlueprints`, exact 16-test automation inventory, generated metadata/identity/save-policy/lifecycle contracts, artifact/source hashes, and token-file scan.
- `certify-p1.0-live.sh`: P1.0 tri-state availability, dirty failed-compile receipt, exact live/offline recovery, restart preservation, catalog binding, and retained-evidence token scan.
- `p1.1-manifest.json`: P1.1 UE 5.8.1/56057345 native cell, 40-record catalog identity, runtime limits, universal plugin arches, exact 19-test inventory, and Blueprint-only pipeline contract.
- `certify-p1.1.sh`: integrated P1.1 gate covering Rust malformed-output fixtures, release CLI project build, universal plugin package, `CompileAllBlueprints`, exact 19-test inventory with rollback fault injection, Blueprint-only cook/package, artifact/source-tree/CLI hashes, live fixture outputs, runtime limits, and retained-evidence token scan.
- `certify-p1.1-live.sh`: P1.1 create/author/compile/save/restart lifecycle, deterministic graph identities, missing/stale revision and invalid-intent matrix, cross-key ownership conflicts, node/event/default/link no-ops, PIE input transform, and token scans.
- `p1.2-manifest.json`: historical P1.2 UE 5.8.1/56057345 native cell, 48-record catalog identity, runtime limits, universal plugin arches, exact 22-test inventory, and Blueprint-only pipeline contract.
- `certify-p1.2.sh`: historical P1.2 integrated gate covering Rust 44+5+44 gate, universal plugin package, `CompileAllBlueprints`, exact 22-test inventory, live interface/two-Blueprint/nested-SCS/overlap/two-PIE proof, Blueprint-only cook/package, registry/generated classes/IoStore/inventory/receipt/token checks, and retained evidence.
- `certify-p1.2-live.sh`: historical P1.2 interface creation/implementation, SCS identity/hierarchy/update/remove, component observation, interaction overlap, save/restart persistence, deterministic two-PIE reset, and token scans.
- `p1.3-manifest.json`: P1.3 UE 5.8.1/56057345 native cell, 55-record catalog identity, runtime limits, universal plugin arches, exact 27-test inventory, and Blueprint-only pipeline contract.
- `certify-p1.3.sh`: integrated P1.3 gate covering Rust 46+5+44 gate, universal plugin package, `CompileAllBlueprints`, exact 27-test inventory, live UI state/reset proof, Blueprint-only cook/package, IoStore/Asset Registry/inventory/receipt/token checks, and retained evidence.
- `certify-p1.3-live.sh`: P1.3 widget creation/tree/property/event/viewport lifecycle, UI observation, save/restart persistence, deterministic two-PIE reset, stale-session rejection, and token scans.
- `p1.4-manifest.json`: P1.4 UE 5.8.1/56057345 native cell, 70-record catalog identity, universal plugin arches, exact 33-test inventory, explicit arrival tolerance, deterministic runtime-reset assertion, and Blueprint-only pipeline contract.
- `certify-p1.4.sh`: integrated P1.4 gate covering Rust 50+6+44 gate, universal plugin package, `CompileAllBlueprints`, exact 33-test inventory, live AI navigation/reset proof, Blueprint-only `.app`, IoStore/Asset Registry/inventory/receipt/source-integrity/token checks, and retained evidence.
- `certify-p1.4-live.sh`: P1.4 bounded NavMesh, Blackboard, Behavior Tree, Controller/pawn authoring, save/restart/no-op lifecycle, real PIE possession/arrival/authored Wait/refreshed MoveTo, explicit tolerances, deterministic second-session reset, and token scans.
- `p1.5-manifest.json`: P1.5 UE 5.8.1/56057345 native cell, 79-record catalog identity, owned seed identity, universal plugin arches, exact 38-test inventory, exact 10-warning regression baseline, nine certified operation cells, and Blueprint-only pipeline contract.
- `certify-p1.5.sh`: integrated P1.5 gate covering Rust 55+6+44 gate, universal plugin package, `CompileAllBlueprints`, exact 38-test inventory with five zero-warning P1.5 tests and exact 10-warning regression map, live animation/reset proof, Blueprint-only `.app`, IoStore/Asset Registry/inventory/receipt/source-integrity/token checks, and retained evidence.
- `certify-p1.5-live.sh`: P1.5 typed Character/Animation Blueprint authoring, exact Skeleton binding, save/restart/no-op lifecycle, verified receipts, and real PIE idle → moving → idle proof.
- `certify-m8-live.sh`: verifies checksum binding, allowlisted archive inventory/path safety, arm64 binary and ad-hoc codesign, clean extraction/install, isolated-HOME agent setup/idempotency/context, plugin lifecycle, project build, full 27-test MagiUnrealAXI automation, editor read/mutation/save/restart persistence, uninstall, and retained-evidence token scan.

Run current gates from repository root:

```sh
./tests/unreal/certify-m6.sh
./tests/unreal/certify-m6-live.sh
./tests/unreal/certify-m7-live.sh
./tests/unreal/certify-p1.0.sh
./tests/unreal/certify-p1.0-live.sh
./tests/unreal/certify-m8-live.sh target/release/magi-unreal-axi-0.1.0-macos-arm64.tar.gz
./tests/unreal/certify-p1.1.sh
./tests/unreal/certify-p1.1-live.sh
./tests/unreal/certify-p1.2.sh
./tests/unreal/certify-p1.2-live.sh
./tests/unreal/certify-p1.3.sh
./tests/unreal/certify-p1.3-live.sh
./tests/unreal/certify-p1.4.sh
./tests/unreal/certify-p1.4-live.sh
./tests/unreal/certify-p1.5.sh
```

Latest M6 evidence: `~/Library/Caches/magi-unreal-axi/m6/native/evidence.T25zpG` and `~/Library/Caches/magi-unreal-axi/m6/live/evidence.m9J5dO`.

Latest M7 evidence: `~/Library/Caches/magi-unreal-axi/m7/live/evidence.Ekd7HQ`.
P1.0 evidence: native pass at `~/Library/Caches/magi-unreal-axi/p1.0/native/evidence.zLZ1or` and live pass at `~/Library/Caches/magi-unreal-axi/p1.0/live/evidence.FRJY3A`.
P1.1 integrated native/live evidence is retained under `~/Library/Caches/magi-unreal-axi/p1.1/{native,live}/`; each successful gate writes its latest evidence path to `latest`.
- P1.2 integrated evidence: historical `~/Library/Caches/magi-unreal-axi/p1.2/native/evidence.iY67AH`.
- P1.3 integrated evidence: latest successful path is recorded in `~/Library/Caches/magi-unreal-axi/p1.3/native/latest`.
- P1.4 integrated evidence: latest successful path is recorded in `~/Library/Caches/magi-unreal-axi/p1.4/native/latest`.
- P1.5 integrated evidence: latest successful path is recorded in `~/Library/Caches/magi-unreal-axi/p1.5/native/latest`.

M8 exact-artifact evidence: latest successful path is recorded in `~/Library/Caches/magi-unreal-axi/m8/live/latest`; agent evaluation remains at `~/Library/Caches/magi-unreal-axi/m8/agent-evaluation/run.Gy9dcQ/evidence`.
UE 5.8.1/macOS arm64 is certified through P1.5; P1.5 is complete, P1.6 has not started, P1 remains active, and M8 release and installation baseline remains certified.
