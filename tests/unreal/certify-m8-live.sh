#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
version=$(awk -F'"' '/^version = "/ { print $2; exit }' "$repo_root/Cargo.toml")
manifest="$repo_root/tests/unreal/p1.0-manifest.json"
[[ -f "$manifest" ]] || { echo "P1.0 manifest missing" >&2; exit 1; }
[[ -n $version ]] || { echo "Cargo package version missing" >&2; exit 1; }
[[ $# == 1 && -f $1 ]] || { echo "usage: $0 ARTIFACT.tar.gz" >&2; exit 2; }
artifact=$(cd "$(dirname "$1")" && pwd -P)/$(basename "$1")
artifact_dir=$(dirname "$artifact")
artifact_name=$(basename "$artifact")
[[ $artifact_name == "magi-unreal-axi-${version}-macos-arm64.tar.gz" ]] || { echo "unexpected release artifact name" >&2; exit 1; }
checksums="$artifact_dir/SHA256SUMS"
[[ -f $checksums ]] || { echo "SHA256SUMS missing beside artifact" >&2; exit 1; }
[[ $(awk -v name="$artifact_name" '$2 == name && $1 ~ /^[0-9a-fA-F]{64}$/ && NF == 2 { count++ } END { print count + 0 }' "$checksums") == 1 ]] || { echo "exact artifact checksum entry missing or duplicated" >&2; exit 1; }
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor_version="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ $(uname -m) == arm64 && $(plutil -extract Changelist raw -o - "$editor_version") == 56057345 ]]

cache_root="$HOME/Library/Caches/magi-unreal-axi/m8/live"
package_root="magi-unreal-axi-${version}-macos-arm64"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
artifact_hash=$(shasum -a 256 "$artifact" | cut -d' ' -f1)
(cd "$artifact_dir" && shasum -a 256 -c SHA256SUMS) | tee "$evidence/checksum.txt" >/dev/null
real_home=$HOME
home="$work/home"
project_dir="$work/project"
project="$project_dir/MagiUnrealAXIFixture.uproject"
pid=
token=

cleanup() {
  local status=$?
  trap - EXIT
  if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then kill -TERM "$pid" 2>/dev/null || true; fi
  if [[ $status != 0 ]]; then
    echo "M8 live certification failed; work retained at $work; evidence at $evidence" >&2
    exit "$status"
  fi
  [[ ${KEEP_M8_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

mkdir -p "$home" "$project_dir"
tar_paths=$(tar -tzf "$artifact")
while IFS= read -r path; do
  [[ -n "$path" && "$path" != /* && "$path" != ./* && "$path" != ../* && "$path" != *'/../'* && "$path" != *'/..' ]] || { echo "unsafe archive path: $path" >&2; exit 1; }
  case "$path" in
    "$package_root/"|"$package_root/magi-unreal-axi"|"$package_root/Cargo.lock"|"$package_root/README.md"|"$package_root/LICENSE"|"$package_root/THIRD_PARTY_NOTICES.md"|"$package_root/CHANGELOG.md"|"$package_root/skills/"|"$package_root/skills/magi-unreal-axi/"|"$package_root/skills/magi-unreal-axi/SKILL.md"|"$package_root/docs/"|"$package_root/docs/engine-support.md"|"$package_root/docs/agent-evaluation.md") ;;
    *) echo "unexpected archive layout: $path" >&2; exit 1 ;;
  esac
done <<< "$tar_paths"
printf '%s\n' "$tar_paths" >"$evidence/archive-list.txt"
[[ $(printf '%s\n' "$tar_paths" | grep -Fxc "$package_root/magi-unreal-axi") == 1 ]] || { echo "binary missing or duplicated" >&2; exit 1; }
while IFS= read -r line; do
  case ${line:0:1} in l|h) echo "archive contains link: $line" >&2; exit 1;; esac
done < <(tar -tvzf "$artifact")
tar -xzf "$artifact" -C "$work"
bin="$work/$package_root/magi-unreal-axi"
[[ -f "$bin" && -x "$bin" && ! -L "$bin" ]]
file "$bin" | grep -Eiq 'arm64|aarch64' || { echo "release binary is not arm64" >&2; exit 1; }
[[ $("$bin" --version) == "magi-unreal-axi $version" ]] || { echo "release binary version mismatch" >&2; exit 1; }
codesign --verify --strict --verbose=2 "$bin" >"$evidence/codesign.txt" 2>&1
codesign -dv --verbose=4 "$bin" >>"$evidence/codesign.txt" 2>&1
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"

export HOME="$home"
export XDG_CONFIG_HOME="$home/.config"
mkdir -p "$work/bin"
install -m 755 "$bin" "$work/bin/magi-unreal-axi"
export PATH="$work/bin:/usr/bin:/bin"
bin=$(command -v magi-unreal-axi)
[[ "$bin" == "$work/bin/magi-unreal-axi" ]]
"$bin" --version >"$evidence/version.txt"
(cd "$project_dir" && magi-unreal-axi --format json) >"$evidence/home.json"
jq -e '.project.found == true and .project.name == "MagiUnrealAXIFixture"' "$evidence/home.json" >/dev/null

# Agent integrations use only isolated HOME. Repeated setup must be byte-identical no-op.
"$bin" setup agents --format json >"$evidence/agents-first.json"
settings="$home/.claude/settings.json"
settings_hash=$(shasum -a 256 "$settings" | cut -d' ' -f1)
"$bin" setup agents --format json >"$evidence/agents-repeat.json"
[[ $(jq -r .changed "$evidence/agents-first.json") == true ]]
[[ $(jq -r .changed "$evidence/agents-repeat.json") == false ]]
[[ $(shasum -a 256 "$settings" | cut -d' ' -f1) == "$settings_hash" ]]
for skill in "$home/.claude/skills" "$home/.agents/skills" "$home/.config/opencode/skills"; do
  [[ -s "$skill/magi-unreal-axi/SKILL.md" ]]
done
hook=$(jq -r '.hooks.SessionStart[] | .hooks[] | select(.command | endswith(" agent context --format json")) | .command' "$settings")
(cd "$project_dir" && /bin/sh -c "$hook") >"$evidence/agent-context.json"
[[ $(wc -c <"$evidence/agent-context.json" | tr -d ' ') -lt 400 ]]
jq -e '.magiUnrealAxi.project.name == "MagiUnrealAXIFixture"' "$evidence/agent-context.json" >/dev/null

# Unreal resolves user settings through macOS account APIs, while CLI runtime paths use HOME.
# Restore matching account HOME only after proving agent config isolation.
export HOME="$real_home"
unset XDG_CONFIG_HOME

axi() { "$bin" --project "$project" --engine "$engine_root" --format json "$@"; }
mutation_when_safe() {
  local output=$1
  shift
  local status reason
  for _ in $(seq 1 600); do
    set +e
    axi "$@" >"$output"
    status=$?
    set -e
    [[ $status == 0 ]] && return 0
    reason=$(jq -r '.error.reason // empty' "$output" 2>/dev/null || true)
    [[ $reason == unsafe_editor_state ]] || return "$status"
    sleep .2
  done
  return 1
}
axi setup plugin install >"$evidence/plugin-install.json"
jq -e '.plugin.installed == true and .plugin.managed == true and .plugin.compatible == true' "$evidence/plugin-install.json" >/dev/null
axi project doctor >"$evidence/doctor.json"
jq -e '.healthy == true' "$evidence/doctor.json" >/dev/null
axi --timeout 1800 project build >"$evidence/build.json"
jq -e '.operation.status == "passed" or .operation.status == "up_to_date"' "$evidence/build.json" >/dev/null
mkdir -p "$work/report"
axi --timeout 1800 project test run --filter MagiUnrealAXI --report "$work/report" >"$evidence/test-run.json"
index="$work/report/index.json"
[[ -f "$index" ]] || { echo "automation report missing" >&2; exit 1; }
jq -e --slurpfile manifest "$manifest" '.failed == 0 and .notRun == 0 and .inProcess == 0 and (.succeeded + .succeededWithWarnings) == ($manifest[0].automationTests | length) and (all(.tests[]; .state == "Success" and .errors == 0)) and ([.tests[].fullTestPath] | sort) == ($manifest[0].automationTests | sort)' "$index" >/dev/null
cp "$index" "$evidence/automation-index.json"
cp "$manifest" "$evidence/p1.0-manifest.json"

# Release binary owns editor lifecycle, read, mutation, explicit save, and restart persistence.
axi --timeout 120 editor start >"$evidence/editor-start.json"
pid=$(jq -r '.editor.pid // .editorPid // empty' "$evidence/editor-start.json")
axi editor status >"$evidence/editor-status.json"
[[ -n "$pid" ]] || pid=$(jq -r '.editor.pid // .editor.editorPid // .editorPid // empty' "$evidence/editor-status.json")
jq -e '.editor.state == "ready"' "$evidence/editor-status.json" >/dev/null
level=/Game/MagiM8CleanInstall
mutation_when_safe "$evidence/level-create.json" level create --path "$level"
mutation_when_safe "$evidence/actor-spawn.json" actor spawn --level "$level" --class /Script/Engine.StaticMeshActor --agent-key m8-clean-install --label M8CleanInstall --location 10,20,30
actor=$(jq -r .result.id "$evidence/actor-spawn.json")
axi actor view "$actor" >"$evidence/actor-view.json"
jq -e --arg actor "$actor" '.id == $actor and .location == [10,20,30]' "$evidence/actor-view.json" >/dev/null
mutation_when_safe "$evidence/level-save.json" level save --path "$level"
jq -e '.receipt.persistence == "saved"' "$evidence/level-save.json" >/dev/null
runtime_token=$(find "$real_home/Library/Caches/magi-unreal-axi" -path "*/$pid/token" -type f -print -quit)
[[ -n "$runtime_token" ]] || { echo "bridge token unavailable for leak scan" >&2; exit 1; }
token=$(cat "$runtime_token")
axi editor stop >"$evidence/editor-stop.json"
pid=

axi --timeout 120 editor start >"$evidence/editor-restart.json"
pid=$(jq -r '.editor.pid // .editorPid // empty' "$evidence/editor-restart.json")
mutation_when_safe "$evidence/level-open-restart.json" level open --path "$level"
axi actor view "$actor" >"$evidence/actor-view-restart.json"
jq -e --arg actor "$actor" '.id == $actor and .location == [10,20,30]' "$evidence/actor-view-restart.json" >/dev/null
axi editor stop >"$evidence/editor-stop-restart.json"
pid=

axi setup plugin uninstall --force >"$evidence/plugin-uninstall.json"
jq -e '.plugin.installed == false and .plugin.changed == true' "$evidence/plugin-uninstall.json" >/dev/null
[[ ! -e "$project_dir/Plugins/MagiUnrealAXI" ]]
jq -e '([.Plugins[]? | select(.Name == "MagiUnrealAXI" and .Enabled == true)] | length) == 1' "$project" >/dev/null
if grep -R -I -Fq -- "$token" "$evidence"; then
  echo "bridge token leaked into retained M8 evidence" >&2
  exit 1
fi

printf 'target=UE 5.8.1 changelist 56057345 host=%s\nartifact=%s\nartifactSha256=%s\nsourceRevision=%s\nworkflowRun=%s\nhome=passed\nagents=claude-hook-codex-skill-opencode-skill-idempotent-context-under-400-bytes-isolated-home\nplugin=installed-matching-managed-uninstalled\neditor=health-read-mutation-save-stop-restart-persistence\nbuild=passed\nautomation=exact-p1.0-manifest-inventory-passed\ntokenScan=passed\ncleanInstall=checksum-allowlisted-archive-path-install-disposable-project-agent-config-isolation\n' "$(uname -m)" "$artifact" "$artifact_hash" "${GITHUB_SHA:-unavailable-no-git-metadata}" "${GITHUB_RUN_ID:-local}" | tee "$evidence/summary.txt"
printf '%s\n' "$evidence" >"$cache_root/latest"
echo "M8 live certification: PASS (evidence retained at $evidence; agent config isolated from $real_home)"
