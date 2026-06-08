# Changelog

All notable changes to Aleph-w are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.0]

### Added
- Cellular-automata framework: lattices, neighborhoods, synchronous /
  asynchronous / parallel engines, Hashlife, multi-field SoA engines, and a
  broad set of rules (Game of Life, Wolfram, Gray-Scott, stochastic, block).
- Visualization and I/O sinks for CA: ASCII, SVG, PNG, GIF, NPY, VTK, TikZ,
  DOT, an HTML player, an X11 live viewer and an optional FFmpeg sink.
- Checkpointing with optional DEFLATE compression (vendored miniz backend).
- Weekly scientific reproductions (`reproductions/`) with quantitative gates.
- Performance-regression gate (`benchmarks/`) with six anchor benchmarks, a CI
  perf gate, baseline tracking and a 90-day trend page.
- Python bindings (`python/`) published as the `aleph-ca` wheel.
- **Packaging and distribution**:
  - CMake package config exporting the `Aleph::Aleph` and `Aleph::CA` imported
    targets via `find_package(Aleph 2.0 COMPONENTS CA REQUIRED)`.
  - CPack source/`.deb`/`.rpm` generators and a `release.yml` workflow on
    `v*` tags.
  - vcpkg port, Conan recipe and Spack package.
  - Canonical `VERSION` file and this changelog.

### Changed
- Project version bumped to `2.0.0`; CMake now reads it from the `VERSION`
  file instead of hard-coding it.

## [1.0.0]

### Added
- Initial public lineage: header-dominant C++ library of data structures and
  algorithms (sequences, trees, hash tables, graphs, geometry, numeric and
  compiler utilities) under `namespace Aleph`.

[Unreleased]: https://github.com/lrleon/Aleph-w/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/lrleon/Aleph-w/releases/tag/v2.0.0
[1.0.0]: https://github.com/lrleon/Aleph-w/releases/tag/v1.0.0