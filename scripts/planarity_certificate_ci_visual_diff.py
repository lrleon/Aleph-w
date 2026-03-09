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

Deterministic visual golden-diff runner for Aleph planarity certificate renders.

The runner executes `planarity_certificate_validator.py` in render mode for
each input/profile case, computes artifact digests and compares them against a
golden manifest.

Exit codes:
- 0: all visual checks passed
- 2: at least one visual case failed
- 3: runtime/usage error
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path


def _default_validator_path() -> Path:
    return Path(__file__).resolve().with_name("planarity_certificate_validator.py")


def _default_render_catalog_path() -> Path:
    return Path(__file__).resolve().with_name("planarity_gephi_render_profiles.json")


def _default_manifest_path() -> Path:
    return Path(__file__).resolve().with_name("planarity_visual_golden_manifest.json")


def _sanitize_id(raw: str) -> str:
    token = re.sub(r"[^a-zA-Z0-9._-]+", "_", raw.strip())
    return token if token else "case"


def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run deterministic render checks for Aleph planarity certificates "
            "and compare against golden digests."
        )
    )
    parser.add_argument(
        "--input",
        dest="inputs",
        action="append",
        default=[],
        help="Certificate input file (repeatable).",
    )
    parser.add_argument(
        "--input-glob",
        dest="input_globs",
        action="append",
        default=[],
        help="Glob pattern for certificate inputs (repeatable).",
    )
    parser.add_argument(
        "--profile",
        dest="profiles",
        action="append",
        default=[],
        help="Render profile id (repeatable).",
    )
    parser.add_argument(
        "--validator",
        default=str(_default_validator_path()),
        help="Path to planarity_certificate_validator.py.",
    )
    parser.add_argument(
        "--render-profile-catalog",
        default=str(_default_render_catalog_path()),
        help="Path to render profile catalog JSON.",
    )
    parser.add_argument(
        "--render-output-dir",
        default="/tmp/aleph_planarity_visual_renders",
        help="Base output directory for rendered artifacts.",
    )
    parser.add_argument(
        "--golden-manifest",
        default=str(_default_manifest_path()),
        help="Path to golden digest manifest JSON.",
    )
    parser.add_argument(
        "--report",
        default="/tmp/aleph_planarity_visual_diff_report.json",
        help="Output JSON report path.",
    )
    parser.add_argument(
        "--update-golden",
        action="store_true",
        help=(
            "Update/create golden entries from current render outputs instead "
            "of failing on mismatches/missing entries."
        ),
    )
    parser.add_argument("--print-summary", action="store_true")
    return parser.parse_args(argv)


def _collect_inputs(inputs: list[str], globs: list[str]) -> list[Path]:
    files: set[Path] = set()
    for raw in inputs:
        files.add(Path(raw).resolve())

    for pattern in globs:
        for p in Path(".").glob(pattern):
            files.add(p.resolve())

    return sorted(files, key=lambda p: p.as_posix())


def _sha256_of_file(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            chunk = f.read(65536)
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest()


def _load_manifest(path: Path) -> dict:
    if not path.exists():
        return {"schema_version": 1, "entries": []}

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {"schema_version": 1, "entries": []}

    if not isinstance(data, dict):
        return {"schema_version": 1, "entries": []}

    entries = data.get("entries", [])
    if not isinstance(entries, list):
        entries = []
    return {
        "schema_version": int(data.get("schema_version", 1)),
        "entries": entries,
    }


def _entry_match_score(
    entry: dict,
    profile_id: str,
    output_kind: str,
    output_ext: str,
    input_stem: str,
) -> int:
    if str(entry.get("profile_id", "")).strip() != profile_id:
        return -1
    if str(entry.get("output_ext", "")).strip().lower() != output_ext.lower():
        return -1

    entry_kind = str(entry.get("output_kind", "")).strip().lower()
    if entry_kind and entry_kind != output_kind.lower():
        return -1

    entry_stem = str(entry.get("input_stem", "")).strip()
    if entry_stem:
        return 2 if entry_stem == input_stem else -1
    return 1


def _find_manifest_entry(
    entries: list[dict],
    profile_id: str,
    output_kind: str,
    output_ext: str,
    input_stem: str,
) -> dict | None:
    best: dict | None = None
    best_score = -1
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        score = _entry_match_score(entry, profile_id, output_kind, output_ext, input_stem)
        if score > best_score:
            best = entry
            best_score = score
    return best


def _stable_sorted_entries(entries: list[dict]) -> list[dict]:
    def key_fn(entry: dict) -> tuple[str, str, str, str]:
        return (
            str(entry.get("profile_id", "")),
            str(entry.get("output_kind", "")),
            str(entry.get("output_ext", "")),
            str(entry.get("input_stem", "")),
        )

    return sorted(entries, key=key_fn)


def _run_validator_case(
    validator: Path,
    input_path: Path,
    profile_id: str,
    render_catalog: Path,
    output_dir: Path,
) -> tuple[int, dict]:
    cmd = [
        "python3",
        str(validator),
        "--json",
        "--input",
        str(input_path),
        "--render-gephi",
        "--require-render",
        "--render-profile",
        profile_id,
        "--render-profile-catalog",
        str(render_catalog),
        "--render-output-dir",
        str(output_dir),
    ]

    proc = subprocess.run(
        cmd,
        check=False,
        capture_output=True,
        text=True,
        timeout=240,
    )

    payload: dict = {
        "command": cmd,
        "exit_code": int(proc.returncode),
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "json": None,
    }
    if proc.stdout.strip():
        try:
            payload["json"] = json.loads(proc.stdout)
        except Exception:
            payload["json"] = None

    return int(proc.returncode), payload


def _extract_render_output_info(validator_payload: dict) -> tuple[Path | None, str, list[str]]:
    errors: list[str] = []
    payload = validator_payload.get("json")
    if not isinstance(payload, dict):
        errors.append("Validator did not emit parseable JSON output.")
        return None, "", errors

    reports = payload.get("reports")
    if not isinstance(reports, list) or len(reports) != 1:
        errors.append("Validator JSON must contain exactly one report per visual case.")
        return None, "", errors

    report = reports[0]
    if not isinstance(report, dict):
        errors.append("Validator report has invalid shape.")
        return None, "", errors

    if not bool(report.get("valid")):
        errors.append("Validator marked render case as invalid.")
        for err in report.get("errors", []):
            errors.append(f"validator: {err}")
        return None, "", errors

    stats = report.get("stats")
    if not isinstance(stats, dict):
        errors.append("Validator report is missing stats.")
        return None, "", errors

    render_output = stats.get("render_output")
    if not isinstance(render_output, str) or not render_output.strip():
        errors.append("Validator report missing stats.render_output.")
        return None, "", errors

    output_kind = str(stats.get("render_output_kind", "")).strip().lower()
    if not output_kind:
        errors.append("Validator report missing stats.render_output_kind.")
        return None, "", errors

    return Path(render_output), output_kind, errors


def main(argv: list[str]) -> int:
    try:
        args = _parse_args(argv)
    except SystemExit as exc:
        return int(exc.code)
    except Exception:
        return 3

    inputs = _collect_inputs(args.inputs, args.input_globs)
    profiles = sorted(set(p.strip() for p in args.profiles if p.strip()))
    if not inputs:
        print("error: no inputs provided (use --input or --input-glob).", file=sys.stderr)
        return 3
    if not profiles:
        print("error: at least one --profile is required.", file=sys.stderr)
        return 3

    validator_path = Path(args.validator).resolve()
    render_catalog = Path(args.render_profile_catalog).resolve()
    output_base = Path(args.render_output_dir).resolve()
    report_path = Path(args.report).resolve()
    manifest_path = Path(args.golden_manifest).resolve()

    manifest = _load_manifest(manifest_path)
    entries = list(manifest.get("entries", []))
    manifest_updated = False

    results: list[dict] = []
    total_failures = 0

    for profile_id in profiles:
        profile_out = output_base / _sanitize_id(profile_id)
        profile_out.mkdir(parents=True, exist_ok=True)

        for input_path in inputs:
            case: dict = {
                "profile_id": profile_id,
                "input": str(input_path),
                "passed": False,
                "errors": [],
                "warnings": [],
                "validator": {},
                "artifact": {},
            }

            if not input_path.exists():
                case["errors"].append("Input file does not exist.")
                results.append(case)
                total_failures += 1
                continue

            rc, validator_payload = _run_validator_case(
                validator=validator_path,
                input_path=input_path,
                profile_id=profile_id,
                render_catalog=render_catalog,
                output_dir=profile_out,
            )
            case["validator"] = validator_payload
            if rc != 0:
                case["errors"].append(f"Validator exit code: {rc}.")

            artifact_path, output_kind, extract_errors = _extract_render_output_info(validator_payload)
            case["errors"].extend(extract_errors)
            if artifact_path is None:
                results.append(case)
                total_failures += 1
                continue

            if not artifact_path.exists():
                case["errors"].append(f"Rendered artifact does not exist: {artifact_path}.")
                results.append(case)
                total_failures += 1
                continue

            output_ext = artifact_path.suffix.lower().lstrip(".")
            input_stem = input_path.stem
            actual_sha = _sha256_of_file(artifact_path)
            actual_bytes = int(artifact_path.stat().st_size)
            case["artifact"] = {
                "path": str(artifact_path),
                "output_kind": output_kind,
                "output_ext": output_ext,
                "sha256": actual_sha,
                "bytes": actual_bytes,
            }

            expected = _find_manifest_entry(
                entries=entries,
                profile_id=profile_id,
                output_kind=output_kind,
                output_ext=output_ext,
                input_stem=input_stem,
            )

            if expected is None:
                if args.update_golden:
                    entries.append(
                        {
                            "profile_id": profile_id,
                            "output_kind": output_kind,
                            "output_ext": output_ext,
                            "sha256": actual_sha,
                            "bytes": actual_bytes,
                        }
                    )
                    manifest_updated = True
                    case["warnings"].append("Golden entry created from current artifact.")
                    case["passed"] = True
                else:
                    case["errors"].append(
                        "Golden entry not found for profile/output. "
                        "Use --update-golden to create it."
                    )
            else:
                expected_sha = str(expected.get("sha256", "")).strip().lower()
                expected_bytes = int(expected.get("bytes", -1))
                expected_kind = str(expected.get("output_kind", "")).strip().lower()
                case["artifact"]["expected_sha256"] = expected_sha
                case["artifact"]["expected_bytes"] = expected_bytes
                case["artifact"]["expected_output_kind"] = expected_kind

                if expected_sha != actual_sha:
                    if args.update_golden:
                        expected["sha256"] = actual_sha
                        manifest_updated = True
                        case["warnings"].append("Golden sha256 updated from current artifact.")
                    else:
                        case["errors"].append(
                            f"Golden hash mismatch (expected={expected_sha}, actual={actual_sha})."
                        )

                if expected_kind and expected_kind != output_kind:
                    if args.update_golden:
                        expected["output_kind"] = output_kind
                        manifest_updated = True
                        case["warnings"].append("Golden output_kind updated from current artifact.")
                    else:
                        case["errors"].append(
                            f"Golden output kind mismatch (expected={expected_kind}, actual={output_kind})."
                        )

                if expected_bytes != actual_bytes:
                    if args.update_golden:
                        expected["bytes"] = actual_bytes
                        manifest_updated = True
                        case["warnings"].append("Golden size updated from current artifact.")
                    else:
                        case["errors"].append(
                            f"Golden size mismatch (expected={expected_bytes}, actual={actual_bytes})."
                        )

                case["passed"] = len(case["errors"]) == 0

            if not case["passed"]:
                total_failures += 1
            results.append(case)

    if manifest_updated:
        manifest["schema_version"] = 1
        manifest["entries"] = _stable_sorted_entries(entries)
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        manifest_path.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    overall_passed = total_failures == 0
    report = {
        "visual_runner": "planarity_certificate_ci_visual_diff.py",
        "overall_passed": overall_passed,
        "num_cases": len(results),
        "num_failures": total_failures,
        "manifest_path": str(manifest_path),
        "manifest_updated": manifest_updated,
        "settings": {
            "validator": str(validator_path),
            "render_profile_catalog": str(render_catalog),
            "render_output_dir": str(output_base),
            "profiles": profiles,
            "num_inputs": len(inputs),
            "update_golden": bool(args.update_golden),
        },
        "results": results,
    }

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True), encoding="utf-8")

    if args.print_summary:
        print(f"cases={len(results)}")
        print(f"failures={total_failures}")
        print(f"manifest_updated={manifest_updated}")
        print(f"report={report_path}")
        print(f"overall_passed={overall_passed}")

    return 0 if overall_passed else 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
