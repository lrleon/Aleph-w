#!/usr/bin/env ruby

require 'open3'
require 'pathname'

HEADER_EXTS = %w[.h .H .hpp .hxx .hh].freeze
TEST_EXTS = %w[.cc .cpp .C .cxx].freeze


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


def mit_license_header?(path)
  begin
    head = path.read.lines.first(80)
  rescue StandardError
    return false
  end

  txt = head.join
  required = [
    /Permission is hereby granted/i,
    /The above copyright notice and this permission notice/i,
    /THE SOFTWARE IS PROVIDED "AS IS"/i
  ]

  required.all? { |r| txt.match?(r) }
end


def docs_or_license_only_change?(path, base)
  return false unless path.exist?

  # Get the diff for this file
  begin
    diff_output = run!(['git', 'diff', "origin/#{base}...HEAD", '--', path.to_s])
  rescue StandardError
    return false
  end

  # Check if the diff only contains license-related or documentation changes
  added_lines = diff_output.lines.select { |l| l.start_with?('+') && !l.start_with?('+++') }
  removed_lines = diff_output.lines.select { |l| l.start_with?('-') && !l.start_with?('---') }

  return false if added_lines.empty? && removed_lines.empty?

  # Patterns for non-functional changes (license, comments, docs)
  license_keywords = /GPL|MIT|General Public License|Permission is hereby granted|Copyright|LICENSE|WARRANTY|NONINFRINGEMENT|sublicense/i
  comment_patterns = %r{
    ^\+\s*//|              # C++ single-line comment
    ^\+\s*/\*|             # C-style comment start
    ^\+\s*\*|              # C-style comment continuation
    ^\+\s*\*/|             # C-style comment end
    ^\-\s*//|              # Removed C++ comment
    ^\-\s*/\*|             # Removed C-style comment start
    ^\-\s*\*|              # Removed C-style comment continuation
    ^\-\s*\*/|             # Removed C-style comment end
    ^\+\s*$|               # Added blank line
    ^\-\s*$|               # Removed blank line
    @param|@return|@brief|@note|@see|@tparam  # Doxygen tags
  }x

  # ALL changed lines must be license-related, comments, or whitespace
  (added_lines + removed_lines).all? do |line|
    line.match?(license_keywords) || line.match?(comment_patterns)
  end
end


def main
  if github_event_name != 'pull_request'
    puts '[skip] not a pull_request event'
    return 0
  end

  base = base_ref
  files = changed_files(base)

  headers_changed = files.select { |f| HEADER_EXTS.include?(File.extname(f)) }
  tests_changed = files.select { |f| f.start_with?('Tests/') && TEST_EXTS.include?(File.extname(f)) }

  failures = []

  headers_changed.each do |hf|
    p = Pathname.new(hf)
    next unless p.exist?

    failures << "missing/invalid MIT license header: #{hf}" unless mit_license_header?(p)
  end

  # Filter out headers that only have license or documentation changes
  headers_with_code_changes = headers_changed.reject { |hf| docs_or_license_only_change?(Pathname.new(hf), base) }

  if !headers_with_code_changes.empty? && tests_changed.empty?
    failures << 'headers changed but no Tests/ files changed (add/modify tests)'
  end

  unless failures.empty?
    failures.each { |f| puts "[fail] #{f}" }
    return 1
  end

  puts "[ok] headers changed: #{headers_changed.size}"
  puts "[ok] headers with code changes: #{headers_with_code_changes.size}"
  puts "[ok] tests changed: #{tests_changed.size}"
  0
end


exit(main)
