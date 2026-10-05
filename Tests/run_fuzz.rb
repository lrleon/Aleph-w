#!/usr/bin/env ruby
# frozen_string_literal: true

# Run Aleph's libFuzzer targets locally with private, persistent corpora.

require 'fileutils'
require 'optparse'

ROOT = File.expand_path('..', __dir__)
TARGETS = {
  'fuzz_rle_parser' => 'rle',
  'fuzz_life_parser' => 'life',
  'fuzz_csv_reader' => 'csv',
  'fuzz_checkpoint_loader' => 'checkpoint',
  'fuzz_dynarray' => 'dynarray',
  'fuzz_sort' => 'sort'
}.freeze

options = {
  seconds: 30,
  build_dir: File.join(ROOT, 'build-fuzz-local'),
  targets: []
}

parser = OptionParser.new do |opts|
  opts.banner = 'Usage: ruby Tests/run_fuzz.rb [--seconds N] [--target NAME] [--build-dir DIR]'
  opts.on('--seconds N', Integer, 'Seconds per target (default: 30)') do |value|
    options[:seconds] = value
  end
  opts.on('--target NAME', 'Run only this target; repeat to select several') do |value|
    options[:targets] << value
  end
  opts.on('--build-dir DIR', 'CMake build directory (default: build-fuzz-local)') do |value|
    options[:build_dir] = File.expand_path(value, ROOT)
  end
  opts.on('--list', 'List available targets') do
    puts TARGETS.keys
    exit 0
  end
  opts.on('-h', '--help', 'Show this help') do
    puts opts
    exit 0
  end
end

begin
  parser.parse!
rescue OptionParser::ParseError => e
  abort "#{e}\n#{parser}"
end
abort "Unexpected arguments: #{ARGV.join(' ')}" unless ARGV.empty?
abort '--seconds must be positive' unless options[:seconds].positive?

unknown = options[:targets] - TARGETS.keys
abort "Unknown target(s): #{unknown.join(', ')}. Use --list." unless unknown.empty?
selected = options[:targets].empty? ? TARGETS.keys : options[:targets].uniq
build_dir = options[:build_dir]

def run!(command)
  puts "\n> #{command.join(' ')}"
  $stdout.flush
  success = system(*command, chdir: ROOT)
  abort "Command failed: #{command.first}" unless success
end

run!(['cmake', '-S', ROOT, '-B', build_dir, '-G', 'Ninja',
      '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++',
      '-DBUILD_TESTS=ON', '-DBUILD_EXAMPLES=OFF',
      '-DALEPH_BUILD_X11_VIEWER=OFF', '-DALEPH_BUILD_C_API=OFF',
      '-DALEPH_FETCH_GTEST=OFF', '-DALEPH_BUILD_FUZZERS=ON'])
run!(['cmake', '--build', build_dir, '--target', *selected])

selected.each do |target|
  corpus_name = TARGETS.fetch(target)
  corpus = File.join(build_dir, 'Tests', 'fuzz', 'corpus', corpus_name)
  artifacts = File.join(build_dir, 'Tests', 'fuzz', 'artifacts', target)
  FileUtils.mkdir_p([corpus, artifacts])
  Dir.children(File.join(ROOT, 'Tests', 'fuzz', 'corpus', corpus_name)).each do |seed|
    source = File.join(ROOT, 'Tests', 'fuzz', 'corpus', corpus_name, seed)
    destination = File.join(corpus, seed)
    FileUtils.cp(source, destination) if File.file?(source) && !File.exist?(destination)
  end

  binary = File.join(build_dir, 'Tests', 'fuzz', target)
  puts "\nFuzzing #{target} for #{options[:seconds]}s; corpus: #{corpus}"
  $stdout.flush
  success = system({ 'ASAN_OPTIONS' => 'detect_leaks=0',
                     'UBSAN_OPTIONS' => 'print_stacktrace=1:halt_on_error=1' },
                   binary, "-max_total_time=#{options[:seconds]}",
                   '-rss_limit_mb=2048', '-timeout=25',
                   '-verbosity=0', '-print_final_stats=1',
                   "-artifact_prefix=#{artifacts}/",
                   corpus, chdir: ROOT)
  abort "#{target} failed; inspect #{artifacts}" unless success
end

puts "\nAll #{selected.length} fuzz target(s) completed without failures."
