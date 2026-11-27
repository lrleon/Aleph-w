#!/usr/bin/env ruby
# frozen_string_literal: true

# The declarations of a C++ header, for the header docstring coverage gate.
#
# It reads the header as tokens and statements, not line by line: at namespace
# and class scope it gathers each statement up to its ";" or to the "{" of its
# body, tells the declarations of functions, classes, structs, unions and
# concepts from the rest, and keeps the lines of each signature, from its
# template header, requires clause or attributes to just before its body; the
# bodies are skipped by matching braces. A constructor's initializers are not
# part of the signature. A definition outside its class is not a declaration.
# It has no symbol table, so "Type f(a, b);" is read as a function even if a
# and b are variables.
module CppDeclarations
  Token = Struct.new(:text, :line)

  # start_line and end_line bound the signature; close_line is that of the
  # ";" or the "}" that ends the declaration
  Declaration = Struct.new(:kind, :name, :start_line, :end_line, :close_line, :name_line,
                           :public, keyword_init: true) do
    def type?
      TYPE_KINDS.include?(kind)
    end
  end

  TYPE_KINDS = %w[class struct union concept].freeze
  MULTI_CHARACTER = %w[<=> >>= <<= :: -> == != <= += -= *= /= %= &= |= ^= << && ||].freeze
  SPECIFIERS = %w[inline constexpr consteval constinit static virtual explicit friend extern
                  export mutable thread_local register volatile typename].freeze
  NOT_NAMES = %w[void int char bool float double long short unsigned signed auto const return
                 if for while switch catch class struct union enum].freeze
  # A "(" after one of these does not open the parameter list
  NOT_PARAMETERS = %w[decltype __attribute__ __declspec alignas explicit requires noexcept
                      sizeof alignof throw __typeof__ static_assert].freeze
  CLOSERS = { '(' => ')', '[' => ']', '{' => '}' }.freeze
  # Doxygen commands that group or hide code and document nothing
  STRUCTURAL = /[@\\](?:cond|endcond|name|addtogroup|defgroup|ingroup|weakgroup)\b[^\n]*|[@\\][{}]/

  module_function

  def tokens(source)
    list = []
    line = 1
    i = 0
    length = source.length
    line_start = true
    while i < length
      c = source[i]
      if c == "\n"
        line += 1
        i += 1
        line_start = true
        next
      end
      if c.match?(/\s/)
        i += 1
        next
      end
      if line_start && c == '#' # a preprocessor line, with its continuations
        loop do
          eol = source.index("\n", i) || length
          continued = source[i...eol].rstrip.end_with?('\\') && eol < length
          i = eol
          break unless continued

          line += 1
          i += 1
        end
        next
      end
      line_start = false
      if source[i, 2] == '//'
        i = source.index("\n", i) || length
      elsif source[i, 2] == '/*'
        close = (source.index('*/', i + 2) || length - 2) + 2
        line += source[i...close].count("\n")
        i = close
      elsif (raw = /\G(?:u8|u|U|L)?R"([^\s()\\]{0,16})\(/.match(source, i))
        finish = (source.index(")#{raw[1]}\"", raw.end(0)) || length) + raw[1].length + 2
        list << Token.new('"', line)
        line += source[i...finish].count("\n")
        i = finish
      elsif c == '"' || c == "'"
        j = i + 1
        j += (source[j] == '\\' ? 2 : 1) while j < length && source[j] != c && source[j] != "\n"
        list << Token.new('"', line)
        i = j + 1
      elsif (word = /\G(?:[A-Za-z_]\w*|\d(?:[\w.]|'(?=\w))*)/.match(source, i))
        list << Token.new(word[0], line)
        i = word.end(0)
      else
        operator = MULTI_CHARACTER.find { |o| source[i, o.length] == o } || c
        list << Token.new(operator, line)
        i += operator.length
      end
    end
    list
  end

  def identifier?(text)
    !text.nil? && text.match?(/\A[A-Za-z_]\w*\z/)
  end

  # The index of the token that closes the bracket opened at i
  def closing(list, i)
    depth = 0
    (i...list.size).each do |j|
      depth += 1 if CLOSERS.key?(list[j].text)
      depth -= 1 if CLOSERS.value?(list[j].text)
      return j if depth.zero?
    end
    list.size - 1
  end

  # The index after the ">" that closes the "<" at i
  def past_angles(list, i)
    depth = 0
    j = i
    while j < list.size
      text = list[j].text
      if CLOSERS.key?(text)
        j = closing(list, j)
      elsif text == '<'
        depth += 1
      elsif text == '>'
        depth -= 1
        return j + 1 if depth.zero?
      end
      j += 1
    end
    j
  end

  def past_constraint(list, i)
    loop do
      case list[i]&.text
      when '('
        i = closing(list, i) + 1
      when 'requires' # a requires expression
        i += 1
        i = closing(list, i) + 1 if list[i]&.text == '('
        i = closing(list, i) + 1 if list[i]&.text == '{'
      else
        i += 1 if list[i]&.text == '!'
        i += 2 while identifier?(list[i]&.text) && list[i + 1]&.text == '::'
        i += 1
        i = past_angles(list, i) if list[i]&.text == '<'
      end
      return i unless %w[&& || and or].include?(list[i]&.text)

      i += 1
    end
  end

  # The statement without its template headers, attributes and requires clause
  def strip_prefix(statement)
    i = 0
    loop do
      text = statement[i]&.text
      if text == 'template' && statement[i + 1]&.text == '<'
        i = past_angles(statement, i + 1)
      elsif text == '[' && statement[i + 1]&.text == '['
        i = closing(statement, i) + 1
      elsif text == 'requires'
        i = past_constraint(statement, i + 1)
      else
        return statement[i..] || []
      end
    end
  end

  # The first word after the specifiers and attributes
  def keyword(core)
    i = 0
    loop do
      text = core[i]&.text
      if SPECIFIERS.include?(text) || text == '"'
        i += 1
        i = closing(core, i) + 1 if text == 'explicit' && core[i]&.text == '('
      elsif %w[alignas __declspec __attribute__].include?(text)
        i = closing(core, i + 1) + 1
      elsif text == '[' && core[i + 1]&.text == '['
        i = closing(core, i) + 1
      else
        return text
      end
    end
  end

  # [open, close, name, name_index] of the parameter list, or nil
  def parameter_list(core)
    angles = 0
    i = 0
    while i < core.size
      text = core[i].text
      return operator_parameters(core, i) if text == 'operator'

      if text == '<' && i.positive? && identifier?(core[i - 1].text)
        angles += 1
      elsif text == '>' && angles.positive?
        angles -= 1
      elsif text == '=' && angles.zero?
        return nil # an initializer
      elsif text == '[' || text == '{'
        i = closing(core, i)
      elsif text == '('
        previous = i.positive? ? core[i - 1].text : nil
        return named_parameters(core, i) unless angles.positive? || NOT_PARAMETERS.include?(previous)

        i = closing(core, i)
      end
      i += 1
    end
    nil
  end

  def operator_parameters(core, at)
    j = at + 1
    if %w[( \[].include?(core[j]&.text) && core[j + 1]&.text == CLOSERS[core[j].text]
      symbol = core[j].text + core[j + 1].text
      j += 2
    else
      symbol = +''
      while j < core.size && core[j].text != '('
        symbol << core[j].text
        j += 1
      end
    end
    return nil unless core[j]&.text == '('

    [j, closing(core, j), "operator#{symbol}", at]
  end

  def named_parameters(core, open)
    k = open - 1
    if k >= 0 && core[k].text == '>' # f<int>( : the name before the "<"
      depth = 0
      while k.positive?
        depth += 1 if core[k].text == '>'
        depth -= 1 if core[k].text == '<'
        k -= 1
        break if depth.zero?
      end
    end
    name = k >= 0 ? core[k].text : nil
    return nil unless identifier?(name) && !NOT_NAMES.include?(name)

    if k.positive? && core[k - 1].text == '~'
      name = "~#{name}"
      k -= 1
    end
    [open, closing(core, open), name, k]
  end

  # Whether the parameter list is that of a declaration to count
  def function_declaration?(core, list, scope)
    open, close, name, at = list
    return false if at.positive? && core[at - 1].text == '::' # defined outside its class

    first = core[open + 1]&.text
    return false if open + 1 < close && first.match?(/\A(?:\d|"|-\z)/) # a variable

    rest = core[(close + 1)..] || []
    equals = rest.index { |t| t.text == '=' }
    return false if equals && %w[default delete].include?(rest[equals + 1]&.text)
    return true if name.start_with?('operator', '~') || name == scope[:name]

    before = core[0...at].map(&:text) - SPECIFIERS
    !before.empty? && before.first != '"' # a macro call has no return type
  end

  # The index in core of the ":" that starts a constructor's initializers
  def initializers_at(core, close)
    i = close + 1
    while i < core.size
      return i if core[i].text == ':'

      i = closing(core, i) if CLOSERS.key?(core[i].text)
      i += 1
    end
    nil
  end

  def requires_expression?(statement)
    return true if statement.last&.text == 'requires'
    return false unless statement.last&.text == ')'

    depth = 0
    (statement.size - 1).downto(0) do |k|
      depth += 1 if statement[k].text == ')'
      depth -= 1 if statement[k].text == '('
      return k.positive? && statement[k - 1].text == 'requires' if depth.zero?
    end
    false
  end

  # What the "{" after the statement opens; :skip is part of the statement
  def brace_kind(statement)
    return :skip if statement.empty?

    texts = statement.map(&:text)
    return :namespace if texts.first == 'namespace' || texts.first(2) == %w[inline namespace]
    return :namespace if texts == ['extern', '"']
    return :skip if requires_expression?(statement)

    core = strip_prefix(statement)
    word = keyword(core)
    return :skip if %w[enum concept].include?(word)

    list = parameter_list(core)
    return :class if %w[class struct union].include?(word) && list.nil?
    return :skip if list.nil?
    # the braces of an initializer in a constructor's list
    return :skip if initializers_at(core, list[1]) &&
                    (identifier?(core.last.text) || core.last.text == '>')

    :function
  end

  def class_name(core, at)
    name = nil
    i = at + 1
    while i < core.size
      text = core[i].text
      break if %w[: <].include?(text)

      if text == '[' || text == '('
        i = closing(core, i)
      elsif identifier?(text) && text != 'final' && text != 'alignas'
        name = text
      end
      i += 1
    end
    name
  end

  def visible?(scope)
    scope[:visible] && (scope[:kind] != :class || scope[:access] == 'public')
  end

  def declaration(kind, name, statement, last, close, name_line, scope)
    Declaration.new(kind: kind, name: name, start_line: statement.first.line, end_line: last.line,
                    close_line: close.line, name_line: name_line, public: visible?(scope))
  end

  # The function that the statement declares, or nil; semicolon is nil for a
  # definition, which close ends
  def function_in(statement, scope, semicolon, close)
    core = strip_prefix(statement)
    list = parameter_list(core)
    return nil unless list && function_declaration?(core, list, scope)

    initializers = semicolon ? nil : initializers_at(core, list[1])
    last = semicolon || (initializers ? core[initializers - 1] : statement.last)
    kind = scope[:kind] == :class ? 'method' : 'function'
    declaration(kind, list[2], statement, last, close, core[list[3]].line, scope)
  end

  # The declaration that a statement ending in ";" makes, or nil
  def finish(statement, semicolon, scope)
    core = strip_prefix(statement)
    return nil if core.empty?

    word = keyword(core)
    return nil if %w[using typedef static_assert namespace asm template].include?(word)

    if word == 'concept'
      at = core.index { |t| t.text == 'concept' }
      return declaration('concept', core[at + 1].text, statement, semicolon, semicolon,
                         core[at].line, scope)
    end
    return nil if %w[class struct union enum].include?(word) && parameter_list(core).nil?

    function_in(statement, scope, semicolon, semicolon)
  end

  # The declarations of a header, public or not, in order
  def parse(source)
    list = tokens(source)
    found = []
    scopes = [{ kind: :namespace, visible: true, access: 'public', name: nil }]
    statement = []
    depth = 0
    i = 0
    while i < list.size
      token = list[i]
      case token.text
      when '(', '['
        depth += 1
        statement << token
      when ')', ']'
        depth -= 1 if depth.positive?
        statement << token
      when '{'
        case depth.positive? ? :skip : brace_kind(statement)
        when :namespace
          scopes << { kind: :namespace, visible: visible?(scopes.last), access: 'public', name: nil }
          statement = []
        when :class
          core = strip_prefix(statement)
          at = core.index { |t| %w[class struct union].include?(t.text) }
          name = class_name(core, at)
          if name
            found << declaration(core[at].text, name, statement, statement.last,
                                 list[closing(list, i)], core[at].line, scopes.last)
          end
          scopes << { kind: :class, visible: visible?(scopes.last), name: name,
                      access: core[at].text == 'class' ? 'private' : 'public' }
          statement = []
        when :function
          body_end = closing(list, i)
          found << function_in(statement, scopes.last, nil, list[body_end])
          i = body_end
          statement = []
        else
          j = closing(list, i)
          statement.concat(list[i..j])
          i = j
        end
      when '}'
        scopes.pop if scopes.size > 1
        statement = []
        depth = 0
      when ':'
        if depth.zero? && scopes.last[:kind] == :class && statement.size == 1 &&
           %w[public private protected].include?(statement[0].text)
          scopes.last[:access] = statement[0].text
          statement = []
        else
          statement << token
        end
      when ';'
        if depth.zero?
          found << finish(statement, token, scopes.last) unless statement.empty?
          statement = []
        else
          statement << token
        end
      else
        statement << token
      end
      i += 1
    end
    found.compact
  end

  # The lines of the comment that ends at index i (0-based), going up, if it
  # is a Doxygen comment that does not belong to the line before; else nil
  def doc_comment_ending_at(lines, i)
    stripped = lines[i].strip
    if stripped.start_with?('///', '//!')
      return nil if stripped.match?(%r{\A//[/!]<})

      first = i
      first -= 1 while first.positive? && lines[first - 1].strip.match?(%r{\A//[/!](?!<)})
      return lines[first..i]
    end
    return nil unless stripped.end_with?('*/')

    first = i
    first -= 1 while first >= 0 && !lines[first].include?('/*')
    return nil unless first >= 0 && lines[first].strip.match?(%r{\A/\*[*!](?!<)})

    lines[first..i]
  end

  def documents?(comment)
    return false if comment.nil?

    text = comment.join("\n").gsub(%r{/\*[*!]<?|\*/|^\s*//[/!]<?|^\s*\*}, '')
    !text.gsub(STRUCTURAL, '').strip.empty?
  end

  # Whether the text documents a declaration that Doxygen does not report: a
  # Doxygen comment right before its signature or between its template header
  # and its name, or a trailing one after the ";" or "}" that ends it
  def documented_by_text?(lines, found)
    return true if lines[found.close_line - 1].to_s.match?(%r{[;\}]\s*//[/!]<})

    start = found.start_line - 1
    if lines[start].to_s.strip.match?(%r{\A/\*[*!](?!<)})
      finish = start
      finish += 1 while finish < lines.size - 1 && !lines[finish].include?('*/')
      return true if documents?(lines[start..finish])
    end
    (start...(found.name_line - 1)).each do |k|
      next unless lines[k].strip.match?(%r{(?:\A(?://[/!])|\*/\z)})
      return true if documents?(doc_comment_ending_at(lines, k))
    end
    above = start - 1
    above -= 1 while above >= 0 && lines[above].strip.empty?
    above >= 0 && documents?(doc_comment_ending_at(lines, above))
  end
end
