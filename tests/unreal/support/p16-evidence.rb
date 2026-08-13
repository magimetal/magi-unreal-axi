#!/usr/bin/env ruby
# frozen_string_literal: true

require "digest"
require "fileutils"
require "json"
require "securerandom"
require "tmpdir"

module EvidenceTree
  SCHEMA = "magi-unreal-axi/evidence-tree/v1"
  HEX_SHA256 = /\A[0-9a-f]{64}\z/.freeze
  PATH_CONTROLS = /[\x00-\x1f\x7f\u200b-\u200f\u202a-\u202e\u2060-\u206f\ufeff]/.freeze
  MAX_MANIFEST_BYTES = 8 * 1024 * 1024
  MAX_FILE_COUNT = 4096
  MAX_FILE_BYTES = 64 * 1024 * 1024
  MAX_TOTAL_BYTES = 256 * 1024 * 1024
  MAX_PATH_DEPTH = 64
  MAX_PATH_BYTES = 4096
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

  def safe_relative_path(path)
    text = path.dup.force_encoding(Encoding::UTF_8)
    raise "invalid UTF-8 path" unless text.valid_encoding?
    parts = text.split("/", -1)
    raise "unsafe path: #{path.inspect}" if text.empty? || text.start_with?("/") || text.include?("\\") || text.match?(PATH_CONTROLS) || parts.any? { |part| part.empty? || part == "." || part == ".." || part != part.unicode_normalize(:nfc) }
    raise "path exceeds byte bound" if text.bytesize > MAX_PATH_BYTES
    raise "path exceeds depth bound" if parts.length > MAX_PATH_DEPTH
    text
  end

  def output_path(path)
    parent = File.realpath(File.dirname(File.expand_path(path)))
    File.join(parent, File.basename(path))
  end

  def manifest_location(root, path)
    output = output_path(path)
    prefix = "#{root}#{File::SEPARATOR}"
    raise "manifest must be inside evidence root" unless output.start_with?(prefix)
    relative = safe_relative_path(output.delete_prefix(prefix))
    if File.exist?(output) || File.symlink?(output)
      stat = File.lstat(output)
      raise "manifest must be regular non-symlink" unless stat.file? && !stat.symlink? && stat.nlink == 1
    end
    [output, relative]
  end

  def read_stable_file(path, expected, relative)
    File.open(path, File::RDONLY | File::NOFOLLOW) do |file|
      before = file.stat
      raise "file changed during traversal: #{relative}" unless before.file? && before.nlink == 1 && [before.dev, before.ino] == [expected.dev, expected.ino]
      digest = Digest::SHA256.new
      bytes = 0
      while (chunk = file.read(1024 * 1024))
        bytes += chunk.bytesize
        raise "file exceeds byte bound: #{relative}" if bytes > MAX_FILE_BYTES
        digest.update(chunk)
      end
      after = file.stat
      identity = ->(stat) { [stat.dev, stat.ino, stat.size, stat.mtime.to_r, stat.ctime.to_r, stat.nlink] }
      raise "file changed while hashing: #{relative}" unless identity.call(before) == identity.call(after)
      {"path" => relative, "bytes" => bytes, "sha256" => digest.hexdigest}
    end
  rescue Errno::ELOOP
    raise "symlink rejected: #{relative}"
  end

  def entries(root, manifest_path = nil)
    root = File.realpath(root)
    excluded = manifest_path && manifest_location(root, manifest_path).last
    files = []
    seen = { [File.lstat(root).dev, File.lstat(root).ino] => true }
    seen_files = {}
    stack = [root]
    until stack.empty?
      directory = stack.pop
      Dir.children(directory).sort_by { |name| name.encode(Encoding::UTF_8).bytes }.each do |name|
        path = File.join(directory, name)
        relative = safe_relative_path(path.delete_prefix("#{root}/"))
        stat = File.lstat(path)
        next if relative == excluded
        raise "symlink rejected: #{relative}" if stat.symlink?
        if stat.directory?
          identity = [stat.dev, stat.ino]
          raise "directory cycle rejected: #{relative}" if seen.key?(identity)
          seen[identity] = true
          resolved = File.realpath(path)
          raise "directory escaped evidence root: #{relative}" unless resolved.start_with?("#{root}/")
          stack << resolved
        elsif stat.file?
          raise "hardlink rejected: #{relative}" unless stat.nlink == 1
          identity = [stat.dev, stat.ino]
          raise "duplicate file inode rejected: #{relative}" if seen_files.key?(identity)
          seen_files[identity] = true
          files << read_stable_file(path, stat, relative)
          raise "file count exceeds bound" if files.length > MAX_FILE_COUNT
        else
          raise "special file rejected: #{relative}"
        end
      end
    end
    raise "total file bytes exceed bound" if files.sum { |entry| entry["bytes"] } > MAX_TOTAL_BYTES
    files.sort_by { |entry| entry["path"].encode(Encoding::UTF_8).bytes }
  end

  def tree_hash(files)
    Digest::SHA256.hexdigest(canonical_json(files))
  end
  def manifest_for(root, path = nil)
    files = entries(root, path)
    {"schema" => SCHEMA, "files" => files, "treeSha256" => tree_hash(files)}
  end

  def write(root, output)
    root = File.realpath(root)
    output, = manifest_location(root, output)
    manifest = manifest_for(root, output)
    temporary = File.join(File.dirname(output), ".#{File.basename(output)}.#{Process.pid}.#{SecureRandom.hex(8)}")
    begin
      File.open(temporary, File::WRONLY | File::CREAT | File::EXCL, 0o600) { |file| file.write(canonical_json(manifest) << "\n"); file.flush; file.fsync }
      File.chmod(0o644, temporary)
      File.rename(temporary, output)
      File.open(File.dirname(output), File::RDONLY) { |directory| directory.fsync }
    ensure
      File.unlink(temporary) if File.exist?(temporary)
    end
    puts manifest["treeSha256"]
  end

  def verify(root, manifest_path)
    root = File.realpath(root)
    manifest_path, = manifest_location(root, manifest_path)
    raw = File.binread(manifest_path)
    raise "manifest exceeds byte bound" if raw.bytesize > MAX_MANIFEST_BYTES
    raise "manifest must be UTF-8" unless raw.dup.force_encoding(Encoding::UTF_8).valid_encoding?
    manifest = JSON.parse(raw)
    raise "manifest must be canonical" unless manifest.is_a?(Hash) && manifest.keys.sort == %w[files schema treeSha256].sort && raw == canonical_json(manifest) << "\n"
    raise "unsupported schema" unless manifest["schema"] == SCHEMA
    expected = manifest["files"]
    raise "manifest files must be an array" unless expected.is_a?(Array) && expected.length <= MAX_FILE_COUNT
    expected.each { |entry| raise "invalid file entry" unless entry.is_a?(Hash) && entry.keys.sort == %w[bytes path sha256].sort && entry["bytes"].is_a?(Integer) && entry["bytes"] >= 0 && entry["bytes"] <= MAX_FILE_BYTES && entry["sha256"].is_a?(String) && entry["sha256"].match?(HEX_SHA256); safe_relative_path(entry["path"]) }
    raise "manifest paths are not sorted or unique" unless expected.map { |e| e["path"] } == expected.map { |e| e["path"] }.uniq.sort_by { |p| p.encode(Encoding::UTF_8).bytes }
    raise "invalid tree hash" unless manifest["treeSha256"].is_a?(String) && manifest["treeSha256"].match?(HEX_SHA256)
    raise "file set or content mismatch" unless tree_hash(expected) == manifest["treeSha256"] && entries(root, manifest_path) == expected
    puts "OK #{manifest["treeSha256"]}"
  rescue JSON::ParserError => error
    raise "invalid manifest JSON: #{error.message}"
  end

  def expect_failure(label); yield; raise "self-test failed: #{label} accepted"; rescue RuntimeError => error; raise if error.message.start_with?("self-test failed") end
  def self_test
    Dir.mktmpdir("p16-evidence-") do |root|
      FileUtils.mkdir_p("#{root}/nested"); File.binwrite("#{root}/a.txt", "alpha\n"); File.binwrite("#{root}/nested/b.bin", "\x00\xFF".b)
      manifest = "#{root}/tree.json"; write(root, manifest); verify(root, manifest)
      File.binwrite("#{root}/a.txt", "mutated\n"); expect_failure("post-manifest mutation") { verify(root, manifest) }; File.binwrite("#{root}/a.txt", "alpha\n")
      begin; FileUtils.ln("#{root}/a.txt", "#{root}/hardlink"); expect_failure("hardlink") { entries(root, manifest) }; File.unlink("#{root}/hardlink"); rescue Errno::EOPNOTSUPP, Errno::EPERM; end
      File.symlink("a.txt", "#{root}/link"); expect_failure("symlink") { entries(root, manifest) }; File.unlink("#{root}/link")
      File.binwrite("#{root}/certificate.json", "not a manifest"); raise "self-test failed: arbitrary certificate excluded" unless entries(root, manifest).any? { |entry| entry["path"] == "certificate.json" }; File.unlink("#{root}/certificate.json")
      original = File.binread(manifest); malformed = JSON.parse(original); malformed["schema"] = "wrong"; File.write(manifest, JSON.generate(malformed) << "\n"); expect_failure("malformed schema") { verify(root, manifest) }; File.write(manifest, original.sub(/\n\z/, " ")); expect_failure("non-canonical manifest") { verify(root, manifest) }; File.binwrite(manifest, original.sub(/"treeSha256":"[0-9a-f]+"/, '"treeSha256":"bad"')); expect_failure("malformed hash") { verify(root, manifest) }; File.binwrite(manifest, original)
      File.symlink("a.txt", "#{root}/tree-link.json"); expect_failure("output symlink") { write(root, "#{root}/tree-link.json") }; File.unlink("#{root}/tree-link.json"); expect_failure("outside output") { write(root, "#{Dir.tmpdir}/tree.json") }
      puts "self-test OK"
    end
  end
end

begin
  case ARGV[0]
  when "write" then raise "usage" unless ARGV.length == 3; EvidenceTree.write(ARGV[1], ARGV[2])
  when "verify" then raise "usage" unless ARGV.length == 3; EvidenceTree.verify(ARGV[1], ARGV[2])
  when "self-test" then raise "usage" unless ARGV.length == 1; EvidenceTree.self_test
  else raise "usage: write ROOT OUTPUT | verify ROOT MANIFEST | self-test"
  end
rescue StandardError => error
  warn "p16-evidence: #{error.message}"
  exit 1
end
