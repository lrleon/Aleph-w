# FFT Changelog
Release: v2.0.0b — 2026-03-09

This file records notable FFT-facing release changes for the current
stabilization cycle (e.g., following the "Release: vX.Y.Z — YYYY-MM-DD" format).

## Current release highlights

- consolidated FFT and DSP capabilities in `fft.H`
- maintained support for both power-of-two and arbitrary-size transforms
- preserved reusable plan APIs for repeated transforms
- kept parallel `ThreadPool` variants aligned with scalar behavior
- kept streaming convolution helpers available for low-latency workflows
- maintained STFT/ISTFT, FIR/IIR, spectral-estimation, and N-dimensional utilities
- kept release-audit tooling for benchmark validation, external cross-checking,
  and optional performance regression review

## Release-facing expectations

- correctness changes must be reflected in FFT-focused tests
- contract changes must be reflected in `fft-contracts.md`
- support changes must be reflected in `fft-support-matrix.md`
- validation process changes must be reflected in `fft-validation.md`

## Known documentation dependencies

The FFT release audit expects this changelog plus:

- `fft-plan.md`
- `fft-validation.md`
- `fft-contracts.md`
- `fft-support-matrix.md`
