#!/usr/bin/env ruby
# frozen_string_literal: true

# Aleph-w
#
# Data structures & Algorithms
# version 2.0.0b
# https://github.com/lrleon/Aleph-w
#
# This file is part of Aleph-w library
#
# Copyright (c) 2002-2026 Leandro Rabindranath Leon
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# The check of the moved-from pattern (see Tests/moved_from.H).
#
# A class that keeps an owning container (an Array, a DynList, a
# std::vector...) and, beside it, a scalar member (a size, an index, a
# flag, a pointer, an enumerator), and leaves its moves implicit or
# defaulted, is left inconsistent by a move: the move takes the container,
# which is left empty in the source, but copies the scalar, so the
# moved-from object describes storage that it no longer has. A class that
# writes its moves must register a Moved_From_Case in Tests/, so that the
# typed suite checks them.
#
# A class is exempt when a comment in its body has MOVED_FROM_MARKER and
# the reason, for instance when its scalars are parameters that do not
# describe the container.
#
# The analysis reads the text of a header, without a compiler: it finds the
# definitions of classes, their data members and their special members. It
# keeps the first branch of each conditional of the preprocessor and
# ignores the definitions inside macros.
#
# Use: ruby scripts/ci_moved_from_check.rb [--all] [header ...]
#   lists the classes of the headers (default: those of the root) that
#   have the pattern, or that write their moves without a moved-from test.

require 'open3'
require 'strscan'

MOVED_FROM_MARKER = 'moved-from:'

# The owning containers whose move leaves the source empty
MOVED_FROM_CONTAINER = /
  (?:\A|[^\w:]|Aleph::)
  (?:Array|DynArray|MemArray|DynList|DynDlist|DynSlist|DynMatrix|DynListQueue|
     DynListStack|ArrayQueue|ArrayStack|FixedStack|FixedQueue|DynSetTree|
     DynMapTree|DynSetHash|DynMapHash|DynSetLinHash|DynMapLinHash)\s*<
  | \bBitArray\b
  | \bstd::(?:vector|deque|list|forward_list|map|multimap|set|multiset|unordered_map|
            unordered_set|unique_ptr|shared_ptr|basic_string|unique_lock|shared_lock)\s*<
  | \bstd::(?:string|wstring)\b
/x

# The types whose move is a copy: the arithmetic types and the pointers
MOVED_FROM_SCALAR = /
  \*\s*(?:const\s*)?\z
  | \A(?:(?:const|volatile)\s+)*(?:(?:unsigned|signed|long|short)\s+)*
    (?:(?:std::)?(?:size_t|ptrdiff_t|u?int(?:8|16|32|64)_t|u?int_fast\d+_t|uintptr_t)|
       ssize_t|int|long|short|char|bool|double|float|unsigned|signed|size_type|difference_type)
    \z
/x

# Comments, literals, and the preprocessor lines and the branches after the
# first one of each conditional, blanked; offsets and newlines are kept.
# It works on the bytes: indexing a UTF-8 string is linear
def moved_from_blank(src)
  s = StringScanner.new(src.b)
  out = String.new(capacity: src.bytesize, encoding: Encoding::BINARY)
  until s.eos?
    if s.scan(%r{//[^\n]*}n) or s.scan(%r{/\*.*?(?:\*/|\z)}mn) or
       s.scan(/(?:u8|[uUL])?R"([^()\\\s]{0,16})\(.*?\)\1"/mn) or
       s.scan(/(?:u8|[uUL])?"(?:[^"\\\n]|\\.)*"?/n) or s.scan(/(?:u8|[uUL])?'(?:[^'\\\n]|\\.)*'?/n)
      out << s.matched.gsub(/[^\n]/n, ' ')
    elsif s.scan(/[A-Za-z_]\w*|\d[\w'.]*/n)
      out << s.matched  # an identifier, or a number, whose separators are not quotes
    else
      out << s.scan(%r{[^/"'A-Za-z_\d]+|.}mn)
    end
  end

  # The preprocessor, line by line on what is left
  lines = out.split("\n", -1)
  skipping = []  # per open conditional: whether its current branch is skipped
  continued = false
  lines.map! do |line|
    directive = continued || line.match?(/\A\s*#/)
    was_continued = continued
    continued = directive && line.end_with?('\\')
    if directive and not was_continued
      d = line[/\A\s*#\s*(\w+)/, 1].to_s
      case d
      when 'if', 'ifdef', 'ifndef' then skipping.push(skipping.last || false)
      when 'elif', 'else', 'elifdef', 'elifndef' then skipping[-1] = true unless skipping.empty?
      when 'endif' then skipping.pop
      end
    end
    directive || skipping.last ? ' ' * line.size : line
  end
  lines.join("\n")
end

# The index of the brace that closes the one at i, in blanked code
def moved_from_close(code, i)
  depth = 0
  (i...code.size).each do |k|
    case code[k]
    when '{' then depth += 1
    when '}'
      depth -= 1
      return k if depth.zero?
    end
  end
  nil
end

# Whether a declaration, without the braces that follow it, is a function
def moved_from_function?(decl)
  t = decl.gsub(/operator\s*(?:\(\s*\)|\[\s*\]|[^\w\s(]+)\s*\(/, 'operator_x(')
  head = t.split(/(?<![=!<>])=(?!=)/, 2).first.to_s.dup
  1 while head.gsub!(/<[^<>]*>/, ' ')
  head.include?('(')
end

# The declarations of the body of a class: [text, offset, function?]
def moved_from_declarations(code, from, to)
  decls = []
  buf = +''
  start = from
  i = from
  while i < to
    c = code[i]
    if c == ';'
      decls << [buf, start, moved_from_function?(buf)]
      buf = +''
      start = i + 1
      i += 1
    elsif c == '{'
      close = moved_from_close(code, i) || to
      if moved_from_function?(buf)
        # the braces of its initializers and its body: a function ends at
        # the first group that neither a comma nor another group follows
        i = close + 1
        loop do
          k = i
          k += 1 while k < to and code[k].match?(/\s/)
          break unless code[k] == ',' or code[k] == '{'

          nxt = code.index('{', k) || to
          close = moved_from_close(code, nxt) || to
          i = close + 1
        end
        decls << [buf, start, true]
        buf = +''
        start = i
      else
        # a nested class or a brace initializer: what it holds is not of
        # the declaration, whose access labels and parentheses it would
        # change; the members take their text before the first brace
        buf << '{}'
        i = close + 1
      end
    else
      buf << c
      i += 1
    end
  end
  decls
end

# The data member that a declaration declares: [type, names], or nil
def moved_from_data_member(decl)
  s = decl.gsub(/\b(?:public|private|protected)\s*:(?!:)/, ' ').gsub(/\[\[.*?\]\]/m, ' ')
  s = s.gsub(/\A\s*(?:mutable|volatile)\s+/, '').strip
  return nil if s.empty?
  return nil if s.match?(/\A(?:using|typedef|friend|static|static_assert|template|enum|class|struct|
                               union|namespace|concept|requires|inline|constexpr|extern|consteval)\b/x)

  # the declarators, at the commas outside brackets
  parts = []
  depth = 0
  cur = +''
  s.each_char do |ch|
    depth += 1 if '<({['.include?(ch)
    depth -= 1 if '>)}]'.include?(ch)
    if ch == ',' and depth.zero?
      parts << cur
      cur = +''
    else
      cur << ch
    end
  end
  parts << cur
  first = parts.first.split(/(?<![=!<>])=(?!=)|\{|\[|(?<!:):(?!:)/, 2).first.to_s.strip
  name = first[/(\w+)\s*\z/, 1]
  return nil unless name

  type = first[0...first.rindex(name)].strip
  return nil if type.empty?

  names = [name] + parts.drop(1).filter_map { |p| p.split(/=|\{|\[/, 2).first.to_s[/(\w+)\s*\z/, 1] }
  [type, names]
end

# The names that the headers give to the scalar types: the enumerations
# and the aliases of an arithmetic type
def moved_from_scalar_names(text)
  code = moved_from_blank(text)
  enums = code.scan(/\benum\s+(?:class\s+|struct\s+)?(\w+)/).flatten
  aliases = code.scan(/\busing\s+(\w+)\s*=\s*([^;=]+);/).select { |_, t| t.strip.match?(MOVED_FROM_SCALAR) }
  typedefs = code.scan(/\btypedef\s+([^;]+?)\s+(\w+)\s*;/).select { |t, _| t.strip.match?(MOVED_FROM_SCALAR) }
  (enums + aliases.map(&:first) + typedefs.map(&:last)).uniq
end

# The classes of a header with what the check needs of each one; scalar
# names are the names of scalar types declared elsewhere
def moved_from_classes(src, scalar_names = [])
  src = src.b
  code = moved_from_blank(src)
  enums = moved_from_scalar_names(src) | scalar_names
  head = /\b(class|struct)\s+(?:alignas\s*\([^()]*\)\s*)?(?:\[\[[^\]]*\]\]\s*)?([A-Za-z_]\w*)\s*
          (?:final\s*)?(?::[^{;]*)?\{/x
  classes = []
  scopes = []  # [open, close, name] of each namespace and class
  pos = 0
  while (m = /\bnamespace\s+([A-Za-z_][\w:]*)\s*\{/.match(code, pos))
    pos = m.end(0)
    close = moved_from_close(code, m.end(0) - 1) and scopes << [m.end(0) - 1, close, m[1].split('::').last]
  end
  pos = 0
  while (m = head.match(code, pos))
    pos = m.end(0)
    next if code[0...m.begin(0)].match?(/\benum\s+\z/)

    open = m.end(0) - 1
    close = moved_from_close(code, open) or next
    name = m[2]
    scopes << [open, close, name]
    decls = moved_from_declarations(code, open + 1, close)
    # Only the members that the class hides keep an invariant: those of a
    # plain aggregate are values that a move leaves valid
    access = m[1] == 'class' ? 'private' : 'public'
    members = decls.filter_map do |text, _, fn|
      label = text.scan(/\b(public|private|protected)\s*:(?!:)/).flatten.last
      access = label if label
      next if fn or access == 'public'

      moved_from_data_member(text)
    end
    next if decls.none? { |_, _, fn| fn }

    containers = members.select { |t, _| t.match?(MOVED_FROM_CONTAINER) }.flat_map(&:last)
    scalars = members.select do |t, _|
      bare = t.sub(/\A(?:const|volatile)\s+/, '')
      t.match?(MOVED_FROM_SCALAR) or enums.include?(bare) or enums.include?(bare.split('::').last)
    end.flat_map(&:last)
    special = {}
    decls.select { |_, _, fn| fn }.each do |text, _, _|
      t = text.gsub(/\s+/, ' ')
      n = Regexp.escape(name)
      kind = if t.match?(/~\s*#{n}\s*\(/) then :dtor
             elsif t.match?(/(?<![\w:])#{n}\s*\(\s*#{n}\b[^&()]*&&/) then :move_ctor
             elsif t.match?(/(?<![\w:])#{n}\s*\(\s*(?:const\s+#{n}\b[^&()]*|#{n}\b[^&()]*\bconst)\s*&(?!&)/)
               :copy_ctor
             elsif t.match?(/operator\s*=\s*\(\s*#{n}\b[^&()]*&&/) then :move_assign
             elsif t.match?(/operator\s*=\s*\(\s*(?:const\s+)?#{n}\b[^()]*\)/) then :copy_assign
             end
      next unless kind

      special[kind] = if t.match?(/=\s*default\s*\z/) then :default
                      elsif t.match?(/=\s*delete\s*\z/) then :delete
                      else :user
                      end
    end
    implicit = special.empty? ||
               (special.keys - %i[move_ctor move_assign]).empty? &&
               special.values.all? { |v| v == :default }
    classes << { name: name, line: src[0...m.begin(0)].count("\n") + 1 + m[0][/\A\s*/].count("\n"),
                 open: open, close: close, containers: containers, scalars: scalars,
                 implicit: implicit || special[:move_ctor] == :default || special[:move_assign] == :default,
                 written: special[:move_ctor] == :user || special[:move_assign] == :user,
                 exempt: src[open..close].include?(MOVED_FROM_MARKER) }
  end
  # The scope of a class: the innermost named namespace or class around it,
  # which a qualified name of the class puts before its name
  classes.each do |c|
    c[:scope] = scopes.select { |o, e, _| o < c[:open] and e > c[:close] }.max_by(&:first)&.last
  end
  classes
end

# The outer type that a type names, as [scope, name], without the
# arguments: [nil, Storage] for Storage, [F, Storage] for F::Storage,
# [Sync, Ptr] for Sync<long>::Ptr, and [nil, Array], not Storage, for
# Array<Storage>. nil if the text does not start with a name, or if it is
# a pointer, a reference or an array of the type (Storage *), which is
# not the type
def moved_from_type_ref(type)
  t = type.strip.sub(/\A(?:(?:const|volatile|typename|class|struct)\s+)+/, '')
  1 while t.gsub!(/<[^<>]*>/, ' ')
  head = t[/\A(?:::\s*)?\w+(?:\s*::\s*\w+)*/] or return nil
  return nil unless t[head.size..].gsub(/\b(?:const|volatile)\b/, '').strip.empty?

  parts = head.split('::').map(&:strip).reject(&:empty?)
  [parts[-2], parts.last]
end

# The outer types of the bases of a base clause, as [scope, name]
def moved_from_base_types(clause)
  parts = [+'']
  depth = 0
  clause.each_char do |ch|
    depth += 1 if '<(['.include?(ch)
    depth -= 1 if '>)]'.include?(ch)
    ch == ',' && depth.zero? ? parts << +'' : parts.last << ch
  end
  parts.filter_map { |p| moved_from_type_ref(p.gsub(/\b(?:public|protected|private|virtual)\b/, ' ')) }
end

# The aliases and the classes with bases of a text, outside its comments
# and literals: [alias, outer type of its definition] and [class, outer
# types of its bases], as [scope, name], computed once for each text
def moved_from_relations(text)
  @moved_from_relations ||= {}.compare_by_identity
  @moved_from_relations[text] ||= begin
    code = moved_from_blank(text)
    aliases = code.scan(/\busing\s+(\w+)\s*=\s*([^;]*);/).map { |a, t| [a, moved_from_type_ref(t)] }
    derived = code.scan(/\b(?:class|struct)\s+(\w+)\s*(?:final\s*)?:(?!:)([^;{]*)\{/).map do |d, bases|
      [d, moved_from_base_types(bases)]
    end
    [aliases, derived]
  end
end

# Whether the type ref, [scope, name], names the class c: by its name, and,
# if the ref has a scope, by the scope of c, or by an alias of it in the
# texts (F::Plan, with using F = FFT<double>, names the Plan of FFT)
def moved_from_ref_names?(ref, c, texts)
  scope, name = ref
  return false unless name == c[:name]
  return true if scope.nil? or scope == c[:scope]

  texts.any? { |t| moved_from_relations(t).first.any? { |a, r| a == scope and r and r.last == c[:scope] } }
end

# Whether a Moved_From_Case of the tests, given as [scope, name], tests
# the class c: if it names c, or, without a scope, an alias of c or a
# class derived from c, in the headers or in the tests. An alias of, or a
# class derived from, a type that only takes the class as an argument
# (Array<Storage>) does not name it
def moved_from_tested?(c, registered, texts)
  registered.any? do |ref|
    moved_from_ref_names?(ref, c, texts) or ref.first.nil? && texts.any? do |t|
      aliases, derived = moved_from_relations(t)
      aliases.any? { |a, r| a == ref.last and r and moved_from_ref_names?(r, c, texts) } or
        derived.any? { |d, bases| d == ref.last and bases.any? { |b| moved_from_ref_names?(b, c, texts) } }
    end
  end
end

# The outer types of the Moved_From_Case that the tests name, as [scope,
# name], outside their comments and literals: the classes that the typed
# suite tests, and not their arguments (Moved_From_Case<Array<Storage>>
# tests Array)
def moved_from_registered(tests_text)
  @moved_from_registered ||= {}
  return @moved_from_registered[tests_text] if @moved_from_registered.key?(tests_text)

  code = moved_from_blank(tests_text)
  types = []
  pos = 0
  while (m = /\bMoved_From_Case\s*</.match(code, pos))
    depth = 1
    k = m.end(0)
    while k < code.size and depth.positive?
      depth += 1 if code[k] == '<'
      depth -= 1 if code[k] == '>'
      k += 1
    end
    types << moved_from_type_ref(code[m.end(0)...k - 1])
    pos = k
  end
  @moved_from_registered[tests_text] = types.compact.uniq
end

# The problems of a header: [line, message] for each class that has the
# pattern, or that writes its moves without a moved-from test
def moved_from_problems(src, tests_text, headers_text, scalar_names = [])
  registered = moved_from_registered(tests_text)
  moved_from_classes(src, scalar_names).filter_map do |c|
    next if c[:exempt] or c[:containers].empty? or c[:scalars].empty?
    next unless c[:implicit] or c[:written]

    # The typed suite decides whether the moves of a class that it tests
    # are right, whether they are implicit or written: the class is tested
    # if a Moved_From_Case tests it, or an alias of it in the headers or in
    # the tests, not if its name is only elsewhere in the tests, nor only
    # among the arguments of the type that a Moved_From_Case tests, nor if
    # the case qualifies the name with another scope
    next if moved_from_tested?(c, registered, [headers_text, tests_text])

    what = "keeps #{c[:containers].join(', ')} and #{c[:scalars].join(', ')}"
    if c[:implicit]
      [c[:line], "#{c[:name]} #{what}, and its moves are implicit or defaulted: a move empties the " \
                 'container and copies the scalars. Write its moves (std::exchange() of the scalars, ' \
                 'or the same parameters and swap(); the assignment by swap()) and register a ' \
                 "Moved_From_Case in Tests/, or mark the class with `// #{MOVED_FROM_MARKER} <why>`"]
    else
      [c[:line], "#{c[:name]} #{what}, and writes its moves without a moved-from test: register a " \
                 "Moved_From_Case in Tests/ (see Tests/moved_from.H), or mark the class with " \
                 "`// #{MOVED_FROM_MARKER} <why>`"]
    end
  end
end

# Whether path is a header of the library, in the root of the repository
def moved_from_checked_file?(path)
  not path.include?('/') and %w[.h .H .hpp .hxx .hh].include?(File.extname(path))
end

# The failures of a pull request: the classes of the headers that it
# changes that have the pattern and that the base did not have with it.
# files_details are those of ci_policy_checks.rb; the base of each header
# comes from origin/base
def moved_from_failures(base, files_details)
  checked = files_details.select { |d| moved_from_checked_file?(d[:path]) and File.exist?(d[:path]) }
  return [] if checked.empty?

  tests_text, headers_text = moved_from_context
  scalar_names = moved_from_scalar_names(headers_text)
  checked.flat_map do |d|
    previous, = Open3.capture2('git', 'show', "origin/#{base}:#{d[:old_path] || d[:path]}", err: File::NULL)
    known = moved_from_problems(previous, tests_text, headers_text, scalar_names).map { |_, m| m[/\A\w+/] }
    moved_from_problems(File.read(d[:path]), tests_text, headers_text, scalar_names).filter_map do |line, msg|
      "#{d[:path]}:#{line}: #{msg}" unless known.include?(msg[/\A\w+/])
    end
  end
end

# The text of the moved-from tests, and that of the headers of the root
def moved_from_context(root = '.')
  tests = Dir.glob(File.join(root, 'Tests', '**', '*.{cc,H}')).select do |f|
    File.read(f).include?('Moved_From_Case')
  end
  headers = Dir.glob(File.join(root, '*.H'))
  [tests.map { |f| File.read(f) }.join("\n"), headers.map { |f| File.read(f) }.join("\n")]
end

if __FILE__ == $PROGRAM_NAME
  args = ARGV.reject { |a| a == '--all' }
  files = args.empty? ? Dir.glob('*.H').sort : args
  tests_text, headers_text = moved_from_context
  scalar_names = moved_from_scalar_names(headers_text)
  count = 0
  files.each do |f|
    moved_from_problems(File.read(f), tests_text, headers_text, scalar_names).each do |line, msg|
      puts "#{f}:#{line}: #{msg}"
      count += 1
    end
  end
  puts "#{count} classes with the moved-from pattern"
  exit(count.zero? ? 0 : 1)
end
