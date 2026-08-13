#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "stringio"
require "zlib"

class ArchiveError < StandardError; end

module P16Archive
  module_function

  def verify(archive, manifest, version)
    release = JSON.parse(File.binread(manifest)).fetch("releaseArchive")
    compressed = File.binread(archive)
    raise ArchiveError, "compressed size" if compressed.bytesize > release.fetch("maxCompressedBytes")
    expected = release.fetch("allowlist").map { |path| path.gsub("{version}", version) }.sort
    payload = gunzip(compressed)
    entries = parse_tar(payload, expected, release.fetch("maxFileBytes"), release.fetch("maxTotalUncompressedBytes"))
    raise ArchiveError, "entry count" unless entries == expected
    true
  rescue Errno::ENOENT, JSON::ParserError, KeyError => e
    raise ArchiveError, "invalid input: #{e.message}"
  end

  def gunzip(data)
    raise ArchiveError, "gzip header" unless data.bytesize >= 18 && data.getbyte(0) == 0x1f && data.getbyte(1) == 0x8b && data.getbyte(2) == 8
    raise ArchiveError, "gzip flags" unless data.getbyte(3) == 0
    raise ArchiveError, "gzip mtime" unless data.byteslice(4, 4) == "\0\0\0\0"
    raise ArchiveError, "gzip fixed header" unless data.getbyte(8) == 0 && data.getbyte(9) == 3
    reader = Zlib::GzipReader.new(StringIO.new(data))
    output = reader.read
    trailing = reader.unused
    reader.close
    raise ArchiveError, "gzip trailing data" unless trailing.nil? || trailing.empty?
    output
  rescue Zlib::Error, EOFError => e
    raise ArchiveError, "gzip: #{e.message}"
  end

  def parse_tar(data, expected, max_file, max_total)
    names = []
    offset = 0
    total = 0
    loop do
      raise ArchiveError, "truncated header" if offset + 512 > data.bytesize
      header = data.byteslice(offset, 512)
      offset += 512
      if header == "\0" * 512
        raise ArchiveError, "trailer" unless data.byteslice(offset, 512) == "\0" * 512 && offset + 512 == data.bytesize
        break
      end
      raise ArchiveError, "ustar header" unless header.byteslice(257, 6) == "ustar\0" && header.byteslice(263, 2) == "00"
      type = header.getbyte(156).chr
      raise ArchiveError, "pax, link, or special entry" unless %w[0 5].include?(type)
      raise ArchiveError, "header checksum" unless header.byteslice(148, 8).match?(/\A[0-7]{6}\0 \z/) && octal(header.byteslice(148, 8)) == checksum(header)
      raise ArchiveError, "owner" unless header.byteslice(108, 8) == "000000 \0" && header.byteslice(116, 8) == "000000 \0" && zero_field?(header, 265, 64)
      raise ArchiveError, "link" unless zero_field?(header, 157, 100)
      raise ArchiveError, "prefix" unless zero_field?(header, 345, 155)
      raise ArchiveError, "reserved header" unless zero_field?(header, 500, 12)
      raise ArchiveError, "device metadata" unless header.byteslice(329, 8) == "000000 \0" && header.byteslice(337, 8) == "000000 \0"
      name_field = header.byteslice(0, 100)
      name = name_field.sub(/\0.*\z/m, "")
      raise ArchiveError, "path encoding" unless name.dup.force_encoding(Encoding::UTF_8).valid_encoding? && name_field.byteslice(name.bytesize, 100 - name.bytesize).bytes.all?(&:zero?)
      size = canonical_octal(header.byteslice(124, 12), 11, " ")
      mode = canonical_octal(header.byteslice(100, 8), 6, " \0")
      canonical_octal(header.byteslice(136, 12), 11, " ").zero? || raise(ArchiveError, "mtime")
      raise ArchiveError, "path" unless expected.include?(name) && (names.empty? || names.last < name)
      raise ArchiveError, "size" if size > max_file
      if type == "5"
        raise ArchiveError, "directory metadata" unless mode == 0o755 && size.zero? && name.end_with?("/")
      else
        expected_mode = name.end_with?("/magi-unreal-axi") ? 0o755 : 0o644
        raise ArchiveError, "file metadata" unless mode == expected_mode && !name.end_with?("/")
      end
      raise ArchiveError, "total size" if total + size > max_total
      names << name
      total += size
      padded = (size + 511) / 512 * 512
      raise ArchiveError, "truncated payload" if offset + padded > data.bytesize
      padding = data.byteslice(offset + size, padded - size)
      raise ArchiveError, "payload padding" unless padding.bytes.all?(&:zero?)
      offset += padded
    end
    raise ArchiveError, "order or entries" unless names == expected
    names
  end

  def zero_field?(header, offset, length)
    header.byteslice(offset, length).bytes.all?(&:zero?)
  end

  def canonical_octal(field, digits, suffix)
    raise ArchiveError, "numeric field" unless field.bytesize == digits + suffix.bytesize && field.match?(/\A[0-7]{#{digits}}#{Regexp.escape(suffix)}\z/)
    field.byteslice(0, digits).to_i(8)
  end

  def octal(field)
    text = field.delete("\0 ")
    raise ArchiveError, "numeric field" unless text.match?(/\A[0-7]+\z/)
    text.to_i(8)
  end

  def checksum(header)
    header.bytes.each_with_index.sum { |byte, index| (148...156).cover?(index) ? 32 : byte }
  end

  def self_test
    paths = %w[root/ root/magi-unreal-axi root/README.md].sort
    tar = make_tar(paths)
    valid = gzip(tar)
    raise "valid archive rejected" unless parse_tar(gunzip(valid), paths, 1000, 2000) == paths
    {
      "owner" => mutate(tar, 108, "000001 \0"),
      "mode" => mutate(tar, 100, "000644 \0"),
      "mtime" => mutate(tar, 136, "00000000001 "),
      "order" => make_tar(paths.reverse),
      "pax" => mutate(tar, 156, "x"),
      "padding" => mutate(tar, 1025, "y"),
      "gzip-name" => begin
        named = valid.dup
        named.setbyte(3, 8)
        named.insert(10, "name\0")
        named
      end
    }.each do |label, bad|
      begin
        verify_bytes(bad, paths)
        raise "negative #{label} accepted"
      rescue ArchiveError
      end
    end
    true
  end

  def verify_bytes(data, expected)
    parse_tar(gunzip(data), expected, 1000, 2000)
  end

  def mutate(data, offset, bytes)
    copy = data.dup
    copy[offset, bytes.bytesize] = bytes
    copy[148, 8] = format("%06o\0 ", checksum(copy))
    copy
  end

  def make_tar(paths)
    body = paths.map do |path|
      type = path.end_with?("/") ? "5" : "0"
      content = type == "0" ? "x" : ""
      header = "\0" * 512
      header[0, 100] = path.ljust(100, "\0")
      header[100, 8] = format("%06o \0", type == "0" ? (path.end_with?("/magi-unreal-axi") ? 0o755 : 0o644) : 0o755)
      header[108, 8] = header[116, 8] = "000000 \0"
      header[124, 12] = format("%011o ", content.bytesize)
      header[136, 12] = "00000000000 "
      header[148, 8] = "        "
      header[156] = type
      header[257, 6] = "ustar\0"
      header[263, 2] = "00"
      header[329, 8] = header[337, 8] = "000000 \0"
      header[148, 8] = format("%06o\0 ", checksum(header))
      header + content.ljust((content.bytesize + 511) / 512 * 512, "\0")
    end.join + "\0" * 1024
  end

  def gzip(data)
    io = StringIO.new("".b, "w+")
    writer = Zlib::GzipWriter.new(io, Zlib::DEFAULT_COMPRESSION)
    writer.write(data)
    writer.close
    result = io.string
    result[4, 4] = "\0\0\0\0"
    result.setbyte(8, 0); result.setbyte(9, 3); result
  end
end

begin
  if ARGV == ["self-test"]
    P16Archive.self_test
    puts "ok"
  elsif ARGV.length == 4 && ARGV[0] == "verify"
    P16Archive.verify(ARGV[1], ARGV[2], ARGV[3])
    puts "ok"
  else
    warn "usage: #{File.basename($PROGRAM_NAME)} verify ARCHIVE MANIFEST VERSION | self-test"
    exit 2
  end
rescue ArchiveError, RuntimeError => e
  warn e.message
  exit 1
end
