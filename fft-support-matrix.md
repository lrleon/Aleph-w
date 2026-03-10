# FFT Support Matrix

## Feature matrix

| Feature | Status | Notes |
|---------|--------|-------|
| Complex FFT/IFFT | Supported | Core `fft.H` functionality |
| Real FFT/IFFT (`rfft`/`irfft`) | Supported | Compact real-spectrum helpers |
| Power-of-two sizes | Supported | Optimized radix kernels |
| Arbitrary sizes | Supported | Bluestein fallback |
| Reusable plans | Supported | Fixed-size precomputation |
| Batch transforms | Supported | Sequential and parallel forms |
| STFT/ISTFT | Supported | Includes processor-style helpers |
| FIR design helpers | Supported | Window, least-squares, and Remez families |
| IIR design helpers | Supported | Butterworth/Chebyshev/Bessel/Elliptic families |
| Spectral estimation | Supported | PSD, CSD, coherence helpers |
| Streaming convolution | Supported | Overlap-add, overlap-save, partitioned paths |
| N-dimensional transforms | Supported | 2-D and higher-dimensional helpers |
| Polynomial root utilities | Supported | Companion-matrix-based helpers |

## Parallel and backend matrix

| Capability | Status | Notes |
|-----------|--------|-------|
| `ThreadPool` parallel APIs | Supported | Parallel APIs are prefixed with `p` |
| x86-64 AVX2 runtime dispatch | Supported when available | Optimization path only |
| AArch64 NEON runtime dispatch | Supported when available | Optimization path only |
| Scalar fallback | Supported | Baseline portable path |

## Toolchain expectations

| Toolchain area | Expected support |
|----------------|------------------|
| C++ standard | C++20 build is the default release target |
| GCC | Supported in CI |
| Clang | Supported in CI |
| libc++ | Supported in CI for Clang jobs |
| libstdc++ | Supported in local Clang and GCC workflows where configured |

## Audit dependencies

The release audit additionally expects:

- `fft_benchmark`
- `fft_reference_probe`
- `scripts/fft_reference_check.rb`
- the release markdown files in the repository root
