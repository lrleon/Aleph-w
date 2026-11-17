#!/usr/bin/env ruby

require 'minitest/autorun'
require 'tmpdir'
require_relative 'ci_policy_checks'

class TestCiPolicyChecks < Minitest::Test
  def test_cpp_token_stream_ignores_comments_and_whitespace
    code = <<~CPP
      // This is a line comment
      #include <iostream>
      /* This is a 
         block comment */
      int main() {
        return 0; // exit
      }
    CPP
    
    expected = ["#", "include", "<", "iostream", ">", "int", "main", "(", ")", "{", "return", "0", ";", "}"]
    assert_equal expected.join("\n"), cpp_token_stream(code)
  end

  def test_cpp_token_stream_preserves_literals
    code = 'const char* s = "hello world"; char c = \'a\';'
    tokens = cpp_token_stream(code).split("\n")
    assert_includes tokens, '"hello world"'
    assert_includes tokens, "'a'"
  end

  def test_cpp_token_stream_raw_string_literal
    code = 'const char* s = R"(hello\nworld)";'
    tokens = cpp_token_stream(code).split("\n")
    assert_includes tokens, 'R"(hello\nworld)"'
    refute_includes tokens, 'hello'
  end

  def test_cpp_token_stream_raw_string_with_custom_delimiter
    code = 'const char* s = R"delim(x"y)delim";'
    tokens = cpp_token_stream(code).split("\n")
    assert_includes tokens, 'R"delim(x"y)delim"'
    refute_includes tokens, '"y'
  end

  def test_header_has_code_changes_identical
    path = "test.H"
    base = "main"
    content = "void foo();"
    
    def_mock_git_show({ "origin/#{base}:#{path}" => content }) do
      def_mock_file_read({ path => content }) do
        refute header_has_code_changes?(base, path), "Identical files should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_whitespace_only
    path = "test.H"
    base = "main"
    old_content = "void foo();"
    new_content = "  void   foo ( ) ; "
    
    def_mock_git_show({ "origin/#{base}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        refute header_has_code_changes?(base, path), "Whitespace-only changes should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_doc_only
    path = "test.H"
    base = "main"
    
    old_content = <<~CPP
      /** @brief Old doc */
      void foo();
    CPP
    
    new_content = <<~CPP
      /** @brief New doc
          Extended explanation */
      void foo();
    CPP
    
    def_mock_git_show({ "origin/#{base}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        refute header_has_code_changes?(base, path), "Doc-only changes should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_real_code
    path = "test.H"
    base = "main"
    
    old_content = "void foo();"
    new_content = "void foo(int x);"
    
    def_mock_git_show({ "origin/#{base}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        assert header_has_code_changes?(base, path), "Real code changes SHOULD be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_rename_no_code_change
    old_path = "old.H"
    new_path = "new.H"
    base = "main"
    content = "void foo();"
    
    def_mock_git_show({ "origin/#{base}:#{old_path}" => content }) do
      def_mock_file_read({ new_path => content }) do
        refute header_has_code_changes?(base, new_path, old_path), "Rename with NO code changes should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_rename_with_code_change
    old_path = "old.H"
    new_path = "new.H"
    base = "main"
    old_content = "void foo();"
    new_content = "void bar();"
    
    def_mock_git_show({ "origin/#{base}:#{old_path}" => old_content }) do
      def_mock_file_read({ new_path => new_content }) do
        assert header_has_code_changes?(base, new_path, old_path), "Rename WITH code changes SHOULD be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_new_header_doc_only
    path = "new.H"
    base = "main"
    
    content = <<~CPP
      /** @brief New header with only docs */
    CPP
    
    def_mock_git_show({}) do
      def_mock_file_read({ path => content }) do
        refute header_has_code_changes?(base, path), "A new header with ONLY docs should NOT require tests"
      end
    end
  end

  def test_header_has_code_changes_new_header_with_code
    path = "new.H"
    base = "main"
    
    content = "void bar();"
    
    def_mock_git_show({}) do
      def_mock_file_read({ path => content }) do
        assert header_has_code_changes?(base, path), "A new header with CODE should require tests"
      end
    end
  end

  def test_changed_files_parses_rename
    def_mock_run(["", "R100\told.H\tnew.H\n"]) do
      result = changed_files('main')
      assert_equal 1, result.size
      assert_equal 'R', result[0][:status]
      assert_equal 'old.H', result[0][:old_path]
      assert_equal 'new.H', result[0][:path]
    end
  end

  def test_changed_files_parses_copy
    def_mock_run(["", "C100\tsrc.H\tdst.H\n"]) do
      result = changed_files('main')
      assert_equal 1, result.size
      assert_equal 'C', result[0][:status]
      assert_equal 'src.H', result[0][:old_path]
      assert_equal 'dst.H', result[0][:path]
    end
  end

  def test_changed_files_parses_added
    def_mock_run(["", "A\tnew.H\n"]) do
      result = changed_files('main')
      assert_equal 1, result.size
      assert_equal 'A', result[0][:status]
      assert_equal 'new.H', result[0][:path]
    end
  end

  def test_changed_files_parses_multiple_entries
    git_output = "A\tadded.H\nM\tmodified.cc\nR090\told.H\tnew.H\nC100\tsrc.cc\tdst.cc\n"
    def_mock_run(["", git_output]) do
      result = changed_files('main')
      assert_equal 4, result.size
      assert_equal 'new.H',    result[2][:path]
      assert_equal 'old.H',    result[2][:old_path]
      assert_equal 'dst.cc',   result[3][:path]
      assert_equal 'src.cc',   result[3][:old_path]
    end
  end

  def test_std_uses_counts_the_code_only
    code = <<~CPP
      // std::vector in a comment
      /* std::sort in a block */
      const char * s = "std::map in a literal";
      std::vector<int> a;  std::vector<std::pair<int, int>> b;
      std::array<int, 3> c;
      void f() { std::sort(a.begin(), a.end()); }
      std::set<int> d;  // std-interop: what the standard library takes
      ::std::deque<int> e;
    CPP
    assert_equal({ 'vector' => 2, 'sort' => 1, 'deque' => 1 }, std_uses(code))
  end

  def test_added_std_uses_reports_what_a_file_adds
    path = 'tpl_x.H'
    base = 'main'
    old_content = "std::vector<int> a;\nstd::vector<int> b;\n"
    new_content = "std::vector<int> b;\nstd::vector<int> a;\nstd::vector<int> c;\n" \
                  "void f() { std::sort(p, q); }\n"
    def_mock_git_show({ "origin/#{base}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        assert_equal({ 'vector' => 1, 'sort' => 1 }, added_std_uses(base, path))
      end
    end
  end

  def test_added_std_uses_ignores_moved_and_removed_uses
    path = 'tpl_x.H'
    base = 'main'
    old_content = "std::vector<int> a;\nstd::sort(p, q);\n"
    new_content = "void g();\nstd::vector<int> a;\n"
    def_mock_git_show({ "origin/#{base}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        assert_empty added_std_uses(base, path)
      end
    end
  end

  def test_added_std_uses_of_a_new_and_of_a_renamed_file
    base = 'main'
    code = "std::map<int, int> m;\n"
    def_mock_git_show({ "origin/#{base}:old.H" => code }) do
      def_mock_file_read({ 'new.H' => code, 'fresh.H' => code }) do
        assert_empty added_std_uses(base, 'new.H', 'old.H')
        assert_equal({ 'map' => 1 }, added_std_uses(base, 'fresh.H'))
      end
    end
  end

  def test_std_checked_library_files
    assert std_checked_library_file?('tpl_net.H')
    assert std_checked_library_file?('aleph-graph.C')
    refute std_checked_library_file?('Tests/tpl_net_test.cc')
    refute std_checked_library_file?('benchmarks/ca/bench_support.H')
    refute std_checked_library_file?('ah-convert.H')
    refute std_checked_library_file?('README.md')
  end

  private

  def def_mock_run(results)
    call_count = 0
    old_method = method(:run!)
    Object.send(:define_method, :run!) do |*_args|
      results[call_count].tap { call_count += 1 }
    end
    yield
  ensure
    Object.send(:define_method, :run!, old_method)
  end

  def def_mock_git_show(mapping)
    old_method = method(:git_show_optional)
    Object.send(:define_method, :git_show_optional) do |spec|
      mapping[spec]
    end
    yield
  ensure
    Object.send(:define_method, :git_show_optional, old_method)
  end

  def def_mock_file_read(mapping)
    old_method = File.method(:read)
    File.singleton_class.send(:define_method, :read) do |p|
      mapping.key?(p) ? mapping[p] : old_method.call(p)
    end
    yield
  ensure
    File.singleton_class.send(:define_method, :read, old_method)
  end
end


# The check of the moved-from pattern (scripts/ci_moved_from_check.rb)
class TestMovedFromCheck < Minitest::Test
  STORAGE = <<~CPP
    class Storage
    {
      Array<int> items;
      size_t n = 0;
    public:
      size_t size() const { return n; }
    };
  CPP

  def problems(src, tests = '', headers = '')
    moved_from_problems(src, tests, headers + src).map { |_, msg| msg[/\A\w+/] }
  end

  def test_flags_a_class_whose_implicit_move_copies_a_size
    assert_equal ['Storage'], problems(STORAGE)
  end

  def test_a_marked_class_passes
    assert_empty problems(STORAGE.sub("{\n", "{\n  // moved-from: n is a parameter\n"))
  end

  def test_a_tested_class_passes
    assert_empty problems(STORAGE, 'template <> struct Moved_From_Case<Storage> {};')
  end

  def test_an_alias_names_the_class_in_a_test
    assert_empty problems(STORAGE, 'Moved_From_Case<Int_Storage>', 'using Int_Storage = Storage;')
  end

  def test_an_alias_of_a_test_names_the_class_in_it
    assert_empty problems(STORAGE, "using Int_Storage = Storage;\nMoved_From_Case<Int_Storage>")
  end

  # The name of the class elsewhere in a test, as a use, a comment or a
  # case of another class, does not test it
  def test_only_a_moved_from_case_tests_a_class
    tests = "// Storage\nStorage s;\ntemplate <> struct Moved_From_Case<Other_Storage> {};\n"
    assert_equal ['Storage'], problems(STORAGE, tests)
    assert_empty problems(STORAGE, tests + "template <> struct Moved_From_Case<F::Storage> {};\n")
  end

  # The access labels of a nested class are not those of the class
  def test_a_nested_class_with_access_labels
    src = STORAGE.sub("{\n", "{\n  struct Node { public: int x; };\n")
    assert_equal ['Storage'], problems(src)
  end

  # A brace initializer does not make a member a function
  def test_a_member_with_a_brace_initializer
    src = STORAGE.sub('size_t n = 0;', 'size_t n{count()};')
    assert_equal ['Storage'], problems(src)
  end

  def test_a_defaulted_move_is_implicit
    assert_equal ['Storage'], problems(STORAGE.sub('public:', "public:\n  Storage(Storage &&) = default;"))
  end

  def test_a_destructor_makes_the_moves_copies
    assert_empty problems(STORAGE.sub('public:', "public:\n  ~Storage() { }"))
  end

  def test_written_moves_need_a_test
    src = STORAGE.sub('public:', "public:\n  Storage(Storage && o) noexcept\n" \
                                 "    : items(std::move(o.items)), n(std::exchange(o.n, 0)) { }")
    assert_equal ['Storage'], problems(src)
    assert_empty problems(src, 'Moved_From_Case<Storage>')
  end

  def test_an_aggregate_is_not_flagged
    src = "struct Result\n{\n  Array<int> path;\n  size_t cost = 0;\n  bool ok() const { return true; }\n};\n"
    assert_empty problems(src)
  end

  def test_comments_literals_and_other_branches_do_not_count
    src = <<~CPP
      // class Fake { Array<int> a; size_t n; public: void f(); };
      const char * s = "class Fake2 { Array<int> a; size_t n; public: void f(); };";
      const char * r = R"x(class Fake3 { Array<int> a; size_t n; public: void f(); };)x";
      int big = 1'000'000;
      #if X
      class Real { Array<int> a; size_t n; public: void f(); };
      #else
      class Other { Array<int> a; size_t n; public: void f(); };
      #endif
    CPP
    assert_equal ['Real'], problems(src)
  end

  def test_enumerations_and_aliases_are_scalars
    src = "enum class Mode { a, b };\nusing Count = std::size_t;\n" \
          "class E { Array<int> a; Mode m; public: void f(); };\n" \
          "class F { Array<int> a; Count c; public: void f(); };\n"
    assert_equal %w[E F], problems(src)
  end

  def test_a_nested_class_under_a_base_clause
    src = "template <class T> class Outer : public Base<T>\n{\n" \
          "  class Inner { DynList<T> l; T * last; public: void f(); };\n" \
          "  Inner in;\npublic:\n  void g();\n};\n"
    assert_equal ['Inner'], problems(src)
  end

  def test_a_class_that_the_base_had_does_not_fail
    Dir.mktmpdir do |dir|
      Dir.chdir(dir) do
        File.write('storage.H', STORAGE)
        old = Open3.method(:capture2)
        detail = [{ path: 'storage.H', old_path: 'storage.H' }]
        begin
          Open3.singleton_class.send(:define_method, :capture2) { |*_a, **_k| [STORAGE, nil] }
          assert_empty moved_from_failures('master', detail)
          Open3.singleton_class.send(:define_method, :capture2) { |*_a, **_k| ['', nil] }
          assert_equal 1, moved_from_failures('master', detail).size
        ensure
          Open3.singleton_class.send(:define_method, :capture2, old)
        end
      end
    end
  end
end
