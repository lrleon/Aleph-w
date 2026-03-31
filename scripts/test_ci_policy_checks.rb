#!/usr/bin/env ruby

require 'minitest/autorun'
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

  private

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
