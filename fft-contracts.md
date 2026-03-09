# FFT Contracts

Status date: 2026-03-06

This document records the operational contracts of `fft.H` as a production-facing
FFT/DSP header.

## Scope

`fft.H` provides:
- complex and real FFT / IFFT
- reusable `Plan` objects
- batch, axis-wise, 2-D, and 3-D transforms
- STFT / ISTFT, offline and stateful
- real and complex convolution
- FIR/IIR/SOS filtering and filter design
- DSP utility layers such as Welch PSD, coherence, rational resampling,
  overlap-save, and partitioned convolution

## Complexity

The table below describes the intended asymptotic cost of the main APIs.
Constants differ across power-of-two, mixed-radix, and Bluestein paths.

| API | Complexity | Notes |
|---|---:|---|
| `transform()/inverse_transform()` | `O(N log N)` | power-of-two uses radix-2/radix-4 kernels; arbitrary `N` uses mixed-radix or Bluestein |
| `rfft()/irfft()` | `O(N log N)` | compact one-sided layout of size `floor(N/2)+1` |
| `Plan::*transform*()` | `O(N log N)` | amortizes twiddle/dispatch setup across repeated calls |
| `transform_batch()` | `O(B N log N)` | batch size `B`, with reusable plans and SIMD-aware dispatch where available |
| `transform_axis()/transform_axes()` | `O(M N log N)` | `M` axis slices of logical length `N` |
| `stft()` / `istft()` | `O(F K log K)` | `F` frames, `K` configured `fft_size` |
| `multiply()` | `O(L log L)` | `L` padded convolution length |
| `OverlapAdd` / `OverlapSave` | `O(Q K log K)` | `Q` processing blocks of FFT size `K` |
| `PartitionedConvolver` | `O(Q P K)` in the current implementation | `Q` processed partitions, `P` kernel partitions, `K` FFT size per partition |
| `freqz()` sampled response | `O(M R)` | `M` samples, `R` coefficient/section cost |
| `welch()/csd()/coherence()` | `O(F K log K)` | one-sided estimators over `F` windowed frames |
| `upfirdn()` | `O(U N H)` | direct FIR path, `U=up`, `N` samples, `H` taps |
| `resample_poly()` | dominated by `upfirdn()` | default FIR design is an extra one-time setup cost |

## Numerical Contracts

- Precision is template-parametric over `Real` and validated for `float`,
  `double`, and `long double`.
- The current reference envelope is documented in
  [fft-validation.md](/home/lrleon/Insync/leandro.r.leon@gmail.com/Google%20Drive/Aleph-w/fft-validation.md).
- FFT validation thresholds are intentionally looser than observed maxima so
  that platform differences do not trigger false regressions.
- Real inverse transforms require Hermitian-compatible spectra. Invalid spectra
  are rejected.
- `istft()` reconstructs exactly only when the configured analysis/synthesis
  pair satisfies the stated overlap constraints. `validate_nola` and
  `validate_cola` turn these checks into hard runtime preconditions.
- Filter-stability helpers assume discrete-time BIBO stability, i.e. poles
  strictly inside the unit circle.

## API Contracts

- Empty FFT inputs are rejected unless the specific API explicitly documents
  empty passthrough behavior.
- Stateful processors (`STFTProcessor`, `ISTFTProcessor`, `LFilter`,
  `SOSFilter`, `OverlapAdd`, `OverlapSave`, `PartitionedConvolver`, banks)
  preserve internal state across `process_block()` calls until `reset()` or
  `flush()`.
- `flush()` finalizes any pending overlap/history and should be called once at
  the logical end of a stream.
- Batch APIs require channel/frame counts and per-item sizes to match the
  configured processor or plan.
- `resample_poly(signal, up, down, coeffs)` assumes that `coeffs` were designed
  for the upsampled domain. The convenience overload designs a Kaiser FIR
  internally and applies the conventional gain compensation by `up/gcd(up,down)`.
- `PartitionedConvolver` is designed for low-latency streaming, not minimum
  total FLOPs. Its current partition accumulation loops over all stored kernel
  partitions for each emitted partition.

## Thread Safety

- Functional stateless APIs are thread-safe when called concurrently on
  independent inputs.
- `Plan` is safe for concurrent const-style use where each call works on a
  separate input/output object.
- Mutable stateful processors are not safe for concurrent mutation from
  multiple threads.
- `p...` APIs do not make the underlying input container share-safe; callers
  must still avoid racing on the same mutable object.
- SIMD backend selection is process-global only through environment variables
  read at runtime; the selected policy is then treated as immutable process
  configuration.

## Failure Model

- Contract violations raise Aleph's standard translated exceptions via
  `ah_invalid_argument_if`, `ah_domain_error_if`, `ah_runtime_error_if`, or
  related macros.
- Numerical validation helpers fail closed: they reject malformed shapes,
  invalid overlap conditions, unstable filters, and inconsistent batch layouts
  rather than trying to continue silently.
