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
  content = File.read(path) rescue return true
  # Extract Doxygen blocks (/** ... */)
  docs = content.scan(%r{/\*\*.*?\*/}m).join(' ')
  return true if docs.empty?

  # Check for common Spanish words that shouldn't be in English documentation
  # (avoiding very short words or those that overlap with English/technical terms)
  spanish_words = %w[
    algoritmo biblioteca cabecera función parámetro retorno estructura
    herencia polimorfismo puntero memoria asignación búscqueda ordenamiento
    grafo nodo arista camino ciclo árbol hoja raíz
    implementación descripción ejemplo advertencia nota opcional requerido
    devuelve booleano entero real cadena carácter
  ]
  
  # Case-insensitive word-boundary search
  spanish_words.any? { |w| docs.match?(/\b#{Regexp.escape(w)}\b/i) } ? false : true
end


def main
  if github_event_name != 'pull_request'
    puts '[skip] not a pull_request event'
    return 0
  end

  base = base_ref
  files = changed_files(base)

  headers_changed = files.select { |f| HEADER_EXTS.include?(File.extname(f)) }
  sources_changed = files.select { |f| %w[.C .cc .cpp].include?(File.extname(f)) }
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
