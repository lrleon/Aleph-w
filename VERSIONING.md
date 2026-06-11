# Versioning, stability and deprecation policy

This document is the contract consumers can rely on when planning
upgrades of Aleph-w.

## Semantic Versioning

Aleph-w follows [Semantic Versioning 2.0.0](https://semver.org/):

- **MAJOR** (`X.0.0`): breaking changes to the public API.
- **MINOR** (`x.Y.0`): backwards-compatible additions.
- **PATCH** (`x.y.Z`): backwards-compatible bug fixes.

The canonical version lives in the top-level [`VERSION.txt`](VERSION.txt)
file. CMake, the packaging recipes (vcpkg, Conan, Spack) and the release
workflow all read it; nothing else hard-codes the version. Releases are
cut by pushing a `v*` tag, which triggers `.github/workflows/release.yml`
(source tarball, `.deb`/`.rpm`, install test).

`find_package(Aleph 4.0 ...)` uses `SameMajorVersion` compatibility:
any installed `4.x` satisfies it; a future `5.0` will not.

### Version history note

Releases proceed `Genius` (2022) → `3.0.0` → `3.1.0` → `3.1.1` →
`4.0.0`. There are no published `1.x` or `2.x` releases: those numbers
appear in older internal documents (e.g. the cellular-automata roadmap
milestone "v2.0") and in the historical `version 2.0.0b` banner of
header comments, but they were never tagged. `4.0.0` is the first
release containing the cellular-automata (CA) module.

## What is public API

**Public** (covered by SemVer):

- Every documented (Doxygen) type, function, macro and constant declared
  in the top-level `*.H` headers under `namespace Aleph` (including
  `Aleph::CA`), **except** the exclusions below.
- The CMake consumption surface: package name `Aleph`, components
  (`CA`), imported targets `Aleph::Aleph` and `Aleph::CA`, and the
  propagated usage requirements.
- The Python package surface exported by `aleph_ca` (`python/`).
- File formats written/read by the library where documented (RLE,
  Life 1.05/1.06, NetPBM, checkpoint container format).

**Internal** (may change in any release, without notice):

- Anything in a namespace whose name ends in `_detail`
  (`ca_png_detail`, `ca_io_detail`, `ca_checkpoint_detail`, …).
- `private`/`protected` members, and identifiers prefixed with `__`.
- Undocumented helpers, test fixtures (`Tests/`), examples
  (`Examples/`), benchmarks, reproductions and scripts.
- Vendored third-party code (`third_party/`).
- Build-system internals not listed as public above.

When in doubt: if it is documented with Doxygen in a root header and is
not in a `_detail` namespace, it is public.

## Deprecation procedure

1. The API is marked `[[deprecated("use <replacement> instead")]]` in
   the header and listed under **Deprecated** in `CHANGELOG.md`,
   together with its replacement and the earliest version where removal
   may happen.
2. The attribute stays for **at least two minor releases** (or, if a
   major release comes first, at least one full minor cycle) so
   consumers get compiler warnings before anything disappears.
3. Removal happens **only in a major release**, listed under
   **Removed** in `CHANGELOG.md`.
4. Where feasible, the deprecation ships with a migration note in the
   changelog (as done for the `std::vector` → `Aleph::Array` migration
   guide in 4.0.0).

## ABI compatibility

Aleph-w is header-dominant: almost all code is templates instantiated
in the consumer's translation units, plus a small static library
(`libAleph.a`) and the optional vendored miniz object. Consequently:

- **No ABI stability is promised between any two releases**, including
  patch releases. Consumers must recompile against the headers of the
  release they link.
- Mixing object files compiled against different Aleph-w releases in
  one binary is unsupported.
- The planned C ABI shim (`libalephca-c.so`, roadmap phase 25) will be
  the first surface with an ABI guarantee; until it ships, ABI
  stability is out of scope.

| Release | API compatible with previous | ABI compatible with previous |
|---|---|---|
| Genius (2022) | — (baseline) | — |
| 3.0.0 | No (major) | No |
| 3.1.0 | Yes (additive) | No (recompile) |
| 3.1.1 | Yes (fixes) | No (recompile) |
| 4.0.0 | No (major: CA module added; CA API finalised on `Aleph::Array`; core additions since 3.1.1) | No |

## Release automation

- Ongoing releases are prepared by **release-please**
  (`.github/workflows/release-please.yml`): it parses
  [Conventional Commits](https://www.conventionalcommits.org/) on
  `master`, opens a release PR with the version bump (`VERSION.txt`)
  and the generated `CHANGELOG.md` section, and tags the release when
  that PR merges. The `v*` tag then triggers the artifact build in
  `release.yml`.
- PR titles are linted for Conventional Commit form (see the
  `conventional-pr` workflow) so the changelog and version calculation
  stay reliable: `feat:` → minor, `fix:` → patch,
  `feat!:`/`BREAKING CHANGE:` → major.