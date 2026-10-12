#!/usr/bin/env ruby
# frozen_string_literal: true

require 'optparse'
require 'pathname'
require 'set'

INCLUDE_RE = /^\s*#\s*include\s*[<"]([^>"]+)[>"]/

# Tokens of the CMake list `set(<name> ...)`, without comments.
def parse_cmake_list(cmake_path, name)
  tokens = []
  inside = false
  opening = "set(#{name}"

  cmake_path.each_line do |line|
    stripped = line.sub(/#.*/, '').strip
    next if stripped.empty? and !inside

    if !inside
      next unless stripped == opening or stripped.start_with?("#{opening} ")

      inside = true
      stripped = stripped.delete_prefix(opening).strip
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

hlist = parse_cmake_list(cmake_path, 'HLIST')
fail_with('Could not parse HLIST from CMakeLists.txt') if hlist.empty?

not_installed = parse_cmake_list(cmake_path, 'ALEPH_HEADERS_NOT_INSTALLED')

header_map = repo_header_map(repo_root)

missing_files = hlist.reject { |name| header_map.key?(name) }
unless missing_files.empty?
  fail_with(
    "HLIST references missing headers:\n" \
    "#{missing_files.sort.map { |name| "  - #{name}" }.join("\n")}"
  )
end

duplicated = hlist.tally.select { |_, count| count > 1 }.keys
unless duplicated.empty?
  fail_with(
    "HLIST lists headers more than once:\n" \
    "#{duplicated.sort.map { |name| "  - #{name}" }.join("\n")}"
  )
end

missing_exclusions = not_installed.reject { |name| header_map.key?(name) }
unless missing_exclusions.empty?
  fail_with(
    "ALEPH_HEADERS_NOT_INSTALLED references missing headers:\n" \
    "#{missing_exclusions.sort.map { |name| "  - #{name}" }.join("\n")}"
  )
end

both = hlist & not_installed
unless both.empty?
  fail_with(
    "Headers listed both in HLIST and in ALEPH_HEADERS_NOT_INSTALLED:\n" \
    "#{both.sort.map { |name| "  - #{name}" }.join("\n")}"
  )
end

# Every top-level header is public API unless it is explicitly excluded, so a
# header that is in neither list was left out of the installation by accident.
unlisted = header_map.keys - hlist - not_installed
unless unlisted.empty?
  fail_with(
    "Top-level headers that are neither installed (HLIST) nor excluded " \
    "(ALEPH_HEADERS_NOT_INSTALLED) in CMakeLists.txt:\n" \
    "#{unlisted.sort.map { |name| "  - #{name}" }.join("\n")}"
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

puts "HLIST closure check passed: #{hlist.size} install headers, #{closure.size} reachable local headers, " \
     "#{not_installed.size} top-level headers excluded on purpose."
