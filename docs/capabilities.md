# Capability catalog

Canonical source: `capabilities/catalog.json`. `cargo run --locked --bin xtask -- capabilities check` validates strict records, sorted unique IDs, closed bounded schemas, mutation safety metadata, receipt-compatible operation metadata, and generated Rust/C++ parity. Current catalog contains 34 records with SHA-256 `b1888d3416a0873e31b1b600f6c84a7e01cdce982fedc8088ab42e8b76c3b506`. Runtime handshake rejects mismatch before operation transmission.

## M6 surface

- assets: `asset.create_input_action`, `asset.create_input_mapping_context`, `asset.save`
- Blueprints: `blueprint.view`, `blueprint.compile`
- components: `component.list`, `component.view`, `component.add`, `component.update`, `component.remove`
- level settings: `level.settings`, `level.set_game_mode`
- play: `play.start`, `play.status`, `play.observe`, `play.input`, `play.screenshot`, `play.stop`

M6 adds no Blueprint authoring. Creation of Blueprints, graphs, variables, functions, nodes, pins, and connections remains post-V1 P1.

## Contracts

Typed and generic commands share one generated input validator and bridge request path. Generated native registry is sole public dispatch allowlist; generated operation-specific C++ validators check handler output before serialization, and Rust checks it again after transport.

Identities remain exact: assets and Blueprints use canonical object paths; levels use long package paths; actors use actor-owning level package plus persistent `ActorGuid`; components use actor identity plus persisted component name. Mutations require canonical 64-hex revisions where conflict protection applies. Receipts bind operation ID, project ID, editor PID, operation, exact target, changed state, transaction, reversibility, dirty/saved packages, persistence, revision, and operation-specific verification.

Persistence is explicit. Asset creation, component and GameMode changes, and Blueprint compile dirtiness are not durable until corresponding asset/level save. M6 live evidence proves canonical revisions survive save/restart/reopen and destructive component removal remains absent after save/restart.

`play.input` accepts input on game thread, then defers response to next tick and compares `play.observe` before/after revisions. Receipt success requires accepted input and matching observation readback. `play.stop` calls PIE shutdown synchronously and returns success only after `play.status` verifies stopped. `play.screenshot` confines `.png` output under `Saved/MagiUnrealAXI/Screenshots`, rejects symlink/path escape, validates PNG signature/nonblank pixels, and reports dimensions constrained to 1–16384 without emitting image bytes.

`blueprint.view` reports parent/generated classes, status, graph count, diagnostics, and revision. `blueprint.compile` is revision-checked, stable on repeated valid compile, and returns bounded structured error/warning diagnostics with graph, node GUID, and node title when available.

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
