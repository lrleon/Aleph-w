# FFT API Contracts

This document records the intended behavioral contracts of the FFT module.

## General contracts

- APIs in `fft.H` operate on finite-size containers and reject invalid shapes
- transform helpers preserve logical signal size unless the API explicitly pads
- inverse helpers reconstruct the signal implied by the corresponding forward form
- parallel entry points are semantically equivalent to their sequential variants
- plan-based APIs must agree with the corresponding static helpers for the same size

## Input contracts

- empty-input handling follows the behavior exercised by `Tests/fft_test.cc`
- real compact-spectrum helpers require enough metadata to reconstruct the target size
- STFT and ISTFT helpers require compatible frame, hop, and window parameters
- filter-design helpers require valid band and sampling assumptions

## Output contracts

- complex transforms return frequency-domain coefficients in the same logical order
- compact real transforms expose the non-redundant half-spectrum
- convolution helpers return the full linear convolution unless documented otherwise
- streaming processors preserve the same result model as the equivalent offline path

## Error-handling contracts

- invalid arguments must fail deterministically
- unsupported parameter combinations must not silently produce truncated results
- environment-gated performance tests may skip cleanly instead of failing

## Parallel and backend contracts

- `ThreadPool` APIs must preserve correctness relative to the scalar path
- SIMD-dispatched kernels are an optimization only and must not change semantics
- backend selection may change performance but not the mathematical contract

## Stability contracts

- public FFT-facing APIs should remain source-compatible across routine maintenance
- behavior changes that affect callers must be noted in `fft-changelog.md`
