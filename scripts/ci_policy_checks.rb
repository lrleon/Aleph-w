#!/usr/bin/env ruby

require 'open3'
require 'pathname'
begin
  require 'unicode_normalize'
rescue LoadError
  warn '[warn] unicode_normalize extension not available; documentation language check is best-effort.'
end

HEADER_EXTS = %w[.h .H .hpp .hxx .hh].freeze
TEST_EXTS = %w[.cc .cpp .C .cxx].freeze
VENDORED_PATH_PREFIXES = %w[third_party/].freeze

# The standard containers and sorts that Aleph has, which the library uses
# instead (CONTRIBUTING.md, "Coding conventions")
STD_NAMES_WITH_ALEPH_EQUIVALENT = %w[
  vector deque list forward_list map multimap set multiset
  unordered_map unordered_multimap unordered_set unordered_multiset
  queue stack priority_queue sort stable_sort partial_sort
].freeze

# What marks a line that uses one of them to interoperate with the
# standard library (docs/std-vector-whitelist.md)
STD_INTEROP_MARKER = 'std-interop:'.freeze

# The files of the library whose purpose is to interoperate with the
# standard library
STD_INTEROP_FILES = %w[
  ah-convert.H ah-convert-common.H ah-stl-functional.H ah-stl-utils.H
  ah-stl-zip.H ah-uni-functional.H ah-zip.H ah-zip-utils.H
].freeze


def vendored_path?(path)
  VENDORED_PATH_PREFIXES.any? { |prefix| path.start_with?(prefix) }
end


def run!(cmd)
  out, err, status = Open3.capture3(*cmd)
  raise "command failed: #{cmd.join(' ')}\n#{err.strip}" unless status.success?
  out
end


def github_event_name
  ENV.fetch('GITHUB_EVENT_NAME', '')
end


def base_ref
  ENV['GITHUB_BASE_REF']
end


def changed_files(base)
  return [] if base.nil? || base.empty?

  run!(['git', 'fetch', '--no-tags', '--prune', 'origin', base])
  out = run!(['git', 'diff', '--name-status', '--diff-filter=ACMR', "origin/#{base}...HEAD"])
  out.lines.map do |line|
    parts = line.strip.split("\t")
    status = parts[0]
    if status.start_with?('R', 'C')
      { status: status[0], old_path: parts[1], path: parts[2] }
    else
      { status: status, old_path: parts[1], path: parts[1] }
    end
  end
end


def git_show_optional(spec)
  out, err, status = Open3.capture3('git', 'show', spec)
  return out if status.success?
  return nil if err.match?(/exists on disk, but not in|does not exist in/)

  raise "command failed: git show #{spec}\n#{err.strip}"
end


def read_quoted_literal(content, start_idx)
  quote = content[start_idx]
  idx = start_idx + 1

  while idx < content.length
    ch = content[idx]
    if ch == '\\'
      idx += 2
      next
    end

    idx += 1
    break if ch == quote
  end

  [content[start_idx...idx], idx]
end


def read_raw_string_literal(content, start_idx)
  return nil unless content[start_idx] == 'R' && content[start_idx + 1] == '"'

  delimiter_start = start_idx + 2
  delimiter_end = delimiter_start

  while delimiter_end < content.length && content[delimiter_end] != '('
    ch = content[delimiter_end]
    return nil if ch =~ /[\s\\)]/

    delimiter_end += 1
  end

  return nil if delimiter_end >= content.length || content[delimiter_end] != '('

  delimiter = content[delimiter_start...delimiter_end]
  terminator = ')' + delimiter + '"'
  literal_end = content.index(terminator, delimiter_end + 1)
  return nil if literal_end.nil?

  literal_end += terminator.length
  [content[start_idx...literal_end], literal_end]
end


def cpp_token_stream(content)
  tokens = []
  idx = 0

  while idx < content.length
    if content[idx] =~ /\s/
      idx += 1
      next
    end

    if content[idx, 2] == '//'
      idx += 2
      idx += 1 while idx < content.length && content[idx] != "\n"
      next
    end

    if content[idx, 2] == '/*'
      idx += 2
      idx += 1 while idx < content.length && content[idx, 2] != '*/'
      idx += 2 if idx < content.length
      next
    end

    if (literal = read_raw_string_literal(content, idx))
      tokens << literal[0]
      idx = literal[1]
      next
    end

    if content[idx] == '"' || content[idx] == "'"
      literal, idx = read_quoted_literal(content, idx)
      tokens << literal
      next
    end

    if content[idx] =~ /[A-Za-z_]/
      start_idx = idx
      idx += 1
      idx += 1 while idx < content.length && content[idx] =~ /[A-Za-z0-9_]/
      tokens << content[start_idx...idx]
      next
    end

    if content[idx] =~ /\d/
      start_idx = idx
      idx += 1
      idx += 1 while idx < content.length && content[idx] =~ /[A-Za-z0-9_'.]/
      tokens << content[start_idx...idx]
      next
    end

    tokens << content[idx]
    idx += 1
  end

  tokens.join("\n")
end


def header_has_code_changes?(base, path, old_path = nil)
  current = File.read(path)
  previous = git_show_optional("origin/#{base}:#{old_path || path}") || ''
  cpp_token_stream(current) != cpp_token_stream(previous)
end


# Whether path is a file of the library, in the root of the repository,
# that must not add the standard containers and sorts that Aleph has
def std_checked_library_file?(path)
  !path.include?('/') && (HEADER_EXTS + ['.C']).include?(File.extname(path)) &&
    !STD_INTEROP_FILES.include?(path)
end


# How many times the code of content names each of
# STD_NAMES_WITH_ALEPH_EQUIVALENT, without its comments and literals, nor
# the lines that STD_INTEROP_MARKER marks
def std_uses(content)
  code = content.lines.map do |line|
    line.include?(STD_INTEROP_MARKER) ? line.gsub(/\bstd(\s*::)/, 'std_interop\1') : line
  end.join
  counts = Hash.new(0)
  cpp_token_stream(code).split("\n").each_cons(4) do |a, b, c, name|
    counts[name] += 1 if a == 'std' && b == ':' && c == ':' && STD_NAMES_WITH_ALEPH_EQUIVALENT.include?(name)
  end
  counts
end


# The standard containers and sorts that Aleph has and that a file adds,
# with how many: those that its code names more times than in the base.
# Counting by file, moving code does not add any
def added_std_uses(base, path, old_path = nil)
  current = std_uses(File.read(path))
  previous = std_uses(git_show_optional("origin/#{base}:#{old_path || path}") || '')
  current.each_with_object({}) do |(name, n), added|
    added[name] = n - previous[name] if n > previous[name]
  end
end


def mit_license_header?(path)
  begin
    head = path.read.lines.first(80)
  rescue StandardError
    return false
  end

  txt = head.join
  required = [
    /Permission is hereby granted/i,
    /The above copyright notice and this permission notice/i,
    /THE SOFTWARE IS PROVIDED "AS IS"/i
  ]

  required.all? { |r| txt.match?(r) }
end


def english_documentation?(path)
  begin
    content = File.read(path)
  rescue StandardError => e
    $stderr.puts "english_documentation?: could not read #{path}: #{e.message}"
    return false
  end
  block_docs = content.scan(%r{/\*[*!].*?\*/}m).join(' ')
  line_docs = content.scan(%r{^\s*(?:///|//!).*$}).join(' ')
  docs = [block_docs, line_docs].reject(&:empty?).join(' ')
  return true if docs.empty?

  # Strip code blocks before scanning for Spanish words to avoid false positives
  # from identifiers or examples inside fenced blocks, @code...@endcode, or
  # inline backtick spans.
  docs_without_code = docs
    .gsub(/```.*?```/m, ' ')
    .gsub(/@code\b.*?@endcode\b/m, ' ')
    .gsub(/`[^`]*`/, ' ')

  # Check for common Spanish words that shouldn't be in English documentation
  # (avoiding very short words or those that overlap with English/technical terms)
  # Note: accented variants are normalized to ASCII during comparison when
  # unicode_normalize is available (best-effort otherwise).
  spanish_words = %w[
    algoritmo biblioteca cabecera funcion parametro retorno estructura
    herencia polimorfismo puntero memoria asignacion busqueda ordenamiento
    grafo nodo arista camino ciclo arbol hoja raiz
    implementacion descripcion ejemplo advertencia opcional requerido
    devuelve booleano entero cadena caracter
  ]

  # Normalize text and each word to ASCII (strip diacritics) so both accented
  # and unaccented forms are caught by a single comparison.
  unicode_normalize_supported = ''.respond_to?(:unicode_normalize)
  normalize = lambda do |str|
    return str unless unicode_normalize_supported

    str.unicode_normalize(:nfd).gsub(/\p{Mn}/, '')
  end

  normalized_docs = normalize.call(docs_without_code)
  normalized_words = spanish_words.map { |w| normalize.call(w) }.uniq

  has_spanish = normalized_words.any? do |w|
    normalized_docs.match?(/(^|[^\p{L}])#{Regexp.escape(w)}([^\p{L}]|$)/i)
  end

  !has_spanish
end

def run_regression_tests
  puts '[info] running regression tests...'
  test_script = File.join(File.dirname(__FILE__), 'test_ci_policy_checks.rb')
  system('ruby', test_script) || raise('Regression tests failed')
end


def main
  run_regression_tests if github_event_name == 'pull_request'

  if github_event_name != 'pull_request'
    puts '[skip] not a pull_request event'
    return 0
  end

  base = base_ref
  files_details = changed_files(base)

  headers_details = files_details.select { |d| HEADER_EXTS.include?(File.extname(d[:path])) }
  headers_changed = headers_details.map { |d| d[:path] }
  sources_changed = files_details.select { |d| %w[.C .cc .cpp .cxx].include?(File.extname(d[:path])) }.map { |d| d[:path] }
  tests_changed = files_details.select { |d| d[:path].start_with?('Tests/') && TEST_EXTS.include?(File.extname(d[:path])) }.map { |d| d[:path] }

  headers_requiring_tests = headers_details.select do |detail|
    p = Pathname.new(detail[:path])
    p.exist? && !vendored_path?(detail[:path]) &&
      header_has_code_changes?(base, detail[:path], detail[:old_path])
  end.map { |d| d[:path] }

  failures = []

  (headers_changed + sources_changed).each do |file|
    p = Pathname.new(file)
    next unless p.exist?
    next if vendored_path?(file)

    failures << "missing/invalid MIT license header: #{file}" if HEADER_EXTS.include?(File.extname(file)) && !mit_license_header?(p)
    failures << "documentation seems to be in Spanish (must be English): #{file}" unless english_documentation?(p)
  end

  if !headers_requiring_tests.empty? && tests_changed.empty?
    failures << "headers with code changes but no Tests/ files changed (add/modify tests): #{headers_requiring_tests.join(', ')}"
  end

  files_details.each do |detail|
    path = detail[:path]
    next unless std_checked_library_file?(path) && File.exist?(path)

    added = added_std_uses(base, path, detail[:old_path])
    next if added.empty?

    names = added.map { |name, n| "std::#{name} (#{n} more)" }.join(', ')
    failures << "#{path} adds standard containers or sorts that Aleph has: #{names}; " \
                'use those of Aleph (Array, DynList, DynSetTree, DynMapTree, DynBinHeap, ' \
                'DynListQueue, in_place_sort(), mergesort()...), or end each line that ' \
                "interoperates with the standard library with `// #{STD_INTEROP_MARKER} <why>`"
  end

  unless failures.empty?
    failures.each { |f| puts "[fail] #{f}" }
    return 1
  end

  puts "[ok] headers changed: #{headers_changed.size}"
  puts "[ok] headers requiring tests: #{headers_requiring_tests.size}"
  puts "[ok] tests changed: #{tests_changed.size}"
  0
end


if __FILE__ == $0
  exit(main)
end
