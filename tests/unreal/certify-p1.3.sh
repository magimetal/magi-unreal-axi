#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.3-manifest.json"
p12_manifest="$repo_root/tests/unreal/p1.2-manifest.json"
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -f "$manifest" && -f "$p12_manifest" && -x "$run_uat" && -x "$editor_cmd" && -x "$dotnet_root/dotnet" ]]
jq -e '(.phase == "P1.3" and .engine == {version:"5.8.1",changelist:56057345,hostArchitecture:"arm64"} and .catalog == {count:55,sha256:"7cd513c54122e73b4c0b5faaf8f3669f89819584822e10602017e9f41f19e05b"} and .fixture.name == "ui-state-loop" and .fixture.widgetPath == "/Game/MagiP13/WBP_UIState" and .fixture.hostPath == "/Game/MagiP13/BP_UIStateHost" and .fixture.level == "/Game/MagiP13/P13UIState" and .fixture.root == {name:"StateRoot",class:"VerticalBox"} and .fixture.text == {name:"StateText",class:"TextBlock",ready:"READY",active:"ACTIVE"} and .fixture.inputKey == "E" and .fixture.zOrder == 0 and (.nativeTests|length) == 5 and (.p12RegressionTests|length) == 22 and (.automationTests|length) == 27 and (.automationTests|unique|length) == 27 and (.liveAssertions|length) == 8 and .pipeline.blueprintOnly and .pipeline.compileAllBlueprints and .pipeline.cook and .pipeline.package and .plugin.requiredArchitectures == ["arm64","x86_64"])' "$manifest" >/dev/null
jq -e --slurpfile p12 "$p12_manifest" '.p12RegressionTests == $p12[0].automationTests and ((.p12RegressionTests + .nativeTests) | sort) == (.automationTests | sort)' "$manifest" >/dev/null
jq -e --arg cell '5.8.1-macos-arm64-56057345' --argjson operations '["play.ui_observe","widget.create","widget.tree_view","widget.child_ensure","widget.property_set","widget.event_ensure","widget.viewport_ensure"]' '[.[] | select(.id as $id | $operations | index($id)) | select(.engineSupport.certified == [$cell])] | length == 7' "$repo_root/capabilities/catalog.json" >/dev/null
[[ $(uname -m) == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == "$(jq -r .engine.changelist "$manifest")" ]]
[[ "$(plutil -extract MajorVersion raw -o - "$version_file").$(plutil -extract MinorVersion raw -o - "$version_file").$(plutil -extract PatchVersion raw -o - "$version_file")" == "$(jq -r .engine.version "$manifest")" ]]

cache_root="$HOME/Library/Caches/magi-unreal-axi/p1.3/native"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
cleanup() {
  local status=$?
  trap - EXIT
  if [[ $status != 0 ]]; then
    echo "P1.3 native certification failed; work retained at $work; evidence at $evidence" >&2
    for log in "$work"/*.log; do [[ -f "$log" ]] && { echo "--- $log" >&2; tail -80 "$log" >&2; }; done
    exit "$status"
  fi
  [[ ${KEEP_P13_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

source_inventory() {
  local output=$1
  { printf '%s\n' "$repo_root/Cargo.toml" "$repo_root/Cargo.lock"; for root in src capabilities plugin tests/unreal; do find "$repo_root/$root" \( -type d \( -name Saved -o -name Binaries -o -name Intermediate -o -name DerivedDataCache \) -prune \) -o -type f -print; done; } |
    sed "s#^$repo_root/##" | LC_ALL=C sort -u | while IFS= read -r file; do
      [[ -f "$repo_root/$file" ]] || return 1
      printf '%s\t%s\n' "$file" "$(shasum -a 256 "$repo_root/$file" | cut -d' ' -f1)"
    done >"$output"
  [[ -s "$output" ]] && LC_ALL=C sort -c "$output"
  awk -F '\t' 'NF != 2 || $2 !~ /^[0-9a-f]{64}$/ { exit 1 }' "$output"
}
inventory_tree() {
  local root=$1 output=$2
  [[ -z "$(find "$root" ! -type d ! -type f -print -quit)" ]]
  (cd "$root" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s\t%s\n' "${file#./}" "$(shasum -a 256 "$file" | cut -d' ' -f1)"; done) >"$output"
  [[ -s "$output" ]] && LC_ALL=C sort -c "$output"
  awk -F '\t' 'NF != 2 || $2 !~ /^[0-9a-f]{64}$/ { exit 1 }' "$output"
}
scan_secrets() {
  local root=$1 status
  [[ -z "$(find "$root" -type f \( -name token -o -name bridge-v1.json \) -print -quit)" ]]
  set +e; grep -R -I -E -q 'Authorization:[[:space:]]*Bearer[[:space:]]+[A-Za-z0-9._-]+' "$root"; status=$?; set -e
  [[ $status == 1 ]]
}

mkdir -p "$work/project/Plugins" "$work/plugin" "$work/report" "$work/cooked" "$work/package" "$work/package-inventory" "$work/registry-dump"
source_inventory "$work/source-inventory-before.txt"
source_inventory_hash=$(shasum -a 256 "$work/source-inventory-before.txt" | cut -d' ' -f1)
source_root=$(jq -r .plugin.sourceRoot "$manifest")
inventory_tree "$repo_root/$source_root" "$work/plugin-source-tree-before.txt"
plugin_source_hash=$(shasum -a 256 "$work/plugin-source-tree-before.txt" | cut -d' ' -f1)

package_project_dir="$work/package-project"
mkdir -p "$package_project_dir/Config"
cp "$repo_root/tests/unreal/MagiUnrealAXIPackageFixture/MagiUnrealAXIPackageFixture.uproject" "$package_project_dir/"
cat >"$package_project_dir/Config/DefaultEngine.ini" <<EOF
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=$(jq -r .fixture.level "$manifest")
EditorStartupMap=$(jq -r .fixture.level "$manifest")
GlobalDefaultGameMode=/Script/Engine.GameModeBase
EOF
cat >"$package_project_dir/Config/DefaultGame.ini" <<EOF
[/Script/UnrealEd.ProjectPackagingSettings]
+MapsToCook=(FilePath="$(jq -r .fixture.level "$manifest")")
+DirectoriesToAlwaysCook=(Path="/Game/$(jq -r .packageAssertions.contentRoot "$manifest")")
UsePakFile=True
bUseIoStore=True
bUseZenStore=False
EOF
for forbidden in Source Modules Plugins Binaries Intermediate Saved DDC DerivedDataCache; do [[ ! -e "$package_project_dir/$forbidden" ]]; done
export DOTNET_ROOT="$dotnet_root"
export PATH="$DOTNET_ROOT:$PATH"

catalog_line=$(cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check)
printf '%s\n' "$catalog_line" >"$work/catalog-check.txt"
catalog_count=$(sed -E 's/^capability catalog: ([0-9]+) records.*/\1/' <<<"$catalog_line")
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' <<<"$catalog_line")
[[ $catalog_count == 55 && $catalog_hash == "$(jq -r .catalog.sha256 "$manifest")" ]]
cargo test --all-targets --all-features --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-tests.log" 2>&1
cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/release-build.log" 2>&1
cargo fmt --manifest-path "$repo_root/Cargo.toml" --all -- --check >"$work/fmt.log" 2>&1
cargo clippy --all-targets --all-features --locked --manifest-path "$repo_root/Cargo.toml" -- -D warnings >"$work/clippy.log" 2>&1
cli="$repo_root/$(jq -r .cli.binary "$manifest")"; [[ -x "$cli" ]]; cli_hash=$(shasum -a 256 "$cli" | cut -d' ' -f1); cp "$cli" "$work/magi-unreal-axi"; chmod 0755 "$work/magi-unreal-axi"

"$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$work/plugin" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1
plugin_binary=$(find "$work/plugin" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit); [[ -n "$plugin_binary" ]]
arches=$(lipo -archs "$plugin_binary"); for arch in $(jq -r '.plugin.requiredArchitectures[]' "$manifest"); do grep -qw "$arch" <<<"$arches"; done; [[ $(wc -w <<<"$arches" | tr -d ' ') == 2 ]]
artifact_hash=$(shasum -a 256 "$plugin_binary" | cut -d' ' -f1)
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/project"
ditto "$work/plugin" "$work/project/Plugins/MagiUnrealAXI"
project="$work/project/MagiUnrealAXIFixture.uproject"; package_project="$package_project_dir/MagiUnrealAXIPackageFixture.uproject"
axi=("$work/magi-unreal-axi" --project "$project" --engine "$engine_root" --timeout 1800 --format json); package_axi=("$work/magi-unreal-axi" --project "$package_project" --engine "$engine_root" --timeout 7200 --format json)
"${axi[@]}" project build >"$work/project-build.json"
jq -e '.operation.kind == "build" and (.operation.status == "passed" or .operation.status == "up_to_date")' "$work/project-build.json" >/dev/null
"$editor_cmd" "$project" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/compile.log" >"$work/compile.stdout" 2>&1
grep -E '0 error\(s\), 0 warning\(s\)' "$work/compile.stdout" >"$work/compile-proof.txt"

"${axi[@]}" project test list --filter MagiUnrealAXI --limit 100 >"$work/test-list.json"
jq -e --slurpfile m "$manifest" '(.count == 27 and .total == 27 and ([.items[].id] | sort) == ($m[0].automationTests | sort))' "$work/test-list.json" >/dev/null
"${axi[@]}" project test run --filter MagiUnrealAXI --report "$work/report" >"$work/test-run.json"
jq -e '.totals.failed == 0 and .totals.notRun == 0 and .totals.inProcess == 0 and .totals.matched == 27' "$work/test-run.json" >/dev/null
jq -e --slurpfile m "$manifest" '(.tests | map(.fullTestPath) | sort) == ($m[0].automationTests | sort) and all(.tests[]; .state == "Success" and .errors == 0)' "$work/report/index.json" >/dev/null

live_evidence="$work/live-evidence"
P13_LIVE_EVIDENCE_DIR="$live_evidence" P13_PACKAGE_PROJECT_DIR="$package_project_dir" P13_CLI_PATH="$work/magi-unreal-axi" P13_CLI_SHA256="$cli_hash" P13_PLUGIN_DIR="$work/plugin" P13_PLUGIN_SHA256="$artifact_hash" P13_WORKSPACE="$work" "$repo_root/tests/unreal/certify-p1.3-live.sh" >"$work/live-certification.log" 2>&1
[[ -d "$live_evidence" ]]
grep -qx 'phase=P1.3' "$live_evidence/summary.txt"
grep -qx "catalogHash=$catalog_hash" "$live_evidence/summary.txt"
grep -qx "artifactSha256=$artifact_hash" "$live_evidence/summary.txt"
grep -qx "cliSha256=$cli_hash" "$live_evidence/summary.txt"
grep -qx 'fixture=ui-state-loop' "$live_evidence/summary.txt"
grep -qx 'tokenScan=passed' "$live_evidence/summary.txt"

while IFS= read -r required; do [[ -f "$package_project_dir/Content/$required" ]]; done < <(jq -r '.packageAssertions.widget.file,.packageAssertions.host.file,.packageAssertions.map' "$manifest")
jq -e 'has("Modules") | not' "$package_project" >/dev/null
"$editor_cmd" "$package_project" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/package-compile.log" >"$work/package-compile.stdout" 2>&1
grep -E '0 error\(s\), 0 warning\(s\)' "$work/package-compile.stdout" >"$work/package-compile-proof.txt"
"${package_axi[@]}" project cook --output "$work/cooked" >"$work/cook.json"
"${package_axi[@]}" project package --output "$work/package" >"$work/package.json"
jq -e '.operation.status == "passed" and (.artifacts | length) > 0' "$work/cook.json" "$work/package.json" >/dev/null
for operation_json in "$work/cook.json" "$work/package.json"; do
  operation_id=$(jq -r .operation.id "$operation_json")
  "${package_axi[@]}" operation view "$operation_id" >"$work/operation-$operation_id.json"
  jq -e --arg id "$operation_id" '.operation.id == $id and .operation.status == "passed"' "$work/operation-$operation_id.json" >/dev/null
done
find "$work/package" -type d -name '*.app' -print -quit | grep -q .

utoc=$(find "$work/package" -type f -name 'MagiUnrealAXIPackageFixture-Mac.utoc' -print -quit); [[ -n "$utoc" ]]
unreal_pak="$engine_root/Engine/Binaries/Mac/UnrealPak"; [[ -x "$unreal_pak" ]]
"$unreal_pak" -ListContainer="$utoc" -Csv="$work/package-inventory/iostore.csv" >"$work/package-inventory/unrealpak.log" 2>&1
assert_iostore_export() { local expected=$1; awk -F',' -v expected="/Content/$expected" 'NR > 1 { filename=$5; chunk=$12; gsub(/^[[:space:]]+|[[:space:]]+$/, "", filename); gsub(/^[[:space:]]+|[[:space:]]+$/, "", chunk); if (substr(filename, length(filename) - length(expected) + 1) == expected && chunk == "ExportBundleData") found=1 } END { exit !found }' "$work/package-inventory/iostore.csv"; }
while IFS= read -r required; do assert_iostore_export "$required"; done < <(jq -r '.packageAssertions.widget.file,.packageAssertions.host.file,.packageAssertions.map' "$manifest")
registry=$(find "$work/cooked" -type f -name AssetRegistry.bin -print -quit); [[ -n "$registry" ]]
"$editor_cmd" "$package_project" -run=AssetRegistryDump "-input=$registry" "-outdir=$work/registry-dump" -unattended -nop4 -nosplash -nullrhi -NoSound >"$work/registry-dump.log" 2>&1
registry_files=$(find "$work/registry-dump" -type f -not -name '*.log' -print); [[ -n "$registry_files" ]]; registry_utf8="$work/registry-dump-utf8.txt"; : >"$registry_utf8"
while IFS= read -r registry_file; do bom=$(od -An -tx1 -N2 "$registry_file" | tr -d '[:space:]'); if [[ $bom == fffe || $bom == feff ]]; then iconv -f UTF-16 -t UTF-8 "$registry_file" >>"$registry_utf8"; else cat "$registry_file" >>"$registry_utf8"; fi; printf '\n' >>"$registry_utf8"; done < <(printf '%s\n' "$registry_files" | LC_ALL=C sort)
registry_text="$work/registry-dump-normalized-utf8.txt"; LC_ALL=C tr '[:upper:]' '[:lower:]' <"$registry_utf8" >"$registry_text"
registry_group_row() {
  local section=$1 expected=$2 file=$3
  awk -v section="$section" -v expected="$expected" '$0 == "--- begin " section " ---" { in_section=1; next } in_section && $0 ~ /^--- end / { exit found ? 0 : 1 } in_section { row=$0; sub(/^[[:space:]]+/, "", row); sub(/[[:space:]]+$/, "", row); if (row == expected) found=1 } END { exit found ? 0 : 1 }' "$file"
}
registry_property_row() {
  local property=$1 expected=$2 file=$3
  awk -v property="$property" -v expected="$expected" '{ line=$0; sub(/^[[:space:]]+/, "", line) } line ~ "^" property " :" { in_property=1; next } in_property && line ~ /^[^[:space:]]/ && line ~ / :/ { exit found ? 0 : 1 } in_property { row=line; sub(/[[:space:]]+$/, "", row); if (row == expected) found=1 } END { exit found ? 0 : 1 }' "$file"
}
for key in widget host; do
  object=$(jq -r ".packageAssertions.$key.object" "$manifest" | tr '[:upper:]' '[:lower:]')
  package=${object%.*}
  generated=$(jq -r ".packageAssertions.$key.generatedRuntime" "$manifest" | tr '[:upper:]' '[:lower:]')
  generated_type=/script/engine.blueprintgeneratedclass; [[ $key == widget ]] && generated_type=/script/umg.widgetblueprintgeneratedclass
  registry_group_row cachedassetsbyobjectpath "$object" "$registry_text"
  registry_property_row generatedclass "$object, $generated_type'$package.$generated'" "$registry_text"
done
map_package=$(jq -r .packageAssertions.mapPackage "$manifest" | tr '[:upper:]' '[:lower:]')
registry_property_row /script/engine.world "$map_package.$(basename "$map_package")" "$registry_text"
set +e
grep -R -I -E -q '/Script/(MagiUnrealAXIFixture|MagiUnrealAXI)' "$registry_text" "$work/package-inventory"
forbidden_status=$?
set -e
printf 'forbiddenReferences=%s\n' "$forbidden_status" >"$work/forbidden-reference-status.txt"
[[ $forbidden_status == 1 ]]
scan_all_runtime_material() { scan_secrets "$work"; scan_secrets "$evidence"; [[ -z "${P13_WORKSPACE:-}" ]] || scan_secrets "$P13_WORKSPACE"; }
inventory_tree "$work/cooked" "$work/package-inventory/cooked-tree.txt"; inventory_tree "$work/package" "$work/package-inventory/package-tree.txt"
cooked_inventory_hash=$(shasum -a 256 "$work/package-inventory/cooked-tree.txt" | cut -d' ' -f1); package_inventory_hash=$(shasum -a 256 "$work/package-inventory/package-tree.txt" | cut -d' ' -f1)
source_inventory "$work/source-inventory-after.txt"; source_inventory_after_hash=$(shasum -a 256 "$work/source-inventory-after.txt" | cut -d' ' -f1); diff -u "$work/source-inventory-before.txt" "$work/source-inventory-after.txt" >"$work/source-inventory.diff"; [[ ! -s "$work/source-inventory.diff" && "$source_inventory_hash" == "$source_inventory_after_hash" ]]
ditto "$live_evidence" "$evidence/live"
cp "$manifest" "$evidence/manifest.json"
cp "$work/catalog-check.txt" "$work/compile-proof.txt" "$work/package-compile-proof.txt" "$work/forbidden-reference-status.txt" "$evidence/"
cp "$work/source-inventory-before.txt" "$work/source-inventory-after.txt" "$work/source-inventory.diff" "$work/plugin-source-tree-before.txt" "$evidence/"
cp "$work/rust-tests.log" "$work/release-build.log" "$work/fmt.log" "$work/clippy.log" "$work/plugin-build.log" "$work/live-certification.log" "$work/registry-dump.log" "$evidence/"
cp "$work/project-build.json" "$work/test-list.json" "$work/test-run.json" "$work/cook.json" "$work/package.json" "$evidence/"
cp "$work"/operation-*.json "$evidence/"
ditto "$work/package-inventory" "$evidence/package-inventory"
ditto "$work/registry-dump" "$evidence/registry-dump"
cp "$work/registry-dump-utf8.txt" "$work/registry-dump-normalized-utf8.txt" "$evidence/"
cp "$work/report/index.json" "$evidence/automation-index.json"
printf 'phase=P1.3\ntarget=UE %s changelist %s host=%s\ncatalogCount=%s\ncatalogHash=%s\npluginArchitectures=%s\nartifactSha256=%s\npluginSourceTreeSha256=%s\nsourceInventorySha256=%s\nsourceInventoryAfterSha256=%s\ncliSha256=%s\ncookedInventorySha256=%s\npackageInventorySha256=%s\nrust=tests-release-fmt-clippy-capabilities-passed\nautomation=27/27-passed\nfixture=ui-state-loop\nblueprintOnlyCookPackage=passed\niostoreAssetRegistry=passed\ntokenScan=passed\n' "$(jq -r .engine.version "$manifest")" "$(jq -r .engine.changelist "$manifest")" "$(uname -m)" "$catalog_count" "$catalog_hash" "$arches" "$artifact_hash" "$plugin_source_hash" "$source_inventory_hash" "$source_inventory_after_hash" "$cli_hash" "$cooked_inventory_hash" "$package_inventory_hash" | tee "$evidence/summary.txt"
scan_all_runtime_material
printf '%s\n' "$evidence" >"$cache_root/latest"
echo "P1.3 native certification: PASS (evidence retained at $evidence)"
