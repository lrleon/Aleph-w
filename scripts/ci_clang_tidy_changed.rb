#!/usr/bin/env ruby

require 'open3'
require 'pathname'

SOURCE_EXTS = %w[.c .C .cc .cpp .cxx].freeze
HEADER_EXTS = %w[.h .H .hh .hpp .hxx].freeze


def run!(cmd, env: {})
  out, err, status = Open3.capture3(env, *cmd)
  raise "command failed: #{cmd.join(' ')}\n#{err.strip}" unless status.success?
  out
end


def github_event_name
  ENV.fetch('GITHUB_EVENT_NAME', '')
end


def base_ref
  ENV['GITHUB_BASE_REF']
end


def changed_files(base)
  return [] if base.nil? || base.empty?

  run!(['git', 'fetch', '--no-tags', '--prune', 'origin', "#{base}:#{base}"])
  out = run!(['git', 'diff', '--name-only', '--diff-filter=ACMR', "origin/#{base}...HEAD"])
  out.lines.map(&:strip).reject(&:empty?)
end


def main
  if github_event_name != 'pull_request'
    puts '[skip] not a pull_request event'
    return 0
  end

  base = base_ref
  files = changed_files(base)
  sources = files.select { |f| SOURCE_EXTS.include?(File.extname(f)) }

  # Only analyze Tests/ files (library files are mostly header-only and difficult to configure)
  test_sources = sources.select { |f| f.start_with?('Tests/') }

  if test_sources.empty?
    puts '[ok] no changed test source files (library files are skipped)'
    return 0
  end

  build_dir = Pathname.new(ENV.fetch('TIDY_BUILD_DIR', 'build-tidy'))

  cmake_args = [
    'cmake', '-S', '.', '-B', build_dir.to_s, '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=Debug',
    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    '-DBUILD_EXAMPLES=OFF',
    '-DBUILD_TESTS=ON',
    '-DCMAKE_C_COMPILER=clang',
    '-DCMAKE_CXX_COMPILER=clang++'
  ]

  run!(cmake_args)

  failures = 0
  test_sources.each do |file|
    next unless File.exist?(file)

    puts "[info] clang-tidy: #{file}"

    begin
      run!(['clang-tidy', '-p', build_dir.to_s, file, '--'])
    rescue StandardError => e
      failures += 1
      warn "[fail] clang-tidy failed for #{file}: #{e.message.lines.first.strip}"
    end
  end

  return 1 if failures > 0

  puts "[ok] clang-tidy passed for #{test_sources.size} changed test file(s)"
  0
end


exit(main)
