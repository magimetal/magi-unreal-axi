# frozen_string_literal: true

require "fileutils"
require "json"
require "tmpdir"

module P16OutcomeFixtures
  REVISIONS = (1..9).to_h { |number| [number, number.to_s * 64] }.freeze
  GUIDS = {
    target: "11111111-1111-1111-1111-111111111111",
    player: "22222222-2222-2222-2222-222222222222",
    floor: "33333333-3333-3333-3333-333333333333"
  }.freeze

  class Transcript
    attr_reader :dir, :job, :ops

    def initialize(root, job, repo)
      @dir = Dir.mktmpdir("#{job}-", root)
      @job = job
      @repo = repo
      @rows = []
      @ops = Hash.new { |hash, key| hash[key] = [] }
      @catalog = JSON.parse(File.read(File.join(repo, "capabilities", "catalog.json"))).to_h { |entry| [entry.fetch("id"), entry] }
      FileUtils.mkdir_p(File.join(@dir, "project", "Plugins", "MagiUnrealAXI"))
      File.write(project, "{}\n")
    end

    def project
      File.join(@dir, "project", "MagiUnrealAXIPackageFixture.uproject")
    end

    def command(op, input = nil, extra = [])
      argv = case op
             when "setup.plugin.status" then %w[setup plugin status]
             when "project.doctor", "engine.view", "capability.search", "capability.describe",
                  "editor.start", "editor.status", "editor.stop", "blueprint.compile", "blueprint.view",
                  "asset.save", "level.create", "level.open", "level.save", "actor.spawn", "actor.view",
                  "play.start", "play.input", "play.stop", "project.cook", "project.package", "operation.view"
               op.split(".")
             else
               ["capability", "execute", op]
             end
      argv += ["--input-json", JSON.generate(input)] if input
      argv + extra
    end

    def add(op, output, input: nil, extra: [], argv: nil)
      sequence = @rows.length + 1
      path = File.join(@dir, "stdout-#{sequence}")
      output = output.merge("warningCount" => 0) if op == "blueprint.view" && output.is_a?(Hash) && !output.key?("warningCount")
      shaped = catalog_shape(op, output)
      File.write(path, shaped.is_a?(String) ? shaped : JSON.generate(shaped))
      @rows << {"sequence" => sequence, "argv" => argv || command(op, input, extra), "exit" => 0, "stdoutPath" => path}
      @ops[op] << sequence
      sequence
    end

    def catalog_shape(op, value)
      return value if op == "editor.status" || op == "operation.view"
      schema = @catalog.dig(op, "outputSchema")
      return value unless schema
      value.is_a?(Hash) && value["result"].is_a?(Hash) ? value.merge("result" => shape(schema, value["result"])) : shape(schema, value)
    end

    def shape(schema, value)
      return value unless schema.is_a?(Hash)
      if schema["variants"]
        variant = schema["variants"].find { |candidate| candidate.fetch("required", []).all? { |key| value.is_a?(Hash) && value.key?(key) } && candidate.fetch("properties", {}).key?((value.is_a?(Hash) && (value.keys - candidate.fetch("properties", {}).keys)).first) == false } || schema["variants"].max_by { |candidate| (candidate.fetch("properties", {}).keys & value.to_h.keys).length }
        return shape(variant, value)
      end
      types = Array(schema["type"])
      return nil if value.nil? && types.include?("null")
      return value if value.nil? && !types.include?("null")
      if types.include?("object")
        properties = schema.fetch("properties", {})
        result = value.is_a?(Hash) ? value.select { |key, _| properties.key?(key) }.dup : {}
        schema.fetch("required", []).each { |key| result[key] = catalog_default(properties.fetch(key, {})) unless result.key?(key) }
        properties.each { |key, child| result[key] = shape(child, result[key]) if result.key?(key) }
        result
      elsif types.include?("array")
        result = Array(value).map { |item| shape(schema.fetch("items", {}), item) }
        result += Array.new([schema.fetch("minItems", 0) - result.length, 0].max) { catalog_default(schema.fetch("items", {})) }
        result
      else
        value
      end
    end

    def catalog_default(schema)
      return schema["enum"].first if schema["enum"]
      return schema["variants"].first.fetch("required", []).to_h { |key| [key, catalog_default(schema["variants"].first.fetch("properties", {}).fetch(key, {}))] } if schema["variants"]
      types = Array(schema["type"])
      return nil if types.include?("null")
      return Array.new(schema.fetch("minItems", 0)) { catalog_default(schema.fetch("items", {})) } if types.include?("array")
      return {} if types.include?("object")
      return false if types.include?("boolean")
      return schema["minimum"] || 0 if types.include?("integer") || types.include?("number")
      "f" * 64
    end
    def validate_schema!(schema, value, label)
      if schema["variants"]
        matched = false
        schema["variants"].each do |variant|
          begin
            validate_schema!(variant, value, label)
            matched = true
            break
          rescue StandardError
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
        allowed = schema.fetch("properties", {}).keys
        raise "#{label} additionalProperties" if schema["additionalProperties"] == false && (value.keys - allowed).any?
        schema.fetch("required", []).each { |key| raise "#{label} required #{key}" unless value.key?(key) }
        schema.fetch("properties", {}).each { |key, child| validate_schema!(child, value[key], "#{label}.#{key}") if value.key?(key) }
      elsif value.is_a?(Array)
        raise "#{label} minItems" if schema["minItems"] && value.length < schema["minItems"]
        raise "#{label} maxItems" if schema["maxItems"] && value.length > schema["maxItems"]
        value.each { |item| validate_schema!(schema.fetch("items", {}), item, label) }
      elsif value.is_a?(String)
        raise "#{label} enum" if schema["enum"] && !schema["enum"].include?(value)
        raise "#{label} minLength" if schema["minLength"] && value.length < schema["minLength"]
        raise "#{label} maxLength" if schema["maxLength"] && value.length > schema["maxLength"]
      elsif value.is_a?(Numeric)
        raise "#{label} minimum" if schema["minimum"] && value < schema["minimum"]
        raise "#{label} maximum" if schema["maximum"] && value > schema["maximum"]
      end
    end

    def catalog_validate!
      @rows.each do |row|
        op = catalog_operation(row)
        next unless (entry = @catalog[op])
        body = JSON.parse(File.read(row.fetch("stdoutPath")))
        result = body["result"].is_a?(Hash) ? body["result"] : body
        validate_schema!(entry.fetch("outputSchema"), result, "#{op} output")
        input = catalog_input(row, op)
        validate_schema!(entry.fetch("inputSchema"), input, "#{op} input")
      end
    end

    def catalog_operation(row)
      return row["argv"][2] if row["argv"][0, 2] == %w[capability execute]
      pairs = %w[blueprint compile blueprint view asset save level create level open level save actor spawn actor view play start play input play stop]
      pairs.each_slice(2).map { |a, b| "#{a}.#{b}" }.find { |op| @catalog.key?(op) && row["argv"][0, 2] == op.split(".") }
    end

    def catalog_input(row, op)
      index = row["argv"].index("--input-json")
      return JSON.parse(row["argv"].fetch(index + 1)) if index
      argv = row["argv"]
      case op
      when "blueprint.compile", "blueprint.view", "asset.save", "actor.view" then {"id" => argv[2]}
      when "level.create", "level.open", "level.save" then {"path" => argv[argv.index("--path") + 1]}
      when "actor.spawn"
        value = {"levelId" => argv[argv.index("--level") + 1], "class" => argv[argv.index("--class") + 1], "agentKey" => argv[argv.index("--agent-key") + 1]}
        value["location"] = argv[argv.index("--location") + 1].split(",").map(&:to_f) if argv.include?("--location")
        value
      when "play.start" then {}
      when "play.input" then {"sessionId" => argv[argv.index("--session-id") + 1], "key" => argv[2], "event" => argv[argv.index("--event") + 1]}
      when "play.stop" then {"sessionId" => argv[argv.index("--session-id") + 1]}
      else {}
      end
    end


    def validate
      OutcomeValidator.new(@dir, @job, @repo).validate
    end

    def read(op, result, input: nil, extra: [], argv: nil)
      entry = @catalog[op]
      return mutation(op, result, input: input, extra: extra, argv: argv, persistence: "unchanged") if entry && entry["mutates"] == true
      add(op, result, input: input, extra: extra, argv: argv)
    end

    def mutation(op, result, input: nil, extra: [], argv: nil, changed: true, persistence: "dirty")
      revision = result.fetch("revision", REVISIONS[9])
      body = result.merge("changed" => changed)
      entry = @catalog.fetch(op)
      target = case op
               when "animation_blueprint.create", "blackboard.create", "behavior_tree.create"
                 path = input.fetch("path"); "#{path}.#{File.basename(path)}"
               when "animation.character_configure" then "#{input['characterBlueprintId']}#animation-character"
               when "animation.variable_ensure" then "#{input['animationBlueprintId']}#variable:#{input['name']}"
               when "animation.state_machine_ensure" then "#{input['animationBlueprintId']}#state-machine:#{input['name']}"
               when "animation.state_ensure" then "#{input['animationBlueprintId']}##{input['stateMachineId']}#state:#{input['name']}"
               when "animation.transition_ensure" then [input["animationBlueprintId"], input["stateMachineId"], input["fromStateId"], input["toStateId"]].join("#")
               when "blueprint.interface_ensure" then "#{input['blueprintId']}##{input['interfaceId']}"
               when "blueprint.scs_component_ensure" then "#{input['blueprintId']}#scs-name:#{input['name']}"
               when "blueprint.scs_component_update", "blueprint.scs_component_remove" then "#{input['blueprintId']}#scs:#{input['variableGuid']}"
               when "blueprint.event_ensure", "blueprint.node_ensure" then [input["blueprintId"], input["graphId"], input["agentKey"]].join("#")
               when "blueprint.pin_default_set" then [input["blueprintId"], input["pinId"]].join("#")
               when "blueprint.pin_connect" then [input["blueprintId"], input["sourcePinId"], input["targetPinId"]].join("#")
               when "widget.viewport_ensure" then "#{input['hostBlueprintId']}#viewport:#{input['agentKey']}"
               when "blackboard.key_ensure" then "#{input['blackboardId']}##{input['keyName']}"
               when "play.ai_target_set" then [input["sessionId"], input["pawnId"], input["keyName"], input["targetActorId"]].join("#")
               when "ai.controller_configure" then "#{input['blueprintId']}#ai-controller:#{input['behaviorTreeId']}"
               when "ai.pawn_configure" then "#{input['blueprintId']}#ai-pawn"
               when "navigation.bounds_ensure" then "#{input['levelId']}#nav-bounds:#{input['agentKey']}"
               when "play.input" then [body["sessionId"], body["key"], body["event"]].join("#")
               else
                 fields = Array(entry.dig("verification", "targetFields")); properties = entry.fetch("outputSchema").fetch("properties", {}); values = fields.map { |field| body[field] || input&.[](field) || (catalog_default(properties.fetch(field)) if properties.key?(field)) }.compact
                 values.empty? ? (result["sessionId"] || input&.fetch("sessionId", nil) || op) : values.join("#")
               end
      receipt = {
        "operationId"=>"fixture-operation-#{@rows.length + 1}", "operation"=>op, "state"=>"completed", "projectId"=>project, "editorPid"=>1,
        "target"=>target, "changed"=>changed, "transaction"=>entry["transactionBehavior"], "reversibility"=>entry["reversibility"],
        "dirtyPackages"=>body.fetch("dirtyPackages", []), "savedPackages"=>body.fetch("savedPackages", []), "revision"=>revision, "persistence"=>persistence,
        "verification"=>{"readback"=>entry.dig("verification", "readback") || "operation.view", "target"=>target, "matched"=>true}
      }
      add(op, {"result"=>body, "receipt"=>receipt}, input: input, extra: extra, argv: argv)
    end

    def view(sequence)
      source = @rows.fetch(sequence - 1)
      source_output = JSON.parse(File.read(source.fetch("stdoutPath")))
      process = %w[project.cook project.package].include?(source["argv"][0, 2].join("."))
      value = process ? source_output.fetch("operation") : source_output.fetch("receipt")
      id = process ? value.fetch("id") : value.fetch("operationId")
      add("operation.view", value, argv: ["operation", "view", id])
    end
    def editor(op, state)
      read(op, {"editor" => {"state" => state, "projectId" => project, "editorPid" => 1, "levelId" => "", "pie" => "stopped", "dirtyPackageCount" => 0}}, argv: op.split("."))
    end

    def compile(asset, revision)
      mutation("blueprint.compile", {"id" => asset, "status" => "up_to_date", "errorCount" => 0, "warningCount" => 0, "revision" => revision}, argv: ["blueprint", "compile", asset])
    end

    def save(asset, revision)
      mutation("asset.save", {"id" => asset, "revision" => revision}, argv: ["asset", "save", asset], persistence: "saved")
    end

    def spawn(level, klass, key, id, revision = REVISIONS[8], location: nil)
      argv = ["actor", "spawn", "--level", level, "--class", klass, "--agent-key", key]
      argv += ["--location", location.join(",")] if location
      mutation("actor.spawn", {"id" => id, "revision" => revision}, argv: argv)
    end

    def actor_views(ids, details = {})
      ids.each { |id| read("actor.view", {"id" => id, "revision" => REVISIONS[8]}.merge(details.fetch(id, {})), argv: ["actor", "view", id]) }
    end

    def play_start(session)
      mutation("play.start", {"sessionId" => session, "revision" => REVISIONS[9]}, argv: %w[play start], persistence: "unchanged")
    end

    def play_stop(session)
      mutation("play.stop", {"sessionId" => session, "revision" => REVISIONS[9]}, argv: ["play", "stop", "--session-id", session], persistence: "unchanged")
    end

    def accepted_input(session)
      mutation("play.input", {"sessionId" => session, "key" => "E", "event" => "pressed", "accepted" => true, "changed" => true, "revision" => REVISIONS[9]}, argv: ["play", "input", "E", "--session-id", session, "--event", "pressed"], persistence: "unchanged")
    end

    def common_start(level)
      editor("editor.start", "ready")
      editor("editor.status", "ready")
      mutation("level.create", {"level" => level, "revision" => REVISIONS[8]}, argv: ["level", "create", "--path", level])
    end

    def save_level(level)
      mutation("level.save", {"level" => level, "revision" => REVISIONS[8]}, argv: ["level", "save", "--path", level], persistence: "saved")
    end

    def restart(level)
      editor("editor.stop", "stopped")
      editor("editor.start", "ready")
      editor("editor.status", "ready")
      read("level.open", {"level" => level, "revision" => REVISIONS[8]}, argv: ["level", "open", "--path", level])
    end

    def durable_native_views
      selections = OutcomeValidator::DURABLE_VIEW_SELECTION.fetch(@job)
      selections.first(7).each do |selection|
        sequence = @ops.fetch(selection.fetch(:operation)).find do |candidate|
          row = @rows.fetch(candidate - 1)
          selection.fetch(:target, nil).nil? || row["argv"].include?(selection[:target]) || output(candidate).dig("result", "id") == selection[:target] || output(candidate).dig("result", "blueprintId") == selection[:target] || output(candidate).dig("result", "blackboardId") == selection[:target] || output(candidate).dig("result", "animationBlueprintId") == selection[:target]
        end
        sequence = @ops.fetch(selection.fetch(:operation)).last if selection[:last]
        raise "fixture durable source #{@job} #{selection.inspect}" unless sequence
        view(sequence)
      end
    end

    def pipeline
      editor("editor.stop", "stopped")
      durable_native_views
      cooked = File.join(@dir, "cooked")
      package = File.join(@dir, "package")
      FileUtils.mkdir_p(File.join(cooked, "Mac"))
      File.write(File.join(cooked, "Mac", "AssetRegistry.bin"), "registry")
      app = File.join(package, "Mac", "MagiUnrealAXIPackageFixture.app")
      FileUtils.mkdir_p(app)
      File.write(File.join(package, "Mac", "MagiUnrealAXIPackageFixture-Mac.utoc"), "utoc")
      cook = read("project.cook", {"operation" => {"id" => "proc-cook", "kind" => "cook", "status" => "passed", "project" => project, "exitCode" => 0}, "artifacts" => [{"kind" => "cooked-output", "path" => cooked, "exists" => true}]}, argv: ["project", "cook", "--output", cooked])
      view(cook)
      package_sequence = read("project.package", {"operation" => {"id" => "proc-package", "kind" => "package", "status" => "passed", "project" => project, "exitCode" => 0}, "artifacts" => [{"kind" => "package-output", "path" => package, "exists" => true}]}, argv: ["project", "package", "--output", package])
      view(package_sequence)
    end

    def write_evidence
      File.write(File.join(@dir, "ledger.jsonl"), @rows.map { |row| JSON.generate(row) }.join("\n") + "\n")
      required = @job == "unknown-project-orientation" ? ["orientation"] : %w[compile save restart runtime noop package]
      refs = OutcomeValidator::CATEGORIES.to_h { |category| [category, []] }
      outcome = {
        "job" => @job, "status" => "passed", "requiredCategories" => required,
        "references" => refs, "expectedFailureSequences" => [],
        "metrics" => {"retries" => 0, "avoidableRetries" => 0, "structuredOutputFailures" => 0}
      }
      File.write(File.join(@dir, "agent-outcome.json"), JSON.generate(outcome))
      if @job == "unknown-project-orientation"
        refs["orientation"] = (1..@rows.length).to_a
      else
        validator = OutcomeValidator.new(@dir, @job, @repo)
        validator.instance_variable_set(:@spec, OutcomeValidator::SPECS.fetch(@job))
        validator.instance_variable_set(:@project, project)
        validator.send(:locate_phases)
        refs = validator.send(:expected_references)
      end
      outcome["references"] = refs
      File.write(File.join(@dir, "agent-outcome.json"), JSON.generate(outcome))
      self
    end


    def output(sequence)
      row = @rows.fetch(sequence - 1)
      JSON.parse(File.read(row.fetch("stdoutPath")))
    end

    def edit_output(sequence)
      value = output(sequence)
      yield value
      File.write(@rows.fetch(sequence - 1).fetch("stdoutPath"), JSON.generate(value))
    end

    def edit_input(sequence)
      row = @rows.fetch(sequence - 1)
      index = row["argv"].index("--input-json")
      value = JSON.parse(row["argv"].fetch(index + 1))
      yield value
      row["argv"][index + 1] = JSON.generate(value)
      rewrite_ledger
    end

    def edit_outcome
      path = File.join(@dir, "agent-outcome.json")
      value = JSON.parse(File.read(path))
      yield value
      File.write(path, JSON.generate(value))
    end

    def edit_row(sequence)
      yield @rows.fetch(sequence - 1)
      rewrite_ledger
    end

    def move_before(sequence, before_sequence)
      row = @rows.delete_at(sequence - 1)
      @rows.insert(before_sequence - 1, row)
      @rows.each_with_index { |value, index| value["sequence"] = index + 1 }
      rewrite_ledger
    end

    def duplicate(sequence)
      @rows << @rows.fetch(sequence - 1).dup.merge("sequence" => @rows.length + 1)
      rewrite_ledger
    end

    def delete(sequence)
      @rows.delete_at(sequence - 1)
      @rows.each_with_index { |value, index| value["sequence"] = index + 1 }
      rewrite_ledger
    end

    def refresh_references
      rewrite_ledger
      validator = OutcomeValidator.new(@dir, @job, @repo)
      validator.instance_variable_set(:@spec, OutcomeValidator::SPECS.fetch(@job))
      validator.instance_variable_set(:@project, project)
      validator.send(:locate_phases)
      refs = validator.send(:expected_references)
      edit_outcome { |value| value["references"] = refs }
    end

    def rewrite_ledger
      File.write(File.join(@dir, "ledger.jsonl"), @rows.map { |row| JSON.generate(row) }.join("\n") + "\n")
    end
  end

  def self.orientation(root, repo)
    transcript = Transcript.new(root, "unknown-project-orientation", repo)
    transcript.add("help", "Usage: magi-unreal-axi project doctor\n", argv: %w[project doctor --help])
    transcript.read("project.doctor", {"project" => transcript.project, "checks" => [{"name" => "descriptor", "passed" => true}], "help" => ["magi-unreal-axi capability search <query>"]})
    transcript.read("engine.view", {"root" => "/Users/Shared/Epic Games/UE_5.8", "version" => "5.8.1", "changelist" => 56_057_345})
    transcript.read("setup.plugin.status", {"plugin" => {"path" => File.join(transcript.dir, "project", "Plugins", "MagiUnrealAXI"), "installed" => true, "managed" => true, "compatible" => true}})
    transcript.read("editor.status", {"editor" => {"state" => "stopped"}})
    transcript.read("capability.search", {"scope" => "generated capability catalog", "count" => 1, "total" => 79, "items" => [{"id" => "actor.spawn"}]}, argv: %w[capability search actor])
    transcript.write_evidence
  end

  def self.graph_pin(node_id, name, direction, default = "", links = [])
    {"pinId" => "#{node_id}:#{direction}:#{name}", "name" => name, "direction" => direction, "type" => "fixture", "defaultValue" => default.to_s, "links" => links}
  end

  def self.author_graph(transcript, asset, graph_id, revision, node_specs, defaults, links)
    transcript.read("blueprint.graph_view", {"blueprintId" => asset, "count" => 1, "total" => 1, "scope" => asset, "items" => [{"graphId" => graph_id, "kind" => "ubergraph", "name" => "EventGraph", "nodeCount" => node_specs.length}], "nextCursor" => nil, "revision" => revision}, input: {"blueprintId" => asset})
    nodes = node_specs.to_h do |key, op, intent, extra, pins|
      node_id = "#{graph_id}:#{key}"
      input = {"blueprintId" => asset, "graphId" => graph_id, "agentKey" => "p16.#{key}", op == "blueprint.event_ensure" ? "event" : "node" => intent}.merge(extra)
      transcript.mutation(op, {"blueprintId" => asset, "graphId" => graph_id, "nodeId" => node_id, "revision" => revision}, input: input)
      [key, {"nodeId" => node_id, "class" => "FixtureNode", "title" => intent, "x" => 0, "y" => 0, "pins" => pins.map { |name, direction| graph_pin(node_id, name, direction) }}]
    end
    transcript.read("blueprint.graph_view", {"blueprintId" => asset, "count" => nodes.length, "total" => nodes.length, "scope" => graph_id, "items" => nodes.values, "nextCursor" => nil, "revision" => revision}, input: {"blueprintId" => asset, "graphId" => graph_id})
    defaults.each do |node_key, pin_name, type, value|
      pin_id = "#{nodes.fetch(node_key).fetch('nodeId')}:input:#{pin_name}"
      transcript.mutation("blueprint.pin_default_set", {"blueprintId" => asset, "pinId" => pin_id, "revision" => revision}, input: {"blueprintId" => asset, "pinId" => pin_id, "value" => {"type" => type, "value" => value}})
      nodes.fetch(node_key).fetch("pins").find { |pin| pin["pinId"] == pin_id }["defaultValue"] = value.to_s
    end
    links.each do |source_key, source_name, target_key, target_name|
      source = "#{nodes.fetch(source_key).fetch('nodeId')}:output:#{source_name}"
      target = "#{nodes.fetch(target_key).fetch('nodeId')}:input:#{target_name}"
      transcript.mutation("blueprint.pin_connect", {"blueprintId" => asset, "sourcePinId" => source, "targetPinId" => target, "revision" => revision}, input: {"blueprintId" => asset, "sourcePinId" => source, "targetPinId" => target})
      nodes.values.flat_map { |node| node["pins"] }.find { |pin| pin["pinId"] == source }["links"] = [target]
      nodes.values.flat_map { |node| node["pins"] }.find { |pin| pin["pinId"] == target }["links"] = [source]
    end
    [graph_id, nodes.values]
  end

  def self.interaction(root, repo)
    transcript = Transcript.new(root, "interaction-loop", repo)
    level = "/Game/MagiP12/P12Interaction"
    interface = "/Game/MagiP12/BPI_Interact.BPI_Interact"
    target = "/Game/MagiP12/BP_Interactable.BP_Interactable"
    player = "/Game/MagiP12/BP_Player.BP_Player"
    target_class, player_class = "#{target}_C", "#{player}_C"
    transcript.common_start(level)
    transcript.mutation("blueprint.interface_create", {"id" => interface, "function" => "Interact", "revision" => REVISIONS[1]}, input: {"path" => "/Game/MagiP12/BPI_Interact", "function" => "Interact"})
    [[target, target_class, REVISIONS[2]], [player, player_class, REVISIONS[3]]].each do |asset, klass, revision|
      transcript.mutation("blueprint.create", {"blueprintId" => asset, "generatedClass" => klass, "revision" => revision}, input: {"path" => asset.split(".").first, "parentClass" => "/Script/Engine.Actor"})
      transcript.mutation("blueprint.interface_ensure", {"blueprintId" => asset, "interfaceId" => interface, "revision" => revision}, input: {"blueprintId" => asset, "interfaceId" => interface})
    end
    [[target, "OverlapBox", GUIDS[:target], REVISIONS[2]], [player, "InteractionBox", GUIDS[:player], REVISIONS[3]]].each do |asset, name, guid, revision|
      transcript.mutation("blueprint.scs_component_ensure", {"blueprintId" => asset, "variableGuid" => guid, "revision" => revision}, input: {"blueprintId" => asset, "name" => name, "class" => "BoxComponent"})
      transcript.mutation("blueprint.scs_component_update", {"blueprintId" => asset, "variableGuid" => guid, "revision" => revision}, input: {"blueprintId" => asset, "variableGuid" => guid, "collisionEnabled" => "QueryOnly", "collisionProfile" => "OverlapAllDynamic", "generateOverlapEvents" => true, "boxExtent" => [100, 100, 100]})
    end
    target_specs = [
      [:overlap, "blueprint.event_ensure", "component.begin_overlap", {"variableGuid" => GUIDS[:target]}, [["then", "output"], ["OtherActor", "output"]]],
      [:message, "blueprint.node_ensure", "interface.message_interact", {"interfaceId" => interface}, [["execute", "input"], ["self", "input"]]],
      [:begin, "blueprint.event_ensure", "actor.begin_play", {}, [["then", "output"]]],
      [:input, "blueprint.event_ensure", "input.key_e", {}, [["Pressed", "output"]]],
      [:controller, "blueprint.node_ensure", "game.get_player_controller", {}, [["PlayerIndex", "input"], ["ReturnValue", "output"]]],
      [:enable, "blueprint.node_ensure", "actor.enable_input", {}, [["execute", "input"], ["PlayerController", "input"]]],
      [:vector, "blueprint.node_ensure", "math.make_vector", {}, [["X", "input"], ["Y", "input"], ["Z", "input"], ["ReturnValue", "output"]]],
      [:offset, "blueprint.node_ensure", "actor.add_world_offset", {}, [["execute", "input"], ["DeltaLocation", "input"]]]
    ]
    target_defaults = [[:controller, "PlayerIndex", "integer", 0], [:vector, "X", "real", 200], [:vector, "Y", "real", 0], [:vector, "Z", "real", 0]]
    target_links = [[:overlap, "then", :message, "execute"], [:overlap, "OtherActor", :message, "self"], [:begin, "then", :enable, "execute"], [:controller, "ReturnValue", :enable, "PlayerController"], [:input, "Pressed", :offset, "execute"], [:vector, "ReturnValue", :offset, "DeltaLocation"]]
    player_specs = [
      [:interact, "blueprint.event_ensure", "interface.interact", {"interfaceId" => interface}, [["then", "output"]]],
      [:vector, "blueprint.node_ensure", "math.make_vector", {}, [["X", "input"], ["Y", "input"], ["Z", "input"], ["ReturnValue", "output"]]],
      [:offset, "blueprint.node_ensure", "actor.add_world_offset", {}, [["execute", "input"], ["DeltaLocation", "input"]]]
    ]
    player_defaults = [[:vector, "X", "real", 100], [:vector, "Y", "real", 0], [:vector, "Z", "real", 0]]
    player_links = [[:interact, "then", :offset, "execute"], [:vector, "ReturnValue", :offset, "DeltaLocation"]]
    target_graph, target_nodes = author_graph(transcript, target, "target-ubergraph", REVISIONS[2], target_specs, target_defaults, target_links)
    player_graph, player_nodes = author_graph(transcript, player, "player-ubergraph", REVISIONS[3], player_specs, player_defaults, player_links)
    transcript.compile(target, REVISIONS[2])
    transcript.compile(player, REVISIONS[3])
    transcript.spawn(level, target_class, "target", "actor-target")
    transcript.spawn(level, player_class, "player", "actor-player")
    transcript.save(interface, REVISIONS[1])
    transcript.save(target, REVISIONS[2])
    transcript.save(player, REVISIONS[3])
    transcript.save_level(level)
    transcript.restart(level)
    transcript.read("blueprint.interface_view", {"id" => interface, "function" => "Interact", "revision" => REVISIONS[1]}, input: {"id" => interface})
    [[target, "OverlapBox", GUIDS[:target], REVISIONS[2], target_graph, target_nodes], [player, "InteractionBox", GUIDS[:player], REVISIONS[3], player_graph, player_nodes]].each do |asset, name, guid, revision, graph_id, nodes|
      transcript.read("blueprint.scs_view", {"blueprintId" => asset, "components" => [{"variableGuid" => guid, "name" => name, "collisionEnabled" => "QueryOnly", "collisionProfile" => "OverlapAllDynamic", "generateOverlapEvents" => true, "boxExtent" => [100, 100, 100]}], "revision" => revision}, input: {"blueprintId" => asset})
      transcript.read("blueprint.graph_view", {"blueprintId" => asset, "count" => nodes.length, "total" => nodes.length, "scope" => graph_id, "items" => nodes, "nextCursor" => nil, "revision" => revision}, input: {"blueprintId" => asset, "graphId" => graph_id})
      transcript.read("blueprint.view", {"id" => asset, "generatedClass" => "#{asset}_C", "errorCount" => 0, "warningCount" => 0, "revision" => revision}, argv: ["blueprint", "view", asset])
    end
    transcript.actor_views(%w[actor-target actor-player])
    transcript.mutation("blueprint.interface_ensure", {"blueprintId" => player, "interfaceId" => interface, "revision" => REVISIONS[3]}, input: {"blueprintId" => player, "interfaceId" => interface}, changed: false, persistence: "unchanged")
    2.times do |index|
      session = "interaction-session-#{index + 1}"
      transcript.play_start(session)
      [["actor-target", GUIDS[:target]], ["actor-player", GUIDS[:player]]].each { |actor, guid| transcript.read("play.component_observe", {"sessionId" => session, "actorId" => actor, "variableGuid" => guid, "resolved" => true, "overlapCount" => 0, "overlappingActorIds" => [], "interactionDisplacement" => [0, 0, 0]}, input: {"sessionId" => session, "actorId" => actor, "variableGuid" => guid}) }
      transcript.accepted_input(session)
      transcript.read("play.component_observe", {"sessionId" => session, "actorId" => "actor-target", "variableGuid" => GUIDS[:target], "resolved" => true, "overlapCount" => 1, "overlappingActorIds" => ["actor-player"], "interactionDisplacement" => [200, 0, 0]}, input: {"sessionId" => session, "actorId" => "actor-target", "variableGuid" => GUIDS[:target]})
      transcript.read("play.component_observe", {"sessionId" => session, "actorId" => "actor-player", "variableGuid" => GUIDS[:player], "resolved" => true, "overlapCount" => 1, "overlappingActorIds" => ["actor-target"], "interactionDisplacement" => [100, 0, 0]}, input: {"sessionId" => session, "actorId" => "actor-player", "variableGuid" => GUIDS[:player]})
      transcript.play_stop(session)
    end
    transcript.pipeline
    transcript.write_evidence
  end

  def self.ui(root, repo)
    transcript = Transcript.new(root, "ui-state-loop", repo)
    fixture = OutcomeValidator::P13_FIXTURE
    level = fixture.fetch("level")
    widget = "#{fixture.fetch('widgetPath')}.WBP_UIState"
    host = "#{fixture.fetch('hostPath')}.BP_UIStateHost"
    root_id = "#{widget}#widget:StateRoot"
    text_id = "#{widget}#widget:StateText"
    widget_class, host_class = "#{widget}_C", "#{host}_C"
    event_id = "#{widget}#event:#{fixture.fetch('agentKey')}"
    action = {"kind" => "text.set", "targetWidgetId" => text_id, "text" => fixture.dig("text", "active")}
    transcript.common_start(level)
    transcript.mutation("widget.create", {"blueprintId" => widget, "generatedClass" => widget_class, "rootWidgetId" => root_id, "revision" => REVISIONS[1]}, input: {"path" => fixture["widgetPath"], "rootName" => fixture.dig("root", "name"), "rootClass" => fixture.dig("root", "class")})
    transcript.mutation("widget.child_ensure", {"blueprintId" => widget, "widgetId" => text_id, "revision" => REVISIONS[1]}, input: {"blueprintId" => widget, "parentWidgetId" => root_id, "name" => fixture.dig("text", "name"), "class" => fixture.dig("text", "class")})
    transcript.mutation("widget.property_set", {"blueprintId" => widget, "widgetId" => text_id, "text" => fixture.dig("text", "ready"), "revision" => REVISIONS[1]}, input: {"blueprintId" => widget, "widgetId" => text_id, "property" => "text", "text" => fixture.dig("text", "ready")})
    event_input = {"blueprintId" => widget, "agentKey" => fixture["agentKey"], "event" => "activate", "actions" => [action]}
    transcript.mutation("widget.event_ensure", {"blueprintId" => widget, "eventId" => event_id, "agentKey" => fixture["agentKey"], "event" => "activate", "actions" => [action], "revision" => REVISIONS[1]}, input: event_input)
    transcript.mutation("blueprint.create", {"blueprintId" => host, "generatedClass" => host_class, "revision" => REVISIONS[2]}, input: {"path" => fixture["hostPath"], "parentClass" => "/Script/Engine.Actor"})
    transcript.mutation("widget.viewport_ensure", {"hostBlueprintId" => host, "widgetBlueprintId" => widget, "revision" => REVISIONS[2]}, input: {"hostBlueprintId" => host, "widgetBlueprintId" => widget, "agentKey" => fixture["agentKey"], "inputKey" => fixture["inputKey"], "zOrder" => fixture["zOrder"]})
    transcript.compile(widget, REVISIONS[1])
    transcript.compile(host, REVISIONS[2])
    transcript.spawn(level, host_class, "host", "actor-host")
    transcript.save(widget, REVISIONS[1])
    transcript.save(host, REVISIONS[2])
    transcript.save_level(level)
    transcript.restart(level)
    widgets = [
      {"widgetId" => root_id, "name" => fixture.dig("root", "name"), "class" => fixture.dig("root", "class"), "parentWidgetId" => nil, "index" => 0, "text" => nil, "visibility" => "Visible", "enabled" => true},
      {"widgetId" => text_id, "name" => fixture.dig("text", "name"), "class" => fixture.dig("text", "class"), "parentWidgetId" => root_id, "index" => 1, "text" => fixture.dig("text", "ready"), "visibility" => "Visible", "enabled" => true}
    ]
    transcript.read("widget.tree_view", {"blueprintId" => widget, "generatedClass" => widget_class, "rootWidgetId" => root_id, "count" => 2, "total" => 2, "scope" => widget, "widgets" => widgets, "events" => [{"eventId" => event_id, "agentKey" => fixture["agentKey"], "event" => "activate", "actions" => [action]}], "revision" => REVISIONS[1]}, input: {"blueprintId" => widget})
    transcript.read("blueprint.view", {"id" => host, "generatedClass" => host_class, "errorCount" => 0, "warningCount" => 0, "revision" => REVISIONS[2]}, argv: ["blueprint", "view", host])
    transcript.actor_views(["actor-host"])
    transcript.mutation("widget.event_ensure", {"blueprintId" => widget, "eventId" => event_id, "agentKey" => fixture["agentKey"], "event" => "activate", "actions" => [action], "revision" => REVISIONS[1]}, input: event_input, changed: false, persistence: "unchanged")
    2.times do |index|
      session, instance = "ui-session-#{index + 1}", "widget-instance-#{index + 1}"
      transcript.play_start(session)
      [fixture.dig("text", "ready"), fixture.dig("text", "active")].each_with_index do |text, state_index|
        transcript.accepted_input(session) if state_index == 1
        runtime_widget = {"widgetId" => text_id, "name" => fixture.dig("text", "name"), "class" => fixture.dig("text", "class"), "text" => text, "visibility" => "Visible", "enabled" => true}
        transcript.read("play.ui_observe", {"sessionId" => session, "widgetBlueprintId" => widget, "instanceId" => instance, "inViewport" => true, "widgets" => [runtime_widget]}, input: {"sessionId" => session, "widgetBlueprintId" => widget, "widgetIds" => [text_id]})
      end
      transcript.play_stop(session)
    end
    transcript.pipeline
    transcript.write_evidence
  end

  def self.ai(root, repo)
    transcript = Transcript.new(root, "ai-navigation-loop", repo)
    level = "/Game/MagiP14/P14AI"
    assets = OutcomeValidator::SPECS.fetch("ai-navigation-loop")[:assets].keys
    bb, tree, controller, pawn, floor = assets
    classes = {controller => "#{controller}_C", pawn => "#{pawn}_C", floor => "#{floor}_C"}
    transcript.common_start(level)
    transcript.mutation("blackboard.create", {"blackboardId" => bb, "revision" => REVISIONS[1]}, input: {"path" => "/Game/MagiP14/BB_P14AI"})
    transcript.mutation("blackboard.key_ensure", {"blackboardId" => bb, "keyName" => "TargetActor", "keyType" => "Actor", "revision" => REVISIONS[1]}, input: {"blackboardId" => bb, "keyName" => "TargetActor", "keyType" => "Actor"})
    transcript.mutation("behavior_tree.create", {"behaviorTreeId" => tree, "blackboardId" => bb, "revision" => REVISIONS[2]}, input: {"path" => "/Game/MagiP14/BT_P14AI", "blackboardId" => bb})
    nodes = [["loop", "sequence", nil, nil], ["move", "move_to", "TargetActor", nil], ["wait", "wait", nil, 0.5]]
    nodes.each do |id, type, key, wait|
      transcript.mutation("behavior_tree.node_ensure", {"behaviorTreeId" => tree, "nodeId" => id, "nodeType" => type, "keyName" => key, "waitSeconds" => wait, "revision" => REVISIONS[2]}, input: {"behaviorTreeId" => tree, "nodeId" => id, "nodeType" => type})
    end
    links = [["root", "loop", 0], ["loop", "move", 0], ["loop", "wait", 1]]
    links.each { |parent, child, index| transcript.mutation("behavior_tree.connect", {"behaviorTreeId" => tree, "revision" => REVISIONS[2]}, input: {"behaviorTreeId" => tree, "parentNodeId" => parent, "childNodeId" => child, "childIndex" => index}) }
    [[controller, "/Script/AIModule.AIController", REVISIONS[3]], [pawn, "/Script/Engine.Character", REVISIONS[4]], [floor, "/Script/Engine.Actor", REVISIONS[5]]].each do |asset, parent, revision|
      transcript.mutation("blueprint.create", {"blueprintId" => asset, "generatedClass" => classes.fetch(asset), "revision" => revision}, input: {"path" => asset.split(".").first, "parentClass" => parent})
    end
    transcript.mutation("ai.controller_configure", {"blueprintId" => controller, "behaviorTreeId" => tree, "revision" => REVISIONS[3]}, input: {"blueprintId" => controller, "behaviorTreeId" => tree})
    transcript.mutation("ai.pawn_configure", {"blueprintId" => pawn, "controllerBlueprintId" => controller, "revision" => REVISIONS[4]}, input: {"blueprintId" => pawn, "controllerBlueprintId" => controller})
    transcript.mutation("blueprint.scs_component_ensure", {"blueprintId" => floor, "variableGuid" => GUIDS[:floor], "revision" => REVISIONS[5]}, input: {"blueprintId" => floor, "name" => "FloorBox", "class" => "BoxComponent"})
    transcript.mutation("blueprint.scs_component_update", {"blueprintId" => floor, "variableGuid" => GUIDS[:floor], "revision" => REVISIONS[5]}, input: {"blueprintId" => floor, "variableGuid" => GUIDS[:floor], "collisionEnabled" => "QueryAndPhysics", "collisionProfile" => "BlockAll", "boxExtent" => [1000, 1000, 25]})
    transcript.mutation("navigation.bounds_ensure", {"levelId" => level, "agentKey" => "p14-nav", "boundsId" => "bounds", "location" => [0, 0, 0], "extent" => [1000, 1000, 100], "revision" => REVISIONS[8]}, input: {"levelId" => level, "agentKey" => "p14-nav", "location" => [0, 0, 0], "extent" => [1000, 1000, 100]})
    [[controller, REVISIONS[3]], [pawn, REVISIONS[4]], [floor, REVISIONS[5]]].each { |asset, revision| transcript.compile(asset, revision) }
    transcript.spawn(level, classes.fetch(floor), "floor", "actor-floor", location: [0, 0, -25])
    transcript.spawn(level, classes.fetch(pawn), "pawn", "actor-pawn", location: [0, 0, 100])
    transcript.spawn(level, "/Script/Engine.TargetPoint", "target", "actor-target", location: [600, 0, 100])
    [[bb, REVISIONS[1]], [tree, REVISIONS[2]], [controller, REVISIONS[3]], [pawn, REVISIONS[4]], [floor, REVISIONS[5]]].each { |asset, revision| transcript.save(asset, revision) }
    transcript.save_level(level)
    transcript.restart(level)
    transcript.read("blackboard.view", {"blackboardId" => bb, "keys" => [{"keyName" => "TargetActor", "keyType" => "Actor"}], "revision" => REVISIONS[1]}, input: {"blackboardId" => bb})
    transcript.read("behavior_tree.view", {"behaviorTreeId" => tree, "blackboardId" => bb, "nodes" => nodes.map { |id, type, key, wait| {"nodeId" => id, "nodeType" => type, "keyName" => key, "waitSeconds" => wait} }, "links" => links.each_with_index.map { |(parent, child, index), link_index| {"linkId" => "link-#{link_index}", "parentNodeId" => parent, "childNodeId" => child, "childIndex" => index} }, "revision" => REVISIONS[2]}, input: {"behaviorTreeId" => tree})
    [[controller, REVISIONS[3]], [pawn, REVISIONS[4]], [floor, REVISIONS[5]]].each { |asset, revision| transcript.read("blueprint.view", {"id" => asset, "generatedClass" => classes.fetch(asset), "errorCount" => 0, "warningCount" => 0, "revision" => revision}, argv: ["blueprint", "view", asset]) }
    transcript.read("blueprint.scs_view", {"blueprintId" => floor, "components" => [{"variableGuid" => GUIDS[:floor], "name" => "FloorBox", "collisionEnabled" => "QueryAndPhysics", "collisionProfile" => "BlockAll", "boxExtent" => [1000, 1000, 25]}], "revision" => REVISIONS[5]}, input: {"blueprintId" => floor})
    details = {"actor-floor" => {"class" => classes.fetch(floor), "location" => [0, 0, -25]}, "actor-pawn" => {"class" => classes.fetch(pawn), "location" => [0, 0, 100]}, "actor-target" => {"class" => "/Script/Engine.TargetPoint", "location" => [600, 0, 100]}}
    transcript.actor_views(%w[actor-floor actor-pawn actor-target], details)
    transcript.mutation("blackboard.key_ensure", {"blackboardId" => bb, "keyName" => "TargetActor", "keyType" => "Actor", "revision" => REVISIONS[1]}, input: {"blackboardId" => bb, "keyName" => "TargetActor", "keyType" => "Actor"}, changed: false, persistence: "unchanged")
    transcript.read("navigation.build", {"levelId" => level, "ticketId" => "ticket", "state" => "scheduled", "changed" => true, "revision" => REVISIONS[8]}, input: {"levelId" => level})
    transcript.read("navigation.status", {"ticketId" => "ticket", "levelId" => level, "state" => "succeeded", "terminal" => true, "revision" => REVISIONS[8]}, input: {"ticketId" => "ticket"})
    transcript.read("navigation.path_query", {"levelId" => level, "start" => [0, 0, 100], "target" => [600, 0, 100], "reachable" => true, "partial" => false, "pathLength" => 600, "points" => [[0, 0, 100], [600, 0, 100]], "revision" => REVISIONS[8]}, input: {"levelId" => level, "start" => [0, 0, 100], "target" => [600, 0, 100]})
    transcript.play_start("ai-session-1")
    transcript.mutation("play.ai_target_set", {"sessionId" => "ai-session-1", "pawnId" => "actor-pawn", "controllerId" => "controller", "keyName" => "TargetActor", "targetActorId" => "actor-target", "targetLocation" => [600, 0, 100], "changed" => true, "restarted" => true, "revision" => REVISIONS[9]}, input: {"sessionId" => "ai-session-1", "pawnId" => "actor-pawn", "keyName" => "TargetActor", "targetActorId" => "actor-target"}, persistence: "unchanged")
    common_ai = {"pawnId" => "actor-pawn", "controllerId" => "controller", "targetActorId" => "actor-target", "targetLocation" => [600, 0, 100], "behaviorTreeId" => tree, "possessed" => true, "behavior" => "running", "blackboardValues" => [{"keyName" => "TargetActor", "keyType" => "Actor", "valueActorId" => "actor-target"}]}
    transcript.read("play.ai_observe", common_ai.merge("sessionId" => "ai-session-1", "pawnLocation" => [100, 0, 100], "destination" => [600, 0, 100], "distanceToTarget" => 500, "moveStatus" => "moving", "activeNodeIds" => ["move"]), input: {"sessionId" => "ai-session-1", "pawnId" => "actor-pawn", "keyName" => "TargetActor"})
    transcript.read("play.ai_observe", common_ai.merge("sessionId" => "ai-session-1", "pawnLocation" => [590, 0, 100], "destination" => nil, "distanceToTarget" => 10, "moveStatus" => "reached", "activeNodeIds" => ["wait"]), input: {"sessionId" => "ai-session-1", "pawnId" => "actor-pawn", "keyName" => "TargetActor"})
    transcript.play_stop("ai-session-1")
    transcript.play_start("ai-session-2")
    transcript.read("play.ai_observe", {"sessionId" => "ai-session-2", "pawnId" => "actor-pawn", "controllerId" => "controller", "pawnLocation" => [0, 0, 100], "targetActorId" => nil, "targetLocation" => nil, "distanceToTarget" => nil, "moveStatus" => "idle", "behaviorTreeId" => tree, "possessed" => true, "behavior" => "running", "destination" => nil, "activeNodeIds" => [], "blackboardValues" => [{"keyName" => "TargetActor", "keyType" => "Actor", "valueActorId" => nil}]}, input: {"sessionId" => "ai-session-2", "pawnId" => "actor-pawn", "keyName" => "TargetActor"})
    transcript.play_stop("ai-session-2")
    transcript.pipeline
    transcript.write_evidence
  end

  def self.animation(root, repo)
    transcript = Transcript.new(root, "animation-state-loop", repo)
    p14, p15 = OutcomeValidator::P14_FIXTURE, OutcomeValidator::P15_FIXTURE
    level = p15.fetch("level")
    bb, tree, controller, floor = OutcomeValidator::P14_ASSETS.values
    abp, character = OutcomeValidator::P15_ASSETS.values
    classes = {controller => "#{controller}_C", floor => "#{floor}_C", character => "#{character}_C"}
    nodes = [["loop", "sequence", nil, nil], ["move", "move_to", "TargetActor", nil], ["wait", "wait", nil, 0.5]]
    links = [["root", "loop", 0], ["loop", "move", 0], ["loop", "wait", 1]]
    transcript.common_start(level)
    transcript.mutation("blackboard.create", {"blackboardId" => bb, "revision" => REVISIONS[3]}, input: {"path" => "/Game/MagiP15/BB_P16AnimationAI"})
    transcript.mutation("blackboard.key_ensure", {"blackboardId" => bb, "keyName" => "TargetActor", "keyType" => "Actor", "revision" => REVISIONS[3]}, input: {"blackboardId" => bb, "keyName" => "TargetActor", "keyType" => "Actor"})
    transcript.mutation("behavior_tree.create", {"behaviorTreeId" => tree, "blackboardId" => bb, "revision" => REVISIONS[4]}, input: {"path" => "/Game/MagiP15/BT_P16AnimationAI", "blackboardId" => bb})
    nodes.each { |id, type, key, wait| transcript.mutation("behavior_tree.node_ensure", {"behaviorTreeId" => tree, "nodeId" => id, "nodeType" => type, "keyName" => key, "waitSeconds" => wait, "revision" => REVISIONS[4]}, input: {"behaviorTreeId" => tree, "nodeId" => id, "nodeType" => type}) }
    links.each { |parent, child, index| transcript.mutation("behavior_tree.connect", {"behaviorTreeId" => tree, "revision" => REVISIONS[4]}, input: {"behaviorTreeId" => tree, "parentNodeId" => parent, "childNodeId" => child, "childIndex" => index}) }
    [[controller, "/Script/AIModule.AIController", REVISIONS[5]], [floor, "/Script/Engine.Actor", REVISIONS[6]], [character, "/Script/Engine.Character", REVISIONS[2]]].each do |asset, parent, revision|
      transcript.mutation("blueprint.create", {"blueprintId" => asset, "generatedClass" => classes.fetch(asset), "revision" => revision}, input: {"path" => asset.split(".").first, "parentClass" => parent})
    end
    transcript.mutation("ai.controller_configure", {"blueprintId" => controller, "behaviorTreeId" => tree, "revision" => REVISIONS[5]}, input: {"blueprintId" => controller, "behaviorTreeId" => tree})
    transcript.mutation("ai.pawn_configure", {"blueprintId" => character, "controllerBlueprintId" => controller, "revision" => REVISIONS[2]}, input: {"blueprintId" => character, "controllerBlueprintId" => controller})
    transcript.mutation("blueprint.scs_component_ensure", {"blueprintId" => floor, "variableGuid" => GUIDS[:floor], "revision" => REVISIONS[6]}, input: {"blueprintId" => floor, "name" => "FloorBox", "class" => "BoxComponent"})
    transcript.mutation("blueprint.scs_component_update", {"blueprintId" => floor, "variableGuid" => GUIDS[:floor], "revision" => REVISIONS[6]}, input: {"blueprintId" => floor, "variableGuid" => GUIDS[:floor], "collisionEnabled" => "QueryAndPhysics", "collisionProfile" => "BlockAll", "boxExtent" => [1000, 1000, 25]})
    transcript.mutation("animation_blueprint.create", {"animationBlueprintId" => abp, "generatedClass" => "#{abp}_C", "revision" => REVISIONS[1]}, input: {"path" => p15["animationBlueprint"], "skeletonId" => p15["skeleton"]})
    transcript.mutation("animation.variable_ensure", {"animationBlueprintId" => abp, "revision" => REVISIONS[1]}, input: {"animationBlueprintId" => abp, "name" => "Speed", "type" => "float", "source" => "owner_planar_speed"})
    transcript.mutation("animation.state_machine_ensure", {"animationBlueprintId" => abp, "stateMachineId" => "machine", "revision" => REVISIONS[1]}, input: {"animationBlueprintId" => abp, "name" => "locomotion"})
    transcript.mutation("animation.state_ensure", {"animationBlueprintId" => abp, "stateMachineId" => "machine", "stateId" => "idle-state", "revision" => REVISIONS[1]}, input: {"animationBlueprintId" => abp, "stateMachineId" => "machine", "name" => "idle", "sequenceId" => p15["idle"]})
    transcript.mutation("animation.state_ensure", {"animationBlueprintId" => abp, "stateMachineId" => "machine", "stateId" => "moving-state", "revision" => REVISIONS[1]}, input: {"animationBlueprintId" => abp, "stateMachineId" => "machine", "name" => "moving", "sequenceId" => p15["moving"]})
    transcript.mutation("animation.transition_ensure", {"animationBlueprintId" => abp, "revision" => REVISIONS[1]}, input: {"animationBlueprintId" => abp, "stateMachineId" => "machine", "fromStateId" => "idle-state", "toStateId" => "moving-state", "expression" => "Speed > 10"})
    transcript.mutation("animation.transition_ensure", {"animationBlueprintId" => abp, "revision" => REVISIONS[1]}, input: {"animationBlueprintId" => abp, "stateMachineId" => "machine", "fromStateId" => "moving-state", "toStateId" => "idle-state", "expression" => "Speed <= 10"})
    transcript.mutation("animation.character_configure", {"characterBlueprintId" => character, "skeletalMeshId" => p15["skeletalMesh"], "animationBlueprintId" => abp, "revision" => REVISIONS[2]}, input: {"characterBlueprintId" => character, "skeletalMeshId" => p15["skeletalMesh"], "animationBlueprintId" => abp})
    transcript.mutation("navigation.bounds_ensure", {"levelId" => level, "agentKey" => "p14-nav", "boundsId" => "bounds", "location" => [0, 0, 0], "extent" => [1000, 1000, 100], "revision" => REVISIONS[8]}, input: {"levelId" => level, "agentKey" => "p14-nav", "location" => [0, 0, 0], "extent" => [1000, 1000, 100]})
    [[controller, REVISIONS[5]], [floor, REVISIONS[6]], [abp, REVISIONS[1]], [character, REVISIONS[2]]].each { |asset, revision| transcript.compile(asset, revision) }
    transcript.spawn(level, classes.fetch(floor), "floor", "actor-floor", location: [0, 0, -25])
    transcript.spawn(level, classes.fetch(character), "character", "actor-character", location: [0, 0, 100])
    transcript.spawn(level, "/Script/Engine.TargetPoint", "target", "actor-target", location: [600, 0, 100])
    [[bb, REVISIONS[3]], [tree, REVISIONS[4]], [controller, REVISIONS[5]], [floor, REVISIONS[6]], [abp, REVISIONS[1]], [character, REVISIONS[2]]].each { |asset, revision| transcript.save(asset, revision) }
    transcript.save_level(level)
    transcript.restart(level)
    transcript.read("blackboard.view", {"blackboardId" => bb, "keys" => [{"keyName" => "TargetActor", "keyType" => "Actor"}], "revision" => REVISIONS[3]}, input: {"blackboardId" => bb})
    transcript.read("behavior_tree.view", {"behaviorTreeId" => tree, "blackboardId" => bb, "nodes" => nodes.map { |id, type, key, wait| {"nodeId" => id, "nodeType" => type, "keyName" => key, "waitSeconds" => wait} }, "links" => links.each_with_index.map { |(parent, child, index), i| {"linkId" => "link-#{i}", "parentNodeId" => parent, "childNodeId" => child, "childIndex" => index} }, "revision" => REVISIONS[4]}, input: {"behaviorTreeId" => tree})
    [[controller, REVISIONS[5]], [floor, REVISIONS[6]], [character, REVISIONS[2]]].each { |asset, revision| transcript.read("blueprint.view", {"id" => asset, "generatedClass" => classes.fetch(asset), "errorCount" => 0, "warningCount" => 0, "revision" => revision}, argv: ["blueprint", "view", asset]) }
    transcript.read("blueprint.scs_view", {"blueprintId" => floor, "components" => [{"variableGuid" => GUIDS[:floor], "name" => "FloorBox", "collisionEnabled" => "QueryAndPhysics", "collisionProfile" => "BlockAll", "boxExtent" => [1000, 1000, 25]}], "revision" => REVISIONS[6]}, input: {"blueprintId" => floor})
    states = [
      {"stateId"=>"idle-state","stateGraphId"=>"idle-graph","resultNodeId"=>"idle-result","sequencePlayerNodeId"=>"idle-player","name"=>"idle","sequenceId"=>p15["idle"],"skeletonId"=>p15["skeleton"],"initial"=>true},
      {"stateId"=>"moving-state","stateGraphId"=>"moving-graph","resultNodeId"=>"moving-result","sequencePlayerNodeId"=>"moving-player","name"=>"moving","sequenceId"=>p15["moving"],"skeletonId"=>p15["skeleton"],"initial"=>false}
    ]
    transitions = [
      {"transitionId"=>"idle-moving","transitionGraphId"=>"idle-moving-graph","resultNodeId"=>"idle-moving-result","variableGetterNodeId"=>"idle-moving-getter","comparisonNodeId"=>"idle-moving-compare","fromStateId"=>"idle-state","toStateId"=>"moving-state","expression"=>"Speed > 10"},
      {"transitionId"=>"moving-idle","transitionGraphId"=>"moving-idle-graph","resultNodeId"=>"moving-idle-result","variableGetterNodeId"=>"moving-idle-getter","comparisonNodeId"=>"moving-idle-compare","fromStateId"=>"moving-state","toStateId"=>"idle-state","expression"=>"Speed <= 10"}
    ]
    variable = {"variableId"=>"speed-variable","bindingId"=>"speed-binding","name"=>"Speed","type"=>"float","source"=>"owner_planar_speed","updateGraphId"=>"update-graph","eventNodeId"=>"update-event","ownerNodeId"=>"owner-node","velocityNodeId"=>"velocity-node","planarSpeedNodeId"=>"planar-speed-node","setterNodeId"=>"speed-setter"}
    transcript.read("animation.graph_view", {"animationBlueprintId"=>abp,"skeletonId"=>p15["skeleton"],"generatedClass"=>"#{abp}_C","animGraphId"=>"anim-graph","rootNodeId"=>"anim-root","variables"=>[variable],"stateMachines"=>[{"stateMachineId"=>"machine","stateMachineGraphId"=>"machine-graph","entryNodeId"=>"machine-entry","name"=>"locomotion","initialStateId"=>"idle-state","states"=>states,"transitions"=>transitions}],"revision"=>REVISIONS[1]}, input: {"animationBlueprintId"=>abp})
    transcript.read("animation.character_view", {"characterBlueprintId"=>character,"meshComponentId"=>"mesh-component","skeletalMeshId"=>p15["skeletalMesh"],"skeletonId"=>p15["skeleton"],"animationMode"=>"AnimationBlueprint","animationBlueprintId"=>abp,"animClass"=>"#{abp}_C","revision"=>REVISIONS[2]}, input: {"characterBlueprintId"=>character})
    details = {"actor-floor" => {"class" => classes.fetch(floor), "location" => [0, 0, -25]}, "actor-character" => {"class" => classes.fetch(character), "location" => [0, 0, 100]}, "actor-target" => {"class" => "/Script/Engine.TargetPoint", "location" => [600, 0, 100]}}
    transcript.actor_views(%w[actor-floor actor-character actor-target], details)
    transcript.mutation("animation.variable_ensure", {"animationBlueprintId" => abp, "revision" => REVISIONS[1]}, input: {"animationBlueprintId" => abp, "name" => "Speed", "type" => "float", "source" => "owner_planar_speed"}, changed: false, persistence: "unchanged")
    transcript.read("navigation.build", {"levelId" => level, "ticketId" => "ticket", "state" => "scheduled", "changed" => true, "revision" => REVISIONS[8]}, input: {"levelId" => level})
    transcript.read("navigation.status", {"ticketId" => "ticket", "levelId" => level, "state" => "succeeded", "terminal" => true, "revision" => REVISIONS[8]}, input: {"ticketId" => "ticket"})
    transcript.read("navigation.path_query", {"levelId" => level, "start" => [0, 0, 100], "target" => [600, 0, 100], "reachable" => true, "partial" => false, "pathLength" => 600, "points" => [[0, 0, 100], [600, 0, 100]], "revision" => REVISIONS[8]}, input: {"levelId" => level, "start" => [0, 0, 100], "target" => [600, 0, 100]})
    2.times do |index|
      sid = "animation-session-#{index + 1}"
      transcript.play_start(sid)
      weights = lambda { |active| [{"stateId"=>"idle-state","name"=>"idle","weight"=>active == "idle" ? 1 : 0},{"stateId"=>"moving-state","name"=>"moving","weight"=>active == "moving" ? 1 : 0}] }
      animation = lambda do |state, speed|
        {"sessionId"=>sid,"characterId"=>"actor-character","meshComponentId"=>"mesh-component","skeletalMeshId"=>p15["skeletalMesh"],"skeletonId"=>p15["skeleton"],"animationBlueprintId"=>abp,"animClass"=>"#{abp}_C","animationInstanceId"=>"animation-instance-#{index + 1}","stateMachineId"=>"machine","speed"=>speed,"ownerPlanarSpeed"=>speed,"stateWeights"=>weights.call(state),"activeStateId"=>"#{state}-state","activeStateName"=>state,"activeTransition"=>{"active"=>false,"transitionId"=>nil,"fromStateId"=>nil,"toStateId"=>nil,"elapsedFraction"=>nil},"revision"=>REVISIONS[9]}
      end
      transcript.read("play.animation_observe", animation.call("idle", 0), input: {"sessionId"=>sid,"characterId"=>"actor-character","animationBlueprintId"=>abp,"stateMachineId"=>"machine"})
      transcript.mutation("play.ai_target_set", {"sessionId"=>sid,"pawnId"=>"actor-character","controllerId"=>"controller","keyName"=>"TargetActor","targetActorId"=>"actor-target","targetLocation"=>[600,0,100],"changed"=>true,"restarted"=>true,"revision"=>REVISIONS[9]}, input: {"sessionId"=>sid,"pawnId"=>"actor-character","keyName"=>"TargetActor","targetActorId"=>"actor-target"}, persistence: "unchanged")
      common_ai = {"sessionId"=>sid,"pawnId"=>"actor-character","controllerId"=>"controller","targetActorId"=>"actor-target","targetLocation"=>[600,0,100],"behaviorTreeId"=>tree,"possessed"=>true,"behavior"=>"running","blackboardValues"=>[{"keyName"=>"TargetActor","keyType"=>"Actor","valueActorId"=>"actor-target"}]}
      transcript.read("play.ai_observe", common_ai.merge("pawnLocation"=>[100,0,100],"destination"=>[600,0,100],"distanceToTarget"=>500,"moveStatus"=>"moving","activeNodeIds"=>["move"]), input: {"sessionId"=>sid,"pawnId"=>"actor-character","keyName"=>"TargetActor"})
      transcript.read("play.animation_observe", animation.call("moving", 200), input: {"sessionId"=>sid,"characterId"=>"actor-character","animationBlueprintId"=>abp,"stateMachineId"=>"machine"})
      transcript.read("play.ai_observe", common_ai.merge("pawnLocation"=>[590,0,100],"destination"=>nil,"distanceToTarget"=>10,"moveStatus"=>"reached","activeNodeIds"=>["wait"]), input: {"sessionId"=>sid,"pawnId"=>"actor-character","keyName"=>"TargetActor"})
      transcript.read("play.animation_observe", animation.call("idle", 0), input: {"sessionId"=>sid,"characterId"=>"actor-character","animationBlueprintId"=>abp,"stateMachineId"=>"machine"})
      transcript.play_stop(sid)
    end
    transcript.pipeline
    transcript.write_evidence
  end

  def self.build(root, repo, job)
    {"unknown-project-orientation" => method(:orientation), "interaction-loop" => method(:interaction), "ui-state-loop" => method(:ui), "ai-navigation-loop" => method(:ai), "animation-state-loop" => method(:animation)}.fetch(job).call(root, repo)
  end

  def self.reject!(name, transcript, expected)
    transcript.validate
    raise "invalid fixture accepted: #{name}"
  rescue StandardError => error
    raise if error.message == "invalid fixture accepted: #{name}"
    raise "#{name}: expected #{expected.inspect}, got #{error.message.inspect}" unless error.message == expected
  end

  def self.run(repo)
    valid = 0
    invalid = 0
    inventory_path = File.join(__dir__, "p16-outcome-negative-cases.txt")
    expected_names = File.readlines(inventory_path, chomp: true).reject(&:empty?)
    Dir.mktmpdir("p16-outcome-suite-") do |root|
      OutcomeValidator::JOBS.each do |job|
        transcript = build(root, repo, job)
        transcript.validate
        composite = case job
                    when "interaction-loop" then ["blueprint.interface_ensure", "/Game/MagiP12/BP_Interactable.BP_Interactable#/Game/MagiP12/BPI_Interact.BPI_Interact"]
                    when "ai-navigation-loop" then ["play.ai_target_set", "ai-session-1#actor-pawn#TargetActor#actor-target"]
                    when "animation-state-loop" then ["animation.transition_ensure", "/Game/MagiP15/ABP_P15Animation.ABP_P15Animation#machine#idle-state#moving-state"]
                    end
        if composite
          receipt = transcript.output(transcript.ops.fetch(composite.first).first).fetch("receipt")
          raise "positive composite target #{job}" unless receipt["target"] == composite.last && receipt.dig("verification", "target") == composite.last
        end
        valid += 1
      end
      portable = build(root, repo, "unknown-project-orientation")
      recorded_dir = "/tmp/p16-recorded-orientation"
      portable.instance_variable_get(:@rows).each do |row|
        path = row.fetch("stdoutPath")
        row["stdoutPath"] = path.sub(portable.dir, recorded_dir)
        output = File.read(path).gsub(portable.dir, recorded_dir)
        File.write(path, output)
      end
      portable.rewrite_ledger
      OutcomeValidator.new(portable.dir, portable.job, repo, recorded_dir).validate
      valid += 1
      portable.instance_variable_get(:@rows).first["stdoutPath"] = "#{recorded_dir}-sibling/stdout-1"
      portable.rewrite_ledger
      begin
        OutcomeValidator.new(portable.dir, portable.job, repo, recorded_dir).validate
        raise "portable sibling path accepted"
      rescue StandardError => error
        raise if error.message == "portable sibling path accepted"
      end
      cases = []
      cases << ["orientation-help", "unknown-project-orientation", "help empty", ->(t) { File.write(t.instance_variable_get(:@rows)[0]["stdoutPath"], "") }]
      cases << ["orientation-total", "unknown-project-orientation", "missing capability.search", ->(t) { t.edit_output(t.ops["capability.search"].first) { |value| value["total"] = 78 } }]
      cases << ["metrics", "interaction-loop", "metrics", ->(t) { t.edit_outcome { |value| value["metrics"]["retries"] = 1 } }]
      cases << ["nonzero-exit", "interaction-loop", "failures", ->(t) { t.edit_row(1) { |row| row["exit"] = 1 } }]
      cases << ["references", "interaction-loop", "reference mismatch compile", ->(t) { t.edit_outcome { |value| value["references"]["compile"].delete_at(0) } }]
      cases << ["interface-save", "interaction-loop", "save /Game/MagiP12/BPI_Interact.BPI_Interact", ->(t) { t.edit_row(t.ops["asset.save"].first) { |row| row["argv"][2] = "/Game/Wrong.Wrong" } }]
      cases << ["save-revision", "interaction-loop", ->(t) { "receipt #{t.ops['asset.save'][1]}" }, ->(t) { t.edit_output(t.ops["asset.save"][1]) { |value| value["result"]["revision"] = REVISIONS[9] } }]
      cases << ["level-order", "interaction-loop", "reference mismatch compile", ->(t) { t.move_before(t.ops["level.save"].first, t.ops["actor.spawn"].first) }]
      cases << ["level-open", "interaction-loop", "unknown operation level.current", ->(t) { t.edit_row(t.ops["level.open"].first) { |row| row["argv"] = %w[level current] } }]
      cases << ["actor-view", "interaction-loop", "actor identity", ->(t) { t.edit_output(t.ops["actor.view"].first) { |value| value["id"] = "wrong" } }]
      cases << ["noop-revision", "interaction-loop", ->(t) { "receipt #{t.ops['blueprint.interface_ensure'].last}" }, ->(t) { t.edit_output(t.ops["blueprint.interface_ensure"].last) { |value| value["result"]["revision"] = REVISIONS[9] } }]
      cases << ["interaction-actor", "interaction-loop", "interaction runtime OverlapBox", ->(t) { t.edit_output(t.ops["play.component_observe"].first) { |value| value["actorId"] = "wrong" } }]
      cases << ["rejected-input", "interaction-loop", "input rejected", ->(t) { t.edit_output(t.ops["play.input"].first) { |value| value["result"]["accepted"] = false } }]
      cases << ["ui-event-target", "ui-state-loop", "missing widget.event_ensure", ->(t) { t.edit_input(t.ops["widget.event_ensure"].first) { |value| value["actions"][0]["targetWidgetId"] = "wrong" } }]
      cases << ["ui-instance-reset", "ui-state-loop", "ui reset", ->(t) { first = t.output(t.ops["play.ui_observe"].first)["instanceId"]; t.ops["play.ui_observe"].last(2).each { |sequence| t.edit_output(sequence) { |value| value["instanceId"] = first } } }]
      cases << ["ai-link", "ai-navigation-loop", "missing behavior_tree.connect", ->(t) { t.edit_input(t.ops["behavior_tree.connect"].first) { |value| value["childNodeId"] = "wrong" } }]
      cases << ["ai-reset", "ai-navigation-loop", "ai runtime", ->(t) { t.edit_output(t.ops["play.ai_observe"].last) { |value| value["targetActorId"] = "actor-target" } }]
      cases << ["animation-transition", "animation-state-loop", "missing animation.transition_ensure", ->(t) { sequence = t.ops["animation.transition_ensure"].first; t.edit_input(sequence) { |value| value["fromStateId"] = "moving-state" }; t.edit_output(sequence) { |value| target = [value.dig("result", "animationBlueprintId"), "machine", "moving-state", "moving-state"].join("#"); value["receipt"]["target"] = target; value["receipt"]["verification"]["target"] = target } }]
      cases << ["composite-receipt-target", "animation-state-loop", ->(t) { "receipt #{t.ops['animation.transition_ensure'].first}" }, ->(t) { t.edit_output(t.ops["animation.transition_ensure"].first) { |value| value["receipt"]["target"] = "wrong"; value["receipt"]["verification"]["target"] = "wrong" } }]
      cases << ["animation-same-character", "animation-state-loop", "animation ai runtime 1", ->(t) { t.edit_output(t.ops["play.ai_observe"].first) { |value| value["pawnId"] = "wrong-character" } }]
      cases << ["animation-target-after-moving", "animation-state-loop", "animation ai runtime 1", ->(t) { set = t.ops["play.ai_target_set"].first; moving = t.ops["play.ai_observe"].first; t.move_before(set, moving + 1) }]
      cases << ["animation-initial-order", "animation-state-loop", "animation runtime 1", ->(t) { initial = t.ops["play.animation_observe"].first; set = t.ops["play.ai_target_set"].first; t.move_before(initial, set) }]
      cases << ["animation-moving-order", "animation-state-loop", "animation runtime 1", ->(t) { moving = t.ops["play.animation_observe"][1]; t.move_before(moving, t.ops["play.ai_observe"].first) }]
      cases << ["animation-reached-order", "animation-state-loop", "animation runtime 1", ->(t) { reached = t.ops["play.ai_observe"][1]; final_idle = t.ops["play.animation_observe"][2]; t.move_before(reached, final_idle) }]
      cases << ["animation-final-order", "animation-state-loop", "animation runtime 1", ->(t) { final_idle = t.ops["play.animation_observe"][2]; t.move_before(final_idle, t.ops["play.ai_observe"][1]) }]
      cases << ["destructive-actor-delete", "interaction-loop", "destructive operation actor.delete", ->(t) { t.add("actor.delete", {"id" => "actor-target", "changed" => true, "dirtyPackages" => [], "savedPackages" => [], "revision" => REVISIONS[9]}, input: {"id" => "actor-target"}); t.rewrite_ledger }]
      cases << ["unknown-operation", "interaction-loop", "unknown operation not-a-capability", ->(t) { t.add("not-a-capability", {}); t.rewrite_ledger }]
      cases << ["unreferenced-operation", "interaction-loop", "missing positional operation.view", ->(t) { t.add("operation.view", {}); t.rewrite_ledger }]
      cases << ["durable-view-wrong-id", "interaction-loop", ->(t) { "operation.view id #{t.ops['operation.view'].first}" }, ->(t) { t.edit_row(t.ops["operation.view"].first) { |row| row["argv"][2] = "unknown-operation-id" } }]
      cases << ["durable-view-altered-native", "interaction-loop", ->(t) { "operation.view receipt #{t.ops['operation.view'].first}" }, ->(t) { t.edit_output(t.ops["operation.view"].first) { |value| value["target"] = "wrong" } }]
      cases << ["durable-view-altered-process", "interaction-loop", ->(t) { "operation.view receipt #{t.ops['operation.view'].last}" }, ->(t) { t.edit_output(t.ops["operation.view"].last) { |value| value["status"] = "failed" } }]
      cases << ["durable-view-missing", "interaction-loop", "operation.view count", ->(t) { t.delete(t.ops["operation.view"].last) }]
      cases << ["durable-view-extra", "interaction-loop", "operation.view count", ->(t) { t.duplicate(t.ops["operation.view"].first) }]
      cases << ["durable-view-offline-order", "interaction-loop", ->(t) { row = t.instance_variable_get(:@rows).find { |candidate| candidate["argv"][0, 2] == %w[operation view] && candidate["argv"][2] == t.output(t.ops["play.stop"].last).dig("receipt", "operationId") }; "operation.view offline #{row.fetch('sequence')}" }, ->(t) { t.move_before(t.ops["editor.stop"].last, t.ops["operation.view"][7]); t.refresh_references }]
      cases << ["durable-view-process-order", "interaction-loop", ->(t) { row = t.instance_variable_get(:@rows).find { |candidate| candidate["argv"][0, 2] == %w[operation view] && candidate["argv"][2] == "proc-cook" }; "operation.view order #{row.fetch('sequence')}" }, ->(t) { t.move_before(t.ops["project.package"].first, t.ops["operation.view"][7]); t.refresh_references }]
      cases << ["catalog-required-input", "interaction-loop", "blueprint.interface_ensure input required blueprintId", ->(t) { t.edit_input(t.ops["blueprint.interface_ensure"].first) { |value| value.delete("blueprintId") } }]
      cases << ["catalog-invalid-field", "interaction-loop", "blueprint.interface_ensure input additionalProperties", ->(t) { t.edit_input(t.ops["blueprint.interface_ensure"].first) { |value| value["unexpected"] = true } }]
      cases << ["receipt-changed-contradiction", "interaction-loop", ->(t) { "receipt #{t.ops["play.start"].first}" }, ->(t) { t.edit_output(t.ops["play.start"].first) { |value| value["receipt"]["changed"] = !value["result"]["changed"] } }]
      cases << ["receipt-revision-contradiction", "interaction-loop", ->(t) { "receipt #{t.ops["play.start"].first}" }, ->(t) { t.edit_output(t.ops["play.start"].first) { |value| value["receipt"]["revision"] = REVISIONS[1] } }]
      cases << ["runtime-before-start", "interaction-loop", ->(t) { "session window #{t.ops['play.start'].first}" }, ->(t) { t.move_before(t.ops["play.component_observe"].first, t.ops["play.start"].first) }]
      cases << ["runtime-after-stop", "interaction-loop", ->(t) { "session window #{t.ops['play.stop'].first + 1}" }, ->(t) { t.move_before(t.ops["play.component_observe"].first, t.ops["play.stop"].first + 1) }]
      cases << ["pipeline-status", "interaction-loop", ->(t) { "operation.view receipt #{t.ops['operation.view'].last}" }, ->(t) { t.edit_output(t.ops["project.package"].first) { |value| value["operation"]["status"] = "failed" } }]
      cases << ["pipeline-root", "interaction-loop", "pipeline artifact", ->(t) { t.edit_output(t.ops["project.cook"].first) { |value| value["artifacts"][0]["path"] = "/tmp/wrong" } }]
      cases << ["missing-utoc", "interaction-loop", "package utoc", ->(t) { FileUtils.rm_f(Dir.glob(File.join(t.dir, "package", "**", "*.utoc")).first) }]
      cases << ["extra-cook", "interaction-loop", "pipeline count", ->(t) { t.duplicate(t.ops["project.cook"].first) }]
      raise "fixture case inventory" unless cases.map(&:first) == expected_names
      cases.each do |name, job, expected, mutation|
        transcript = build(root, repo, job)
        mutation.call(transcript)
        expected = expected.call(transcript) if expected.respond_to?(:call)
        reject!(name, transcript, expected)
        invalid += 1
      end
    end
    raise "fixture coverage" unless valid == 6 && invalid == expected_names.length
    puts "P1.6 outcome validator self-test: #{valid} valid, #{invalid} invalid: PASS"
  end
end
