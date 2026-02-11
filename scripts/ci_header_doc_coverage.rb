#!/usr/bin/env ruby

require 'open3'
require 'optparse'
require 'set'

HEADER_EXTS = %w[.h .H .hpp .hxx .hh].freeze
EXCLUDED_TOP_LEVEL = %w[Tests Examples docs .github scripts Testing].freeze
CONTROL_KEYWORDS = %w[if for while switch catch return sizeof static_assert].freeze

Declaration = Struct.new(
  :file,
  :line,
  :start_line,
  :end_line,
  :kind,
  :name,
  :public,
  keyword_init: true
)


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


def opens_doc_comment_block?(line)
  s = line.strip
  s.start_with?('/**', '/*!')
end


def doxygen_comment_before?(lines, line_1_based)
  idx = line_1_based - 1
  return false if idx.negative? || idx >= lines.size

  here = lines[idx].strip
  return true if here.match?(/\/\/\/</) || here.match?(/\/\/!</)
  return true if opens_doc_comment_block?(here)

  i = idx - 1
  while i >= 0 && (idx - i) <= 20
    s = lines[i].strip

    if s.empty?
      i -= 1
      next
    end

    template_continuation = s.match?(/^(?:typename|class)\b.+(?:,|>|>>)\s*$/)
    if s.match?(/^(public|private|protected)\s*:\s*$/) ||
       s.start_with?('template <', 'requires ', '[[', ']]') ||
       template_continuation
      i -= 1
      next
    end

    return true if s.start_with?('///', '//!')
    return true if opens_doc_comment_block?(s)

    if s.include?('*/')
      j = i
      while j >= 0
        t = lines[j].strip
        return true if opens_doc_comment_block?(t)
        break if t.start_with?('/*')
        j -= 1
      end
      return false
    end

    break
  end

  false
end


def sanitize_for_braces(line, in_block_comment = false)
  sanitized = +''
  in_string = nil
  escaped = false
  i = 0

  while i < line.length
    ch = line[i]
    nxt = i + 1 < line.length ? line[i + 1] : nil

    if in_block_comment
      if ch == '*' && nxt == '/'
        in_block_comment = false
        i += 2
      else
        i += 1
      end
      next
    end

    unless in_string.nil?
      if escaped
        escaped = false
      elsif ch == '\\'
        escaped = true
      elsif ch == in_string
        in_string = nil
      end
      i += 1
      next
    end

    if ch == '/' && nxt == '/'
      break
    end

    if ch == '/' && nxt == '*'
      in_block_comment = true
      i += 2
      next
    end

    if ch == '"' || ch == "'"
      in_string = ch
      i += 1
      next
    end

    sanitized << ch
    i += 1
  end

  [sanitized, in_block_comment]
end


def declaration_statement_from_line(line)
  return nil unless line.include?('(')
  return nil unless line.rstrip.end_with?(';', '{')

  s = line.strip
  return nil if s.start_with?('#', 'using ', 'typedef ')

  lowered = s.downcase
  return nil if CONTROL_KEYWORDS.any? { |kw| lowered.start_with?("#{kw} ", "#{kw}(") }

  if s.include?('=') && !s.match?(/\)\s*=\s*(default|delete)\s*;/)
    # Usually assignment or invocation in a function body.
    return nil
  end

  s
end


def parse_function_like_decl(line, current_class_name)
  stmt = declaration_statement_from_line(line)
  return nil if stmt.nil?

  m = stmt.match(
    /^(?<prefix>.*?)(?<name>~?[A-Za-z_]\w*|operator[^\s(]+)\s*\([^;{}]*\)\s*(?:const\b[^;{}]*)?(?:noexcept(?:\([^)]*\))?[^;{}]*)?(?:->\s*[^;{}]+)?\s*(?:=\s*(?:default|delete))?\s*[;{]\s*$/
  )
  return nil if m.nil?

  name = m[:name]
  prefix = m[:prefix].strip
  return nil if CONTROL_KEYWORDS.include?(name)

  is_ctor_or_dtor = !current_class_name.nil? && (name == current_class_name || name == "~#{current_class_name}")
  return nil if prefix.empty? && !is_ctor_or_dtor

  name
end


def parse_changed_public_declarations(file, added_lines)
  lines = File.readlines(file, chomp: true, encoding: 'utf-8')
  declarations = []

  brace_depth = 0
  class_stack = []
  pending_class = nil
  in_block_comment = false

  lines.each_with_index do |raw_line, idx|
    line_no = idx + 1
    stripped = raw_line.strip
    in_decl_scope =
      if class_stack.any?
        brace_depth == class_stack.last[:depth]
      else
        # Free declarations are usually at global scope (0) or one namespace level (1).
        brace_depth <= 1
      end

    if in_decl_scope &&
       !stripped.start_with?('//', '/*', '*') &&
       (m = stripped.match(/^(?:template\s*<.*>\s*)?(class|struct)\s+([A-Za-z_]\w*)\b/))
      kind = m[1]
      name = m[2]
      is_public = class_stack.empty? || class_stack.last[:access] == 'public'
      declarations << Declaration.new(
        file: file,
        line: line_no,
        start_line: line_no,
        end_line: line_no,
        kind: kind,
        name: name,
        public: is_public
      )

      if stripped.include?('{')
        pending_class = { name: name, access: kind == 'struct' ? 'public' : 'private', attach_now: true }
      elsif !stripped.include?(';')
        pending_class = { name: name, access: kind == 'struct' ? 'public' : 'private', attach_now: false }
      end
    end

    if class_stack.any? &&
       (m = stripped.match(/^(public|private|protected)\s*:\s*(?:(?:\/\/|\/\*).*)?$/))
      class_stack.last[:access] = m[1]
    end

    if in_decl_scope &&
       !stripped.start_with?('//', '/*', '*') &&
       (m = stripped.match(/^concept\s+([A-Za-z_]\w*)\s*=/))
      is_public = class_stack.empty? || class_stack.last[:access] == 'public'
      declarations << Declaration.new(
        file: file,
        line: line_no,
        start_line: line_no,
        end_line: line_no,
        kind: 'concept',
        name: m[1],
        public: is_public
      )
    end

    if in_decl_scope
      current_class_name = class_stack.empty? ? nil : class_stack.last[:name]
      fn_name = parse_function_like_decl(raw_line, current_class_name)
      unless fn_name.nil?
        is_public = class_stack.empty? || class_stack.last[:access] == 'public'
        declarations << Declaration.new(
          file: file,
          line: line_no,
          start_line: line_no,
          end_line: line_no,
          kind: class_stack.empty? ? 'function' : 'method',
          name: fn_name,
          public: is_public
        )
      end
    end

    before_depth = brace_depth
    sanitized, in_block_comment = sanitize_for_braces(raw_line, in_block_comment)
    brace_depth += sanitized.count('{')
    brace_depth -= sanitized.count('}')

    if !pending_class.nil?
      should_attach = pending_class[:attach_now] || (before_depth < brace_depth && sanitized.include?('{'))
      if should_attach
        class_stack << {
          name: pending_class[:name],
          access: pending_class[:access],
          depth: brace_depth
        }
        pending_class = nil
      end
    end

    while class_stack.any? && brace_depth < class_stack.last[:depth]
      class_stack.pop
    end
  end

  changed_public = declarations.select do |decl|
    decl.public && (decl.start_line..decl.end_line).any? { |ln| added_lines.include?(ln) }
  end

  documented = changed_public.select { |decl| doxygen_comment_before?(lines, decl.start_line) }
  undocumented = changed_public - documented

  [documented, undocumented]
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

  files.each do |file|
    added = added_lines_for_file(diff_range, file)
    next if added.empty?

    doc, undoc = parse_changed_public_declarations(file, added)
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
    return 1
  end

  puts "[ok] header docstring coverage meets threshold (#{format('%.2f', coverage)}% >= #{min}%)"
  0
end


exit(main)
