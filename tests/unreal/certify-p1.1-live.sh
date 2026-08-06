#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.1-manifest.json"
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -f "$manifest" && -f "$version_file" && -x "$editor" && -x "$run_uat" ]]
jq -e '.phase == "P1.1" and (.engine.version | type) == "string" and (.engine.changelist | type) == "number" and (.engine.hostArchitecture | type) == "string" and (.automationTests | length) > 0 and (.liveAssertions | length) == 9 and .pipeline.blueprintOnly == true and .plugin.requiredArchitectures == ["arm64","x86_64"] and (.cli.binary | type) == "string"' "$manifest" >/dev/null
[[ "$(uname -m)" == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
actual_version="$(plutil -extract MajorVersion raw -o - "$version_file").$(plutil -extract MinorVersion raw -o - "$version_file").$(plutil -extract PatchVersion raw -o - "$version_file")"
[[ "$actual_version" == "$(jq -r .engine.version "$manifest")" ]]
[[ "$(plutil -extract Changelist raw -o - "$version_file")" == "$(jq -r .engine.changelist "$manifest")" ]]
cache_root="$HOME/Library/Caches/magi-unreal-axi/p1.1/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
project="$work/project/MagiUnrealAXIFixture.uproject"
project_dir=$(dirname "$project")
bin=${P11_CLI_PATH:-$work/magi-unreal-axi}
plugin_dir=${P11_PLUGIN_DIR:-$work/plugin}
package_project_dir=${P11_PACKAGE_PROJECT_DIR:-}
pid=
session=
tokens=()
level=$(jq -r .packageAssertions.mapPackage "$manifest")
blueprint=$(jq -r .packageAssertions.blueprintObject "$manifest")
blueprint_file=$(jq -r .packageAssertions.blueprint "$manifest")
map_file=$(jq -r .packageAssertions.map "$manifest")
content_dir=$(dirname "$blueprint_file")
[[ $(dirname "$map_file") == "$content_dir" ]]
package="${blueprint%.*}"
generated="$blueprint.$(jq -r .packageAssertions.generatedRuntime "$manifest")"

export DOTNET_ROOT="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
export PATH="$DOTNET_ROOT:$PATH"
editor_alive() { kill -0 "$1" 2>/dev/null && [[ $(ps -p "$1" -o stat= 2>/dev/null) != Z* ]]; }
axi() { "$bin" --project "$project" --engine "$engine_root" --timeout 15 --format json "$@"; }
retry_mutation() {
  local out=$1
  shift
  for _ in $(seq 1 150); do
    if axi "$@" >"$out"; then return 0; fi
    [[ $(jq -r '.error.reason == "unsafe_editor_state" and .error.retryable == true' "$out") == true ]] || return 1
    sleep .2
  done
  return 1
}
stop_editor() {
  [[ -n "$pid" ]] || return 0
  local stopped_pid=$pid stopped_session=$session
  if editor_alive "$stopped_pid"; then
    axi --editor "$stopped_pid" editor stop >"$work/editor-stop-$stopped_pid.json" 2>/dev/null || true
    for _ in $(seq 1 200); do editor_alive "$stopped_pid" || break; sleep .1; done
    editor_alive "$stopped_pid" && kill -TERM "$stopped_pid" 2>/dev/null || true
    for _ in $(seq 1 200); do editor_alive "$stopped_pid" || break; sleep .1; done
    editor_alive "$stopped_pid" && kill -KILL "$stopped_pid" 2>/dev/null || true
  fi
  wait "$stopped_pid" 2>/dev/null || true
  for _ in $(seq 1 100); do [[ ! -e "$stopped_session/bridge-v1.json" && ! -e "$stopped_session/token" ]] && break; sleep .1; done
  [[ ! -e "$stopped_session/bridge-v1.json" && ! -e "$stopped_session/token" ]]
  pid=; session=
}
start_editor() {
  local label=$1 record
  "$editor" "$project" -unattended -nop4 -nosplash -RenderOffscreen -ResX=640 -ResY=360 -NoSound "-log=$work/editor-$label-ue.log" >"$work/editor-$label.log" 2>&1 &
  pid=$!
  session="$runtime_root/$pid"
  record="$session/bridge-v1.json"
  for _ in $(seq 1 1200); do [[ -f "$record" ]] && break; editor_alive "$pid" || return 1; sleep .1; done
  [[ -f "$record" && $(jq -r .projectPath "$record") == "$canonical_project" ]]
  tokens+=("$(cat "$session/token")")
  for _ in $(seq 1 600); do
    if axi --editor "$pid" editor status >"$work/status-$label.json" 2>/dev/null && [[ $(jq -r .editor.state "$work/status-$label.json") == ready ]]; then return 0; fi
    sleep .1
  done
  return 1
}
cleanup() {
  local status=$?
  trap - EXIT
  stop_editor || status=1
  if [[ $status != 0 ]]; then echo "P1.1 live certification failed; work retained at $work; evidence at $evidence" >&2; exit "$status"; fi
  [[ ${KEEP_P11_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check >"$work/catalog.txt"
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' "$work/catalog.txt")
[[ "$catalog_hash" == "$(jq -r .catalog.sha256 "$manifest")" ]]
if [[ -z "${P11_CLI_PATH:-}" ]]; then
  cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-build.log" 2>&1
  cp "$repo_root/target/release/magi-unreal-axi" "$bin"
fi
chmod 0755 "$bin"
[[ -x "$bin" ]]
cli_hash=$(shasum -a 256 "$bin" | cut -d' ' -f1)
[[ -z "${P11_CLI_SHA256:-}" || "$cli_hash" == "$P11_CLI_SHA256" ]]
mkdir -p "$project_dir/Plugins"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"
if [[ -z "${P11_PLUGIN_DIR:-}" ]]; then
  "$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$plugin_dir" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
fi
[[ -d "$plugin_dir" ]]
plugin_binary=$(find "$plugin_dir" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit)
[[ -n "$plugin_binary" ]]
plugin_hash=$(shasum -a 256 "$plugin_binary" | cut -d' ' -f1)
[[ -z "${P11_PLUGIN_SHA256:-}" || "$plugin_hash" == "$P11_PLUGIN_SHA256" ]]
ditto "$plugin_dir" "$project_dir/Plugins/MagiUnrealAXI"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.log" 2>&1
canonical_project=$(cd "$project_dir" && pwd -P)/$(basename "$project")
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
runtime_root="$HOME/Library/Caches/magi-unreal-axi/$project_hash"
find "$runtime_root" -mindepth 1 -maxdepth 1 -type d -exec /usr/bin/trash {} + 2>/dev/null || true

start_editor author
retry_mutation "$work/level-create.json" level create --path "$level"
axi capability execute blueprint.create --input-json "{\"path\":\"$package\",\"parentClass\":\"/Script/Engine.StaticMeshActor\"}" >"$work/create.json"
jq -e --arg id "$blueprint" '.result.blueprintId == $id and .result.changed == true and (.result.revision|length)==64 and .receipt.verification.matched == true' "$work/create.json" >/dev/null
axi capability execute blueprint.create --input-json "{\"path\":\"$package\",\"parentClass\":\"/Script/Engine.StaticMeshActor\"}" >"$work/no-op-create.json"
jq -e --arg id "$blueprint" --arg rev "$(jq -r .result.revision "$work/create.json")" '.result.blueprintId == $id and .result.changed == false and .result.revision == $rev and .receipt.verification.matched == true' "$work/no-op-create.json" >/dev/null
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\"}" >"$work/graphs.json"
graph=$(jq -r '.items[] | select(.kind=="ubergraph") | .graphId' "$work/graphs.json" | head -1)
[[ -n "$graph" ]]
rev=$(jq -r .revision "$work/graphs.json")
ensure() {
  local operation=$1 key=$2 field=$3 intent=$4
  local out="$work/$key.json"
  axi capability execute "$operation" --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"$key\",\"$field\":\"$intent\"}" >"$out"
  jq -e '.result.changed == true and .receipt.verification.matched == true' "$out" >/dev/null
  rev=$(jq -r .result.revision "$out")
}
ensure blueprint.event_ensure p11.begin event actor.begin_play
begin_node=$(jq -r .result.nodeId "$work/p11.begin.json")
ensure blueprint.event_ensure p11.input event input.key_e
input_node=$(jq -r .result.nodeId "$work/p11.input.json")
ensure blueprint.node_ensure p11.controller node game.get_player_controller
controller_node=$(jq -r .result.nodeId "$work/p11.controller.json")
ensure blueprint.node_ensure p11.enable node actor.enable_input
enable_node=$(jq -r .result.nodeId "$work/p11.enable.json")
ensure blueprint.node_ensure p11.vector node math.make_vector
vector_node=$(jq -r .result.nodeId "$work/p11.vector.json")
ensure blueprint.node_ensure p11.offset node actor.add_world_offset
offset_node=$(jq -r .result.nodeId "$work/p11.offset.json")

axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\"}" >"$work/nodes.json"
pin() { jq -r --arg node "$1" --arg name "$2" --arg direction "$3" '.items[]|select(.nodeId==$node)|.pins[]|select(.name==$name and .direction==$direction)|.pinId' "$work/nodes.json"; }
begin_then=$(pin "$begin_node" then output)
input_pressed=$(pin "$input_node" Pressed output)
controller_index=$(pin "$controller_node" PlayerIndex input)
controller_return=$(pin "$controller_node" ReturnValue output)
enable_exec=$(pin "$enable_node" execute input)
enable_controller=$(pin "$enable_node" PlayerController input)
vector_x=$(pin "$vector_node" X input); vector_y=$(pin "$vector_node" Y input); vector_z=$(pin "$vector_node" Z input)
vector_return=$(pin "$vector_node" ReturnValue output)
offset_exec=$(pin "$offset_node" execute input)
offset_delta=$(pin "$offset_node" DeltaLocation input)
for required in "$begin_then" "$input_pressed" "$controller_index" "$controller_return" "$enable_exec" "$enable_controller" "$vector_x" "$vector_y" "$vector_z" "$vector_return" "$offset_exec" "$offset_delta"; do [[ -n "$required" ]]; done
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"limit\":2}" >"$work/page-nodes-1.json"
page_cursor=$(jq -r .nextCursor "$work/page-nodes-1.json")
jq -e '.count == 2 and .total >= 6 and (.nextCursor | type) == "string"' "$work/page-nodes-1.json" >/dev/null
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"limit\":2,\"cursor\":\"$page_cursor\"}" >"$work/page-nodes-2.json"
jq -s -e '.[0].revision == .[1].revision and .[0].items[-1].nodeId < .[1].items[0].nodeId' "$work/page-nodes-1.json" "$work/page-nodes-2.json" >/dev/null
expect_error() {
  local name=$1 reason=$2 status
  shift 2
  set +e
  axi "$@" >"$work/invalid-$name.json"
  status=$?
  set -e
  [[ $status != 0 ]]
  jq -e --arg reason "$reason" '.error.reason == $reason' "$work/invalid-$name.json" >/dev/null
  axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\"}" >"$work/invalid-$name-readback.json"
  jq -e --arg rev "$rev" '.revision == $rev' "$work/invalid-$name-readback.json" >/dev/null
}
expect_error missing-revision expected_revision_required capability execute blueprint.node_ensure --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"p11.missing\",\"node\":\"math.make_vector\"}"
expect_error stale-revision conflict capability execute blueprint.node_ensure --expected-revision "$(printf '%064d' 0)" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"p11.stale\",\"node\":\"math.make_vector\"}"
expect_error wrong-graph invalid_capability_input capability execute blueprint.event_ensure --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph-wrong\",\"agentKey\":\"p11.wrong-graph\",\"event\":\"actor.begin_play\"}"
expect_error wrong-event invalid_capability_input capability execute blueprint.event_ensure --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"p11.wrong-event\",\"event\":\"actor.tick\"}"
expect_error wrong-function invalid_capability_input capability execute blueprint.node_ensure --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"p11.wrong-function\",\"node\":\"actor.destroy\"}"
expect_error cross-key-before conflict capability execute blueprint.event_ensure --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"p11.other-before\",\"event\":\"actor.begin_play\"}"
expect_error wrong-pin invalid_capability_input capability execute blueprint.pin_default_set --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"pinId\":\"$vector_node#pin:input:Missing\",\"value\":{\"type\":\"real\",\"value\":0}}"
expect_error wrong-type invalid_capability_input capability execute blueprint.pin_default_set --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"pinId\":\"$controller_index\",\"value\":{\"type\":\"real\",\"value\":0}}"
expect_error wrong-default invalid_capability_input capability execute blueprint.pin_default_set --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"pinId\":\"$vector_x\",\"value\":{\"type\":\"real\",\"value\":1000001}}"
expect_error wrong-link invalid_capability_input capability execute blueprint.pin_connect --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"sourcePinId\":\"$vector_return\",\"targetPinId\":\"$enable_controller\"}"
set_default() {
  local name=$1 pin_id=$2 type=$3 value=$4
  axi capability execute blueprint.pin_default_set --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"pinId\":\"$pin_id\",\"value\":{\"type\":\"$type\",\"value\":$value}}" >"$work/default-$name.json"
  rev=$(jq -r .result.revision "$work/default-$name.json")
}
set_default player-index "$controller_index" integer 0
set_default x "$vector_x" real 0
set_default y "$vector_y" real 0
set_default z "$vector_z" real 100
expect_error stale-cursor stale_cursor capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"limit\":2,\"cursor\":\"$page_cursor\"}"
connect() {
  local name=$1 source=$2 target=$3
  axi capability execute blueprint.pin_connect --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"sourcePinId\":\"$source\",\"targetPinId\":\"$target\"}" >"$work/connect-$name.json"
  jq -e '.result.changed == true and .receipt.verification.matched == true' "$work/connect-$name.json" >/dev/null
  rev=$(jq -r .result.revision "$work/connect-$name.json")
}
connect begin-enable "$begin_then" "$enable_exec"
connect controller-enable "$controller_return" "$enable_controller"
connect input-offset "$input_pressed" "$offset_exec"
connect vector-offset "$vector_return" "$offset_delta"
axi capability execute blueprint.pin_default_set --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"pinId\":\"$vector_x\",\"value\":{\"type\":\"real\",\"value\":0}}" >"$work/no-op-default.json"
jq -e --arg rev "$rev" '.result.changed == false and .result.revision == $rev' "$work/no-op-default.json" >/dev/null
axi capability execute blueprint.pin_connect --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"sourcePinId\":\"$begin_then\",\"targetPinId\":\"$enable_exec\"}" >"$work/no-op-link.json"
jq -e --arg rev "$rev" '.result.changed == false and .result.revision == $rev' "$work/no-op-link.json" >/dev/null
expect_error cross-key-after conflict capability execute blueprint.event_ensure --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"p11.other-after\",\"event\":\"actor.begin_play\"}"
axi blueprint compile "$blueprint" --expected-revision "$rev" >"$work/compile.json"
jq -e '.result.status != "error" and .result.errorCount == 0' "$work/compile.json" >/dev/null
rev=$(jq -r .result.revision "$work/compile.json")
axi asset save "$blueprint" --expected-revision "$rev" >"$work/blueprint-save.json"
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\"}" >"$work/graph-saved.json"
final_graph_revision=$(jq -r .revision "$work/graph-saved.json")
settings_rev=$(axi level settings --level-id "$level" | jq -r .revision)
axi level set-game-mode --level-id "$level" --class /Script/Engine.GameModeBase --expected-revision "$settings_rev" >"$work/game-mode.json"
axi actor spawn --level "$level" --class "$generated" --agent-key p11-authored-actor --label 'P1.1 Authored Actor' --location 300,0,0 >"$work/actor-spawn.json"
actor=$(jq -r .result.id "$work/actor-spawn.json")
axi level save --path "$level" >"$work/level-save.json"
stop_editor
sleep 1

start_editor restart
retry_mutation "$work/level-open-restart.json" level open --path "$level"
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\"}" >"$work/graph-restart.json"
jq -e --arg rev "$final_graph_revision" '.revision == $rev and .total >= 6' "$work/graph-restart.json" >/dev/null
rev=$(jq -r .revision "$work/graph-restart.json")
axi capability execute blueprint.create --input-json "{\"path\":\"$package\",\"parentClass\":\"/Script/Engine.StaticMeshActor\"}" >"$work/no-op-create-restart.json"
jq -e --arg rev "$rev" '.result.changed == false and .result.revision == $rev and .receipt.verification.matched == true' "$work/no-op-create-restart.json" >/dev/null
repeat_ensure() {
  local operation=$1 key=$2 field=$3 intent=$4
  axi capability execute "$operation" --expected-revision "$rev" --input-json "{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"$key\",\"$field\":\"$intent\"}" >"$work/repeat-$key.json"
  jq -e --arg rev "$rev" '.result.changed == false and .result.revision == $rev' "$work/repeat-$key.json" >/dev/null
}
repeat_ensure blueprint.event_ensure p11.begin event actor.begin_play
repeat_ensure blueprint.event_ensure p11.input event input.key_e
repeat_ensure blueprint.node_ensure p11.controller node game.get_player_controller
repeat_ensure blueprint.node_ensure p11.enable node actor.enable_input
repeat_ensure blueprint.node_ensure p11.vector node math.make_vector
repeat_ensure blueprint.node_ensure p11.offset node actor.add_world_offset
sid=$(axi play start | tee "$work/play-start.json" | jq -r .result.sessionId)
for _ in $(seq 1 120); do axi play status --session-id "$sid" >"$work/play-status.json"; [[ $(jq -r .state "$work/play-status.json") == running ]] && break; sleep .2; done
[[ $(jq -r .state "$work/play-status.json") == running ]]
sleep .5
axi play observe --session-id "$sid" >"$work/observe-before.json"
before_z=$(jq -r --arg class "$generated" '.actors[]|select(.class==$class)|.location[2]' "$work/observe-before.json")
[[ "$before_z" == 0 || "$before_z" == 0.0 ]]
axi play input E --session-id "$sid" --event pressed >"$work/input-e.json"
jq -e '.result.accepted == true and .result.changed == true and .receipt.verification.matched == true' "$work/input-e.json" >/dev/null
axi play observe --session-id "$sid" >"$work/observe-after.json"
after_z=$(jq -r --arg class "$generated" '.actors[]|select(.class==$class)|.location[2]' "$work/observe-after.json")
[[ "$after_z" == 100 || "$after_z" == 100.0 ]]
axi play input E --session-id "$sid" --event released >"$work/input-e-release.json"
axi play stop --session-id "$sid" >"$work/play-stop.json"
stop_editor
if [[ -n "$package_project_dir" ]]; then
  [[ -d "$package_project_dir" && -f "$package_project_dir/MagiUnrealAXIPackageFixture.uproject" ]]
  for forbidden in Source Modules Plugins Binaries Intermediate Saved DDC DerivedDataCache; do [[ ! -e "$package_project_dir/$forbidden" ]]; done
  [[ -d "$project_dir/Content/$content_dir" ]]
  mkdir -p "$package_project_dir/Content/$content_dir"
  ditto "$project_dir/Content/$content_dir" "$package_project_dir/Content/$content_dir"
  [[ -f "$package_project_dir/Content/$blueprint_file" ]]
  [[ -f "$package_project_dir/Content/$map_file" ]]
  for root in "$project_dir/Content/$content_dir" "$package_project_dir/Content/$content_dir"; do
    invalid_entries=$(find "$root" ! -type d ! -type f -print -quit) || exit 1
    [[ -z "$invalid_entries" ]]
  done
  hash_tree() {
    local root=$1
    find "$root" -type f -print | LC_ALL=C sort | while IFS= read -r file; do
      printf '%s\n' "${file#"$root/"}"
      shasum -a 256 "$file" | cut -d' ' -f1
    done | shasum -a 256 | cut -d' ' -f1
  }
  source_count=$(find "$project_dir/Content/$content_dir" -type f | wc -l | tr -d ' ')
  destination_count=$(find "$package_project_dir/Content/$content_dir" -type f | wc -l | tr -d ' ')
  [[ "$source_count" == "$destination_count" ]]
  source_content_hash=$(hash_tree "$project_dir/Content/$content_dir")
  dest_content_hash=$(hash_tree "$package_project_dir/Content/$content_dir")
  [[ "$source_content_hash" == "$dest_content_hash" ]]
  printf 'blueprint=%s\nmap=%s\nsourceEntryCount=%s\ndestinationEntryCount=%s\nsourceSha256=%s\ndestinationSha256=%s\n' "$blueprint_file" "$map_file" "$source_count" "$destination_count" "$source_content_hash" "$dest_content_hash" >"$evidence/package-materialization.txt"
fi

for secret in "${tokens[@]}"; do
  set +e; grep -R -I -Fq -- "$secret" "$work"; secret_status=$?; set -e
  if [[ $secret_status == 0 ]]; then echo "runtime token leaked into P1.1 evidence" >&2; exit 1; fi
  [[ $secret_status == 1 ]]
done
cp "$work"/*.json "$evidence/"
cp "$work/catalog.txt" "$evidence/"
artifact=$(find "$plugin_dir" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit)
[[ -n "$artifact" ]]
artifact_hash=$(shasum -a 256 "$artifact" | cut -d' ' -f1)
arches=$(lipo -archs "$artifact")
for arch in $(jq -r '.plugin.requiredArchitectures[]' "$manifest"); do grep -qw "$arch" <<<"$arches"; done
[[ $(wc -w <<<"$arches" | tr -d ' ') == 2 ]]
[[ -z "${P11_PLUGIN_SHA256:-}" || "$artifact_hash" == "$P11_PLUGIN_SHA256" ]]
printf 'phase=P1.1\nengineVersion=%s\nengineChangelist=%s\nhostArchitecture=%s\ncatalogHash=%s\nartifactSha256=%s\ncliSha256=%s\npluginArchitectures=%s\ngraphRevision=%s\nrestart=stable\nidempotency=create-node-event-default-link-safe-no-op\ninvalidMatrix=missing-stale-revision-cursor-graph-event-function-pin-type-default-link-cross-key\npieOffset=0-to-100\ntokenScan=passed\n' "$(jq -r .engine.version "$manifest")" "$(jq -r .engine.changelist "$manifest")" "$(uname -m)" "$catalog_hash" "$artifact_hash" "$cli_hash" "$arches" "$final_graph_revision" | tee "$evidence/summary.txt"
for secret in "${tokens[@]}"; do
  set +e; grep -R -I -Fq -- "$secret" "$evidence"; secret_status=$?; set -e
  [[ $secret_status == 1 ]]
done
printf '%s\n' "$evidence" >"$cache_root/latest"
echo "P1.1 live certification: PASS (evidence retained at $evidence)"
