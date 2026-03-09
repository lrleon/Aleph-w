# FFT Validation Baseline

Status date: 2026-03-06

This document records the Phase 0 baseline for `fft.H` after enabling arbitrary-length FFT execution and compact real-spectrum APIs.

## Build and test commands

Debug baseline:

```bash
cmake -S . -B build-test-clean
cmake --build build-test-clean -j4 --target fft_test fft_example fft_benchmark fft_reference_probe
./build-test-clean/Tests/fft_test
./build-test-clean/Examples/fft_example
ctest --test-dir build-test-clean \\
  -R '^(FFT|FFTPlan|FFTLargeN|FFTSTFT|FFTOverlapAdd)\\.' \\
  --output-on-failure
```

Sanitizer build:

```bash
cmake -S . -B build-test-asan \
  -DALEPH_USE_SANITIZERS=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=ON
cmake --build build-test-asan -j4 --target fft_test fft_benchmark fft_reference_probe
ctest --test-dir build-test-asan \\
  -R '^(FFT|FFTPlan|FFTLargeN|FFTSTFT|FFTOverlapAdd)\\.' \\
  --output-on-failure
```

Benchmark baseline:

```bash
./build-test-clean/Examples/fft_benchmark
./build-test-clean/Examples/fft_benchmark --json --warmup 3 --samples 9 --batch-count 16 > /tmp/fft-baseline.json
./scripts/fft_perf_regression.rb \
  --baseline /tmp/fft-baseline.json \
  --benchmark ./build-test-clean/Examples/fft_benchmark \
  --metric median \
  --warmup 3 \
  --samples 9 \
  --batch-count 16 \
  --warn-slowdown 0.15 \
  --fail-slowdown 0.30
./build-test-clean/Examples/fft_benchmark --validate
./scripts/fft_reference_check.rb

# Optional SIMD policy experiments for FFT<double> on supported x86 CPUs.
ALEPH_FFT_SIMD=avx2 ./build-test-clean/Examples/fft_benchmark \
  --json --sizes 128,256,512,1024 --repetitions 16 --warmup 2 --samples 5 \
  --batch-count 8 > /tmp/fft-avx2.json
ALEPH_FFT_SIMD=scalar ./build-test-clean/Examples/fft_benchmark \
  --json --sizes 128,256,512,1024 --repetitions 16 --warmup 2 --samples 5 \
  --batch-count 8 > /tmp/fft-scalar.json
```

Notes:
- `fft_benchmark --json` now emits a stable schema with per-case timing statistics (`mean`, `median`, `min`, `max`, `stddev`) and explicit metadata (`seed`, `threads`, `simd_backend`, `batched_plan_simd_backend`, `detected_simd_backend`, `simd_preference`, `avx2_kernel_compiled`, `neon_kernel_compiled`, `avx2_runtime_available`, `neon_runtime_available`, `warmup_samples`, `samples`, `batch_count`).
- `fft_perf_regression.rb` compares two JSON payloads or runs the benchmark executable directly and checks slowdowns against configurable thresholds.
- The benchmark now also measures `batched_complex_fft`, `batched_real_rfft`, `overlap_add_bank`, and `lfilter_bank`, which exercise the reusable-plan and multichannel throughput paths introduced in Phase 2.
- `ALEPH_FFT_SIMD=auto|scalar|avx2|neon` controls the runtime SIMD policy. `auto` keeps standalone transforms conservative while letting throughput-oriented batch-plan paths use the available SIMD backend when the current CPU supports it. On x86 this currently means the AVX2 `double` kernel; on ARM it means the guarded NEON path. Legacy `ALEPH_FFT_ENABLE_AVX2` / `ALEPH_FFT_DISABLE_AVX2` are still accepted.
- `pstft()` now applies an extra local heuristic on top of `auto`: parallel STFT keeps SIMD conservative for small FFT sizes and only enables the batch-plan SIMD path from `fft_size >= 256`, which avoids the measured small-size regression while preserving the larger-size gain.
- `pstft()` and `OverlapAddBank` now also gate `ThreadPool` use on total batch work instead of just `pool != nullptr`; this prevents wasting parallel overhead on small multiframe or multichannel jobs.

For native ARM validation, the recommended audit loop is:

```bash
ALEPH_FFT_SIMD=neon ./build-test-clean/Tests/fft_test
ALEPH_FFT_SIMD=neon ./build-test-clean/Examples/fft_benchmark --validate
ALEPH_FFT_SIMD=neon ./build-test-clean/Examples/fft_benchmark --json \
  --sizes 128,512,1024 --repetitions 16 --warmup 3 --samples 9 --batch-count 8
```

The JSON metadata should then report `neon_kernel_compiled=true` and
`neon_runtime_available=true` on the target host.

## Phase 2 throughput profile

On the 2026-03-06 validation run used to close Phase 2, the conservative
`ALEPH_FFT_SIMD=auto` policy was compared against `ALEPH_FFT_SIMD=scalar`
with:

```bash
./build-test-clean/Examples/fft_benchmark --json \
  --sizes 128,512,1024 --repetitions 16 --warmup 3 --samples 9 --batch-count 8
```

The throughput-oriented paths that Phase 2 targets showed the following median
improvements:

| Case | 128 | 512 | 1024 |
|---|---:|---:|---:|
| `batched_complex_fft` sequential | `-52.58%` | `-56.02%` | `+11.55%` |
| `batched_complex_fft` parallel | `-11.13%` | `-41.64%` | `+4.01%` |
| `batched_real_rfft` sequential | `-11.58%` | `-36.14%` | `-7.06%` |
| `batched_real_rfft` parallel | `-17.90%` | `-15.04%` | `+17.98%` |
| `stft` sequential | `-17.64%` | `-27.71%` | `-4.04%` |
| `stft` parallel | `+12.57%` | `-8.20%` | `+8.84%` |
| `overlap_add_bank` sequential | `+0.11%` | `+1.84%` | `-1.36%` |
| `overlap_add_bank` parallel | `+0.13%` | `-0.94%` | `-1.16%` |

Interpretation:
- Phase 2 is considered complete because the reusable-plan batch paths show
  clear and repeatable gains where SIMD is actually applied.
- `auto` remains intentionally conservative for cases that do not show a clear
  win on this host, which is why some 1024-point paths stay near scalar
  performance instead of forcing a wider SIMD dispatch.
- The remaining work from this point on is no longer Phase 2 tuning, but the
  numerical and structural work of Phase 4 and beyond.

## Phase 4 numerical validation

The 2026-03-06 Phase 4 closing run used:

```bash
cmake --build build-test-clean -j4 --target fft_test fft_example fft_benchmark fft_reference_probe
./build-test-clean/Tests/fft_test
./build-test-clean/Examples/fft_example
./build-test-clean/Examples/fft_benchmark --validate
./scripts/fft_reference_check.rb
ctest --test-dir build-test-clean \
  -R '^(FFT|FFTPlan|FFTLargeN|FFTSTFT|FFTOverlapAdd)\\.' \
  --output-on-failure
```

Observed results:
- `./build-test-clean/Tests/fft_test`: `123` tests passed.
- Filtered `ctest`: `81` FFT-focused tests passed.
- `fft_benchmark --validate` preserved the existing FFT envelope:
  - `float`: forward `4.876112e-05`, round-trip `1.570500e-06`, `real_ifft` `5.066395e-07`, compact `irfft` `2.384186e-06`
  - `double`: forward `5.596229e-14`, round-trip `1.737774e-15`, `real_ifft` `1.110223e-15`, compact `irfft` `1.887379e-15`
  - `long double`: forward `2.584157e-17`, round-trip `7.457585e-19`, `real_ifft` `5.963112e-19`, compact `irfft` `8.402567e-19`
- `scripts/fft_reference_check.rb` remained clean against NumPy, with worst observed case `prime_fft = 7.710801e-14`.

Phase 4-specific coverage now includes:
- conditioned and randomly generated stable root sets for the IIR pole solver
- SOS analytic group/phase delay agreement with section-wise sums away from singular bins
- refined SOS gain/phase margins matched against high-resolution sampled references
- SOS stability margin validation based on the worst section instead of a coarse aggregate

## Phase 3 API validation

The 2026-03-06 Phase 3 closing run used:

```bash
cmake --build build-test-clean -j4 --target fft_test fft_example fft_benchmark fft_reference_probe
./build-test-clean/Tests/fft_test
./build-test-clean/Examples/fft_example
./build-test-clean/Examples/fft_benchmark --validate
./scripts/fft_reference_check.rb
ctest --test-dir build-test-clean \
  -R '^(FFT|FFTPlan|FFTLargeN|FFTSTFT|FFTOverlapAdd|FFTND)\\.' \
  --output-on-failure
```

Observed results:
- `./build-test-clean/Tests/fft_test`: `127` tests passed.
- Filtered `ctest`: `85` FFT-focused tests passed, including the new `FFTND.*` coverage.
- `fft_benchmark --validate` preserved the same FFT envelope used to close Phase 4.
- `scripts/fft_reference_check.rb` remained clean against NumPy, with worst observed case `prime_fft = 7.710801e-14`.

Phase 3-specific coverage now includes:
- axis-wise transforms on flat complex buffers with explicit row-major and padded custom strides
- 2-D transforms matched against a manual row/column pipeline
- 3-D round-trip validation and batched 2-D transforms over tensor leading dimensions
- multichannel STFT/ISTFT layout conversion between channel-major and frame-major forms without manual transposes

## Phase 5 filter-design validation

The 2026-03-06 Phase 5 closing run used:

```bash
cmake --build build-test-clean -j4 --target fft_test fft_example
./build-test-clean/Tests/fft_test
./build-test-clean/Examples/fft_example
ctest --test-dir build-test-clean \
  -R '^(FFT|FFTPlan|FFTLargeN|FFTSTFT|FFTOverlapAdd|FFTND|FFTIIR)\\.' \
  --output-on-failure
```

Observed results:
- `./build-test-clean/Tests/fft_test`: `132` tests passed.
- Filtered `ctest`: `132` FFT/IIR-focused tests passed.
- `fft_example` executed successfully with the new FIR/firls/remez/elliptic demo output.

Phase 5-specific validated additions now include:
- `kaiser_beta()` and `kaiser_window()`
- `firwin_lowpass/highpass/bandpass/bandstop()`
- `firls()` weighted least-squares FIR design over piecewise-linear bands
- `remez()` dense-grid equiripple FIR design over weighted piecewise-linear bands
- Butterworth, Chebyshev-I, Chebyshev-II, Bessel, and elliptic/Cauer low/high/band-pass/band-stop SOS design

The current validation surface covers:
- representative passband/stopband response checks for the new FIR and IIR families
- symmetry and weighting behavior for `firls()` and `remez()`
- argument validation for invalid tap counts, malformed bands, malformed weights, ripple/attenuation, and cutoff specifications

## Phase 6 DSP-utility validation

The 2026-03-06 Phase 6 closing run used:

```bash
cmake --build build-test-clean -j4 --target fft_test fft_example fft_benchmark fft_reference_probe
./build-test-clean/Tests/fft_test
./build-test-clean/Examples/fft_example
./build-test-clean/Examples/fft_benchmark --validate
./scripts/fft_reference_check.rb
ctest --test-dir build-test-clean \
  -R '^(FFT|FFTPlan|FFTLargeN|FFTSTFT|FFTOverlapAdd|FFTND|FFTIIR|FFTDSPUtilities|FFTOverlapSave|FFTPartitionedConvolver).*' \
  --output-on-failure
```

Observed results:
- `./build-test-clean/Tests/fft_test`: `137` tests passed.
- Filtered `ctest`: `137` FFT/DSP-focused tests passed.
- `fft_example` executed successfully with the new PSD/coherence, resampling, overlap-save, and partitioned-convolution demo output.
- `fft_benchmark --validate` preserved the same FFT numerical envelope used to close earlier phases.
- `scripts/fft_reference_check.rb` remained clean against NumPy, with worst observed case `prime_fft = 7.710801e-14`.

Phase 6-specific validated additions now include:
- window/framing helpers: `window_energy()`, `window_coherent_gain()`, `window_enbw()`, `frame_offsets()`, `overlap_add_frames()`
- one-sided spectral estimators: `welch()`, `csd()`, `coherence()`
- rational-rate conversion helpers: `upfirdn()`, `resample_poly()`
- reusable streaming FIR engines: `OverlapSave` / `overlap_save_convolution()`
- low-latency frequency-domain streaming via `PartitionedConvolver` / `partitioned_convolution()`

The current validation surface covers:
- tone-localization and coherence checks for Welch/CSD/coherence
- direct-reference cross-checks for `upfirdn()` and centered identity behavior for `resample_poly()`
- exact convolution agreement for overlap-save and partitioned streaming, including chunked `process_block()` + `flush()`
- framing and window metric checks that support spectral-analysis workflows

## Phase 7 release validation

The 2026-03-06 Phase 7 closing run used:

```bash
cmake -S . -B build-test-clean
cmake --build build-test-clean -j4 --target fft_test fft_example fft_benchmark fft_reference_probe
./build-test-clean/Tests/fft_test
ctest --test-dir build-test-clean -R '^FFTRelease\\.' --output-on-failure
ctest --test-dir build-test-clean \
  -R '^(FFT|FFTPlan|FFTLargeN|FFTSTFT|FFTOverlapAdd|FFTND|FFTIIR|FFTDSPUtilities|FFTOverlapSave|FFTPartitionedConvolver|FFTRelease).*' \
  --output-on-failure
./build-test-clean/Examples/fft_example
./scripts/fft_release_audit.rb --build-dir build-test-clean
ctest -N --test-dir build-test-clean
```

Observed results:
- `./build-test-clean/Tests/fft_test`: `137` tests passed.
- `ctest -R '^FFTRelease\\.'`: `3/3` release-audit tests passed:
  - `FFTRelease.BenchmarkValidate`
  - `FFTRelease.ReferenceCrossCheck`
  - `FFTRelease.Audit`
- Filtered FFT/DSP/release `ctest`: `175/175` tests passed.
- `./build-test-clean/Examples/fft_example` executed successfully, including the production DSP utility demo.
- `ctest -N --test-dir build-test-clean`: `11306` discovered tests registered in the tree.

Phase 7-specific validated additions now include:
- release-facing contracts in
  [fft-contracts.md](/home/lrleon/Insync/leandro.r.leon@gmail.com/Google%20Drive/Aleph-w/fft-contracts.md)
- supported-platform and feature scope documentation in
  [fft-support-matrix.md](/home/lrleon/Insync/leandro.r.leon@gmail.com/Google%20Drive/Aleph-w/fft-support-matrix.md)
- technical release history in
  [fft-changelog.md](/home/lrleon/Insync/leandro.r.leon@gmail.com/Google%20Drive/Aleph-w/fft-changelog.md)
- scripted release auditing through `scripts/fft_release_audit.rb`
- dedicated `FFTRelease.*` CTest coverage for benchmark validation, NumPy differential cross-check, and the full release audit

Operational note:
- this test tree uses `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)`, so concurrent `ctest` invocations against the same build directory are not a supported workflow. Run one `ctest` process per build tree to avoid discovery-file races.

## CTest baseline

`ctest -N` in `build-test-clean` reports a populated registry. On the latest validation run used for this baseline, the tree exposed `11306` discovered tests.

## Validation envelope

Observed maxima from:

```bash
./build-test-clean/Examples/fft_benchmark --validate
```

| Precision | Forward FFT | Complex round-trip | Real `inverse_transform_real` | Compact `irfft` |
|---|---:|---:|---:|---:|
| `float` | `4.876112e-05` | `1.570500e-06` | `5.066395e-07` | `2.384186e-06` |
| `double` | `5.596229e-14` | `1.737774e-15` | `1.110223e-15` | `1.887379e-15` |
| `long double` | `2.584157e-17` | `7.457585e-19` | `5.963112e-19` | `8.402567e-19` |

Recommended gating thresholds for routine regression checks:

| Precision | Forward FFT | Complex round-trip | Real/compact inverse |
|---|---:|---:|---:|
| `float` | `1e-4` | `1e-5` | `1e-5` |
| `double` | `1e-12` | `1e-12` | `1e-12` |
| `long double` | `1e-15` | `1e-15` | `1e-15` |

These thresholds are intentionally looser than the observed maxima to tolerate platform variation while still catching real regressions.

## External cross-validation

`./scripts/fft_reference_check.rb` compares deterministic Aleph outputs against NumPy when `python3` and `numpy` are available.

Reference cases covered:
- complex FFT of length `10`
- full real FFT of length `7`
- compact `rfft` of length `7`
- compact `irfft` round-trip of length `7`
- prime-length `Plan(17)` FFT

Observed maxima on the validation run used for this baseline:

| Case | Max abs error vs NumPy |
|---|---:|
| `complex_fft` | `2.808667e-15` |
| `real_full_fft` | `4.373771e-15` |
| `real_compact_fft` | `2.512148e-15` |
| `real_irfft` | `1.332268e-15` |
| `prime_fft` | `7.710801e-14` |

## Benchmark notes

The benchmark executable reports sequential and explicit-`ThreadPool` parallel timings for:
- complex FFT
- batched complex FFT with plan reuse
- batched compact real FFT with plan reuse
- multichannel overlap-add convolution with shared kernel FFT reuse
- multichannel stateful `LFilterBank`
- full real FFT
- compact `rfft`
- FFT convolution
- STFT
- zero-phase FIR `filtfilt`

Interpretation notes:
- small transforms are often slower in the parallel path because scheduling cost dominates
- non-power-of-two sizes such as `300` and `1536` exercise the new arbitrary-length backend directly
- STFT now reuses the same batch transform path internally after windowing/zero-padding
- standalone transforms remain conservative by default, but batch-plan paths can now auto-select AVX2 on supported x86 hardware
- `OverlapAddBank` shares one FIR spectrum across channels; this is the intended industrial path for multichannel long-FIR streaming
