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

  header = label ? "#{label}: #{cmd}" : cmd
  puts "\n>> #{header}"
  started_at = Time.now

  unless verbose
    stdout, stderr, status = Open3.capture3(cmd, **capture_opts)
    return [stdout, stderr, status]
  end

  stdout_buf = +''
  stderr_buf = +''
  status = nil

  Open3.popen3(cmd, **capture_opts) do |stdin, stdout, stderr, wait_thr|
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

def extract_failed_tests_from_ctest_log(log_text)
  failed = []
  log_text.each_line do |line|
    # Typical CTest format: "2019:SmallDomain.validations" (may vary by CMake version)
    if (m = line.match(/^\s*\d+\s*:(.+?)\s*$/))
      name = m[1].strip
      failed << name unless name.empty?
      next
    end
    # Fallback: match summary-style lines if they appear in the log
    if (m = line.match(/^\s*\d+\s*-\s*(.+?)\s*$/))
      name = m[1].strip
      failed << name unless name.empty?
    end
  end
  failed.uniq
end

options = {
  build_dir: DEFAULT_BUILD_DIR,
  skip_configure: false,
  skip_build: false,
  ctest_args: [],
  cmake_args: [
    '-DBUILD_EXAMPLES=OFF',
    '-DBUILD_TESTS=ON'
  ],
  compiler: 'g++',
  with_sanitizers: false,
  verbose: false
}

OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage: run_all_tests.rb [options]

    Runs the full Aleph test suite via CTest. By default it configures
    (if needed), builds, and executes all tests in ../build.

    Flags:
      -B, --build-dir DIR      Use DIR as the CMake build directory.
      --skip-configure         Assume DIR already contains a CMake cache.
      --skip-build             Run ctest without rebuilding first.
      --ctest-arg ARG          Forward ARG to every ctest invocation (repeatable).
      --cmake-arg ARG          Forward ARG to the cmake configure step (repeatable).
      --compiler BIN           Use BIN as the C++ compiler (default: g++).
      --with-examples          Configure with BUILD_EXAMPLES=ON (tests only needs OFF).
      --with-sanitizers        Enable AddressSanitizer + UBSan (uses build-sanitizers dir).
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
  opts.on('--ctest-arg ARG', 'Forward an extra argument to ctest (repeatable)') do |arg|
    options[:ctest_args] << arg
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

  opts.on('--verbose', 'Print timestamps and stream command output in real time') do
    options[:verbose] = true
  end
end.parse!

build_dir = options[:build_dir]
ctest_args = options[:ctest_args]
cmake_args = options[:cmake_args]
verbose = options[:verbose]
unless cmake_args.any? { |arg| arg.start_with?('-DCMAKE_CXX_COMPILER=') }
  cmake_args << "-DCMAKE_CXX_COMPILER=#{options[:compiler]}"
end

# Add sanitizer flags if requested
if options[:with_sanitizers]
  cmake_args << '-DUSE_SANITIZERS=ON'
  cmake_args << '-DCMAKE_BUILD_TYPE=Debug'
  # Use a separate build directory for sanitizer builds
  build_dir = File.join(ROOT_DIR, 'build-sanitizers') unless options[:build_dir] != DEFAULT_BUILD_DIR
  ENV['ASAN_OPTIONS'] = 'detect_leaks=1:halt_on_error=1'
  ENV['UBSAN_OPTIONS'] = 'print_stacktrace=1'
  puts "Running with AddressSanitizer and UndefinedBehaviorSanitizer enabled"
end

cmake_cache = File.join(build_dir, 'CMakeCache.txt')

unless options[:skip_configure]
  unless File.exist?(cmake_cache)
    FileUtils.mkdir_p(build_dir)
    configure_cmd = ['cmake', "-S #{ROOT_DIR}", "-B #{build_dir}", *cmake_args].join(' ')
    log('Configuring (first time)...', verbose: verbose)
    stdout, stderr, status = run_command(configure_cmd, verbose: verbose, label: 'configure')
    unless status.success?
      warn stdout
      warn stderr
      abort 'CMake configuration failed'
    end
  else
    # Refresh cache to ensure flags like BUILD_EXAMPLES stay in sync.
    configure_cmd = ['cmake', "-S #{ROOT_DIR}", "-B #{build_dir}", *cmake_args].join(' ')
    log('Re-configuring...', verbose: verbose)
    stdout, stderr, status = run_command(configure_cmd, verbose: verbose, label: 'reconfigure')
    unless status.success?
      warn stdout
      warn stderr
      abort 'CMake re-configuration failed'
    end
  end
end

unless options[:skip_build]
  log('Building...', verbose: verbose)
  stdout, stderr, status = run_command('cmake --build .', chdir: build_dir, verbose: verbose, label: 'build')
  unless status.success?
    warn stdout
    warn stderr
    abort 'Build failed'
  end
end

ctest_cmd = ['ctest', '--output-on-failure']
ctest_cmd << '--progress' if verbose
ctest_cmd += ctest_args
log('Running ctest...', verbose: verbose)
stdout, stderr, status = run_command(ctest_cmd.join(' '), chdir: build_dir, verbose: verbose, label: 'ctest')
puts stdout unless verbose
warn stderr if !verbose && stderr && !stderr.empty?

unless status.success?
  last_failed_path = File.join(build_dir, 'Testing', 'Temporary', 'LastTestsFailed.log')
  if File.exist?(last_failed_path)
    last_failed = File.read(last_failed_path)
    puts "\n>> Failed tests (from #{last_failed_path}):\n#{last_failed}"

    failed_tests = extract_failed_tests_from_ctest_log(last_failed)
    if failed_tests.any?
      puts "\n>> Re-running failed tests with extra verbosity (ctest -VV --output-on-failure)..."
      failed_tests.each do |t|
        # Anchor the regex to match the test name exactly.
        pattern = "^#{Regexp.escape(t)}$"
        rerun_cmd = "ctest -VV --output-on-failure -R '#{pattern}'"
        run_command(rerun_cmd, chdir: build_dir, verbose: true, label: "rerun-failed")
      end
    end
  end

  abort 'Some tests failed'
end

puts '\nAll tests completed successfully.'
