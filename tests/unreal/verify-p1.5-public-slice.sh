#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
build="$engine_root/Engine/Build/BatchFiles/Mac/Build.sh"
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
[[ -x "$build" && -x "$editor" ]]

work=${P15_PUBLIC_WORKSPACE:-$(mktemp -d /tmp/magi-p15-public.XXXXXX)}
project_dir="$work/MagiUnrealAXIFixture"
project="$project_dir/MagiUnrealAXIFixture.uproject"
filter=MagiUnrealAXI.P15.PublicCreateGraphViewPersistence
oracle="$project_dir/Saved/Automation/MagiP15PublicCreateGraphViewOracle.json"

fail() {
  local status=$?
  if [[ $status != 0 ]]; then
    echo "P1.5 public slice verification failed; work retained at $work" >&2
    for log in "$work"/*.log "$work"/*.stdout; do [[ -f "$log" ]] && { echo "--- $log" >&2; tail -80 "$log" >&2; }; done
  fi
  exit "$status"
}
trap fail EXIT

[[ ! -e "$project_dir" ]]
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"
mkdir -p "$project_dir/Plugins"
ditto "$repo_root/plugin/MagiUnrealAXI" "$project_dir/Plugins/MagiUnrealAXI"
ditto "$repo_root/tests/unreal/MagiP15AnimationSeed/Content" "$project_dir/Content"

"$build" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex -NoUBA >"$work/build.log" 2>&1
grep -q '^Result: Succeeded$' "$work/build.log"
! grep -Eq 'warning:|error:' "$work/build.log"

run_session() {
  local label=$1 report="$work/report-$1"
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound \
    "-ExecCmds=Automation RunTests $filter" '-TestExit=Automation Test Queue Empty' \
    "-ReportOutputPath=$report" "-log=$work/$label.log" >"$work/$label.stdout" 2>&1
  jq -e --arg filter "$filter" '
    .succeeded == 1 and .succeededWithWarnings == 0 and .failed == 0 and
    .notRun == 0 and .inProcess == 0 and (.tests | length) == 1 and
    .tests[0].fullTestPath == $filter and .tests[0].state == "Success" and
    .tests[0].warnings == 0 and .tests[0].errors == 0
  ' "$report/index.json" >/dev/null
}

run_session first
[[ -f "$oracle" ]]
cp "$oracle" "$work/first-session-oracle.json"
jq -e '
  (.revision | test("^[0-9a-f]{64}$")) and
  (.generatedClass | endswith("ABP_MagiP15Public_C")) and
  (.animGraphId as $graph | ($graph | contains("#graph:other:")) and (.rootNodeId | startswith($graph + "#node:")))
' "$oracle" >/dev/null
run_session second
cmp "$work/first-session-oracle.json" "$oracle"

trap - EXIT
printf '%s\n' "$work" >/tmp/magi-p15-last-public-workspace
printf 'P1.5 public slice: PASS (two editor sessions; evidence at %s)\n' "$work"
