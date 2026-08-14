#!/usr/bin/env ruby
# frozen_string_literal: true

require "digest"
require "fileutils"
require "json"
require "open3"
require "pathname"
require "tmpdir"

module P16Provenance
  SCHEMA = "magi-unreal-axi/p1.6-provenance/v1"
  SHA = /\A[0-9a-f]{64}\z/.freeze
  COMMIT = /\A[0-9a-f]{40}\z/.freeze
  IDENTITY_KEYS = %w[artifactSha256 binarySha256 catalogSha256 manifestSha256 pluginSha256].freeze
  PATH_CONTROLS = /[\x00-\x1f\x7f\u200b-\u200f\u202a-\u202e\u2060-\u206f\ufeff]/.freeze
  module_function

  def canonical(value)
    case value
    when Hash then value.keys.sort_by { |key| key.encode(Encoding::UTF_8).bytes }.to_h { |key| [key, canonical(value.fetch(key))] }
    when Array then value.map { |item| canonical(item) }
    else value
    end
  end

  def json(value)
    JSON.generate(canonical(value), ascii_only: false)
  end

  def digest(path)
    Digest::SHA256.file(path).hexdigest
  end
  def git(root, *args)
    stdout, status = Open3.capture2("git", "-C", root, *args, binmode: true)
    raise "git #{args.join(" ")} failed" unless status.success?
    stdout
  end

  def inventory(root, revision = "HEAD")
    paths = git(root, "ls-tree", "-r", "-z", "--name-only", revision).split("\0").reject(&:empty?).sort_by { |path| path.encode(Encoding::UTF_8).bytes }
    paths.map do |path|
      blob = git(root, "show", "#{revision}:#{path}")
      "#{path}\t#{Digest::SHA256.hexdigest(blob)}"
    end.join("\n") << "\n"
  end

  def source(root, inventory_raw)
    {"commit" => git(root, "rev-parse", "HEAD").strip,
     "tree" => git(root, "rev-parse", "HEAD^{tree}").strip,
     "inventorySha256" => Digest::SHA256.hexdigest(inventory_raw)}
  end

  def portable_inventory(raw)
    text = raw.dup.force_encoding(Encoding::UTF_8)
    raise "inventory must be UTF-8" unless text.valid_encoding?
    raise "inventory must end with one newline" unless text.end_with?("\n") && !text.end_with?("\n\n")
    rows = text.lines(chomp: true).map do |line|
      path, value = line.split("\t", -1)
      parts = path&.split("/", -1)
      safe = path.is_a?(String) && !path.empty? && !path.start_with?("/") && !path.include?("\\") && !path.match?(PATH_CONTROLS) && parts&.none? { |part| part.empty? || part == "." || part == ".." || part != part.unicode_normalize(:nfc) }
      raise "invalid inventory row" unless safe && value&.match?(SHA)
      [path, value]
    end
    raise "empty inventory" if rows.empty?
    paths = rows.map(&:first)
    raise "inventory not canonical" unless paths == paths.uniq.sort_by { |path| path.encode(Encoding::UTF_8).bytes }
    text
  end
  def inventory_map(raw)
    portable_inventory(raw).lines(chomp: true).to_h { |line| line.split("\t", 2) }
  end

  def verify_identity_inventory(identity, raw)
    paths = inventory_map(raw)
    raise "catalog identity" unless identity["catalogSha256"] == paths["capabilities/catalog.json"]
    raise "manifest identity" unless identity["manifestSha256"] == paths["tests/unreal/p1.6-manifest.json"]
  end

  def verify_worktree(root, inventory_path, expected_commit)
    root = File.realpath(root)
    raise "expected source commit" unless expected_commit.is_a?(String) && expected_commit.match?(COMMIT)
    raise "source commit mismatch" unless git(root, "rev-parse", "HEAD").strip == expected_commit
    expected = inventory_map(File.binread(inventory_path))
    tracked = git(root, "ls-files", "-z", "--cached").split("\0").reject(&:empty?).sort_by { |path| path.encode(Encoding::UTF_8).bytes }
    raise "tracked path set differs from commit" unless tracked == expected.keys
    verbose = git(root, "ls-files", "-v").lines
    tagged = git(root, "ls-files", "-t").lines
    raise "assume-unchanged source path" if verbose.any? { |line| line.getbyte(0)&.between?(97, 122) }
    raise "skip-worktree source path" if tagged.any? { |line| line.start_with?("S ") }
    expected.each do |path, digest_value|
      full = File.join(root, path)
      raise "source inventory path missing: #{path}" unless File.file?(full) && !File.symlink?(full)
      raise "working-tree source differs: #{path}" unless digest(full) == digest_value
    end
    puts "OK worktreeSourceCommit=#{expected_commit} files=#{expected.length}"
  end

  def parse_identity(path)
    raw = File.binread(path)
    value = JSON.parse(raw)
    raise "invalid identity" unless value.is_a?(Hash) && value.keys.sort == IDENTITY_KEYS.sort && value.values.all? { |item| item.is_a?(String) && item.match?(SHA) }
    raise "identity is not canonical" unless raw == json(value) << "\n"
    value
  end

  def verify_portable(inventory_path, provenance_path, identity_path, expected_provenance_sha:, expected_source_commit:, expected_source_tree:, expected_identities:)
    raise "expected provenance SHA" unless expected_provenance_sha.is_a?(String) && expected_provenance_sha.match?(SHA)
    raise "expected source commit" unless expected_source_commit.is_a?(String) && expected_source_commit.match?(COMMIT)
    raise "expected source tree" unless expected_source_tree.is_a?(String) && expected_source_tree.match?(COMMIT)
    raise "provenance digest" unless digest(provenance_path) == expected_provenance_sha
    raw = portable_inventory(File.binread(inventory_path))
    provenance_raw = File.binread(provenance_path)
    value = JSON.parse(provenance_raw)
    identity = parse_identity(identity_path)
    verify_identity_inventory(identity, raw)
    raise "provenance is not canonical" unless value.is_a?(Hash) && provenance_raw == json(value) << "\n"
    raise "provenance schema" unless value.keys.sort == %w[identities schema source].sort && value["schema"] == SCHEMA
    expected_source = {"commit" => expected_source_commit, "tree" => expected_source_tree, "inventorySha256" => Digest::SHA256.hexdigest(raw)}
    raise "source identity" unless value["source"] == expected_source
    raise "provenance identity document" unless value["identities"] == identity
    expected_identities.each do |key, expected|
      raise "expected identity #{key}" unless IDENTITY_KEYS.include?(key) && expected.is_a?(String) && expected.match?(SHA)
      raise "identity mismatch #{key}" unless identity[key] == expected
    end
    puts "OK portable provenanceSha256=#{expected_provenance_sha} sourceCommit=#{expected_source_commit}"
  rescue JSON::ParserError => error
    raise "invalid provenance JSON: #{error.message}"
  end

  def write(root, inventory_path, provenance_path, identity_path)
    root = File.realpath(root)
    inventory_raw = inventory(root)
    File.binwrite(inventory_path, inventory_raw)
    identity = parse_identity(identity_path)
    verify_identity_inventory(identity, inventory_raw)
    provenance = {"schema" => SCHEMA, "source" => source(root, inventory_raw), "identities" => identity}
    File.binwrite(provenance_path, json(provenance) << "\n")
    puts Digest::SHA256.hexdigest(File.binread(provenance_path))
  end

  def verify(root, inventory_path, provenance_path, identity_path)
    root = File.realpath(root)
    raw = File.binread(inventory_path)
    raise "inventory is not canonical" unless raw == inventory(root)
    provenance_raw = File.binread(provenance_path)
    value = JSON.parse(provenance_raw)
    expected = parse_identity(identity_path)
    raise "provenance is not canonical" unless value.is_a?(Hash) && provenance_raw == json(value) << "\n"
    raise "provenance schema" unless value.keys.sort == %w[identities schema source].sort && value["schema"] == SCHEMA
    raise "source identity" unless value["source"] == source(root, raw)
    raise "artifact identity" unless value["identities"] == expected
    verify_identity_inventory(expected, raw)
    puts "OK provenanceSha256=#{digest(provenance_path)} sourceCommit=#{value.dig("source", "commit")}"
  rescue JSON::ParserError => error
    raise "invalid provenance JSON: #{error.message}"
  end

  def self_test
    Dir.mktmpdir("p16-provenance-") do |root|
      system("git", "-C", root, "init", "-q") || raise("git init")
      system("git", "-C", root, "config", "user.email", "test@example.invalid") || raise("git config")
      system("git", "-C", root, "config", "user.name", "P16 test") || raise("git config")
      File.write(File.join(root, "source.txt"), "source\n")
      catalog_dir = File.join(root, "capabilities"); FileUtils.mkdir_p(catalog_dir); File.write(File.join(catalog_dir, "catalog.json"), "catalog\n")
      manifest_dir = File.join(root, "tests", "unreal"); FileUtils.mkdir_p(manifest_dir); File.write(File.join(manifest_dir, "p1.6-manifest.json"), "manifest\n")
      system("git", "-C", root, "add", "source.txt", "capabilities/catalog.json", "tests/unreal/p1.6-manifest.json") || raise("git add")
      system("git", "-C", root, "commit", "-qm", "test") || raise("git commit")
      Dir.mkdir(File.join(root, "out"))
      commit_inventory = inventory(root)
      identity = {"artifactSha256" => "a" * 64, "binarySha256" => "b" * 64, "catalogSha256" => inventory_map(commit_inventory).fetch("capabilities/catalog.json"), "manifestSha256" => inventory_map(commit_inventory).fetch("tests/unreal/p1.6-manifest.json"), "pluginSha256" => "e" * 64}
      identity_path = File.join(root, "out", "identity.json"); File.write(identity_path, json(identity) << "\n")
      inventory_path = File.join(root, "out", "source-inventory.tsv"); provenance_path = File.join(root, "out", "provenance.json")
      write(root, inventory_path, provenance_path, identity_path); verify(root, inventory_path, provenance_path, identity_path)
      provenance_sha = digest(provenance_path)
      source_value = source(root, File.binread(inventory_path))
      verify_worktree(root, inventory_path, source_value.fetch("commit"))
      Dir.mktmpdir("portable-no-git-") do |portable|
        portable_inventory_path = File.join(portable, "inventory.tsv"); portable_provenance = File.join(portable, "provenance.json"); portable_identity = File.join(portable, "identity.json")
        FileUtils.cp(inventory_path, portable_inventory_path); FileUtils.cp(provenance_path, portable_provenance); FileUtils.cp(identity_path, portable_identity)
        verify_portable(portable_inventory_path, portable_provenance, portable_identity, expected_provenance_sha: provenance_sha, expected_source_commit: source_value.fetch("commit"), expected_source_tree: source_value.fetch("tree"), expected_identities: identity.slice("artifactSha256", "binarySha256", "pluginSha256"))
        File.write(portable_provenance, "{}\n")
        begin
          verify_portable(portable_inventory_path, portable_provenance, portable_identity, expected_provenance_sha: provenance_sha, expected_source_commit: source_value.fetch("commit"), expected_source_tree: source_value.fetch("tree"), expected_identities: {})
          raise "portable tamper accepted"
        rescue RuntimeError => error
          raise if error.message == "portable tamper accepted"
        end
      end
      File.write(provenance_path, "{}\n")
      begin
        verify(root, inventory_path, provenance_path, identity_path)
        raise "tamper accepted"
      rescue RuntimeError => error
        raise if error.message == "tamper accepted"
      end
    end
    puts "P1.6 provenance self-test: PASS"
  end
end

if $PROGRAM_NAME == __FILE__
begin
  case ARGV[0]
  when "inventory" then raise "usage" unless ARGV.length == 3; File.binwrite(ARGV[2], P16Provenance.inventory(File.realpath(ARGV[1])))
  when "write" then raise "usage" unless ARGV.length == 5; P16Provenance.write(ARGV[1], ARGV[2], ARGV[3], ARGV[4])
  when "verify" then raise "usage" unless ARGV.length == 5; P16Provenance.verify(ARGV[1], ARGV[2], ARGV[3], ARGV[4])
  when "verify-worktree" then raise "usage" unless ARGV.length == 4; P16Provenance.verify_worktree(ARGV[1], ARGV[2], ARGV[3])
  when "self-test" then raise "usage" unless ARGV.length == 1; P16Provenance.self_test
  else raise "usage: inventory ROOT OUT | write ROOT INVENTORY PROVENANCE IDENTITIES | verify ROOT INVENTORY PROVENANCE IDENTITIES | verify-worktree ROOT INVENTORY EXPECTED_COMMIT | self-test"
  end
rescue StandardError => error
  warn "p16-provenance: #{error.message}"
  exit 1
end
end
