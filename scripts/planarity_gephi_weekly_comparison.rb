#!/usr/bin/env ruby
# Aleph-w
# Data structures & Algorithms
# https://github.com/lrleon/Aleph-w
#
# Copyright (c) 2002-2026 Leandro Rabindranath Leon
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Aggregate per-case Gephi nightly artifacts and detect regressions across tags.
#
# Exit codes:
# - 0: report generated and no enforced regressions
# - 2: regressions found and --fail-on-regressions is enabled
# - 3: runtime/usage error

require "fileutils"
require "json"
require "optparse"
require "pathname"


def parse_args(argv)
  options = {
    "artifacts_root" => nil,
    "resolved_tags" => "",
    "run_id" => "",
    "run_attempt" => "",
    "git_sha" => "",
    "report_json" => "/tmp/gephi_weekly_comparison.json",
    "report_md" => "/tmp/gephi_weekly_comparison.md",
    "fail_on_regressions" => false,
    "print_summary" => false
  }

  parser = OptionParser.new do |opts|
    opts.banner = "Usage: #{$PROGRAM_NAME} --artifacts-root DIR [options]"

    opts.on("--artifacts-root DIR", "Root directory containing downloaded per-case artifacts.") do |value|
      options["artifacts_root"] = value
    end

    opts.on("--resolved-tags CSV", "Comma-separated tag list resolved by the workflow.") do |value|
      options["resolved_tags"] = value
    end

    opts.on("--run-id ID") { |value| options["run_id"] = value }
    opts.on("--run-attempt N") { |value| options["run_attempt"] = value }
    opts.on("--git-sha SHA") { |value| options["git_sha"] = value }

    opts.on("--report-json PATH", "Output JSON report path.") do |value|
      options["report_json"] = value
    end

    opts.on("--report-md PATH", "Output Markdown report path.") do |value|
      options["report_md"] = value
    end

    opts.on("--fail-on-regressions", "Return exit code 2 when regressions are detected.") do
      options["fail_on_regressions"] = true
    end

    opts.on("--print-summary", "Print compact summary to stdout.") do
      options["print_summary"] = true
    end
  end

  parser.parse!(argv)
  if options["artifacts_root"].nil? or options["artifacts_root"].strip.empty?
    raise OptionParser::MissingArgument, "--artifacts-root"
  end

  options
end


def parse_kv_file(path)
  out = {}
  return out unless path.file?

  path.each_line do |raw|
    line = raw.strip
    next if line.empty?
    next unless line.include?("=")

    key, value = line.split("=", 2)
    out[key.strip] = value.strip
  end

  out
end


def to_int(value, default)
  return value if value.is_a?(Integer)

  if value.is_a?(String)
    begin
      return Integer(value.strip)
    rescue StandardError
      return default
    end
  end

  default
end


def normalize_bool(value)
  return value if value == true or value == false
  return value != 0 if value.is_a?(Integer)

  if value.is_a?(String)
    normalized = value.strip.downcase
    return true if ["1", "true", "yes", "y", "ok"].include?(normalized)
  end

  false
end


def parse_semver(tag)
  match = /^v?(\d+)\.(\d+)\.(\d+)$/.match(tag.to_s.strip)
  return nil if match.nil?

  [match[1].to_i, match[2].to_i, match[3].to_i]
end


def tag_sort_key(tag)
  parsed = parse_semver(tag)
  return [1_000_000_000, 1_000_000_000, 1_000_000_000, tag.to_s] if parsed.nil?

  [parsed[0], parsed[1], parsed[2], tag.to_s]
end


def collect_entries(artifacts_root)
  entries = []
  return entries unless artifacts_root.directory?

  artifacts_root.children.select(&:directory?).sort.each do |artifact_dir|
    kv = parse_kv_file(artifact_dir.join("downloaded_gephi_asset.txt"))

    case_data = {}
    case_path = artifact_dir.join("nightly_case_summary.json")
    if case_path.file?
      begin
        parsed = JSON.parse(case_path.read)
        case_data = parsed if parsed.is_a?(Hash)
      rescue StandardError
        case_data = {}
      end
    end

    entry = {
      "artifact_name" => artifact_dir.basename.to_s,
      "os" => (case_data["os"] || kv["runner_os"] || "").to_s,
      "gephi_tag" => (case_data["gephi_tag"] || kv["tag"] || "").to_s,
      "asset_name" => (case_data["asset_name"] || kv["asset_name"] || "").to_s,
      "validator_exit_code" => to_int(case_data["validator_exit_code"], -1),
      "overall_valid" => normalize_bool(case_data["overall_valid"]),
      "gephi_executable" => (kv["gephi_executable"] || "").to_s
    }
    entries << entry
  end

  entries.sort_by { |entry| [tag_sort_key(entry["gephi_tag"]), entry["os"].to_s] }
end


def detect_regressions(entries)
  by_os = {}
  entries.each do |entry|
    os_name = entry["os"].to_s
    by_os[os_name] ||= []
    by_os[os_name] << entry
  end

  regressions = []
  by_os.each do |os_name, os_entries|
    ordered = os_entries.sort_by { |entry| tag_sort_key(entry["gephi_tag"]) }
    (1...ordered.length).each do |index|
      prev = ordered[index - 1]
      curr = ordered[index]
      reasons = []

      prev_valid = !!prev["overall_valid"]
      curr_valid = !!curr["overall_valid"]
      prev_code = to_int(prev["validator_exit_code"], -1)
      curr_code = to_int(curr["validator_exit_code"], -1)

      reasons << "overall_valid_regressed" if prev_valid and not curr_valid
      reasons << "exit_code_regressed" if prev_code == 0 and curr_code != 0
      next if reasons.empty?

      regressions << {
        "os" => os_name,
        "from_tag" => prev["gephi_tag"].to_s,
        "to_tag" => curr["gephi_tag"].to_s,
        "from_overall_valid" => prev_valid,
        "to_overall_valid" => curr_valid,
        "from_exit_code" => prev_code,
        "to_exit_code" => curr_code,
        "reason_codes" => reasons
      }
    end
  end

  regressions.sort_by do |item|
    [item["os"].to_s, tag_sort_key(item["from_tag"]), tag_sort_key(item["to_tag"])]
  end
end


def build_markdown(report)
  lines = []
  lines << "## Gephi Nightly Comparison"
  lines << ""
  lines << "- run_id: #{report["run_id"]}"
  lines << "- run_attempt: #{report["run_attempt"]}"
  lines << "- git_sha: #{report["git_sha"]}"
  lines << "- resolved_tags: #{report["resolved_tags"]}"
  lines << "- num_entries: #{report["num_entries"]}"
  lines << "- num_regressions: #{report["num_regressions"]}"
  lines << ""
  lines << "| Gephi tag | OS | valid | exit_code | asset | executable |"
  lines << "|---|---|---:|---:|---|---|"

  entries = report["entries"]
  if entries.is_a?(Array) and not entries.empty?
    entries.each do |entry|
      valid_text = entry["overall_valid"] ? "true" : "false"
      lines << "| #{entry["gephi_tag"]} | #{entry["os"]} | #{valid_text} | " \
               "#{entry["validator_exit_code"]} | #{entry["asset_name"]} | " \
               "#{entry["gephi_executable"]} |"
    end
  else
    lines << "| (none) | (none) | false | -1 | (none) | (none) |"
  end

  lines << ""
  lines << "### Regression Alerts"
  lines << ""
  regressions = report["regressions"]
  if regressions.is_a?(Array) and not regressions.empty?
    regressions.each do |reg|
      lines << "- `#{reg["os"]}`: `#{reg["from_tag"]}` -> `#{reg["to_tag"]}` " \
               "(reasons: #{reg["reason_codes"].join(", ")})"
    end
  else
    lines << "- none"
  end

  lines << ""
  lines.join("\n")
end


def append_github_outputs(num_regressions, has_regressions)
  output_path = ENV.fetch("GITHUB_OUTPUT", "").strip
  return if output_path.empty?

  File.open(output_path, "a") do |file|
    file.puts("num_regressions=#{num_regressions}")
    file.puts("has_regressions=#{has_regressions ? "true" : "false"}")
  end
end


def main(argv)
  options = parse_args(argv)

  artifacts_root = Pathname.new(options["artifacts_root"]).expand_path
  entries = collect_entries(artifacts_root)
  regressions = detect_regressions(entries)

  report = {
    "schema_version" => 1,
    "run_id" => options["run_id"],
    "run_attempt" => options["run_attempt"],
    "git_sha" => options["git_sha"],
    "resolved_tags" => options["resolved_tags"],
    "artifacts_root" => artifacts_root.to_s,
    "num_entries" => entries.length,
    "entries" => entries,
    "num_regressions" => regressions.length,
    "has_regressions" => regressions.length > 0,
    "regressions" => regressions
  }

  report_json_path = Pathname.new(options["report_json"]).expand_path
  report_md_path = Pathname.new(options["report_md"]).expand_path
  FileUtils.mkdir_p(report_json_path.dirname)
  FileUtils.mkdir_p(report_md_path.dirname)
  File.write(report_json_path, JSON.pretty_generate(report) + "\n")
  File.write(report_md_path, build_markdown(report))

  append_github_outputs(regressions.length, regressions.length > 0)

  if options["print_summary"]
    puts("entries=#{entries.length}")
    puts("regressions=#{regressions.length}")
    puts("report_json=#{report_json_path}")
    puts("report_md=#{report_md_path}")
  end

  return 2 if options["fail_on_regressions"] and regressions.length > 0
  0
rescue OptionParser::ParseError => e
  warn(e.message)
  3
rescue StandardError => e
  warn(e.message)
  3
end


exit(main(ARGV))
