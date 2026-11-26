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

  # The prefix of a raw literal is not an identifier: the literal stays
  # whole, and what it holds is not code
  def test_cpp_token_stream_prefixed_raw_string_literals
    %w[u8R uR UR LR].each do |prefix|
      code = "auto s = #{prefix}\"tag(\" std::vector )tag\";\nint x;\n"
      tokens = cpp_token_stream(code).split("\n")
      assert_includes tokens, "#{prefix}\"tag(\" std::vector )tag\"", prefix
      assert_equal %w[; int x ;], tokens.last(4), prefix
      assert_empty std_uses(code), prefix
    end
  end

  def test_header_has_code_changes_identical
    path = "test.H"
    before = "origin/main"
    content = "void foo();"
    
    def_mock_git_show({ "#{before}:#{path}" => content }) do
      def_mock_file_read({ path => content }) do
        refute header_has_code_changes?(before, path), "Identical files should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_whitespace_only
    path = "test.H"
    before = "origin/main"
    old_content = "void foo();"
    new_content = "  void   foo ( ) ; "
    
    def_mock_git_show({ "#{before}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        refute header_has_code_changes?(before, path), "Whitespace-only changes should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_doc_only
    path = "test.H"
    before = "origin/main"
    
    old_content = <<~CPP
      /** @brief Old doc */
      void foo();
    CPP
    
    new_content = <<~CPP
      /** @brief New doc
          Extended explanation */
      void foo();
    CPP
    
    def_mock_git_show({ "#{before}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        refute header_has_code_changes?(before, path), "Doc-only changes should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_real_code
    path = "test.H"
    before = "origin/main"
    
    old_content = "void foo();"
    new_content = "void foo(int x);"
    
    def_mock_git_show({ "#{before}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        assert header_has_code_changes?(before, path), "Real code changes SHOULD be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_rename_no_code_change
    old_path = "old.H"
    new_path = "new.H"
    before = "origin/main"
    content = "void foo();"
    
    def_mock_git_show({ "#{before}:#{old_path}" => content }) do
      def_mock_file_read({ new_path => content }) do
        refute header_has_code_changes?(before, new_path, old_path), "Rename with NO code changes should NOT be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_rename_with_code_change
    old_path = "old.H"
    new_path = "new.H"
    before = "origin/main"
    old_content = "void foo();"
    new_content = "void bar();"
    
    def_mock_git_show({ "#{before}:#{old_path}" => old_content }) do
      def_mock_file_read({ new_path => new_content }) do
        assert header_has_code_changes?(before, new_path, old_path), "Rename WITH code changes SHOULD be counted as code changes"
      end
    end
  end

  def test_header_has_code_changes_new_header_doc_only
    path = "new.H"
    before = "origin/main"
    
    content = <<~CPP
      /** @brief New header with only docs */
    CPP
    
    def_mock_git_show({}) do
      def_mock_file_read({ path => content }) do
        refute header_has_code_changes?(before, path), "A new header with ONLY docs should NOT require tests"
      end
    end
  end

  def test_header_has_code_changes_new_header_with_code
    path = "new.H"
    before = "origin/main"
    
    content = "void bar();"
    
    def_mock_git_show({}) do
      def_mock_file_read({ path => content }) do
        assert header_has_code_changes?(before, path), "A new header with CODE should require tests"
      end
    end
  end

  def test_changed_files_parses_rename
    def_mock_run(["", "b0\n", "R100\told.H\tnew.H\n"]) do
      result = changed_files('main')
      assert_equal 1, result.size
      assert_equal 'R', result[0][:status]
      assert_equal 'old.H', result[0][:old_path]
      assert_equal 'new.H', result[0][:path]
    end
  end

  def test_changed_files_parses_copy
    def_mock_run(["", "b0\n", "C100\tsrc.H\tdst.H\n"]) do
      result = changed_files('main')
      assert_equal 1, result.size
      assert_equal 'C', result[0][:status]
      assert_equal 'src.H', result[0][:old_path]
      assert_equal 'dst.H', result[0][:path]
    end
  end

  def test_changed_files_parses_added
    def_mock_run(["", "b0\n", "A\tnew.H\n"]) do
      result = changed_files('main')
      assert_equal 1, result.size
      assert_equal 'A', result[0][:status]
      assert_equal 'new.H', result[0][:path]
    end
  end

  def test_changed_files_parses_multiple_entries
    git_output = "A\tadded.H\nM\tmodified.cc\nR090\told.H\tnew.H\nC100\tsrc.cc\tdst.cc\n"
    def_mock_run(["", "b0\n", git_output]) do
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
    before = 'origin/main'
    old_content = "std::vector<int> a;\nstd::vector<int> b;\n"
    new_content = "std::vector<int> b;\nstd::vector<int> a;\nstd::vector<int> c;\n" \
                  "void f() { std::sort(p, q); }\n"
    def_mock_git_show({ "#{before}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        assert_equal({ 'vector' => 1, 'sort' => 1 }, added_std_uses(before, path))
      end
    end
  end

  def test_added_std_uses_ignores_moved_and_removed_uses
    path = 'tpl_x.H'
    before = 'origin/main'
    old_content = "std::vector<int> a;\nstd::sort(p, q);\n"
    new_content = "void g();\nstd::vector<int> a;\n"
    def_mock_git_show({ "#{before}:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        assert_empty added_std_uses(before, path)
      end
    end
  end

  def test_added_std_uses_of_a_new_and_of_a_renamed_file
    before = 'origin/main'
    code = "std::map<int, int> m;\n"
    def_mock_git_show({ "#{before}:old.H" => code }) do
      def_mock_file_read({ 'new.H' => code, 'fresh.H' => code }) do
        assert_empty added_std_uses(before, 'new.H', 'old.H')
        assert_equal({ 'map' => 1 }, added_std_uses(before, 'fresh.H'))
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

  def test_std_checked_test_files
    assert std_checked_test_file?('Tests/tpl_net_test.cc')
    assert std_checked_test_file?('Tests/moved_from.H')
    refute std_checked_test_file?('Tests/ah-stl-zip_test.cc')
    refute std_checked_test_file?('Tests/ah_convert_composition_test.cc')
    refute std_checked_test_file?('tpl_net.H')
    refute std_checked_test_file?('Examples/dijkstra_example.cc')
    refute std_checked_test_file?('Tests/CMakeLists.txt')
  end

  def test_a_test_adds_only_what_it_did_not_have
    path = 'Tests/x_test.cc'
    old_content = "std::vector<int> a;\nstd::vector<int> b;\n"
    new_content = old_content + "std::vector<int> c;\n" \
                  "std::sort(a.begin(), a.end()); // std-interop: an independent oracle\n"
    def_mock_git_show({ "origin/main:#{path}" => old_content }) do
      def_mock_file_read({ path => new_content }) do
        assert_equal({ 'vector' => 1 }, added_std_uses('origin/main', path))
      end
    end
  end

  # What each file had before comes from the merge base of HEAD and the
  # base, not from the tip of the base: the merge that the CI checks out, of
  # an event before master moved, adds nothing by changing an assertion in a
  # test whose master then dropped one of its std::vector
  def test_the_merge_of_an_event_before_master_moved_adds_nothing
    in_a_repository_whose_master_moved do |git, c|
      git.call('switch', '-q', '--detach', c[:b0])
      git.call('merge', '-q', '--no-ff', '-m', 'merge', c[:assertion])
      assert_equal ['Tests/x_test.cc'], changed_files('master').map { |d| d[:path] }
      assert_empty added_std_uses(merge_base('master'), 'Tests/x_test.cc')
    end
  end

  # Nor does the head of that pull request, behind master
  def test_the_head_of_a_pull_request_behind_master_adds_nothing
    in_a_repository_whose_master_moved do |git, c|
      git.call('switch', '-q', '--detach', c[:assertion])
      assert_equal ['Tests/x_test.cc'], changed_files('master').map { |d| d[:path] }
      assert_empty added_std_uses(merge_base('master'), 'Tests/x_test.cc')
    end
  end

  # And a std::vector that a pull request adds there counts once, not with
  # the one that master dropped
  def test_a_use_added_before_master_moved_counts_once
    in_a_repository_whose_master_moved do |git, c|
      git.call('switch', '-q', '--detach', c[:b0])
      git.call('merge', '-q', '--no-ff', '-m', 'merge', c[:addition])
      changed_files('master')
      assert_equal({ 'vector' => 1 }, added_std_uses(merge_base('master'), 'Tests/x_test.cc'))
    end
  end

  private

  # A test with two std::vector, a and b, apart from the assertion that a
  # pull request changes, so that the changes merge
  X_TEST = "#include <vector>\nstd::vector<int> a;\nint f() { return 1; }\n" \
           "int g() { return 3; }\nint h() { return 4; }\nint k() { return 5; }\n" \
           "std::vector<int> b;\n"

  # Yields, in a repository of its own whose origin is a bare repository,
  # the commits by name: b0 has X_TEST in Tests/x_test.cc; assertion, from
  # b0, changes its assertion; addition, from b0, adds a third std::vector;
  # and b1, from b0 and the master of origin, drops b
  def in_a_repository_whose_master_moved
    Dir.mktmpdir do |dir|
      run!(['git', 'init', '-q', '--bare', '-b', 'master', "#{dir}/origin.git"])
      run!(['git', 'clone', '-q', "#{dir}/origin.git", "#{dir}/work"])
      Dir.chdir("#{dir}/work") do
        git = ->(*args) { run!(['git', '-c', 'user.name=t', '-c', 'user.email=t@t', *args]).strip }
        commit = lambda do |from, text, message|
          git.call('switch', '-q', '--detach', from) if from
          FileUtils.mkdir_p('Tests')
          File.write('Tests/x_test.cc', text)
          git.call('add', '-A')
          git.call('commit', '-q', '-m', message)
          git.call('rev-parse', 'HEAD')
        end
        c = { b0: commit.call(nil, X_TEST, 'b0') }
        c[:assertion] = commit.call(c[:b0], X_TEST.sub('return 1', 'return 2'), 'assertion')
        c[:addition] = commit.call(c[:b0], "#{X_TEST}std::vector<int> c;\n", 'addition')
        c[:b1] = commit.call(c[:b0], X_TEST.sub("std::vector<int> b;\n", ''), 'b1')
        git.call('push', '-q', 'origin', "#{c[:b1]}:refs/heads/master")
        yield git, c
      end
    end
  end

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

  # The same class, in the namespace F and in the class template Sync
  F_STORAGE = "namespace F\n{\n#{STORAGE}}\n".freeze
  SYNC_STORAGE = "template <class T>\nstruct Sync\n{\n#{STORAGE}};\n".freeze

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
    assert_empty problems(F_STORAGE, tests + "template <> struct Moved_From_Case<F::Storage> {};\n")
  end

  # A name qualified with another scope names another class: the Storage
  # of Other does not test the global one, nor that of F (c03)
  def test_a_qualified_name_names_the_class_of_its_scope
    assert_equal ['Storage'], problems(STORAGE, 'Moved_From_Case<Other::Storage>')
    assert_equal ['Storage'], problems(F_STORAGE, 'Moved_From_Case<Other::Storage>')
    assert_empty problems(F_STORAGE, 'Moved_From_Case<F::Storage>')
    assert_empty problems(F_STORAGE, 'Moved_From_Case<Storage>')
    assert_empty problems(STORAGE, 'Moved_From_Case<::Storage>')
    nested = "class Fft\n{\n  Array<int> a;\npublic:\n  Fft(Fft &&) = delete;\n#{STORAGE}};\n"
    assert_empty problems(nested, "using F = Fft;\nMoved_From_Case<F::Storage>")
    assert_empty problems(nested, 'Moved_From_Case<F::Storage>', 'using F = Aleph::Fft;')
    assert_equal ['Storage'], problems(nested, "using F = Other;\nMoved_From_Case<F::Storage>")
    assert_equal ['Storage'], problems(STORAGE, "using F = Other;\nusing W = F::Storage;\nMoved_From_Case<W>")
  end

  # A pointer, a reference or an array of a class is not the class (c02)
  def test_a_pointer_to_the_class_does_not_name_it
    assert_equal ['Storage'], problems(STORAGE, "using P = Storage *;\nMoved_From_Case<P>")
    assert_equal ['Storage'], problems(STORAGE, "using R = const Storage &;\nMoved_From_Case<R>")
    assert_equal ['Storage'], problems(STORAGE, "using A = Storage[4];\nMoved_From_Case<A>")
    assert_equal ['Storage'], problems(STORAGE, 'Moved_From_Case<Storage *>')
    assert_empty problems(STORAGE, "using P = Storage;\nMoved_From_Case<P>")
    assert_empty problems(STORAGE, "using C = const Storage;\nMoved_From_Case<C>")
  end

  # A case of a type that takes the class as an argument tests that type,
  # not the class: the moves of Array<Storage> do not run those of Storage.
  # Nor is an alias of, or a class derived from, such a type one of it
  def test_a_case_of_a_type_that_takes_the_class_does_not_test_it
    assert_equal ['Storage'], problems(STORAGE, 'template <> struct Moved_From_Case<Array<Storage>> {};')
    assert_equal ['Storage'], problems(STORAGE, "using Wrapped = Array<Storage>;\nMoved_From_Case<Wrapped>")
    assert_equal ['Storage'], problems(STORAGE, 'Moved_From_Case<Wrapped>', 'using Wrapped = Array<Storage>;')
    assert_equal ['Storage'], problems(STORAGE, "struct D : Array<Storage> {};\nMoved_From_Case<D>")
    assert_empty problems(F_STORAGE, "using Wrapped = F::Storage;\nMoved_From_Case<Wrapped>")
    assert_empty problems(SYNC_STORAGE, "using Ptr = Sync<long>::Storage;\nMoved_From_Case<Ptr>")
    assert_empty problems(F_STORAGE, "struct D : public F::Storage {};\nMoved_From_Case<D>")
  end

  # An alias or a class in a comment or a literal names nothing, and a
  # qualified name is not a base clause
  def test_aliases_and_bases_only_in_code_name_the_class
    assert_equal ['Storage'],
                 problems(STORAGE, "using Alias = Other;\n// using Alias = Storage;\nMoved_From_Case<Alias>")
    assert_equal ['Storage'], problems(STORAGE, 'Moved_From_Case<Alias>', '/* using Alias = Storage; */')
    assert_equal ['Storage'],
                 problems(STORAGE, "const char * s = \"using Alias = Storage;\";\nMoved_From_Case<Alias>")
    assert_equal ['Storage'], problems(STORAGE, "// struct D : Storage {};\nMoved_From_Case<D>")
    assert_equal ['Storage'], problems(STORAGE, 'Moved_From_Case<N>', 'friend class N::Helper<Storage>;')
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
          assert_empty moved_from_failures('origin/master', detail)
          Open3.singleton_class.send(:define_method, :capture2) { |*_a, **_k| ['', nil] }
          assert_equal 1, moved_from_failures('origin/master', detail).size
        ensure
          Open3.singleton_class.send(:define_method, :capture2, old)
        end
      end
    end
  end
end
