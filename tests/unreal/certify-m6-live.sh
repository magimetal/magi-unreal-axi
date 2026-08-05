#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -x "$editor" && -x "$editor_cmd" && $(uname -m) == arm64 ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == 56057345 ]]

cache_root="$HOME/Library/Caches/magi-unreal-axi/m6/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
project="$work/project/MagiUnrealAXIFixture.uproject"
project_dir=$(dirname "$project")
level=/Game/MagiM6/M6Interaction
bin="$work/magi-unreal-axi"
pid=
session=
token=
tokens=()

cleanup() {
  local status=$? cleanup_pid=$pid cleanup_session=$session
  trap - EXIT
  if ! stop_editor; then status=1; fi
  if [[ -n "$cleanup_pid" ]] && ! editor_alive "$cleanup_pid" && [[ -e "$cleanup_session" ]]; then
    /usr/bin/trash "$cleanup_session" 2>/dev/null || status=1
    [[ ! -e "$cleanup_session" ]] || status=1
  fi
  [[ ${KEEP_M6_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
  [[ $status == 0 ]] || echo "M6 live failed; logs: $work" >&2
  exit "$status"
}
trap cleanup EXIT

cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check >"$work/catalog-check.txt"
cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-build.log" 2>&1
cp "$repo_root/target/release/magi-unreal-axi" "$bin"
chmod 0755 "$bin"
mkdir -p "$project_dir"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"
mkdir -p "$project_dir/Content"
sentinel="$project_dir/Content/m6-unrelated-sentinel.txt"
printf 'm6 sentinel unchanged\n' >"$sentinel"
sentinel_hash=$(shasum -a 256 "$sentinel" | cut -d' ' -f1)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
"$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/plugin" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
mkdir -p "$project_dir/Plugins/MagiUnrealAXI"
ditto "$work/plugin" "$project_dir/Plugins/MagiUnrealAXI"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.log" 2>&1
canonical_project=$(cd "$project_dir" && pwd -P)/$(basename "$project")
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
runtime_root="$HOME/Library/Caches/magi-unreal-axi/$project_hash"
report_prep="$work/report-blueprint-prep"
mkdir -p "$report_prep"
"$editor_cmd" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound '-ExecCmds=Automation RunTests MagiUnrealAXI.M6.BlueprintDiagnosticsContract' '-TestExit=Automation Test Queue Empty' -ReportOutputPath="$report_prep" -log="$work/blueprint-prep.log" >"$work/blueprint-prep.stdout" 2>&1 &
prep_pid=$!
wait "$prep_pid"
prep_index="$report_prep/index.json"
jq -e '.failed == 0 and .notRun == 0 and .succeeded == 1 and .tests[0].fullTestPath == "MagiUnrealAXI.M6.BlueprintDiagnosticsContract"' "$prep_index" >/dev/null || { echo "blueprint prep automation failed" >&2; exit 1; }
prep_session="$runtime_root/$prep_pid"
for _ in $(seq 1 100); do [[ ! -e "$prep_session/bridge-v1.json" && ! -e "$prep_session/token" ]] && break; sleep .1; done
if [[ -e "$prep_session/bridge-v1.json" || -e "$prep_session/token" ]]; then /usr/bin/trash "$prep_session"; fi
[[ ! -e "$prep_session" ]] || { echo "blueprint prep discovery cleanup failed" >&2; exit 1; }
axi() { "$bin" --project "$project" --engine "$engine_root" --timeout 10 --format json "$@"; }
mutation_when_safe() {
  local output=$1
  shift
  local status reason message
  for _ in $(seq 1 900); do
    set +e
    axi "$@" >"$output"
    status=$?
    set -e
    [[ $status == 0 ]] && return 0
    reason=$(jq -r '.error.reason // empty' "$output" 2>/dev/null || true)
    message=$(jq -r '.error.message // empty' "$output" 2>/dev/null || true)
    if [[ "$reason" != unsafe_editor_state ]] || [[ "$message" != *loading* && "$message" != *compil* ]]; then
      return "$status"
    fi
    sleep .1
  done
  return 1
}
assert_ok() { jq -e 'has("result") and (.error == null or (has("error") | not))' "$1" >/dev/null || { echo "assert_ok failed: $1" >&2; exit 1; }; }
editor_alive() { kill -0 "$1" 2>/dev/null && [[ "$(ps -p "$1" -o stat= 2>/dev/null)" != Z* ]]; }
start_editor() {
  local run=$1 record
  [[ -z "$pid" ]]
  session=
  token=
  "$editor" "$project" -unattended -nop4 -nosplash -RenderOffscreen -ResX=640 -ResY=360 -NoSound -log="$work/editor-$run-ue.log" >"$work/editor-$run.log" 2>&1 &
  pid=$!
  session="$runtime_root/$pid"
  record="$session/bridge-v1.json"
  for _ in $(seq 1 1200); do
    [[ -f "$record" ]] && break
    kill -0 "$pid" 2>/dev/null || { wait "$pid" 2>/dev/null || true; return 1; }
    editor_alive "$pid" || { wait "$pid" 2>/dev/null || true; return 1; }
    sleep .1
  done
  [[ -f "$record" ]]
  [[ $(jq -r .pid "$record") == "$pid" ]]
  [[ $(jq -r .projectPath "$record") == "$canonical_project" ]]
  token=$(cat "$session/token")
  tokens+=("$token")
  for _ in $(seq 1 600); do
    if axi --editor "$pid" editor status >"$work/status-$run.json" 2>/dev/null && [[ $(jq -r .editor.state "$work/status-$run.json") == ready ]]; then return 0; fi
    sleep .1
  done
  return 1
}
stop_editor() {
  [[ -n "$pid" ]] || return 0
  local stopped_pid=$pid stopped_session=$session
  if editor_alive "$stopped_pid"; then
    axi --editor "$stopped_pid" editor stop >"$work/editor-stop-$stopped_pid.json" 2>/dev/null || true
    for _ in $(seq 1 200); do editor_alive "$stopped_pid" || break; sleep .1; done
    if editor_alive "$stopped_pid"; then
      kill -TERM "$stopped_pid" 2>/dev/null || true
      for _ in $(seq 1 200); do editor_alive "$stopped_pid" || break; sleep .1; done
    fi
    if editor_alive "$stopped_pid"; then
      kill -KILL "$stopped_pid" 2>/dev/null || true
      for _ in $(seq 1 100); do editor_alive "$stopped_pid" || break; sleep .1; done
    fi
  fi
  if editor_alive "$stopped_pid"; then
    echo "editor $stopped_pid survived bounded TERM/KILL shutdown" >&2
    return 1
  fi
  wait "$stopped_pid" 2>/dev/null || true
  if kill -0 "$stopped_pid" 2>/dev/null; then
    echo "editor $stopped_pid did not fully exit after wait" >&2
    return 1
  fi
  for _ in $(seq 1 100); do
    [[ ! -e "$stopped_session/bridge-v1.json" && ! -e "$stopped_session/token" ]] && { pid=; session=; return 0; }
    sleep .1
  done
  echo "editor $stopped_pid exited without cleaning runtime discovery under $stopped_session" >&2
  return 1
}

start_editor first
mutation_when_safe "$work/level-create.json" level create --path "$level"
assert_ok "$work/level-create.json"
[[ $(jq -r .result.changed "$work/level-create.json") == true && ! -e "$project_dir/Content/MagiM6/M6Interaction.umap" ]]
mutation_when_safe "$work/move-action.json" capability execute asset.create_input_action --input-json '{"path":"/Game/MagiM6/Input/IA_Move","valueType":"Axis1D"}'
mutation_when_safe "$work/interact-action.json" capability execute asset.create_input_action --input-json '{"path":"/Game/MagiM6/Input/IA_Interact","valueType":"Boolean"}'
mutation_when_safe "$work/mapping.json" capability execute asset.create_input_mapping_context --input-json '{"path":"/Game/MagiM6/Input/IMC_MagiM6","mappings":[{"actionId":"/Game/MagiM6/Input/IA_Move.IA_Move","key":"W"},{"actionId":"/Game/MagiM6/Input/IA_Interact.IA_Interact","key":"E"}]}'
mutation_when_safe "$work/mapping-repeat.json" capability execute asset.create_input_mapping_context --input-json '{"path":"/Game/MagiM6/Input/IMC_MagiM6","mappings":[{"actionId":"/Game/MagiM6/Input/IA_Move.IA_Move","key":"W"},{"actionId":"/Game/MagiM6/Input/IA_Interact.IA_Interact","key":"E"}]}'
[[ $(jq -r .result.changed "$work/mapping-repeat.json") == false ]]
for asset in IA_Move IA_Interact IMC_MagiM6; do
  id="/Game/MagiM6/Input/$asset.$asset"
  case "$asset" in
    IA_Move) revision=$(jq -r .result.revision "$work/move-action.json") ;;
    IA_Interact) revision=$(jq -r .result.revision "$work/interact-action.json") ;;
    IMC_MagiM6) revision=$(jq -r .result.revision "$work/mapping.json") ;;
  esac
  [[ -n "$revision" && "$revision" != null ]] || { echo "missing asset revision: $asset" >&2; exit 1; }
  axi asset save "$id" --expected-revision "$revision" >"$work/$asset-save.json" || { echo "asset save failed: $asset" >&2; exit 1; }
  assert_ok "$work/$asset-save.json"
  axi asset view "$id" >"$work/$asset-view.json" || { echo "asset view failed: $asset" >&2; exit 1; }
  [[ $(jq -r .id "$work/$asset-view.json") == "$id" ]] || { echo "asset view id mismatch: $asset" >&2; exit 1; }
done
mutation_when_safe "$work/actor-spawn.json" actor spawn --level "$level" --class /Script/MagiUnrealAXIFixture.MagiFixtureInteractable --agent-key m6-interactable --label 'M6 Interactable' --location 300,0,0
actor=$(jq -r .result.id "$work/actor-spawn.json"); actor_rev=$(jq -r .result.revision "$work/actor-spawn.json")
mutation_when_safe "$work/component-add.json" component add --actor-id "$actor" --class /Script/Engine.SceneComponent --name M6PersistedComponent --location 10,20,30 --expected-revision "$actor_rev"
axi actor view "$actor" >"$work/actor-after-component-add.json" || { echo "actor read after component add failed" >&2; exit 1; }
[[ $(jq -r .revision "$work/actor-after-component-add.json") != "$actor_rev" ]] || { echo "component add did not invalidate actor revision" >&2; exit 1; }
component=$(jq -r .result.id "$work/component-add.json"); component_rev=$(jq -r .result.revision "$work/component-add.json")
axi component list --actor-id "$actor" >"$work/component-list.json" || { echo "typed component list failed" >&2; exit 1; }
axi capability execute component.list --input-json "{\"actorId\":\"$actor\",\"limit\":100}" >"$work/component-list-generic.json" || { echo "generic component list failed" >&2; exit 1; }
jq -e --arg id "$component" '.items | any(.id == $id)' "$work/component-list.json" >/dev/null || { echo "typed component list omitted added component" >&2; exit 1; }
jq -e --arg id "$component" '.items | any(.id == $id)' "$work/component-list-generic.json" >/dev/null || { echo "generic component list omitted added component" >&2; exit 1; }
mutation_when_safe "$work/component-update.json" component update "$component" --location 40,50,60 --expected-revision "$component_rev"
axi actor view "$actor" >"$work/actor-before-save.json" || { echo "actor read before save failed" >&2; exit 1; }
actor_saved_rev=$(jq -r .revision "$work/actor-before-save.json")
settings_rev=$(axi level settings --level-id "$level" | jq -r .revision) || { echo "level settings read failed" >&2; exit 1; }
axi level set-game-mode --level-id "$level" --class /Script/MagiUnrealAXIFixture.MagiFixtureGameMode --expected-revision "$settings_rev" >"$work/game-mode.json" || { echo "game mode mutation failed" >&2; exit 1; }
mutation_when_safe "$work/level-save.json" level save --path "$level"
[[ -f "$project_dir/Content/MagiM6/M6Interaction.umap" ]]
stop_editor
sleep 2
start_editor restart
mutation_when_safe "$work/level-reopen.json" level open --path "$level"
axi level settings --level-id "$level" >"$work/persistence-settings.json" || { echo "persistence settings read failed" >&2; exit 1; }
axi actor view "$actor" >"$work/persistence-actor.json" || { echo "persistence actor read failed" >&2; exit 1; }
axi component view "$component" >"$work/persistence-component.json" || { echo "persistence component read failed" >&2; exit 1; }
[[ $(jq -r .actorGuid "$work/persistence-actor.json") == "$(jq -r .result.actorGuid "$work/actor-spawn.json")" ]] || { echo "actor persistence mismatch" >&2; exit 1; }
[[ $(jq -r .name "$work/persistence-component.json") == M6PersistedComponent ]] || { echo "component persistence mismatch" >&2; exit 1; }
[[ $(jq -r '.location == [40,50,60]' "$work/persistence-component.json") == true ]] || { echo "component transform persistence mismatch" >&2; exit 1; }
[[ $(jq -r .gameModeClass "$work/persistence-settings.json") == /Script/MagiUnrealAXIFixture.MagiFixtureGameMode ]] || { echo "game mode persistence mismatch" >&2; exit 1; }
[[ $(jq -r .result.revision "$work/level-reopen.json") == $(jq -r .result.revision "$work/level-save.json") ]] || { echo "level revision changed across save/restart/reopen" >&2; exit 1; }
[[ $(jq -r .revision "$work/persistence-actor.json") == "$actor_saved_rev" ]] || { echo "actor revision changed across save/restart/reopen" >&2; exit 1; }
component_rev=$(jq -r .revision "$work/persistence-component.json")
set +e
axi component remove "$component" --expected-revision "$component_rev" >"$work/component-remove-no-force.json"
remove_status=$?
set -e
[[ $remove_status == 2 && $(jq -r .error.reason "$work/component-remove-no-force.json") == force_required ]] || { echo "component remove did not refuse without force" >&2; exit 1; }
axi component remove "$component" --dry-run --expected-revision "$component_rev" >"$work/component-remove-dry-run.json"
[[ $(jq -r .result.changed "$work/component-remove-dry-run.json") == false && $(jq -r .result.dryRun "$work/component-remove-dry-run.json") == true && $(jq -r .result.revision "$work/component-remove-dry-run.json") == "$component_rev" && $(jq -r .receipt.verification.exists "$work/component-remove-dry-run.json") == true ]] || { echo "component remove dry-run mismatch" >&2; exit 1; }
axi component remove "$component" --force --expected-revision "$component_rev" >"$work/component-remove.json"
[[ $(jq -r .result.changed "$work/component-remove.json") == true && $(jq -r .result.revision "$work/component-remove.json") != "$component_rev" && $(jq -r .receipt.verification.exists "$work/component-remove.json") == false ]] || { echo "component remove mismatch" >&2; exit 1; }
set +e
axi component view "$component" >"$work/component-absent.json"
absent_status=$?
set -e
[[ $absent_status == 1 && $(jq -r .error.reason "$work/component-absent.json") == not_found ]] || { echo "removed component readback still exists" >&2; exit 1; }
mutation_when_safe "$work/level-save-remove.json" level save --path "$level"
stop_editor
sleep 2
start_editor remove-restart
mutation_when_safe "$work/level-reopen-remove.json" level open --path "$level"
set +e
axi component view "$component" >"$work/component-absent-restart.json"
absent_restart_status=$?
set -e
[[ $absent_restart_status == 1 && $(jq -r .error.reason "$work/component-absent-restart.json") == not_found ]] || { echo "removed component persisted unexpectedly" >&2; exit 1; }
valid_blueprint=/Game/MagiM6/BP_ValidCompile.BP_ValidCompile
axi blueprint view "$valid_blueprint" >"$work/blueprint-valid-view.json"
valid_rev=$(jq -r .revision "$work/blueprint-valid-view.json")
axi blueprint compile "$valid_blueprint" --expected-revision "$valid_rev" >"$work/blueprint-valid-compile.json"
jq -e --arg id "$valid_blueprint" '.result.id == $id and (.result.revision | length) == 64 and .receipt.verification.readback == "blueprint.view" and .receipt.verification.matched == true and .receipt.verification.observedRevision == .result.revision' "$work/blueprint-valid-compile.json" >/dev/null || { echo "valid blueprint compile receipt mismatch" >&2; exit 1; }
valid_after=$(jq -r .result.revision "$work/blueprint-valid-compile.json")
axi blueprint compile "$valid_blueprint" --expected-revision "$valid_after" >"$work/blueprint-valid-compile-repeat.json"
jq -e --arg rev "$valid_after" '.result.changed == false and .result.revision == $rev and .receipt.revision == $rev and .receipt.verification.observedRevision == $rev' "$work/blueprint-valid-compile-repeat.json" >/dev/null || { echo "valid blueprint repeat was not stable no-op" >&2; exit 1; }
axi asset view "$valid_blueprint" >"$work/blueprint-valid-asset-view.json"
valid_asset_rev=$(jq -r .revision "$work/blueprint-valid-asset-view.json")
axi asset save "$valid_blueprint" --expected-revision "$valid_asset_rev" >"$work/blueprint-valid-save.json"
stop_editor
start_editor blueprint-restart
mutation_when_safe "$work/level-reopen-blueprint.json" level open --path "$level"
axi blueprint view "$valid_blueprint" >"$work/blueprint-valid-restart.json"
[[ $(jq -r .revision "$work/blueprint-valid-restart.json") == "$valid_after" ]] || { echo "valid Blueprint revision did not persist across restart" >&2; exit 1; }
blueprint=/Game/MagiM6/BP_InvalidCompile.BP_InvalidCompile
axi blueprint view "$blueprint" >"$work/blueprint-view.json"
blueprint_rev=$(jq -r .revision "$work/blueprint-view.json")
[[ $(jq -r .id "$work/blueprint-view.json") == "$blueprint" && -n "$blueprint_rev" && "$blueprint_rev" != null ]] || { echo "blueprint view mismatch" >&2; exit 1; }
set +e
axi blueprint compile "$blueprint" --expected-revision "$blueprint_rev" >"$work/blueprint-compile-invalid.json"
blueprint_status=$?
set -e
[[ $blueprint_status == 1 && $(jq -r .error.reason "$work/blueprint-compile-invalid.json") == blueprint_compile_failed && $(jq -r .error.errorCount "$work/blueprint-compile-invalid.json") -gt 0 && $(jq -r '.error.diagnostics | length' "$work/blueprint-compile-invalid.json") -gt 0 && $(jq -r '[.error.diagnostics[] | select(.graph != "" and .nodeGuid != "" and .nodeTitle != "")] | length' "$work/blueprint-compile-invalid.json") -gt 0 ]] || { echo "invalid blueprint compile contract mismatch" >&2; exit 1; }

play_start() { axi play start >"$work/$1-start.json"; jq -r .result.sessionId "$work/$1-start.json"; }
play_wait() { local sid=$1 out=$2 state=; for _ in $(seq 1 120); do axi play status --session-id "$sid" >"$out"; state=$(jq -r .state "$out"); [[ "$state" == running ]] && return 0; sleep .2; done; return 1; }
assert_play_receipt() {
  local file=$1 label=$2 operation=$3 target=$4 transaction=$5 readback=$6 matched=$7 persistence=$8 operation_id view
  jq -e --arg operation "$operation" --arg target "$target" --arg transaction "$transaction" --arg readback "$readback" --arg persistence "$persistence" --argjson matched "$matched" '
    .receipt.operation == $operation and
    .receipt.target == $target and
    .receipt.transaction == $transaction and
    .receipt.reversibility == "none" and
    .receipt.persistence == $persistence and
    .receipt.verification.readback == $readback and
    .receipt.verification.target == $target and
    .receipt.verification.matched == $matched
  ' "$file" >/dev/null || { echo "play receipt mismatch: $label" >&2; exit 1; }
  if [[ "$operation" == play.input ]]; then
    jq -e '.result.accepted == true and (.result.beforeRevision | length) == 64 and (.result.afterRevision | length) == 64 and .result.revision == .result.afterRevision and .receipt.verification.accepted == true and .receipt.verification.beforeRevision == .result.beforeRevision and .receipt.verification.afterRevision == .result.afterRevision and .receipt.verification.observedRevision == .result.afterRevision and .result.changed == (.result.beforeRevision != .result.afterRevision)' "$file" >/dev/null || { echo "play input evidence mismatch: $label" >&2; exit 1; }
  fi
  operation_id=$(jq -r .receipt.operationId "$file")
  view="$work/$label-operation.json"
  axi operation view "$operation_id" >"$view"
  jq -e --slurpfile mutation "$file" '. == $mutation[0].receipt' "$view" >/dev/null || { echo "operation.view receipt mismatch: $label" >&2; exit 1; }
}
sid=$(play_start first-pie); play_wait "$sid" "$work/first-pie-status.json" || { echo "first PIE did not start" >&2; exit 1; }
assert_play_receipt "$work/first-pie-start.json" first-pie-start play.start "$sid" atomic play.status true unchanged
axi play observe --session-id "$sid" >"$work/observe-baseline.json"
axi play input W --session-id "$sid" --event pressed >"$work/input-w-pressed.json"
sleep 2
axi play input W --session-id "$sid" --event released >"$work/input-w-released.json"
axi play observe --session-id "$sid" >"$work/observe-moved.json"
axi play input E --session-id "$sid" --event pressed >"$work/input-e-pressed.json"
sleep .2
axi play input E --session-id "$sid" --event released >"$work/input-e-released.json"
axi play observe --session-id "$sid" >"$work/observe-interacted.json"
assert_play_receipt "$work/input-w-pressed.json" input-w-pressed play.input "$sid#W#pressed" none play.observe true unchanged
assert_play_receipt "$work/input-w-released.json" input-w-released play.input "$sid#W#released" none play.observe true unchanged
assert_play_receipt "$work/input-e-pressed.json" input-e-pressed play.input "$sid#E#pressed" none play.observe true unchanged
assert_play_receipt "$work/input-e-released.json" input-e-released play.input "$sid#E#released" none play.observe true unchanged
[[ $(jq -r .result.accepted "$work/input-w-pressed.json") == true && $(jq -r .result.accepted "$work/input-w-released.json") == true && $(jq -r .result.accepted "$work/input-e-pressed.json") == true && $(jq -r .result.accepted "$work/input-e-released.json") == true ]] || { echo "PIE input was not accepted" >&2; exit 1; }
jq -se --slurpfile baseline "$work/observe-baseline.json" --slurpfile moved "$work/observe-moved.json" --slurpfile interacted "$work/observe-interacted.json" '
  (all(.[]; .result.changed == (.result.beforeRevision != .result.afterRevision))) and
  .[0].result.beforeRevision == $baseline[0].revision and .[0].result.afterRevision == $moved[0].revision and .[0].result.changed == true and
  .[1].result.beforeRevision == $moved[0].revision and .[1].result.afterRevision == $moved[0].revision and .[1].result.changed == false and
  .[2].result.beforeRevision == $moved[0].revision and .[2].result.afterRevision == $interacted[0].revision and .[2].result.changed == true and
  .[3].result.beforeRevision == $interacted[0].revision and .[3].result.afterRevision == $interacted[0].revision and .[3].result.changed == false
' "$work/input-w-pressed.json" "$work/input-w-released.json" "$work/input-e-pressed.json" "$work/input-e-released.json" >/dev/null || { echo "play input operation-specific observation mismatch" >&2; exit 1; }
baseline_pawn_x=$(jq -r '.actors[] | select(.class | endswith("MagiFixturePawn")) | .location[0]' "$work/observe-baseline.json")
moved_pawn_x=$(jq -r '.actors[] | select(.class | endswith("MagiFixturePawn")) | .location[0]' "$work/observe-moved.json")
baseline_interactable=$(jq -c '.actors[] | select(.class | endswith("MagiFixtureInteractable"))' "$work/observe-baseline.json")
interacted=$(jq -c '.actors[] | select(.class | endswith("MagiFixtureInteractable"))' "$work/observe-interacted.json")
jq -n --argjson before "$baseline_pawn_x" --argjson after "$moved_pawn_x" '($after - $before - 100 | fabs) < 0.01' | grep -qx true || { echo "pawn increment was not exactly 100" >&2; exit 1; }
[[ $(jq -r '((.tags | index("MagiM6.Interacted")) == null) and (.location == [300,0,0])' <<<"$baseline_interactable") == true ]] || { echo "interactable baseline was not pristine" >&2; exit 1; }
[[ $(jq -r '((.tags | index("MagiM6.Interacted")) != null) and (.location == [300,0,100])' <<<"$interacted") == true ]] || { echo "interactable did not change exactly once from input" >&2; exit 1; }
axi play screenshot --session-id "$sid" --path m6-live.png >"$work/screenshot.json"
screenshot=$(jq -r .result.path "$work/screenshot.json")
[[ "$screenshot" == "$project_dir/Saved/MagiUnrealAXI/Screenshots/"* && -s "$screenshot" ]] || { echo "screenshot path/file invalid" >&2; exit 1; }
[[ $(jq -r .result.format "$work/screenshot.json") == png && $(jq -r .result.width "$work/screenshot.json") -gt 0 && $(jq -r .result.height "$work/screenshot.json") -gt 0 ]] || { echo "screenshot metadata invalid" >&2; exit 1; }
[[ $(xxd -p -l 8 "$screenshot") == 89504e470d0a1a0a ]] || { echo "screenshot is not PNG" >&2; exit 1; }
bmp="$work/m6-live.bmp"
sips -s format bmp "$screenshot" --out "$bmp" >/dev/null
pixel_offset=$(od -An -t u4 -j 10 -N 4 "$bmp" | tr -d ' ')
od -An -v -t u1 -j "$pixel_offset" "$bmp" | awk '{ for (i = 1; i <= NF; ++i) if ($i != 0) visible = 1 } END { exit visible ? 0 : 1 }' || { echo "screenshot pixels are blank" >&2; exit 1; }
assert_play_receipt "$work/screenshot.json" screenshot play.screenshot "$screenshot" none artifact true saved
axi play stop --session-id "$sid" >"$work/first-pie-stop.json"
for _ in $(seq 1 120); do axi play status --session-id "$sid" >"$work/first-pie-stopped.json" || true; [[ $(jq -r '.state // empty' "$work/first-pie-stopped.json") == stopped ]] && break; sleep .2; done
[[ $(jq -r .state "$work/first-pie-stopped.json") == stopped ]] || { echo "first PIE did not stop" >&2; exit 1; }
assert_play_receipt "$work/first-pie-stop.json" first-pie-stop play.stop "$sid" atomic play.status true unchanged
sid2=$(play_start second-pie); play_wait "$sid2" "$work/second-pie-status.json" || { echo "second PIE did not start" >&2; exit 1; }; axi play observe --session-id "$sid2" >"$work/observe-reset.json"
assert_play_receipt "$work/second-pie-start.json" second-pie-start play.start "$sid2" atomic play.status true unchanged
reset_pawn_x=$(jq -r '.actors[] | select(.class | endswith("MagiFixturePawn")) | .location[0]' "$work/observe-reset.json"); reset_interactable=$(jq -c '.actors[] | select(.class | endswith("MagiFixtureInteractable"))' "$work/observe-reset.json")
jq -n --argjson baseline "$baseline_pawn_x" --argjson reset "$reset_pawn_x" '($baseline - $reset | fabs) < 1' | grep -qx true || { echo "pawn did not reset" >&2; exit 1; }
[[ $(jq -r '(.tags | index("MagiM6.Interacted") == null) and (.location == [300,0,0])' <<<"$reset_interactable") == true ]] || { echo "interactable did not reset" >&2; exit 1; }
axi play stop --session-id "$sid2" >"$work/second-pie-stop.json"
for _ in $(seq 1 120); do axi play status --session-id "$sid2" >"$work/second-pie-stopped.json" || true; [[ $(jq -r '.state // empty' "$work/second-pie-stopped.json") == stopped ]] && break; sleep .2; done
[[ $(jq -r .state "$work/second-pie-stopped.json") == stopped ]] || { echo "second PIE did not stop" >&2; exit 1; }
assert_play_receipt "$work/second-pie-stop.json" second-pie-stop play.stop "$sid2" atomic play.status true unchanged
"$bin" --project "$project" --engine "$engine_root" --timeout 180 --format json project build >"$work/project-build.json"
[[ $(jq -r '.process.code' "$work/project-build.json") == 0 ]] || { echo "project build failed" >&2; exit 1; }
[[ $(shasum -a 256 "$sentinel" | cut -d' ' -f1) == "$sentinel_hash" ]] || { echo "sentinel changed" >&2; exit 1; }
[[ $(find "$project_dir/Content" -type f -name '*.umap' | wc -l | tr -d ' ') == 1 ]] || { echo "unexpected map inventory" >&2; exit 1; }
[[ $(find "$project_dir/Content/MagiM6/Input" -type f -name '*.uasset' | wc -l | tr -d ' ') == 3 ]] || { echo "unexpected input asset inventory" >&2; exit 1; }
[[ -f "$project_dir/Content/MagiM6/BP_InvalidCompile.uasset" && -f "$project_dir/Content/MagiM6/BP_ValidCompile.uasset" && $(find "$project_dir/Content/MagiM6" -type f -name 'BP_*.uasset' | wc -l | tr -d ' ') == 2 ]] || { echo "unexpected Blueprint fixture inventory" >&2; exit 1; }
stop_editor
if [[ -e "$runtime_root" ]] && find "$runtime_root" -type f \( -name bridge-v1.json -o -name token \) -print -quit | grep -q .; then echo "runtime discovery survived cleanup under $runtime_root" >&2; exit 1; fi
for secret in "${tokens[@]}"; do
  if grep -R -I -Fq -- "$secret" "$work"; then echo "runtime token leaked into work evidence" >&2; exit 1; fi
done
cp "$work"/*.json "$evidence/" 2>/dev/null || true
cp "$screenshot" "$evidence/m6-live.png"
for secret in "${tokens[@]}"; do
  if grep -R -I -Fq -- "$secret" "$evidence"; then echo "runtime token leaked into retained evidence" >&2; exit 1; fi
done
printf 'target=UE 5.8.1 changelist 56057345 host=%s\neditorRestart=passed\npieInputObserve=operation-specific-revisions-and-deterministic-100-unit-input-passed\nplayReceipts=matched-native-readback-and-operation-view-passed\nscreenshot=valid-rendered-png\nblueprintCompile=success-repeat-save-restart-and-structured-failure-passed\ncomponentRemove=dry-run-force-save-restart-absence-passed\npersistenceReset=passed\nprojectBuild=passed\nshutdown=process-and-discovery-clean\ninventory=1 umap,3 input uassets,2 Blueprint fixtures,sentinel unchanged\n' "$(uname -m)" | tee "$evidence/summary.txt"
echo "M6 live certification: PASS (evidence retained at $evidence)"