#!/usr/bin/env ruby
# Mechanical coverage matrix generator for tpl_sort_utils.H
# - Strips C/C++ comments and string/char literals
# - Extracts function definitions from tpl_sort_utils.H (heuristic regex)
# - Scans Tests/*.cc for function call references grouped by TEST(...) scope
# - Writes Tests/sort_utils_coverage_matrix.md

require 'pathname'

ROOT = Pathname.new(__FILE__).realpath.dirname.parent
HEADER = ROOT.join('tpl_sort_utils.H')
TESTS_DIR = ROOT.join('Tests')
OUT = TESTS_DIR.join('sort_utils_coverage_matrix.md')

CONTROL = %w[
  if for while switch return sizeof catch
  static_cast reinterpret_cast const_cast dynamic_cast
  new delete
].to_h { |x| [x, true] }


def strip_comments_and_literals(src)
  out = src.dup
  i = 0
  n = src.length
  state = :code

  while i < n
    ch = src.getbyte(i)

    case state
    when :code
      if ch == '/'.ord && i + 1 < n
        nxt = src.getbyte(i + 1)
        if nxt == '/'.ord
          out.setbyte(i, ' '.ord)
          out.setbyte(i + 1, ' '.ord)
          i += 2
          state = :line_comment
          next
        elsif nxt == '*'.ord
          out.setbyte(i, ' '.ord)
          out.setbyte(i + 1, ' '.ord)
          i += 2
          state = :block_comment
          next
        end
      end

      if ch == '"'.ord
        out.setbyte(i, ' '.ord)
        i += 1
        state = :string
        next
      end

      if ch == "'".ord
        out.setbyte(i, ' '.ord)
        i += 1
        state = :char
        next
      end

    when :line_comment
      if ch == "\n".ord
        state = :code
      else
        out.setbyte(i, ' '.ord)
      end
      i += 1
      next

    when :block_comment
      if ch == '*'.ord && i + 1 < n && src.getbyte(i + 1) == '/'.ord
        out.setbyte(i, ' '.ord)
        out.setbyte(i + 1, ' '.ord)
        i += 2
        state = :code
        next
      end
      out.setbyte(i, ' '.ord) unless ch == "\n".ord
      i += 1
      next

    when :string
      if ch == '\\'.ord && i + 1 < n
        out.setbyte(i, ' '.ord)
        out.setbyte(i + 1, ' '.ord)
        i += 2
        next
      end
      if ch == '"'.ord
        out.setbyte(i, ' '.ord)
        i += 1
        state = :code
        next
      end
      out.setbyte(i, ' '.ord) unless ch == "\n".ord
      i += 1
      next

    when :char
      if ch == '\\'.ord && i + 1 < n
        out.setbyte(i, ' '.ord)
        out.setbyte(i + 1, ' '.ord)
        i += 2
        next
      end
      if ch == "'".ord
        out.setbyte(i, ' '.ord)
        i += 1
        state = :code
        next
      end
      out.setbyte(i, ' '.ord) unless ch == "\n".ord
      i += 1
      next
    end

    i += 1
  end

  out
end


def find_matching_paren(s, start_idx)
  depth = 0
  i = start_idx
  in_str = nil
  n = s.length

  while i < n
    ch = s.getbyte(i)

    if in_str
      if ch == '\\'.ord
        i += 2
        next
      end
      if ch == in_str
        in_str = nil
      end
      i += 1
      next
    end

    if ch == '"'.ord || ch == "'".ord
      in_str = ch
      i += 1
      next
    end

    if ch == '('.ord
      depth += 1
    elsif ch == ')'.ord
      depth -= 1
      return i if depth == 0
    end

    i += 1
  end

  -1
end


def is_definition?(s, pos_after_paren)
  window = s[pos_after_paren, 800] || ''
  m = window.match(/[;{]/)
  return false if m.nil?
  m[0] == '{'
end


def extract_function_definitions(header_text)
  cleaned = strip_comments_and_literals(header_text)

  fn_sig = Regexp.new(
    '^\s*(?:template\s*<[^;{>]*>\s*)*' \
    '(?:inline\s+|static\s+inline\s+|static\s+|constexpr\s+|friend\s+|extern\s+)*' \
    '[A-Za-z_][\w:<>,\s\*&\[\]]*\s+' \
    '(?<name>[A-Za-z_]\w*)\s*\(',
    Regexp::MULTILINE
  )

  functions = []
  seen = {}

  cleaned.to_enum(:scan, fn_sig).each do
    m = Regexp.last_match
    name = m[:name]
    next if CONTROL[name]

    open_paren = m.end(0) - 1
    close_paren = find_matching_paren(cleaned, open_paren)
    next if close_paren == -1
    next unless is_definition?(cleaned, close_paren + 1)

    line = cleaned[0...m.begin(0)].count("\n") + 1
    key = [name, line]
    next if seen[key]

    seen[key] = true
    functions << { name: name, line: line }
  end

  functions.sort_by { |f| [f[:name], f[:line]] }
end


def extract_test_calls(test_text, file_name)
  cleaned = strip_comments_and_literals(test_text)
  lines = cleaned.split("\n", -1)

  test_start = /^\s*TEST(_F)?\s*\(\s*([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)\s*\)/
  call_pat = /\b(?<name>[A-Za-z_]\w*)\s*(?:<[^;{()]*?>)?\s*\(/ # allow commas in template args

  current = '<global>'
  depth = 0
  in_test = false

  calls = []

  lines.each do |line|
    m = line.match(test_start)
    if m
      suite = m[2]
      tname = m[3]
      current = "#{file_name}:#{suite}.#{tname}"
      in_test = true
      depth = 0
    end

    if in_test
      depth += line.count('{')
      depth -= line.count('}')
      if depth == 0 && line.include?('}')
        in_test = false
        current = '<global>'
      end
    end

    scope = in_test ? current : '<global>'

    line.to_enum(:scan, call_pat).each do
      cm = Regexp.last_match
      calls << [cm[:name], scope]
    end
  end

  calls
end


def main
  header_text = HEADER.read
  functions = extract_function_definitions(header_text)

  refs = {}
  functions.each { |f| refs[f[:name]] = {} }

  TESTS_DIR.glob('*.cc').sort.each do |path|
    calls = extract_test_calls(path.read, path.basename.to_s)
    calls.each do |name, scope|
      next unless refs.key?(name)
      refs[name][scope] = true
    end
  end

  rows = []
  unref = []

  functions.each do |f|
    scopes = refs[f[:name]].keys.sort
    unref << [f[:name], f[:line]] if scopes.empty?
    rows << [f[:name], f[:line], scopes.length, scopes]
  end

  test_files = TESTS_DIR.glob('*.cc').length

  OUT.open('w') do |out|
    out.puts '# Mechanical Coverage Matrix: tpl_sort_utils.H -> Tests usage'
    out.puts
    out.puts "- Header: `#{HEADER}`"
    out.puts "- Tests scanned: `#{TESTS_DIR}/*.cc` (#{test_files} files)"
    out.puts "- Functions detected (definitions only): #{functions.length}"
    out.puts "- Functions with 0 references in tests: #{unref.length}"
    out.puts
    out.puts '## Table'
    out.puts
    out.puts '| Function | Def line | #Scopes | Referenced in (scope) |'
    out.puts '|---|---:|---:|---|'

    rows.each do |name, line, cnt, scopes|
      tests_str = if scopes.length > 6
        shown = scopes[0, 6]
        "#{shown.join(', ')} ... (+#{scopes.length - 6} more)"
      else
        scopes.join(', ')
      end
      out.puts "| `#{name}` | #{line} | #{cnt} | #{tests_str} |"
    end

    out.puts
    out.puts '## Unreferenced functions (0 test scopes matched)'
    out.puts
    unref.uniq.sort.each do |name, line|
      out.puts "- `#{name}` (line #{line})"
    end
  end

  puts "Wrote: #{OUT}"
  puts "unref_count=#{unref.length}"
  if unref.length > 0
    puts "unreferenced: #{unref.uniq.sort.inspect}"
  end
end

main
