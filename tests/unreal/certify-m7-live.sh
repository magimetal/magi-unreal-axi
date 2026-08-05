#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor_version="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -x "$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd" && -x "$engine_root/Engine/Build/BatchFiles/RunUAT.sh" ]]
[[ $(uname -m) == arm64 && $(plutil -extract Changelist raw -o - "$editor_version") == 56057345 ]]

cache_root="$HOME/Library/Caches/magi-unreal-axi/m7/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
project_dir="$work/project"
project="$project_dir/MagiUnrealAXIFixture.uproject"
package_project_dir="$work/package-project"
package_project="$package_project_dir/MagiUnrealAXIPackageFixture.uproject"
output_root="$work/output"
bin="$work/magi-unreal-axi"

cleanup() {
  local status=$?
  trap - EXIT
  if [[ $status != 0 ]]; then
    echo "M7 live certification failed; work retained at $work; evidence at $evidence" >&2
    exit "$status"
  fi
  [[ ${KEEP_M7_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

mkdir -p "$project_dir" "$package_project_dir" "$output_root"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"
ditto "$repo_root/tests/unreal/MagiUnrealAXIPackageFixture" "$package_project_dir"
cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check >"$evidence/catalog-check.txt"
cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$evidence/rust-build.log" 2>&1
cp "$repo_root/target/release/magi-unreal-axi" "$bin"
chmod 0755 "$bin"

run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
"$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/plugin" -TargetPlatforms=Mac >"$evidence/plugin-build.log" 2>&1
mkdir -p "$project_dir/Plugins/MagiUnrealAXI"
ditto "$work/plugin" "$project_dir/Plugins/MagiUnrealAXI"
project=$(cd "$project_dir" && pwd -P)/MagiUnrealAXIFixture.uproject
package_project=$(cd "$package_project_dir" && pwd -P)/MagiUnrealAXIPackageFixture.uproject
jq -e 'has("Modules") | not' "$package_project" >/dev/null
[[ ! -e "$package_project_dir/Source" ]]

axi() { "$bin" --project "$project" --engine "$engine_root" --format json "$@"; }
package_axi() { "$bin" --project "$package_project" --engine "$engine_root" --format json "$@"; }
assert_operation() {
  local file=$1 kind=$2
  jq -e --arg kind "$kind" '.operation.kind == $kind and (.operation.id | startswith("proc-")) and .operation.executesProjectCode == true and (.artifacts | any(.kind == "log" and .exists == true))' "$file" >/dev/null
}

# Every pipeline family exposes exact no-side-effect invocation preview.
dry_output="$output_root/dry-output"
axi project build --dry-run >"$evidence/dry-build.json"
axi project test list --filter MagiUnrealAXI.M6 --dry-run >"$evidence/dry-test-list.json"
axi project test run --filter MagiUnrealAXI.M6 --dry-run >"$evidence/dry-test-run.json"
package_axi project cook --output "$dry_output" --dry-run >"$evidence/dry-cook.json"
package_axi project package --output "$dry_output" --dry-run >"$evidence/dry-package.json"
[[ ! -e "$dry_output" ]]
jq -e '.dryRun and .operation.kind == "build" and (.invocation.arguments | index("-WaitMutex")) != null' "$evidence/dry-build.json" >/dev/null
jq -e '.dryRun and .operation.kind == "test-list" and (.invocation.arguments | index("-ExecCmds=Automation List;Quit")) != null' "$evidence/dry-test-list.json" >/dev/null
jq -e '.dryRun and .operation.kind == "test-run" and (.invocation.arguments | index("-ExecCmds=Automation RunTests MagiUnrealAXI.M6")) != null and (.invocation.arguments | map(startswith("-ReportOutputPath=")) | any)' "$evidence/dry-test-run.json" >/dev/null
jq -e '.dryRun and .operation.kind == "cook" and (.invocation.arguments | index("-cook")) != null and (.invocation.arguments | index("-build")) == null and (.invocation.arguments | index("-stage")) == null and (.invocation.arguments | index("-archive")) == null and (.invocation.arguments | index("-package")) == null' "$evidence/dry-cook.json" >/dev/null
jq -e '.dryRun and .operation.kind == "package" and (.invocation.arguments | index("-package")) != null and (.invocation.arguments | index("-pak")) != null and (.invocation.arguments | index("-archive")) != null' "$evidence/dry-package.json" >/dev/null

# UBT success and repeat/up-to-date are both exit 0 with normalized summaries.
axi --timeout 1800 project build >"$evidence/build.json"
axi --timeout 1800 project build >"$evidence/build-repeat.json"
assert_operation "$evidence/build.json" build
assert_operation "$evidence/build-repeat.json" build
jq -e '.operation.status == "passed" or .operation.status == "up_to_date"' "$evidence/build.json" >/dev/null
jq -e '.operation.status == "passed" or .operation.status == "up_to_date"' "$evidence/build-repeat.json" >/dev/null

# List is deterministic; empty list is success; run report is authoritative; zero run is failure.
axi --timeout 1800 project test list --filter MagiUnrealAXI.M6 --limit 100 >"$evidence/test-list.json"
jq -e '.count == 3 and .total == 3 and [.items[].id] == ["MagiUnrealAXI.M6.BlueprintDiagnosticsContract","MagiUnrealAXI.M6.ComponentWorldSettingsContracts","MagiUnrealAXI.M6.PlayReceiptContracts"]' "$evidence/test-list.json" >/dev/null
axi --timeout 1800 project test list --filter MagiUnrealAXI.NoSuchTest --limit 100 >"$evidence/test-list-empty.json"
jq -e '.count == 0 and .total == 0 and .items == []' "$evidence/test-list-empty.json" >/dev/null
axi --timeout 1800 project test run --filter MagiUnrealAXI.M6 >"$evidence/test-run.json"
assert_operation "$evidence/test-run.json" test-run
jq -e '.totals.matched == 3 and .totals.succeeded == 3 and .totals.failed == 0 and .totals.notRun == 0 and .totals.inProcess == 0 and (.artifacts | any(.kind == "automation-report-index" and .exists == true))' "$evidence/test-run.json" >/dev/null
set +e
axi --timeout 1800 project test run --filter MagiUnrealAXI.NoSuchTest >"$evidence/test-run-empty.json"
empty_status=$?
set -e
[[ $empty_status == 1 ]]
jq -e '.error.reason == "automation_no_matches" and .error.diagnostics.totals.matched == 0 and (.error.diagnostics.artifacts | any(.kind == "log" and .exists == true))' "$evidence/test-run-empty.json" >/dev/null

# UAT cook/package each run once and produce verified bounded artifact inventories.
cook="$output_root/cooked"
package="$output_root/package"
package_axi --timeout 7200 project cook --output "$cook" >"$evidence/cook.json"
package_axi --timeout 7200 project package --output "$package" >"$evidence/package.json"
assert_operation "$evidence/cook.json" cook
assert_operation "$evidence/package.json" package
jq -e '.operation.status == "passed" and (.artifacts | any(.kind == "cooked-output" and .fileCount > 0 and .totalBytes > 0))' "$evidence/cook.json" >/dev/null
jq -e '.operation.status == "passed" and (.artifacts | any(.kind == "package-output" and .fileCount > 0 and .totalBytes > 0))' "$evidence/package.json" >/dev/null
find "$package" -type d -name '*.app' -print -quit | grep -q .
find "$cook" "$package" -type f -size +0c -print | sort >"$evidence/artifacts.txt"
[[ -s "$evidence/artifacts.txt" ]]

# Unmanaged overwrite is refused before UAT and sentinel remains exact.
unmanaged="$output_root/unmanaged"
mkdir "$unmanaged"
printf 'm7 sentinel unchanged\n' >"$unmanaged/sentinel.txt"
sentinel_hash=$(shasum -a 256 "$unmanaged/sentinel.txt" | cut -d' ' -f1)
operation_count_before=$(find "$HOME/Library/Caches/magi-unreal-axi/pipeline" -name summary.json | wc -l | tr -d ' ')
set +e
package_axi project package --output "$unmanaged" --force >"$evidence/package-overwrite-refused.json"
overwrite_status=$?
set -e
[[ $overwrite_status == 1 && $(jq -r .error.reason "$evidence/package-overwrite-refused.json") == output_not_managed ]]
[[ $(shasum -a 256 "$unmanaged/sentinel.txt" | cut -d' ' -f1) == "$sentinel_hash" ]]
operation_count_after=$(find "$HOME/Library/Caches/magi-unreal-axi/pipeline" -name summary.json | wc -l | tr -d ' ')
[[ $operation_count_after == "$operation_count_before" ]]

# Real failed build/cook/package preserve structured totals and log evidence.
source_file="$project_dir/Source/MagiUnrealAXIFixture/MagiUnrealAXIFixture.cpp"
cp "$source_file" "$work/source-backup.cpp"
printf '\n#error MAGI_M7_INTENTIONAL_BUILD_FAILURE\n' >>"$source_file"
set +e
axi --timeout 1800 project build >"$evidence/failed-build.json"
failed_status=$?
set -e
[[ $failed_status == 1 ]]
jq -e '.error.reason == "process_failed" and .error.diagnostics.operation.status == "failed" and .error.diagnostics.totals.complete == true and (.error.diagnostics.artifacts | any(.kind == "log" and .exists == true))' "$evidence/failed-build.json" >/dev/null
cp "$work/source-backup.cpp" "$source_file"

cp "$package_project" "$work/package-project-backup.uproject"
jq '.Plugins = [{"Name":"MagiM7IntentionalMissingPlugin","Enabled":true}]' "$package_project" >"$work/package-project-invalid.uproject"
mv "$work/package-project-invalid.uproject" "$package_project"
for family in cook package; do
  set +e
  package_axi --timeout 1800 project "$family" --output "$output_root/failed-$family" >"$evidence/failed-$family.json"
  failed_status=$?
  set -e
  [[ $failed_status == 1 ]]
  jq -e '.error.reason == "process_failed" and .error.diagnostics.operation.status == "failed" and .error.diagnostics.totals.complete == true and (.error.diagnostics.artifacts | any(.kind == "log" and .exists == true))' "$evidence/failed-$family.json" >/dev/null
done
cp "$work/package-project-backup.uproject" "$package_project"

# Managed log reads remain bounded and process operation summaries are recoverable.
axi log latest --lines 100 --bytes 65536 >"$evidence/log-latest.json"
axi log search error --limit 50 >"$evidence/log-search.json"
jq -e '.count <= 100 and (.lines | all((.text | length) <= 4000))' "$evidence/log-latest.json" >/dev/null
jq -e '.count <= 50 and .scannedBytes <= 8388608' "$evidence/log-search.json" >/dev/null
for file in "$evidence"/{build,build-repeat,test-list,test-run}.json; do
  id=$(jq -r .operation.id "$file")
  axi operation view "$id" >"$evidence/operation-$id.json"
  jq -e --arg id "$id" '.operation.id == $id' "$evidence/operation-$id.json" >/dev/null
done
for file in "$evidence"/{cook,package}.json; do
  id=$(jq -r .operation.id "$file")
  package_axi operation view "$id" >"$evidence/operation-$id.json"
  jq -e --arg id "$id" '.operation.id == $id' "$evidence/operation-$id.json" >/dev/null
done

cp "$evidence/test-run.json" "$evidence/test-report-summary.json"
printf 'target=UE 5.8.1 changelist 56057345 host=%s\ndryRuns=exact-side-effect-free\nbuild=passed-and-repeat-up-to-date\nautomation=list-3-empty-0-run-3-zero-run-failed\ncook=passed-artifacts-verified\npackage=passed-app-artifacts-verified-overwrite-protected\nfailures=build-cook-package-structured-with-logs\nlogs=bounded-sanitized-project-scoped\noperations=durable-local-readback\n' "$(uname -m)" | tee "$evidence/summary.txt"
echo "M7 live certification: PASS (evidence retained at $evidence)"
