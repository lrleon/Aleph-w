# Changelog

All notable changes to Aleph-w are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [5.0.0](https://github.com/lrleon/Aleph-w/compare/v4.0.0...v5.0.0) (2026-06-20)


### ⚠ BREAKING CHANGES

* cellular automata framework, packaging and 4.0.0 release infrastructure ([#55](https://github.com/lrleon/Aleph-w/issues/55))

### Features

* **ah-stl-functional:** Implement reserve for stl_range and stl_linspace ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **ah-stl-functional:** Implement reserve for stl_range and stl_linspace ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* **archeap:** Add move constructors and assignments ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **archeap:** Add move constructors and assignments ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* cellular automata framework, packaging and 4.0.0 release infrastructure ([#55](https://github.com/lrleon/Aleph-w/issues/55)) ([730d5f3](https://github.com/lrleon/Aleph-w/commit/730d5f3b37238db2128eee979d0f238957a7830b))
* **driven_table:** Add `Event_Table<Signature>` for type safety ([4325d64](https://github.com/lrleon/Aleph-w/commit/4325d64c2139cdd0cfac823ff6c5443a372851c2))
* **driven_table:** Add `Event_Table<Signature>` for type safety ([f4d9347](https://github.com/lrleon/Aleph-w/commit/f4d93472b6422722403657ff31a3eefec71a0147))
* **driven_table:** Add `Event_Table<Signature>` for type safety ([1f177e5](https://github.com/lrleon/Aleph-w/commit/1f177e5da05e7ab5da858e7ee3fdce0dd5b62032))
* **driven_table:** Add `Event_Table<Signature>` for type safety ([9a6aec7](https://github.com/lrleon/Aleph-w/commit/9a6aec727cc61ad549dcb18933f216a081efd028))
* **driven_table:** Add move constructors and assignments for Static_Event_Table ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **driven_table:** Add move constructors and assignments for Static_Event_Table ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* **functional:** Add sum, product, min, max, join, to_vector/dynlist methods ([6a169f9](https://github.com/lrleon/Aleph-w/commit/6a169f9f2e90701f1d192ed68e1498c62cedf13f))
* **functional:** Implement unique, intersperse, chunk, sliding, zip_with ([6a169f9](https://github.com/lrleon/Aleph-w/commit/6a169f9f2e90701f1d192ed68e1498c62cedf13f))
* **geometry:** Add Constrained Delaunay and Fortune's Voronoi algorithms ([91eb6fd](https://github.com/lrleon/Aleph-w/commit/91eb6fd6e39783c2c1d11f185d327003cd821f80))
* **graph:** Consolidate graph utilities and remove redundant files ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **graph:** Consolidate graph utilities and remove redundant files ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* **linalg:** Add matrix_dims helper for modular_linalg.H ([6a169f9](https://github.com/lrleon/Aleph-w/commit/6a169f9f2e90701f1d192ed68e1498c62cedf13f))
* Migrate installation to INSTALL.md ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* Migrate installation to INSTALL.md ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* **protected_lhash, protected_odhash:** Disable copy/move for thread-safe hash tables ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **protected_lhash, protected_odhash:** Disable copy/move for thread-safe hash tables ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* **quadtree:** Add move constructors and assignments ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **quadtree:** Add move constructors and assignments ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* **ringfilecache:** Use strncpy for cache file name in RingFileCache ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **ringfilecache:** Use strncpy for cache file name in RingFileCache ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))


### Bug Fixes

* **minhash:** Update iterator concept and constructor initialization ([6a169f9](https://github.com/lrleon/Aleph-w/commit/6a169f9f2e90701f1d192ed68e1498c62cedf13f))
* **parse_utils:** Catch `const std::out_of_range&` instead of `out_of_range` ([af612d6](https://github.com/lrleon/Aleph-w/commit/af612d6afa4a154dc9bbf2c12ed417a0435a9bbc))
* **parse_utils:** Catch `const std::out_of_range&` instead of `out_of_range` ([5c49df6](https://github.com/lrleon/Aleph-w/commit/5c49df6163abf9fa500b07207517962d57a8e5b8))
* **reservoir-sampling:** Correct overflow check in update ([6a169f9](https://github.com/lrleon/Aleph-w/commit/6a169f9f2e90701f1d192ed68e1498c62cedf13f))

## [Unreleased]

### Security
- **Checkpoint loader hardening (CWE-789 / CWE-190).** `load_checkpoint_into`
  could be driven into a multi-gigabyte allocation by a hostile or corrupt
  checkpoint header. `read_raw_payload` now bounds the declared `payload_size`
  against the actual file size, and `inspect_checkpoint` rejects headers whose
  `cell_count` is inconsistent with the declared extents or whose
  `cell_count * state_type_size` would overflow. Surfaced by the new
  checkpoint fuzzer (Phase 26); regression tests in
  `Tests/ca_checkpoint_safety_test.cc`.

### Added
- **Property-based tests** (`Tests/ca_properties_test.cc`): particle
  conservation for `BBM_Rule`/`TM_Gas_Rule`, Margolus reversibility,
  synchronous-engine determinism and `at_safe` well-definedness, each swept
  over many random grids.
- **Fuzzing harness** (`Tests/fuzz/`, Phase 26): libFuzzer targets for the
  RLE, Life 1.05/1.06, CSV and checkpoint parsers, with seed corpora and a
  weekly `fuzz` workflow.
- **OSS compliance** (Phase 30): `SECURITY.md`, `CODE_OF_CONDUCT.md`,
  `CONTRIBUTING.md`, Dependabot config, REUSE 3.x licensing (`REUSE.toml` +
  `LICENSES/`) with a `reuse-lint` job, and a reproducible-build check
  (`repro-build`).
- **Coverage** (Phase 27): Codecov upload, README coverage/REUSE badges and a
  per-module (`tpl_ca_*.H` vs `ca-*.H`) coverage breakdown in CI.
- **Documentation** (Phase 28): a six-chapter CA tutorial
  (`docs/tutorial/`), a performance tuning guide (`docs/performance.md`), and a
  reproducible visual gallery (`docs/gallery/`, 16 rendered rules) generated by
  `scripts/regen_gallery.sh` / `Examples/ca_gallery_gen.cc`, gated by a
  `docs-build` workflow with an internal link checker.
- **C ABI shim** (Phase 25): a flat, versioned C ABI (`ca-c-api.h`) built as the
  `libalephca_c` shared library, exposing the Game-of-Life engine to any
  language with a C FFI. Includes a pure-C smoke test (`Tests/c_abi_smoke.c`)
  and experimental Rust bindings (`bindings/rust/`: `aleph-ca-sys` + `aleph-ca`),
  gated by the `c-abi` workflow.

### Changed
- Relicensed 88 legacy `Examples/` files that still carried GPL-3.0 headers to
  MIT, making the repository uniformly MIT (the copyright holder's decision;
  resolves the contradiction with the root `LICENSE`).

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
[`docs/std-vector-whitelist.md`](https://github.com/lrleon/Aleph-w/blob/master/docs/std-vector-whitelist.md).
Affected signatures:

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
