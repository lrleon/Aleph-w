#!/usr/bin/env ruby
# frozen_string_literal: true

require 'fileutils'
require 'optparse'
require 'open3'

ROOT_DIR = File.expand_path('..', __dir__)
DEFAULT_BUILD_DIR = File.join(ROOT_DIR, 'build')

def run_command(cmd, chdir: nil)
  puts "\n>> #{cmd}"
  capture_opts = {}
  capture_opts[:chdir] = chdir if chdir
  stdout, stderr, status = Open3.capture3(cmd, **capture_opts)
  [stdout, stderr, status]
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
  with_sanitizers: false
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
end.parse!

build_dir = options[:build_dir]
ctest_args = options[:ctest_args]
cmake_args = options[:cmake_args]
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
    stdout, stderr, status = run_command(configure_cmd)
    unless status.success?
      warn stdout
      warn stderr
      abort 'CMake configuration failed'
    end
  else
    # Refresh cache to ensure flags like BUILD_EXAMPLES stay in sync.
    configure_cmd = ['cmake', "-S #{ROOT_DIR}", "-B #{build_dir}", *cmake_args].join(' ')
    stdout, stderr, status = run_command(configure_cmd)
    unless status.success?
      warn stdout
      warn stderr
      abort 'CMake re-configuration failed'
    end
  end
end

unless options[:skip_build]
  stdout, stderr, status = run_command('cmake --build .', chdir: build_dir)
  unless status.success?
    warn stdout
    warn stderr
    abort 'Build failed'
  end
end

ctest_cmd = ['ctest', '--output-on-failure'] + ctest_args
stdout, stderr, status = run_command(ctest_cmd.join(' '), chdir: build_dir)
puts stdout
warn stderr if stderr && !stderr.empty?
abort 'Some tests failed' unless status.success?

puts '\nAll tests completed successfully.'
