#!/usr/bin/env ruby
# frozen_string_literal: true

require 'json'
require 'open3'
require 'optparse'
require 'shellwords'

options = {
  metric: 'median',
  warn_slowdown: 0.10,
  fail_slowdown: 0.20,
  repetitions: nil,
  warmup: nil,
  samples: nil,
  seed: nil,
  sizes: nil
}

parser = OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage:
      polynomial_perf_regression.rb --baseline BASE.json --current CUR.json [options]
      polynomial_perf_regression.rb --baseline BASE.json --benchmark /path/to/polynomial_benchmark [options]
  USAGE

  opts.on('--baseline PATH', 'Baseline benchmark JSON') { |value| options[:baseline] = value }
  opts.on('--current PATH', 'Current benchmark JSON') { |value| options[:current] = value }
  opts.on('--benchmark PATH', 'Benchmark executable to run and compare') { |value| options[:benchmark] = value }
  opts.on('--metric NAME', 'mean or median (default: median)') { |value| options[:metric] = value }
  opts.on('--warn-slowdown FLOAT', Float, 'Warn slowdown ratio, default 0.10') { |value| options[:warn_slowdown] = value }
  opts.on('--fail-slowdown FLOAT', Float, 'Fail slowdown ratio, default 0.20') { |value| options[:fail_slowdown] = value }
  opts.on('--repetitions N', Integer, 'Forwarded to polynomial_benchmark') { |value| options[:repetitions] = value }
  opts.on('--warmup N', Integer, 'Forwarded to polynomial_benchmark') { |value| options[:warmup] = value }
  opts.on('--samples N', Integer, 'Forwarded to polynomial_benchmark') { |value| options[:samples] = value }
  opts.on('--seed N', Integer, 'Forwarded to polynomial_benchmark') { |value| options[:seed] = value }
  opts.on('--sizes CSV', 'Forwarded to polynomial_benchmark') { |value| options[:sizes] = value }
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

def run_benchmark(options)
  cmd = [options[:benchmark], '--json']
  {
    repetitions: '--repetitions',
    warmup: '--warmup',
    samples: '--samples',
    seed: '--seed',
    sizes: '--sizes'
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
    key = [row.fetch('case'), row.fetch('size')]
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
  row_label = "#{key[0]}[#{key[1]}]"
  base_value = base_row.fetch(metric_key).to_f
  curr_value = curr_row.fetch(metric_key).to_f
  delta = slowdown_ratio(base_value, curr_value)
  pct = delta * 100.0

  report_lines << format('%-36s base=%10.4f current=%10.4f delta=%8.2f%%',
                         row_label,
                         base_value,
                         curr_value,
                         pct)

  if delta > options[:fail_slowdown]
    fail_lines << format('%s slowdown %.2f%% exceeds fail threshold %.2f%%',
                         row_label,
                         pct,
                         options[:fail_slowdown] * 100.0)
  elsif delta > options[:warn_slowdown]
    warn_lines << format('%s slowdown %.2f%% exceeds warn threshold %.2f%%',
                         row_label,
                         pct,
                         options[:warn_slowdown] * 100.0)
  end
end

puts "Polynomial performance regression report (metric=#{options[:metric]})"
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
