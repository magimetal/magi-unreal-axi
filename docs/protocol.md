# Native bridge protocol v1

Certified target: Unreal Engine 5.8.1 on macOS arm64 through P1.3. Transport is TCP bound only to `127.0.0.1` on an OS-selected port. No remote-host override exists.

M8 adds agent integration, documentation, packaging, and release gates only. It does not expand protocol v1, catalog operations, framing, authentication, or runtime semantics.

## Framing and bounds

Each message is one `u32` little-endian byte length followed by that many UTF-8 JSON bytes.

- request: 1 byte through 8 MiB
- response: 1 byte through 16 MiB
- handshake deadline: 2 seconds
- operation deadline: 30 seconds
- game-thread queue: 64 requests
- active editor operations: serial
- active connections: 72

Zero, oversized, incomplete, non-UTF-8, or malformed JSON frames fail closed. Partial transfers share one absolute deadline.

## Discovery and authentication

Plugin creates current-user `0700` directories and atomically writes `0600` regular files:

```text
~/Library/Caches/magi-unreal-axi/<project-sha256>/<pid>/bridge-v1.json
~/Library/Caches/magi-unreal-axi/<project-sha256>/<pid>/token
```

Discovery identifies protocol/plugin version, PID plus kernel process-start identity, canonical project path/ID, engine version, loopback host/port, session nonce, and start time. CLI validates owner, type, symlink status, permissions, project/session identity, and process identity before authentication. First frame includes exact protocol, token, canonical project, PID/process start, nonce, and CLI version. Plugin compares token in constant time; 64 failures within 10 seconds temporarily close handshake admission.

Successful handshake repeats runtime identity and generated `catalogHash`. Current 55-record hash is `7cd513c54122e73b4c0b5faaf8f3669f89819584822e10602017e9f41f19e05b`. CLI rejects identity or hash mismatch before operation dispatch. Token never enters discovery, responses, errors, logs, receipts, or retained evidence.

## Operations

Bootstrap operations: `bridge.health`, `bridge.describe`, `editor.stop`.

Catalogued operations through P1.3:

- M4 reads: `editor.status`, `level.current`, `level.list`, `actor.list`, `actor.view`, `asset.list`, `asset.view`
- M5 mutations/lookup: `level.create`, `level.open`, `level.save`, `actor.spawn`, `actor.update_transform`, `actor.delete`, `operation.view`
- M6 assets: `asset.create_input_action`, `asset.create_input_mapping_context`, `asset.save`
- M6 Blueprints: `blueprint.view`, `blueprint.compile`
- M6 components: `component.list`, `component.view`, `component.add`, `component.update`, `component.remove`
- M6 settings: `level.settings`, `level.set_game_mode`
- M6 play: `play.start`, `play.status`, `play.observe`, `play.input`, `play.screenshot`, `play.stop`
- P1.1 Blueprint authoring: `blueprint.create`, `blueprint.graph_view`, `blueprint.event_ensure`, `blueprint.node_ensure`, `blueprint.pin_default_set`, `blueprint.pin_connect`
- P1.2 interaction: `blueprint.interface_create`, `blueprint.interface_view`, `blueprint.interface_ensure`, `blueprint.scs_view`, `blueprint.scs_component_ensure`, `blueprint.scs_component_update`, `blueprint.scs_component_remove`, `play.component_observe`
- P1.3 widget/UI: `widget.create`, `widget.tree_view`, `widget.child_ensure`, `widget.property_set`, `widget.event_ensure`, `widget.viewport_ensure`, `play.ui_observe`

Request envelope:

```json
{"protocol":1,"id":"opaque-client-id","operation":"component.update","args":{"id":"<actor-id>#Component","location":[40,50,60]},"deadlineMs":30000,"expectedRevision":"64-hex-characters","idempotencyKey":"optional-request-key"}
```

Mutation success includes result plus receipt:

```json
{"protocol":1,"id":"opaque-client-id","status":"ok","result":{"id":"<target>","changed":true,"revision":"..."},"receipt":{"operationId":"opaque-client-id","operation":"component.update","state":"completed","projectId":"sha256:...","editorPid":1234,"target":"<target>","changed":true,"transaction":"atomic","reversibility":"source-control","dirtyPackages":["/Game/Map"],"savedPackages":[],"revision":"...","persistence":"dirty","verification":{"readback":"component.view","target":"<target>","matched":true}}}
```

Receipt construction validates operation-specific identity and catalogued safety metadata: exact project/editor/operation/target, transaction, reversibility, persistence, changed state, package effects, revision, and readback. Any transmitted mutation with unconfirmed response returns `outcome_unknown` plus operation ID; `operation.view` checks live ledger then private bounded journal.

`bridge.describe` is additive in protocol v1. Its result includes `protocol`, `catalogHash`, all public operation names in `operations`, and complete `nativeOperations` entries shaped as `{"operation":"blueprint.compile","availability":"available|unavailable","reasons":[{"code":"...","subject":"...","message":"..."}]}`. Live entries are `available` with empty reasons or `unavailable` with bounded structured reasons such as `missing_module` and `editor_state`. CLI discovery merges this evidence with local catalog data: local capabilities remain `available` offline, while native capabilities become `unknown` with `editor_offline` when no authenticated matching editor exists. Clients execute only available entries.

Failed compile error envelope (64-character revision values abbreviated here):

```json
{"protocol":1,"id":"op-1","status":"error","error":{"type":"blueprint_compile_failed","message":"Blueprint compile failed","retryable":false,"dirtyPackageCount":1,"dirtyPackages":["/Game/BP"],"errorCount":1,"warningCount":0,"diagnostics":[{"severity":"error","message":"invalid graph","graph":"/Game/BP.BP:EventGraph","nodeGuid":"00000000-0000-0000-0000-000000000001","nodeTitle":"Broken"}]},"receipt":{"operationId":"op-1","operation":"blueprint.compile","state":"failed","projectId":"sha256:...","editorPid":1234,"target":"/Game/BP.BP","changed":false,"transaction":"non-atomic","reversibility":"source-control","dirtyPackages":["/Game/BP"],"savedPackages":[],"revision":"<64-hex>","persistence":"dirty","verification":{"readback":"blueprint.view","target":"/Game/BP.BP","matched":true,"beforeRevision":"<64-hex>","observedRevision":"<64-hex>","observedStatus":"error","failureType":"blueprint_compile_failed","errorCount":1,"warningCount":0,"diagnostics":[{"severity":"error","message":"invalid graph","graph":"/Game/BP.BP:EventGraph","nodeGuid":"00000000-0000-0000-0000-000000000001","nodeTitle":"Broken"}],"changedObjects":[]}}}
```

No rollback or saved persistence is claimed. Exit status is 1; inspect `operation.view` before retry, and use bounded journal fallback when editor is offline. `operation.view` is allowed during editing and PIE recovery. P1.0 adds no P1.1 operations.

## M6 runtime semantics

- Editing mutations enforce unsafe-state gates and revision conflicts. Persistence remains explicit through `asset.save` or `level.save`.
- `play.input` captures canonical observation revision before input, accepts input on game thread, and defers completion to next tick. Success includes accepted flag, before/after revisions, truthful changed flag, and matched `play.observe` readback.
- `play.stop` calls `EndPlayMap` synchronously, refuses success while PIE still exists, and verifies `play.status` stopped revision in receipt.
- `play.screenshot` stays under project `Saved/MagiUnrealAXI/Screenshots`, requires `.png`, rejects path/symlink escape, validates file/signature, and reports width/height bounded to 1–16384. Binary bytes never enter protocol output.
- `blueprint.compile` returns structured `blueprint_compile_failed` details with bounded error/warning totals and graph/node context when available. P1.1, P1.2, and P1.3 authoring remains limited to catalogued allowlists.

## Lists, threading, and teardown

Lists accept limit 1–100, optional opaque cursor, and family-specific fields. Results include count, total, scope, ordered items, snapshot revision, and next cursor. Cursor binds operation, scope, projection, and canonical ordered row content; changed state returns `stale_cursor`.

Socket parsing/authentication runs on bounded workers. Unreal API preflight and handlers run only on game thread. Queue-to-dispatch transition is atomic; deadline/disconnect removes only queued work. Shutdown removes delegates/ticker, wakes requests, closes sockets, joins workers, and removes discovery/token.
