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
- `p1.6-manifest.json`: kickoff traceability manifest pinning exact catalog, historical manifest hashes, 38-test/10-warning baseline, 45 P1 operations, bounded regular-file/directory archive policy, and five representative agent jobs (orientation plus four construction loops); certification remains incomplete.
- `verify-p1.6-kickoff.sh`: fail-closed no-artifact manifest/catalog/partition/certified-cell verifier. Artifact mode requires externally trusted `P16_EXPECTED_ARTIFACT_SHA256`, then checks exact adjacent checksum, bounded type-safe archive inventory, codesign before execution, embedded catalog identity, byte-identical packaged sources, and retained evidence. It does not claim combined certification.

- `evaluate-p1.6-agents.sh`: prepare, record, finalize, and portable-revalidate five sequential representative-agent jobs. Prepare requires trusted artifact/source/combined-tree identities and clean source bytes matching commit inventory. Finalization seals immutable inputs and full run inventory. Revalidation requires externally trusted run-inventory SHA plus downloaded combined evidence, snapshots inventory-bound run bytes before semantic checks, and verifies combined evidence tree/provenance without Git. Script never runs agents or Unreal. Each construction transcript must include exactly nine selected durable `operation view` readbacks: domain creation, compile, asset save, level save/open, post-restart no-op, second `play.stop` after final editor stop for offline journal recovery, cook, and package; each readback must exactly equal source receipt or process summary.
- `support/p16-agent-lifecycle-fixtures.rb`: hermetic prepare-shaped five-job lifecycle fixture; self-test drives public record/finalize/relocate/revalidate and tamper negatives without Unreal, agents, network, or release tooling.
- `support/p16-evidence-path.rb` and `support/p16-workflow-contracts.rb`: hermetic exact combined-evidence path handoff tests plus static workflow/action/runner/environment/artifact contract validation.
- `close-p1.6.sh`: externally trusted closure gate joining exact artifact bytes, combined evidence tree/provenance, finalized agent run inventory, clean reviewed source commit/tree/inventory, and canonical independent-review attestation. `write` emits an external no-overwrite closure certificate; `verify` re-derives it. Closure alone does not change repository support claims.
- `p1.6-independent-review.schema.json` and `p1.6-closure.schema.json`: strict portable contracts for evidence-scoped clean review and final external closure certificate.
- `.github/workflows/p16-evidence-assembly.yml`: manually dispatched from exact immutable certification tag with four source run IDs. It accepts only successful same-tag runs at trusted commit, keeps independent review in a distinct run, checks exact live artifact identities, revalidates archive/agent/combined evidence, writes and verifies closure, and uploads only five exact P1.6 artifact names. `p16-release-gate` must permit selected immutable `v*` tags only, require independent approval, prevent self-review, and disallow bypass. Workflow does not run agents, Unreal, packaging, or publication.
- `.github/workflows/p16-certification.yml`: protected trusted self-hosted UE runner; exact manual tag/ref binding; builds, signs, packages, kickoff-verifies, and runs combined certification on one archive. Certification returns its exact evidence path through a private runner-temporary file, revalidates copied bytes, and uploads only release archive and combined evidence.
- `.github/workflows/p16-agent-evidence.yml`: protected trusted runner; accepts trusted certification run ID and finalized path confined under `P16_AGENT_EVIDENCE_ROOT`, materializes exact inventory-bound files, revalidates those upload bytes against protected combined evidence identity, uploads only agent run, and never executes agents.
- `.github/workflows/p16-independent-review.yml`: protected self-hosted review-ingestion runner; accepts canonical review JSON confined under `P16_INDEPENDENT_REVIEW_ROOT`, binds exact source tag/ref/commit, copies and validates expected SHA/reviewer through `p16-closure`, and uploads only independent review. It does not perform or rewrite review.
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
./tests/unreal/verify-p1.6-kickoff.sh
./tests/unreal/verify-p1.6-kickoff.sh --self-test
# Artifact preflight after obtaining digest from trusted build/provenance:
P16_EXPECTED_ARTIFACT_SHA256=<trusted-sha256> ./tests/unreal/verify-p1.6-kickoff.sh target/release/magi-unreal-axi-0.1.0-macos-arm64.tar.gz
```

P1.6 agent harness usage:

```sh
P16_EXPECTED_ARTIFACT_SHA256=<trusted-sha256> P16_EXPECTED_SOURCE_COMMIT=<trusted-commit> P16_EXPECTED_COMBINED_EVIDENCE_TREE_SHA256=<trusted-sha256> ./tests/unreal/evaluate-p1.6-agents.sh prepare target/release/magi-unreal-axi-0.1.0-macos-arm64.tar.gz
./tests/unreal/evaluate-p1.6-agents.sh job-context <run> unknown-project-orientation
./tests/unreal/evaluate-p1.6-agents.sh record <run> <job> SESSION_JSONL
./tests/unreal/evaluate-p1.6-agents.sh finalize <run>
./tests/unreal/close-p1.6.sh --self-test
P16_EXPECTED_RUN_INVENTORY_SHA256=<trusted-sha256> ./tests/unreal/evaluate-p1.6-agents.sh revalidate <downloaded-run> <downloaded-combined-evidence>
P16_EXPECTED_ARTIFACT_SHA256=<trusted-sha256> P16_EXPECTED_SOURCE_COMMIT=<trusted-commit> P16_EXPECTED_RUN_INVENTORY_SHA256=<trusted-sha256> P16_EXPECTED_COMBINED_EVIDENCE_TREE_SHA256=<trusted-sha256> P16_EXPECTED_REVIEW_SHA256=<trusted-sha256> P16_EXPECTED_REVIEWER_ID=<trusted-reviewer> ./tests/unreal/close-p1.6.sh verify <artifact> <downloaded-run> <downloaded-combined-evidence> <independent-review.json> <closure.json>
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
UE 5.8.1/macOS arm64 is certified through P1.5; P1.6 kickoff is active but certification is incomplete, P1 remains active, and support remains certified through P1.5 only.
