# Changelog

All notable changes to Aleph-w are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [4.0.0]

> Version note: this is the first release cut after `v3.1.1`. The cellular
> automata work was drafted internally under the milestone label "v2.0
> (industrial baseline)", but the public release line was already at 3.x,
> so the milestone ships as **4.0.0** to preserve SemVer ordering.

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
- Project version bumped to `4.0.0`; CMake now reads it from the
  `VERSION.txt` file instead of hard-coding it.

### Deprecated
- `Event_Fct`, `Legacy_Static_Event_Table` and
  `Legacy_Dynamic_Event_Table` (`driven_table.H`): use the templated
  `Event_Table<Signature>` family with typed signatures. Removal no
  earlier than 5.0.0.
- `UseCondVar` (`useCondVar.H`): prefer `std::condition_variable` or
  `Aleph::bounded_channel<T>` from `concurrency_utils.H`. Removal no
  earlier than 5.0.0.
- Note: the `Exception_Prototypes` macro (`ahDefs.H`) remains
  documented as deprecated; dynamic exception specifications do not
  compile under the C++20 baseline, so the macro is unusable and will
  be removed in 5.0.0 (attributes cannot be attached to macros).

### Changed (BREAKING) — CA module migrated from `std::vector` to `Aleph::Array`

All public CA signatures that exposed `std::vector` now expose
`Aleph::Array` (phase 27.5.B). Exceptions are documented in
[`docs/std-vector-whitelist.md`](docs/std-vector-whitelist.md). Affected
signatures:

- `ca-io.H`: `Binary_Cell_Pattern::comments` → `Array<std::string>`,
  `Binary_Cell_Pattern::alive` → `Array<Coord_Vec<2>>`,
  `Grid_Snapshot<T>::cells` → `Array<T>`, and
  `ca_io_detail::split_comment_lines` returns `Array<std::string>`.
- `ca-observer.H`: `Density_Observer::counts()/totals()`,
  `Activity_Observer::activity()`, `Entropy_Observer::entropy()`,
  `Sampling_Observer::snapshots()/step_indices()` return
  `const Array<…>&` instead of `const std::vector<…>&`.
- `ca-frame-stream.H`: `Directory_Frame_Sink::written_frames()` returns
  `const Array<std::pair<std::size_t, std::filesystem::path>>&`.
- `ca-metrics.H`: `state_histogram` returns `Array<ca_size_t>`.
- `tpl_ca_graph_automaton.H`: `Graph_Lattice` constructor takes
  `const Array<Array<std::size_t>>&`; `make_grid_graph_adjacency` and
  `make_path_graph_adjacency` return `Array<Array<std::size_t>>`.
- `ca-gif.H`: `ca_gif_detail::Frame::pixels` → `Array<RGB8>`;
  `write_gif` takes `const Array<ca_gif_detail::Frame>&`.

Migration guide (`std::vector` → `Aleph::Array`):

| `std::vector` idiom        | `Aleph::Array` equivalent                  |
|----------------------------|--------------------------------------------|
| `v.push_back(x)`           | `v.append(x)`                              |
| `v.emplace_back(a, b)`     | `v.append(T{a, b})`                        |
| `v.empty()`                | `v.is_empty()`                             |
| `v.data()`                 | `&v.base()` (guard with `is_empty()`)      |
| `v.erase(v.begin())`       | `(void) v.remove_first()`                  |
| `v.resize(n, x)`           | `while (v.size() < n) v.append(x);`        |
| `std::vector<T> v(n)`      | `Array<T> v(n, T{})` (`Array(n)` reserves capacity, it does NOT size) |
| `std::sort(v.begin(), v.end())` | `quicksort(&v.base(), 0, long(v.size()) - 1)` (`tpl_sort_utils.H`) |
| range-for, `v.size()`, `v[i]`, `v.reserve(n)`, `v.clear()` | unchanged |

## [3.1.1] — 2026-04-05

### Fixed
- Removed redundant namespace directives and consolidated error handling.

## [3.1.0] — 2026-03-05

### Added
- Four shortest-path algorithms: Bidirectional BFS, Dial, IDA* and 0-1 BFS.

### Fixed
- Geometry bug fixes; interval-tree improvements.

## [3.0.0] — 2026-01-27

### Added
- Large expansion of data structures and algorithms coverage, utility and
  infrastructure additions, and a significantly more complete test suite
  over the older `Genius` baseline.

### Changed
- License updated from GPLv3 to MIT.

## [Genius] — 2022-03-08

### Added
- Historical baseline: header-dominant C++ library of data structures and
  algorithms (sequences, trees, hash tables, graphs, geometry, numeric and
  compiler utilities) under `namespace Aleph`.

[Unreleased]: https://github.com/lrleon/Aleph-w/compare/v4.0.0...HEAD
[4.0.0]: https://github.com/lrleon/Aleph-w/compare/v3.1.1...v4.0.0
[3.1.1]: https://github.com/lrleon/Aleph-w/compare/v3.1.0...v3.1.1
[3.1.0]: https://github.com/lrleon/Aleph-w/compare/v3.0.0...v3.1.0
[3.0.0]: https://github.com/lrleon/Aleph-w/compare/Genius...v3.0.0
[Genius]: https://github.com/lrleon/Aleph-w/releases/tag/Genius1.0