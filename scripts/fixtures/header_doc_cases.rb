# frozen_string_literal: true

# The matrix of the header docstring coverage gate. Each case is a header, the
# lines that a change adds, and what the gate must count in it: [group, name,
# documented?], where group is :fn or :type. The expected answers follow the
# rules below; whether a comment documents a declaration is what Doxygen
# publishes (checked with Doxygen 1.9.8).
#
# Counted: the public functions, methods, constructors, destructors,
# operators, classes, structs, unions and concepts, also in detail namespaces,
# whose signature, from the template header, requires clause or attributes to
# just before the body, has an added line. Pure virtual functions count;
# "= default" and "= delete" do not, nor do definitions of functions declared
# elsewhere. Members of non-public sections or of non-public nested classes
# are not public. A concept's requires expression is part of its signature.
module HeaderDocCases
  Case = Struct.new(:category, :text, :added, :expected, keyword_init: true)

  # A layout is a list of [line, role]; roles: :sig, :brace, :body and :close
  LAYOUTS = {
    'prototype' => [['int FN(int x);', :sig]],
    'brace on the same line (K&R)' => [['int FN(int x) {', :sig], ['  return x;', :body], ['}', :close]],
    'brace on the next line' => [['int FN(int x)', :sig], ['{', :brace], ['  return x;', :body], ['}', :close]],
    'one line' => [['int FN(int x) { return x; }', :sig]],
    'split parameters' => [['int FN(int x,', :sig], ['       int y)', :sig], ['{', :brace],
                           ['  return x + y;', :body], ['}', :close]],
    'split template header' => [['template <typename T,', :sig], ['          class U = int>', :sig],
                                ['void FN(T, U)', :sig], ['{', :brace], ['}', :close]],
    'requires clause in parentheses' => [['template <typename T>', :sig], ['  requires (sizeof(T) > 1 and', :sig],
                                         ['            sizeof(T) < 8)', :sig], ['void FN(T)', :sig],
                                         ['{', :brace], ['}', :close]],
    'requires expression' => [['template <typename T>', :sig], ['  requires requires(T t) { t.size(); }', :sig],
                              ['void FN(T)', :sig], ['{', :brace], ['}', :close]],
    'attribute on its own line' => [['[[nodiscard]]', :sig], ['inline int FN()', :sig], ['{', :brace],
                                    ['  return 1;', :body], ['}', :close]],
    'split noexcept' => [['int FN(int x) noexcept(noexcept(', :sig], ['    int(x)))', :sig],
                         ['{', :brace], ['  return x;', :body], ['}', :close]],
    'trailing return type' => [['auto FN(int x)', :sig], ['  -> int', :sig], ['{', :brace],
                               ['  return x;', :body], ['}', :close]],
    'default arguments' => [['int FN(int x = 0,', :sig], ['       const char *s = "{(;")', :sig],
                            ['{', :brace], ['  return x;', :body], ['}', :close]],
    'return type on its own line' => [['static inline', :sig], ['unsigned long FN()', :sig], ['{', :brace],
                                      ['  return 0;', :body], ['}', :close]],
    'body with calls' => [['void FN(int x)', :sig], ['{', :brace], ['  if (x > 0) { g(x); }', :body],
                          ['  auto h = [](int y) { return y; };', :body],
                          ['  for (int i = 0; i < x; ++i) { h(i); }', :body], ['}', :close]]
  }.freeze

  TEMPLATE_LAYOUTS = ['split template header', 'requires clause in parentheses', 'requires expression'].freeze

  # A documentation form: [lines before, text after the last :sig line, documented?]
  DOCS = {
    'no comment' => [[], '', false],
    '/** */' => [['/** @brief Doc. */'], '', true],
    '///' => [['/// Doc.'], '', true],
    'multi-line block' => [['/** @brief Doc.', ' *', ' *  More.', ' */'], '', true],
    '//!' => [['//! Doc.'], '', true],
    # Doxygen takes a trailing ///< for the declaration after its ";" or its
    # closing "}"; before the body, after the parameters or a trailing return
    # type it does not (it documents the last parameter, or nothing), but
    # after noexcept or const it does
    'trailing ///< on the signature' => [[], ' ///< Doc.', :if_ends],
    'trailing ///< after the closing brace' => [[], :after_close, true],
    '// is not Doxygen' => [['// not a doc'], '', false],
    '/* */ is not Doxygen' => [['/* not a doc */'], '', false],
    '/** */ and a blank line' => [['/** @brief Doc. */', ''], '', true],
    'trailing ///< of the previous declaration' => [['int w_prev_ID = 0; ///< about w_prev'], '', false],
    'two-line trailing ///< of the previous one' => [['int w_prev_ID = 0; ///< about w_prev,',
                                                     '                             ///< continued'], '', false]
  }.freeze

  PUBLIC_CONTEXTS = ['namespace', 'detail namespace', 'public in a class', 'struct', 'public nested class',
                     'extern "C"'].freeze
  CONTEXTS = {
    'namespace' => ->(d, _) { ['namespace Aleph', '{', *d, '}'] },
    'detail namespace' => ->(d, _) { ['namespace Aleph', '{', 'namespace x_detail', '{', *d, '}', '}'] },
    'public in a class' => lambda { |d, id|
      ['namespace Aleph', '{', '/** @brief W. */', "class W_#{id}", '{', 'public:', *d, '};', '}']
    },
    'struct' => ->(d, id) { ['namespace Aleph', '{', '/** @brief W. */', "struct W_#{id}", '{', *d, '};', '}'] },
    'public nested class' => lambda { |d, id|
      ['namespace Aleph', '{', '/** @brief W. */', "class W_#{id}", '{', 'public:', '/** @brief W. */',
       "class W_in_#{id}", '{', 'public:', *d, '};', '};', '}']
    },
    'extern "C"' => ->(d, _) { ['extern "C"', '{', *d, '}'] },
    'private by default' => lambda { |d, id|
      ['namespace Aleph', '{', '/** @brief W. */', "class W_#{id}", '{', *d, '};', '}']
    },
    'protected' => lambda { |d, id|
      ['namespace Aleph', '{', '/** @brief W. */', "class W_#{id}", '{', 'protected:', *d, '};', '}']
    },
    'private nested class' => lambda { |d, id|
      ['namespace Aleph', '{', '/** @brief W. */', "class W_#{id}", '{', 'private:', '/** @brief W. */',
       "struct W_in_#{id}", '{', *d, '};', '};', '}']
    }
  }.freeze

  module_function

  # One header; returns its text and the line numbers of each role
  def build(layout, doc, context, id)
    before, trailing, = DOCS.fetch(doc)
    last_sig = layout.rindex { |_, r| r == :sig }
    close = layout.rindex { |_, r| r == :close }
    declaration = layout.each_with_index.map do |(line, role), i|
      line = line.gsub('FN', "f_#{id}")
      if trailing == :after_close
        line += ' ///< Doc.' if i == close
      elsif i == last_sig
        line += trailing
      end
      [line, role]
    end
    tagged = before.map { |l| [l.gsub('ID', id.to_s), :doc] } + declaration
    lines = CONTEXTS.fetch(context).call(tagged.map(&:first), id)
    offset = lines.index(tagged.first.first)
    roles = Hash.new { |h, k| h[k] = [] }
    tagged.each_with_index { |(_, role), i| roles[role] << offset + i + 1 }
    ["#{lines.join("\n")}\n", roles]
  end

  # Every layout, with every form of documentation, in every context
  def layout_cases
    id = 0
    LAYOUTS.flat_map do |layout_name, layout|
      DOCS.keys.product(CONTEXTS.keys).filter_map do |doc, context|
        next if context == 'extern "C"' && TEMPLATE_LAYOUTS.include?(layout_name)
        next if DOCS[doc][1] == :after_close && layout.none? { |_, r| r == :close }

        id += 1
        documented = DOCS[doc][2]
        if documented == :if_ends
          documented = layout.reverse.find { |_, r| r == :sig }.first.end_with?(';', '}') ||
                       layout_name == 'split noexcept' # its last line closes a noexcept
        end
        Case.new(category: "layout x documentation x context: #{layout_name} | #{doc} | #{context}",
                 text: build(layout, doc, context, id).first, added: :all,
                 expected: PUBLIC_CONTEXTS.include?(context) ? [[:fn, "f_#{id}", documented]] : [])
      end
    end
  end

  # One line of the signature touches the declaration; the body or the comment
  # alone does not (a lone "{" is not asked either way)
  def touch_cases
    id = 10_000
    LAYOUTS.flat_map do |layout_name, layout|
      next [] if layout.count { |_, r| r == :sig || r == :body } < 2

      text, roles = build(layout, '/** */', 'namespace', id += 1)
      touching = roles[:sig].map do |n|
        Case.new(category: "touches the signature: #{layout_name} line #{n}", text: text,
                 added: [n], expected: [[:fn, "f_#{id}", true]])
      end
      touching + (roles[:body] + roles[:doc]).map do |n|
        Case.new(category: "does not touch the signature: #{layout_name} line #{n}", text: text,
                 added: [n], expected: [])
      end
    end
  end

  def special(category, name, text, expected, added: :all)
    id = (@special_id = (@special_id || 20_000) + 1).to_s
    Case.new(category: "#{category}: #{name}", text: text.gsub('ID', id), added: added,
             expected: expected.map { |g, n, d| [g, n.gsub('ID', id), d] })
  end

  def in_class(body)
    "namespace Aleph\n{\n/** @brief K. */\nclass K_ID\n{\npublic:\n#{body}\n};\n}\n"
  end

  def in_namespace(body)
    "namespace Aleph\n{\n#{body}\n}\n"
  end

  def kind_cases
    k = lambda do |name, body, expected|
      special('kind of declaration', name, in_class(body), [[:type, 'K_ID', true], *expected])
    end
    n = ->(name, body, expected) { special('kind of declaration', name, in_namespace(body), expected) }
    [
      k.('constructor with initializers', "  K_ID()\n    : a{0},\n      b(1)\n  {\n  }\n  int a, b;",
         [[:fn, 'K_ID', false]]),
      k.('braces in the initializers', "  K_ID(int x)\n    : first{x},\n      second{x}\n  {\n  }\n  int first, second;",
         [[:fn, 'K_ID', false]]),
      k.('destructor', "  ~K_ID()\n  {\n  }", [[:fn, '~K_ID', false]]),
      k.('operator==', "  bool operator==(const K_ID &o) const noexcept\n  {\n    return true;\n  }",
         [[:fn, 'operator==', false]]),
      k.('operator()', "  int operator()(int x) const\n  {\n    return x;\n  }", [[:fn, 'operator()', false]]),
      k.('operator bool', "  explicit operator bool() const\n  {\n    return true;\n  }",
         [[:fn, 'operatorbool', false]]),
      k.('operator[]', "  int &operator[](unsigned long i)\n  {\n    return v[i];\n  }\n  int v[4];",
         [[:fn, 'operator[]', false]]),
      k.('operator<=>', "  auto operator<=>(const K_ID &) const\n  {\n    return 0;\n  }",
         [[:fn, 'operator<=>', false]]),
      k.('operator<', "  bool operator<(const K_ID &) const\n  {\n    return false;\n  }",
         [[:fn, 'operator<', false]]),
      k.('operator>>=', "  K_ID &operator>>=(int)\n  {\n    return *this;\n  }", [[:fn, 'operator>>=', false]]),
      k.('static member function', "  static K_ID make()\n  {\n    return {};\n  }", [[:fn, 'make', false]]),
      k.('pure virtual', '  virtual void run() = 0;', [[:fn, 'run', false]]),
      k.('= default and = delete',
         "  K_ID() = default;\n  K_ID(const K_ID &) = delete;\n  K_ID &operator=(const K_ID &) = delete;", []),
      k.('friend defined in the class',
         "  friend bool equals(const K_ID &a, const K_ID &b)\n  {\n    return true;\n  }",
         [[:fn, 'equals', false]]),
      k.('friend class', '  friend class Other_ID;', []),
      k.('explicit(condition)', "  explicit(sizeof(int) > 1) K_ID(int x)\n  {\n  }", [[:fn, 'K_ID', false]]),
      k.('member initialized by a call', "  unsigned long n = size_of_it();\n  void (*callback)(int);", []),
      n.('decltype(auto)', "decltype(auto) pass_ID(int &x)\n{\n  return x;\n}", [[:fn, 'pass_ID', false]]),
      n.('__attribute__', "__attribute__((unused)) static void quiet_ID()\n{\n}", [[:fn, 'quiet_ID', false]]),
      n.('function specialization', "template <>\nvoid spec_ID<int>(int)\n{\n}", [[:fn, 'spec_ID', false]]),
      n.('std::function returned', "std::function<void(int)> make_ID()\n{\n  return {};\n}",
         [[:fn, 'make_ID', false]]),
      n.('trailing ///< of a parameter', "void takes_ID(int x, ///< the x\n              int y)\n{\n}",
         [[:fn, 'takes_ID', false]]),
      n.('comment between the template header and the name',
         "template <typename T>\n/** @brief Doc. */\nvoid between_ID(T)\n{\n}", [[:fn, 'between_ID', true]]),
      n.('a group opener documents nothing', "/** @{ */\nvoid grouped_ID();\n/** @} */",
         [[:fn, 'grouped_ID', false]]),
      n.('an override needs its own documentation',
         "struct B_ID\n{\n  /** @brief D. */\n  virtual void run_ID();\n};\n/// Doc.\nstruct D_ID : B_ID\n" \
         "{\n  void run_ID() override;\n};",
         [[:type, 'B_ID', false], [:fn, 'run_ID', true], [:type, 'D_ID', true], [:fn, 'run_ID', false]]),
      n.('undocumented class', "class T_ID\n{\n};", [[:type, 'T_ID', false]]),
      n.('struct with bases on two lines',
         "/// Doc.\nstruct T_ID : public Base<int>,\n              public Other\n{\n};", [[:type, 'T_ID', true]]),
      n.('class template', "template <typename T>\nclass T_ID\n{\n};", [[:type, 'T_ID', false]]),
      n.('concept', "template <typename T>\nconcept C_ID = requires(T t) { t.size(); };", [[:type, 'C_ID', false]]),
      # Doxygen skips the plain comment; judging by the text would not, so this
      # case needs the entry of Doxygen to match the specialization
      special('kind of declaration', 'specialization with "::" in its arguments',
              "namespace Aleph\n{\nstruct X_ID {};\n}\n/** @brief Doc. */\n// A plain comment.\ntemplate <>\n" \
              "struct std::hash<Aleph::X_ID>\n{\n};\n", [[:type, 'X_ID', false], [:type, 'hash', true]])
    ]
  end

  def negative_cases
    n = lambda do |name, body, expected = []|
      special('not a declaration', name, in_namespace(body), expected)
    end
    [
      n.('multi-line macro', "#define ALEPH_ID(x)   \\\n  void x()             \\\n  {                    \\\n  }"),
      n.('commented code', "// void gone_ID();\n/* int also_ID(int)\n{\n} */"),
      n.('code inside a string', 'inline const char *s_ID = "void f() {";'),
      n.('variable initialized by a call', 'inline const int v_ID = compute(3);'),
      n.('variable with a literal in parentheses', 'inline std::string s_ID("abc");'),
      n.('function pointers and aliases',
         "using Fn_ID = void (*)(int);\ntypedef void (*Fn2_ID)(int);\nextern void (*hook_ID)(int);"),
      n.('static_assert', 'static_assert(sizeof(int) >= 2, "x");'),
      n.('explicit instantiation', "template class Foo_ID<int>;\nextern template void g_ID<int>(int);"),
      n.('forward declarations', "class X_ID;\nstruct Y_ID;\ntemplate <class T> class Z_ID;"),
      n.('enumerators with calls', 'enum class E_ID { A = f(1), B };'),
      n.('variable with braces', 'inline std::array<int, 2> a_ID{f(1), 2};'),
      n.('lambda in a variable', 'inline auto l_ID = [](int x) { return x; };'),
      n.('definition outside its class', "template <typename T>\nvoid Holder_ID<T>::run()\n{\n}"),
      n.('digit separators', "inline constexpr long big_ID = 1'000'000;\nvoid after_ID()\n{\n}",
         [[:fn, 'after_ID', false]]),
      # with a single one, a reader that takes it for a quote loses the ";"
      n.('a single digit separator', "inline constexpr long one_ID = 1'000;\nvoid after_one_ID()\n{\n}",
         [[:fn, 'after_one_ID', false]]),
      n.('raw string', "inline const char *r_ID = R\"x(void f() { )x\";\nvoid after_ID()\n{\n}",
         [[:fn, 'after_ID', false]])
    ]
  end

  # The lines of a constructor's initializers are not its signature; a trailing
  # requires expression over several lines is
  def signature_cases
    constructor = "  /** @brief Doc. */\n  K_ID(int x)\n    : first{x},\n      second(x)\n  {\n  }\n  int first, second;"
    trailing = "template <typename T>\nvoid constrained_ID(T t) requires requires {\n    t.size();\n  }\n{\n}"
    touch = lambda do |category, name, text, line, expected|
      special(category, name, text, expected, added: [line])
    end
    [
      touch.('touches the signature', 'a constructor', in_class(constructor), 8, [[:fn, 'K_ID', true]]),
      touch.('does not touch the signature', 'the first initializer', in_class(constructor), 9, []),
      touch.('does not touch the signature', 'the second initializer', in_class(constructor), 10, []),
      touch.('touches the signature', 'a trailing requires expression', in_namespace(trailing), 5,
             [[:fn, 'constrained_ID', false]])
    ]
  end

  # Found by comparing analyzers on the whole library and on merged pull
  # requests, and the declarations that Doxygen does not report
  def real_code_cases
    twice = "/** @brief Doc. */\nint twice_ID(int x);\n\nint twice_ID(const int x)\n{\n  return 2 * x;\n}"
    concept = "/// Doc.\ntemplate <class T>\nconcept Eq_ID = requires(const T &x)\n{\n  { x == x };\n};"
    overload = lambda do |argument|
      "/** @brief Doc. */\nint find_ID(int key);\n\n/// \\overload#{argument}\nint find_ID(long key);"
    end
    dropped = ->(comment) { "#ifdef ALEPH_NEVER_DEFINED_ID\n#{comment}\nvoid dropped_ID(int);\n#endif" }
    r = lambda do |name, body, expected, added: :all|
      special('found in real code', name, in_namespace(body), expected, added: added)
    end
    [
      r.('definition of a function already declared', twice, [[:fn, 'twice_ID', true]]),
      r.('only the definition is touched', twice, [], added: [6]),
      r.('the requires expression of a concept', concept, [[:type, 'Eq_ID', true]], added: [7]),
      r.('\overload with a signature documents the other one', overload.(' find_ID(int key)'),
         [[:fn, 'find_ID', true], [:fn, 'find_ID', false]]),
      r.('\overload without one documents its own', overload.(''),
         [[:fn, 'find_ID', true], [:fn, 'find_ID', true]]),
      r.('dropped by the preprocessor, with @cond only', dropped.('/// @cond INTERNAL'),
         [[:fn, 'dropped_ID', false]]),
      r.('dropped by the preprocessor, documented', dropped.('/** @brief Doc. */'),
         [[:fn, 'dropped_ID', true]]),
      r.('dropped by the preprocessor, trailing comments',
         "#ifdef ALEPH_NEVER_DEFINED_ID\nvoid before_ID(int x) ///< about x\n{\n}\n" \
         "void after_ID(int x); ///< Doc.\n#endif",
         [[:fn, 'before_ID', false], [:fn, 'after_ID', true]])
    ]
  end

  def all
    @all ||= layout_cases + touch_cases + kind_cases + negative_cases + signature_cases + real_code_cases
  end
end
