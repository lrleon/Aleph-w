# Versioning, stability and deprecation policy

This document is the contract consumers can rely on when planning
upgrades of Aleph-w.

## Semantic Versioning

Aleph-w follows [Semantic Versioning 2.0.0](https://semver.org/):

- **MAJOR** (`X.0.0`): breaking changes to the public API.
- **MINOR** (`x.Y.0`): backwards-compatible additions.
- **PATCH** (`x.y.Z`): backwards-compatible bug fixes.

The canonical version lives in the top-level [`VERSION.txt`](VERSION.txt)
file. CMake, the Conan recipe and the release workflows read it, and
release-please updates it (see *Release automation* below).
A release is published when the release PR of release-please merges; the
same run builds the source tarball and the `.deb`/`.rpm` packages, tests
installing the `.deb` on clean Ubuntu 22.04, 24.04 and 26.04 and Debian 12
and 13, and only then attaches them to the release
(`.github/workflows/package-release.yml`). A `v*` tag pushed by hand
triggers `.github/workflows/release.yml` instead, which builds and tests
the same artifacts with that workflow and creates the release with them.

Three recipes still carry versions of their own:

- The vcpkg port (`ports/aleph-w/`) and the Spack package
  (`var/spack/repos/builtin/packages/aleph-w/`) are unverified drafts.
  Their version (`4.0.0`) and the hash of the release tarball are
  placeholders, to be filled in and checked against a published release
  before either is submitted upstream; until then, neither installs a
  release.
- The Python package `aleph-ca` (`python/`) has a version of its own, a
  pre-release (`4.0.0b0`), and is published only by hand, to TestPyPI
  (`.github/workflows/python-bindings-publish.yml`). It does not follow
  the releases of the library.

`find_package(Aleph 7.0 ...)` uses `SameMajorVersion` compatibility:
any installed `7.x` satisfies it; `8.0` will not.

### Version history note

Releases proceed `Genius` (2022) → `3.0.0` → `3.1.0` → `3.1.1` →
`4.0.0` → `5.0.0` … `5.11.0` → `6.0.0` → `6.1.0` → `7.0.0` → `7.0.1` →
`7.1.0` → `7.1.1` → `7.1.2` → `7.2.0` → `7.2.1` → `7.2.2`.
There are no published `1.x` or `2.x` releases: those numbers appear in
older internal documents (e.g. the cellular-automata roadmap milestone
"v2.0") and in the historical `version 2.0.0b` banner of header
comments, but they were never tagged. `4.0.0` is the first release
containing the cellular-automata (CA) module.

## What is public API

**Public** (covered by SemVer):

- Every documented (Doxygen) type, function, macro and constant declared
  in the top-level `*.H` headers under `namespace Aleph` (including
  `Aleph::CA`), **except** the exclusions below.
- The CMake consumption surface: package name `Aleph`, components
  (`CA`), imported targets `Aleph::Aleph` and `Aleph::CA`, and the
  propagated usage requirements.
- The Python package surface exported by `aleph_ca` (`python/`), under
  the version of that package (see *Semantic Versioning* above).
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
- The flat C ABI of the cellular-automata module (`ca-c-api.h`, shared
  library `libalephca_c`, CMake target `Aleph::CApi`; built by default,
  option `ALEPH_BUILD_C_API`) has an ABI version of its own,
  `ALEPH_CA_API_VERSION_MAJOR`.`ALEPH_CA_API_VERSION_MINOR` (1.0 today),
  whose major is the `SOVERSION` of the library. Every entry point
  carries a `_v1` suffix, so that an incompatible revision can coexist
  with it. `ca-c-api.h` still marks it experimental: it carries no ABI
  guarantee yet either.

| Release | API compatible with previous | ABI compatible with previous |
|---|---|---|
| Genius (2022) | — (baseline) | — |
| 3.0.0 | No (major) | No |
| 3.1.0 | Yes (additive) | No (recompile) |
| 3.1.1 | Yes (fixes) | No (recompile) |
| 4.0.0 | No (major: CA module added; CA API finalised on `Aleph::Array`; core additions since 3.1.1) | No |
| 5.0.0 | No (major) | No |
| 5.1.0 … 5.11.0 | Yes (additive) | No (recompile) |
| 6.0.0 | No (major) | No |
| 6.1.0 | Yes (additive) | No (recompile) |
| 7.0.0 | No (major: array family and constant iteration; see `docs/release-notes/7.0.0.md`) | No |
| 7.0.1 | Yes (fixes; see `docs/release-notes/7.0.1.md`) | No (recompile) |
| 7.1.0 | Yes (additive, one overload deprecated; see `docs/release-notes/7.1.0.md`) | No (recompile) |
| 7.1.1 | Yes (fixes; see `docs/release-notes/7.1.1.md`) | No (recompile) |
| 7.1.2 | Yes (fixes; see `docs/release-notes/7.1.2.md`) | No (recompile) |
| 7.2.0 | Yes (additive, with behavior changes in the hash tables; see `docs/release-notes/7.2.0.md`) | No (recompile) |
| 7.2.1 | Yes (fixes, with behavior changes in Floyd-Warshall and the binary heap; see `docs/release-notes/7.2.1.md`) | No (recompile) |
| 7.2.2 | Yes (fixes, with a new requirement on the access functor of the arc heaps; see `docs/release-notes/7.2.2.md`) | No (recompile) |

## Release automation

- Ongoing releases are prepared by **release-please**
  (`.github/workflows/release-please.yml`): it parses
  [Conventional Commits](https://www.conventionalcommits.org/) on
  `master`, opens a release PR with the version bump (`VERSION.txt`)
  and the generated `CHANGELOG.md` section, and publishes the release
  and its `v*` tag when that PR merges. The same run then builds the
  artifacts and attaches them, through `package-release.yml`: a tag
  that a workflow creates with its own token triggers no other workflow,
  so `release.yml` does not run for it.
- PR titles are linted for Conventional Commit form (see the
  `conventional-pr` workflow) so the changelog and version calculation
  stay reliable: `feat:` → minor, `fix:` → patch,
  `feat!:`/`BREAKING CHANGE:` → major.