/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/**
 * @file fft_example.cc
 * @brief Detailed demonstration of the Fast Fourier Transform (FFT).
 *
 * This example illustrates three primary use cases for the FFT:
 * 1. Spectral Analysis: Breaking down a signal into its constituent frequencies.
 * 2. Real-valued Convolution: Using the "Convolution Theorem" to filter data.
 * 3. Polynomial Multiplication: Efficiently multiplying polynomials with 
 *    complex coefficients.
 */

# include <cmath>
# include <complex>
# include <iomanip>
# include <iostream>
# include <numbers>
# include <vector>

# include <fft.H>
# include <print_rule.H>
# include <tpl_array.H>

using namespace std;
using namespace Aleph;

namespace
{
  // Alias for common types to make the code more readable.
  using FFTD = FFT<double>;
  using Complex = FFTD::Complex;

  /**
   * @brief Formats a complex number as "a + bi" or "a - bi".
   */
  void print_complex(const Complex & z)
  {
    cout << fixed << setprecision(4) << setw(9) << z.real();
    cout << (z.imag() < 0 ? " - " : " + ");
    cout << fixed << setprecision(4) << setw(8) << std::abs(z.imag()) << "i";
  }

  /**
   * @brief Prints a sequence of real numbers with their indices.
   */
  template <typename T>
  void print_real_sequence(const char * title, const Array<T> & values)
  {
    cout << title << "\n";
    for (size_t i = 0; i < values.size(); ++i)
      cout << "  [" << setw(2) << i << "] = "
           << fixed << setprecision(4) << values[i] << "\n";
    cout << "\n";
  }

  /**
   * @brief Prints a sequence of complex numbers with their indices.
   */
  void print_complex_sequence(const char * title, const Array<Complex> & values)
  {
    cout << title << "\n";
    for (size_t i = 0; i < values.size(); ++i)
      {
        cout << "  [" << setw(2) << i << "] = ";
        print_complex(values[i]);
        cout << "\n";
      }
    cout << "\n";
  }

  /**
   * @brief Case 1: Spectral Analysis of a Real Signal.
   * 
   * This demo constructs a signal by combining two sinusoids:
   * x(t) = 1.5 * cos(2*pi*1*t/N) + 0.75 * sin(2*pi*3*t/N)
   * 
   * We then use the FFT to identify these frequencies.
   */
  void demo_real_signal_spectrum()
  {
    cout << "[1] Spectral analysis of a real signal\n";
    cout << "Goal: Identify frequency components of a composite signal.\n";
    print_rule();

    constexpr size_t n = 10; // Arbitrary sizes are now supported directly.
    Array<double> signal;
    signal.reserve(n);

    // Step 1: Generate the signal in the time domain.
    for (size_t t = 0; t < n; ++t)
      {
        // Calculate the base angle for index t
        const double angle = 2.0 * std::numbers::pi * static_cast<double>(t)
                             / static_cast<double>(n);
        
        // Sum of a fundamental frequency (k=1) and its third harmonic (k=3)
        const double sample = 1.50 * std::cos(angle)
                              + 0.75 * std::sin(3.0 * angle);
        signal.append(sample);
      }

    // Step 2: Perform the FFT to move to the frequency domain.
    // The result is an array of complex numbers where X[k] represents the 
    // amplitude and phase of the frequency corresponding to index k.
    const auto spectrum = FFTD::spectrum(signal);
    const auto compact_spectrum = FFTD::rfft(signal);
    const auto restored_from_compact = FFTD::irfft(compact_spectrum, signal.size());
    const auto magnitudes = FFTD::magnitude_spectrum(spectrum);
    const auto phases = FFTD::phase_spectrum(spectrum);
    const auto hann_windowed = FFTD::windowed_spectrum(signal, FFTD::hann_window(signal.size()));
    const auto hann_magnitudes = FFTD::magnitude_spectrum(hann_windowed);

    print_real_sequence("Time-domain samples x[t]:", signal);

    cout << "Frequency bins X[k], magnitudes |X[k]| and phase arg(X[k]):\n";
    for (size_t k = 0; k < spectrum.size(); ++k)
      {
        cout << "  k=" << k << "  X[k] = ";
        print_complex(spectrum[k]);
        cout << "    |X[k]| = " << fixed << setprecision(4)
             << magnitudes[k]
             << "    phase = " << phases[k] << "\n";
      }

    cout << "\nCompact real spectrum R[k] (only 0..floor(N/2)):\n";
    for (size_t k = 0; k < compact_spectrum.size(); ++k)
      {
        cout << "  k=" << k << "  R[k] = ";
        print_complex(compact_spectrum[k]);
        cout << "\n";
      }

    cout << "\nAnalysis:\n";
    cout << "- Bin k=1 shows energy from 1.5 * cos(theta).\n";
    cout << "- Bin k=3 shows energy from 0.75 * sin(3*theta).\n";
    cout << "- For real signals, the spectrum is Hermitian symmetric:\n";
    cout << "  X[k] is the conjugate of X[N-k]. This is why |X[1]| = |X[9]|.\n";
    cout << "- FFT<Real>::rfft() exposes only the non-redundant bins and\n";
    cout << "  FFT<Real>::irfft() reconstructs the original arbitrary-length signal.\n";
    cout << "  Compact round-trip sample 0 = " << restored_from_compact[0] << ".\n";
    cout << "- Applying FFT<Real>::windowed_spectrum(..., hann_window(N)) reduces\n";
    cout << "  edge discontinuity; here Hann-windowed |X[1]| = "
         << hann_magnitudes[1] << ".\n";
    print_rule();
    cout << "\n";
  }

  /**
   * @brief Case 2: Linear Convolution via FFT (Filtering).
   * 
   * The Convolution Theorem states that convolution in the time domain 
   * is equivalent to element-wise multiplication in the frequency domain.
   * 
   * This demo applies a simple smoothing kernel (weighted average) to a signal.
   */
  void demo_real_convolution()
  {
    cout << "[2] Real convolution via FFT\n";
    cout << "Goal: Apply a smoothing filter to a sequence of samples.\n";
    print_rule();

    // The signal samples
    Array<double> samples = {3.0, 1.0, 4.0, 1.0, 5.0, 9.0};
    
    // A smoothing kernel (3-point moving average weights: 0.25, 0.5, 0.25)
    Array<double> kernel = {0.25, 0.50, 0.25};

    // FFT::multiply automatically:
    // 1. Pads both arrays to the nearest power-of-two size (N >= size1 + size2 - 1).
    // 2. Computes FFT for both.
    // 3. Multiplies them element-wise.
    // 4. Computes IFFT to return the result to the time domain.
    const auto filtered = FFTD::multiply(samples, kernel);

    print_real_sequence("Original Samples:", samples);
    print_real_sequence("Smoothing kernel (Filter):", kernel);
    print_real_sequence("Filtered Result (Linear Convolution):", filtered);

    cout << "This overload uses the optimized sequential path for real-valued inputs.\n";
    cout << "For long streams, FFT<Real>::OverlapAdd or overlap_add_convolution()\n";
    cout << "lets you reuse the filter spectrum block-by-block.\n";
    cout << "Observation: The convolution result has size N + M - 1 = 6 + 3 - 1 = 8.\n";
    cout << "The values at the edges reflect the ramp-up and ramp-down of the filter.\n";
    print_rule();
    cout << "\n";
  }

  /**
   * @brief Case 3: Complex Polynomial Multiplication.
   * 
   * Polynomials can be represented by their coefficients. Multiplying 
   * polynomials is equivalent to convolving their coefficients.
   * 
   * For large polynomials (n > 100), the FFT approach (O(n log n)) is 
   * significantly faster than the naive O(n^2) algorithm.
   */
  void demo_complex_convolution()
  {
    cout << "[3] Complex polynomial multiplication\n";
    cout << "Goal: Multiply two polynomials with complex coefficients.\n";
    print_rule();

    // A(x) = (1 + 0i) + (2 - 1i)x + (0 + 3i)x^2
    Array<Complex> a = {
      Complex(1.0, 0.0),
      Complex(2.0, -1.0),
      Complex(0.0, 3.0)
    };

    // B(x) = (2 + 0i) + (-1 + 0i)x + (0.5 + 2i)x^2
    Array<Complex> b = {
      Complex(2.0, 0.0),
      Complex(-1.0, 0.0),
      Complex(0.5, 2.0)
    };

    // Compute C(x) = A(x) * B(x)
    const auto c = FFTD::multiply(a, b);

    print_complex_sequence("A(x) coefficients:", a);
    print_complex_sequence("B(x) coefficients:", b);
    print_complex_sequence("C(x) = A(x) * B(x):", c);

    cout << "The result coefficients represent the new polynomial C(x).\n";
    cout << "This shows how the FFT gracefully handles complex values natively.\n";
    print_rule();
    cout << "\n";
  }

  void demo_generic_containers()
  {
    cout << "[4] Generic iterable containers\n";
    cout << "Goal: Use FFT directly with std::vector inputs.\n";
    print_rule();

    std::vector<double> real_signal = {1.0, 2.0, -1.0, 0.5, 3.0, -2.0, 4.0, -0.75};
    const auto real_spectrum = FFTD::transform(real_signal);

    std::vector<Complex> poly_a = {
      Complex(1.0, 0.0),
      Complex(2.0, -1.0),
      Complex(0.0, 3.0)
    };
    std::vector<Complex> poly_b = {
      Complex(2.0, 0.0),
      Complex(-1.0, 0.0),
      Complex(0.5, 2.0)
    };
    const auto product = FFTD::multiply(poly_a, poly_b);

    cout << "Input std::vector<double> size: " << real_signal.size() << "\n";
    cout << "FFT(real_signal) returned an Aleph Array with "
         << real_spectrum.size() << " frequency bins.\n\n";

    print_complex_sequence("FFT(std::vector<double>) spectrum:", real_spectrum);
    print_complex_sequence("FFT::multiply(std::vector<Complex>, std::vector<Complex>):",
                           product);

    cout << "No manual conversion to Array is needed: compatible iterable\n"
         << "containers are accepted directly by FFT<Real>.\n";
    print_rule();
    cout << "\n";
  }

  void demo_concurrent_fft()
  {
    cout << "[5] Concurrent FFT with ThreadPool\n";
    cout << "Goal: Keep the sequential and concurrent APIs explicitly separated.\n";
    print_rule();

    ThreadPool pool(4);

    Array<double> signal = {0.5, -1.25, 3.5, 2.0, -0.75, 4.25, 1.0, -2.0};
    Array<double> kernel = {0.25, 0.50, 0.25};
    Array<Array<Complex>> complex_batch = {
      Array<Complex>({Complex(0.5, 0.0), Complex(-1.25, 0.0), Complex(3.5, 0.0),
                      Complex(2.0, 0.0), Complex(-0.75, 0.0), Complex(4.25, 0.0),
                      Complex(1.0, 0.0), Complex(-2.0, 0.0)}),
      Array<Complex>({Complex(1.0, 0.5), Complex(-0.5, 0.25), Complex(0.75, -1.0),
                      Complex(2.0, 0.0), Complex(-1.0, 0.5), Complex(0.25, -0.25),
                      Complex(1.5, 0.75), Complex(-0.75, -0.5)})
    };

    const auto seq_spectrum = FFTD::transform(signal);
    const auto par_spectrum = FFTD::ptransform(pool, signal);
    const auto par_filtered = FFTD::pmultiply(pool, signal, kernel);
    FFTD::Plan plan(complex_batch[0].size());
    const auto batch_spectra = plan.ptransformed_batch(pool, complex_batch, false);
    Array<Array<Complex>> image = {
      Array<Complex>({Complex(1.0, 0.0), Complex(2.0, -0.5),
                      Complex(-1.0, 0.25), Complex(0.5, 1.0)}),
      Array<Complex>({Complex(-0.75, 1.5), Complex(1.25, -1.0),
                      Complex(0.0, 0.5), Complex(-2.0, 0.0)}),
      Array<Complex>({Complex(0.5, -0.25), Complex(-1.5, 0.75),
                      Complex(2.5, 0.0), Complex(1.0, -1.5)})
    };
    const auto image_spectrum = FFTD::transformed2d(image);
    FFTD::OverlapAddBank overlap_bank(2, kernel, 4);
    Array<Array<double>> overlap_signals = {
      signal,
      Array<double>({1.0, -0.5, 0.75, 2.0, -1.25, 0.5, 0.25, -0.75})
    };
    const auto overlap_batch = overlap_bank.pconvolve(pool, overlap_signals);

    print_real_sequence("Input signal:", signal);
    print_complex_sequence("Sequential FFT(signal):", seq_spectrum);
    print_complex_sequence("Concurrent FFT(signal):", par_spectrum);
    print_real_sequence("Concurrent convolution result:", par_filtered);
    print_complex_sequence("Concurrent batch FFT(signal 0):", batch_spectra[0]);
    print_complex_sequence("2-D FFT(image row 0):", image_spectrum[0]);
    print_real_sequence("Concurrent overlap-add bank (channel 0):", overlap_batch[0]);
    cout << "SIMD backend for FFT<double>: " << FFTD::simd_backend_name()
         << " (batch-plan backend: " << FFTD::batched_plan_simd_backend_name()
         << ", detected hardware backend: "
         << FFTD::detected_simd_backend_name()
         << ", policy: " << FFTD::simd_preference_name()
         << ", avx2 compiled/runtime: "
         << (FFTD::avx2_kernel_compiled() ? "yes" : "no") << "/"
         << (FFTD::avx2_runtime_available() ? "yes" : "no")
         << ", neon compiled/runtime: "
         << (FFTD::neon_kernel_compiled() ? "yes" : "no") << "/"
         << (FFTD::neon_runtime_available() ? "yes" : "no") << ")\n";

    cout << "Use FFT<Real>::transform()/multiply() for the sequential path and\n"
         << "FFT<Real>::ptransform()/pmultiply() when you want explicit ThreadPool\n"
         << "parallelism under Aleph's concurrency style. When many complex inputs\n"
         << "share the same length, FFT<Real>::Plan::ptransformed_batch() reuses one\n"
         << "plan and parallelizes across the batch instead of across a single FFT.\n"
         << "For image-like data, transformed2d()/ptransformed2d() remove the need to\n"
         << "manually loop over rows and columns, while transform_axes() targets flat\n"
         << "buffers with explicit shape/stride metadata.\n"
         << "For multichannel FIR streaming, FFT<Real>::OverlapAddBank shares the\n"
         << "kernel spectrum across channels instead of rebuilding one convolver per\n"
         << "channel.\n";
    print_rule();
    cout << "\n";
  }

  void demo_stft_and_zero_phase_filtering()
  {
    cout << "[6] STFT, ISTFT, and zero-phase filtering\n";
    cout << "Goal: Analyze by frames, reconstruct by overlap-add, inspect IIR response,\n"
         << "stream STFT in chunks, and compare causal vs zero-phase filtering.\n";
    print_rule();

    ThreadPool pool(4);
    Array<double> signal = {
      0.0, 0.8, 1.0, 0.2, -0.7, -1.0, -0.3, 0.6,
      1.1, 0.4, -0.5, -0.9
    };
    Array<double> analysis_window = {1.0, 0.75, 0.5, 1.0};
    Array<double> fir = {0.2, 0.6, 0.2};
    FFTD::BiquadSection iir = {0.075, 0.15, 0.075, 1.0, -0.9, 0.2};

    FFTD::STFTOptions stft_options;
    stft_options.hop_size = 2;
    stft_options.centered = true;
    stft_options.pad_end = true;
    stft_options.fft_size = 8;
    stft_options.validate_nola = true;

    FFTD::ISTFTOptions istft_options;
    istft_options.hop_size = 2;
    istft_options.centered = true;
    istft_options.signal_length = signal.size();
    istft_options.validate_nola = true;

    const auto frames = FFTD::frame_signal(signal, analysis_window.size(), 2, true);
    const auto spectrogram = FFTD::stft(signal, analysis_window, stft_options);
    const auto parallel_spectrogram = FFTD::pstft(pool,
                                                  signal,
                                                  analysis_window,
                                                  stft_options);
    const auto reconstructed = FFTD::istft(spectrogram, analysis_window, istft_options);
    const auto zero_phase_fir = FFTD::filtfilt(signal, fir, 4);
    const auto zero_phase_iir = FFTD::filtfilt(signal, iir);
    const auto designed_fir = FFTD::firwin_lowpass(33, 1200.0, 8000.0, 70.0);
    Array<double> firls_bands = {0.0, 900.0, 1200.0, 4000.0};
    Array<double> firls_desired = {1.0, 1.0, 0.0, 0.0};
    Array<double> firls_weights = {1.0, 12.0};
    const auto designed_firls =
      FFTD::firls(33, firls_bands, firls_desired, 8000.0, firls_weights);
    Array<double> remez_weights = {1.0, 16.0};
    const auto designed_remez =
      FFTD::remez(33, firls_bands, firls_desired, 8000.0, remez_weights);
    const auto designed_bandpass =
      FFTD::chebyshev2_bandpass(4, 40.0, 700.0, 1500.0, 8000.0);
    const auto designed_elliptic =
      FFTD::elliptic_bandstop(4, 1.0, 40.0, 700.0, 1500.0, 8000.0);
    const auto response = FFTD::freqz(iir, 5);
    const auto designed_firls_response = FFTD::freqz(designed_firls, 5, false);
    const auto designed_remez_response = FFTD::freqz(designed_remez, 5, false);
    const auto designed_bandpass_response = FFTD::freqz(designed_bandpass, 5);
    const auto designed_elliptic_response = FFTD::freqz(designed_elliptic, 5);
    const auto poles = FFTD::poles(iir);
    const auto pairs = FFTD::pair_poles_and_zeros(iir);
    const auto group = FFTD::group_delay(iir, 5);
    const auto phase = FFTD::phase_delay(iir, 5);
    const auto stability_margin = FFTD::stability_margin(iir);
    const auto min_pair_distance = FFTD::minimum_pole_zero_distance(iir);

    FFTD::STFTProcessor processor(analysis_window, stft_options);
    FFTD::ISTFTProcessor inverse_processor(spectrogram[0].size(),
                                           analysis_window,
                                           istft_options);
    Array<Array<Complex>> streamed_spectrogram;
    Array<double> streamed_reconstruction;

    FFTD::LFilter causal_iir(iir);
    Array<double> first_half;
    Array<double> second_half;
    for (size_t i = 0; i < signal.size() / 2; ++i)
      first_half.append(signal[i]);
    for (size_t i = signal.size() / 2; i < signal.size(); ++i)
      second_half.append(signal[i]);
    const auto streamed_first = processor.process_block(first_half);
    const auto streamed_second = processor.process_block(second_half);
    const auto streamed_tail = processor.flush();
    const auto causal_first = causal_iir.filter(first_half);
    const auto causal_second = causal_iir.filter(second_half);
    Array<double> causal_streamed;
    for (size_t i = 0; i < causal_first.size(); ++i)
      causal_streamed.append(causal_first[i]);
    for (size_t i = 0; i < causal_second.size(); ++i)
      causal_streamed.append(causal_second[i]);
    for (size_t i = 0; i < streamed_first.size(); ++i)
      streamed_spectrogram.append(streamed_first[i]);
    for (size_t i = 0; i < streamed_second.size(); ++i)
      streamed_spectrogram.append(streamed_second[i]);
    for (size_t i = 0; i < streamed_tail.size(); ++i)
      streamed_spectrogram.append(streamed_tail[i]);

    Array<Array<Complex>> first_frames;
    Array<Array<Complex>> second_frames;
    for (size_t i = 0; i < spectrogram.size() / 2; ++i)
      first_frames.append(spectrogram[i]);
    for (size_t i = spectrogram.size() / 2; i < spectrogram.size(); ++i)
      second_frames.append(spectrogram[i]);
    const auto reconstructed_first = inverse_processor.process_block(first_frames);
    const auto reconstructed_second = inverse_processor.process_block(second_frames);
    const auto reconstructed_tail = inverse_processor.flush();
    for (size_t i = 0; i < reconstructed_first.size(); ++i)
      streamed_reconstruction.append(reconstructed_first[i]);
    for (size_t i = 0; i < reconstructed_second.size(); ++i)
      streamed_reconstruction.append(reconstructed_second[i]);
    for (size_t i = 0; i < reconstructed_tail.size(); ++i)
      streamed_reconstruction.append(reconstructed_tail[i]);

    Array<Array<double>> batch_signals = {signal, zero_phase_fir};
    const auto batched = FFTD::batched_stft(batch_signals,
                                            analysis_window,
                                            stft_options);
    const auto frame_major =
      FFTD::multichannel_stft(batch_signals,
                              analysis_window,
                              stft_options,
                              FFTD::SpectrogramLayout::frame_channel_bin);
    const auto multichannel_reconstructed =
      FFTD::multichannel_istft(frame_major,
                               analysis_window,
                               analysis_window,
                               istft_options,
                               Array<size_t>({signal.size(), zero_phase_fir.size()}),
                               FFTD::SpectrogramLayout::frame_channel_bin);
    Array<double> designed_fir_preview;
    for (size_t i = 0; i < std::min<size_t>(6, designed_fir.size()); ++i)
      designed_fir_preview.append(designed_fir[i]);
    Array<double> designed_firls_preview;
    for (size_t i = 0; i < std::min<size_t>(6, designed_firls.size()); ++i)
      designed_firls_preview.append(designed_firls[i]);
    Array<double> designed_remez_preview;
    for (size_t i = 0; i < std::min<size_t>(6, designed_remez.size()); ++i)
      designed_remez_preview.append(designed_remez[i]);

    print_real_sequence("Input signal:", signal);
    cout << "Raw frame_signal frames: " << frames.size()
         << ", centered STFT frames: " << spectrogram.size()
         << ", FFT bins per frame: " << spectrogram[0].size() << "\n";
    cout << "Parallel STFT frames: " << parallel_spectrogram.size()
         << ", streamed STFT frames: " << streamed_spectrogram.size()
         << ", streamed ISTFT samples: " << streamed_reconstruction.size()
         << ", batched STFT items: " << batched.size()
         << ", frame-major multichannel frames: " << frame_major.size() << "\n";
    print_real_sequence("ISTFT reconstruction:", reconstructed);
    print_real_sequence("Streamed ISTFT reconstruction:", streamed_reconstruction);
    print_real_sequence("Causal streamed IIR result:", causal_streamed);
    print_real_sequence("Zero-phase FIR result:", zero_phase_fir);
    print_real_sequence("Zero-phase IIR result:", zero_phase_iir);
    print_real_sequence("firwin_lowpass taps (first 6):", designed_fir_preview);
    print_real_sequence("firls low-pass taps (first 6):", designed_firls_preview);
    print_real_sequence("remez low-pass taps (first 6):", designed_remez_preview);
    print_real_sequence("Frame-major multichannel ISTFT(channel 0):",
                        multichannel_reconstructed[0]);
    print_complex_sequence("STFT frame 0:", spectrogram[0]);
    print_complex_sequence("STFT frame 1:", spectrogram[1]);
    print_complex_sequence("IIR poles:", poles);
    print_complex_sequence("Chebyshev-II band-pass response samples:",
                           designed_bandpass_response.response);
    print_complex_sequence("Elliptic band-stop response samples:",
                           designed_elliptic_response.response);
    print_complex_sequence("firls low-pass response samples:",
                           designed_firls_response.response);
    print_complex_sequence("remez low-pass response samples:",
                           designed_remez_response.response);
    cout << "Pole/zero pairs tracked: " << pairs.size() << "\n";
    print_real_sequence("Estimated group delay:", group);
    print_real_sequence("Estimated phase delay:", phase);
    cout << "IIR |H(0)| = " << std::abs(response.response[0])
         << ", |H(pi)| = " << std::abs(response.response[response.response.size() - 1])
         << ", stable = " << (FFTD::is_stable(iir) ? "yes" : "no")
         << ", stability margin = " << stability_margin
         << ", min pole/zero distance = " << min_pair_distance
         << "\n";

    cout << "frame_signal() exposes the hop-size layout explicitly, stft() applies\n"
         << "windowing + zero-padding per frame, pstft() parallelizes the same analysis,\n"
         << "STFTProcessor and ISTFTProcessor keep chunked analysis/synthesis aligned\n"
         << "with the offline result, the centered STFT/ISTFT option pair reconstructs\n"
         << "the original signal exactly under NOLA, batched_stft() scales the API to\n"
         << "multiple signals, multichannel_stft()/multichannel_istft() switch between\n"
         << "channel-major and frame-major layouts without manual transposes, and\n"
         << "freqz() plus poles()/pair_poles_and_zeros(),\n"
         << "stability_margin(), group_delay()/phase_delay() expose filter behavior.\n"
         << "The same header also covers FIR design through firwin_*(), firls(), remez(),\n"
         << "and reusable Chebyshev-II plus elliptic/Cauer SOS designs.\n";
    print_rule();
    cout << "\n";
  }

  void demo_production_dsp_utilities()
  {
    cout << "[7] Production DSP utilities\n";
    cout << "Goal: Estimate PSD/coherence, resample by rational factors, and\n"
         << "compare overlap-save with low-latency partitioned convolution.\n";
    print_rule();

    constexpr double sample_rate = 8000.0;
    Array<double> signal;
    signal.reserve(256);
    Array<double> shifted_signal;
    shifted_signal.reserve(256);
    for (size_t i = 0; i < 256; ++i)
      {
        const double t = static_cast<double>(i) / sample_rate;
        signal.append(std::sin(2.0 * std::numbers::pi * 1000.0 * t)
                      + 0.35 * std::sin(2.0 * std::numbers::pi * 1800.0 * t));
        shifted_signal.append(1.5 * std::sin(2.0 * std::numbers::pi * 1000.0 * t + 0.25)
                              + 0.35 * std::sin(2.0 * std::numbers::pi * 1800.0 * t));
      }

    const auto window = FFTD::hann_window(64);
    FFTD::WelchOptions welch_options;
    welch_options.hop_size = 32;
    welch_options.fft_size = 128;

    const auto psd = FFTD::welch(signal, window, sample_rate, welch_options);
    const auto cross = FFTD::csd(signal, shifted_signal, window, sample_rate, welch_options);
    const auto coh = FFTD::coherence(signal, shifted_signal, window, sample_rate, welch_options);

    size_t peak = 0;
    for (size_t i = 1; i < psd.density.size(); ++i)
      if (psd.density[i] > psd.density[peak])
        peak = i;

    Array<double> prototype = {1.0, -1.0, 2.0, 0.5};
    Array<double> kernel = {0.25, 0.5, 0.25};
    const auto upfirdn_result = FFTD::upfirdn(prototype, kernel, 3, 2);
    const auto resampled = FFTD::resample_poly(signal, 3, 2);

    FFTD::OverlapSave overlap_save(kernel, 16);
    Array<double> overlap_save_streamed;
    const auto overlap_first =
      overlap_save.process_block(Array<double>({1.0, -1.0, 0.5}));
    const auto overlap_second =
      overlap_save.process_block(Array<double>({2.0, 0.25, -0.5, 1.0}));
    const auto overlap_tail = overlap_save.flush();
    for (size_t i = 0; i < overlap_first.size(); ++i)
      overlap_save_streamed.append(overlap_first[i]);
    for (size_t i = 0; i < overlap_second.size(); ++i)
      overlap_save_streamed.append(overlap_second[i]);
    for (size_t i = 0; i < overlap_tail.size(); ++i)
      overlap_save_streamed.append(overlap_tail[i]);

    FFTD::PartitionedConvolver partitioned(kernel, 8);
    Array<double> partitioned_streamed;
    const auto partitioned_first =
      partitioned.process_block(Array<double>({1.0, -1.0, 0.5}));
    const auto partitioned_second =
      partitioned.process_block(Array<double>({2.0, 0.25, -0.5, 1.0}));
    const auto partitioned_tail = partitioned.flush();
    for (size_t i = 0; i < partitioned_first.size(); ++i)
      partitioned_streamed.append(partitioned_first[i]);
    for (size_t i = 0; i < partitioned_second.size(); ++i)
      partitioned_streamed.append(partitioned_second[i]);
    for (size_t i = 0; i < partitioned_tail.size(); ++i)
      partitioned_streamed.append(partitioned_tail[i]);

    Array<double> resampled_preview;
    for (size_t i = 0; i < std::min<size_t>(10, resampled.size()); ++i)
      resampled_preview.append(resampled[i]);
    Array<double> upfirdn_preview;
    for (size_t i = 0; i < std::min<size_t>(10, upfirdn_result.size()); ++i)
      upfirdn_preview.append(upfirdn_result[i]);

    print_real_sequence("upfirdn prototype result (first 10):", upfirdn_preview);
    print_real_sequence("resample_poly(signal, 3/2) (first 10):", resampled_preview);
    print_real_sequence("Overlap-save streaming result:", overlap_save_streamed);
    print_real_sequence("Partitioned streaming result:", partitioned_streamed);

    cout << "Welch peak frequency: " << psd.frequency[peak]
         << " Hz, PSD = " << psd.density[peak] << "\n";
    cout << "Cross-spectrum magnitude at that bin: "
         << std::abs(cross.density[peak])
         << ", coherence = " << coh.magnitude_squared[peak] << "\n";
    cout << "Window coherent gain = " << FFTD::window_coherent_gain(window)
         << ", ENBW = " << FFTD::window_enbw(window)
         << ", frame count (exact) = "
         << FFTD::frame_offsets(signal.size(), window.size(), welch_options.hop_size, false).size()
         << "\n";

    cout << "welch()/csd()/coherence() provide one-sided spectral estimators,\n"
         << "upfirdn()/resample_poly() cover rational-rate sample conversion,\n"
         << "overlap_save_convolution() targets block FIR filtering, and\n"
         << "PartitionedConvolver keeps latency tied to the configured partition size.\n";
    print_rule();
    cout << "\n";
  }
}

/**
 * @brief Main entry point.
 */
int main()
{
  cout << "\n=== Aleph-w: Fast Fourier Transform (FFT) Demonstration ===\n\n";

  // Case 1: Decomposing a signal into frequency components.
  demo_real_signal_spectrum();
  
  // Case 2: Using convolution for filtering real data.
  demo_real_convolution();
  
  // Case 3: Multiplying polynomials with complex coefficients.
  demo_complex_convolution();

  // Case 4: Working directly with compatible iterable containers.
  demo_generic_containers();

  // Case 5: Using the separate concurrent API with ThreadPool.
  demo_concurrent_fft();

  // Case 6: Short-time analysis and zero-phase FIR filtering.
  demo_stft_and_zero_phase_filtering();

  // Case 7: Production DSP helpers on top of the FFT core.
  demo_production_dsp_utilities();

  cout << "All demonstrations completed successfully.\n";
  return 0;
}
