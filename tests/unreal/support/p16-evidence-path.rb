#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "tmpdir"

module P16EvidencePath
  module_function

  def fail!(message)
    raise "P1.6 evidence path failed: #{message}"
  end

  def write(evidence, output)
    evidence = File.realpath(evidence)
    fail! "evidence must be directory" unless File.directory?(evidence) && !File.symlink?(evidence)
    fail! "output must be a new absolute path" unless output.start_with?(File::SEPARATOR) && !File.exist?(output) && !File.symlink?(output)
    parent = File.realpath(File.dirname(output))
    target = File.join(parent, File.basename(output))
    temporary = File.join(parent, ".#{File.basename(output)}.#{Process.pid}.tmp")
    begin
      File.open(temporary, File::WRONLY | File::CREAT | File::EXCL, 0o600) do |file|
        file.write("#{evidence}\n")
        file.flush
        file.fsync
      end
      File.link(temporary, target)
      File.unlink(temporary)
      File.open(parent, File::RDONLY) { |directory| directory.fsync }
    ensure
      File.unlink(temporary) if File.exist?(temporary)
    end
    target
  rescue Errno::ENOENT, Errno::EEXIST, Errno::ELOOP => error
    fail! error.message
  end

  def expect_failure(label)
    yield
    raise "self-test failed: #{label} accepted"
  rescue RuntimeError => error
    raise if error.message.start_with?("self-test failed")
  end

  def self_test
    Dir.mktmpdir("p16-evidence-path-") do |root|
      evidence = File.join(root, "evidence")
      FileUtils.mkdir_p(evidence)
      output = File.join(root, "combined-path")
      write(evidence, output)
      fail! "content" unless File.binread(output) == "#{File.realpath(evidence)}\n"
      fail! "mode" unless File.stat(output).mode & 0o777 == 0o600
      expect_failure("existing output") { write(evidence, output) }
      occupied = File.join(root, "occupied"); File.write(occupied, "racer\n")
      expect_failure("occupied output") { write(evidence, occupied) }
      fail! "occupied output replaced" unless File.binread(occupied) == "racer\n"
      File.symlink("missing", File.join(root, "link"))
      expect_failure("symlink output") { write(evidence, File.join(root, "link")) }
      expect_failure("relative output") { write(evidence, "relative") }
      puts "P1.6 evidence path self-test: PASS"
    end
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    if ARGV.length == 3 && ARGV[0] == "write"
      P16EvidencePath.write(ARGV[1], ARGV[2])
    elsif [%w[self-test], %w[--self-test]].include?(ARGV)
      P16EvidencePath.self_test
    else
      warn "usage: #{File.basename($PROGRAM_NAME)} write EVIDENCE OUTPUT | self-test"
      exit 2
    end
  rescue StandardError => error
    warn error.message
    exit 1
  end
end
