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

Batch CI wrapper for planarity certificate validation.

The script runs `planarity_certificate_validator.py` on a deterministic list
of inputs and writes a reproducible JSON report for CI artifacts.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


def _default_validator_path() -> Path:
    return Path(__file__).resolve().with_name("planarity_certificate_validator.py")


def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Batch CI wrapper for Aleph planarity certificate validation."
    )
    parser.add_argument(
        "--input",
        dest="inputs",
        action="append",
        default=[],
        help="Input file to validate (repeatable).",
    )
    parser.add_argument(
        "--input-glob",
        dest="input_globs",
        action="append",
        default=[],
        help="Glob pattern for input files (repeatable).",
    )
    parser.add_argument(
        "--report",
        default="/tmp/aleph_planarity_certificate_ci_report.json",
        help="Output JSON report path.",
    )
    parser.add_argument(
        "--validator",
        default=str(_default_validator_path()),
        help="Path to planarity_certificate_validator.py",
    )
    parser.add_argument(
        "--format",
        choices=["auto", "graphml", "gexf"],
        default="auto",
        help="Input format forwarded to validator.",
    )
    parser.add_argument("--networkx", action="store_true")
    parser.add_argument("--require-networkx", action="store_true")
    parser.add_argument("--gephi", action="store_true")
    parser.add_argument("--require-gephi", action="store_true")
    parser.add_argument("--gephi-cmd", default="gephi --version")
    parser.add_argument("--gephi-template", default="")
    parser.add_argument(
        "--gephi-template-catalog",
        default=str(Path(__file__).resolve().with_name("planarity_gephi_templates.json")),
    )
    parser.add_argument("--render-gephi", action="store_true")
    parser.add_argument("--require-render", action="store_true")
    parser.add_argument("--render-profile", default="")
    parser.add_argument(
        "--render-profile-catalog",
        default=str(Path(__file__).resolve().with_name("planarity_gephi_render_profiles.json")),
    )
    parser.add_argument(
        "--render-output-dir",
        default="/tmp/aleph_planarity_renders",
    )
    parser.add_argument("--print-summary", action="store_true")
    return parser.parse_args(argv)


def _collect_inputs(inputs: list[str], globs: list[str]) -> list[Path]:
    paths: set[Path] = set()
    for raw in inputs:
        paths.add(Path(raw).resolve())

    for pattern in globs:
        for p in Path(".").glob(pattern):
            paths.add(p.resolve())

    return sorted(paths, key=lambda p: p.as_posix())


def _run_validator(args: argparse.Namespace, files: list[Path]) -> tuple[int, dict]:
    cmd: list[str] = ["python3", args.validator, "--json", "--format", args.format]
    for p in files:
        cmd.extend(["--input", str(p)])

    if args.networkx:
        cmd.append("--networkx")
    if args.require_networkx:
        cmd.append("--require-networkx")
    if args.gephi:
        cmd.append("--gephi")
    if args.require_gephi:
        cmd.append("--require-gephi")
    if args.gephi_cmd:
        cmd.extend(["--gephi-cmd", args.gephi_cmd])
    if args.gephi_template:
        cmd.extend(["--gephi-template", args.gephi_template])
    if args.gephi_template_catalog:
        cmd.extend(["--gephi-template-catalog", args.gephi_template_catalog])
    if args.render_gephi:
        cmd.append("--render-gephi")
    if args.require_render:
        cmd.append("--require-render")
    if args.render_profile:
        cmd.extend(["--render-profile", args.render_profile])
    if args.render_profile_catalog:
        cmd.extend(["--render-profile-catalog", args.render_profile_catalog])
    if args.render_output_dir:
        cmd.extend(["--render-output-dir", args.render_output_dir])

    proc = subprocess.run(
        cmd,
        check=False,
        capture_output=True,
        text=True,
        timeout=300,
    )

    payload: dict = {
        "command": cmd,
        "validator_exit_code": proc.returncode,
        "validator_stdout": proc.stdout,
        "validator_stderr": proc.stderr,
        "validator_json": None,
    }

    if proc.stdout.strip():
        try:
            payload["validator_json"] = json.loads(proc.stdout)
        except Exception:
            payload["validator_json"] = None

    return proc.returncode, payload


def main(argv: list[str]) -> int:
    try:
        args = _parse_args(argv)
    except SystemExit as exc:
        return int(exc.code)
    except Exception:
        return 3

    report_path = Path(args.report).resolve()
    files = _collect_inputs(args.inputs, args.input_globs)
    if not files:
        print("error: no inputs provided (use --input or --input-glob).", file=sys.stderr)
        return 3

    exit_code, run_payload = _run_validator(args, files)

    report = {
        "batch_runner": "planarity_certificate_ci_batch.py",
        "num_inputs": len(files),
        "inputs": [str(p) for p in files],
        "settings": {
            "format": args.format,
            "networkx": args.networkx,
            "require_networkx": args.require_networkx,
            "gephi": args.gephi,
            "require_gephi": args.require_gephi,
            "gephi_cmd": args.gephi_cmd,
            "gephi_template": args.gephi_template,
            "gephi_template_catalog": args.gephi_template_catalog,
            "render_gephi": args.render_gephi,
            "require_render": args.require_render,
            "render_profile": args.render_profile,
            "render_profile_catalog": args.render_profile_catalog,
            "render_output_dir": args.render_output_dir,
        },
        "run": run_payload,
    }

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True), encoding="utf-8")

    if args.print_summary:
        overall_valid = None
        if isinstance(run_payload.get("validator_json"), dict):
            overall_valid = run_payload["validator_json"].get("overall_valid")
        print(f"inputs={len(files)}")
        print(f"report={report_path}")
        print(f"validator_exit_code={exit_code}")
        if overall_valid is not None:
            print(f"overall_valid={overall_valid}")

    return int(exit_code)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
