# Security Policy

## Supported Versions

Aleph-w follows [Semantic Versioning 2.0](https://semver.org/). Security
fixes are provided for the latest released minor of the current major
line. Older majors receive fixes only for critical vulnerabilities and
only while the maintainer judges the effort proportionate.

| Version line | Supported          |
| ------------ | ------------------ |
| 5.x (latest) | :white_check_mark: |
| < 5.0        | :x: (best effort)  |

The canonical version is recorded in [`VERSION.txt`](VERSION.txt).

## Reporting a Vulnerability

**Please do not open a public GitHub issue for security problems.**

Report privately through one of:

1. **GitHub Security Advisories** — preferred. Open a draft advisory at
   <https://github.com/lrleon/Aleph-w/security/advisories/new>. This keeps
   the report private until a fix is published.
2. **Email** — `leandro.r.leon@gmail.com` with the subject prefixed by
   `[SECURITY] Aleph-w`. If you wish to encrypt, request the maintainer's
   public key in a first, content-free message.

Please include:

- A description of the vulnerability and its impact.
- The affected header(s)/component(s) and version or commit SHA.
- A minimal reproducer (input file, code snippet, or test) when possible.
- Any suggested remediation.

## Response Targets (SLA)

This is a maintainer-driven open-source project; the targets below are
best-effort, not contractual guarantees.

| Stage                         | Target                |
| ----------------------------- | --------------------- |
| Acknowledge receipt           | within 5 business days |
| Initial severity assessment   | within 10 business days |
| Fix or mitigation for critical| within 30 days         |
| Public disclosure             | coordinated, after fix |

## Disclosure Policy

We follow **coordinated disclosure**. We will agree on a disclosure date
with the reporter, credit the reporter (unless anonymity is requested),
and publish a GitHub Security Advisory together with the fixed release.

## Scope Notes

Aleph-w is a header-dominant C++ library. The most security-relevant
surfaces are the file/format parsers that consume untrusted input:

- Cellular-automata I/O readers (`ca-io.H`: RLE / Life / CSV / PGM/PPM).
- Checkpoint loaders (`ca-checkpoint*.H`, including the bundled `miniz`).

Memory-safety findings (out-of-bounds, use-after-free, integer overflow
leading to OOB) in those paths are in scope and prioritized. Issues that
require a malicious build configuration, patched headers, or undefined
behavior triggered only by deliberately invalid template instantiations
are generally out of scope.