# magi-unreal-axi

Self-contained agent-facing Rust 2024 CLI plus bundled native Unreal editor plugin. Unreal Engine 5.8.1 on macOS arm64 is certified through P1.2. Default stdout is TOON; `--format json` emits strict JSON.

## Install a release artifact

Keep archive and `SHA256SUMS` together, then verify before extraction:

```sh
shasum -a 256 -c SHA256SUMS
tar -xzf magi-unreal-axi-0.1.0-macos-arm64.tar.gz
install -d "$HOME/.local/bin"
install -m 755 magi-unreal-axi-0.1.0-macos-arm64/magi-unreal-axi "$HOME/.local/bin/magi-unreal-axi"
codesign --verify --strict --verbose=2 "$HOME/.local/bin/magi-unreal-axi"
magi-unreal-axi --version
```

Release binaries use ad-hoc macOS signing (`codesign --sign -`) for integrity verification. They are not Developer ID signed or Apple-notarized. Tagged GitHub workflow builds archive once, records SHA-256, sends that exact archive to approved self-hosted UE 5.8.1/macOS arm64 live certification, then publishes only after that job succeeds. GitHub build provenance is attached at publish time.

## Commands

```sh
magi-unreal-axi
magi-unreal-axi project view
magi-unreal-axi project doctor
magi-unreal-axi engine list
magi-unreal-axi --engine "/Users/Shared/Epic Games/UE_5.8" engine view
magi-unreal-axi setup plugin status
magi-unreal-axi setup plugin install
magi-unreal-axi editor start
magi-unreal-axi editor status
magi-unreal-axi editor describe
magi-unreal-axi editor stop
magi-unreal-axi capability search actor
magi-unreal-axi capability describe actor.list
magi-unreal-axi level current
magi-unreal-axi level list --limit 100
magi-unreal-axi level settings --level-id /Game/Maps/AgentProof
magi-unreal-axi actor list --fields id,label,class,levelId
magi-unreal-axi actor view '<level-package>#<ActorGuid>'
magi-unreal-axi asset list --limit 100
magi-unreal-axi asset view /Game/Path/Asset.Asset
magi-unreal-axi level create --path /Game/Maps/AgentProof
magi-unreal-axi actor spawn --level /Game/Maps/AgentProof --class /Script/Engine.StaticMeshActor --agent-key proof
magi-unreal-axi actor update-transform '<level-package>#<ActorGuid>' --location 100,200,300 --expected-revision <revision>
magi-unreal-axi component list --actor-id '<level-package>#<ActorGuid>'
magi-unreal-axi component add --actor-id '<level-package>#<ActorGuid>' --class /Script/Engine.SceneComponent --name AgentComponent --expected-revision <revision>
magi-unreal-axi component view '<actor-id>#component:<component-name>'
magi-unreal-axi level set-game-mode --level-id /Game/Maps/AgentProof --class /Script/Engine.GameModeBase --expected-revision <revision>
magi-unreal-axi capability execute asset.create_input_action --input-json '{"path":"/Game/Input/IA_Move","valueType":"Axis1D"}'
magi-unreal-axi asset save /Game/Input/IA_Move.IA_Move --expected-revision <revision>
magi-unreal-axi blueprint view /Game/Path/BP_Game.BP_Game
magi-unreal-axi blueprint compile /Game/Path/BP_Game.BP_Game --expected-revision <revision>
magi-unreal-axi level save --path /Game/Maps/AgentProof
magi-unreal-axi play start
magi-unreal-axi play status --session-id <session-id>
magi-unreal-axi play observe --session-id <session-id>
magi-unreal-axi play input W --session-id <session-id> --event pressed
magi-unreal-axi play screenshot --session-id <session-id> --path proof.png
magi-unreal-axi play stop --session-id <session-id>
magi-unreal-axi actor delete '<level-package>#<ActorGuid>' --dry-run --expected-revision <revision>
magi-unreal-axi actor delete '<level-package>#<ActorGuid>' --force --expected-revision <revision>
magi-unreal-axi project build
magi-unreal-axi project test list --filter MagiUnrealAXI
magi-unreal-axi project test run --filter MagiUnrealAXI.M6
magi-unreal-axi project cook --output ./Cooked
magi-unreal-axi project package --output ./Package
magi-unreal-axi operation view <operation-id>
magi-unreal-axi log latest
magi-unreal-axi log search failure --limit 20
```

Project resolution: `--project`, `MAGI_UNREAL_PROJECT`, then nearest directory containing exactly one `.uproject`. Engine resolution: `--engine`, `MAGI_UNREAL_ENGINE`, project `.magi/unreal-axi.toml` `engine` key, then validated local UE paths; `MAGI_UNREAL_ENGINE_DISCOVERY_ROOT` overrides conventional discovery root. Plugin setup is explicit, atomic, idempotent, hash-managed, preserves unrelated descriptor fields, refuses modified trees, and backs up forced replacement/removal.

Editor bridge uses authenticated loopback-only framed TCP. Native operations execute serially on game thread. Catalog contains 48 records with SHA-256 `fc2c7109093b848359b6307908ede3e5939389c301929394f916a0e0e00c2d60`; every handshake enforces Rust/C++ parity. Discovery reports local `available`, native `unknown` with `editor_offline` when no live matching editor exists, and live native `available` or `unavailable` with structured reasons; execute only `available`. Mutations enforce editor-state gates, expected canonical revisions, idempotency, explicit persistence, and receipts that validate project/editor/operation/target identity plus safety metadata. Failed Blueprint compile returns exit 1 with `blueprint_compile_failed`, a failed non-atomic receipt, no rollback or saved-persistence claim, and `retryable:false`. Dirty invalid assets report dirty persistence and packages; already-invalid clean assets truthfully report unchanged persistence. Inspect `operation view` before retry, including journal fallback offline. P1.1 adds certified bounded `blueprint.create`, `blueprint.graph_view`, event/node ensures, typed pin defaults, and allowlisted pin connections. P1.2 adds certified `blueprint.interface_create`, `blueprint.interface_view`, `blueprint.interface_ensure`, `blueprint.scs_view`, `blueprint.scs_component_ensure`, `blueprint.scs_component_update`, `blueprint.scs_component_remove`, and `play.component_observe`. `play.input` completes after deferred observation readback. `play.stop` ends PIE synchronously and verifies stopped status before success. Screenshots stay under project `Saved/MagiUnrealAXI/Screenshots`, return PNG path and dimensions bounded to 1–16384, and never print binary data.

`project build`, `project test list|run`, cook-only materialization from `Saved/Cooked/Mac`, transactional Blueprint-only package output, bounded logs, strict report parsing, and protected output paths are live-certified. P1.2 exact 22/22 Unreal automation, interface/two-Blueprint/nested-SCS/overlap/interaction/two-PIE reset, registry/generated-class/IoStore/inventory/receipt/token gates pass. M8 adds agent setup/evaluation, exact-archive clean installation, and release gating.

Expected errors are structured on stdout. Exit codes: 0 success, 1 operational failure, 2 usage failure. Progress and child logs never enter structured stdout.

## Agent setup

`magi-unreal-axi setup agents` installs all supported targets; `--claude`, `--codex`, and `--opencode` select targets explicitly. Claude setup merges managed SessionStart hook into `~/.claude/settings.json` and installs `~/.claude/skills/magi-unreal-axi/SKILL.md`. Codex installs `~/.agents/skills/magi-unreal-axi/SKILL.md`; OpenCode installs `~/.config/opencode/skills/magi-unreal-axi/SKILL.md`. Setup preserves unrelated configuration, rejects modified managed skills, repairs its managed Claude hook executable path, and is idempotent on repeat. Codex has no documented command SessionStart hook; OpenCode ambient plugins require JavaScript/TypeScript, which this distribution prohibits.

For clean-install testing, agent configuration may use an isolated `HOME`. Unreal editor/plugin lifecycle must use actual macOS account `HOME` because Unreal resolves user settings and runtime paths through macOS account APIs. Do not treat isolated agent-config HOME as an isolated Unreal account.

## Development

Requires Rust 1.88. Rust gate covers 44 library tests, 5 xtask tests, and 44 real-binary integration tests = 93 tests. Run:

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
./tests/unreal/certify-m8-live.sh target/release/magi-unreal-axi-0.1.0-macos-arm64.tar.gz
./tests/unreal/certify-p1.1.sh
./tests/unreal/certify-p1.2.sh
```
