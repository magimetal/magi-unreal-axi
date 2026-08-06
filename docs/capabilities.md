# Capability catalog

Canonical source: `capabilities/catalog.json`. `cargo run --locked --bin xtask -- capabilities check` validates strict records, sorted unique IDs, closed bounded schemas, mutation safety metadata, receipt-compatible operation metadata, and generated Rust/C++ parity. Current catalog contains 40 records with SHA-256 `6213c83a5ad2a61336ec08bd4bfebb9564e434f7f12a9bf2b9bc951f0fc14922`. Runtime handshake rejects mismatch before operation transmission.

## M6 surface

- assets: `asset.create_input_action`, `asset.create_input_mapping_context`, `asset.save`
- Blueprints: `blueprint.view`, `blueprint.compile`
- components: `component.list`, `component.view`, `component.add`, `component.update`, `component.remove`
- level settings: `level.settings`, `level.set_game_mode`
- play: `play.start`, `play.status`, `play.observe`, `play.input`, `play.screenshot`, `play.stop`

M6 added no Blueprint authoring; bounded authoring enters through P1.1 below.

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
