# FFT Elite Plan

Status date: 2026-03-06

This document starts after the completion of `fft-plan.md`.
The previous plan made `fft.H` broad, serious, and releaseable.
This plan targets the stricter claim: elite industrial quality.

## Current Position

`fft.H` already provides:
- arbitrary-length complex FFT plus compact `rfft/irfft`
- reusable plans, batch transforms, axis/stride transforms, and 2-D / 3-D support
- STFT / ISTFT, stateful processing, overlap-add, overlap-save, and partitioned convolution
- FIR / IIR / SOS filtering and broad filter-design support
- benchmark, external differential validation, release audit, support matrix, and contracts

What still prevents a hard "elite industrial" claim is not API breadth.
It is platform validation depth, externally measured performance, real-time memory discipline, and packaging/distribution rigor.

## Definition Of Done

`fft.H` can be called elite industrial only when all of the following are true:

1. The same release is validated on native x86_64 and ARM64 hosts.
2. The library is benchmarked against serious external baselines on representative hardware.
3. SIMD backends are not only present, but consistently beneficial on the supported paths.
4. Real-time / low-allocation workflows are documented and enforceable.
5. CI covers compilers, architectures, sanitizers, and performance gates.
6. Packaging and versioned consumption are production-ready.

## Phases

| Phase | Goal | Impact | Risk | Cost |
|---|---|---:|---:|---:|
| E0 | Platform-validation matrix and CI discipline | Critical | Medium | L |
| E1 | External benchmark and competitiveness baseline | Critical | Medium | L |
| E2 | Backend maturation for x86_64 and ARM64 | Critical | High | XL |
| E3 | Real-time memory and allocator discipline | High | Medium | L |
| E4 | Packaging, versioning, and consumption contracts | High | Medium | L |
| E5 | Long-run robustness, fuzzing, and failure hardening | High | Medium | L |
| E6 | Final industrial sign-off | Critical | Medium | M |

Cost scale:
- `M`: days
- `L`: 1-2 weeks
- `XL`: several weeks

## Phase E0

Goal: make correctness claims platform-real, not host-local.

Deliverables:
- CI matrix for:
  - Linux x86_64
  - Linux ARM64
  - macOS ARM64 if available
  - GCC
  - Clang
  - MSVC if Windows support is intended
- sanitizer jobs:
  - ASAN
  - UBSAN
  - TSAN for thread-sensitive paths
- scripted native validation profiles for:
  - scalar
  - AVX2
  - NEON
- release artifact collection:
  - benchmark JSON
  - reference-cross-check output
  - support-matrix snapshot

Exit criteria:
- every supported compiler/architecture pair has an explicit validation record
- NEON is validated on real ARM hardware, not only compiled
- the support matrix reflects measured support, not intended support

## Phase E1

Goal: establish objective competitiveness against external references.

Deliverables:
- repeatable benchmark harness against:
  - FFTW
  - pocketfft or NumPy/SciPy
  - MKL or vDSP where applicable
- benchmark suites by:
  - precision (`float`, `double`)
  - size class (small, medium, large, prime, mixed-radix)
  - path (complex FFT, real FFT, STFT, convolution)
  - hardware family (x86_64, ARM64)
- stable export format and comparison scripts for external baselines
- documented expectations:
  - parity targets
  - acceptable loss regions
  - known weak cases

Exit criteria:
- performance claims are comparative, not absolute
- the project can answer where it is faster, where it is slower, and why

## Phase E2

Goal: make the backend genuinely top-tier on the claimed hardware.

Deliverables:
- AVX2 path tuned until it is consistently beneficial on the targeted throughput paths
- NEON path tuned and measured on real ARM64 hosts
- optional AVX-512 path, only if it provides measurable value without destabilizing maintenance
- better planner/backend selection:
  - size-aware
  - path-aware
  - hardware-aware
- optional Rader support for small primes if benchmarks justify it
- further locality work for:
  - STFT
  - partitioned convolution
  - multichannel filtering

Exit criteria:
- SIMD paths are measured wins on their intended workloads
- `auto` policy is data-driven and platform-specific
- no backend is marked supported without native benchmark evidence

## Phase E3

Goal: make `fft.H` suitable for stricter real-time and low-allocation workloads.

Deliverables:
- explicit allocation contracts for major APIs:
  - may allocate
  - amortized allocation only
  - no allocation after construction
- reusable external workspaces where meaningful
- allocator hooks or configurable aligned storage for heavy processors
- explicit real-time guidance for:
  - STFT processors
  - overlap-save / overlap-add
  - partitioned convolution
  - filter banks
- low-latency IR update policy for partitioned convolution, ideally with safe transition behavior

Exit criteria:
- users can select APIs by allocation behavior, not guesswork
- the main streaming paths can be kept allocation-stable after initialization

## Phase E4

Goal: make the library production-consumable as a dependency.

Deliverables:
- `install()` rules
- exported targets
- `find_package(Aleph-w CONFIG)` support
- version metadata and compatibility policy
- clear public/private header boundaries, even if `fft.H` remains the façade
- release notes template and migration notes

Exit criteria:
- downstream consumers can use the library without source-tree coupling
- release versioning and compatibility expectations are explicit

## Phase E5

Goal: harden failure behavior and numerical edge cases beyond the current test surface.

Deliverables:
- fuzz/property testing for:
  - FFT round-trips
  - STFT/ISTFT reconstruction
  - filter design argument surfaces
  - convolution equivalence
- explicit behavior tests for:
  - `NaN`
  - `Inf`
  - denormals
  - pathological filter orders
  - extreme dynamic range
- cross-platform tolerance envelopes, not only one host envelope
- longer-run stability tests for streaming/stateful processors

Exit criteria:
- pathological numeric inputs are either supported or explicitly rejected
- tolerance envelopes are platform-aware and reproducible

## Phase E6

Goal: close with evidence strong enough for an industrial claim.

Deliverables:
- one release audit command that also verifies the elite matrix
- consolidated report with:
  - supported compilers
  - supported architectures
  - benchmark parity/wins/losses
  - numeric envelopes by platform
  - known limits
- final sign-off checklist tied to actual artifacts

Exit criteria:
- release claims match measured artifacts
- there is no hidden "supported in principle" surface left undocumented

## Recommended Execution Order

1. E0
2. E1
3. E2
4. E3
5. E4
6. E5
7. E6

## Hard Judgment

If only three elite phases can be funded immediately, the correct choice is:

1. E0
2. E1
3. E2

That sequence determines whether the project is merely feature-rich or genuinely elite.

## Immediate Gaps

The most concrete gaps today are:
- native ARM64 benchmark and validation closure
- native Clang validation closure
- external performance comparison against serious FFT baselines
- stronger allocation/workspace contracts for streaming and low-latency paths
- production packaging and versioned consumption

## First Practical Step

Start with E0 and E1 together:
- run native ARM64 and Clang validation
- create external benchmark baselines
- update `fft-support-matrix.md` from "intended" to "measured"

Only after that is it rational to spend more time on backend tuning.
