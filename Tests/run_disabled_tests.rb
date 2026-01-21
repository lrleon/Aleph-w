#!/usr/bin/env ruby
# frozen_string_literal: true

require 'fileutils'
require 'json'
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

  [stdout_buf, stderr_buf, status]
end

def property_present?(test_obj, prop_name)
  props = test_obj['properties']
  return false unless props.is_a?(Array)
  props.any? { |p| p.is_a?(Hash) && p['name'] == prop_name }
end

def property_value(test_obj, prop_name)
  props = test_obj['properties']
  return nil unless props.is_a?(Array)
  p = props.find { |x| x.is_a?(Hash) && x['name'] == prop_name }
  return nil unless p
  v = p['value']
  return nil unless v

  if v.is_a?(Array)
    v.join(' ')
  else
    v.to_s
  end
end

options = {
  build_dir: DEFAULT_BUILD_DIR,
  skip_configure: false,
  skip_build: false,
  jobs: nil,
  ctest_args: [],
  cmake_args: [
    '-DBUILD_EXAMPLES=OFF',
    '-DBUILD_TESTS=ON'
  ],
  compiler: 'g++',
  verbose: false,
  only_disabled: false,
  only_skipped: false,
  include_skipped: false,
  list_only: false,
  name_regex: nil
}

OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage: run_disabled_tests.rb [options]

    Runs tests that are marked DISABLED (and optionally tests that have
    SKIP_REGULAR_EXPRESSION) by executing their underlying command directly.

    Notes:
      - CTest does not run DISABLED tests via normal ctest invocations.
      - This script uses `ctest --show-only=json-v1` to discover tests and their commands.

    Flags:
      -B, --build-dir DIR      Use DIR as the CMake build directory.
      --skip-configure         Assume DIR already contains a CMake cache.
      --skip-build             Do not build before running.
      -j, --jobs N             Parallel build jobs (passed to cmake --build --parallel).
      --ctest-arg ARG          Forward ARG to the ctest discovery step (repeatable).
      --cmake-arg ARG          Forward ARG to the cmake configure step (repeatable).
      --compiler BIN           Use BIN as the C++ compiler (default: g++).
      --only-disabled          Run only DISABLED tests.
      --include-skipped        Also include tests with SKIP_REGULAR_EXPRESSION.
      --only-skipped           Run only tests with SKIP_REGULAR_EXPRESSION.
      --list                   List selected tests and exit.
      -R, --regex REGEX        Filter discovered tests by name (Ruby regexp).
      --verbose                Stream command output in real time.
  USAGE

  opts.on('-B DIR', '--build-dir DIR', 'CMake build directory (default: ../build)') do |dir|
    options[:build_dir] = File.expand_path(dir)
  end
  opts.on('--skip-configure', 'Assume the build directory already has a CMake cache') do
    options[:skip_configure] = true
  end
  opts.on('--skip-build', 'Skip building tests before running them') do
    options[:skip_build] = true
  end
  opts.on('-j N', '--jobs N', Integer, 'Parallel build jobs (passed to cmake --build --parallel)') do |n|
    options[:jobs] = n
  end
  opts.on('--ctest-arg ARG', 'Forward an extra argument to ctest discovery step (repeatable)') do |arg|
    options[:ctest_args] << arg
  end
  opts.on('--cmake-arg ARG', 'Forward an extra argument to cmake configure step (repeatable)') do |arg|
    options[:cmake_args] << arg
  end
  opts.on('--compiler BIN', 'C++ compiler to pass as CMAKE_CXX_COMPILER (default: g++)') do |bin|
    options[:compiler] = bin
  end
  opts.on('--only-disabled', 'Run only DISABLED tests') do
    options[:only_disabled] = true
  end
  opts.on('--include-skipped', 'Also include tests with SKIP_REGULAR_EXPRESSION') do
    options[:include_skipped] = true
  end
  opts.on('--only-skipped', 'Run only tests with SKIP_REGULAR_EXPRESSION') do
    options[:only_skipped] = true
  end
  opts.on('--list', 'List selected tests and exit') do
    options[:list_only] = true
  end
  opts.on('-R REGEX', '--regex REGEX', 'Filter tests by name (Ruby regexp)') do |re|
    options[:name_regex] = re
  end
  opts.on('--verbose', 'Stream command output in real time') do
    options[:verbose] = true
  end
end.parse!

if options[:only_disabled] && options[:only_skipped]
  abort 'Use either --only-disabled or --only-skipped, not both'
end

if options[:only_skipped] && options[:include_skipped]
  abort 'Use either --only-skipped or --include-skipped, not both'
end

build_dir = options[:build_dir]
ctest_args = options[:ctest_args]
cmake_args = options[:cmake_args]
jobs = options[:jobs]
verbose = options[:verbose]

unless cmake_args.any? { |arg| arg.start_with?('-DCMAKE_CXX_COMPILER=') }
  cmake_args << "-DCMAKE_CXX_COMPILER=#{options[:compiler]}"
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

show_cmd = ['ctest', '--show-only=json-v1']
show_cmd += ctest_args
stdout, stderr, status = run_command(show_cmd, chdir: build_dir, verbose: false, label: 'discover')
unless status.success?
  warn stdout
  warn stderr
  abort 'CTest discovery failed'
end

begin
  data = JSON.parse(stdout)
rescue JSON::ParserError => e
  warn stdout
  warn stderr
  abort "Failed to parse ctest json-v1 output: #{e.message}"
end

tests = data['tests']
unless tests.is_a?(Array)
  abort 'CTest json-v1 output did not contain a tests array'
end

name_re = options[:name_regex] ? Regexp.new(options[:name_regex]) : nil

selected = tests.select do |t|
  next false unless t.is_a?(Hash)

  name = t['name'].to_s
  next false if name_re && !name.match?(name_re)

  disabled = property_present?(t, 'DISABLED')
  skippable = property_present?(t, 'SKIP_REGULAR_EXPRESSION')

  if options[:only_disabled]
    disabled
  elsif options[:only_skipped]
    skippable
  elsif options[:include_skipped]
    disabled || skippable
  else
    disabled
  end
end

if selected.empty?
  puts 'No matching tests found.'
  exit 0
end

if options[:list_only]
  selected.each do |t|
    tags = []
    tags << 'DISABLED' if property_present?(t, 'DISABLED')
    tags << 'SKIPPABLE' if property_present?(t, 'SKIP_REGULAR_EXPRESSION')
    puts "#{t['name']} [#{tags.join(', ')}]"
  end
  exit 0
end

failures = []
selected.each_with_index do |t, idx|
  name = t['name'].to_s
  cmd = t['command']

  unless cmd.is_a?(Array) && !cmd.empty?
    failures << name
    warn "Skipping #{name}: missing command"
    next
  end

  workdir = property_value(t, 'WORKING_DIRECTORY')
  workdir = build_dir if workdir.nil? || workdir.strip.empty?

  log("[#{idx + 1}/#{selected.size}] Running #{name}...", verbose: verbose)
  out, err, st = run_command(cmd, chdir: workdir, verbose: verbose, label: name)
  puts out unless verbose
  warn err if !verbose && err && !err.empty?

  failures << name unless st.success?
end

if failures.any?
  warn "\nSome selected tests failed (#{failures.size}/#{selected.size}):"
  failures.each { |n| warn "  - #{n}" }
  exit 1
end

puts "\nAll selected tests completed successfully."
