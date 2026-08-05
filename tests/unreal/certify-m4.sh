#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
engine_root=$(cd "$engine_root" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
fixture_build="$engine_root/Engine/Build/BatchFiles/Mac/Build.sh"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
cache_root="$HOME/Library/Caches/magi-unreal-axi/m4/native"

[[ -x "$run_uat" && -x "$editor_cmd" && -x "$fixture_build" && -x "$dotnet_root/dotnet" ]] || { echo "UE 5.8 toolchain is incomplete under $engine_root" >&2; exit 1; }
[[ "$(uname -m)" == arm64 ]] || { echo "M4 native certification requires arm64 host" >&2; exit 1; }
[[ "$(plutil -extract MajorVersion raw -o - "$version_file")" == 5 ]]
[[ "$(plutil -extract MinorVersion raw -o - "$version_file")" == 8 ]]
[[ "$(plutil -extract PatchVersion raw -o - "$version_file")" == 1 ]]
[[ "$(plutil -extract Changelist raw -o - "$version_file")" == 56057345 ]]

mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
cleanup() { [[ ${KEEP_M4_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true; }
trap cleanup EXIT
mkdir -p "$work/Project/Plugins" "$work/package" "$work/report"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/Project"

export DOTNET_ROOT="$dotnet_root"
export PATH="$DOTNET_ROOT:$PATH"
"$run_uat" BuildPlugin \
  -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" \
  -Package="$work/package" \
  -TargetPlatforms=Mac >"$work/build.log" 2>&1
ditto "$work/package" "$work/Project/Plugins/MagiUnrealAXI"
"$fixture_build" MagiUnrealAXIFixtureEditor Mac Development "$work/Project/MagiUnrealAXIFixture.uproject" -WaitMutex >"$work/fixture-build.log" 2>&1

"$editor_cmd" "$work/Project/MagiUnrealAXIFixture.uproject" \
  -unattended -nop4 -nosplash -nullrhi -NoSound \
  '-ExecCmds=Automation RunTests MagiUnrealAXI.Read' \
  '-TestExit=Automation Test Queue Empty' \
  -ReportOutputPath="$work/report" \
  -log="$work/automation.log" >"$work/stdout.log" 2>&1

# Unreal can return before report writer finishes its final atomic replacement.
previous_index_hash=
stable_reads=0
for _ in $(seq 1 50); do
  if [[ -f "$work/report/index.json" ]]; then
    current_index_hash=$(shasum -a 256 "$work/report/index.json" | cut -d' ' -f1)
    if [[ "$current_index_hash" == "$previous_index_hash" ]]; then
      stable_reads=$((stable_reads + 1))
      [[ $stable_reads -ge 5 ]] && break
    else
      previous_index_hash=$current_index_hash
      stable_reads=0
    fi
  fi
  sleep 0.2
done
[[ $stable_reads -ge 5 ]] || { echo "automation report did not stabilize" >&2; exit 1; }

index="$work/report/index.json"
[[ -f "$index" ]] || { echo "automation report missing" >&2; exit 1; }
report_hash=$(shasum -a 256 "$index" | cut -d' ' -f1)
cp "$index" "$evidence/index.json"
[[ $(shasum -a 256 "$evidence/index.json" | cut -d' ' -f1) == "$report_hash" ]] || { echo "automation report changed while evidence was copied" >&2; exit 1; }
jq -e '.succeeded == 2 and .succeededWithWarnings == 0 and .failed == 0 and .notRun == 0 and .inProcess == 0 and ([.tests[].fullTestPath] | sort) == ["MagiUnrealAXI.Read.CatalogContract","MagiUnrealAXI.Read.PaginationContract"]' "$evidence/index.json" >/dev/null || { echo "M4 automation report mismatch" >&2; exit 1; }
[[ ! -f "$work/automation.log" ]] || ! grep -Eqi 'assertion failed|fatal error|crash' "$work/automation.log"
{
  echo "target=UE 5.8.1 changelist 56057345 host=$(uname -m)"
  echo "tests=2 succeeded=2 warnings=0 failed=0 notRun=0 inProcess=0"
  echo "catalogHash=$(shasum -a 256 "$repo_root/capabilities/catalog.json" | cut -d' ' -f1)"
  jq -r '.tests[].fullTestPath' "$evidence/index.json" | sort -u
  echo "reportHash=$report_hash"
} >"$evidence/summary.txt"
cat "$evidence/summary.txt"
echo "M4 native automation: PASS (evidence retained at $evidence)"
