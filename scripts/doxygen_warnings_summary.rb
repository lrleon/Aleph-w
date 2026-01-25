#!/usr/bin/env ruby
# frozen_string_literal: true

# Genera un resumen de warnings de Doxygen agrupados por archivo y por tipo.
#
# Uso:
#   doxygen Doxyfile 2> /tmp/doxygen_err.txt
#   ruby scripts/doxygen_warnings_summary.rb /tmp/doxygen_err.txt docs/doxygen_warnings.md

require 'pathname'

WARN_RE = %r{^(?<file>/.+?):(?<line>\d+):\s+warning:\s+(?<msg>.+)$}

def normalize_msg(msg)
  msg = msg.strip.gsub(/\s+/, ' ')
  msg.gsub!(/at line \d+ in file .+$/, 'at line <n> in file <file>')
  msg.gsub!(/line \d+/, 'line <n>')
  msg.gsub!(/\b\d+\b/, '<n>')
  msg
end

def main
  if ARGV.size != 2
    warn 'usage: doxygen_warnings_summary.rb DOXYGEN_ERR_TXT OUT_MD'
    exit 2
  end

  err_path = Pathname.new(ARGV[0]).realpath
  out_path = Pathname.new(ARGV[1]).expand_path

  text = err_path.read(encoding: 'utf-8').lines
  rows = []
  text.each do |line|
    m = WARN_RE.match(line)
    next unless m

    rows << [m[:file], m[:line].to_i, m[:msg]]
  end

  by_file = Hash.new { |h, k| h[k] = [] }
  rows.each do |f, ln, msg|
    by_file[f] << [ln, msg]
  end

  total = rows.size
  files_sorted = by_file.sort_by { |_k, v| -v.size }

  out = []
  out << '## Resumen de warnings de Doxygen'
  out << ''
  out << "- **Total warnings**: #{total}"
  out << "- **Archivos con warnings**: #{by_file.size}"
  out << ''

  out << '### Top archivos (por cantidad)'
  out << ''
  out << '| Archivo | Warnings | Top tipos |'
  out << '|---|---:|---|'
  files_sorted.first(30).each do |f, items|
    counter = Hash.new(0)
    items.each { |_ln, msg| counter[normalize_msg(msg)] += 1 }
    topk = counter.sort_by { |_k, v| -v }.first(3).map { |k, v| "`#{k}` (#{v})" }.join(', ')
    out << "| `#{Pathname.new(f).basename}` | #{items.size} | #{topk} |"
  end
  out << ''

  out << '### Detalle por archivo (top 30)'
  out << ''
  files_sorted.first(30).each do |f, items|
    out << "#### `#{f}`"
    out << ''
    out << "- **Warnings**: #{items.size}"
    counter = Hash.new(0)
    items.each { |_ln, msg| counter[normalize_msg(msg)] += 1 }
    out << '- **Tipos más comunes**:'
    counter.sort_by { |_k, v| -v }.first(10).each do |k, v|
      out << "  - #{v}x #{k}"
    end
    out << ''
    out << '- **Ejemplos (primeros 8)**:'
    items.sort_by { |ln, _msg| ln }.first(8).each do |ln, msg|
      out << "  - L#{ln}: #{msg}"
    end
    out << ''
  end

  out_path.dirname.mkpath
  out_path.write(out.join("\n"), encoding: 'utf-8')
  puts "[ok] #{out_path}"
  0
end

exit main if __FILE__ == $PROGRAM_NAME
