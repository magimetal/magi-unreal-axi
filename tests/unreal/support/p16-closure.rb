#!/usr/bin/env ruby
# frozen_string_literal: true

require "digest"
require "fileutils"
require "json"
require "open3"
require "securerandom"
require "tmpdir"
require_relative "p16-agent-lifecycle-fixtures"
require_relative "p16-archive"
require_relative "p16-evidence"
require_relative "p16-provenance"
require_relative "p16-revalidate"
require_relative "p16-run-inventory"

module P16Closure
  SCHEMA = "magi-unreal-axi/p1.6-closure/v1"
  REVIEW_SCHEMA = "magi-unreal-axi/p1.6-independent-review/v1"
  SHA = /\A[0-9a-f]{64}\z/.freeze
  COMMIT = /\A[0-9a-f]{40}\z/.freeze
  REVIEWER = /\A[a-z0-9][a-z0-9._-]{0,127}\z/.freeze
  REVIEW_KEYS = %w[findings reviewerId schema status subject].freeze
  SUBJECT_KEYS = %w[artifactSha256 binarySha256 combinedEvidenceTreeSha256 combinedProvenanceSha256 pluginSha256 runInventorySha256 sourceCommit sourceTree].freeze
  CLOSURE_KEYS = %w[certificationComplete identities phase review schema status].freeze
  IDENTITY_KEYS = (SUBJECT_KEYS + %w[reviewSha256]).sort.freeze
  REVIEW_KEYS_NESTED = %w[reviewerId status].freeze
  MAX_JSON_BYTES = 1024 * 1024
  module_function

  def canonical(value)
    case value
    when Hash then value.keys.sort_by { |key| key.encode(Encoding::UTF_8).bytes }.to_h { |key| [key, canonical(value.fetch(key))] }
    when Array then value.map { |item| canonical(item) }
    else value
    end
  end

  def canonical_json(value)
    JSON.generate(canonical(value), ascii_only: false)
  end

  def fail!(message)
    raise "P1.6 closure failed: #{message}"
  end

  def sha(path)
    digest = Digest::SHA256.new
    File.open(path, File::RDONLY | File::NOFOLLOW) do |io|
      while (chunk = io.read(1024 * 1024))
        digest.update(chunk)
      end
    end
    digest.hexdigest
  rescue Errno::ELOOP
    fail! "symlink rejected: #{path}"
  end

  def regular(path)
    stat = File.lstat(path)
    stat.file? && !stat.symlink? && stat.nlink == 1
  rescue Errno::ENOENT
    false
  end

  def stable_read(path, label, max_bytes = MAX_JSON_BYTES)
    fail! "#{label} must be regular and unlinked" unless regular(path)
    File.open(path, File::RDONLY | File::NOFOLLOW) do |io|
      before = io.stat
      fail! "oversized #{label}" if before.size > max_bytes
      raw = io.read(max_bytes + 1)
      fail! "oversized #{label}" if raw.bytesize > max_bytes
      after = io.stat
      identity = ->(stat) { [stat.dev, stat.ino, stat.size, stat.mtime.to_r, stat.ctime.to_r, stat.nlink] }
      fail! "#{label} changed while reading" unless identity.call(before) == identity.call(after)
      raw
    end
  rescue Errno::ELOOP
    fail! "symlink rejected: #{label}"
  end

  def parse_canonical(path, label)
    raw = stable_read(path, label)
    value = JSON.parse(raw)
    fail! "#{label} must be a canonical object" unless value.is_a?(Hash) && raw == canonical_json(value) << "\n"
    [value, raw]
  rescue JSON::ParserError => error
    fail! "invalid #{label}: #{error.message}"
  end
  def parse_object(path, label)
    raw = stable_read(path, label)
    value = JSON.parse(raw)
    fail! "#{label} must be an object" unless value.is_a?(Hash)
    [value, raw]
  rescue JSON::ParserError => error
    fail! "invalid #{label}: #{error.message}"
  end

  def hex(value, label)
    fail! "invalid #{label}" unless value.is_a?(String) && value.match?(SHA)
    value
  end

  def trust_roots
    commit = ENV.fetch("P16_EXPECTED_SOURCE_COMMIT")
    reviewer = ENV.fetch("P16_EXPECTED_REVIEWER_ID")
    fail! "invalid source commit" unless commit.match?(COMMIT)
    fail! "invalid reviewer id" unless reviewer.match?(REVIEWER)
    {
      "artifactSha256" => hex(ENV.fetch("P16_EXPECTED_ARTIFACT_SHA256"), "artifact SHA"),
      "combinedEvidenceTreeSha256" => hex(ENV.fetch("P16_EXPECTED_COMBINED_EVIDENCE_TREE_SHA256"), "combined tree SHA"),
      "reviewSha256" => hex(ENV.fetch("P16_EXPECTED_REVIEW_SHA256"), "review SHA"),
      "reviewerId" => reviewer,
      "runInventorySha256" => hex(ENV.fetch("P16_EXPECTED_RUN_INVENTORY_SHA256"), "run inventory SHA"),
      "sourceCommit" => commit
    }
  rescue KeyError => error
    fail! "missing trust root #{error.key}"
  end

  def version(repo)
    value = File.binread(File.join(repo, "Cargo.toml"))[/^version = "([^"]+)"$/, 1]
    fail! "package version missing" unless value
    value
  end

  def verify_source_checkout(repo, expected_commit)
    stdout, stderr, status = Open3.capture3("git", "-C", repo, "rev-parse", "HEAD")
    fail! "source HEAD: #{stderr.strip}" unless status.success? && stdout.strip == expected_commit
    stdout, stderr, status = Open3.capture3("git", "-C", repo, "status", "--porcelain=v1", "--untracked-files=all")
    fail! "source status: #{stderr.strip}" unless status.success?
    fail! "source checkout must be clean" unless stdout.empty?
    inventory = P16Provenance.inventory(repo, expected_commit)
    Dir.mktmpdir("p16-closure-source-") do |directory|
      inventory_path = File.join(directory, "source-inventory.tsv")
      File.binwrite(inventory_path, inventory)
      P16Provenance.verify_worktree(repo, inventory_path, expected_commit)
    end
    tree, stderr, status = Open3.capture3("git", "-C", repo, "rev-parse", "#{expected_commit}^{tree}")
    fail! "source tree: #{stderr.strip}" unless status.success? && tree.strip.match?(COMMIT)
    {"inventory" => inventory, "tree" => tree.strip}
  end

  def verify_artifact(artifact, repo, expected)
    raw = stable_read(File.expand_path(artifact), "artifact", 64 * 1024 * 1024)
    fail! "artifact SHA mismatch" unless Digest::SHA256.hexdigest(raw) == expected
    version_value = version(repo)
    manifest = File.join(repo, "tests/unreal/p1.6-manifest.json")
    P16Archive.inspect_bytes(raw, manifest, version_value)
    binary_path = "magi-unreal-axi-#{version_value}-macos-arm64/magi-unreal-axi"
    binary = P16Archive.file_bytes(raw, manifest, version_value, binary_path)
    binary_sha = Digest::SHA256.hexdigest(binary)
    Dir.mktmpdir("p16-closure-binary-") do |directory|
      path = File.join(directory, "magi-unreal-axi")
      File.binwrite(path, binary)
      File.chmod(0o755, path)
      _stdout, stderr, status = Open3.capture3("codesign", "--verify", "--strict", path)
      fail! "artifact binary codesign: #{stderr.strip}" unless status.success?
      stdout, stderr, status = Open3.capture3("lipo", "-archs", path)
      architectures = stdout.split.map { |architecture| architecture == "arm64e" ? "arm64" : architecture }.uniq
      fail! "artifact binary architecture: #{stderr.strip}" unless status.success? && architectures == ["arm64"]
    end
    binary_sha
  rescue ArchiveError => error
    fail! "archive: #{error.message}"
  end

  def verify_review(path, roots)
    review, raw = parse_canonical(path, "independent review")
    fail! "independent review SHA mismatch" unless Digest::SHA256.hexdigest(raw) == roots["reviewSha256"]
    fail! "independent review keys" unless review.keys.sort == REVIEW_KEYS.sort
    fail! "independent review schema" unless review["schema"] == REVIEW_SCHEMA
    fail! "independent review disposition" unless review["status"] == "passed" && review["findings"] == []
    fail! "independent reviewer mismatch" unless review["reviewerId"] == roots["reviewerId"]
    subject = review["subject"]
    fail! "independent review subject" unless subject.is_a?(Hash) && subject.keys.sort == SUBJECT_KEYS.sort
    SUBJECT_KEYS.grep(/Sha256\z/).each { |key| hex(subject[key], "review #{key}") }
    fail! "invalid review source commit" unless subject["sourceCommit"].is_a?(String) && subject["sourceCommit"].match?(COMMIT)
    fail! "invalid review source tree" unless subject["sourceTree"].is_a?(String) && subject["sourceTree"].match?(COMMIT)
    fail! "review source commit mismatch" unless subject["sourceCommit"] == roots["sourceCommit"]
    review
  end

  def copy_combined_verified(source, destination, expected_tree)
    source = File.expand_path(source)
    fail! "combined evidence must be a directory" unless File.directory?(source) && !File.symlink?(source)
    manifest = File.join(source, "evidence-tree.json")
    tree, = parse_canonical(manifest, "combined evidence tree")
    fail! "combined evidence tree trust mismatch" unless tree["treeSha256"] == expected_tree
    EvidenceTree.verify(source, manifest)
    FileUtils.cp_r(source, destination, preserve: true)
    copied_manifest = File.join(destination, "evidence-tree.json")
    copied_tree, = parse_canonical(copied_manifest, "copied combined evidence tree")
    fail! "copied combined evidence tree mismatch" unless copied_tree["treeSha256"] == expected_tree
    EvidenceTree.verify(destination, copied_manifest)
  end

  def snapshots(run, combined, roots)
    parent = Dir.mktmpdir("p16-closure-")
    run_snapshot = File.join(parent, "run")
    combined_snapshot = File.join(parent, "combined")
    P16RunInventory.materialize_verified(run, roots["runInventorySha256"], run_snapshot)
    copy_combined_verified(combined, combined_snapshot, roots["combinedEvidenceTreeSha256"])
    [parent, run_snapshot, combined_snapshot]
  rescue StandardError
    FileUtils.remove_entry(parent) if parent && File.exist?(parent)
    raise
  end

  def derive(run, combined, review, roots, trusted_source = nil)
    P16Revalidate.validate(run, combined, roots["runInventorySha256"], snapshot: false)
    manifest, = parse_object(File.join(run, "manifest.json"), "run manifest")
    inventory, = parse_canonical(File.join(run, "run-inventory.json"), "run inventory")
    fail! "run inventory tree" unless inventory["treeSha256"].is_a?(String) && inventory["treeSha256"].match?(SHA)
    provenance, = parse_canonical(File.join(combined, "provenance.json"), "combined provenance")
    identities, = parse_canonical(File.join(combined, "provenance-identities.json"), "combined identities")
    subject = {
      "artifactSha256" => roots["artifactSha256"],
      "binarySha256" => manifest.fetch("binarySha256"),
      "combinedEvidenceTreeSha256" => roots["combinedEvidenceTreeSha256"],
      "combinedProvenanceSha256" => manifest.fetch("combinedProvenanceSha256"),
      "pluginSha256" => manifest.fetch("pluginSha256"),
      "runInventorySha256" => roots["runInventorySha256"],
      "sourceCommit" => roots["sourceCommit"],
      "sourceTree" => manifest.fetch("sourceTree")
    }
    if trusted_source
      fail! "trusted source tree mismatch" unless subject["sourceTree"] == trusted_source.fetch("tree")
      fail! "trusted source inventory mismatch" unless File.binread(File.join(combined, "source-inventory.tsv")) == trusted_source.fetch("inventory")
    end
    fail! "review subject mismatch" unless review["subject"] == subject
    fail! "run identity mismatch" unless manifest["artifactSha256"] == subject["artifactSha256"] && manifest["combinedEvidenceTreeSha256"] == subject["combinedEvidenceTreeSha256"] && manifest["sourceCommit"] == subject["sourceCommit"]
    fail! "combined source mismatch" unless provenance.dig("source", "commit") == subject["sourceCommit"] && provenance.dig("source", "tree") == subject["sourceTree"]
    fail! "combined identity mismatch" unless identities.values_at("artifactSha256", "binarySha256", "pluginSha256") == subject.values_at("artifactSha256", "binarySha256", "pluginSha256")
    fail! "combined provenance mismatch" unless sha(File.join(combined, "provenance.json")) == subject["combinedProvenanceSha256"]
    P16Provenance.verify_portable(
      File.join(combined, "source-inventory.tsv"),
      File.join(combined, "provenance.json"),
      File.join(combined, "provenance-identities.json"),
      expected_provenance_sha: subject["combinedProvenanceSha256"],
      expected_source_commit: subject["sourceCommit"],
      expected_source_tree: subject["sourceTree"],
      expected_identities: subject.slice("artifactSha256", "binarySha256", "pluginSha256")
    )
    {
      "certificationComplete" => true,
      "identities" => subject.merge("reviewSha256" => roots["reviewSha256"]),
      "phase" => "P1.6",
      "review" => {"reviewerId" => roots["reviewerId"], "status" => "passed"},
      "schema" => SCHEMA,
      "status" => "closed"
    }
  rescue KeyError => error
    fail! "missing identity #{error.key}"
  end

  def validate_closure(value)
    fail! "closure keys" unless value.keys.sort == CLOSURE_KEYS.sort
    fail! "closure disposition" unless value["schema"] == SCHEMA && value["phase"] == "P1.6" && value["status"] == "closed" && value["certificationComplete"] == true
    identities = value["identities"]
    fail! "closure identities" unless identities.is_a?(Hash) && identities.keys.sort == IDENTITY_KEYS
    IDENTITY_KEYS.grep(/Sha256\z/).each { |key| hex(identities[key], "closure #{key}") }
    fail! "closure source commit" unless identities["sourceCommit"].is_a?(String) && identities["sourceCommit"].match?(COMMIT)
    fail! "closure source tree" unless identities["sourceTree"].is_a?(String) && identities["sourceTree"].match?(COMMIT)
    review = value["review"]
    fail! "closure review" unless review.is_a?(Hash) && review.keys.sort == REVIEW_KEYS_NESTED && review["status"] == "passed" && review["reviewerId"].is_a?(String) && review["reviewerId"].match?(REVIEWER)
  end

  def derive_from_inputs(repo, artifact, run, combined, review_path, roots, trusted_source: nil)
    trusted_source ||= verify_source_checkout(repo, roots["sourceCommit"])
    binary_sha = verify_artifact(artifact, repo, roots["artifactSha256"])
    review = verify_review(review_path, roots)
    fail! "artifact binary mismatch" unless review.dig("subject", "binarySha256") == binary_sha
    parent, run_snapshot, combined_snapshot = snapshots(run, combined, roots)
    [derive(run_snapshot, combined_snapshot, review, roots, trusted_source), parent]
  end
  def verify(repo, artifact, run, combined, review_path, closure_path, roots = trust_roots, trusted_source: nil)
    expected, parent = derive_from_inputs(repo, artifact, run, combined, review_path, roots, trusted_source: trusted_source)
    actual, raw = parse_canonical(closure_path, "closure")
    validate_closure(actual)
    fail! "closure mismatch" unless actual == expected && raw == canonical_json(expected) << "\n"
    expected
  ensure
    FileUtils.remove_entry(parent) if parent && File.exist?(parent)
  end
  def write(repo, artifact, run, combined, review_path, closure_path, roots = trust_roots, trusted_source: nil)
    expected, parent = derive_from_inputs(repo, artifact, run, combined, review_path, roots, trusted_source: trusted_source)
    output = File.expand_path(closure_path)
    output_parent = File.realpath(File.dirname(output))
    fail! "closure destination must not exist" if File.exist?(output) || File.symlink?(output)
    temporary = File.join(output_parent, ".#{File.basename(output)}.#{Process.pid}.#{SecureRandom.hex(8)}")
    begin
      File.open(temporary, File::WRONLY | File::CREAT | File::EXCL, 0o600) do |io|
        io.write(canonical_json(expected) << "\n")
        io.flush
        io.fsync
      end
      File.chmod(0o644, temporary)
      staged, raw = parse_canonical(temporary, "staged closure")
      validate_closure(staged)
      fail! "staged closure mismatch" unless staged == expected && raw == canonical_json(expected) << "\n"
      File.link(temporary, output)
      File.unlink(temporary)
      File.open(output_parent, File::RDONLY) { |io| io.fsync }
      expected
    ensure
      File.unlink(temporary) if temporary && File.exist?(temporary)
    end
  ensure
    FileUtils.remove_entry(parent) if parent && File.exist?(parent)
  end

  def expect_failure(label)
    yield
    fail! "self-test #{label} accepted"
  rescue RuntimeError => error
    raise if error.message.end_with?("accepted")
  end

  def fixture_archive(repo, root)
    version_value = version(repo)
    release = JSON.parse(File.binread(File.join(repo, "tests/unreal/p1.6-manifest.json"))).fetch("releaseArchive")
    paths = release.fetch("allowlist").map { |path| path.gsub("{version}", version_value) }.sort
    binary_path = "magi-unreal-axi-#{version_value}-macos-arm64/magi-unreal-axi"
    fixture_binary = File.join(root, "fixture-arm64")
    FileUtils.cp("/usr/bin/true", fixture_binary)
    _stdout, _stderr, status = Open3.capture3("lipo", fixture_binary, "-thin", "arm64e", "-output", fixture_binary)
    fail! "fixture arm64 extraction" unless status.success?
    contents = {binary_path => File.binread(fixture_binary)}
    archive = File.join(root, "artifact.tar.gz")
    File.binwrite(archive, P16Archive.gzip(P16Archive.make_tar(paths, contents)))
    [archive, fixture_binary, Digest::SHA256.hexdigest(contents.fetch(binary_path))]
  end

  def self_test(repo)
    repo = File.realpath(repo)
    original_status = `git -C #{Shellwords.escape(repo)} status --porcelain=v1 --untracked-files=all`
    Dir.mktmpdir("p16-closure-self-test-") do |root|
      artifact, fixture_binary, fixture_binary_sha = fixture_archive(repo, root)
      artifact_sha = sha(artifact)
      fixture_root = File.join(root, "fixture")
      FileUtils.mkdir_p(fixture_root)
      fixture = P16AgentLifecycleFixtures.build_finalized(repo, fixture_root, artifact: artifact_sha, exact_binary_source: fixture_binary)
      run = fixture.fetch(:run)
      combined = fixture.fetch(:combined)
      manifest = JSON.parse(File.binread(File.join(run, "manifest.json")))
      fail! "fixture binary drift" unless manifest.fetch("binarySha256") == fixture_binary_sha
      roots = {
        "artifactSha256" => artifact_sha,
        "combinedEvidenceTreeSha256" => manifest.fetch("combinedEvidenceTreeSha256"),
        "reviewSha256" => nil,
        "reviewerId" => "fixture-reviewer",
        "runInventorySha256" => fixture.fetch(:inventory_sha),
        "sourceCommit" => manifest.fetch("sourceCommit")
      }
      subject = {
        "artifactSha256" => artifact_sha,
        "binarySha256" => manifest.fetch("binarySha256"),
        "combinedEvidenceTreeSha256" => manifest.fetch("combinedEvidenceTreeSha256"),
        "combinedProvenanceSha256" => manifest.fetch("combinedProvenanceSha256"),
        "pluginSha256" => manifest.fetch("pluginSha256"),
        "runInventorySha256" => fixture.fetch(:inventory_sha),
        "sourceCommit" => manifest.fetch("sourceCommit"),
        "sourceTree" => manifest.fetch("sourceTree")
      }
      review_path = File.join(root, "review.json")
      review = {"findings" => [], "reviewerId" => roots["reviewerId"], "schema" => REVIEW_SCHEMA, "status" => "passed", "subject" => subject}
      File.binwrite(review_path, canonical_json(review) << "\n")
      roots["reviewSha256"] = sha(review_path)
      closure = File.join(root, "closure.json")
      trusted_source = {"inventory" => File.binread(File.join(combined, "source-inventory.tsv")), "tree" => manifest.fetch("sourceTree")}
      write(repo, artifact, run, combined, review_path, closure, roots, trusted_source: trusted_source)
      verify(repo, artifact, run, combined, review_path, closure, roots, trusted_source: trusted_source)
      expect_failure("existing output") { write(repo, artifact, run, combined, review_path, closure, roots, trusted_source: trusted_source) }
      wrong = roots.merge("artifactSha256" => "f" * 64)
      expect_failure("artifact trust") { verify(repo, artifact, run, combined, review_path, closure, wrong, trusted_source: trusted_source) }
      wrong = roots.merge("reviewSha256" => "f" * 64)
      expect_failure("review trust") { verify(repo, artifact, run, combined, review_path, closure, wrong, trusted_source: trusted_source) }
      wrong = roots.merge("runInventorySha256" => "f" * 64)
      expect_failure("run trust") { verify(repo, artifact, run, combined, review_path, closure, wrong, trusted_source: trusted_source) }
      wrong_review = review.merge("subject" => subject.merge("pluginSha256" => "f" * 64))
      wrong_review_path = File.join(root, "wrong-review.json")
      File.binwrite(wrong_review_path, canonical_json(wrong_review) << "\n")
      wrong_roots = roots.merge("reviewSha256" => sha(wrong_review_path))
      expect_failure("review subject") { verify(repo, artifact, run, combined, wrong_review_path, closure, wrong_roots, trusted_source: trusted_source) }
      tampered_closure = File.join(root, "tampered-closure.json")
      File.binwrite(tampered_closure, File.binread(closure).sub('"status":"closed"', '"status":"open"'))
      expect_failure("closure tamper") { verify(repo, artifact, run, combined, review_path, tampered_closure, roots, trusted_source: trusted_source) }
      fail! "repository changed during closure self-test" unless `git -C #{Shellwords.escape(repo)} status --porcelain=v1 --untracked-files=all` == original_status
    end
    puts "P1.6 closure gate self-test: PASS write=1 verify=1 trust-negatives=4"
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    mode = ARGV.shift
    repo = File.expand_path("../../..", __dir__)
    if %w[self-test --self-test].include?(mode)
      P16Closure.self_test(ARGV.shift || repo)
    elsif %w[write verify].include?(mode) && ARGV.length == 5
      result = P16Closure.public_send(mode, repo, *ARGV)
      puts "closureSha256=#{Digest::SHA256.hexdigest(P16Closure.canonical_json(result) << "\n")}"
    else
      warn "usage: p16-closure.rb write|verify ARTIFACT RUN COMBINED REVIEW CLOSURE | self-test [REPO]"
      exit 2
    end
  rescue StandardError => error
    warn error.message
    exit 1
  end
end
