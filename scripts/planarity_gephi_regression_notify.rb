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
# Optional notification helper for Gephi nightly regressions.
#
# Exit codes:
# - 0: success (with or without notifications)
# - 2: regression exists and required webhook delivery failed
# - 3: runtime/usage error

require "fileutils"
require "json"
require "net/http"
require "optparse"
require "pathname"
require "uri"


def parse_args(argv)
  options = {
    "report_json" => nil,
    "output_md" => "/tmp/gephi_nightly_alert.md",
    "repository" => "",
    "run_url" => "",
    "webhook_url" => "",
    "webhook_env" => "",
    "require_webhook_success" => false,
    "print_summary" => false
  }

  parser = OptionParser.new do |opts|
    opts.banner = "Usage: #{$PROGRAM_NAME} --report-json PATH [options]"

    opts.on("--report-json PATH", "Gephi nightly comparison JSON report.") do |value|
      options["report_json"] = value
    end

    opts.on("--output-md PATH", "Markdown alert output path.") do |value|
      options["output_md"] = value
    end

    opts.on("--repository REPO", "Repository slug, e.g. owner/repo.") do |value|
      options["repository"] = value
    end

    opts.on("--run-url URL", "Workflow run URL.") do |value|
      options["run_url"] = value
    end

    opts.on("--webhook-url URL", "Webhook endpoint for notifications.") do |value|
      options["webhook_url"] = value
    end

    opts.on("--webhook-env NAME", "Read webhook URL from environment variable NAME.") do |value|
      options["webhook_env"] = value
    end

    opts.on("--require-webhook-success", "Fail when regressions exist and webhook delivery is unavailable/failed.") do
      options["require_webhook_success"] = true
    end

    opts.on("--print-summary", "Print compact summary to stdout.") do
      options["print_summary"] = true
    end
  end

  parser.parse!(argv)
  if options["report_json"].nil? or options["report_json"].strip.empty?
    raise OptionParser::MissingArgument, "--report-json"
  end

  options
end


def normalize_bool(value)
  return value if value == true or value == false
  return value != 0 if value.is_a?(Integer)
  return ["1", "true", "yes", "y", "ok"].include?(value.strip.downcase) if value.is_a?(String)

  false
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


def build_markdown(report, repository, run_url)
  num_entries = to_int(report["num_entries"], 0)
  num_regressions = to_int(report["num_regressions"], 0)
  regressions = report["regressions"].is_a?(Array) ? report["regressions"] : []
  tags = report["resolved_tags"].to_s
  sha = report["git_sha"].to_s

  lines = []
  lines << "## Gephi Nightly Regression Alert"
  lines << ""
  lines << "- repository: #{repository}" unless repository.empty?
  lines << "- run_url: #{run_url}" unless run_url.empty?
  lines << "- git_sha: #{sha}" unless sha.empty?
  lines << "- resolved_tags: #{tags}" unless tags.empty?
  lines << "- num_entries: #{num_entries}"
  lines << "- num_regressions: #{num_regressions}"
  lines << ""

  if num_regressions <= 0 or regressions.empty?
    lines << "No regressions detected."
    lines << ""
    return lines.join("\n")
  end

  lines << "Detected regression(s):"
  regressions.each do |reg|
    os_name = reg["os"].to_s
    from_tag = reg["from_tag"].to_s
    to_tag = reg["to_tag"].to_s
    reasons = reg["reason_codes"].is_a?(Array) ? reg["reason_codes"] : []
    from_valid = normalize_bool(reg["from_overall_valid"])
    to_valid = normalize_bool(reg["to_overall_valid"])
    from_code = to_int(reg["from_exit_code"], -1)
    to_code = to_int(reg["to_exit_code"], -1)

    lines << "- `#{os_name}`: `#{from_tag}` -> `#{to_tag}`"
    lines << "  reasons: #{reasons.join(", ")}"
    lines << "  valid: #{from_valid} -> #{to_valid}; exit_code: #{from_code} -> #{to_code}"
  end
  lines << ""
  lines.join("\n")
end


def build_webhook_text(markdown)
  markdown.lines.map(&:rstrip).join("\n")
end


def post_webhook(url, markdown)
  uri = URI.parse(url)
  request = Net::HTTP::Post.new(uri.request_uri.empty? ? "/" : uri.request_uri)
  request["Content-Type"] = "application/json"
  request.body = JSON.generate({ "text" => build_webhook_text(markdown) })

  http = Net::HTTP.new(uri.host, uri.port)
  http.use_ssl = (uri.scheme == "https")
  http.open_timeout = 10
  http.read_timeout = 20
  response = http.request(request)
  response.code.to_i >= 200 and response.code.to_i < 300
end


def resolved_webhook_url(options)
  direct = options["webhook_url"].to_s.strip
  return direct unless direct.empty?

  env_name = options["webhook_env"].to_s.strip
  return "" if env_name.empty?

  ENV.fetch(env_name, "").strip
end


def main(argv)
  options = parse_args(argv)
  report_path = Pathname.new(options["report_json"]).expand_path
  report = JSON.parse(report_path.read)
  raise "comparison report is not a JSON object" unless report.is_a?(Hash)

  repository = options["repository"].to_s
  run_url = options["run_url"].to_s
  markdown = build_markdown(report, repository, run_url)

  output_md_path = Pathname.new(options["output_md"]).expand_path
  FileUtils.mkdir_p(output_md_path.dirname)
  File.write(output_md_path, markdown)

  num_regressions = to_int(report["num_regressions"], 0)
  webhook_url = resolved_webhook_url(options)
  webhook_sent = false
  webhook_ok = false

  if num_regressions > 0 and not webhook_url.empty?
    webhook_sent = true
    webhook_ok = post_webhook(webhook_url, markdown)
  end

  if options["print_summary"]
    puts("num_regressions=#{num_regressions}")
    puts("report_json=#{report_path}")
    puts("alert_md=#{output_md_path}")
    puts("webhook_sent=#{webhook_sent}")
    puts("webhook_ok=#{webhook_ok}")
  end

  if num_regressions > 0 and options["require_webhook_success"]
    return 2 if webhook_url.empty?
    return 2 if webhook_sent and not webhook_ok
  end

  0
rescue OptionParser::ParseError => e
  warn(e.message)
  3
rescue StandardError => e
  warn(e.message)
  3
end


exit(main(ARGV))
