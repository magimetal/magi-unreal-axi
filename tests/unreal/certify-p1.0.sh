#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.0-manifest.json"
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -f "$manifest" && -x "$run_uat" && -x "$editor_cmd" && -x "$dotnet_root/dotnet" ]]
[[ $(uname -m) == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == "$(jq -r .engine.changelist "$manifest")" ]]
jq -e '
  .generatedSafetyFields == ["id","mutates","destructive","idempotency","saveBehavior","transactionBehavior","reversibility","allowedEditorStates","requiresModules","readback","targetFields","failureReceipt"] and
  .liveAssertions == ["native-availability-complete","offline-native-availability-unknown-editor-offline","failed-compile-non-atomic-dirty-receipt","failed-compile-no-saved-persistence-or-rollback","offline-operation-view-recovery","retained-evidence-token-absence"]
' "$manifest" >/dev/null

cache_root="$HOME/Library/Caches/magi-unreal-axi/p1.0/native"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
cleanup() {
  local status=$?
  trap - EXIT
  if [[ $status != 0 ]]; then
    echo "P1.0 native certification failed; work retained at $work" >&2
    for log in "$work"/*.log; do [[ -f "$log" ]] && { echo "--- $log" >&2; tail -80 "$log" >&2; }; done
    exit "$status"
  fi
  [[ ${KEEP_P10_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

mkdir -p "$work/project/Plugins" "$work/package" "$work/report"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/project"
export DOTNET_ROOT="$dotnet_root"
export PATH="$DOTNET_ROOT:$PATH"

catalog_line=$(cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check)
catalog_count=$(sed -E 's/^capability catalog: ([0-9]+) records.*/\1/' <<<"$catalog_line")
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' <<<"$catalog_line")
[[ $catalog_count == "$(jq -r .catalog.count "$manifest")" && $catalog_hash == "$(jq -r .catalog.sha256 "$manifest")" ]]

"$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/package" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
ditto "$work/package" "$work/project/Plugins/MagiUnrealAXI"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$work/project/MagiUnrealAXIFixture.uproject" -WaitMutex >"$work/fixture-build.log" 2>&1
"$editor_cmd" "$work/project/MagiUnrealAXIFixture.uproject" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/compile.log" >"$work/compile.stdout" 2>&1
"$editor_cmd" "$work/project/MagiUnrealAXIFixture.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound '-ExecCmds=Automation RunTests MagiUnrealAXI' '-TestExit=Automation Test Queue Empty' -ReportOutputPath="$work/report" -log="$work/automation.log" >"$work/automation.stdout" 2>&1

index="$work/report/index.json"
[[ -f "$index" ]]
jq -e --slurpfile manifest "$manifest" '
  .failed == 0 and .notRun == 0 and .inProcess == 0 and
  (.succeeded + .succeededWithWarnings) == ($manifest[0].automationTests | length) and
  (all(.tests[]; .state == "Success" and .errors == 0)) and
  ([.tests[].fullTestPath] | sort) == ($manifest[0].automationTests | sort)
' "$index" >/dev/null

dylib=$(find "$work/package" -type f -name 'libUnrealEditor-MagiUnrealAXI.dylib' -print -quit)
[[ -n "$dylib" ]]
lipo -archs "$dylib" | tr ' ' '\n' | grep -qx arm64
artifact_hash=$(shasum -a 256 "$dylib" | cut -d' ' -f1)
source_hash=$(shasum -a 256 "$repo_root/plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp" | cut -d' ' -f1)
[[ $(find "$work" -type f -name token -print -quit) == "" ]]
limits=$(jq -c .limits "$manifest")

cp "$manifest" "$evidence/manifest.json"
cp "$index" "$evidence/automation-index.json"
printf '%s\n' "$catalog_line" >"$evidence/catalog-check.txt"
printf 'phase=P1.0\ntarget=UE %s changelist %s host=%s\ncatalogCount=%s\ncatalogHash=%s\nartifactSha256=%s\npluginSourceSha256=%s\nlimits=%s\ncompileAllBlueprints=passed\nautomation=%s/%s passed\nruntimeContract=generated-metadata-identity-save-policy-lifecycle-fail-closed\ntokenScan=no-runtime-token-files\n' \
  "$(jq -r .engine.version "$manifest")" "$(jq -r .engine.changelist "$manifest")" "$(uname -m)" "$catalog_count" "$catalog_hash" "$artifact_hash" "$source_hash" "$limits" \
  "$(( $(jq -r .succeeded "$index") + $(jq -r .succeededWithWarnings "$index") ))" "$(jq -r '.automationTests | length' "$manifest")" | tee "$evidence/summary.txt"
printf '%s\n' "$evidence" >"$cache_root/latest"
echo "P1.0 native certification: PASS (evidence retained at $evidence)"
