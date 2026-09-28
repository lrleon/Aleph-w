#!/usr/bin/env ruby

require 'minitest/autorun'
require 'set'
require 'tmpdir'
require_relative 'ci_header_doc_coverage'

class TestCiHeaderDocCoverage < Minitest::Test
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
