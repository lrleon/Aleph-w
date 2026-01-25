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
  headers = files.select { |f| HEADER_EXTS.include?(File.extname(f)) }

  # Only analyze library files (exclude Tests/ and Examples/)
  sources = sources.reject { |f| f.start_with?('Tests/', 'Examples/') }
  headers = headers.reject { |f| f.start_with?('Tests/', 'Examples/') }

  all_files = sources + headers

  if all_files.empty?
    puts '[ok] no changed library C/C++ source or header files'
    return 0
  end

  build_dir = Pathname.new(ENV.fetch('TIDY_BUILD_DIR', 'build-tidy'))

  cmake_args = [
    'cmake', '-S', '.', '-B', build_dir.to_s, '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=Debug',
    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    '-DBUILD_EXAMPLES=ON',
    '-DBUILD_TESTS=ON',
    '-DCMAKE_C_COMPILER=clang',
    '-DCMAKE_CXX_COMPILER=clang++'
  ]

  run!(cmake_args)

  failures = 0
  all_files.each do |file|
    next unless File.exist?(file)

    puts "[info] clang-tidy: #{file}"

    # For headers, we need to provide compilation flags since they may not be in compile_commands.json
    if HEADER_EXTS.include?(File.extname(file))
      # Add extra flags for header-only files
      extra_args = [
        '--extra-arg=-std=c++20',
        '--extra-arg=-I.',
        '--extra-arg=-Wno-unknown-warning-option'
      ]
      cmd = ['clang-tidy', '-p', build_dir.to_s] + extra_args + [file, '--']
    else
      cmd = ['clang-tidy', '-p', build_dir.to_s, file, '--']
    end

    begin
      run!(cmd)
    rescue StandardError => e
      failures += 1
      warn "[fail] clang-tidy failed for #{file}: #{e.message.lines.first.strip}"
    end
  end

  return 1 if failures > 0

  puts "[ok] clang-tidy passed for #{all_files.size} changed file(s) (#{sources.size} source, #{headers.size} header)"
  0
end


exit(main)
