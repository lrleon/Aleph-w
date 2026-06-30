#!/usr/bin/env python3
"""Phase 28 — internal link checker for the documentation.

Scans Markdown files for relative links and image sources and verifies the
targets exist on disk. External links (http/https/mailto), in-page anchors
(#...) and protocol-relative URLs are skipped. Exits non-zero if any internal
link is broken.

Usage:
    scripts/check_doc_links.py [path ...]      # default: docs/
"""
from __future__ import annotations

import os
import re
import sys

LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
FENCED_RE = re.compile(r"```.*?```", re.DOTALL)
INLINE_CODE_RE = re.compile(r"`[^`]*`")
SKIP_PREFIXES = ("http://", "https://", "mailto:", "//", "#", "data:")


def strip_code(text: str) -> str:
    """Remove fenced and inline code so C++ lambdas like ``[](int v)`` are
    not mistaken for Markdown links."""
    text = FENCED_RE.sub("", text)
    return INLINE_CODE_RE.sub("", text)


def md_files(paths: list[str]) -> list[str]:
    out: list[str] = []
    for p in paths:
        if os.path.isdir(p):
            for root, _dirs, files in os.walk(p):
                out += [os.path.join(root, f) for f in files if f.endswith(".md")]
        elif p.endswith(".md"):
            out.append(p)
    return sorted(out)


def check(md_path: str) -> list[str]:
    errors: list[str] = []
    base = os.path.dirname(md_path)
    with open(md_path, encoding="utf-8") as fh:
        text = strip_code(fh.read())
    for m in LINK_RE.finditer(text):
        target = m.group(1).strip()
        # Drop a trailing "title": [x](path "title")
        if " " in target:
            target = target.split(" ", 1)[0]
        if not target or target.startswith(SKIP_PREFIXES):
            continue
        # Strip an in-page anchor on a relative path: file.md#section
        path = target.split("#", 1)[0]
        if not path:
            continue
        resolved = os.path.normpath(os.path.join(base, path))
        if not os.path.exists(resolved):
            errors.append(f"{md_path}: broken link -> {target} ({resolved})")
    return errors


def main(argv: list[str]) -> int:
    paths = argv[1:] or ["docs"]
    files = md_files(paths)
    all_errors: list[str] = []
    for f in files:
        all_errors += check(f)
    if all_errors:
        print("Broken internal links found:")
        for e in all_errors:
            print(f"  {e}")
        print(f"\n{len(all_errors)} broken link(s) in {len(files)} file(s).")
        return 1
    print(f"OK: no broken internal links in {len(files)} Markdown file(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))