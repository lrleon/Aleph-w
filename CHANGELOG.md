# Changelog

All notable changes to Aleph-w are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [7.2.1](https://github.com/lrleon/Aleph-w/compare/v7.2.0...v7.2.1) (2026-10-24)


### Bug Fixes

* **graph:** Make Floyd-Warshall handle parallel arcs and loops ([#139](https://github.com/lrleon/Aleph-w/issues/139)) ([2c4ccb5](https://github.com/lrleon/Aleph-w/commit/2c4ccb543db909025d6d74e250aa44a2e185c8dd))
* **graph:** Make Prim map its tree back and free its data on failure ([#140](https://github.com/lrleon/Aleph-w/issues/140)) ([aadcb40](https://github.com/lrleon/Aleph-w/commit/aadcb4032a3cf51ac259b9d07108e6d6964559ee))
* **graph:** Make Prim order arcs by its distance and paint its tree ([#137](https://github.com/lrleon/Aleph-w/issues/137)) ([c2960a4](https://github.com/lrleon/Aleph-w/commit/c2960a431a0cee797516e657bbca76d06383dfe6))

## [7.2.0](https://github.com/lrleon/Aleph-w/compare/v7.1.2...v7.2.0) (2026-10-22)


### Features

* **geom:** Enable the point.H formatters with libc++ 17 and 18 ([#129](https://github.com/lrleon/Aleph-w/issues/129)) ([2dd008e](https://github.com/lrleon/Aleph-w/commit/2dd008eafcba1c6ea2ba49da8dc8b10c3cea556e))
* **graphs:** Rewrite generate_df_tree.H to draw df and low trees ([#127](https://github.com/lrleon/Aleph-w/issues/127)) ([2f5436f](https://github.com/lrleon/Aleph-w/commit/2f5436fdb73220ea5f1b8bc065c39de802bc46cd))
* **hash:** Add set_with_resize() and shrink tables on remove(key) ([#130](https://github.com/lrleon/Aleph-w/issues/130)) ([0d27b83](https://github.com/lrleon/Aleph-w/commit/0d27b838152201c5fdcf4e85a6cf7a061d0e539b))


### Bug Fixes

* **trees:** Check the depth in generate_tree() before writing past its buffer ([#127](https://github.com/lrleon/Aleph-w/issues/127)) ([2f5436f](https://github.com/lrleon/Aleph-w/commit/2f5436fdb73220ea5f1b8bc065c39de802bc46cd))
* **hash:** Keep the capacity, without throwing or ending the program, when a removal cannot shrink the table for lack of memory ([#130](https://github.com/lrleon/Aleph-w/issues/130)) ([0d27b83](https://github.com/lrleon/Aleph-w/commit/0d27b838152201c5fdcf4e85a6cf7a061d0e539b))
* **hash:** Make DynLhashTable use its comparator, keep its configuration in copies and refuse a key already in the table ([#130](https://github.com/lrleon/Aleph-w/issues/130)) ([0d27b83](https://github.com/lrleon/Aleph-w/commit/0d27b838152201c5fdcf4e85a6cf7a061d0e539b))
* **hash:** Check the pointer given to ODhashTable::remove_ptr() ([#130](https://github.com/lrleon/Aleph-w/issues/130)) ([0d27b83](https://github.com/lrleon/Aleph-w/commit/0d27b838152201c5fdcf4e85a6cf7a061d0e539b))
* **hash:** Keep busy_slots() of the linear hash table equal to its busy lists ([#130](https://github.com/lrleon/Aleph-w/issues/130)) ([0d27b83](https://github.com/lrleon/Aleph-w/commit/0d27b838152201c5fdcf4e85a6cf7a061d0e539b))
* **hash:** Stop the chained tables from rebuilding at the same size or growing back when they shrink ([#130](https://github.com/lrleon/Aleph-w/issues/130)) ([0d27b83](https://github.com/lrleon/Aleph-w/commit/0d27b838152201c5fdcf4e85a6cf7a061d0e539b))

## [7.1.2](https://github.com/lrleon/Aleph-w/compare/v7.1.1...v7.1.2) (2026-10-20)


### Bug Fixes

* **primes:** Stop skipping n + 1 in next_prime_number_greater_than() ([#125](https://github.com/lrleon/Aleph-w/issues/125)) ([0d36f8e](https://github.com/lrleon/Aleph-w/commit/0d36f8e823bcc381c0c50f3d9f807318044280e8))
* **primes:** Stop the int counter of is_prime() from overflowing ([#125](https://github.com/lrleon/Aleph-w/issues/125)) ([0d36f8e](https://github.com/lrleon/Aleph-w/commit/0d36f8e823bcc381c0c50f3d9f807318044280e8))

## [7.1.1](https://github.com/lrleon/Aleph-w/compare/v7.1.0...v7.1.1) (2026-10-19)


### Bug Fixes

* **build:** Ship libalephca_c and make the .deb install on more distros ([#122](https://github.com/lrleon/Aleph-w/issues/122)) ([c7f2856](https://github.com/lrleon/Aleph-w/commit/c7f285642c3ef79463e877de8ce601802ca23ce5))

## [7.1.0](https://github.com/lrleon/Aleph-w/compare/v7.0.1...v7.1.0) (2026-10-18)


### Features

* **concurrency:** Deliver cancellation to condition-variable waiters ([#119](https://github.com/lrleon/Aleph-w/issues/119)) ([11c5c01](https://github.com/lrleon/Aleph-w/commit/11c5c0161235519e5089f64974d209d777e83cfc))


### Bug Fixes

* **hash:** Count removed keys' buckets and stop del() at the last key ([#121](https://github.com/lrleon/Aleph-w/issues/121)) ([d37658b](https://github.com/lrleon/Aleph-w/commit/d37658bf9ad0a64f19717687bee16b4b7c618472))


### Deprecated

* `CancellationToken::notify_on_cancel(std::condition_variable &)` (`thread_pool.H`): use `notify_on_cancel(cv, mutex)`, which cannot miss the notification. Removal no earlier than 8.0.0. Taking the address of `notify_on_cancel` now needs the type of the overload; see `docs/release-notes/7.1.0.md`.

## [7.0.1](https://github.com/lrleon/Aleph-w/compare/v7.0.0...v7.0.1) (2026-10-17)


### Bug Fixes

* **build:** Export C++20 and make the headers compile alone and together ([#117](https://github.com/lrleon/Aleph-w/issues/117)) ([dc63843](https://github.com/lrleon/Aleph-w/commit/dc638433eef186b6401cd86419bf16a2b7a8bf3d))
* **concurrency:** Deliver cancellations safely and recover failed starts ([#116](https://github.com/lrleon/Aleph-w/issues/116)) ([da6a686](https://github.com/lrleon/Aleph-w/commit/da6a6867bb9ef0e35cf4fc4a5a08906453ff5d64))
* **containers:** Keep FlatMap, FlatSet and tree sets valid under aliasing, exceptions and degenerate shapes ([#109](https://github.com/lrleon/Aleph-w/issues/109)) ([259d152](https://github.com/lrleon/Aleph-w/commit/259d15207660b92eef6379376d6b61e397f238c9))
* **docs:** Correct build and versioning docs and brace error macros ([#118](https://github.com/lrleon/Aleph-w/issues/118)) ([b7c9afb](https://github.com/lrleon/Aleph-w/commit/b7c9afb72107c8bb824dd0d11a9806957b8aaa88))
* **graph:** Use the given distance and free cookies in shortest paths ([#113](https://github.com/lrleon/Aleph-w/issues/113)) ([3c011f5](https://github.com/lrleon/Aleph-w/commit/3c011f5fe0f9e2cd7669354bae6bdb122e30a4e6))
* **hash:** Handle failed insertions, full tables and shrinking in open addressing tables ([#112](https://github.com/lrleon/Aleph-w/issues/112)) ([e2b1280](https://github.com/lrleon/Aleph-w/commit/e2b12806a8196e02ab9348c2d7dd444fc747db29))
* **hash:** Keep hash tables consistent through copy, move and swap ([#111](https://github.com/lrleon/Aleph-w/issues/111)) ([c6291a4](https://github.com/lrleon/Aleph-w/commit/c6291a47a40cfa0b1c31d7580a6b7c81e703e747))
* **overflow:** Make pattern readers and engines report integer overflow ([#114](https://github.com/lrleon/Aleph-w/issues/114)) ([f6810cb](https://github.com/lrleon/Aleph-w/commit/f6810cb5dd1c553dfe4abf5a239548ec6ebec312))

## [7.0.0](https://github.com/lrleon/Aleph-w/compare/v6.1.0...v7.0.0) (2026-10-12)


### ⚠ BREAKING CHANGES

* **array:** Overhaul MemArray, Array, and ArrayQueue ([#108](https://github.com/lrleon/Aleph-w/issues/108))

### Features

* **array:** Overhaul MemArray, Array, and ArrayQueue ([#108](https://github.com/lrleon/Aleph-w/issues/108)) ([3503de8](https://github.com/lrleon/Aleph-w/commit/3503de8fb3adbf39aa52adf1c32af6f465381852))
* **fuzz:** Add libFuzzer harnesses for DynArray, sorting, compiler front end and CSV utilities ([#105](https://github.com/lrleon/Aleph-w/issues/105)) ([21e388a](https://github.com/lrleon/Aleph-w/commit/21e388a73a862c1d13788747d2c0f0bbbd808b17))


### Bug Fixes

* **ca:** Bound the live cells read_rle stores ([#107](https://github.com/lrleon/Aleph-w/issues/107)) ([d8f113f](https://github.com/lrleon/Aleph-w/commit/d8f113fd2142896e6399202dde24f918c2f3f5f4))

## [6.1.0](https://github.com/lrleon/Aleph-w/compare/v6.0.0...v6.1.0) (2026-10-04)


### Features

* **core:** Add arbitrage cycle detection ([#103](https://github.com/lrleon/Aleph-w/issues/103)) ([b758d85](https://github.com/lrleon/Aleph-w/commit/b758d8520bb0a4cb8d85724d4c03f864a2236052))

## [6.0.0](https://github.com/lrleon/Aleph-w/compare/v5.11.0...v6.0.0) (2026-09-28)


### ⚠ BREAKING CHANGES

* add TikZ galleries and tighten container APIs ([#98](https://github.com/lrleon/Aleph-w/issues/98))

### Features

* add TikZ galleries and tighten container APIs ([#98](https://github.com/lrleon/Aleph-w/issues/98)) ([97cd79b](https://github.com/lrleon/Aleph-w/commit/97cd79b2bd057c7795715de2c39d115c95c238c9))

## [5.11.0](https://github.com/lrleon/Aleph-w/compare/v5.10.0...v5.11.0) (2026-07-20)


### Features

* **core:** agregar MemorySanitizer, mejorar Regular_Polygon, QuadTre… ([#95](https://github.com/lrleon/Aleph-w/issues/95)) ([334bed0](https://github.com/lrleon/Aleph-w/commit/334bed03f2c0d919cdb63e766798db38107d4b50))

## [5.10.0](https://github.com/lrleon/Aleph-w/compare/v5.9.0...v5.10.0) (2026-07-19)


### Features

* **core:** Implement R-tree and R*-tree data structures ([#93](https://github.com/lrleon/Aleph-w/issues/93)) ([d8389ea](https://github.com/lrleon/Aleph-w/commit/d8389ea2e0c21ce647f856e30fa563f278614f03))

## [5.9.0](https://github.com/lrleon/Aleph-w/compare/v5.8.0...v5.9.0) (2026-07-14)


### Features

* **core:** Add persistent treap and vector containers ([#88](https://github.com/lrleon/Aleph-w/issues/88)) ([f0e3e7f](https://github.com/lrleon/Aleph-w/commit/f0e3e7f724a8802485d01e5ad24b38689ccec070))

## [5.8.0](https://github.com/lrleon/Aleph-w/compare/v5.7.0...v5.8.0) (2026-07-12)


### Features

* **core:** Add Rope data structure implementation ([#87](https://github.com/lrleon/Aleph-w/issues/87)) ([74a40dc](https://github.com/lrleon/Aleph-w/commit/74a40dcdd279be4b4fc165a7e688eb741185903a))
* **core:** Implement Radix and Patricia Tries ([a9246ec](https://github.com/lrleon/Aleph-w/commit/a9246ec5c95f1cc57c978d678a3628be4355e190))
* **core:** Implement Radix and Patricia Tries ([79c00de](https://github.com/lrleon/Aleph-w/commit/79c00de6af4ddb6768379d98bef07708edd95e92))
* **core:** Implement Radix and Patricia Tries ([#85](https://github.com/lrleon/Aleph-w/issues/85)) ([09d58c2](https://github.com/lrleon/Aleph-w/commit/09d58c2b12793b551731d0d9d8e9281a63d554a9))

## [5.7.0](https://github.com/lrleon/Aleph-w/compare/v5.6.0...v5.7.0) (2026-07-07)


### Features

* **core:** Add concurrent hash map example and tests ([3e2a7c6](https://github.com/lrleon/Aleph-w/commit/3e2a7c64b6724a4cece37aa78002aafe7d542878))
* **core:** Add concurrent hash map example and tests ([7c16eee](https://github.com/lrleon/Aleph-w/commit/7c16eee86fddbadfda4df8151b13f4292881f0a6))
* **core:** Implement lock-free MPSC queue ([6ea8125](https://github.com/lrleon/Aleph-w/commit/6ea81251651067e6c9a8112b49fe166956890fe1))

## [5.6.0](https://github.com/lrleon/Aleph-w/compare/v5.5.0...v5.6.0) (2026-07-07)


### Features

* **mdspan:** Implement mdspan views and examples ([#75](https://github.com/lrleon/Aleph-w/issues/75)) ([cdaae30](https://github.com/lrleon/Aleph-w/commit/cdaae305267276421e1cf687f8fa782b5559c78b))

## [5.5.0](https://github.com/lrleon/Aleph-w/compare/v5.4.0...v5.5.0) (2026-07-05)


### Features

* **core:** Add coroutine generators ([#73](https://github.com/lrleon/Aleph-w/issues/73)) ([7965ec2](https://github.com/lrleon/Aleph-w/commit/7965ec2010609022adb724df676860d461b4a34f))

## [5.4.0](https://github.com/lrleon/Aleph-w/compare/v5.3.0...v5.4.0) (2026-07-03)


### Features

* **core:** Add flat_map, flat_set, ring_buffer, small_vector ([#71](https://github.com/lrleon/Aleph-w/issues/71)) ([4bf55e0](https://github.com/lrleon/Aleph-w/commit/4bf55e0dac7a0e4aef00edf691ea98337b42bb59))

## [5.3.0](https://github.com/lrleon/Aleph-w/compare/v5.2.0...v5.3.0) (2026-07-01)


### Features

* **core:** Add expected parsing for CA IO ([#68](https://github.com/lrleon/Aleph-w/issues/68)) ([9165f0c](https://github.com/lrleon/Aleph-w/commit/9165f0ccdd1b9170777ef42b668150e0fb2ea1d5))

## [5.2.0](https://github.com/lrleon/Aleph-w/compare/v5.1.0...v5.2.0) (2026-06-30)


### Features

* **cpp-compat:** add polyfill for std::expected with C++23 monadic ops ([#66](https://github.com/lrleon/Aleph-w/issues/66)) ([a11be29](https://github.com/lrleon/Aleph-w/commit/a11be292c5570be22e6db43e5750819093564925))

## [5.1.0](https://github.com/lrleon/Aleph-w/compare/v5.0.0...v5.1.0) (2026-06-29)


### Features

* Windows/MSVC and clang-cl port for Aleph-w ([#58](https://github.com/lrleon/Aleph-w/issues/58)) ([342b831](https://github.com/lrleon/Aleph-w/commit/342b831e6733ac80178fdb6677224858edf7cc84))
* Flat, versioned C ABI (`libalephca_c` / `ca-c-api.h`) exposing the cellular-automata engine to any language with a C FFI, plus a pure-C smoke test and experimental Rust bindings (`bindings/rust/`).
* Cellular-automata documentation: a six-chapter tutorial (`docs/tutorial/`), a performance tuning guide (`docs/performance.md`), and a reproducible 16-rule visual gallery (`docs/gallery/`) with a `docs-build` link-check workflow.
* Property-based tests and a libFuzzer harness (`Tests/fuzz/`) for the RLE/Life/CSV/checkpoint parsers.
* OSS compliance: `SECURITY.md`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, Dependabot, REUSE 3.x licensing with a `reuse-lint` gate, and a reproducible-build check.
* Coverage: Codecov upload and a per-module (`tpl_ca_*.H` vs `ca-*.H`) coverage breakdown.

### Security

* Checkpoint loader hardened against unbounded allocation (CWE-789 / CWE-190): `read_raw_payload` now bounds the declared `payload_size` against the actual file size and `inspect_checkpoint` validates `cell_count` against the extents. Surfaced by the new checkpoint fuzzer; regression tests added.

### Bug Fixes

* Relicensed 88 legacy `Examples/` files from GPL-3.0 to MIT so the repository is uniformly MIT.
* `Tarjan.H`: fixed two latent `_stack`/`stack_` typos that broke Debug builds instantiating `connected_components`/`test_connectivity`, and corrected the SCC documentation.

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
