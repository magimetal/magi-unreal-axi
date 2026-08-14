#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.6-manifest.json"
artifact=${1-}
fail(){ echo "P1.6 combined certification failed: $*" >&2; exit 1; }
[[ $# == 1 && -f "$artifact" ]] || fail "usage: $0 ARTIFACT.tar.gz"
[[ -n "${P16_EXPECTED_ARTIFACT_SHA256:-}" && "$P16_EXPECTED_ARTIFACT_SHA256" =~ ^[0-9a-fA-F]{64}$ ]] || fail "P16_EXPECTED_ARTIFACT_SHA256 required"
expected_source_commit=${P16_EXPECTED_SOURCE_COMMIT:-}
[[ "$expected_source_commit" =~ ^[0-9a-f]{40}$ ]] || fail "P16_EXPECTED_SOURCE_COMMIT required as lowercase 40-hex commit"
[[ "$(git -C "$repo_root" rev-parse HEAD)" == "$expected_source_commit" ]] || fail "P16_EXPECTED_SOURCE_COMMIT differs from HEAD"
git -C "$repo_root" diff --quiet || fail "tracked worktree must be clean for source provenance"
git -C "$repo_root" diff --cached --quiet || fail "index must be clean for source provenance"
[[ -z $(git -C "$repo_root" ls-files --others --exclude-standard) ]] || fail "untracked files must be absent for source provenance"
for command in jq shasum tar file codesign lipo ditto trash; do command -v "$command" >/dev/null || fail "$command missing"; done
actual_home=${HOME:?HOME required}
cache="$actual_home/Library/Caches/magi-unreal-axi/p1.6/combined"; mkdir -p "$cache"
artifact=$(cd "$(dirname "$artifact")" && pwd -P)/$(basename "$artifact")
stage_dir=$(mktemp -d "$cache/artifact.XXXXXX")
staged_artifact="$stage_dir/$(basename "$artifact")"
cp -p -- "$artifact" "$staged_artifact"
[[ -f "$(dirname "$artifact")/SHA256SUMS" ]] && cp -p -- "$(dirname "$artifact")/SHA256SUMS" "$stage_dir/SHA256SUMS"
export P16_ARTIFACT_STAGED=1 P16_ARTIFACT_STAGE_DIR="$stage_dir"
kickoff_log=$(mktemp "$cache/kickoff.log.XXXXXX")
"$repo_root/tests/unreal/verify-p1.6-kickoff.sh" "$staged_artifact" >"$kickoff_log" 2>&1 || { cat "$kickoff_log" >&2; exit 1; }

engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
dotnet_root="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -x "$run_uat" && -x "$editor_cmd" && -x "$dotnet_root/dotnet" && -f "$version_file" ]] || fail "UE 5.8.1 toolchain missing"
expected_engine_version=$(jq -r '.engine.version' "$manifest")
expected_changelist=$(jq -r '.engine.changelist' "$manifest")
expected_host_arch=$(jq -r '.engine.hostArchitecture' "$manifest")
[[ "$(uname -m)" == "$expected_host_arch" ]] || fail "host architecture differs from manifest"
[[ "$(plutil -extract Changelist raw -o - "$version_file")" == "$expected_changelist" ]] || fail "engine changelist differs from manifest"
actual_engine_version="$(plutil -extract MajorVersion raw -o - "$version_file").$(plutil -extract MinorVersion raw -o - "$version_file").$(plutil -extract PatchVersion raw -o - "$version_file")"
[[ "$actual_engine_version" == "$expected_engine_version" ]] || fail "engine version differs from manifest"
[[ -x "$run_uat" && -x "$editor_cmd" && -x "$dotnet_root/dotnet" ]] || fail "UE 5.8.1 toolchain missing"
version=$(sed -n 's/^version = "\([^"]*\)"/\1/p' "$repo_root/Cargo.toml" | head -1)
root="magi-unreal-axi-${version}-macos-arm64"
work=$(mktemp -d "$cache/work.XXXXXX"); evidence=$(mktemp -d "$cache/evidence.XXXXXX")
cleanup(){ local s=$?; trap - EXIT; [[ $s == 0 ]] || echo "P1.6 combined work retained: $work; evidence: $evidence" >&2; [[ $s == 0 && ${KEEP_P16_WORK:-0} != 1 ]] && trash "$work" "$stage_dir" "$kickoff_log" 2>/dev/null || true; exit "$s"; }; trap cleanup EXIT

tar -xzf "$staged_artifact" -C "$work"
bin="$work/$root/magi-unreal-axi"; [[ -x "$bin" && ! -L "$bin" ]] || fail "exact binary missing"
codesign --verify --strict "$bin" >/dev/null 2>&1 || fail "exact binary codesign failed"
cli_hash=$(shasum -a 256 "$bin"|cut -d' ' -f1); catalog_hash=$(jq -r .catalog.sha256 "$manifest"); grep -aFq "$catalog_hash" "$bin" || fail "exact binary catalog hash mismatch"
"$bin" --version >"$work/version.txt"; grep -Fxq "magi-unreal-axi $version" "$work/version.txt" || fail "exact binary version mismatch"

export DOTNET_ROOT="$dotnet_root" PATH="$dotnet_root:$PATH"
source_inventory(){ ruby "$repo_root/tests/unreal/support/p16-provenance.rb" inventory "$repo_root" "$1"; }
source_inventory "$work/source-before.txt"; source_before=$(shasum -a 256 "$work/source-before.txt"|cut -d' ' -f1)
ruby "$repo_root/tests/unreal/support/p16-provenance.rb" verify-worktree "$repo_root" "$work/source-before.txt" "$expected_source_commit" >/dev/null || fail "working tree differs from source commit"
# Exact embedded source is materialized only by exact binary, then BuildPlugin consumes that tree.
mkdir -p "$work/install-project/Plugins"
cp "$repo_root/tests/unreal/MagiUnrealAXIFixture/MagiUnrealAXIFixture.uproject" "$work/install-project/"
project="$work/install-project/MagiUnrealAXIFixture.uproject"
installed="$work/install-project/Plugins/MagiUnrealAXI"
HOME="$work/isolated-home" "$bin" --project "$project" --engine "$engine_root" --format json setup plugin install >"$work/plugin-install-first.json"
HOME="$work/isolated-home" "$bin" --project "$project" --engine "$engine_root" --format json setup plugin install >"$work/plugin-install-repeat.json"
HOME="$work/isolated-home" "$bin" --project "$project" --engine "$engine_root" --format json setup plugin status >"$work/plugin-status.json"
HOME="$work/isolated-home" "$bin" --project "$project" --engine "$engine_root" --format json setup agents --codex --claude --opencode >"$work/agent-setup-first.json"
settings="$work/isolated-home/.claude/settings.json"
[[ -s "$settings" ]]
settings_hash=$(shasum -a 256 "$settings" | cut -d' ' -f1)
HOME="$work/isolated-home" "$bin" --project "$project" --engine "$engine_root" --format json setup agents --codex --claude --opencode >"$work/agent-setup-repeat.json"
[[ $(shasum -a 256 "$settings" | cut -d' ' -f1) == "$settings_hash" ]]
for skill_root in "$work/isolated-home/.claude/skills" "$work/isolated-home/.agents/skills" "$work/isolated-home/.config/opencode/skills"; do [[ -s "$skill_root/magi-unreal-axi/SKILL.md" ]]; done
hook=$(jq -r '[.hooks.SessionStart[] | .hooks[] | select(.command | endswith(" agent context --format json")) | .command] | if length == 1 then .[0] else empty end' "$settings")
[[ -n "$hook" ]]
(cd "$work/install-project" && HOME="$work/isolated-home" /bin/sh -c "$hook") >"$work/agent-context.json"
[[ $(wc -c <"$work/agent-context.json" | tr -d ' ') -lt 400 ]]
[[ -f "$installed/MagiUnrealAXI.uplugin" ]]
jq -e '.plugin.changed == true' "$work/plugin-install-first.json" >/dev/null
jq -e '.plugin.changed == false and .plugin.compatible == true' "$work/plugin-install-repeat.json" >/dev/null
jq -e '.plugin.compatible == true' "$work/plugin-status.json" >/dev/null
jq -e '.changed == true' "$work/agent-setup-first.json" >/dev/null
jq -e '.changed == false' "$work/agent-setup-repeat.json" >/dev/null
jq -e '.magiUnrealAxi.project.name == "MagiUnrealAXIFixture" and .magiUnrealAxi.project.pluginInstalled == true' "$work/agent-context.json" >/dev/null
source_tree_inventory() { local root=$1; (cd "$root" && find . -type f -print | LC_ALL=C sort); }
source_tree_inventory "$repo_root/plugin/MagiUnrealAXI/Source/MagiUnrealAXI" >"$work/reviewed-plugin-source.txt"
source_tree_inventory "$installed/Source/MagiUnrealAXI" >"$work/installed-plugin-source.txt"
diff -u "$work/reviewed-plugin-source.txt" "$work/installed-plugin-source.txt" >/dev/null || fail "embedded installed plugin source inventory differs"
while IFS= read -r relative; do cmp -s "$repo_root/plugin/MagiUnrealAXI/Source/MagiUnrealAXI/${relative#./}" "$installed/Source/MagiUnrealAXI/${relative#./}" || fail "embedded installed source differs: $relative"; done <"$work/reviewed-plugin-source.txt"
jq -e '.EngineVersion == "5.8.0" and .Installed == true and .CreatedBy == "" and .CreatedByURL == "" and .DocsURL == "" and .MarketplaceURL == "" and .SupportURL == ""' "$installed/MagiUnrealAXI.uplugin" >/dev/null
jq -S . "$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" >"$work/reviewed-plugin-descriptor.json"
jq -S 'del(.CreatedBy,.CreatedByURL,.DocsURL,.MarketplaceURL,.SupportURL,.EngineVersion,.Installed)' "$installed/MagiUnrealAXI.uplugin" >"$work/installed-plugin-descriptor-normalized.json"
diff -u "$work/reviewed-plugin-descriptor.json" "$work/installed-plugin-descriptor-normalized.json" >/dev/null || fail "embedded plugin descriptor semantics differ"
[[ ! -e "$installed/Config" ]] || fail "unreviewed plugin packaging config embedded"
{
  printf '%s\n' MagiUnrealAXI.uplugin
  sed 's#^\./#Source/MagiUnrealAXI/#' "$work/reviewed-plugin-source.txt"
} | LC_ALL=C sort >"$work/reviewed-plugin-build-inputs.txt"
(cd "$installed" && { printf '%s\n' MagiUnrealAXI.uplugin; find Source -type f -print; } | LC_ALL=C sort) >"$work/installed-plugin-build-inputs.txt"
diff -u "$work/reviewed-plugin-build-inputs.txt" "$work/installed-plugin-build-inputs.txt" >/dev/null || fail "embedded plugin build-input inventory differs"
printf 'reviewedSha256=%s\ninstalledSha256=%s\n' "$(shasum -a 256 "$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" | cut -d' ' -f1)" "$(shasum -a 256 "$installed/MagiUnrealAXI.uplugin" | cut -d' ' -f1)" >"$work/plugin-descriptor-sha256.txt"
installed_binary_count=$(find "$installed" -type f -name libUnrealEditor-MagiUnrealAXI.dylib | wc -l | tr -d ' ')
[[ "$installed_binary_count" == 1 ]] || fail "installed exact plugin must contain one dylib"
installed_binary=$(find "$installed" -type f -name libUnrealEditor-MagiUnrealAXI.dylib -print -quit)
installed_binary_hash=$(shasum -a 256 "$installed_binary" | cut -d' ' -f1)
ditto "$installed" "$work/exact-plugin"
plugin_binary_count=$(find "$work/exact-plugin" -type f -name libUnrealEditor-MagiUnrealAXI.dylib | wc -l | tr -d ' '); [[ "$plugin_binary_count" == 1 ]] || fail "exact plugin must contain one dylib"
HOME="$work/isolated-home" "$bin" --project "$project" --engine "$engine_root" --format json setup plugin uninstall --force >"$work/plugin-uninstall.json"
jq -e '.plugin.installed == false and .plugin.changed == true' "$work/plugin-uninstall.json" >/dev/null
[[ ! -e "$installed" ]] || fail "plugin uninstall did not remove installed plugin"
plugin_inventory=$(find "$work/exact-plugin" -type f -print | LC_ALL=C sort | while IFS= read -r path; do printf '%s\t%s\n' "${path#"$work/exact-plugin/"}" "$(shasum -a 256 "$path" | cut -d' ' -f1)"; done | shasum -a 256 | cut -d' ' -f1)
printf '%s\n' "$plugin_inventory" >"$work/plugin-inventory-sha256.txt"
plugin_binary=$(find "$work/exact-plugin" -type f -name libUnrealEditor-MagiUnrealAXI.dylib -print -quit); [[ -n "$plugin_binary" ]]
arches=$(lipo -archs "$plugin_binary"); grep -qw arm64 <<<"$arches"; grep -qw x86_64 <<<"$arches"; [[ $(wc -w <<<"$arches"|tr -d ' ') == 2 ]]
plugin_hash=$(shasum -a 256 "$plugin_binary"|cut -d' ' -f1)
[[ "$plugin_hash" == "$installed_binary_hash" ]] || fail "copied exact plugin binary hash mismatch"

# Native control and all four loops reuse exact binary and exact plugin package. Automation runs once.
copy_tracked_tree() { local source=$1 destination=$2 relative; mkdir -p "$destination"; while IFS= read -r -d '' relative; do relative=${relative#"$source"/}; [[ -f "$repo_root/$source/$relative" && ! -L "$repo_root/$source/$relative" ]] || fail "untracked or non-regular fixture entry: $relative"; mkdir -p "$destination/$(dirname "$relative")"; cp -p "$repo_root/$source/$relative" "$destination/$relative"; done < <(git -C "$repo_root" ls-files -z -- "$source/"); }
for forbidden in Source Modules Plugins Binaries Intermediate Saved DDC DerivedDataCache Content; do [[ ! -e "$work/native/$forbidden" ]] || fail "native fixture contains forbidden generated path: $forbidden"; done
mkdir -p "$work/native/Plugins"; copy_tracked_tree tests/unreal/MagiUnrealAXIFixture "$work/native"
copy_tracked_tree tests/unreal/MagiP15AnimationSeed/Content "$work/native/Content"
ditto "$work/exact-plugin" "$work/native/Plugins/MagiUnrealAXI"
copied_native_plugin=$(find "$work/native/Plugins/MagiUnrealAXI" -type f -name libUnrealEditor-MagiUnrealAXI.dylib -print -quit); [[ -n "$copied_native_plugin" && $(shasum -a 256 "$copied_native_plugin" | cut -d' ' -f1) == "$plugin_hash" ]] || fail "native plugin hash mismatch"
"$repo_root/tests/unreal/verify-p1.5-seed.sh" >"$work/seed-verification.log" 2>&1
native_project="$work/native/MagiUnrealAXIFixture.uproject"
"$bin" --project "$native_project" --engine "$engine_root" --timeout 1800 --format json project build >"$work/native-build.json"
jq -e '.operation.status == "passed" or .operation.status == "up_to_date"' "$work/native-build.json" >/dev/null
/usr/bin/trash "$work/native/Plugins/MagiUnrealAXI/Binaries"
ditto "$work/exact-plugin/Binaries" "$work/native/Plugins/MagiUnrealAXI/Binaries"
native_loaded_plugin=$(find "$work/native/Plugins/MagiUnrealAXI" -type f -name libUnrealEditor-MagiUnrealAXI.dylib -print -quit)
[[ -n "$native_loaded_plugin" && $(shasum -a 256 "$native_loaded_plugin" | cut -d' ' -f1) == "$plugin_hash" ]] || fail "native post-build plugin hash mismatch"
"$editor_cmd" "$native_project" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/native-compile.log" >"$work/native-compile.stdout" 2>&1
grep -E '0 error\(s\), 0 warning\(s\)' "$work/native-compile.stdout" >"$work/native-compile-proof.txt"
"$bin" --project "$native_project" --engine "$engine_root" --timeout 1800 --format json project test list --filter MagiUnrealAXI --limit 100 >"$work/test-list.json"
jq -e --slurpfile m "$manifest" '.count == 38 and .total == 38 and ([.items[].id] | sort) == ($m[0].automation.tests | sort)' "$work/test-list.json" >/dev/null
mkdir -p "$work/report"
"$bin" --project "$native_project" --engine "$engine_root" --timeout 1800 --format json project test run --filter MagiUnrealAXI --report "$work/report" >"$work/test-run.json"
jq -e '.totals.matched == 38 and (.totals.succeeded + .totals.succeededWithWarnings) == 38 and .totals.failed == 0 and .totals.notRun == 0 and .totals.inProcess == 0 and .totals.errors == 0 and .totals.complete == true' "$work/test-run.json" >/dev/null
jq -e --slurpfile m "$manifest" '(.tests | map({test:.id,warnings}) | sort_by(.test)) == ($m[0].automation.warningBaseline | sort_by(.test)) and .totals.warnings == ([$m[0].automation.warningBaseline[].warnings] | add)' "$work/test-run.json" >/dev/null
jq -e --slurpfile m "$manifest" --slurpfile p15 "$repo_root/tests/unreal/p1.5-manifest.json" '
  (.tests | length) == 38 and
  (.tests | map(.fullTestPath) | sort) == ($m[0].automation.tests | sort) and
  all(.tests[]; (.state == "Success" or .state == "SuccessWithWarnings") and .errors == 0) and
  ([.tests[] | select(.warnings > 0) | {test:.fullTestPath,warnings}] | sort_by(.test)) == ($m[0].automation.warningBaseline | sort_by(.test)) and
  ([.tests[].warnings] | add) == ([$m[0].automation.warningBaseline[].warnings] | add) and
  ([.tests[] as $test | select(($p15[0].nativeTests | index($test.fullTestPath)) != null and $test.state == "Success" and $test.warnings == 0 and $test.errors == 0)] | length) == 5
' "$work/report/index.json" >/dev/null
[[ $(shasum -a 256 "$native_loaded_plugin" | cut -d' ' -f1) == "$plugin_hash" ]] || fail "native post-automation plugin hash mismatch"

package_project_dir="$work/combined-package"
mkdir -p "$package_project_dir/Config"
cp "$repo_root/tests/unreal/MagiUnrealAXIPackageFixture/MagiUnrealAXIPackageFixture.uproject" "$package_project_dir/"
cat >"$package_project_dir/Config/DefaultEngine.ini" <<'EOF'
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/MagiP12/P12Interaction
GlobalDefaultGameMode=/Script/Engine.GameModeBase
EOF
{
  echo '[/Script/UnrealEd.ProjectPackagingSettings]'
  echo 'UsePakFile=True'
  echo 'bUseIoStore=True'
  echo 'bUseZenStore=False'
  for m in 2 3 4 5; do jq -r '.packageAssertions.mapPackage // .packageAssertions.map.package' "$repo_root/tests/unreal/p1.$m-manifest.json"; done | while IFS= read -r map; do
    [[ -n "$map" && "$map" != null ]] || fail "missing map package"
    printf '+MapsToCook=(FilePath="%s")\n' "$map"
  done
  for root_name in MagiP12 MagiP13 MagiP14 MagiP15 MagiP15Seed; do printf '+DirectoriesToAlwaysCook=(Path="/Game/%s")\n' "$root_name"; done
} >"$package_project_dir/Config/DefaultGame.ini"
for forbidden in Source Modules Plugins Binaries Intermediate Saved DDC DerivedDataCache Content; do [[ ! -e "$package_project_dir/$forbidden" ]] || fail "package fixture not clean: $forbidden"; done
package_project="$package_project_dir/MagiUnrealAXIPackageFixture.uproject"
package_axi=("$bin" --project "$package_project" --engine "$engine_root" --timeout 7200 --format json)

for spec in 2 3 4 5; do
  evidence_dir="$work/loop-$spec"
  env "P1${spec}_ACCOUNT_HOME=$actual_home" "P1${spec}_LIVE_EVIDENCE_DIR=$evidence_dir" "P1${spec}_PACKAGE_PROJECT_DIR=$package_project_dir" "P1${spec}_CLI_PATH=$bin" "P1${spec}_CLI_SHA256=$cli_hash" "P1${spec}_PLUGIN_DIR=$work/exact-plugin" "P1${spec}_PLUGIN_SHA256=$plugin_hash" "P1${spec}_CATALOG_COUNT=79" "P1${spec}_CATALOG_HASH=$catalog_hash" "P1${spec}_WORKSPACE=$work" "$repo_root/tests/unreal/certify-p1.$spec-live.sh" >"$work/loop-$spec.log" 2>&1
  summary="$evidence_dir/summary.txt"
  grep -Fxq "phase=P1.$spec" "$summary"
  grep -Fxq "catalogHash=$catalog_hash" "$summary"
  grep -Fxq "artifactSha256=$plugin_hash" "$summary"
  grep -Fxq "cliSha256=$cli_hash" "$summary"
  grep -Fxq 'tokenScan=passed' "$summary"
  case $spec in
    2)
      grep -Fxq 'fixture=one-interface-two-distinct-actor-blueprints' "$summary"
      grep -Fxq 'pieSessions=2-deterministic-reset' "$summary"
      [[ -f "$evidence_dir/package-materialization.txt" ]]
      ;;
    3)
      grep -Fxq 'fixture=ui-state-loop' "$summary"
      grep -Fxq 'package=Content/MagiP13' "$summary"
      ;;
    4)
      grep -Fxq 'fixture=ai-navigation-loop' "$summary"
      grep -Fxq 'package=Content/MagiP14' "$summary"
      ;;
    5)
      grep -Fxq 'noops=8/8-preserved-revisions' "$summary"
      grep -Fxq 'package=Content/MagiP15+Content/MagiP15Seed' "$summary"
      ;;
  esac
done

mkdir -p "$work/package-inventory" "$work/registry-dump"
expected_files="$work/expected-package-files.txt"
{
  jq -r '.packageAssertions.interface.file,.packageAssertions.targetBlueprint.file,.packageAssertions.playerBlueprint.file,.packageAssertions.map' "$repo_root/tests/unreal/p1.2-manifest.json"
  jq -r '.packageAssertions.widget.file,.packageAssertions.host.file,.packageAssertions.map' "$repo_root/tests/unreal/p1.3-manifest.json"
  jq -r '.packageAssertions.blackboard.file,.packageAssertions.behaviorTree.file,.packageAssertions.controller.file,.packageAssertions.pawn.file,.packageAssertions.floor.file,.packageAssertions.map' "$repo_root/tests/unreal/p1.4-manifest.json"
  jq -r '.packageAssertions.animationBlueprint.file,.packageAssertions.character.file,.packageAssertions.map.file,.packageAssertions.seedFiles[]' "$repo_root/tests/unreal/p1.5-manifest.json"
} | LC_ALL=C sort -u >"$expected_files"
[[ $(wc -l <"$expected_files" | tr -d ' ') == 22 ]]
while IFS= read -r expected; do [[ -f "$package_project_dir/Content/$expected" ]] || fail "missing expected package file: $expected"; done <"$expected_files"
(cd "$package_project_dir/Content" && find . -type f -print | sed 's#^\./##' | LC_ALL=C sort) >"$work/aggregate-content.txt"
diff -u "$expected_files" "$work/aggregate-content.txt" >/dev/null || fail "aggregate Content inventory differs from exact manifest set"
for forbidden in Source Modules Plugins Binaries Intermediate Saved DDC DerivedDataCache; do [[ ! -e "$package_project_dir/$forbidden" ]] || fail "Blueprint-only package has forbidden directory: $forbidden"; done
"$editor_cmd" "$package_project" -run=CompileAllBlueprints -unattended -nop4 -nosplash -nullrhi -NoSound -log="$work/package-compile.log" >"$work/package-compile.stdout" 2>&1
grep -E '0 error\(s\), 0 warning\(s\)' "$work/package-compile.stdout" >"$work/package-compile-proof.txt"

"${package_axi[@]}" project cook --output "$work/cooked" >"$work/cook.json"
"${package_axi[@]}" project package --output "$work/package" >"$work/package.json"
for operation_json in "$work/cook.json" "$work/package.json"; do
  operation_id=$(jq -r .operation.id "$operation_json")
  [[ "$operation_id" =~ ^[A-Za-z0-9._:-]+$ ]]
  "${package_axi[@]}" operation view "$operation_id" >"$work/operation-$operation_id.json"
  jq -e --arg id "$operation_id" '.operation.id == $id and .operation.status == "passed"' "$work/operation-$operation_id.json" >/dev/null
done
find "$work/package" -type d -name '*.app' -print -quit | grep -q .
jq -se 'all(.[]; .operation.status == "passed" and (.artifacts | length) > 0)' "$work/cook.json" "$work/package.json" >/dev/null
utoc=$(find "$work/package" -type f -name 'MagiUnrealAXIPackageFixture-Mac.utoc' -print -quit)
[[ -n "$utoc" ]]
unreal_pak="$engine_root/Engine/Binaries/Mac/UnrealPak"
[[ -x "$unreal_pak" ]]
"$unreal_pak" -ListContainer="$utoc" -Csv="$work/package-inventory/iostore.csv" >"$work/package-inventory/unrealpak.log" 2>&1
assert_iostore_export() {
  local expected=$1
  awk -F',' -v expected="/Content/$expected" 'NR > 1 { filename=$5; chunk=$12; gsub(/^[[:space:]]+|[[:space:]]+$/, "", filename); gsub(/^[[:space:]]+|[[:space:]]+$/, "", chunk); if (substr(filename, length(filename)-length(expected)+1) == expected && chunk == "ExportBundleData") found=1 } END { exit !found }' "$work/package-inventory/iostore.csv"
}
while IFS= read -r expected; do assert_iostore_export "$expected"; done <"$expected_files"

registry=$(find "$work/cooked" -type f -name AssetRegistry.bin -print -quit)
[[ -n "$registry" ]]
"$editor_cmd" "$package_project" -run=AssetRegistryDump "-input=$registry" "-outdir=$work/registry-dump" -unattended -nop4 -nosplash -nullrhi -NoSound >"$work/registry-dump.log" 2>&1
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
  local section=$1 expected=$2
  awk -v section="$section" -v expected="$expected" '$0 == "--- begin " section " ---" { in_section=1; next } in_section && $0 ~ /^--- end / { exit found ? 0 : 1 } in_section { row=$0; sub(/^[[:space:]]+/, "", row); sub(/[[:space:]]+$/, "", row); if (row == expected) found=1 } END { exit found ? 0 : 1 }' "$registry_text"
}
registry_property_row() {
  local property=$1 expected=$2
  awk -v property="$property" -v expected="$expected" '{ line=$0; sub(/^[[:space:]]+/, "", line) } line ~ "^" property " :" { in_property=1; next } in_property && line ~ /^[^[:space:]]/ && line ~ / :/ { exit found ? 0 : 1 } in_property { row=line; sub(/[[:space:]]+$/, "", row); if (row == expected) found=1 } END { exit found ? 0 : 1 }' "$registry_text"
}
expected_objects="$work/expected-registry-objects.txt"
{
  jq -r '.packageAssertions.interface.object,.packageAssertions.targetBlueprint.object,.packageAssertions.playerBlueprint.object' "$repo_root/tests/unreal/p1.2-manifest.json"
  jq -r '.packageAssertions.widget.object,.packageAssertions.host.object' "$repo_root/tests/unreal/p1.3-manifest.json"
  jq -r '.packageAssertions.blackboard.object,.packageAssertions.behaviorTree.object,.packageAssertions.controller.object,.packageAssertions.pawn.object,.packageAssertions.floor.object' "$repo_root/tests/unreal/p1.4-manifest.json"
  jq -r '.packageAssertions.animationBlueprint.object,.packageAssertions.character.object,.packageAssertions.seedObjects[]' "$repo_root/tests/unreal/p1.5-manifest.json"
} | LC_ALL=C tr '[:upper:]' '[:lower:]' | sort -u >"$expected_objects"
[[ $(wc -l <"$expected_objects" | tr -d ' ') == 18 ]]
while IFS= read -r object; do registry_group_row cachedassetsbyobjectpath "$object"; done <"$expected_objects"
assert_generated_class() {
  local manifest_file=$1 key=$2 generated_type=$3 object package generated
  object=$(jq -r ".packageAssertions.$key.object" "$manifest_file" | tr '[:upper:]' '[:lower:]')
  package=${object%.*}
  generated=$(jq -r ".packageAssertions.$key.generatedRuntime" "$manifest_file" | tr '[:upper:]' '[:lower:]')
  registry_property_row generatedclass "$object, $generated_type'$package.$generated'"
}
assert_generated_class "$repo_root/tests/unreal/p1.2-manifest.json" interface /script/engine.blueprintgeneratedclass
assert_generated_class "$repo_root/tests/unreal/p1.2-manifest.json" targetBlueprint /script/engine.blueprintgeneratedclass
assert_generated_class "$repo_root/tests/unreal/p1.2-manifest.json" playerBlueprint /script/engine.blueprintgeneratedclass
assert_generated_class "$repo_root/tests/unreal/p1.3-manifest.json" widget /script/umg.widgetblueprintgeneratedclass
assert_generated_class "$repo_root/tests/unreal/p1.3-manifest.json" host /script/engine.blueprintgeneratedclass
for key in controller pawn floor; do assert_generated_class "$repo_root/tests/unreal/p1.4-manifest.json" "$key" /script/engine.blueprintgeneratedclass; done
assert_generated_class "$repo_root/tests/unreal/p1.5-manifest.json" animationBlueprint /script/engine.animblueprintgeneratedclass
assert_generated_class "$repo_root/tests/unreal/p1.5-manifest.json" character /script/engine.blueprintgeneratedclass
for m in 2 3 4 5; do
  map_package=$(jq -r '.packageAssertions.mapPackage // .packageAssertions.map.package' "$repo_root/tests/unreal/p1.$m-manifest.json" | tr '[:upper:]' '[:lower:]')
  registry_property_row /script/engine.world "$map_package.$(basename "$map_package")"
done
set +e
grep -I -E -i -q '/script/(magiunrealaxifixture|magiunrealaxi)' "$registry_text" "$work/package-inventory/iostore.csv"
forbidden_status=$?
set -e
[[ $forbidden_status == 1 ]]
printf 'forbiddenReferences=%s\n' "$forbidden_status" >"$work/forbidden-reference-status.txt"

inventory_tree() { local root=$1 out=$2; [[ -z $(find "$root" ! -type d ! -type f -print -quit) ]]; (cd "$root" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s\t%s\n' "${file#./}" "$(shasum -a 256 "$file" | cut -d' ' -f1)"; done) >"$out"; }
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
cooked_marker=$(awk -F '\t' '$1 ~ /(^|\/)AssetRegistry\.bin$/ { print $1; exit }' "$work/package-inventory/cooked-tree.txt")
[[ -n "$cooked_marker" ]]
bind_inventory "$work/package-inventory/cooked-tree.txt" "$work/cook.json" cooked-output "$work/cooked" "$cooked_marker"
bind_inventory "$work/package-inventory/package-tree.txt" "$work/package.json" package-output "$work/package" .magi-unreal-axi-package.json

trace_tsv="$work/p1-operation-traceability.tsv"
cat >"$trace_tsv" <<'EOF'
blueprint.create	loop-2/target-create.json	json
blueprint.graph_view	loop-2/target-graphs.json	json
blueprint.event_ensure	loop-2/p12.target.overlap.json	json
blueprint.node_ensure	loop-2/p12.target.message.json	json
blueprint.pin_default_set	loop-2/default-target-x.json	json
blueprint.pin_connect	loop-2/connect-target-overlap-message.json	json
blueprint.interface_create	loop-2/interface-create.json	json
blueprint.interface_view	loop-2/interface-view.json	json
blueprint.interface_ensure	loop-2/player-interface.json	json
blueprint.scs_view	loop-2/target-scs-after-remove.json	json
blueprint.scs_component_ensure	loop-2/target-root.json	json
blueprint.scs_component_update	loop-2/target-box-first-update.json	json
blueprint.scs_component_remove	loop-2/target-box-remove.json	json
play.component_observe	loop-2/component-target-one.json	json
widget.create	loop-3/widget-create.json	json
widget.tree_view	loop-3/missing-revision-before.json	json
widget.child_ensure	loop-3/widget-child.json	json
widget.property_set	loop-3/widget-text.json	json
widget.event_ensure	loop-3/widget-event.json	json
widget.viewport_ensure	loop-3/viewport.json	json
play.ui_observe	loop-3/ui-one-ready.json	json
ai.controller_configure	loop-4/controller-configure.json	json
ai.pawn_configure	loop-4/pawn-configure.json	json
behavior_tree.connect	loop-4/bt-link-loop-move.json	json
behavior_tree.create	loop-4/bt-create.json	json
behavior_tree.node_ensure	loop-4/bt-node-loop.json	json
behavior_tree.view	loop-4/behavior-tree-restart.json	json
blackboard.create	loop-4/bb-create.json	json
blackboard.key_ensure	loop-4/bb-key.json	json
blackboard.view	loop-4/blackboard-restart.json	json
navigation.build	loop-4/nav-build.json	json
navigation.status	loop-4/nav-status.json	json
navigation.bounds_ensure	loop-4/bounds.json	json
navigation.path_query	loop-4/path.json	json
play.ai_target_set	loop-4/target-set.json	json
play.ai_observe	loop-4/ai-observe.json	json
animation_blueprint.create	loop-5/abp-create.json	json
animation.character_configure	loop-5/character-configure.json	json
animation.character_view	loop-5/character-restart.json	json
animation.graph_view	loop-5/graph-restart.json	json
animation.state_ensure	loop-5/state-idle.json	json
animation.state_machine_ensure	loop-5/machine.json	json
animation.transition_ensure	loop-5/transition-out.json	json
animation.variable_ensure	loop-5/variable.json	json
play.animation_observe	report/index.json	MagiUnrealAXI.P15.PublicRuntimeAnimationObserve
EOF
[[ $(wc -l <"$trace_tsv" | tr -d ' ') == 45 ]]
cut -f1 "$trace_tsv" | LC_ALL=C sort >"$work/trace-operations.txt"
jq -r '.p1Operations[]' "$manifest" | LC_ALL=C sort >"$work/manifest-p1-operations.txt"
diff -u "$work/manifest-p1-operations.txt" "$work/trace-operations.txt" >/dev/null
: >"$work/p1-operation-traceability.jsonl"
while IFS=$'\t' read -r operation relative proof; do
  path="$work/$relative"
  [[ -s "$path" ]] || fail "missing traceability evidence for $operation: $relative"
  if [[ $proof == json ]]; then
    jq -e 'type == "object"' "$path" >/dev/null
  else
    jq -e --arg test "$proof" 'any(.tests[]; .fullTestPath == $test and (.state == "Success" or .state == "SuccessWithWarnings") and .errors == 0)' "$path" >/dev/null
  fi
  jq -cn --arg operation "$operation" --arg evidence "$relative" --arg proof "$proof" --arg sha256 "$(shasum -a 256 "$path" | cut -d' ' -f1)" '{operation:$operation,evidence:$evidence,proof:$proof,sha256:$sha256}' >>"$work/p1-operation-traceability.jsonl"
done <"$trace_tsv"
jq -s 'sort_by(.operation)' "$work/p1-operation-traceability.jsonl" | ruby -rjson -e 'p=ARGV.fetch(0); canonical=->(v){case v; when Hash then v.keys.sort_by{|k| k.encode(Encoding::UTF_8).bytes}.to_h{|k| [k,canonical.call(v.fetch(k))]}; when Array then v.map{|x| canonical.call(x)}; else v end}; File.binwrite(p,JSON.generate(canonical.call(JSON.parse(STDIN.read)),ascii_only:false)+"\n")' "$work/p1-operation-traceability.json"
mkdir -p "$evidence" 2>/dev/null || true
ruby "$repo_root/tests/unreal/support/p16-proofs.rb" verify "$work" "$repo_root/tests/unreal/p1.6-operation-proofs.json" "$manifest" "$repo_root/capabilities/catalog.json" "$repo_root" || fail "strict operation evidence verification"
jq -e --slurpfile m "$manifest" 'length == 45 and ([.[].operation] | unique | length) == 45 and ([.[].operation] | sort) == ($m[0].p1Operations | sort) and all(.[]; .sha256 | test("^[0-9a-f]{64}$"))' "$work/p1-operation-traceability.json" >/dev/null

source_inventory "$work/source-after.txt"
source_after=$(shasum -a 256 "$work/source-after.txt" | cut -d' ' -f1)
diff -u "$work/source-before.txt" "$work/source-after.txt" >"$work/source.diff"
[[ ! -s "$work/source.diff" && "$source_before" == "$source_after" ]]
if find "$work" -type f \( -name token -o -name bridge-v1.json \) -print -quit | grep -q .; then fail "retained runtime token"; fi

cp "$manifest" "$evidence/manifest.json"
cp "$kickoff_log" "$evidence/kickoff.log"
cp "$settings" "$evidence/claude-settings.json"
printf '%s\n' "$settings_hash" >"$evidence/claude-settings-sha256.txt"
cp "$work"/{version.txt,plugin-inventory-sha256.txt,plugin-descriptor-sha256.txt,reviewed-plugin-source.txt,installed-plugin-source.txt,reviewed-plugin-build-inputs.txt,installed-plugin-build-inputs.txt,plugin-install-first.json,plugin-install-repeat.json,plugin-status.json,plugin-uninstall.json,agent-setup-first.json,agent-setup-repeat.json,agent-context.json,native-build.json,test-list.json,test-run.json,native-compile-proof.txt,package-compile-proof.txt,seed-verification.log,source-before.txt,source-after.txt,source.diff,aggregate-content.txt,expected-package-files.txt,expected-registry-objects.txt,forbidden-reference-status.txt,cook.json,package.json,registry-dump.log,registry-dump-utf8.txt,registry-dump-normalized-utf8.txt,p1-operation-traceability.tsv,p1-operation-traceability.json} "$evidence/"
cp "$work"/operation-*.json "$evidence/"
ditto "$work/report" "$evidence/report"
ditto "$work/package-inventory" "$evidence/package-inventory"
ditto "$work/registry-dump" "$evidence/registry-dump"
for n in 2 3 4 5; do ditto "$work/loop-$n" "$evidence/loop-$n"; cp "$work/loop-$n.log" "$evidence/"; done
if find "$evidence" -type f \( -name token -o -name bridge-v1.json \) -print -quit | grep -q .; then fail "runtime material retained in evidence"; fi
ruby -I "$repo_root/tests/unreal/support" -e 'require "find"; require "p16-revalidate"; root=ARGV.fetch(0); abort "credential retained in combined evidence" if Find.find(root).select { |path| P16Revalidate.regular(path) }.any? { |path| P16Revalidate.retained_credential?(path) }' "$evidence" || fail "combined evidence credential scan"
for n in 2 3 4 5; do grep -Fxq 'tokenScan=passed' "$evidence/loop-$n/summary.txt"; done
identity_file="$work/provenance-identities.json"
jq -n --arg artifact "$(printf '%s' "$P16_EXPECTED_ARTIFACT_SHA256" | tr '[:upper:]' '[:lower:]')" --arg binary "$cli_hash" --arg plugin "$plugin_hash" --arg catalog "$catalog_hash" --arg manifest "$(shasum -a 256 "$manifest" | cut -d' ' -f1)" '{artifactSha256:$artifact,binarySha256:$binary,pluginSha256:$plugin,catalogSha256:$catalog,manifestSha256:$manifest}' >"$identity_file"
ruby "$repo_root/tests/unreal/support/p16-provenance.rb" write "$repo_root" "$evidence/source-inventory.tsv" "$evidence/provenance.json" "$identity_file" >"$work/provenance.sha256"
cp "$identity_file" "$evidence/provenance-identities.json"
ruby "$repo_root/tests/unreal/support/p16-provenance.rb" verify "$repo_root" "$evidence/source-inventory.tsv" "$evidence/provenance.json" "$evidence/provenance-identities.json" >/dev/null || fail "combined provenance verification"
provenance_sha256=$(cat "$work/provenance.sha256")
expected_source_tree=$(git -C "$repo_root" rev-parse HEAD^{tree})
printf 'phase=P1.6\nstatus=active\ncertificationComplete=false\nartifactSha256=%s\ncliSha256=%s\npluginSha256=%s\ncatalog=79/%s\nautomation=38/38-passed\nwarningBaseline=10-exact\np1Operations=45/45-traceable\nagentSetup=first-change-repeat-no-op-context-passed\nloops=P1.2,P1.3,P1.4,P1.5-passed\ncombinedBlueprintOnlyPackage=passed-iostore-asset-registry-generated-classes-four-maps\nsourceInventory=before-after-identical\nsourceCommit=%s\nsourceTree=%s\nprovenanceSha256=%s\nretainedEvidence=token-scan-passed\nremaining=agentJobs,independent-review,docs\n' "$(printf '%s' "$P16_EXPECTED_ARTIFACT_SHA256" | tr '[:upper:]' '[:lower:]')" "$cli_hash" "$plugin_hash" "$catalog_hash" "$expected_source_commit" "$expected_source_tree" "$provenance_sha256" | tee "$evidence/summary.txt"
printf 'artifactSha256=%s\n' "$P16_EXPECTED_ARTIFACT_SHA256" >"$evidence/statement.txt"
tree_sha256=$(ruby "$repo_root/tests/unreal/support/p16-evidence.rb" write "$evidence" "$evidence/evidence-tree.json")
ruby "$repo_root/tests/unreal/support/p16-evidence.rb" verify "$evidence" "$evidence/evidence-tree.json" >/dev/null
latest_tmp=$(mktemp "$cache/latest.XXXXXX")
printf '%s\n' "$evidence" >"$latest_tmp"
mv -f "$latest_tmp" "$cache/latest"
if [[ -n ${P16_EVIDENCE_PATH_FILE:-} ]]; then
  ruby "$repo_root/tests/unreal/support/p16-evidence-path.rb" write "$evidence" "$P16_EVIDENCE_PATH_FILE" || fail "exact evidence path output"
fi
echo "P1.6 combined core: PASS (active; agentJobs/review/docs remain; evidence retained at $evidence)"
