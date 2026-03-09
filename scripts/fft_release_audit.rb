#!/usr/bin/env ruby
# frozen_string_literal: true

require 'open3'
require 'optparse'
require 'pathname'
require 'shellwords'

options = {
  build_dir: 'build-test-clean',
  perf_baseline: nil,
  warn_slowdown: 0.15,
  fail_slowdown: 0.30,
  samples: 9,
  warmup: 3,
  batch_count: 16,
  sizes: nil,
  threads: nil,
  seed: nil
}

parser = OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage:
      fft_release_audit.rb [options]

    The audit verifies:
      1. required release documents exist
      2. fft_benchmark --validate passes
      3. fft_reference_check.rb passes (or skips cleanly if NumPy is unavailable)
      4. optional performance regression gate against a baseline JSON
  USAGE

  opts.on('--build-dir DIR', 'Build directory (default: build-test-clean)') do |value|
    options[:build_dir] = value
  end
  opts.on('--perf-baseline PATH', 'Optional baseline JSON for fft_perf_regression.rb') do |value|
    options[:perf_baseline] = value
  end
  opts.on('--warn-slowdown FLOAT', Float, 'Warn slowdown threshold (default: 0.15)') do |value|
    options[:warn_slowdown] = value
  end
  opts.on('--fail-slowdown FLOAT', Float, 'Fail slowdown threshold (default: 0.30)') do |value|
    options[:fail_slowdown] = value
  end
  opts.on('--samples N', Integer, 'Forwarded to fft_perf_regression.rb') do |value|
    options[:samples] = value
  end
  opts.on('--warmup N', Integer, 'Forwarded to fft_perf_regression.rb') do |value|
    options[:warmup] = value
  end
  opts.on('--batch-count N', Integer, 'Forwarded to fft_perf_regression.rb') do |value|
    options[:batch_count] = value
  end
  opts.on('--sizes CSV', 'Forwarded to fft_perf_regression.rb') do |value|
    options[:sizes] = value
  end
  opts.on('--threads N', Integer, 'Forwarded to fft_perf_regression.rb') do |value|
    options[:threads] = value
  end
  opts.on('--seed N', Integer, 'Forwarded to fft_perf_regression.rb') do |value|
    options[:seed] = value
  end
end
parser.parse!(ARGV)

repo_root = Pathname(__dir__).parent
build_dir = repo_root.join(options[:build_dir])
benchmark = build_dir.join('Examples', 'fft_benchmark')
probe = build_dir.join('Examples', 'fft_reference_probe')
reference_script = repo_root.join('scripts', 'fft_reference_check.rb')
perf_script = repo_root.join('scripts', 'fft_perf_regression.rb')

required_docs = %w[
  fft-plan.md
  fft-validation.md
  fft-contracts.md
  fft-support-matrix.md
  fft-changelog.md
].map { |name| repo_root.join(name) }

missing_docs = required_docs.reject(&:exist?)
unless missing_docs.empty?
  warn 'Missing release documents:'
  missing_docs.each { |path| warn "  #{path}" }
  exit 1
end

unless benchmark.exist?
  warn "fft_benchmark not found at #{benchmark}. Build it first with:"
  warn "  cmake --build #{options[:build_dir]} -j4 --target fft_benchmark"
  exit 1
end

unless probe.exist?
  warn "fft_reference_probe not found at #{probe}. Build it first with:"
  warn "  cmake --build #{options[:build_dir]} -j4 --target fft_reference_probe"
  exit 1
end

def run_step(label, command)
  puts "[audit] #{label}"
  puts "        #{command.shelljoin}"
  stdout, stderr, status = Open3.capture3(*command)
  print stdout unless stdout.empty?
  warn stderr unless stderr.empty?
  abort("[audit] #{label} failed") unless status.success?
end

run_step('benchmark-validate', [benchmark.to_s, '--validate'])
run_step('reference-cross-check',
         ['ruby', reference_script.to_s, '--build-dir', options[:build_dir]])

if options[:perf_baseline]
  command = [
    'ruby',
    perf_script.to_s,
    '--baseline', options[:perf_baseline],
    '--benchmark', benchmark.to_s,
    '--metric', 'median',
    '--warn-slowdown', options[:warn_slowdown].to_s,
    '--fail-slowdown', options[:fail_slowdown].to_s,
    '--warmup', options[:warmup].to_s,
    '--samples', options[:samples].to_s,
    '--batch-count', options[:batch_count].to_s
  ]
  command.concat(['--sizes', options[:sizes]]) if options[:sizes]
  command.concat(['--threads', options[:threads].to_s]) if options[:threads]
  command.concat(['--seed', options[:seed].to_s]) if options[:seed]
  run_step('performance-regression', command)
else
  puts '[audit] performance-regression skipped (no --perf-baseline provided)'
end

puts '[audit] FFT release audit passed'
