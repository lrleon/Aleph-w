# FFT Support Matrix

Status date: 2026-03-06

This matrix captures what `fft.H` currently supports operationally.

## Compilers and Language

| Item | Status | Notes |
|---|---|---|
| C++20 | Supported | project is configured for C++20 |
| GCC | Supported | current validation host uses GCC |
| Clang | Supported | validated in CI (ubuntu-22.04/24.04, Debug/Release with libc++) |

## Precision

| Precision | Status | Validation |
|---|---|---|
| `float` | Supported | benchmark envelope and round-trip tests |
| `double` | Supported | benchmark envelope and reference cross-checks |
| `long double` | Supported | benchmark envelope |

## Hardware Backends

| Backend | Status | Selection |
|---|---|---|
| scalar | Supported | always available |
| AVX2 | Supported | runtime-detected, policy `ALEPH_FFT_SIMD=avx2|auto` |
| NEON | Supported in code path | runtime-detected on ARM hosts, native ARM validation still host-dependent |
| AVX-512 | Not implemented | out of scope for current phases |

## FFT Surface

| Capability | Status |
|---|---|
| 1-D complex FFT/IFFT | Supported |
| real FFT/IFFT (`rfft/irfft`) | Supported |
| arbitrary sizes | Supported |
| batch FFT | Supported |
| axis-wise flat-buffer FFT | Supported |
| 2-D / 3-D complex FFT | Supported |
| multichannel STFT/ISTFT | Supported |

## Filtering and DSP

| Capability | Status |
|---|---|
| FIR convolution via FFT | Supported |
| overlap-add | Supported |
| overlap-save | Supported |
| partitioned convolution | Supported |
| causal FIR/IIR/SOS filtering | Supported |
| zero-phase FIR/IIR filtering | Supported |
| PSD / CSD / coherence | Supported |
| rational resampling | Supported |

## Filter Design

| Family | Modes |
|---|---|
| FIR `firwin` | low/high/band-pass/band-stop |
| FIR `firls` | weighted least-squares |
| FIR `remez` | dense-grid equiripple |
| Butterworth | low/high/band-pass/band-stop |
| Chebyshev I | low/high/band-pass/band-stop |
| Chebyshev II | low/high/band-pass/band-stop |
| Bessel | low/high/band-pass/band-stop |
| Elliptic / Cauer | low/high/band-pass/band-stop |

## External Tooling

| Tool | Role | Required |
|---|---|---|
| Ruby | utility scripts and release audit | yes for scripted audit |
| Python 3 + NumPy | external FFT differential cross-check | optional but recommended |
| CTest | test registry execution | yes for integrated validation |

## Known Limits

| Area | Current limit |
|---|---|
| SIMD | `auto` is intentionally conservative outside throughput-oriented paths |
| Partitioned convolution | current implementation is uniform-partition and scalar accumulation over partitions |
| Reference cross-check | depends on NumPy availability |
| Performance gate | hardware-specific baseline must be supplied explicitly |
