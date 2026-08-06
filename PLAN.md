# PLAN.md — `magi-unreal-axi`

Status: active
Mode: AXI new build
Repository: `/Users/magimetal/Dev/magi-axi-unreal`
Primary runtime: Rust 2024 CLI + Unreal Engine C++ editor plugin
Binary: `magi-unreal-axi`
Initial host target: macOS arm64 (`arm64` host)
Initial Unreal target: certified Epic Launcher Unreal Engine 5.8.1 installed build

## 1. Decision

**Conditional GO.** Building a self-contained AXI CLI for AI-driven Unreal development is viable.

Viable product shape:

1. Rust AXI CLI owns command parsing, project/engine discovery, output, errors, setup, subprocess execution, bridge communication, agent integrations, and release packaging.
2. Bundled native Unreal editor plugin owns editor-only APIs, game-thread dispatch, asset/world mutations, editor-state validation, persistence checks, and operation receipts.
3. Private authenticated local bridge connects them. It is not MCP and does not expose MCP tools, resources, prompts, sessions, HTTP, SSE, or WebSocket semantics.
4. Repository owns all capability definitions, protocol fixtures, plugin source, tests, fixture projects, docs, and installed Agent Skill.

A native plugin is required for reliable live-editor inspection and mutation. External process invocation alone can build, cook, package, and run commandlets, but cannot safely replace in-process access to `UObject`, editor subsystems, Blueprint graphs, transactions, dirty packages, PIE state, and game-thread-only APIs.

### Meaning of “self-contained”

Required:

- No dependency on `ue-mcp`, `Unreal_mcp`, MCP SDKs, MCP clients, Node.js, or Python.
- No separately installed daemon or server.
- No remotely reachable or separately managed network service; while editor runs, bundled plugin owns one ephemeral loopback-only listener.
- One repository and release owns CLI plus matching plugin payload.
- `magi-unreal-axi setup plugin install` can deploy matching plugin without cloning another repository.
- All public behavior remains usable through non-interactive shell commands.

Still required externally:

- Licensed Unreal Engine installation.
- Platform compiler/toolchain required by selected Unreal build.
- Platform SDKs required for target packaging.
- Target Unreal project.

Not promised:

- One universal precompiled plugin binary across all UE minors, platforms, and custom engine builds.
- Redistribution of Unreal Engine binaries, headers, SDKs, or Epic sample content.

### Verified local engine baseline

Observed on current host:

| Item | Verified value |
|---|---|
| Engine root | `/Users/Shared/Epic Games/UE_5.8` |
| Engine version | `5.8.1` |
| Changelist | `56057345` |
| Compatible changelist / BuildId | `55116800` |
| Branch | `++UE5+Release-5.8` |
| Distribution | Epic Launcher promoted installed build |
| Host | macOS arm64 |
| UnrealEditor | `Engine/Binaries/Mac/UnrealEditor`; universal arm64/x86_64 executable |
| UnrealEditor-Cmd | `Engine/Binaries/Mac/UnrealEditor-Cmd` |
| UnrealBuildTool | `Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool`; arm64 executable |
| RunUAT | `Engine/Build/BatchFiles/RunUAT.sh` |
| Bundled .NET | `Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64` |
| Xcode | `26.6` (`17F113`) |
| Apple clang | `21.0.0` |
| macOS SDK | Xcode `MacOSX.sdk` (resolved installation currently reports 26.5) |

Evidence sources:

- `Engine/Build/Build.version`
- `Engine/Binaries/Mac/UnrealEditor.version`
- executable inspection on current host
- successful UBT `-Help` using engine-bundled `DOTNET_ROOT`
- successful RunUAT `-Help`, which selects same bundled .NET SDK
- `RunUAT BuildPlugin` completed in 9 seconds with `BUILD SUCCESSFUL`
- packaged `libUnrealEditor-MagiAxi.dylib` verified as arm64
- `UnrealEditor-Cmd -run=CompileAllBlueprints` loaded module startup marker and exited 0
- commandlet completed with `0 error(s), 0 warning(s)`

Direct UBT invocation must set:

```sh
DOTNET_ROOT="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
```

System `dotnet` is not on `PATH`; this is not a blocker because engine ships required runtime. Engine resolution must return environment overrides alongside executable/argv, and child runner must not require users to modify shell profile.

Build fixture must use a canonical real path such as `~/Library/Caches/...`, not `/tmp/...` on macOS. UE 5.8.1 UBA writes `/private/tmp` while generated compile paths retain `/tmp`; that alias mismatch caused missing PCH/object files in certification attempt. Same source built immediately at cache path. M2 process runner and test fixtures must select a non-symlink canonical workspace and test this condition explicitly.

Repository-owned commandlet and interactive editor fixtures, automation reports, plugin build/load, authenticated lifecycle, live reads/mutations, restart persistence, M6 game-building loop, M7 build/test/cook/package pipeline, and M8 agent/release integration are verified.

## 2. Product objective

Enable an agent to construct, inspect, run, verify, persist, build, and package an Unreal game through deterministic CLI operations.

First complete loop:

1. Resolve project and engine.
2. Install/verify bundled plugin.
3. Start or connect to correct editor instance.
4. Inspect current editor, level, assets, actors, and dirty state.
5. Create/open a level.
6. Spawn/configure actors and components.
7. Save explicitly.
8. Run PIE.
9. Observe structured runtime state and capture screenshot evidence.
10. Stop PIE, build and package project, restart editor, and prove changes persisted.

Broad action count is not success. A smaller set that completes and verifies this loop is more valuable than hundreds of shallow handlers.

## 3. Sources and implementation policy

### Normative AXI sources

Implementation must re-read current versions before public contract is frozen:

- `$axi` skill and its discovery, Rust architecture, and release matrices.
- AXI specification: <https://axi.md/>
- Canonical AXI skill: <https://github.com/kunchenguid/axi/blob/main/.agents/skills/axi/SKILL.md>
- TOON specification: <https://toonformat.dev/reference/spec.html>

Apply all ten AXI areas: TOON, minimal schemas, truncation, aggregates, empty states, structured errors, ambient context, content-first home, contextual disclosure, and focused help.

### Local AXI references

Use `/Users/magimetal/Dev/magi-axi-linear` for:

- Thin `main.rs` and testable library exit boundary.
- Typed CLI/config/output/error separation.
- Rust 2024, committed lockfile, strict parser checks.
- TOON default plus strict JSON.
- Hermetic real-binary tests.

Use `/Users/magimetal/Dev/magi-axi-nx` for:

- Self-contained replacement of a prior MCP surface.
- Directory-scoped offline home.
- Direct local transport and subprocess trust boundaries.
- Bounded logs/artifacts.
- Explicit, preserving, staged with rollback on write/commit error, idempotent agent setup.
- Project-scoped SessionStart context and installable skill.

Improve on both references:

- One runtime/config resolver only.
- Structured expected errors on stdout with empty stderr.
- 500–1,500 character semantic truncation range; start at 1,000.
- Generate shared guidance and fail CI on stale skill/docs.
- Test actual hook context, not file creation alone.
- Keep `lib.rs` and command modules focused rather than monolithic.

### Unreal reference implementations

Use these as architecture, feature, and risk references only:

- `/Users/magimetal/Dev/_third-party/ue-mcp`
- `/Users/magimetal/Dev/_third-party/Unreal_mcp`

Lessons to retain:

- Editor API work must execute on game thread.
- Project-scoped editor discovery and per-editor identity matter.
- Pending requests, queues, frames, outputs, retries, and timeouts must be bounded.
- Mutations need idempotency, readback, save semantics, and editor-state gates.
- Asset/level save and load require centralized safety wrappers.
- Capability breadth needs progressive search/describe rather than huge default context.
- TypeScript/native drift in prior systems argues for one canonical neutral capability catalog.
- Loopback without authentication is insufficient for an editor-level automation endpoint.

Do not carry forward:

- MCP protocol or SDK types.
- Node server.
- HTTP/SSE/WebSocket server.
- MCP gateway/tool/resource terminology in public contract.
- Arbitrary Python, shell, or unrestricted console execution.
- Hundreds of initial actions.
- Broad plugin dependencies enabled by default.
- Name-only client/native parity checks.

Both reference repositories are MIT licensed. Default implementation policy is original code using official Unreal APIs and independently defined contracts. If a proven handler is later ported, record source commit/path, retain required MIT notices, add it to `THIRD_PARTY_NOTICES.md`, and adapt it behind this project’s safety and capability contracts. No silent copy/paste.

## 4. Scope

### V1 scope

- Project and engine discovery.
- Plugin install, repair, status, update, and safe uninstall.
- Editor start/connect/status/stop with unsaved-state protection.
- Authenticated local bridge.
- Capability search, describe, and execute.
- Asset, actor, level, editor, PIE, screenshot, save, build, test, cook, package, and log foundations.
- One verified game-building fixture.
- TOON/JSON AXI contract.
- Claude Code, Codex, and OpenCode setup where current host APIs support it.
- Installable `skills/magi-unreal-axi/SKILL.md`.
- Hermetic tests plus owned live-Unreal release gate.

V1 gates are M0–M6, M7, and M8. Post-V1 milestone P1 and §11 expansion do not block initial release.

### Explicitly deferred

- Feature parity with either MCP repository.
- Remote/LAN bridge access.
- Arbitrary Python, shell, unrestricted console commands, and raw C++ execution.
- Fab marketplace automation.
- Epic UE 5.8 Toolset Registry wrapping.
- Plugin marketplace/publishing system.
- Niagara, PCG, GAS, StateTree, networking, Sequencer, Control Rig, landscape, and foliage until core loop is reliable.
- Multi-editor mutation fan-out.
- Multi-agent concurrent mutation.
- Universal prebuilt plugin.
- Session-end history until measured continuity benefit exists.

## 5. Architecture

```text
Agent
  |
  | shell invocation; TOON/JSON stdout
  v
magi-unreal-axi (Rust)
  |- AXI parser / output / errors / home / help
  |- project + engine discovery
  |- setup + embedded plugin payload
  |- direct UBT/UAT/UnrealEditor process runner
  |- canonical capability catalog and input validation
  |- authenticated bridge client
  `- operation/result normalization
             |
             | loopback TCP, private framed JSON protocol
             v
MagiUnrealAXI (editor-only C++ plugin)
  |- discovery + authentication
  |- bounded socket/frame handling
  |- versioned operation registry
  |- bounded serial request queue
  |- game-thread dispatcher
  |- editor-state and authorization gates
  |- domain handlers
  |- transaction/save/load safety wrappers
  `- structured verification receipts
             |
             v
Unreal Editor APIs / UBT / UAT / commandlets
```

### Responsibilities

#### Rust CLI owns

- Public command names, flags, focused help, suggestions, and exit codes.
- TOON and JSON output.
- Recursive truncation and pagination.
- Project/config/engine precedence.
- Plugin deployment and version compatibility.
- Editor process lifecycle.
- Direct UBT/UAT/commandlet invocation.
- Bridge discovery, authentication, framing, timeouts, and error translation.
- Capability input validation before transport.
- Agent session setup and static skill generation.
- Durable local summaries of latest operations.

#### Unreal plugin owns

- Re-validation of every operation and target project.
- Socket listener and authenticated connection state.
- Operation availability for current engine/modules/editor state.
- Game-thread execution.
- Serialized mutations.
- `FScopedTransaction`, `Modify()`, edit notifications, dirtying, compile, save, reload, and postcondition logic where applicable.
- Refusal during unsafe editor states.
- Idempotency ledger and operation receipt lookup.
- Sanitized structured results.

Neither side trusts validation performed only by the other.

## 6. Private bridge protocol

### Transport

Use raw TCP bound to `127.0.0.1:0`.

Why:

- Unreal and Rust both support it without external runtime.
- OS-selected port avoids collisions.
- Length framing is simpler than WebSocket and avoids browser-origin attack surface.
- Same design can later support Windows without replacing public CLI semantics.

Frame:

```text
u32 little-endian JSON byte length
UTF-8 JSON payload
```

Initial hard bounds:

- Request frame: 8 MiB.
- Response frame: 16 MiB.
- Queue: 64 requests.
- One active editor operation at a time.
- Default request deadline: 30 seconds unless capability declares a larger bounded deadline.
- Lists paginate before reaching response cap.

### Discovery

Plugin binds first, then atomically writes:

```text
<user-private-runtime>/magi-unreal-axi/<project-hash>/<pid>/bridge-v1.json
<user-private-runtime>/magi-unreal-axi/<project-hash>/<pid>/token
```

Discovery record fields:

```json
{
  "protocol": 1,
  "pluginVersion": "0.1.0",
  "pid": 1234,
  "projectPath": "/canonical/Game.uproject",
  "projectId": "sha256:...",
  "engineVersion": "5.x.y",
  "host": "127.0.0.1",
  "port": 54321,
  "sessionNonce": "...",
  "startedAt": "..."
}
```

Rules:

- Canonical project path must match CLI-selected project.
- PID, process start, protocol, plugin version, and session nonce must be validated.
- Stale records are reported and repairable, never blindly trusted.
- Runtime parent directory is owned by current user and mode `0700`; token is a regular non-symlink file with mode `0600` on POSIX. Windows uses current-user ACL equivalent.
- Plugin rejects symlinked/wrong-owner/insecure runtime paths. CLI repeats ownership, type, and permission checks before reading.
- Token rotates every editor session. Plugin writes token first and discovery record last, both atomically, only after listener is bound.
- Discovery record and token are removed on clean shutdown; stale session directories remain diagnosable and bounded cleanup removes only verified stale entries.
- Handshake has short deadline, unauthenticated connection cap, failed-attempt cap, and immediate close on failure.
- Token is never emitted in stdout, stderr, logs, crash context, hooks, receipts, or argv.
- Threat model excludes arbitrary code already running as same OS user; token and permissions protect other users, accidental clients, stale clients, and non-browser local access paths.

### Handshake

First client frame authenticates token and binds connection to:

- protocol version;
- canonical project ID;
- editor PID/session nonce;
- CLI version;
- requested deadline.

Token comparison must be constant-time. No operation dispatch before successful handshake.

### Request envelope

```json
{
  "protocol": 1,
  "id": "01J...",
  "operation": "actor.spawn",
  "args": {},
  "deadlineMs": 30000,
  "idempotencyKey": "optional",
  "expectedRevision": "optional"
}
```

### Response envelope

```json
{
  "protocol": 1,
  "id": "01J...",
  "status": "ok",
  "result": {},
  "receipt": {
    "operationId": "01J...",
    "changed": true,
    "dirtyPackages": [],
    "savedPackages": [],
    "revision": "...",
    "verification": {}
  }
}
```

Error fields:

```json
{
  "status": "error",
  "error": {
    "type": "invalid_input|unsupported|busy|wrong_project|unsafe_editor_state|not_found|conflict|operation_failed|timeout",
    "message": "...",
    "retryable": false
  }
}
```

This is a project-specific wire contract, not JSON-RPC and not MCP.

### Concurrency and cancellation

- Serialize reads and mutations initially. Correctness beats throughput.
- Socket thread parses, authenticates, bounds, and enqueues only.
- Plugin tick drains queue and invokes handlers on game thread.
- Socket I/O never blocks game thread.
- Queue overflow returns `busy`.
- Connection loss cancels queued requests and suppresses late responses.
- Already-dispatched UE work is not claimed cancelled unless handler supports true cancellation.
- Long operations eventually use operation tickets; do not invent generic cancellation in first slice.
- CLI never auto-retries non-idempotent mutations.
- Operation states are `queued`, `running`, `completed`, `failed`, or `outcome_unknown`.
- Deadline may remove queued work. Once dispatched, timeout never claims cancellation; CLI returns `outcome_unknown` with operation ID.
- Plugin atomically records bounded receipt/idempotency state before sending mutation response. Initial bound: 1,024 records per project/editor session with 24-hour TTL.
- `operation view <id>` checks live plugin then bounded user-private receipt journal. After ambiguous outcome, caller must inspect receipt or stable target readback before retrying.

## 7. Canonical capability model

Keep neutral capability records under `capabilities/` as source of truth. Do not derive contract from C++ registration text or hand-maintain separate Rust/C++ schemas.

Minimum record:

```json
{
  "id": "actor.spawn",
  "version": 1,
  "domain": "actor",
  "summary": "Spawn or return an actor identified by an agent key in current level",
  "mutates": true,
  "destructive": false,
  "idempotency": "natural-key",
  "saveBehavior": "dirty-only",
  "allowedEditorStates": ["editing"],
  "requiresModules": ["UnrealEd"],
  "inputSchema": {},
  "outputSchema": {},
  "verification": {
    "readback": "actor.view",
    "persistence": "level.save"
  },
  "engineSupport": {
    "certified": [],
    "unsupported": []
  }
}
```

A Rust `xtask` will:

1. Parse and validate strict records.
2. Reject duplicate IDs, unknown fields, invalid schemas, missing safety metadata, and conflicting aliases.
3. Generate Rust catalog/search data.
4. Generate C++ operation metadata/registration declarations.
5. Generate protocol contract fixtures and hashes.
6. Generate capability docs and static Agent Skill guidance.
7. Fail CI when generated artifacts drift.

Handlers remain handwritten. Generation removes contract duplication; it must not generate opaque business logic.

Schema dialect is a documented restricted JSON Schema 2020-12 subset: closed objects, `required`, primitive types, enums, numeric/string bounds, arrays with bounded `items`, and defaults. Unsupported schema keywords fail generation. `xtask` generates equivalent Rust and C++ validators plus shared valid/invalid fixtures; neither runtime embeds a general dynamic schema interpreter.

### Stable identity and revisions

- Assets and levels use canonical Unreal long package/object paths. Rename creates a new identity and invalidates prior cursors/revisions.
- Actors use level package path plus persistent editor `ActorGuid`. Labels are display metadata only and never identify update/delete targets.
- If selected engine/object cannot provide durable `ActorGuid`, destructive/durable actor operations are refused rather than falling back to label.
- Components use actor ID plus persisted component GUID or unique persisted component name, verified at capability implementation time.
- Spawn accepts explicit `agentKey` unique within level as natural idempotency key; success returns resulting `ActorGuid`. Duplicate labels remain allowed and irrelevant.
- Read responses return opaque revision. Update/delete requires `expectedRevision`; stale revision returns `conflict` with exact re-read command.

### Public discovery commands

```sh
magi-unreal-axi capability search "spawn actor"
magi-unreal-axi capability describe actor.spawn
magi-unreal-axi capability execute actor.spawn --input-json '{...}'
magi-unreal-axi capability execute actor.spawn --input-file request.json
magi-unreal-axi capability execute actor.spawn --input-file -
```

`capability execute` accepts only catalogued operations. It is not arbitrary transport access.

Core high-frequency operations also receive ergonomic typed commands that compile into the same internal request:

```sh
magi-unreal-axi actor list --limit 100 --fields id,label,class
magi-unreal-axi actor view <actor-id>
magi-unreal-axi actor spawn --class <path> --key <agent-key> --label <label> --location <x,y,z>
magi-unreal-axi level save --path <package>
magi-unreal-axi play start
```

No duplicate execution path: typed command and generic capability execution both produce one validated `OperationRequest`.

## 8. AXI public contract

### Global flags

```text
--project <path>      .uproject or directory
--engine <path>       engine root override
--editor <pid>        disambiguate multiple matching editors
--format toon|json    default toon
--full                disable semantic string truncation
--timeout <duration>  bounded command timeout override
--verbose             sanitized diagnostics to stderr
```

Globals must work consistently before or after subcommands and be covered by parser tests.

### Resolution precedence

Project selector:

1. `--project`
2. `MAGI_UNREAL_PROJECT`
3. nearest ancestor containing exactly one `.uproject`

All other configurable values:

1. CLI flag
2. `MAGI_UNREAL_*` environment value
3. `<Project>/.magi/unreal-axi.toml`
4. platform global config, such as `~/.config/magi-unreal-axi/config.toml`
5. documented default

Engine resolution after project selection:

1. explicit resolved engine setting from precedence above;
2. source engine containing project;
3. `.uproject` `EngineAssociation` through platform registration;
4. unambiguous conventional installation match.

Rules:

- Empty explicit/env values are errors, not absence.
- Multiple projects or engines are never resolved by “first match.”
- Malformed config reports file and key and is never silently replaced.
- Config writes preserve unrelated keys and use atomic replacement.
- Bridge token is runtime secret state, never config.
- `setup agents` with no target flags installs all currently supported targets; explicit flags select only requested targets.

### No-argument home

Home never starts Unreal, builds code, installs plugin, or mutates project.

It may perform bounded filesystem discovery and a short authenticated health probe when a valid matching discovery record exists.

Home fields:

```json
{
  "bin": "~/.local/bin/magi-unreal-axi",
  "description": "Build, inspect, run, and safely automate the current Unreal project",
  "project": {
    "found": true,
    "name": "Game",
    "path": "/.../Game.uproject"
  },
  "engine": {
    "resolved": true,
    "version": "5.x.y"
  },
  "plugin": {
    "installed": true,
    "compatible": true
  },
  "editor": {
    "state": "ready|stopped|stale|incompatible",
    "pid": 1234,
    "level": "/Game/Maps/Main",
    "pie": "stopped",
    "dirtyPackages": 0
  },
  "latest": {
    "operation": "actor.spawn",
    "status": "ok"
  },
  "help": []
}
```

Home behavior:

- Outside project: exit 0 with `project.found: false`, cwd scope, and exact `--project` suggestion.
- Engine unresolved: exit 0 with useful offline project state and exact `--engine` correction.
- Editor stopped: exit 0 and suggest `editor start` only when project/plugin state supports it.
- Malformed descriptor/config: structured exit 1.
- Session context target: measured maximum 400 tokens.
- Latest operation is one bounded atomic summary, not unbounded history.

### Command families

Foundation:

```text
project view|doctor|build|test|cook|package
engine list|view
setup plugin install|status|update|repair|uninstall
setup agents [--claude] [--codex] [--opencode]
editor start|status|stop|restart
capability search|describe|execute
operation view <id>
log latest|search
```

Game-building core:

```text
asset list|view|save
level list|view|create|open|save
actor list|view|spawn|update|delete
component list|view|add|update|remove
blueprint view|compile
play start|status|observe|input|screenshot|stop
```

Post-V1 Blueprint authoring commands (`create|variable|function|node|connect`) ship under P1 only after core release gates pass.

Later families are added only with complete fixture evidence.

### Output

- Default stdout: one valid TOON document.
- `--format json`: one strict compact JSON document.
- Expected success and expected structured errors leave stderr empty.
- `--verbose`: sanitized diagnostics on stderr only.
- No progress, ANSI, child logs, tables, or decoration on stdout.
- Lists default to 3–4 decision fields.
- Each list includes `count`, `total`, `scope`, and deterministic ordering.
- Empty result states explicit zero and scope, exit 0.
- Long semantic strings truncate recursively at 1,000 Unicode scalar values.
- Truncated value states original size.
- `--full` restores semantic fields.
- Logs, binary-derived data, screenshots, and large reports remain independently bounded even under `--full`.
- `--fields` accepts validated family-specific fields only.
- Default limits are domain-specific: capability 50, actor 100, asset 100, logs 100 lines unless evidence changes them.
- List commands expose `--limit`, opaque `--cursor`, and validated `--fields`. Cursor binds query, sort, scope, and revision; changed state returns explicit `stale_cursor` requiring fresh list.

### Errors and exit codes

Stable AXI envelope:

```json
{
  "error": {
    "type": "usage|project|engine|setup|bridge|editor_state|capability|operation|process|security|output",
    "code": 1,
    "message": "What failed, where, and why",
    "help": "magi-unreal-axi <specific corrective command>",
    "retryable": false
  }
}
```

- Exit `0`: success, explicit empty result, already-satisfied safe no-op.
- Exit `1`: project, engine, setup, bridge, operation, process, security, or output failure.
- Exit `2`: invalid command, flag, argument, selector, or conflict.
- Unknown input is rejected before project discovery, bridge connection, subprocess, or mutation.
- Help/version exit 0 without config, engine, editor, or network requirements.
- Raw Unreal/dependency stack traces never enter stable error fields.
- Corrective help points to this CLI, not UBT/UAT internals.

### Mutation contract

Every mutation declares and returns:

- stable operation ID;
- target project/editor session;
- target object/package;
- changed versus safe no-op;
- editor-state preconditions;
- idempotency class;
- transaction behavior;
- dirty packages;
- saved packages;
- postcondition readback;
- persistence status;
- reversibility: transaction-only, source-control recoverable, or destructive.

Defaults:

- Mutations dirty packages but do not save unless command explicitly includes `--save` or separate save command runs.
- Create/update operations use natural keys where Unreal has stable names/paths.
- Destructive operations require exact stable target plus `--force`.
- `--dry-run` is required for destructive commands and offered for setup/build/package operations where truthful preview is possible.
- Writes are refused during PIE, modal interaction, shutdown, async package load, compile, or other unsafe state unless capability explicitly supports it.
- Editor undo is never described as durable rollback.
- Non-idempotent mutations are never automatically retried.

### Editor ownership and shutdown

- `editor start` records PID, OS process-start identity, canonical project, and whether CLI launched process.
- Start refuses to create second matching editor unless caller explicitly selects/authorizes another instance.
- Editor attached but not launched by this CLI is never terminated by default.
- `editor stop` requests graceful plugin-mediated shutdown.
- Dirty packages refuse shutdown and return exact `level save`/`asset save` or explicit discard command.
- Discarding unsaved state or killing process requires separate destructive flag and confirmation token; plain `--force` is insufficient.
- Stop/restart reports whether editor was owned, attached, gracefully stopped, or force-terminated.

## 9. Repository shape

Start with one Rust application crate. Split only after responsibilities have real size.

```text
magi-axi-unreal/
|- Cargo.toml
|- Cargo.lock
|- PLAN.md
|- README.md
|- CHANGELOG.md
|- LICENSE
|- THIRD_PARTY_NOTICES.md
|- capabilities/                 # canonical neutral records
|- protocol/
|  |- README.md
|  `- fixtures/                  # shared valid/invalid frames
|- plugin/
|  `- MagiUnrealAXI/
|     |- MagiUnrealAXI.uplugin
|     `- Source/MagiUnrealAXI/
|        |- Public/
|        `- Private/
|           |- Bridge/
|           |- Core/
|           |- Compatibility/
|           |- Safety/
|           `- Domains/
|- src/
|  |- main.rs                    # ExitCode delegation only
|  |- lib.rs                     # parse/dispatch/render boundary
|  |- cli.rs                     # canonical clap schema
|  |- app.rs                     # one resolved command context
|  |- config.rs                  # precedence + atomic writes
|  |- error.rs                   # typed AXI errors
|  |- output.rs                  # TOON/JSON + truncation
|  |- project.rs                 # project discovery/descriptors
|  |- engine.rs                  # engine/tool resolution
|  |- process.rs                 # direct bounded child execution
|  |- bridge.rs                  # discovery/auth/framing
|  |- capability.rs              # catalog/search/validation
|  |- setup.rs                   # plugin + agent setup
|  `- commands/
|- src/bin/xtask.rs              # contract generation/checks
|- tests/
|  |- integration.rs             # real binary + fake engine/bridge
|  |- fixtures/
|  `- unreal/                    # text-owned disposable UE fixture
|- skills/magi-unreal-axi/SKILL.md
|- docs/
|  |- contract.md
|  |- protocol.md
|  |- capabilities.md
|  |- engine-support.md
|  `- verification.md
`- .github/workflows/
   |- ci.yml
   `- release.yml
```

Release binary embeds or ships a version-matched plugin source payload. Ordinary commands never download code. Plugin build/install occurs only through explicit setup.

## 10. Implementation milestones

Use statuses `not-started`, `active`, `blocked`, `done`. A milestone becomes `done` only when all acceptance checks have evidence in `docs/verification.md`.

### M0 — Evidence, target cell, and frozen vertical contract

Status: done

Selected initial cell:

- Unreal Engine 5.8.1, changelist `56057345`, Epic Launcher promoted installed build.
- Engine root `/Users/Shared/Epic Games/UE_5.8`.
- macOS arm64 host; universal UnrealEditor and arm64 UBT.
- Xcode 26.6 / Apple clang 21.0.0 / installed macOS SDK.
- Engine-bundled .NET 10.0 runtime; no system `dotnet` dependency.

Confirmed:

- Engine and version metadata discovered from authoritative local files.
- UnrealEditor, UnrealEditor-Cmd, UBT, and RunUAT paths exist and are executable.
- UBT starts successfully when CLI supplies bundled `DOTNET_ROOT`.
- RunUAT starts successfully and selects bundled arm64 .NET SDK.
- Minimal `MagiUnrealAXI` editor plugin compiled through `RunUAT BuildPlugin` for Mac arm64 in about 10 seconds.
- Xcode 26.6, Apple clang 21.0.0, and macOS SDK 26.5 were accepted by UE 5.8.1 UBT.
- Packaged arm64 `libUnrealEditor-MagiUnrealAXI.dylib` loaded from repository-owned disposable fixture.
- `UnrealEditor-Cmd -run=CompileAllBlueprints` emitted plugin startup marker, exited 0, and reported 0 errors/0 warnings.
- Certification retains bounded sanitized evidence outside repository and removes generated workspace artifacts.
Assigned to later milestones:

- Automation execution, cook, and package normalization: M7.
- Claude Code, Codex, and OpenCode integration format verification: M8.
- Remaining release command/output matrix, skill, notices, and provenance: M8.

Acceptance:

- [x] Selected UE 5.8.1/macOS arm64 cell builds and loads minimal plugin.
- [x] Disposable project opens and exits under controlled commandlet invocation.
- [x] Every first-slice behavior has source, decision, and verification evidence.
- [x] Unsupported platforms/minors are stated rather than implied.
- [x] No implementation depends on undocumented MCP behavior.

Stop/rescope if installed UE 5.8.1 and Xcode toolchain cannot compile/load minimal plugin, or no owned disposable Unreal runner can support live release gates.

### M1 — AXI kernel and offline home

Status: done

Work:

- Initialize Rust 2024 package with intentional MSRV and committed lockfile.
- Implement thin process entry and typed library boundary.
- Implement canonical clap schema.
- Implement TOON/JSON renderer, recursive semantic truncation, errors, and exit mapping.
- Implement project discovery and no-argument home outside/inside project.
- Add config resolver and atomic writes.

Acceptance:

- No args returns content-first valid TOON.
- JSON mode parses strictly.
- `--help` and `--version` exit 0 without project/engine work.
- Unknown command/flag exits 2 with structured correction and empty stderr.
- Operational error exits 1 in selected format.
- Empty states are explicit.
- `Cli::command().debug_assert()` passes.
- Real-binary tests isolate HOME, cwd, env, and filesystem.

### M2 — Engine discovery, process runner, and plugin packaging

Status: done

Work:

- Resolve project descriptors, engine association, engine binaries, UBT, and UAT.
- Build typed process invocations without shell.
- Bound/capture child stdout and stderr; retain raw logs separately.
- Add `project doctor` and `engine list|view`.
- Bundle plugin source payload.
- Implement plugin install/status/update/repair/uninstall with managed manifest and hashes.

Acceptance:

- Multiple projects/engines produce deterministic ambiguity errors.
- Fake engine receives exact executable and argv.
- Child output cannot corrupt structured stdout.
- Timeout and process-tree cleanup are tested on initial host.
- Plugin setup is explicit, atomic, idempotent, reports every project change, and preserves unrelated `.uproject` fields.
- Uninstall removes only unchanged managed files; modified plugin tree requires explicit force and preserves backup.
- Blueprint-only project path is proven for selected engine cell.

### M3 — Authenticated native bridge bootstrap

Status: done

Work:

- Implement C++ listener, length framing, token generation, atomic discovery, handshake, and cleanup.
- Implement Rust discovery/auth/framing client.
- Add `bridge.health`, `bridge.describe`, and plugin compatibility reporting.
- Add malformed frame, wrong token, stale record, wrong project, version mismatch, queue, and size-limit tests.

Acceptance:

- CLI connects only to selected project/editor.
- Wrong token/project/session/version fail closed.
- Listener is loopback-only with no remote override.
- Token never appears in any captured output/log/error.
- Malformed/oversized frames cannot crash editor.
- Multiple editor records require explicit disambiguation.
- Editor shutdown removes discovery state; stale state remains diagnosable.

### M4 — Read-only editor vertical slice

Status: done

Capabilities:

- `editor.status`
- `level.current`
- `level.list`
- `actor.list`
- `actor.view`
- `asset.list`
- `asset.view`
- `capability.search`
- `capability.describe`

Work:

- Add bounded game-thread queue.
- Add capability records and generator/check mode.
- Implement pagination and stable IDs.
- Validate outputs on Rust and C++ boundaries.

Acceptance:

- No UE API runs on socket thread.
- Empty level/actor/asset lists state zero and scope.
- Lists have totals and deterministic ordering.
- Search results remain compact; describe returns full selected contract.
- Current level and actor/asset identities survive repeated reads.
- Rust catalog and C++ registry hashes match in CI and runtime handshake.

### M5 — First safe mutation and persistence proof

Status: done

Capabilities:

- `level.create`
- `level.open`
- `actor.spawn`
- `actor.update_transform`
- `actor.delete`
- `level.save`
- `operation.view`

Work:

- Add editor-state gates, serial mutations, transactions, dirty tracking, idempotency ledger, receipts, save wrappers, and readback.
- Use explicit `agentKey` unique within level as spawn natural key; return persistent `ActorGuid` and revision. Actor label/path is display/readback data only.
- Add exact destructive target and `--force` behavior.

Acceptance:

- Repeating `actor.spawn` with same level-scoped `agentKey` is safe no-op and returns same actor ID.
- Mutation response identifies changed/dirty/saved/readback state.
- Save is explicit.
- Actor and transform survive editor restart.
- Delete dry-run identifies exact target; delete without `--force` fails before bridge call.
- PIE/modal/compile/unsafe state refuses mutation with actionable error.
- Ambiguous disconnect can be resolved using operation ID or natural-key readback.
- One mutation from two concurrent CLI processes serializes or receives deterministic `busy`.

This milestone is architecture proof. Do not expand capability count before it passes.

### M6 — First game-building loop

Status: done

Capabilities:

- `asset.create_input_action`
- `asset.create_input_mapping_context`
- `asset.save`
- `blueprint.view`
- `blueprint.compile`
- `component.list`
- `component.view`
- `component.add`
- `component.update`
- `component.remove`
- `level.settings`
- `level.set_game_mode`
- `play.start`
- `play.status`
- `play.observe`
- `play.input`
- `play.screenshot`
- `play.stop`

Completed fixture loop:

- Created and explicitly saved input action/mapping assets, configured persistent components and GameMode, and proved canonical revisions across save/restart/reopen.
- Started PIE, observed structured world/session/actor state, sent deterministic movement/interaction input, captured bounded PNG evidence, synchronously verified stop, reran, and proved deterministic reset.
- Deferred `play.input` success until next-tick `play.observe` readback; receipts validate exact project/editor/operation/target identity plus declared transaction, reversibility, persistence, and safety metadata.
- Compiled valid Blueprint as stable no-op on repeat and returned structured graph/node diagnostics for invalid Blueprint. Blueprint authoring remains excluded.
- Built source-backed fixture project successfully. This proves `project build` for M6 fixture; full build/test/cook/package pipeline normalization remains M7.
- Canonical catalog contains 40 records with SHA-256 `6213c83a5ad2a61336ec08bd4bfebb9564e434f7f12a9bf2b9bc951f0fc14922`. P1.1 six-capability implementation and integrated certification are complete.

Acceptance:

- [x] Every mutation has operation-specific readback and validated receipt metadata.
- [x] Blueprint compile failure is structured with graph, node GUID, and node title when available.
- [x] PIE observation reports world, session, actor transforms, and tags without log scraping.
- [x] Screenshot returns bounded path/dimensions, validates PNG signature and pixels, and does not print binary data.
- [x] Save/restart/reopen proves asset, component, level setting, Blueprint, and removal persistence.
- [x] Fixture project build proves source gameplay code remains valid.
- [x] Fixture resets deterministically and preserves unrelated sentinel data.

### P1 — Post-V1 Blueprint authoring and reusable gameplay construction

Status: active

Detailed multi-phase plan: [`docs/plans/p1-capability-expansion.md`](docs/plans/p1-capability-expansion.md). P1.0–P1.6 cover contract hardening, Blueprint construction, interaction gameplay, UI, AI/navigation, animation, and combined certification.

Add only operations required by verified fixtures:

- Blueprint creation.
- Variables, functions, events, nodes, pins, defaults, compile, and graph reads.
- Components, interfaces, delegates, timers.
- Widgets and UI binding.
- Collision/physics.
- Basic AI controller, NavMesh, Blackboard, and Behavior Tree.
- Animation Blueprint basics.

Acceptance per capability, where applicable to its read/mutation/runtime contract:

1. Schema valid.
2. Discoverable.
3. Handler reaches correct UE API.
4. Mutation reads back.
5. Authored Blueprint compiles.
6. Durable authored state persists after restart.
7. PIE behavior is observed for runtime capabilities.
8. Dedicated fixture identifies supported engine cell and known limits.

Fixture order:

1. `interaction-loop`
2. `ui-state-loop`
3. `ai-navigation-loop`
4. `animation-state-loop`

### M7 — Production pipeline

Status: done

Work:

- UBT build normalization.
- Unreal automation test list/run/report parsing.
- UAT cook and package.
- Bounded log latest/search.
- Build artifacts and operation summaries.

Acceptance:

- [x] Dry-run shows exact executable/argv without execution.
- [x] Failed build/test/cook/package returns exit 1 plus structured totals and log/report path.
- [x] Passed and up-to-date operations return exit 0.
- [x] Test list zero is success; test run matching zero is failure.
- [x] Package destination is explicit, canonicalized, and protected from unintended overwrite.
- [x] UAT operations are never automatically retried.
- [x] Log reads are bounded for bytes, lines, Unicode, and control sequences.

### M8 — Agent integration, documentation, and release

Status: done

Work:

- Implement explicit agent setup against current host schemas.
- Generate static skill/guidance from canonical source.
- Add CI, self-hosted Unreal verification, release artifacts, checksums, notices, and clean-install tests.
- Run representative agent evaluation.

Completed evidence:

- Staged with rollback on write/commit error, idempotent setup supports Claude Code, Codex, and OpenCode target selection. Claude receives SessionStart context; Codex/OpenCode receive Agent Skills with documented ambient-context N/A reasons.
- Guidance and release checks are wired into hosted CI and release workflow.
- Tagged release workflow builds once, ad-hoc signs, packages archive/checksum, certifies exact artifact on approved self-hosted UE host, then publishes with GitHub provenance.
- Hardened exact-artifact clean-install pass: `~/Library/Caches/magi-unreal-axi/m8/live/evidence.w517tz` (current archive SHA-256 `4be0341503bf63cc8ea7ed928f4cd3864764ad16a4608b08c59f626243c16424`).
- Representative jobs 1–8: `~/Library/Caches/magi-unreal-axi/m8/agent-evaluation/run.Gy9dcQ/evidence`; jobs 2–7 succeeded for intent, job 1 was partial orientation, C++ source job 8 exposed pre-fix 14/15 automation and game-target arm64 link failure/no package, and distinct Blueprint-only job 8b packaged 37 files/1,389,331,062 bytes.
- Concrete input-action fixture correction followed by source build and full 15/15 automation: `~/Library/Caches/magi-unreal-axi/read-fixture-fix.KGV2FR/evidence`.

Final gate:

- Current hardened `certify-m8-live.sh` passed with checksum binding, allowlisted archive inventory, ad-hoc codesign, artifact/source identity, full 16-test automation, clean-install lifecycle, and retained-evidence token scan.

Acceptance:

- [x] Claude/Codex/OpenCode support is evidence-backed or marked N/A.
- [x] Setup preserves unrelated config, deduplicates, repairs executable path, and is repeated no-op.
- [x] Hook invokes compact project-scoped home/context and stays under measured token budget.
- [x] Skill frontmatter is trigger-shaped and examples are parser-tested/non-interactive.
- [x] README, help, skill, capability docs, protocol docs, and generated artifacts agree.
- [x] Refreshed release artifact installs CLI and matching plugin without external repository under current hardened gate.
- [x] Refreshed clean install proves checksum/archive/codesign, home, agent setup, plugin setup, full automation, editor health, read, mutation, restart persistence, build, uninstall, and token non-disclosure.
- [x] Supported engine/platform matrix contains only UE 5.8.1/macOS arm64, certified through M8.

## 11. Capability expansion order

After M6 passes, prioritize complete workflows rather than action count:

1. Assets, levels, actors, components, save/readback.
2. Blueprint graph and gameplay framework.
3. PIE observation, input, UI, collision, and physics.
4. AI/navigation and animation basics.
5. Materials, material instances, audio, and Niagara.
6. Sequencer and rendering.
7. Landscape, foliage, and PCG.
8. GAS, StateTree, networking, advanced animation.
9. Fab/Epic registry/plugin ecosystem only after explicit product decision.

A domain does not ship because one handler works. It ships when fixture evidence covers creation, inspection, mutation, save/reload, failure, and relevant runtime behavior.

## 12. Test strategy

### Layer 1 — Rust unit tests

- Parser and schema assertion.
- Config/project/engine precedence.
- TOON/JSON rendering.
- Truncation and `--full`.
- Stable errors and exit codes.
- Frame codec and size limits.
- Capability schema validation/search.
- Invocation construction and log normalization.
- Secret redaction.

### Layer 2 — Hermetic real-binary tests

Use fake engine binaries and fake bridge server. Assert:

- stdout parses;
- stderr contract;
- exact exit code;
- exact child argv;
- no call after usage failure;
- auth handshake and project identity;
- empty states/totals/pagination/suggestions;
- malformed config;
- setup merge/idempotence/path repair;
- distinctive fake token absent everywhere.

No hermetic test reads developer HOME, credentials, engine, or project; all use isolated fixture state.

### Layer 3 — C++ protocol and handler tests

- Shared valid/invalid protocol fixtures.
- Constant-time token helper contract.
- Frame and queue bounds.
- Operation registry/schema hashes.
- Editor-state gates.
- Transactions, dirty packages, save wrappers, idempotency, and receipts.
- Include resolution and source-structure checks.
- Duplicate/renamed labels, persistent actor IDs across restart, stale revisions, and stale cursors.
- Dispatched-operation timeout, atomic receipt recovery, and crash/outcome-unknown handling.
- Slow/partial frames, unauthenticated connection exhaustion, and failed-handshake limits.
- Token/discovery symlink, owner, permission, and publication-order attacks.
- External-editor stop refusal, dirty shutdown, graceful stop, and explicit forced lifecycle paths.

### Layer 4 — Live Unreal fixtures

Owned self-hosted runner executes:

1. Fresh disposable text-owned project.
2. Install/build plugin.
3. Launch editor unattended.
4. Connect real release CLI.
5. Run read/mutation/game loop.
6. Save and stop editor.
7. Restart editor.
8. Verify persisted state.
9. Run build/test/package as milestone requires.
10. Collect structured reports, logs, crashes, and changed-file inventory.

No release based only on mock tests.

### Layer 5 — Representative agent evaluation

Measure at least these jobs:

- Orient in unknown Unreal project.
- Diagnose missing/incompatible plugin.
- Find actor and inspect relevant properties.
- Create level and place configured actor.
- Save and prove persistence.
- Run PIE and observe behavior.
- Diagnose compile/build failure.
- Package validated project.

For each record success, CLI calls, Unreal calls, output tokens, avoidable follow-ups, and failure ambiguity.

## 13. Security and correctness rules

- Loopback-only bridge in v1; no configuration can widen bind address.
- Authentication required even on loopback.
- Exact project/session identity on every connection.
- Hard frame, response, queue, recursion, string, file, and path limits.
- Canonicalize and resolve symlinks before project-boundary checks.
- No shell interpolation.
- No arbitrary code/console/Python execution.
- No raw provider output in AXI data.
- No secrets in args, output, errors, logs, hooks, receipts, snapshots, or fixtures.
- One active editor operation; mutations serialized.
- Unsafe editor states fail closed.
- No mutation success before postcondition readback.
- No persistence success before save/reload evidence where operation claims persistence.
- No automatic retries for non-idempotent operations.
- Explicit target and force flag for destructive operations.
- Plugin setup never silently edits or deletes unmanaged files.
- Agent should operate on source-controlled/disposable projects; output states changed and dirty files.
- Offline home/project inspection is data-only. Build, test, cook, package, editor launch, and plugin operations load or execute trusted project/engine code and return `executesProjectCode: true` in result metadata.
- “No arbitrary code execution” means no generic bridge operation for Python, shell, console, or source execution; it does not claim Unreal builds or project modules are inert.

## 14. Engine and platform support policy

- Certify installed Epic Launcher UE 5.8.1 (`56057345`) on macOS arm64 first.
- Keep version-specific APIs under plugin `Compatibility/`.
- Centralize engine compile guards; do not scatter them through domain handlers.
- Detect optional modules and report capability unavailable rather than enabling broad plugins silently.
- Add platform/minor cells only after plugin build, load, live mutation, save/restart, and clean-install evidence.
- Source/custom engines are separate support cells.
- Prebuilt plugin packages must match engine minor, platform, architecture, and toolchain.
- Source plugin remains fallback only when required build toolchain is verified.
- Capability describe output reports certified, runtime-available, and unavailable states separately.

Installed Unreal Engine 5.8.1 (`56057345`) on macOS arm64 is certified through P1.1 for native plugin build/load, authenticated interactive lifecycle, catalogued editor reads and safe mutations, bounded Blueprint construction, explicit persistence, PIE input/observation/screenshot/stop, Blueprint diagnostics, restart/reset, C++ fixture build/editor proof, full automation, Blueprint-only cook/package output, agent integration, exact-artifact clean installation, protected destinations, bounded logs, and durable process summaries.

## 15. Release and dependency policy

Rust baseline: 1.88.

```sh
cargo fmt --check
cargo test --locked
cargo clippy --all-targets --all-features --locked -- -D warnings
cargo build --release --locked
cargo run --locked --bin xtask -- capabilities check
cargo run --locked --bin xtask -- guidance check
cargo run --locked --bin xtask -- release check
cargo audit
cargo deny check
```

Release gate also requires:

- capability generation `--check`;
- skill/docs generation `--check`;
- plugin build against each advertised engine cell;
- live fixture suite;
- clean install/uninstall;
- package metadata, license, notices, checksums, and provenance;
- installed `--version`, home, setup, read, mutation, restart, build.

Tagged release workflow builds archive once. Exact archive and checksum pass approved self-hosted UE 5.8.1/macOS arm64 live gate before publish. Binary is ad-hoc signed, not Developer ID signed or notarized; publish adds GitHub build provenance.

Keep Rust dependencies narrow. Expected initial crates:

- `clap`
- `serde`
- `serde_json`
- `toon-format`
- `toml`
- `tempfile` and `assert_cmd` for tests

Add async runtime, archive, schema, hashing, or platform crates only when implementation evidence shows standard library/current dependencies insufficient.

## 16. Risks and mitigations

| Risk | Impact | Mitigation | Gate |
|---|---|---|---|
| Scope expands toward MCP parity | Unfinishable, shallow surface | Freeze fixture-driven V1; add domains only with complete evidence | M0/M6 |
| Incorrect UE mutation lifecycle | Corrupt/lost assets | Game-thread handlers, transactions, dirty/save/readback/restart checks | M5 |
| Local endpoint abuse | Editor-level mutation/code execution | Loopback-only, token auth, project identity, allowlisted operations | M3 |
| UE API drift | Compile/runtime breakage | One minor first, compatibility layer, per-cell certification | M0+ |
| Concurrent CLI calls | Races and stale state | Serialized bounded queue, revision/conflict/busy errors | M5 |
| Plugin packaging mismatch | Setup/build failure | Matching embedded payload, exact compatibility report, explicit setup | M2 |
| Mock tests hide editor bugs | False release confidence | Owned live Unreal runner required | M4+ |
| Save claims are hollow | Changes disappear after restart | Persistence receipt plus restart fixture | M5 |
| Capability schema drifts | CLI/plugin disagreement | Neutral source, generated metadata, hashes, fixture parity | M4 |
| Arbitrary escape hatch becomes unsafe | Full local code execution | Catalog-only execute; no Python/shell/raw operation | V1 |
| Copied source loses provenance | License/maintenance risk | Original implementation by default; explicit port manifest/notices | all |
| Agent context wastes tokens | Constant session tax | Compact measured home/context; deep data on demand | M8 |

## 17. Proceed and stop criteria

### Proceed when

- [x] UE 5.8.1/macOS arm64 target selected; minimal plugin builds and loads in disposable project.
- [x] Durable repository fixture and full editor lifecycle pass through M6.
- AXI contract is frozen and tested.
- Authenticated project-specific bridge works.
- Actor read and one idempotent mutation pass end-to-end.
- Mutation survives save and editor restart.
- Overlapping mutations serialize or fail deterministically.
- Owned Unreal runner can enforce release gate.

### Stop or rescope when

- V1 requires hundreds of prior MCP actions.
- Product requires one universal precompiled plugin.
- Remote unauthenticated access is required.
- Arbitrary Python/shell/console execution is mandatory by default.
- No live Unreal runner is available.
- Mutation correctness cannot be proven after reload.
- Initial plugin requires enabling broad unrelated engine plugins.
- Multiple UE minors/platforms must be claimed without per-cell evidence.

## 18. Definition of done

Project is V1 complete only when:

- No MCP runtime, protocol, SDK, server, repository, Node, or Python dependency exists.
- Release artifact contains CLI and matching native plugin payload.
- Agent completes first game-building loop non-interactively.
- All public output follows AXI contract.
- All mutations return truthful verification receipts.
- Saved changes survive editor restart.
- Build, automation test, cook/package, and PIE verification pass in live fixture.
- Home, help, setup, skill, docs, generated capability catalog, and implementation agree.
- Clean installation and safe uninstall pass.
- Advertised engine/platform support has live evidence.
- Remaining unsupported domains are explicit.

## 19. First implementation sequence

Do not begin broad capability work. Execute in this order:

1. M0 target evidence and repository fixture — complete.
2. M1 AXI kernel and offline home — complete.
3. M2 engine resolver and explicit plugin installer — complete.
4. M3 authenticated `bridge.health` vertical slice — complete.
5. M4 read-only game-thread editor vertical slice — complete.
6. M5 idempotent mutation, explicit save, and restart verification — complete.
7. M6 game-building loop, PIE verification, and fixture project build — complete.
8. M7 production pipeline and M8 agent/release integration — complete.
9. Begin post-V1 P1 only after initial release evidence is green.
