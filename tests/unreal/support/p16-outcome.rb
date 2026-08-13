#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "pathname"

class OutcomeValidator
  CATEGORIES = %w[orientation compile save restart runtime noop package].freeze
  JOBS = %w[unknown-project-orientation interaction-loop ui-state-loop ai-navigation-loop animation-state-loop].freeze
  GLOBAL_VALUE_FLAGS = %w[--project --engine --format --timeout --editor].freeze
  MAX_STDOUT_BYTES = 64 * 1024 * 1024
  MUTATION_TARGET = {
    "blueprint.interface_ensure" => "blueprintId", "blueprint.scs_component_ensure" => "blueprintId",
    "blueprint.scs_component_update" => "blueprintId", "blueprint.event_ensure" => "blueprintId",
    "blueprint.node_ensure" => "blueprintId", "blueprint.pin_default_set" => "blueprintId",
    "blueprint.pin_connect" => "blueprintId", "widget.child_ensure" => "blueprintId",
    "widget.property_set" => "blueprintId", "widget.event_ensure" => "blueprintId",
    "widget.viewport_ensure" => "hostBlueprintId", "blackboard.key_ensure" => "blackboardId",
    "behavior_tree.node_ensure" => "behaviorTreeId", "behavior_tree.connect" => "behaviorTreeId",
    "ai.controller_configure" => "blueprintId", "ai.pawn_configure" => "blueprintId",
    "animation.variable_ensure" => "animationBlueprintId", "animation.state_machine_ensure" => "animationBlueprintId",
    "animation.state_ensure" => "animationBlueprintId", "animation.transition_ensure" => "animationBlueprintId",
    "animation.character_configure" => "characterBlueprintId"
  }.freeze
  MUTATIONS = (MUTATION_TARGET.keys + %w[blueprint.create blueprint.interface_create widget.create blackboard.create behavior_tree.create animation_blueprint.create navigation.bounds_ensure actor.spawn level.create level.set_game_mode]).freeze
  P14_FIXTURE = JSON.parse(File.read(File.join(__dir__, "..", "p1.4-manifest.json"))).fetch("fixture").freeze
  P15_FIXTURE = JSON.parse(File.read(File.join(__dir__, "..", "p1.5-manifest.json"))).fetch("fixture").freeze
  P13_FIXTURE = JSON.parse(File.read(File.join(__dir__, "..", "p1.3-manifest.json"))).fetch("fixture").freeze
  P14_ASSETS = {
    bb: "/Game/MagiP15/BB_P16AnimationAI.BB_P16AnimationAI", tree: "/Game/MagiP15/BT_P16AnimationAI.BT_P16AnimationAI",
    controller: "/Game/MagiP15/BP_P16AnimationController.BP_P16AnimationController", floor: "/Game/MagiP15/BP_P16AnimationFloor.BP_P16AnimationFloor"
  }.freeze
  P15_ASSETS = {animation_blueprint: "/Game/MagiP15/ABP_P15Animation.ABP_P15Animation", character: "/Game/MagiP15/BP_P15Character.BP_P15Character"}.freeze
  CATALOG = JSON.parse(File.read(File.join(__dir__, "..", "..", "..", "capabilities", "catalog.json"))).to_h { |entry| [entry.fetch("id"), entry] }.freeze
  DESTRUCTIVE = CATALOG.values.select { |entry| entry["destructive"] == true }.map { |entry| entry["id"] }.freeze
  REQUIRED_OPS = {
    "interaction-loop" => %w[blueprint.interface_create blueprint.interface_ensure blueprint.scs_component_ensure blueprint.scs_component_update blueprint.graph_view blueprint.interface_view blueprint.scs_view blueprint.event_ensure blueprint.node_ensure blueprint.pin_default_set blueprint.pin_connect blueprint.create blueprint.view blueprint.compile asset.save level.create level.save level.open actor.spawn actor.view play.start play.input play.component_observe play.stop project.cook project.package],
    "ui-state-loop" => %w[widget.create widget.child_ensure widget.property_set widget.event_ensure widget.viewport_ensure widget.tree_view blueprint.create blueprint.view blueprint.compile asset.save level.create level.save level.open actor.spawn actor.view play.start play.input play.ui_observe play.stop project.cook project.package],
    "ai-navigation-loop" => %w[blackboard.create blackboard.key_ensure blackboard.view behavior_tree.create behavior_tree.node_ensure behavior_tree.connect behavior_tree.view blueprint.create blueprint.scs_component_ensure blueprint.scs_component_update blueprint.scs_view ai.controller_configure ai.pawn_configure blueprint.view blueprint.compile asset.save level.create level.save level.open actor.spawn actor.view navigation.bounds_ensure navigation.build navigation.status navigation.path_query play.start play.ai_target_set play.ai_observe play.stop project.cook project.package],
    "animation-state-loop" => %w[blackboard.create blackboard.key_ensure blackboard.view behavior_tree.create behavior_tree.node_ensure behavior_tree.connect behavior_tree.view blueprint.create blueprint.scs_component_ensure blueprint.scs_component_update blueprint.scs_view ai.controller_configure ai.pawn_configure navigation.bounds_ensure navigation.build navigation.status navigation.path_query animation_blueprint.create animation.variable_ensure animation.state_machine_ensure animation.state_ensure animation.transition_ensure animation.character_configure animation.graph_view animation.character_view blueprint.view blueprint.compile asset.save level.create level.save level.open actor.spawn actor.view play.start play.ai_target_set play.ai_observe play.animation_observe play.stop project.cook project.package]
  }.freeze
  OPERATION_COUNTS = {
    "interaction-loop" => {"actor.spawn"=>2,"actor.view"=>2,"asset.save"=>3,"blueprint.compile"=>2,"blueprint.create"=>2,"blueprint.event_ensure"=>4,"blueprint.graph_view"=>6,"blueprint.interface_create"=>1,"blueprint.interface_ensure"=>3,"blueprint.interface_view"=>1,"blueprint.node_ensure"=>7,"blueprint.pin_connect"=>8,"blueprint.pin_default_set"=>7,"blueprint.scs_component_ensure"=>2,"blueprint.scs_component_update"=>2,"blueprint.scs_view"=>2,"blueprint.view"=>2,"editor.start"=>2,"editor.status"=>2,"editor.stop"=>2,"level.create"=>1,"level.open"=>1,"level.save"=>1,"operation.view"=>9,"play.component_observe"=>8,"play.input"=>2,"play.start"=>2,"play.stop"=>2,"project.cook"=>1,"project.package"=>1},
    "ui-state-loop" => {"actor.spawn"=>1,"actor.view"=>1,"asset.save"=>2,"blueprint.compile"=>2,"blueprint.create"=>1,"blueprint.view"=>1,"editor.start"=>2,"editor.status"=>2,"editor.stop"=>2,"level.create"=>1,"level.open"=>1,"level.save"=>1,"operation.view"=>9,"play.input"=>2,"play.start"=>2,"play.stop"=>2,"play.ui_observe"=>4,"project.cook"=>1,"project.package"=>1,"widget.child_ensure"=>1,"widget.create"=>1,"widget.event_ensure"=>2,"widget.property_set"=>1,"widget.tree_view"=>1,"widget.viewport_ensure"=>1},
    "ai-navigation-loop" => {"actor.spawn"=>3,"actor.view"=>3,"ai.controller_configure"=>1,"ai.pawn_configure"=>1,"asset.save"=>5,"behavior_tree.connect"=>3,"behavior_tree.create"=>1,"behavior_tree.node_ensure"=>3,"behavior_tree.view"=>1,"blackboard.create"=>1,"blackboard.key_ensure"=>2,"blackboard.view"=>1,"blueprint.compile"=>3,"blueprint.create"=>3,"blueprint.scs_component_ensure"=>1,"blueprint.scs_component_update"=>1,"blueprint.scs_view"=>1,"blueprint.view"=>3,"editor.start"=>2,"editor.status"=>2,"editor.stop"=>2,"level.create"=>1,"level.open"=>1,"level.save"=>1,"navigation.bounds_ensure"=>1,"navigation.build"=>1,"navigation.path_query"=>1,"navigation.status"=>1,"operation.view"=>9,"play.ai_observe"=>3,"play.ai_target_set"=>1,"play.start"=>2,"play.stop"=>2,"project.cook"=>1,"project.package"=>1},
    "animation-state-loop" => {"actor.spawn"=>3,"actor.view"=>3,"ai.controller_configure"=>1,"ai.pawn_configure"=>1,"animation.character_configure"=>1,"animation.character_view"=>1,"animation.graph_view"=>1,"animation.state_ensure"=>2,"animation.state_machine_ensure"=>1,"animation.transition_ensure"=>2,"animation.variable_ensure"=>2,"animation_blueprint.create"=>1,"asset.save"=>6,"behavior_tree.connect"=>3,"behavior_tree.create"=>1,"behavior_tree.node_ensure"=>3,"behavior_tree.view"=>1,"blackboard.create"=>1,"blackboard.key_ensure"=>1,"blackboard.view"=>1,"blueprint.compile"=>4,"blueprint.create"=>3,"blueprint.scs_component_ensure"=>1,"blueprint.scs_component_update"=>1,"blueprint.scs_view"=>1,"blueprint.view"=>3,"editor.start"=>2,"editor.status"=>2,"editor.stop"=>2,"level.create"=>1,"level.open"=>1,"level.save"=>1,"navigation.bounds_ensure"=>1,"navigation.build"=>1,"navigation.path_query"=>1,"navigation.status"=>1,"operation.view"=>9,"play.ai_observe"=>4,"play.ai_target_set"=>2,"play.animation_observe"=>6,"play.start"=>2,"play.stop"=>2,"project.cook"=>1,"project.package"=>1}
  }.freeze
  DURABLE_VIEW_SELECTION = {
    "interaction-loop" => [{operation: "blueprint.interface_create"}, {operation: "blueprint.compile", target: "/Game/MagiP12/BP_Interactable.BP_Interactable"}, {operation: "asset.save", target: "/Game/MagiP12/BPI_Interact.BPI_Interact"}, {operation: "level.save"}, {operation: "level.open"}, {operation: "blueprint.interface_ensure", last: true}, {operation: "play.stop", last: true}, {operation: "project.cook"}, {operation: "project.package"}],
    "ui-state-loop" => [{operation: "widget.create"}, {operation: "blueprint.compile", target: "/Game/MagiP13/WBP_UIState.WBP_UIState"}, {operation: "asset.save", target: "/Game/MagiP13/WBP_UIState.WBP_UIState"}, {operation: "level.save"}, {operation: "level.open"}, {operation: "widget.event_ensure", last: true}, {operation: "play.stop", last: true}, {operation: "project.cook"}, {operation: "project.package"}],
    "ai-navigation-loop" => [{operation: "blackboard.create"}, {operation: "blueprint.compile", target: "/Game/MagiP14/BP_P14AIController.BP_P14AIController"}, {operation: "asset.save", target: "/Game/MagiP14/BB_P14AI.BB_P14AI"}, {operation: "level.save"}, {operation: "level.open"}, {operation: "blackboard.key_ensure", last: true}, {operation: "play.stop", last: true}, {operation: "project.cook"}, {operation: "project.package"}],
    "animation-state-loop" => [{operation: "animation_blueprint.create"}, {operation: "blueprint.compile", target: P15_ASSETS[:animation_blueprint]}, {operation: "asset.save", target: P15_ASSETS[:animation_blueprint]}, {operation: "level.save"}, {operation: "level.open"}, {operation: "animation.variable_ensure", last: true}, {operation: "play.stop", last: true}, {operation: "project.cook"}, {operation: "project.package"}]
  }.freeze
  DURABLE_VIEW_CATEGORIES = %w[compile compile save save restart noop runtime package package].freeze
  SPECS = {
    "interaction-loop" => {level: "/Game/MagiP12/P12Interaction", assets: {"/Game/MagiP12/BPI_Interact.BPI_Interact" => false, "/Game/MagiP12/BP_Interactable.BP_Interactable" => true, "/Game/MagiP12/BP_Player.BP_Player" => true}},
    "ui-state-loop" => {level: "/Game/MagiP13/P13UIState", assets: {"/Game/MagiP13/WBP_UIState.WBP_UIState" => true, "/Game/MagiP13/BP_UIStateHost.BP_UIStateHost" => true}},
    "ai-navigation-loop" => {level: "/Game/MagiP14/P14AI", assets: {"/Game/MagiP14/BB_P14AI.BB_P14AI" => false, "/Game/MagiP14/BT_P14AI.BT_P14AI" => false, "/Game/MagiP14/BP_P14AIController.BP_P14AIController" => true, "/Game/MagiP14/BP_P14AIPawn.BP_P14AIPawn" => true, "/Game/MagiP14/BP_P14AIFloor.BP_P14AIFloor" => true}},
    "animation-state-loop" => {level: "/Game/MagiP15/P15Animation", assets: {P14_ASSETS[:bb] => false, P14_ASSETS[:tree] => false, P14_ASSETS[:controller] => true, P14_ASSETS[:floor] => true, P15_ASSETS[:animation_blueprint] => true, P15_ASSETS[:character] => true}}
  }.freeze

  def initialize(dir, job, repo_root, recorded_dir = nil)
    raise "unknown job" unless JOBS.include?(job)
    @dir, @recorded_dir, @job, @repo_root = File.expand_path(dir), File.expand_path(recorded_dir || dir), job, File.expand_path(repo_root)
    ledger = File.join(@dir, "ledger.jsonl")
    outcome = File.join(@dir, "agent-outcome.json")
    raise "invalid evidence" unless File.file?(ledger) && !File.symlink?(ledger) && File.file?(outcome) && !File.symlink?(outcome)
    @rows = File.readlines(ledger, chomp: true).each_with_index.map do |line, index|
      row = JSON.parse(line)
      raise "ledger sequence" unless row["sequence"] == index + 1
      row["output"] = parse_stdout(row)
      row["input"] = parse_input(row)
      row
    end
    raise "empty ledger" if @rows.empty?
    @outcome = JSON.parse(File.read(outcome))
  rescue Errno::ENOENT, JSON::ParserError
    raise "invalid evidence"
  end

  def validate
    validate_common
    validate_catalog
    validate_receipt_consistency
    if @job == "unknown-project-orientation"
      validate_orientation
      return true
    end
    @spec = SPECS.fetch(@job)
    @project = File.join(@recorded_dir, "project", "MagiUnrealAXIPackageFixture.uproject")
    locate_phases
    validate_operation_views
    refs = expected_references
    validate_references(refs)
    validate_required_operations
    validate_receipts
    validate_persistence
    validate_restart
    validate_noop
    validate_runtime
    validate_package
    validate_job
    true
  end

  private

  def parse_stdout(row)
    recorded_path = row["stdoutPath"]
    raise "stdout missing" unless recorded_path.is_a?(String) && Pathname.new(recorded_path).absolute?
    recorded_path = File.expand_path(recorded_path)
    recorded_prefix = "#{@recorded_dir}#{File::SEPARATOR}"
    raise "stdout missing" unless recorded_path.start_with?(recorded_prefix)
    path = File.expand_path(File.join(@dir, recorded_path.delete_prefix(recorded_prefix)))
    raise "stdout missing" unless path.start_with?("#{@dir}#{File::SEPARATOR}") && File.file?(path) && !File.symlink?(path)
    raise "stdout oversized" if File.size(path) > MAX_STDOUT_BYTES
    text = File.binread(path, MAX_STDOUT_BYTES + 1) || ""
    raise "stdout oversized" if text.bytesize > MAX_STDOUT_BYTES
    if row.fetch("argv", []).include?("--help")
      raise "help empty" if text.strip.empty?
      return text
    end
    value = JSON.parse(text)
    raise "stdout object" unless value.is_a?(Hash)
    value
  rescue JSON::ParserError
    raise "stdout object"
  end

  def parse_input(row)
    argv = row.fetch("argv")
    indexes = argv.each_index.select { |index| argv[index] == "--input-json" || argv[index].start_with?("--input-json=") }
    raise "duplicate --input-json" if indexes.length > 1
    row["_input_json_present"] = !indexes.empty?
    return {} if indexes.empty?
    index = indexes.first
    value = argv[index] == "--input-json" ? argv[index + 1] : argv[index].delete_prefix("--input-json=")
    raise "input object" unless value
    parsed = JSON.parse(value)
    raise "input object" unless parsed.is_a?(Hash)
    parsed
  rescue JSON::ParserError
    raise "input object"
  end

  def flag(row, name)
    argv = row.fetch("argv")
    values = []
    argv.each_with_index do |token, index|
      values << argv[index + 1] if token == name
      values << token.delete_prefix("#{name}=") if token.start_with?("#{name}=")
    end
    raise "duplicate #{name}" if values.length > 1
    values.first
  end

  def vector_flag(row, name)
    value = flag(row, name)
    return nil unless value
    parts = value.split(",")
    return nil unless parts.length == 3
    parts.map { |part| Float(part) }
  rescue ArgumentError
    nil
  end

  def distance(left, right)
    return Float::INFINITY unless left.is_a?(Array) && right.is_a?(Array) && left.length == 3 && right.length == 3
    Math.sqrt(left.zip(right).sum { |a, b| (a.to_f - b.to_f)**2 })
  end
  def command(row)
    argv = row.fetch("argv")
    result = []
    index = 0
    while index < argv.length
      token = argv[index]
      if GLOBAL_VALUE_FLAGS.include?(token)
        raise "missing global value" unless argv[index + 1]
        index += 2
      elsif GLOBAL_VALUE_FLAGS.any? { |name| token.start_with?("#{name}=") } || %w[--full --verbose].include?(token)
        index += 1
      else
        result << token
        index += 1
      end
    end
    result
  end

  def operation(row)
    return "help" if row["argv"].include?("--help")
    return row["argv"][2] if row["argv"][0, 2] == %w[capability execute]
    argv = command(row)
    if (index = argv.each_index.find { |i| argv[i, 2] == %w[capability execute] })
      argv[index + 2] || "unknown"
    elsif (index = argv.each_index.find { |i| argv[i, 3] == %w[setup plugin status] })
      "setup.plugin.status"
    else
      pairs = %w[project doctor engine view capability search capability describe editor start editor status editor stop blueprint compile blueprint view asset save level create level open level save level current level set-game-mode actor spawn actor view play start play status play input play stop project cook project package operation view]
      pairs.each_slice(2) do |a, b|
        return "#{a}.#{b}" if argv.each_cons(2).any? { |x, y| x == a && y == b }
      end
      "unknown"
    end
  end
  def catalog_operation(row)
    return "help" if row["argv"].include?("--help")
    argv = command(row)
    return argv[2] if argv[0, 2] == %w[capability execute]
    CATALOG.keys.find { |id| argv.each_cons(id.split(".").length).any? { |tokens| tokens == id.split(".") } }
  end

  def catalog_input(row, op)
    return row["input"] if row["_input_json_present"]
    argv = command(row)
    properties = CATALOG.fetch(op).fetch("inputSchema").fetch("properties", {})
    input = {}
    positionals = {
      "blueprint.compile" => {"id" => 2}, "blueprint.view" => {"id" => 2},
      "asset.save" => {"id" => 2}, "actor.view" => {"id" => 2},
      "play.input" => {"key" => 2}, "capability.search" => {"query" => 2},
      "capability.describe" => {"id" => 2}
    }
    positionals["operation.view"] = {"id" => 2}
    positionals.fetch(op, {}).each { |key, index| input[key] = argv[index] if argv[index] }
    flag_aliases = {"levelId" => "--level", "agentKey" => "--agent-key", "sessionId" => "--session-id"}
    properties.each_key do |key|
      flag_name = flag_aliases[key] || "--#{key.gsub(/([a-z0-9])([A-Z])/, '\\1-\\2').downcase}"
      value = flag(row, flag_name)
      next unless value
      schema = properties.fetch(key)
      types = Array(schema["type"])
      input[key] = if types.include?("array")
                     value.split(",").map { |part| Float(part) }
                   elsif types.include?("integer") && value.match?(/\A-?\d+\z/)
                     Integer(value)
                   elsif types.include?("number")
                     Float(value)
                   elsif types.include?("boolean")
                     raise "catalog input #{op}" unless %w[true false].include?(value)
                     value == "true"
                   else
                     value
                   end
    end
    input
  rescue ArgumentError
    raise "catalog input #{op}"
  end

  def validate_schema(schema, value, label)
    if schema["variants"]
      matched = schema["variants"].any? do |variant|
        begin
          validate_schema(variant, value, label)
          true
        rescue StandardError
          false
        end
      end
      raise "#{label} variants" unless matched
      return
    end
    types = Array(schema["type"])
    valid = types.any? { |type| {"object" => value.is_a?(Hash), "array" => value.is_a?(Array), "string" => value.is_a?(String), "integer" => value.is_a?(Integer), "number" => value.is_a?(Numeric), "boolean" => value == true || value == false, "null" => value.nil?}[type] }
    raise "#{label} type" unless valid
    return if value.nil?
    if value.is_a?(Hash)
      properties = schema.fetch("properties", {})
      raise "#{label} additionalProperties" if schema["additionalProperties"] == false && (value.keys - properties.keys).any?
      schema.fetch("required", []).each { |key| raise "#{label} required #{key}" unless value.key?(key) }
      properties.each { |key, child| validate_schema(child, value[key], "#{label}.#{key}") if value.key?(key) }
    elsif value.is_a?(Array)
      raise "#{label} minItems" if schema["minItems"] && value.length < schema["minItems"]
      raise "#{label} maxItems" if schema["maxItems"] && value.length > schema["maxItems"]
      value.each { |item| validate_schema(schema.fetch("items", {}), item, label) }
    elsif value.is_a?(String)
      raise "#{label} enum" if schema["enum"] && !schema["enum"].include?(value)
      raise "#{label} minLength" if schema["minLength"] && value.length < schema["minLength"]
      raise "#{label} maxLength" if schema["maxLength"] && value.length > schema["maxLength"]
    elsif value.is_a?(Numeric)
      raise "#{label} minimum" if schema["minimum"] && value < schema["minimum"]
      raise "#{label} maximum" if schema["maximum"] && value > schema["maximum"]
    end
    raise "#{label} enum" if schema["enum"] && !schema["enum"].include?(value)
  end

  def validate_catalog
    allowed = if @job == "unknown-project-orientation"
                %w[help project.doctor engine.view setup.plugin.status editor.status capability.search capability.describe]
              else
                REQUIRED_OPS.fetch(@job) + %w[editor.start editor.status editor.stop operation.view]
              end
    @rows.each do |row|
      op = operation(row)
      raise "unknown operation" if op == "unknown"
      next if op == "help"
      validate_argv(row, op)
      entry = CATALOG[op]
      raise "destructive operation #{op}" if entry && entry["destructive"]
      raise "unknown operation #{op}" unless allowed.include?(op)
      next unless entry
      validate_schema(entry.fetch("inputSchema"), catalog_input(row, op), "#{op} input")
      next if op == "operation.view"
      output = body(row)
      if op == "editor.status" && output.is_a?(Hash) && output.key?("editor")
        editor = output.fetch("editor")
        next if @job == "unknown-project-orientation" && %w[stopped stale].include?(editor["state"])
        output = {"state" => editor["state"], "projectId" => editor["projectId"], "editorPid" => editor["editorPid"] || editor["pid"], "levelId" => editor["levelId"] || editor["level"] || "", "pie" => editor["pie"], "dirtyPackageCount" => editor["dirtyPackageCount"] || editor["dirtyPackages"]&.length || 0}
      end
      validate_schema(entry.fetch("outputSchema"), output, "#{op} output")
    end
  end

  def validate_argv(row, op)
    return if row["_input_json_present"]
    argv = command(row)
    grammars = {"blueprint.compile" => [3, []], "blueprint.view" => [3, []], "asset.save" => [3, []], "actor.view" => [3, []], "capability.search" => [3, ["--limit"]], "capability.describe" => [3, []], "level.create" => [2, ["--path"]], "level.open" => [2, ["--path"]], "level.save" => [2, ["--path"]], "actor.spawn" => [2, %w[--level --class --agent-key --location]], "play.start" => [2, []], "play.input" => [3, %w[--session-id --event]], "play.stop" => [2, ["--session-id"]], "project.cook" => [2, ["--output"]], "project.package" => [2, ["--output"]], "operation.view" => [3, []]}
    return unless grammars.key?(op)
    positional, flags = grammars.fetch(op)
    raise "missing positional #{op}" if argv.length < positional
    index = positional
    while index < argv.length
      name = argv[index]
      raise "unknown flag #{name}" unless flags.include?(name)
      raise "missing flag value #{name}" unless argv[index + 1] && !argv[index + 1].start_with?("-")
      index += 2
    end
    required = {"level.create"=>%w[--path],"level.open"=>%w[--path],"level.save"=>%w[--path],"actor.spawn"=>%w[--level --class --agent-key],"play.input"=>%w[--session-id --event],"play.stop"=>%w[--session-id],"project.cook"=>%w[--output],"project.package"=>%w[--output],"operation.view"=>[]}.fetch(op, [])
    raise "missing flag #{(required - argv).first}" unless (required - argv).empty?
  end


  def validate_receipt_consistency
    receipt_schema = CATALOG.fetch("operation.view").fetch("outputSchema")
    @rows.each do |row|
      b, r = body(row), receipt(row)
      next if r.empty?
      validate_schema(receipt_schema, r, "receipt #{row['sequence']}")
      entry = CATALOG.fetch(operation(row))
      raise "receipt #{row['sequence']}" unless r.values_at("operation", "state", "transaction", "reversibility") == [operation(row), "completed", entry["transactionBehavior"], entry["reversibility"]]
      %w[changed revision].each do |key|
        next unless b.key?(key)
        raise "receipt #{row['sequence']}" unless r[key] == b[key]
      end
      %w[dirtyPackages savedPackages].each do |key|
        raise "receipt #{row['sequence']}" unless r[key] == b[key] if b.key?(key) && !b[key].empty?
      end
      expected_target = receipt_target(row)
      raise "receipt #{row['sequence']}" unless expected_target && r["target"] == expected_target && r.dig("verification", "target") == expected_target
      expected_readback = entry.dig("verification", "readback") || "operation.view"
      raise "receipt #{row['sequence']}" unless r.dig("verification", "readback") == expected_readback && r.dig("verification", "matched") == true
      expected_persistence = case entry["saveBehavior"]
                             when "none" then "unchanged"
                             else b["changed"] == false ? "unchanged" : (%w[asset.save level.save].include?(operation(row)) ? "saved" : "dirty")
                             end
      raise "receipt #{row['sequence']}" unless r["persistence"] == expected_persistence
    end
  end

  def receipt_target(row)
    op = operation(row); input = row["input"]; result = body(row)
    return "#{input['animationBlueprintId']}#variable:#{input['name']}" if op == "animation.variable_ensure"
    return "#{input['animationBlueprintId']}#state-machine:#{input['name']}" if op == "animation.state_machine_ensure"
    return "#{input['animationBlueprintId']}##{input['stateMachineId']}#state:#{input['name']}" if op == "animation.state_ensure"
    return [input["animationBlueprintId"], input["stateMachineId"], input["fromStateId"], input["toStateId"]].join("#") if op == "animation.transition_ensure"
    return "#{input['characterBlueprintId']}#animation-character" if op == "animation.character_configure"
    return "#{input['blueprintId']}##{input['interfaceId']}" if op == "blueprint.interface_ensure"
    return "#{input['blueprintId']}#scs-name:#{input['name']}" if op == "blueprint.scs_component_ensure"
    return "#{input['blueprintId']}#scs:#{input['variableGuid']}" if %w[blueprint.scs_component_update blueprint.scs_component_remove].include?(op)
    return [input["blueprintId"], input["graphId"], input["agentKey"]].join("#") if %w[blueprint.event_ensure blueprint.node_ensure].include?(op)
    return [input["blueprintId"], input["pinId"]].join("#") if op == "blueprint.pin_default_set"
    return [input["blueprintId"], input["sourcePinId"], input["targetPinId"]].join("#") if op == "blueprint.pin_connect"
    return "#{input['hostBlueprintId']}#viewport:#{input['agentKey']}" if op == "widget.viewport_ensure"
    return "#{input['blackboardId']}##{input['keyName']}" if op == "blackboard.key_ensure"
    return [input["sessionId"], input["pawnId"], input["keyName"], input["targetActorId"]].join("#") if op == "play.ai_target_set"
    return "#{input['blueprintId']}#ai-controller:#{input['behaviorTreeId']}" if op == "ai.controller_configure"
    return "#{input['blueprintId']}#ai-pawn" if op == "ai.pawn_configure"
    return "#{input['levelId']}#nav-bounds:#{input['agentKey']}" if op == "navigation.bounds_ensure"
    return [result["behaviorTreeId"], result["linkId"]].join("#") if op == "behavior_tree.connect"
    return [result["behaviorTreeId"], result["nodeId"]].join("#") if op == "behavior_tree.node_ensure"
    return [result["blueprintId"], result["widgetId"], result["property"]].join("#") if op == "widget.property_set"
    return [result["blueprintId"], result["eventId"]].join("#") if op == "widget.event_ensure"
    return [result["blueprintId"], result["widgetId"]].join("#") if op == "widget.child_ensure"
    fields = Array(CATALOG.dig(op, "verification", "targetFields"))
    values = fields.map { |field| result[field] || input[field] }.compact
    return values.join("#") unless values.empty? || op == "play.input"
    return [result["sessionId"], result["key"], result["event"]].join("#") if op == "play.input"
    target_for(row) || result["ticketId"] || result["level"] || direct_target(row)
  end

  def body(row)
    output = row["output"]
    output.is_a?(Hash) && output["result"].is_a?(Hash) ? output["result"] : output
  end

  def receipt(row)
    row["output"].is_a?(Hash) && row["output"]["receipt"].is_a?(Hash) ? row["output"]["receipt"] : {}
  end

  def changed?(row)
    body(row)["changed"] == true || receipt(row)["changed"] == true
  end

  def revision(row)
    body(row)["revision"]
  end

  def row_for(op, rows = @rows, &block)
    rows.find { |row| operation(row) == op && (!block || block.call(row)) }
  end

  def rows_for(op, rows = @rows)
    rows.select { |row| operation(row) == op }
  end

  def require_row(op, rows = @rows, &block)
    row_for(op, rows, &block) || raise("missing #{op}")
  end

  def validate_common
    keys = %w[expectedFailureSequences job metrics references requiredCategories status]
    raise "outcome schema" unless @outcome.keys.sort == keys.sort
    raise "outcome status" unless @outcome["job"] == @job && @outcome["status"] == "passed"
    raise "failures" unless @outcome["expectedFailureSequences"] == [] && @rows.all? { |row| row["exit"] == 0 }
    metrics = @outcome["metrics"]
    raise "metrics" unless metrics.is_a?(Hash) && metrics.keys.sort == %w[avoidableRetries retries structuredOutputFailures].sort && metrics.values.all? { |value| value.is_a?(Integer) && value.zero? }
  end

  def validate_orientation
    raise "orientation categories" unless @outcome["requiredCategories"] == ["orientation"]
    refs = CATEGORIES.to_h { |category| [category, category == "orientation" ? (1..@rows.length).to_a : []] }
    validate_references(refs)
    allowed = %w[help project.doctor engine.view setup.plugin.status editor.status capability.search capability.describe]
    raise "orientation operation" unless @rows.all? { |row| allowed.include?(operation(row)) }
    raise "orientation help" unless @rows.any? { |row| operation(row) == "help" }
    project = File.join(@recorded_dir, "project", "MagiUnrealAXIPackageFixture.uproject")
    doctor = require_row("project.doctor") { |row| body(row)["project"] == project && body(row).fetch("checks", []).any? { |check| check["name"] == "descriptor" && check["passed"] == true } }
    raise "doctor help" unless doctor["output"]["help"].is_a?(Array) && !doctor["output"]["help"].empty?
    require_row("engine.view") { |row| body(row)["version"] == "5.8.1" && body(row)["changelist"] == 56_057_345 }
    require_row("setup.plugin.status") { |row| body(row).dig("plugin", "path") == File.join(@recorded_dir, "project", "Plugins", "MagiUnrealAXI") && %w[installed managed compatible].all? { |key| [true, false].include?(body(row).dig("plugin", key)) } }
    require_row("editor.status") { |row| %w[stopped ready stale].include?(body(row).dig("editor", "state")) }
    require_row("capability.search") { |row| body(row)["scope"] == "generated capability catalog" && body(row)["total"] == 79 && body(row)["items"].is_a?(Array) && !body(row)["items"].empty? }
    true
  end

  def locate_phases
    @first_save_index = @rows.index { |row| %w[asset.save level.save].include?(operation(row)) }
    raise "missing save phase" unless @first_save_index
    save_rows = @rows.select { |row| %w[asset.save level.save].include?(operation(row)) }
    @last_save_sequence = save_rows.map { |row| row["sequence"] }.max
    @author_stop = @rows.find { |row| row["sequence"] > @last_save_sequence && operation(row) == "editor.stop" }
    raise "author stop" unless @author_stop
    @restart_start = @rows.find { |row| row["sequence"] > @author_stop["sequence"] && operation(row) == "editor.start" }
    @restart_status = @rows.find { |row| @restart_start && row["sequence"] > @restart_start["sequence"] && operation(row) == "editor.status" && body(row).dig("editor", "state") == "ready" }
    @level_open = @rows.find { |row| @restart_status && row["sequence"] > @restart_status["sequence"] && operation(row) == "level.open" && (flag(row, "--path") == @spec[:level] || body(row)["level"] == @spec[:level]) }
    raise "restart lifecycle" unless @restart_start && @restart_status && @level_open
    @first_runtime = @rows.find { |row| operation(row).start_with?("play.") || (%w[ai-navigation-loop animation-state-loop].include?(@job) && %w[navigation.build navigation.status navigation.path_query].include?(operation(row))) }
    raise "runtime phase" unless @first_runtime
    @final_stop = @rows.reverse.find { |row| operation(row) == "editor.stop" && row["sequence"] > @first_runtime["sequence"] }
    raise "final stop" unless @final_stop
    @cook = rows_for("project.cook").first
    @package = rows_for("project.package").first
    raise "pipeline count" unless rows_for("project.cook").length == 1 && rows_for("project.package").length == 1
  end

  def expected_references
    refs = CATEGORIES.to_h { |category| [category, []] }
    view_index = 0
    @rows.each do |row|
      op = operation(row)
      category = if op == "operation.view"
                   raise "operation.view count" if view_index >= DURABLE_VIEW_CATEGORIES.length
                   DURABLE_VIEW_CATEGORIES.fetch(view_index).tap { view_index += 1 }
                 elsif row.equal?(@final_stop) || %w[project.cook project.package].include?(op)
                   "package"
                 elsif row["sequence"] >= @first_runtime["sequence"] && row["sequence"] < @final_stop["sequence"] && (op.start_with?("play.") || %w[navigation.build navigation.status navigation.path_query].include?(op))
                   "runtime"
                 elsif row["sequence"] > @level_open["sequence"] && row["sequence"] < @first_runtime["sequence"] && (body(row)["changed"] == false || receipt(row)["persistence"] == "unchanged")
                   "noop"
                 elsif %w[asset.save level.save].include?(op)
                   "save"
                 elsif row["sequence"] >= @author_stop["sequence"] && row["sequence"] < @first_runtime["sequence"]
                   "restart"
                 elsif row["sequence"] < @author_stop["sequence"] && !%w[help unknown].include?(op)
                   "compile"
                 end
      refs[category] << row["sequence"] if category
    end
    refs
  end

  def validate_references(expected)
    submitted = @outcome["references"]
    raise "reference schema" unless submitted.is_a?(Hash) && submitted.keys.sort == CATEGORIES.sort
    CATEGORIES.each do |category|
      values = submitted[category]
      raise "reference values" unless values.is_a?(Array) && values == values.sort.uniq && values.all? { |value| value.is_a?(Integer) && value.between?(1, @rows.length) }
      raise "reference mismatch #{category}" unless values == expected[category]
    end
    flat = submitted.values.flatten
    raise "reference overlap" unless flat.uniq.length == flat.length
    raise "reference coverage" unless flat.sort == (1..@rows.length).to_a
  end

  def durable_source(selection)
    candidates = rows_for(selection.fetch(:operation))
    candidates = candidates.select do |row|
      target = selection[:target]
      target.nil? || direct_target(row) == target || body(row).values_at("id", "blueprintId", "blackboardId", "animationBlueprintId").include?(target)
    end
    selection[:last] ? candidates.last : candidates.first
  end

  def validate_operation_views
    return if @job == "unknown-project-orientation"
    views = rows_for("operation.view")
    selections = DURABLE_VIEW_SELECTION.fetch(@job)
    raise "operation.view count" unless views.length == selections.length
    ids = views.map { |row| catalog_input(row, "operation.view")["id"] }
    raise "operation.view ids" unless ids.all? { |id| id.is_a?(String) && !id.empty? } && ids.uniq.length == ids.length
    selections.zip(views).each do |selection, view|
      source = durable_source(selection)
      raise "operation.view source #{view['sequence']}" unless source && source["sequence"] < view["sequence"]
      process = %w[project.cook project.package].include?(operation(source))
      expected = process ? source.dig("output", "operation") : receipt(source)
      expected_id = process ? expected["id"] : expected["operationId"]
      raise "operation.view id #{view['sequence']}" unless catalog_input(view, "operation.view")["id"] == expected_id
      raise "operation.view order #{view['sequence']}" if process && view["sequence"] != source["sequence"] + 1
      raise "operation.view receipt #{view['sequence']}" unless view["output"] == expected
      raise "operation.view offline #{view['sequence']}" if operation(source) == "play.stop" && view["sequence"] <= @final_stop["sequence"]
    end
  end

  def validate_required_operations
    raise "required categories" unless @outcome["requiredCategories"] == %w[compile save restart runtime noop package]
    operations = @rows.map { |row| operation(row) }
    missing = REQUIRED_OPS.fetch(@job) - operations
    raise "missing operations #{missing.join(',')}" unless missing.empty?
    validate_operation_counts
  end

  def validate_operation_counts
    actual = @rows.each_with_object(Hash.new(0)) { |row, counts| counts[operation(row)] += 1 }
    expected = OPERATION_COUNTS.fetch(@job)
    mismatch = (actual.keys | expected.keys).find { |op| actual[op] != expected.fetch(op, 0) }
    raise "operation multiplicity #{mismatch}" if mismatch
  end

  def validate_receipts
    @rows.each do |row|
      entry = CATALOG[operation(row)]
      next unless entry && entry["mutates"] == true
      value = receipt(row)
      raise "receipt #{row['sequence']}" unless value["operation"] == operation(row) && value["state"] == "completed" && value.dig("verification", "matched") == true
      validate_schema(CATALOG.fetch("operation.view").fetch("outputSchema"), value, "receipt #{row['sequence']}")
    end
  end

  def target_for(row)
    op = operation(row)
    return body(row)["id"] if op == "blueprint.interface_create"
    return body(row)["blueprintId"] if %w[blueprint.create widget.create].include?(op)
    return body(row)["blackboardId"] if op == "blackboard.create"
    return body(row)["behaviorTreeId"] if op == "behavior_tree.create"
    return body(row)["animationBlueprintId"] if op == "animation_blueprint.create"
    return body(row)["sessionId"] || row["input"]["sessionId"] if %w[play.start play.input play.stop].include?(op)
    return row["input"]["pawnId"] if op == "play.ai_target_set"
    field = MUTATION_TARGET[op]
    field ? row["input"][field] : nil
  end

  def direct_target(row)
    argv = command(row)
    op = operation(row)
    pair = op == "blueprint.compile" ? %w[blueprint compile] : %w[asset save]
    index = argv.each_index.find { |i| argv[i, 2] == pair }
    index ? argv[index + 2] : nil
  end

  def validate_persistence
    @spec[:assets].each do |asset, compile_required|
      mutations = @rows.select { |row| row["sequence"] < @author_stop["sequence"] && changed?(row) && target_for(row) == asset }
      raise "asset not authored #{asset}" if mutations.empty?
      last_mutation = mutations.map { |row| row["sequence"] }.max
      compile = rows_for("blueprint.compile").select { |row| direct_target(row) == asset && row["sequence"] > last_mutation && row["sequence"] < @author_stop["sequence"] }.min_by { |row| row["sequence"] }
      if compile_required
        raise "compile #{asset}" unless compile && body(compile)["errorCount"] == 0 && body(compile)["warningCount"] == 0 && revision(compile).to_s.match?(/\A[0-9a-f]{64}\z/)
      else
        raise "unexpected compile #{asset}" if compile
      end
      prior = compile || mutations.max_by { |row| row["sequence"] }
      save = rows_for("asset.save").select { |row| direct_target(row) == asset && row["sequence"] > prior["sequence"] && row["sequence"] < @author_stop["sequence"] }.min_by { |row| row["sequence"] }
      prior = compile || mutations.max_by { |row| row["sequence"] }
      raise "save #{asset}" unless save && save["sequence"] > prior["sequence"] && revision(save) == revision(prior) && receipt(save)["persistence"] == "saved"
      raise "post-save mutation #{asset}" if @rows.any? { |row| row["sequence"] > save["sequence"] && row["sequence"] < @level_open["sequence"] && changed?(row) && target_for(row) == asset }
    end
    level_mutations = @rows.select do |row|
      next false unless changed?(row) && row["sequence"] < @author_stop["sequence"]
      op = operation(row)
      (op == "level.create" && (flag(row, "--path") == @spec[:level] || body(row)["level"] == @spec[:level])) ||
        (op == "actor.spawn" && flag(row, "--level") == @spec[:level]) ||
        (op == "navigation.bounds_ensure" && row["input"]["levelId"] == @spec[:level]) || op == "level.set_game_mode"
    end
    level_save = rows_for("level.save").find { |row| flag(row, "--path") == @spec[:level] || body(row)["level"] == @spec[:level] }
    raise "level persistence" unless level_save && !level_mutations.empty? && level_save["sequence"] > level_mutations.map { |row| row["sequence"] }.max
    raise "post-author mutation" if @rows.any? { |row| row["sequence"] > @author_stop["sequence"] && row["sequence"] < @first_runtime["sequence"] && changed?(row) && MUTATIONS.include?(operation(row)) && operation(row) != "level.open" }
  end

  def validate_restart
    raise "restart order" unless @last_save_sequence < @author_stop["sequence"] && @author_stop["sequence"] < @restart_start["sequence"] && @restart_start["sequence"] < @restart_status["sequence"] && @restart_status["sequence"] < @level_open["sequence"] && @level_open["sequence"] < @first_runtime["sequence"]
    raise "author stop state" unless body(@author_stop).dig("editor", "state") == "stopped"
    raise "restart start state" unless body(@restart_start).dig("editor", "state") == "ready"
    spawns = rows_for("actor.spawn").select { |row| flag(row, "--level") == @spec[:level] }
    raise "spawn actors" if spawns.empty?
    spawns.each do |spawn|
      id = body(spawn)["id"]
      raise "actor identity" unless id.is_a?(String) && row_for("actor.view") { |row| row["sequence"] > @level_open["sequence"] && row["sequence"] < @first_runtime["sequence"] && (flag(row, "--id") == id || body(row)["id"] == id) }
    end
  end

  def validate_noop
    noops = @rows.select { |row| row["sequence"] > @level_open["sequence"] && row["sequence"] < @first_runtime["sequence"] && (body(row)["changed"] == false || receipt(row)["changed"] == false) }
    expected = {"interaction-loop" => "blueprint.interface_ensure", "ui-state-loop" => "widget.event_ensure", "ai-navigation-loop" => "blackboard.key_ensure", "animation-state-loop" => "animation.variable_ensure"}.fetch(@job)
    noop = noops.find { |row| operation(row) == expected }
    raise "required noop" unless noop
    saved_target = target_for(noop)
    saved_row = rows_for("asset.save").find { |row| direct_target(row) == saved_target }
    raise "required noop" unless saved_row && !changed?(noop) && receipt(noop)["persistence"] == "unchanged" && revision(noop) == revision(saved_row) && receipt(noop)["revision"] == revision(noop)
  end

  def session_windows
    starts = rows_for("play.start")
    stops = rows_for("play.stop")
    raise "session count" unless starts.length == 2 && stops.length == 2
    ids = starts.map { |row| body(row)["sessionId"] }
    raise "session ids" unless ids.all? { |id| id.is_a?(String) && !id.empty? } && ids.uniq.length == 2
    raise "session order" unless starts[0]["sequence"] < stops[0]["sequence"] && stops[0]["sequence"] < starts[1]["sequence"] && starts[1]["sequence"] < stops[1]["sequence"]
    starts.each_with_index.map do |start, index|
      stop = stops[index]
      id = ids[index]
      stop_values = [stop["input"]["sessionId"], body(stop)["sessionId"], flag(stop, "--session-id")].compact
      raise "session stop" unless stop_values.length.positive? && stop_values.uniq == [id]
      [id, start["sequence"], stop["sequence"]]
    end.tap do |windows|
      @rows.each do |row|
        next unless operation(row).start_with?("play.") && !%w[play.start play.stop].include?(operation(row))
        values = [row["input"]["sessionId"], body(row)["sessionId"], flag(row, "--session-id")].compact
        raise "session evidence #{row['sequence']}" unless values.length.positive? && values.uniq.length == 1
        id, start_sequence, stop_sequence = windows.find { |window| window[0] == values.first }
        raise "session window #{row['sequence']}" unless id && row["sequence"] > start_sequence && row["sequence"] < stop_sequence
      end
    end
  end

  def validate_runtime
    windows = session_windows
    rows_for("play.input").each { |row| raise "input rejected" unless body(row)["accepted"] == true }
    windows
  end

  def validate_package
    raise "pipeline order" unless @final_stop["sequence"] < @cook["sequence"] && @cook["sequence"] < @package["sequence"]
    critical_after = @rows.select { |row| row["sequence"] > @package["sequence"] && operation(row) != "operation.view" }
    raise "post-package command" unless critical_after.empty?
    { @cook => ["cook", "cooked-output", "cooked"], @package => ["package", "package-output", "package"] }.each do |row, (kind, artifact_kind, relative)|
      recorded_root = File.join(@recorded_dir, relative)
      local_root = File.join(@dir, relative)
      operation_data = row["output"]["operation"]
      raise "pipeline status" unless operation_data.is_a?(Hash) && operation_data["kind"] == kind && operation_data["status"] == "passed" && operation_data["project"] == @project && operation_data["exitCode"] == 0
      raise "pipeline output" unless flag(row, "--output") == recorded_root
      artifacts = row["output"].fetch("artifacts", []).select { |artifact| artifact["kind"] == artifact_kind }
      raise "pipeline artifact" unless artifacts.length == 1 && artifacts[0]["exists"] == true && File.expand_path(artifacts[0]["path"]) == recorded_root && File.directory?(local_root)
    end
    raise "cooked registry" unless Dir.glob(File.join(@dir, "cooked", "**", "AssetRegistry.bin")).any? { |path| File.file?(path) }
    raise "package app" unless Dir.glob(File.join(@dir, "package", "**", "MagiUnrealAXIPackageFixture.app")).any? { |path| File.directory?(path) }
    raise "package utoc" unless Dir.glob(File.join(@dir, "package", "**", "MagiUnrealAXIPackageFixture-Mac.utoc")).any? { |path| File.file?(path) }
  end

  def validate_job
    case @job
    when "interaction-loop" then validate_interaction
    when "ui-state-loop" then validate_ui
    when "ai-navigation-loop" then validate_ai
    when "animation-state-loop" then validate_animation
    end
  end

  def validate_interaction
    interface_id = "/Game/MagiP12/BPI_Interact.BPI_Interact"
    target = "/Game/MagiP12/BP_Interactable.BP_Interactable"
    player = "/Game/MagiP12/BP_Player.BP_Player"
    interface = require_row("blueprint.interface_create") { |row| row["input"].values_at("path", "function") == ["/Game/MagiP12/BPI_Interact", "Interact"] && body(row)["id"] == interface_id }
    creates = rows_for("blueprint.create")
    target_create = require_row("blueprint.create", creates) { |row| body(row)["blueprintId"] == target && row["input"]["parentClass"] == "/Script/Engine.Actor" }
    player_create = require_row("blueprint.create", creates) { |row| body(row)["blueprintId"] == player && row["input"]["parentClass"] == "/Script/Engine.Actor" }
    [target, player].each { |id| require_row("blueprint.interface_ensure") { |row| row["input"].values_at("blueprintId", "interfaceId") == [id, interface_id] } }
    boxes = [[target, "OverlapBox"], [player, "InteractionBox"]].map do |id, name|
      ensured = require_row("blueprint.scs_component_ensure") { |row| row["input"].values_at("blueprintId", "name", "class") == [id, name, "BoxComponent"] }
      guid = body(ensured)["variableGuid"]
      require_row("blueprint.scs_component_update") { |row| row["input"].values_at("blueprintId", "variableGuid", "collisionEnabled", "collisionProfile", "generateOverlapEvents", "boxExtent") == [id, guid, "QueryOnly", "OverlapAllDynamic", true, [100, 100, 100]] }
      [id, guid, name]
    end
    boxes.each do |id, guid, name|
      require_row("blueprint.scs_view") { |row| row["sequence"] > @level_open["sequence"] && body(row)["blueprintId"] == id && body(row).fetch("components", []).any? { |component| component.values_at("variableGuid", "name", "collisionEnabled", "collisionProfile", "generateOverlapEvents", "boxExtent") == [guid, name, "QueryOnly", "OverlapAllDynamic", true, [100, 100, 100]] } }
      require_row("blueprint.view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("id", "errorCount", "warningCount") == [id, 0, 0] }
    end
    require_row("blueprint.interface_view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("id", "function", "revision") == [interface_id, "Interact", revision(rows_for("asset.save").find { |save| direct_target(save) == interface_id })] }

    graph_contract = lambda do |asset, intents, defaults, links, label|
      list = require_row("blueprint.graph_view") { |row| row["sequence"] < @author_stop["sequence"] && row["input"] == {"blueprintId" => asset} && body(row)["blueprintId"] == asset && body(row).fetch("items", []).any? { |item| item["kind"] == "ubergraph" } }
      graph_id = body(list).fetch("items").find { |item| item["kind"] == "ubergraph" }.fetch("graphId")
      nodes = {}
      intents.each do |key, op, intent, extra|
        mutation = require_row(op) { |row| row["sequence"] < @author_stop["sequence"] && row["input"].values_at("blueprintId", "graphId", op == "blueprint.event_ensure" ? "event" : "node") == [asset, graph_id, intent] && extra.all? { |field, value| row["input"][field] == value } }
        raise "interaction graph #{label}" unless body(mutation).values_at("blueprintId", "graphId") == [asset, graph_id]
        nodes[key] = body(mutation)["nodeId"]
      end
      node_view = require_row("blueprint.graph_view") { |row| row["sequence"] < @author_stop["sequence"] && row["input"].values_at("blueprintId", "graphId") == [asset, graph_id] && nodes.values.all? { |id| body(row).fetch("items", []).any? { |item| item["nodeId"] == id } } }
      pin = lambda do |node_key, name, direction|
        node = body(node_view).fetch("items").find { |item| item["nodeId"] == nodes.fetch(node_key) }
        value = node && node.fetch("pins", []).find { |item| item.values_at("name", "direction") == [name, direction] }
        value && value["pinId"]
      end
      defaults.each do |node_key, name, type, value|
        pin_id = pin.call(node_key, name, "input")
        raise "interaction graph #{label}" unless pin_id && require_row("blueprint.pin_default_set") { |row| row["sequence"] < @author_stop["sequence"] && row["input"] == {"blueprintId" => asset, "pinId" => pin_id, "value" => {"type" => type, "value" => value}} }
      end
      expected_links = links.map do |source_node, source_name, target_node, target_name|
        source = pin.call(source_node, source_name, "output")
        target_pin = pin.call(target_node, target_name, "input")
        raise "interaction graph #{label}" unless source && target_pin && require_row("blueprint.pin_connect") { |row| row["sequence"] < @author_stop["sequence"] && row["input"] == {"blueprintId" => asset, "sourcePinId" => source, "targetPinId" => target_pin} }
        [source, target_pin]
      end
      saved_revision = revision(rows_for("asset.save").find { |save| direct_target(save) == asset })
      restart = require_row("blueprint.graph_view") { |row| row["sequence"] > @level_open["sequence"] && row["sequence"] < @first_runtime["sequence"] && row["input"].values_at("blueprintId", "graphId") == [asset, graph_id] && body(row).values_at("blueprintId", "revision") == [asset, saved_revision] }
      restart_pins = body(restart).fetch("items", []).flat_map { |item| item.fetch("pins", []) }.to_h { |item| [item["pinId"], item] }
      defaults.each do |node_key, name, _type, value|
        item = restart_pins[pin.call(node_key, name, "input")]
        raise "interaction graph #{label}" unless item && Float(item["defaultValue"]) == value.to_f
      rescue ArgumentError, TypeError
        raise "interaction graph #{label}"
      end
      expected_links.each { |source, target_pin| raise "interaction graph #{label}" unless restart_pins[source]&.fetch("links", []) == [target_pin] && restart_pins[target_pin]&.fetch("links", []) == [source] }
    end

    graph_contract.call(target,
      [[:overlap, "blueprint.event_ensure", "component.begin_overlap", {"variableGuid" => boxes[0][1]}], [:message, "blueprint.node_ensure", "interface.message_interact", {"interfaceId" => interface_id}], [:begin, "blueprint.event_ensure", "actor.begin_play", {}], [:input, "blueprint.event_ensure", "input.key_e", {}], [:controller, "blueprint.node_ensure", "game.get_player_controller", {}], [:enable, "blueprint.node_ensure", "actor.enable_input", {}], [:vector, "blueprint.node_ensure", "math.make_vector", {}], [:offset, "blueprint.node_ensure", "actor.add_world_offset", {}]],
      [[:controller, "PlayerIndex", "integer", 0], [:vector, "X", "real", 200], [:vector, "Y", "real", 0], [:vector, "Z", "real", 0]],
      [[:overlap, "then", :message, "execute"], [:overlap, "OtherActor", :message, "self"], [:begin, "then", :enable, "execute"], [:controller, "ReturnValue", :enable, "PlayerController"], [:input, "Pressed", :offset, "execute"], [:vector, "ReturnValue", :offset, "DeltaLocation"]], "target")
    graph_contract.call(player,
      [[:interact, "blueprint.event_ensure", "interface.interact", {"interfaceId" => interface_id}], [:vector, "blueprint.node_ensure", "math.make_vector", {}], [:offset, "blueprint.node_ensure", "actor.add_world_offset", {}]],
      [[:vector, "X", "real", 100], [:vector, "Y", "real", 0], [:vector, "Z", "real", 0]],
      [[:interact, "then", :offset, "execute"], [:vector, "ReturnValue", :offset, "DeltaLocation"]], "player")

    actors = [[target_create, boxes[0]], [player_create, boxes[1]]].map do |create, box|
      spawn = require_row("actor.spawn") { |row| flag(row, "--class") == body(create)["generatedClass"] }
      [body(spawn)["id"], box[1], box[2]]
    end
    finals = rows_for("play.start").map do |start|
      sid = body(start)["sessionId"]
      per_actor = actors.map do |actor_id, guid, name|
        observations = rows_for("play.component_observe").select { |row| body(row).values_at("sessionId", "actorId", "variableGuid") == [sid, actor_id, guid] }
        before = observations.find { |row| body(row)["resolved"] == true && body(row)["overlapCount"] == 0 && body(row)["interactionDisplacement"] == [0, 0, 0] }
        input = rows_for("play.input").find { |row| before && row["sequence"] > before["sequence"] && body(row).values_at("sessionId", "key", "event", "accepted") == [sid, "E", "pressed", true] }
        after = observations.find { |row| input && row["sequence"] > input["sequence"] && body(row)["resolved"] == true && body(row)["interactionDisplacement"].to_a.any? { |value| value != 0 } }
        raise "interaction runtime #{name}" unless before && input && after
        body(after).slice("actorId", "variableGuid", "overlapCount", "overlappingActorIds", "interactionDisplacement")
      end
      raise "target overlap" unless per_actor[0]["overlapCount"].to_i > 0 && per_actor[0]["overlappingActorIds"].to_a.include?(actors[1][0])
      per_actor
    end
    raise "interaction deterministic" unless finals.uniq.length == 1
  end

  def validate_ui
    fixture = P13_FIXTURE
    widget = "/Game/MagiP13/WBP_UIState.WBP_UIState"
    host = "/Game/MagiP13/BP_UIStateHost.BP_UIStateHost"
    create = require_row("widget.create") { |row| row["input"].values_at("path", "rootName", "rootClass") == [fixture["widgetPath"], fixture.dig("root", "name"), fixture.dig("root", "class")] && body(row)["blueprintId"] == widget }
    root_id = body(create)["rootWidgetId"]
    child = require_row("widget.child_ensure") { |row| row["input"].values_at("blueprintId", "parentWidgetId", "name", "class") == [widget, root_id, fixture.dig("text", "name"), fixture.dig("text", "class")] }
    text_id = body(child)["widgetId"]
    action = {"kind" => "text.set", "targetWidgetId" => text_id, "text" => fixture.dig("text", "active")}
    event = require_row("widget.event_ensure") { |row| row["sequence"] < @author_stop["sequence"] && row["input"] == {"blueprintId" => widget, "agentKey" => fixture["agentKey"], "event" => "activate", "actions" => [action]} }
    host_create = require_row("blueprint.create") { |row| body(row)["blueprintId"] == host && row["input"].values_at("path", "parentClass") == [fixture["hostPath"], "/Script/Engine.Actor"] }
    require_row("widget.viewport_ensure") { |row| row["input"].values_at("hostBlueprintId", "widgetBlueprintId", "agentKey", "inputKey", "zOrder") == [host, widget, fixture["agentKey"], fixture["inputKey"], fixture["zOrder"]] }
    widget_revision = revision(rows_for("asset.save").find { |row| direct_target(row) == widget })
    host_revision = revision(rows_for("asset.save").find { |row| direct_target(row) == host })
    expected_widgets = [
      [root_id, fixture.dig("root", "name"), fixture.dig("root", "class"), nil, 0, nil, "Visible", true],
      [text_id, fixture.dig("text", "name"), fixture.dig("text", "class"), root_id, 1, fixture.dig("text", "ready"), "Visible", true]
    ]
    tree = require_row("widget.tree_view") { |row| row["sequence"] > @level_open["sequence"] && row["sequence"] < @first_runtime["sequence"] && row["input"] == {"blueprintId" => widget} }
    tree_body = body(tree)
    actual_widgets = tree_body.fetch("widgets", []).map { |item| item.values_at("widgetId", "name", "class", "parentWidgetId", "index", "text", "visibility", "enabled") }
    actual_events = tree_body.fetch("events", []).map { |item| item.values_at("eventId", "agentKey", "event", "actions") }
    raise "ui tree" unless tree_body.values_at("blueprintId", "generatedClass", "rootWidgetId", "count", "total", "scope", "revision") == [widget, body(create)["generatedClass"], root_id, 2, 2, widget, widget_revision] && actual_widgets == expected_widgets && actual_events == [[body(event)["eventId"], fixture["agentKey"], "activate", [action]]]
    host_view = require_row("blueprint.view") { |row| row["sequence"] > @level_open["sequence"] && row["sequence"] < @first_runtime["sequence"] && body(row)["id"] == host }
    raise "ui host" unless body(host_view).values_at("generatedClass", "revision", "errorCount", "warningCount") == [body(host_create)["generatedClass"], host_revision, 0, 0]
    require_row("actor.spawn") { |row| flag(row, "--class") == body(host_create)["generatedClass"] }
    instances = rows_for("play.start").map do |start|
      sid = body(start)["sessionId"]
      observations = rows_for("play.ui_observe").select { |row| body(row).values_at("sessionId", "widgetBlueprintId") == [sid, widget] }
      exact_widget = lambda { |row, text| body(row).fetch("widgets", []).map { |item| item.values_at("widgetId", "name", "class", "text", "visibility", "enabled") } == [[text_id, fixture.dig("text", "name"), fixture.dig("text", "class"), text, "Visible", true]] }
      ready = observations.find { |row| body(row)["inViewport"] == true && exact_widget.call(row, fixture.dig("text", "ready")) }
      input = rows_for("play.input").find { |row| ready && row["sequence"] > ready["sequence"] && body(row).values_at("sessionId", "key", "event", "accepted") == [sid, fixture["inputKey"], "pressed", true] && flag(row, "--session-id") == sid && flag(row, "--event") == "pressed" && command(row).include?(fixture["inputKey"]) }
      active = observations.find { |row| input && row["sequence"] > input["sequence"] && body(row)["instanceId"] == body(ready)["instanceId"] && exact_widget.call(row, fixture.dig("text", "active")) }
      raise "ui runtime" unless ready && input && active
      body(ready)["instanceId"]
    end
    raise "ui reset" unless instances.none?(&:nil?) && instances.uniq.length == 2
  end

  def validate_ai
    fixture = P14_FIXTURE
    bb, tree, controller, pawn, floor = @spec[:assets].keys
    require_row("blackboard.create") { |row| row["input"]["path"] == fixture["blackboard"] && body(row)["blackboardId"] == bb }
    require_row("blackboard.key_ensure") { |row| row["input"].values_at("blackboardId", "keyName", "keyType") == [bb, fixture.dig("blackboardKey", "name"), fixture.dig("blackboardKey", "keyType")] }
    bb_save = rows_for("asset.save").find { |row| direct_target(row) == bb }
    expected_key = {"keyName" => fixture.dig("blackboardKey", "name"), "keyType" => fixture.dig("blackboardKey", "keyType")}
    require_row("blackboard.view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("blackboardId", "keys", "revision") == [bb, [expected_key], revision(bb_save)] }
    require_row("behavior_tree.create") { |row| row["input"].values_at("path", "blackboardId") == [fixture["behaviorTree"], bb] && body(row)["behaviorTreeId"] == tree }
    nodes = [["loop", "sequence", nil, nil], ["move", "move_to", "TargetActor", nil], ["wait", "wait", nil, 0.5]]
    nodes.each { |id, type, key, wait| require_row("behavior_tree.node_ensure") { |row| row["input"].values_at("behaviorTreeId", "nodeId", "nodeType") == [tree, id, type] && body(row).values_at("keyName", "waitSeconds") == [key, wait] } }
    links = fixture["behaviorTreeLinks"].map { |link| link.values_at("parentNodeId", "childNodeId", "childIndex") }
    links.each { |link| require_row("behavior_tree.connect") { |row| row["input"].values_at("behaviorTreeId", "parentNodeId", "childNodeId", "childIndex") == [tree] + link } }
    tree_save = rows_for("asset.save").find { |row| direct_target(row) == tree }
    tree_view = require_row("behavior_tree.view") { |row| row["sequence"] > @level_open["sequence"] && body(row)["behaviorTreeId"] == tree }
    raise "ai tree" unless body(tree_view)["blackboardId"] == bb && body(tree_view)["revision"] == revision(tree_save) && body(tree_view).fetch("nodes", []).map { |node| node.values_at("nodeId", "nodeType", "keyName", "waitSeconds") }.sort_by(&:first) == nodes.sort_by(&:first) && body(tree_view).fetch("links", []).map { |link| link.values_at("parentNodeId", "childNodeId", "childIndex") }.sort == links.sort
    path = require_row("navigation.path_query") { |row| row["input"].values_at("levelId", "start", "target") == [@spec[:level], fixture["pawnLocation"], fixture["targetLocation"]] && body(row).values_at("levelId", "start", "target", "reachable", "partial") == [@spec[:level], fixture["pawnLocation"], fixture["targetLocation"], true, false] && body(row)["pathLength"].is_a?(Numeric) && body(row)["pathLength"] > 0 && body(row).fetch("points", []).values_at(0, -1) == [fixture["pawnLocation"], fixture["targetLocation"]] }
    creates = rows_for("blueprint.create")
    build = require_row("navigation.build") { |row| row["input"]["levelId"] == @spec[:level] }
    require_row("navigation.status") { |row| body(row).values_at("ticketId", "levelId", "state", "terminal") == [body(build)["ticketId"], @spec[:level], "succeeded", true] }
    controller_create = require_row("blueprint.create", creates) { |row| body(row)["blueprintId"] == controller && row["input"]["parentClass"] == "/Script/AIModule.AIController" }
    pawn_create = require_row("blueprint.create", creates) { |row| body(row)["blueprintId"] == pawn && row["input"]["parentClass"] == "/Script/Engine.Character" }
    floor_create = require_row("blueprint.create", creates) { |row| body(row)["blueprintId"] == floor && row["input"]["parentClass"] == "/Script/Engine.Actor" }
    require_row("ai.controller_configure") { |row| row["input"].values_at("blueprintId", "behaviorTreeId") == [controller, tree] }
    require_row("ai.pawn_configure") { |row| row["input"].values_at("blueprintId", "controllerBlueprintId") == [pawn, controller] }
    box = require_row("blueprint.scs_component_ensure") { |row| row["input"].values_at("blueprintId", "name", "class") == [floor, fixture.dig("floorComponent", "name"), fixture.dig("floorComponent", "class")] }
    require_row("blueprint.scs_component_update") { |row| row["input"].values_at("blueprintId", "variableGuid", "collisionEnabled", "collisionProfile", "boxExtent") == [floor, body(box)["variableGuid"], "QueryAndPhysics", fixture.dig("floorComponent", "collisionProfile"), [1000, 1000, 25]] }
    [controller_create, pawn_create, floor_create].each do |create|
      asset = body(create)["blueprintId"]
      save = rows_for("asset.save").find { |row| direct_target(row) == asset }
      require_row("blueprint.view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("id", "generatedClass", "revision", "errorCount", "warningCount") == [asset, body(create)["generatedClass"], revision(save), 0, 0] }
    end
    require_row("blueprint.scs_view") { |row| row["sequence"] > @level_open["sequence"] && body(row)["blueprintId"] == floor && body(row).fetch("components", []).any? { |item| item.values_at("variableGuid", "name", "collisionEnabled", "collisionProfile", "boxExtent") == [body(box)["variableGuid"], fixture.dig("floorComponent", "name"), "QueryAndPhysics", fixture.dig("floorComponent", "collisionProfile"), [1000, 1000, 25]] } }
    floor_spawn = require_row("actor.spawn") { |row| flag(row, "--class") == body(floor_create)["generatedClass"] && vector_flag(row, "--location") == [0.0, 0.0, -25.0] }
    pawn_spawn = require_row("actor.spawn") { |row| flag(row, "--class") == body(pawn_create)["generatedClass"] && vector_flag(row, "--location") == fixture["pawnLocation"].map(&:to_f) }
    target_spawn = require_row("actor.spawn") { |row| flag(row, "--class") == "/Script/Engine.TargetPoint" && vector_flag(row, "--location") == fixture["targetLocation"].map(&:to_f) }
    [[floor_spawn, body(floor_create)["generatedClass"], [0, 0, -25]], [pawn_spawn, body(pawn_create)["generatedClass"], fixture["pawnLocation"]], [target_spawn, "/Script/Engine.TargetPoint", fixture["targetLocation"]]].each do |spawn, klass, location|
      require_row("actor.view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("id", "class", "location") == [body(spawn)["id"], klass, location] }
    end
    windows = session_windows
    sid1, sid2 = windows.map(&:first)
    pawn_id, target_id = body(pawn_spawn)["id"], body(target_spawn)["id"]
    set = require_row("play.ai_target_set") { |row| row["sequence"] > windows[0][1] && row["sequence"] < windows[0][2] && body(row).values_at("sessionId", "pawnId", "keyName", "targetActorId", "targetLocation", "changed", "restarted") == [sid1, pawn_id, "TargetActor", target_id, fixture["targetLocation"], true, true] }
    observations = rows_for("play.ai_observe")
    moving = observations.find { |row| row["sequence"] > set["sequence"] && row["sequence"] < windows[0][2] && body(row).values_at("sessionId", "pawnId", "targetActorId", "targetLocation", "destination", "moveStatus", "behaviorTreeId", "possessed", "behavior") == [sid1, pawn_id, target_id, fixture["targetLocation"], fixture["targetLocation"], "moving", tree, true, "running"] && body(row).fetch("activeNodeIds", []).include?("move") }
    reached = observations.find { |row| moving && row["sequence"] > moving["sequence"] && row["sequence"] < windows[0][2] && body(row).values_at("sessionId", "pawnId", "targetActorId", "targetLocation", "destination", "moveStatus", "behaviorTreeId", "possessed", "behavior") == [sid1, pawn_id, target_id, fixture["targetLocation"], nil, "reached", tree, true, "running"] && body(row).fetch("activeNodeIds", []).include?("wait") && body(row)["distanceToTarget"].is_a?(Numeric) && body(row)["distanceToTarget"] <= fixture["arrivalDistanceTolerance"] && distance(body(row)["pawnLocation"], fixture["targetLocation"]) <= fixture["arrivalDistanceTolerance"] }
    reset = observations.find { |row| row["sequence"] > windows[1][1] && row["sequence"] < windows[1][2] && body(row).values_at("sessionId", "pawnId", "targetActorId", "targetLocation", "destination", "distanceToTarget", "moveStatus", "behaviorTreeId", "possessed", "behavior") == [sid2, pawn_id, nil, nil, nil, nil, "idle", tree, true, "running"] && body(row).fetch("blackboardValues", []).any? { |value| value.values_at("keyName", "keyType", "valueActorId") == ["TargetActor", "Actor", nil] } }
    raise "ai runtime" unless set && moving && reached && reset
    path
  end

  def validate_animation
    p14, p15 = P14_FIXTURE, P15_FIXTURE
    bb, tree, controller, floor = P14_ASSETS.values
    abp, character = P15_ASSETS.values
    key = p14.fetch("blackboardKey")
    require_row("blackboard.create") { |row| row["input"]["path"] == "/Game/MagiP15/BB_P16AnimationAI" && body(row)["blackboardId"] == bb }
    require_row("blackboard.key_ensure") { |row| row["input"].values_at("blackboardId", "keyName", "keyType") == [bb, key["name"], key["keyType"]] }
    bb_save = rows_for("asset.save").find { |row| direct_target(row) == bb }
    expected_key = {"keyName" => key["name"], "keyType" => key["keyType"]}
    require_row("blackboard.view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("blackboardId", "keys", "revision") == [bb, [expected_key], revision(bb_save)] }
    require_row("behavior_tree.create") { |row| row["input"].values_at("path", "blackboardId") == ["/Game/MagiP15/BT_P16AnimationAI", bb] && body(row)["behaviorTreeId"] == tree }
    nodes = [["loop", "sequence", nil, nil], ["move", "move_to", "TargetActor", nil], ["wait", "wait", nil, 0.5]]
    nodes.each { |id, type, node_key, wait| require_row("behavior_tree.node_ensure") { |row| row["input"].values_at("behaviorTreeId", "nodeId", "nodeType") == [tree, id, type] && body(row).values_at("keyName", "waitSeconds") == [node_key, wait] } }
    links = p14["behaviorTreeLinks"].map { |link| link.values_at("parentNodeId", "childNodeId", "childIndex") }
    links.each { |link| require_row("behavior_tree.connect") { |row| row["input"].values_at("behaviorTreeId", "parentNodeId", "childNodeId", "childIndex") == [tree] + link } }
    tree_save = rows_for("asset.save").find { |row| direct_target(row) == tree }
    tree_view = require_row("behavior_tree.view") { |row| row["sequence"] > @level_open["sequence"] && body(row)["behaviorTreeId"] == tree }
    raise "animation ai tree" unless body(tree_view)["blackboardId"] == bb && body(tree_view)["revision"] == revision(tree_save) && body(tree_view).fetch("nodes", []).map { |node| node.values_at("nodeId", "nodeType", "keyName", "waitSeconds") }.sort_by(&:first) == nodes.sort_by(&:first) && body(tree_view).fetch("links", []).map { |link| link.values_at("parentNodeId", "childNodeId", "childIndex") }.sort == links.sort
    creates = rows_for("blueprint.create")
    controller_create = require_row("blueprint.create", creates) { |row| row["input"].values_at("path", "parentClass") == ["/Game/MagiP15/BP_P16AnimationController", "/Script/AIModule.AIController"] && body(row)["blueprintId"] == controller }
    floor_create = require_row("blueprint.create", creates) { |row| row["input"].values_at("path", "parentClass") == ["/Game/MagiP15/BP_P16AnimationFloor", "/Script/Engine.Actor"] && body(row)["blueprintId"] == floor }
    character_create = require_row("blueprint.create", creates) { |row| row["input"].values_at("path", "parentClass") == [p15["character"], "/Script/Engine.Character"] && body(row)["blueprintId"] == character }
    machine = require_row("animation.state_machine_ensure") { |row| row["input"].values_at("animationBlueprintId", "name") == [abp, "locomotion"] }
    machine_id = body(machine)["stateMachineId"]
    require_row("ai.controller_configure") { |row| row["input"].values_at("blueprintId", "behaviorTreeId") == [controller, tree] }
    require_row("ai.pawn_configure") { |row| row["input"].values_at("blueprintId", "controllerBlueprintId") == [character, controller] }
    floor_box = require_row("blueprint.scs_component_ensure") { |row| row["input"].values_at("blueprintId", "name", "class") == [floor, p14.dig("floorComponent", "name"), p14.dig("floorComponent", "class")] }
    require_row("blueprint.scs_component_update") { |row| row["input"].values_at("blueprintId", "variableGuid", "collisionEnabled", "collisionProfile", "boxExtent") == [floor, body(floor_box)["variableGuid"], "QueryAndPhysics", p14.dig("floorComponent", "collisionProfile"), [1000, 1000, 25]] }
    require_row("animation_blueprint.create") { |row| row["input"].values_at("path", "skeletonId") == [p15["animationBlueprint"], p15["skeleton"]] && body(row)["animationBlueprintId"] == abp }
    require_row("animation.variable_ensure") { |row| row["input"].values_at("animationBlueprintId", "name", "type", "source") == [abp, "Speed", "float", "owner_planar_speed"] }
    machine = require_row("animation.state_machine_ensure") { |row| row["input"].values_at("animationBlueprintId", "name") == [abp, "locomotion"] }
    path = require_row("navigation.path_query") { |row| row["input"].values_at("levelId", "start", "target") == [@spec[:level], p14["pawnLocation"], p14["targetLocation"]] && body(row).values_at("levelId", "start", "target", "reachable", "partial") == [@spec[:level], p14["pawnLocation"], p14["targetLocation"], true, false] && body(row)["pathLength"].is_a?(Numeric) && body(row)["pathLength"] > 0 && body(row).fetch("points", []).values_at(0, -1) == [p14["pawnLocation"], p14["targetLocation"]] }
    build = require_row("navigation.build") { |row| row["input"]["levelId"] == @spec[:level] && body(row)["levelId"] == @spec[:level] }
    status = require_row("navigation.status") { |row| row["sequence"] > build["sequence"] && row["sequence"] < path["sequence"] && body(row).values_at("ticketId", "levelId", "state", "terminal") == [body(build)["ticketId"], @spec[:level], "succeeded", true] }
    raise "animation navigation order" unless build["sequence"] < status["sequence"] && status["sequence"] < path["sequence"]
    idle = require_row("animation.state_ensure") { |row| row["input"].values_at("animationBlueprintId", "stateMachineId", "name", "sequenceId") == [abp, machine_id, "idle", p15["idle"]] }
    moving_state = require_row("animation.state_ensure") { |row| row["input"].values_at("animationBlueprintId", "stateMachineId", "name", "sequenceId") == [abp, machine_id, "moving", p15["moving"]] }
    require_row("animation.transition_ensure") { |row| row["input"].values_at("animationBlueprintId", "stateMachineId", "fromStateId", "toStateId", "expression") == [abp, machine_id, body(idle)["stateId"], body(moving_state)["stateId"], "Speed > 10"] }
    require_row("animation.transition_ensure") { |row| row["input"].values_at("animationBlueprintId", "stateMachineId", "fromStateId", "toStateId", "expression") == [abp, machine_id, body(moving_state)["stateId"], body(idle)["stateId"], "Speed <= 10"] }
    require_row("animation.character_configure") { |row| row["input"].values_at("characterBlueprintId", "skeletalMeshId", "animationBlueprintId") == [character, p15["skeletalMesh"], abp] }
    graph = require_row("animation.graph_view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("animationBlueprintId", "skeletonId", "revision") == [abp, p15["skeleton"], revision(rows_for("asset.save").find { |save| direct_target(save) == abp })] }
    graph_body = body(graph)
    variable = graph_body.fetch("variables", []).find { |item| item.values_at("name", "type", "source") == ["Speed", "float", "owner_planar_speed"] }
    graph_machine = graph_body.fetch("stateMachines", []).find { |item| item.values_at("stateMachineId", "name", "initialStateId") == [machine_id, "locomotion", body(idle)["stateId"]] }
    graph_states = graph_machine && graph_machine.fetch("states", []).map { |item| item.values_at("stateId", "name", "sequenceId", "skeletonId", "initial") }.sort_by(&:first)
    expected_states = [[body(idle)["stateId"], "idle", p15["idle"], p15["skeleton"], true], [body(moving_state)["stateId"], "moving", p15["moving"], p15["skeleton"], false]].sort_by(&:first)
    graph_transitions = graph_machine && graph_machine.fetch("transitions", []).map { |item| item.values_at("fromStateId", "toStateId", "expression") }.sort
    expected_transitions = [[body(idle)["stateId"], body(moving_state)["stateId"], "Speed > 10"], [body(moving_state)["stateId"], body(idle)["stateId"], "Speed <= 10"]].sort
    raise "animation graph" unless variable && graph_states == expected_states && graph_transitions == expected_transitions
    character_view = require_row("animation.character_view") { |row| row["sequence"] > @level_open["sequence"] && body(row).values_at("characterBlueprintId", "skeletalMeshId", "skeletonId", "animationMode", "animationBlueprintId", "revision") == [character, p15["skeletalMesh"], p15["skeleton"], "AnimationBlueprint", abp, revision(rows_for("asset.save").find { |save| direct_target(save) == character })] }
    raise "animation character" unless body(character_view)["animClass"] == graph_body["generatedClass"]
    bounds = require_row("navigation.bounds_ensure") { |row| row["input"].values_at("levelId", "agentKey", "location", "extent") == [@spec[:level], "p14-nav", [0, 0, 0], [1000, 1000, 100]] && body(row).values_at("levelId", "agentKey", "location", "extent") == [@spec[:level], "p14-nav", [0, 0, 0], [1000, 1000, 100]] }
    raise "animation bounds order" unless bounds["sequence"] < build["sequence"]
    character_spawn = require_row("actor.spawn") { |row| flag(row, "--class") == body(character_create)["generatedClass"] }
    target_spawn = require_row("actor.spawn") { |row| flag(row, "--class") == "/Script/Engine.TargetPoint" }
    character_id, target_id = body(character_spawn)["id"], body(target_spawn)["id"]
    windows = session_windows
    rows_for("play.start").each_with_index do |start, index|
      sid, start_sequence, stop_sequence = windows[index]
      set = require_row("play.ai_target_set") { |row| row["sequence"] > start_sequence && row["sequence"] < stop_sequence && body(row).values_at("sessionId", "pawnId", "keyName", "targetActorId", "targetLocation", "changed", "restarted") == [sid, character_id, "TargetActor", target_id, p14["targetLocation"], true, true] }
      ai = rows_for("play.ai_observe").select { |row| body(row).values_at("sessionId", "pawnId", "targetActorId") == [sid, character_id, target_id] }
      moving_ai = ai.find { |row| row["sequence"] > set["sequence"] && row["sequence"] < stop_sequence && body(row).values_at("targetLocation", "destination", "moveStatus", "behaviorTreeId", "possessed", "behavior") == [p14["targetLocation"], p14["targetLocation"], "moving", tree, true, "running"] && body(row).fetch("activeNodeIds", []).include?("move") }
      reached = ai.find { |row| moving_ai && row["sequence"] > moving_ai["sequence"] && row["sequence"] < stop_sequence && body(row).values_at("targetLocation", "destination", "moveStatus", "behaviorTreeId", "possessed", "behavior") == [p14["targetLocation"], nil, "reached", tree, true, "running"] && body(row).fetch("activeNodeIds", []).include?("wait") && body(row)["distanceToTarget"].is_a?(Numeric) && body(row)["distanceToTarget"] <= p14["arrivalDistanceTolerance"] && distance(body(row)["pawnLocation"], p14["targetLocation"]) <= p14["arrivalDistanceTolerance"] }
      anim = rows_for("play.animation_observe").select { |row| body(row).values_at("sessionId", "characterId", "animationBlueprintId", "stateMachineId") == [sid, character_id, abp, machine_id] }
      semantic_animation = lambda do |row, state_id, state_name|
        b = body(row)
        weights = b.fetch("stateWeights", []).map { |weight| weight.values_at("stateId", "name", "weight") }
        expected_weights = [[body(idle)["stateId"], "idle", state_name == "idle" ? 1 : 0], [body(moving_state)["stateId"], "moving", state_name == "moving" ? 1 : 0]]
        b.values_at("skeletalMeshId", "skeletonId", "animClass", "activeStateId", "activeStateName") == [p15["skeletalMesh"], p15["skeleton"], graph_body["generatedClass"], state_id, state_name] && weights.sort == expected_weights.sort && weights.sum { |weight| weight[2] } == 1
      end
      initial_idle = anim.find { |row| row["sequence"] > start_sequence && row["sequence"] < set["sequence"] && body(row)["speed"].to_f <= 10 && body(row)["ownerPlanarSpeed"].to_f <= 10 && semantic_animation.call(row, body(idle)["stateId"], "idle") }
      moving_anim = anim.find { |row| moving_ai && row["sequence"] > moving_ai["sequence"] && reached && row["sequence"] < reached["sequence"] && body(row)["speed"].to_f > 10 && body(row)["ownerPlanarSpeed"].to_f > 10 && semantic_animation.call(row, body(moving_state)["stateId"], "moving") }
      final_idle = anim.find { |row| reached && row["sequence"] > reached["sequence"] && row["sequence"] < stop_sequence && body(row)["speed"].to_f <= 10 && body(row)["ownerPlanarSpeed"].to_f <= 10 && semantic_animation.call(row, body(idle)["stateId"], "idle") }
      raise "animation ai runtime #{index + 1}" unless set && moving_ai && reached
      raise "animation runtime #{index + 1}" unless initial_idle && moving_anim && final_idle
    end
  end

end

if $PROGRAM_NAME == __FILE__
  begin
    if ARGV[0] == "validate" && [4, 5].include?(ARGV.length)
      OutcomeValidator.new(ARGV[1], ARGV[2], ARGV[3], ARGV[4]).validate
    elsif ARGV[0] == "self-test"
      require_relative "p16-outcome-fixtures"
      P16OutcomeFixtures.run(ARGV[1] || Dir.pwd)
    else
      warn "usage: #{File.basename($PROGRAM_NAME)} validate DIR JOB REPO_ROOT [RECORDED_DIR] | self-test"
      exit 2
    end
  rescue StandardError => error
    warn "outcome validation failed: #{error.message}"
    exit 1
  end
end
