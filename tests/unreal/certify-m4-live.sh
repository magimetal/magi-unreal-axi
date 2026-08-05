#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
engine_root=$(cd "$engine_root" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
cache_root="$HOME/Library/Caches/magi-unreal-axi/m4/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
editor_pid=
stop_process() {
  [[ -n "${1:-}" ]] || return 0
  kill -0 "$1" 2>/dev/null || return 0
  /bin/kill -TERM "$1" 2>/dev/null || true
  for _ in $(seq 1 50); do kill -0 "$1" 2>/dev/null || return 0; sleep 0.1; done
  /bin/kill -KILL "$1" 2>/dev/null || true
}
cleanup() {
  stop_process "$editor_pid"
  [[ ${KEEP_M4_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check
cargo build --release --locked --manifest-path "$repo_root/Cargo.toml"
mkdir -p "$work/project"
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$work/project"
cp "$repo_root/target/release/magi-unreal-axi" "$work/magi-unreal-axi"
chmod 0755 "$work/magi-unreal-axi"
bin="$work/magi-unreal-axi"
project="$work/project/MagiUnrealAXIFixture.uproject"
canonical_project="$(cd "$(dirname "$project")" && pwd -P)/$(basename "$project")"
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
project_runtime="$HOME/Library/Caches/magi-unreal-axi/$project_hash"
catalog_hash=$(shasum -a 256 "$repo_root/capabilities/catalog.json" | cut -d' ' -f1)

"$bin" --project "$project" --engine "$engine_root" setup plugin install --format json >"$work/setup.json" 2>"$work/setup.stderr"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.log" 2>&1
report="$work/read-report"
mkdir -p "$report"
"$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound \
  '-ExecCmds=Automation RunTests MagiUnrealAXI.LiveFixture.ReadData' \
  "-ReportOutputPath=$report" >"$work/editor.log" 2>&1 &
editor_pid=$!
record="$project_runtime/$editor_pid/bridge-v1.json"
for _ in $(seq 1 1200); do
  [[ -f "$record" ]] && break
  kill -0 "$editor_pid" 2>/dev/null || { echo "read fixture editor exited before discovery" >&2; exit 1; }
  sleep 0.1
done
[[ -f "$record" ]] || { echo "read fixture discovery missing" >&2; exit 1; }
for _ in $(seq 1 200); do
  [[ -f "$report/index.json" ]] && [[ "$(plutil -extract succeeded raw -o - "$report/index.json")" == 1 ]] && break
  sleep 0.1
done
[[ -f "$report/index.json" && "$(plutil -extract succeeded raw -o - "$report/index.json")" == 1 ]]

session=$(dirname "$record")
project_id=$(plutil -extract projectId raw -o - "$record")
process_start=$(plutil -extract processStart raw -o - "$record")
nonce=$(plutil -extract sessionNonce raw -o - "$record")
umask 077
printf '{"pid":%s,"processStart":"%s","projectId":"%s","sessionNonce":"%s","executable":"%s"}\n' \
  "$editor_pid" "$process_start" "$project_id" "$nonce" "$bin" >"$session/owner-v1.json"
chmod 0600 "$session/owner-v1.json"
token_pattern="$work/token.pattern"
cp "$session/token" "$token_pattern"
chmod 0600 "$token_pattern"

for _ in $(seq 1 100); do
  if "$bin" --project "$project" --editor "$editor_pid" --timeout 2 editor status --format json >"$work/editor-status.json" 2>"$work/editor-status.stderr" \
    && [[ "$(plutil -extract editor.state raw -o - "$work/editor-status.json")" == ready ]]; then break; fi
  sleep 0.1
done
[[ "$(plutil -extract editor.state raw -o - "$work/editor-status.json")" == ready ]]

inventory() {
  find "$work/project" -type f \
    ! -path '*/Saved/*' ! -path '*/Intermediate/*' ! -path '*/DerivedDataCache/*' \
    ! -path '*/Binaries/*' -print0 | sort -z | xargs -0 shasum -a 256
}
inventory >"$work/inventory-before.txt"

"$bin" --project "$project" --editor "$editor_pid" level current --format json >"$work/level-current-1.json" 2>"$work/level-current-1.stderr"
"$bin" --project "$project" --editor "$editor_pid" level current --format json >"$work/level-current-2.json" 2>"$work/level-current-2.stderr"
"$bin" --project "$project" --editor "$editor_pid" level list --limit 1 --format json >"$work/level-list.json" 2>"$work/level-list.stderr"
"$bin" --project "$project" --editor "$editor_pid" actor list --limit 100 --fields id,label,class,levelId --format json >"$work/actor-list.json" 2>"$work/actor-list.stderr"
"$bin" --project "$project" --editor "$editor_pid" asset list --limit 1 --fields id,name,class,packagePath --format json >"$work/asset-list.json" 2>"$work/asset-list.stderr"
"$bin" capability search actor --format json >"$work/capability-search.json" 2>"$work/capability-search.stderr"
"$bin" capability describe actor.list --format json >"$work/capability-describe.json" 2>"$work/capability-describe.stderr"
"$bin" --project "$project" --editor "$editor_pid" editor describe --format json >"$work/bridge-describe.json" 2>"$work/bridge-describe.stderr"

actor_total=$(plutil -extract total raw -o - "$work/actor-list.json")
actor_count=$(plutil -extract count raw -o - "$work/actor-list.json")
[[ "$actor_total" -ge 1 && "$actor_count" -ge 1 ]]
actor_id=$(plutil -extract items.0.id raw -o - "$work/actor-list.json")
if [[ "$actor_total" -gt "$actor_count" ]]; then
  cursor=$(plutil -extract nextCursor raw -o - "$work/actor-list.json")
  "$bin" --project "$project" --editor "$editor_pid" actor list --limit 100 --cursor "$cursor" --fields id,label,class,levelId --format json >"$work/actor-list-page-2.json" 2>"$work/actor-list-page-2.stderr"
  [[ "$(plutil -extract revision raw -o - "$work/actor-list.json")" == "$(plutil -extract revision raw -o - "$work/actor-list-page-2.json")" ]]
  [[ "$(plutil -extract count raw -o - "$work/actor-list-page-2.json")" -ge 1 ]]
fi
asset_id=/Game/MagiUnrealAXIReadFixture.MagiUnrealAXIReadFixture
[[ "$(plutil -extract total raw -o - "$work/asset-list.json")" -ge 1 ]]
[[ "$(plutil -extract items.0.id raw -o - "$work/asset-list.json")" == "$asset_id" ]]

"$bin" --project "$project" --editor "$editor_pid" actor view "$actor_id" --format json >"$work/actor-view-1.json" 2>"$work/actor-view-1.stderr"
"$bin" --project "$project" --editor "$editor_pid" actor view "$actor_id" --format json >"$work/actor-view-2.json" 2>"$work/actor-view-2.stderr"
"$bin" --project "$project" --editor "$editor_pid" asset view "$asset_id" --format json >"$work/asset-view-1.json" 2>"$work/asset-view-1.stderr"
"$bin" --project "$project" --editor "$editor_pid" asset view "$asset_id" --format json >"$work/asset-view-2.json" 2>"$work/asset-view-2.stderr"
"$bin" --project "$project" --editor "$editor_pid" capability execute actor.view --input-json "{\"id\":\"$actor_id\"}" --format json >"$work/actor-generic.json" 2>"$work/actor-generic.stderr"

[[ "$(plutil -extract level.id raw -o - "$work/level-current-1.json")" == "$(plutil -extract level.id raw -o - "$work/level-current-2.json")" ]]
[[ "$(plutil -extract level.revision raw -o - "$work/level-current-1.json")" == "$(plutil -extract level.revision raw -o - "$work/level-current-2.json")" ]]
for field in id actorGuid levelId objectPath revision; do
  [[ "$(plutil -extract "$field" raw -o - "$work/actor-view-1.json")" == "$(plutil -extract "$field" raw -o - "$work/actor-view-2.json")" ]]
done
for field in id packagePath objectPath revision; do
  [[ "$(plutil -extract "$field" raw -o - "$work/asset-view-1.json")" == "$(plutil -extract "$field" raw -o - "$work/asset-view-2.json")" ]]
done
[[ "$(plutil -extract id raw -o - "$work/actor-generic.json")" == "$actor_id" ]]
[[ "$(plutil -extract runtime.catalogHash raw -o - "$work/capability-describe.json")" == "$catalog_hash" ]]
[[ "$(plutil -extract catalogHash raw -o - "$work/bridge-describe.json")" == "$catalog_hash" ]]
[[ "$(plutil -extract count raw -o - "$work/capability-search.json")" -ge 2 ]]
for list in level-list actor-list asset-list; do
  count=$(plutil -extract count raw -o - "$work/$list.json")
  total=$(plutil -extract total raw -o - "$work/$list.json")
  [[ "$count" -le "$total" ]]
  plutil -extract scope raw -o - "$work/$list.json" >/dev/null
  plutil -extract items json -o - "$work/$list.json" >/dev/null
done

inventory >"$work/inventory-after.txt"
cmp "$work/inventory-before.txt" "$work/inventory-after.txt"
for stderr in "$work"/*.stderr; do [[ ! -s "$stderr" ]]; done
! grep -Fq -f "$token_pattern" "$work"/*.json "$work"/*.stderr "$work/editor.log"
! grep -F '"token"' "$work"/*.json "$work"/*.stderr "$work/editor.log"
! grep -Eqi 'assertion failed|fatal error|crash' "$work/editor.log"

sleep 21
set +e
"$bin" --project "$project" --editor "$editor_pid" --timeout 30 editor stop --format json >"$work/stop.json" 2>"$work/stop.stderr"
stop_status=$?
set -e
if [[ $stop_status -ne 0 ]]; then cat "$work/stop.json" >&2; cat "$work/stop.stderr" >&2; exit "$stop_status"; fi
[[ "$(plutil -extract editor.state raw -o - "$work/stop.json")" == stopped ]]
for _ in $(seq 1 150); do kill -0 "$editor_pid" 2>/dev/null || break; sleep 0.1; done
kill -0 "$editor_pid" 2>/dev/null && { echo "M4 editor did not exit" >&2; exit 1; }
editor_pid=
[[ ! -e "$session/bridge-v1.json" && ! -e "$session/token" ]]

cp "$work"/*.json "$work"/inventory-*.txt "$evidence/"
{
  echo "target=UE 5.8.1 changelist 56057345 host=$(uname -m)"
  echo "catalogHash=$catalog_hash"
  echo "commands=editor.status level.current level.list actor.list actor.view asset.list asset.view capability.search capability.describe capability.execute"
  echo "actorId=$actor_id"
  echo "assetId=$asset_id"
  echo "readOnlyInventory=unchanged"
  echo "lifecycle=stopped process-exited discovery-cleaned"
} >"$evidence/summary.txt"
cat "$evidence/summary.txt"
echo "M4 live reads: PASS (evidence retained at $evidence)"
