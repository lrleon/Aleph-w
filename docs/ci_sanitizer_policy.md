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

Each skip below is annotated in the workflow YAML with
`# sanitizer:skip-<name>` plus a one-line justification. Updating this
table without also updating the inline comment is a review block.

### `tsan` (ThreadSanitizer)

| Test (CTest regex)                              | Reason for skip |
|-------------------------------------------------|-----------------|
| `hash_statistical_test`                         | Long-running statistical sweep; 10× the runtime under TSan with no race signal. |
| `tpl_ca_parallel_engine_test.*Stress`           | Intentionally oversubscribes the work-stealing pool; the relaxations there are benign and produce noise rather than findings. |

### `ubsan` (UndefinedBehaviorSanitizer)

| Test (CTest regex)                              | Reason for skip |
|-------------------------------------------------|-----------------|
| `hash_statistical_test`                         | Runtime cost only; UBSan offers no signal that the regular Debug build does not already cover. |

### `sanitizers` (combined ASan + UBSan, gcc)

| Test (CTest regex)                              | Reason for skip |
|-------------------------------------------------|-----------------|
| `hash_statistical_test`                         | Same rationale as above. |

## Skipped tests by platform

The cross-platform jobs in `ci-platform.yml` exclude tests that depend
on POSIX-only system calls or on optional GUI/multimedia dependencies
that are not packaged for the runner.

### `build-macos` (`macos-13`, `macos-14`)

X11 is unavailable on the GitHub-hosted runners (no XQuartz).

| Test (CTest regex)                              | Reason for skip |
|-------------------------------------------------|-----------------|
| `hash_statistical_test`                         | Runtime cost. |
| `ca_animation_sinks_test`                       | Exercises the `Ffmpeg_Frame_Sink` + the X11 viewer hooks; ffmpeg is not installed and there is no X11. |

### `build-arm64-linux` (`ubuntu-24.04-arm`)

Same dependency surface as the gating Linux matrix; no additional
skips beyond `hash_statistical_test`.

### `build-windows` (`windows-2022` × `msvc`, `clang-cl`)

The Aleph-w Windows port is restricted to the headless subset. The
following tests are filtered out because they bind to POSIX system
calls or to X11/ffmpeg.

| Test (CTest regex)                              | Reason for skip |
|-------------------------------------------------|-----------------|
| `hash_statistical_test`                         | Runtime cost. |
| `ca_animation_sinks_test`                       | Requires libX11 and an external `ffmpeg` process. |
| `file_b_tree_test`, `file_bplus_tree_test`      | Use `<sys/mman.h>` for memory-mapped storage tests; no Windows replacement currently. |
| `timeoutQueue_test`                             | Uses `<sys/select.h>` for timing assertions. |

The Windows job sets `-DALEPH_BUILD_X11_VIEWER=OFF` so the library does
not pull `libX11`. `ca-x11-viewer.H` is then compiled as a no-op stub
guarded by `__has_include(<X11/Xlib.h>)`.

## Re-enabling a skipped test

1. Update the matching CTest regex in the workflow file.
2. Remove the corresponding row from this document.
3. Add an entry to the PR description explaining what changed (in the
   test or in the dependency surface) that unblocks the skip.

## Source-level skip annotations

When a skip is *inevitable in the code itself* (for example because a
test exercises behaviour that TSan deliberately mis-reports), prefer a
local annotation in the test source over a workflow-level filter:

```cpp
// sanitizer:skip-tsan — work-stealing relaxation; the benign race here
// is documented in tpl_ca_parallel_engine.H §"Memory model".
```

These comments are intentionally grep-able. CI surveys them weekly to
confirm the workflow regex and the source annotations stay in sync.