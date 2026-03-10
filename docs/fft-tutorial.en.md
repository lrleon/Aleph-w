# Tutorial: FFT and Digital Signal Processing in Aleph-w

Language: English | [Español](fft-tutorial.md)

## 1. Conceptual Foundations

### 1.1 What is the Discrete Fourier Transform (DFT)?

The DFT decomposes a discrete signal of N samples into N frequency
components. Given a vector x[0..N-1], the DFT produces X[0..N-1] where:

    X[k] = sum_{t=0}^{N-1} x[t] * exp(-2*pi*i*k*t / N)

Each X[k] is a complex number whose **magnitude** indicates the amplitude of
frequency k and whose **phase** indicates the temporal offset.

The direct DFT has O(N^2) complexity. The **Fast Fourier Transform (FFT)**
computes the same operation in O(N log N) using the Cooley-Tukey recursive
factorization.

### 1.2 Key Properties

- **Linearity**: FFT(a*x + b*y) = a*FFT(x) + b*FFT(y)
- **Hermitian symmetry**: If x is real, X[k] = conj(X[N-k])
- **Convolution theorem**: x * y in time = X . Y in frequency
- **Parseval**: Energy is conserved between domains

### 1.3 Where is the FFT Used?

| Domain | Application |
|--------|------------|
| Audio | Spectral analysis, equalizers, compression |
| Image | Filtering, JPEG compression |
| Telecommunications | OFDM, channel estimation |
| Science | Seismic signal processing, astronomy |
| Mathematics | Fast polynomial multiplication, large number multiplication |

---

## 2. Architecture of `fft.H`

### 2.1 Main Class

```cpp
# include <fft.H>

// Template class parameterized by precision
template <std::floating_point Real = double>
class FFT { ... };

// Convenient aliases
using FFTD = FFT<double>;   // Double precision (most common)
using FFTF = FFT<float>;    // Single precision
```

All methods are `static` -- no need to instantiate the class.

### 2.2 Important Internal Types

```cpp
using Complex = std::complex<Real>;

// Biquad section for IIR filters
struct BiquadSection { Real b0, b1, b2, a0, a1, a2; };

// Frequency response
struct FrequencyResponse {
  Array<Real>    omega;      // Angular frequencies
  Array<Complex> response;   // Complex H(omega)
};

// STFT analysis result
// Array<Array<Complex>>  -- each row is a spectral frame
```

### 2.3 Implementation Strategy

Aleph-w's FFT uses three strategies depending on the size N:

1. **Power of 2** (N = 2^k): Optimized Cooley-Tukey radix-2/4, with
   optional AVX2/NEON dispatch
2. **Arbitrary size**: Bluestein algorithm (chirp-z), which reduces
   any N to a convolution of power-of-2 size
3. **Reusable plan**: Precomputes bit-reversal and twiddle factors for
   repeated transforms of the same size

---

## 3. Basic Usage

### 3.1 Forward and Inverse Transform

```cpp
# include <fft.H>
# include <tpl_array.H>

using namespace Aleph;
using FFTD = FFT<double>;
using Complex = FFTD::Complex;

// FFT of a real signal
Array<double> signal = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
auto spectrum = FFTD::spectrum(signal);  // Array<Complex>

// Inverse: return to the time domain
auto recovered = FFTD::inverse_transform_real(spectrum);
// recovered ≈ signal (with floating-point error)

// Compact FFT for real signals (only N/2+1 bins, no redundancy)
auto compact = FFTD::rfft(signal);              // Array<Complex>, size N/2+1
auto restored = FFTD::irfft(compact, signal.size());  // reconstructs N samples
```

### 3.2 Spectral Analysis

```cpp
auto magnitudes = FFTD::magnitude_spectrum(spectrum);  // |X[k]|
auto powers     = FFTD::power_spectrum(spectrum);      // |X[k]|^2
auto phases     = FFTD::phase_spectrum(spectrum);      // arg(X[k])

// Apply window before FFT (reduces spectral leakage)
auto window = FFTD::hann_window(signal.size());
auto windowed = FFTD::windowed_spectrum(signal, window);
```

### 3.3 Polynomial Multiplication / Convolution

```cpp
// Real polynomials: p(x) = 1 + 2x + 3x^2,  q(x) = 4 + 5x
Array<double> p = {1.0, 2.0, 3.0};
Array<double> q = {4.0, 5.0};
auto product = FFTD::multiply(p, q);
// product = {4.0, 13.0, 22.0, 15.0}  -->  4 + 13x + 22x^2 + 15x^3

// Complex polynomials
Array<Complex> a = {Complex(1,0), Complex(2,-1)};
Array<Complex> b = {Complex(3,0), Complex(0,1)};
auto c = FFTD::multiply(a, b);
```

### 3.4 Generic Containers

The API accepts any iterable container, not just `Array`:

```cpp
std::vector<double> vec_signal = {1.0, 2.0, 3.0, 4.0};
auto vec_spectrum = FFTD::transform(vec_signal);  // returns Array<Complex>
```

---

## 4. Window Functions

Windows reduce the *spectral leakage* phenomenon when analyzing
signals that are not periodic within the observation window.

```cpp
auto hann     = FFTD::hann_window(256);
auto hamming  = FFTD::hamming_window(256);
auto blackman = FFTD::blackman_window(256);
auto kaiser   = FFTD::kaiser_window(256, 8.0);  // beta = 8

// Compute Kaiser beta from desired attenuation
double beta = FFTD::kaiser_beta(60.0);  // 60 dB attenuation

// Window properties
double cg   = FFTD::window_coherent_gain(hann);
double enbw = FFTD::window_enbw(hann);
```

**Selection guide**:
- **Hann**: General use, good resolution/leakage balance
- **Hamming**: Lower sidelobes than Hann
- **Blackman**: Excellent sidelobe suppression
- **Kaiser**: Parameterizable via beta -- versatile

---

## 5. STFT: Time-Frequency Analysis

The Short-Time Fourier Transform analyzes the signal through sliding
windows, producing a **spectrogram** (time x frequency matrix).

### 5.1 Analysis and Synthesis

```cpp
Array<double> signal = { /* audio samples */ };
auto window = FFTD::hann_window(512);

// Analysis
FFTD::STFTOptions opts;
opts.hop_size = 128;       // Advance between frames
opts.fft_size = 1024;      // Zero-padding for better resolution
opts.centered = true;
opts.pad_end = true;

auto spectrogram = FFTD::stft(signal, window, opts);
// spectrogram: Array<Array<Complex>>
// spectrogram[frame][bin]

// Synthesis (perfect reconstruction under NOLA)
FFTD::ISTFTOptions iopts;
iopts.hop_size = 128;
iopts.centered = true;
iopts.signal_length = signal.size();

auto reconstructed = FFTD::istft(spectrogram, window, iopts);
// reconstructed ≈ signal
```

### 5.2 Frame-by-Frame Processing (Streaming)

```cpp
FFTD::STFTProcessor analyzer(window, opts);
FFTD::ISTFTProcessor synthesizer(fft_size, window, iopts);

// Feed sample blocks
auto frames = analyzer.process_block(chunk_of_samples);
// Each frame is an Array<Complex>

// Here you can modify frames (e.g.: noise gate, pitch shift)

auto output_samples = synthesizer.process_block(frames);

// At the end
auto tail_frames = analyzer.flush();
auto tail_samples = synthesizer.flush();
```

---

## 6. FIR Filter Design

### 6.1 Window Method (firwin)

```cpp
double sample_rate = 44100.0;

// Low-pass: cutoff at 1 kHz, 51 taps, 60 dB attenuation
auto lp = FFTD::firwin_lowpass(51, 1000.0, sample_rate, 60.0);

// High-pass
auto hp = FFTD::firwin_highpass(51, 1000.0, sample_rate, 60.0);

// Band-pass
auto bp = FFTD::firwin_bandpass(51, 500.0, 2000.0, sample_rate, 60.0);

// Band-stop (notch)
auto bs = FFTD::firwin_bandstop(51, 500.0, 2000.0, sample_rate, 60.0);
```

### 6.2 Advanced Design: firls and Remez

```cpp
// firls: weighted least squares
Array<double> bands   = {0.0, 900.0, 1200.0, 4000.0};
Array<double> desired = {1.0, 1.0,   0.0,    0.0};
Array<double> weights = {1.0, 12.0};  // 12x more weight on stopband
auto firls_taps = FFTD::firls(33, bands, desired, 8000.0, weights);

// Remez (equiripple): minimizes maximum error
auto remez_taps = FFTD::remez(33, bands, desired, 8000.0, weights);
```

### 6.3 Applying the FIR Filter

```cpp
// Direct convolution via FFT
auto filtered = FFTD::overlap_add_convolution(signal, lp);

// Zero-phase filtering (no phase shift, non-causal)
auto zero_phase = FFTD::filtfilt(signal, lp, 4);
```

---

## 7. IIR Filter Design

IIR filters are designed as cascades of biquad sections
(second-order sections, SOS) for numerical stability.

### 7.1 Filter Families

```cpp
double sr = 8000.0;  // sample rate

// Butterworth: maximally flat response
auto bw = FFTD::butterworth_lowpass(4, 1000.0, sr);

// Chebyshev I: passband ripple (0.5 dB)
auto ch1 = FFTD::chebyshev1_lowpass(4, 0.5, 1000.0, sr);

// Chebyshev II: stopband ripple (40 dB)
auto ch2 = FFTD::chebyshev2_lowpass(4, 40.0, 1000.0, sr);

// Bessel: maximally flat group delay
auto bs = FFTD::bessel_lowpass(4, 1000.0, sr);

// Elliptic (Cauer): equiripple in both bands
auto el = FFTD::elliptic_lowpass(4, 1.0, 40.0, 1000.0, sr);
```

Each family has `_lowpass`, `_highpass`, `_bandpass`, `_bandstop` variants.

### 7.2 Response Analysis

```cpp
// Frequency response (512 points, half band)
auto resp = FFTD::freqz(bw, 512, false);
// resp.omega[k]    -- angular frequency
// resp.response[k] -- complex H(omega)

// Poles and zeros
auto poles = FFTD::poles(bw);
auto zeros = FFTD::zeros(bw);

// Stability
bool stable = FFTD::is_stable(bw);
double margin = FFTD::stability_margin(bw);

// Group delay and phase delay
auto gd = FFTD::group_delay(bw, 512);
auto pd = FFTD::phase_delay(bw, 512);
```

### 7.3 Applying the IIR Filter

```cpp
// Causal (single pass)
auto y = FFTD::sosfilt(signal, bw);

// Zero phase (forward-backward)
auto y_zp = FFTD::filtfilt(signal, bw);

// Streaming causal with state
FFTD::LFilter lf(bw);
auto chunk1_out = lf.filter(chunk1);
auto chunk2_out = lf.filter(chunk2);  // maintains state
```

### 7.4 Filter Selection Guide

| Family | Advantage | Disadvantage |
|--------|-----------|--------------|
| Butterworth | No ripple, smooth response | Slow transition |
| Chebyshev I | Faster transition | Passband ripple |
| Chebyshev II | Flat passband | Stopband ripple |
| Bessel | Linear phase (preserves shape) | Very slow transition |
| Elliptic | Sharpest transition | Ripple in both bands |

---

## 8. Efficient Convolution for Long Signals

### 8.1 Overlap-Add

```cpp
Array<double> long_signal = { /* thousands of samples */ };
Array<double> kernel = { /* FIR filter */ };

// Single call (chooses block_size automatically)
auto result = FFTD::overlap_add_convolution(long_signal, kernel);

// Reusable object (streaming)
FFTD::OverlapAdd convolver(kernel, 1024);  // block_size = 1024
auto out1 = convolver.process_block(chunk1);
auto out2 = convolver.process_block(chunk2);
auto tail = convolver.flush();
convolver.reset();  // ready for another signal
```

### 8.2 Overlap-Save

```cpp
FFTD::OverlapSave ols(kernel, 256);
auto out = ols.process_block(chunk);
auto rem = ols.flush();
```

### 8.3 Partitioned Convolution (Low Latency)

```cpp
// Latency = partition_size samples
FFTD::PartitionedConvolver pc(kernel, 64);  // 64-sample latency
auto out = pc.process_block(chunk);
```

### 8.4 Multichannel

```cpp
size_t num_channels = 2;
FFTD::OverlapAddBank bank(num_channels, kernel, 512);

Array<Array<double>> channels = {left_signal, right_signal};
auto filtered = bank.pconvolve(pool, channels);
// filtered[0] = filtered left, filtered[1] = filtered right
```

---

## 9. Spectral Estimation

### 9.1 Welch (PSD)

```cpp
auto window = FFTD::hann_window(256);
FFTD::WelchOptions opts;
opts.hop_size = 128;
opts.fft_size = 512;

auto psd = FFTD::welch(signal, window, sample_rate, opts);
// psd.frequency[k] -- Hz
// psd.density[k]   -- V^2/Hz
```

### 9.2 Cross Spectral Density

```cpp
auto csd = FFTD::csd(signal_x, signal_y, window, sample_rate, opts);
```

### 9.3 Coherence

```cpp
auto coh = FFTD::coherence(signal_x, signal_y, window, sample_rate, opts);
// coh.magnitude_squared[k] in [0, 1]
// 1.0 = perfect correlation at that frequency
```

---

## 10. Resampling

```cpp
// Upsample by 3, downsample by 2 (factor 3/2)
auto resampled = FFTD::resample_poly(signal, 3, 2);

// upfirdn: primitive upsample-filter-downsample operation
auto result = FFTD::upfirdn(signal, filter_taps, up_factor, down_factor);
```

---

## 11. Concurrency with ThreadPool

All computational methods have a parallel variant with the `p` prefix:

```cpp
# include <thread_pool.H>

ThreadPool pool(4);  // 4 threads

// Transforms
auto spec = FFTD::ptransform(pool, signal);
auto inv  = FFTD::pinverse_transform(pool, spectrum);

// Convolution
auto conv = FFTD::pmultiply(pool, signal, kernel);

// STFT
auto sg = FFTD::pstft(pool, signal, window, opts);
auto rec = FFTD::pistft(pool, sg, window, iopts);

// Batch FFTs
FFTD::Plan plan(n);
auto batch = plan.ptransformed_batch(pool, signals, false);
```

---

## 12. Reusable Plan

When transforming multiple signals of the same size, a `Plan`
precomputes the twiddle factor tables once:

```cpp
FFTD::Plan plan(1024);  // precompute for N=1024

// Individual transforms
auto X = plan.transformed(signal);
auto x = plan.inverse_transform(X);

// Batches
Array<Array<Complex>> batch = { sig1, sig2, sig3 };
auto results = plan.transformed_batch(batch, false);

// Compact real FFT
auto R = plan.rfft(real_signal);
auto s = plan.irfft(R, 1024);
```

---

## 13. N-Dimensional FFT

```cpp
// 2-D (rows of a complex "image")
Array<Array<Complex>> image = { row0, row1, row2, row3 };
auto spectrum_2d = FFTD::transformed2d(image);
auto recovered_2d = FFTD::transformed2d(spectrum_2d, true);  // inverse

// 3-D
Array<Array<Array<Complex>>> volume = { /* ... */ };
auto spectrum_3d = FFTD::transformed3d(volume);
```

---

## 14. Polynomial Roots

```cpp
// Find roots of p(x) = 1 + 2x + x^2 = (1+x)^2
Array<double> coeffs = {1.0, 2.0, 1.0};  // coef[i] = coefficient of x^i
auto roots = FFTD::polynomial_roots(coeffs);
// roots ≈ {(-1, 0), (-1, 0)}
```

---

## 15. SIMD Control

The SIMD backend is selected via environment variable:

```bash
# Force scalar (no SIMD)
ALEPH_FFT_SIMD=scalar ./my_program

# Force AVX2
ALEPH_FFT_SIMD=avx2 ./my_program

# Automatic (default): uses the best available
ALEPH_FFT_SIMD=auto ./my_program
```

Programmatic query:

```cpp
auto backend = FFTD::simd_backend_name();       // "scalar", "avx2", "neon"
bool avx2_ok = FFTD::avx2_dispatch_available();
```

---

## 16. Complete Example: Frequency Analyzer

```cpp
# include <cmath>
# include <numbers>
# include <iostream>
# include <fft.H>
# include <tpl_array.H>

using namespace Aleph;
using FFTD = FFT<double>;

int main()
{
  constexpr double sample_rate = 8000.0;
  constexpr size_t N = 256;

  // Generate signal: 440 Hz + 1000 Hz
  Array<double> signal;
  signal.reserve(N);
  for (size_t i = 0; i < N; ++i)
    {
      double t = static_cast<double>(i) / sample_rate;
      signal.append(std::sin(2.0 * std::numbers::pi * 440.0 * t)
                    + 0.5 * std::sin(2.0 * std::numbers::pi * 1000.0 * t));
    }

  // Window + FFT
  auto window = FFTD::hann_window(N);
  auto spectrum = FFTD::windowed_spectrum(signal, window);
  auto magnitudes = FFTD::magnitude_spectrum(spectrum);

  // Find peaks
  double freq_resolution = sample_rate / static_cast<double>(N);
  for (size_t k = 1; k < N / 2; ++k)
    if (magnitudes[k] > magnitudes[k-1] and magnitudes[k] > magnitudes[k+1]
        and magnitudes[k] > 5.0)
      std::cout << "Peak at " << k * freq_resolution << " Hz, "
                << "magnitude = " << magnitudes[k] << "\n";

  // Filter: keep only < 800 Hz
  auto lp = FFTD::butterworth_lowpass(4, 800.0, sample_rate);
  auto filtered = FFTD::sosfilt(signal, lp);

  // Verify that the 1000 Hz peak is gone
  auto filtered_spec = FFTD::windowed_spectrum(filtered, window);
  auto filtered_mags = FFTD::magnitude_spectrum(filtered_spec);

  std::cout << "Magnitude at 440 Hz after filtering: "
            << filtered_mags[static_cast<size_t>(440.0 / freq_resolution)]
            << "\n";
  std::cout << "Magnitude at 1000 Hz after filtering: "
            << filtered_mags[static_cast<size_t>(1000.0 / freq_resolution)]
            << "\n";

  return 0;
}
```

---

## 17. Quick Reference

| What do you want to do? | Method |
|--------------------------|--------|
| FFT of real signal | `FFTD::spectrum(signal)` or `FFTD::rfft(signal)` |
| Inverse FFT | `FFTD::inverse_transform(X)` or `FFTD::irfft(R, N)` |
| Convolution / multiplication | `FFTD::multiply(a, b)` |
| Spectrogram | `FFTD::stft(signal, window, opts)` |
| FIR filter | `FFTD::firwin_lowpass(taps, fc, sr, att)` |
| IIR filter | `FFTD::butterworth_lowpass(order, fc, sr)` |
| Apply IIR filter | `FFTD::sosfilt(signal, sections)` |
| Zero phase | `FFTD::filtfilt(signal, filter)` |
| PSD (Welch) | `FFTD::welch(signal, window, sr, opts)` |
| Resampling | `FFTD::resample_poly(signal, up, down)` |
| Frequency response | `FFTD::freqz(filter, npoints)` |
| Polynomial roots | `FFTD::polynomial_roots(coeffs)` |
| Streaming FIR | `FFTD::OverlapAdd(kernel, block_size)` |
| Streaming IIR | `FFTD::LFilter(biquads)` |
| Parallel FFT | `FFTD::ptransform(pool, signal)` |
| 2-D FFT | `FFTD::transformed2d(matrix)` |