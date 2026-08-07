#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.2-manifest.json"
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -f "$manifest" && -x "$run_uat" && -x "$editor_cmd" && -x "$dotnet_root/dotnet" ]]
[[ $(uname -m) == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == "$(jq -r .engine.changelist "$manifest")" ]]
[[ $(plutil -extract MajorVersion raw -o - "$version_file").$(plutil -extract MinorVersion raw -o - "$version_file").$(plutil -extract PatchVersion raw -o - "$version_file") == "$(jq -r .engine.version "$manifest")" ]]
jq -e '.phase == "P1.2" and (.catalog.count | type) == "number" and (.catalog.sha256 | test("^[0-9a-f]{64}$")) and (.automationTests | length) == 22 and (.automationTests | unique | length) == 22 and (.liveAssertions | length) == 6 and .fixture.blueprints == 2 and .fixture.interfaces == 1 and .fixture.timerOrDelegate == false and .pipeline.blueprintOnly == true and .pipeline.compileAllBlueprints and .pipeline.cook and .pipeline.package and .plugin.binary == "libUnrealEditor-MagiUnrealAXI.dylib" and .plugin.requiredArchitectures == ["arm64","x86_64"] and (.plugin.sourceRoot | type) == "string" and (.cli.binary | type) == "string" and (.packageAssertions.contentRoot | type) == "string" and (.packageAssertions.interface.object | type) == "string" and (.packageAssertions.targetBlueprint.object | type) == "string" and (.packageAssertions.playerBlueprint.object | type) == "string" and (.packageAssertions.interface.generatedRuntime | endswith("_C")) and (.packageAssertions.targetBlueprint.generatedRuntime | endswith("_C")) and (.packageAssertions.playerBlueprint.generatedRuntime | endswith("_C")) and (.packageAssertions.mapPackage | type) == "string"' "$manifest" >/dev/null

source_inventory() {
  local output=$1
  {
    printf '%s\n' "$repo_root/Cargo.toml" "$repo_root/Cargo.lock"
    for root in src capabilities plugin tests/unreal; do
      find "$repo_root/$root" \( -type d \( -name Saved -o -name Binaries -o -name Intermediate -o -name DerivedDataCache \) -prune \) -o -type f -print
    done
  } | sed "s#^$repo_root/##" | LC_ALL=C sort -u | while IFS= read -r file; do
    [[ -f "$repo_root/$file" ]] || { echo "source inventory path is not a regular file: $file" >&2; return 1; }
    printf '%s\t%s\n' "$file" "$(shasum -a 256 "$repo_root/$file" | cut -d' ' -f1)"
  done >"$output"
  [[ -s "$output" ]]
  awk -F '\t' 'NF != 2 || $1 == "" || $2 !~ /^[0-9a-f]{64}$/ { exit 1 }' "$output"
  LC_ALL=C sort -c "$output"
  [[ $(cut -f1 "$output" | LC_ALL=C sort -u | wc -l | tr -d ' ') == $(wc -l <"$output" | tr -d ' ') ]]
}

inventory_tree() {
  local root=$1 output=$2 invalid
  invalid=$(find "$root" ! -type d ! -type f -print -quit)
  [[ -z "$invalid" ]]
  (cd "$root" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s\t%s\n' "${file#./}" "$(shasum -a 256 "$file" | cut -d' ' -f1)"; done) >"$output"
  [[ -s "$output" ]]
  awk -F '\t' 'NF != 2 || $1 == "" || $2 !~ /^[0-9a-f]{64}$/ { exit 1 }' "$output"
  LC_ALL=C sort -c "$output"
  [[ $(cut -f1 "$output" | LC_ALL=C sort -u | wc -l | tr -d ' ') == $(wc -l <"$output" | tr -d ' ') ]]
}

scan_for_runtime_material() {
  local root=$1 found grep_status
  found=$(find "$root" -type f \( -name token -o -name bridge-v1.json \) -print -quit)
  [[ -z "$found" ]]
  set +e
  grep -R -I -E -q 'Authorization:[[:space:]]*Bearer[[:space:]]+[A-Za-z0-9._-]+' "$root"
  grep_status=$?
  set -e
  [[ $grep_status == 1 ]]
}

cache_root="$HOME/Library/Caches/magi-unreal-axi/p1.2/native"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
cleanup() {
  local status=$?
  trap - EXIT
  if [[ $status != 0 ]]; then
    echo "P1.2 native certification failed; work retained at $work; evidence at $evidence" >&2
    for log in "$work"/*.log; do [[ -f "$log" ]] && { echo "--- $log" >&2; tail -80 "$log" >&2; }; done
    exit "$status"
  fi
  [[ ${KEEP_P12_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

source_inventory "$work/source-inventory-before.txt"
source_inventory_hash=$(shasum -a 256 "$work/source-inventory-before.txt" | cut -d' ' -f1)
source_root=$(jq -r .plugin.sourceRoot "$manifest")
inventory_tree "$repo_root/$source_root" "$work/plugin-source-tree-before.txt"
plugin_source_hash=$(shasum -a 256 "$work/plugin-source-tree-before.txt" | cut -d' ' -f1)

mkdir -p "$work/project/Plugins" "$work/plugin" "$work/report" "$work/cooked" "$work/package" "$work/registry-dump" "$work/package-inventory"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/project"
package_project_dir="$work/package-project"
mkdir -p "$package_project_dir/Config"
cp "$repo_root/tests/unreal/MagiUnrealAXIPackageFixture/MagiUnrealAXIPackageFixture.uproject" "$package_project_dir/"
cat >"$package_project_dir/Config/DefaultEngine.ini" <<'EOF'
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/MagiP12/P12Interaction
EditorStartupMap=/Game/MagiP12/P12Interaction
GlobalDefaultGameMode=/Script/Engine.GameModeBase
EOF
cat >"$package_project_dir/Config/DefaultGame.ini" <<'EOF'
[/Script/UnrealEd.ProjectPackagingSettings]
+MapsToCook=(FilePath="/Game/MagiP12/P12Interaction")
+DirectoriesToAlwaysCook=(Path="/Game/MagiP12")
UsePakFile=True
bUseIoStore=True
bUseZenStore=False
EOF
for forbidden in Source Modules Plugins Binaries Intermediate Saved DDC DerivedDataCache; do [[ ! -e "$package_project_dir/$forbidden" ]]; done
export DOTNET_ROOT="$dotnet_root"
export PATH="$DOTNET_ROOT:$PATH"

catalog_line=$(cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check)
catalog_count=$(sed -E 's/^capability catalog: ([0-9]+) records.*/\1/' <<<"$catalog_line")
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' <<<"$catalog_line")
[[ $catalog_count == "$(jq -r .catalog.count "$manifest")" && $catalog_hash == "$(jq -r .catalog.sha256 "$manifest")" ]]
automation_count=$(jq -r '.automationTests | length' "$manifest")
printf '%s\n' "$catalog_line" >"$work/catalog-check.txt"

cargo test --all-targets --all-features --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-tests.log" 2>&1
cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/release-build.log" 2>&1
cli="$repo_root/$(jq -r .cli.binary "$manifest")"
[[ -x "$cli" ]]
cli_hash=$(shasum -a 256 "$cli" | cut -d' ' -f1)
cp "$cli" "$work/magi-unreal-axi"
chmod 0755 "$work/magi-unreal-axi"

"$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/plugin" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
plugin_binary=$(find "$work/plugin" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit)
[[ -n "$plugin_binary" ]]
arches=$(lipo -archs "$plugin_binary")
for arch in $(jq -r '.plugin.requiredArchitectures[]' "$manifest"); do grep -qw "$arch" <<<"$arches"; done
[[ $(wc -w <<<"$arches" | tr -d ' ') == 2 ]]
artifact_hash=$(shasum -a 256 "$plugin_binary" | cut -d' ' -f1)
ditto "$work/plugin" "$work/project/Plugins/MagiUnrealAXI"
project="$work/project/MagiUnrealAXIFixture.uproject"
package_project="$package_project_dir/MagiUnrealAXIPackageFixture.uproject"
jq -e 'has("Modules") | not' "$package_project" >/dev/null
[[ ! -e "$package_project_dir/Source" ]]
axi=("$work/magi-unreal-axi" --project "$project" --engine "$engine_root" --timeout 1800 --format json)
package_axi=("$work/magi-unreal-axi" --project "$package_project" --engine "$engine_root" --timeout 7200 --format json)
"${axi[@]}" project build >"$work/project-build.json"
jq -e '.operation.kind == "build" and (.operation.status == "passed" or .operation.status == "up_to_date")' "$work/project-build.json" >/dev/null
"$editor_cmd" "$project" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/compile.log" >"$work/compile.stdout" 2>&1
grep -q 'MAGI_UNREAL_AXI_FIXTURE_STARTUP' "$work/compile.stdout"
grep -E '0 error\(s\), 0 warning\(s\)' "$work/compile.stdout" >"$work/compile-proof.txt"
"${axi[@]}" project test list --filter MagiUnrealAXI --limit 100 >"$work/test-list.json"
jq -e --slurpfile m "$manifest" --argjson count "$automation_count" '(.count == $count and .total == $count and ([.items[].id] | sort) == ($m[0].automationTests | sort))' "$work/test-list.json" >/dev/null
"${axi[@]}" project test run --filter MagiUnrealAXI --report "$work/report" >"$work/test-run.json"
jq -e --argjson count "$automation_count" '(.totals.failed == 0 and .totals.notRun == 0 and .totals.inProcess == 0 and .totals.matched == $count)' "$work/test-run.json" >/dev/null
jq -e --slurpfile m "$manifest" '(.tests | map(.fullTestPath) | sort) == ($m[0].automationTests | sort) and all(.tests[]; .state == "Success" and .errors == 0)' "$work/report/index.json" >/dev/null

live_evidence="$work/live-evidence"
P12_LIVE_EVIDENCE_DIR="$live_evidence" P12_PACKAGE_PROJECT_DIR="$package_project_dir" P12_CLI_PATH="$work/magi-unreal-axi" P12_CLI_SHA256="$cli_hash" P12_PLUGIN_DIR="$work/plugin" P12_PLUGIN_SHA256="$artifact_hash" "$repo_root/tests/unreal/certify-p1.2-live.sh" >"$work/live-certification.log" 2>&1
[[ -d "$live_evidence" ]]
grep -qx 'phase=P1.2' "$live_evidence/summary.txt"
grep -qx "catalogHash=$catalog_hash" "$live_evidence/summary.txt"
grep -qx "artifactSha256=$artifact_hash" "$live_evidence/summary.txt"
grep -qx "cliSha256=$cli_hash" "$live_evidence/summary.txt"
grep -qx 'fixture=one-interface-two-distinct-actor-blueprints' "$live_evidence/summary.txt"
grep -qx 'overlap=one' "$live_evidence/summary.txt"
grep -qx 'displacement=\[100,0,0\]' "$live_evidence/summary.txt"
grep -qx 'pieSessions=2-deterministic-reset' "$live_evidence/summary.txt"
grep -qx 'tokenScan=passed' "$live_evidence/summary.txt"
while IFS= read -r required; do [[ -f "$package_project_dir/Content/$required" ]]; done < <(jq -r '.packageAssertions.interface.file,.packageAssertions.targetBlueprint.file,.packageAssertions.playerBlueprint.file,.packageAssertions.map' "$manifest")
"$editor_cmd" "$package_project" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/package-compile.log" >"$work/package-compile.stdout" 2>&1
grep -E '0 error\(s\), 0 warning\(s\)' "$work/package-compile.stdout" >"$work/package-compile-proof.txt"

"${package_axi[@]}" project cook --output "$work/cooked" >"$work/cook.json"
"${package_axi[@]}" project package --output "$work/package" >"$work/package.json"
jq -e '.operation.status == "passed" and (.artifacts | length) > 0' "$work/cook.json" "$work/package.json" >/dev/null
find "$work/package" -type d -name '*.app' -print -quit | grep -q .
for operation_json in "$work/cook.json" "$work/package.json"; do
  operation_id=$(jq -r .operation.id "$operation_json")
  "${package_axi[@]}" operation view "$operation_id" >"$work/operation-$operation_id.json"
  jq -e --arg id "$operation_id" '.operation.id == $id and .operation.status == "passed"' "$work/operation-$operation_id.json" >/dev/null
done

utoc=$(find "$work/package" -type f -name 'MagiUnrealAXIPackageFixture-Mac.utoc' -print -quit)
[[ -n "$utoc" ]]
unreal_pak="$engine_root/Engine/Binaries/Mac/UnrealPak"
[[ -x "$unreal_pak" ]]
"$unreal_pak" -ListContainer="$utoc" -Csv="$work/package-inventory/iostore.csv" >"$work/package-inventory/unrealpak.log" 2>&1
assert_iostore_export() {
  local expected=$1
  awk -F',' -v expected="/Content/$expected" 'NR > 1 { filename=$5; chunk=$12; gsub(/^[[:space:]]+|[[:space:]]+$/, "", filename); gsub(/^[[:space:]]+|[[:space:]]+$/, "", chunk); if (substr(filename, length(filename) - length(expected) + 1) == expected && chunk == "ExportBundleData") found=1 } END { exit !found }' "$work/package-inventory/iostore.csv"
}
while IFS= read -r required; do assert_iostore_export "$required"; done < <(jq -r '.packageAssertions.interface.file,.packageAssertions.targetBlueprint.file,.packageAssertions.playerBlueprint.file,.packageAssertions.map' "$manifest")

registry=$(find "$work/cooked" -type f -name 'AssetRegistry.bin' -print -quit)
[[ -n "$registry" ]]
"$editor_cmd" "$package_project" -run=AssetRegistryDump "-input=$registry" "-outdir=$work/registry-dump" -unattended -nop4 -nosplash -nullrhi -NoSound >"$work/registry-dump/AssetRegistryDump.log" 2>&1
registry_files=$(find "$work/registry-dump" -type f -not -name '*.log' -print)
[[ -n "$registry_files" ]]
registry_utf8="$work/registry-dump-utf8.txt"
: >"$registry_utf8"
while IFS= read -r registry_file; do
  bom=$(od -An -tx1 -N2 "$registry_file" | tr -d '[:space:]')
  if [[ $bom == fffe || $bom == feff ]]; then iconv -f UTF-16 -t UTF-8 "$registry_file" >>"$registry_utf8"; else cat "$registry_file" >>"$registry_utf8"; fi
  printf '\n' >>"$registry_utf8"
done < <(printf '%s\n' "$registry_files" | LC_ALL=C sort)
registry_text="$work/registry-dump-normalized-utf8.txt"
LC_ALL=C tr '[:upper:]' '[:lower:]' <"$registry_utf8" >"$registry_text"
registry_group_row() {
  local section=$1 expected=$2 file=$3
  awk -v section="$section" -v expected="$expected" '$0 == "--- begin " section " ---" { in_section=1; next } in_section && $0 ~ /^--- end / { exit found ? 0 : 1 } in_section { row=$0; sub(/^[[:space:]]+/, "", row); sub(/[[:space:]]+$/, "", row); if (row == expected) found=1 } END { exit found ? 0 : 1 }' "$file"
}
registry_property_row() {
  local property=$1 expected=$2 file=$3
  awk -v property="$property" -v expected="$expected" '{ line=$0; sub(/^[[:space:]]+/, "", line) } line ~ "^" property " :" { in_property=1; next } in_property && line ~ /^[^[:space:]]/ && line ~ / :/ { exit found ? 0 : 1 } in_property { row=line; sub(/[[:space:]]+$/, "", row); if (row == expected) found=1 } END { exit found ? 0 : 1 }' "$file"
}
for key in interface targetBlueprint playerBlueprint; do
  object=$(jq -r ".packageAssertions.$key.object" "$manifest" | tr '[:upper:]' '[:lower:]')
  package=${object%.*}
  generated=$(jq -r ".packageAssertions.$key.generatedRuntime" "$manifest" | tr '[:upper:]' '[:lower:]')
  registry_group_row cachedassetsbyobjectpath "$object" "$registry_text"
  registry_property_row generatedclass "$object, /script/engine.blueprintgeneratedclass'$package.$generated'" "$registry_text"
done
map_package=$(jq -r .packageAssertions.mapPackage "$manifest" | tr '[:upper:]' '[:lower:]')
registry_property_row /script/engine.world "$map_package.$(basename "$map_package")" "$registry_text"
set +e
grep -R -I -E -q '/Script/(MagiUnrealAXIFixture|MagiUnrealAXI)' "$registry_text" "$work/package-inventory"
forbidden_status=$?
set -e
[[ $forbidden_status == 1 ]]

bind_inventory() {
  local output=$1 json=$2 kind=$3 root=$4 marker=$5 rows bytes canonical_root
  rows=$(wc -l <"$output" | tr -d ' ')
  bytes=$(find "$root" -type f -exec stat -f '%z' {} + | awk '{ total += $1 } END { print total + 0 }')
  canonical_root=$(cd "$root" && pwd -P)
  jq -e --rawfile inventory "$output" --arg kind "$kind" --arg path "$canonical_root" --argjson rows "$rows" --argjson bytes "$bytes" '($inventory | split("\n") | map(select(length > 0) | split("\t")[0])) as $paths | [.artifacts[] | select(.kind == $kind)] as $artifacts | ($artifacts | length) == 1 and $artifacts[0].exists == true and $artifacts[0].path == $path and $artifacts[0].fileCount == $rows and $artifacts[0].totalBytes == $bytes and ($artifacts[0].entries | type) == "array" and ($artifacts[0].inventoryComplete | type) == "boolean" and ($artifacts[0].entries == ($artifacts[0].entries | sort | unique)) and all($artifacts[0].entries[]; . as $entry | ($paths | index($entry)) != null) and ($artifacts[0].inventoryComplete == ($artifacts[0].entries == $paths))' "$json" >/dev/null
  grep -Fqx "$marker" <(cut -f1 "$output")
}
inventory_tree "$work/cooked" "$work/package-inventory/cooked-tree.txt"
inventory_tree "$work/package" "$work/package-inventory/package-tree.txt"
cooked_manifest_marker=$(awk -F '\t' '$1 ~ /(^|\/)AssetRegistry\.bin$/ { print $1 }' "$work/package-inventory/cooked-tree.txt")
[[ -n "$cooked_manifest_marker" ]]
bind_inventory "$work/package-inventory/cooked-tree.txt" "$work/cook.json" cooked-output "$work/cooked" "$cooked_manifest_marker"
bind_inventory "$work/package-inventory/package-tree.txt" "$work/package.json" package-output "$work/package" .magi-unreal-axi-package.json
cooked_inventory_hash=$(shasum -a 256 "$work/package-inventory/cooked-tree.txt" | cut -d' ' -f1)
package_inventory_hash=$(shasum -a 256 "$work/package-inventory/package-tree.txt" | cut -d' ' -f1)

scan_for_runtime_material "$work"
source_inventory "$work/source-inventory-after.txt"
source_inventory_after_hash=$(shasum -a 256 "$work/source-inventory-after.txt" | cut -d' ' -f1)
diff -u "$work/source-inventory-before.txt" "$work/source-inventory-after.txt" >"$work/source-inventory.diff"
[[ ! -s "$work/source-inventory.diff" && $source_inventory_hash == "$source_inventory_after_hash" ]]

ditto "$live_evidence" "$evidence/live"
cp "$work/live-certification.log" "$work/rust-tests.log" "$work/release-build.log" "$work/plugin-build.log" "$evidence/"
cp "$work/source-inventory-before.txt" "$work/source-inventory-after.txt" "$work/source-inventory.diff" "$work/plugin-source-tree-before.txt" "$evidence/"
cp "$manifest" "$evidence/manifest.json"
cp "$work/catalog-check.txt" "$work/compile-proof.txt" "$work/package-compile-proof.txt" "$evidence/"
cp "$work/project-build.json" "$work/test-list.json" "$work/test-run.json" "$work/cook.json" "$work/package.json" "$evidence/"
cp "$work"/operation-proc-*.json "$evidence/"
cp -R "$work/package-inventory" "$evidence/package-inventory"
cp -R "$work/registry-dump" "$evidence/registry-dump"
cp "$work/report/index.json" "$evidence/automation-index.json"
printf 'phase=P1.2\ntarget=UE %s changelist %s host=%s\ncatalogCount=%s\ncatalogHash=%s\npluginArchitectures=%s\nartifactSha256=%s\npluginSourceTreeSha256=%s\nsourceInventorySha256=%s\nsourceInventoryBeforeSha256=%s\nsourceInventoryAfterSha256=%s\ncliSha256=%s\ncookedInventorySha256=%s\npackageInventorySha256=%s\nrustTests=passed\ncompileAllBlueprints=source-and-blueprint-only-package-passed\nprojectBuild=passed-via-release-cli\nautomation=%s/%s-passed\nblueprintOnlyCookPackage=passed-with-interface-two-blueprints-map-and-generated-classes\nliveFixture=two-blueprint-overlap-interface-offset-two-pie-reset-passed\nreceiptRecovery=live-mutations-cook-package-passed\nruntimeLimits=%s\ntokenScan=passed\n' "$(jq -r .engine.version "$manifest")" "$(jq -r .engine.changelist "$manifest")" "$(uname -m)" "$catalog_count" "$catalog_hash" "$arches" "$artifact_hash" "$plugin_source_hash" "$source_inventory_hash" "$source_inventory_hash" "$source_inventory_after_hash" "$cli_hash" "$cooked_inventory_hash" "$package_inventory_hash" "$automation_count" "$automation_count" "$(jq -c .limits "$manifest")" | tee "$evidence/summary.txt"
scan_for_runtime_material "$evidence"
printf '%s\n' "$evidence" >"$cache_root/latest"
echo "P1.2 native certification: PASS (evidence retained at $evidence)"
