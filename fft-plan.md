# FFT Industrial Plan

Status update 2026-03-06:
- Phase 0 is now materially implemented: `ctest` registration works in `build-test-clean`, `Examples/fft_benchmark` provides timing/validation baselines, `scripts/fft_reference_check.rb` cross-validates against NumPy when available, and `ALEPH_USE_SANITIZERS` is available at the root CMake level.
- Phase 1 is also materially implemented: arbitrary-length complex FFT dispatch now uses mixed-radix `2/3/4/5` when possible and Bluestein fallback otherwise, `FFT::Plan` accepts arbitrary positive sizes, and compact `rfft/irfft` APIs are available.
- Phase 0 has been hardened further: `fft_benchmark --json` now emits a stable schema for CI/performance regression workflows, and `scripts/fft_perf_regression.rb` compares baseline vs current runs with configurable slowdown thresholds.
- Phase 2 is now complete by the criteria of this plan: reusable-plan batch FFT APIs cover complex, compact real, and full-spectrum real inverse transforms; SIMD backends are available through explicit runtime policy plus hardware detection (`auto|scalar|avx2|neon`) with compiled/runtime capability reporting; STFT uses cache-tiling plus small-size and total-work guards for parallel `pstft()`; `OverlapAddBank` shares one FIR spectrum across channels and avoids redundant slices/transforms for undersized multichannel jobs; and the measured throughput profile shows clear gains on the throughput-oriented batch paths while conservative fallback keeps unstable cases on scalar code.
- Phase 3 is now complete by the criteria of this plan: flat complex buffers can be transformed over arbitrary axes with explicit `shape/strides` layouts, 2-D and 3-D complex wrappers are available for rectangular Aleph matrices/tensors, batched 2-D transforms can be expressed directly either through nested matrix stacks or `transform_axes()` on leading batch dimensions, and multichannel STFT/ISTFT now supports both channel-major and frame-major spectrogram layouts without manual repacking.
- Phase 4 is now complete by the criteria of this plan: polynomial roots use a balanced monic path with Aberth-Ehrlich iteration, Durand-Kerner fallback, Newton polishing, and residual checks; direct transfer evaluation applies explicit coefficient scaling on the unit circle; SOS delay diagnostics are analytic and section-centric instead of relying on sampled `freqz()` output; gain and phase margins for coefficient and SOS APIs refine crossover locations against dense seeds; and deterministic randomized tests cover conditioned root recovery, SOS delay consistency, stability margins, and refined-margin agreement against high-resolution references.
- Phase 5 is now complete by the criteria of this plan: `fft.H` now includes Kaiser windows, `firwin_lowpass/highpass/bandpass/bandstop`, general weighted least-squares FIR design via `firls()`, dense-grid equiripple FIR design via `remez()`, and Butterworth/Chebyshev-I/Chebyshev-II/Bessel/Elliptic low/high/band-pass/band-stop design returned as SOS, all with validation coverage for representative response-shape checks and argument validation.
- Phase 6 is now complete by the criteria of this plan: `fft.H` now exposes production DSP utilities including window metrics (`window_energy`, coherent gain, ENBW), explicit framing helpers (`frame_offsets`, `overlap_add_frames`), one-sided spectral estimators (`welch`, `csd`, `coherence`), rational-rate conversion (`upfirdn`, `resample_poly`), overlap-save FIR convolution, and a reusable low-latency `PartitionedConvolver`, all backed by validation against direct references plus example coverage.
- Phase 7 is now complete by the criteria of this plan: the library now carries explicit release-facing contracts (`fft-contracts.md`), a support matrix (`fft-support-matrix.md`), and a technical changelog (`fft-changelog.md`); external differential validation and benchmark validation are exposed as dedicated `FFTRelease.*` CTest entries; and `scripts/fft_release_audit.rb` executes the documented release audit end-to-end from one command.

If the goal is truly "elite industrial" level, the work should be staged.

## Phases

| Phase | Goal | Impact | Risk | Cost |
|---|---|---:|---:|---:|
| 0 | Baseline, benchmarks, cross-validation | Very high | Low | M |
| 1 | Arbitrary-length FFT + compact `rfft/irfft` | Critical | High | XL |
| 2 | SIMD + cache-aware kernels + fast batch path | Critical | High | XL |
| 3 | Production API for layouts, axes, and N-D | High | Medium | L-XL |
| 4 | Numerical robustness and serious diagnostics | High | Medium | L |
| 5 | Complete filter-design layer | High | Medium | XL |
| 6 | Production DSP utilities | High | Medium | XL |
| 7 | Final hardening and release discipline | Critical | Medium | L |

Cost scale:
- `M`: days
- `L`: 1-2 weeks
- `XL`: several weeks

## Phase 0

Goal: establish a measurable baseline before touching the FFT core.

Deliverables:
- `ctest` registered and usable end-to-end.
- Benchmark suite by size, type, and path:
  - complex FFT
  - real FFT
  - convolution
  - STFT
  - filters
- Automatic comparison against external references when available.
- Tolerance matrix for `float`, `double`, and `long double`.
- Sanitizers and debug/release validation.

Exit criteria:
- Performance and accuracy are reported with numbers, not intuition.
- Bottlenecks and error envelopes are known.

## Phase 1

Goal: remove the strongest current limitation, namely power-of-two-only FFTs.

Deliverables:
- Mixed-radix support for `2/3/4/5`.
- Bluestein for arbitrary sizes.
- Optional Rader path for small primes.
- Compact `rfft/irfft` layout of size `N/2 + 1`.
- Reusable plans for non-power-of-two lengths.

Exit criteria:
- Any reasonable `N` works.
- Real FFT APIs no longer require a full Hermitian spectrum layout.
- Accuracy and speed are acceptable against the baseline.

## Phase 2

Goal: real throughput.

Deliverables:
- SIMD kernels:
  - AVX2 first
  - AVX-512 optional
  - NEON if ARM matters
- CPU feature dispatch.
- Alignment and cache blocking.
- Optimized batched path for many FFTs of equal length.
- Fewer temporaries and better locality.

Exit criteria:
- Clear and repeatable benchmark gains.
- No numerical regressions.
- Competitive performance on common sizes.

## Phase 3

Goal: production-grade API, not just correct primitives.

Deliverables:
- Transforms over arbitrary axes and strides.
- Multidimensional batched transforms.
- 2-D and 3-D FFTs.
- More flexible STFT/ISTFT layouts.
- Better real multichannel support.

Exit criteria:
- Audio, image, and multichannel signal workflows fit without awkward repacking.

## Phase 4

Goal: serious numerical rigor.

Deliverables:
- Replace or complement the current root solver with a more robust method.
- SOS-centric analysis without unnecessary global polynomial reconstruction.
- More exact delay and margin evaluation.
- Explicit scaling policy for conditioning and overflow/underflow.
- Aggressive conditioning tests and fuzz/property testing.

Exit criteria:
- Sensitive routines no longer rely on favorable numerical luck.

## Phase 5

Goal: complete filter-design support.

Deliverables:
- Butterworth and Chebyshev I/II:
  - low-pass
  - high-pass
  - band-pass
  - band-stop
- Elliptic filters.
- Bessel filters.
- FIR design:
  - `firwin`
  - Kaiser
  - least-squares
  - Remez for a higher-end target
- SOS as the preferred output representation.

Exit criteria:
- `fft.H` can design filters seriously, not only execute them.

## Phase 6

Goal: practical DSP utilities for production pipelines.

Deliverables:
- `welch`, `csd`, and `coherence`
- `resample_poly` and `upfirdn`
- `overlap_save`
- Low-latency partitioned convolution
- More complete framing and windowing helpers

Exit criteria:
- Real DSP pipelines can be built directly on top of the header.

## Phase 7

Goal: finish as a serious library.

Deliverables:
- Documentation of complexity, precision, and contracts.
- Documented thread-safety guarantees.
- Differential tests against external references.
- Performance regression gates.
- Technical changelog and support matrix.

Exit criteria:
- The library can be released without caveats.

## Recommended Execution Order

1. Phase 0
2. Phase 1
3. Phase 2
4. Phase 4
5. Phase 3
6. Phase 5
7. Phase 6
8. Phase 7

## Why This Order

- Without Phase 0, improvements are not measurable.
- Without Phases 1 and 2, the FFT backend is not elite.
- Phase 4 avoids building more API surface over numerically fragile internals.
- Phases 5 and 6 matter, but only after the backend is strong.

## Hard Judgment

If only three phases could be funded immediately, the correct choice is:

1. Phase 0
2. Phase 1
3. Phase 2

That is what moves the project's actual level the most.
