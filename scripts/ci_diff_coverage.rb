#!/usr/bin/env ruby
# frozen_string_literal: true

require 'optparse'
require 'open3'

CPP_EXTENSIONS = %w[
  .c .cc .cpp .cxx .h .hh .hpp .hxx .C .H
].freeze

EXCLUDED_PREFIXES = [
  'Tests/',
  'Examples/',
  'docs/',
  '.github/',
  'build/',
  'build-',
  'cmake-build-'
].freeze

def run!(cmd)
  out, err, status = Open3.capture3(*cmd)
  return out if status.success?

  warn "Command failed: #{cmd.join(' ')}"
  warn err
  exit 2
end

def changed_files(diff_range)
  out = run!(%W[git diff --name-only --diff-filter=ACMR #{diff_range}])
  out.lines.map(&:strip).reject(&:empty?).map { |p| p.tr('\\', '/') }
end

def parse_coverage(xml_path)
  table = Hash.new { |h, k| h[k] = { covered: 0, valid: 0 } }
  current_file = nil

  File.foreach(xml_path) do |line|
    if (m = line.match(/<class\b[^>]*filename="([^"]+)"/))
      current_file = m[1].tr('\\', '/')
      next
    end

    if line.include?('</class>')
      current_file = nil
      next
    end

    next if current_file.nil?
    next unless (m = line.match(/<line\b[^>]*hits="(\d+)"/))

    cov = table[current_file]
    cov[:valid] += 1
    cov[:covered] += 1 if m[1].to_i.positive?
  end

  table
end

def cpp_source?(path)
  CPP_EXTENSIONS.include?(File.extname(path))
end

def excluded_path?(path)
  EXCLUDED_PREFIXES.any? { |prefix| path.start_with?(prefix) }
end

def pct(covered, valid)
  return 0.0 if valid.zero?

  (100.0 * covered) / valid
end

def append_summary(summary_file, text)
  return if summary_file.nil? || summary_file.empty?

  File.open(summary_file, 'a', encoding: 'UTF-8') { |f| f.write(text) }
end

options = {
  min_line: 70.0,
  summary_file: nil
}

parser = OptionParser.new do |opts|
  opts.banner = 'Usage: ci_diff_coverage.rb --coverage-xml PATH --diff-range RANGE [options]'

  opts.on('--coverage-xml PATH', String, 'Path to coverage.xml') do |v|
    options[:coverage_xml] = v
  end

  opts.on('--diff-range RANGE', String, 'Git diff range, e.g. origin/main...HEAD') do |v|
    options[:diff_range] = v
  end

  opts.on('--min-line FLOAT', Float, 'Minimum line coverage percentage for changed files') do |v|
    options[:min_line] = v
  end

  opts.on('--summary-file PATH', String, 'Optional path to append markdown summary') do |v|
    options[:summary_file] = v
  end
end

begin
  parser.parse!
rescue OptionParser::ParseError => e
  warn e.message
  warn parser
  exit 2
end

if options[:coverage_xml].nil? || options[:diff_range].nil?
  warn 'Missing required options: --coverage-xml and --diff-range'
  warn parser
  exit 2
end

coverage = parse_coverage(options[:coverage_xml])
changed = changed_files(options[:diff_range])

candidates = changed.select { |f| cpp_source?(f) && !excluded_path?(f) }
with_data = []
missing_data = []

candidates.each do |path|
  cov = coverage[path]
  if cov.nil? || cov[:valid].zero?
    missing_data << path
    next
  end

  with_data << [path, cov]
end

total_covered = with_data.sum { |_, c| c[:covered] }
total_valid = with_data.sum { |_, c| c[:valid] }
total_pct = pct(total_covered, total_valid)

lines = []
lines << '## Diff Coverage'
lines << ''
lines << "- Diff range: `#{options[:diff_range]}`"
lines << format('- Threshold: `%.1f%%`', options[:min_line])
lines << "- Changed C/C++ files considered: `#{candidates.length}`"
lines << "- Files with coverage data: `#{with_data.length}`"
lines << "- Files without coverage data (ignored): `#{missing_data.length}`"
lines << ''

if with_data.any?
  lines << '| File | Covered / Valid | Line Coverage |'
  lines << '| --- | ---: | ---: |'
  with_data.sort_by(&:first).each do |path, cov|
    lines << format('| `%s` | `%d/%d` | `%.1f%%` |', path, cov[:covered], cov[:valid], pct(cov[:covered], cov[:valid]))
  end
  lines << ''
  lines << format('**Aggregate changed-files coverage:** `%.1f%%` (`%d/%d`)', total_pct, total_covered, total_valid)
else
  lines << 'No changed C/C++ files with measurable coverage entries. Gate skipped.'
end

if missing_data.any?
  lines << ''
  lines << 'Changed files without coverage entries (not gating):'
  missing_data.sort.each do |path|
    lines << "- `#{path}`"
  end
end

summary = lines.join("\n") + "\n"
puts(summary)
append_summary(options[:summary_file], summary)

if with_data.any? && total_pct + 1e-9 < options[:min_line]
  warn format('Diff coverage check failed: %.1f%% < %.1f%%', total_pct, options[:min_line])
  exit 1
end

exit 0
