#!/usr/bin/env ruby
# frozen_string_literal: true

# Aleph-w — cellular-automata performance-regression gate (roadmap Phase 20).
#
# Runs the anchor benchmarks, parses their structured-JSON output, and either
# refreshes the committed baseline (--update-baseline) or compares the fresh
# numbers against it and fails when any benchmark regresses past its configured
# threshold. A Markdown comparison table is written for the PR comment.
#
# The gate keys on `wall_ns` (lower is better): a benchmark regresses when its
# median wall time grows by more than its threshold percentage relative to the
# baseline. The default threshold is 10 %; per-benchmark overrides live in
# benchmarks/perf_config.json so they survive baseline regeneration.

require 'json'
require 'open3'
require 'optparse'
require 'shellwords'
require 'time'

# Benchmark binaries, in the order they are reported. These mirror the targets
# declared in benchmarks/ca/CMakeLists.txt.
BENCHMARKS = %w[
  bench_gol_1024
  bench_wolfram_30_1d
  bench_gray_scott_512
  bench_parallel_gol
  bench_hashlife_breeder
  bench_multi_field_lv
].freeze

REQUIRED_KEYS = %w[name wall_ns cells_per_sec compiler cpu].freeze

# Recursively sort hash keys so the serialised JSON is stable across runs.
def sorted_deep(obj)
  case obj
  when Hash
    obj.keys.sort.each_with_object({}) { |k, acc| acc[k] = sorted_deep(obj[k]) }
  when Array
    obj.map { |e| sorted_deep(e) }
  else
    obj
  end
end

def utc_now
  Time.now.utc.strftime('%Y-%m-%dT%H:%M:%SZ')
end

def run_benchmark(binary_path, taskset_cpus, repeats)
  cmd = []
  cmd += ['taskset', '-c', taskset_cpus] unless taskset_cpus.nil?
  cmd << binary_path
  env = {}
  env['CA_BENCH_REPEATS'] = repeats.to_s unless repeats.nil?

  stdout, stderr, status = Open3.capture3(env, *cmd)
  unless status.success?
    warn(stderr)
    abort("Benchmark command failed: #{cmd.shelljoin}")
  end
  line = stdout.strip.lines.last.to_s.strip
  abort("#{binary_path}: produced no output") if line.empty?
  obj = JSON.parse(line) # raises on malformed JSON: the gate fails loudly.
  missing = REQUIRED_KEYS - obj.keys
  abort("#{binary_path}: JSON missing keys #{missing.inspect}") unless missing.empty?
  obj
end

def collect(options)
  results = {}
  compiler = nil
  cpu = nil
  BENCHMARKS.each do |binary|
    path = File.join(options[:build_dir], 'benchmarks', 'ca', binary)
    abort("benchmark binary not found: #{path}") unless File.exist?(path)
    obj = run_benchmark(path, options[:taskset_cpus], options[:repeats])
    compiler = obj['compiler']
    cpu = obj['cpu']
    results[obj['name']] = {
      'wall_ns' => obj['wall_ns'].to_i,
      'cells_per_sec' => obj['cells_per_sec'].to_f
    }
    puts format('  %-20s wall=%d ns  cells/s=%.3f',
                obj['name'], obj['wall_ns'].to_i, obj['cells_per_sec'].to_f)
  end
  [results, compiler, cpu]
end

def write_baseline(path, results, compiler, cpu)
  doc = {
    'schema' => 1,
    'metric' => 'wall_ns',
    'generated_utc' => utc_now,
    'compiler' => compiler,
    'cpu' => cpu,
    'benchmarks' => results
  }
  File.write(path, "#{JSON.pretty_generate(sorted_deep(doc))}\n")
  puts "Wrote baseline with #{results.size} benchmarks to #{path}"
end

def append_history(path, results, compiler, cpu)
  record = {
    'utc' => utc_now,
    'compiler' => compiler,
    'cpu' => cpu,
    'benchmarks' => results.transform_values { |r| r['wall_ns'] }
  }
  dir = File.dirname(path)
  require 'fileutils'
  FileUtils.mkdir_p(dir) unless dir.empty? || Dir.exist?(dir)
  File.open(path, 'a') { |fh| fh.puts(JSON.generate(sorted_deep(record))) }
  puts "Appended trend sample to #{path}"
end

def threshold_for(config, name)
  default = (config['default_threshold_pct'] || 10.0).to_f
  entry = (config['benchmarks'] || {})[name] || {}
  (entry['threshold_pct'] || default).to_f
end

def compare(results, baseline, config)
  base = baseline['benchmarks'] || {}
  rows = []
  regressed = []
  results.keys.sort.each do |name|
    cur = results[name]
    thr = threshold_for(config, name)
    unless base.key?(name)
      rows << { name: name, base_ns: nil, cur_ns: cur['wall_ns'],
                delta_pct: nil, threshold: thr, status: 'new' }
      next
    end
    base_ns = base[name]['wall_ns']
    delta = base_ns.to_f.zero? ? 0.0 : (cur['wall_ns'] - base_ns) / base_ns.to_f * 100.0
    status =
      if delta > thr
        regressed << name
        'regressed'
      elsif delta < -thr
        'improved'
      else
        'ok'
      end
    rows << { name: name, base_ns: base_ns, cur_ns: cur['wall_ns'],
              delta_pct: delta, threshold: thr, status: status }
  end
  [rows, regressed]
end

def ms(ns)
  ns.nil? ? '—' : format('%.2f', ns / 1e6)
end

def render_markdown(rows, baseline, advisory: false)
  icon = { 'ok' => '✅', 'improved' => '🟢', 'regressed' => '❌', 'new' => '🆕',
           'advisory' => 'ℹ️' }
  lines = ['### Cellular-automata perf gate', '']
  meta = []
  meta << "baseline CPU `#{baseline['cpu']}`" if baseline['cpu']
  meta << "compiler `#{baseline['compiler']}`" if baseline['compiler']
  meta << "recorded #{baseline['generated_utc']}" if baseline['generated_utc']
  lines += [meta.join(' · '), ''] unless meta.empty?
  lines += [
    '| Benchmark | Baseline (ms) | Current (ms) | Δ | Threshold | Status |',
    '|---|--:|--:|--:|--:|:--|'
  ]
  rows.each do |r|
    delta = r[:delta_pct].nil? ? '—' : format('%+.1f%%', r[:delta_pct])
    # Across mismatched CPUs the pass/fail verdict is meaningless, so the
    # status column is neutralised to "advisory" and only the raw numbers
    # remain (still useful as a sanity check).
    status = advisory && r[:status] != 'new' ? 'advisory' : r[:status]
    lines << format('| `%s` | %s | %s | %s | ±%d%% | %s %s |',
                    r[:name], ms(r[:base_ns]), ms(r[:cur_ns]),
                    delta, r[:threshold], icon[status], status)
  end
  lines << ''
  lines << '_Lower wall time is better; Δ is current vs. baseline._'
  "#{lines.join("\n")}\n"
end

def parse_options(argv)
  options = {
    build_dir: 'build-benchmarks',
    baseline: 'benchmarks/baseline.json',
    config: 'benchmarks/perf_config.json',
    taskset_cpus: nil,
    repeats: nil,
    update_baseline: false,
    current_out: nil,
    history_append: nil,
    markdown_out: nil,
    soft_on_cpu_mismatch: false
  }
  OptionParser.new do |opts|
    opts.banner = 'Usage: ca_perf_regression.rb [options]'
    opts.on('--build-dir DIR', 'CMake build directory with the benchmarks') { |v| options[:build_dir] = v }
    opts.on('--baseline PATH', 'Baseline JSON (read on compare, written on update)') { |v| options[:baseline] = v }
    opts.on('--config PATH', 'Per-benchmark threshold config') { |v| options[:config] = v }
    opts.on('--taskset-cpus LIST', "Pin benchmarks to this CPU list via 'taskset -c' (e.g. 0-3)") { |v| options[:taskset_cpus] = v }
    opts.on('--repeats N', Integer, 'Override CA_BENCH_REPEATS for every benchmark') { |v| options[:repeats] = v }
    opts.on('--update-baseline', 'Overwrite the baseline with the fresh numbers') { options[:update_baseline] = true }
    opts.on('--current-out PATH', 'Write the fresh results as JSON to this path') { |v| options[:current_out] = v }
    opts.on('--history-append PATH', 'Append a timestamped sample to this JSON-lines file') { |v| options[:history_append] = v }
    opts.on('--markdown-out PATH', 'Write the comparison table to this path') { |v| options[:markdown_out] = v }
    opts.on('--soft-on-cpu-mismatch', 'Never fail when the baseline CPU differs (advisory)') { options[:soft_on_cpu_mismatch] = true }
  end.parse!(argv)
  options
end

def main(argv)
  options = parse_options(argv)

  puts 'Running cellular-automata anchor benchmarks...'
  results, compiler, cpu = collect(options)

  write_baseline(options[:current_out], results, compiler, cpu) if options[:current_out]
  append_history(options[:history_append], results, compiler, cpu) if options[:history_append]

  if options[:update_baseline]
    write_baseline(options[:baseline], results, compiler, cpu)
    return 0
  end

  # No committed baseline yet (the very first run, before `perf-update` has
  # recorded one on the canonical runner): there is nothing to compare against,
  # so the gate is advisory and must not fail the PR.
  unless File.exist?(options[:baseline])
    note = "### Cellular-automata perf gate\n\n" \
           "> ⚠️ No baseline at `#{options[:baseline]}` yet. The gate is " \
           "**advisory** until one is recorded on this hardware (run the " \
           "`perf-update` workflow on `master`).\n"
    puts
    puts note
    File.write(options[:markdown_out], note) if options[:markdown_out]
    warn('PERF GATE ADVISORY: no baseline to compare against; not failing.')
    return 0
  end

  baseline = JSON.parse(File.read(options[:baseline]))
  config = File.exist?(options[:config]) ? JSON.parse(File.read(options[:config])) : {}

  rows, regressed = compare(results, baseline, config)
  cpu_mismatch = baseline['cpu'] && cpu && baseline['cpu'] != cpu
  table = render_markdown(rows, baseline, advisory: cpu_mismatch)

  if cpu_mismatch
    note = "> ⚠️ Baseline was recorded on `#{baseline['cpu']}` but this run used " \
           "`#{cpu}`. Absolute times are not comparable across CPUs, so the gate " \
           'is **advisory** until the baseline is regenerated on this hardware ' \
           "(run the `perf-update` workflow on `master`).\n\n"
    table = note + table
  end

  puts
  puts table
  File.write(options[:markdown_out], table) if options[:markdown_out]

  if !regressed.empty? && !(cpu_mismatch && options[:soft_on_cpu_mismatch])
    warn("PERF GATE FAILED: #{regressed.size} regression(s): #{regressed.join(', ')}")
    return 1
  elsif !regressed.empty?
    warn('PERF GATE ADVISORY: regressions detected but baseline CPU differs; not failing.')
    return 0
  end
  puts 'PERF GATE PASSED: no benchmark regressed past its threshold.'
  0
end

exit(main(ARGV)) if $PROGRAM_NAME == __FILE__