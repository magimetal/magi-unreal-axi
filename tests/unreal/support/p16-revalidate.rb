#!/usr/bin/env ruby
# frozen_string_literal: true

require "digest"
require "find"
require "json"
require "pathname"
require "shellwords"
require "tmpdir"
require "time"
require_relative "p16-outcome"
require_relative "p16-run-inventory"
require_relative "p16-evidence"
require_relative "p16-provenance"

module P16Revalidate
  module_function

  JOBS = OutcomeValidator::JOBS
  LEDGER_KEYS = %w[argv cwd ended estimatedTokens exit homeCategory sequence started stderrBytes stderrPath stderrSha256 stdoutBytes stdoutPath stdoutSha256].freeze
  RECORD_KEYS = %w[ended job recordedAt sessionSha256 started].freeze
  OUTCOME_KEYS = %w[expectedFailureSequences job metrics references requiredCategories status].freeze
  MAX_METADATA_BYTES = 64 * 1024 * 1024
  MAX_ARTIFACT_FILE_BYTES = 2 * 1024 * 1024 * 1024
  MAX_TOTAL_BYTES = 16 * 1024 * 1024 * 1024
  MAX_LINES = 100_000
  MAX_FILES = 100_000
  MAX_NODES = 200_000
  MAX_DEPTH = 64
  MAX_PATH_BYTES = 4096

  def fail(message)
    raise "P1.6 revalidation failed: #{message}"
  end

  def regular(path, allow_hardlink: false)
    stat = File.lstat(path)
    stat.file? && !stat.symlink? && (allow_hardlink || stat.nlink == 1)
  rescue Errno::ENOENT
    false
  end

  def bounded_read(path, label, max = MAX_METADATA_BYTES)
    fail "missing #{label}" unless regular(path)
    fail "oversized #{label}" if File.size(path) > max
    data = File.binread(path, max + 1)
    fail "oversized #{label}" if data.bytesize > max
    data
  end

  def lines(path, label, max = MAX_LINES)
    data = bounded_read(path, label)
    result = data.lines(chomp: true)
    fail "too many lines #{label}" if result.length > max
    result
  end

  def parse_json(path, label)
    JSON.parse(bounded_read(path, label))
  rescue JSON::ParserError => e
    fail "invalid JSON #{label}: #{e.message}"
  end

  def sha(path)
    digest = Digest::SHA256.new
    File.open(path, "rb") { |io| while (chunk = io.read(1024 * 1024)); digest.update(chunk); end }
    digest.hexdigest
  end
  def credential_placeholder?(value)
    value.match?(/\A(?:\*{3}|REDACTED|\[REDACTED\]|<redacted>|\$\{?[A-Za-z_][A-Za-z0-9_]*\}?)\z/i)
  end

  def retained_credential?(path)
    patterns = [
      /["']?Authorization["']?\s*:\s*["']?\s*(?:Basic|Bearer)\s+["']?\s*([A-Za-z0-9._~+\/=:-]+)/i,
      /["']?(?:X-API-Key|Api-Key)["']?\s*:\s*["']?\s*([^\s"',}]+)/i,
      /["']?(?:OPENAI_API_KEY|ANTHROPIC_API_KEY|GITHUB_TOKEN|GH_TOKEN|AWS_SECRET_ACCESS_KEY)["']?\s*[=:]\s*["']?\s*([^\s"',}]+)/i,
      /\b((?:gh[pousr]_[A-Za-z0-9_]{20,}|sk-[A-Za-z0-9_-]{20,}))\b/
    ]
    File.open(path, "rb") do |io|
      tail = "".b
      while (chunk = io.read(1024 * 1024))
        data = tail + chunk
        return true if patterns.any? { |pattern| data.scan(pattern).any? { |capture| !credential_placeholder?(capture.first) } }
        tail = data.byteslice(-512, 512) || data
      end
    end
    false
  end

  def absolute_under(path, root)
    value = Pathname.new(path).cleanpath.to_s
    base = Pathname.new(root).cleanpath.to_s
    value == base || value.start_with?("#{base}#{File::SEPARATOR}")
  end

  def map_path(path, original, run)
    fail "path is not absolute" unless path.is_a?(String) && Pathname.new(path).absolute?
    fail "path outside original execution root: #{path}" unless absolute_under(path, original)
    relative = Pathname.new(path).cleanpath.relative_path_from(Pathname.new(original).cleanpath).to_s
    mapped = File.expand_path(File.join(run, relative))
    fail "mapped path escapes downloaded run" unless absolute_under(mapped, run)
    mapped
  end

  def timestamp(value, label)
    fail "invalid timestamp #{label}" unless value.is_a?(String) && value.match?(/\A\d{4}-\d\d-\d\dT.*\.\d{9}Z\z/)
    Time.iso8601(value)
  rescue ArgumentError, TypeError
    fail "invalid timestamp #{label}"
  end

  def exact_file_tree(run)
    files = 0; nodes = 0; total = 0
    Find.find(run) do |path|
      next if path == run
      stat = File.lstat(path)
      fail "symlink or special path #{path}" if stat.symlink? || (!stat.file? && !stat.directory?) || (stat.file? && stat.nlink != 1)
      relative = Pathname.new(path).relative_path_from(Pathname.new(run)).to_s
      nodes += 1
      fail "too many paths" if nodes > MAX_NODES
      fail "path too deep #{path}" if relative.split(File::SEPARATOR).length > MAX_DEPTH
      fail "path too long #{path}" if relative.bytesize > MAX_PATH_BYTES
      next unless stat.file?
      files += 1; total += stat.size
      fail "too many files" if files > MAX_FILES
      fail "evidence too large" if total > MAX_TOTAL_BYTES
      fail "file too large #{path}" if stat.size > MAX_ARTIFACT_FILE_BYTES
    end
  end

  def verify_hash_list(path, root, label)
    entries = lines(path, label); fail "empty #{label}" if entries.empty?
    seen = {}
    entries.each do |line|
      relative, expected = line.split("\t", -1)
      fail "invalid #{label} entry" unless relative && expected&.match?(/\A[0-9a-f]{64}\z/) && relative != "" && !Pathname.new(relative).absolute? && !relative.split("/").include?("..") && !relative.include?("\\")
      fail "duplicate #{label} entry" if seen[relative]
      seen[relative] = true
      target = File.expand_path(File.join(root, relative))
      fail "#{label} path escapes" unless absolute_under(target, root) && regular(target)
      fail "#{label} hash #{relative}" unless sha(target) == expected
    end
    seen.keys
  end

  def verify_sidecar(row, kind, original, run, job)
    path = map_path(row.fetch("#{kind}Path"), original, run)
    fail "#{kind} sidecar #{job}" unless regular(path) && row["#{kind}Bytes"] == File.size(path) && row["#{kind}Sha256"] == sha(path)
  end

  def expected_immutable(job)
    paths = %w[PROMPT.txt bin/axi-record project/Config/DefaultEngine.ini]
    unless job == "unknown-project-orientation"
      paths += %w[plugin-install.json plugin.sha256 plugin-tree.sha256 project/Config/DefaultGame.ini]
      paths += %w[seed-source.txt seed-destination.txt] if job == "animation-state-loop"
    end
    paths.sort
  end
  def verify_session(path, original, run, job, rows, outcome_path)
    events = lines(path, "#{job}/session.jsonl").map { |line| JSON.parse(line) }
    fail "session empty #{job}" if events.empty?
    fail "session event schema #{job}" unless events.all? { |e| e.is_a?(Hash) && e["session_id"].is_a?(String) && !e["session_id"].empty? && e["cwd"] == File.join(original, "jobs", job, "project") }
    fail "session identity #{job}" unless events.map { |e| e["session_id"] }.uniq.length == 1
    calls = events.select { |e| e["event_type"] == "tool_call" }; results = events.select { |e| e["event_type"] == "tool_result" }
    ids = calls.map { |e| e.dig("payload", "id") }
    fail "call ids #{job}" unless ids.all? { |id| id.is_a?(String) && !id.empty? } && ids.uniq.length == ids.length && ids == results.map { |e| e.dig("payload", "call_id") }
    fail "failed tool result #{job}" unless results.all? { |e| e["success"] == true || e.dig("payload", "result", "success") == true }
    user_indexes = events.each_index.select { |index| events[index]["event_type"] == "user_input" }
    assistant_indexes = events.each_index.select { |index| events[index]["event_type"] == "assistant_output" }
    fail "session boundaries #{job}" unless user_indexes.length == 1 && assistant_indexes.length == 1 && user_indexes.first < (events.index(calls.first) || events.length) && assistant_indexes.first == events.length - 1
    bash = 0; reads = 0; writes = 0
    calls.each do |call|
      name = call.dig("payload", "name"); args = call.dig("payload", "arguments")
      fail "tool schema #{job}" unless %w[bash read write].include?(name) && args.is_a?(Hash)
      case name
      when "read"
        reads += 1
        valid_bounds = (!args.key?("offset") || args["offset"] == 1) && (!args.key?("limit") || args["limit"] == 2000)
        fail "skill read #{job}" unless reads == 1 && args["paths"] == ["skill://axi"] && (args.keys - %w[paths offset limit]).empty? && valid_bounds
      when "bash"
        allowed = args.keys.sort == ["command"] || args.keys.sort == %w[command timeout].sort
        fail "bash correlation #{job}" unless allowed && (!args.key?("timeout") || (args["timeout"].is_a?(Integer) && args["timeout"].between?(1, 300))) && bash < rows.length
        expected = Shellwords.join([File.join(run, "jobs", job, "bin", "axi-record")] + rows[bash]["argv"])
        original_command = Shellwords.join([File.join(original, "jobs", job, "bin", "axi-record")] + rows[bash]["argv"])
        fail "bash command #{job}" unless [expected, original_command].include?(args["command"])
        bash += 1
      when "write"
        writes += 1; fail "final outcome write #{job}" unless writes == 1 && call.equal?(calls.last) && args.keys.sort == %w[content path].sort && map_path(args["path"], original, run) == outcome_path && args["content"] == bounded_read(outcome_path, "agent-outcome.json")
      end
    end
    fail "tool counts #{job}" unless reads == 1 && writes == 1 && bash == rows.length
  rescue JSON::ParserError => e
    fail "invalid session #{job}: #{e.message}"
  end

  def validate(run, combined, expected_inventory = ENV["P16_EXPECTED_RUN_INVENTORY_SHA256"], snapshot: true)
    run = File.expand_path(run); fail "RUN must be directory" unless File.directory?(run) && !File.symlink?(run)
    fail "P16_EXPECTED_RUN_INVENTORY_SHA256 required" unless expected_inventory.is_a?(String) && expected_inventory.match?(/\A[0-9a-f]{64}\z/)
    if snapshot
      Dir.mktmpdir("p16-revalidate-snapshot-") do |parent|
        materialized = File.join(parent, "run")
        P16RunInventory.materialize_verified(run, expected_inventory, materialized)
        return validate(materialized, combined, expected_inventory, snapshot: false)
      end
    end
    P16RunInventory.verify(run, expected_inventory)
    exact_file_tree(run)
    manifest = parse_json(File.join(run, "manifest.json"), "manifest.json")
    fail "manifest keys" unless manifest.keys.sort == %w[artifactSha256 binarySha256 combinedEvidence combinedEvidenceTreeSha256 combinedProvenanceSha256 engineRoot immutableSha256s jobs phase pluginSha256 run sourceCommit sourceTree status].sort
    original = manifest["run"]
    fail "manifest original run" unless original.is_a?(String) && Pathname.new(original).absolute? && File.expand_path(original) == original && original != run
    fail "manifest identities" unless manifest["phase"] == "P1.6" && manifest["status"] == "prepared" && manifest["jobs"] == JOBS && manifest["engineRoot"].is_a?(String) && Pathname.new(manifest["engineRoot"]).absolute? && manifest["sourceCommit"].match?(/\A[0-9a-f]{40}\z/) && manifest["sourceTree"].match?(/\A[0-9a-f]{40}\z/) && %w[artifactSha256 binarySha256 pluginSha256 combinedEvidenceTreeSha256 combinedProvenanceSha256].all? { |key| manifest[key].is_a?(String) && manifest[key].match?(/\A[0-9a-f]{64}\z/) }
    engine_root = manifest.fetch("engineRoot")
    immutable_seals = manifest["immutableSha256s"]
    fail "immutable seals" unless immutable_seals.is_a?(Hash) && immutable_seals.keys.sort == JOBS.sort && immutable_seals.values.all? { |value| value.is_a?(String) && value.match?(/\A[0-9a-f]{64}\z/) }
    fail "combined evidence" unless manifest["combinedEvidence"].is_a?(String) && Pathname.new(manifest["combinedEvidence"]).absolute?
    combined = File.expand_path(combined); fail "COMBINED must be directory" unless File.directory?(combined) && !File.symlink?(combined)
    combined_tree_path = File.join(combined, "evidence-tree.json")
    combined_tree = parse_json(combined_tree_path, "combined/evidence-tree.json")
    fail "combined tree identity" unless combined_tree["treeSha256"] == manifest["combinedEvidenceTreeSha256"]
    EvidenceTree.verify(combined, combined_tree_path)
    fail "combined credential retained" if Find.find(combined).select { |p| regular(p) }.any? { |p| retained_credential?(p) }
    P16Provenance.verify_portable(
      File.join(combined, "source-inventory.tsv"),
      File.join(combined, "provenance.json"),
      File.join(combined, "provenance-identities.json"),
      expected_provenance_sha: manifest["combinedProvenanceSha256"],
      expected_source_commit: manifest["sourceCommit"],
      expected_source_tree: manifest["sourceTree"],
      expected_identities: {"artifactSha256" => manifest["artifactSha256"], "binarySha256" => manifest["binarySha256"], "pluginSha256" => manifest["pluginSha256"]}
    )
    summary_lines = lines(File.join(run, "summary.txt"), "summary.txt")
    pairs = summary_lines.map { |line| line.split("=", 2) }
    fail "summary lines" unless pairs.all? { |pair| pair.length == 2 && !pair[0].empty? } && pairs.map(&:first).uniq.length == pairs.length
    summary = pairs.to_h
    required = %w[phase status artifactSha256 binarySha256 combinedPluginSha256 agentPluginSha256s combinedEvidence combinedEvidenceTreeSha256 combinedProvenanceSha256 sourceCommit sourceTree jobs metrics tokenScan]
    fail "summary keys" unless summary.keys.sort == required.sort
    fail "summary identity" unless summary.values_at("phase", "status", "artifactSha256", "binarySha256", "combinedPluginSha256", "combinedEvidence", "combinedEvidenceTreeSha256", "combinedProvenanceSha256", "sourceCommit", "sourceTree", "jobs", "tokenScan") == ["P1.6", "passed", manifest["artifactSha256"], manifest["binarySha256"], manifest["pluginSha256"], manifest["combinedEvidence"], manifest["combinedEvidenceTreeSha256"], manifest["combinedProvenanceSha256"], manifest["sourceCommit"], manifest["sourceTree"], "5/5-passed-sequential", "passed"]
    summary_metrics = JSON.parse(summary["metrics"]); fail "summary metrics" unless summary_metrics.is_a?(Hash)
    fail "runtime secret retained" if Find.find(run).any? { |p| File.basename(p) == "token" || File.basename(p) == "bridge-v1.json" }
    fail "credential retained" if Find.find(run).select { |p| regular(p) }.any? { |p| retained_credential?(p) }
    %w[artifact.sha256 exact-binary exact-binary.sha256 plugin.sha256 git-status.before kickoff.txt].each { |name| fail "missing #{name}" unless regular(File.join(run, name)) }
    fail "artifact identity" unless bounded_read(File.join(run, "artifact.sha256"), "artifact.sha256", 128).strip == manifest["artifactSha256"]
    exact_recorded = bounded_read(File.join(run, "exact-binary"), "exact-binary", MAX_PATH_BYTES).strip
    exact_local = map_path(exact_recorded, original, run)
    exact_hash = bounded_read(File.join(run, "exact-binary.sha256"), "exact-binary.sha256", 128).strip
    fail "binary identity" unless exact_hash == manifest["binarySha256"] && regular(exact_local) && sha(exact_local) == exact_hash
    fail "plugin identity" unless bounded_read(File.join(run, "plugin.sha256"), "plugin.sha256", 128).strip == manifest["pluginSha256"]
    previous_end = nil; plugin_hashes = {}; metrics = {"cliCalls" => 0, "stdoutBytes" => 0, "stderrBytes" => 0, "estimatedTokens" => 0, "retries" => 0, "avoidableRetries" => 0, "structuredOutputFailures" => 0}
    JOBS.each_with_index do |job, index|
      dir = File.join(run, "jobs", job); %w[record.json session.jsonl ledger.jsonl immutable.sha256 sequence agent-outcome.json PROMPT.txt].each { |name| fail "missing #{job}/#{name}" unless regular(File.join(dir, name)) }
      immutable = verify_hash_list(File.join(dir, "immutable.sha256"), dir, "immutable #{job}")
      fail "immutable seal #{job}" unless sha(File.join(dir, "immutable.sha256")) == immutable_seals.fetch(job)
      fail "immutable inventory #{job}" unless immutable.sort == expected_immutable(job)
      record = parse_json(File.join(dir, "record.json"), "#{job}/record.json"); fail "record schema #{job}" unless record.keys.sort == RECORD_KEYS.sort && record["job"] == job
      rows = lines(File.join(dir, "ledger.jsonl"), "#{job}/ledger.jsonl").map { |line| JSON.parse(line) }; fail "empty ledger #{job}" if rows.empty?
      starts = rows.map.with_index { |row, i| timestamp(row["started"], "#{job} started #{i}") }; ends = rows.map.with_index { |row, i| timestamp(row["ended"], "#{job} ended #{i}") }
      project = File.join(original, "jobs", job, "project", "MagiUnrealAXIPackageFixture.uproject")
      project_dir = File.dirname(project)
      rows.each_with_index do |row, i|
        numeric = %w[stdoutBytes stderrBytes estimatedTokens exit].all? { |key| row[key].is_a?(Integer) && row[key] >= 0 }
        argv = row["argv"]
        invocation = if argv&.include?("--help")
          argv.count("--help") == 1
        else
          {"--project" => project, "--engine" => engine_root, "--format" => "json"}.all? { |flag, value| argv&.count(flag) == 1 && argv[argv.index(flag) + 1] == value }
        end
        fail "ledger interval #{job}" unless row.keys.sort == LEDGER_KEYS.sort && row["sequence"] == i + 1 && argv.is_a?(Array) && !argv.empty? && argv.all? { |value| value.is_a?(String) && !value.empty? } && row["cwd"] == project_dir && numeric && row["exit"] == 0 && row["exit"] <= 255 && row["homeCategory"] == "actual-account-home" && row["estimatedTokens"] == (row["stdoutBytes"] + 3) / 4 && starts[i] <= ends[i] && (i.zero? || starts[i] >= ends[i - 1]) && invocation
      end
      sequence = Integer(bounded_read(File.join(dir, "sequence"), "#{job}/sequence", 64).strip) rescue fail("sequence #{job}")
      fail "sequence #{job}" unless sequence == rows.length
      fail "job order #{job}" if previous_end && starts.min <= previous_end
      fail "record interval #{job}" unless timestamp(record["started"], "record started") == starts.min && timestamp(record["ended"], "record ended") == ends.max && record["sessionSha256"] == sha(File.join(dir, "session.jsonl"))
      Time.iso8601(record["recordedAt"]) rescue fail("record timestamp #{job}")
      rows.each { |row| metrics["cliCalls"] += 1; %w[stdout stderr].each { |kind| verify_sidecar(row, kind, original, run, job); metrics["#{kind}Bytes"] += row["#{kind}Bytes"] }; metrics["estimatedTokens"] += row["estimatedTokens"] }
      outcome = parse_json(File.join(dir, "agent-outcome.json"), "#{job}/agent-outcome.json"); fail "outcome schema #{job}" unless outcome.keys.sort == OUTCOME_KEYS.sort && outcome["job"] == job && outcome["status"] == "passed" && outcome["expectedFailureSequences"] == []
      outcome.fetch("metrics").each { |key, value| fail "outcome metric #{job}" unless metrics.key?(key) && value.is_a?(Integer) && value >= 0; metrics[key] += value }
      verify_session(File.join(dir, "session.jsonl"), original, run, job, rows, File.join(dir, "agent-outcome.json"))
      OutcomeValidator.new(dir, job, File.expand_path(File.join(__dir__, "..", "..", "..")), File.join(original, "jobs", job)).validate
      if index.positive?
        install = parse_json(File.join(dir, "plugin-install.json"), "#{job}/plugin-install.json")
        expected_plugin = File.join(original, "jobs", job, "project", "Plugins", "MagiUnrealAXI")
        expected_project = File.join(original, "jobs", job, "project", "MagiUnrealAXIPackageFixture.uproject")
        fail "plugin install #{job}" unless install.dig("plugin", "installed") == true && install.dig("plugin", "managed") == true && install.dig("plugin", "compatible") == true && install.dig("plugin", "path") == expected_plugin && install.dig("projectDescriptor", "path") == expected_project
        plugin_root = File.join(dir, "project", "Plugins", "MagiUnrealAXI")
        inventory = verify_hash_list(File.join(dir, "plugin-tree.sha256"), plugin_root, "plugin tree #{job}").sort
        actual = Dir.glob(File.join(plugin_root, "**", "*"), File::FNM_DOTMATCH).select { |p| regular(p) }.map { |p| Pathname.new(p).relative_path_from(Pathname.new(plugin_root)).to_s }.sort
        fail "plugin inventory #{job}" unless inventory == actual
        dylibs = inventory.select { |p| File.basename(p) == "libUnrealEditor-MagiUnrealAXI.dylib" }; plugin_hash = bounded_read(File.join(dir, "plugin.sha256"), "#{job}/plugin.sha256", 128).strip
        fail "plugin binary #{job}" unless dylibs.length == 1 && plugin_hash.match?(/\A[0-9a-f]{64}\z/) && plugin_hash == manifest["pluginSha256"] && plugin_hash == sha(File.join(plugin_root, dylibs.first)); plugin_hashes[job] = plugin_hash
      end
      previous_end = ends.max
    end
    expected_plugins = JOBS[1..].map { |job| "#{job}:#{plugin_hashes.fetch(job)}" }.join(",")
    fail "plugin summary" unless summary["agentPluginSha256s"] == expected_plugins
    fail "metrics summary" unless summary_metrics == metrics
    P16RunInventory.verify(run, expected_inventory)
    puts "P1.6 agent evidence revalidation: PASS (read-only snapshot; downloaded run=#{run}; combined=#{combined})"
  end

  def self_test
    Dir.mktmpdir("p16-revalidate-") do |dir|
      nested = File.join(dir, "nested"); Dir.mkdir(nested)
      probe = File.join(nested, "probe"); File.write(probe, "ok")
      fail "self-test read" unless bounded_read(probe, "probe") == "ok" && sha(probe) == Digest::SHA256.hexdigest("ok")
      exact_file_tree(dir)
      original = "/tmp/original-run"
      fail "self-test map" unless map_path("#{original}/jobs/x", original, dir) == File.join(dir, "jobs", "x")
      begin map_path("#{original}-sibling/x", original, dir); fail "self-test sibling accepted"; rescue RuntimeError => e; raise if e.message.end_with?("accepted"); end
      File.symlink("probe", File.join(nested, "link")); fail "self-test symlink" if regular(File.join(nested, "link"))
      begin exact_file_tree(dir); fail "self-test tree symlink accepted"; rescue RuntimeError => e; raise if e.message.end_with?("accepted"); end
      File.unlink(File.join(nested, "link")); File.link(probe, File.join(nested, "hardlink"))
      begin exact_file_tree(dir); fail "self-test hardlink accepted"; rescue RuntimeError => e; raise if e.message.end_with?("accepted"); end
    end
    Dir.mktmpdir("p16-revalidate-credentials-") do |dir|
      [
        "OPENAI_API_KEY=REDACTED\n",
        "OPENAI_API_KEY=<redacted>\n",
        "GH_TOKEN=***\n",
        "AWS_SECRET_ACCESS_KEY=[REDACTED]\n",
        "OPENAI_API_KEY=$TOKEN\n",
        "{\"OPENAI_API_KEY\":\"${TOKEN}\"}\n"
      ].each_with_index do |content, index|
        clean = File.join(dir, "clean-#{index}"); File.write(clean, content)
        fail "self-test clean credential false positive #{index}" if retained_credential?(clean)
      end
      [
        "Authorization: Bearer abc+/=._-\n",
        "authorization: Basic YWxhZGRpbjpvcGVuc2VzYW1l=\n",
        "Authorization: \"Bearer quoted-token\"\n",
        "Authorization: Bearer \"quoted-value\"\n",
        "{\"Authorization\":\"Bearer json-token\"}\n",
        "X-API-Key: secret-value\n",
        "OPENAI_API_KEY=sk-secret\n",
        "OPENAI_API_KEY=\"sk-quoted\"\n",
        "{\"OPENAI_API_KEY\":\"sk-json\"}\n",
        "GH_TOKEN='gh-secret'\n",
        "{\"GH_TOKEN\":\"ghp_secret\"}\n",
        "AWS_SECRET_ACCESS_KEY=aws-secret\n",
        "token: ghp_abcdefghijklmnopqrstuvwxyz123456\n"
      ].each_with_index do |content, index|
        path = File.join(dir, "credential-#{index}"); File.write(path, content)
        fail "self-test retained credential #{index}" unless retained_credential?(path)
      end
      boundary = File.join(dir, "boundary")
      File.binwrite(boundary, "x" * (1024 * 1024 - 10) + "Authorization: Bearer boundary+/=\n")
      fail "self-test boundary credential" unless retained_credential?(boundary)

    end
    puts "P1.6 revalidator self-test: PASS"
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    if ARGV == ["--self-test"] || ARGV == ["self-test"]
      P16Revalidate.self_test
    elsif ARGV.length == 2
      P16Revalidate.validate(ARGV[0], ARGV[1])
    elsif ARGV.length == 3
      P16Revalidate.validate(ARGV[0], ARGV[1], ARGV[2])
    else
      warn "usage: #{File.basename($PROGRAM_NAME)} RUN COMBINED [EXPECTED_INVENTORY_SHA256] | --self-test"
      exit 2
    end
  rescue StandardError => e
    warn e.message
    exit 1
  end
end