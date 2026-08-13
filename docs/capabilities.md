# Capability catalog

Canonical source: `capabilities/catalog.json`. `cargo run --locked --bin xtask -- capabilities check` validates strict records, sorted unique IDs, closed bounded schemas, mutation safety metadata, receipt-compatible operation metadata, and generated Rust/C++ parity. Current catalog contains 79 records with full-file SHA-256 `a1f1906449ba158584f4b07f0adc0cccb4dba27df12f371e04aadb88452aae8f`. Runtime handshake rejects mismatch before operation transmission.

## M6 surface

- assets: `asset.create_input_action`, `asset.create_input_mapping_context`, `asset.save`
- Blueprints: `blueprint.view`, `blueprint.compile`
- components: `component.list`, `component.view`, `component.add`, `component.update`, `component.remove`
- level settings: `level.settings`, `level.set_game_mode`
- play: `play.start`, `play.status`, `play.observe`, `play.input`, `play.screenshot`, `play.stop`

M6 added no Blueprint authoring; bounded authoring enters through P1.1–P1.5 below.

## Contracts

Typed and generic commands share one generated input validator and bridge request path. Generated native registry is sole public dispatch allowlist; generated operation-specific C++ validators check handler output before serialization, and Rust checks it again after transport.

Identities remain exact: assets and Blueprints use canonical object paths; levels use long package paths; actors use actor-owning level package plus persistent `ActorGuid`; components use actor identity plus persisted component name. Mutations require canonical 64-hex revisions where conflict protection applies. Receipts bind operation ID, project ID, editor PID, operation, exact target, changed state, transaction, reversibility, dirty/saved packages, persistence, revision, and operation-specific verification.

Persistence is explicit. Asset creation, component and GameMode changes, and Blueprint compile dirtiness are not durable until corresponding asset/level save. M6 live evidence proves canonical revisions survive save/restart/reopen and destructive component removal remains absent after save/restart.

`play.input` accepts input on game thread, then defers response to next tick and compares `play.observe` before/after revisions. Receipt success requires accepted input and matching observation readback. `play.stop` calls PIE shutdown synchronously and returns success only after `play.status` verifies stopped. `play.screenshot` confines `.png` output under `Saved/MagiUnrealAXI/Screenshots`, rejects symlink/path escape, validates PNG signature/nonblank pixels, and reports dimensions constrained to 1–16384 without emitting image bytes.

`blueprint.view` reports parent/generated classes, status, graph count, diagnostics, and revision. `blueprint.compile` is revision-checked, stable on repeated valid compile, and returns bounded structured error/warning diagnostics with graph, node GUID, and node title when available.

## P1.0 public contracts

Discovery availability is tri-state: local capabilities are `available` offline with empty reasons; native capabilities are `unknown` with `editor_offline` without a live matching editor; live native entries are `available` with empty structured reasons or `unavailable` with structured reasons. Execute only `available` capabilities.

Failed `blueprint.compile` exits 1 as `blueprint_compile_failed` and returns a validated failed, non-atomic receipt. `savedPackages` is empty; before/observed revisions and status, `changedObjects`, and diagnostics describe preserved state. Invalid dirty authoring remains dirty with exact `dirtyPackages`; an already-invalid clean asset may truthfully report unchanged persistence. Receipt makes no rollback or saved-persistence claim. Inspect `operation view` before retry; journal fallback supports offline recovery. No automatic retry.

Public identity contracts: graph identity is Blueprint object path + persisted graph GUID + graph kind; node identity is persisted node GUID plus durable agent-owned natural key metadata; pin identity is node identity + direction + percent-encoded semantic/internal pin name; SCS component identity is Blueprint path + `VariableGuid`, separate from actor-instance component identity. Invalid persisted GUIDs fail closed. Blueprint content revisions hash canonical graph identity order, node identity/class/title/comment/owner/enabled state/position, pin identities/types/defaults/links, variables/interfaces, generated StaticMesh mobility, and SCS identity/class/attachment/relative transform. Graph reads order by identity; snapshot revisions bind canonical rows, and opaque cursors bind operation, scope, and snapshot so changed content returns `stale_cursor`.

## P1.1 bounded Blueprint authoring

P1.1 exposes exactly `blueprint.create`, `blueprint.graph_view`, `blueprint.event_ensure`, `blueprint.node_ensure`, `blueprint.pin_default_set`, and `blueprint.pin_connect`. Parent, event, function, typed default, and connection allowlists are closed. Mutations after create require `expectedRevision`; natural-key ownership survives restart; no-ops preserve revision; invalid or conflicting intent preserves pre-operation content and dirty/status state. Receipts bind exact request semantics and operation-specific graph/node/pin readback. Integrated native/live certification passes 19/19 Unreal automation plus compile, build, cook/package, restart, invalid/no-op, PIE, source-provenance, and token gates.

## P1.2 reusable interaction gameplay

P1.2 adds exactly `blueprint.interface_create`, `blueprint.interface_view`, `blueprint.interface_ensure`, `blueprint.scs_view`, `blueprint.scs_component_ensure`, `blueprint.scs_component_update`, `blueprint.scs_component_remove`, and `play.component_observe`. Certified fixture proves one interface, two Actor Blueprints, nested SCS hierarchy, collision/overlap interaction, component observation, deterministic reset across two PIE sessions, and Blueprint-only CompileAllBlueprints/cook/package output. Integrated native/live certification passes exact 22/22 Unreal automation with registry/generated-class, IoStore, inventory, receipt, and token gates.

## P1.3 widget-driven UI state

P1.3 adds exactly `widget.create`, `widget.tree_view`, `widget.child_ensure`, `widget.property_set`, `widget.event_ensure`, `widget.viewport_ensure`, and `play.ui_observe`. Certified fixture proves fixed VerticalBox/TextBlock authoring, restart persistence and no-ops, `READY` → `ACTIVE`, two-PIE reset, stale-session rejection, confined content-hashed screenshots, and Blueprint-only CompileAllBlueprints/cook/package output. Integrated native/live certification passes exact 27/27 Unreal automation with the P1.2 22-test regression plus five P1.3 tests.

## P1.4 AI navigation loop

P1.4 adds exactly `navigation.bounds_ensure`, `navigation.build`, `navigation.status`, `navigation.path_query`, `blackboard.create`, `blackboard.key_ensure`, `blackboard.view`, `behavior_tree.create`, `behavior_tree.node_ensure`, `behavior_tree.connect`, `behavior_tree.view`, `ai.controller_configure`, `ai.pawn_configure`, `play.ai_target_set`, and `play.ai_observe`. Certified fixture proves geometry-backed reachable/nonpartial paths, saved and restart-stable authored identities, AI possession, target arrival within 50 units/120 seconds, authored Wait, refreshed-target MoveTo progress, and clean second-session runtime reset. Integrated certification passes exact 33/33 Unreal automation plus universal plugin and Blueprint-only `.app`/IoStore/Asset Registry/inventory/receipt/token gates.

## P1.5 Animation Blueprint state

P1.5 adds exactly nine operations: `animation_blueprint.create`, `animation.character_configure`, `animation.character_view`, `animation.graph_view`, `animation.variable_ensure`, `animation.state_machine_ensure`, `animation.state_ensure`, `animation.transition_ensure`, and `play.animation_observe`. The seven original animation operations use animation-specific graph/schema APIs; the two character operations bind and inspect Character mesh/AnimBP defaults. The fixture uses repository-owned seed GLB SHA-256 `e5127ab92df4d7414e8a78191513b9b6ad1cde3ef699fb2a2a7f04e035f3f286`, validates one exact Skeleton across SkeletalMesh and both sequences, and exposes deterministic graph, variable, state-machine, state, transition, and node identities. Certification proves atomic failure rollback, stale-revision rejection, revision-preserving no-ops, compile/save/restart identity, real PIE idle → moving → idle, verified receipts, universal plugin output, Blueprint-only `.app`/IoStore/Asset Registry owned content, source integrity, and token absence. Full automation passes 38/38; all five P1.5 tests pass with zero warnings, while regressions retain 10 known pre-existing warnings.

P1.5 is complete. P1.6 kickoff is active but certification is incomplete; P1 remains active and support remains certified through P1.5 only. Kickoff traceability partitions all 79 operations exactly once and identifies the 45 P1 operations exercised by the four construction loops.

## Examples

```sh
magi-unreal-axi capability search play
magi-unreal-axi capability describe play.input
magi-unreal-axi capability execute asset.create_input_action --input-json '{"path":"/Game/Input/IA_Move","valueType":"Axis1D"}'
magi-unreal-axi component list --actor-id '<level-package>#<ActorGuid>'
magi-unreal-axi level settings --level-id /Game/Maps/Main
magi-unreal-axi blueprint view /Game/Blueprints/BP_Game.BP_Game
magi-unreal-axi play observe --session-id <session-id>
```
