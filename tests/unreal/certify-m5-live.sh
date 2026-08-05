#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
engine_root=$(cd "$engine_root" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -x "$editor" && $(uname -m) == arm64 ]]
[[ $(plutil -extract MajorVersion raw -o - "$version_file") == 5 ]]
[[ $(plutil -extract MinorVersion raw -o - "$version_file") == 8 ]]
[[ $(plutil -extract PatchVersion raw -o - "$version_file") == 1 ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == 56057345 ]]

cache_root="$HOME/Library/Caches/magi-unreal-axi/m5/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
pid=
session=
runtime_project_dir=
bin="$work/magi-unreal-axi"
project="$work/project/MagiUnrealAXIFixture.uproject"
level=/Game/MagiM5Level
axi() { "$bin" --timeout 10 "$@"; }
mutation_when_safe() {
  local output=$1
  shift
  local status reason
  for _ in $(seq 1 600); do
    set +e
    axi "$@" >"$output"
    status=$?
    set -e
    [[ $status == 0 ]] && return 0
    reason=$(plutil -extract error.reason raw -o - "$output" 2>/dev/null || true)
    [[ $reason == unsafe_editor_state ]] || return "$status"
    sleep .1
  done
  return 1
}
stop_editor() {
  [[ -n "$pid" ]] || return 0
  local stopped_pid=$pid
  if kill -0 "$stopped_pid" 2>/dev/null; then
    kill -TERM "$stopped_pid" 2>/dev/null || true
    for _ in $(seq 1 200); do
      kill -0 "$stopped_pid" 2>/dev/null || break
      sleep .1
    done
    if kill -0 "$stopped_pid" 2>/dev/null; then
      kill -KILL "$stopped_pid" 2>/dev/null || true
    fi
    wait "$stopped_pid" 2>/dev/null || true
  fi
  pid=
  for _ in $(seq 1 100); do
    [[ ! -e "$session/bridge-v1.json" && ! -e "$session/token" ]] && return 0
    sleep .1
  done
  echo "editor $stopped_pid exited without cleaning runtime discovery under $session" >&2
  return 1
}
cleanup() { stop_editor; [[ ${KEEP_M5_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true; }
trap cleanup EXIT

cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check >"$work/catalog-check.txt"
cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/release-build.txt" 2>&1
mkdir -p "$work/project"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/project"
cp "$repo_root/target/release/magi-unreal-axi" "$bin"
chmod 0755 "$bin"
axi --project "$project" --engine "$engine_root" setup plugin install --format json >"$work/setup.json"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.txt" 2>&1
canonical_project="$(cd "$(dirname "$project")" && pwd -P)/$(basename "$project")"
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
runtime_project_dir="$HOME/Library/Caches/magi-unreal-axi/$project_hash"

start_editor() {
  local run=$1
  [[ -z "$pid" ]]
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/editor-$run-ue.log" >"$work/editor-$run.log" 2>&1 &
  pid=$!
  local record="$runtime_project_dir/$pid/bridge-v1.json"
  for _ in $(seq 1 1200); do
    [[ -f "$record" ]] && break
    kill -0 "$pid" 2>/dev/null || { wait "$pid" 2>/dev/null || true; return 1; }
    sleep .1
  done
  [[ -f "$record" ]] || return 1
  [[ $(plutil -extract pid raw -o - "$record") == "$pid" ]]
  [[ $(plutil -extract projectPath raw -o - "$record") == "$canonical_project" ]]
  session=$(dirname "$record")
  for _ in $(seq 1 600); do
    if axi --project "$project" --editor "$pid" editor status --format json >"$work/status-$run.json" 2>/dev/null \
      && [[ $(plutil -extract editor.state raw -o - "$work/status-$run.json") == ready ]]; then return 0; fi
    sleep .1
  done
  return 1
}

start_editor first
token=$(cat "$session/token")
mutation_when_safe "$work/create.json" --project "$project" --editor "$pid" level create --path "$level" --format json
[[ $(plutil -extract result.level raw -o - "$work/create.json") == "$level" ]]
[[ $(plutil -extract receipt.target raw -o - "$work/create.json") == "$level" ]]
[[ $(plutil -extract result.changed raw -o - "$work/create.json") == true ]]
[[ $(plutil -extract receipt.persistence raw -o - "$work/create.json") == dirty ]]
[[ $(plutil -extract result.dirtyPackages.0 raw -o - "$work/create.json") == "$level" ]]
[[ $(plutil -extract result.savedPackages json -o - "$work/create.json") == '[]' ]]
[[ ! -e "$work/project/Content/MagiM5Level.umap" ]]
axi --project "$project" --editor "$pid" level create --path "$level" --format json >"$work/create-repeat.json"
[[ $(plutil -extract result.changed raw -o - "$work/create-repeat.json") == false ]]
[[ ! -e "$work/project/Content/MagiM5Level.umap" ]]
axi --project "$project" --editor "$pid" actor spawn --level "$level" --class /Script/Engine.StaticMeshActor --agent-key m5-static --label M5Static --location 100,200,300 --format json >"$work/spawn.json"
actor=$(plutil -extract result.id raw -o - "$work/spawn.json")
guid=$(plutil -extract result.actorGuid raw -o - "$work/spawn.json")
rev=$(plutil -extract result.revision raw -o - "$work/spawn.json")
[[ $(plutil -extract result.changed raw -o - "$work/spawn.json") == true ]]
[[ $(plutil -extract receipt.persistence raw -o - "$work/spawn.json") == dirty ]]
[[ $(plutil -extract receipt.savedPackages json -o - "$work/spawn.json") == '[]' ]]
[[ $(plutil -extract receipt.target raw -o - "$work/spawn.json") == "$actor" ]]
[[ $(plutil -extract receipt.verification.target raw -o - "$work/spawn.json") == "$actor" ]]
[[ $(plutil -extract receipt.verification.matched raw -o - "$work/spawn.json") == true ]]
axi --project "$project" --editor "$pid" actor spawn --level "$level" --class /Script/Engine.StaticMeshActor --agent-key m5-static --label M5Static --format json >"$work/spawn-repeat.json"
[[ $(plutil -extract result.changed raw -o - "$work/spawn-repeat.json") == false ]]
[[ $(plutil -extract result.id raw -o - "$work/spawn-repeat.json") == "$actor" ]]
axi --project "$project" --editor "$pid" actor update-transform "$actor" --location 400,500,600 --expected-revision "$rev" --format json >"$work/update.json"
rev2=$(plutil -extract result.revision raw -o - "$work/update.json")
operation_id=$(plutil -extract receipt.operationId raw -o - "$work/update.json")
[[ "$rev2" != "$rev" ]]
axi --project "$project" --editor "$pid" operation view "$operation_id" --format json >"$work/operation-live.json"
[[ $(plutil -extract operationId raw -o - "$work/operation-live.json") == "$operation_id" ]]
set +e
axi --project "$project" --editor "$pid" actor update-transform "$actor" --location 1,2,3 --expected-revision "$rev" --format json >"$work/stale.json" 2>"$work/stale.stderr"
stale_status=$?
set -e
[[ $stale_status == 2 ]]
[[ $(plutil -extract error.reason raw -o - "$work/stale.json") == conflict ]]
[[ ! -e "$work/project/Content/MagiM5Level.umap" ]]
axi --project "$project" --editor "$pid" level save --path "$level" --format json >"$work/save.json"
[[ $(plutil -extract receipt.persistence raw -o - "$work/save.json") == saved ]]
saved_level_revision=$(plutil -extract result.revision raw -o - "$work/save.json")
[[ $(plutil -extract receipt.target raw -o - "$work/save.json") == "$level" ]]
[[ $(plutil -extract receipt.verification.target raw -o - "$work/save.json") == "$level" ]]
[[ -f "$work/project/Content/MagiM5Level.umap" ]]

# Two independent writers must both serialize successfully or one must return deterministic busy.
axi --project "$project" --editor "$pid" actor spawn --level "$level" --class /Script/Engine.StaticMeshActor --agent-key concurrent-a --label ConcurrentA --format json >"$work/concurrent-a.json" 2>&1 & a_pid=$!
axi --project "$project" --editor "$pid" actor spawn --level "$level" --class /Script/Engine.StaticMeshActor --agent-key concurrent-b --label ConcurrentB --format json >"$work/concurrent-b.json" 2>&1 & b_pid=$!
set +e; wait "$a_pid"; a_status=$?; wait "$b_pid"; b_status=$?; set -e
[[ $a_status == 0 || $(plutil -extract error.reason raw -o - "$work/concurrent-a.json") == busy ]]
[[ $b_status == 0 || $(plutil -extract error.reason raw -o - "$work/concurrent-b.json") == busy ]]
[[ $a_status == 0 || $b_status == 0 ]]
a_actor=; b_actor=
[[ $a_status != 0 ]] || a_actor=$(plutil -extract result.id raw -o - "$work/concurrent-a.json")
[[ $b_status != 0 ]] || b_actor=$(plutil -extract result.id raw -o - "$work/concurrent-b.json")
axi --project "$project" --editor "$pid" level save --path "$level" --format json >"$work/save-concurrent.json"
saved_level_revision=$(plutil -extract result.revision raw -o - "$work/save-concurrent.json")

stop_editor
axi --project "$project" operation view "$operation_id" --format json >"$work/operation-journal.json"
[[ $(plutil -extract operationId raw -o - "$work/operation-journal.json") == "$operation_id" ]]
start_editor second
mutation_when_safe "$work/reopen.json" --project "$project" --editor "$pid" level open --path "$level" --format json
axi --project "$project" --editor "$pid" actor view "$actor" --format json >"$work/view-restarted.json"
axi --project "$project" --editor "$pid" level current --format json >"$work/level-current-restarted.json"
[[ "$(plutil -extract level.revision raw -o - "$work/level-current-restarted.json")" == "$saved_level_revision" ]]
[[ $(plutil -extract actorGuid raw -o - "$work/view-restarted.json") == "$guid" ]]
[[ $(plutil -extract location.0 raw -o - "$work/view-restarted.json") == 400 ]]
[[ $(plutil -extract location.1 raw -o - "$work/view-restarted.json") == 500 ]]
[[ $(plutil -extract location.2 raw -o - "$work/view-restarted.json") == 600 ]]
if [[ -n "$a_actor" ]]; then axi --project "$project" --editor "$pid" actor view "$a_actor" --format json >"$work/concurrent-a-restarted.json"; fi
if [[ -n "$b_actor" ]]; then axi --project "$project" --editor "$pid" actor view "$b_actor" --format json >"$work/concurrent-b-restarted.json"; fi
rev3=$(plutil -extract revision raw -o - "$work/view-restarted.json")
[[ "$rev3" == "$rev2" ]]
axi --project "$project" --editor "$pid" actor spawn --level "$level" --class /Script/Engine.StaticMeshActor --agent-key m5-static --label M5Static --format json >"$work/spawn-after-restart.json"
[[ $(plutil -extract result.changed raw -o - "$work/spawn-after-restart.json") == false ]]
[[ $(plutil -extract result.id raw -o - "$work/spawn-after-restart.json") == "$actor" ]]
set +e
axi --project "$project" --editor "$pid" actor delete "$actor" --expected-revision "$rev3" --format json >"$work/delete-no-force.json" 2>"$work/delete-no-force.stderr"
no_force_status=$?
set -e
[[ $no_force_status == 2 ]]
[[ $(plutil -extract error.reason raw -o - "$work/delete-no-force.json") == force_required ]]
axi --project "$project" --editor "$pid" actor delete "$actor" --dry-run --expected-revision "$rev3" --format json >"$work/delete-dry-run.json"
[[ $(plutil -extract result.id raw -o - "$work/delete-dry-run.json") == "$actor" ]]
[[ $(plutil -extract result.changed raw -o - "$work/delete-dry-run.json") == false ]]
[[ $(plutil -extract result.dryRun raw -o - "$work/delete-dry-run.json") == true ]]
[[ $(plutil -extract result.revision raw -o - "$work/delete-dry-run.json") == "$rev3" ]]
[[ $(plutil -extract result.dirtyPackages json -o - "$work/delete-dry-run.json") == '[]' ]]
plutil -extract receipt.verification.exists raw -o - "$work/delete-dry-run.json" | grep -qx true
[[ $(plutil -extract receipt.verification.exists raw -o - "$work/delete-dry-run.json") == true ]]
axi --project "$project" --editor "$pid" actor delete "$actor" --force --expected-revision "$rev3" --format json >"$work/delete.json"
[[ $(plutil -extract result.changed raw -o - "$work/delete.json") == true ]]
plutil -extract receipt.verification.exists raw -o - "$work/delete.json" | grep -qx false
[[ $(plutil -extract receipt.verification.exists raw -o - "$work/delete.json") == false ]]
axi --project "$project" --editor "$pid" level save --path "$level" --format json >"$work/save-delete.json"
stop_editor
start_editor third
mutation_when_safe "$work/reopen-delete.json" --project "$project" --editor "$pid" level open --path "$level" --format json
set +e
axi --project "$project" --editor "$pid" actor view "$actor" --format json >"$work/absent.json" 2>"$work/absent.stderr"
absent_status=$?
set -e
[[ $absent_status == 1 ]]
[[ $(plutil -extract error.reason raw -o - "$work/absent.json") == not_found ]]
[[ $(find "$work/project/Content" -type f -name '*.umap' -print | wc -l | tr -d ' ') == 1 ]]
[[ -f "$work/project/Content/MagiM5Level.umap" ]]
! grep -Fq "$token" "$work"/*.json "$work"/*.txt "$work"/*.stderr
grep -q 'GetActiveModalWindow' "$repo_root/plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp"
grep -q 'GetNumRemainingAssets' "$repo_root/plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp"
grep -q 'IsPlaySessionInProgress' "$repo_root/plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp"

cp "$work"/*.json "$work"/*.stderr "$evidence/" 2>/dev/null || true
{
  echo "target=UE 5.8.1 changelist 56057345 host=$(uname -m)"
  echo "catalogHash=$(shasum -a 256 "$repo_root/capabilities/catalog.json" | cut -d' ' -f1)"
  echo "level=$level actor=$actor actorGuid=$guid"
  echo "levelCreate=dirty-unsaved,repeat-no-op explicitSave=first-umap-write"
  echo "spawnRepeat=same-id-no-op transform=400,500,600 staleRevision=exit2"
  echo "saveRestart=actor-guid-and-transform-persisted agentKey=restart-no-op"
  echo "delete=no-force-exit2,dry-run-exact,force,save,restart-absent"
  echo "operationView=live-and-private-journal concurrency=serialized-or-busy"
  echo "unsafeGate=runtime-detection-present native-state-matrix-certified"
  echo "inventory=one-intended-umap token=absent"
} >"$evidence/summary.txt"
cat "$evidence/summary.txt"
echo "M5 live mutation/persistence: PASS (evidence retained at $evidence)"
