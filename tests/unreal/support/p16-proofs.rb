#!/usr/bin/env ruby
# frozen_string_literal: true
require "json"
require "digest"
require "pathname"
require "tmpdir"

class ProofError < StandardError; end
module P16Proofs
  SCHEMA = "magi-unreal-axi/operation-proofs/v1"
  HEX = /\A[0-9a-f]{64}\z/
  REPORT_PATH = "report/index.json"
  CONTROL = /[\x00-\x1f\x7f\u200b-\u200f\u202a-\u202e\u2060-\u206f\ufeff]/
  module_function

  def canonical(v)
    case v
    when Hash then v.keys.sort_by { |k| k.encode(Encoding::UTF_8).bytes }.to_h { |k| [k, canonical(v.fetch(k))] }
    when Array then v.map { |x| canonical(x) }
    else v end
  end
  def canonical_json(v)
    JSON.generate(canonical(v), ascii_only: false)
  end
  def canonical_file?(path, value)
    File.binread(path) == canonical_json(value) + "\n"
  end
  def exact_keys!(value, keys, label)
    raise ProofError, "#{label} keys" unless value.is_a?(Hash) && value.keys.sort == keys.sort
  end
  def json(path)
    bytes = File.binread(path)
    bytes = bytes.byteslice(3..) if bytes.start_with?("\xEF\xBB\xBF".b)
    JSON.parse(bytes)
  rescue Errno::ENOENT => e
    raise ProofError, "missing #{path}: #{e.message}"
  rescue JSON::ParserError => e
    raise ProofError, "invalid JSON #{path}: #{e.message}"
  end
  def safe_path(path)
    raise ProofError, "unsafe path #{path.inspect}" unless path.is_a?(String) && !path.empty? && path.valid_encoding? && !path.start_with?("/") && !path.include?("\\") && !path.match?(CONTROL)
    parts = path.split("/", -1)
    raise ProofError, "unsafe path #{path.inspect}" unless parts.all? { |p| !p.empty? && p != "." && p != ".." && p == p.unicode_normalize(:nfc) }
    path
  end
  def regular(root, relative)
    safe_path(relative)
    root = File.realpath(root)
    path = File.join(root, relative)
    raise ProofError, "path escapes root #{relative}" unless File.expand_path(path).start_with?("#{root}/")
    parts = Pathname.new(relative).each_filename.to_a
    current = root
    parts.each do |part|
      current = File.join(current, part)
      st = File.lstat(current)
      raise ProofError, "symlink rejected #{relative}" if st.symlink?
    end
    st = File.stat(path)
    raise ProofError, "not regular #{relative}" unless st.file? && st.nlink == 1
    path
  end
  def reject!(label)
    yield
    raise ProofError, "#{label} accepted"
  rescue ProofError => error
    raise if error.message == "#{label} accepted"
  end
  def load_inputs(plan_path, manifest_path, catalog_path, repo)
    plan = json(plan_path); manifest = json(manifest_path); catalog = json(catalog_path)
    exact_keys!(plan, %w[baseline operations p1 schema version], "plan")
    raise ProofError, "plan canonical" unless canonical_file?(plan_path, plan)
    raise ProofError, "plan schema" unless plan["schema"] == SCHEMA && plan["version"] == 1
    raise ProofError, "catalog" unless catalog.is_a?(Array) && catalog.length == 79
    expected_catalog = manifest.dig("catalog", "sha256")
    raise ProofError, "catalog hash" unless expected_catalog.is_a?(String) && expected_catalog.match?(HEX) && Digest::SHA256.file(catalog_path).hexdigest == expected_catalog
    ids = catalog.map { |x| x.fetch("id") }
    raise ProofError, "catalog duplicate" unless ids == ids.uniq
    [plan, manifest, catalog, File.realpath(repo)]
  end
  def operation_ids(plan, manifest, catalog)
    ids = plan.fetch("operations")
    raise ProofError, "operations must be sorted unique" unless ids.is_a?(Array) && ids.length == 79 && ids == ids.uniq && ids == ids.sort
    catalog_ids = catalog.map { |x| x.fetch("id") }.sort
    raise ProofError, "operation catalog mismatch" unless ids == catalog_ids
    trace = manifest.fetch("traceability"); groups = trace.values
    raise ProofError, "traceability shape" unless groups.all? { |x| x.is_a?(Array) }
    flat = groups.flatten
    raise ProofError, "traceability partition" unless flat.length == 79 && flat.uniq.length == 79 && flat.sort == ids
    baseline = trace.fetch("baseline"); p1 = manifest.fetch("p1Operations")
    baseline_proofs = plan.fetch("baseline").map { |x| x.fetch("operation") }
    p1_proofs = plan.fetch("p1").map { |x| x.fetch("operation") }
    raise ProofError, "plan baseline" unless baseline_proofs.length == 34 && baseline_proofs.uniq.length == 34 && baseline_proofs.sort == baseline.sort
    raise ProofError, "plan P1" unless p1_proofs.length == 45 && p1_proofs.uniq.length == 45 && p1_proofs.sort == p1.sort
    raise ProofError, "p1 partition" unless p1.sort == trace.fetch_values("interaction", "ui", "ai", "animation").flatten.sort && p1.uniq.length == 45 && (baseline + p1).sort == ids
  end
  def verify_plan(plan_path, manifest_path, catalog_path, repo)
    plan, manifest, catalog, repo = load_inputs(plan_path, manifest_path, catalog_path, repo)
    operation_ids(plan, manifest, catalog)
    source_checks(plan, repo, manifest)
    puts "OK plan operations=79 baseline=34 p1=45"
  end
  def source_checks(plan, repo, manifest)
    tests = manifest.fetch("automation").fetch("tests")
    seen = []
    plan.fetch("baseline").each do |proof|
      op = proof.fetch("operation"); seen << op
      test = proof.fetch("automationTest")
      raise ProofError, "automation test #{op}" unless tests.include?(test)
      source = proof.fetch("source")
      exact_keys!(source, %w[marker path], "source #{op}")
      raise ProofError, "source path #{op}" unless source["path"] == "plugin/MagiUnrealAXI/Source/MagiUnrealAXI/Private/MagiUnrealAXI.cpp"
      expected_marker = op.start_with?("capability.") ? "CapabilityMetadata(Operation)" : op
      raise ProofError, "source marker #{op}" unless source["marker"] == expected_marker
      path = regular(repo, source.fetch("path"))
      raise ProofError, "source marker #{op}" unless File.binread(path).include?(expected_marker) || (op.start_with?("capability.") && source["marker"] == op)
    end
    raise ProofError, "baseline duplicates" unless seen.uniq.length == 34
    plan.fetch("p1").each do |proof|
      exact_keys!(proof, %w[evidence operation], "P1 proof")
      op = proof.fetch("operation")
      evidence = proof.fetch("evidence")
      safe_path(evidence.fetch("path"))
      format = evidence.fetch("format")
      expected_keys = format == "automation-test" ? %w[format path required test] : %w[format path required]
      exact_keys!(evidence, expected_keys, "evidence #{op}")
      raise ProofError, "invalid evidence format #{op}" unless %w[json automation-test].include?(format)
      required = evidence.fetch("required")
      raise ProofError, "required paths #{op}" unless required.is_a?(Array) && !required.empty?
      required.each do |requirement|
        exact_keys!(requirement, %w[path type], "requirement #{op}")
        raise ProofError, "required path #{op}" unless requirement["path"].is_a?(String) && !requirement["path"].empty?
        raise ProofError, "required type #{op}" unless %w[string object array boolean integer number].include?(requirement["type"])
      end
      raise ProofError, "automation evidence path #{op}" if format == "automation-test" && evidence["path"] != REPORT_PATH
    end
  end
  def dig_path(value, path)
    path.split(".").reduce(value) { |current, key| current.is_a?(Hash) ? current[key] : nil }
  end

  def type_ok?(value, type)
    classes = {"string" => String, "object" => Hash, "array" => Array, "boolean" => [TrueClass, FalseClass], "integer" => Integer, "number" => Numeric}.fetch(type)
    classes.is_a?(Array) ? classes.include?(value.class) : value.is_a?(classes)
  end

  def validate_schema(schema, value, label)
    if schema["variants"]
      return if schema["variants"].any? do |variant|
        begin
          validate_schema(variant, value, label)
          true
        rescue ProofError
          false
        end
      end
      raise ProofError, "#{label} variants"
    end
    types = Array(schema["type"])
    valid = types.any? { |type| {"object" => value.is_a?(Hash), "array" => value.is_a?(Array), "string" => value.is_a?(String), "integer" => value.is_a?(Integer), "number" => value.is_a?(Numeric), "boolean" => value == true || value == false, "null" => value.nil?}[type] }
    raise ProofError, "#{label} type" unless valid
    return if value.nil?
    if value.is_a?(Hash)
      properties = schema.fetch("properties", {})
      raise ProofError, "#{label} additional properties" if schema["additionalProperties"] == false && (value.keys - properties.keys).any?
      schema.fetch("required", []).each { |key| raise ProofError, "#{label} missing #{key}" unless value.key?(key) }
      properties.each { |key, child| validate_schema(child, value[key], "#{label}.#{key}") if value.key?(key) }
    elsif value.is_a?(Array)
      raise ProofError, "#{label} min items" if schema["minItems"] && value.length < schema["minItems"]
      raise ProofError, "#{label} max items" if schema["maxItems"] && value.length > schema["maxItems"]
      value.each { |item| validate_schema(schema.fetch("items", {}), item, label) }
    elsif value.is_a?(String)
      raise ProofError, "#{label} min length" if schema["minLength"] && value.length < schema["minLength"]
      raise ProofError, "#{label} max length" if schema["maxLength"] && value.length > schema["maxLength"]
    elsif value.is_a?(Numeric)
      raise ProofError, "#{label} minimum" if schema["minimum"] && value < schema["minimum"]
      raise ProofError, "#{label} maximum" if schema["maximum"] && value > schema["maximum"]
    end
    raise ProofError, "#{label} enum" if schema["enum"] && !schema["enum"].include?(value)
  end
  def validate_mutation(root, evidence_relative, op, entry, receipt_schema, result, value)
    receipt = value["receipt"]
    validate_schema(receipt_schema, receipt, "#{op} receipt")
    raise ProofError, "receipt #{op}" unless receipt["operation"] == op && receipt["state"] == "completed" && receipt.dig("verification", "matched") == true
    raise ProofError, "revision #{op}" unless result["revision"].is_a?(String) && result["revision"].match?(HEX) && receipt["revision"] == result["revision"] && receipt.dig("verification", "observedRevision") == result["revision"]
    expected_readback = entry.dig("verification", "readback") || "operation.view"
    raise ProofError, "readback #{op}" unless receipt.dig("verification", "readback") == expected_readback
    raise ProofError, "transaction #{op}" unless receipt["transaction"] == entry["transactionBehavior"] && receipt["reversibility"] == entry["reversibility"]
    operation_id = receipt["operationId"]
    raise ProofError, "operation id #{op}" unless operation_id.is_a?(String) && operation_id.match?(/\A[A-Za-z0-9._:-]{1,128}\z/)
    receipt_relative = [File.dirname(evidence_relative), "operation-#{operation_id}.json"].reject { |part| part == "." }.join("/")
    durable = json(regular(root, receipt_relative))
    validate_schema(receipt_schema, durable, "#{op} durable receipt")
    raise ProofError, "durable receipt #{op}" unless durable == receipt
  end
  def verify_evidence(root, plan_path, manifest_path, catalog_path, repo)
    plan, manifest, catalog, repo = load_inputs(plan_path, manifest_path, catalog_path, repo)
    operation_ids(plan, manifest, catalog); source_checks(plan, repo, manifest)
    index = regular(root, "p1-operation-traceability.json")
    rows = json(index)
    raise ProofError, "traceability index canonical" unless canonical_file?(index, rows)
    raise ProofError, "traceability index" unless rows.is_a?(Array) && rows.length == 45 && rows.all? { |row| row.is_a?(Hash) && row.keys.sort == %w[evidence operation proof sha256].sort }
    row_ops = rows.map { |row| row.fetch("operation") }
    raise ProofError, "traceability index order" unless row_ops == row_ops.sort
    raise ProofError, "traceability index operations" unless row_ops.uniq.length == 45 && row_ops == manifest.fetch("p1Operations").sort
    by_op = rows.to_h { |row| [row.fetch("operation"), row] }
    report = json(regular(root, REPORT_PATH))
    successful_tests = report.fetch("tests").select { |test| %w[Success SuccessWithWarnings].include?(test["state"]) && test["errors"] == 0 }.map { |test| test["fullTestPath"] }
    plan.fetch("baseline").each do |proof|
      raise ProofError, "baseline automation #{proof['operation']}" unless successful_tests.include?(proof.fetch("automationTest"))
    end
    receipt_schema = catalog.find { |entry| entry["id"] == "operation.view" }.fetch("outputSchema")
    plan.fetch("p1").each do |proof|
      op = proof.fetch("operation")
      evidence = proof.fetch("evidence")
      row = by_op.fetch(op) { raise ProofError, "missing evidence index #{op}" }
      raise ProofError, "evidence path #{op}" unless row.fetch("evidence") == evidence.fetch("path")
      path = regular(root, evidence.fetch("path"))
      digest = Digest::SHA256.file(path).hexdigest
      raise ProofError, "evidence hash #{op}" unless row.fetch("sha256") == digest && row.fetch("sha256").match?(HEX)
      if evidence.fetch("format") == "automation-test"
        test = evidence.fetch("test")
        raise ProofError, "automation proof #{op}" unless row["proof"] == test && successful_tests.include?(test)
        next
      end
      raise ProofError, "proof kind #{op}" unless row["proof"] == "json"
      value = json(path)
      raise ProofError, "evidence object #{op}" unless value.is_a?(Hash)
      result = value["result"] || value
      entry = catalog.find { |candidate| candidate["id"] == op }
      validate_schema(entry.fetch("outputSchema"), result, "#{op} output")
      revision = result["revision"]
      raise ProofError, "revision #{op}" unless revision.is_a?(String) && revision.match?(HEX)
      evidence.fetch("required").each do |requirement|
        required_path = requirement.fetch("path")
        required_value = if required_path.start_with?("result.")
                           dig_path(result, required_path.delete_prefix("result."))
                         else
                           dig_path(value, required_path)
                         end
        raise ProofError, "required evidence #{op}:#{required_path}" unless required_value != nil && type_ok?(required_value, requirement.fetch("type"))
      end
      next unless entry["mutates"] == true
      validate_mutation(root, evidence.fetch("path"), op, entry, receipt_schema, result, value)
    end
    puts "OK evidence operations=79 baseline=34 p1=45"
  end
  def self_test
    repo = File.realpath(File.join(__dir__, "..", "..", ".."))
    plan_path = File.join(repo, "tests", "unreal", "p1.6-operation-proofs.json")
    manifest_path = File.join(repo, "tests", "unreal", "p1.6-manifest.json")
    catalog_path = File.join(repo, "capabilities", "catalog.json")
    plan, manifest, catalog, = load_inputs(plan_path, manifest_path, catalog_path, repo)
    Dir.mktmpdir("p16-proofs-") do |dir|
      reject!("unsafe path") { safe_path("../x") }
      File.write(File.join(dir, "x"), "x")
      raise ProofError, "regular check" unless regular(dir, "x")
      File.symlink("x", File.join(dir, "link"))
      reject!("symlink") { regular(dir, "link") }
      noncanonical = File.join(dir, "noncanonical.json")
      File.write(noncanonical, "{\"b\":1,\"a\":2}\n")
      reject!("noncanonical") { raise ProofError, "noncanonical" unless canonical_file?(noncanonical, json(noncanonical)) }
      duplicate = Marshal.load(Marshal.dump(plan))
      duplicate["p1"][1] = Marshal.load(Marshal.dump(duplicate["p1"][0]))
      reject!("duplicate P1 proof") { operation_ids(duplicate, manifest, catalog) }
      entry = catalog.find { |candidate| candidate["id"] == "blueprint.create" }
      receipt_schema = catalog.find { |candidate| candidate["id"] == "operation.view" }.fetch("outputSchema")
      revision = "a" * 64
      receipt = {"operationId"=>"self-test-1","operation"=>"blueprint.create","state"=>"completed","projectId"=>"sha256:#{'b' * 64}","editorPid"=>1,"target"=>"/Game/Test.Test","changed"=>true,"transaction"=>"atomic","reversibility"=>"source-control","dirtyPackages"=>["/Game/Test"],"savedPackages"=>[],"revision"=>revision,"persistence"=>"dirty","verification"=>{"readback"=>"blueprint.graph_view","observedRevision"=>revision,"target"=>"/Game/Test.Test","matched"=>true}}
      value = {"result"=>{"revision"=>revision},"receipt"=>receipt}
      File.write(File.join(dir, "operation-self-test-1.json"), JSON.generate(receipt))
      validate_mutation(dir, "evidence.json", "blueprint.create", entry, receipt_schema, value["result"], value)
      reject!("revision tamper") { validate_mutation(dir, "evidence.json", "blueprint.create", entry, receipt_schema, {"revision"=>"z" * 64}, value) }
      File.write(File.join(dir, "operation-self-test-1.json"), JSON.generate(receipt.merge("state"=>"failed")))
      reject!("durable receipt tamper") { validate_mutation(dir, "evidence.json", "blueprint.create", entry, receipt_schema, value["result"], value) }
      expected = Digest::SHA256.hexdigest("evidence")
      reject!("hash tamper") { raise ProofError, "hash" unless "0" * 64 == expected }
    end
    puts "self-test OK"
  end
end

begin
  case ARGV[0]
  when "verify-plan" then raise "usage" unless ARGV.length == 5; P16Proofs.verify_plan(*ARGV.drop(1))
  when "verify" then raise "usage" unless ARGV.length == 6; P16Proofs.verify_evidence(*ARGV.drop(1))
  when "self-test" then raise "usage" unless ARGV.length == 1; P16Proofs.self_test
  else warn "usage: verify-plan PLAN MANIFEST CATALOG REPO | verify EVIDENCE_ROOT PLAN MANIFEST CATALOG REPO | self-test"; exit 2 end
rescue StandardError => e
  warn "error: #{e.message}"; exit 1
end
