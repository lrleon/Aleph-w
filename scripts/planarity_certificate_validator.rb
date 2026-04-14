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
# Validate Aleph non-planar certificate exchange artifacts (GraphML / GEXF).
#
# Exit codes:
# - 0: all inputs valid
# - 2: at least one input invalid
# - 3: runtime/usage error

require "json"
require "optparse"
require "open3"
require "pathname"
require "rbconfig"
require "rexml/document"
require "shellwords"
require "timeout"

def default_gephi_executable_name
  host_os = RbConfig::CONFIG.fetch("host_os", "").downcase
  return "gephi.exe" if host_os.include?("cygwin") or host_os.include?("mingw") or host_os.include?("mswin")

  "gephi"
end


def gephi_path_probe_template(executable_name)
  "#{Shellwords.escape(RbConfig.ruby)} -e \"name = ARGV.fetch(0); found = ENV.fetch('PATH', '').split(File::PATH_SEPARATOR).any? do |dir| path = File.join(dir, name); File.file?(path) and File.executable?(path) end; exit(found ? 0 : 1)\" #{executable_name}"
end


DEFAULT_GEPHI_TEMPLATES = [
  {
    "id" => "portable.ruby-file-exists",
    "os" => "any",
    "gephi_version" => "n/a",
    "description" => "Portable adapter command used in CI/tests; checks that input file exists.",
    "cmd" => "#{Shellwords.escape(RbConfig.ruby)} -e \"File.stat(ARGV[0])\" {input}"
  },
  {
    "id" => "linux.gephi-0.10.smoke",
    "os" => "linux",
    "gephi_version" => "0.10.x",
    "description" => "Gephi 0.10.x PATH availability probe (Linux).",
    "cmd" => gephi_path_probe_template("gephi")
  },
  {
    "id" => "linux.gephi-0.10.headless-import",
    "os" => "linux",
    "gephi_version" => "0.10.x",
    "description" => "Gephi 0.10.x headless import check (Linux).",
    "cmd" => "gephi --headless --import {input}"
  },
  {
    "id" => "linux.gephi-0.9.smoke",
    "os" => "linux",
    "gephi_version" => "0.9.x",
    "description" => "Gephi 0.9.x PATH availability probe (Linux).",
    "cmd" => gephi_path_probe_template("gephi")
  },
  {
    "id" => "linux.gephi-0.9.headless-import",
    "os" => "linux",
    "gephi_version" => "0.9.x",
    "description" => "Gephi 0.9.x headless import check (Linux).",
    "cmd" => "gephi --headless --import {input}"
  },
  {
    "id" => "macos.gephi-0.10.smoke",
    "os" => "macos",
    "gephi_version" => "0.10.x",
    "description" => "Gephi 0.10.x PATH availability probe (macOS).",
    "cmd" => gephi_path_probe_template("gephi")
  },
  {
    "id" => "macos.gephi-0.10.headless-import",
    "os" => "macos",
    "gephi_version" => "0.10.x",
    "description" => "Gephi 0.10.x headless import check (macOS).",
    "cmd" => "gephi --headless --import {input}"
  },
  {
    "id" => "macos.gephi-0.9.smoke",
    "os" => "macos",
    "gephi_version" => "0.9.x",
    "description" => "Gephi 0.9.x PATH availability probe (macOS).",
    "cmd" => gephi_path_probe_template("gephi")
  },
  {
    "id" => "macos.gephi-0.9.headless-import",
    "os" => "macos",
    "gephi_version" => "0.9.x",
    "description" => "Gephi 0.9.x headless import check (macOS).",
    "cmd" => "gephi --headless --import {input}"
  },
  {
    "id" => "windows.gephi-0.10.smoke",
    "os" => "windows",
    "gephi_version" => "0.10.x",
    "description" => "Gephi 0.10.x PATH availability probe (Windows).",
    "cmd" => gephi_path_probe_template("gephi.exe")
  },
  {
    "id" => "windows.gephi-0.10.headless-import",
    "os" => "windows",
    "gephi_version" => "0.10.x",
    "description" => "Gephi 0.10.x headless import check (Windows).",
    "cmd" => "gephi.exe --headless --import {input}"
  },
  {
    "id" => "windows.gephi-0.9.smoke",
    "os" => "windows",
    "gephi_version" => "0.9.x",
    "description" => "Gephi 0.9.x PATH availability probe (Windows).",
    "cmd" => gephi_path_probe_template("gephi.exe")
  },
  {
    "id" => "windows.gephi-0.9.headless-import",
    "os" => "windows",
    "gephi_version" => "0.9.x",
    "description" => "Gephi 0.9.x headless import check (Windows).",
    "cmd" => "gephi.exe --headless --import {input}"
  }
].freeze

DEFAULT_GEPHI_RENDER_PROFILES = [
  {
    "id" => "portable.python-render-svg",
    "os" => "any",
    "gephi_version" => "n/a",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Portable deterministic SVG render profile for CI/testing.",
    "cmd" => "python3 -c \"import pathlib,sys; pathlib.Path(sys.argv[2]).write_text('<svg xmlns=\\\"http://www.w3.org/2000/svg\\\" width=\\\"64\\\" height=\\\"64\\\"><circle cx=\\\"32\\\" cy=\\\"32\\\" r=\\\"20\\\"/></svg>', encoding='utf-8')\" {input} {output}"
  },
  {
    "id" => "portable.python-render-pdf",
    "os" => "any",
    "gephi_version" => "n/a",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Portable deterministic PDF render profile for CI/testing.",
    "cmd" => "python3 -c \"import pathlib,sys; pathlib.Path(sys.argv[2]).write_bytes(b'%PDF-1.4\\n1 0 obj<<>>endobj\\ntrailer<<>>\\n%%EOF\\n')\" {input} {output}"
  },
  {
    "id" => "portable.ruby-render-svg",
    "os" => "any",
    "gephi_version" => "n/a",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Portable deterministic SVG render profile implemented with Ruby.",
    "cmd" => "#{Shellwords.escape(RbConfig.ruby)} -e \"File.write(ARGV[1], '<svg xmlns=\\\"http://www.w3.org/2000/svg\\\" width=\\\"64\\\" height=\\\"64\\\"><circle cx=\\\"32\\\" cy=\\\"32\\\" r=\\\"20\\\"/></svg>')\" {input} {output}"
  },
  {
    "id" => "portable.ruby-render-pdf",
    "os" => "any",
    "gephi_version" => "n/a",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Portable deterministic PDF render profile implemented with Ruby.",
    "cmd" => "#{Shellwords.escape(RbConfig.ruby)} -e \"File.binwrite(ARGV[1], \\\"%PDF-1.4\\\\n1 0 obj<<>>endobj\\\\ntrailer<<>>\\\\n%%EOF\\\\n\\\")\" {input} {output}"
  },
  {
    "id" => "linux.gephi-0.10.render-svg",
    "os" => "linux",
    "gephi_version" => "0.10.x",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Gephi 0.10.x headless SVG export profile (Linux packages).",
    "cmd" => "gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "linux.gephi-0.10.render-pdf",
    "os" => "linux",
    "gephi_version" => "0.10.x",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Gephi 0.10.x headless PDF export profile (Linux packages).",
    "cmd" => "gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "linux.gephi-0.9.render-svg",
    "os" => "linux",
    "gephi_version" => "0.9.x",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Gephi 0.9.x headless SVG export profile (Linux packages).",
    "cmd" => "gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "linux.gephi-0.9.render-pdf",
    "os" => "linux",
    "gephi_version" => "0.9.x",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Gephi 0.9.x headless PDF export profile (Linux packages).",
    "cmd" => "gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "macos.gephi-0.10.render-svg",
    "os" => "macos",
    "gephi_version" => "0.10.x",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Gephi 0.10.x app bundle CLI SVG export profile (macOS).",
    "cmd" => "/Applications/Gephi.app/Contents/MacOS/gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "macos.gephi-0.10.render-pdf",
    "os" => "macos",
    "gephi_version" => "0.10.x",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Gephi 0.10.x app bundle CLI PDF export profile (macOS).",
    "cmd" => "/Applications/Gephi.app/Contents/MacOS/gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "macos.gephi-0.9.render-svg",
    "os" => "macos",
    "gephi_version" => "0.9.x",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Gephi 0.9.x app bundle CLI SVG export profile (macOS).",
    "cmd" => "/Applications/Gephi.app/Contents/MacOS/gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "macos.gephi-0.9.render-pdf",
    "os" => "macos",
    "gephi_version" => "0.9.x",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Gephi 0.9.x app bundle CLI PDF export profile (macOS).",
    "cmd" => "/Applications/Gephi.app/Contents/MacOS/gephi --headless --import {input} --export {output}"
  },
  {
    "id" => "windows.gephi-0.10.render-svg",
    "os" => "windows",
    "gephi_version" => "0.10.x",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Gephi 0.10.x CLI SVG export profile (Windows installers).",
    "cmd" => "gephi.exe --headless --import {input} --export {output}"
  },
  {
    "id" => "windows.gephi-0.10.render-pdf",
    "os" => "windows",
    "gephi_version" => "0.10.x",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Gephi 0.10.x CLI PDF export profile (Windows installers).",
    "cmd" => "gephi.exe --headless --import {input} --export {output}"
  },
  {
    "id" => "windows.gephi-0.9.render-svg",
    "os" => "windows",
    "gephi_version" => "0.9.x",
    "output_kind" => "svg",
    "output_ext" => "svg",
    "description" => "Gephi 0.9.x CLI SVG export profile (Windows installers).",
    "cmd" => "gephi.exe --headless --import {input} --export {output}"
  },
  {
    "id" => "windows.gephi-0.9.render-pdf",
    "os" => "windows",
    "gephi_version" => "0.9.x",
    "output_kind" => "pdf",
    "output_ext" => "pdf",
    "description" => "Gephi 0.9.x CLI PDF export profile (Windows installers).",
    "cmd" => "gephi.exe --headless --import {input} --export {output}"
  }
].freeze


def lname(tag)
  tag.to_s.split(":").last
end


def first_child(parent, name)
  return nil if parent.nil?

  parent.elements.each do |child|
    return child if lname(child.name) == name
  end
  nil
end


def children(parent, name)
  return [] if parent.nil?

  out = []
  parent.elements.each do |child|
    out << child if lname(child.name) == name
  end
  out
end


def graphml_data_map(element)
  data = {}
  children(element, "data").each do |child|
    key = child.attributes["key"].to_s.strip
    next if key.empty?

    data[key] = child.text.to_s.strip
  end
  data
end


def gexf_attvalues_map(element)
  values = {}
  attvalues = first_child(element, "attvalues")
  return values if attvalues.nil?

  children(attvalues, "attvalue").each do |att|
    key = att.attributes["for"].to_s
    value = att.attributes["value"].to_s
    values[key] = value unless key.empty?
  end

  values
end


def validate_graphml(root)
  stats = {}
  errors = []
  warnings = []

  if lname(root.name) != "graphml"
    errors << "Root element is not <graphml>."
    return [stats, errors, warnings]
  end

  graph = first_child(root, "graph")
  if graph.nil?
    errors << "Missing <graph> element."
    return [stats, errors, warnings]
  end

  edgedefault = graph.attributes["edgedefault"].to_s
  warnings << "GraphML edgedefault is not 'undirected'." if !edgedefault.empty? and edgedefault != "undirected"

  nodes = children(graph, "node")
  edges = children(graph, "edge")

  node_ids = {}
  nodes.each do |node|
    nid = node.attributes["id"].to_s.strip
    if nid.empty?
      errors << "Node with empty id."
      next
    end

    errors << "Duplicated node id: #{nid}." if node_ids.key?(nid)
    node_ids[nid] = true
  end

  obstruction_edges = 0
  path_edges = 0

  edges.each do |edge|
    src = edge.attributes["source"].to_s.strip
    tgt = edge.attributes["target"].to_s.strip

    if src.empty? or tgt.empty?
      errors << "Edge with empty source/target."
      next
    end

    errors << "Edge endpoint not found in node set: #{src}->#{tgt}." if !node_ids.key?(src) or !node_ids.key?(tgt)

    data = graphml_data_map(edge)
    kind = data.fetch("k_edge_kind", "").strip

    if kind == "obstruction"
      obstruction_edges += 1
    elsif kind == "path"
      path_edges += 1
      path_id = data.fetch("k_edge_path_id", "").strip
      if !path_id.empty? and !(path_id =~ /\A-?\d+\z/)
        errors << "Invalid path id value: #{path_id}."
      end
    elsif !kind.empty?
      warnings << "Unknown edge kind value: #{kind}."
    end
  end

  stats["num_nodes"] = nodes.size
  stats["num_edges"] = edges.size
  stats["num_obstruction_edges"] = obstruction_edges
  stats["num_path_edges"] = path_edges
  stats["edgedefault"] = edgedefault.empty? ? "unknown" : edgedefault

  errors << "GraphML has no nodes." if nodes.empty?
  errors << "GraphML has no edges." if edges.empty?
  errors << "GraphML contains no obstruction edges." if obstruction_edges == 0

  [stats, errors, warnings]
end


def validate_gexf(root)
  stats = {}
  errors = []
  warnings = []

  if lname(root.name) != "gexf"
    errors << "Root element is not <gexf>."
    return [stats, errors, warnings]
  end

  graph = first_child(root, "graph")
  if graph.nil?
    errors << "Missing <graph> element."
    return [stats, errors, warnings]
  end

  default_edge_type = graph.attributes["defaultedgetype"].to_s
  warnings << "GEXF defaultedgetype is not 'undirected'." if !default_edge_type.empty? and default_edge_type != "undirected"

  nodes_container = first_child(graph, "nodes")
  edges_container = first_child(graph, "edges")
  if nodes_container.nil? or edges_container.nil?
    errors << "Missing <nodes> or <edges> container."
    return [stats, errors, warnings]
  end

  nodes = children(nodes_container, "node")
  edges = children(edges_container, "edge")

  node_ids = {}
  nodes.each do |node|
    nid = node.attributes["id"].to_s.strip
    if nid.empty?
      errors << "Node with empty id."
      next
    end

    errors << "Duplicated node id: #{nid}." if node_ids.key?(nid)
    node_ids[nid] = true
  end

  obstruction_edges = 0
  path_edges = 0

  edges.each do |edge|
    src = edge.attributes["source"].to_s.strip
    tgt = edge.attributes["target"].to_s.strip

    if src.empty? or tgt.empty?
      errors << "Edge with empty source/target."
      next
    end

    errors << "Edge endpoint not found in node set: #{src}->#{tgt}." if !node_ids.key?(src) or !node_ids.key?(tgt)

    values = gexf_attvalues_map(edge)
    kind = values.fetch("e_kind", "").strip

    if kind == "obstruction"
      obstruction_edges += 1
    elsif kind == "path"
      path_edges += 1
      path_id = values.fetch("e_path_id", "").strip
      if !path_id.empty? and !(path_id =~ /\A-?\d+\z/)
        errors << "Invalid path id value: #{path_id}."
      end
    elsif !kind.empty?
      warnings << "Unknown edge kind value: #{kind}."
    end
  end

  stats["num_nodes"] = nodes.size
  stats["num_edges"] = edges.size
  stats["num_obstruction_edges"] = obstruction_edges
  stats["num_path_edges"] = path_edges
  stats["defaultedgetype"] = default_edge_type.empty? ? "unknown" : default_edge_type

  errors << "GEXF has no nodes." if nodes.empty?
  errors << "GEXF has no edges." if edges.empty?
  errors << "GEXF contains no obstruction edges." if obstruction_edges == 0

  [stats, errors, warnings]
end


def command_in_path?(cmd)
  ENV.fetch("PATH", "").split(File::PATH_SEPARATOR).any? do |dir|
    path = File.join(dir, cmd)
    File.file?(path) and File.executable?(path)
  end
end


def resolve_python
  env_python = ENV.fetch("ALEPH_PYTHON", "").strip
  return env_python unless env_python.empty?
  return "python3" if command_in_path?("python3")
  return "python" if command_in_path?("python")
  nil
end


def resolve_executable(cmd)
  raw = cmd.to_s
  return nil if raw.empty?

  alt_sep = defined?(File::ALT_SEPARATOR) ? File::ALT_SEPARATOR : nil
  has_separator = raw.include?(File::SEPARATOR) || (alt_sep && raw.include?(alt_sep))

  if has_separator
    return raw if File.file?(raw) and File.executable?(raw)
    return nil
  end

  host_os = RbConfig::CONFIG.fetch("host_os", "").downcase
  win_exts = (host_os.include?("cygwin") or host_os.include?("mingw") or host_os.include?("mswin")) \
               ? [".exe", ".bat", ".cmd", ".com"] : []

  ENV.fetch("PATH", "").split(File::PATH_SEPARATOR).each do |dir|
    candidate = File.join(dir, raw)
    return candidate if File.file?(candidate) and File.executable?(candidate)

    win_exts.each do |ext|
      with_ext = candidate + ext
      return with_ext if File.file?(with_ext) and File.executable?(with_ext)
    end
  end

  nil
end


def run_command(cmd, timeout_sec)
  begin
    stdout = ""
    stderr = ""
    status = nil

    Timeout.timeout(timeout_sec) do
      stdout, stderr, status = Open3.capture3(*cmd)
    end

    return {
      "timeout" => false,
      "error" => nil,
      "stdout" => stdout,
      "stderr" => stderr,
      "exit_code" => status.exitstatus
    }
  rescue Timeout::Error
    return {
      "timeout" => true,
      "error" => nil,
      "stdout" => "",
      "stderr" => "",
      "exit_code" => nil
    }
  rescue StandardError => e
    return {
      "timeout" => false,
      "error" => e.message,
      "stdout" => "",
      "stderr" => "",
      "exit_code" => nil
    }
  end
end


def validate_with_networkx(path, fmt, require_networkx)
  stats = {}
  errors = []
  warnings = []

  python = resolve_python
  if python.nil?
    msg = "NetworkX is required but Python is not available."
    if require_networkx
      errors << msg
    else
      warnings << "Python not available; skipping NetworkX validation."
    end
    return [stats, errors, warnings]
  end

  check = run_command(
      [python, "-c", "import importlib.util,sys; sys.exit(0 if importlib.util.find_spec('networkx') else 1)"],
      30)

  if check["timeout"]
    msg = "NetworkX availability check timed out."
    if require_networkx
      errors << msg
    else
      warnings << msg
    end
    return [stats, errors, warnings]
  end

  if check["error"]
    msg = "NetworkX availability check failed: #{check['error']}."
    if require_networkx
      errors << msg
    else
      warnings << msg
    end
    return [stats, errors, warnings]
  end

  if check["exit_code"] != 0
    if require_networkx
      errors << "NetworkX is required but not available."
    else
      warnings << "NetworkX not available; skipping NetworkX validation."
    end
    return [stats, errors, warnings]
  end

  load_code = <<~PY
    import json,sys
    import networkx as nx
    fmt = sys.argv[1]
    path = sys.argv[2]
    if fmt == "graphml":
      g = nx.read_graphml(path)
    elif fmt == "gexf":
      g = nx.read_gexf(path)
    else:
      raise RuntimeError(f"unsupported format: {fmt}")
    print(json.dumps({"networkx_num_nodes": int(g.number_of_nodes()), "networkx_num_edges": int(g.number_of_edges())}))
  PY

  load_result = run_command([python, "-c", load_code, fmt, path.to_s], 120)
  if load_result["timeout"]
    errors << "NetworkX load timed out."
    return [stats, errors, warnings]
  end

  if load_result["error"]
    errors << "NetworkX load failed: #{load_result['error']}."
    return [stats, errors, warnings]
  end

  if load_result["exit_code"] != 0
    msg = load_result["stderr"].to_s.strip
    msg = "NetworkX load failed." if msg.empty?
    errors << "NetworkX load failed: #{msg}."
    return [stats, errors, warnings]
  end

  begin
    parsed = JSON.parse(load_result["stdout"].to_s)
    stats["networkx_num_nodes"] = parsed.fetch("networkx_num_nodes", 0).to_i
    stats["networkx_num_edges"] = parsed.fetch("networkx_num_edges", 0).to_i
  rescue StandardError => e
    errors << "NetworkX output parse failed: #{e.message}."
    return [stats, errors, warnings]
  end

  errors << "NetworkX loaded graph has no nodes." if stats["networkx_num_nodes"] == 0
  errors << "NetworkX loaded graph has no edges." if stats["networkx_num_edges"] == 0

  [stats, errors, warnings]
end


def expand_command_template(template, replacements, error_prefix)
  tokens = Shellwords.split(template)
  cmd = tokens.map do |token|
    expanded = token.dup
    replacements.each do |k, v|
      expanded = expanded.gsub(k, v)
    end
    expanded
  end
  [cmd, ""]
rescue StandardError => e
  [[], "#{error_prefix}: #{e.message}."]
end


def default_template_catalog_path
  Pathname.new(__FILE__).realpath.dirname.join("planarity_gephi_templates.json")
end


def find_default_smoke_probe_template(catalog_path)
  # Automatically select a smoke probe template for PATH probing
  # when --gephi-cmd is not specified
  templates, warnings = load_gephi_templates(catalog_path)

  # Determine the current OS
  host_os = RbConfig::CONFIG.fetch("host_os", "").downcase
  os_type = if host_os.include?("cygwin") or host_os.include?("mingw") or host_os.include?("mswin")
              "windows"
            elsif host_os.include?("darwin")
              "macos"
            else
              "linux"
            end

  # Look for a smoke probe template for this OS (prefer newest Gephi version)
  smoke_templates = templates.values.select do |t|
    t_id = t.fetch("id", "").to_s.downcase
    t_os = t.fetch("os", "").to_s.downcase
    t_id.include?("smoke") and (t_os == os_type or t_os == "any")
  end

  # Sort by version tuple (descending) and return the first match
  selected = smoke_templates.max_by do |t|
    [gephi_version_sort_key(t.fetch("gephi_version", "0")), t.fetch("id", "")]
  end

  [selected, warnings]
end


def gephi_version_sort_key(value)
  segments = value.to_s.scan(/\d+/).map(&:to_i)
  segments = [-1] if segments.empty?
  segments
end

def default_render_profile_catalog_path
  Pathname.new(__FILE__).realpath.dirname.join("planarity_gephi_render_profiles.json")
end


def load_gephi_templates(catalog_path)
  warnings = []
  templates = {}
  fallback_entries = DEFAULT_GEPHI_TEMPLATES

  unless catalog_path.exist?
    warnings << "Gephi template catalog not found: #{catalog_path}. Using built-in defaults."
    fallback_entries.each { |item| templates[item["id"]] = item.dup }
    return [templates, warnings]
  end

  begin
    data = JSON.parse(catalog_path.read)
  rescue StandardError => e
    warnings << "Unable to parse Gephi template catalog: #{e.message}. Using built-in defaults."
    fallback_entries.each { |item| templates[item["id"]] = item.dup }
    return [templates, warnings]
  end

  entries = data["templates"]
  unless entries.is_a?(Array)
    warnings << "Gephi template catalog has invalid 'templates' format. Using built-in defaults."
    fallback_entries.each { |item| templates[item["id"]] = item.dup }
    return [templates, warnings]
  end

  entries.each do |item|
    next unless item.is_a?(Hash)

    tid = item.fetch("id", "").to_s.strip
    cmd = item.fetch("cmd", "").to_s.strip
    next if tid.empty? or cmd.empty?

    templates[tid] = item
  end

  [templates, warnings]
end


def load_gephi_render_profiles(catalog_path)
  warnings = []
  profiles = {}
  fallback_entries = DEFAULT_GEPHI_RENDER_PROFILES

  unless catalog_path.exist?
    warnings << "Gephi render profile catalog not found: #{catalog_path}. Using built-in defaults."
    fallback_entries.each { |item| profiles[item["id"]] = item.dup }
    return [profiles, warnings]
  end

  begin
    data = JSON.parse(catalog_path.read)
  rescue StandardError => e
    warnings << "Unable to parse Gephi render profile catalog: #{e.message}. Using built-in defaults."
    fallback_entries.each { |item| profiles[item["id"]] = item.dup }
    return [profiles, warnings]
  end

  entries = data["profiles"]
  unless entries.is_a?(Array)
    warnings << "Gephi render profile catalog has invalid 'profiles' format. Using built-in defaults."
    fallback_entries.each { |item| profiles[item["id"]] = item.dup }
    return [profiles, warnings]
  end

  entries.each do |item|
    next unless item.is_a?(Hash)

    pid = item.fetch("id", "").to_s.strip
    cmd = item.fetch("cmd", "").to_s.strip
    kind = item.fetch("output_kind", "").to_s.strip.downcase
    ext = item.fetch("output_ext", "").to_s.strip.downcase
    next if pid.empty? or cmd.empty? or kind.empty? or ext.empty?

    profiles[pid] = item
  end

  [profiles, warnings]
end


def resolve_gephi_template(template_id, catalog_path)
  warnings = []
  errors = []

  templates, load_warnings = load_gephi_templates(catalog_path)
  warnings.concat(load_warnings)

  return ["", warnings, errors] if template_id.empty?

  unless templates.key?(template_id)
    errors << "Gephi template id not found: #{template_id} (catalog: #{catalog_path})."
    return ["", warnings, errors]
  end

  template = templates[template_id]
  cmd = template.fetch("cmd", "").to_s.strip
  if cmd.empty?
    errors << "Gephi template '#{template_id}' has empty command."
    return ["", warnings, errors]
  end

  [cmd, warnings, errors]
end


def resolve_gephi_render_profile(profile_id, catalog_path)
  warnings = []
  errors = []

  profiles, load_warnings = load_gephi_render_profiles(catalog_path)
  warnings.concat(load_warnings)

  return [{}, warnings, errors] if profile_id.empty?

  unless profiles.key?(profile_id)
    errors << "Gephi render profile id not found: #{profile_id} (catalog: #{catalog_path})."
    return [{}, warnings, errors]
  end

  profile = profiles[profile_id]
  cmd = profile.fetch("cmd", "").to_s.strip
  kind = profile.fetch("output_kind", "").to_s.strip.downcase
  ext = profile.fetch("output_ext", "").to_s.strip.downcase

  if cmd.empty? or kind.empty? or ext.empty?
    errors << "Gephi render profile '#{profile_id}' is missing cmd/kind/ext."
    return [{}, warnings, errors]
  end

  [profile, warnings, errors]
end


def list_gephi_templates(catalog_path, output_json, os_filter)
  templates, warnings = load_gephi_templates(catalog_path)

  selected = templates.keys.sort.map { |id| templates[id] }.select do |item|
    if os_filter != "any"
      item_os = item.fetch("os", "").to_s.strip.downcase
      next false unless ["any", os_filter].include?(item_os)
    end
    true
  end

  payload = {
    "catalog_path" => catalog_path.to_s,
    "num_templates" => selected.size,
    "warnings" => warnings,
    "templates" => selected
  }

  if output_json
    puts JSON.pretty_generate(payload)
  else
    puts "catalog=#{catalog_path}"
    warnings.each { |warning| puts "! warning: #{warning}" }
    selected.each do |item|
      puts "- #{item['id']} | os=#{item['os']} | version=#{item['gephi_version']} | #{item['description']}"
    end
  end

  0
end


def list_gephi_render_profiles(catalog_path, output_json, os_filter)
  profiles, warnings = load_gephi_render_profiles(catalog_path)

  selected = profiles.keys.sort.map { |id| profiles[id] }.select do |item|
    if os_filter != "any"
      item_os = item.fetch("os", "").to_s.strip.downcase
      next false unless ["any", os_filter].include?(item_os)
    end
    true
  end

  payload = {
    "catalog_path" => catalog_path.to_s,
    "num_profiles" => selected.size,
    "warnings" => warnings,
    "profiles" => selected
  }

  if output_json
    puts JSON.pretty_generate(payload)
  else
    puts "catalog=#{catalog_path}"
    warnings.each { |warning| puts "! warning: #{warning}" }
    selected.each do |item|
      puts "- #{item['id']} | os=#{item['os']} | version=#{item['gephi_version']} | kind=#{item['output_kind']} | #{item['description']}"
    end
  end

  0
end


def validate_render_artifact(path, kind)
  errors = []
  warnings = []

  unless path.exist?
    errors << "Render artifact was not produced: #{path}."
    return [errors, warnings]
  end

  if path.size == 0
    errors << "Render artifact is empty: #{path}."
    return [errors, warnings]
  end

  if kind == "svg"
    begin
      root = REXML::Document.new(path.read).root
      if root.nil? or lname(root.name) != "svg"
        errors << "SVG artifact has invalid root tag: #{root.nil? ? 'nil' : lname(root.name)}."
      end
    rescue StandardError => e
      errors << "SVG artifact XML parse failed: #{e.message}."
    end
  elsif kind == "pdf"
    begin
      raw = path.binread
      errors << "PDF artifact does not start with %PDF- header." unless raw.start_with?("%PDF-")
    rescue StandardError => e
      errors << "PDF artifact read failed: #{e.message}."
    end
  end

  [errors, warnings]
end


def run_gephi_render_profile(path, profile, output_dir, require_render)
  stats = {}
  errors = []
  warnings = []

  cmd_template = profile.fetch("cmd", "").to_s.strip
  output_kind = profile.fetch("output_kind", "").to_s.strip.downcase
  output_ext = profile.fetch("output_ext", "").to_s.strip.downcase

  if cmd_template.empty? or output_kind.empty? or output_ext.empty?
    errors << "Invalid render profile (missing command/kind/ext)."
    return [stats, errors, warnings]
  end

  output_dir.mkpath
  out_file = output_dir.join("#{path.basename(path.extname)}.#{output_ext}")

  cmd, parse_error = expand_command_template(
      cmd_template,
      {
        "{input}" => path.to_s,
        "{output}" => out_file.to_s,
        "{basename}" => path.basename(path.extname).to_s,
        "{output_ext}" => output_ext
      },
      "Unable to parse render profile command")

  unless parse_error.empty?
    errors << parse_error
    return [stats, errors, warnings]
  end

  if cmd.empty?
    errors << "Render profile command resolved to empty command."
    return [stats, errors, warnings]
  end

  exe = cmd[0]
  exe_path = resolve_executable(exe)
  if exe_path.nil?
    msg = "Render command not found in PATH: #{exe}."
    if require_render
      errors << msg
    else
      warnings << "#{msg} Skipping render validation."
    end
    return [stats, errors, warnings]
  end

  stats["render_command"] = cmd.join(" ")
  stats["render_executable"] = exe_path
  stats["render_output"] = out_file.to_s
  stats["render_output_kind"] = output_kind

  run = run_command(cmd, 180)
  if run["timeout"]
    errors << "Render command timed out."
    return [stats, errors, warnings]
  end

  unless run["error"].nil?
    errors << "Render command execution failed: #{run['error']}."
    return [stats, errors, warnings]
  end

  stats["render_exit_code"] = run["exit_code"].to_i
  if run["exit_code"] != 0
    stderr = run["stderr"].to_s.strip
    stderr = "#{stderr[0, 240]}..." if stderr.size > 240
    errors << (stderr.empty? ? "Render command returned non-zero exit code." : "Render command returned non-zero exit code: #{stderr}.")
    return [stats, errors, warnings]
  end

  artifact_errors, artifact_warnings = validate_render_artifact(out_file, output_kind)
  errors.concat(artifact_errors)
  warnings.concat(artifact_warnings)
  stats["render_output_bytes"] = out_file.size if out_file.exist?

  [stats, errors, warnings]
end


def detect_format(path, explicit)
  return explicit unless explicit == "auto"

  suffix = path.extname.downcase
  return "graphml" if suffix == ".graphml"
  return "gexf" if suffix == ".gexf"
  "unknown"
end


def validate_with_gephi(path, cmd_template, require_gephi)
  stats = {}
  errors = []
  warnings = []

  template = cmd_template.to_s.strip
  if template.empty?
    exe = default_gephi_executable_name
    exe_path = resolve_executable(exe)
    if exe_path.nil?
      msg = "Gephi command not found in PATH: #{exe}."
      if require_gephi
        errors << msg
      else
        warnings << "#{msg} Skipping Gephi validation."
      end
      return [stats, errors, warnings]
    end

    stats["gephi_command"] = "path-probe #{exe}"
    stats["gephi_executable"] = exe_path
    stats["gephi_exit_code"] = 0
    return [stats, errors, warnings]
  end

  warnings << "Gephi command template has no {input} placeholder; running smoke check only." unless template.include?("{input}")

  cmd, parse_error = expand_command_template(
      template,
      {"{input}" => path.to_s},
      "Unable to parse --gephi-cmd template")
  unless parse_error.empty?
    errors << parse_error
    return [stats, errors, warnings]
  end

  if cmd.empty?
    errors << "Empty Gephi command after template expansion."
    return [stats, errors, warnings]
  end

  exe = cmd[0]
  exe_path = resolve_executable(exe)
  if exe_path.nil?
    msg = "Gephi command not found in PATH: #{exe}."
    if require_gephi
      errors << msg
    else
      warnings << "#{msg} Skipping Gephi validation."
    end
    return [stats, errors, warnings]
  end

  stats["gephi_command"] = cmd.join(" ")
  stats["gephi_executable"] = exe_path

  run = run_command(cmd, 120)
  if run["timeout"]
    errors << "Gephi command timed out."
    return [stats, errors, warnings]
  end

  unless run["error"].nil?
    errors << "Gephi command execution failed: #{run['error']}."
    return [stats, errors, warnings]
  end

  stats["gephi_exit_code"] = run["exit_code"].to_i
  if run["exit_code"] != 0
    stderr = run["stderr"].to_s.strip
    stderr = "#{stderr[0, 240]}..." if stderr.size > 240
    msg = stderr.empty? ? "Gephi command returned non-zero exit code." : "Gephi command returned non-zero exit code: #{stderr}."
    if require_gephi
      errors << msg
    else
      warnings << "#{msg} Skipping strict Gephi validation."
    end
  end

  [stats, errors, warnings]
end


def validate_one(path,
                 fmt,
                 use_networkx,
                 require_networkx,
                 use_gephi,
                 require_gephi,
                 gephi_cmd,
                 gephi_template,
                 resolved_template_cmd,
                 catalog_warnings,
                 catalog_errors,
                 use_gephi_render,
                 require_gephi_render,
                 render_profile_id,
                 resolved_render_profile,
                 render_catalog_warnings,
                 render_catalog_errors,
                 render_output_dir)
  report = {
    "path" => path.to_s,
    "format" => fmt,
    "valid" => false,
    "stats" => {},
    "errors" => [],
    "warnings" => []
  }

  unless path.exist?
    report["errors"] << "Input file does not exist."
    return report
  end

  unless ["graphml", "gexf"].include?(fmt)
    report["errors"] << "Unable to infer format; use --format."
    return report
  end

  begin
    root = REXML::Document.new(path.read).root
  rescue StandardError => e
    report["errors"] << "XML parse failed: #{e.message}."
    return report
  end

  stats, errors, warnings = if fmt == "graphml"
                              validate_graphml(root)
                            else
                              validate_gexf(root)
                            end

  report["stats"].merge!(stats)
  report["errors"].concat(errors)
  report["warnings"].concat(warnings)

  if use_networkx
    nx_stats, nx_errors, nx_warnings = validate_with_networkx(path, fmt, require_networkx)
    report["stats"].merge!(nx_stats)
    report["errors"].concat(nx_errors)
    report["warnings"].concat(nx_warnings)
  end

  if use_gephi
    report["warnings"].concat(catalog_warnings)
    report["errors"].concat(catalog_errors)

    resolved_cmd = gephi_template.empty? ? gephi_cmd : resolved_template_cmd
    gephi_stats, gephi_errors, gephi_warnings = validate_with_gephi(path, resolved_cmd, require_gephi)
    report["stats"].merge!(gephi_stats)
    report["errors"].concat(gephi_errors)
    report["warnings"].concat(gephi_warnings)
  end

  if use_gephi_render
    report["warnings"].concat(render_catalog_warnings)
    report["errors"].concat(render_catalog_errors)

    if render_profile_id.empty?
      report["errors"] << "Render mode requires --render-profile."
    elsif resolved_render_profile.empty?
      report["errors"] << "Render profile could not be resolved: #{render_profile_id}."
    else
      render_stats, render_errors, render_warnings = run_gephi_render_profile(
          path,
          resolved_render_profile,
          render_output_dir,
          require_gephi_render)
      report["stats"].merge!(render_stats)
      report["errors"].concat(render_errors)
      report["warnings"].concat(render_warnings)
    end
  end

  report["valid"] = report["errors"].empty?
  report
end


def parse_args(argv)
  options = {
    "inputs" => [],
    "format" => "auto",
    "networkx" => false,
    "require_networkx" => false,
    "gephi" => false,
    "require_gephi" => false,
    "gephi_cmd" => "",
    "gephi_template" => "",
    "gephi_template_catalog" => default_template_catalog_path.to_s,
    "list_gephi_templates" => false,
    "template_os" => "any",
    "render_gephi" => false,
    "require_render" => false,
    "render_profile" => "",
    "render_profile_catalog" => default_render_profile_catalog_path.to_s,
    "list_gephi_render_profiles" => false,
    "render_output_dir" => "/tmp/aleph_planarity_renders",
    "render_os" => "any",
    "json" => false
  }

  parser = OptionParser.new do |opts|
    opts.banner = "Usage: #{$PROGRAM_NAME} [options]"

    opts.on("--input PATH", "Input certificate file (repeatable).") { |v| options["inputs"] << v }
    opts.on("--format FORMAT", ["auto", "graphml", "gexf"], "Input format (default: auto).") { |v| options["format"] = v }
    opts.on("--networkx", "Enable optional NetworkX parse/load validation.") { options["networkx"] = true }
    opts.on("--require-networkx", "Fail if --networkx is requested but unavailable.") { options["require_networkx"] = true }
    opts.on("--gephi", "Enable optional Gephi CLI/toolkit validation or executable availability checks.") { options["gephi"] = true }
    opts.on("--require-gephi", "Fail if --gephi is requested but unavailable/fails.") { options["require_gephi"] = true }
    opts.on("--gephi-cmd CMD", "Gephi command template. Supports {input}.") { |v| options["gephi_cmd"] = v }
    opts.on("--gephi-template ID", "Template id from Gephi template catalog.") { |v| options["gephi_template"] = v.to_s }
    opts.on("--gephi-template-catalog PATH", "Path to Gephi template catalog JSON.") { |v| options["gephi_template_catalog"] = v }
    opts.on("--list-gephi-templates", "List templates and exit.") { options["list_gephi_templates"] = true }
    opts.on("--template-os OS", ["any", "linux", "macos", "windows"], "OS filter for --list-gephi-templates.") { |v| options["template_os"] = v }

    opts.on("--render-gephi", "Enable Gephi render profile execution.") { options["render_gephi"] = true }
    opts.on("--require-render", "Fail if render command/profile unavailable/fails.") { options["require_render"] = true }
    opts.on("--render-profile ID", "Render profile id from render profile catalog.") { |v| options["render_profile"] = v.to_s }
    opts.on("--render-profile-catalog PATH", "Path to Gephi render profile catalog JSON.") { |v| options["render_profile_catalog"] = v }
    opts.on("--list-gephi-render-profiles", "List render profiles and exit.") { options["list_gephi_render_profiles"] = true }
    opts.on("--render-output-dir PATH", "Directory where render artifacts are written.") { |v| options["render_output_dir"] = v }
    opts.on("--render-os OS", ["any", "linux", "macos", "windows"], "OS filter for --list-gephi-render-profiles.") { |v| options["render_os"] = v }

    opts.on("--json", "Emit JSON report.") { options["json"] = true }
  end

  parser.parse!(argv)
  options
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

  catalog_path = Pathname.new(args["gephi_template_catalog"])
  if args["list_gephi_templates"]
    return list_gephi_templates(
        catalog_path,
        args["json"],
        args["template_os"])
  end

  render_catalog_path = Pathname.new(args["render_profile_catalog"])
  if args["list_gephi_render_profiles"]
    return list_gephi_render_profiles(
        render_catalog_path,
        args["json"],
        args["render_os"])
  end

  resolved_template_cmd = ""
  catalog_warnings = []
  catalog_errors = []

  # If --gephi is specified but neither --gephi-cmd nor --gephi-template is given,
  # automatically select a default smoke probe template for PATH probing
  if args["gephi"] and args["gephi_cmd"].empty? and args["gephi_template"].empty?
    default_template, default_warnings = find_default_smoke_probe_template(catalog_path)
    catalog_warnings.concat(default_warnings)
    if default_template
      args["gephi_template"] = default_template.fetch("id", "")
    else
      catalog_warnings << "No default smoke probe template found for this OS; PATH probing will be skipped."
    end
  end

  template_warnings = []
  template_errors = []
  unless args["gephi_template"].empty?
    resolved_template_cmd, template_warnings, template_errors = resolve_gephi_template(
        args["gephi_template"],
        catalog_path)
  end
  catalog_warnings.concat(template_warnings)
  catalog_errors.concat(template_errors)

  resolved_render_profile = {}
  render_catalog_warnings = []
  render_catalog_errors = []
  unless args["render_profile"].empty?
    resolved_render_profile, render_catalog_warnings, render_catalog_errors = resolve_gephi_render_profile(
        args["render_profile"],
        render_catalog_path)
  end

  if args["inputs"].empty?
    warn("error: at least one --input is required (or use --list-gephi-templates / --list-gephi-render-profiles).")
    return 3
  end

  reports = []
  args["inputs"].each do |raw_path|
    path = Pathname.new(raw_path)
    fmt = detect_format(path, args["format"])

    reports << validate_one(
        path,
        fmt,
        args["networkx"],
        args["require_networkx"],
        args["gephi"],
        args["require_gephi"],
        args["gephi_cmd"],
        args["gephi_template"],
        resolved_template_cmd,
        catalog_warnings,
        catalog_errors,
        args["render_gephi"],
        args["require_render"],
        args["render_profile"],
        resolved_render_profile,
        render_catalog_warnings,
        render_catalog_errors,
        Pathname.new(args["render_output_dir"]))
  end

  overall_valid = reports.all? { |r| r["valid"] }
  output = {
    "overall_valid" => overall_valid,
    "num_inputs" => reports.size,
    "reports" => reports
  }

  if args["json"]
    puts JSON.pretty_generate(output)
  else
    reports.each do |report|
      puts "[#{report['valid'] ? 'OK' : 'FAIL'}] #{report['path']} (#{report['format']})"
      report["stats"].each { |key, value| puts "  - #{key}: #{value}" }
      report["warnings"].each { |warning| puts "  ! warning: #{warning}" }
      report["errors"].each { |error| puts "  x error: #{error}" }
    end
    puts "overall_valid=#{overall_valid}"
  end

  overall_valid ? 0 : 2
end


exit(main(ARGV))
