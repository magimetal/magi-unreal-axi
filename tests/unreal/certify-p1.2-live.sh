#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.2-manifest.json"
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -f "$manifest" && -f "$version_file" && -x "$editor" && -x "$run_uat" ]]
jq -e '.phase == "P1.2" and .fixture.blueprints == 2 and .fixture.interfaces == 1 and .fixture.interfaceFunction == "Interact" and .fixture.timerOrDelegate == false and (.liveAssertions | length) == 6 and .pipeline.blueprintOnly == true and .pipeline.compileAllBlueprints and .pipeline.cook and .pipeline.package and .plugin.requiredArchitectures == ["arm64","x86_64"] and (.packageAssertions.interface.generatedRuntime | endswith("_C")) and (.packageAssertions.targetBlueprint.generatedRuntime | endswith("_C")) and (.packageAssertions.playerBlueprint.generatedRuntime | endswith("_C"))' "$manifest" >/dev/null
[[ "$(uname -m)" == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
actual_version="$(plutil -extract MajorVersion raw -o - "$version_file").$(plutil -extract MinorVersion raw -o - "$version_file").$(plutil -extract PatchVersion raw -o - "$version_file")"
[[ "$actual_version" == "$(jq -r .engine.version "$manifest")" ]]
[[ "$(plutil -extract Changelist raw -o - "$version_file")" == "$(jq -r .engine.changelist "$manifest")" ]]

cache_root="$HOME/Library/Caches/magi-unreal-axi/p1.2/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
if [[ -n "${P12_LIVE_EVIDENCE_DIR:-}" ]]; then
  evidence=$P12_LIVE_EVIDENCE_DIR
  [[ ! -e "$evidence" ]]
  mkdir -p "$evidence"
else
  evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
fi
project="$work/project/MagiUnrealAXIFixture.uproject"
project_dir=$(dirname "$project")
bin=${P12_CLI_PATH:-$work/magi-unreal-axi}
plugin_dir=${P12_PLUGIN_DIR:-$work/plugin}
package_project_dir=${P12_PACKAGE_PROJECT_DIR:-}
pid=
session=
tokens=()

level=$(jq -r .packageAssertions.mapPackage "$manifest")
content_root=$(jq -r .packageAssertions.contentRoot "$manifest")
interface_path=$(jq -r .fixture.paths.interface "$manifest")
interface_id=$(jq -r .packageAssertions.interface.object "$manifest")
target_path=$(jq -r .fixture.paths.targetBlueprint "$manifest")
target_id=$(jq -r .packageAssertions.targetBlueprint.object "$manifest")
target_package=${target_id%.*}
target_class="$target_package.$(jq -r .packageAssertions.targetBlueprint.generatedRuntime "$manifest")"
player_path=$(jq -r .fixture.paths.playerBlueprint "$manifest")
player_id=$(jq -r .packageAssertions.playerBlueprint.object "$manifest")
player_package=${player_id%.*}
player_class="$player_package.$(jq -r .packageAssertions.playerBlueprint.generatedRuntime "$manifest")"
[[ "$interface_id" == "$interface_path.$(basename "$interface_path")" ]]
[[ "$target_id" == "$target_path.$(basename "$target_path")" ]]
[[ "$player_id" == "$player_path.$(basename "$player_path")" ]]
[[ "$target_class" != "$player_class" ]]

export DOTNET_ROOT="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
export PATH="$DOTNET_ROOT:$PATH"
editor_alive() { kill -0 "$1" 2>/dev/null && [[ $(ps -p "$1" -o stat= 2>/dev/null) != Z* ]]; }
axi() { "$bin" --project "$project" --engine "$engine_root" --timeout 30 --format json "$@"; }
retry_mutation() {
  local out=$1
  shift
  for _ in $(seq 1 600); do
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
scan_for_runtime_material() {
  local root=$1 found grep_status
  found=$(find "$root" -type f \( -name token -o -name bridge-v1.json \) -print -quit)
  [[ -z "$found" ]]
  set +e
  grep -R -I -E -q 'Authorization:[[:space:]]*Bearer[[:space:]]+[A-Za-z0-9._-]+' "$root"
  grep_status=$?
  set -e
  [[ $grep_status == 1 ]]
}
cleanup() {
  local status=$?
  trap - EXIT
  stop_editor || status=1
  if [[ $status != 0 ]]; then echo "P1.2 live certification failed; work retained at $work; evidence at $evidence" >&2; exit "$status"; fi
  [[ ${KEEP_P12_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check >"$work/catalog.txt"
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' "$work/catalog.txt")
[[ "$catalog_hash" == "$(jq -r .catalog.sha256 "$manifest")" ]]
if [[ -z "${P12_CLI_PATH:-}" ]]; then
  cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-build.log" 2>&1
  cp "$repo_root/target/release/magi-unreal-axi" "$bin"
fi
chmod 0755 "$bin"
[[ -x "$bin" ]]
cli_hash=$(shasum -a 256 "$bin" | cut -d' ' -f1)
[[ -z "${P12_CLI_SHA256:-}" || "$cli_hash" == "$P12_CLI_SHA256" ]]
mkdir -p "$project_dir/Plugins"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"
if [[ -z "${P12_PLUGIN_DIR:-}" ]]; then
  "$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$plugin_dir" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
fi
[[ -d "$plugin_dir" ]]
plugin_binary=$(find "$plugin_dir" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit)
[[ -n "$plugin_binary" ]]
plugin_hash=$(shasum -a 256 "$plugin_binary" | cut -d' ' -f1)
[[ -z "${P12_PLUGIN_SHA256:-}" || "$plugin_hash" == "$P12_PLUGIN_SHA256" ]]
ditto "$plugin_dir" "$project_dir/Plugins/MagiUnrealAXI"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.log" 2>&1
canonical_project=$(cd "$project_dir" && pwd -P)/$(basename "$project")
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
runtime_root="$HOME/Library/Caches/magi-unreal-axi/$project_hash"
find "$runtime_root" -mindepth 1 -maxdepth 1 -type d -exec /usr/bin/trash {} + 2>/dev/null || true

validate_receipt() {
  local mutation=$1 operation_id view
  operation_id=$(jq -r .receipt.operationId "$mutation")
  [[ "$operation_id" =~ ^[A-Za-z0-9._:-]+$ ]]
  view="$work/operation-$operation_id.json"
  axi operation view "$operation_id" >"$view"
  jq -e --slurpfile mutation "$mutation" '. == $mutation[0].receipt' "$view" >/dev/null
}
assert_changed() {
  jq -e '.result.changed == true and (.result.revision | test("^[0-9a-f]{64}$")) and .receipt.state == "completed" and .receipt.verification.matched == true' "$1" >/dev/null
  validate_receipt "$1"
}
mutate() {
  local out=$1 operation=$2 revision=$3 input=$4
  retry_mutation "$out" capability execute "$operation" --expected-revision "$revision" --input-json "$input"
  assert_changed "$out"
}
pin() {
  local file=$1 node=$2 name=$3 direction=$4
  jq -r --arg node "$node" --arg name "$name" --arg direction "$direction" '.items[] | select(.nodeId == $node) | .pins[] | select(.name == $name and .direction == $direction) | .pinId' "$file"
}
connect() {
  local blueprint=$1 source=$2 target=$3 out=$4
  mutate "$out" blueprint.pin_connect "$rev" "{\"blueprintId\":\"$blueprint\",\"sourcePinId\":\"$source\",\"targetPinId\":\"$target\"}"
  rev=$(jq -r .result.revision "$out")
}
set_default() {
  local blueprint=$1 pin_id=$2 value=$3 out=$4 type=${5:-real} before=$rev
  retry_mutation "$out" capability execute blueprint.pin_default_set --expected-revision "$before" --input-json "{\"blueprintId\":\"$blueprint\",\"pinId\":\"$pin_id\",\"value\":{\"type\":\"$type\",\"value\":$value}}"
  jq -e --arg before "$before" '.receipt.state == "completed" and .receipt.verification.matched == true and (.result.revision | test("^[0-9a-f]{64}$")) and (.result.changed == true or (.result.changed == false and .result.revision == $before))' "$out" >/dev/null
  validate_receipt "$out"
  rev=$(jq -r .result.revision "$out")
}
ensure_graph_node() {
  local blueprint=$1 graph=$2 operation=$3 key=$4 field=$5 intent=$6 identity=${7:-} out input
  out="$work/$key.json"
  input="{\"blueprintId\":\"$blueprint\",\"graphId\":\"$graph\",\"agentKey\":\"$key\",\"$field\":\"$intent\""
  [[ -z "$identity" ]] || input+=",$identity"
  input+='}'
  mutate "$out" "$operation" "$rev" "$input"
  rev=$(jq -r .result.revision "$out")
}

start_editor author
retry_mutation "$work/level-create.json" level create --path "$level"
validate_receipt "$work/level-create.json"

retry_mutation "$work/interface-create.json" capability execute blueprint.interface_create --input-json "{\"path\":\"$interface_path\",\"function\":\"Interact\"}"
assert_changed "$work/interface-create.json"
interface_rev=$(jq -r .result.revision "$work/interface-create.json")
jq -e --arg id "$interface_id" '.result.id == $id and .result.function == "Interact"' "$work/interface-create.json" >/dev/null
axi capability execute blueprint.interface_view --input-json "{\"id\":\"$interface_id\"}" >"$work/interface-view.json"
jq -e --arg id "$interface_id" --arg rev "$interface_rev" '.id == $id and .function == "Interact" and .revision == $rev' "$work/interface-view.json" >/dev/null
retry_mutation "$work/interface-save.json" asset save "$interface_id" --expected-revision "$interface_rev"
jq -e --arg id "$interface_id" '.result.id == $id and .receipt.state == "completed" and .receipt.verification.matched == true' "$work/interface-save.json" >/dev/null
validate_receipt "$work/interface-save.json"

retry_mutation "$work/target-create.json" capability execute blueprint.create --input-json "{\"path\":\"$target_path\",\"parentClass\":\"/Script/Engine.Actor\"}"
assert_changed "$work/target-create.json"
target_rev=$(jq -r .result.revision "$work/target-create.json")
jq -e --arg id "$target_id" --arg class "$target_class" '.result.blueprintId == $id and .result.generatedClass == $class' "$work/target-create.json" >/dev/null
mutate "$work/target-root.json" blueprint.scs_component_ensure "$target_rev" "{\"blueprintId\":\"$target_id\",\"name\":\"InteractionRoot\",\"class\":\"SceneComponent\"}"
target_root_guid=$(jq -r .result.variableGuid "$work/target-root.json"); target_rev=$(jq -r .result.revision "$work/target-root.json")
mutate "$work/target-box-first.json" blueprint.scs_component_ensure "$target_rev" "{\"blueprintId\":\"$target_id\",\"name\":\"OverlapBox\",\"class\":\"BoxComponent\",\"parent\":\"$target_root_guid\"}"
removed_target_box_guid=$(jq -r .result.variableGuid "$work/target-box-first.json"); target_rev=$(jq -r .result.revision "$work/target-box-first.json")
mutate "$work/target-box-first-update.json" blueprint.scs_component_update "$target_rev" "{\"blueprintId\":\"$target_id\",\"variableGuid\":\"$removed_target_box_guid\",\"collisionEnabled\":\"QueryOnly\",\"collisionProfile\":\"OverlapAllDynamic\",\"generateOverlapEvents\":true,\"boxExtent\":[100,100,100]}"
target_rev=$(jq -r .result.revision "$work/target-box-first-update.json")
retry_mutation "$work/target-box-remove-dry-run.json" capability execute blueprint.scs_component_remove --input-json "{\"blueprintId\":\"$target_id\",\"variableGuid\":\"$removed_target_box_guid\",\"force\":true,\"dryRun\":true}"
jq -e --arg rev "$target_rev" '.result.changed == false and .result.dryRun == true and .result.revision == $rev and .receipt.verification.matched == true' "$work/target-box-remove-dry-run.json" >/dev/null
validate_receipt "$work/target-box-remove-dry-run.json"
mutate "$work/target-box-remove.json" blueprint.scs_component_remove "$target_rev" "{\"blueprintId\":\"$target_id\",\"variableGuid\":\"$removed_target_box_guid\",\"force\":true,\"dryRun\":false}"
target_rev=$(jq -r .result.revision "$work/target-box-remove.json")
axi capability execute blueprint.scs_view --input-json "{\"blueprintId\":\"$target_id\"}" >"$work/target-scs-after-remove.json"
jq -e --arg removed "$removed_target_box_guid" --arg root "$target_root_guid" --arg rev "$target_rev" '.revision == $rev and any(.components[]; .variableGuid == $root) and all(.components[]; .variableGuid != $removed)' "$work/target-scs-after-remove.json" >/dev/null
mutate "$work/target-box.json" blueprint.scs_component_ensure "$target_rev" "{\"blueprintId\":\"$target_id\",\"name\":\"OverlapBox\",\"class\":\"BoxComponent\",\"parent\":\"$target_root_guid\"}"
target_box_guid=$(jq -r .result.variableGuid "$work/target-box.json"); target_rev=$(jq -r .result.revision "$work/target-box.json")
[[ "$target_box_guid" != "$removed_target_box_guid" ]]
mutate "$work/target-box-update.json" blueprint.scs_component_update "$target_rev" "{\"blueprintId\":\"$target_id\",\"variableGuid\":\"$target_box_guid\",\"collisionEnabled\":\"QueryOnly\",\"collisionProfile\":\"OverlapAllDynamic\",\"generateOverlapEvents\":true,\"boxExtent\":[100,100,100]}"
target_rev=$(jq -r .result.revision "$work/target-box-update.json")
retry_mutation "$work/target-scs-compile.json" blueprint compile "$target_id" --expected-revision "$target_rev"
jq -e '.result.status != "error" and .result.errorCount == 0 and .receipt.verification.matched == true' "$work/target-scs-compile.json" >/dev/null
validate_receipt "$work/target-scs-compile.json"
target_rev=$(jq -r .result.revision "$work/target-scs-compile.json")
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$target_id\"}" >"$work/target-graphs.json"
target_graph=$(jq -r '.items[] | select(.kind == "ubergraph") | .graphId' "$work/target-graphs.json" | head -1)
[[ -n "$target_graph" ]]
rev=$(jq -r .revision "$work/target-graphs.json")
[[ "$rev" == "$target_rev" ]]
ensure_graph_node "$target_id" "$target_graph" blueprint.event_ensure p12.target.overlap event component.begin_overlap "\"variableGuid\":\"$target_box_guid\""
target_overlap_node=$(jq -r .result.nodeId "$work/p12.target.overlap.json")
ensure_graph_node "$target_id" "$target_graph" blueprint.node_ensure p12.target.message node interface.message_interact "\"interfaceId\":\"$interface_id\""
target_message_node=$(jq -r .result.nodeId "$work/p12.target.message.json")
ensure_graph_node "$target_id" "$target_graph" blueprint.event_ensure p12.target.begin event actor.begin_play
target_begin_node=$(jq -r .result.nodeId "$work/p12.target.begin.json")
ensure_graph_node "$target_id" "$target_graph" blueprint.event_ensure p12.target.input event input.key_e
target_input_node=$(jq -r .result.nodeId "$work/p12.target.input.json")
ensure_graph_node "$target_id" "$target_graph" blueprint.node_ensure p12.target.controller node game.get_player_controller
target_controller_node=$(jq -r .result.nodeId "$work/p12.target.controller.json")
ensure_graph_node "$target_id" "$target_graph" blueprint.node_ensure p12.target.enable node actor.enable_input
target_enable_node=$(jq -r .result.nodeId "$work/p12.target.enable.json")
ensure_graph_node "$target_id" "$target_graph" blueprint.node_ensure p12.target.vector node math.make_vector
target_vector_node=$(jq -r .result.nodeId "$work/p12.target.vector.json")
ensure_graph_node "$target_id" "$target_graph" blueprint.node_ensure p12.target.offset node actor.add_world_offset
target_offset_node=$(jq -r .result.nodeId "$work/p12.target.offset.json")
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$target_id\",\"graphId\":\"$target_graph\"}" >"$work/target-nodes.json"
target_overlap_then=$(pin "$work/target-nodes.json" "$target_overlap_node" then output)
target_overlap_other=$(pin "$work/target-nodes.json" "$target_overlap_node" OtherActor output)
target_message_exec=$(pin "$work/target-nodes.json" "$target_message_node" execute input)
target_message_target=$(pin "$work/target-nodes.json" "$target_message_node" self input)
target_begin_then=$(pin "$work/target-nodes.json" "$target_begin_node" then output)
target_input_pressed=$(pin "$work/target-nodes.json" "$target_input_node" Pressed output)
target_controller_index=$(pin "$work/target-nodes.json" "$target_controller_node" PlayerIndex input)
target_controller_return=$(pin "$work/target-nodes.json" "$target_controller_node" ReturnValue output)
target_enable_exec=$(pin "$work/target-nodes.json" "$target_enable_node" execute input)
target_enable_controller=$(pin "$work/target-nodes.json" "$target_enable_node" PlayerController input)
target_vector_x=$(pin "$work/target-nodes.json" "$target_vector_node" X input)
target_vector_y=$(pin "$work/target-nodes.json" "$target_vector_node" Y input)
target_vector_z=$(pin "$work/target-nodes.json" "$target_vector_node" Z input)
target_vector_return=$(pin "$work/target-nodes.json" "$target_vector_node" ReturnValue output)
target_offset_exec=$(pin "$work/target-nodes.json" "$target_offset_node" execute input)
target_offset_delta=$(pin "$work/target-nodes.json" "$target_offset_node" DeltaLocation input)
for required in "$target_overlap_then" "$target_overlap_other" "$target_message_exec" "$target_message_target" "$target_begin_then" "$target_input_pressed" "$target_controller_index" "$target_controller_return" "$target_enable_exec" "$target_enable_controller" "$target_vector_x" "$target_vector_y" "$target_vector_z" "$target_vector_return" "$target_offset_exec" "$target_offset_delta"; do [[ -n "$required" ]]; done
set_default "$target_id" "$target_controller_index" 0 "$work/default-target-player-index.json" integer
set_default "$target_id" "$target_vector_x" 200 "$work/default-target-x.json"
set_default "$target_id" "$target_vector_y" 0 "$work/default-target-y.json"
set_default "$target_id" "$target_vector_z" 0 "$work/default-target-z.json"
connect "$target_id" "$target_overlap_then" "$target_message_exec" "$work/connect-target-overlap-message.json"
connect "$target_id" "$target_overlap_other" "$target_message_target" "$work/connect-target-other-message.json"
connect "$target_id" "$target_begin_then" "$target_enable_exec" "$work/connect-target-begin-enable.json"
connect "$target_id" "$target_controller_return" "$target_enable_controller" "$work/connect-target-controller-enable.json"
connect "$target_id" "$target_input_pressed" "$target_offset_exec" "$work/connect-target-input-offset.json"
connect "$target_id" "$target_vector_return" "$target_offset_delta" "$work/connect-target-vector-offset.json"
target_rev=$rev
retry_mutation "$work/target-compile.json" blueprint compile "$target_id" --expected-revision "$target_rev"
jq -e '.result.status != "error" and .result.errorCount == 0 and .result.warningCount == 0 and .receipt.verification.matched == true' "$work/target-compile.json" >/dev/null
validate_receipt "$work/target-compile.json"
target_rev=$(jq -r .result.revision "$work/target-compile.json")
retry_mutation "$work/target-save.json" asset save "$target_id" --expected-revision "$target_rev"
jq -e '.receipt.state == "completed" and .receipt.verification.matched == true' "$work/target-save.json" >/dev/null
validate_receipt "$work/target-save.json"

retry_mutation "$work/player-create.json" capability execute blueprint.create --input-json "{\"path\":\"$player_path\",\"parentClass\":\"/Script/Engine.Actor\"}"
assert_changed "$work/player-create.json"
player_rev=$(jq -r .result.revision "$work/player-create.json")
jq -e --arg id "$player_id" --arg class "$player_class" '.result.blueprintId == $id and .result.generatedClass == $class' "$work/player-create.json" >/dev/null
mutate "$work/player-interface.json" blueprint.interface_ensure "$player_rev" "{\"blueprintId\":\"$player_id\",\"interfaceId\":\"$interface_id\"}"
player_rev=$(jq -r .result.revision "$work/player-interface.json")
mutate "$work/player-root.json" blueprint.scs_component_ensure "$player_rev" "{\"blueprintId\":\"$player_id\",\"name\":\"InteractionRoot\",\"class\":\"SceneComponent\"}"
player_root_guid=$(jq -r .result.variableGuid "$work/player-root.json"); player_rev=$(jq -r .result.revision "$work/player-root.json")
mutate "$work/player-box.json" blueprint.scs_component_ensure "$player_rev" "{\"blueprintId\":\"$player_id\",\"name\":\"InteractionBox\",\"class\":\"BoxComponent\",\"parent\":\"$player_root_guid\"}"
player_box_guid=$(jq -r .result.variableGuid "$work/player-box.json"); player_rev=$(jq -r .result.revision "$work/player-box.json")
mutate "$work/player-box-update.json" blueprint.scs_component_update "$player_rev" "{\"blueprintId\":\"$player_id\",\"variableGuid\":\"$player_box_guid\",\"collisionEnabled\":\"QueryOnly\",\"collisionProfile\":\"OverlapAllDynamic\",\"generateOverlapEvents\":true,\"boxExtent\":[100,100,100]}"
player_rev=$(jq -r .result.revision "$work/player-box-update.json")
retry_mutation "$work/player-scs-compile.json" blueprint compile "$player_id" --expected-revision "$player_rev"
jq -e '.result.status != "error" and .result.errorCount == 0 and .receipt.verification.matched == true' "$work/player-scs-compile.json" >/dev/null
validate_receipt "$work/player-scs-compile.json"
player_rev=$(jq -r .result.revision "$work/player-scs-compile.json")
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$player_id\"}" >"$work/player-graphs.json"
player_graph=$(jq -r '.items[] | select(.kind == "ubergraph") | .graphId' "$work/player-graphs.json" | head -1)
[[ -n "$player_graph" ]]
rev=$(jq -r .revision "$work/player-graphs.json")
[[ "$rev" == "$player_rev" ]]
ensure_graph_node "$player_id" "$player_graph" blueprint.event_ensure p12.player.interact event interface.interact "\"interfaceId\":\"$interface_id\""
player_interact_node=$(jq -r .result.nodeId "$work/p12.player.interact.json")
ensure_graph_node "$player_id" "$player_graph" blueprint.node_ensure p12.player.vector node math.make_vector
player_vector_node=$(jq -r .result.nodeId "$work/p12.player.vector.json")
ensure_graph_node "$player_id" "$player_graph" blueprint.node_ensure p12.player.offset node actor.add_world_offset
player_offset_node=$(jq -r .result.nodeId "$work/p12.player.offset.json")
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$player_id\",\"graphId\":\"$player_graph\"}" >"$work/player-nodes.json"
player_interact_then=$(pin "$work/player-nodes.json" "$player_interact_node" then output)
player_vector_x=$(pin "$work/player-nodes.json" "$player_vector_node" X input)
player_vector_y=$(pin "$work/player-nodes.json" "$player_vector_node" Y input)
player_vector_z=$(pin "$work/player-nodes.json" "$player_vector_node" Z input)
player_vector_return=$(pin "$work/player-nodes.json" "$player_vector_node" ReturnValue output)
player_offset_exec=$(pin "$work/player-nodes.json" "$player_offset_node" execute input)
player_offset_delta=$(pin "$work/player-nodes.json" "$player_offset_node" DeltaLocation input)
for required in "$player_interact_then" "$player_vector_x" "$player_vector_y" "$player_vector_z" "$player_vector_return" "$player_offset_exec" "$player_offset_delta"; do [[ -n "$required" ]]; done
set_default "$player_id" "$player_vector_x" 100 "$work/default-player-x.json"
set_default "$player_id" "$player_vector_y" 0 "$work/default-player-y.json"
set_default "$player_id" "$player_vector_z" 0 "$work/default-player-z.json"
connect "$player_id" "$player_interact_then" "$player_offset_exec" "$work/connect-player-interact-offset.json"
connect "$player_id" "$player_vector_return" "$player_offset_delta" "$work/connect-player-vector-offset.json"
player_rev=$rev
retry_mutation "$work/player-compile.json" blueprint compile "$player_id" --expected-revision "$player_rev"
jq -e '.result.status != "error" and .result.errorCount == 0 and .result.warningCount == 0 and .receipt.verification.matched == true' "$work/player-compile.json" >/dev/null
validate_receipt "$work/player-compile.json"
player_rev=$(jq -r .result.revision "$work/player-compile.json")
retry_mutation "$work/player-save.json" asset save "$player_id" --expected-revision "$player_rev"
jq -e '.receipt.state == "completed" and .receipt.verification.matched == true' "$work/player-save.json" >/dev/null
validate_receipt "$work/player-save.json"

settings_rev=$(axi level settings --level-id "$level" | jq -r .revision)
retry_mutation "$work/game-mode.json" level set-game-mode --level-id "$level" --class /Script/Engine.GameModeBase --expected-revision "$settings_rev"
retry_mutation "$work/spawn-player.json" actor spawn --level "$level" --class "$player_class" --agent-key p12-player --label 'P1.2 Interaction Player' --location 1250,0,0
player_actor=$(jq -r .result.id "$work/spawn-player.json")
retry_mutation "$work/spawn-target.json" actor spawn --level "$level" --class "$target_class" --agent-key p12-target --label 'P1.2 Interaction Target' --location 1000,0,0
target_actor=$(jq -r .result.id "$work/spawn-target.json")
[[ -n "$target_actor" && -n "$player_actor" && "$target_actor" != "$player_actor" ]]
retry_mutation "$work/level-save.json" level save --path "$level"
for mutation in "$work/game-mode.json" "$work/spawn-target.json" "$work/spawn-player.json" "$work/level-save.json"; do
  jq -e '.receipt.state == "completed" and .receipt.verification.matched == true' "$mutation" >/dev/null
  validate_receipt "$mutation"
done
stop_editor
sleep 1

start_editor restart
retry_mutation "$work/level-open-restart.json" level open --path "$level"
axi capability execute blueprint.interface_view --input-json "{\"id\":\"$interface_id\"}" >"$work/interface-restart.json"
jq -e --arg id "$interface_id" '.id == $id and .function == "Interact"' "$work/interface-restart.json" >/dev/null
axi capability execute blueprint.scs_view --input-json "{\"blueprintId\":\"$target_id\"}" >"$work/target-scs-restart.json"
jq -e --arg root "$target_root_guid" --arg box "$target_box_guid" --arg removed "$removed_target_box_guid" 'any(.components[]; .variableGuid == $root and .parent == null) and any(.components[]; .variableGuid == $box and .parent == $root and .collisionEnabled == "QueryOnly" and .collisionProfile == "OverlapAllDynamic" and .generateOverlapEvents == true and .boxExtent == [100,100,100]) and all(.components[]; .variableGuid != $removed)' "$work/target-scs-restart.json" >/dev/null
axi capability execute blueprint.scs_view --input-json "{\"blueprintId\":\"$player_id\"}" >"$work/player-scs-restart.json"
jq -e --arg root "$player_root_guid" --arg box "$player_box_guid" 'any(.components[]; .variableGuid == $root and .parent == null) and any(.components[]; .variableGuid == $box and .parent == $root and .collisionEnabled == "QueryOnly" and .collisionProfile == "OverlapAllDynamic" and .generateOverlapEvents == true and .boxExtent == [100,100,100])' "$work/player-scs-restart.json" >/dev/null
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$target_id\",\"graphId\":\"$target_graph\"}" >"$work/target-graph-restart.json"
jq -e --arg overlap "$target_overlap_node" --arg message "$target_message_node" --arg a "$target_message_exec" --arg b "$target_message_target" '.items as $nodes | any($nodes[]; .nodeId == $overlap and any(.pins[]; .links | index($a))) and any($nodes[]; .nodeId == $overlap and any(.pins[]; .links | index($b))) and any($nodes[]; .nodeId == $message)' "$work/target-graph-restart.json" >/dev/null
axi capability execute blueprint.graph_view --input-json "{\"blueprintId\":\"$player_id\",\"graphId\":\"$player_graph\"}" >"$work/player-graph-restart.json"
jq -e --arg handler "$player_interact_node" --arg vector "$player_vector_node" --arg offset "$player_offset_node" --arg exec "$player_offset_exec" --arg delta "$player_offset_delta" '.items as $nodes | any($nodes[]; .nodeId == $handler and any(.pins[]; .links | index($exec))) and any($nodes[]; .nodeId == $vector and any(.pins[]; .name == "X" and .direction == "input" and (.defaultValue | tonumber) == 100) and any(.pins[]; .name == "Y" and .direction == "input" and (.defaultValue | tonumber) == 0) and any(.pins[]; .name == "Z" and .direction == "input" and (.defaultValue | tonumber) == 0) and any(.pins[]; .links | index($delta))) and any($nodes[]; .nodeId == $offset)' "$work/player-graph-restart.json" >/dev/null
axi blueprint view "$target_id" >"$work/target-view-restart.json"
axi blueprint view "$player_id" >"$work/player-view-restart.json"
jq -e --arg t "$target_class" '.generatedClass == $t and .status != "error" and .errorCount == 0' "$work/target-view-restart.json" >/dev/null
jq -e --arg p "$player_class" '.generatedClass == $p and .status != "error" and .errorCount == 0' "$work/player-view-restart.json" >/dev/null

run_pie() {
  local label=$1 sid target_out player_out
  target_out="$work/component-target-$label.json"
  player_out="$work/component-player-$label.json"
  retry_mutation "$work/play-$label-start.json" play start
  sid=$(jq -r .result.sessionId "$work/play-$label-start.json")
  jq -e '.receipt.state == "completed" and .receipt.verification.matched == true' "$work/play-$label-start.json" >/dev/null
  validate_receipt "$work/play-$label-start.json"
  for _ in $(seq 1 120); do axi play status --session-id "$sid" >"$work/play-$label-status.json"; [[ $(jq -r .state "$work/play-$label-status.json") == running ]] && break; sleep .2; done
  [[ $(jq -r .state "$work/play-$label-status.json") == running ]]
  axi play observe --session-id "$sid" >"$work/play-observe-$label.json"
  jq -e --arg target "$target_class" --arg player "$player_class" 'any(.actors[]; .class == $target) and any(.actors[]; .class == $player) and $target != $player' "$work/play-observe-$label.json" >/dev/null
  axi capability execute play.component_observe --input-json "{\"sessionId\":\"$sid\",\"actorId\":\"$target_actor\",\"variableGuid\":\"$target_box_guid\"}" >"$work/component-target-$label-before.json"
  axi capability execute play.component_observe --input-json "{\"sessionId\":\"$sid\",\"actorId\":\"$player_actor\",\"variableGuid\":\"$player_box_guid\"}" >"$work/component-player-$label-before.json"
  jq -e '.resolved == true and .overlapCount == 0 and .overlappingActorIds == [] and .interactionDisplacement == [0,0,0]' "$work/component-target-$label-before.json" >/dev/null
  jq -e '.resolved == true and .overlapCount == 0 and .overlappingActorIds == [] and .interactionDisplacement == [0,0,0]' "$work/component-player-$label-before.json" >/dev/null
  retry_mutation "$work/play-$label-input-pressed.json" play input E --session-id "$sid" --event pressed
  jq -e '.result.accepted == true and .result.changed == true and .receipt.state == "completed" and .receipt.verification.matched == true' "$work/play-$label-input-pressed.json" >/dev/null
  validate_receipt "$work/play-$label-input-pressed.json"
  local matched=false
  for _ in $(seq 1 100); do
    axi capability execute play.component_observe --input-json "{\"sessionId\":\"$sid\",\"actorId\":\"$target_actor\",\"variableGuid\":\"$target_box_guid\"}" >"$target_out"
    axi capability execute play.component_observe --input-json "{\"sessionId\":\"$sid\",\"actorId\":\"$player_actor\",\"variableGuid\":\"$player_box_guid\"}" >"$player_out"
    if jq -e --arg player "$player_actor" '.resolved == true and .reason == null and .componentName == "OverlapBox" and .componentClass == "/Script/Engine.BoxComponent" and .collisionEnabled == "QueryOnly" and .collisionProfile == "OverlapAllDynamic" and .generateOverlapEvents == true and .overlapCount == 1 and .overlappingActorIds == [$player] and .interactionDisplacement == [200,0,0]' "$target_out" >/dev/null && jq -e '.resolved == true and .reason == null and .componentName == "InteractionBox" and .componentClass == "/Script/Engine.BoxComponent" and .collisionEnabled == "QueryOnly" and .collisionProfile == "OverlapAllDynamic" and .generateOverlapEvents == true and .interactionDisplacement == [100,0,0]' "$player_out" >/dev/null; then matched=true; break; fi
    sleep .05
  done
  [[ "$matched" == true ]]
  retry_mutation "$work/play-$label-input-released.json" play input E --session-id "$sid" --event released
  jq -e '.result.accepted == true and .result.changed == false and .receipt.state == "completed" and .receipt.verification.matched == true' "$work/play-$label-input-released.json" >/dev/null
  validate_receipt "$work/play-$label-input-released.json"
  retry_mutation "$work/play-$label-stop.json" play stop --session-id "$sid"
  jq -e '.receipt.state == "completed" and .receipt.verification.matched == true' "$work/play-$label-stop.json" >/dev/null
  validate_receipt "$work/play-$label-stop.json"
}
run_pie one
run_pie two
jq -s -e '.[0].actorLocation == .[1].actorLocation and .[0].interactionDisplacement == [100,0,0] and .[1].interactionDisplacement == [100,0,0]' "$work/component-player-one.json" "$work/component-player-two.json" >/dev/null
jq -s -e '.[0].overlapCount == 1 and .[1].overlapCount == 1 and .[0].overlappingActorIds == .[1].overlappingActorIds' "$work/component-target-one.json" "$work/component-target-two.json" >/dev/null
stop_editor

if [[ -n "$package_project_dir" ]]; then
  package_project="$package_project_dir/MagiUnrealAXIPackageFixture.uproject"
  [[ -d "$package_project_dir" && -f "$package_project" ]]
  jq -e 'has("Modules") | not' "$package_project" >/dev/null
  for forbidden in Source Modules Plugins Binaries Intermediate Saved DDC DerivedDataCache; do [[ ! -e "$package_project_dir/$forbidden" ]]; done
  source_content="$project_dir/Content/$content_root"
  destination_content="$package_project_dir/Content/$content_root"
  [[ -d "$source_content" && ! -e "$destination_content" ]]
  mkdir -p "$(dirname "$destination_content")"
  ditto "$source_content" "$destination_content"
  while IFS= read -r required; do [[ -f "$package_project_dir/Content/$required" ]]; done < <(jq -r '.packageAssertions.interface.file,.packageAssertions.targetBlueprint.file,.packageAssertions.playerBlueprint.file,.packageAssertions.map' "$manifest")
  hash_tree() { local root=$1; (cd "$root" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s\t%s\n' "${file#./}" "$(shasum -a 256 "$file" | cut -d' ' -f1)"; done) | shasum -a 256 | cut -d' ' -f1; }
  for root in "$source_content" "$destination_content"; do [[ -z $(find "$root" ! -type d ! -type f -print -quit) ]]; done
  source_count=$(find "$source_content" -type f | wc -l | tr -d ' ')
  destination_count=$(find "$destination_content" -type f | wc -l | tr -d ' ')
  source_content_hash=$(hash_tree "$source_content")
  destination_content_hash=$(hash_tree "$destination_content")
  [[ "$source_count" == "$destination_count" && "$source_content_hash" == "$destination_content_hash" ]]
  printf 'contentRoot=%s\nsourceEntryCount=%s\ndestinationEntryCount=%s\nsourceSha256=%s\ndestinationSha256=%s\n' "$content_root" "$source_count" "$destination_count" "$source_content_hash" "$destination_content_hash" >"$evidence/package-materialization.txt"
fi

for secret in "${tokens[@]}"; do
  set +e; grep -R -I -Fq -- "$secret" "$work"; secret_status=$?; set -e
  [[ $secret_status == 1 ]]
done
scan_for_runtime_material "$work"
cp "$work"/*.json "$evidence/"
cp "$work/catalog.txt" "$evidence/"
artifact=$(find "$plugin_dir" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit)
[[ -n "$artifact" ]]
artifact_hash=$(shasum -a 256 "$artifact" | cut -d' ' -f1)
arches=$(lipo -archs "$artifact")
for arch in $(jq -r '.plugin.requiredArchitectures[]' "$manifest"); do grep -qw "$arch" <<<"$arches"; done
[[ $(wc -w <<<"$arches" | tr -d ' ') == 2 ]]
printf 'phase=P1.2\nengineVersion=%s\nengineChangelist=%s\nhostArchitecture=%s\ncatalogHash=%s\nartifactSha256=%s\ncliSha256=%s\npluginArchitectures=%s\nfixture=one-interface-two-distinct-actor-blueprints\nscs=guid-hierarchy-removal-recreate-restart-passed\ngraphs=overlap-message-and-interface-offset-wiring-persisted\noverlap=one\ndisplacement=[100,0,0]\npieSessions=2-deterministic-reset\ntokenScan=passed\n' "$(jq -r .engine.version "$manifest")" "$(jq -r .engine.changelist "$manifest")" "$(uname -m)" "$catalog_hash" "$artifact_hash" "$cli_hash" "$arches" | tee "$evidence/summary.txt"
for secret in "${tokens[@]}"; do
  set +e; grep -R -I -Fq -- "$secret" "$evidence"; secret_status=$?; set -e
  [[ $secret_status == 1 ]]
done
scan_for_runtime_material "$evidence"
if [[ -z "${P12_LIVE_EVIDENCE_DIR:-}" ]]; then printf '%s\n' "$evidence" >"$cache_root/latest"; fi
echo "P1.2 live certification: PASS (evidence retained at $evidence)"
