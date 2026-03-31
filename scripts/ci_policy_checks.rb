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
  out = run!(['git', 'diff', '--name-only', '--diff-filter=ACMR', "origin/#{base}...HEAD"])
  out.lines.map(&:strip).reject(&:empty?)
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


def header_has_code_changes?(base, path)
  current = File.read(path)
  previous = git_show_optional("origin/#{base}:#{path}") || ''
  cpp_token_stream(current) != cpp_token_stream(previous)
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
  files = changed_files(base)

  headers_changed = files.select { |f| HEADER_EXTS.include?(File.extname(f)) }
  sources_changed = files.select { |f| %w[.C .cc .cpp .cxx].include?(File.extname(f)) }
  tests_changed = files.select { |f| f.start_with?('Tests/') && TEST_EXTS.include?(File.extname(f)) }
  headers_requiring_tests = headers_changed.select do |file|
    p = Pathname.new(file)
    p.exist? && header_has_code_changes?(base, file)
  end

  failures = []

  (headers_changed + sources_changed).each do |file|
    p = Pathname.new(file)
    next unless p.exist?

    failures << "missing/invalid MIT license header: #{file}" if HEADER_EXTS.include?(File.extname(file)) && !mit_license_header?(p)
    failures << "documentation seems to be in Spanish (must be English): #{file}" unless english_documentation?(p)
  end

  if !headers_requiring_tests.empty? && tests_changed.empty?
    failures << "headers with code changes but no Tests/ files changed (add/modify tests): #{headers_requiring_tests.join(', ')}"
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
