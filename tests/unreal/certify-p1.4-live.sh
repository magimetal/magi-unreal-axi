#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.4-manifest.json"
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -f "$manifest" && -x "$editor" && -x "$run_uat" && -f "$version_file" ]]

jq -e '.phase == "P1.4" and (.catalog.count == 70) and (.nativeTests|length) == 6 and
  (.p13RegressionTests|length) == 27 and (.automationTests|length) == 33 and
  .automationTests == (.p13RegressionTests + .nativeTests) and
  .fixture.blackboardKey == {name:"TargetActor",keyType:"Actor"} and
  .fixture.targetRefreshLocation == [-600,0,100] and .fixture.arrivalDistanceTolerance == 50 and .fixture.arrivalTimeoutSeconds == 120 and
  .fixture.behaviorTreeLinks == [
    {parentNodeId:"root",childNodeId:"loop",childIndex:0},
    {parentNodeId:"loop",childNodeId:"move",childIndex:0},
    {parentNodeId:"loop",childNodeId:"wait",childIndex:1}
  ] and .pipeline.blueprintOnly and .pipeline.compileAllBlueprints and .pipeline.cook and .pipeline.package' "$manifest" >/dev/null
[[ $(uname -m) == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == "$(jq -r .engine.changelist "$manifest")" ]]
[[ "$(plutil -extract MajorVersion raw -o - "$version_file").$(plutil -extract MinorVersion raw -o - "$version_file").$(plutil -extract PatchVersion raw -o - "$version_file")" == "$(jq -r .engine.version "$manifest")" ]]

account_home=${P14_ACCOUNT_HOME:-$HOME}
cache_root="$account_home/Library/Caches/magi-unreal-axi/p1.4/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
if [[ -n "${P14_LIVE_EVIDENCE_DIR:-}" ]]; then
  evidence=$P14_LIVE_EVIDENCE_DIR
  [[ ! -e "$evidence" ]]
  mkdir -p "$evidence"
else
  evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
fi
project="$work/project/MagiUnrealAXIFixture.uproject"
project_dir=$(dirname "$project")
bin=${P14_CLI_PATH:-$work/magi-unreal-axi}
plugin_dir=${P14_PLUGIN_DIR:-$work/plugin}
package_project_dir=${P14_PACKAGE_PROJECT_DIR:-}
package_claim=not-requested
pid=
session=
tokens=()

level=$(jq -r .fixture.level "$manifest")
bb_path=$(jq -r .fixture.blackboard "$manifest")
bt_path=$(jq -r .fixture.behaviorTree "$manifest")
controller_path=$(jq -r .fixture.controller "$manifest")
pawn_path=$(jq -r .fixture.pawn "$manifest")
floor_path=$(jq -r .fixture.floor "$manifest")
bounds_agent=$(jq -r .fixture.bounds.agentKey "$manifest")
bounds_location=$(jq -c .fixture.bounds.location "$manifest")
pawn_location=$(jq -c .fixture.pawnLocation "$manifest")
target_location=$(jq -c .fixture.targetLocation "$manifest")
target_refresh_location=$(jq -c .fixture.targetRefreshLocation "$manifest")
arrival_tolerance=$(jq -r .fixture.arrivalDistanceTolerance "$manifest")
arrival_timeout_seconds=$(jq -r .fixture.arrivalTimeoutSeconds "$manifest")
bounds_extent=$(jq -c .fixture.bounds.extent "$manifest")
expected_catalog_count=${P14_CATALOG_COUNT:-$(jq -r .catalog.count "$manifest")}
expected_catalog_hash=${P14_CATALOG_HASH:-$(jq -r .catalog.sha256 "$manifest")}

export DOTNET_ROOT="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
export PATH="$DOTNET_ROOT:$PATH"
editor_alive() { kill -0 "$1" 2>/dev/null && [[ $(ps -p "$1" -o stat= 2>/dev/null) != Z* ]]; }
axi() { "$bin" --project "$project" --engine "$engine_root" --timeout 180 --format json "$@"; }
axi_timeout() { local timeout=$1; shift; "$bin" --project "$project" --engine "$engine_root" --timeout "$timeout" --format json "$@"; }
retry() {
  local out=$1
  shift
  for _ in $(seq 1 600); do
    if axi "$@" >"$out"; then return 0; fi
    [[ $(jq -r '.error.reason == "unsafe_editor_state" and .error.retryable == true' "$out" 2>/dev/null) == true ]] || return 1
    sleep .2
  done
  return 1
}
receipt() { jq -e '.receipt.state == "completed" and .receipt.verification.matched == true' "$1" >/dev/null; }
view_receipt() {
  local id view
  id=$(jq -r .receipt.operationId "$1")
  view="$work/operation-$id.json"
  axi operation view "$id" >"$view"
  jq -e --slurpfile mutation "$1" '. == $mutation[0].receipt' "$view" >/dev/null
}
verify_mutation() { receipt "$1"; view_receipt "$1"; }
mutate() {
  local out=$1 operation=$2 revision=$3 input=$4
  retry "$out" capability execute "$operation" --expected-revision "$revision" --input-json "$input"
  verify_mutation "$out"
  jq -e '.result.changed == true' "$out" >/dev/null
}
stop_editor() {
  [[ -n "$pid" ]] || return 0
  local p=$pid s=$session
  if editor_alive "$p"; then axi --editor "$p" editor stop >/dev/null 2>&1 || true; fi
  for _ in $(seq 1 200); do editor_alive "$p" || break; sleep .1; done
  if editor_alive "$p"; then kill -TERM "$p" 2>/dev/null || true; fi
  for _ in $(seq 1 100); do editor_alive "$p" || break; sleep .1; done
  if editor_alive "$p"; then kill -KILL "$p" 2>/dev/null || true; fi
  wait "$p" 2>/dev/null || true
  for _ in $(seq 1 100); do [[ ! -e "$s/bridge-v1.json" && ! -e "$s/token" ]] && break; sleep .1; done
  [[ ! -e "$s/bridge-v1.json" && ! -e "$s/token" ]]
  pid=
  session=
}
start_editor() {
  local label=$1
  "$editor" "$project" -unattended -nop4 -nosplash -RenderOffscreen -ResX=640 -ResY=360 -NoSound "-abslog=$work/editor-$label-ue.log" >"$work/editor-$label.log" 2>&1 &
  pid=$!
  session="$runtime_root/$pid"
  for _ in $(seq 1 1200); do
    [[ -f "$session/bridge-v1.json" ]] && break
    editor_alive "$pid" || return 1
    sleep .1
  done
  [[ -f "$session/bridge-v1.json" && $(jq -r .projectPath "$session/bridge-v1.json") == "$canonical_project" ]]
  tokens+=("$(cat "$session/token")")
  for _ in $(seq 1 600); do
    axi --editor "$pid" editor status >"$work/status-$label.json" 2>/dev/null && [[ $(jq -r .editor.state "$work/status-$label.json") == ready ]] && return 0
    sleep .1
  done
  return 1
}
cleanup() {
  local status=$?
  trap - EXIT
  stop_editor || status=1
  if [[ $status != 0 ]]; then
    echo "P1.4 live certification failed; work retained at $work; evidence at $evidence" >&2
    exit "$status"
  fi
  [[ ${KEEP_P14_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

mkdir -p "$project_dir/Plugins"
copy_tracked_tree() { local source=$1 destination=$2 relative; mkdir -p "$destination"; while IFS= read -r -d '' relative; do relative=${relative#"$source"/}; [[ -f "$repo_root/$source/$relative" && ! -L "$repo_root/$source/$relative" ]] || return 1; mkdir -p "$destination/$(dirname "$relative")"; cp -p "$repo_root/$source/$relative" "$destination/$relative"; done < <(git -C "$repo_root" ls-files -z -- "$source/"); }
copy_tracked_tree tests/unreal/MagiUnrealAXIFixture "$project_dir"
catalog_line=$(cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check)
printf '%s\n' "$catalog_line" >"$work/catalog.txt"
catalog_count=$(sed -E 's/^capability catalog: ([0-9]+) records.*/\1/' <<<"$catalog_line")
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' <<<"$catalog_line")
[[ "$catalog_count" == "$expected_catalog_count" && "$catalog_hash" == "$expected_catalog_hash" ]]
if [[ -z "${P14_CLI_PATH:-}" ]]; then
  cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-build.log" 2>&1
  cp "$repo_root/target/release/magi-unreal-axi" "$bin"
fi
chmod 0755 "$bin"
cli_hash=$(shasum -a 256 "$bin" | cut -d' ' -f1)
[[ -z "${P14_CLI_SHA256:-}" || "$cli_hash" == "$P14_CLI_SHA256" ]]
if [[ -z "${P14_PLUGIN_DIR:-}" ]]; then
  "$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$plugin_dir" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
fi
plugin_binary=$(find "$plugin_dir" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit)
[[ -n "$plugin_binary" ]]
artifact_hash=$(shasum -a 256 "$plugin_binary" | cut -d' ' -f1)
[[ -z "${P14_PLUGIN_SHA256:-}" || "$artifact_hash" == "$P14_PLUGIN_SHA256" ]]
arches=$(lipo -archs "$plugin_binary")
for arch in $(jq -r '.plugin.requiredArchitectures[]' "$manifest"); do grep -qw "$arch" <<<"$arches"; done
[[ $(wc -w <<<"$arches" | tr -d ' ') == 2 ]]
mkdir -p "$project_dir/Plugins/MagiUnrealAXI"
ditto "$plugin_dir" "$project_dir/Plugins/MagiUnrealAXI"
copied_plugin_binary=$(find "$project_dir/Plugins/MagiUnrealAXI" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit); [[ -n "$copied_plugin_binary" && $(shasum -a 256 "$copied_plugin_binary" | cut -d' ' -f1) == "$artifact_hash" ]]
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.log" 2>&1
/usr/bin/trash "$project_dir/Plugins/MagiUnrealAXI/Binaries"
ditto "$plugin_dir/Binaries" "$project_dir/Plugins/MagiUnrealAXI/Binaries"
post_build_plugin=$(find "$project_dir/Plugins/MagiUnrealAXI" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit); [[ -n "$post_build_plugin" && $(shasum -a 256 "$post_build_plugin" | cut -d' ' -f1) == "${P14_PLUGIN_SHA256:-$artifact_hash}" ]]
canonical_project=$(cd "$project_dir" && pwd -P)/$(basename "$project")
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
runtime_root="$account_home/Library/Caches/magi-unreal-axi/$project_hash"
find "$runtime_root" -mindepth 1 -maxdepth 1 -type d -exec /usr/bin/trash {} + 2>/dev/null || true

start_editor author
retry "$work/level-create.json" level create --path "$level"
verify_mutation "$work/level-create.json"

bb_create_input=$(jq -cn --arg path "$bb_path" '{path:$path}')
retry "$work/bb-create.json" capability execute blackboard.create --input-json "$bb_create_input"
verify_mutation "$work/bb-create.json"
bb=$(jq -r .result.blackboardId "$work/bb-create.json")
bb_rev=$(jq -r .result.revision "$work/bb-create.json")
bb_key_input=$(jq -cn --arg id "$bb" '{blackboardId:$id,keyName:"TargetActor",keyType:"Actor"}')
mutate "$work/bb-key.json" blackboard.key_ensure "$bb_rev" "$bb_key_input"
bb_rev=$(jq -r .result.revision "$work/bb-key.json")

bt_create_input=$(jq -cn --arg path "$bt_path" --arg bb "$bb" '{path:$path,blackboardId:$bb}')
retry "$work/bt-create.json" capability execute behavior_tree.create --input-json "$bt_create_input"
verify_mutation "$work/bt-create.json"
bt=$(jq -r .result.behaviorTreeId "$work/bt-create.json")
bt_rev=$(jq -r .result.revision "$work/bt-create.json")
for node_id in loop move wait; do
  case $node_id in loop) node_type=sequence;; move) node_type=move_to;; wait) node_type=wait;; esac
  input=$(jq -cn --arg tree "$bt" --arg id "$node_id" --arg type "$node_type" '{behaviorTreeId:$tree,nodeId:$id,nodeType:$type}')
  mutate "$work/bt-node-$node_id.json" behavior_tree.node_ensure "$bt_rev" "$input"
  bt_rev=$(jq -r .result.revision "$work/bt-node-$node_id.json")
done
for link_name in loop-move loop-wait root-loop; do
  case $link_name in loop-move) parent=loop; child=move; index=0;; loop-wait) parent=loop; child=wait; index=1;; root-loop) parent=root; child=loop; index=0;; esac
  input=$(jq -cn --arg tree "$bt" --arg parent "$parent" --arg child "$child" --argjson index "$index" '{behaviorTreeId:$tree,parentNodeId:$parent,childNodeId:$child,childIndex:$index}')
  mutate "$work/bt-link-$link_name.json" behavior_tree.connect "$bt_rev" "$input"
  bt_rev=$(jq -r .result.revision "$work/bt-link-$link_name.json")
done

controller_create_input=$(jq -cn --arg path "$controller_path" '{path:$path,parentClass:"/Script/AIModule.AIController"}')
retry "$work/controller-create.json" capability execute blueprint.create --input-json "$controller_create_input"
verify_mutation "$work/controller-create.json"
controller=$(jq -r .result.blueprintId "$work/controller-create.json")
controller_rev=$(jq -r .result.revision "$work/controller-create.json")
controller_input=$(jq -cn --arg blueprint "$controller" --arg tree "$bt" '{blueprintId:$blueprint,behaviorTreeId:$tree}')
mutate "$work/controller-configure.json" ai.controller_configure "$controller_rev" "$controller_input"
controller_rev=$(jq -r .result.revision "$work/controller-configure.json")

pawn_create_input=$(jq -cn --arg path "$pawn_path" '{path:$path,parentClass:"/Script/Engine.Character"}')
retry "$work/pawn-create.json" capability execute blueprint.create --input-json "$pawn_create_input"
verify_mutation "$work/pawn-create.json"
pawn=$(jq -r .result.blueprintId "$work/pawn-create.json")
pawn_class=$(jq -r .result.generatedClass "$work/pawn-create.json")
pawn_rev=$(jq -r .result.revision "$work/pawn-create.json")
pawn_input=$(jq -cn --arg blueprint "$pawn" --arg controller "$controller" '{blueprintId:$blueprint,controllerBlueprintId:$controller}')
mutate "$work/pawn-configure.json" ai.pawn_configure "$pawn_rev" "$pawn_input"
pawn_rev=$(jq -r .result.revision "$work/pawn-configure.json")

floor_create_input=$(jq -cn --arg path "$floor_path" '{path:$path,parentClass:"/Script/Engine.Actor"}')
retry "$work/floor-create.json" capability execute blueprint.create --input-json "$floor_create_input"
verify_mutation "$work/floor-create.json"
floor=$(jq -r .result.blueprintId "$work/floor-create.json")
floor_class=$(jq -r .result.generatedClass "$work/floor-create.json")
floor_rev=$(jq -r .result.revision "$work/floor-create.json")
floor_root_input=$(jq -cn --arg blueprint "$floor" '{blueprintId:$blueprint,name:"FloorRoot",class:"SceneComponent"}')
mutate "$work/floor-root.json" blueprint.scs_component_ensure "$floor_rev" "$floor_root_input"
floor_root=$(jq -r .result.variableGuid "$work/floor-root.json")
floor_rev=$(jq -r .result.revision "$work/floor-root.json")
floor_ensure_input=$(jq -cn --arg blueprint "$floor" --arg parent "$floor_root" '{blueprintId:$blueprint,name:"FloorBox",class:"BoxComponent",parent:$parent}')
mutate "$work/floor-box.json" blueprint.scs_component_ensure "$floor_rev" "$floor_ensure_input"
floor_box=$(jq -r .result.variableGuid "$work/floor-box.json")
floor_rev=$(jq -r .result.revision "$work/floor-box.json")
floor_update_input=$(jq -cn --arg blueprint "$floor" --arg guid "$floor_box" '{blueprintId:$blueprint,variableGuid:$guid,collisionEnabled:"QueryAndPhysics",collisionProfile:"BlockAll",boxExtent:[1000,1000,25]}')
mutate "$work/floor-box-update.json" blueprint.scs_component_update "$floor_rev" "$floor_update_input"
floor_rev=$(jq -r .result.revision "$work/floor-box-update.json")

retry "$work/controller-compile.json" blueprint compile "$controller" --expected-revision "$controller_rev"
verify_mutation "$work/controller-compile.json"
controller_rev=$(jq -r .result.revision "$work/controller-compile.json")
retry "$work/pawn-compile.json" blueprint compile "$pawn" --expected-revision "$pawn_rev"
verify_mutation "$work/pawn-compile.json"
pawn_rev=$(jq -r .result.revision "$work/pawn-compile.json")
retry "$work/floor-compile.json" blueprint compile "$floor" --expected-revision "$floor_rev"
verify_mutation "$work/floor-compile.json"
floor_rev=$(jq -r .result.revision "$work/floor-compile.json")
for asset_name in bb bt controller pawn floor; do
  case $asset_name in bb) asset=$bb; rev=$bb_rev;; bt) asset=$bt; rev=$bt_rev;; controller) asset=$controller; rev=$controller_rev;; pawn) asset=$pawn; rev=$pawn_rev;; floor) asset=$floor; rev=$floor_rev;; esac
  retry "$work/$asset_name-save.json" asset save "$asset" --expected-revision "$rev"
  verify_mutation "$work/$asset_name-save.json"
  saved_rev=$(jq -r .result.revision "$work/$asset_name-save.json")
  case $asset_name in bb) bb_rev=$saved_rev;; bt) bt_rev=$saved_rev;; controller) controller_rev=$saved_rev;; pawn) pawn_rev=$saved_rev;; floor) floor_rev=$saved_rev;; esac
done

axi blueprint view "$controller" >"$work/controller-view.json"
axi blueprint view "$pawn" >"$work/pawn-view.json"
axi blueprint view "$floor" >"$work/floor-view.json"
for view in controller pawn floor; do jq -e '.status != "error" and .errorCount == 0' "$work/$view-view.json" >/dev/null; done

retry "$work/spawn-floor.json" actor spawn --level "$level" --class "$floor_class" --agent-key p14-floor --label 'P1.4 Floor' --location 0,0,-25
verify_mutation "$work/spawn-floor.json"
retry "$work/spawn-pawn.json" actor spawn --level "$level" --class "$pawn_class" --agent-key p14-pawn --label 'P1.4 AI Pawn' --location "$(jq -r '.fixture.pawnLocation|join(",")' "$manifest")"
verify_mutation "$work/spawn-pawn.json"
pawn_actor=$(jq -r .result.id "$work/spawn-pawn.json")
retry "$work/spawn-target.json" actor spawn --level "$level" --class /Script/Engine.TargetPoint --agent-key p14-target --label 'P1.4 Target' --location "$(jq -r '.fixture.targetLocation|join(",")' "$manifest")"
verify_mutation "$work/spawn-target.json"
target_actor=$(jq -r .result.id "$work/spawn-target.json")
retry "$work/spawn-target-refresh.json" actor spawn --level "$level" --class /Script/Engine.TargetPoint --agent-key p14-target-refresh --label 'P1.4 Target Refresh' --location="$(tr -d '[] ' <<<"$target_refresh_location")"
verify_mutation "$work/spawn-target-refresh.json"
target_refresh_actor=$(jq -r .result.id "$work/spawn-target-refresh.json")
bounds_input=$(jq -cn --arg level "$level" --arg agent "$bounds_agent" --argjson location "$bounds_location" --argjson extent "$bounds_extent" '{levelId:$level,agentKey:$agent,location:$location,extent:$extent}')
retry "$work/bounds.json" capability execute navigation.bounds_ensure --input-json "$bounds_input"
verify_mutation "$work/bounds.json"
bounds_id=$(jq -r .result.boundsId "$work/bounds.json")
bounds_rev=$(jq -r .result.revision "$work/bounds.json")
retry "$work/level-save.json" level save --path "$level"
verify_mutation "$work/level-save.json"

stop_editor
start_editor restart
retry "$work/level-open.json" level open --path "$level"
verify_mutation "$work/level-open.json"

axi capability execute blackboard.view --input-json "$(jq -cn --arg id "$bb" '{blackboardId:$id}')" >"$work/blackboard-restart.json"
axi capability execute behavior_tree.view --input-json "$(jq -cn --arg id "$bt" '{behaviorTreeId:$id}')" >"$work/behavior-tree-restart.json"
jq -e --arg bb "$bb" --arg rev "$bb_rev" '.blackboardId == $bb and .revision == $rev and .keys == [{keyName:"SelfActor",keyType:"Actor"},{keyName:"TargetActor",keyType:"Actor"}]' "$work/blackboard-restart.json" >/dev/null
jq -e --arg bt "$bt" --arg bb "$bb" --arg rev "$bt_rev" '.behaviorTreeId == $bt and .blackboardId == $bb and .revision == $rev and .nodes == [
  {nodeId:"loop",nodeType:"sequence",keyName:null,waitSeconds:null},
  {nodeId:"move",nodeType:"move_to",keyName:"TargetActor",waitSeconds:null},
  {nodeId:"wait",nodeType:"wait",keyName:null,waitSeconds:0.5}
] and (.links|map({parentNodeId,childNodeId,childIndex})) == [
  {parentNodeId:"loop",childNodeId:"move",childIndex:0},
  {parentNodeId:"loop",childNodeId:"wait",childIndex:1},
  {parentNodeId:"root",childNodeId:"loop",childIndex:0}
]' "$work/behavior-tree-restart.json" >/dev/null
axi capability execute blueprint.scs_view --input-json "$(jq -cn --arg id "$floor" '{blueprintId:$id}')" >"$work/floor-scs-restart.json"
jq -e --arg root "$floor_root" --arg box "$floor_box" --arg rev "$floor_rev" '.revision == $rev and (.components|length) == 2 and
  (.components|map(select(.variableGuid == $root and .name == "FloorRoot" and .class == "/Script/Engine.SceneComponent" and .parent == null))|length) == 1 and
  (.components|map(select(.variableGuid == $box and .name == "FloorBox" and .class == "/Script/Engine.BoxComponent" and .parent == $root and .collisionEnabled == "QueryAndPhysics" and .collisionProfile == "BlockAll" and .boxExtent == [1000,1000,25]))|length) == 1' "$work/floor-scs-restart.json" >/dev/null
for view in controller pawn floor; do
  case $view in controller) asset=$controller; rev=$controller_rev;; pawn) asset=$pawn; rev=$pawn_rev;; floor) asset=$floor; rev=$floor_rev;; esac
  axi blueprint view "$asset" >"$work/$view-view-restart.json"
  jq -e --arg rev "$rev" '.status != "error" and .errorCount == 0 and .revision == $rev' "$work/$view-view-restart.json" >/dev/null
done

retry "$work/noop-bb-key.json" capability execute blackboard.key_ensure --expected-revision "$bb_rev" --input-json "$bb_key_input"
verify_mutation "$work/noop-bb-key.json"
jq -e --arg rev "$bb_rev" --arg id "$bb" '.result.changed == false and .result.revision == $rev and .result.blackboardId == $id and .result.keyName == "TargetActor" and .result.keyType == "Actor"' "$work/noop-bb-key.json" >/dev/null
noop_node_input=$(jq -cn --arg tree "$bt" '{behaviorTreeId:$tree,nodeId:"loop",nodeType:"sequence"}')
retry "$work/noop-bt-node.json" capability execute behavior_tree.node_ensure --expected-revision "$bt_rev" --input-json "$noop_node_input"
verify_mutation "$work/noop-bt-node.json"
jq -e --arg rev "$bt_rev" --arg id "$bt" '.result.changed == false and .result.revision == $rev and .result.behaviorTreeId == $id and .result.nodeId == "loop" and .result.nodeType == "sequence"' "$work/noop-bt-node.json" >/dev/null
noop_link_input=$(jq -cn --arg tree "$bt" '{behaviorTreeId:$tree,parentNodeId:"loop",childNodeId:"wait",childIndex:1}')
retry "$work/noop-bt-link.json" capability execute behavior_tree.connect --expected-revision "$bt_rev" --input-json "$noop_link_input"
verify_mutation "$work/noop-bt-link.json"
jq -e --arg rev "$bt_rev" --arg id "$bt" '.result.changed == false and .result.revision == $rev and .result.behaviorTreeId == $id and .result.linkId == "loop->wait" and .result.childIndex == 1' "$work/noop-bt-link.json" >/dev/null
retry "$work/noop-controller.json" capability execute ai.controller_configure --expected-revision "$controller_rev" --input-json "$controller_input"
verify_mutation "$work/noop-controller.json"
jq -e --arg rev "$controller_rev" --arg id "$controller" --arg tree "$bt" '.result.changed == false and .result.revision == $rev and .result.blueprintId == $id and .result.behaviorTreeId == $tree and .result.semantic == "on_possess.run_behavior_tree"' "$work/noop-controller.json" >/dev/null
retry "$work/noop-pawn.json" capability execute ai.pawn_configure --expected-revision "$pawn_rev" --input-json "$pawn_input"
verify_mutation "$work/noop-pawn.json"
jq -e --arg rev "$pawn_rev" --arg id "$pawn" --arg controller "$controller" '.result.changed == false and .result.revision == $rev and .result.blueprintId == $id and .result.controllerBlueprintId == $controller and .result.typedDefaults.autoPossessAI == "PlacedInWorldOrSpawned" and .result.typedDefaults.maxWalkSpeed == 600' "$work/noop-pawn.json" >/dev/null
retry "$work/noop-floor.json" capability execute blueprint.scs_component_update --expected-revision "$floor_rev" --input-json "$floor_update_input"
verify_mutation "$work/noop-floor.json"
jq -e --arg rev "$floor_rev" --arg id "$floor" --arg guid "$floor_box" '.result.changed == false and .result.revision == $rev and .result.blueprintId == $id and .result.variableGuid == $guid and .result.savedPackages == []' "$work/noop-floor.json" >/dev/null
retry "$work/noop-bounds.json" capability execute navigation.bounds_ensure --input-json "$bounds_input"
verify_mutation "$work/noop-bounds.json"
jq -e --arg rev "$bounds_rev" --arg id "$bounds_id" '.result.changed == false and .result.revision == $rev and .result.boundsId == $id' "$work/noop-bounds.json" >/dev/null

retry "$work/nav-build.json" capability execute navigation.build --input-json "$(jq -cn --arg level "$level" '{levelId:$level}')"
verify_mutation "$work/nav-build.json"
ticket=$(jq -r .result.ticketId "$work/nav-build.json")
for _ in $(seq 1 30); do
  axi capability execute navigation.status --input-json "$(jq -cn --arg ticket "$ticket" '{ticketId:$ticket}')" >"$work/nav-status.json"
  [[ $(jq -r .state "$work/nav-status.json") == succeeded ]] && break
  sleep .2
done
jq -e --arg ticket "$ticket" --arg level "$level" '.ticketId == $ticket and .levelId == $level and .terminal == true and .state == "succeeded" and .message == null' "$work/nav-status.json" >/dev/null
axi capability execute navigation.path_query --input-json "$(jq -cn --arg level "$level" --argjson start "$pawn_location" --argjson target "$target_location" '{levelId:$level,start:$start,target:$target}')" >"$work/path.json"
jq -e --arg level "$level" --argjson start "$pawn_location" --argjson target "$target_location" '.levelId == $level and .start == $start and .target == $target and .reachable == true and .partial == false and .pathLength > 0 and (.points|length) >= 2' "$work/path.json" >/dev/null

retry "$work/play-start.json" play start
verify_mutation "$work/play-start.json"
sid=$(jq -r .result.sessionId "$work/play-start.json")
for _ in $(seq 1 120); do
  axi play status --session-id "$sid" >"$work/play-status.json"
  [[ $(jq -r .state "$work/play-status.json") == running ]] && break
  sleep .2
done
[[ $(jq -r .state "$work/play-status.json") == running ]]
target_input=$(jq -cn --arg sid "$sid" --arg pawn "$pawn_actor" --arg target "$target_actor" '{sessionId:$sid,pawnId:$pawn,keyName:"TargetActor",targetActorId:$target}')
retry "$work/target-set.json" capability execute play.ai_target_set --input-json "$target_input"
receipt "$work/target-set.json"
jq -e --arg sid "$sid" --arg pawn "$pawn_actor" --arg target "$target_actor" '.result.sessionId == $sid and .result.pawnId == $pawn and .result.targetActorId == $target and .result.keyName == "TargetActor" and .result.changed == true and .result.restarted == true' "$work/target-set.json" >/dev/null
observe_input=$(jq -cn --arg sid "$sid" --arg pawn "$pawn_actor" '{sessionId:$sid,pawnId:$pawn,keyName:"TargetActor"}')
initial_observed=
arrival_deadline=$((SECONDS + arrival_timeout_seconds))
while (( SECONDS < arrival_deadline )); do
  remaining=$((arrival_deadline - SECONDS))
  if axi_timeout "$remaining" capability execute play.ai_observe --input-json "$observe_input" >"$work/ai-observe-initial.json" 2>/dev/null &&
    jq -e --arg target "$target_actor" --arg tree "$bt" --argjson tolerance "$arrival_tolerance" '.targetActorId == $target and .behaviorTreeId == $tree and (.activeNodeIds|index("wait")) != null and .behavior == "running" and .moveStatus == "reached" and .destination == null and .distanceToTarget <= $tolerance' "$work/ai-observe-initial.json" >/dev/null; then
    initial_observed=true
    break
  fi
  sleep .2
done
[[ $initial_observed == true ]]
view_receipt "$work/target-set.json"
selected_target=$target_refresh_actor
refresh_input=$(jq -cn --arg sid "$sid" --arg pawn "$pawn_actor" --arg target "$selected_target" '{sessionId:$sid,pawnId:$pawn,keyName:"TargetActor",targetActorId:$target}')
retry "$work/target-refresh.json" capability execute play.ai_target_set --input-json "$refresh_input"
receipt "$work/target-refresh.json"
jq -e '.result.changed == true and .result.restarted == true' "$work/target-refresh.json" >/dev/null
observed_target=
for _ in $(seq 1 120); do
  if axi capability execute play.ai_observe --input-json "$observe_input" >"$work/ai-observe.json" 2>/dev/null &&
    jq -e --arg sid "$sid" --arg pawn "$pawn_actor" --arg target "$selected_target" --arg tree "$bt" '.sessionId == $sid and .pawnId == $pawn and .possessed == true and .targetActorId == $target and .blackboardValues == [{keyName:"TargetActor",keyType:"Actor",valueActorId:$target}] and .behaviorTreeId == $tree and (.activeNodeIds|index("move")) != null and .completedNodeIds == [] and .behavior == "running" and .moveStatus == "moving" and .distanceToTarget < 1190' "$work/ai-observe.json" >/dev/null; then
    observed_target=$selected_target
    break
  fi
  sleep .2
done
[[ -n "$observed_target" ]]
view_receipt "$work/target-refresh.json"
jq -e --arg target "$observed_target" --arg tree "$bt" '.possessed == true and .targetActorId == $target and .behaviorTreeId == $tree and .behavior == "running" and (.activeNodeIds|index("move")) != null and .moveStatus == "moving" and .distanceToTarget < 1190' "$work/ai-observe.json" >/dev/null
retry "$work/play-stop.json" play stop --session-id "$sid"
verify_mutation "$work/play-stop.json"
retry "$work/play-start-reset.json" play start
verify_mutation "$work/play-start-reset.json"
reset_sid=$(jq -r .result.sessionId "$work/play-start-reset.json")
[[ $reset_sid != "$sid" ]]
for _ in $(seq 1 120); do
  axi play status --session-id "$reset_sid" >"$work/play-status-reset.json"
  [[ $(jq -r .state "$work/play-status-reset.json") == running ]] && break
  sleep .2
done
[[ $(jq -r .state "$work/play-status-reset.json") == running ]]
reset_observe_input=$(jq -cn --arg sid "$reset_sid" --arg pawn "$pawn_actor" '{sessionId:$sid,pawnId:$pawn,keyName:"TargetActor"}')
reset_observed=
for _ in $(seq 1 120); do
  if axi capability execute play.ai_observe --input-json "$reset_observe_input" >"$work/ai-observe-reset.json" 2>/dev/null &&
    jq -e --arg sid "$reset_sid" --arg pawn "$pawn_actor" --arg tree "$bt" '
      .sessionId == $sid and .pawnId == $pawn and .possessed == true and .targetActorId == null and .targetLocation == null and .distanceToTarget == null and
      .blackboardValues == [{keyName:"TargetActor",keyType:"Actor",valueActorId:null}] and .behaviorTreeId == $tree and .behavior == "running" and .moveStatus == "idle" and .destination == null
    ' "$work/ai-observe-reset.json" >/dev/null; then
    reset_observed=true
    break
  fi
  sleep .2
done
[[ $reset_observed == true ]]
retry "$work/play-stop-reset.json" play stop --session-id "$reset_sid"
verify_mutation "$work/play-stop-reset.json"
stop_editor

if [[ -n "$package_project_dir" ]]; then
  [[ -d "$package_project_dir" && -f "$package_project_dir/MagiUnrealAXIPackageFixture.uproject" ]]
  content_root=$(jq -r .packageAssertions.contentRoot "$manifest")
  destination="$package_project_dir/Content/$content_root"
  [[ ! -e "$destination" ]]
  mkdir -p "$destination"
  ditto "$project_dir/Content/$content_root" "$destination"
  while IFS= read -r required; do [[ -f "$package_project_dir/Content/$required" ]]; done < <(jq -r '.packageAssertions.blackboard.file,.packageAssertions.behaviorTree.file,.packageAssertions.controller.file,.packageAssertions.pawn.file,.packageAssertions.floor.file,.packageAssertions.map' "$manifest")
  (cd "$project_dir/Content/$content_root" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s  %s\n' "$(shasum -a 256 "$file" | cut -d' ' -f1)" "$file"; done) >"$work/source-content.txt"
  (cd "$destination" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s  %s\n' "$(shasum -a 256 "$file" | cut -d' ' -f1)" "$file"; done) >"$work/destination-content.txt"
  diff -u "$work/source-content.txt" "$work/destination-content.txt" >/dev/null
  package_claim="Content/$content_root"
fi

cp "$work"/*.json "$evidence/"
if [[ -n "$package_project_dir" ]]; then cp "$work/source-content.txt" "$work/destination-content.txt" "$evidence/"; fi
scan_runtime_material() {
  local root=$1 status
  [[ -z "$(find "$root" -type f \( -name token -o -name bridge-v1.json \) -print -quit)" ]]
  set +e
  grep -R -I -E -q 'Authorization:[[:space:]]*Bearer[[:space:]]+[A-Za-z0-9._-]+' "$root"
  status=$?
  set -e
  [[ $status == 1 ]]
}
for secret in "${tokens[@]}"; do
  for root_to_scan in "$work" "$evidence" ${P14_WORKSPACE:+"$P14_WORKSPACE"}; do
    set +e
    grep -R -I -Fq -- "$secret" "$root_to_scan"
    secret_status=$?
    set -e
    [[ $secret_status == 1 ]]
  done
done
scan_runtime_material "$work"
scan_runtime_material "$evidence"
[[ -z "${P14_WORKSPACE:-}" ]] || scan_runtime_material "$P14_WORKSPACE"
printf 'phase=P1.4\ncatalogHash=%s\nartifactSha256=%s\ncliSha256=%s\nfixture=ai-navigation-loop\nblackboard=TargetActor:Actor\nbehaviorTree=root-loop-loop-move-loop-wait\nnav=build-succeeded-reachable-nonpartial\npie=target-arrived-within-%s-units-%ss-authored-wait-refreshed-move\nreset=second-session-target-cleared\npackage=%s\ntokenScan=passed\n' "$catalog_hash" "$artifact_hash" "$cli_hash" "$arrival_tolerance" "$arrival_timeout_seconds" "$package_claim" | tee "$evidence/summary.txt"
[[ -z "${P14_LIVE_EVIDENCE_DIR:-}" ]] && printf '%s\n' "$evidence" >"$cache_root/latest"
echo "P1.4 live certification: PASS (evidence retained at $evidence)"
