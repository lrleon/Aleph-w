#!/usr/bin/env ruby

require 'minitest/autorun'
require 'set'
require 'tmpdir'
require_relative 'ci_header_doc_coverage'
require_relative 'fixtures/header_doc_cases'

class TestCiHeaderDocCoverage < Minitest::Test
  MATRIX_CATEGORIES = ['layout x documentation x context', 'touches the signature',
                       'does not touch the signature', 'kind of declaration', 'not a declaration',
                       'found in real code'].freeze

  # The failed cases of the matrix, with one run of Doxygen for all of them
  def self.matrix_failures
    @matrix_failures ||= Dir.mktmpdir do |dir|
      cases = HeaderDocCases.all
      by_text = {}
      paths = cases.map do |c|
        by_text[c.text] ||= File.join(dir, "case#{by_text.size}.H").tap { |p| File.write(p, c.text) }
      end
      doxygen = DoxygenDeclarations.read(by_text.values)
      cases.each_with_index.filter_map do |c, i|
        added = c.added == :all ? (1..c.text.lines.size).to_set : c.added.to_set
        documented, undocumented = parse_changed_public_declarations(
          paths[i], added, doxygen.fetch(DoxygenDeclarations.key(paths[i]), [])
        )
        got = counted(documented, true) + counted(undocumented, false)
        next if got.sort_by(&:to_s) == c.expected.sort_by(&:to_s)

        "#{c.category}\n  expected #{c.expected.sort_by(&:to_s)}\n  got      #{got.sort_by(&:to_s)}"
      end
    end
  end

  # [group, name, documented?] of each declaration but the wrapper classes
  def self.counted(declarations, documented)
    declarations.reject { |d| d.name.start_with?('W_') }.map do |d|
      [CppDeclarations::TYPE_KINDS.include?(d.kind) ? :type : :fn, d.name.delete(' '), documented]
    end
  end

  MATRIX_CATEGORIES.each do |category|
    define_method("test_matrix_#{category.tr(' ', '_')}") do
      assert HeaderDocCases.all.any? { |c| c.category.start_with?("#{category}:") }, 'no case'
      failed = self.class.matrix_failures.select { |f| f.start_with?("#{category}:") }
      assert failed.empty?, "#{failed.size} cases fail:\n#{failed.first(5).join("\n")}"
    end
  end

  def test_every_case_of_the_matrix_is_in_a_tested_category
    others = HeaderDocCases.all.reject do |c|
      MATRIX_CATEGORIES.any? { |category| c.category.start_with?("#{category}:") }
    end
    assert_empty others.map(&:category)
  end

  def test_a_header_that_doxygen_does_not_report_fails
    Dir.mktmpdir do |dir|
      header = File.join(dir, 'x.H')
      File.write(header, "int f();\n")
      error = assert_raises(RuntimeError) { DoxygenDeclarations.read([header], 'GENERATE_XML' => 'NO') }
      assert_match(/did not report 1 headers/, error.message)
    end
  end

  def test_a_missing_doxygen_is_not_silently_ignored
    path = ENV.fetch('PATH')
    Dir.mktmpdir do |dir|
      header = File.join(dir, 'x.H')
      File.write(header, "int f();\n")
      ENV['PATH'] = dir
      assert_raises(SystemCallError) { DoxygenDeclarations.read([header]) }
    end
  ensure
    ENV['PATH'] = path
  end

  def test_nested_template_class_documentation_is_checked
    documented, undocumented = parse_header(<<~CPP)
      /** @brief A documented class. */
      template <typename T = Box<Box<int>>> class C
      {
      };
      template <typename T = Box<Box<int>>> struct D
      {
      };
      /** @brief A documented specialization. */
      template <> struct hash<C<int>>
      {
      };
    CPP

    assert_equal %w[C hash], documented.map(&:name)
    assert_equal ['D'], undocumented.map(&:name)
  end

  def test_extern_c_declarations_are_checked_without_counting_function_bodies
    documented, undocumented = parse_header(<<~CPP)
      extern "C" {
        /** @brief A documented API. */
        int api();
        int missing();
        /** @brief A documented wrapper. */
        inline int wrapper() {
          std::abort();
          return 0;
        }
      }
    CPP

    assert_equal %w[api wrapper], documented.map(&:name)
    assert_equal ['missing'], undocumented.map(&:name)
  end

  private

  def parse_header(content)
    Dir.mktmpdir do |dir|
      path = File.join(dir, 'coverage.H')
      File.write(path, content)
      parse_changed_public_declarations(path, Set.new(1..content.lines.size))
    end
  end
end
