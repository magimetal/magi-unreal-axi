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
cache_root="$HOME/Library/Caches/magi-unreal-axi/m5/native"
[[ -x "$run_uat" && -x "$editor_cmd" && -x "$fixture_build" && -x "$dotnet_root/dotnet" ]] || { echo "UE 5.8 toolchain incomplete: $engine_root" >&2; exit 1; }
[[ $(uname -m) == arm64 ]] || { echo "M5 native certification requires arm64 host" >&2; exit 1; }
[[ $(plutil -extract MajorVersion raw -o - "$version_file") == 5 ]]
[[ $(plutil -extract MinorVersion raw -o - "$version_file") == 8 ]]
[[ $(plutil -extract PatchVersion raw -o - "$version_file") == 1 ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == 56057345 ]]
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
trap '/usr/bin/trash "$work" 2>/dev/null || true' EXIT
mkdir -p "$work/Project/Plugins" "$work/package" "$work/report"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/Project"
export DOTNET_ROOT="$dotnet_root"
export PATH="$DOTNET_ROOT:$PATH"
"$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/package" -TargetPlatforms=Mac >"$work/build.log" 2>&1
ditto "$work/package" "$work/Project/Plugins/MagiUnrealAXI"
"$fixture_build" MagiUnrealAXIFixtureEditor Mac Development "$work/Project/MagiUnrealAXIFixture.uproject" -WaitMutex >"$work/fixture-build.log" 2>&1
"$editor_cmd" "$work/Project/MagiUnrealAXIFixture.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound '-ExecCmds=Automation RunTests MagiUnrealAXI.Mutation' '-TestExit=Automation Test Queue Empty' -ReportOutputPath="$work/report" -log="$work/automation.log" >"$work/stdout.log" 2>&1
for _ in $(seq 1 50); do [[ -f "$work/report/index.json" ]] && break; sleep .2; done
index="$work/report/index.json"
[[ -f "$index" ]]
[[ $(plutil -extract succeeded raw -o - "$index") == 3 ]]
[[ $(plutil -extract succeededWithWarnings raw -o - "$index") == 0 ]]
[[ $(plutil -extract failed raw -o - "$index") == 0 ]]
[[ $(plutil -extract notRun raw -o - "$index") == 0 ]]
[[ $(plutil -extract inProcess raw -o - "$index") == 0 ]]
[[ ! -f "$work/automation.log" ]] || ! grep -Eqi 'assertion failed|fatal error|crash' "$work/automation.log"
cp "$index" "$evidence/index.json"
{
  echo "target=UE 5.8.1 changelist 56057345 host=$(uname -m)"
  echo "tests=3 succeeded=3 warnings=0 failed=0 notRun=0 inProcess=0"
  plutil -extract tests json -o - "$index" | grep -o 'MagiUnrealAXI.Mutation.[A-Za-z]*' | sort -u
  echo "pluginBuild=passed"
} >"$evidence/summary.txt"
cat "$evidence/summary.txt"
echo "M5 native mutation: PASS (evidence retained at $evidence)"
