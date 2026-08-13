#!/usr/bin/env ruby
# frozen_string_literal: true

require "digest"
require "fileutils"
require "json"
require "pathname"
require "securerandom"
require "tmpdir"

module P16RunInventory
  module_function

  SCHEMA = "magi-unreal-axi/run-inventory/v1"
  INVENTORY_NAME = "run-inventory.json"
  PATH_CONTROLS = /[\x00-\x1f\x7f\u200b-\u200f\u202a-\u202e\u2060-\u206f\ufeff]/.freeze
  HEX_SHA256 = /\A[0-9a-f]{64}\z/.freeze
  MAX_ARTIFACT_FILE_BYTES = 2 * 1024 * 1024 * 1024
  MAX_TOTAL_BYTES = 16 * 1024 * 1024 * 1024
  MAX_FILES = 100_000
  MAX_NODES = 200_000
  MAX_DEPTH = 64
  MAX_PATH_BYTES = 4096
  MAX_INVENTORY_BYTES = 256 * 1024 * 1024

  def fail(message)
    raise "P1.6 run inventory failed: #{message}"
  end

  def canonical(value)
    case value
    when Hash
      value.keys.sort_by { |key| key.encode(Encoding::UTF_8).bytes }.to_h { |key| [key, canonical(value.fetch(key))] }
    when Array then value.map { |item| canonical(item) }
    else value
    end
  end

  def canonical_json(value)
    JSON.generate(canonical(value), ascii_only: false)
  end

  def safe_path(path)
    text = path.dup.force_encoding(Encoding::UTF_8)
    fail "invalid UTF-8 path" unless text.valid_encoding?
    parts = text.split("/", -1)
    fail "unsafe path #{path.inspect}" if text.empty? || text.start_with?("/") || text.include?("\\") || text.match?(PATH_CONTROLS) || parts.any? { |part| part.empty? || part == "." || part == ".." || part != part.unicode_normalize(:nfc) }
    fail "path exceeds byte bound" if text.bytesize > MAX_PATH_BYTES
    fail "path exceeds depth bound" if parts.length > MAX_DEPTH
    text
  end

  def regular_inventory(path)
    stat = File.lstat(path)
    stat.file? && !stat.symlink? && stat.nlink == 1
  rescue Errno::ENOENT
    false
  end

  def stable_file(path, relative, expected, accounting)
    fail "NOFOLLOW unavailable" unless File.const_defined?(:NOFOLLOW)
    flags = File::RDONLY | File::NOFOLLOW
    File.open(path, flags) do |io|
      before = io.stat
      fail "file changed before hashing #{relative}" unless before.file? && before.nlink == 1 && [before.dev, before.ino] == [expected.dev, expected.ino]
      digest = Digest::SHA256.new
      bytes = 0
      while (chunk = io.read(1024 * 1024))
        bytes += chunk.bytesize
        accounting[:total] += chunk.bytesize
        fail "file too large #{relative}" if bytes > MAX_ARTIFACT_FILE_BYTES
        fail "total file bytes exceed bound" if accounting[:total] > MAX_TOTAL_BYTES
        digest.update(chunk)
      end
      after = io.stat
      identity = ->(stat) { [stat.dev, stat.ino, stat.size, stat.mtime.to_r, stat.ctime.to_r, stat.nlink] }
      fail "file changed while hashing #{relative}" unless identity.call(before) == identity.call(after)
      {"path" => relative, "type" => "file", "bytes" => bytes, "sha256" => digest.hexdigest}
    end
  rescue Errno::ELOOP
    fail "symlink rejected #{relative}"
  end

  def stable_inventory(path)
    fail "NOFOLLOW unavailable" unless File.const_defined?(:NOFOLLOW)
    expected = File.lstat(path)
    fail "inventory path must be regular and unlinked" unless expected.file? && !expected.symlink? && expected.nlink == 1
    File.open(path, File::RDONLY | File::NOFOLLOW) do |io|
      before = io.stat
      fail "inventory changed before reading" unless [before.dev, before.ino] == [expected.dev, expected.ino] && before.file? && before.nlink == 1
      raw = io.read(MAX_INVENTORY_BYTES + 1)
      fail "inventory too large" if raw.bytesize > MAX_INVENTORY_BYTES
      after = io.stat
      identity = ->(stat) { [stat.dev, stat.ino, stat.size, stat.mtime.to_r, stat.ctime.to_r, stat.nlink] }
      fail "inventory changed while reading" unless identity.call(before) == identity.call(after)
      raw
    end
  rescue Errno::ELOOP
    fail "inventory symlink rejected"
  end

  def entries(root)
    fail "NOFOLLOW unavailable" unless File.const_defined?(:NOFOLLOW)
    expanded = File.expand_path(root)
    fail "RUN must be directory" unless File.directory?(expanded) && !File.symlink?(expanded)
    root = File.realpath(expanded)
    root_stat = File.lstat(root)
    root_identity = [root_stat.dev, root_stat.ino, root_stat.mode, root_stat.nlink]
    seen_dirs = {[root_stat.dev, root_stat.ino] => true}
    seen_files = {}
    result = []
    files = 0
    accounting = {total: 0}
    stack = [[root, [root_stat.dev, root_stat.ino]]]
    until stack.empty?
      directory, expected_directory = stack.pop
      current_directory = File.lstat(directory)
      fail "directory changed before descent" unless [current_directory.dev, current_directory.ino] == expected_directory
      before_stat = current_directory
      before_children = Dir.children(directory).sort
      before_children.sort_by { |name| name.encode(Encoding::UTF_8).bytes }.reverse_each do |name|
        path = File.join(directory, name)
        relative = safe_path(path.delete_prefix("#{root}/"))
        stat = File.lstat(path)
        if relative == INVENTORY_NAME
          fail "inventory path must be regular and unlinked" unless stat.file? && !stat.symlink? && stat.nlink == 1
          next
        end
        fail "symlink rejected #{relative}" if stat.symlink?
        if stat.directory?
          identity = [stat.dev, stat.ino]
          fail "directory cycle or hardlink rejected #{relative}" if seen_dirs.key?(identity)
          seen_dirs[identity] = true
          resolved = File.realpath(path)
          fail "directory escaped RUN #{relative}" unless resolved == root || resolved.start_with?("#{root}/")
          result << {"path" => relative, "type" => "directory"}
          fail "too many nodes" if result.length > MAX_NODES
          stack << [resolved, identity]
        elsif stat.file?
          identity = [stat.dev, stat.ino]
          fail "hardlink rejected #{relative}" if stat.nlink != 1 || seen_files.key?(identity)
          seen_files[identity] = true
          files += 1
          fail "too many files" if files > MAX_FILES
          result << stable_file(path, relative, stat, accounting)
          fail "too many nodes" if result.length > MAX_NODES
        else
          fail "special file rejected #{relative}"
        end
      end
      after_stat = File.lstat(directory)
      after_children = Dir.children(directory).sort
      fail "directory changed while reading #{directory}" unless [after_stat.dev, after_stat.ino, after_stat.mode, after_stat.nlink] == [before_stat.dev, before_stat.ino, before_stat.mode, before_stat.nlink] && before_children == after_children
    end
    final_root = File.lstat(root)
    fail "RUN root changed" unless [final_root.dev, final_root.ino, final_root.mode, final_root.nlink] == root_identity
    result.sort_by { |entry| entry.fetch("path").encode(Encoding::UTF_8).bytes }
  end

  def materialize_verified(source, expected, destination)
    fail "expected inventory SHA must be lowercase hexadecimal" unless expected.is_a?(String) && expected.match?(HEX_SHA256)
    expanded = File.expand_path(source)
    fail "RUN must be directory" unless File.directory?(expanded) && !File.symlink?(expanded)
    destination = File.expand_path(destination)
    fail "snapshot destination must be new private directory" if File.exist?(destination) || File.symlink?(destination)
    raw = stable_inventory(output_path(expanded))
    fail "inventory SHA mismatch" unless Digest::SHA256.hexdigest(raw) == expected
    manifest = JSON.parse(raw)
    listed = manifest.fetch("entries")
    verify(expanded, expected)
    Dir.mkdir(destination, 0o700)
    listed.each do |entry|
      relative = safe_path(entry.fetch("path"))
      target = File.join(destination, relative)
      if entry.fetch("type") == "directory"
        Dir.mkdir(target, 0o700)
        next
      end
      File.open(File.join(expanded, relative), File::RDONLY | File::NOFOLLOW) do |input|
        before = input.stat
        fail "snapshot source is not stable regular file #{relative}" unless before.file? && before.nlink == 1 && before.size == entry.fetch("bytes")
        digest = Digest::SHA256.new
        bytes = 0
        File.open(target, File::WRONLY | File::CREAT | File::EXCL, 0o600) do |output|
          while (chunk = input.read(1024 * 1024))
            bytes += chunk.bytesize
            fail "snapshot file too large #{relative}" if bytes > entry.fetch("bytes")
            digest.update(chunk)
            output.write(chunk)
          end
          output.flush
          output.fsync
        end
        after = input.stat
        identity = ->(stat) { [stat.dev, stat.ino, stat.size, stat.mtime.to_r, stat.ctime.to_r, stat.nlink] }
        fail "snapshot source changed #{relative}" unless identity.call(before) == identity.call(after) && bytes == entry.fetch("bytes") && digest.hexdigest == entry.fetch("sha256")
      end
    end
    File.binwrite(output_path(destination), raw)
    verify(destination, expected)
    verify(expanded, expected)
    destination
  rescue JSON::ParserError, KeyError => error
    fail "invalid inventory for snapshot: #{error.message}"
  end

  def tree_sha(entries)
    Digest::SHA256.hexdigest(canonical_json(entries))
  end

  def manifest_for(root)
    listed = entries(root)
    {"entries" => listed, "schema" => SCHEMA, "treeSha256" => tree_sha(listed)}
  end

  def output_path(root)
    File.join(root, INVENTORY_NAME)
  end

  def write(run)
    expanded = File.expand_path(run)
    fail "RUN must be directory" unless File.directory?(expanded) && !File.symlink?(expanded)
    root = File.realpath(expanded)
    output = output_path(root)
    fail "run inventory already exists" if File.exist?(output) || File.symlink?(output)
    manifest = manifest_for(root)
    raw = canonical_json(manifest) << "\n"
    temporary = File.join(root, ".#{INVENTORY_NAME}.#{Process.pid}.#{SecureRandom.hex(8)}")
    begin
      File.open(temporary, File::WRONLY | File::CREAT | File::EXCL, 0o600) { |io| io.write(raw); io.flush; io.fsync }
      File.chmod(0o644, temporary)
      File.link(temporary, output)
      File.unlink(temporary)
      File.open(root, File::RDONLY) { |io| io.fsync }
    ensure
      File.unlink(temporary) if File.exist?(temporary)
    end
    inventory_sha = Digest::SHA256.hexdigest(raw)
    puts "inventorySha256=#{inventory_sha} treeSha256=#{manifest.fetch("treeSha256")}"
  end

  def verify(run, expected)
    fail "expected inventory SHA must be lowercase hexadecimal" unless expected.is_a?(String) && expected.match?(HEX_SHA256)
    expanded = File.expand_path(run)
    fail "RUN must be directory" unless File.directory?(expanded) && !File.symlink?(expanded)
    root = File.realpath(expanded)
    root_stat = File.lstat(root)
    root_identity = [root_stat.dev, root_stat.ino, root_stat.mode, root_stat.nlink]
    path = output_path(root)
    fail "missing run inventory" unless regular_inventory(path)
    raw = stable_inventory(path)
    fail "inventory must be UTF-8" unless raw.dup.force_encoding(Encoding::UTF_8).valid_encoding?
    fail "inventory SHA mismatch" unless Digest::SHA256.hexdigest(raw) == expected
    manifest = JSON.parse(raw)
    fail "inventory is not canonical" unless manifest.is_a?(Hash) && manifest.keys.sort == %w[entries schema treeSha256].sort && raw == canonical_json(manifest) << "\n"
    fail "unsupported schema" unless manifest["schema"] == SCHEMA
    listed = manifest["entries"]
    fail "invalid entries" unless listed.is_a?(Array) && listed.length <= MAX_NODES
    listed.each do |entry|
      fail "invalid inventory entry" unless entry.is_a?(Hash) && entry["path"].is_a?(String) && entry["type"].is_a?(String)
      safe_path(entry["path"])
      fail "invalid entry type" unless %w[file directory].include?(entry["type"])
      if entry["type"] == "file"
        fail "invalid file entry" unless entry.keys.sort == %w[bytes path sha256 type].sort && entry["bytes"].is_a?(Integer) && entry["bytes"].between?(0, MAX_ARTIFACT_FILE_BYTES) && entry["sha256"].match?(HEX_SHA256)
      else
        fail "invalid directory entry" unless entry.keys.sort == %w[path type].sort
      end
    end
    fail "entries not sorted or unique" unless listed.map { |e| e["path"] } == listed.map { |e| e["path"] }.uniq.sort_by { |p| p.encode(Encoding::UTF_8).bytes }
    fail "invalid tree SHA" unless manifest["treeSha256"].is_a?(String) && manifest["treeSha256"].match?(HEX_SHA256)
    fail "tree SHA mismatch" unless tree_sha(listed) == manifest["treeSha256"]
    actual = entries(root)
    second_actual = entries(root)
    second_raw = stable_inventory(path)
    fail "inventory changed during verification" unless Digest::SHA256.hexdigest(second_raw) == expected && second_raw == raw
    fail "run contents do not match inventory" unless actual == listed && second_actual == listed && second_actual == actual
    final_root = File.lstat(root)
    fail "RUN root changed" unless [final_root.dev, final_root.ino, final_root.mode, final_root.nlink] == root_identity
    puts "OK inventorySha256=#{expected} treeSha256=#{manifest.fetch("treeSha256")}"
  rescue JSON::ParserError => e
    fail "invalid inventory JSON: #{e.message}"
  end

  def expect_failure(label)
    failed = false
    begin
      yield
    rescue RuntimeError
      failed = true
    end
    fail "self-test #{label} accepted" unless failed
  end

  def self_test
    Dir.mktmpdir("p16-run-inventory-") do |root|
      FileUtils.mkdir_p(File.join(root, "nested")); File.binwrite(File.join(root, "a.txt"), "alpha\n"); File.binwrite(File.join(root, "nested", "b"), "beta")
      manifest = manifest_for(root); raw = canonical_json(manifest) << "\n"; expected = Digest::SHA256.hexdigest(raw)
      File.binwrite(output_path(root), raw); verify(root, expected)
      File.binwrite(File.join(root, "new"), "new"); expect_failure("add") { verify(root, expected) }; File.unlink(File.join(root, "new"))
      File.unlink(File.join(root, "nested", "b")); expect_failure("delete") { verify(root, expected) }; File.binwrite(File.join(root, "nested", "b"), "beta")
      File.binwrite(File.join(root, "a.txt"), "tampered"); expect_failure("tamper") { verify(root, expected) }; File.binwrite(File.join(root, "a.txt"), "alpha\n")
      File.symlink("a.txt", File.join(root, "link")); expect_failure("symlink") { entries(root) }; File.unlink(File.join(root, "link"))
      begin File.link(File.join(root, "a.txt"), File.join(root, "hardlink")); expect_failure("hardlink") { entries(root) }; ensure File.unlink(File.join(root, "hardlink")) if File.exist?(File.join(root, "hardlink")); end
      File.binwrite(output_path(root), raw.sub(/\n\z/, " ")); expect_failure("noncanonical") { verify(root, expected) }; File.binwrite(output_path(root), raw)
      expect_failure("wrong external hash") { verify(root, "A" * 64) }
      snapshot = File.join(Dir.mktmpdir("p16-run-snapshot-parent-"), "snapshot")
      materialize_verified(root, expected, snapshot)
      verify(snapshot, expected)
      File.binwrite(File.join(snapshot, "a.txt"), "tampered")
      expect_failure("snapshot tamper") { verify(snapshot, expected) }
    end
    puts "self-test OK"
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    if ARGV == ["self-test"] || ARGV == ["--self-test"]
      P16RunInventory.self_test
    elsif ARGV.first == "write"
      raise "usage: write RUN" unless ARGV.length == 2
      P16RunInventory.write(ARGV[1])
    elsif ARGV.first == "verify"
      raise "usage: verify RUN EXPECTED_INVENTORY_SHA" unless ARGV.length == 3
      P16RunInventory.verify(ARGV[1], ARGV[2])
    elsif ARGV.first == "snapshot"
      raise "usage: snapshot RUN EXPECTED_INVENTORY_SHA DESTINATION" unless ARGV.length == 4
      P16RunInventory.materialize_verified(ARGV[1], ARGV[2], ARGV[3])
    else
      raise "usage: write RUN | verify RUN EXPECTED_INVENTORY_SHA | snapshot RUN EXPECTED_INVENTORY_SHA DESTINATION | self-test"
    end
  rescue StandardError => e
    warn e.message
    exit 1
  end
end
