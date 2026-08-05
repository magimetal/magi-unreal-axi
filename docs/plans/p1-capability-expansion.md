# P1 capability expansion plan

Status: not-started
Created: 2026-08-04
Parent: [`../../PLAN.md`](../../PLAN.md#p1--post-v1-blueprint-authoring-and-reusable-gameplay-construction)
Certified cell: Unreal Engine 5.8.1, changelist `56057345`, macOS arm64

## 1. Objective

Expand V1 into fixture-proven Blueprint gameplay authoring without introducing arbitrary Unreal reflection, raw graph mutation, Python, shell, console, or remote execution.

P1 ships complete workflows, not a capability-count target:

1. Create and author ordinary Blueprints.
2. Construct reusable interaction gameplay with Blueprint-owned components, interfaces, events, delegates/timers where required, collision, and basic physics.
3. Construct and observe runtime UI state.
4. Construct and observe basic AI navigation behavior.
5. Construct and observe basic Animation Blueprint state.

Every new catalogued capability is bounded, discoverable, and certified on the supported engine cell. Reads return stable identity/revision data where applicable. Mutations execute on the game thread, require truthful receipts and operation-specific readback, and prove explicit save/restart persistence when they author durable state. Lifecycle operations retain their separate bridge contract.

## 2. Baseline

P1 starts from V1 commit `891b65e` and these verified contracts:

- 34 canonical capability records generated into Rust and C++ validators.
- Authenticated project/editor-specific bridge with bounded game-thread dispatch.
- Mutation revisions, idempotency, conflict handling, receipts, operation recovery, explicit save, and restart persistence.
- `blueprint.view` and `blueprint.compile`, including structured compile diagnostics.
- Actor, instance-component, level, asset, PIE input, structured observation, screenshot, build, test, cook, and package operations.
- Exact-artifact live certification for UE 5.8.1/macOS arm64.

Current gaps that must be addressed before graph authoring expands:

- Native mutation classification, receipt metadata, and readback routing are hard-coded separately from catalog metadata.
- `blueprint.view` reports summary data, not deterministic graph/node/pin structure.
- Blueprint graph, node, pin, and SCS component identities are not public contracts.
- Normal K2, SCS, UMG, Behavior Tree, Blackboard, navigation, and animation use different Unreal data models and must not share an unsafe generic node API.
- Structured runtime observation does not yet expose UI, collision/physics, AI, Blackboard, Behavior Tree, or animation state.

## 3. Scope rules

1. Add operation only when current phase fixture calls it.
2. Freeze schema and identity semantics before implementing handler.
3. New authoring and semantic-node adapters use explicit class/function/node allowlists; they never offer arbitrary `UK2Node`, raw reflection, or unrestricted `UFunction` execution. Existing class-path operations retain their V1 compatibility contract, including verified project-owned generated actor classes.
4. Use Unreal schema APIs for connection validation and pin defaults; never mutate `LinkedTo` or raw default fields directly.
5. Keep ordinary K2, SCS, WidgetTree, Behavior Tree, Blackboard, navigation, and AnimGraph adapters domain-specific.
6. Require expected revision for every authoring mutation after initial asset creation.
7. Validate complete intent before mutation. If handler fails after mutation begins, undo and verify pre-operation revision before returning failure.
8. Keep Blueprint compilation explicit. P1.0 must replace `blueprint.compile`'s V1 `atomic` transaction metadata with an explicit compile-failure contract before dirty-state receipts ship. Compile failure preserves invalid dirty authoring state, returns an additive failed-mutation receipt containing observed revision, changed objects, dirty packages, and diagnostics, and never claims atomic rollback or persistence.
9. No phase broadens supported engine/platform claims.
10. No phase is complete using screenshots or logs as sole behavioral evidence.

## 4. Phase map

| Phase | Status | Primary result | Fixture |
|---|---|---|---|
| P1.0 | not-started | Catalog-driven native safety and stable authoring identities | V1 regression fixture |
| P1.1 | not-started | Ordinary Blueprint creation and bounded K2 graph authoring | `interaction-loop` foundation |
| P1.2 | not-started | Reusable interaction, SCS components, collision, and physics | `interaction-loop` complete |
| P1.3 | not-started | Widget-driven runtime state | `ui-state-loop` |
| P1.4 | not-started | NavMesh, AI Controller, Blackboard, and Behavior Tree | `ai-navigation-loop` |
| P1.5 | not-started | Basic Animation Blueprint state machine | `animation-state-loop` |
| P1.6 | not-started | Combined P1 agent evaluation and exact-artifact certification | all P1 fixtures |

Phases execute in order. P1.1–P1.5 are independently release-capable once their promotion gate passes. P1 remains active until P1.6 passes.

## 5. Universal capability promotion gate

A new capability enters public catalog only when all applicable checks pass:

1. Closed, bounded input/output schema validates and generated Rust/C++ artifacts match catalog.
2. Offline search and describe expose exact operation and known limits.
3. Live availability reports supported, unavailable with reason, or offline unknown; it never assumes required modules are loaded.
4. Native handler executes on game thread through shared safety path.
5. Success, safe no-op, invalid input, stale revision, conflicting intent, unsupported type, and unsafe editor state are tested.
6. Mutation receipt reports truthful destructive classification, idempotency, transaction, reversibility, persistence, target, changed state, revision, and operation-specific readback.
7. Handler failure leaves pre-operation revision unchanged unless operation contract explicitly preserves dirty state.
8. Explicit save and editor restart preserve exact authored structure and revision.
9. Compile and relevant `CompileAllBlueprints` checks pass.
10. Relevant structured PIE observation proves behavior.
11. Cook/package passes for Blueprint-only fixture when runtime assets are introduced.
12. Prior V1/P1 regressions remain green.
13. Dedicated evidence identifies engine version, changelist, host architecture, catalog hash, artifact hash, limits, and token scan result.

## 6. P1.0 — Contract and runtime hardening

### Goal

Make catalog authoritative for native safety before adding mutation operations.

### Work

- Generate native runtime metadata for operation ID, mutation and destructive flags, idempotency, allowed editor states, transaction behavior, reversibility, save behavior, required modules, and verification readback.
- Consume every generated safety field at runtime: mutation, destructive-force/dry-run and editor-state gating, idempotency/retry policy, required-module availability, receipt metadata, save behavior, ambiguity ledger participation, and readback routing. Startup fails closed on catalog/runtime disagreement.
- Add field-by-field parity tests proving every catalogued operation receives its declared mutation/destructive gating, idempotency/retry policy, receipt, ledger, persistence, and readback behavior; retain the explicit registry for non-catalogued bridge lifecycle operations.
- Define live capability availability and missing-module/plugin reasons. Offline discovery reports availability as unknown when editor state is unavailable.
- Define public identities:
  - graph: Blueprint object path + persisted graph GUID + graph kind;
  - node: persisted node GUID + optional agent-owned natural key;
  - pin: node identity + direction + semantic/internal pin name;
  - SCS component: Blueprint path + `VariableGuid`, separate from actor-instance component IDs.
- Define deterministic ordering, pagination, cursor invalidation, and revision input for graph reads.
- Split only new shared metadata/safety and Blueprint handler boundaries from current native monolith; transport and established domain handlers remain unchanged.
- Generalize release/live scripts to use current Cargo/plugin version instead of fixed `0.1.0` archive assumptions.
- Define a non-atomic compile-failure transaction value and semantics, migrate `blueprint.compile` from its V1 `atomic` metadata, then add the matching additive failed-mutation receipt contract: native receipt creation, Rust validation, structured error rendering, ambiguity-journal recovery through `operation view`, and token-safe retained evidence. Keep protocol version unchanged only if the existing envelope can carry the metadata and receipt compatibly; otherwise version protocol explicitly.

### Acceptance

- Every existing catalogued mutation consumes generated safety metadata without unintended external behavior changes.
- Every generated safety field, including `destructive` and `idempotency`, has runtime parity coverage; catalog/runtime disagreement fails build or startup closed.
- No catalogued capability lacks handler/output validation, and no mutation lacks declared receipt/readback routing. `blueprint.compile` metadata and failed receipts truthfully report preserved dirty state as non-atomic. Non-catalogued lifecycle operations remain explicitly registered and tested.
- Existing 31 unit and 42 integration tests pass unchanged or with contract-preserving updates.
- Existing full Unreal automation passes 15/15.
- M8 exact-artifact lifecycle remains green after version generalization.
- Invalid dirty Blueprint compile test proves catalog metadata, error receipt/recovery, observed revision and dirty packages, and absence of false atomic rollback/persistence claims.

### Non-goals

- No new authoring capability.
- No gratuitous bridge protocol version change; additive failed-receipt design must be proven compatible or versioned deliberately.
- No broad native rewrite or unrelated domain extraction.

## 7. P1.1 — Blueprint construction kernel

### Goal

Create, inspect, author, compile, save, restart, and run one ordinary Actor Blueprint through bounded K2 operations.

### Candidate capabilities

Names freeze during phase contract review; remove any operation fixture does not require.

- `blueprint.create`
- `blueprint.graph_view`
- `blueprint.variable_ensure`
- `blueprint.function_ensure`
- `blueprint.event_ensure`
- `blueprint.node_ensure`
- `blueprint.pin_default_set`
- `blueprint.pin_connect`

Existing `blueprint.compile`, `asset.save`, `actor.spawn`, and `play.*` complete lifecycle.

### Work

- Allow Blueprint creation only for explicit supported parent classes needed by fixture.
- Return bounded deterministic graph, node, pin, default, and link data.
- Model Blueprint-compatible primitive/object types explicitly; reject unknown or incompatible types before mutation.
- Use natural keys for repeatable event/node intent and persisted GUIDs for readback.
- Start node allowlist with exact event and function-call intents required by fixture.
- Use `UEdGraphSchema_K2` validation/connection/default APIs.
- Extend compile diagnostics and additive failed-mutation receipts while preserving structured stdout and exit-code semantics; validate native error payload, Rust rendering, journal recovery, and token redaction.

### Fixture

`interaction-loop` foundation:

1. Create Actor Blueprint.
2. Add interaction event or event override.
3. Add one allowlisted state-changing call.
4. Set required literal defaults and connect execution/data pins.
5. Read graph and capture revision.
6. Compile and save.
7. Restart editor and prove graph/revision persistence.
8. Spawn generated class, run PIE input, and observe exact tag or transform change.
9. Repeat authoring commands and prove safe no-op.

### Acceptance

- Stable graph/node/pin identities survive compile, save, reload, and restart.
- Stale revision and conflicting natural-key intent return `conflict` without mutation.
- Invalid graph, node kind, function, pin, type, default, or connection preserves pre-operation revision.
- Authored Blueprint compiles with zero errors and runs in PIE.
- `CompileAllBlueprints`, project build, and Blueprint-only cook/package pass.

### Non-goals

- Raw graph import/export or arbitrary node class/function calls.
- Macros, Blueprint function libraries, rename/delete, widgets, AI, or animation.

## 8. P1.2 — Reusable interaction gameplay

### Goal

Complete `interaction-loop` with Blueprint-owned components and semantic runtime proof.

### Candidate surface

- Blueprint SCS component ensure/update/remove operations using `VariableGuid`.
- Blueprint Interface asset creation and implementation operations.
- Event dispatcher/delegate and timer graph intents only if fixture requires them.
- Safe primitive-component collision and physics fields through bounded component operations.
- Structured component/collision/physics observation under PIE.

Final operation names and count remain fixture-driven.

### Fixture

Construct reusable interactable gameplay entirely through CLI:

- visual/scene and collision components;
- interface-driven interaction;
- one delegate or delayed timer behavior when required by design;
- overlap or hit behavior;
- bounded basic physics state;
- deterministic reset across two PIE sessions.

### Acceptance

- SCS identities and attachment hierarchy persist across compile/restart.
- Interface implementation and graph calls read back before compile.
- Removal requires established dry-run/force contract and proves absence after restart.
- Structured observation reports collision event/state, component transform, and required physics values; tolerances are explicit.
- Interaction fires exact expected count and resets on second PIE run.
- Compile, save/restart, build, cook, and package pass.

### Non-goals

- Arbitrary UObject property setting, custom collision-channel project edits, Chaos destruction, vehicles, cloth, or replication.

## 9. P1.3 — Widget-driven UI state

### Goal

Create and verify runtime UI state without screenshot parsing.

### Candidate capabilities

- `widget.create`
- `widget.tree_view`
- `widget.child_ensure`
- `widget.property_set`
- `widget.event_ensure`
- `play.ui_observe`

Add binding operation only if fixture cannot use explicit event/state propagation.

### Work

- Add narrowly required UMG/editor modules and prove plugin package/load.
- Treat `UWidgetBlueprint` and `WidgetTree` as dedicated models.
- Allowlist widget classes and safe hierarchy/layout/text/visibility/enabled fields.
- Reuse K2 adapter only for compatible Widget Blueprint event graphs.

### Fixture

`ui-state-loop` creates a Widget Blueprint, deterministic hierarchy, state text/visibility behavior, viewport construction, and one input-driven state transition.

### Acceptance

- Tree and event graph read back deterministically.
- Invalid parent, duplicate conflicting name, unsupported class/property, or cycle fails without partial mutation.
- `play.ui_observe` reports widget class/instance, selected semantic name, text, visibility, enabled state, and relevant bound value.
- Structured observation and screenshot agree.
- Save/restart, two PIE runs, cook, and package pass.

### Non-goals

- Arbitrary Slate, Editor Utility Widgets, localization pipeline, complex styling, UI animation authoring, or pixel-coordinate automation.

## 10. P1.4 — AI navigation loop

### Goal

Create and observe one deterministic NavMesh-backed Behavior Tree loop.

### Candidate capabilities

- `navigation.build`
- `navigation.status`
- `navigation.path_query`
- `blackboard.create`
- `blackboard.key_ensure`
- `blackboard.view`
- `behavior_tree.create`
- `behavior_tree.view`
- `behavior_tree.node_ensure`
- `behavior_tree.connect`
- `ai.controller_configure`
- `play.ai_observe`

### Work

- Add explicit bounded status/wait or operation-ticket semantics for asynchronous navigation build.
- Keep Blackboard and Behavior Tree adapters separate from K2 graph adapter.
- Allowlist key types and semantic tree nodes: selector, sequence, move-to, wait, and fixture-required decorators.
- Add narrowly required AI/navigation modules and prove package/load.

### Fixture

`ai-navigation-loop` creates navigation bounds/configuration, AI Controller Blueprint, typed Blackboard, bounded Behavior Tree, controlled pawn, and target.

### Acceptance

- Navigation build reaches terminal success; path query reaches target before PIE behavior test.
- Blackboard keys and Behavior Tree node/link identities survive restart.
- Structured observation reports controller, pawn, move status, destination, selected Blackboard values, and active/completed behavior.
- Pawn reaches target within explicit distance/time tolerance and resets deterministically.
- Unsupported task/decorator/key types fail without dirtying assets.
- Build, cook, and package pass.

### Non-goals

- EQS, perception, Smart Objects, Mass AI, StateTree, crowd simulation, custom BT class generation, or performance claims.

## 11. P1.5 — Basic Animation Blueprint state

### Goal

Create and verify one idle/moving Animation Blueprint state machine.

### Entry gate

Repository must own or generate legally redistributable minimal Skeleton, SkeletalMesh, and animation sequences. Provenance is recorded. If this gate cannot pass, P1.5 becomes `blocked` and P1 remains `active` unless parent scope is formally revised.

Hash-pinned, provenance-recorded seed Skeleton, SkeletalMesh, and animation sequences are permitted certified fixture inputs. Public P1 operations must author the Animation Blueprint and state-machine outputs; P1 does not promise mesh/animation import or generation.

### Candidate capabilities

- `animation_blueprint.create`
- `animation.graph_view`
- `animation.variable_ensure`
- `animation.state_machine_ensure`
- `animation.state_ensure`
- `animation.transition_ensure`
- `play.animation_observe`

### Work

- Use animation-specific graph/schema APIs; do not route AnimGraph mutations through generic K2 node creation.
- Bind exact Skeleton identity and reject incompatible assets.
- Add only required animation modules after installed-engine header/API validation.

### Fixture

`animation-state-loop` uses controlled movement to drive idle → moving → idle with one speed variable and bounded state transitions.

### Acceptance

- Exact Skeleton compatibility is validated before mutation.
- State machine/state/transition identities read back and persist across restart.
- Animation Blueprint compiles without warnings/errors.
- `play.animation_observe` reports mesh, AnimBP class, speed, active state, and transition status.
- Runtime movement enters moving state and arrival/stop returns idle.
- Cook/package contains all referenced owned assets.

### Non-goals

- Montages, Blend Spaces, IK, motion matching, Control Rig, retargeting, imports, Sequencer, or advanced layered graphs.

## 12. P1.6 — Combined certification and closure

### Work

- Run all P1 native and live fixtures against one release archive and checksum.
- Run representative agent jobs from unknown-project orientation through each P1 construction loop.
- Record command count, avoidable retries, structured-output failures, token estimate, persistence evidence, and runtime outcome.
- Reconcile capability catalog, generated docs, README, skill/guidance, engine support, verification, changelog, and parent plan.
- Independent review covers correctness, safety metadata, failure atomicity claims, capability/fixture traceability, retained evidence, and unsupported domains.

### Acceptance

- All P1 fixtures begin from clean disposable projects. Public CLI operations author every P1 output; hash-pinned, provenance-recorded animation seed assets are the sole permitted pre-authored domain input.
- Repeated runs are idempotent or return documented deterministic conflicts.
- Combined fixture assets survive editor restart and Blueprint-only packaging.
- Exact archive passes codesign, inventory, clean install, setup, plugin lifecycle, full automation, P1 fixtures, uninstall, and token scan.
- No capability exists without fixture traceability and no fixture depends on undocumented escape hatches.
- `PLAN.md` P1 status changes to `done` only after independent review is clean.

## 13. Per-phase execution sequence

For each phase:

1. Freeze fixture behavior and capability-to-fixture matrix.
2. Freeze schemas, identities, idempotency, failure semantics, and readback.
3. Generate Rust/C++ artifacts and add contract tests.
4. Implement native adapter through shared safety pipeline.
5. Add hermetic real-binary success/failure tests.
6. Add native Unreal automation tests.
7. Run live fixture: author → inspect → compile → save → restart → PIE observe → repeat → build/package.
8. Update docs/guidance and exact-artifact gate.
9. Run full verification and independent review.
10. Mark phase `done`; do not begin dependent phase with unresolved correctness findings.

## 14. Verification baseline

Every phase runs applicable full repository gates:

```sh
cargo fmt --check
cargo test --locked
cargo +1.88.0 test --locked
cargo clippy --all-targets --all-features --locked -- -D warnings
cargo +1.88.0 check --all-targets --locked
cargo run --locked --bin xtask -- capabilities check
cargo run --locked --bin xtask -- guidance check
cargo run --locked --bin xtask -- release check
cargo deny check
cargo audit
cargo package --locked --allow-dirty
bash -n tests/unreal/*.sh
```

Native/live gates also verify packaged plugin build/load, source fixture build, `CompileAllBlueprints`, full Unreal automation, phase fixture evidence, process cleanup, retained-evidence token absence, and exact release artifact identity.

## 15. P1-wide non-goals

- Additional Unreal versions, changelists, host architectures, or platforms.
- Remote bridge access, multi-editor fan-out, parallel mutations, or unrelated bridge protocol redesign. A protocol increment is permitted only if P1.0 proves failed-mutation receipts cannot be represented compatibly in v1.
- Raw reflection/property/node escape hatches, Python, shell, console, source generation, or text graph import.
- Niagara, materials, audio, Sequencer, PCG, landscape, foliage, GAS, StateTree, networking, advanced animation, or ecosystem/plugin automation.
- Developer ID signing/notarization.
- Capability parity with prior MCP projects.

## 16. Completion rule

P1 is done only when P1.0–P1.6 are `done`, all advertised operations pass universal promotion gate, all four fixtures pass from clean state through restart and relevant runtime observation, exact-artifact certification is green, and independent review has no unresolved correctness or security findings.
