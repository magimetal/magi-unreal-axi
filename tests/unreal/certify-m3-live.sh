#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
engine_root=$(cd "$engine_root" && pwd -P)
editor_app="$engine_root/Engine/Binaries/Mac/UnrealEditor.app"
cache_root="$HOME/Library/Caches/magi-unreal-axi/m3/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
evidence=$(mktemp -d "$cache_root/evidence.XXXXXX")
normal_pid=
dirty_pid=
stop_process() {
  local pid=$1
  [[ -n "$pid" ]] || return 0
  kill -0 "$pid" 2>/dev/null || return 0
  /bin/kill -TERM "$pid" 2>/dev/null || true
  for _ in $(seq 1 50); do kill -0 "$pid" 2>/dev/null || return 0; sleep 0.1; done
  /bin/kill -KILL "$pid" 2>/dev/null || true
}
cleanup() {
  stop_process "$normal_pid"
  stop_process "$dirty_pid"
  /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

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

"$bin" --project "$project" --engine "$engine_root" setup plugin install --format json >"$work/setup.json" 2>"$work/setup.stderr"
"$bin" --project "$project" --engine "$engine_root" --timeout 120 editor start --format json >"$work/start.json" 2>"$work/start.stderr"
normal_pid=$(plutil -extract editor.pid raw -o - "$work/start.json")
normal_session="$project_runtime/$normal_pid"
normal_token_pattern="$work/normal-token.pattern"
cp "$normal_session/token" "$normal_token_pattern"
chmod 0600 "$normal_token_pattern"
"$bin" --project "$project" --editor "$normal_pid" --timeout 30 editor status --format json >"$work/status.json" 2>"$work/status.stderr"
[[ "$(stat -f %Lp "$HOME/Library/Caches/magi-unreal-axi")" == 700 ]]
[[ "$(stat -f %Lp "$project_runtime")" == 700 ]]
[[ "$(stat -f %Lp "$normal_session")" == 700 ]]
for private_file in bridge-v1.json token owner-v1.json; do [[ "$(stat -f %Lp "$normal_session/$private_file")" == 600 ]]; done
"$bin" --project "$project" --editor "$normal_pid" --timeout 30 editor stop --format json >"$work/stop.json" 2>"$work/stop.stderr"
normal_log=$(plutil -extract editor.logPath raw -o - "$work/start.json")
! grep -Fq -f "$normal_token_pattern" "$work/setup.json" "$work/start.json" "$work/status.json" "$work/stop.json" "$work"/*.stderr "$normal_log"
! grep -F '"token"' "$work/setup.json" "$work/start.json" "$work/status.json" "$work/stop.json" "$work"/*.stderr "$normal_log"
[[ ! -s "$work/setup.stderr" && ! -s "$work/start.stderr" && ! -s "$work/status.stderr" && ! -s "$work/stop.stderr" ]]
! grep -Eqi 'assertion failed|fatal error|crash' "$normal_log"
[[ "$(plutil -extract editor.state raw -o - "$work/start.json")" == ready ]]
[[ "$(plutil -extract editor.state raw -o - "$work/status.json")" == ready ]]
[[ "$(plutil -extract editor.state raw -o - "$work/stop.json")" == stopped ]]
[[ -n "$(plutil -extract result.operationId raw -o - "$work/stop.json")" ]]
for _ in $(seq 1 150); do kill -0 "$normal_pid" 2>/dev/null || break; sleep 0.1; done
kill -0 "$normal_pid" 2>/dev/null && { echo "normal editor did not exit" >&2; exit 1; }
[[ ! -e "$normal_session/bridge-v1.json" && ! -e "$normal_session/token" ]]
normal_pid=

dirty_report="$work/dirty-report"
mkdir -p "$dirty_report"
"$editor_app/Contents/MacOS/UnrealEditor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound \
  '-ExecCmds=Automation RunTests MagiUnrealAXI.LiveFixture.DirtyPackage' \
  "-ReportOutputPath=$dirty_report" >"$work/dirty-editor.log" 2>&1 &
dirty_pid=$!
record="$project_runtime/$dirty_pid/bridge-v1.json"
for _ in $(seq 1 1200); do
  [[ -f "$record" ]] && break
  kill -0 "$dirty_pid" 2>/dev/null || { echo "dirty fixture editor exited before discovery" >&2; exit 1; }
  sleep 0.1
done
[[ -f "$record" ]] || { echo "dirty fixture discovery missing" >&2; exit 1; }
for _ in $(seq 1 100); do
  [[ -f "$dirty_report/index.json" ]] && [[ "$(plutil -extract succeeded raw -o - "$dirty_report/index.json")" == 1 ]] && break
  sleep 0.1
done
[[ -f "$dirty_report/index.json" ]] || { echo "dirty fixture automation report missing" >&2; exit 1; }
[[ "$(plutil -extract succeeded raw -o - "$dirty_report/index.json")" == 1 ]] || { echo "dirty fixture automation failed" >&2; exit 1; }
for _ in $(seq 1 100); do
  if "$bin" --project "$project" --editor "$dirty_pid" --timeout 2 editor status --format json >"$work/dirty-status.json" 2>"$work/dirty-status.stderr" \
    && [[ "$(plutil -extract editor.state raw -o - "$work/dirty-status.json")" == ready ]]; then break; fi
  sleep 0.1
done
[[ "$(plutil -extract editor.state raw -o - "$work/dirty-status.json")" == ready ]]
session=$(dirname "$record")
project_id=$(plutil -extract projectId raw -o - "$record")
process_start=$(plutil -extract processStart raw -o - "$record")
nonce=$(plutil -extract sessionNonce raw -o - "$record")
umask 077
printf '{"pid":%s,"processStart":"%s","projectId":"%s","sessionNonce":"%s","executable":"%s"}\n' \
  "$dirty_pid" "$process_start" "$project_id" "$nonce" "$bin" >"$session/owner-v1.json"
chmod 0600 "$session/owner-v1.json"
dirty_token_pattern="$work/dirty-token.pattern"
cp "$session/token" "$dirty_token_pattern"
chmod 0600 "$dirty_token_pattern"
[[ "$(stat -f %Lp "$session")" == 700 ]]
for private_file in bridge-v1.json token owner-v1.json; do [[ "$(stat -f %Lp "$session/$private_file")" == 600 ]]; done
sleep 0.25
set +e
"$bin" --project "$project" --editor "$dirty_pid" --timeout 5 editor stop --format json >"$work/dirty-refusal.json" 2>"$work/dirty-refusal.stderr"
refusal_status=$?
set -e
[[ $refusal_status -eq 1 ]] || { echo "dirty stop did not refuse" >&2; cat "$work/dirty-refusal.json" >&2; exit 1; }
[[ "$(plutil -extract error.reason raw -o - "$work/dirty-refusal.json")" == unsafe_editor_state ]] || { echo "dirty stop reason mismatch" >&2; exit 1; }
[[ "$(plutil -extract error.dirtyPackageCount raw -o - "$work/dirty-refusal.json")" -ge 1 ]] || { echo "dirty package count missing" >&2; exit 1; }
[[ "$(plutil -extract error.dirtyPackages.0 raw -o - "$work/dirty-refusal.json")" == /Game/MagiUnrealAXIDirtyFixture ]] || { echo "dirty package name missing" >&2; exit 1; }
kill -0 "$dirty_pid" || { echo "editor exited after dirty refusal" >&2; exit 1; }

sleep 21
"$bin" --project "$project" --editor "$dirty_pid" --timeout 30 editor stop --format json >"$work/dirty-clean-stop.json" 2>"$work/dirty-clean-stop.stderr"
[[ "$(plutil -extract editor.state raw -o - "$work/dirty-clean-stop.json")" == stopped ]]
! grep -Fq -f "$dirty_token_pattern" "$work/dirty-status.json" "$work/dirty-refusal.json" "$work/dirty-clean-stop.json" "$work"/dirty-*.stderr "$work/dirty-editor.log"
! grep -F '"token"' "$work/dirty-status.json" "$work/dirty-refusal.json" "$work/dirty-clean-stop.json" "$work"/dirty-*.stderr "$work/dirty-editor.log"
[[ ! -s "$work/dirty-refusal.stderr" && ! -s "$work/dirty-clean-stop.stderr" ]]
! grep -Eqi 'assertion failed|fatal error|crash' "$work/dirty-editor.log"
for _ in $(seq 1 150); do kill -0 "$dirty_pid" 2>/dev/null || break; sleep 0.1; done
kill -0 "$dirty_pid" 2>/dev/null && { echo "dirty fixture editor did not exit" >&2; exit 1; }
dirty_pid=
[[ ! -e "$session/bridge-v1.json" && ! -e "$session/token" ]]

cp "$work"/*.json "$evidence/"
{
  echo "target=UE 5.8.1 changelist 56057345 host=$(uname -m)"
  echo "normal=start-ready status-ready stop-stopped process-exited"
  echo "dirty=refused reason=unsafe_editor_state count=$(plutil -extract error.dirtyPackageCount raw -o - "$work/dirty-refusal.json") package=$(plutil -extract error.dirtyPackages.0 raw -o - "$work/dirty-refusal.json")"
  echo "dirty_cleanup=stop-stopped process-exited"
} >"$evidence/summary.txt"
cat "$evidence/summary.txt"
echo "M3 live lifecycle: PASS (evidence retained at $evidence)"
