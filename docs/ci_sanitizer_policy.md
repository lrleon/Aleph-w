# CI sanitizer and platform skip policy

This document is the single source of truth for *which* tests CI excludes
on *which* sanitizer / platform combinations, plus the rationale for each
exclusion. It is referenced from `.github/workflows/ci.yml` and
`.github/workflows/ci-platform.yml`.

The current shape was driven by Phase 16 of the CA industrial roadmap
(an internal planning document kept outside the repository).

## Sanitizer matrix

| Job   | Compiler | Sanitizer            | Source flag                           | Notes |
|-------|----------|----------------------|---------------------------------------|-------|
| `sanitizers` | gcc   | ASan + UBSan combined | `-DALEPH_USE_SANITIZERS=ON`           | Existing combined run. ASan dominates so UBSan signals can be noisy. |
| `memory-sanitizer` | clang | MemorySanitizer | `-DALEPH_USE_MEMORY_SANITIZER=ON` | Targeted `Regular_Polygon` default-state probe; intentionally independent of prebuilt GoogleTest. |
| `tsan`       | clang | ThreadSanitizer       | `-fsanitize=thread`                   | Targets the CA `Parallel_Engine`, `thread_pool` and graph automaton concurrency. |
| `ubsan`      | clang | UBSan only            | `-fsanitize=undefined -fno-sanitize-recover=undefined` | Split out from the combined job so the signal is not masked by ASan. |

### Why split UBSan from the combined ASan+UBSan?

Two reasons:

1. **Cleaner signal.** When both sanitizers are active and ASan fires
   first, UBSan reports often arrive after the process has already been
   terminated, making them hard to interpret.
2. **Different fail-fast policy.** UBSan with `halt_on_error=1` plus
   `-fno-sanitize-recover=undefined` aborts the run on the first
   diagnostic. That is desirable for UBSan (signed overflow / vptr
   violations are bugs that mask others), but unwanted with ASan, where
   memory-leak reports legitimately come at process exit.

## Skipped tests by sanitizer

No sanitizer job excludes a test by name. `memory-sanitizer` builds and
runs a single test (see its section below); the others run every test that
their compiler builds (see
[Tests that only some compilers build](#tests-that-only-some-compilers-build)).
A test case that cannot run under a sanitizer skips itself, as the cases
below do (see [Source-level skips](#source-level-skips)).

A `ctest -E` regex, where one is needed, must match the names that CTest
gets from `gtest_discover_tests`, which are GoogleTest names
(`Suite.Test`; `ctest -N` lists them), not the name of the test program.
Until 7.0.1 the workflows excluded `hash_statistical_test`,
`tpl_ca_parallel_engine_test.*Stress`, `ca_animation_sinks_test`,
`file_b_tree_test`, `file_bplus_tree_test` and `timeoutQueue_test`, and
none of those patterns matched a test, so those tests already ran
everywhere they are built.

### `tsan` (ThreadSanitizer)

`HashStatisticalTest`, the statistical sweep of `hash_statistical_test`,
takes about a minute under TSan.

`prefix_tree_test` skips 4 individual cases (`InsertWordCleansDetachedPathOnAllocationFailure`,
`CloneCleansPartialCopyOnAllocationFailure`, `OwnsRootAndDestroysNodes`,
`CopyAssignmentKeepsOldTreeOnAllocationFailure`) via `GTEST_SKIP()` inside
the test source itself, not a `ctest -E` regex: those tests rely on
`prefix_tree_test.cc`'s own global `operator new`/`delete` overrides to
inject allocation failures, which collide at link time ("multiple
definition") with TSan's own strong `operator new`/`delete` replacements
(`tsan_new_delete.cpp.o`). The overrides are compiled out under TSan
(detected via `__SANITIZE_THREAD__` / `__has_feature(thread_sanitizer)`),
so the whole binary still links; the 4 cases that depend on them report
`SKIPPED` at run time instead.

`tree_allocation_test` follows the same pattern: it counts allocations by
replacing the global allocation functions, so under TSan (and under ASan,
MSan and HWASan, which also bring their own allocator) the replacement is
compiled out and the 4 cases that count allocations
(`constructing_a_tree_does_not_allocate`, `moving_a_tree_does_not_allocate`,
`insert_and_remove_allocate_only_the_node`, `iterating_does_not_allocate`)
report `SKIPPED`. `noexcept_specifications_hold` still runs.

### `ubsan` (UndefinedBehaviorSanitizer)

Runs every test it builds (Clang does not build `k2tree_test`).

### `sanitizers` (combined ASan + UBSan, gcc)

Runs every test. The 4 allocation-counting cases of `tree_allocation_test`
report `SKIPPED` here too (see the `tsan` section).

### `memory-sanitizer` (MemorySanitizer, clang)

This configuration statically checks `polygon.H`, then builds and runs only
`MemorySanitizer.RegularPolygonDefaultState`. The narrow dynamic scope is
deliberate: the system GoogleTest package and several external libraries are
not compiled with MemorySanitizer and otherwise report false positives before
Aleph tests start. The standalone probe reads every publicly observable scalar
in the default `Regular_Polygon` state without linking GoogleTest. Cppcheck
covers the private `angle_` and `beta_` members that the empty object's public
contract cannot read. Its `nullPointer` diagnostic for the two dereferences in
`Polygon::remove_vertex` (the preceding Aleph error macro already throws when
the pointer is null) is suppressed inline at each site via
`// cppcheck-suppress nullPointer`, so the check still covers the rest of
`polygon.H`.

## Skipped tests by platform

Of the cross-platform jobs in `ci-platform.yml`, only the Windows one
leaves tests out because of the platform: those that depend on POSIX-only
facilities or on programs that the runner does not provide. The macOS and
ARM64 jobs run every test they build, and with Clang they do not build all
of them (see
[Tests that only some compilers build](#tests-that-only-some-compilers-build)).

### `build-macos` (`macos-15`, `macos-15-intel`)

Runs every test it builds; AppleClang does not build `k2tree_test`. X11
is unavailable on the GitHub-hosted runners (no XQuartz) and ffmpeg is not
installed, but `ca_animation_sinks_test` needs neither: it checks that the
ffmpeg sink and the X11 viewer do nothing when they are disabled.

### `build-arm64-linux` (`ubuntu-24.04-arm`)

Same dependency surface as the gating Linux matrix; runs every test it
builds, which with Clang and libc++ leaves out `k2tree_test` and
`gmpfrxx_test`.

### `build-windows` (`windows-2022` × `msvc`, `clang-cl`)

The Aleph-w Windows port is restricted to the headless subset.

Two skip levels exist on Windows:

1. **Compile-level** (`ALEPH_POSIX_ONLY_TESTS` in `Tests/CMakeLists.txt`):
   tests whose sources or transitive Aleph headers require POSIX-only
   facilities and do not compile under MSVC/clang-cl. They are removed
   from the build entirely. The POSIX-only library translation units
   (`ahDaemonize.C`, `socket_wrappers.C`, `useMutex.C`, `timeoutQueue.C`)
   are likewise excluded from `libAleph` in the root `CMakeLists.txt`.

| Test (compile-level skip)                       | Reason |
|-------------------------------------------------|--------|
| `ah_date_test`                                  | `ah-date.H` wraps POSIX `strptime`. |
| `ah_signal_test`                                | POSIX signals. |
| `ca_checkpoint_safety_test`                     | `fork()`-based crash-safety harness. |
| `concurrency_utils_test`                        | pthread wrappers (`useMutex.H`/`useCondVar.H`). |
| `file_b_tree_test`, `file_bplus_tree_test`      | Use `<sys/mman.h>` for memory-mapped storage tests; no Windows replacement currently. |
| `net_utils_test`                                | BSD sockets (`socket_wrappers.H`). |
| `ringfilecache`                                 | `ringfilecache.H` uses `<sys/time.h>`. |
| `timeoutQueue_test`                             | Uses `<sys/select.h>` for timing assertions. |
| `skiplist_test`                                 | `tpl_skipList.H` derives from a class with a zero-sized trailing array (`forward[0]`), which MSVC rejects (C2503). |
| `ntt_test`                                      | `ntt.H` requires `__uint128_t`. |
| `montgomery_test`                               | clang-cl defines `__SIZEOF_INT128__` but lacks the `__umodti3` runtime. |
| `gmpfrxx_test` and the geometry tests (`point_test`, `line_test`, `segment_test`, `polygon_test`, `quadtree_test`, `eepicgeom_test`, `euclidian_graph_test`, `robust_predicates_test`, `tikzgeom_test`, `tikzgeom_algorithms_test`, `geom_algorithms_test*`) | Need `gmpfrxx.C` (not built on Windows) or the compiled C++ operators of `libgmpxx` (`operator<<` on `mpq`/`mpz`), whose MinGW mangling `link.exe`/`lld-link` cannot resolve. |

A test that only needs to *print* a GMP value on failure (through
GoogleTest) can stay on Windows by asserting a `bool` instead, as
`point_utils_header_test` does.

2. **Run-level** (`ctest -E` filter in `ci-platform.yml`): tests that
   compile but cannot run on the CI runner.

| Test (CTest regex)                              | Reason for skip |
|-------------------------------------------------|-----------------|
| `PlanarityTest\.External`, `PlanarityTest\.ExternalGephi` | Run `ruby` and `python3` through `std::system` with POSIX shell redirections (`> /dev/null 2>&1`). |
| `CompilerSdk\.InstallHeadersSmoke`              | Compiles a program against the installed headers with GCC-style options (`-std=c++20`, `-I`). |
| `ThreadPoolTest\.BenchmarkVsStdAsync`           | Benchmark against `std::async`; left out with the Windows port (#58), with no recorded reason. |

The Windows job sets `-DALEPH_BUILD_X11_VIEWER=OFF` so the library does
not pull `libX11`. `ca-x11-viewer.H` is then compiled as a no-op stub
guarded by `__has_include(<X11/Xlib.h>)`.

### Tests that only some compilers build

Outside macOS the system `libgmpxx` is built with libstdc++, so its
compiled C++ operators do not link with libc++. `Tests/CMakeLists.txt`
therefore leaves out:

| Test | Condition |
|------|-----------|
| `k2tree_test`  | Built only with GCC: no Clang, AppleClang or Windows job builds it. |
| `gmpfrxx_test` | Skipped when Clang uses libc++ (`ALEPH_USE_LIBCXX`, or `-stdlib=libc++` in `CMAKE_CXX_FLAGS` or in the flags of the active build type) on a non-Apple host: the Clang jobs of `build-and-test` in `ci.yml` and of `build-arm64-linux`. Homebrew's GMP on macOS is built with libc++, so the test runs there. |

## Re-enabling a skipped test

1. Update the matching CTest regex in the workflow file.
2. Remove the corresponding row from this document.
3. Add an entry to the PR description explaining what changed (in the
   test or in the dependency surface) that unblocks the skip.

## Source-level skips

When a test case cannot run in some configuration because of the code
itself (for example, it replaces the global `operator new`, which the
sanitizer also replaces), prefer a `GTEST_SKIP()` in the test, guarded by
the macro that detects the configuration and with the reason in its
message, over a workflow-level filter, as `prefix_tree_test.cc` and
`tree_allocation_test.cc` do. The case then reports `SKIPPED` instead of
disappearing from the run.
