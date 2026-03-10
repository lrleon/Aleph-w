# FFT Validation Notes

This document summarizes how the FFT module is validated before release.

## Validation layers

- Unit and integration coverage in `Tests/fft_test.cc`
- Benchmark self-validation through `Examples/fft_benchmark --validate`
- External numerical cross-check through `scripts/fft_reference_check.rb`
- Optional performance regression gate through `scripts/fft_perf_regression.rb`

## What is validated

- forward/inverse transforms for real and complex inputs
- power-of-two and arbitrary-size paths
- reusable plans and batch APIs
- sequential and `ThreadPool` parallel equivalence
- convolution helpers and streaming convolution paths
- STFT/ISTFT reconstruction behavior
- FIR/IIR design helpers and DSP utilities
- N-dimensional transforms
- SIMD-dispatched paths against scalar expectations where applicable

## Recommended release commands

Build the audit dependencies:

```bash
cmake --build build-test-clean --parallel --target fft_benchmark fft_reference_probe
```

Run the audit:

```bash
ruby scripts/fft_release_audit.rb --build-dir build-test-clean
```

Run the focused CTest subset:

```bash
ctest --test-dir build-test-clean --output-on-failure -R 'FFT'
```

## Numerical expectations

- exact equality is not required for floating-point outputs
- comparisons should use tolerance-based checks
- sequential and parallel paths should agree within the same tested tolerances
- real FFT compact forms must reconstruct the original signal size correctly

## Release interpretation

A release candidate is considered acceptable when:

- the FFT audit passes
- no new correctness failures appear in FFT-focused tests
- observed performance changes are understood and accepted
