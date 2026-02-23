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
# Deterministic visual golden-diff runner for Aleph planarity certificate renders.

require "digest"
require "json"
require "optparse"
require "open3"
require "pathname"
require "rbconfig"
require "timeout"

DEFAULT_VISUAL_GOLDEN_ENTRIES = [
  {
    "bytes" => 45,
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "profile_id" => "portable.ruby-render-pdf",
    "sha256" => "5a838678058f6de375e8635b5f2fea47a4e5f07cb1a882a44b10f39abc6f34ff"
  },
  {
    "bytes" => 101,
    "output_kind" => "svg",
    "output_ext" => "svg",
    "profile_id" => "portable.ruby-render-svg",
    "sha256" => "0459f595d6268e7bdf9e8e273d4394fa50d23a89ec3d2449d10b7b47941b1327"
  }
].freeze


def default_validator_path
  Pathname.new(__FILE__).realpath.dirname.join("planarity_certificate_validator.rb")
end


def default_render_catalog_path
  Pathname.new(__FILE__).realpath.dirname.join("planarity_gephi_render_profiles.json")
end


def default_manifest_path
  Pathname.new(__FILE__).realpath.dirname.join("planarity_visual_golden_manifest.json")
end


def sanitize_id(raw)
  token = raw.to_s.strip.gsub(/[^a-zA-Z0-9._-]+/, "_")
  token.empty? ? "case" : token
end


def parse_args(argv)
  options = {
    "inputs" => [],
    "input_globs" => [],
    "profiles" => [],
    "validator" => default_validator_path.to_s,
    "render_profile_catalog" => default_render_catalog_path.to_s,
    "render_output_dir" => "/tmp/aleph_planarity_visual_renders",
    "golden_manifest" => default_manifest_path.to_s,
    "report" => "/tmp/aleph_planarity_visual_diff_report.json",
    "update_golden" => false,
    "print_summary" => false
  }

  parser = OptionParser.new do |opts|
    opts.banner = "Usage: #{$PROGRAM_NAME} [options]"

    opts.on("--input PATH", "Certificate input file (repeatable).") { |v| options["inputs"] << v }
    opts.on("--input-glob GLOB", "Glob pattern for certificate inputs (repeatable).") { |v| options["input_globs"] << v }
    opts.on("--profile ID", "Render profile id (repeatable).") { |v| options["profiles"] << v }

    opts.on("--validator PATH", "Path to planarity_certificate_validator.rb.") { |v| options["validator"] = v }
    opts.on("--render-profile-catalog PATH", "Path to render profile catalog JSON.") { |v| options["render_profile_catalog"] = v }
    opts.on("--render-output-dir PATH", "Base output directory for rendered artifacts.") { |v| options["render_output_dir"] = v }
    opts.on("--golden-manifest PATH", "Path to golden digest manifest JSON.") { |v| options["golden_manifest"] = v }
    opts.on("--report PATH", "Output JSON report path.") { |v| options["report"] = v }

    opts.on("--update-golden", "Update/create golden entries from current outputs.") { options["update_golden"] = true }
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


def sha256_of_file(path)
  Digest::SHA256.file(path.to_s).hexdigest
end


def load_manifest(path)
  fallback_entries = DEFAULT_VISUAL_GOLDEN_ENTRIES.map(&:dup)

  unless path.exist?
    return {
      "schema_version" => 1,
      "entries" => fallback_entries
    }
  end

  begin
    parsed = JSON.parse(path.read)
  rescue StandardError
    return {
      "schema_version" => 1,
      "entries" => fallback_entries
    }
  end

  unless parsed.is_a?(Hash)
    return {
      "schema_version" => 1,
      "entries" => fallback_entries
    }
  end

  entries = parsed["entries"]
  entries = fallback_entries unless entries.is_a?(Array)

  {
    "schema_version" => parsed.fetch("schema_version", 1).to_i,
    "entries" => entries
  }
end


def entry_match_score(entry, profile_id, output_kind, output_ext, input_stem)
  return -1 unless entry.fetch("profile_id", "").to_s.strip == profile_id
  return -1 unless entry.fetch("output_ext", "").to_s.strip.downcase == output_ext.downcase

  entry_kind = entry.fetch("output_kind", "").to_s.strip.downcase
  return -1 if !entry_kind.empty? and entry_kind != output_kind.downcase

  entry_stem = entry.fetch("input_stem", "").to_s.strip
  return 2 if !entry_stem.empty? and entry_stem == input_stem
  return -1 if !entry_stem.empty?

  1
end


def find_manifest_entry(entries, profile_id, output_kind, output_ext, input_stem)
  best = nil
  best_score = -1

  entries.each do |entry|
    next unless entry.is_a?(Hash)

    score = entry_match_score(entry, profile_id, output_kind, output_ext, input_stem)
    if score > best_score
      best = entry
      best_score = score
    end
  end

  best
end


def stable_sorted_entries(entries)
  entries.sort_by do |entry|
    [
      entry.fetch("profile_id", "").to_s,
      entry.fetch("output_kind", "").to_s,
      entry.fetch("output_ext", "").to_s,
      entry.fetch("input_stem", "").to_s
    ]
  end
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
    "stderr" => "visual runner timeout",
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


def run_validator_case(validator, input_path, profile_id, render_catalog, output_dir)
  cmd = [
    RbConfig.ruby,
    validator.to_s,
    "--json",
    "--input",
    input_path.to_s,
    "--render-gephi",
    "--require-render",
    "--render-profile",
    profile_id,
    "--render-profile-catalog",
    render_catalog.to_s,
    "--render-output-dir",
    output_dir.to_s
  ]

  run = run_command(cmd, 240)
  payload = {
    "command" => cmd,
    "exit_code" => run["exit_code"].to_i,
    "stdout" => run["stdout"],
    "stderr" => run["stderr"],
    "json" => nil
  }
  payload["runner_error"] = run["error"] unless run["error"].nil?
  payload["runner_timeout"] = true if run["timeout"]

  unless run["stdout"].to_s.strip.empty?
    begin
      payload["json"] = JSON.parse(run["stdout"])
    rescue StandardError
      payload["json"] = nil
    end
  end

  [run["exit_code"].to_i, payload]
end


def extract_render_output_info(validator_payload)
  errors = []

  payload = validator_payload["json"]
  unless payload.is_a?(Hash)
    errors << "Validator did not emit parseable JSON output."
    return [nil, "", errors]
  end

  reports = payload["reports"]
  unless reports.is_a?(Array) and reports.size == 1
    errors << "Validator JSON must contain exactly one report per visual case."
    return [nil, "", errors]
  end

  report = reports[0]
  unless report.is_a?(Hash)
    errors << "Validator report has invalid shape."
    return [nil, "", errors]
  end

  unless report["valid"]
    errors << "Validator marked render case as invalid."
    if report["errors"].is_a?(Array)
      report["errors"].each { |err| errors << "validator: #{err}" }
    end
    return [nil, "", errors]
  end

  stats = report["stats"]
  unless stats.is_a?(Hash)
    errors << "Validator report is missing stats."
    return [nil, "", errors]
  end

  render_output = stats.fetch("render_output", "").to_s.strip
  if render_output.empty?
    errors << "Validator report missing stats.render_output."
    return [nil, "", errors]
  end

  output_kind = stats.fetch("render_output_kind", "").to_s.strip.downcase
  if output_kind.empty?
    errors << "Validator report missing stats.render_output_kind."
    return [nil, "", errors]
  end

  [Pathname.new(render_output), output_kind, errors]
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

  inputs = collect_inputs(args["inputs"], args["input_globs"])
  profiles = args["profiles"].map { |v| v.to_s.strip }.reject(&:empty?).uniq.sort

  if inputs.empty?
    warn("error: no inputs provided (use --input or --input-glob).")
    return 3
  end

  if profiles.empty?
    warn("error: at least one --profile is required.")
    return 3
  end

  validator_path = Pathname.new(args["validator"]).expand_path
  render_catalog = Pathname.new(args["render_profile_catalog"]).expand_path
  output_base = Pathname.new(args["render_output_dir"]).expand_path
  report_path = Pathname.new(args["report"]).expand_path
  manifest_path = Pathname.new(args["golden_manifest"]).expand_path

  manifest = load_manifest(manifest_path)
  entries = manifest.fetch("entries", [])
  manifest_updated = false

  results = []
  total_failures = 0

  profiles.each do |profile_id|
    profile_out = output_base.join(sanitize_id(profile_id))
    profile_out.mkpath

    inputs.each do |input_path|
      case_item = {
        "profile_id" => profile_id,
        "input" => input_path.to_s,
        "passed" => false,
        "errors" => [],
        "warnings" => [],
        "validator" => {},
        "artifact" => {}
      }

      unless input_path.exist?
        case_item["errors"] << "Input file does not exist."
        results << case_item
        total_failures += 1
        next
      end

      rc, validator_payload = run_validator_case(
          validator_path,
          input_path,
          profile_id,
          render_catalog,
          profile_out)
      case_item["validator"] = validator_payload
      case_item["errors"] << "Validator exit code: #{rc}." if rc != 0

      artifact_path, output_kind, extract_errors = extract_render_output_info(validator_payload)
      case_item["errors"].concat(extract_errors)

      if artifact_path.nil?
        results << case_item
        total_failures += 1
        next
      end

      unless artifact_path.exist?
        case_item["errors"] << "Rendered artifact does not exist: #{artifact_path}."
        results << case_item
        total_failures += 1
        next
      end

      output_ext = artifact_path.extname.downcase.sub(/\A\./, "")
      input_stem = input_path.basename(input_path.extname).to_s
      actual_sha = sha256_of_file(artifact_path)
      actual_bytes = artifact_path.size

      case_item["artifact"] = {
        "path" => artifact_path.to_s,
        "output_kind" => output_kind,
        "output_ext" => output_ext,
        "sha256" => actual_sha,
        "bytes" => actual_bytes
      }

      expected = find_manifest_entry(entries, profile_id, output_kind, output_ext, input_stem)

      if expected.nil?
        if args["update_golden"]
          entries << {
            "profile_id" => profile_id,
            "output_kind" => output_kind,
            "output_ext" => output_ext,
            "sha256" => actual_sha,
            "bytes" => actual_bytes
          }
          manifest_updated = true
          case_item["warnings"] << "Golden entry created from current artifact."
          case_item["passed"] = true
        else
          case_item["errors"] << "Golden entry not found for profile/output. Use --update-golden to create it."
        end
      else
        expected_sha = expected.fetch("sha256", "").to_s.strip.downcase
        expected_bytes = expected.fetch("bytes", -1).to_i
        expected_kind = expected.fetch("output_kind", "").to_s.strip.downcase

        case_item["artifact"]["expected_sha256"] = expected_sha
        case_item["artifact"]["expected_bytes"] = expected_bytes
        case_item["artifact"]["expected_output_kind"] = expected_kind

        if expected_sha != actual_sha
          if args["update_golden"]
            expected["sha256"] = actual_sha
            manifest_updated = true
            case_item["warnings"] << "Golden sha256 updated from current artifact."
          else
            case_item["errors"] << "Golden hash mismatch (expected=#{expected_sha}, actual=#{actual_sha})."
          end
        end

        if !expected_kind.empty? and expected_kind != output_kind
          if args["update_golden"]
            expected["output_kind"] = output_kind
            manifest_updated = true
            case_item["warnings"] << "Golden output_kind updated from current artifact."
          else
            case_item["errors"] << "Golden output kind mismatch (expected=#{expected_kind}, actual=#{output_kind})."
          end
        end

        if expected_bytes != actual_bytes
          if args["update_golden"]
            expected["bytes"] = actual_bytes
            manifest_updated = true
            case_item["warnings"] << "Golden size updated from current artifact."
          else
            case_item["errors"] << "Golden size mismatch (expected=#{expected_bytes}, actual=#{actual_bytes})."
          end
        end

        case_item["passed"] = case_item["errors"].empty?
      end

      total_failures += 1 unless case_item["passed"]
      results << case_item
    end
  end

  # Write the manifest when entries changed OR when --update-golden was
  # requested and the file does not yet exist on disk (the in-memory
  # defaults matched, but the caller expects a concrete manifest file).
  should_write = manifest_updated ||
                 (args["update_golden"] and not manifest_path.exist?)

  if should_write
    manifest["schema_version"] = 1
    manifest["entries"] = stable_sorted_entries(entries)
    manifest_path.dirname.mkpath
    manifest_path.write(JSON.pretty_generate(manifest) + "\n")
    manifest_updated = true
  end

  overall_passed = total_failures == 0
  report = {
    "visual_runner" => "planarity_certificate_ci_visual_diff.rb",
    "overall_passed" => overall_passed,
    "num_cases" => results.size,
    "num_failures" => total_failures,
    "manifest_path" => manifest_path.to_s,
    "manifest_updated" => manifest_updated,
    "settings" => {
      "validator" => validator_path.to_s,
      "render_profile_catalog" => render_catalog.to_s,
      "render_output_dir" => output_base.to_s,
      "profiles" => profiles,
      "num_inputs" => inputs.size,
      "update_golden" => !!args["update_golden"]
    },
    "results" => results
  }

  report_path.dirname.mkpath
  report_path.write(JSON.pretty_generate(report))

  if args["print_summary"]
    puts "cases=#{results.size}"
    puts "failures=#{total_failures}"
    puts "manifest_updated=#{manifest_updated}"
    puts "report=#{report_path}"
    puts "overall_passed=#{overall_passed}"
  end

  overall_passed ? 0 : 2
end


exit(main(ARGV))
