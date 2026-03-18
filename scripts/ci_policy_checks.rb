#!/usr/bin/env ruby

require 'open3'
require 'pathname'

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
  # Note: accented variants are normalized to ASCII during comparison
  spanish_words = %w[
    algoritmo biblioteca cabecera funcion parametro retorno estructura
    herencia polimorfismo puntero memoria asignacion busqueda ordenamiento
    grafo nodo arista camino ciclo arbol hoja raiz
    implementacion descripcion ejemplo advertencia opcional requerido
    devuelve booleano entero cadena caracter
  ]

  # Normalize text and each word to ASCII (strip diacritics) so both accented
  # and unaccented forms are caught by a single comparison.
  normalize = lambda do |str|
    str.unicode_normalize(:nfd).gsub(/\p{Mn}/, '')
  end

  normalized_docs = normalize.call(docs_without_code)
  normalized_words = spanish_words.map { |w| normalize.call(w) }.uniq

  has_spanish = normalized_words.any? do |w|
    normalized_docs.match?(/(^|[^\p{L}])#{Regexp.escape(w)}([^\p{L}]|$)/i)
  end

  !has_spanish
end


def main
  if github_event_name != 'pull_request'
    puts '[skip] not a pull_request event'
    return 0
  end

  base = base_ref
  files = changed_files(base)

  headers_changed = files.select { |f| HEADER_EXTS.include?(File.extname(f)) }
  sources_changed = files.select { |f| %w[.C .cc .cpp .cxx].include?(File.extname(f)) }
  tests_changed = files.select { |f| f.start_with?('Tests/') && TEST_EXTS.include?(File.extname(f)) }

  failures = []

  (headers_changed + sources_changed).each do |file|
    p = Pathname.new(file)
    next unless p.exist?

    failures << "missing/invalid MIT license header: #{file}" if HEADER_EXTS.include?(File.extname(file)) && !mit_license_header?(p)
    failures << "documentation seems to be in Spanish (must be English): #{file}" unless english_documentation?(p)
  end

  if !headers_changed.empty? && tests_changed.empty?
    failures << 'headers changed but no Tests/ files changed (add/modify tests)'
  end

  unless failures.empty?
    failures.each { |f| puts "[fail] #{f}" }
    return 1
  end

  puts "[ok] headers changed: #{headers_changed.size}"
  puts "[ok] tests changed: #{tests_changed.size}"
  0
end


exit(main)
