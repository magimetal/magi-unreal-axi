#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
engine_root=$(cd "$engine_root" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
cache_root="$HOME/Library/Caches/magi-unreal-axi/m3/native"

[[ -x "$run_uat" && -x "$editor_cmd" && -x "$dotnet_root/dotnet" ]] || { echo "UE 5.8 toolchain is incomplete under $engine_root" >&2; exit 1; }
[[ "$(uname -m)" == arm64 ]] || { echo "M3 native certification requires arm64 host" >&2; exit 1; }
[[ "$(plutil -extract MajorVersion raw -o - "$version_file")" == 5 ]]
[[ "$(plutil -extract MinorVersion raw -o - "$version_file")" == 8 ]]
[[ "$(plutil -extract PatchVersion raw -o - "$version_file")" == 1 ]]
[[ "$(plutil -extract Changelist raw -o - "$version_file")" == 56057345 ]]

mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
cleanup() { /usr/bin/trash "$work" 2>/dev/null || true; }
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

"$editor_cmd" "$work/Project/MagiUnrealAXIFixture.uproject" \
  -unattended -nop4 -nosplash -nullrhi -NoSound \
  '-ExecCmds=Automation RunTests MagiUnrealAXI.Bridge' \
  '-TestExit=Automation Test Queue Empty' \
  -ReportOutputPath="$work/report" \
  -log="$work/automation.log" >"$work/stdout.log" 2>&1

index="$work/report/index.json"
[[ -f "$index" ]] || { echo "automation report missing" >&2; exit 1; }
[[ "$(plutil -extract succeeded raw -o - "$index")" == 5 ]]
[[ "$(plutil -extract succeededWithWarnings raw -o - "$index")" == 0 ]]
[[ "$(plutil -extract failed raw -o - "$index")" == 0 ]]
[[ "$(plutil -extract notRun raw -o - "$index")" == 0 ]]
[[ "$(plutil -extract inProcess raw -o - "$index")" == 0 ]]

cp "$index" "$evidence/index.json"
{
  echo "target=UE 5.8.1 changelist 56057345 host=$(uname -m)"
  echo "tests=5 succeeded=5 warnings=0 failed=0 notRun=0 inProcess=0"
  plutil -extract tests json -o - "$index" | grep -o 'MagiUnrealAXI.Bridge.[A-Za-z]*' | sort -u
} >"$evidence/summary.txt"
cat "$evidence/summary.txt"
echo "M3 native automation: PASS (evidence retained at $evidence)"
