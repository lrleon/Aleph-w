#!/usr/bin/env ruby

# Fails when less than a share (80 % by default) of the public declarations
# whose signature a change touches, in the headers of the library, are
# documented, as Doxygen publishes them (see ci_cpp_declarations.rb and
# ci_doxygen_declarations.rb). It needs doxygen in the PATH.

require 'open3'
require 'optparse'
require 'set'
require_relative 'ci_cpp_declarations'
require_relative 'ci_doxygen_declarations'

HEADER_EXTS = %w[.h .H .hpp .hxx .hh].freeze
EXCLUDED_TOP_LEVEL = %w[Tests Examples docs .github scripts Testing third_party].freeze

Declaration = Struct.new(:file, :line, :kind, :name, keyword_init: true)


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


def parse_options
  opts = { min: 80, base_ref: nil, diff_range: nil, summary_file: nil }

  OptionParser.new do |parser|
    parser.banner = 'Usage: ci_header_doc_coverage.rb [options]'
    parser.on('--min N', Integer, 'Minimum coverage percentage (default: 80)') do |v|
      opts[:min] = v
    end
    parser.on('--base-ref BRANCH', String, 'Base branch (default: GITHUB_BASE_REF)') do |v|
      opts[:base_ref] = v
    end
    parser.on('--diff-range RANGE', String, 'Git diff range, e.g. origin/master...HEAD') do |v|
      opts[:diff_range] = v
    end
    parser.on('--summary-file PATH', String, 'Optional markdown output path') do |v|
      opts[:summary_file] = v
    end
  end.parse!

  opts
end


def in_scope_header?(path)
  return false unless HEADER_EXTS.include?(File.extname(path))

  parts = path.split('/')
  return false if parts.empty?
  return false if EXCLUDED_TOP_LEVEL.include?(parts.first)
  return false if parts.any? { |p| p.start_with?('build') || p.start_with?('cmake-build-') }

  true
end


def resolve_diff_range(opts)
  return opts[:diff_range] unless opts[:diff_range].nil? || opts[:diff_range].empty?

  base = opts[:base_ref] || base_ref
  raise 'missing base branch (use --base-ref or set GITHUB_BASE_REF)' if base.nil? || base.empty?

  run!(['git', 'fetch', '--no-tags', '--prune', 'origin', base])
  "origin/#{base}...HEAD"
end


def changed_in_scope_headers(diff_range)
  out = run!(['git', 'diff', '--name-only', '--diff-filter=ACMR', diff_range])
  out.lines.map(&:strip).reject(&:empty?).select { |f| in_scope_header?(f) }
end


def parse_added_lines(diff_text)
  added = Set.new
  cur_new = nil

  diff_text.each_line do |line|
    if (m = line.match(/^@@ -\d+(?:,\d+)? \+(?<new>\d+)(?:,\d+)? @@/))
      cur_new = m[:new].to_i
      next
    end

    next if cur_new.nil?
    next if line.start_with?('+++', '---', 'diff --git', 'index ')
    next if line.start_with?('\\ No newline')

    if line.start_with?('+')
      added << cur_new
      cur_new += 1
    elsif line.start_with?('-')
      # deletion, no increment on destination line counter
    else
      cur_new += 1
    end
  end

  added
end


def added_lines_for_file(diff_range, file)
  diff = run!(['git', 'diff', '--unified=0', '--diff-filter=ACMR', diff_range, '--', file])
  parse_added_lines(diff)
end


# The public declarations of a header whose signature has an added line, split
# into [documented, undocumented]. The declarations and their signatures come
# from CppDeclarations; whether each one is documented, from Doxygen, which
# also joins the definition of a function with its declaration. A declaration
# that Doxygen does not report at that line (code that the preprocessor drops,
# or a construct it misreads) is judged by the text.
def parse_changed_public_declarations(file, added_lines, doxygen = nil)
  doxygen ||= DoxygenDeclarations.read([file]).fetch(DoxygenDeclarations.key(file), [])
  source = File.read(file, encoding: 'utf-8')
  lines = source.lines(chomp: true)
  touched = CppDeclarations.parse(source).select do |decl|
    decl.public && (decl.start_line..decl.end_line).any? { |n| added_lines.include?(n) }
  end

  judged = touched.filter_map do |decl|
    same = doxygen.select { |e| e.type == decl.type? && e.name.delete(' ') == decl.name.delete(' ') }
    documented =
      if (entry = same.find { |e| e.line == decl.name_line })
        entry.documented
      elsif same.any? { |e| e.bodystart == decl.name_line }
        next # the definition of a function declared elsewhere: not counted
      else
        CppDeclarations.documented_by_text?(lines, decl)
      end
    [Declaration.new(file: file, line: decl.name_line, kind: decl.kind, name: decl.name), documented]
  end

  judged.partition { |_, documented| documented }.map { |group| group.map(&:first) }
end


def render_summary(path:, min:, diff_range:, files:, total:, documented:, coverage:, undocumented:)
  lines = []
  lines << '## Header Docstring Coverage'
  lines << ''
  lines << "- Diff range: `#{diff_range}`"
  lines << "- In-scope changed headers: **#{files.size}**"
  lines << "- Covered declarations: **#{documented}/#{total}**"
  lines << "- Coverage: **#{format('%.2f', coverage)}%** (min: **#{min}%**)"
  lines << ''

  if undocumented.empty?
    lines << 'Result: PASS'
  else
    lines << 'Result: FAIL'
    lines << ''
    lines << 'Undocumented changed declarations:'
    undocumented.first(200).each do |decl|
      lines << "- `#{decl.file}:#{decl.line}` (#{decl.kind} `#{decl.name}`)"
    end
    extra = undocumented.size - 200
    lines << "- ... and #{extra} more" if extra.positive?
  end

  File.write(path, lines.join("\n") + "\n", encoding: 'utf-8')
end


def main
  opts = parse_options
  min = opts[:min].to_i

  if github_event_name != 'pull_request' && opts[:diff_range].nil?
    puts '[skip] not a pull_request event (use --diff-range for local runs)'
    return 0
  end

  diff_range = resolve_diff_range(opts)
  files = changed_in_scope_headers(diff_range)

  if files.empty?
    puts '[ok] no in-scope changed headers'
    return 0
  end

  documented = []
  undocumented = []

  added_by_file = files.to_h { |file| [file, added_lines_for_file(diff_range, file)] }
                       .reject { |_, added| added.empty? }
  begin
    doxygen = DoxygenDeclarations.read(added_by_file.keys)
  rescue StandardError => e
    puts "[fail] unable to run Doxygen: #{e.message}"
    return 1
  end

  added_by_file.each do |file, added|
    doc, undoc = parse_changed_public_declarations(file, added,
                                                   doxygen.fetch(DoxygenDeclarations.key(file), []))
    documented.concat(doc)
    undocumented.concat(undoc)
  rescue StandardError => e
    puts "[fail] unable to analyze #{file}: #{e.message}"
    return 1
  end

  total = documented.size + undocumented.size
  if total.zero?
    puts '[ok] no in-scope changed public declarations'
    return 0
  end

  coverage = 100.0 * documented.size / total
  puts "[info] analyzed headers: #{files.size}"
  puts "[info] documented declarations: #{documented.size}/#{total} (#{format('%.2f', coverage)}%)"

  unless opts[:summary_file].nil? || opts[:summary_file].empty?
    render_summary(
      path: opts[:summary_file],
      min: min,
      diff_range: diff_range,
      files: files,
      total: total,
      documented: documented.size,
      coverage: coverage,
      undocumented: undocumented
    )
    puts "[info] summary written to #{opts[:summary_file]}"
  end

  if coverage + 1e-9 < min
    puts "[fail] header docstring coverage below threshold: #{format('%.2f', coverage)}% < #{min}%"
    undocumented.first(200).each do |decl|
      puts "[fail] undocumented changed declaration: #{decl.file}:#{decl.line} (#{decl.kind} #{decl.name})"
    end
    extra = undocumented.size - 200
    puts "[fail] ... and #{extra} more" if extra.positive?
    puts '[info] documented means that Doxygen publishes documentation for it:'
    puts '[info] "\overload name(args)" documents the named function, not the one below,'
    puts '[info] and a "///<" right after the parameters, before the body, documents the last one.'
    return 1
  end

  puts "[ok] header docstring coverage meets threshold (#{format('%.2f', coverage)}% >= #{min}%)"
  0
end


exit(main) if $PROGRAM_NAME == __FILE__
