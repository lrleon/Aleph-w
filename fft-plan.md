# FFT Release Plan

This document defines the release scope for `fft.H` and the supporting
artifacts used by the FFT release audit.

## Scope

The FFT module covers:

- complex and real FFT/IFFT paths
- power-of-two kernels plus Bluestein fallback for arbitrary sizes
- reusable plans
- STFT/ISTFT
- FIR and IIR filter utilities
- convolution helpers, including overlap-add/overlap-save variants
- N-dimensional transforms
- explicit `ThreadPool` parallel variants
- optional runtime SIMD dispatch on supported targets

See also:

- `README.md` "Signal Processing (FFT)"
- `docs/fft-tutorial.en.md`
- `docs/fft-tutorial.md`

## Release goals

- keep the public API in `fft.H` coherent and documented
- preserve behavioral equivalence between sequential and parallel entry points
- preserve numerical stability within tested tolerances
- keep arbitrary-size transforms available through Bluestein
- keep release tooling able to validate correctness and regressions

## Non-goals

- bitwise-identical results across all compilers and CPU backends
- ABI stability across unrelated major refactors
- performance guarantees on machines without the expected SIMD support

## Release checklist

- unit and integration tests pass
- `fft_benchmark --validate` passes
- `scripts/fft_reference_check.rb` passes or skips cleanly when NumPy is unavailable
- performance regression checks are reviewed when a baseline is provided
- the release documents listed in `scripts/fft_release_audit.rb` are present

## Main risks

- floating-point sensitivity across optimization levels and standard libraries
- backend-specific code paths drifting from the scalar reference path
- algorithmic regressions in Bluestein, batch, STFT, or filter design helpers
- performance regressions hidden by correctness-only test coverage
