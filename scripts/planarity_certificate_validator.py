#!/usr/bin/env python3
"""
Aleph-w
Data structures & Algorithms
https://github.com/lrleon/Aleph-w

Copyright (c) 2002-2026 Leandro Rabindranath Leon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Validate Aleph non-planar certificate exchange artifacts (GraphML / GEXF).

The validator checks:
- XML parseability
- expected top-level graph containers
- node/edge endpoint consistency
- presence of at least one obstruction edge
- optional NetworkX loadability checks

Exit codes:
- 0: all inputs valid
- 2: at least one input invalid
- 3: runtime/usage error
"""

from __future__ import annotations

import argparse
import json
import shlex
import shutil
import subprocess
import sys
import xml.etree.ElementTree as et
from pathlib import Path


def _lname(tag: str) -> str:
    if "}" in tag:
        return tag.rsplit("}", 1)[1]
    return tag


def _first_child(parent: et.Element, name: str) -> et.Element | None:
    for child in list(parent):
        if _lname(child.tag) == name:
            return child
    return None


def _children(parent: et.Element, name: str) -> list[et.Element]:
    return [child for child in list(parent) if _lname(child.tag) == name]


def _graphml_data_map(element: et.Element) -> dict[str, str]:
    data: dict[str, str] = {}
    for child in list(element):
        if _lname(child.tag) != "data":
            continue
        key = child.get("key", "")
        if not key:
            continue
        data[key] = (child.text or "").strip()
    return data


def _gexf_attvalues_map(element: et.Element) -> dict[str, str]:
    values: dict[str, str] = {}
    attvalues = _first_child(element, "attvalues")
    if attvalues is None:
        return values

    for att in _children(attvalues, "attvalue"):
        key = att.get("for", "")
        value = att.get("value", "")
        if key:
            values[key] = value
    return values


def _validate_graphml(root: et.Element) -> tuple[dict, list[str], list[str]]:
    stats: dict[str, int | str] = {}
    errors: list[str] = []
    warnings: list[str] = []

    if _lname(root.tag) != "graphml":
        errors.append("Root element is not <graphml>.")
        return stats, errors, warnings

    graph = _first_child(root, "graph")
    if graph is None:
        errors.append("Missing <graph> element.")
        return stats, errors, warnings

    edgedefault = graph.get("edgedefault", "")
    if edgedefault and edgedefault != "undirected":
        warnings.append("GraphML edgedefault is not 'undirected'.")

    nodes = _children(graph, "node")
    edges = _children(graph, "edge")

    node_ids: set[str] = set()
    for node in nodes:
        nid = node.get("id", "").strip()
        if not nid:
            errors.append("Node with empty id.")
            continue
        if nid in node_ids:
            errors.append(f"Duplicated node id: {nid}.")
        node_ids.add(nid)

    obstruction_edges = 0
    path_edges = 0
    for edge in edges:
        src = edge.get("source", "").strip()
        tgt = edge.get("target", "").strip()
        if not src or not tgt:
            errors.append("Edge with empty source/target.")
            continue
        if src not in node_ids or tgt not in node_ids:
            errors.append(f"Edge endpoint not found in node set: {src}->{tgt}.")

        data = _graphml_data_map(edge)
        kind = data.get("k_edge_kind", "").strip()
        if kind == "obstruction":
            obstruction_edges += 1
        elif kind == "path":
            path_edges += 1
            path_id = data.get("k_edge_path_id", "").strip()
            if path_id and not path_id.lstrip("-").isdigit():
                errors.append(f"Invalid path id value: {path_id}.")
        elif kind:
            warnings.append(f"Unknown edge kind value: {kind}.")

    stats["num_nodes"] = len(nodes)
    stats["num_edges"] = len(edges)
    stats["num_obstruction_edges"] = obstruction_edges
    stats["num_path_edges"] = path_edges
    stats["edgedefault"] = edgedefault or "unknown"

    if len(nodes) == 0:
        errors.append("GraphML has no nodes.")
    if len(edges) == 0:
        errors.append("GraphML has no edges.")
    if obstruction_edges == 0:
        errors.append("GraphML contains no obstruction edges.")

    return stats, errors, warnings


def _validate_gexf(root: et.Element) -> tuple[dict, list[str], list[str]]:
    stats: dict[str, int | str] = {}
    errors: list[str] = []
    warnings: list[str] = []

    if _lname(root.tag) != "gexf":
        errors.append("Root element is not <gexf>.")
        return stats, errors, warnings

    graph = _first_child(root, "graph")
    if graph is None:
        errors.append("Missing <graph> element.")
        return stats, errors, warnings

    default_edge_type = graph.get("defaultedgetype", "")
    if default_edge_type and default_edge_type != "undirected":
        warnings.append("GEXF defaultedgetype is not 'undirected'.")

    nodes_container = _first_child(graph, "nodes")
    edges_container = _first_child(graph, "edges")
    if nodes_container is None or edges_container is None:
        errors.append("Missing <nodes> or <edges> container.")
        return stats, errors, warnings

    nodes = _children(nodes_container, "node")
    edges = _children(edges_container, "edge")

    node_ids: set[str] = set()
    for node in nodes:
        nid = node.get("id", "").strip()
        if not nid:
            errors.append("Node with empty id.")
            continue
        if nid in node_ids:
            errors.append(f"Duplicated node id: {nid}.")
        node_ids.add(nid)

    obstruction_edges = 0
    path_edges = 0
    for edge in edges:
        src = edge.get("source", "").strip()
        tgt = edge.get("target", "").strip()
        if not src or not tgt:
            errors.append("Edge with empty source/target.")
            continue
        if src not in node_ids or tgt not in node_ids:
            errors.append(f"Edge endpoint not found in node set: {src}->{tgt}.")

        values = _gexf_attvalues_map(edge)
        kind = values.get("e_kind", "").strip()
        if kind == "obstruction":
            obstruction_edges += 1
        elif kind == "path":
            path_edges += 1
            path_id = values.get("e_path_id", "").strip()
            if path_id and not path_id.lstrip("-").isdigit():
                errors.append(f"Invalid path id value: {path_id}.")
        elif kind:
            warnings.append(f"Unknown edge kind value: {kind}.")

    stats["num_nodes"] = len(nodes)
    stats["num_edges"] = len(edges)
    stats["num_obstruction_edges"] = obstruction_edges
    stats["num_path_edges"] = path_edges
    stats["defaultedgetype"] = default_edge_type or "unknown"

    if len(nodes) == 0:
        errors.append("GEXF has no nodes.")
    if len(edges) == 0:
        errors.append("GEXF has no edges.")
    if obstruction_edges == 0:
        errors.append("GEXF contains no obstruction edges.")

    return stats, errors, warnings


def _validate_with_networkx(path: Path, fmt: str, require: bool) -> tuple[dict, list[str], list[str]]:
    stats: dict[str, int | str] = {}
    errors: list[str] = []
    warnings: list[str] = []

    try:
        import networkx as nx  # type: ignore
    except Exception:
        if require:
            errors.append("NetworkX is required but not available.")
        else:
            warnings.append("NetworkX not available; skipping NetworkX validation.")
        return stats, errors, warnings

    try:
        if fmt == "graphml":
            g = nx.read_graphml(path)
        elif fmt == "gexf":
            g = nx.read_gexf(path)
        else:
            errors.append(f"Unsupported NetworkX format: {fmt}.")
            return stats, errors, warnings
    except Exception as exc:
        errors.append(f"NetworkX load failed: {exc}.")
        return stats, errors, warnings

    stats["networkx_num_nodes"] = int(g.number_of_nodes())
    stats["networkx_num_edges"] = int(g.number_of_edges())
    if g.number_of_nodes() == 0:
        errors.append("NetworkX loaded graph has no nodes.")
    if g.number_of_edges() == 0:
        errors.append("NetworkX loaded graph has no edges.")
    return stats, errors, warnings


def _validate_with_gephi(
    path: Path,
    cmd_template: str,
    require: bool,
) -> tuple[dict, list[str], list[str]]:
    stats: dict[str, int | str] = {}
    errors: list[str] = []
    warnings: list[str] = []

    template = (cmd_template or "").strip()
    if not template:
        template = "gephi --version"

    if "{input}" not in template:
        warnings.append(
            "Gephi command template has no {input} placeholder; running smoke check only."
        )

    cmd, parse_error = _expand_command_template(
        template=template,
        replacements={"{input}": str(path)},
        error_prefix="Unable to parse --gephi-cmd template",
    )
    if parse_error:
        errors.append(parse_error)
        return stats, errors, warnings

    if not cmd:
        errors.append("Empty Gephi command after template expansion.")
        return stats, errors, warnings

    exe = cmd[0]
    exe_path = shutil.which(exe)
    if exe_path is None:
        msg = f"Gephi command not found in PATH: {exe}."
        if require:
            errors.append(msg)
        else:
            warnings.append(msg + " Skipping Gephi validation.")
        return stats, errors, warnings

    stats["gephi_command"] = " ".join(cmd)
    stats["gephi_executable"] = exe_path

    try:
        proc = subprocess.run(
            cmd,
            check=False,
            capture_output=True,
            text=True,
            timeout=120,
        )
    except subprocess.TimeoutExpired:
        errors.append("Gephi command timed out.")
        return stats, errors, warnings
    except Exception as exc:
        errors.append(f"Gephi command execution failed: {exc}.")
        return stats, errors, warnings

    stats["gephi_exit_code"] = int(proc.returncode)
    if proc.returncode != 0:
        stderr = (proc.stderr or "").strip()
        if len(stderr) > 240:
            stderr = stderr[:240] + "..."
        msg = "Gephi command returned non-zero exit code" + (f": {stderr}" if stderr else ".")
        if require:
            errors.append(msg)
        else:
            warnings.append(msg + " Skipping strict Gephi validation.")

    return stats, errors, warnings


def _expand_command_template(
    template: str,
    replacements: dict[str, str],
    error_prefix: str,
) -> tuple[list[str], str]:
    try:
        tokens = shlex.split(template)
    except Exception as exc:
        return [], f"{error_prefix}: {exc}."

    cmd: list[str] = []
    for token in tokens:
        expanded = token
        for key, value in replacements.items():
            expanded = expanded.replace(key, value)
        cmd.append(expanded)

    return cmd, ""


def _default_template_catalog_path() -> Path:
    return Path(__file__).resolve().with_name("planarity_gephi_templates.json")


def _default_render_profile_catalog_path() -> Path:
    return Path(__file__).resolve().with_name("planarity_gephi_render_profiles.json")


def _load_gephi_templates(catalog_path: Path) -> tuple[dict[str, dict], list[str]]:
    warnings: list[str] = []
    templates: dict[str, dict] = {}

    if not catalog_path.exists():
        warnings.append(f"Gephi template catalog not found: {catalog_path}.")
        return templates, warnings

    try:
        data = json.loads(catalog_path.read_text(encoding="utf-8"))
    except Exception as exc:
        warnings.append(f"Unable to parse Gephi template catalog: {exc}.")
        return templates, warnings

    entries = data.get("templates", [])
    if not isinstance(entries, list):
        warnings.append("Gephi template catalog has invalid 'templates' format.")
        return templates, warnings

    for item in entries:
        if not isinstance(item, dict):
            continue
        tid = str(item.get("id", "")).strip()
        cmd = str(item.get("cmd", "")).strip()
        if not tid or not cmd:
            continue
        templates[tid] = item

    return templates, warnings


def _load_gephi_render_profiles(catalog_path: Path) -> tuple[dict[str, dict], list[str]]:
    warnings: list[str] = []
    profiles: dict[str, dict] = {}

    if not catalog_path.exists():
        warnings.append(f"Gephi render profile catalog not found: {catalog_path}.")
        return profiles, warnings

    try:
        data = json.loads(catalog_path.read_text(encoding="utf-8"))
    except Exception as exc:
        warnings.append(f"Unable to parse Gephi render profile catalog: {exc}.")
        return profiles, warnings

    entries = data.get("profiles", [])
    if not isinstance(entries, list):
        warnings.append("Gephi render profile catalog has invalid 'profiles' format.")
        return profiles, warnings

    for item in entries:
        if not isinstance(item, dict):
            continue
        pid = str(item.get("id", "")).strip()
        cmd = str(item.get("cmd", "")).strip()
        kind = str(item.get("output_kind", "")).strip().lower()
        ext = str(item.get("output_ext", "")).strip().lower()
        if not pid or not cmd or not kind or not ext:
            continue
        profiles[pid] = item

    return profiles, warnings


def _resolve_gephi_template(
    template_id: str,
    catalog_path: Path,
) -> tuple[str, list[str], list[str]]:
    warnings: list[str] = []
    errors: list[str] = []

    templates, load_warnings = _load_gephi_templates(catalog_path)
    warnings.extend(load_warnings)

    if not template_id:
        return "", warnings, errors

    if template_id not in templates:
        errors.append(
            f"Gephi template id not found: {template_id} (catalog: {catalog_path})."
        )
        return "", warnings, errors

    template = templates[template_id]
    cmd = str(template.get("cmd", "")).strip()
    if not cmd:
        errors.append(f"Gephi template '{template_id}' has empty command.")
        return "", warnings, errors

    return cmd, warnings, errors


def _resolve_gephi_render_profile(
    profile_id: str,
    catalog_path: Path,
) -> tuple[dict, list[str], list[str]]:
    warnings: list[str] = []
    errors: list[str] = []

    profiles, load_warnings = _load_gephi_render_profiles(catalog_path)
    warnings.extend(load_warnings)

    if not profile_id:
        return {}, warnings, errors

    if profile_id not in profiles:
        errors.append(
            f"Gephi render profile id not found: {profile_id} (catalog: {catalog_path})."
        )
        return {}, warnings, errors

    profile = profiles[profile_id]
    cmd = str(profile.get("cmd", "")).strip()
    kind = str(profile.get("output_kind", "")).strip().lower()
    ext = str(profile.get("output_ext", "")).strip().lower()
    if not cmd or not kind or not ext:
        errors.append(f"Gephi render profile '{profile_id}' is missing cmd/kind/ext.")
        return {}, warnings, errors

    return profile, warnings, errors


def _list_gephi_templates(
    catalog_path: Path,
    output_json: bool,
    os_filter: str,
) -> int:
    templates, warnings = _load_gephi_templates(catalog_path)

    selected: list[dict] = []
    for _, item in sorted(templates.items(), key=lambda kv: kv[0]):
        if os_filter and os_filter != "any":
            item_os = str(item.get("os", "")).strip().lower()
            if item_os not in ("any", os_filter):
                continue
        selected.append(item)

    payload = {
        "catalog_path": str(catalog_path),
        "num_templates": len(selected),
        "warnings": warnings,
        "templates": selected,
    }

    if output_json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print(f"catalog={catalog_path}")
        for warning in warnings:
            print(f"! warning: {warning}")
        for item in selected:
            print(
                f"- {item.get('id')} | os={item.get('os')} | version={item.get('gephi_version')} | {item.get('description')}"
            )

    return 0


def _list_gephi_render_profiles(
    catalog_path: Path,
    output_json: bool,
    os_filter: str,
) -> int:
    profiles, warnings = _load_gephi_render_profiles(catalog_path)

    selected: list[dict] = []
    for _, item in sorted(profiles.items(), key=lambda kv: kv[0]):
        if os_filter and os_filter != "any":
            item_os = str(item.get("os", "")).strip().lower()
            if item_os not in ("any", os_filter):
                continue
        selected.append(item)

    payload = {
        "catalog_path": str(catalog_path),
        "num_profiles": len(selected),
        "warnings": warnings,
        "profiles": selected,
    }

    if output_json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print(f"catalog={catalog_path}")
        for warning in warnings:
            print(f"! warning: {warning}")
        for item in selected:
            print(
                f"- {item.get('id')} | os={item.get('os')} | version={item.get('gephi_version')} | kind={item.get('output_kind')} | {item.get('description')}"
            )

    return 0


def _validate_render_artifact(path: Path, kind: str) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []

    if not path.exists():
        errors.append(f"Render artifact was not produced: {path}.")
        return errors, warnings

    if path.stat().st_size == 0:
        errors.append(f"Render artifact is empty: {path}.")
        return errors, warnings

    k = (kind or "").strip().lower()
    if k == "svg":
        try:
            root = et.parse(path).getroot()
            if _lname(root.tag) != "svg":
                errors.append(f"SVG artifact has invalid root tag: {_lname(root.tag)}.")
        except Exception as exc:
            errors.append(f"SVG artifact XML parse failed: {exc}.")
    elif k == "pdf":
        try:
            raw = path.read_bytes()
            if not raw.startswith(b"%PDF-"):
                errors.append("PDF artifact does not start with %PDF- header.")
        except Exception as exc:
            errors.append(f"PDF artifact read failed: {exc}.")

    return errors, warnings


def _run_gephi_render_profile(
    path: Path,
    profile: dict,
    output_dir: Path,
    require_render: bool,
) -> tuple[dict, list[str], list[str]]:
    stats: dict[str, int | str] = {}
    errors: list[str] = []
    warnings: list[str] = []

    cmd_template = str(profile.get("cmd", "")).strip()
    output_kind = str(profile.get("output_kind", "")).strip().lower()
    output_ext = str(profile.get("output_ext", "")).strip().lower()

    if not cmd_template or not output_kind or not output_ext:
        errors.append("Invalid render profile (missing command/kind/ext).")
        return stats, errors, warnings

    output_dir.mkdir(parents=True, exist_ok=True)
    out_file = output_dir / f"{path.stem}.{output_ext}"

    cmd, parse_error = _expand_command_template(
        template=cmd_template,
        replacements={
            "{input}": str(path),
            "{output}": str(out_file),
            "{basename}": path.stem,
            "{output_ext}": output_ext,
        },
        error_prefix="Unable to parse render profile command",
    )
    if parse_error:
        errors.append(parse_error)
        return stats, errors, warnings

    if not cmd:
        errors.append("Render profile command resolved to empty command.")
        return stats, errors, warnings

    exe = cmd[0]
    exe_path = shutil.which(exe)
    if exe_path is None:
        msg = f"Render command not found in PATH: {exe}."
        if require_render:
            errors.append(msg)
        else:
            warnings.append(msg + " Skipping render validation.")
        return stats, errors, warnings

    stats["render_command"] = " ".join(cmd)
    stats["render_executable"] = exe_path
    stats["render_output"] = str(out_file)
    stats["render_output_kind"] = output_kind

    try:
        proc = subprocess.run(
            cmd,
            check=False,
            capture_output=True,
            text=True,
            timeout=180,
        )
    except subprocess.TimeoutExpired:
        errors.append("Render command timed out.")
        return stats, errors, warnings
    except Exception as exc:
        errors.append(f"Render command execution failed: {exc}.")
        return stats, errors, warnings

    stats["render_exit_code"] = int(proc.returncode)
    if proc.returncode != 0:
        stderr = (proc.stderr or "").strip()
        if len(stderr) > 240:
            stderr = stderr[:240] + "..."
        errors.append(
            "Render command returned non-zero exit code"
            + (f": {stderr}" if stderr else ".")
        )
        return stats, errors, warnings

    artifact_errors, artifact_warnings = _validate_render_artifact(out_file, output_kind)
    errors.extend(artifact_errors)
    warnings.extend(artifact_warnings)
    if out_file.exists():
        stats["render_output_bytes"] = int(out_file.stat().st_size)

    return stats, errors, warnings


def _detect_format(path: Path, explicit: str) -> str:
    if explicit != "auto":
        return explicit
    suffix = path.suffix.lower()
    if suffix == ".graphml":
        return "graphml"
    if suffix == ".gexf":
        return "gexf"
    return "unknown"


def _validate_one(
    path: Path,
    fmt: str,
    use_networkx: bool,
    require_networkx: bool,
    use_gephi: bool,
    require_gephi: bool,
    gephi_cmd: str,
    gephi_template: str,
    resolved_template_cmd: str,
    catalog_warnings: list[str],
    catalog_errors: list[str],
    use_gephi_render: bool,
    require_gephi_render: bool,
    render_profile_id: str,
    resolved_render_profile: dict,
    render_catalog_warnings: list[str],
    render_catalog_errors: list[str],
    render_output_dir: Path,
) -> dict:
    report = {
        "path": str(path),
        "format": fmt,
        "valid": False,
        "stats": {},
        "errors": [],
        "warnings": [],
    }

    if not path.exists():
        report["errors"].append("Input file does not exist.")
        return report

    if fmt not in ("graphml", "gexf"):
        report["errors"].append("Unable to infer format; use --format.")
        return report

    try:
        root = et.parse(path).getroot()
    except Exception as exc:
        report["errors"].append(f"XML parse failed: {exc}.")
        return report

    if fmt == "graphml":
        stats, errors, warnings = _validate_graphml(root)
    else:
        stats, errors, warnings = _validate_gexf(root)

    report["stats"].update(stats)
    report["errors"].extend(errors)
    report["warnings"].extend(warnings)

    if use_networkx:
        nx_stats, nx_errors, nx_warnings = _validate_with_networkx(path, fmt, require_networkx)
        report["stats"].update(nx_stats)
        report["errors"].extend(nx_errors)
        report["warnings"].extend(nx_warnings)

    if use_gephi:
        report["warnings"].extend(catalog_warnings)
        report["errors"].extend(catalog_errors)

        resolved_cmd = gephi_cmd
        if gephi_template:
            resolved_cmd = resolved_template_cmd

        gephi_stats, gephi_errors, gephi_warnings = _validate_with_gephi(
            path=path,
            cmd_template=resolved_cmd,
            require=require_gephi,
        )
        report["stats"].update(gephi_stats)
        report["errors"].extend(gephi_errors)
        report["warnings"].extend(gephi_warnings)

    if use_gephi_render:
        report["warnings"].extend(render_catalog_warnings)
        report["errors"].extend(render_catalog_errors)

        if not render_profile_id:
            report["errors"].append(
                "Render mode requires --render-profile."
            )
        elif not resolved_render_profile:
            report["errors"].append(
                f"Render profile could not be resolved: {render_profile_id}."
            )
        else:
            render_stats, render_errors, render_warnings = _run_gephi_render_profile(
                path=path,
                profile=resolved_render_profile,
                output_dir=render_output_dir,
                require_render=require_gephi_render,
            )
            report["stats"].update(render_stats)
            report["errors"].extend(render_errors)
            report["warnings"].extend(render_warnings)

    report["valid"] = len(report["errors"]) == 0
    return report


def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate Aleph non-planar certificate exchange files (GraphML/GEXF)."
    )
    parser.add_argument(
        "--input",
        dest="inputs",
        action="append",
        default=[],
        help="Input certificate file (repeatable).",
    )
    parser.add_argument(
        "--format",
        choices=["auto", "graphml", "gexf"],
        default="auto",
        help="Input format. Default: auto by file extension.",
    )
    parser.add_argument(
        "--networkx",
        action="store_true",
        help="Enable optional NetworkX parse/load validation.",
    )
    parser.add_argument(
        "--require-networkx",
        action="store_true",
        help="Fail if --networkx is requested but NetworkX is unavailable.",
    )
    parser.add_argument(
        "--gephi",
        action="store_true",
        help="Enable optional Gephi CLI/toolkit command validation.",
    )
    parser.add_argument(
        "--require-gephi",
        action="store_true",
        help="Fail if --gephi is requested but Gephi command is unavailable or fails.",
    )
    parser.add_argument(
        "--gephi-cmd",
        default="gephi --version",
        help=(
            "Gephi command template. Use {input} placeholder for file path. "
            "Default performs only availability smoke-check."
        ),
    )
    parser.add_argument(
        "--gephi-template",
        default="",
        help=(
            "Template id from the Gephi template catalog. "
            "When set, it overrides --gephi-cmd."
        ),
    )
    parser.add_argument(
        "--gephi-template-catalog",
        default=str(_default_template_catalog_path()),
        help="Path to Gephi template catalog JSON.",
    )
    parser.add_argument(
        "--list-gephi-templates",
        action="store_true",
        help="List templates from Gephi catalog and exit.",
    )
    parser.add_argument(
        "--template-os",
        choices=["any", "linux", "macos", "windows"],
        default="any",
        help="OS filter used by --list-gephi-templates.",
    )
    parser.add_argument(
        "--render-gephi",
        action="store_true",
        help="Enable Gephi render profile execution and artifact validation.",
    )
    parser.add_argument(
        "--require-render",
        action="store_true",
        help="Fail if render command/profile is unavailable or rendering fails.",
    )
    parser.add_argument(
        "--render-profile",
        default="",
        help="Render profile id from render profile catalog.",
    )
    parser.add_argument(
        "--render-profile-catalog",
        default=str(_default_render_profile_catalog_path()),
        help="Path to Gephi render profile catalog JSON.",
    )
    parser.add_argument(
        "--list-gephi-render-profiles",
        action="store_true",
        help="List render profiles from render profile catalog and exit.",
    )
    parser.add_argument(
        "--render-output-dir",
        default="/tmp/aleph_planarity_renders",
        help="Directory where render artifacts are written/validated.",
    )
    parser.add_argument(
        "--render-os",
        choices=["any", "linux", "macos", "windows"],
        default="any",
        help="OS filter used by --list-gephi-render-profiles.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit JSON report instead of plain text.",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    try:
        args = _parse_args(argv)
    except SystemExit as exc:
        return int(exc.code)
    except Exception:
        return 3

    catalog_path = Path(args.gephi_template_catalog)
    if args.list_gephi_templates:
        return _list_gephi_templates(
            catalog_path=catalog_path,
            output_json=args.json,
            os_filter=args.template_os,
        )

    render_catalog_path = Path(args.render_profile_catalog)
    if args.list_gephi_render_profiles:
        return _list_gephi_render_profiles(
            catalog_path=render_catalog_path,
            output_json=args.json,
            os_filter=args.render_os,
        )

    resolved_template_cmd = ""
    catalog_warnings: list[str] = []
    catalog_errors: list[str] = []
    if args.gephi_template:
        resolved_template_cmd, catalog_warnings, catalog_errors = _resolve_gephi_template(
            template_id=args.gephi_template,
            catalog_path=catalog_path,
        )

    resolved_render_profile: dict = {}
    render_catalog_warnings: list[str] = []
    render_catalog_errors: list[str] = []
    if args.render_profile:
        (
            resolved_render_profile,
            render_catalog_warnings,
            render_catalog_errors,
        ) = _resolve_gephi_render_profile(
            profile_id=args.render_profile,
            catalog_path=render_catalog_path,
        )

    if not args.inputs:
        print(
            "error: at least one --input is required "
            "(or use --list-gephi-templates / --list-gephi-render-profiles).",
            file=sys.stderr,
        )
        return 3

    reports = []
    for raw_path in args.inputs:
        path = Path(raw_path)
        fmt = _detect_format(path, args.format)
        reports.append(
            _validate_one(
                path=path,
                fmt=fmt,
                use_networkx=args.networkx,
                require_networkx=args.require_networkx,
                use_gephi=args.gephi,
                require_gephi=args.require_gephi,
                gephi_cmd=args.gephi_cmd,
                gephi_template=args.gephi_template,
                resolved_template_cmd=resolved_template_cmd,
                catalog_warnings=catalog_warnings,
                catalog_errors=catalog_errors,
                use_gephi_render=args.render_gephi,
                require_gephi_render=args.require_render,
                render_profile_id=args.render_profile,
                resolved_render_profile=resolved_render_profile,
                render_catalog_warnings=render_catalog_warnings,
                render_catalog_errors=render_catalog_errors,
                render_output_dir=Path(args.render_output_dir),
            )
        )

    overall_valid = all(bool(r["valid"]) for r in reports)
    output = {
        "overall_valid": overall_valid,
        "num_inputs": len(reports),
        "reports": reports,
    }

    if args.json:
        print(json.dumps(output, indent=2, sort_keys=True))
    else:
        for report in reports:
            print(f"[{ 'OK' if report['valid'] else 'FAIL' }] {report['path']} ({report['format']})")
            for key, value in report["stats"].items():
                print(f"  - {key}: {value}")
            for warning in report["warnings"]:
                print(f"  ! warning: {warning}")
            for error in report["errors"]:
                print(f"  x error: {error}")
        print(f"overall_valid={overall_valid}")

    return 0 if overall_valid else 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
