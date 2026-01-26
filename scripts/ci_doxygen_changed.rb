#!/usr/bin/env ruby

require 'open3'
require 'pathname'

WARN_RE = /^(?<file>\/.+?):(?<line>\d+):\s+warning:\s+(?<msg>.+)$/


def run!(cmd)
  out, err, status = Open3.capture3(*cmd)
  raise "command failed: #{cmd.join(' ')}\n#{err.strip}" unless status.success?
  out
end


def github_event_name
  ENV.fetch('GITHUB_EVENT_NAME', '')
end


def base_ref
  ENV['GITHUB_BASE_REF']
end


def changed_files(base)
  return [] if base.nil? || base.empty?

  run!(['git', 'fetch', '--no-tags', '--prune', 'origin', "#{base}:#{base}"])
  out = run!(['git', 'diff', '--name-only', '--diff-filter=ACMR', "origin/#{base}...HEAD"])
  out.lines.map(&:strip).reject(&:empty?)
end


def main
  if github_event_name != 'pull_request'
    puts '[skip] not a pull_request event'
    return 0
  end

  base = base_ref
  changed = changed_files(base).to_h { |p| [p, true] }

  if changed.empty?
    puts '[ok] no changed files detected'
    return 0
  end

  repo_root = Pathname.new('.').realpath

  _out, err, status = Open3.capture3('doxygen', 'Doxyfile')

  hits = []
  err.each_line do |line|
    m = WARN_RE.match(line)
    next unless m

    begin
      rel = Pathname.new(m[:file]).realpath.relative_path_from(repo_root).to_s
    rescue StandardError
      next
    end

    next unless changed.key?(rel)

    hits << [rel, m[:line].to_i, m[:msg]]
  end

  if !hits.empty?
    hits.first(200).each do |rel, ln, msg|
      puts "[fail] doxygen warning in changed file: #{rel}:#{ln}: #{msg}"
    end
    puts "[fail] ... and #{hits.size - 200} more" if hits.size > 200
    return 1
  end

  puts '[ok] no doxygen warnings in changed files'
  0
end


exit(main)
