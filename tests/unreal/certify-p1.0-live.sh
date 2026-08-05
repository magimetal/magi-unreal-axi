#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.0-manifest.json"
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -x "$editor" && -x "$editor_cmd" && -x "$run_uat" && -f "$manifest" ]]
[[ $(uname -m) == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == "$(jq -r .engine.changelist "$manifest")" ]]
jq -e '
  .generatedSafetyFields == ["id","mutates","destructive","idempotency","saveBehavior","transactionBehavior","reversibility","allowedEditorStates","requiresModules","readback","targetFields","failureReceipt"] and
  .liveAssertions == ["native-availability-complete","offline-native-availability-unknown-editor-offline","failed-compile-non-atomic-dirty-receipt","failed-compile-no-saved-persistence-or-rollback","offline-operation-view-recovery","retained-evidence-token-absence"]
' "$manifest" >/dev/null

cache_root="$HOME/Library/Caches/magi-unreal-axi/p1.0/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
project="$work/project/MagiUnrealAXIFixture.uproject"
project_dir=$(dirname "$project")
bin="$work/magi-unreal-axi"
pid=
session=
tokens=()

editor_alive() { kill -0 "$1" 2>/dev/null && [[ $(ps -p "$1" -o stat= 2>/dev/null) != Z* ]]; }
axi() { "$bin" --project "$project" --engine "$engine_root" --timeout 10 --format json "$@"; }
stop_editor() {
  [[ -n "$pid" ]] || return 0
  local stopped_pid=$pid stopped_session=$session
  if editor_alive "$stopped_pid"; then
    axi --editor "$stopped_pid" editor stop >"$work/editor-stop-$stopped_pid.json" 2>/dev/null || true
    for _ in $(seq 1 100); do editor_alive "$stopped_pid" || break; sleep .1; done
    editor_alive "$stopped_pid" && kill -TERM "$stopped_pid" 2>/dev/null || true
    for _ in $(seq 1 200); do editor_alive "$stopped_pid" || break; sleep .1; done
    editor_alive "$stopped_pid" && kill -KILL "$stopped_pid" 2>/dev/null || true
  fi
  wait "$stopped_pid" 2>/dev/null || true
  editor_alive "$stopped_pid" && return 1
  for _ in $(seq 1 100); do
    [[ ! -e "$stopped_session/bridge-v1.json" && ! -e "$stopped_session/token" ]] && break
    sleep .1
  done
  [[ ! -e "$stopped_session/bridge-v1.json" && ! -e "$stopped_session/token" ]] || return 1
  pid=
  session=
}
start_editor() {
  local label=$1 exec_cmd=${2:-} record
  local -a args=(-unattended -nop4 -nosplash -RenderOffscreen -ResX=640 -ResY=360 -NoSound "-log=$work/editor-$label-ue.log")
  [[ -z "$exec_cmd" ]] || args+=("-ExecCmds=$exec_cmd")
  "$editor" "$project" "${args[@]}" >"$work/editor-$label.log" 2>&1 &
  pid=$!
  session="$runtime_root/$pid"
  record="$session/bridge-v1.json"
  for _ in $(seq 1 1200); do
    [[ -f "$record" ]] && break
    editor_alive "$pid" || return 1
    sleep .1
  done
  [[ -f "$record" && $(jq -r .pid "$record") == "$pid" && $(jq -r .projectPath "$record") == "$canonical_project" ]]
  tokens+=("$(cat "$session/token")")
  for _ in $(seq 1 600); do
    if axi --editor "$pid" editor status >"$work/editor-status-$label.json" 2>/dev/null && [[ $(jq -r .editor.state "$work/editor-status-$label.json") == ready ]]; then return 0; fi
    sleep .1
  done
  return 1
}
cleanup() {
  local status=$?
  trap - EXIT
  stop_editor || status=1
  if [[ $status != 0 ]]; then echo "P1.0 live certification failed; work retained at $work; evidence at $evidence" >&2; exit "$status"; fi
  [[ ${KEEP_P10_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

catalog_line=$(cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check)
catalog_count=$(sed -E 's/^capability catalog: ([0-9]+) records.*/\1/' <<<"$catalog_line")
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' <<<"$catalog_line")
[[ $catalog_count == "$(jq -r .catalog.count "$manifest")" && $catalog_hash == "$(jq -r .catalog.sha256 "$manifest")" ]]
printf '%s\n' "$catalog_line" >"$work/catalog-check.txt"
cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-build.log" 2>&1
cp "$repo_root/target/release/magi-unreal-axi" "$bin"
chmod 0755 "$bin"
mkdir -p "$project_dir"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"
"$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/plugin" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
mkdir -p "$project_dir/Plugins/MagiUnrealAXI"
ditto "$work/plugin" "$project_dir/Plugins/MagiUnrealAXI"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.log" 2>&1
canonical_project=$(cd "$project_dir" && pwd -P)/$(basename "$project")
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
runtime_root="$HOME/Library/Caches/magi-unreal-axi/$project_hash"

mkdir -p "$work/report-prep"
"$editor_cmd" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound '-ExecCmds=Automation RunTests MagiUnrealAXI.M6.BlueprintDiagnosticsContract' '-TestExit=Automation Test Queue Empty' -ReportOutputPath="$work/report-prep" -log="$work/blueprint-prep.log" >"$work/blueprint-prep.stdout" 2>&1
jq -e '.failed == 0 and .notRun == 0 and .succeeded == 1' "$work/report-prep/index.json" >/dev/null
find "$runtime_root" -mindepth 1 -maxdepth 1 -type d -exec /usr/bin/trash {} + 2>/dev/null || true

start_editor dirty-blueprint 'Automation RunTests MagiUnrealAXI.LiveFixture.DirtyPackage'
blueprint=/Game/MagiM6/BP_InvalidCompile.BP_InvalidCompile
for _ in $(seq 1 200); do
  axi blueprint view "$blueprint" >"$work/blueprint-dirty-view.json"
  [[ $(jq -r .status "$work/blueprint-dirty-view.json") == dirty ]] && break
  sleep .1
done
jq -e --arg id "$blueprint" '.id == $id and .status == "dirty" and (.revision | length) == 64' "$work/blueprint-dirty-view.json" >/dev/null
before_revision=$(jq -r .revision "$work/blueprint-dirty-view.json")
axi capability describe blueprint.compile >"$work/capability-live.json"
jq -e '.runtime.availability == "available" and .runtime.reasons == []' "$work/capability-live.json" >/dev/null
axi capability search blueprint >"$work/capability-search-live.json"
jq -e '.count == 2 and all(.items[]; .availability == "available" and .reasons == [])' "$work/capability-search-live.json" >/dev/null

set +e
axi blueprint compile "$blueprint" --expected-revision "$before_revision" >"$work/compile-failed.json"
compile_status=$?
set -e
[[ $compile_status == 1 ]]
jq -e --arg id "$blueprint" --arg rev "$before_revision" '
  .error.reason == "blueprint_compile_failed" and .error.retryable == false and
  .error.operationId == .error.receipt.operationId and
  .error.receipt.operation == "blueprint.compile" and .error.receipt.state == "failed" and
  .error.receipt.target == $id and .error.receipt.transaction == "non-atomic" and
  .error.receipt.reversibility == "source-control" and .error.receipt.persistence == "dirty" and
  .error.receipt.savedPackages == [] and (.error.receipt.dirtyPackages | index("/Game/MagiM6/BP_InvalidCompile")) != null and
  .error.receipt.changed == ($rev != .error.receipt.verification.observedRevision) and
  .error.receipt.revision == .error.receipt.verification.observedRevision and
  .error.receipt.verification.beforeRevision == $rev and .error.receipt.verification.observedStatus == "error" and
  .error.receipt.verification.failureType == "blueprint_compile_failed" and .error.receipt.verification.matched == true and
  (.error.receipt.verification.errorCount > 0) and (.error.receipt.verification.diagnostics | length) > 0 and
  ([.error.receipt.verification.diagnostics[] | select(.graph != "" and .nodeGuid != "" and .nodeTitle != "")] | length) > 0 and
  (.error.receipt.verification.changedObjects | type) == "array"
' "$work/compile-failed.json" >/dev/null
operation_id=$(jq -r .error.operationId "$work/compile-failed.json")
axi operation view "$operation_id" >"$work/receipt-live.json"
jq -e --slurpfile failed "$work/compile-failed.json" '. == $failed[0].error.receipt' "$work/receipt-live.json" >/dev/null
stop_editor

axi capability describe blueprint.compile >"$work/capability-offline.json"
jq -e '.runtime.availability == "unknown" and (.runtime.reasons | any(.code == "editor_offline"))' "$work/capability-offline.json" >/dev/null
axi operation view "$operation_id" >"$work/receipt-offline.json"
jq -e --slurpfile failed "$work/compile-failed.json" '. == $failed[0].error.receipt' "$work/receipt-offline.json" >/dev/null
start_editor restart
axi blueprint view "$blueprint" >"$work/blueprint-restart.json"
jq -e --arg id "$blueprint" --arg rev "$before_revision" '.id == $id and .status == "error" and .revision == $rev' "$work/blueprint-restart.json" >/dev/null
stop_editor

for secret in "${tokens[@]}"; do
  grep -R -I -Fq -- "$secret" "$work" && { echo "runtime token leaked into P1.0 work evidence" >&2; exit 1; }
done
cp "$work"/*.json "$evidence/"
cp "$manifest" "$evidence/manifest.json"
artifact=$(find "$work/plugin" -type f -name 'libUnrealEditor-MagiUnrealAXI.dylib' -print -quit)
artifact_hash=$(shasum -a 256 "$artifact" | cut -d' ' -f1)
printf 'phase=P1.0\ntarget=UE %s changelist %s host=%s\ncatalogCount=%s\ncatalogHash=%s\nartifactSha256=%s\ncapabilityAvailability=live-available-offline-unknown-editor_offline\nfailedCompile=non-atomic-dirty-no-saved-persistence-or-rollback\noperationRecovery=live-and-offline-exact-receipt\nrestart=invalid-structure-revision-preserved\ntokenScan=passed\n' "$(jq -r .engine.version "$manifest")" "$(jq -r .engine.changelist "$manifest")" "$(uname -m)" "$catalog_count" "$catalog_hash" "$artifact_hash" | tee "$evidence/summary.txt"
for secret in "${tokens[@]}"; do
  grep -R -I -Fq -- "$secret" "$evidence" && { echo "runtime token leaked into retained P1.0 evidence" >&2; exit 1; }
done
printf '%s\n' "$evidence" >"$cache_root/latest"
echo "P1.0 live certification: PASS (evidence retained at $evidence)"
