#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.6-manifest.json"
cache_root="${P16_CACHE_ROOT:-$HOME/Library/Caches/magi-unreal-axi/p1.6/agents}"
engine_root="${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
JOBS=(unknown-project-orientation interaction-loop ui-state-loop ai-navigation-loop animation-state-loop)
fail(){ echo "P1.6 agent evaluation failed: $*" >&2; exit 1; }
usage(){ cat <<'USAGE'
Usage:
  evaluate-p1.6-agents.sh prepare ARTIFACT.tar.gz
  evaluate-p1.6-agents.sh job-context RUN JOB
  evaluate-p1.6-agents.sh record RUN JOB SESSION_JSONL
  evaluate-p1.6-agents.sh finalize RUN
  evaluate-p1.6-agents.sh revalidate RUN COMBINED
  evaluate-p1.6-agents.sh --self-test
USAGE
}
[[ -f "$manifest" ]] || fail "manifest missing"
[[ $# -ge 1 ]] || { usage; exit 2; }
mode=$1; shift
for command in jq shasum tar codesign git ditto trash ruby lipo; do
  case "$mode" in prepare|finalize) command -v "$command" >/dev/null || fail "$command missing" ;; esac
done
is_job(){ printf '%s\n' "${JOBS[@]}" | grep -Fxq "$1"; }
job_index(){ local job=$1 index; for index in "${!JOBS[@]}"; do [[ ${JOBS[$index]} == "$job" ]] && { echo $((index+1)); return; }; done; return 1; }
run_dir(){ [[ $1 = /* ]] && printf '%s\n' "$1" || printf '%s/%s\n' "$cache_root" "$1"; }
hash(){ shasum -a 256 "$1" | awk '{print $1}'; }

plugin_inventory(){
  local root=$1 output=$2 relative
  (cd "$root" && find . -type f ! -name plugin-tree.sha256 -print | LC_ALL=C sort | while IFS= read -r relative; do printf '%s\t%s\n' "${relative#./}" "$(hash "$relative")"; done) >"$output"
}
copy_tracked(){
  local source=$1 destination=$2 relative
  mkdir -p "$destination"
  while IFS= read -r -d '' relative; do
    [[ -f "$repo_root/$relative" && ! -L "$repo_root/$relative" ]] || fail "tracked source is not regular: $relative"
    relative=${relative#"$source"/}
    mkdir -p "$destination/$(dirname "$relative")"
    cp -p "$repo_root/$source/$relative" "$destination/$relative"
  done < <(git -C "$repo_root" ls-files -z -- "$source/")
}

write_prompt(){
  local directory=$1 job=$2 required receipt_readbacks
  case $job in
    unknown-project-orientation) required='Read-only orientation. Required exact help, project doctor, engine view, setup plugin status, editor status, capability search total 79, generated catalog scope, and nonempty next safe action. Help may be plain nonempty text; all other stdout JSON objects. Never mutate, package, or start/stop editor.' ;;
    interaction-loop) required='Start/status editor. Author exact interface, two Actor Blueprints, components, owners/GUIDs/config, graph wiring, compile/save all assets, exact level.save. Restart/open/read back exact level. Spawn created classes; run exactly 2 ordered PIE chains with accepted input and deterministic observations. Final stop, one cook, one package.' ;;
    ui-state-loop) required='Start/status editor. Author widget/host, derived root/text IDs, parent, READY, exact E action to ACTIVE and viewport, compile/save assets and level. Restart/read back tree. Two sessions prove READY, accepted E, ACTIVE, new instance/reset. Final stop, one cook, one package.' ;;
    ai-navigation-loop) required='Start/status editor. Save BB/BT/controller/pawn/floor; prove TargetActor, exact loop/move/wait nodes and 3 links, configs, floor, bounds, level/readbacks. Restart; nav succeeds/reachable. Session one moves to target; session two null target idle. Final stop, one cook, one package.' ;;
    animation-state-loop) required='Start/status editor. Use exact complete contract: assets /Game/MagiP15/BB_P16AnimationAI.BB_P16AnimationAI, /Game/MagiP15/BT_P16AnimationAI.BT_P16AnimationAI, /Game/MagiP15/BP_P16AnimationController.BP_P16AnimationController, /Game/MagiP15/BP_P16AnimationFloor.BP_P16AnimationFloor, /Game/MagiP15/ABP_P15Animation.ABP_P15Animation, /Game/MagiP15/BP_P15Character.BP_P15Character. Blackboard key TargetActor with key type Actor. Behavior tree nodes loop (sequence), move (move_to using TargetActor), wait (wait 0.5); links root->loop child index 0, loop->move child index 0, loop->wait child index 1. Controller uses BT; same Character pawn config uses controller. FloorBox BoxComponent uses QueryAndPhysics, BlockAll, extent [1000,1000,25]. Level /Game/MagiP15/P15Animation. Navigation bounds agent key p14-nav at [0,0,0], extent [1000,1000,100]; floor at [0,0,-25], Character at [0,0,100], TargetPoint at [600,0,100]. Animation authoring contract: skeleton /Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seed_Skeleton.magi-p15-owned-seed_Skeleton; skeletal mesh /Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seed.magi-p15-owned-seed; idle sequence /Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seedIdle.magi-p15-owned-seedIdle; moving sequence /Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seedMoving.magi-p15-owned-seedMoving. Exact blueprint parent classes: controller /Script/AIModule.AIController, floor /Script/Engine.Actor, Character /Script/Engine.Character. Animation graph has Speed float variable owner_planar_speed, locomotion state machine, idle and moving states mapped to exact idle and moving sequences; transitions idle->moving when Speed > 10 and moving->idle when Speed <= 10. Compile and save all assets and level; restart editor, open exact level, and prove persisted readbacks for blackboard, behavior tree, controller, Character, floor, animation graph, bounds, and spawned actors. Build navigation and prove succeeded status plus reachable non-partial path with positive path length and endpoints [0,0,100] to [600,0,100]. Run exactly two sessions; each exact order: start, observe idle animation, set TargetActor on same Character pawn to spawned TargetPoint, observe moving AI with move active, observe moving animation, observe reached AI with wait active, distance to target <=50 and Character near target, observe final idle animation, stop. Final editor stop, one cook, one package.' ;;
  esac
  if [[ $job != unknown-project-orientation ]]; then
    case $job in
      interaction-loop) receipt_readbacks='blueprint.interface_create; blueprint.compile for /Game/MagiP12/BP_Interactable.BP_Interactable; asset.save for /Game/MagiP12/BPI_Interact.BPI_Interact; level.save; level.open; unchanged post-restart blueprint.interface_ensure' ;;
      ui-state-loop) receipt_readbacks='widget.create; blueprint.compile for /Game/MagiP13/WBP_UIState.WBP_UIState; asset.save for /Game/MagiP13/WBP_UIState.WBP_UIState; level.save; level.open; unchanged post-restart widget.event_ensure' ;;
      ai-navigation-loop) receipt_readbacks='blackboard.create; blueprint.compile for /Game/MagiP14/BP_P14AIController.BP_P14AIController; asset.save for /Game/MagiP14/BB_P14AI.BB_P14AI; level.save; level.open; unchanged post-restart blackboard.key_ensure' ;;
      animation-state-loop) receipt_readbacks='animation_blueprint.create; blueprint.compile for /Game/MagiP15/ABP_P15Animation.ABP_P15Animation; asset.save for /Game/MagiP15/ABP_P15Animation.ABP_P15Animation; level.save; level.open; unchanged post-restart animation.variable_ensure' ;;
    esac
    required+=" Durable receipt contract: invoke operation view once for each exact source: $receipt_readbacks. Also view second play.stop only after final editor.stop to prove offline journal recovery. View project cook immediately after cook and project package immediately after package. Exactly 9 operation.view calls total. Every view ID comes from its selected prior source; viewed JSON must exactly equal native receipt or cook/package operation summary. Classify each view reference with source category: compile,compile,save,save,restart,noop,runtime,package,package."
  fi
  cat >"$directory/PROMPT.txt" <<EOF
P1.6 representative agent job: $job
Project: $directory/project/MagiUnrealAXIPackageFixture.uproject
Recorder: $directory/bin/axi-record
Package output must remain under: $directory/package

$required

Use Recorder for every CLI invocation. First read skill://axi with read tool. Bash calls MUST invoke only Recorder as one executable plus exact argv. No direct binary, shell compound command, redirection, substitution, pipeline, or non-wrapper command. Use --project project path, --engine "$engine_root", --format json exactly once for every non-help invocation. Public editor start/status/stop lifecycle goes through Recorder. Derive IDs, revisions, session IDs, and operation IDs from earlier stdout. Never read tests/manifests, prior evidence, bridge-v1.json, token files, or runtime directories. Never use Python, MCP, Unreal console/Python, raw bridge/network access, direct Content mutation, or repository scripts. All invocations must exit zero; do not retry failed calls. Construction runs exactly one public project cook --output $directory/cooked and one project package --output $directory/package after runtime and final editor stop. Harness alone runs external artifact inspectors.

Write agent outcome once, as final tool call, to $directory/agent-outcome.json. Exact schema: {"job":"$job","status":"passed","requiredCategories":[],"references":{"orientation":[],"compile":[],"save":[],"restart":[],"runtime":[],"noop":[],"package":[]},"expectedFailureSequences":[],"metrics":{"retries":0,"avoidableRetries":0,"structuredOutputFailures":0}}
Orientation requiredCategories=["orientation"] and only orientation references are nonempty. Construction requiredCategories=["compile","save","restart","runtime","noop","package"] and exactly those references are nonempty. References are increasing unique Recorder sequence integers; each sequence belongs to one category. Compile references cover initial editor lifecycle plus all authoring and compile commands before authoring stop; save references cover asset.save and level.save; restart references cover author stop/start/status, exact level.open, and persisted readbacks; noop references cover unchanged post-restart proof; runtime references cover navigation runtime proof and both PIE sessions; package references cover final editor.stop, project cook, and project package. Final write must contain exact observed sequence numbers.
EOF
}

write_wrapper(){
  local directory=$1 binary=$2 ledger=$3 actual_home=$HOME
  {
    printf '%s\n' '#!/usr/bin/env bash' 'set -u' 'set -o pipefail'
    printf 'binary=%q\nledger=%q\nsequence_file=%q\nactual_home=%q\nproject=%q\n' "$binary" "$ledger" "$directory/sequence" "$actual_home" "$directory/project/MagiUnrealAXIPackageFixture.uproject"
    cat <<'WRAPPER'
cwd=$(pwd -P)
[[ "$cwd" == "$(dirname "$project")" ]] || { echo 'axi-record: wrong cwd' >&2; exit 2; }
sequence=$(cat "$sequence_file"); sequence=$((sequence+1)); printf '%s\n' "$sequence" >"$sequence_file"
temporary=$(mktemp -d "${TMPDIR:-/tmp}/axi-record.XXXXXX"); trap 'trash "$temporary" >/dev/null 2>&1 || true' EXIT
started=$(ruby -rtime -e 'puts Time.now.utc.iso8601(9)')
set +e; HOME="$actual_home" "$binary" "$@" >"$temporary/stdout" 2>"$temporary/stderr"; status=$?; set -e
ended=$(ruby -rtime -e 'puts Time.now.utc.iso8601(9)')
cat "$temporary/stdout"; cat "$temporary/stderr" >&2
stdout_bytes=$(wc -c <"$temporary/stdout" | tr -d ' '); stderr_bytes=$(wc -c <"$temporary/stderr" | tr -d ' ')
stdout_hash=$(shasum -a 256 "$temporary/stdout" | awk '{print $1}'); stderr_hash=$(shasum -a 256 "$temporary/stderr" | awk '{print $1}')
stdout_path="$ledger.$sequence.stdout"; stderr_path="$ledger.$sequence.stderr"; cp -p "$temporary/stdout" "$stdout_path"; cp -p "$temporary/stderr" "$stderr_path"
argv=$(jq -cn --args '$ARGS.positional' -- "$@")
jq -cn --argjson sequence "$sequence" --argjson argv "$argv" --arg cwd "$cwd" --arg started "$started" --arg ended "$ended" --argjson exit "$status" --arg stdoutPath "$stdout_path" --arg stderrPath "$stderr_path" --argjson stdoutBytes "$stdout_bytes" --argjson stderrBytes "$stderr_bytes" --arg stdoutSha256 "$stdout_hash" --arg stderrSha256 "$stderr_hash" '{sequence:$sequence,argv:$argv,cwd:$cwd,started:$started,ended:$ended,exit:$exit,stdoutPath:$stdoutPath,stderrPath:$stderrPath,stdoutBytes:$stdoutBytes,stderrBytes:$stderrBytes,stdoutSha256:$stdoutSha256,stderrSha256:$stderrSha256,estimatedTokens:(($stdoutBytes+3)/4|floor),homeCategory:"actual-account-home"}' >>"$ledger"
exit "$status"
WRAPPER
  } >"$directory/bin/axi-record"
  chmod 0555 "$directory/bin/axi-record"; printf '0\n' >"$directory/sequence"
}

ledger_valid(){
  local directory=$1 project_dir=$2 engine=$3
  ruby -rjson -rtime -rdigest -e '
    dir, project_dir, engine = ARGV
    ledger = File.join(dir, "ledger.jsonl")
    abort unless File.file?(ledger) && !File.symlink?(ledger)
    rows = File.readlines(ledger, chomp: true).map { |line| JSON.parse(line) }
    abort if rows.empty?
    keys = %w[argv cwd ended estimatedTokens exit homeCategory sequence started stderrBytes stderrPath stderrSha256 stdoutBytes stdoutPath stdoutSha256].sort
    project = File.join(project_dir, "MagiUnrealAXIPackageFixture.uproject")
    previous_end = nil
    rows.each_with_index do |row, index|
      n = index + 1
      abort unless row.keys.sort == keys && row["sequence"] == n
      abort unless row["argv"].is_a?(Array) && !row["argv"].empty? && row["argv"].all? { |v| v.is_a?(String) && !v.empty? }
      abort unless row["cwd"] == project_dir && row["homeCategory"] == "actual-account-home"
      %w[stdoutBytes stderrBytes estimatedTokens exit].each { |k| abort unless row[k].is_a?(Integer) && row[k] >= 0 }
      abort unless row["exit"] <= 255 && row["estimatedTokens"] == (row["stdoutBytes"] + 3) / 4
      started = Time.iso8601(row["started"]); ended = Time.iso8601(row["ended"])
      abort unless row["started"].match?(/\.\d{9}Z\z/) && row["ended"].match?(/\.\d{9}Z\z/) && started <= ended
      abort if previous_end && started < previous_end
      previous_end = ended
      %w[stdout stderr].each do |kind|
        path = "#{ledger}.#{n}.#{kind}"
        stat = File.lstat(path) rescue abort
        abort unless stat.file? && !File.symlink?(path) && row["#{kind}Path"] == path
        data = File.binread(path)
        abort unless row["#{kind}Bytes"] == data.bytesize && row["#{kind}Sha256"] == Digest::SHA256.hexdigest(data)
      end
      argv = row["argv"]
      if argv.include?("--help")
        abort unless argv.count("--help") == 1
      else
        {"--project"=>project,"--engine"=>engine,"--format"=>"json"}.each do |flag, value|
          abort unless argv.count(flag) == 1
          i = argv.index(flag); abort unless argv[i + 1] == value
        end
      end
    end
    abort unless File.file?(File.join(dir, "sequence")) && Integer(File.read(File.join(dir, "sequence")).strip) == rows.length
  ' "$directory" "$project_dir" "$engine" >/dev/null 2>&1
}

session_valid(){
  local session=$1 project_dir=$2 wrapper=$3 directory
  directory=$(cd "$(dirname "$wrapper")/.." && pwd -P)
  ledger_valid "$directory" "$project_dir" "$engine_root" || return 1
  ruby -rjson -rshellwords -e '
    session, project_dir, wrapper, ledger_path, outcome = ARGV
    events = File.readlines(session, chomp: true).map { |line| JSON.parse(line) }
    abort if events.empty? || events.any? { |e| !e["session_id"].is_a?(String) || e["session_id"].empty? || e["cwd"] != project_dir }
    abort unless events.map { |e| e["session_id"] }.uniq.length == 1
    calls = events.select { |e| e["event_type"] == "tool_call" }
    results = events.select { |e| e["event_type"] == "tool_result" }
    abort unless calls.map { |e| e.dig("payload", "id") }.all? { |id| id.is_a?(String) && !id.empty? }
    abort unless calls.map { |e| e.dig("payload", "id") }.uniq.length == calls.length
    abort unless calls.map { |e| e.dig("payload", "id") } == results.map { |e| e.dig("payload", "call_id") }
    abort unless results.all? { |e| e["success"] == true || e.dig("payload", "result", "success") == true }
    users = events.each_index.select { |i| events[i]["event_type"] == "user_input" }; assistants = events.each_index.select { |i| events[i]["event_type"] == "assistant_output" }
    abort unless users.length == 1 && assistants.length == 1 && users.first < (events.index(calls.first) || events.length) && assistants.first == events.length - 1
    rows = File.readlines(ledger_path, chomp: true).map { |line| JSON.parse(line) }
    result_by_id = results.each_with_object({}) { |e,h| h[e.dig("payload","call_id")] = e }
    bash_index = 0; reads = 0; writes = 0
    calls.each do |call|
      name = call.dig("payload", "name"); args = call.dig("payload", "arguments")
      abort unless %w[bash read write].include?(name) && args.is_a?(Hash)
      case name
      when "bash"
        abort unless bash_index < rows.length && (args.keys.sort == ["command"] || args.keys.sort == ["command", "timeout"]) && (!args.key?("timeout") || (args["timeout"].is_a?(Integer) && args["timeout"].between?(1, 300)))
        tokens = Shellwords.split(args["command"]); abort unless tokens == [wrapper] + rows[bash_index]["argv"]
        result = result_by_id.fetch(call.dig("payload","id")); wrapped_success = result.dig("payload","result","success")
        abort unless wrapped_success.nil? || wrapped_success == (rows[bash_index]["exit"] == 0)
        bash_index += 1
      when "read"
        reads += 1; abort unless reads == 1 && (args.keys - %w[paths offset limit]).empty? && args["paths"] == ["skill://axi"]
        abort if args.key?("offset") && args["offset"] != 1
        abort if args.key?("limit") && args["limit"] != 2000
      when "write"
        writes += 1; abort unless writes == 1 && call.equal?(calls.last) && args.keys.sort == ["content", "path"] && args["path"] == outcome
        abort unless args["content"].is_a?(String) && JSON.parse(args["content"]) == JSON.parse(File.read(outcome))
      end
    end
    abort unless reads == 1 && writes == 1 && bash_index == rows.length
  ' "$session" "$project_dir" "$wrapper" "$directory/ledger.jsonl" "$directory/agent-outcome.json" >/dev/null 2>&1
}

validate_outcome(){
  local directory=$1 job=$2
  ruby "$repo_root/tests/unreal/support/p16-outcome.rb" validate "$directory" "$job" "$repo_root"
}

record_valid(){
  local directory=$1 job=$2 previous_end=${3-}
  ruby -rjson -rtime -rdigest -e '
    dir, job, previous_end = ARGV
    record = JSON.parse(File.binread(File.join(dir, "record.json")))
    abort unless record.is_a?(Hash) && record.keys.sort == %w[ended job recordedAt sessionSha256 started].sort && record["job"] == job
    rows = File.readlines(File.join(dir, "ledger.jsonl"), chomp: true).map { |line| JSON.parse(line) }
    abort if rows.empty?
    starts = rows.map { |row| Time.iso8601(row.fetch("started")) }
    ends = rows.map { |row| Time.iso8601(row.fetch("ended")) }
    recorded_start = Time.iso8601(record.fetch("started")); recorded_end = Time.iso8601(record.fetch("ended")); Time.iso8601(record.fetch("recordedAt"))
    abort unless recorded_start == starts.min && recorded_end == ends.max && rows.each_index.all? { |index| starts[index] <= ends[index] && (index.zero? || starts[index] >= ends[index - 1]) }
    abort unless previous_end.empty? || recorded_start > Time.iso8601(previous_end)
    abort unless record["sessionSha256"] == Digest::SHA256.file(File.join(dir, "session.jsonl")).hexdigest
    puts record.fetch("ended")
  ' "$directory" "$job" "$previous_end" 2>/dev/null
}

plugin_valid(){
  local directory=$1 plugin expected count arches current status
  [[ -f "$directory/plugin-install.json" && -f "$directory/plugin.sha256" && -f "$directory/plugin-tree.sha256" ]] || return 1
  jq -e --arg plugin "$directory/project/Plugins/MagiUnrealAXI" --arg project "$directory/project/MagiUnrealAXIPackageFixture.uproject" '.plugin.installed==true and .plugin.managed==true and .plugin.compatible==true and .plugin.path==$plugin and .projectDescriptor.path==$project' "$directory/plugin-install.json" >/dev/null || return 1
  count=$(find "$directory/project/Plugins/MagiUnrealAXI" -type f -name libUnrealEditor-MagiUnrealAXI.dylib | wc -l | tr -d ' '); [[ $count == 1 ]] || return 1
  plugin=$(find "$directory/project/Plugins/MagiUnrealAXI" -type f -name libUnrealEditor-MagiUnrealAXI.dylib -print -quit)
  [[ -f "$plugin" && ! -L "$plugin" ]] || return 1; expected=$(cat "$directory/plugin.sha256"); [[ $expected =~ ^[0-9a-f]{64}$ && $(hash "$plugin") == "$expected" ]] || return 1
  arches=$(lipo -archs "$plugin"); grep -Eqw 'arm64' <<<"$arches" || return 1
  current=$(mktemp); plugin_inventory "$directory/project/Plugins/MagiUnrealAXI" "$current"; cmp -s "$current" "$directory/plugin-tree.sha256"; status=$?; trash "$current" >/dev/null 2>&1 || true; return $status
}

case $mode in
  --self-test|self-test)
    ruby "$repo_root/tests/unreal/support/p16-agent-lifecycle-fixtures.rb" || fail "hermetic lifecycle fixture self-test"
    temporary=$(mktemp -d); temporary=$(cd "$temporary" && pwd -P); trap 'trash "$temporary" >/dev/null 2>&1 || true' EXIT
    mkdir -p "$temporary/bin" "$temporary/project"; printf '#!/usr/bin/env bash\nprintf "{}\\n"\n' >"$temporary/bin/dummy"; chmod +x "$temporary/bin/dummy"
    write_wrapper "$temporary" "$temporary/bin/dummy" "$temporary/ledger.jsonl"
    project="$temporary/project"; wrapper="$temporary/bin/axi-record"; project_file="$project/MagiUnrealAXIPackageFixture.uproject"
    (cd "$project" && "$wrapper" --project "$project_file" --engine "$engine_root" --format json project doctor >/dev/null)
    hostile='{"value":"a; b | c & d < e > f $(never)"}'
    (cd "$project" && "$wrapper" --project "$project_file" --engine "$engine_root" --format json capability execute test.operation --input-json "$hostile" >/dev/null)
    printf '{}\n' >"$temporary/agent-outcome.json"
    ruby -rjson -rshellwords -e '
      dir, project, wrapper = ARGV; rows=File.readlines(File.join(dir,"ledger.jsonl"),chomp:true).map{|l|JSON.parse(l)}; sid="self-test"; events=[]
      add=->(name,args,id){events << {event_type:"tool_call",session_id:sid,cwd:project,payload:{id:id,name:name,arguments:args}}; events << {event_type:"tool_result",session_id:sid,cwd:project,payload:{call_id:id,result:{success:true}}}}
      events << {event_type:"user_input",session_id:sid,cwd:project}; add.call("read",{paths:["skill://axi"],offset:1,limit:2000},"read")
      rows.each_with_index{|r,i|add.call("bash",{command:Shellwords.join([wrapper]+r["argv"]),timeout:30},"bash#{i}")}
      add.call("write",{path:File.join(dir,"agent-outcome.json"),content:"{}\n"},"write"); events << {event_type:"assistant_output",session_id:sid,cwd:project}
      File.write(File.join(dir,"session.jsonl"),events.map{|e|JSON.generate(e)}.join("\n")+"\n")
    ' "$temporary" "$project" "$wrapper"
    ledger_valid "$temporary" "$project" "$engine_root" || fail "generated ledger self-test"
    session_valid "$temporary/session.jsonl" "$project" "$wrapper" || fail "actual-schema/quoted argv self-test"
    cp "$temporary/ledger.jsonl" "$temporary/ledger.good"; cp "$temporary/session.jsonl" "$temporary/session.good"; cp "$temporary/ledger.jsonl.1.stdout" "$temporary/sidecar.good"
    jq -c 'if .event_type=="tool_call" and .payload.name=="bash" then del(.payload.arguments.timeout) else . end' "$temporary/session.good" >"$temporary/no-timeout.jsonl"
    session_valid "$temporary/no-timeout.jsonl" "$project" "$wrapper" || fail "optional bash timeout self-test"
    for invalid_timeout in 0 -1 301; do jq -c --argjson timeout "$invalid_timeout" 'if .event_type=="tool_call" and .payload.name=="bash" then .payload.arguments.timeout=$timeout else . end' "$temporary/session.good" >"$temporary/bad.jsonl"; session_valid "$temporary/bad.jsonl" "$project" "$wrapper" && fail "invalid bash timeout accepted: $invalid_timeout"; done
    printf '{}\n' >"$temporary/ledger.jsonl"; ledger_valid "$temporary" "$project" "$engine_root" && fail "null ledger accepted"; cp "$temporary/ledger.good" "$temporary/ledger.jsonl"
    tail -r "$temporary/ledger.good" >"$temporary/ledger.jsonl"; ledger_valid "$temporary" "$project" "$engine_root" && fail "reordered ledger accepted"; cp "$temporary/ledger.good" "$temporary/ledger.jsonl"
    printf 'tamper\n' >>"$temporary/ledger.jsonl.1.stdout"; ledger_valid "$temporary" "$project" "$engine_root" && fail "tampered sidecar accepted"; cp "$temporary/sidecar.good" "$temporary/ledger.jsonl.1.stdout"
    jq -c 'if .event_type=="tool_result" and .payload.call_id=="bash0" then .payload.call_id="wrong" else . end' "$temporary/session.good" >"$temporary/bad.jsonl"; session_valid "$temporary/bad.jsonl" "$project" "$wrapper" && fail "wrong result id accepted"
    jq -c 'if .event_type=="tool_call" and .payload.name=="bash" then .payload.arguments.command="/bin/true" else . end' "$temporary/session.good" >"$temporary/bad.jsonl"; session_valid "$temporary/bad.jsonl" "$project" "$wrapper" && fail "direct command accepted"
    jq -c 'if .event_type=="tool_call" and .payload.name=="bash" then .payload.arguments.command += "; true" else . end' "$temporary/session.good" >"$temporary/bad.jsonl"; session_valid "$temporary/bad.jsonl" "$project" "$wrapper" && fail "compound command accepted"
    jq -c 'if .event_type=="tool_call" and .payload.name=="read" then .payload.arguments.paths += ["/etc/passwd"] else . end' "$temporary/session.good" >"$temporary/bad.jsonl"; session_valid "$temporary/bad.jsonl" "$project" "$wrapper" && fail "extra read accepted"
    jq -c 'if .sequence==1 then .argv[(.argv|index("--project"))+1]="/wrong" else . end' "$temporary/ledger.good" >"$temporary/ledger.jsonl"; ledger_valid "$temporary" "$project" "$engine_root" && fail "wrong project accepted"; cp "$temporary/ledger.good" "$temporary/ledger.jsonl"
    ruby "$repo_root/tests/unreal/support/p16-run-inventory.rb" self-test
    ruby "$repo_root/tests/unreal/support/p16-revalidate.rb" self-test
    ruby "$repo_root/tests/unreal/support/p16-outcome.rb" self-test "$repo_root"
    printf 'P1.6 agent harness self-test: PASS\n'
    ;;
  prepare)
    [[ $# == 1 && -f $1 ]] || { usage; exit 2; }
    artifact=$(cd "$(dirname "$1")" && pwd -P)/$(basename "$1")
    [[ ${P16_EXPECTED_ARTIFACT_SHA256:-} =~ ^[0-9a-fA-F]{64}$ ]] || fail "P16_EXPECTED_ARTIFACT_SHA256 required"
    [[ ${P16_EXPECTED_SOURCE_COMMIT:-} =~ ^[0-9a-f]{40}$ ]] || fail "P16_EXPECTED_SOURCE_COMMIT required"
    [[ -z $(git -C "$repo_root" status --porcelain=v1 --untracked-files=all) ]] || fail "repository must be clean at prepare"
    [[ "$(git -C "$repo_root" rev-parse HEAD)" == "$P16_EXPECTED_SOURCE_COMMIT" ]] || fail "source commit differs from HEAD"
    [[ ${P16_EXPECTED_COMBINED_EVIDENCE_TREE_SHA256:-} =~ ^[0-9a-f]{64}$ ]] || fail "P16_EXPECTED_COMBINED_EVIDENCE_TREE_SHA256 required"
    trusted=$(printf '%s' "$P16_EXPECTED_ARTIFACT_SHA256" | tr A-F a-f)
    combined_pointer="$HOME/Library/Caches/magi-unreal-axi/p1.6/combined/latest"
    [[ -f $combined_pointer ]] || fail "combined latest missing"
    combined=$(cat "$combined_pointer"); [[ -d "$combined" && -f "$combined/summary.txt" ]] || fail "combined evidence missing"
    grep -Fxq "artifactSha256=$trusted" "$combined/summary.txt" || fail "artifact differs from combined pass"
    combined_tree=$(jq -r .treeSha256 "$combined/evidence-tree.json"); [[ $combined_tree == "$P16_EXPECTED_COMBINED_EVIDENCE_TREE_SHA256" ]] || fail "combined evidence tree differs from trusted hash"
    ruby "$repo_root/tests/unreal/support/p16-evidence.rb" verify "$combined" "$combined/evidence-tree.json" >/dev/null || fail "combined evidence tree invalid"
    combined_provenance=$(sed -n 's/^provenanceSha256=//p' "$combined/summary.txt"); [[ $combined_provenance =~ ^[0-9a-f]{64}$ && $(hash "$combined/provenance.json") == "$combined_provenance" ]] || fail "combined provenance hash mismatch"
    ruby "$repo_root/tests/unreal/support/p16-provenance.rb" verify "$repo_root" "$combined/source-inventory.tsv" "$combined/provenance.json" "$combined/provenance-identities.json" >/dev/null || fail "combined provenance invalid"
    [[ $(jq -r .source.commit "$combined/provenance.json") == "$P16_EXPECTED_SOURCE_COMMIT" ]] || fail "combined source commit mismatch"
    combined_plugin=$(sed -n 's/^pluginSha256=//p' "$combined/summary.txt"); [[ $combined_plugin =~ ^[0-9a-f]{64}$ ]] || fail "combined plugin hash missing"
    mkdir -p "$cache_root"; run=$(mktemp -d "$cache_root/run.XXXXXX"); chmod 0700 "$run"
    stage="$run/artifact"; mkdir "$stage"; staged="$stage/$(basename "$artifact")"
    cp -p "$artifact" "$staged"; cp -p "$(dirname "$artifact")/SHA256SUMS" "$stage/SHA256SUMS"
    P16_ARTIFACT_STAGED=1 P16_ARTIFACT_STAGE_DIR="$stage" P16_EXPECTED_ARTIFACT_SHA256="$trusted" "$repo_root/tests/unreal/verify-p1.6-kickoff.sh" "$staged" >"$run/kickoff.txt"
    [[ $(hash "$staged") == "$trusted" ]] || fail "staged artifact digest"
    version=$(sed -n 's/^version = "\([^"]*\)"/\1/p' "$repo_root/Cargo.toml" | head -1); archive_root="magi-unreal-axi-${version}-macos-arm64"
    mkdir "$run/extract"; tar -xzf "$staged" -C "$run/extract"; exact="$run/extract/$archive_root/magi-unreal-axi"
    [[ -x $exact && ! -L $exact ]] || fail "exact binary missing"; codesign --verify --strict "$exact" >/dev/null 2>&1 || fail "exact binary codesign"
    engine_version="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"; [[ -f $engine_version && $(uname -m) == arm64 ]] || fail "certified engine/host missing"
    [[ $(plutil -extract Changelist raw -o - "$engine_version") == 56057345 ]] || fail "engine changelist"
    git -C "$repo_root" status --porcelain=v1 --untracked-files=all >"$run/git-status.before"
    printf '%s\n' "$trusted" >"$run/artifact.sha256"; printf '%s\n' "$combined_plugin" >"$run/plugin.sha256"; printf '%s\n' "$exact" >"$run/exact-binary"; hash "$exact" >"$run/exact-binary.sha256"
    previous_end=0
    for job in "${JOBS[@]}"; do
      directory="$run/jobs/$job"; mkdir -p "$directory/project" "$directory/bin" "$directory/agent-home" "$directory/cooked" "$directory/package"
      copy_tracked tests/unreal/MagiUnrealAXIPackageFixture "$directory/project"
      if [[ $job == animation-state-loop ]]; then
        copy_tracked tests/unreal/MagiP15AnimationSeed/Content "$directory/project/Content"
        (cd "$repo_root/tests/unreal/MagiP15AnimationSeed/Content" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s\t%s\n' "${file#./}" "$(hash "$file")"; done) >"$directory/seed-source.txt"
        (cd "$directory/project/Content" && find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do printf '%s\t%s\n' "${file#./}" "$(hash "$file")"; done) >"$directory/seed-destination.txt"
        diff -u "$directory/seed-source.txt" "$directory/seed-destination.txt" >/dev/null || fail "animation seed differs"
      fi
      if [[ $job != unknown-project-orientation ]]; then
        case "$job" in
          interaction-loop) map_dir=MagiP12; map_name=P12Interaction ;;
          ui-state-loop) map_dir=MagiP13; map_name=P13UIState ;;
          ai-navigation-loop) map_dir=MagiP14; map_name=P14AI ;;
          animation-state-loop) map_dir=MagiP15; map_name=P15Animation ;;
        esac
        cat >"$directory/project/Config/DefaultEngine.ini" <<EOF
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/$map_dir/$map_name
EditorStartupMap=/Game/$map_dir/$map_name
GlobalDefaultGameMode=/Script/Engine.GameModeBase
EOF
        cat >"$directory/project/Config/DefaultGame.ini" <<EOF
[/Script/UnrealEd.ProjectPackagingSettings]
+MapsToCook=(FilePath="/Game/$map_dir/$map_name")
+DirectoriesToAlwaysCook=(Path="/Game/$map_dir")
+DirectoriesToAlwaysCook=(Path="/Game/MagiP15Seed")
UsePakFile=True
bUseIoStore=True
bUseZenStore=False
EOF
      fi
      if [[ $job != unknown-project-orientation ]]; then
        project="$directory/project/MagiUnrealAXIPackageFixture.uproject"
        HOME="$directory/agent-home" "$exact" --project "$project" --engine "$engine_root" --format json setup plugin install >"$directory/plugin-install.json"
        plugin=$(find "$directory/project/Plugins/MagiUnrealAXI" -type f -name libUnrealEditor-MagiUnrealAXI.dylib -print -quit)
        [[ -n $plugin ]] || fail "plugin missing: $job"
        arches=$(lipo -archs "$plugin"); grep -qw arm64 <<<"$arches" || fail "plugin lacks arm64: $job"
        plugin_hash=$(hash "$plugin")
        [[ $plugin_hash == "$combined_plugin" ]] || fail "job plugin differs from combined core: $job"
        printf '%s\n' "$plugin_hash" >"$directory/plugin.sha256"
        plugin_inventory "$directory/project/Plugins/MagiUnrealAXI" "$directory/plugin-tree.sha256"
      fi
      write_prompt "$directory" "$job"; write_wrapper "$directory" "$exact" "$directory/ledger.jsonl"
      for immutable in PROMPT.txt bin/axi-record plugin-install.json plugin.sha256 seed-source.txt seed-destination.txt plugin-tree.sha256 project/Config/DefaultEngine.ini project/Config/DefaultGame.ini; do [[ ! -f "$directory/$immutable" ]] || printf '%s\t%s\n' "$immutable" "$(hash "$directory/$immutable")"; done >"$directory/immutable.sha256"
    done
    immutable_hashes=$(for job in "${JOBS[@]}"; do printf '%s\t%s\n' "$job" "$(hash "$run/jobs/$job/immutable.sha256")"; done | jq -Rn 'reduce inputs as $line ({}; ($line | split("\t")) as $parts | .[$parts[0]] = $parts[1])')
    jq -n --arg run "$run" --arg artifact "$trusted" --arg binary "$(hash "$exact")" --arg plugin "$combined_plugin" --arg combined "$combined" --arg engineRoot "$engine_root" --arg sourceCommit "$P16_EXPECTED_SOURCE_COMMIT" --arg sourceTree "$(git -C "$repo_root" rev-parse HEAD^{tree})" --arg evidenceTree "$combined_tree" --arg provenance "$combined_provenance" --argjson immutable "$immutable_hashes" '{phase:"P1.6",status:"prepared",run:$run,artifactSha256:$artifact,binarySha256:$binary,pluginSha256:$plugin,combinedEvidence:$combined,combinedEvidenceTreeSha256:$evidenceTree,combinedProvenanceSha256:$provenance,engineRoot:$engineRoot,sourceCommit:$sourceCommit,sourceTree:$sourceTree,immutableSha256s:$immutable,jobs:["unknown-project-orientation","interaction-loop","ui-state-loop","ai-navigation-loop","animation-state-loop"]}' >"$run/manifest.json"
    printf '%s\n' "$run"
    ;;
  job-context)
    [[ $# == 2 ]] || { usage; exit 2; }; run=$(run_dir "$1"); job=$2; is_job "$job" || fail "unknown job"
    directory="$run/jobs/$job"; [[ -f "$directory/PROMPT.txt" && -x "$directory/bin/axi-record" ]] || fail "job not prepared"
    cat "$directory/PROMPT.txt"
    ;;
  # record/finalize revalidate exact per-job plugin produced by staged CLI installation.
  record)
    [[ $# == 3 ]] || { usage; exit 2; }; run=$(run_dir "$1"); job=$2; session=$3; is_job "$job" || fail "unknown job"
    directory="$run/jobs/$job"; [[ -d $directory && -f $session && ! -e "$directory/record.json" ]] || fail "job/session missing or already recorded"
    index=$(job_index "$job"); if ((index>1)); then previous=${JOBS[$((index-2))]}; [[ -f "$run/jobs/$previous/record.json" ]] || fail "jobs must record sequentially"; fi
    cp -p "$session" "$directory/session.jsonl"
    session_valid "$directory/session.jsonl" "$directory/project" "$directory/bin/axi-record" || fail "session/ledger correlation: $job"
    [[ -f "$directory/agent-outcome.json" ]] || fail "agent outcome missing"
    validate_outcome "$directory" "$job" || fail "job outcome evidence: $job"
    [[ $job == unknown-project-orientation ]] || plugin_valid "$directory" || fail "job plugin identity: $job"
    [[ $job == unknown-project-orientation || $(cat "$directory/plugin.sha256") == $(jq -r .pluginSha256 "$run/manifest.json") ]] || fail "job plugin differs from combined core: $job"
    [[ $(hash "$directory/immutable.sha256") == $(jq -r --arg job "$job" '.immutableSha256s[$job]' "$run/manifest.json") ]] || fail "immutable inventory seal changed: $job"
    while IFS=$'\t' read -r immutable expected; do [[ $(hash "$directory/$immutable") == "$expected" ]] || fail "immutable input changed: $job/$immutable"; done <"$directory/immutable.sha256"
    started=$(jq -sr 'map(.started)|min' "$directory/ledger.jsonl"); ended=$(jq -sr 'map(.ended)|max' "$directory/ledger.jsonl")
    if ((index>1)); then previous=${JOBS[$((index-2))]}; previous_end=$(jq -r .ended "$run/jobs/$previous/record.json"); [[ $started > $previous_end ]] || fail "job overlap/order"; fi
    jq -n --arg job "$job" --arg session "$(hash "$directory/session.jsonl")" --arg started "$started" --arg ended "$ended" '{job:$job,sessionSha256:$session,started:$started,ended:$ended,recordedAt:(now|todateiso8601)}' >"$directory/record.json"
    ;;
  finalize)
    [[ $# == 1 ]] || { usage; exit 2; }; run=$(run_dir "$1")
    latest_run=; [[ ! -f "$cache_root/latest" ]] || latest_run=$(cat "$cache_root/latest")
    [[ $latest_run != "$run" ]] || fail "run already finalized"
    if [[ -e "$run/summary.txt" || -e "$run/run-inventory.json" ]]; then trash "$run/summary.txt" "$run/run-inventory.json" >/dev/null 2>&1 || fail "incomplete finalization cleanup"; fi
    for stale in "$run"/.summary.*; do [[ ! -e "$stale" ]] || trash "$stale" >/dev/null 2>&1 || fail "stale summary cleanup"; done
    [[ -f "$run/manifest.json" ]] || fail "run missing"
    [[ $(hash "$(cat "$run/exact-binary")") == $(cat "$run/exact-binary.sha256") ]] || fail "exact binary changed"
    [[ $(jq -r .artifactSha256 "$run/manifest.json") == $(cat "$run/artifact.sha256") && $(jq -r .pluginSha256 "$run/manifest.json") == $(cat "$run/plugin.sha256") ]] || fail "identity mismatch"
    previous_record_end=
    for job in "${JOBS[@]}"; do
      directory="$run/jobs/$job"; [[ -f "$directory/record.json" && -f "$directory/session.jsonl" ]] || fail "job incomplete: $job"
      [[ $(hash "$directory/session.jsonl") == $(jq -r .sessionSha256 "$directory/record.json") ]] || fail "session changed: $job"
      session_valid "$directory/session.jsonl" "$directory/project" "$directory/bin/axi-record" || fail "session changed: $job"
      while IFS=$'\t' read -r immutable expected; do [[ $(hash "$directory/$immutable") == "$expected" ]] || fail "immutable input changed: $job/$immutable"; done <"$directory/immutable.sha256"
      previous_record_end=$(record_valid "$directory" "$job" "$previous_record_end") || fail "invalid record/order: $job"
      validate_outcome "$directory" "$job" || fail "outcome changed: $job"
      [[ $job == unknown-project-orientation ]] || plugin_valid "$directory" || fail "plugin changed: $job"
      [[ $job == unknown-project-orientation || $(cat "$directory/plugin.sha256") == $(jq -r .pluginSha256 "$run/manifest.json") ]] || fail "job plugin differs from combined core: $job"
      [[ $(hash "$directory/immutable.sha256") == $(jq -r --arg job "$job" '.immutableSha256s[$job]' "$run/manifest.json") ]] || fail "immutable inventory seal changed: $job"
    done
    cmp -s "$run/git-status.before" <(git -C "$repo_root" status --porcelain=v1 --untracked-files=all) || fail "repository state changed"
    if find "$run" -type f \( -name token -o -name bridge-v1.json \) -print -quit | grep -q .; then fail "runtime secret file retained"; fi
    set +e; grep -R -I -E -q 'Authorization:[[:space:]]*Bearer[[:space:]]+[A-Za-z0-9._-]+' "$run"; leaked=$?; set -e; [[ $leaked == 1 ]] || fail "bearer credential retained"
    metrics=$(jq -c -s --slurpfile outcomes <(jq -s '.' "$run"/jobs/*/agent-outcome.json) '{cliCalls:length,stdoutBytes:(map(.stdoutBytes)|add),stderrBytes:(map(.stderrBytes)|add),estimatedTokens:(map(.estimatedTokens)|add),retries:($outcomes[0]|map(.metrics.retries)|add),avoidableRetries:($outcomes[0]|map(.metrics.avoidableRetries)|add),structuredOutputFailures:($outcomes[0]|map(.metrics.structuredOutputFailures)|add)}' "$run"/jobs/*/ledger.jsonl)
    agent_plugins=$(for job in interaction-loop ui-state-loop ai-navigation-loop animation-state-loop; do printf '%s:%s,' "$job" "$(cat "$run/jobs/$job/plugin.sha256")"; done); agent_plugins=${agent_plugins%,}
    summary_tmp=$(mktemp "$run/.summary.XXXXXX")
    printf 'phase=P1.6\nstatus=passed\nartifactSha256=%s\nbinarySha256=%s\ncombinedPluginSha256=%s\nagentPluginSha256s=%s\ncombinedEvidence=%s\ncombinedEvidenceTreeSha256=%s\ncombinedProvenanceSha256=%s\nsourceCommit=%s\nsourceTree=%s\njobs=5/5-passed-sequential\nmetrics=%s\ntokenScan=passed\n' "$(cat "$run/artifact.sha256")" "$(cat "$run/exact-binary.sha256")" "$(cat "$run/plugin.sha256")" "$agent_plugins" "$(jq -r .combinedEvidence "$run/manifest.json")" "$(jq -r .combinedEvidenceTreeSha256 "$run/manifest.json")" "$(jq -r .combinedProvenanceSha256 "$run/manifest.json")" "$(jq -r .sourceCommit "$run/manifest.json")" "$(jq -r .sourceTree "$run/manifest.json")" "$metrics" >"$summary_tmp"
    [[ -e "$run/run-inventory.json" ]] && fail "run inventory already exists"
    mv "$summary_tmp" "$run/summary.txt"
    set +e; inventory_result=$(ruby "$repo_root/tests/unreal/support/p16-run-inventory.rb" write "$run"); inventory_status=$?; set -e
    if ((inventory_status != 0)); then trash "$run/summary.txt" "$run/run-inventory.json" >/dev/null 2>&1 || true; fail "run inventory write"; fi
    inventory_sha=$(sed -n 's/^inventorySha256=\([0-9a-f]\{64\}\) treeSha256=.*/\1/p' <<<"$inventory_result")
    if [[ ! $inventory_sha =~ ^[0-9a-f]{64}$ ]]; then trash "$run/summary.txt" "$run/run-inventory.json" >/dev/null 2>&1 || true; fail "run inventory result"; fi
    set +e; ruby "$repo_root/tests/unreal/support/p16-run-inventory.rb" verify "$run" "$inventory_sha" >/dev/null; inventory_status=$?; set -e
    if ((inventory_status != 0)); then trash "$run/summary.txt" "$run/run-inventory.json" >/dev/null 2>&1 || true; fail "run inventory verification"; fi
    latest_tmp=$(mktemp "$cache_root/latest.XXXXXX"); printf '%s\n' "$run" >"$latest_tmp"; mv -f "$latest_tmp" "$cache_root/latest"
    cat "$run/summary.txt"
    printf '%s\n' "$inventory_result"
    ;;
  revalidate)
    [[ $# == 2 ]] || { usage; exit 2; }
    [[ ${P16_EXPECTED_RUN_INVENTORY_SHA256:-} =~ ^[0-9a-f]{64}$ ]] || fail "P16_EXPECTED_RUN_INVENTORY_SHA256 required"
    run=$(cd "$(dirname "$1")" && pwd -P)/$(basename "$1")
    combined=$(cd "$2" && pwd -P)
    [[ -d "$run" && ! -L "$run" ]] || fail "downloaded run missing"
    [[ -d "$combined" && ! -L "$combined" ]] || fail "downloaded combined evidence missing"
    ruby "$repo_root/tests/unreal/support/p16-revalidate.rb" "$run" "$combined" "$P16_EXPECTED_RUN_INVENTORY_SHA256"
    ;;
  help|--help) usage ;;
  *) usage; exit 2 ;;
esac
