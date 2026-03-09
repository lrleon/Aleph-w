#!/usr/bin/env ruby
# frozen_string_literal: true

require 'json'
require 'open3'
require 'optparse'
require 'shellwords'
require 'tempfile'

options = {
  metric: 'median',
  warn_slowdown: 0.10,
  fail_slowdown: 0.20,
  repetitions: nil,
  warmup: nil,
  samples: nil,
  threads: nil,
  batch_count: nil,
  sizes: nil,
  seed: nil
}

parser = OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage:
      fft_perf_regression.rb --baseline BASE.json --current CUR.json [options]
      fft_perf_regression.rb --baseline BASE.json --benchmark /path/to/fft_benchmark [options]
  USAGE

  opts.on('--baseline PATH', 'Baseline benchmark JSON') { |value| options[:baseline] = value }
  opts.on('--current PATH', 'Current benchmark JSON') { |value| options[:current] = value }
  opts.on('--benchmark PATH', 'Benchmark executable to run and compare') { |value| options[:benchmark] = value }
  opts.on('--metric NAME', 'mean or median (default: median)') { |value| options[:metric] = value }
  opts.on('--warn-slowdown FLOAT', Float, 'Warn slowdown ratio, default 0.10') { |value| options[:warn_slowdown] = value }
  opts.on('--fail-slowdown FLOAT', Float, 'Fail slowdown ratio, default 0.20') { |value| options[:fail_slowdown] = value }
  opts.on('--repetitions N', Integer, 'Forwarded to fft_benchmark') { |value| options[:repetitions] = value }
  opts.on('--warmup N', Integer, 'Forwarded to fft_benchmark') { |value| options[:warmup] = value }
  opts.on('--samples N', Integer, 'Forwarded to fft_benchmark') { |value| options[:samples] = value }
  opts.on('--threads N', Integer, 'Forwarded to fft_benchmark') { |value| options[:threads] = value }
  opts.on('--batch-count N', Integer, 'Forwarded to fft_benchmark') { |value| options[:batch_count] = value }
  opts.on('--sizes CSV', 'Forwarded to fft_benchmark') { |value| options[:sizes] = value }
  opts.on('--seed N', Integer, 'Forwarded to fft_benchmark') { |value| options[:seed] = value }
end
parser.parse!(ARGV)

abort(parser.to_s) unless options[:baseline]
abort('Provide either --current or --benchmark') if options[:current].nil? && options[:benchmark].nil?
abort('Use either --current or --benchmark, not both') if options[:current] && options[:benchmark]
abort("Unsupported metric '#{options[:metric]}'") unless %w[mean median].include?(options[:metric])


def load_json(path)
  JSON.parse(File.read(path))
end


def validate_payload(payload, label)
  abort("#{label}: expected mode=benchmark") unless payload['mode'] == 'benchmark'
  abort("#{label}: expected rows array") unless payload['rows'].is_a?(Array)
end


def fetch_metadata_value(payload, *keys)
  metadata = payload['metadata']
  abort('payload: expected metadata object') unless metadata.is_a?(Hash)

  keys.each do |key|
    return metadata[key] if metadata.key?(key)
  end

  abort("payload metadata missing one of: #{keys.join(', ')}")
end


def compare_metadata(baseline_payload, current_payload, baseline_label, current_label)
  baseline_threads = fetch_metadata_value(baseline_payload, 'threads')
  current_threads = fetch_metadata_value(current_payload, 'threads')
  abort("#{baseline_label} and #{current_label} use different thread counts: #{baseline_threads} vs #{current_threads}") unless baseline_threads == current_threads

  baseline_simd = fetch_metadata_value(baseline_payload, 'simd_backend', 'simd')
  current_simd = fetch_metadata_value(current_payload, 'simd_backend', 'simd')
  abort("#{baseline_label} and #{current_label} use different SIMD backends: #{baseline_simd} vs #{current_simd}") unless baseline_simd == current_simd
end


def run_benchmark(options)
  cmd = [options[:benchmark], '--json']
  {
    repetitions: '--repetitions',
    warmup: '--warmup',
    samples: '--samples',
    threads: '--threads',
    batch_count: '--batch-count',
    sizes: '--sizes',
    seed: '--seed'
  }.each do |key, flag|
    next if options[key].nil?
    cmd << flag << options[key].to_s
  end

  stdout, stderr, status = Open3.capture3(*cmd)
  unless status.success?
    warn(stderr)
    abort("Benchmark command failed: #{cmd.shelljoin}")
  end

  JSON.parse(stdout)
end


def index_rows(payload)
  payload.fetch('rows').each_with_object({}) do |row, map|
    key = [row.fetch('case'), row.fetch('size'), row.fetch('items_per_call')]
    map[key] = row
  end
end


def slowdown_ratio(baseline, current)
  return Float::INFINITY if baseline <= 0.0 && current > 0.0
  return 0.0 if baseline <= 0.0
  current / baseline - 1.0
end

baseline = load_json(options[:baseline])
validate_payload(baseline, 'baseline')
current = options[:current] ? load_json(options[:current]) : run_benchmark(options)
validate_payload(current, 'current')
compare_metadata(baseline, current, 'baseline', 'current')

baseline_rows = index_rows(baseline)
current_rows = index_rows(current)

missing = baseline_rows.keys - current_rows.keys
extra = current_rows.keys - baseline_rows.keys
abort("Current benchmark is missing rows: #{missing.inspect}") unless missing.empty?
abort("Current benchmark has unexpected rows: #{extra.inspect}") unless extra.empty?

metric_key = "#{options[:metric]}_us"
warn_lines = []
fail_lines = []
report_lines = []

baseline_rows.keys.sort.each do |key|
  base_row = baseline_rows.fetch(key)
  curr_row = current_rows.fetch(key)
  row_label = "#{key[0]}[#{key[1]}|batch=#{key[2]}]"

  %w[sequential parallel].each do |path|
    base_value = base_row.fetch(path).fetch(metric_key).to_f
    curr_value = curr_row.fetch(path).fetch(metric_key).to_f
    delta = slowdown_ratio(base_value, curr_value)
    pct = delta * 100.0
    report_lines << format('%-24s %-10s base=%10.4f current=%10.4f delta=%8.2f%%',
                           row_label,
                           path,
                           base_value,
                           curr_value,
                           pct)

    if delta > options[:fail_slowdown]
      fail_lines << format('%s %s slowdown %.2f%% exceeds fail threshold %.2f%%',
                           row_label,
                           path,
                           pct,
                           options[:fail_slowdown] * 100.0)
    elsif delta > options[:warn_slowdown]
      warn_lines << format('%s %s slowdown %.2f%% exceeds warn threshold %.2f%%',
                           row_label,
                           path,
                           pct,
                           options[:warn_slowdown] * 100.0)
    end
  end
end

puts "FFT performance regression report (metric=#{options[:metric]})"
puts report_lines
unless warn_lines.empty?
  puts
  puts 'Warnings:'
  puts warn_lines
end
unless fail_lines.empty?
  puts
  puts 'Failures:'
  puts fail_lines
  exit 1
end
