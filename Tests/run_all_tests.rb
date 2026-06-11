#!/usr/bin/env ruby
# frozen_string_literal: true

require 'fileutils'
require 'etc'
require 'optparse'
require 'open3'
require 'shellwords'

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

  cmd_display = cmd.is_a?(Array) ? Shellwords.join(cmd) : cmd
  header = label ? "#{label}: #{cmd_display}" : cmd_display
  puts "\n>> #{header}"
  started_at = Time.now

  unless verbose
    if cmd.is_a?(Array)
      stdout, stderr, status = Open3.capture3(*cmd, **capture_opts)
    else
      stdout, stderr, status = Open3.capture3(cmd, **capture_opts)
    end
    return [stdout, stderr, status]
  end

  stdout_buf = +''
  stderr_buf = +''
  status = nil

  runner = cmd.is_a?(Array) ? [*cmd] : [cmd]
  Open3.popen3(*runner, **capture_opts) do |stdin, stdout, stderr, wait_thr|
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

def count_cpu_list(cpu_list)
  total = 0

  cpu_list.to_s.strip.split(',').each do |part|
    next if part.empty?

    if (m = part.match(/\A(\d+)-(\d+)\z/))
      first = m[1].to_i
      last = m[2].to_i
      return nil if last < first

      total += last - first + 1
    elsif part.match?(/\A\d+\z/)
      total += 1
    else
      return nil
    end
  end

  total.positive? ? total : nil
end

def cpu_quota_count
  if File.readable?('/sys/fs/cgroup/cpu.max')
    quota, period = File.read('/sys/fs/cgroup/cpu.max').split
    return nil unless quota and period
    return nil if quota == 'max'

    quota = quota.to_i
    period = period.to_i
    return nil unless quota.positive? and period.positive?

    return [(quota / period.to_f).floor, 1].max
  end

  cgroup_v1_paths = [
    ['/sys/fs/cgroup/cpu/cpu.cfs_quota_us',
     '/sys/fs/cgroup/cpu/cpu.cfs_period_us'],
    ['/sys/fs/cgroup/cpu,cpuacct/cpu.cfs_quota_us',
     '/sys/fs/cgroup/cpu,cpuacct/cpu.cfs_period_us']
  ]

  cgroup_v1_paths.each do |quota_path, period_path|
    next unless File.readable?(quota_path) and File.readable?(period_path)

    quota = File.read(quota_path).to_i
    period = File.read(period_path).to_i
    next unless quota.positive? and period.positive?

    return [(quota / period.to_f).floor, 1].max
  end

  nil
end

def available_cpu_count
  candidates = []
  candidates << Etc.nprocessors if Etc.respond_to?(:nprocessors)

  if File.readable?('/proc/self/status')
    status = File.read('/proc/self/status')
    if (m = status.match(/^Cpus_allowed_list:\s*(.+)$/))
      candidates << count_cpu_list(m[1])
    end
  end

  [
    '/sys/fs/cgroup/cpuset.cpus.effective',
    '/sys/fs/cgroup/cpuset.cpus',
    '/sys/fs/cgroup/cpuset/cpuset.cpus',
    '/sys/fs/cgroup/cpuset/cpuset.effective_cpus'
  ].each do |path|
    next unless File.readable?(path)

    candidates << count_cpu_list(File.read(path))
  end

  candidates << cpu_quota_count
  candidates.compact.select(&:positive?).min || 1
rescue StandardError
  1
end

def ctest_parallel_requested?(args)
  args.any? do |arg|
    arg == '--parallel' or
      arg == '-j' or
      arg.start_with?('--parallel=') or
      arg.match?(/\A-j\d+\z/)
  end
end

options = {
  build_dir: DEFAULT_BUILD_DIR,
  skip_configure: false,
  skip_build: false,
  jobs: nil,
  test_jobs: nil,
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
      -j, --jobs N             Parallel build jobs (passed to cmake --build --parallel).
      --test-jobs N            Parallel ctest jobs (default: available CPUs).
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
  opts.on('-j N', '--jobs N', Integer, 'Parallel build jobs (passed to cmake --build --parallel)') do |n|
    options[:jobs] = n
  end
  opts.on('--test-jobs N', Integer, 'Parallel ctest jobs (default: available CPUs)') do |n|
    options[:test_jobs] = n
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
jobs = options[:jobs]
test_jobs = options[:test_jobs] || available_cpu_count
verbose = options[:verbose]
abort '--test-jobs must be positive' unless test_jobs.positive?
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
    configure_cmd = ['cmake', '-S', ROOT_DIR, '-B', build_dir, *cmake_args]
    log('Configuring (first time)...', verbose: verbose)
    stdout, stderr, status = run_command(configure_cmd, verbose: verbose, label: 'configure')
    unless status.success?
      warn stdout
      warn stderr
      abort 'CMake configuration failed'
    end
  else
    # Refresh cache to ensure flags like BUILD_EXAMPLES stay in sync.
    configure_cmd = ['cmake', '-S', ROOT_DIR, '-B', build_dir, *cmake_args]
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
  build_cmd = ['cmake', '--build', '.']
  build_cmd += ['--parallel', jobs.to_s] if jobs
  stdout, stderr, status = run_command(build_cmd, chdir: build_dir, verbose: verbose, label: 'build')
  unless status.success?
    warn stdout
    warn stderr
    abort 'Build failed'
  end
end

ctest_cmd = ['ctest', '--output-on-failure']
ctest_cmd += ['--parallel', test_jobs.to_s] unless ctest_parallel_requested?(ctest_args)
ctest_cmd << '--progress' if verbose
ctest_cmd += ctest_args
log("Running ctest with #{ctest_parallel_requested?(ctest_args) ? 'caller-provided' : test_jobs} parallel jobs...", verbose: verbose)
stdout, stderr, status = run_command(ctest_cmd, chdir: build_dir, verbose: verbose, label: 'ctest')
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
        rerun_cmd = ['ctest', '-VV', '--output-on-failure', '-R', pattern]
        run_command(rerun_cmd, chdir: build_dir, verbose: true, label: "rerun-failed")
      end
    end
  end

  abort 'Some tests failed'
end

puts '\nAll tests completed successfully.'
