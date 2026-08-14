#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "json"
require "tmpdir"
require "open3"
require "digest"
require "shellwords"
require "time"
require_relative "p16-outcome"
require_relative "p16-outcome-fixtures"
require_relative "p16-evidence"
require_relative "p16-provenance"
require_relative "p16-run-inventory"
require_relative "p16-revalidate"

module P16AgentLifecycleFixtures
  JOBS = OutcomeValidator::JOBS
  SCRIPT = File.expand_path("../evaluate-p1.6-agents.sh", __dir__)
  module_function

  def fail!(message)
    raise message
  end

  def sha(path)
    Digest::SHA256.file(path).hexdigest
  end

  def command(env, *args)
    stdout, stderr, status = Open3.capture3(env, SCRIPT, *args)
    return [stdout, stderr] if status.success?

    fail!("public lifecycle command failed (#{status.exitstatus}): #{args.join(' ')}\n#{stdout}#{stderr}")
  end

  def expect_failure(env, expected, *args)
    stdout, stderr, status = Open3.capture3(env, SCRIPT, *args)
    fail!("negative accepted: #{args.join(' ')}") if status.success?
    fail!("negative failed for wrong reason: #{args.join(' ')}\n#{stdout}#{stderr}") unless "#{stdout}#{stderr}".include?(expected)
  end

  def copy_tree(source, destination)
    FileUtils.mkdir_p(destination)
    Dir.children(source).each do |name|
      FileUtils.cp_r(File.join(source, name), File.join(destination, name), preserve: true)
    end
  end
  def session_for(directory, job, original_project, wrapper)
    rows = File.readlines(File.join(directory, "ledger.jsonl"), chomp: true).map { |line| JSON.parse(line) }
    sid = "fixture-#{job}"
    user = {"event_type" => "user_input", "session_id" => sid, "cwd" => original_project}
    assistant = {"event_type" => "assistant_output", "session_id" => sid, "cwd" => original_project}
    calls = []
    calls << ["read", {"paths" => ["skill://axi"], "offset" => 1, "limit" => 2000}, "read"]
    rows.each_with_index { |row, index| calls << ["bash", {"command" => Shellwords.join([wrapper] + row.fetch("argv")), "timeout" => 30}, "bash#{index}"] }
    outcome = File.join(directory, "agent-outcome.json")
    calls << ["write", {"path" => outcome, "content" => File.read(outcome)}, "write"]
    call_events = calls.map { |name, arguments, id| {"event_type" => "tool_call", "session_id" => sid, "cwd" => original_project, "payload" => {"id" => id, "name" => name, "arguments" => arguments}} }
    result_events = calls.map { |_name, _arguments, id| {"event_type" => "tool_result", "session_id" => sid, "cwd" => original_project, "payload" => {"call_id" => id, "result" => {"success" => true}}} }
    events = [user] + call_events + result_events + [assistant]
    File.write(File.join(directory, "session.jsonl"), events.map { |event| JSON.generate(event) }.join("\n") << "\n")
  end

  def prepare_jobs(run, repo, plugin_hash)
    staging = Dir.mktmpdir("p16-fixtures-")
    previous_second = 0
    JOBS.each do |job|
      transcript = P16OutcomeFixtures.build(staging, repo, job)
      directory = File.join(run, "jobs", job)
      copy_tree(transcript.dir, directory)
      project = File.join(directory, "project", "MagiUnrealAXIPackageFixture.uproject")
      FileUtils.mkdir_p(File.join(directory, "bin"))
      wrapper = File.join(directory, "bin", "axi-record")
      File.write(wrapper, "#!/bin/sh\nexit 0\n"); File.chmod(0o555, wrapper)
      rows = File.readlines(File.join(directory, "ledger.jsonl"), chomp: true).map { |line| JSON.parse(line) }
      rows.each_with_index do |row, index|
        recorded_source = row.fetch("stdoutPath")
        source = File.join(directory, File.basename(recorded_source))
        target = File.join(directory, "ledger.jsonl.#{index + 1}.stdout")
        FileUtils.cp(source, target) unless source == target
        rewritten = File.binread(target).gsub(transcript.dir, directory)
        File.binwrite(target, rewritten)
        row["stdoutPath"] = target
        row["stderrPath"] = File.join(directory, "ledger.jsonl.#{index + 1}.stderr")
        File.write(row["stderrPath"], "")
        row["cwd"] = File.dirname(project)
        row["argv"] = ["--project", project, "--engine", "/fixture/engine", "--format", "json"] + row["argv"] unless row["argv"].include?("--help")
        row["argv"] = row["argv"].map { |value| value.gsub(transcript.dir, directory) }
        second = previous_second + index + 1
        started = Time.utc(2024, 1, 1, 0, 0, 0) + second
        row.merge!("started" => started.iso8601(9), "ended" => (started + Rational(1, 1_000_000_000)).iso8601(9), "stdoutBytes" => File.size(target), "stderrBytes" => 0,
                   "stdoutSha256" => sha(target), "stderrSha256" => sha(row["stderrPath"]), "estimatedTokens" => (File.size(target) + 3) / 4,
                   "homeCategory" => "actual-account-home")
      end
      previous_second += rows.length + 1
      File.write(File.join(directory, "ledger.jsonl"), rows.map { |row| JSON.generate(row) }.join("\n") << "\n")
      if job != JOBS.first
        plugin_root = File.join(directory, "project", "Plugins", "MagiUnrealAXI")
        dylib = File.join(plugin_root, "libUnrealEditor-MagiUnrealAXI.dylib")
        FileUtils.mkdir_p(plugin_root); FileUtils.cp("/usr/bin/true", dylib)
        File.write(File.join(directory, "plugin.sha256"), "#{plugin_hash}\n")
        File.write(File.join(directory, "plugin-tree.sha256"), "libUnrealEditor-MagiUnrealAXI.dylib\t#{sha(dylib)}\n")
        File.write(File.join(directory, "plugin-install.json"), JSON.generate("plugin" => {"installed" => true, "managed" => true, "compatible" => true, "path" => plugin_root}, "projectDescriptor" => {"path" => project}) << "\n")
      end
      File.write(File.join(directory, "PROMPT.txt"), "fixture #{job}\n")
      FileUtils.mkdir_p(File.join(directory, "project", "Config"))
      File.write(File.join(directory, "project", "Config", "DefaultEngine.ini"), "[fixture]\n")
      File.write(File.join(directory, "project", "Config", "DefaultGame.ini"), "[fixture]\n") if job != JOBS.first
      if job == "animation-state-loop"
        File.write(File.join(directory, "seed-source.txt"), "fixture-seed\n")
        File.write(File.join(directory, "seed-destination.txt"), "fixture-seed\n")
      end
      immutable = P16Revalidate.expected_immutable(job)
      File.write(File.join(directory, "immutable.sha256"), immutable.map { |path| "#{path}\t#{sha(File.join(directory, path))}" }.join("\n") << "\n")
      File.write(File.join(directory, "sequence"), "#{rows.length}\n")
      session_for(directory, job, File.dirname(project), wrapper)
    end
  ensure
    FileUtils.rm_rf(staging) if staging
  end

  def build_combined(root, repo, artifact, binary_hash, plugin_hash, commit, tree)
    combined = File.join(root, "combined")
    FileUtils.mkdir_p(combined)
    inventory_raw = P16Provenance.inventory(repo)
    inventory_map = P16Provenance.inventory_map(inventory_raw)
    identity = {"artifactSha256" => artifact, "binarySha256" => binary_hash, "pluginSha256" => plugin_hash,
                "catalogSha256" => inventory_map.fetch("capabilities/catalog.json"), "manifestSha256" => inventory_map.fetch("tests/unreal/p1.6-manifest.json")}
    inventory = File.join(combined, "source-inventory.tsv"); File.write(inventory, inventory_raw)
    identity_path = File.join(combined, "provenance-identities.json"); File.write(identity_path, P16Provenance.json(identity) << "\n")
    provenance = File.join(combined, "provenance.json")
    source = {"commit" => commit, "tree" => tree, "inventorySha256" => Digest::SHA256.hexdigest(inventory_raw)}
    File.write(provenance, P16Provenance.json("schema" => P16Provenance::SCHEMA, "source" => source, "identities" => identity) << "\n")
    provenance_sha = sha(provenance)
    File.write(File.join(combined, "summary.txt"), "artifactSha256=#{artifact}\nprovenanceSha256=#{provenance_sha}\n")
    tree_sha = capture_stdout { EvidenceTree.write(combined, File.join(combined, "evidence-tree.json")) }.strip
    [combined, tree_sha, provenance_sha]
  end

  def capture_stdout
    original = $stdout
    reader, writer = IO.pipe
    $stdout = writer
    yield
    writer.close
    reader.read
  ensure
    $stdout = original
    writer&.close unless writer&.closed?
    reader&.close unless reader&.closed?
  end
  def lipo_environment(root, plugin_hash)
    shim_directory = File.join(root, "fixture-bin")
    FileUtils.mkdir_p(shim_directory)
    shim = File.join(shim_directory, "lipo")
    script = <<~SH
      #!/bin/sh
      set -eu
      fixture_root=#{Shellwords.escape(root)}
      expected_sha=#{plugin_hash}
      if [ "$#" -eq 2 ] && [ "$1" = "-archs" ] && [ -f "$2" ]; then
        case "$2" in
          "$fixture_root"/*)
            actual_sha=$(shasum -a 256 "$2" | awk '{print $1}')
            if [ "$actual_sha" = "$expected_sha" ]; then
              printf 'arm64\n'
              exit 0
            fi
            ;;
        esac
      fi
      exec /usr/bin/lipo "$@"
    SH
    File.write(shim, script)
    File.chmod(0o755, shim)
    {"PATH" => "#{shim_directory}:#{ENV.fetch('PATH')}"}
  end

  def build_finalized(repo, root, artifact: "a" * 64, exact_binary_source: "/usr/bin/true", negative_checks: false)
    root = File.realpath(root)
    cache = File.join(root, "cache")
    run = File.join(cache, "run.fixture")
    FileUtils.mkdir_p(run)
    exact_binary = File.join(run, "synthetic-arm64")
    FileUtils.cp(exact_binary_source, exact_binary)
    binary_hash = sha(exact_binary)
    plugin_hash = sha("/usr/bin/true")
    commit = `git -C #{Shellwords.escape(repo)} rev-parse HEAD`.strip
    tree = `git -C #{Shellwords.escape(repo)} rev-parse HEAD^{tree}`.strip
    combined, tree_sha, provenance_sha = build_combined(root, repo, artifact, binary_hash, plugin_hash, commit, tree)
    prepare_jobs(run, repo, plugin_hash)
    File.write(File.join(run, "artifact.sha256"), "#{artifact}\n")
    File.write(File.join(run, "plugin.sha256"), "#{plugin_hash}\n")
    File.write(File.join(run, "exact-binary"), "#{exact_binary}\n")
    File.write(File.join(run, "exact-binary.sha256"), "#{binary_hash}\n")
    File.write(File.join(run, "kickoff.txt"), "synthetic lifecycle fixture\n")
    status_before = `git -C #{Shellwords.escape(repo)} status --porcelain=v1 --untracked-files=all`
    File.write(File.join(run, "git-status.before"), status_before)
    immutable = JOBS.to_h { |job| [job, sha(File.join(run, "jobs", job, "immutable.sha256"))] }
    manifest = {"phase" => "P1.6", "status" => "prepared", "run" => run, "artifactSha256" => artifact, "binarySha256" => binary_hash, "pluginSha256" => plugin_hash, "combinedEvidence" => combined, "combinedEvidenceTreeSha256" => tree_sha, "combinedProvenanceSha256" => provenance_sha, "engineRoot" => "/fixture/engine", "sourceCommit" => commit, "sourceTree" => tree, "immutableSha256s" => immutable, "jobs" => JOBS}
    File.write(File.join(run, "manifest.json"), JSON.generate(manifest) << "\n")
    fixture_artifact = "a" * 64
    env = lipo_environment(root, plugin_hash).merge("P16_CACHE_ROOT" => cache, "UE_ENGINE_ROOT" => "/fixture/engine")
    env["P16_EXPECTED_ARTIFACT_SHA256"] = artifact if artifact != fixture_artifact
    expect_failure(env, "jobs must record sequentially", "record", run, JOBS[1], File.join(run, "jobs", JOBS[1], "session.jsonl")) if negative_checks
    JOBS.each do |job|
      source = File.join(run, "jobs", job, "session.jsonl")
      session = File.join(root, "#{job}.session.jsonl")
      FileUtils.mv(source, session)
      command(env, "record", run, job, session)
    end
    expect_failure(env, "run missing", "finalize", File.join(cache, "missing")) if negative_checks
    command(env, "finalize", run)
    inventory_sha = sha(File.join(run, "run-inventory.json"))
    if negative_checks
      expect_failure(env.merge("P16_EXPECTED_RUN_INVENTORY_SHA256" => "b" * 64), "inventory SHA mismatch", "revalidate", run, combined)
      expect_failure(env, "run already finalized", "finalize", run)
    end
    {run: run, combined: combined, inventory_sha: inventory_sha, env: env, status_before: status_before, manifest: manifest}
  end

  def run(repo)
    root = File.realpath(Dir.mktmpdir("p16-agent-lifecycle-"))
    begin
      fixture = build_finalized(repo, root, negative_checks: true)
      run = fixture.fetch(:run)
      combined = fixture.fetch(:combined)
      inventory_sha = fixture.fetch(:inventory_sha)
      env = fixture.fetch(:env)
      status_before = fixture.fetch(:status_before)
      relocated = File.join(root, "relocated-run"); relocated_combined = File.join(root, "relocated-combined")
      FileUtils.cp_r(run, relocated); FileUtils.cp_r(combined, relocated_combined); FileUtils.rm_rf(run); FileUtils.rm_rf(combined)
      fail!("original lifecycle evidence retained before relocation test") if File.exist?(run) || File.exist?(combined)
      clean_relocated = File.join(root, "relocated-run-clean"); FileUtils.cp_r(relocated, clean_relocated)
      command(env.merge("P16_EXPECTED_RUN_INVENTORY_SHA256" => inventory_sha), "revalidate", relocated, relocated_combined)
      File.write(File.join(relocated, "summary.txt"), "tampered\n")
      expect_failure(env.merge("P16_EXPECTED_RUN_INVENTORY_SHA256" => inventory_sha), "run contents do not match inventory", "revalidate", relocated, relocated_combined)
      tampered_combined = File.join(root, "relocated-combined-tampered"); FileUtils.cp_r(relocated_combined, tampered_combined)
      File.write(File.join(tampered_combined, "summary.txt"), "tampered\n")
      expect_failure(env.merge("P16_EXPECTED_RUN_INVENTORY_SHA256" => inventory_sha), "file set or content mismatch", "revalidate", clean_relocated, tampered_combined)
      fail!("repository changed during lifecycle self-test") unless `git -C #{Shellwords.escape(repo)} status --porcelain=v1 --untracked-files=all` == status_before
      puts "P1.6 agent lifecycle self-test: PASS prepare=1 record=5 finalize=1 relocate=1 revalidate=1"
    ensure
      if ENV["P16_KEEP_LIFECYCLE_FIXTURE"] == "1"
        warn "P1.6 lifecycle fixture retained: #{root}"
      else
        FileUtils.rm_rf(root)
      end
    end
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    P16AgentLifecycleFixtures.run(File.expand_path("../../..", __dir__))
  rescue StandardError => error
    warn error.message
    exit 1
  end
end
