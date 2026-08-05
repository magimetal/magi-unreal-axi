#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
engine_root=$(cd "$engine_root" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
cache_root="$HOME/Library/Caches/magi-unreal-axi/m0"

[[ -x "$run_uat" ]] || { echo "missing RunUAT: $run_uat" >&2; exit 1; }
[[ -x "$editor_cmd" ]] || { echo "missing UnrealEditor-Cmd: $editor_cmd" >&2; exit 1; }
[[ -f "$version_file" ]] || { echo "missing engine version metadata: $version_file" >&2; exit 1; }
[[ "$(uname -m)" == arm64 ]] || { echo "M0 requires arm64 host" >&2; exit 1; }
read_version_field() {
  local key=$1 value
  value=$(plutil -extract "$key" raw -o - "$version_file") || { echo "missing engine field: $key" >&2; exit 1; }
  [[ "$value" =~ ^[0-9]+$ ]] || { echo "engine $key is not exact numeric value: $value" >&2; exit 1; }
  printf '%s' "$value"
}
[[ "$(read_version_field MajorVersion)" == 5 ]] || { echo "engine MajorVersion mismatch" >&2; exit 1; }
[[ "$(read_version_field MinorVersion)" == 8 ]] || { echo "engine MinorVersion mismatch" >&2; exit 1; }
[[ "$(read_version_field PatchVersion)" == 1 ]] || { echo "engine PatchVersion mismatch" >&2; exit 1; }
[[ "$(read_version_field Changelist)" == 56057345 ]] || { echo "engine Changelist mismatch" >&2; exit 1; }
[[ -x "$dotnet_root/dotnet" ]] || { echo "missing bundled arm64 dotnet: $dotnet_root/dotnet" >&2; exit 1; }

mkdir -p "$cache_root"
cache_root=$(cd "$cache_root" && pwd -P)
work=$(mktemp -d "$cache_root/work.XXXXXX")
work=$(cd "$work" && pwd -P)
package="$work/package"
log="$work/certification.log"
evidence="$work/evidence-report.txt"
report=$(mktemp "$cache_root/evidence.XXXXXX")

cleanup() { rm -rf "$work"; }
trap cleanup EXIT

mkdir -p "$work/Project/Plugins" "$package"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/Project"

export DOTNET_ROOT="$dotnet_root"
export PATH="$DOTNET_ROOT:$PATH"
echo "M0 target: UE 5.8.1 changelist 56057345, host $(uname -m), DOTNET_ROOT=$DOTNET_ROOT"
echo "M0 workspace: $work"
echo "M0 BuildPlugin: RunUAT.sh BuildPlugin -TargetPlatforms=Mac"
"$run_uat" BuildPlugin \
  -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" \
  -Package="$package" \
  -TargetPlatforms=Mac

packaged_plugin="$package"
[[ -f "$packaged_plugin/MagiUnrealAXI.uplugin" ]] || { echo "packaged plugin missing: $packaged_plugin" >&2; exit 1; }
dylib=$(find "$packaged_plugin" -type f -name 'libUnrealEditor-MagiUnrealAXI.dylib' -print -quit)
[[ -n "$dylib" ]] || { echo "packaged arm64 dylib missing" >&2; exit 1; }
lipo -archs "$dylib" | tr ' ' '\n' | grep -qx arm64 || { echo "packaged dylib is not arm64: $dylib" >&2; exit 1; }
mkdir -p "$work/Project/Plugins"
ditto "$packaged_plugin" "$work/Project/Plugins/MagiUnrealAXI"

command_log="$work/compile-all-blueprints.log"
echo "M0 CompileAllBlueprints: UnrealEditor-Cmd -run=CompileAllBlueprints"
set +e
"$editor_cmd" "$work/Project/MagiUnrealAXIFixture.uproject" \
  -unattended -nop4 -nosplash -nullrhi -NoSound \
  -run=CompileAllBlueprints -log="$command_log" >"$log" 2>&1
status=$?
set -e
[[ $status -eq 0 ]] || { echo "CompileAllBlueprints failed: $status" >&2; exit "$status"; }
grep -q 'MAGI_UNREAL_AXI_FIXTURE_STARTUP' "$log" || { echo "startup marker missing" >&2; exit 1; }
grep -Eq '0 error\(s\), 0 warning\(s\)' "$log" || { echo "zero-error summary missing" >&2; exit 1; }
{
  echo "target=UE 5.8.1 changelist 56057345 host=$(uname -m)"
  echo "dotnet_root=$DOTNET_ROOT"
  echo "workspace=$work"
  echo "dylib_arches=$(lipo -archs "$dylib")"
  grep -E 'MAGI_UNREAL_AXI_FIXTURE_STARTUP|0 error\(s\), 0 warning\(s\)' "$log" | head -n 20
} > "$evidence"
cat "$evidence"
cp "$evidence" "$report"
echo "M0 certification: PASS (bounded evidence report retained at $report)"
