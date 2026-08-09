#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.3-manifest.json"
[[ -f "$manifest" ]]
property_root=$(jq -r .fixture.root.name "$manifest")
property_root_class=$(jq -r .fixture.root.class "$manifest")
text_name=$(jq -r .fixture.text.name "$manifest")
text_class=$(jq -r .fixture.text.class "$manifest")
ready_text=$(jq -r .fixture.text.ready "$manifest")
active_text=$(jq -r .fixture.text.active "$manifest")
level=$(jq -r .fixture.level "$manifest")
widget=$(jq -r .fixture.widgetPath "$manifest")
host=$(jq -r .fixture.hostPath "$manifest")
agent=$(jq -r .fixture.agentKey "$manifest")
input_key=$(jq -r .fixture.inputKey "$manifest")
z_order=$(jq -r .fixture.zOrder "$manifest")
content_root=$(jq -r .packageAssertions.contentRoot "$manifest")
[[ "$widget" == "/Game/$content_root/WBP_UIState" && "$host" == "/Game/$content_root/BP_UIStateHost" ]]
engine_root=$(cd "${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}" && pwd -P)
editor="$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
run_uat="$engine_root/Engine/Build/BatchFiles/RunUAT.sh"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -x "$editor" && -x "$run_uat" && -f "$version_file" ]]
jq -e '.phase == "P1.3" and .fixture.name == "ui-state-loop" and .fixture.agentKey == "p13.ui-state" and .fixture.root == {name:"StateRoot",class:"VerticalBox"} and .fixture.text == {name:"StateText",class:"TextBlock",ready:"READY",active:"ACTIVE"} and .fixture.inputKey == "E" and .fixture.zOrder == 0 and .plugin.requiredArchitectures == ["arm64","x86_64"]' "$manifest" >/dev/null
[[ $(uname -m) == "$(jq -r .engine.hostArchitecture "$manifest")" ]]
[[ $(plutil -extract Changelist raw -o - "$version_file") == "$(jq -r .engine.changelist "$manifest")" ]]
[[ "$(plutil -extract MajorVersion raw -o - "$version_file").$(plutil -extract MinorVersion raw -o - "$version_file").$(plutil -extract PatchVersion raw -o - "$version_file")" == "$(jq -r .engine.version "$manifest")" ]]

cache_root="$HOME/Library/Caches/magi-unreal-axi/p1.3/live"
mkdir -p "$cache_root"
work=$(mktemp -d "$cache_root/work.XXXXXX")
if [[ -n "${P13_LIVE_EVIDENCE_DIR:-}" ]]; then evidence=$P13_LIVE_EVIDENCE_DIR; [[ ! -e "$evidence" ]]; else evidence=$(mktemp -d "$cache_root/evidence.XXXXXX"); fi
mkdir -p "$evidence"
project="$work/project/MagiUnrealAXIFixture.uproject"
project_dir=$(dirname "$project")
bin=${P13_CLI_PATH:-$work/magi-unreal-axi}
plugin_dir=${P13_PLUGIN_DIR:-$work/plugin}
package_project_dir=${P13_PACKAGE_PROJECT_DIR:-}
pid=
session=
tokens=()
package_claim=not-requested

export DOTNET_ROOT="$engine_root/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
export PATH="$DOTNET_ROOT:$PATH"
editor_alive() { kill -0 "$1" 2>/dev/null && [[ $(ps -p "$1" -o stat= 2>/dev/null) != Z* ]]; }
axi() { "$bin" --project "$project" --engine "$engine_root" --timeout 180 --format json "$@"; }
retry() {
  local out=$1; shift
  for _ in $(seq 1 600); do
    if axi "$@" >"$out"; then return 0; fi
    [[ $(jq -r '.error.reason == "unsafe_editor_state" and .error.retryable == true' "$out" 2>/dev/null) == true ]] || return 1
    sleep .2
  done
  return 1
}
receipt() { jq -e '.receipt.state == "completed" and .receipt.verification.matched == true' "$1" >/dev/null; }
view_receipt() { local id view; id=$(jq -r .receipt.operationId "$1"); view="$work/operation-$id.json"; axi operation view "$id" >"$view"; jq -e --slurpfile x "$1" '. == $x[0].receipt' "$view" >/dev/null; }
stop_editor() {
  [[ -n "$pid" ]] || return 0
  local p=$pid s=$session
  if editor_alive "$p"; then axi --editor "$p" editor stop >/dev/null 2>&1 || true; fi
  for _ in $(seq 1 200); do editor_alive "$p" || break; sleep .1; done
  if editor_alive "$p"; then kill -TERM "$p" 2>/dev/null || true; fi
  for _ in $(seq 1 100); do editor_alive "$p" || break; sleep .1; done
  if editor_alive "$p"; then kill -KILL "$p" 2>/dev/null || true; fi
  wait "$p" 2>/dev/null || true
  for _ in $(seq 1 100); do [[ ! -e "$s/bridge-v1.json" && ! -e "$s/token" ]] && break; sleep .1; done
  [[ ! -e "$s/bridge-v1.json" && ! -e "$s/token" ]]
  pid=; session=
}
start_editor() {
  local label=$1
  "$editor" "$project" -unattended -nop4 -nosplash -RenderOffscreen -ResX=640 -ResY=360 -NoSound "-log=$work/editor-$label-ue.log" >"$work/editor-$label.log" 2>&1 &
  pid=$!; session="$runtime_root/$pid"
  for _ in $(seq 1 1200); do [[ -f "$session/bridge-v1.json" ]] && break; editor_alive "$pid" || return 1; sleep .1; done
  [[ -f "$session/bridge-v1.json" && $(jq -r .projectPath "$session/bridge-v1.json") == "$canonical_project" ]]
  tokens+=("$(cat "$session/token")")
  for _ in $(seq 1 600); do axi --editor "$pid" editor status >"$work/status-$label.json" 2>/dev/null && [[ $(jq -r .editor.state "$work/status-$label.json") == ready ]] && return 0; sleep .1; done
  return 1
}
cleanup() {
  local status=$?; trap - EXIT; stop_editor || status=1
  if [[ $status != 0 ]]; then echo "P1.3 live certification failed; work retained at $work; evidence at $evidence" >&2; exit "$status"; fi
  [[ ${KEEP_P13_WORK:-0} == 1 ]] || /usr/bin/trash "$work" 2>/dev/null || true
}
trap cleanup EXIT

mkdir -p "$project_dir/Plugins"
catalog_line=$(cargo run --locked --manifest-path "$repo_root/Cargo.toml" --bin xtask -- capabilities check)
printf '%s\n' "$catalog_line" >"$work/catalog.txt"
catalog_count=$(sed -E 's/^capability catalog: ([0-9]+) records.*/\1/' <<<"$catalog_line")
catalog_hash=$(sed -E 's/.*sha256:([0-9a-f]{64})$/\1/' <<<"$catalog_line")
[[ "$catalog_count" == "$(jq -r .catalog.count "$manifest")" && "$catalog_hash" == "$(jq -r .catalog.sha256 "$manifest")" ]]
ditto "$repo_root/tests/unreal/MagiUnrealAXIFixture" "$project_dir"
if [[ -z "${P13_CLI_PATH:-}" ]]; then cargo build --release --locked --manifest-path "$repo_root/Cargo.toml" >"$work/rust-build.log" 2>&1; cp "$repo_root/target/release/magi-unreal-axi" "$bin"; fi
chmod 0755 "$bin"; [[ -x "$bin" ]]
cli_hash=$(shasum -a 256 "$bin" | cut -d' ' -f1); [[ -z "${P13_CLI_SHA256:-}" || "$cli_hash" == "$P13_CLI_SHA256" ]]
if [[ -z "${P13_PLUGIN_DIR:-}" ]]; then "$run_uat" BuildPlugin -Plugin="$repo_root/plugin/MagiUnrealAXI/MagiUnrealAXI.uplugin" -Package="$plugin_dir" -TargetPlatforms=Mac >"$work/plugin-build.log" 2>&1; fi
plugin_binary=$(find "$plugin_dir" -type f -name "$(jq -r .plugin.binary "$manifest")" -print -quit); [[ -n "$plugin_binary" ]]
artifact_hash=$(shasum -a 256 "$plugin_binary" | cut -d' ' -f1); [[ -z "${P13_PLUGIN_SHA256:-}" || "$artifact_hash" == "$P13_PLUGIN_SHA256" ]]
arches=$(lipo -archs "$plugin_binary"); for arch in $(jq -r '.plugin.requiredArchitectures[]' "$manifest"); do grep -qw "$arch" <<<"$arches"; done; [[ $(wc -w <<<"$arches" | tr -d ' ') == 2 ]]
mkdir -p "$project_dir/Plugins/MagiUnrealAXI"; ditto "$plugin_dir" "$project_dir/Plugins/MagiUnrealAXI"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" MagiUnrealAXIFixtureEditor Mac Development "$project" -WaitMutex >"$work/fixture-build.log" 2>&1
canonical_project=$(cd "$project_dir" && pwd -P)/$(basename "$project")
project_hash=$(printf '%s' "$canonical_project" | shasum -a 256 | cut -d' ' -f1)
runtime_root="$HOME/Library/Caches/magi-unreal-axi/$project_hash"
find "$runtime_root" -mindepth 1 -maxdepth 1 -type d -exec /usr/bin/trash {} + 2>/dev/null || true

start_editor author
retry "$work/level-create.json" level create --path "$level"; receipt "$work/level-create.json"; view_receipt "$work/level-create.json"
retry "$work/widget-create.json" capability execute widget.create --input-json "{\"path\":\"$widget\",\"rootName\":\"$property_root\",\"rootClass\":\"$property_root_class\"}"
receipt "$work/widget-create.json"; view_receipt "$work/widget-create.json"
widget=$(jq -r .result.blueprintId "$work/widget-create.json"); root=$(jq -r .result.rootWidgetId "$work/widget-create.json"); rev=$(jq -r .result.revision "$work/widget-create.json")
retry "$work/widget-child.json" capability execute widget.child_ensure --expected-revision "$rev" --input-json "{\"blueprintId\":\"$widget\",\"parentWidgetId\":\"$root\",\"name\":\"$text_name\",\"class\":\"$text_class\"}"
receipt "$work/widget-child.json"; view_receipt "$work/widget-child.json"; text=$(jq -r .result.widgetId "$work/widget-child.json"); rev=$(jq -r .result.revision "$work/widget-child.json")
retry "$work/widget-text.json" capability execute widget.property_set --expected-revision "$rev" --input-json "{\"blueprintId\":\"$widget\",\"widgetId\":\"$text\",\"property\":\"text\",\"text\":\"$ready_text\"}"; receipt "$work/widget-text.json"; view_receipt "$work/widget-text.json"; rev=$(jq -r .result.revision "$work/widget-text.json")
retry "$work/widget-event.json" capability execute widget.event_ensure --expected-revision "$rev" --input-json "{\"blueprintId\":\"$widget\",\"agentKey\":\"$agent\",\"event\":\"activate\",\"actions\":[{\"kind\":\"text.set\",\"targetWidgetId\":\"$text\",\"text\":\"$active_text\"}]}"; receipt "$work/widget-event.json"; view_receipt "$work/widget-event.json"; rev=$(jq -r .result.revision "$work/widget-event.json")
retry "$work/widget-compile.json" blueprint compile "$widget" --expected-revision "$rev"; receipt "$work/widget-compile.json"; view_receipt "$work/widget-compile.json"; rev=$(jq -r .result.revision "$work/widget-compile.json")
retry "$work/widget-save.json" asset save "$widget" --expected-revision "$rev"; receipt "$work/widget-save.json"; view_receipt "$work/widget-save.json"; saved_widget_rev=$(jq -r .result.revision "$work/widget-save.json")
retry "$work/host-create.json" capability execute blueprint.create --input-json "{\"path\":\"$host\",\"parentClass\":\"/Script/Engine.Actor\"}"; receipt "$work/host-create.json"; view_receipt "$work/host-create.json"; host=$(jq -r .result.blueprintId "$work/host-create.json"); host_class=$(jq -r .result.generatedClass "$work/host-create.json"); host_rev=$(jq -r .result.revision "$work/host-create.json")
retry "$work/viewport.json" capability execute widget.viewport_ensure --expected-revision "$host_rev" --input-json "{\"hostBlueprintId\":\"$host\",\"widgetBlueprintId\":\"$widget\",\"agentKey\":\"$agent\",\"inputKey\":\"$input_key\",\"zOrder\":$z_order}"; receipt "$work/viewport.json"; view_receipt "$work/viewport.json"; host_rev=$(jq -r .result.revision "$work/viewport.json")
retry "$work/host-compile.json" blueprint compile "$host" --expected-revision "$host_rev"; receipt "$work/host-compile.json"; view_receipt "$work/host-compile.json"; host_rev=$(jq -r .result.revision "$work/host-compile.json")
retry "$work/host-save.json" asset save "$host" --expected-revision "$host_rev"; receipt "$work/host-save.json"; view_receipt "$work/host-save.json"; saved_host_rev=$(jq -r .result.revision "$work/host-save.json")
retry "$work/spawn.json" actor spawn --level "$level" --class "$host_class" --agent-key p13-host --label 'P1.3 UI State Host' --location 0,0,0; receipt "$work/spawn.json"; view_receipt "$work/spawn.json"
retry "$work/level-save.json" level save --path "$level"; receipt "$work/level-save.json"; view_receipt "$work/level-save.json"

# Invalid intent must fail before mutation and preserve exact authored tree.
expect_widget_failure() {
  local label=$1 expected=$2 status; shift 2
  axi capability execute widget.tree_view --input-json "{\"blueprintId\":\"$widget\"}" >"$work/$label-before.json"
  set +e; axi "$@" >"$work/$label-error.json"; status=$?; set -e
  [[ $status != 0 ]] && jq -e --arg expected "$expected" '.error.reason == $expected' "$work/$label-error.json" >/dev/null
  axi capability execute widget.tree_view --input-json "{\"blueprintId\":\"$widget\"}" >"$work/$label-after.json"
  diff -u <(jq -S . "$work/$label-before.json") <(jq -S . "$work/$label-after.json") >/dev/null
}
expect_widget_failure missing-revision expected_revision_required capability execute widget.property_set --input-json "{\"blueprintId\":\"$widget\",\"widgetId\":\"$text\",\"property\":\"text\",\"text\":\"BROKEN\"}"
expect_widget_failure stale-revision conflict capability execute widget.property_set --expected-revision "$(printf '0%.0s' {1..64})" --input-json "{\"blueprintId\":\"$widget\",\"widgetId\":\"$text\",\"property\":\"text\",\"text\":\"BROKEN\"}"
expect_widget_failure invalid-parent invalid_capability_input capability execute widget.child_ensure --expected-revision "$saved_widget_rev" --input-json "{\"blueprintId\":\"$widget\",\"parentWidgetId\":\"$widget#widget:Missing\",\"name\":\"Nested\",\"class\":\"TextBlock\"}"
expect_widget_failure duplicate-conflict conflict capability execute widget.child_ensure --expected-revision "$saved_widget_rev" --input-json "{\"blueprintId\":\"$widget\",\"parentWidgetId\":\"$root\",\"name\":\"StateRoot\",\"class\":\"TextBlock\"}"
expect_widget_failure unsupported-class invalid_capability_input capability execute widget.child_ensure --expected-revision "$saved_widget_rev" --input-json "{\"blueprintId\":\"$widget\",\"parentWidgetId\":\"$root\",\"name\":\"Nested\",\"class\":\"VerticalBox\"}"
expect_widget_failure unsupported-property invalid_capability_input capability execute widget.property_set --expected-revision "$saved_widget_rev" --input-json "{\"blueprintId\":\"$widget\",\"widgetId\":\"$text\",\"property\":\"font\"}"
expect_widget_failure nested-child invalid_capability_input capability execute widget.child_ensure --expected-revision "$saved_widget_rev" --input-json "{\"blueprintId\":\"$widget\",\"parentWidgetId\":\"$text\",\"name\":\"Nested\",\"class\":\"TextBlock\"}"
expect_widget_failure event-conflict conflict capability execute widget.event_ensure --expected-revision "$saved_widget_rev" --input-json "{\"blueprintId\":\"$widget\",\"agentKey\":\"$agent\",\"event\":\"activate\",\"actions\":[{\"kind\":\"text.set\",\"targetWidgetId\":\"$text\",\"text\":\"DIFFERENT\"}]}"

stop_editor; start_editor restart
retry "$work/level-open.json" level open --path "$level"; receipt "$work/level-open.json"; view_receipt "$work/level-open.json"
restart_tree=$(axi capability execute widget.tree_view --input-json "{\"blueprintId\":\"$widget\"}")
jq -e --arg root "$root" --arg text "$text" --arg rev "$saved_widget_rev" --arg root_name "$property_root" --arg root_class "$property_root_class" --arg text_name "$text_name" --arg text_class "$text_class" --arg ready "$ready_text" --arg active "$active_text" --arg agent "$agent" '.revision == $rev and (.widgets | length) == 2 and .widgets[0].widgetId == $root and .widgets[0].name == $root_name and .widgets[0].class == $root_class and .widgets[0].text == null and .widgets[1].widgetId == $text and .widgets[1].name == $text_name and .widgets[1].class == $text_class and .widgets[1].text == $ready and (.events | length) == 1 and .events[0].agentKey == $agent and .events[0].actions[0].text == $active' <<<"$restart_tree" >/dev/null
retry "$work/event-noop.json" capability execute widget.event_ensure --expected-revision "$saved_widget_rev" --input-json "{\"blueprintId\":\"$widget\",\"agentKey\":\"$agent\",\"event\":\"activate\",\"actions\":[{\"kind\":\"text.set\",\"targetWidgetId\":\"$text\",\"text\":\"$active_text\"}]}"; jq -e --arg rev "$saved_widget_rev" '.result.changed == false and .result.revision == $rev' "$work/event-noop.json" >/dev/null; receipt "$work/event-noop.json"; view_receipt "$work/event-noop.json"
retry "$work/viewport-noop.json" capability execute widget.viewport_ensure --expected-revision "$saved_host_rev" --input-json "{\"hostBlueprintId\":\"$host\",\"widgetBlueprintId\":\"$widget\",\"agentKey\":\"$agent\",\"inputKey\":\"$input_key\",\"zOrder\":$z_order}"; jq -e --arg rev "$saved_host_rev" '.result.changed == false and .result.revision == $rev' "$work/viewport-noop.json" >/dev/null; receipt "$work/viewport-noop.json"; view_receipt "$work/viewport-noop.json"
axi blueprint view "$host" >"$work/host-view-restart.json"; jq -e --arg class "$host_class" '.generatedClass == $class and .status != "error" and .errorCount == 0' "$work/host-view-restart.json" >/dev/null

first_sid=; first_instance=; first_ready_widgets=; first_active_widgets=
observe_until() {
  local input=$1 state=$2 output=$3
  for _ in $(seq 1 200); do
    if axi capability execute play.ui_observe --input-json "$input" >"$output" 2>/dev/null && jq -e --arg state "$state" '.inViewport == true and .widgets[1].text == $state' "$output" >/dev/null; then return 0; fi
    sleep .05
  done
  return 1
}
capture_screenshot() {
  local label=$1 state=$2 input=$3 before=$4 state_lower shot_json after screenshot bmp pixel_offset width height expected_ihdr
  state_lower=$(printf '%s' "$state" | tr '[:upper:]' '[:lower:]'); shot_json="$work/${state_lower}-shot-$label.json"; after="$work/ui-$label-${state_lower}-after-shot.json"
  retry "$shot_json" play screenshot --session-id "$(jq -r .sessionId "$before")" --path "$state-$label.png"; receipt "$shot_json"; view_receipt "$shot_json"
  screenshot=$(jq -r .result.path "$shot_json"); [[ "$screenshot" == "$project_dir/Saved/MagiUnrealAXI/Screenshots/"* && -s "$screenshot" ]]
  observe_until "$input" "$state" "$after"; jq -e --slurpfile before "$before" '.revision == $before[0].revision and .instanceId == $before[0].instanceId and .widgets == $before[0].widgets' "$after" >/dev/null
  width=$(jq -r .result.width "$shot_json"); height=$(jq -r .result.height "$shot_json"); jq -e --argjson width "$width" --argjson height "$height" --argjson max_width "$(jq -r .limits.screenshotWidthMax "$manifest")" --argjson max_height "$(jq -r .limits.screenshotHeightMax "$manifest")" '.result.format == "png" and .result.width == $width and .result.height == $height and $width > 0 and $height > 0 and $width <= $max_width and $height <= $max_height' "$shot_json" >/dev/null
  [[ $(xxd -p -l 8 "$screenshot") == 89504e470d0a1a0a ]]; expected_ihdr=$(printf '%08x%08x' "$width" "$height"); [[ $(xxd -p -s 16 -l 8 "$screenshot") == "$expected_ihdr" ]]
  cp "$screenshot" "$work/$state-$label.png"; bmp="$work/$state-$label.bmp"; sips -s format bmp "$screenshot" --out "$bmp" >/dev/null; pixel_offset=$(od -An -t u4 -j 10 -N 4 "$bmp" | tr -d ' '); od -An -v -t u1 -j "$pixel_offset" "$bmp" | awk '{ for (i = 1; i <= NF; ++i) if ($i != 0) visible = 1 } END { exit visible ? 0 : 1 }'
}
run_pie() {
  local label=$1 sid input stale_status ready active adversarial_dir screenshot_dir
  ready="$work/ui-$label-ready.json"; active="$work/ui-$label-active.json"
  retry "$work/play-$label-start.json" play start; sid=$(jq -r .result.sessionId "$work/play-$label-start.json"); receipt "$work/play-$label-start.json"; view_receipt "$work/play-$label-start.json"
  for _ in $(seq 1 120); do axi play status --session-id "$sid" >"$work/play-$label-status.json"; [[ $(jq -r .state "$work/play-$label-status.json") == running ]] && break; sleep .2; done
  [[ $(jq -r .state "$work/play-$label-status.json") == running ]]; input="{\"sessionId\":\"$sid\",\"widgetBlueprintId\":\"$widget\",\"widgetIds\":[\"$root\",\"$text\"]}"
  if [[ -n "$first_sid" ]]; then set +e; axi capability execute play.ui_observe --input-json "{\"sessionId\":\"$first_sid\",\"widgetBlueprintId\":\"$widget\",\"widgetIds\":[\"$root\",\"$text\"]}" >"$work/stale-active-session.json"; stale_status=$?; set -e; [[ $stale_status != 0 ]]; fi
  observe_until "$input" "$ready_text" "$ready"
  jq -e --arg sid "$sid" --arg root "$root" --arg text "$text" --arg root_name "$property_root" --arg root_class "$property_root_class" --arg text_name "$text_name" --arg text_class "$text_class" --arg ready "$ready_text" '.sessionId == $sid and .inViewport == true and ([.widgets[].widgetId]) == [$root,$text] and .widgets[0] == {widgetId:$root,name:$root_name,class:$root_class,text:null,visibility:"Visible",enabled:true} and .widgets[1] == {widgetId:$text,name:$text_name,class:$text_class,text:$ready,visibility:"Visible",enabled:true} and (.revision | test("^[0-9a-f]{64}$"))' "$ready" >/dev/null
  if [[ $label == one ]]; then
    set +e; axi capability execute play.screenshot --input-json "{\"sessionId\":\"$sid\",\"path\":\"claimed.png\\u0000suffix.png\"}" >"$work/screenshot-nul-error.json"; stale_status=$?; set -e
    [[ $stale_status != 0 && $(jq -r .error.reason "$work/screenshot-nul-error.json") == invalid_capability_input && ! -e "$project_dir/Saved/MagiUnrealAXI/Screenshots/claimed.png" ]]
    screenshot_dir="$project_dir/Saved/MagiUnrealAXI/Screenshots"; adversarial_dir="$work/screenshot-symlink-target"; mkdir -p "$(dirname "$screenshot_dir")" "$adversarial_dir"; [[ ! -e "$screenshot_dir" ]]; ln -s "$adversarial_dir" "$screenshot_dir"
    set +e; axi play screenshot --session-id "$sid" --path symlink-test.png >"$work/screenshot-symlink-error.json"; stale_status=$?; set -e
    [[ $stale_status != 0 && $(jq -r .error.reason "$work/screenshot-symlink-error.json") == operation_failed && ! -e "$adversarial_dir/symlink-test.png" ]]; mv "$screenshot_dir" "$work/rejected-screenshot-symlink"
  fi
  capture_screenshot "$label" "$ready_text" "$input" "$ready"
  retry "$work/play-$label-pressed.json" play input "$input_key" --session-id "$sid" --event pressed; jq -e '.result.accepted == true and .result.changed == false' "$work/play-$label-pressed.json" >/dev/null; receipt "$work/play-$label-pressed.json"; view_receipt "$work/play-$label-pressed.json"
  observe_until "$input" "$active_text" "$active"; jq -e --slurpfile ready "$ready" --arg active "$active_text" '.instanceId == $ready[0].instanceId and .widgets[1].text == $active and .revision != $ready[0].revision' "$active" >/dev/null
  capture_screenshot "$label" "$active_text" "$input" "$active"
  retry "$work/play-$label-released.json" play input "$input_key" --session-id "$sid" --event released; jq -e '.result.accepted == true and .result.changed == false' "$work/play-$label-released.json" >/dev/null; receipt "$work/play-$label-released.json"; view_receipt "$work/play-$label-released.json"
  [[ "$(shasum -a 256 "$work/READY-$label.png" | cut -d' ' -f1)" != "$(shasum -a 256 "$work/ACTIVE-$label.png" | cut -d' ' -f1)" ]]
  if [[ -z "$first_sid" ]]; then first_sid=$sid; first_instance=$(jq -r .instanceId "$ready"); first_ready_widgets=$(jq -c .widgets "$ready"); first_active_widgets=$(jq -c .widgets "$active"); else [[ "$sid" != "$first_sid" && $(jq -r .instanceId "$ready") != "$first_instance" ]]; [[ $(jq -c .widgets "$ready") == "$first_ready_widgets" && $(jq -c .widgets "$active") == "$first_active_widgets" ]]; fi
  retry "$work/play-$label-stop.json" play stop --session-id "$sid"; receipt "$work/play-$label-stop.json"; view_receipt "$work/play-$label-stop.json"
  set +e; axi capability execute play.ui_observe --input-json "$input" >"$work/stale-$label.json"; stale_status=$?; set -e; [[ $stale_status != 0 ]]
}
run_pie one
run_pie two
# Slate editor geometry may differ after PIE teardown; each READY/ACTIVE observation sandwich must retain exact dimensions within its session.
one_width=$(jq -r .result.width "$work/ready-shot-one.json"); one_height=$(jq -r .result.height "$work/ready-shot-one.json")
two_width=$(jq -r .result.width "$work/ready-shot-two.json"); two_height=$(jq -r .result.height "$work/ready-shot-two.json")
[[ $(jq -r .result.width "$work/active-shot-one.json") == "$one_width" && $(jq -r .result.height "$work/active-shot-one.json") == "$one_height" ]]
[[ $(jq -r .result.width "$work/active-shot-two.json") == "$two_width" && $(jq -r .result.height "$work/active-shot-two.json") == "$two_height" ]]
stop_editor
if [[ -n "${P13_PACKAGE_PROJECT_DIR:-}" ]]; then
  [[ -d "$package_project_dir" && -f "$package_project_dir/MagiUnrealAXIPackageFixture.uproject" ]]
  destination="$package_project_dir/Content/$content_root"; [[ ! -e "$destination" ]]; mkdir -p "$destination"; ditto "$project_dir/Content/$content_root" "$destination"
  while IFS= read -r required; do [[ -f "$package_project_dir/Content/$required" ]]; done < <(jq -r '.packageAssertions.widget.file,.packageAssertions.host.file,.packageAssertions.map' "$manifest")
  (cd "$project_dir/Content/$content_root" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s  %s\n' "$(shasum -a 256 "$file" | cut -d' ' -f1)" "$file"; done) >"$work/source-content.txt"
  (cd "$destination" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s  %s\n' "$(shasum -a 256 "$file" | cut -d' ' -f1)" "$file"; done) >"$work/destination-content.txt"; diff -u "$work/source-content.txt" "$work/destination-content.txt" >/dev/null
  package_claim="Content/$content_root"
fi
cp "$work"/*.json "$evidence/"
cp "$work"/*.png "$evidence/"
if [[ -n "${P13_PACKAGE_PROJECT_DIR:-}" ]]; then cp "$work/source-content.txt" "$work/destination-content.txt" "$evidence/"; fi
scan_runtime_material() {
  local root=$1 status
  [[ -z "$(find "$root" -type f \( -name token -o -name bridge-v1.json \) -print -quit)" ]]
  set +e; grep -R -I -E -q 'Authorization:[[:space:]]*Bearer[[:space:]]+[A-Za-z0-9._-]+' "$root"; status=$?; set -e
  [[ $status == 1 ]]
}
printf 'phase=P1.3\ncatalogHash=%s\nartifactSha256=%s\ncliSha256=%s\nfixture=ui-state-loop\nwidgetTree=StateRoot/StateText\ninitialState=READY\nactiveState=ACTIVE\nevent=%s\nviewport=input-E-z0\npieSessions=2-deterministic-reset\nscreenshots=READY-ACTIVE-session-one-%sx%s-session-two-%sx%s-capture-sandwich-hash-different\nscreenshotDimensionInvariant=stable-within-session\nstaleSession=rejected\ninvalidInputs=preserved-revision-and-tree\npackage=%s\ntokenScan=passed\n' "$catalog_hash" "$artifact_hash" "$cli_hash" "$agent" "$one_width" "$one_height" "$two_width" "$two_height" "$package_claim" | tee "$evidence/summary.txt"
for secret in "${tokens[@]}"; do
  for root_to_scan in "$work" "$evidence" ${P13_WORKSPACE:+"$P13_WORKSPACE"}; do
    set +e; grep -R -I -Fq -- "$secret" "$root_to_scan"; secret_status=$?; set -e
    [[ $secret_status == 1 ]]
  done
done
scan_runtime_material "$work"
scan_runtime_material "$evidence"
[[ -z "${P13_WORKSPACE:-}" ]] || scan_runtime_material "$P13_WORKSPACE"
[[ -z "${P13_LIVE_EVIDENCE_DIR:-}" ]] && printf '%s\n' "$evidence" >"$cache_root/latest"
echo "P1.3 live certification: PASS (evidence retained at $evidence)"
