#!/usr/bin/env ruby
# frozen_string_literal: true

require 'optparse'
require 'pathname'
require 'set'

INCLUDE_RE = /^\s*#\s*include\s*[<"]([^>"]+)[>"]/

def parse_hlist(cmake_path)
  tokens = []
  inside = false

  cmake_path.each_line do |line|
    stripped = line.sub(/#.*/, '').strip
    next if stripped.empty? and !inside

    if !inside
      next unless stripped.start_with?('set(HLIST')

      inside = true
      stripped = stripped.sub(/^set\(HLIST/, '').strip
    end

    done = false
    if stripped.end_with?(')')
      stripped = stripped[0...-1].strip
      done = true
    end

    tokens.concat(stripped.split(/\s+/)) unless stripped.empty?

    break if done
  end

  tokens.reject(&:empty?)
end

def repo_header_map(repo_root)
  headers = repo_root.glob('*.H') + repo_root.glob('*.h')
  headers.each_with_object({}) do |path, acc|
    acc[path.basename.to_s] = path
  end
end

def local_include_name(raw_include)
  File.basename(raw_include)
end

def collect_local_include_closure(roots, header_map)
  visited = Set.new
  queue = roots.dup

  until queue.empty?
    header_name = queue.shift
    next if visited.include?(header_name)

    visited.add(header_name)
    path = header_map[header_name]
    next unless path

    path.each_line do |line|
      match = INCLUDE_RE.match(line)
      next unless match

      include_name = local_include_name(match[1].strip)
      next unless header_map.key?(include_name)
      next if visited.include?(include_name)

      queue << include_name
    end
  end

  visited
end

def fail_with(message)
  warn message
  exit 1
end

options = { repo: '.' }

OptionParser.new do |opts|
  opts.banner = 'Usage: install_header_closure_check.rb [options]'
  opts.on('--repo PATH', 'Repository root (default: .)') do |value|
    options[:repo] = value
  end
end.parse!

repo_root = Pathname.new(options[:repo]).realpath
cmake_path = repo_root / 'CMakeLists.txt'
fail_with("CMakeLists.txt not found at #{cmake_path}") unless cmake_path.file?

hlist = parse_hlist(cmake_path)
fail_with('Could not parse HLIST from CMakeLists.txt') if hlist.empty?

header_map = repo_header_map(repo_root)

missing_files = hlist.reject { |name| header_map.key?(name) }
unless missing_files.empty?
  fail_with(
    "HLIST references missing headers:\n" \
    "#{missing_files.sort.map { |name| "  - #{name}" }.join("\n")}"
  )
end

closure = collect_local_include_closure(hlist, header_map)
missing_closure = closure.reject { |name| hlist.include?(name) }

unless missing_closure.empty?
  fail_with(
    "HLIST misses local headers required by installed public headers:\n" \
    "#{missing_closure.sort.map { |name| "  - #{name}" }.join("\n")}"
  )
end

puts "HLIST closure check passed: #{hlist.size} install headers, #{closure.size} reachable local headers."
