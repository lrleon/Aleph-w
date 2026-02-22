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
# Batch CI wrapper for planarity certificate validation.

require "json"
require "optparse"
require "open3"
require "pathname"
require "rbconfig"
require "timeout"


def default_validator_path
  Pathname.new(__FILE__).realpath.dirname.join("planarity_certificate_validator.rb")
end


def parse_args(argv)
  options = {
    "inputs" => [],
    "input_globs" => [],
    "report" => "/tmp/aleph_planarity_certificate_ci_report.json",
    "validator" => default_validator_path.to_s,
    "format" => "auto",
    "networkx" => false,
    "require_networkx" => false,
    "gephi" => false,
    "require_gephi" => false,
    "gephi_cmd" => "gephi --version",
    "gephi_template" => "",
    "gephi_template_catalog" => Pathname.new(__FILE__).realpath.dirname.join("planarity_gephi_templates.json").to_s,
    "render_gephi" => false,
    "require_render" => false,
    "render_profile" => "",
    "render_profile_catalog" => Pathname.new(__FILE__).realpath.dirname.join("planarity_gephi_render_profiles.json").to_s,
    "render_output_dir" => "/tmp/aleph_planarity_renders",
    "print_summary" => false
  }

  parser = OptionParser.new do |opts|
    opts.banner = "Usage: #{$PROGRAM_NAME} [options]"

    opts.on("--input PATH", "Input file to validate (repeatable).") { |v| options["inputs"] << v }
    opts.on("--input-glob GLOB", "Glob pattern for input files (repeatable).") { |v| options["input_globs"] << v }
    opts.on("--report PATH", "Output JSON report path.") { |v| options["report"] = v }
    opts.on("--validator PATH", "Path to planarity_certificate_validator.rb") { |v| options["validator"] = v }
    opts.on("--format FORMAT", ["auto", "graphml", "gexf"], "Input format forwarded to validator.") { |v| options["format"] = v }

    opts.on("--networkx") { options["networkx"] = true }
    opts.on("--require-networkx") { options["require_networkx"] = true }
    opts.on("--gephi") { options["gephi"] = true }
    opts.on("--require-gephi") { options["require_gephi"] = true }
    opts.on("--gephi-cmd CMD") { |v| options["gephi_cmd"] = v }
    opts.on("--gephi-template ID") { |v| options["gephi_template"] = v }
    opts.on("--gephi-template-catalog PATH") { |v| options["gephi_template_catalog"] = v }

    opts.on("--render-gephi") { options["render_gephi"] = true }
    opts.on("--require-render") { options["require_render"] = true }
    opts.on("--render-profile ID") { |v| options["render_profile"] = v }
    opts.on("--render-profile-catalog PATH") { |v| options["render_profile_catalog"] = v }
    opts.on("--render-output-dir PATH") { |v| options["render_output_dir"] = v }

    opts.on("--print-summary") { options["print_summary"] = true }
  end

  parser.parse!(argv)
  options
end


def collect_inputs(inputs, globs)
  set = {}

  inputs.each do |raw|
    set[Pathname.new(raw).expand_path.to_s] = true
  end

  globs.each do |pattern|
    Pathname.glob(pattern).each do |p|
      set[p.expand_path.to_s] = true
    end
  end

  set.keys.sort.map { |raw| Pathname.new(raw) }
end


def run_command(cmd, timeout_sec)
  stdout = ""
  stderr = ""
  status = nil

  Timeout.timeout(timeout_sec) do
    stdout, stderr, status = Open3.capture3(*cmd)
  end

  {
    "exit_code" => status.exitstatus,
    "stdout" => stdout,
    "stderr" => stderr,
    "error" => nil,
    "timeout" => false
  }
rescue Timeout::Error
  {
    "exit_code" => 3,
    "stdout" => "",
    "stderr" => "batch runner timeout",
    "error" => nil,
    "timeout" => true
  }
rescue StandardError => e
  {
    "exit_code" => 3,
    "stdout" => "",
    "stderr" => "",
    "error" => e.message,
    "timeout" => false
  }
end


def run_validator(args, files)
  cmd = [RbConfig.ruby, args["validator"], "--json", "--format", args["format"]]
  files.each do |path|
    cmd << "--input"
    cmd << path.to_s
  end

  cmd << "--networkx" if args["networkx"]
  cmd << "--require-networkx" if args["require_networkx"]
  cmd << "--gephi" if args["gephi"]
  cmd << "--require-gephi" if args["require_gephi"]

  unless args["gephi_cmd"].to_s.empty?
    cmd << "--gephi-cmd"
    cmd << args["gephi_cmd"]
  end

  unless args["gephi_template"].to_s.empty?
    cmd << "--gephi-template"
    cmd << args["gephi_template"]
  end

  unless args["gephi_template_catalog"].to_s.empty?
    cmd << "--gephi-template-catalog"
    cmd << args["gephi_template_catalog"]
  end

  cmd << "--render-gephi" if args["render_gephi"]
  cmd << "--require-render" if args["require_render"]

  unless args["render_profile"].to_s.empty?
    cmd << "--render-profile"
    cmd << args["render_profile"]
  end

  unless args["render_profile_catalog"].to_s.empty?
    cmd << "--render-profile-catalog"
    cmd << args["render_profile_catalog"]
  end

  unless args["render_output_dir"].to_s.empty?
    cmd << "--render-output-dir"
    cmd << args["render_output_dir"]
  end

  run = run_command(cmd, 300)
  payload = {
    "command" => cmd,
    "validator_exit_code" => run["exit_code"],
    "validator_stdout" => run["stdout"],
    "validator_stderr" => run["stderr"],
    "validator_json" => nil
  }
  payload["runner_error"] = run["error"] unless run["error"].nil?
  payload["runner_timeout"] = true if run["timeout"]

  unless run["stdout"].to_s.strip.empty?
    begin
      payload["validator_json"] = JSON.parse(run["stdout"])
    rescue StandardError
      payload["validator_json"] = nil
    end
  end

  [run["exit_code"].to_i, payload]
end


def main(argv)
  begin
    args = parse_args(argv)
  rescue OptionParser::ParseError => e
    warn(e.message)
    return 3
  rescue StandardError => e
    warn("error: #{e.message}")
    return 3
  end

  report_path = Pathname.new(args["report"]).expand_path
  files = collect_inputs(args["inputs"], args["input_globs"])

  if files.empty?
    warn("error: no inputs provided (use --input or --input-glob).")
    return 3
  end

  exit_code, run_payload = run_validator(args, files)

  report = {
    "batch_runner" => "planarity_certificate_ci_batch.rb",
    "num_inputs" => files.size,
    "inputs" => files.map(&:to_s),
    "settings" => {
      "format" => args["format"],
      "networkx" => args["networkx"],
      "require_networkx" => args["require_networkx"],
      "gephi" => args["gephi"],
      "require_gephi" => args["require_gephi"],
      "gephi_cmd" => args["gephi_cmd"],
      "gephi_template" => args["gephi_template"],
      "gephi_template_catalog" => args["gephi_template_catalog"],
      "render_gephi" => args["render_gephi"],
      "require_render" => args["require_render"],
      "render_profile" => args["render_profile"],
      "render_profile_catalog" => args["render_profile_catalog"],
      "render_output_dir" => args["render_output_dir"]
    },
    "run" => run_payload
  }

  report_path.dirname.mkpath
  report_path.write(JSON.pretty_generate(report))

  if args["print_summary"]
    overall_valid = nil
    if run_payload["validator_json"].is_a?(Hash)
      overall_valid = run_payload["validator_json"]["overall_valid"]
    end
    puts "inputs=#{files.size}"
    puts "report=#{report_path}"
    puts "validator_exit_code=#{exit_code}"
    puts "overall_valid=#{overall_valid}" unless overall_valid.nil?
  end

  exit_code
end


exit(main(ARGV))
