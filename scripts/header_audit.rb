#!/usr/bin/env ruby
# frozen_string_literal: true

# Auditoría de headers Aleph-w:
# - Documentación: @file/@brief y cobertura aproximada de doc para class/struct.
# - Cobertura de tests: cuáles Tests/*.cc incluyen el header.
# - Cobertura de ejemplos: cuáles Examples/*.{C,cc} incluyen el header.
#
# Uso:
#   ruby scripts/header_audit.rb --repo . --out docs/header_audit.md

require 'pathname'
require 'optparse'

INCLUDE_RE = /^\s*#\s*include\s*[<"]([^>"]+)[>"]/
CLASS_RE = /^\s*(?:template\s*<[^>]*>\s*)?(?:class|struct)\s+([A-Za-z_]\w*)\b/
FUNC_RE = /^\s*(?:template\s*<[^>]*>\s*)?(?:inline\s+|constexpr\s+|static\s+|extern\s+|friend\s+)?(?:[\w:<>]+\s+)+([A-Za-z_]\w*)\s*\([^;]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:->\s*[\w:<>]+\s*)?(?:\{|;)\s*$/
DOXY_LINE_RE = /^\s*\/\/\//
DOXY_BLOCK_START_RE = /^\s*\/\*\*/

HeaderInfo = Struct.new(:header, :has_file_tag, :has_brief_tag, :class_total, :class_doc, :func_total, :func_doc, :tests, :examples) do
  def has_file_doc?
    has_file_tag && has_brief_tag
  end

  def class_doc_ratio
    return 'n/a' if class_total.zero?

    pct = (100 * class_doc) / class_total
    "#{class_doc}/#{class_total} (#{pct}%)"
  end

  def func_doc_ratio
    return 'n/a' if func_total.zero?

    pct = (100 * func_doc) / func_total
    "#{func_doc}/#{func_total} (#{pct}%)"
  end
end

def iter_files(root, patterns)
  patterns.flat_map { |pat| root.glob(pat) }.select(&:file?).sort
end

def build_include_map(files)
  mp = Hash.new { |h, k| h[k] = [] }
  files.each do |f|
    txt = f.read(encoding: 'utf-8').lines
    txt.each do |line|
      m = INCLUDE_RE.match(line)
      next unless m

      inc = m[1].strip
      base = File.basename(inc)
      mp[base] << f.to_s
    end
  rescue StandardError
    next
  end
  mp.each { |k, v| mp[k] = v.uniq.sort }
  mp
end

def analyze_header(path, tests_map, examples_map)
  header = path.basename.to_s
  lines = path.read(encoding: 'utf-8').lines
  head = lines.first(200).join
  has_file_tag = head.include?('@file')
  has_brief_tag = head.include?('@brief')

  class_total = 0
  class_doc = 0
  func_total = 0
  func_doc = 0

  lines.each_with_index do |line, i|
    m = CLASS_RE.match(line)
    if m
      stripped = line.lstrip
      next if stripped.start_with?('*') || stripped.start_with?('//')

      class_total += 1
      doc = false
      ([0, i - 3].max...i).each do |j|
        if DOXY_LINE_RE.match(lines[j]) || DOXY_BLOCK_START_RE.match(lines[j])
          doc = true
          break
        end
      end
      class_doc += 1 if doc
    end
  end

  brace_depth = 0
  lines.each_with_index do |line, i|
    brace_depth += line.count('{')
    brace_depth -= line.count('}')
    next unless brace_depth.zero?

    m = FUNC_RE.match(line)
    next unless m

    name = m[1]
    next if %w[if for while switch catch].include?(name)
    next if line.include?('operator')

    stripped = line.lstrip
    next if stripped.start_with?('#', '*', '//')

    func_total += 1
    doc = false
    ([0, i - 6].max...i).each do |j|
      if DOXY_LINE_RE.match(lines[j]) || DOXY_BLOCK_START_RE.match(lines[j])
        doc = true
        break
      end
    end
    func_doc += 1 if doc
  end

  tests = tests_map[header] || []
  examples = examples_map[header] || []

  HeaderInfo.new(header, has_file_tag, has_brief_tag, class_total, class_doc, func_total, func_doc, tests, examples)
rescue StandardError
  HeaderInfo.new(header, false, false, 0, 0, 0, 0, [], [])
end

def md_escape(s)
  s.gsub('|', '\\|')
end

def format_list(paths, repo_root, limit: 6)
  return '—' if paths.empty?

  rel = paths.map do |p|
    Pathname.new(p).realpath.relative_path_from(repo_root.realpath).to_s
  rescue StandardError
    p
  end

  if rel.size <= limit
    rel.map { |x| "`#{md_escape(x)}`" }.join('<br/>')
  else
    head = rel.first(limit)
    head.map { |x| "`#{md_escape(x)}`" }.join('<br/>') + "<br/>…(+#{rel.size - limit})"
  end
end

def render_report(headers, repo_root)
  total = headers.size
  with_file_doc = headers.count(&:has_file_doc?)
  no_tests = headers.reject { |h| h.tests.any? }
  no_examples = headers.reject { |h| h.examples.any? }

  lines = []
  lines << '## Auditoría de Headers (documentación, tests y ejemplos)'
  lines << ''
  lines << 'Este reporte se genera automáticamente (ver `scripts/header_audit.rb`).'
  lines << ''
  lines << "- **Headers analizados**: #{total}"
  lines << "- **Con `@file` + `@brief`**: #{with_file_doc}"
  lines << "- **Sin tests directos (`#include`) en `Tests/`**: #{no_tests.size}"
  lines << "- **Sin ejemplos directos (`#include`) en `Examples/`**: #{no_examples.size}"
  lines << ''

  lines << '### Resumen de faltantes (por heurística)'
  lines << ''
  if no_tests.any?
    lines << '- **Sin tests directos**:'
    no_tests.first(50).each { |h| lines << "  - `#{h.header}`" }
    lines << "  - …(+#{no_tests.size - 50})" if no_tests.size > 50
  else
    lines << '- **Sin tests directos**: ninguno'
  end
  lines << ''
  if no_examples.any?
    lines << '- **Sin ejemplos directos**:'
    no_examples.first(50).each { |h| lines << "  - `#{h.header}`" }
    lines << "  - …(+#{no_examples.size - 50})" if no_examples.size > 50
  else
    lines << '- **Sin ejemplos directos**: ninguno'
  end
  lines << ''

  lines << '### Matriz header por header'
  lines << ''
  lines << '| Header | `@file/@brief` | Clases doc (aprox) | Funciones libres doc (aprox) | Tests que lo incluyen | Ejemplos que lo incluyen |'
  lines << '|---|---:|---:|---:|---|---|'
  headers.each do |h|
    file_doc = h.has_file_doc? ? '✅' : '❌'
    tests_cell = format_list(h.tests, repo_root)
    examples_cell = format_list(h.examples, repo_root)
    lines << "| `#{h.header}` | #{file_doc} | #{h.class_doc_ratio} | #{h.func_doc_ratio} | #{tests_cell} | #{examples_cell} |"
  end
  lines << ''
  lines << '### Notas importantes'
  lines << ''
  lines << '- **Los checks de documentación son heurísticos**: cuentan `class/struct` por regex y buscan `///` o `/**` cerca. Las funciones se detectan con una heurística muy conservadora (solo firmas en una línea a nivel 0 de llaves).'
  lines << '- **La cobertura de tests/ejemplos es por `#include` directo**. Un header puede estar cubierto indirectamente por otro include, y aquí no aparecer.'
  lines.join("\n")
end

def main
  options = { repo: '.', out: 'docs/header_audit.md' }
  OptionParser.new do |opts|
    opts.banner = 'Usage: header_audit.rb [options]'
    opts.on('--repo PATH', 'Ruta al repo (root)') { |v| options[:repo] = v }
    opts.on('--out PATH', 'Ruta de salida del reporte .md') { |v| options[:out] = v }
  end.parse!

  repo_root = Pathname.new(options[:repo]).realpath
  out_path = (repo_root / options[:out]).expand_path

  headers = iter_files(repo_root, ['*.H'])
  tests = iter_files(repo_root, ['Tests/*.cc'])
  examples = iter_files(repo_root, ['Examples/*.C', 'Examples/*.cc'])

  tests_map = build_include_map(tests)
  examples_map = build_include_map(examples)

  infos = headers.map { |h| analyze_header(h, tests_map, examples_map) }
  infos.sort_by! { |x| [x.has_file_doc? ? 1 : 0, x.tests.any? ? 1 : 0, x.examples.any? ? 1 : 0, x.header] }

  report = render_report(infos, repo_root)
  out_path.dirname.mkpath
  out_path.write(report, encoding: 'utf-8')

  puts "[ok] Reporte generado en: #{out_path}"
  0
end

exit main if __FILE__ == $PROGRAM_NAME
