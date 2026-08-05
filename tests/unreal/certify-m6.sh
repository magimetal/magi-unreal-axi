#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
engine_root=$(cd "$engine_root" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -x "$run_uat" && -x "$editor_cmd" && -x "$dotnet_root/dotnet" ]]
[[ $(uname -m) == arm64 ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == 56057345 ]]
cache_root="$HOME/Library/Caches/magi-unreal-axi/m6/native"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
trap 'status=$?; if [[ $status != 0 && -n ${work:-} ]]; then echo "M6 certification failed (status $status); logs: $work" >&2; for log in "$work"/*.log "$work"/*.stdout; do [[ -f "$log" ]] && { echo "--- $log" >&2; tail -80 "$log" >&2; }; done; fi; [[ ${KEEP_M6_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true' EXIT
mkdir -p "$work/Project/Plugins" "$work/package" "$work/report-m6" "$work/report-mutation"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/Project"
export DOTNET_ROOT="$dotnet_root"
export PATH="$DOTNET_ROOT:$PATH"
run_step() { local name=$1; shift; if ! "$@" >"$work/$name.log" 2>&1; then echo "M6 step failed: $name (logs: $work/$name.log)" >&2; return 1; fi; }
run_step plugin-build "$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/package" -TargetPlatforms=Mac
ditto "$work/package" "$work/Project/Plugins/MagiUnrealAXI"
run_step fixture-build "$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$work/Project/MagiUnrealAXIFixture.uproject" -WaitMutex
run_step blueprint-compile "$editor_cmd" "$work/Project/MagiUnrealAXIFixture.uproject" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/compile.log"
run_step automation-m6 "$editor_cmd" "$work/Project/MagiUnrealAXIFixture.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound '-ExecCmds=Automation RunTests MagiUnrealAXI.M6' '-TestExit=Automation Test Queue Empty' -ReportOutputPath="$work/report-m6" -log="$work/automation-m6.log"
run_step automation-mutation "$editor_cmd" "$work/Project/MagiUnrealAXIFixture.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound '-ExecCmds=Automation RunTests MagiUnrealAXI.Mutation' '-TestExit=Automation Test Queue Empty' -ReportOutputPath="$work/report-mutation" -log="$work/automation-mutation.log"
m6_index="$work/report-m6/index.json"
mutation_index="$work/report-mutation/index.json"
[[ -f "$m6_index" && -f "$mutation_index" ]] || { echo "automation report missing" >&2; exit 1; }
jq -e '.failed == 0 and .notRun == 0 and .succeeded == 3 and ([.tests[].fullTestPath] | sort) == ["MagiUnrealAXI.M6.BlueprintDiagnosticsContract","MagiUnrealAXI.M6.ComponentWorldSettingsContracts","MagiUnrealAXI.M6.PlayReceiptContracts"]' "$m6_index" >/dev/null || { echo "M6 automation report mismatch" >&2; exit 1; }
jq -e '.failed == 0 and .notRun == 0 and .succeeded == 3 and ([.tests[].fullTestPath] | sort) == ["MagiUnrealAXI.Mutation.ActorUpdateDelete","MagiUnrealAXI.Mutation.LevelOperations","MagiUnrealAXI.Mutation.UnsafeStates"]' "$mutation_index" >/dev/null || { echo "Mutation automation report mismatch" >&2; exit 1; }
cp "$m6_index" "$evidence/m6-index.json"
cp "$mutation_index" "$evidence/mutation-index.json"
printf 'target=UE 5.8.1 changelist 56057345 host=%s\nfixture=source-backed pawn/interactable module\npluginBuild=passed\nblueprintCompile=passed\nm6AutomationTests=3 component-world-settings-play-receipts-blueprint-diagnostics\nmutationAutomationTests=3 mutation-level-actor-unsafe\nmutationFailures=0\n' "$(uname -m)" | tee "$evidence/summary.txt"
echo "M6 native foundation: PASS (evidence retained at $evidence)"
