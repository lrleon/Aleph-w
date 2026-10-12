#!/usr/bin/env ruby
# frozen_string_literal: true

# Unit tests for the seed-copying step of Tests/run_fuzz.rb.

require 'minitest/autorun'
require 'stringio'
require 'tmpdir'
require_relative 'run_fuzz'

class TestRunFuzz < Minitest::Test
  def silently
    saved = $stdout
    $stdout = StringIO.new
    yield
  ensure
    $stdout = saved
  end

  def test_missing_seed_directory_is_skipped
    Dir.mktmpdir do |dir|
      corpus = File.join(dir, 'corpus')
      FileUtils.mkdir_p(corpus)
      copied = silently { copy_seeds(File.join(dir, 'no_such_seeds'), corpus) }
      assert_equal 0, copied
      assert_empty Dir.children(corpus)
    end
  end

  def test_seeds_are_copied_without_overwriting_saved_inputs
    Dir.mktmpdir do |dir|
      seeds = File.join(dir, 'seeds')
      corpus = File.join(dir, 'corpus')
      FileUtils.mkdir_p([File.join(seeds, 'subdir'), corpus])
      File.write(File.join(seeds, 'a'), 'seed a')
      File.write(File.join(seeds, 'b'), 'seed b')
      File.write(File.join(corpus, 'b'), 'saved b')

      assert_equal 1, copy_seeds(seeds, corpus)
      assert_equal 'seed a', File.read(File.join(corpus, 'a'))
      assert_equal 'saved b', File.read(File.join(corpus, 'b'))
      refute File.exist?(File.join(corpus, 'subdir'))
    end
  end
end
