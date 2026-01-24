#!/usr/bin/env ruby
# frozen_string_literal: true

require 'fileutils'
require 'optparse'
require 'open3'

ROOT_DIR = File.expand_path('..', __dir__)
DEFAULT_BUILD_DIR = File.join(ROOT_DIR, 'build')

def ts
  Time.now.strftime('%Y-%m-%d %H:%M:%S')
end

def log(msg, verbose:)
  return unless verbose
  puts "[#{ts}] #{msg}"
end

def run_command(cmd, chdir: nil, verbose: false, label: nil)
  capture_opts = {}
  capture_opts[:chdir] = chdir if chdir

  display_cmd = cmd.is_a?(Array) ? cmd.inspect : cmd
  header = label ? "#{label}: #{display_cmd}" : display_cmd
  puts "\n>> #{header}"
  started_at = Time.now

  unless verbose
    stdout, stderr, status =
      if cmd.is_a?(Array)
        Open3.capture3(*cmd, **capture_opts)
      else
        Open3.capture3(cmd, **capture_opts)
      end
    return [stdout, stderr, status]
  end

  stdout_buf = +''
  stderr_buf = +''
  status = nil

  popen_args = cmd.is_a?(Array) ? cmd : [cmd]
  Open3.popen3(*popen_args, **capture_opts) do |stdin, stdout, stderr, wait_thr|
    stdin.close

    out_thread = Thread.new do
      stdout.each_line do |line|
        stdout_buf << line
        print line
      end
    end

    err_thread = Thread.new do
      stderr.each_line do |line|
        stderr_buf << line
        warn line
      end
    end

    out_thread.join
    err_thread.join
    status = wait_thr.value
  end

  elapsed = Time.now - started_at
  log("Completed#{label ? " #{label}" : ''} in #{format('%.2f', elapsed)}s (exit=#{status.exitstatus})", verbose: verbose)
  [stdout_buf, stderr_buf, status]
end

options = {
  build_dir: DEFAULT_BUILD_DIR,
  skip_configure: false,
  skip_build: false,
  jobs: nil,
  ctest_args: [],
  gtest_args: [],
  cmake_args: [
    '-DBUILD_EXAMPLES=OFF',
    '-DBUILD_TESTS=ON'
  ],
  compiler: 'g++',
  with_sanitizers: false,
  list_tests: false,
  from_file: nil,
  exact: false,
  verbose: false
}

from_file_targets = nil

OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage: run_one_test.rb [options] TEST

    Runs a single Aleph test via CTest.

    TEST is passed to ctest as a regex (-R). If you want an exact match,
    use --exact.

    Alternatively, use --from-file FILE to run all tests whose CTest name
    starts with the executable derived from FILE (basename without extension).

    Flags:
      -B, --build-dir DIR      Use DIR as the CMake build directory.
      --skip-configure         Assume DIR already contains a CMake cache.
      --skip-build             Run ctest without rebuilding first.
      --ctest-arg ARG          Forward ARG to ctest (repeatable).
      --gtest-arg ARG          Forward ARG to the gtest executable (only with --from-file, repeatable).
      --cmake-arg ARG          Forward ARG to the cmake configure step (repeatable).
      --compiler BIN           Use BIN as the C++ compiler (default: g++).
      --with-examples          Configure with BUILD_EXAMPLES=ON.
      --with-sanitizers        Enable AddressSanitizer + UBSan (uses build-sanitizers dir).
      --sanitizer              Alias for --with-sanitizers.
      --list-tests             List tests in the build dir (ctest -N) and exit.
      --from-file FILE         Run all tests from a given test source file.
      --exact                  Anchor TEST regex to match exactly.
      -j, --jobs N             Parallel build jobs (passed to cmake --build --parallel).
      --verbose                Stream command output in real time.
  USAGE

  opts.on('-B DIR', '--build-dir DIR', 'CMake build directory (default: ../build)') do |dir|
    options[:build_dir] = File.expand_path(dir)
  end
  opts.on('--skip-configure', 'Assume the build directory already has a CMake cache') do
    options[:skip_configure] = true
  end
  opts.on('--skip-build', 'Skip rebuilding tests before running them') do
    options[:skip_build] = true
  end
  opts.on('-j N', '--jobs N', Integer, 'Parallel build jobs (passed to cmake --build --parallel)') do |n|
    options[:jobs] = n
  end
  opts.on('--ctest-arg ARG', 'Forward an extra argument to ctest (repeatable)') do |arg|
    options[:ctest_args] << arg
  end
  opts.on('--gtest-arg ARG', 'Forward an extra argument to the gtest executable (repeatable; only with --from-file)') do |arg|
    options[:gtest_args] << arg
  end
  opts.on('--cmake-arg ARG', 'Forward an extra argument to cmake configure step (repeatable)') do |arg|
    options[:cmake_args] << arg
  end
  opts.on('--with-examples', 'Also build Examples (defaults to OFF for faster test runs)') do
    options[:cmake_args].reject! { |arg| arg.start_with?('-DBUILD_EXAMPLES=') }
    options[:cmake_args] << '-DBUILD_EXAMPLES=ON'
  end
  opts.on('--compiler BIN', 'C++ compiler to pass as CMAKE_CXX_COMPILER (default: g++)') do |bin|
    options[:compiler] = bin
  end
  opts.on('--with-sanitizers', 'Enable AddressSanitizer and UndefinedBehaviorSanitizer') do
    options[:with_sanitizers] = true
  end
  opts.on('--sanitizer', 'Alias for --with-sanitizers') do
    options[:with_sanitizers] = true
  end
  opts.on('--list-tests', 'List tests in the build directory (ctest -N) and exit') do
    options[:list_tests] = true
  end
  opts.on('--from-file FILE', 'Run all tests from a given test source file') do |file|
    options[:from_file] = file
  end
  opts.on('--exact', 'Match TEST exactly (wraps regex with ^...$)') do
    options[:exact] = true
  end
  opts.on('--verbose', 'Print timestamps and stream command output in real time') do
    options[:verbose] = true
  end
end.parse!

test_pattern = ARGV.shift

# Convenience: allow passing a test source filename directly as TEST.
# CTest filters (-R) match registered test names, not source file names.
if !test_pattern.nil? && !options[:from_file]
  # Search in multiple locations: cwd, Tests/, and ROOT_DIR
  search_paths = [
    File.expand_path(test_pattern),
    File.join(ROOT_DIR, 'Tests', test_pattern),
    File.join(ROOT_DIR, test_pattern)
  ]
  found_file = search_paths.find { |p| File.exist?(p) && File.file?(p) }
  if found_file && found_file.match?(/\.(c|cc|cpp|cxx)\z/i)
    # Store absolute path so downstream code doesn't re-expand incorrectly
    options[:from_file] = File.expand_path(found_file)
    test_pattern = nil
  end
end

if options[:from_file] && test_pattern
  abort 'Use either TEST or --from-file FILE, not both'
end

if !options[:list_tests] && !options[:from_file] && (test_pattern.nil? || test_pattern.strip.empty?)
  abort 'Missing TEST argument. Example: run_one_test.rb my_test_name'
end

build_dir = options[:build_dir]
ctest_args = options[:ctest_args]
gtest_args = options[:gtest_args]
cmake_args = options[:cmake_args]
jobs = options[:jobs]
verbose = options[:verbose]

unless cmake_args.any? { |arg| arg.start_with?('-DCMAKE_CXX_COMPILER=') }
  cmake_args << "-DCMAKE_CXX_COMPILER=#{options[:compiler]}"
end

if options[:from_file]
  file = options[:from_file]
  file = File.expand_path(file, ROOT_DIR) unless file.start_with?('/')
  unless File.exist?(file)
    abort "File not found: #{options[:from_file]}"
  end

  base = File.basename(file)
  base = base.sub(/\.[^.]+\z/, '')
  if base.strip.empty?
    abort "Could not derive test target name from file: #{options[:from_file]}"
  end

  base_dash = base.tr('_', '-')
  base_us = base.tr('-', '_')
  candidates = [base, base_dash, base_us].uniq
  escaped_candidates = candidates.map { |c| Regexp.escape(c) }
  alts = escaped_candidates.length == 1 ? escaped_candidates.first : "(#{escaped_candidates.join('|')})"
  test_pattern = "^(tests_)?#{alts}(\\.|$)"

  from_file_targets = candidates.flat_map { |c| [c, "tests_#{c}"] }.uniq
else
  if options[:exact]
    test_pattern = "^#{test_pattern}$"
  end
end

# Add sanitizer flags if requested
if options[:with_sanitizers]
  cmake_args << '-DUSE_SANITIZERS=ON'
  cmake_args << '-DCMAKE_BUILD_TYPE=Debug'
  build_dir = File.join(ROOT_DIR, 'build-sanitizers') unless options[:build_dir] != DEFAULT_BUILD_DIR
  ENV['ASAN_OPTIONS'] = 'detect_leaks=1:halt_on_error=1'
  ENV['UBSAN_OPTIONS'] = 'print_stacktrace=1'
  puts 'Running with AddressSanitizer and UndefinedBehaviorSanitizer enabled'
end

cmake_cache = File.join(build_dir, 'CMakeCache.txt')

unless options[:skip_configure]
  FileUtils.mkdir_p(build_dir) unless File.exist?(build_dir)

  configure_cmd = ['cmake', '-S', ROOT_DIR, '-B', build_dir, *cmake_args]
  label = File.exist?(cmake_cache) ? 'reconfigure' : 'configure'
  log("#{label.capitalize}...", verbose: verbose)
  stdout, stderr, status = run_command(configure_cmd, verbose: verbose, label: label)
  unless status.success?
    warn stdout
    warn stderr
    abort "CMake #{label} failed"
  end
end

unless options[:skip_build]
  log('Building...', verbose: verbose)
  build_cmd = ['cmake', '--build', '.']
  build_cmd += ['--parallel', jobs.to_s] if jobs
  stdout, stderr, status = run_command(build_cmd, chdir: build_dir, verbose: verbose, label: 'build')
  unless status.success?
    warn stdout
    warn stderr
    abort 'Build failed'
  end
end

if options[:from_file]
  # Ensure the executable target corresponding to the source file is built.
  # This makes --from-file usable even when users pass --skip-build.
  built = false
  built_target = nil
  from_file_targets.each do |t|
    log("Building target #{t}...", verbose: verbose)
    target_cmd = ['cmake', '--build', '.', '--target', t]
    target_cmd += ['--parallel', jobs.to_s] if jobs
    stdout, stderr, status = run_command(target_cmd, chdir: build_dir, verbose: verbose, label: 'build-target')
    if status.success?
      built = true
      built_target = t
      break
    end
  end

  unless built
    abort "Could not build a target for #{options[:from_file]}. Tried: #{from_file_targets.join(', ')}"
  end

  if built_target.nil?
    abort 'Internal error: built_target not set'
  end

  # Prefer running the test executable directly: gtest_discover_tests() may
  # register tests without the executable name as a prefix, making ctest -R
  # filtering unreliable for mapping FILE -> tests.
  exe_candidates = [
    File.join(build_dir, 'Tests', built_target),
    File.join(build_dir, built_target)
  ]
  exe_path = exe_candidates.find { |p| File.exist?(p) && File.file?(p) }
  unless exe_path
    abort "Built target '#{built_target}' but could not locate its executable. Tried: #{exe_candidates.join(', ')}"
  end

  cmd = [exe_path, *gtest_args]
  log("Running executable #{exe_path}...", verbose: verbose)
  stdout, stderr, status = run_command(cmd, chdir: build_dir, verbose: verbose, label: 'gtest')
  puts stdout unless verbose
  warn stderr if !verbose && stderr && !stderr.empty?
  abort 'Test executable failed' unless status.success?

  puts "\nTest completed successfully."
  exit 0
end

if options[:list_tests]
  ctest_list_cmd = ['ctest', '-N']
  ctest_list_cmd += ctest_args
  log('Listing tests (ctest -N)...', verbose: verbose)
  stdout, stderr, status = run_command(ctest_list_cmd, chdir: build_dir, verbose: verbose, label: 'ctest')
  puts stdout unless verbose
  warn stderr if !verbose && stderr && !stderr.empty?
  abort 'Failed to list tests' unless status.success?
  exit 0
end

ctest_cmd = ['ctest', '--output-on-failure', '-R', test_pattern]
ctest_cmd << '--progress' if verbose
ctest_cmd += ctest_args
log("Running ctest -R #{test_pattern}...", verbose: verbose)
stdout, stderr, status = run_command(ctest_cmd, chdir: build_dir, verbose: verbose, label: 'ctest')
puts stdout unless verbose
warn stderr if !verbose && stderr && !stderr.empty?

combined = "#{stdout}\n#{stderr}"
if combined.match?(/No tests were found/i)
  abort "No tests matched (or no tests are registered in #{build_dir}). Try: ./run_one_test.rb --list-tests"
end

abort 'Test failed' unless status.success?

puts "\nTest completed successfully."
