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

/** @file fft.H
 *  @brief Fast Fourier Transform (FFT) and Digital Signal Processing (DSP) toolkit.
 *
 *  This header provides a robust, high-performance toolkit for frequency-domain
 *  computations. It is designed to handle both power-of-two sizes (via an 
 *  optimized mixed radix-2/4 Cooley-Tukey kernel) and arbitrary sizes (via 
 *  Bluestein's algorithm).
 *
 *  ### Features at a Glance:
 *  - **Spectral Analysis**: Forward and inverse FFTs for complex and real signals.
 *  - **Planning**: Precompute and cache twiddle factors and bit-reversal tables 
 *    using @ref Aleph::FFT::Plan for repeated transforms.
 *  - **Filtering**: FIR and IIR filters, second-order sections (SOS), and 
 *    zero-phase filtering (filtfilt).
 *  - **Analysis**: Welch PSD estimation, spectrograms (STFT), frequency 
 *    response (freqz), and group delay.
 *  - **Acceleration**: SIMD vectorization (AVX2/NEON) and multi-core parallelism.
 *
 *  ### Mathematical Foundations:
 *  - **Cooley-Tukey**: Efficient O(N log N) recursive decomposition for N=2^k.
 *  - **Bluestein's Algorithm**: Converts a DFT of any size N into a linear 
 *    convolution, which is then computed using power-of-two FFTs.
 *  - **Mixed Radix**: Optimized kernels for small primes (2, 3, 5).
 *
 *  @example fft_example.cc
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef FFT_H
# define FFT_H

# include <algorithm>
# include <cmath>
# include <complex>
# include <concepts>
# include <cstdlib>
# include <iterator>
# include <limits>
# include <memory>
# include <numeric>
# include <numbers>
# include <string_view>
# include <type_traits>

# if (defined(__GNUC__) or defined(__clang__)) \
      and (defined(__x86_64__) or defined(__i386__) \
           or defined(_M_X64) or defined(_M_IX86))
#   include <immintrin.h>
#   define ALEPH_FFT_HAS_X86_AVX2_DISPATCH 1
#   define ALEPH_FFT_AVX2_TARGET __attribute__((target("avx2")))
# else
#   define ALEPH_FFT_HAS_X86_AVX2_DISPATCH 0
# endif

# if (defined(__GNUC__) or defined(__clang__)) \
      and (defined(__aarch64__) or defined(__arm__) or defined(_M_ARM64) \
           or defined(_M_ARM))
#   if defined(__ARM_NEON) or defined(__ARM_NEON__)
#     include <arm_neon.h>
#     define ALEPH_FFT_HAS_ARM_NEON_DISPATCH 1
#   else
#     define ALEPH_FFT_HAS_ARM_NEON_DISPATCH 0
#   endif
# else
#   define ALEPH_FFT_HAS_ARM_NEON_DISPATCH 0
# endif

# if ALEPH_FFT_HAS_ARM_NEON_DISPATCH and defined(__aarch64__)
#   define ALEPH_FFT_HAS_ARM_NEON_DOUBLE 1
# else
#   define ALEPH_FFT_HAS_ARM_NEON_DOUBLE 0
# endif

# if ALEPH_FFT_HAS_ARM_NEON_DISPATCH and defined(__linux__)
#   include <sys/auxv.h>
#   include <asm/hwcap.h>
# endif

# include <ah-errors.H>
# include <thread_pool.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Fast Fourier Transform (FFT) and DSP Toolkit.
   *
   *  The FFT class provides a high-performance suite for time-to-frequency
   *  domain conversion and signal processing.
   *
   *  ### Performance and Architecture:
   *  - **Mixed Radix Kernel**: Automatically chooses the best algorithm for
   *    the input size. Radix-2/4 for powers of two, Bluestein for primes.
   *  - **Precomputed Plans**: Use @ref Plan to avoid re-calculating twiddle
   *    factors for repeated transforms of the same length.
   *  - **Concurrency**: Parallel APIs (prefixed with `p`) leverage a
   *    `ThreadPool` for large-scale data processing.
   *
   *  ### Thread Safety:
   *  All methods are `static`; there is no shared mutable instance state.
   *  Multiple threads may call any method concurrently on independent data
   *  without synchronization.  The one exception is @ref Plan: a single
   *  `Plan` object must not be used concurrently from multiple threads
   *  unless all concurrent calls are read-only (query-only methods).
   *
   *  ### Exception Safety:
   *  - Methods that allocate memory (e.g. `transformed`, `convolve`) provide
   *    the **basic guarantee**: if `std::bad_alloc` or any other exception is
   *    thrown, no resources are leaked and all objects remain in a valid
   *    (though possibly partially modified) state.
   *  - In-place methods (e.g. `transform(Array<Complex>&, bool)`) provide the
   *    **basic guarantee**: the array remains valid but may be partially
   *    transformed if an exception is thrown mid-way.
   *  - Callers should be prepared to handle `std::bad_alloc` (memory
   *    allocation failure), `std::invalid_argument` (zero-length or
   *    unsupported input), and `std::domain_error` (e.g. non-Hermitian
   *    spectrum passed to an inverse real transform).
   *  - No public method is declared `noexcept`; all may propagate exceptions.
   *
   *  @tparam Real Floating-point precision (double, float, long double).
   */
  template <std::floating_point Real = double>
  class FFT
  {
  public:
    using value_type = Real;
    using Complex = std::complex<Real>;

    /** @brief Precomputed plans for FFT transforms.
     *
     *  Stores twiddle factors and auxiliary data (like bit-reversal tables
     *  or Bluestein kernels) for a fixed transform size.
     */
    class Plan;

    /** @brief SIMD hardware acceleration backends. */
    enum class SimdBackend
    {
      scalar, ///< Generic implementation.
      avx2, ///< Intel/AMD AVX2.
      neon ///< ARM NEON.
    };

    /** @brief User-selected SIMD dispatch preference. */
    enum class SimdPreference
    {
      automatic, ///< Use the fastest available hardware.
      scalar_only, ///< Force the portable scalar implementation.
      avx2_only, ///< Force AVX2 (must be supported by CPU).
      neon_only ///< Force NEON (must be supported by CPU).
    };

    /** @brief Coefficients for an Infinite Impulse Response (IIR) filter.
     *  Represents a rational transfer function H(z) = B(z) / A(z).
     */
    struct IIRCoefficients
    {
      Array<Real> numerator; ///< Feed-forward coefficients (b).
      Array<Real> denominator; ///< Feed-backward coefficients (a).
    };

    /** @brief A stable Second-Order Section (SOS) building block.
     *  Commonly used in biquad cascades to avoid numerical instability.
     */
    struct BiquadSection
    {
      Real b0 = Real(1);
      Real b1 = Real(0);
      Real b2 = Real(0);
      Real a0 = Real(1);
      Real a1 = Real(0);
      Real a2 = Real(0);

      /** @return A copy of the numerator coefficients. */
      [[nodiscard]] Array<Real>
      numerator() const
      {
        return {b0, b1, b2};
      }

      /** @return A copy of the denominator coefficients. */
      [[nodiscard]] Array<Real>
      denominator() const
      {
        return {a0, a1, a2};
      }
    };

    /** @brief Discrete frequency response sampled on a fixed angular grid. */
    struct FrequencyResponse
    {
      Array<Real> omega;
      Array<Complex> response;

      [[nodiscard]] Array<Real>
      magnitude() const
      {
        return FFT::magnitude_spectrum(response);
      }

      [[nodiscard]] Array<Real>
      power() const
      {
        return FFT::power_spectrum(response);
      }

      [[nodiscard]] Array<Real>
      phase() const
      {
        return FFT::phase_spectrum(response);
      }
    };

    /** @brief Phase-margin estimate around a gain crossover. */
    struct PhaseMarginInfo
    {
      Real radians = std::numeric_limits<Real>::infinity();
      Real degrees = std::numeric_limits<Real>::infinity();
      Real crossover_omega = std::numeric_limits<Real>::quiet_NaN();
      bool found = false;
    };

    /** @brief Gain-margin estimate around a phase crossover. */
    struct GainMarginInfo
    {
      Real ratio = std::numeric_limits<Real>::infinity();
      Real decibels = std::numeric_limits<Real>::infinity();
      Real crossover_omega = std::numeric_limits<Real>::quiet_NaN();
      bool found = false;
    };

    /** @brief Greedy nearest-neighbor pole/zero pairing entry. */
    struct PoleZeroPair
    {
      Complex zero = Complex(Real(0), Real(0));
      Complex pole = Complex(Real(0), Real(0));
      Real distance = std::numeric_limits<Real>::infinity();
      bool has_zero = false;
      bool has_pole = false;

      [[nodiscard]] bool
      is_cancellation(const Real tolerance) const noexcept
      {
        return has_zero and has_pole and distance <= tolerance;
      }
    };

    /** @brief Options for STFT analysis. */
    struct STFTOptions
    {
      size_t hop_size = 0;
      bool pad_end = true;
      bool centered = false;
      size_t fft_size = 0;
      bool validate_nola = false;
      bool validate_cola = false;
    };

    /** @brief Options for ISTFT reconstruction. */
    struct ISTFTOptions
    {
      size_t hop_size = 0;
      size_t signal_length = 0;
      bool centered = false;
      bool validate_nola = true;
      bool validate_cola = false;
    };

    /** @brief Options for Welch PSD, CSD, and coherence estimation. */
    struct WelchOptions
    {
      size_t hop_size = 0;
      size_t fft_size = 0;
      bool pad_end = true;
      bool detrend = true;
    };

    /** @brief One-sided power spectral density estimate sampled in Hertz. */
    struct PowerSpectralDensity
    {
      Array<Real> frequency;
      Array<Real> density;
    };

    /** @brief One-sided cross-spectral density estimate sampled in Hertz. */
    struct CrossSpectralDensity
    {
      Array<Real> frequency;
      Array<Complex> density;
    };

    /** @brief Magnitude-squared coherence sampled in Hertz. */
    struct CoherenceEstimate
    {
      Array<Real> frequency;
      Array<Real> magnitude_squared;
    };

    /** @brief Options for default polyphase resampling filter design. */
    struct ResamplePolyOptions
    {
      size_t taps_per_phase = 10;
      Real attenuation_db = Real(80);
    };

    /** @brief Layout descriptor for a flat tensor buffer. */
    struct TensorLayout
    {
      Array<size_t> shape;
      Array<size_t> strides;
    };

    /** @brief Logical layout for multichannel spectrograms. */
    enum class SpectrogramLayout
    {
      channel_frame_bin,
      frame_channel_bin
    };

  private:
    template <typename T>
    struct Is_Std_Complex : std::false_type
    {};

    template <typename T>
    struct Is_Std_Complex<std::complex<T>> : std::true_type
    {};

    template <typename Container>
    static constexpr bool Is_Complex_Container = requires(const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      };

    template <typename Container>
    static constexpr bool Is_Real_Container = requires(const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires std::convertible_to<decltype(*std::begin(c)), Real>;
        requires not Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      };

    template <typename Container>
    static constexpr bool Is_Biquad_Container = requires(const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires std::convertible_to<std::remove_cvref_t<decltype(*std::begin(c))>,
                                     BiquadSection>;
      };

    template <typename Container>
    static constexpr bool Is_Real_Batch_Container = requires(const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires Is_Real_Container<std::remove_cvref_t<decltype(*std::begin(c))>>;
      };

    static constexpr size_t twiddle_refresh_period = 32;

    /** @brief Computes a single twiddle factor exp(j * angle * index). */
    [[nodiscard]] static Complex
    twiddle_at(const Real angle, const size_t index)
    {
      return std::polar(Real(1), angle * static_cast<Real>(index));
    }

    /** @brief Returns log2(N) stages, used for numerical error scaling. */
    [[nodiscard]] static Real
    transform_stages(const size_t n) noexcept
    {
      return std::max(Real(1),
                      std::log2(static_cast<Real>(std::max(n, size_t(2)))));
    }

    /** @brief Rationale: Estimates the maximum expected numerical noise for a value
     *         after an N-point transform, used to validate real projections.
     */
    [[nodiscard]] static Real
    real_projection_tolerance(const Complex & value, const size_t n) noexcept
    {
      const Real scale = std::abs(value.real()) + std::abs(value.imag()) + Real(1);
      return Real(128) * transform_stages(n)
             * std::numeric_limits<Real>::epsilon() * scale;
    }

    /** @brief Rationale: Numerical tolerance for Hermitian symmetry checks.
     *         Determines if X[k] and X[N-k]* are "close enough".
     */
    [[nodiscard]] static Real
    hermitian_tolerance(const Complex & lhs, const Complex & rhs,
                        const size_t n) noexcept
    {
      const Real scale = std::abs(lhs.real()) + std::abs(lhs.imag())
                         + std::abs(rhs.real()) + std::abs(rhs.imag())
                         + Real(1);
      return Real(256) * transform_stages(n)
             * std::numeric_limits<Real>::epsilon() * scale;
    }

    /** @brief Rationale: Ensures a complex spectrum has the Hermitian symmetry
     *         (X[k] = X[N-k]*) required to be the transform of a real signal.
     */
    static void
    validate_real_spectrum(const Array<Complex> & input, const char *ctx)
    {
      const size_t n = input.size();
      ah_invalid_argument_if(n == 0)
        << ctx << ": input must be non-empty";

      ah_domain_error_if(std::abs(input[0].imag())
                         > real_projection_tolerance(input[0], n))
        << ctx << ": coefficient 0 has non-negligible imaginary part "
        << input[0].imag();

      if (n == 1)
        return;

      const size_t half = n / 2;
      if ((n & 1) == 0)
        ah_domain_error_if(std::abs(input[half].imag())
                         > real_projection_tolerance(input[half], n))
            << ctx << ": coefficient " << half
            << " has non-negligible imaginary part " << input[half].imag();

      const size_t limit = (n & 1) == 0 ? half : half + 1;
      for (size_t k = 1; k < limit; ++k)
        {
          const Complex expected = std::conj(input[n - k]);
          ah_domain_error_if(std::abs(input[k] - expected)
                             > hermitian_tolerance(input[k], input[n - k], n))
            << ctx << ": spectrum is not Hermitian at k=" << k;
        }
    }

    /** @brief Internal generator for generalized cosine windows (Hann, Hamming, Blackman). */
    [[nodiscard]] static Array<Real>
    cosine_sum_window(const size_t n, const Real a0,
                      const Real a1 = Real(0),
                      const Real a2 = Real(0))
    {
      if (n == 0)
        return {};

      if (n == 1)
        return {Real(1)};

      Array<Real> window;
      window.reserve(n);
      const Real denom = static_cast<Real>(n - 1);
      for (size_t i = 0; i < n; ++i)
        {
          const Real phase = Real(2) * std::numbers::pi_v<Real>
                             * static_cast<Real>(i) / denom;
          const Real value = a0 - a1 * std::cos(phase)
                             + a2 * std::cos(Real(2) * phase);
          window.append(value);
        }

      return window;
    }

    /** @brief Functional reverse. */
    template <typename T>
    [[nodiscard]] static Array<T>
    reverse_copy(const Array<T> & input)
    {
      Array<T> output;
      output.reserve(input.size());
      for (size_t i = input.size(); i > 0; --i)
        output.append(input[i - 1]);
      return output;
    }

    /** @brief Functional prefix extraction. */
    template <typename T>
    [[nodiscard]] static Array<T>
    prefix_copy(const Array<T> & input, const size_t length)
    {
      ah_invalid_argument_if(length > input.size())
        << "FFT::prefix_copy: length " << length
        << " exceeds input size " << input.size();

      Array<T> output;
      output.reserve(length);
      for (size_t i = 0; i < length; ++i)
        output.append(input[i]);
      return output;
    }

    /** @brief Functional slice extraction. */
    template <typename T>
    [[nodiscard]] static Array<T>
    slice_copy(const Array<T> & input,
               const size_t offset,
               const size_t length)
    {
      ah_invalid_argument_if(offset > input.size())
        << "FFT::slice_copy: offset " << offset << " exceeds input size " << input.size();
      ah_invalid_argument_if(length > input.size() - offset)
        << "FFT::slice_copy: length " << length << " exceeds slice capacity " << (input.size() - offset);

      Array<T> output;
      output.reserve(length);
      for (size_t i = 0; i < length; ++i)
        output.append(input[offset + i]);
      return output;
    }

    /** @brief Rationale: Rule-of-thumb for filtfilt padding to allow transients to decay. */
    [[nodiscard]] static size_t
    default_filtfilt_pad_length(const size_t signal_size,
                                const size_t coeff_size) noexcept
    {
      if (signal_size <= 1 or coeff_size <= 1)
        return 0;

      const size_t taps_minus_one = coeff_size - 1;
      const size_t suggested = taps_minus_one
                               > std::numeric_limits<size_t>::max() / 3 ?
                                 std::numeric_limits<size_t>::max() :
                                 taps_minus_one * 3;
      return std::min(suggested, signal_size - 1);
    }

    /** @brief Rationale: Applies reflection padding at signal edges to reduce
     *         boundary artifacts during filtering.
     */
    [[nodiscard]] static Array<Real>
    reflect_pad_signal(const Array<Real> & signal, const size_t pad_len)
    {
      if (pad_len == 0)
        return signal;

      ah_invalid_argument_if(signal.size() < 2)
        << "FFT::reflect_pad_signal: signal size must be >= 2 when pad_len > 0";
      ah_invalid_argument_if(pad_len >= signal.size())
        << "FFT::reflect_pad_signal: pad length " << pad_len
        << " must be smaller than signal size " << signal.size();

      Array<Real> output;
      output.reserve(signal.size() + pad_len * 2);

      for (size_t i = 0; i < pad_len; ++i)
        output.append(signal[pad_len - i]);

      for (size_t i = 0; i < signal.size(); ++i)
        output.append(signal[i]);

      for (size_t i = 0; i < pad_len; ++i)
        output.append(signal[signal.size() - 2 - i]);

      return output;
    }

    /** @brief functional zero-padding at both ends. */
    [[nodiscard]] static Array<Real>
    zero_pad_edges(const Array<Real> & signal,
                   const size_t left_pad,
                   const size_t right_pad)
    {
      Array<Real> output;
      output.reserve(left_pad + signal.size() + right_pad);

      for (size_t i = 0; i < left_pad; ++i)
        output.append(Real(0));
      for (size_t i = 0; i < signal.size(); ++i)
        output.append(signal[i]);
      for (size_t i = 0; i < right_pad; ++i)
        output.append(Real(0));

      return output;
    }

    /** @brief Rationale: Computes the effective window product for a windowed
     *         overlap-add process to validate reconstruction constraints.
     */
    [[nodiscard]] static Array<Real>
    overlap_profile(const Array<Real> & analysis_window,
                    const Array<Real> & synthesis_window,
                    const size_t hop_size,
                    const char *ctx)
    {
      ah_invalid_argument_if(analysis_window.is_empty())
        << ctx << ": analysis window must be non-empty";
      ah_invalid_argument_if(synthesis_window.is_empty())
        << ctx << ": synthesis window must be non-empty";
      ah_invalid_argument_if(analysis_window.size() != synthesis_window.size())
        << ctx << ": analysis window size " << analysis_window.size()
        << " does not match synthesis window size "
        << synthesis_window.size();
      ah_invalid_argument_if(hop_size == 0) << ctx << ": hop size must be positive";

      Array<Real> profile = Array<Real>::create(hop_size);
      for (size_t i = 0; i < hop_size; ++i)
        profile(i) = Real(0);

      for (size_t i = 0; i < analysis_window.size(); ++i)
        profile(i % hop_size) += analysis_window[i] * synthesis_window[i];

      return profile;
    }

    /** @brief Rationale: Non-zero Overlap-Add (NOLA) ensures the signal can
     *         be reconstructed (it's never multiplied by zero in all frames).
     */
    [[nodiscard]] static bool
    overlap_profile_has_nola(const Array<Real> & profile) noexcept
    {
      const Real tol = (max_abs_value(profile) + Real(1))
                       * Real(128) * std::numeric_limits<Real>::epsilon();
      for (size_t i = 0; i < profile.size(); ++i)
        if (std::abs(profile[i]) <= tol)
          return false;
      return true;
    }

    /** @brief Rationale: Constant Overlap-Add (COLA) ensures that OLA
     *         reconstruction has no amplitude modulation artifacts.
     */
    [[nodiscard]] static bool
    overlap_profile_has_cola(const Array<Real> & profile) noexcept
    {
      if (profile.is_empty())
        return false;

      const Real mean = profile[0];
      const Real tol = (max_abs_value(profile) + Real(1))
                       * Real(256) * std::numeric_limits<Real>::epsilon();
      for (size_t i = 0; i < profile.size(); ++i)
        if (std::abs(profile[i] - mean) > tol)
          return false;
      return true;
    }

    /** @brief Rationale: Internal validator for STFT/ISTFT windowing parameters. */
    static void
    validate_overlap_constraints(const Array<Real> & analysis_window,
                                 const Array<Real> & synthesis_window,
                                 const size_t hop_size,
                                 const bool validate_nola,
                                 const bool validate_cola,
                                 const char *ctx)
    {
      if (not validate_nola and not validate_cola)
        return;

      const Array<Real> profile = overlap_profile(analysis_window,
                                                  synthesis_window,
                                                  hop_size,
                                                  ctx);

      if (validate_nola)
        ah_domain_error_if(not overlap_profile_has_nola(profile))
          << ctx << ": window pair does not satisfy NOLA for hop size "
          << hop_size;

      if (validate_cola)
        ah_domain_error_if(not overlap_profile_has_cola(profile))
          << ctx << ": window pair does not satisfy COLA for hop size "
          << hop_size;
    }

    /** @brief Computes framing offsets. */
    [[nodiscard]] static Array<size_t>
    frame_offsets_impl(const size_t signal_size,
                       const size_t frame_size,
                       const size_t hop_size,
                       const bool pad_end,
                       const char *ctx)
    {
      ah_invalid_argument_if(frame_size == 0) << ctx << ": frame size must be positive";
      ah_invalid_argument_if(hop_size == 0) << ctx << ": hop size must be positive";

      Array<size_t> offsets;
      if (signal_size == 0)
        return offsets;

      for (size_t offset = 0; offset < signal_size; offset += hop_size)
        {
          if (const size_t length = std::min(frame_size, signal_size - offset); not pad_end and length < frame_size)
            break;

          offsets.append(offset);

          if (not pad_end and offset + frame_size >= signal_size)
            break;
        }

      return offsets;
    }

    /** @brief L-infinity norm. */
    [[nodiscard]] static Real
    max_abs_value(const Array<Real> & input) noexcept
    {
      Real max_value = Real(0);
      for (size_t i = 0; i < input.size(); ++i)
        max_value = std::max(max_value, std::abs(input[i]));
      return max_value;
    }

    /** @brief Sum reduction. */
    [[nodiscard]] static Real
    sum_values(const Array<Real> & input) noexcept
    {
      Real total = Real(0);
      for (size_t i = 0; i < input.size(); ++i)
        total += input[i];
      return total;
    }

    /** @brief L2 norm squared. */
    [[nodiscard]] static Real
    sum_squares(const Array<Real> & input) noexcept
    {
      Real total = Real(0);
      for (size_t i = 0; i < input.size(); ++i)
        total += input[i] * input[i];
      return total;
    }

    /** @brief Returns a copy with DC offset removed. */
    [[nodiscard]] static Array<Real>
    demean_copy(const Array<Real> & input)
    {
      if (input.is_empty())
        return {};

      const Real mean = sum_values(input) / static_cast<Real>(input.size());
      Array<Real> output = Array<Real>::create(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output(i) = input[i] - mean;
      return output;
    }

    /** @brief Maps bin indices to frequency in Hz. */
    [[nodiscard]] static Array<Real>
    one_sided_frequency_grid(const size_t fft_size,
                             const Real sample_rate,
                             const char *ctx)
    {
      ah_invalid_argument_if(fft_size == 0) << ctx << ": FFT size must be positive";
      ah_invalid_argument_if(sample_rate <= Real(0))
        << ctx << ": sample rate " << sample_rate << " must be strictly positive";

      Array<Real> frequency = Array<Real>::create(fft_size / 2 + 1);
      for (size_t k = 0; k < frequency.size(); ++k)
        frequency(k) = static_cast<Real>(k) * sample_rate
                       / static_cast<Real>(fft_size);
      return frequency;
    }

    /** @brief Identifies non-redundant spectrum bins (excluding DC and Nyquist). */
    [[nodiscard]] static bool
    is_one_sided_interior_bin(const size_t bin,
                              const size_t fft_size) noexcept
    {
      if (bin == 0)
        return false;
      if (fft_size % 2 == 0 and bin == fft_size / 2)
        return false;
      return true;
    }

    /** @brief Default hop size for Welch analysis (50% overlap). */
    [[nodiscard]] static size_t
    resolve_welch_hop_size(const WelchOptions & options,
                           const size_t frame_size,
                           const char *ctx)
    {
      const size_t hop_size =
          options.hop_size == 0 ? std::max(static_cast<size_t>(1), frame_size / 2) : options.hop_size;
      ah_invalid_argument_if(hop_size == 0) << ctx << ": hop size must be positive";
      return hop_size;
    }

    /** @brief Default FFT size for Welch analysis (zero-padded to power of 2). */
    [[nodiscard]] static size_t
    resolve_welch_fft_size(const WelchOptions & options,
                           const size_t frame_size,
                           const char *ctx)
    {
      const size_t fft_size =
          options.fft_size == 0 ? next_power_of_two(frame_size) : options.fft_size;
      ah_invalid_argument_if(fft_size < frame_size)
        << ctx << ": FFT size " << fft_size << " is smaller than frame size " << frame_size;
      return fft_size;
    }

    /** @brief Rationale: Encapsulates frame extraction, detrending and windowing
     *         common to Welch and other spectral estimators.
     */
    [[nodiscard]] static Array<Array<Real>>
    prepare_welch_frames(const Array<Real> & signal,
                         const Array<Real> & window,
                         const WelchOptions & options,
                         const char *ctx)
    {
      ah_invalid_argument_if(signal.is_empty()) << ctx << ": signal must be non-empty";
      ah_invalid_argument_if(window.is_empty()) << ctx << ": window must be non-empty";

      const size_t hop_size = resolve_welch_hop_size(options, window.size(), ctx);
      Array<Array<Real>> frames =
          frame_signal(signal, window.size(), hop_size, options.pad_end);
      ah_invalid_argument_if(frames.is_empty())
        << ctx << ": framing produced no analysis frames";

      for (size_t i = 0; i < frames.size(); ++i)
        {
          frames(i) = options.detrend ? demean_copy(frames[i]) : frames[i];
          frames(i) = apply_window(frames[i], window);
        }
      return frames;
    }

    /** @brief Rationale: Multiplies spectral densities by 2 for interior bins
     *         to account for energy in the omitted negative frequency half.
     */
    template <typename Density>
    static void
    apply_one_sided_density_scaling(Density & value,
                                    const size_t bin,
                                    const size_t fft_size) noexcept
    {
      if (is_one_sided_interior_bin(bin, fft_size))
        value *= Real(2);
    }

    /** @brief Returns length excluding trailing insignificant zeros. */
    [[nodiscard]] static size_t
    effective_coeff_length(const Array<Real> & input) noexcept
    {
      if (input.is_empty())
        return 0;

      const Real tol = (max_abs_value(input) + Real(1))
                       * Real(64) * std::numeric_limits<Real>::epsilon();
      size_t n = input.size();
      while (n > 1 and std::abs(input[n - 1]) <= tol)
        --n;
      return n;
    }

    /** @brief Scalar scaling. */
    [[nodiscard]] static Array<Real>
    scaled_copy(const Array<Real> & input, const Real factor)
    {
      Array<Real> output = Array<Real>::create(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output(i) = input[i] * factor;
      return output;
    }

    struct BalancedPolynomial
    {
      Array<Real> monic;
      Array<Real> balanced;
      Real root_scale = Real(1);
      Real tol = Real(0);
      Real residual_tol = Real(0);
    };

    struct SeriesEvaluation
    {
      Complex value = Complex(Real(0), Real(0));
      Complex derivative = Complex(Real(0), Real(0));
      Real coefficient_scale = Real(1);
    };

    struct PolynomialEvaluation
    {
      Complex value = Complex(Real(0), Real(0));
      Complex derivative = Complex(Real(0), Real(0));
    };

    [[nodiscard]] static Real
    scaled_tolerance(const Real reference,
                     const Real multiplier = Real(256)) noexcept
    {
      return (std::abs(reference) + Real(1))
             * multiplier * std::numeric_limits<Real>::epsilon();
    }

    [[nodiscard]] static Real
    series_zero_tolerance(const size_t coeff_count,
                          const Real multiplier = Real(256)) noexcept
    {
      return multiplier
             * static_cast<Real>(coeff_count + 1)
             * std::numeric_limits<Real>::epsilon();
    }

    [[nodiscard]] static IIRCoefficients
    normalize_iir_coefficients(const Array<Real> & numerator,
                               const Array<Real> & denominator,
                               const char *ctx)
    {
      const size_t num_length = effective_coeff_length(numerator);
      const size_t den_length = effective_coeff_length(denominator);

      ah_invalid_argument_if(num_length == 0)
        << ctx << ": numerator must be non-empty";
      ah_invalid_argument_if(den_length == 0)
        << ctx << ": denominator must be non-empty";

      const Real a0 = denominator[0];
      const Real tol = (max_abs_value(denominator) + Real(1))
                       * Real(64) * std::numeric_limits<Real>::epsilon();
      ah_domain_error_if(std::abs(a0) <= tol)
        << ctx << ": leading denominator coefficient must be non-zero";

      const size_t order = std::max(num_length, den_length) - 1;
      IIRCoefficients coeffs;
      coeffs.numerator = Array<Real>::create(order + 1);
      coeffs.denominator = Array<Real>::create(order + 1);

      for (size_t i = 0; i <= order; ++i)
        {
          coeffs.numerator(i) = Real(0);
          coeffs.denominator(i) = Real(0);
        }

      for (size_t i = 0; i < num_length; ++i)
        coeffs.numerator(i) = numerator[i] / a0;
      for (size_t i = 0; i < den_length; ++i)
        coeffs.denominator(i) = denominator[i] / a0;
      coeffs.denominator(0) = Real(1);

      return coeffs;
    }

    [[nodiscard]] static Array<Real>
    solve_dense_system(Array<Real> matrix,
                       Array<Real> rhs,
                       const size_t n,
                       const char *ctx)
    {
      if (n == 0)
        return {};

      ah_invalid_argument_if(matrix.size() != n * n)
        << ctx << ": matrix size " << matrix.size()
        << " does not match a dense " << n << "x" << n << " system";
      ah_invalid_argument_if(rhs.size() != n)
        << ctx << ": rhs size " << rhs.size() << " does not match system size " << n;

      Real max_entry = Real(0);
      for (size_t i = 0; i < matrix.size(); ++i)
        max_entry = std::max(max_entry, std::abs(matrix[i]));
      for (size_t i = 0; i < rhs.size(); ++i)
        max_entry = std::max(max_entry, std::abs(rhs[i]));

      const Real pivot_tol = (max_entry + Real(1))
                             * Real(128) * std::numeric_limits<Real>::epsilon();

      auto coeff = [&matrix, n](const size_t row, const size_t col) -> Real &
        {
          return matrix(row * n + col);
        };

      for (size_t col = 0; col < n; ++col)
        {
          size_t pivot_row = col;
          Real pivot_abs = std::abs(coeff(col, col));
          for (size_t row = col + 1; row < n; ++row)
            {
              const Real candidate = std::abs(coeff(row, col));
              if (candidate > pivot_abs)
                {
                  pivot_abs = candidate;
                  pivot_row = row;
                }
            }

          ah_domain_error_if(pivot_abs <= pivot_tol)
            << ctx << ": singular or ill-conditioned linear system";

          if (pivot_row != col)
            {
              for (size_t k = col; k < n; ++k)
                std::swap(coeff(col, k), coeff(pivot_row, k));
              std::swap(rhs(col), rhs(pivot_row));
            }

          const Real pivot = coeff(col, col);
          for (size_t row = col + 1; row < n; ++row)
            {
              const Real factor = coeff(row, col) / pivot;
              if (std::abs(factor) <= pivot_tol)
                {
                  coeff(row, col) = Real(0);
                  continue;
                }

              coeff(row, col) = Real(0);
              for (size_t k = col + 1; k < n; ++k)
                coeff(row, k) -= factor * coeff(col, k);
              rhs(row) -= factor * rhs[col];
            }
        }

      Array<Real> solution = Array<Real>::create(n);
      for (size_t i = 0; i < n; ++i)
        solution(i) = Real(0);

      for (size_t row = n; row > 0; --row)
        {
          const size_t i = row - 1;
          Real sum = rhs[i];
          for (size_t col = i + 1; col < n; ++col)
            sum -= coeff(i, col) * solution[col];

          const Real pivot = coeff(i, i);
          ah_domain_error_if(std::abs(pivot) <= pivot_tol)
            << ctx << ": singular or ill-conditioned linear system";
          solution(i) = sum / pivot;
        }

      return solution;
    }

    [[nodiscard]] static Array<Real>
    iir_steady_state(const Array<Real> & numerator,
                     const Array<Real> & denominator,
                     const char *ctx)
    {
      ah_invalid_argument_if(numerator.size() != denominator.size())
        << ctx << ": numerator size " << numerator.size()
        << " does not match denominator size " << denominator.size();

      const size_t order = denominator.size() - 1;
      if (order == 0)
        return {};

      Array<Real> system = Array<Real>::create(order * order);
      for (size_t i = 0; i < system.size(); ++i)
        system(i) = Real(0);

      auto coeff = [&system, order](const size_t row, const size_t col) -> Real &
        {
          return system(row * order + col);
        };

      for (size_t i = 0; i < order; ++i)
        coeff(i, i) = Real(1);

      for (size_t i = 0; i < order; ++i)
        {
          coeff(i, 0) += denominator[i + 1];
          if (i + 1 < order)
            coeff(i, i + 1) -= Real(1);
        }

      Array<Real> rhs = Array<Real>::create(order);
      for (size_t i = 0; i < order; ++i)
        rhs(i) = numerator[i + 1] - denominator[i + 1] * numerator[0];

      return solve_dense_system(system, rhs, order, ctx);
    }

    [[nodiscard]] static Array<Real>
    iir_filter_impl(const Array<Real> & signal,
                    const Array<Real> & numerator,
                    const Array<Real> & denominator,
                    const Array<Real> & initial_state,
                    const char *ctx,
                    Array<Real> *final_state = nullptr)
    {
      ah_invalid_argument_if(numerator.size() != denominator.size())
        << ctx << ": numerator size " << numerator.size()
        << " does not match denominator size " << denominator.size();

      const size_t order = denominator.size() - 1;
      ah_invalid_argument_if(not initial_state.is_empty()
                             and initial_state.size() != order)
        << ctx << ": initial state size " << initial_state.size()
        << " does not match filter order " << order;

      if (signal.is_empty())
        return {};

      Array<Real> output = Array<Real>::create(signal.size());
      if (order == 0)
        {
          for (size_t i = 0; i < signal.size(); ++i)
            output(i) = numerator[0] * signal[i];
          return output;
        }

      Array<Real> state = Array<Real>::create(order);
      for (size_t i = 0; i < order; ++i)
        state(i) = initial_state.is_empty() ? Real(0) : initial_state[i];

      for (size_t n = 0; n < signal.size(); ++n)
        {
          const Real x = signal[n];
          const Real y = numerator[0] * x + state[0];
          output(n) = y;

          for (size_t i = 0; i + 1 < order; ++i)
            state(i) = state[i + 1] + numerator[i + 1] * x
                       - denominator[i + 1] * y;
          state(order - 1) = numerator[order] * x - denominator[order] * y;
        }

      if (final_state != nullptr)
        *final_state = state;

      return output;
    }

    [[nodiscard]] static Array<Real>
    iir_filtfilt_impl(const Array<Real> & signal,
                      const Array<Real> & numerator,
                      const Array<Real> & denominator,
                      const char *ctx)
    {
      if (signal.is_empty() or numerator.is_empty() or denominator.is_empty())
        return {};

      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, ctx);

      if (const size_t order = coeffs.denominator.size() - 1; order == 0)
        {
          Array<Real> output = Array<Real>::create(signal.size());
          const Real gain = coeffs.numerator[0] * coeffs.numerator[0];
          for (size_t i = 0; i < signal.size(); ++i)
            output(i) = signal[i] * gain;
          return output;
        }

      const size_t pad_len = default_filtfilt_pad_length(signal.size(),
                                                         coeffs.denominator.size());
      const Array<Real> padded = reflect_pad_signal(signal, pad_len);
      const Array<Real> zi = iir_steady_state(coeffs.numerator,
                                              coeffs.denominator,
                                              ctx);

      const Array<Real> forward = iir_filter_impl(padded,
                                                  coeffs.numerator,
                                                  coeffs.denominator,
                                                  scaled_copy(zi, padded[0]),
                                                  ctx);
      const Array<Real> reversed = reverse_copy(forward);
      const Array<Real> backward = iir_filter_impl(reversed,
                                                   coeffs.numerator,
                                                   coeffs.denominator,
                                                   scaled_copy(zi, reversed[0]),
                                                   ctx);
      const Array<Real> zero_phase = reverse_copy(backward);

      return slice_copy(zero_phase, pad_len, signal.size());
    }

    [[nodiscard]] static Array<Real>
    sos_filtfilt_impl(const Array<Real> & signal,
                      const Array<BiquadSection> & sections,
                      const char *ctx)
    {
      if (signal.is_empty() or sections.is_empty())
        return {};

      Array<IIRCoefficients> normalized_sections;
      normalized_sections.reserve(sections.size());
      size_t total_order = 0;
      for (size_t i = 0; i < sections.size(); ++i)
        {
          const IIRCoefficients coeffs =
              normalize_iir_coefficients(sections[i].numerator(),
                                         sections[i].denominator(),
                                         ctx);
          total_order += coeffs.denominator.size() - 1;
          normalized_sections.append(coeffs);
        }

      if (total_order == 0)
        {
          Array<Real> output = signal;
          for (size_t i = 0; i < normalized_sections.size(); ++i)
            {
              const Real gain = normalized_sections[i].numerator[0]
                                * normalized_sections[i].numerator[0];
              for (size_t j = 0; j < output.size(); ++j)
                output(j) *= gain;
            }
          return output;
        }

      const size_t pad_len = default_filtfilt_pad_length(signal.size(),
                                                         total_order + 1);
      Array<Real> stage = reflect_pad_signal(signal, pad_len);

      for (size_t i = 0; i < normalized_sections.size(); ++i)
        {
          const auto & coeffs = normalized_sections[i];
          const Array<Real> zi = iir_steady_state(coeffs.numerator,
                                                  coeffs.denominator,
                                                  ctx);
          stage = iir_filter_impl(stage,
                                  coeffs.numerator,
                                  coeffs.denominator,
                                  scaled_copy(zi, stage[0]),
                                  ctx);
        }

      stage = reverse_copy(stage);

      for (size_t i = 0; i < normalized_sections.size(); ++i)
        {
          const auto & coeffs = normalized_sections[i];
          const Array<Real> zi = iir_steady_state(coeffs.numerator,
                                                  coeffs.denominator,
                                                  ctx);
          stage = iir_filter_impl(stage,
                                  coeffs.numerator,
                                  coeffs.denominator,
                                  scaled_copy(zi, stage[0]),
                                  ctx);
        }

      return slice_copy(reverse_copy(stage), pad_len, signal.size());
    }

    template <typename T>
    static void
    append_all(Array<T> & dst, const Array<T> & src)
    {
      dst.reserve(dst.size() + src.size());
      for (size_t i = 0; i < src.size(); ++i)
        dst.append(src[i]);
    }

    template <typename T>
    static void
    drop_prefix(Array<T> & input, const size_t count)
    {
      ah_invalid_argument_if(count > input.size())
        << "FFT::drop_prefix: count " << count << " exceeds input size " << input.size();

      if (count == 0)
        return;

      if (count == input.size())
        {
          input.empty();
          return;
        }

      Array<T> trimmed;
      trimmed.reserve(input.size() - count);
      for (size_t i = count; i < input.size(); ++i)
        trimmed.append(input[i]);
      input = std::move(trimmed);
    }

    [[nodiscard]] static constexpr size_t
    saturating_product(const size_t lhs, const size_t rhs) noexcept
    {
      return lhs == 0 or rhs == 0 ?
               0 :
               lhs > std::numeric_limits<size_t>::max() / rhs ?
                 std::numeric_limits<size_t>::max() :
                 lhs * rhs;
    }

    [[nodiscard]] static bool
    should_parallelize_batch_work(ThreadPool *pool,
                                  const size_t batch_size,
                                  const size_t transform_size,
                                  const size_t min_work = 8192) noexcept
    {
      return pool != nullptr
             and pool->num_threads() > 1
             and batch_size > 1
             and saturating_product(batch_size, transform_size) >= min_work;
    }

    [[nodiscard]] static size_t
    count_non_empty_real_batch(const Array<Array<Real>> & batch) noexcept
    {
      size_t count = 0;
      for (size_t i = 0; i < batch.size(); ++i)
        if (not batch[i].is_empty())
          ++count;
      return count;
    }

    [[nodiscard]] static size_t
    recommended_cache_tile_size(const size_t transform_size,
                                const size_t batch_size,
                                ThreadPool *pool) noexcept
    {
      constexpr size_t target_bytes = 32 * 1024;
      const size_t bytes_per_item = std::max(sizeof(Complex), sizeof(Real))
                                    * std::max<size_t>(1, transform_size);
      size_t tile = std::max<size_t>(1, target_bytes / bytes_per_item);
      tile = std::max<size_t>(4, tile);
      tile = std::min(tile, std::max<size_t>(static_cast<size_t>(1), batch_size));
      if (pool != nullptr and pool->num_threads() > 1)
        tile = std::max(tile, pool->num_threads() * 2);
      return std::min(tile, std::max<size_t>(static_cast<size_t>(1), batch_size));
    }

    [[nodiscard]] static Array<Real>
    build_padded_frame_from_prefix(const Array<Real> & input,
                                   const size_t frame_size)
    {
      Array<Real> frame = Array<Real>::create(frame_size);
      for (size_t i = 0; i < frame_size; ++i)
        frame(i) = Real(0);

      const size_t length = std::min(frame_size, input.size());
      for (size_t i = 0; i < length; ++i)
        frame(i) = input[i];

      return frame;
    }

    [[nodiscard]] static size_t
    validate_stft_options(const Array<Real> & window,
                          const STFTOptions & options,
                          const char *ctx)
    {
      ah_invalid_argument_if(window.is_empty()) << ctx << ": window must be non-empty";
      ah_invalid_argument_if(options.hop_size == 0) << ctx << ": hop size must be positive";

      const size_t fft_size = options.fft_size == 0 ? next_power_of_two(window.size()) : options.fft_size;
      ah_invalid_argument_if(fft_size < window.size())
        << ctx << ": FFT size " << fft_size << " is smaller than window size " << window.size();

      validate_overlap_constraints(window,
                                   window,
                                   options.hop_size,
                                   options.validate_nola,
                                   options.validate_cola,
                                   ctx);
      return fft_size;
    }

    [[nodiscard]] static Array<Complex>
    prepare_stft_frame_input(const Array<Real> & frame,
                             const Array<Real> & window,
                             const size_t fft_size)
    {
      return lift_real_input(zero_padded_copy(apply_window(frame, window), fft_size));
    }

    [[nodiscard]] static Array<Array<Complex>>
    transform_stft_frames(const Array<Array<Real>> & frames,
                          const Array<Real> & window,
                          const size_t fft_size,
                          const Plan & plan,
                          ThreadPool *pool,
                          const size_t chunk_size)
    {
      if (frames.is_empty())
        return {};

      const bool prefer_batch_simd =
          pool == nullptr or pool->num_threads() <= 1 or fft_size >= 256;
      const size_t tile_size =
          recommended_cache_tile_size(fft_size, frames.size(), pool);

      Array<Array<Complex>> output(frames.size(), Array<Complex>());
      for (size_t offset = 0; offset < frames.size(); offset += tile_size)
        {
          const size_t count = std::min(tile_size, frames.size() - offset);
          Array<Array<Complex>> tile(count, Array<Complex>());
          const bool parallelize_preparation =
              should_parallelize_batch_work(pool, count, fft_size);

          auto prepare_one = [&frames, &window, fft_size, offset, &tile](const size_t i)
            {
              tile(i) = prepare_stft_frame_input(frames[offset + i], window, fft_size);
            };

          if (parallelize_preparation)
            parallel_for_index(*pool, 0, count, prepare_one, chunk_size);
          else
            for (size_t i = 0; i < count; ++i)
              prepare_one(i);

          const bool parallelize_transforms =
              should_parallelize_batch_work(pool, count, fft_size);
          if (parallelize_transforms)
            plan.ptransform_batch(*pool, tile, false, chunk_size, prefer_batch_simd);
          else
            plan.transform_batch(tile, false, prefer_batch_simd);

          for (size_t i = 0; i < count; ++i)
            output(offset + i) = std::move(tile(i));
        }

      return output;
    }

    [[nodiscard]] static Array<Array<Complex>>
    stft_impl(const Array<Real> & signal,
              const Array<Real> & window,
              const STFTOptions & options,
              ThreadPool *pool = nullptr,
              const size_t chunk_size = 0)
    {
      if (signal.is_empty())
        return {};

      const size_t fft_size =
          validate_stft_options(window, options, "FFT::stft");

      const size_t center_pad = options.centered ? window.size() / 2 : 0;
      const Array<Real> prepared_signal = options.centered ? zero_pad_edges(signal, center_pad, center_pad) : signal;
      const auto frames = frame_signal(prepared_signal,
                                       window.size(),
                                       options.hop_size,
                                       options.pad_end);

      if (frames.is_empty())
        return {};

      const Plan plan(fft_size);
      return transform_stft_frames(frames,
                                   window,
                                   fft_size,
                                   plan,
                                   pool,
                                   chunk_size);
    }

    [[nodiscard]] static Complex
    evaluate_transfer_at(const Array<Real> & numerator,
                         const Array<Real> & denominator,
                         const Real omega,
                         const char *ctx)
    {
      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, ctx);
      const SeriesEvaluation numerator_terms =
          evaluate_series_at_unit_circle(coeffs.numerator, omega, ctx);
      const SeriesEvaluation denominator_terms =
          evaluate_series_at_unit_circle(coeffs.denominator, omega, ctx);
      const Real den_tol =
          series_zero_tolerance(coeffs.denominator.size(), Real(512));
      ah_domain_error_if(std::abs(denominator_terms.value) <= den_tol)
        << ctx << ": transfer response is singular at omega=" << omega;

      return (numerator_terms.value * numerator_terms.coefficient_scale)
             / (denominator_terms.value * denominator_terms.coefficient_scale);
    }

    [[nodiscard]] static FrequencyResponse
    freqz_impl(const Array<Real> & numerator,
               const Array<Real> & denominator,
               const size_t num_points,
               const bool whole,
               const char *ctx)
    {
      ah_invalid_argument_if(num_points == 0)
        << ctx << ": number of frequency samples must be positive";

      FrequencyResponse output;
      output.omega = Array<Real>::create(num_points);
      output.response = Array<Complex>::create(num_points);

      const Real max_omega = whole ? Real(2) * std::numbers::pi_v<Real> : std::numbers::pi_v<Real>;
      const Real denom = whole ? static_cast<Real>(num_points) : static_cast<Real>(std::max(size_t(1), num_points - 1));

      for (size_t i = 0; i < num_points; ++i)
        {
          const Real omega = num_points == 1 ? Real(0) : max_omega * static_cast<Real>(i) / denom;
          output.omega(i) = omega;
          output.response(i) = evaluate_transfer_at(numerator,
                                                    denominator,
                                                    omega,
                                                    ctx);
        }

      return output;
    }

    [[nodiscard]] static SeriesEvaluation
    evaluate_series_at_unit_circle(const Array<Real> & coefficients,
                                   const Real omega,
                                   const char *ctx)
    {
      const size_t n = effective_coeff_length(coefficients);
      ah_invalid_argument_if(n == 0)
        << ctx << ": coefficient series must be non-empty";

      const Real scale = max_abs_value(coefficients);
      const Real inv_scale = scale > std::numeric_limits<Real>::min() ? Real(1) / scale : Real(1);
      const Complex z = std::polar(Real(1), -omega);
      Complex value(coefficients[n - 1] * inv_scale, Real(0));
      Complex derivative_poly(Real(0), Real(0));

      for (size_t idx = n - 1; idx-- > 0;)
        {
          derivative_poly = derivative_poly * z + value;
          value = value * z + Complex(coefficients[idx] * inv_scale, Real(0));
        }

      SeriesEvaluation output;
      output.value = value;
      output.derivative = Complex(Real(0), Real(-1)) * z * derivative_poly;
      output.coefficient_scale = scale > std::numeric_limits<Real>::min() ? scale : Real(1);
      return output;
    }

    [[nodiscard]] static PolynomialEvaluation
    evaluate_polynomial_and_derivative(const Array<Real> & coefficients,
                                       const Complex & x,
                                       const char *ctx)
    {
      const size_t n = effective_coeff_length(coefficients);
      ah_invalid_argument_if(n == 0) << ctx << ": polynomial coefficients must be non-empty";

      PolynomialEvaluation output;
      output.value = Complex(coefficients[0], Real(0));
      for (size_t i = 1; i < n; ++i)
        {
          output.derivative = output.derivative * x + output.value;
          output.value = output.value * x + Complex(coefficients[i], Real(0));
        }
      return output;
    }

    [[nodiscard]] static std::pair<Array<Real>, Real>
    divide_polynomial_by_linear_root(const Array<Real> & coefficients,
                                     const Real root,
                                     const char *ctx)
    {
      const size_t n = effective_coeff_length(coefficients);
      ah_invalid_argument_if(n <= 1) << ctx << ": polynomial degree must be at least one";

      Array<Real> quotient = Array<Real>::create(n - 1);
      quotient(0) = coefficients[0];
      for (size_t i = 1; i + 1 < n; ++i)
        quotient(i) = coefficients[i] + root * quotient[i - 1];

      const Real remainder = coefficients[n - 1] + root * quotient[n - 2];
      return {std::move(quotient), remainder};
    }

    [[nodiscard]] static std::pair<Array<Real>, Array<Complex>>
    extract_repeated_unit_roots(const Array<Real> & coefficients,
                                const char *ctx)
    {
      Array<Real> reduced = trim_leading_zeros_copy(coefficients);
      Array<Complex> extracted;
      bool changed = true;
      while (changed and effective_coeff_length(reduced) > 1)
        {
          changed = false;
          const Real coeff_tol =
              scaled_tolerance(max_abs_value(reduced), Real(4096));

          for (const Real root: {Real(1), Real(-1)})
            {
              const PolynomialEvaluation eval =
                  evaluate_polynomial_and_derivative(reduced,
                                                     Complex(root, Real(0)),
                                                     ctx);
              if (std::abs(eval.value) > coeff_tol)
                continue;

              const auto [quotient, remainder] =
                  divide_polynomial_by_linear_root(reduced, root, ctx);
              if (std::abs(remainder) > coeff_tol)
                continue;

              extracted.append(Complex(root, Real(0)));
              reduced = trim_leading_zeros_copy(quotient);
              changed = true;
              break;
            }
        }

      return {std::move(reduced), std::move(extracted)};
    }

    [[nodiscard]] static Real
    polynomial_root_upper_bound(const Array<Real> & monic) noexcept
    {
      Real bound = Real(1);
      const Real leading = std::max(std::abs(monic[0]),
                                    std::numeric_limits<Real>::min());
      for (size_t i = 1; i < monic.size(); ++i)
        bound = std::max(bound, Real(1) + std::abs(monic[i]) / leading);
      return bound;
    }

    [[nodiscard]] static Real
    polynomial_root_lower_bound(const Array<Real> & monic) noexcept
    {
      if (monic.size() <= 1)
        return Real(0);

      const Real constant = std::abs(monic[monic.size() - 1]);
      if (constant <= scaled_tolerance(max_abs_value(monic), Real(1024)))
        return Real(0);

      Real inverse_bound = Real(1);
      for (size_t i = 0; i + 1 < monic.size(); ++i)
        inverse_bound = std::max(inverse_bound,
                                 Real(1) + std::abs(monic[i]) / constant);
      return Real(1) / inverse_bound;
    }

    [[nodiscard]] static BalancedPolynomial
    balance_polynomial_for_roots(const Array<Real> & coefficients,
                                 const char *ctx)
    {
      const size_t n = effective_coeff_length(coefficients);
      ah_invalid_argument_if(n <= 1)
        << ctx << ": polynomial degree must be at least one";

      const Real leading = coefficients[0];
      const Real coeff_max = max_abs_value(coefficients);
      const Real tol = scaled_tolerance(coeff_max, Real(2048));
      ah_domain_error_if(std::abs(leading) <= tol)
        << ctx << ": leading polynomial coefficient must be non-zero";

      BalancedPolynomial output;
      output.monic = Array<Real>::create(n);
      for (size_t i = 0; i < n; ++i)
        output.monic(i) = coefficients[i] / leading;

      const Real upper = polynomial_root_upper_bound(output.monic);
      const Real lower = polynomial_root_lower_bound(output.monic);
      Real root_scale = Real(1);
      if (lower > tol and std::isfinite(lower) and std::isfinite(upper))
        root_scale = std::sqrt(lower * upper);
      else if (upper > Real(1) and std::isfinite(upper))
        root_scale = upper;

      if (not std::isfinite(root_scale)
          or root_scale <= std::numeric_limits<Real>::min())
        root_scale = Real(1);

      output.balanced = Array<Real>::create(n);
      output.balanced(0) = Real(1);
      Real inv_power = Real(1);
      const Real inv_scale = Real(1) / root_scale;
      for (size_t i = 1; i < n; ++i)
        {
          inv_power *= inv_scale;
          output.balanced(i) = output.monic[i] * inv_power;
        }

      output.root_scale = root_scale;
      output.tol = scaled_tolerance(max_abs_value(output.balanced), Real(4096));
      output.residual_tol =
          scaled_tolerance(max_abs_value(output.monic), Real(8192));
      return output;
    }

    [[nodiscard]] static Array<Complex>
    initialize_root_guesses(const Array<Real> & coefficients)
    {
      const size_t degree = effective_coeff_length(coefficients) - 1;
      Array<Complex> roots = Array<Complex>::create(degree);

      const Real upper = polynomial_root_upper_bound(coefficients);
      const Real lower = polynomial_root_lower_bound(coefficients);
      Real base_radius = Real(1);
      if (lower > Real(0) and std::isfinite(lower) and std::isfinite(upper))
        base_radius = std::sqrt(lower * upper);
      else if (std::isfinite(upper) and upper > Real(0))
        base_radius = upper;

      if (not std::isfinite(base_radius)
          or base_radius <= std::numeric_limits<Real>::min())
        base_radius = Real(1);

      for (size_t i = 0; i < degree; ++i)
        {
          const Real angle =
              Real(2) * std::numbers::pi_v<Real>
              * (static_cast<Real>(i) + Real(0.5))
              / static_cast<Real>(degree);
          const Real radius =
              base_radius
              * (Real(0.9)
                 + Real(0.2) * static_cast<Real>(i + 1)
                 / static_cast<Real>(degree + 1));
          roots(i) = std::polar(radius, angle);
        }

      return roots;
    }

    static bool
    try_aberth_ehrlich_roots(const Array<Real> & coefficients,
                             const Real tol,
                             Array<Complex> & roots,
                             const size_t max_iterations = 192) noexcept
    {
      const size_t degree = roots.size();
      for (size_t iter = 0; iter < max_iterations; ++iter)
        {
          bool converged = true;
          for (size_t i = 0; i < degree; ++i)
            {
              const PolynomialEvaluation eval =
                  evaluate_polynomial_and_derivative(coefficients,
                                                     roots[i],
                                                     "FFT::root_solver");
              Complex correction(Real(0), Real(0));
              for (size_t j = 0; j < degree; ++j)
                if (j != i)
                  {
                    const Complex delta = roots[i] - roots[j];
                    if (std::abs(delta) > tol)
                      correction += Complex(Real(1), Real(0)) / delta;
                  }

              Complex denom = eval.derivative - eval.value * correction;
              if (std::abs(denom) <= tol)
                {
                  if (std::abs(eval.derivative) <= tol)
                    {
                      const Real nudge = tol * (Real(4) + std::abs(roots[i]));
                      roots(i) += std::polar(nudge,
                                             Real(2)
                                             * std::numbers::pi_v<Real>
                                             * static_cast<Real>(iter + i + 1)
                                             / static_cast<Real>(degree + 1));
                      converged = false;
                      continue;
                    }
                  denom = eval.derivative;
                }

              const Complex step = eval.value / denom;
              roots(i) -= step;
              if (std::abs(step) > tol * (Real(1) + std::abs(roots[i])))
                converged = false;
            }

          if (converged)
            return true;
        }

      return false;
    }

    static bool
    try_durand_kerner_roots(const Array<Real> & coefficients,
                            const Real tol,
                            Array<Complex> & roots,
                            const size_t max_iterations = 192) noexcept
    {
      const size_t degree = roots.size();
      for (size_t iter = 0; iter < max_iterations; ++iter)
        {
          bool converged = true;
          for (size_t i = 0; i < degree; ++i)
            {
              const PolynomialEvaluation eval =
                  evaluate_polynomial_and_derivative(coefficients,
                                                     roots[i],
                                                     "FFT::root_solver");
              Complex denom(Real(1), Real(0));
              for (size_t j = 0; j < degree; ++j)
                if (j != i)
                  {
                    const Complex delta = roots[i] - roots[j];
                    if (std::abs(delta) <= tol)
                      {
                        const Real nudge = tol * (Real(4) + std::abs(roots[i]));
                        roots(i) += std::polar(nudge,
                                               Real(2)
                                               * std::numbers::pi_v<Real>
                                               * static_cast<Real>(iter + i + 1)
                                               / static_cast<Real>(degree + 1));
                        denom = Complex(Real(0), Real(0));
                        break;
                      }
                    denom *= delta;
                  }

              if (std::abs(denom) <= tol)
                {
                  converged = false;
                  continue;
                }

              const Complex step = eval.value / denom;
              roots(i) -= step;
              if (std::abs(step) > tol * (Real(1) + std::abs(roots[i])))
                converged = false;
            }

          if (converged)
            return true;
        }

      return false;
    }

    static void
    polish_roots_with_newton(const Array<Real> & coefficients,
                             Array<Complex> & roots,
                             const Real tol,
                             const size_t iterations) noexcept
    {
      for (size_t i = 0; i < roots.size(); ++i)
        for (size_t iter = 0; iter < iterations; ++iter)
          {
            const PolynomialEvaluation eval =
                evaluate_polynomial_and_derivative(coefficients,
                                                   roots[i],
                                                   "FFT::root_solver");
            if (std::abs(eval.derivative) <= tol)
              break;

            const Complex step = eval.value / eval.derivative;
            roots(i) -= step;
            if (std::abs(step) <= tol * (Real(1) + std::abs(roots[i])))
              break;
          }
    }

    static void
    enforce_real_polynomial_symmetry(Array<Complex> & roots,
                                     const Real tol) noexcept
    {
      Array<bool> used(roots.size(), false);
      for (size_t i = 0; i < roots.size(); ++i)
        {
          if (used[i])
            continue;

          if (std::abs(roots[i].imag()) <= tol * (Real(8) + std::abs(roots[i])))
            {
              roots(i) = Complex(roots[i].real(), Real(0));
              used[i] = true;
              continue;
            }

          const Complex target = std::conj(roots[i]);
          size_t best = roots.size();
          Real best_error = std::numeric_limits<Real>::infinity();
          for (size_t j = i + 1; j < roots.size(); ++j)
            {
              if (used[j])
                continue;
              const Real error = std::abs(roots[j] - target);
              if (error < best_error)
                {
                  best_error = error;
                  best = j;
                }
            }

          if (best == roots.size()
              or best_error > tol * (Real(16) + std::abs(roots[i])))
            continue;

          const Complex center = (roots[i] + std::conj(roots[best])) / Real(2);
          const Real imag = std::abs(center.imag());
          roots(i) = Complex(center.real(),
                             roots[i].imag() >= Real(0) ? imag : -imag);
          roots(best) = std::conj(roots[i]);
          used[i] = true;
          used[best] = true;
        }
    }

    [[nodiscard]] static bool
    roots_pass_residual_check(const Array<Real> & coefficients,
                              const Array<Complex> & roots,
                              const Real tol) noexcept
    {
      for (size_t i = 0; i < roots.size(); ++i)
        {
          const PolynomialEvaluation eval =
              evaluate_polynomial_and_derivative(coefficients,
                                                 roots[i],
                                                 "FFT::root_solver");
          Real bound = Real(0);
          const Real radius = std::abs(roots[i]);
          for (size_t k = 0; k < coefficients.size(); ++k)
            bound = bound * radius + std::abs(coefficients[k]);

          if (std::abs(eval.value) > tol * (bound + Real(1)))
            return false;
        }

      return true;
    }

    static bool
    try_laguerre_roots(const Array<Real> & coefficients,
                       const Real tol,
                       Array<Complex> & roots) noexcept
    {
      const size_t n = coefficients.size();
      const size_t degree = n - 1;

      Array<Complex> deflated = Array<Complex>::create(n);
      for (size_t i = 0; i < n; ++i)
        deflated(i) = Complex(coefficients[i], Real(0));

      for (size_t k = 0; k < degree; ++k)
        {
          const size_t m = degree - k;
          const Real m_r = static_cast<Real>(m);
          Complex z = std::polar(Real(0.8) + Real(0.4)
                                 * static_cast<Real>(k)
                                 / static_cast<Real>(degree + 1),
                                 Real(2) * std::numbers::pi_v<Real>
                                 * (static_cast<Real>(k) + Real(0.5))
                                 / static_cast<Real>(degree));

          bool found = false;
          for (size_t iter = 0; iter < 512; ++iter)
            {
              Complex p = deflated[0];
              Complex dp(Real(0), Real(0));
              Complex d2p(Real(0), Real(0));
              for (size_t i = 1; i <= m; ++i)
                {
                  d2p = z * d2p + dp;
                  dp = z * dp + p;
                  p = z * p + deflated[i];
                }
              d2p *= Complex(Real(2), Real(0));

              if (std::abs(p) <= tol)
                {
                  found = true;
                  break;
                }

              const Complex G = dp / p;
              const Complex H = G * G - d2p / p;
              const Complex disc =
                  std::sqrt(Complex(m_r - Real(1), Real(0))
                            * (Complex(m_r, Real(0)) * H - G * G));

              const Complex denom1 = G + disc;
              const Complex denom2 = G - disc;
              const Complex & best_denom =
                  std::abs(denom1) >= std::abs(denom2) ?
                    denom1 :
                    denom2;

              Complex step;
              if (std::abs(best_denom) > tol)
                step = Complex(m_r, Real(0)) / best_denom;
              else
                step = (Real(1) + std::abs(z))
                       * std::polar(Real(1),
                                    Real(2) * std::numbers::pi_v<Real>
                                    * static_cast<Real>(iter * 2 + 1)
                                    / Real(97));

              z -= step;
              if (std::abs(step) <= tol * (Real(1) + std::abs(z)))
                {
                  found = true;
                  break;
                }
            }

          if (not found)
            return false;

          roots(k) = z;

          Array<Complex> reduced = Array<Complex>::create(m);
          reduced(0) = deflated[0];
          for (size_t i = 1; i < m; ++i)
            reduced(i) = deflated[i] + z * reduced[i - 1];

          deflated = std::move(reduced);
        }

      return true;
    }

    /** @brief Rationale: Master implementation for polynomial root-finding.
     *         It uses a multi-strategy approach (linear/quadratic solvers,
     *         Aberth-Ehrlich, Durand-Kerner, and Laguerre) with polynomial
     *         balancing and root polishing to ensure convergence and precision.
     */
    [[nodiscard]] static Array<Complex>
    polynomial_roots_impl(const Array<Real> & coefficients,
                          const char *ctx)
    {
      const size_t n = effective_coeff_length(coefficients);
      if (n <= 1)
        return {};

      const Real leading = coefficients[0];
      const Real tol = scaled_tolerance(max_abs_value(coefficients), Real(2048));
      ah_domain_error_if(std::abs(leading) <= tol)
        << ctx << ": leading polynomial coefficient must be non-zero";

      const size_t degree = n - 1;
      if (degree == 1)
        {
          Array<Complex> roots = Array<Complex>::create(1);
          roots(0) = Complex(-coefficients[1] / coefficients[0], Real(0));
          return roots;
        }

      if (degree == 2)
        {
          const Real a = coefficients[0];
          const Real b = coefficients[1];
          const Real c = coefficients[2];
          const Complex disc =
              std::sqrt(Complex(b * b - Real(4) * a * c, Real(0)));
          const Complex signed_disc = b >= Real(0) ? disc : -disc;
          const Complex q = Complex(Real(-0.5), Real(0))
                            * (Complex(b, Real(0)) + signed_disc);
          Array<Complex> roots = Array<Complex>::create(2);
          if (std::abs(q) <= tol)
            {
              const Complex repeated(-b / (Real(2) * a), Real(0));
              roots(0) = repeated;
              roots(1) = repeated;
            }
          else
            {
              roots(0) = q / a;
              roots(1) = c / q;
            }
          enforce_real_polynomial_symmetry(roots, tol);
          return roots;
        }

      const BalancedPolynomial problem =
          balance_polynomial_for_roots(coefficients, ctx);
      Array<Complex> roots = initialize_root_guesses(problem.balanced);
      bool converged =
          try_aberth_ehrlich_roots(problem.balanced, problem.tol, roots);
      if (not converged)
        {
          roots = initialize_root_guesses(problem.balanced);
          converged =
              try_durand_kerner_roots(problem.balanced,
                                      problem.tol * Real(4),
                                      roots);
        }

      if (not converged)
        converged =
            try_laguerre_roots(problem.balanced,
                               problem.tol * Real(4),
                               roots);

      ah_domain_error_if(not converged) << ctx << ": root solver did not converge";

      polish_roots_with_newton(problem.balanced, roots, problem.tol, 8);
      for (size_t i = 0; i < roots.size(); ++i)
        roots(i) *= problem.root_scale;
      polish_roots_with_newton(problem.monic,
                               roots,
                               problem.residual_tol,
                               8);
      enforce_real_polynomial_symmetry(roots, problem.residual_tol);
      ah_domain_error_if(not roots_pass_residual_check(problem.monic,
                           roots,
                           problem.residual_tol))
        << ctx << ": root solver residual check failed";

      return roots;
    }

    /** @brief Rationale: Copy utility that removes leading zero coefficients,
     *         effectively normalizing the polynomial degree.
     */
    [[nodiscard]] static Array<Real>
    trim_leading_zeros_copy(const Array<Real> & input) noexcept
    {
      if (input.is_empty())
        return {};

      size_t first = 0;
      while (first < input.size() and input[first] == Real(0))
        ++first;

      if (first == input.size())
        return {};

      Array<Real> trimmed;
      trimmed.reserve(input.size() - first);
      for (size_t i = first; i < input.size(); ++i)
        trimmed.append(input[i]);
      return trimmed;
    }

    /** @brief Internal storage for complex transfer function evaluation terms. */
    struct TransferTerms
    {
      Complex numerator = Complex(Real(0), Real(0));
      Complex denominator = Complex(Real(0), Real(0));
      Complex numerator_derivative = Complex(Real(0), Real(0));
      Complex denominator_derivative = Complex(Real(0), Real(0));
    };

    /** @brief implementation helper for evaluating H(z) and its derivatives on the unit circle. */
    [[nodiscard]] static TransferTerms
    evaluate_transfer_terms_at(const Array<Real> & numerator,
                               const Array<Real> & denominator,
                               const Real omega,
                               const char *ctx)
    {
      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, ctx);
      TransferTerms terms;

      const SeriesEvaluation numerator_terms =
          evaluate_series_at_unit_circle(coeffs.numerator, omega, ctx);
      const SeriesEvaluation denominator_terms =
          evaluate_series_at_unit_circle(coeffs.denominator, omega, ctx);

      terms.numerator = numerator_terms.value;
      terms.numerator_derivative = numerator_terms.derivative;
      terms.denominator = denominator_terms.value;
      terms.denominator_derivative = denominator_terms.derivative;

      const Real den_tol =
          series_zero_tolerance(coeffs.denominator.size(), Real(512));
      ah_domain_error_if(std::abs(terms.denominator) <= den_tol)
        << ctx << ": transfer response is singular at omega=" << omega;

      return terms;
    }

    /** @brief Rationale: Analytic calculation of group delay (-dphi/domega)
     *         using the complex derivative of the transfer function, which
     *         is more accurate than numerical differentiation.
     */
    [[nodiscard]] static Array<Real>
    analytic_group_delay_impl(const Array<Real> & numerator,
                              const Array<Real> & denominator,
                              const size_t num_points,
                              const bool whole,
                              const char *ctx)
    {
      const FrequencyResponse response =
          freqz_impl(numerator, denominator, num_points, whole, ctx);
      const Array<Real> numeric_fallback = group_delay_impl(response);
      const Array<Real> magnitude = response.magnitude();
      Array<Real> delay = Array<Real>::create(num_points);
      const Real tol = (max_abs_value(magnitude) + Real(1))
                       * Real(256) * std::numeric_limits<Real>::epsilon();

      for (size_t i = 0; i < num_points; ++i)
        {
          const TransferTerms terms =
              evaluate_transfer_terms_at(numerator, denominator, response.omega[i], ctx);
          if (std::abs(terms.numerator)
              <= std::max(tol,
                          series_zero_tolerance(effective_coeff_length(numerator),
                                                Real(512))))
            delay(i) = numeric_fallback[i];
          else
            delay(i) = -std::imag(terms.numerator_derivative / terms.numerator
                                  - terms.denominator_derivative
                                  / terms.denominator);
        }

      return delay;
    }

    /** @brief implementation of analytic group delay for SOS cascades. */
    [[nodiscard]] static Array<Real>
    analytic_sos_group_delay_impl(const Array<BiquadSection> & sections,
                                  const FrequencyResponse & response,
                                  const char *ctx)
    {
      ah_invalid_argument_if(sections.is_empty())
        << ctx << ": at least one biquad section is required";

      const Array<Real> numeric_fallback = group_delay_impl(response);
      Array<Real> delay = Array<Real>::create(response.omega.size());
      for (size_t i = 0; i < response.omega.size(); ++i)
        {
          Real total = Real(0);
          bool analytic = true;
          for (size_t j = 0; j < sections.size(); ++j)
            {
              const TransferTerms terms =
                  evaluate_transfer_terms_at(sections[j].numerator(),
                                             sections[j].denominator(),
                                             response.omega[i],
                                             ctx);
              const Real num_tol =
                  series_zero_tolerance(3, Real(512));
              if (std::abs(terms.numerator) <= num_tol)
                {
                  analytic = false;
                  break;
                }

              total += -std::imag(terms.numerator_derivative / terms.numerator
                                  - terms.denominator_derivative
                                  / terms.denominator);
            }
          delay(i) = analytic ? total : numeric_fallback[i];
        }

      return delay;
    }

    /** @brief implementation of analytic phase delay calculation. */
    [[nodiscard]] static Array<Real>
    analytic_phase_delay_impl(const Array<Real> & numerator,
                              const Array<Real> & denominator,
                              const size_t num_points,
                              const bool whole,
                              const char *ctx)
    {
      const FrequencyResponse response =
          freqz_impl(numerator, denominator, num_points, whole, ctx);
      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      const Array<Real> group =
          analytic_group_delay_impl(numerator, denominator, num_points, whole, ctx);
      Array<Real> delay = Array<Real>::create(num_points);
      const Real tol = (response.omega[response.omega.size() - 1] + Real(1))
                       * Real(128) * std::numeric_limits<Real>::epsilon();

      for (size_t i = 0; i < num_points; ++i)
        delay(i) = std::abs(response.omega[i]) <= tol ? group[i] : -phase[i] / response.omega[i];

      return delay;
    }

    /** @brief Returns the magnitude of the largest root in the set. */
    [[nodiscard]] static Real
    max_root_radius(const Array<Complex> & roots) noexcept
    {
      Real radius = Real(0);
      for (size_t i = 0; i < roots.size(); ++i)
        radius = std::max(radius, static_cast<Real>(std::abs(roots[i])));
      return radius;
    }

    /** @brief Rationale: Matches zeros and poles into pairs by minimizing
     *         Euclidean distance, aiding in SOS decomposition.
     */
    [[nodiscard]] static Array<PoleZeroPair>
    pole_zero_pairs_impl(const Array<Complex> & zeros,
                         const Array<Complex> & poles)
    {
      Array<PoleZeroPair> pairs;
      Array<bool> zero_used(zeros.size(), false);
      Array<bool> pole_used(poles.size(), false);
      const size_t matched = std::min(zeros.size(), poles.size());

      for (size_t count = 0; count < matched; ++count)
        {
          bool found = false;
          size_t best_zero = 0;
          size_t best_pole = 0;
          Real best_distance = std::numeric_limits<Real>::infinity();

          for (size_t i = 0; i < zeros.size(); ++i)
            {
              if (zero_used[i])
                continue;

              for (size_t j = 0; j < poles.size(); ++j)
                {
                  if (pole_used[j])
                    continue;

                  const Real distance = std::abs(zeros[i] - poles[j]);
                  if (distance < best_distance)
                    {
                      best_distance = distance;
                      best_zero = i;
                      best_pole = j;
                      found = true;
                    }
                }
            }

          if (not found)
            break;

          PoleZeroPair pair;
          pair.zero = zeros[best_zero];
          pair.pole = poles[best_pole];
          pair.distance = best_distance;
          pair.has_zero = true;
          pair.has_pole = true;
          pairs.append(pair);
          zero_used[best_zero] = true;
          pole_used[best_pole] = true;
        }

      for (size_t i = 0; i < zeros.size(); ++i)
        if (not zero_used[i])
          {
            PoleZeroPair pair;
            pair.zero = zeros[i];
            pair.has_zero = true;
            pairs.append(pair);
          }

      for (size_t i = 0; i < poles.size(); ++i)
        if (not pole_used[i])
          {
            PoleZeroPair pair;
            pair.pole = poles[i];
            pair.has_pole = true;
            pairs.append(pair);
          }

      return pairs;
    }

    /** @brief Rationale: Estimates the minimum distance between any matched
     *         pole and zero to identify potential cancellations.
     */
    [[nodiscard]] static Real
    minimum_cancellation_distance_impl(const Array<PoleZeroPair> & pairs) noexcept
    {
      Real best = std::numeric_limits<Real>::infinity();
      for (size_t i = 0; i < pairs.size(); ++i)
        if (pairs[i].has_zero and pairs[i].has_pole)
          best = std::min(best, pairs[i].distance);
      return best;
    }

    /** @brief Rationale: Group of one or more roots (typically a conjugate pair)
     *         sharing a common geometric center.
     */
    struct RootGroup
    {
      Array<Complex> roots;
      Complex center = Complex(Real(0), Real(0));
    };

    /** @brief Parameters for an analog filter prototype (S-domain). */
    struct AnalogPrototype
    {
      Array<Complex> zeros;
      Array<Complex> poles;
      Real gain = Real(1);
    };

    /** @brief Value triplet for Jacobi elliptic functions (sn, cn, dn). */
    struct JacobiValues
    {
      Real sn = Real(0);
      Real cn = Real(1);
      Real dn = Real(1);
    };

    /** @brief Frequency-domain grid with target values and importance weights. */
    struct WeightedFrequencyGrid
    {
      Array<Real> omega;
      Array<Real> desired;
      Array<Real> weight;
    };

    /** @brief Rationale: Standard polynomial multiplication (O(N*M)). */
    template <typename T>
    [[nodiscard]] static Array<T>
    polynomial_multiply(const Array<T> & lhs, const Array<T> & rhs)
    {
      if (lhs.is_empty() or rhs.is_empty())
        return {};

      Array<T> output = Array<T>::create(lhs.size() + rhs.size() - 1);
      for (size_t i = 0; i < output.size(); ++i)
        output(i) = T(0);

      for (size_t i = 0; i < lhs.size(); ++i)
        for (size_t j = 0; j < rhs.size(); ++j)
          output(i + j) += lhs[i] * rhs[j];

      return output;
    }

    /** @brief Rationale: In-place addition of a scaled polynomial. */
    template <typename T>
    static void
    add_scaled_polynomial(Array<T> & dst,
                          const Array<T> & src,
                          const T scale)
    {
      ah_invalid_argument_if(dst.size() < src.size())
        << "FFT::add_scaled_polynomial: destination size "
        << dst.size() << " is smaller than source size " << src.size();

      for (size_t i = 0; i < src.size(); ++i)
        dst(i) += src[i] * scale;
    }

    /** @brief Fast integer power utility. */
    [[nodiscard]] static Real
    integer_power(const Real base, const size_t exponent) noexcept
    {
      Real result = Real(1);
      for (size_t i = 0; i < exponent; ++i)
        result *= base;
      return result;
    }

    /** @brief Rationale: Computes (1 + sign*x)^power coefficients. */
    [[nodiscard]] static Array<Real>
    signed_binomial(const size_t power, const Real sign)
    {
      Array<Real> coeffs = Array<Real>::create(power + 1);
      coeffs(0) = Real(1);
      for (size_t k = 1; k <= power; ++k)
        coeffs(k) = coeffs[k - 1]
                    * static_cast<Real>(power - k + 1)
                    / static_cast<Real>(k)
                    * sign;
      return coeffs;
    }

    /** @brief Rationale: Performs the bilinear substitution mapping from the
     *         S-plane to the Z-domain: H(z) = H(s)|s=2*fs*(z-1)/(z+1).
     */
    [[nodiscard]] static Array<Real>
    bilinear_substitute_polynomial(const Array<Real> & analog,
                                   const size_t order,
                                   const Real sample_rate,
                                   const char *ctx)
    {
      ah_invalid_argument_if(analog.is_empty())
        << ctx << ": analog polynomial must be non-empty";
      ah_invalid_argument_if(sample_rate <= Real(0))
        << ctx << ": sample rate " << sample_rate << " must be positive";

      const size_t degree = analog.size() - 1;
      ah_invalid_argument_if(degree > order)
        << ctx << ": analog degree " << degree << " exceeds requested bilinear order " << order;

      Array<Real> output = Array<Real>::create(order + 1);
      for (size_t i = 0; i < output.size(); ++i)
        output(i) = Real(0);

      const Real fs2 = Real(2) * sample_rate;
      for (size_t i = 0; i < analog.size(); ++i)
        {
          const size_t power = degree - i;
          const Array<Real> one_minus = signed_binomial(power, Real(-1));
          const Array<Real> one_plus = signed_binomial(order - power, Real(1));
          const Array<Real> term = polynomial_multiply(one_minus, one_plus);
          add_scaled_polynomial(output,
                                term,
                                analog[i] * integer_power(fs2, power));
        }

      return output;
    }

    /** @brief Rationale: Polynomial expansion (z-r1)(z-r2)... */
    [[nodiscard]] static Array<Complex>
    polynomial_from_roots_complex(const Array<Complex> & roots)
    {
      Array<Complex> coeffs = Array<Complex>::create(1);
      coeffs(0) = Complex(Real(1), Real(0));

      for (size_t i = 0; i < roots.size(); ++i)
        {
          Array<Complex> next = Array<Complex>::create(coeffs.size() + 1);
          for (size_t j = 0; j < next.size(); ++j)
            next(j) = Complex(Real(0), Real(0));

          for (size_t j = 0; j < coeffs.size(); ++j)
            {
              next(j) += coeffs[j];
              next(j + 1) -= coeffs[j] * roots[i];
            }

          coeffs = std::move(next);
        }

      return coeffs;
    }

    /** @brief Validation wrapper for @ref trim_leading_zeros_copy. */
    [[nodiscard]] static Array<Real>
    trim_polynomial_leading_zeros(const Array<Real> & input,
                                  const char *ctx)
    {
      const Array<Real> trimmed = trim_leading_zeros_copy(input);
      ah_invalid_argument_if(trimmed.is_empty()) << ctx << ": polynomial vanished after trimming";
      return trimmed;
    }

    /** @brief implementation of P(x)^n via successive multiplication. */
    [[nodiscard]] static Array<Real>
    polynomial_power(const Array<Real> & poly,
                     const size_t exponent,
                     const char *ctx)
    {
      ah_invalid_argument_if(poly.is_empty()) << ctx << ": polynomial base must be non-empty";

      Array<Real> output = {Real(1)};
      for (size_t i = 0; i < exponent; ++i)
        output = polynomial_multiply(output, poly);
      return output;
    }

    /** @brief Rationale: Evaluates P(N(z)/D(z)) by scaling by the common
     *         denominator D(z)^degree and evaluating the resulting nested
     *         polynomial expansion.
     */
    [[nodiscard]] static Array<Real>
    substitute_rational_polynomial(const Array<Real> & poly,
                                   const Array<Real> & numerator,
                                   const Array<Real> & denominator,
                                   const char *ctx)
    {
      const size_t degree = effective_coeff_length(poly);
      ah_invalid_argument_if(degree == 0)
        << ctx << ": polynomial must be non-empty";
      ah_invalid_argument_if(numerator.is_empty() or denominator.is_empty())
        << ctx << ": rational substitution polynomials must be non-empty";

      const size_t order = degree - 1;
      Array<Array<Real>> numerator_powers(order + 1, Array<Real>());
      Array<Array<Real>> denominator_powers(order + 1, Array<Real>());
      numerator_powers(0) = {Real(1)};
      denominator_powers(0) = {Real(1)};
      for (size_t i = 1; i <= order; ++i)
        {
          numerator_powers(i) =
              polynomial_multiply(numerator_powers[i - 1], numerator);
          denominator_powers(i) =
              polynomial_multiply(denominator_powers[i - 1], denominator);
        }

      Array<Real> output = {Real(0)};
      for (size_t i = 0; i <= order; ++i)
        {
          const Array<Real> term =
              polynomial_multiply(numerator_powers[order - i],
                                  denominator_powers[i]);
          if (output.size() < term.size())
            {
              Array<Real> grown = Array<Real>::create(term.size());
              for (size_t j = 0; j < grown.size(); ++j)
                grown(j) = Real(0);
              for (size_t j = 0; j < output.size(); ++j)
                grown(j + (term.size() - output.size())) = output[j];
              output = std::move(grown);
            }

          Array<Real> aligned = Array<Real>::create(output.size());
          for (size_t j = 0; j < aligned.size(); ++j)
            aligned(j) = Real(0);
          for (size_t j = 0; j < term.size(); ++j)
            aligned(j + (aligned.size() - term.size())) = term[j];
          for (size_t j = 0; j < output.size(); ++j)
            output(j) += poly[i] * aligned[j];
        }

      return trim_polynomial_leading_zeros(output, ctx);
    }

    /** @brief Rationale: Transforms an analog transfer function H(s) via rational
     *         substitution s = N(z)/D(z), as used in bilinear or frequency-selective
     *         mappings.
     */
    [[nodiscard]] static IIRCoefficients
    apply_analog_rational_transform(const Array<Real> & numerator,
                                    const Array<Real> & denominator,
                                    const Array<Real> & map_numerator,
                                    const Array<Real> & map_denominator,
                                    const char *ctx)
    {
      const size_t num_degree = effective_coeff_length(numerator) - 1;
      const size_t den_degree = effective_coeff_length(denominator) - 1;
      Array<Real> transformed_numerator =
          substitute_rational_polynomial(numerator,
                                         map_numerator,
                                         map_denominator,
                                         ctx);
      Array<Real> transformed_denominator =
          substitute_rational_polynomial(denominator,
                                         map_numerator,
                                         map_denominator,
                                         ctx);

      if (den_degree > num_degree)
        transformed_numerator =
            polynomial_multiply(transformed_numerator,
                                polynomial_power(map_denominator,
                                                 den_degree - num_degree,
                                                 ctx));
      else if (num_degree > den_degree)
        transformed_denominator =
            polynomial_multiply(transformed_denominator,
                                polynomial_power(map_denominator,
                                                 num_degree - den_degree,
                                                 ctx));

      Array<Real> compact_numerator =
          trim_polynomial_leading_zeros(transformed_numerator, ctx);
      Array<Real> compact_denominator =
          trim_polynomial_leading_zeros(transformed_denominator, ctx);
      const Real a0 = compact_denominator[0];
      const Real tol = scaled_tolerance(a0, Real(64));
      ah_domain_error_if(std::abs(a0) <= tol)
        << ctx << ": transformed analog denominator leading coefficient "
        << a0 << " is too small";
      for (size_t i = 0; i < compact_numerator.size(); ++i)
        compact_numerator(i) /= a0;
      for (size_t i = 0; i < compact_denominator.size(); ++i)
        compact_denominator(i) /= a0;

      IIRCoefficients coeffs;
      coeffs.numerator = std::move(compact_numerator);
      coeffs.denominator = std::move(compact_denominator);
      return coeffs;
    }

    /** @brief Rationale: Constructs a real-coefficient polynomial from a set
     *         of complex roots, verifying that roots appear in conjugate pairs.
     */
    [[nodiscard]] static Array<Real>
    real_polynomial_from_roots(const Array<Complex> & roots,
                               const char *ctx)
    {
      const Array<Complex> coeffs = polynomial_from_roots_complex(roots);
      Real max_coeff = Real(0);
      for (size_t i = 0; i < coeffs.size(); ++i)
        max_coeff = std::max(max_coeff,
                             static_cast<Real>(std::abs(coeffs[i])));

      const Real tol = (max_coeff + Real(1))
                       * Real(1024) * std::numeric_limits<Real>::epsilon();
      Array<Real> real_coeffs = Array<Real>::create(coeffs.size());
      for (size_t i = 0; i < coeffs.size(); ++i)
        {
          ah_domain_error_if(std::abs(coeffs[i].imag()) > tol)
            << ctx << ": root set does not generate a real polynomial";
          real_coeffs(i) = coeffs[i].real();
        }

      return real_coeffs;
    }

    /** @brief Geometric center of a root set. */
    [[nodiscard]] static Complex
    group_center(const Array<Complex> & roots) noexcept
    {
      if (roots.is_empty())
        return Complex(Real(0), Real(0));

      Complex center(Real(0), Real(0));
      for (size_t i = 0; i < roots.size(); ++i)
        center += roots[i];
      return center / static_cast<Real>(roots.size());
    }

    /** @brief Enforce conjugate symmetry on roots of a real-coefficient
     *  polynomial.  For every complex root the nearest conjugate candidate
     *  is found and both are replaced by the exact conjugate pair built
     *  from the average of their real parts and imaginary magnitudes. */
    [[nodiscard]] static Array<Complex>
    enforce_conjugate_symmetry(const Array<Complex> & roots)
    {
      const Real tol = (max_root_radius(roots) + Real(1))
                       * Real(2048) * std::numeric_limits<Real>::epsilon();
      Array<Complex> result = roots;
      Array<bool> paired(roots.size(), false);

      for (size_t i = 0; i < result.size(); ++i)
        {
          if (paired[i] or std::abs(result[i].imag()) <= tol)
            continue;

          size_t best = result.size();
          Real best_error = std::numeric_limits<Real>::infinity();
          const Complex target = std::conj(result[i]);
          for (size_t j = i + 1; j < result.size(); ++j)
            {
              if (paired[j])
                continue;
              const Real error = std::abs(result[j] - target);
              if (error < best_error)
                {
                  best_error = error;
                  best = j;
                }
            }

          if (best < result.size())
            {
              const Real avg_re =
                  (result[i].real() + result[best].real()) / Real(2);
              const Real avg_im =
              (std::abs(result[i].imag())
               + std::abs(result[best].imag())) / Real(2);
              result(i) = Complex(avg_re, avg_im);
              result(best) = Complex(avg_re, -avg_im);
              paired[i] = true;
              paired[best] = true;
            }
        }

      return result;
    }

    /** @brief Rationale: Partitions a set of roots into real sections and
     *         conjugate pairs to facilitate SOS decomposition.
     */
    [[nodiscard]] static Array<RootGroup>
    root_groups_impl(const Array<Complex> & raw_roots,
                     const char *ctx)
    {
      if (raw_roots.is_empty())
        return {};

      const Array<Complex> roots = enforce_conjugate_symmetry(raw_roots);

      const Real tol = (max_root_radius(roots) + Real(1))
                       * Real(2048) * std::numeric_limits<Real>::epsilon();
      Array<bool> used(roots.size(), false);
      Array<RootGroup> groups;

      for (size_t i = 0; i < roots.size(); ++i)
        {
          if (used[i] or std::abs(roots[i].imag()) <= tol)
            continue;

          size_t best = roots.size();
          Real best_error = std::numeric_limits<Real>::infinity();
          const Complex target = std::conj(roots[i]);
          for (size_t j = i + 1; j < roots.size(); ++j)
            {
              if (used[j])
                continue;

              const Real error = std::abs(roots[j] - target);
              if (error < best_error)
                {
                  best_error = error;
                  best = j;
                }
            }

          ah_domain_error_if(best == roots.size()
                             or best_error > tol * (Real(16)
                               + std::abs(roots[i])))
            << ctx << ": complex root " << roots[i] << " does not have a matching conjugate";

          RootGroup group;
          group.roots = {roots[i], roots[best]};
          group.center = group_center(group.roots);
          groups.append(group);
          used[i] = true;
          used[best] = true;
        }

      Array<Complex> real_roots;
      for (size_t i = 0; i < roots.size(); ++i)
        {
          if (used[i])
            continue;

          ah_domain_error_if(std::abs(roots[i].imag()) > tol)
            << ctx << ": root " << roots[i] << " was not paired into a real section";
          real_roots.append(Complex(roots[i].real(), Real(0)));
        }

      Array<bool> real_used(real_roots.size(), false);
      while (true)
        {
          size_t first = real_roots.size();
          for (size_t i = 0; i < real_roots.size(); ++i)
            if (not real_used[i])
              {
                first = i;
                break;
              }

          if (first == real_roots.size())
            break;

          size_t second = real_roots.size();
          Real best_distance = std::numeric_limits<Real>::infinity();
          for (size_t j = first + 1; j < real_roots.size(); ++j)
            {
              if (real_used[j])
                continue;

              const Real distance = std::abs(real_roots[first] - real_roots[j]);
              if (distance < best_distance)
                {
                  best_distance = distance;
                  second = j;
                }
            }

          RootGroup group;
          group.roots.append(real_roots[first]);
          real_used[first] = true;
          if (second != real_roots.size())
            {
              group.roots.append(real_roots[second]);
              real_used[second] = true;
            }
          group.center = group_center(group.roots);
          groups.append(group);
        }

      return groups;
    }

    /** @brief Rationale: Converts generic IIR coefficients into a stabilized
     *         BiquadSection structure, enforcing the second-order limit.
     */
    [[nodiscard]] static BiquadSection
    section_from_coefficients(const Array<Real> & numerator,
                              const Array<Real> & denominator,
                              const char *ctx)
    {
      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, ctx);
      ah_invalid_argument_if(coeffs.denominator.size() > 3)
        << ctx << ": section denominator order " << (coeffs.denominator.size() - 1)
        << " exceeds second order";

      BiquadSection section;
      Array<Real> b = Array<Real>::create(3);
      Array<Real> a = Array<Real>::create(3);
      for (size_t i = 0; i < 3; ++i)
        {
          b(i) = Real(0);
          a(i) = Real(0);
        }

      for (size_t i = 0; i < coeffs.numerator.size(); ++i)
        b(i) = coeffs.numerator[i];
      for (size_t i = 0; i < coeffs.denominator.size(); ++i)
        a(i) = coeffs.denominator[i];

      section.b0 = b[0];
      section.b1 = b[1];
      section.b2 = b[2];
      section.a0 = a[0];
      section.a1 = a[1];
      section.a2 = a[2];
      return section;
    }

    /** @brief implementation helper for the bilinear transform. */
    [[nodiscard]] static IIRCoefficients
    bilinear_transform_impl(const Array<Real> & analog_numerator,
                            const Array<Real> & analog_denominator,
                            const Real sample_rate,
                            const char *ctx)
    {
      ah_invalid_argument_if(analog_numerator.is_empty())
        << ctx << ": analog numerator must be non-empty";
      ah_invalid_argument_if(analog_denominator.is_empty())
        << ctx << ": analog denominator must be non-empty";

      const size_t order =
          std::max(analog_numerator.size(), analog_denominator.size()) - 1;
      const Array<Real> digital_numerator =
          bilinear_substitute_polynomial(analog_numerator,
                                         order,
                                         sample_rate,
                                         ctx);
      const Array<Real> digital_denominator =
          bilinear_substitute_polynomial(analog_denominator,
                                         order,
                                         sample_rate,
                                         ctx);
      return normalize_iir_coefficients(digital_numerator,
                                        digital_denominator,
                                        ctx);
    }

    /** @brief Rationale: Prewarps an analog cutoff frequency to compensate for
     *         the non-linear frequency mapping of the bilinear transform.
     */
    [[nodiscard]] static Real
    prewarp_frequency(const Real cutoff_frequency,
                      const Real sample_rate,
                      const char *ctx)
    {
      ah_invalid_argument_if(sample_rate <= Real(0))
        << ctx << ": sample rate " << sample_rate << " must be positive";
      ah_invalid_argument_if(cutoff_frequency <= Real(0))
        << ctx << ": cutoff frequency " << cutoff_frequency
        << " must be positive";
      ah_invalid_argument_if(cutoff_frequency >= sample_rate / Real(2))
        << ctx << ": cutoff frequency " << cutoff_frequency
        << " must be smaller than Nyquist " << (sample_rate / Real(2));

      return Real(2) * sample_rate
             * std::tan(std::numbers::pi_v<Real>
                        * cutoff_frequency / sample_rate);
    }

    // -----------------------------------------------------------------
    // Elliptic integral portability layer.
    //
    // std::comp_ellint_1 and std::ellint_1 are C++17 special math
    // functions fully supported in libstdc++ but absent from libc++.
    // The fallback uses the Arithmetic-Geometric Mean (AGM) iteration,
    // which converges quadratically and is accurate to machine epsilon
    // in a handful of iterations.
    // -----------------------------------------------------------------

# if defined(__GLIBCXX__) or defined(_MSVC_STL_VERSION)

    [[nodiscard]] static Real
    ellint_1_impl(const Real k, const Real phi)
    {
      return std::ellint_1(k, phi);
    }

    [[nodiscard]] static Real
    comp_ellint_1_impl(const Real k)
    {
      return std::comp_ellint_1(k);
    }

# else // libc++ or other stdlib without C++17 special math

    [[nodiscard]] static Real
    comp_ellint_1_impl(const Real k)
    {
      // Complete elliptic integral K(k) via AGM iteration.
      Real a = Real(1);
      Real b = std::sqrt(std::max(Real(0), Real(1) - k * k));
      for (size_t i = 0; i < 64 and std::abs(a - b) >
                         std::numeric_limits<Real>::epsilon() * a; ++i)
        {
          const Real next_a = (a + b) / Real(2);
          b = std::sqrt(a * b);
          a = next_a;
        }
      return std::numbers::pi_v<Real> / (Real(2) * a);
    }

    [[nodiscard]] static Real
    carlson_rf_impl(Real x, Real y, Real z)
    {
      // Carlson's symmetric elliptic integral R_F(x, y, z) via the
      // duplication theorem (Carlson 1979).  Converges cubically —
      // typically 5-6 iterations for double precision.
      const Real errtol =
          std::pow(std::numeric_limits<Real>::epsilon() / Real(3),
                   Real(1) / Real(6));
      for (size_t i = 0; i < 64; ++i)
        {
          const Real mu = (x + y + z) / Real(3);
          const Real dx = Real(1) - x / mu;
          const Real dy = Real(1) - y / mu;
          const Real dz = Real(1) - z / mu;
          if (std::max({std::abs(dx), std::abs(dy), std::abs(dz)})
              < errtol)
            {
              const Real e2 = dx * dy - dz * dz;
              const Real e3 = dx * dy * dz;
              return (Real(1) - e2 / Real(10) + e3 / Real(14)
                      + e2 * e2 / Real(24)
                      - Real(3) * e2 * e3 / Real(44))
                     / std::sqrt(mu);
            }
          const Real lam = std::sqrt(x * y) + std::sqrt(x * z)
                           + std::sqrt(y * z);
          x = (x + lam) / Real(4);
          y = (y + lam) / Real(4);
          z = (z + lam) / Real(4);
        }
      return Real(1) / std::sqrt((x + y + z) / Real(3));
    }

    [[nodiscard]] static Real
    ellint_1_impl(const Real k, const Real phi)
    {
      // Incomplete elliptic integral F(phi, k) via Carlson's R_F.
      // F(phi, k) = sin(phi) * R_F(cos^2(phi), 1 - k^2 sin^2(phi), 1).
      // Uses the identity F(pi/2, k) = K(k) for the complete case.
      const Real half_pi = std::numbers::pi_v<Real> / Real(2);
      if (std::abs(phi - half_pi) <=
          std::numeric_limits<Real>::epsilon() * Real(4))
        return comp_ellint_1_impl(k);

      if (std::abs(phi) <=
          std::numeric_limits<Real>::epsilon() * Real(4))
        return Real(0);

      const Real s = std::sin(phi);
      const Real c = std::cos(phi);
      return s * carlson_rf_impl(c * c,
                                 std::max(Real(0),
                                          Real(1) - k * k * s * s),
                                 Real(1));
    }

# endif // stdlib dispatch

    /** @brief Rationale: Complete elliptic integral K(k) using the modulus
     *         convention required by elliptic filter design.
     */
    [[nodiscard]] static Real
    complete_elliptic_first_kind(const Real modulus,
                                 const char *ctx)
    {
      ah_invalid_argument_if(modulus < Real(0) or modulus >= Real(1))
        << ctx << ": elliptic modulus " << modulus << " must lie in [0, 1)";
      return comp_ellint_1_impl(modulus);
    }

    /** @brief Rationale: Simultanous calculation of Jacobi sn, cn, dn functions
     *         using the descending Landen transformation or AGM.
     */
    [[nodiscard]] static JacobiValues
    jacobi_sn_cn_dn(const Real argument,
                    const Real modulus,
                    const char *ctx)
    {
      ah_invalid_argument_if(modulus < Real(0) or modulus >= Real(1))
        << ctx << ": elliptic modulus " << modulus << " must lie in [0, 1)";

      if (std::abs(argument) <= scaled_tolerance(argument, Real(4096)))
        return {};

      const bool negative = argument < Real(0);
      const Real u = std::abs(argument);
      if (modulus <= scaled_tolerance(modulus, Real(4096)))
        {
          JacobiValues values;
          values.sn = std::sin(u);
          values.cn = std::cos(u);
          values.dn = Real(1);
          if (negative)
            values.sn = -values.sn;
          return values;
        }

      const Real K = complete_elliptic_first_kind(modulus, ctx);
      const Real endpoint_tol = scaled_tolerance(K, Real(4096));
      if (std::abs(u - K) <= endpoint_tol)
        {
          JacobiValues values;
          values.sn = Real(1);
          values.cn = Real(0);
          values.dn = std::sqrt(std::max(Real(0),
                                         Real(1) - modulus * modulus));
          if (negative)
            values.sn = -values.sn;
          return values;
        }

      ah_domain_error_if(u > K + endpoint_tol)
        << ctx << ": Jacobi argument " << u
        << " exceeds the supported quarter period " << K;

      Real lower = Real(0);
      Real upper = std::numbers::pi_v<Real> / Real(2);
      Real phi = upper * (u / K);
      phi = std::clamp(phi,
                       std::numeric_limits<Real>::epsilon(),
                       upper - std::numeric_limits<Real>::epsilon());

      for (size_t iter = 0; iter < 96; ++iter)
        {
          const Real value = ellint_1_impl(modulus, phi);
          const Real residual = value - u;
          if (std::abs(residual) <= scaled_tolerance(u, Real(8192)))
            break;

          if (residual > Real(0))
            upper = phi;
          else
            lower = phi;

          const Real sin_phi = std::sin(phi);
          const Real derivative =
              Real(1) / std::sqrt(std::max(Real(0),
                                           Real(1)
                                           - modulus * modulus
                                           * sin_phi * sin_phi));
          Real candidate = phi - residual / derivative;
          if (candidate <= lower or candidate >= upper
              or not std::isfinite(candidate))
            candidate = (lower + upper) / Real(2);
          phi = candidate;
        }

      JacobiValues values;
      values.sn = std::sin(phi);
      values.cn = std::cos(phi);
      values.dn = std::sqrt(std::max(Real(0),
                                     Real(1)
                                     - modulus * modulus
                                     * values.sn * values.sn));
      if (negative)
        values.sn = -values.sn;
      return values;
    }

    [[nodiscard]] static Real
    inverse_jacobi_sc(const Real value,
                      const Real modulus,
                      const char *ctx)
    {
      ah_invalid_argument_if(value < Real(0))
        << ctx << ": inverse Jacobi sc value " << value
        << " must be non-negative";
      ah_invalid_argument_if(modulus < Real(0) or modulus >= Real(1))
        << ctx << ": elliptic modulus " << modulus
        << " must lie in [0, 1)";
      return ellint_1_impl(modulus, std::atan(value));
    }

    [[nodiscard]] static Real
    solve_elliptic_selectivity_modulus(const Real k1,
                                       const size_t order,
                                       const char *ctx)
    {
      ah_invalid_argument_if(order == 0)
        << ctx << ": order must be positive";
      ah_invalid_argument_if(k1 <= Real(0) or k1 >= Real(1))
        << ctx << ": selectivity modulus " << k1
        << " must lie in (0, 1)";

      const Real k1_complement =
          std::sqrt(std::max(Real(0), Real(1) - k1 * k1));
      const Real target =
          complete_elliptic_first_kind(k1_complement, ctx)
          / (static_cast<Real>(order)
             * complete_elliptic_first_kind(k1, ctx));

      Real lower = Real(0);
      Real upper = Real(1) - std::sqrt(std::numeric_limits<Real>::epsilon());
      for (size_t iter = 0; iter < 128; ++iter)
        {
          const Real mid = (lower + upper) / Real(2);
          const Real mid_complement =
              std::sqrt(std::max(Real(0), Real(1) - mid * mid));
          const Real ratio =
              complete_elliptic_first_kind(mid_complement, ctx)
              / complete_elliptic_first_kind(mid, ctx);
          if (ratio > target)
            lower = mid;
          else
            upper = mid;
        }

      return (lower + upper) / Real(2);
    }

    [[nodiscard]] static Complex
    elliptic_cd_minus_imaginary(const Real u,
                                const Real v,
                                const Real modulus,
                                const char *ctx)
    {
      const JacobiValues uv = jacobi_sn_cn_dn(u, modulus, ctx);
      const Real complement =
          std::sqrt(std::max(Real(0), Real(1) - modulus * modulus));
      const JacobiValues vv = jacobi_sn_cn_dn(v, complement, ctx);
      const Complex cn(uv.cn * vv.cn,
                       uv.sn * uv.dn * vv.sn * vv.dn);
      const Complex dn(uv.dn * vv.cn * vv.dn,
                       modulus * modulus * uv.sn * uv.cn * vv.sn);
      ah_domain_error_if(std::abs(dn) <= scaled_tolerance(std::abs(dn), Real(4096)))
        << ctx << ": elliptic cd denominator vanished";
      return cn / dn;
    }

    [[nodiscard]] static AnalogPrototype
    butterworth_prototype(const size_t order,
                          const char *ctx)
    {
      ah_invalid_argument_if(order == 0)
        << ctx << ": order must be positive";

      AnalogPrototype prototype;
      prototype.poles = Array<Complex>::create(order);
      prototype.gain = Real(1);
      for (size_t k = 0; k < order; ++k)
        {
          const Real angle =
              std::numbers::pi_v<Real>
              * (Real(2) * static_cast<Real>(k)
                 + Real(1)
                 + static_cast<Real>(order))
              / (Real(2) * static_cast<Real>(order));
          prototype.poles(k) = std::polar(Real(1), angle);
        }

      return prototype;
    }

    [[nodiscard]] static AnalogPrototype
    chebyshev1_prototype(const size_t order,
                         const Real ripple_db,
                         const char *ctx)
    {
      ah_invalid_argument_if(order == 0)
        << ctx << ": order must be positive";
      ah_invalid_argument_if(ripple_db <= Real(0))
        << ctx << ": passband ripple " << ripple_db
        << " must be positive in decibels";

      const Real epsilon =
          std::sqrt(std::pow(Real(10), ripple_db / Real(10)) - Real(1));
      const Real mu = std::asinh(Real(1) / epsilon)
                      / static_cast<Real>(order);

      AnalogPrototype prototype;
      prototype.poles = Array<Complex>::create(order);
      for (size_t k = 0; k < order; ++k)
        {
          const Real theta =
              std::numbers::pi_v<Real>
              * (Real(2) * static_cast<Real>(k)
                 + Real(1))
              / (Real(2) * static_cast<Real>(order));
          prototype.poles(k) =
              Complex(-std::sinh(mu) * std::sin(theta),
                      std::cosh(mu) * std::cos(theta));
        }

      const Array<Real> denominator =
          real_polynomial_from_roots(prototype.poles, ctx);
      prototype.gain = denominator[denominator.size() - 1];
      if (order % 2 == 0)
        prototype.gain /= std::sqrt(Real(1) + epsilon * epsilon);

      return prototype;
    }

    [[nodiscard]] static AnalogPrototype
    chebyshev2_prototype(const size_t order,
                         const Real attenuation_db,
                         const char *ctx)
    {
      ah_invalid_argument_if(order == 0)
        << ctx << ": order must be positive";
      ah_invalid_argument_if(attenuation_db <= Real(0))
        << ctx << ": stopband attenuation " << attenuation_db
        << " must be positive in decibels";

      const Real epsilon =
          Real(1) / std::sqrt(std::pow(Real(10), attenuation_db / Real(10))
                              - Real(1));
      const Real mu = std::asinh(Real(1) / epsilon)
                      / static_cast<Real>(order);

      AnalogPrototype prototype;
      prototype.zeros.reserve(order);
      prototype.poles = Array<Complex>::create(order);
      for (size_t k = 0; k < order; ++k)
        {
          const Real theta =
              std::numbers::pi_v<Real>
              * (Real(2) * static_cast<Real>(k) + Real(1))
              / (Real(2) * static_cast<Real>(order));
          const Complex inverse_pole(-std::sinh(mu) * std::sin(theta),
                                     std::cosh(mu) * std::cos(theta));
          prototype.poles(k) = Complex(Real(1), Real(0)) / inverse_pole;
        }

      for (size_t k = 0; k < order / 2; ++k)
        {
          const Real theta =
              std::numbers::pi_v<Real>
              * (Real(2) * static_cast<Real>(k) + Real(1))
              / (Real(2) * static_cast<Real>(order));
          const Real imag = Real(1) / std::cos(theta);
          prototype.zeros.append(Complex(Real(0), imag));
          prototype.zeros.append(Complex(Real(0), -imag));
        }

      Array<Real> numerator = {Real(1)};
      if (not prototype.zeros.is_empty())
        numerator = real_polynomial_from_roots(prototype.zeros, ctx);
      const Array<Real> denominator =
          real_polynomial_from_roots(prototype.poles, ctx);
      prototype.gain =
          denominator[denominator.size() - 1] / numerator[numerator.size() - 1];
      return prototype;
    }

    [[nodiscard]] static AnalogPrototype
    elliptic_prototype(const size_t order,
                       const Real ripple_db,
                       const Real attenuation_db,
                       const char *ctx)
    {
      ah_invalid_argument_if(order == 0)
        << ctx << ": order must be positive";
      ah_invalid_argument_if(ripple_db <= Real(0))
        << ctx << ": passband ripple " << ripple_db
        << " must be positive in decibels";
      ah_invalid_argument_if(attenuation_db <= ripple_db)
        << ctx << ": stopband attenuation " << attenuation_db
        << " must exceed passband ripple " << ripple_db;

      const Real epsilon =
          std::sqrt(std::pow(Real(10), ripple_db / Real(10)) - Real(1));
      const Real stop_term =
          std::pow(Real(10), attenuation_db / Real(10)) - Real(1);
      ah_domain_error_if(stop_term <= Real(0))
        << ctx << ": stopband attenuation produced a non-positive stop term";

      const Real k1 = epsilon / std::sqrt(stop_term);
      const Real k1_tol = scaled_tolerance(k1, Real(4096));
      if (k1 <= k1_tol)
        return chebyshev1_prototype(order, ripple_db, ctx);

      const Real modulus = solve_elliptic_selectivity_modulus(k1, order, ctx);
      const Real modulus_tol = scaled_tolerance(modulus, Real(4096));
      if (modulus <= modulus_tol)
        return chebyshev1_prototype(order, ripple_db, ctx);

      const Real K = complete_elliptic_first_kind(modulus, ctx);
      const Real K1 = complete_elliptic_first_kind(k1, ctx);
      const Real k1_complement =
          std::sqrt(std::max(Real(0), Real(1) - k1 * k1));
      const Real r = inverse_jacobi_sc(Real(1) / epsilon,
                                       k1_complement,
                                       ctx);
      const Real v0 =
          K * r / (static_cast<Real>(order) * K1);

      AnalogPrototype prototype;
      prototype.zeros.reserve(order);
      prototype.poles.reserve(order);

      const size_t pair_count = order / 2;
      for (size_t i = 0; i < pair_count; ++i)
        {
          const Real u =
              (Real(2) * static_cast<Real>(i) + Real(1)) * K
              / static_cast<Real>(order);
          const JacobiValues values = jacobi_sn_cn_dn(u, modulus, ctx);
          const Real zero_scale = modulus * values.sn;
          ah_domain_error_if(std::abs(zero_scale)
                             <= scaled_tolerance(zero_scale, Real(4096)))
            << ctx << ": elliptic prototype zero scale vanished";
          const Real zero_imag = Real(1) / zero_scale;
          prototype.zeros.append(Complex(Real(0), zero_imag));
          prototype.zeros.append(Complex(Real(0), -zero_imag));

          Complex pole =
              Complex(Real(0), Real(1))
              * elliptic_cd_minus_imaginary(u, v0, modulus, ctx);
          if (pole.real() > Real(0))
            pole = -pole;
          if (pole.imag() < Real(0))
            pole = std::conj(pole);

          prototype.poles.append(pole);
          prototype.poles.append(std::conj(pole));
        }

      if ((order & 1) != 0)
        {
          const Real complement =
              std::sqrt(std::max(Real(0), Real(1) - modulus * modulus));
          const JacobiValues values = jacobi_sn_cn_dn(v0, complement, ctx);
          ah_domain_error_if(std::abs(values.cn)
                             <= scaled_tolerance(values.cn, Real(4096)))
            << ctx << ": elliptic prototype real pole denominator vanished";
          prototype.poles.append(Complex(-values.sn / values.cn, Real(0)));
        }

      Array<Real> numerator = {Real(1)};
      if (not prototype.zeros.is_empty())
        numerator = real_polynomial_from_roots(prototype.zeros, ctx);
      const Array<Real> denominator =
          real_polynomial_from_roots(prototype.poles, ctx);
      const Real target_gain =
          (order & 1) == 0 ?
            Real(1) / std::sqrt(Real(1) + epsilon * epsilon) :
            Real(1);
      prototype.gain =
          target_gain * denominator[denominator.size() - 1]
          / numerator[numerator.size() - 1];
      return prototype;
    }

    /** @brief Rationale: Direct evaluation of |H(j*omega)| for an analog transfer function. */
    [[nodiscard]] static Real
    evaluate_analog_transfer_magnitude(const Array<Real> & numerator,
                                       const Array<Real> & denominator,
                                       const Real omega,
                                       const char *ctx)
    {
      const PolynomialEvaluation num_eval =
          evaluate_polynomial_and_derivative(numerator,
                                             Complex(Real(0), omega),
                                             ctx);
      const PolynomialEvaluation den_eval =
          evaluate_polynomial_and_derivative(denominator,
                                             Complex(Real(0), omega),
                                             ctx);
      const Real den_tol = scaled_tolerance(max_abs_value(denominator),
                                            Real(4096));
      ah_domain_error_if(std::abs(den_eval.value) <= den_tol)
        << ctx << ": analog transfer response is singular at omega="
        << omega;
      return std::abs(num_eval.value / den_eval.value);
    }

    /** @brief Rationale: Generates coefficients for the Reverse Bessel Polynomial
     *         used in Bessel filter prototypes.
     */
    [[nodiscard]] static Array<Real>
    reverse_bessel_polynomial(const size_t order,
                              const char *ctx)
    {
      ah_invalid_argument_if(order == 0)
        << ctx << ": order must be positive";

      Array<Real> coeffs = Array<Real>::create(order + 1);
      for (size_t k = 0; k <= order; ++k)
        {
          const long double numerator =
              std::tgamma(static_cast<long double>(2 * order - k + 1));
          const long double denominator =
              std::pow(2.0L, static_cast<long double>(order - k))
              * std::tgamma(static_cast<long double>(order - k + 1))
              * std::tgamma(static_cast<long double>(k + 1));
          coeffs(order - k) =
              static_cast<Real>(numerator / denominator);
        }
      return coeffs;
    }

    /** @brief implementation of the Bessel filter analog prototype. */
    [[nodiscard]] static AnalogPrototype
    bessel_prototype(const size_t order,
                     const char *ctx)
    {
      const Array<Real> base_denominator =
          reverse_bessel_polynomial(order, ctx);
      const Real base_gain =
          base_denominator[base_denominator.size() - 1];
      const Real target = Real(1) / std::sqrt(Real(2));

      Real upper = Real(1);
      while (evaluate_analog_transfer_magnitude(Array<Real>({base_gain}),
                                                base_denominator,
                                                upper,
                                                ctx) > target)
        {
          upper *= Real(2);
          ah_runtime_error_if(upper > Real(1e12))
            << ctx << ": could not bracket the -3 dB normalization point";
        }

      Real lower = Real(0);
      for (size_t iter = 0; iter < 96; ++iter)
        {
          const Real mid = (lower + upper) / Real(2);
          const Real magnitude =
              evaluate_analog_transfer_magnitude(Array<Real>({base_gain}),
                                                 base_denominator,
                                                 mid,
                                                 ctx);
          if (magnitude > target)
            lower = mid;
          else
            upper = mid;
        }

      const Real scale = upper;
      Array<Complex> poles = polynomial_roots_impl(base_denominator, ctx);
      for (size_t i = 0; i < poles.size(); ++i)
        poles(i) *= scale;

      AnalogPrototype prototype;
      prototype.poles = std::move(poles);
      const Array<Real> scaled_denominator =
          real_polynomial_from_roots(prototype.poles, ctx);
      prototype.gain = scaled_denominator[scaled_denominator.size() - 1];
      return prototype;
    }

    /** @brief internal helper for designing standard lowpass/highpass SOS filters. */
    [[nodiscard]] static Array<BiquadSection>
    design_low_high_sections(const AnalogPrototype & prototype,
                             const Real cutoff_frequency,
                             const Real sample_rate,
                             const bool highpass,
                             const char *ctx)
    {
      const Real warped_cutoff =
          prewarp_frequency(cutoff_frequency, sample_rate, ctx);
      return design_transformed_sections(prototype,
                                         highpass ? Array<Real>({warped_cutoff}) : Array<Real>({Real(1), Real(0)}),
                                         highpass ? Array<Real>({Real(1), Real(0)}) : Array<Real>({warped_cutoff}),
                                         sample_rate,
                                         ctx);
    }

    [[nodiscard]] static Array<BiquadSection>
    design_prototype_sections(const AnalogPrototype & prototype,
                              const size_t order,
                              const Real cutoff_frequency,
                              const Real sample_rate,
                              const bool highpass,
                              const char *ctx)
    {
      ah_invalid_argument_if(prototype.poles.size() != order)
        << ctx << ": prototype order " << prototype.poles.size()
        << " does not match requested order " << order;

      const Real warped_cutoff =
          prewarp_frequency(cutoff_frequency, sample_rate, ctx);
      const Array<Real> prototype_denominator =
          real_polynomial_from_roots(prototype.poles, ctx);
      const Real prototype_constant =
          prototype_denominator[prototype_denominator.size() - 1];
      const Real first_section_gain = highpass ?
                                        prototype.gain / prototype_constant :
                                        prototype.gain * integer_power(warped_cutoff, order);

      Array<Complex> transformed_poles = Array<Complex>::create(order);
      for (size_t i = 0; i < order; ++i)
        transformed_poles(i) = highpass ?
                                 Complex(warped_cutoff, Real(0)) / prototype.poles[i] :
                                 Complex(warped_cutoff, Real(0)) * prototype.poles[i];

      const Array<RootGroup> pole_groups =
          root_groups_impl(transformed_poles, ctx);
      Array<BiquadSection> sections;
      sections.reserve(pole_groups.size());
      for (size_t i = 0; i < pole_groups.size(); ++i)
        {
          const size_t degree = pole_groups[i].roots.size();
          const Array<Real> analog_denominator =
              real_polynomial_from_roots(pole_groups[i].roots, ctx);
          Array<Real> analog_numerator = Array<Real>::create(degree + 1);
          for (size_t j = 0; j < analog_numerator.size(); ++j)
            analog_numerator(j) = Real(0);

          if (highpass)
            analog_numerator(0) = i == 0 ? first_section_gain : Real(1);
          else
            analog_numerator(degree) = i == 0 ? first_section_gain : Real(1);

          const IIRCoefficients digital =
              bilinear_transform_impl(analog_numerator,
                                      analog_denominator,
                                      sample_rate,
                                      ctx);
          sections.append(section_from_coefficients(digital.numerator,
                                                    digital.denominator,
                                                    ctx));
        }

      return sections;
    }

    /** @brief Prewarps a pair of band edges for bilinear transform. */
    [[nodiscard]] static std::pair<Real, Real>
    prewarp_band_edges(const Real low_cutoff_frequency,
                       const Real high_cutoff_frequency,
                       const Real sample_rate,
                       const char *ctx)
    {
      ah_invalid_argument_if(low_cutoff_frequency <= Real(0))
        << ctx << ": low cutoff " << low_cutoff_frequency
        << " must be positive";
      ah_invalid_argument_if(high_cutoff_frequency <= low_cutoff_frequency)
        << ctx << ": high cutoff " << high_cutoff_frequency
        << " must be greater than low cutoff " << low_cutoff_frequency;
      ah_invalid_argument_if(high_cutoff_frequency >= sample_rate / Real(2))
        << ctx << ": high cutoff " << high_cutoff_frequency
        << " must be smaller than Nyquist " << (sample_rate / Real(2));

      return {
            prewarp_frequency(low_cutoff_frequency, sample_rate, ctx),
            prewarp_frequency(high_cutoff_frequency, sample_rate, ctx)
          };
    }

    /** @brief Rationale: Decomposes a general IIR transfer function into a cascade
     *         of Second-Order Sections (SOS) by pairing poles and zeros.
     */
    [[nodiscard]] static Array<BiquadSection>
    transfer_function_to_sections(const IIRCoefficients & coeffs,
                                  const Array<RootGroup> & zero_groups,
                                  const char *ctx)
    {
      const Array<Complex> pole_roots = poles(coeffs.denominator);
      const Array<RootGroup> pole_groups = root_groups_impl(pole_roots, ctx);

      Array<bool> zero_used(zero_groups.size(), false);
      Array<BiquadSection> sections;
      sections.reserve(pole_groups.size());
      for (size_t i = 0; i < pole_groups.size(); ++i)
        {
          size_t best = zero_groups.size();
          Real best_cost = std::numeric_limits<Real>::infinity();
          for (size_t j = 0; j < zero_groups.size(); ++j)
            {
              if (zero_used[j]
                  or zero_groups[j].roots.size() > pole_groups[i].roots.size())
                continue;

              const Real size_penalty =
                  zero_groups[j].roots.size() == pole_groups[i].roots.size() ? Real(0) : Real(16);
              const Real cost =
                  std::abs(zero_groups[j].center - pole_groups[i].center)
                  + size_penalty;
              if (cost < best_cost)
                {
                  best = j;
                  best_cost = cost;
                }
            }

          Array<Real> section_numerator = {Real(1)};
          if (best != zero_groups.size())
            {
              section_numerator =
                  real_polynomial_from_roots(zero_groups[best].roots, ctx);
              zero_used(best) = true;
            }

          Array<Real> section_denominator =
              real_polynomial_from_roots(pole_groups[i].roots, ctx);
          if (i == 0)
            for (size_t k = 0; k < section_numerator.size(); ++k)
              section_numerator(k) *= coeffs.numerator[0];

          sections.append(section_from_coefficients(section_numerator,
                                                    section_denominator,
                                                    ctx));
        }

      for (size_t i = 0; i < zero_groups.size(); ++i)
        ah_invalid_argument_if(not zero_used[i])
          << ctx << ": could not assign finite-zero group " << i
          << " to any SOS section";

      return sections;
    }

    /** @brief overload of @ref transfer_function_to_sections using raw coefficients. */
    [[nodiscard]] static Array<BiquadSection>
    transfer_function_to_sections(const Array<Real> & numerator,
                                  const Array<Real> & denominator,
                                  const char *ctx)
    {
      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, ctx);
      const Array<Complex> zero_roots = zeros(coeffs.numerator);
      const Array<RootGroup> zero_groups =
          zero_roots.is_empty() ? Array<RootGroup>() : root_groups_impl(zero_roots, ctx);
      return transfer_function_to_sections(coeffs, zero_groups, ctx);
    }

    /** @brief Specialization for band-stop designs to handle zeros explicitly. */
    [[nodiscard]] static Array<BiquadSection>
    design_bandstop_sections_without_numerator_roots(const AnalogPrototype & prototype,
                                                     const size_t order,
                                                     const Real low_cutoff_frequency,
                                                     const Real high_cutoff_frequency,
                                                     const Real sample_rate,
                                                     const char *ctx)
    {
      ah_invalid_argument_if(not prototype.zeros.is_empty())
        << ctx << ": specialized band-stop path expects prototypes without zeros";

      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             ctx);
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      const Array<Real> prototype_denominator =
          real_polynomial_from_roots(prototype.poles, ctx);
      const IIRCoefficients analog =
          apply_analog_rational_transform(Array<Real>({prototype.gain}),
                                          prototype_denominator,
                                          {bandwidth, Real(0)},
                                          {Real(1), Real(0), center * center},
                                          ctx);
      const IIRCoefficients digital =
          bilinear_transform_impl(analog.numerator,
                                  analog.denominator,
                                  sample_rate,
                                  ctx);

      const Complex analog_zero(Real(0), center);
      const Complex digital_zero =
          (Complex(Real(2) * sample_rate, Real(0)) - analog_zero)
          / (Complex(Real(2) * sample_rate, Real(0)) + analog_zero);

      Array<RootGroup> zero_groups;
      zero_groups.reserve(order);
      for (size_t i = 0; i < order; ++i)
        {
          RootGroup group;
          group.roots = {digital_zero, std::conj(digital_zero)};
          group.center = group_center(group.roots);
          zero_groups.append(std::move(group));
        }

      return transfer_function_to_sections(digital, zero_groups, ctx);
    }

    /** @brief Generic implementation for designing transformed SOS cascades. */
    [[nodiscard]] static Array<BiquadSection>
    design_transformed_sections(const AnalogPrototype & prototype,
                                const Array<Real> & map_numerator,
                                const Array<Real> & map_denominator,
                                const Real sample_rate,
                                const char *ctx)
    {
      Array<Real> prototype_numerator = {prototype.gain};
      if (not prototype.zeros.is_empty())
        {
          prototype_numerator =
              real_polynomial_from_roots(prototype.zeros, ctx);
          for (size_t i = 0; i < prototype_numerator.size(); ++i)
            prototype_numerator(i) *= prototype.gain;
        }
      const Array<Real> prototype_denominator =
          real_polynomial_from_roots(prototype.poles, ctx);
      const IIRCoefficients analog =
          apply_analog_rational_transform(prototype_numerator,
                                          prototype_denominator,
                                          map_numerator,
                                          map_denominator,
                                          ctx);
      const IIRCoefficients digital =
          bilinear_transform_impl(analog.numerator,
                                  analog.denominator,
                                  sample_rate,
                                  ctx);
      return transfer_function_to_sections(digital.numerator,
                                           digital.denominator,
                                           ctx);
    }

    /** @brief Standard Zeroth-order Modified Bessel function of the first kind. */
    [[nodiscard]] static Real
    modified_bessel_i0(const Real x) noexcept
    {
      const Real half_x = x / Real(2);
      const Real half_x_sq = half_x * half_x;
      Real sum = Real(1);
      Real term = Real(1);
      for (size_t k = 1; k < 80; ++k)
        {
          term *= half_x_sq
              / (static_cast<Real>(k) * static_cast<Real>(k));
          sum += term;
          if (std::abs(term) <= std::numeric_limits<Real>::epsilon() * sum)
            break;
        }
      return sum;
    }

    /** @brief Standard normalized sinc function sin(pi*x)/(pi*x). */
    [[nodiscard]] static Real
    normalized_sinc(const Real x) noexcept
    {
      const Real pix = std::numbers::pi_v<Real> * x;
      if (std::abs(pix) <= std::numeric_limits<Real>::epsilon())
        return Real(1);
      return std::sin(pix) / pix;
    }

    /** @brief implementation helper for FIR frequency response magnitude. */
    [[nodiscard]] static Real
    evaluate_fir_response_magnitude(const Array<Real> & coeffs,
                                    const Real omega,
                                    const char *ctx)
    {
      return std::abs(evaluate_transfer_at(coeffs,
                                           Array<Real>({Real(1)}),
                                           omega,
                                           ctx));
    }

    /** @brief Normalizes FIR coefficients to have unit gain at frequency @p omega. */
    static void
    normalize_fir_at_omega(Array<Real> & coeffs,
                           const Real omega,
                           const char *ctx)
    {
      const Real gain = evaluate_fir_response_magnitude(coeffs, omega, ctx);
      const Real tol = scaled_tolerance(gain, Real(4096));
      ah_domain_error_if(gain <= tol)
        << ctx << ": FIR gain vanished at normalization omega=" << omega;
      for (size_t i = 0; i < coeffs.size(); ++i)
        coeffs(i) /= gain;
    }

    /** @brief implementation of the window-method FIR design. */
    [[nodiscard]] static Array<Real>
    firwin_lowpass_impl(const size_t num_taps,
                        const Real cutoff_frequency,
                        const Real sample_rate,
                        const Array<Real> & window,
                        const char *ctx)
    {
      ah_invalid_argument_if(num_taps == 0)
        << ctx << ": num_taps must be positive";
      ah_invalid_argument_if(window.size() != num_taps)
        << ctx << ": window size " << window.size()
        << " does not match num_taps " << num_taps;
      ah_invalid_argument_if(sample_rate <= Real(0))
        << ctx << ": sample rate " << sample_rate << " must be positive";
      ah_invalid_argument_if(cutoff_frequency <= Real(0))
        << ctx << ": cutoff frequency " << cutoff_frequency
        << " must be positive";
      ah_invalid_argument_if(cutoff_frequency >= sample_rate / Real(2))
        << ctx << ": cutoff frequency " << cutoff_frequency
        << " must be smaller than Nyquist " << (sample_rate / Real(2));

      const Real normalized_cutoff = cutoff_frequency / sample_rate;
      const Real center = static_cast<Real>(num_taps - 1) / Real(2);
      Array<Real> coeffs = Array<Real>::create(num_taps);
      for (size_t n = 0; n < num_taps; ++n)
        {
          const Real m = static_cast<Real>(n) - center;
          const Real ideal =
              Real(2) * normalized_cutoff
              * normalized_sinc(Real(2) * normalized_cutoff * m);
          coeffs(n) = ideal * window[n];
        }

      normalize_fir_at_omega(coeffs, Real(0), ctx);
      return coeffs;
    }

    /** @brief Rationale: Evaluates integral of cos(k*w) from omega_lo to omega_hi. */
    [[nodiscard]] static Real
    integrate_cos_basis(const Real omega_lo,
                        const Real omega_hi,
                        const size_t harmonic) noexcept
    {
      if (harmonic == 0)
        return omega_hi - omega_lo;

      const Real k = static_cast<Real>(harmonic);
      return (std::sin(k * omega_hi) - std::sin(k * omega_lo)) / k;
    }

    /** @brief Rationale: Evaluates integral of cos(m*w)*cos(n*w). */
    [[nodiscard]] static Real
    integrate_cos_product(const Real omega_lo,
                          const Real omega_hi,
                          const size_t lhs_harmonic,
                          const size_t rhs_harmonic) noexcept
    {
      const size_t delta = lhs_harmonic >= rhs_harmonic ? lhs_harmonic - rhs_harmonic : rhs_harmonic - lhs_harmonic;
      return Real(0.5) * (integrate_cos_basis(omega_lo, omega_hi, delta)
                          + integrate_cos_basis(omega_lo,
                                                omega_hi,
                                                lhs_harmonic + rhs_harmonic));
    }

    /** @brief implementation helper for firls slope integration. */
    [[nodiscard]] static Real
    integrate_offset_cos_basis(const Real omega_lo,
                               const Real omega_hi,
                               const size_t harmonic) noexcept
    {
      const Real width = omega_hi - omega_lo;
      if (harmonic == 0)
        return Real(0.5) * width * width;

      const Real k = static_cast<Real>(harmonic);
      return width * std::sin(k * omega_hi) / k
             + (std::cos(k * omega_hi) - std::cos(k * omega_lo)) / (k * k);
    }

    /** @brief implementation helper for firls linear band integration. */
    [[nodiscard]] static Real
    integrate_linear_cos_basis(const Real omega_lo,
                               const Real omega_hi,
                               const Real desired_lo,
                               const Real desired_hi,
                               const size_t harmonic) noexcept
    {
      const Real width = omega_hi - omega_lo;
      if (width <= Real(0))
        return Real(0);

      const Real slope = (desired_hi - desired_lo) / width;
      return desired_lo * integrate_cos_basis(omega_lo, omega_hi, harmonic)
             + slope * integrate_offset_cos_basis(omega_lo,
                                                  omega_hi,
                                                  harmonic);
    }

    /** @brief implementation of Least-Squares FIR design. */
    [[nodiscard]] static Array<Real>
    firls_impl(const size_t num_taps,
               const Array<Real> & bands,
               const Array<Real> & desired,
               const Real sample_rate,
               const Array<Real> & weights,
               const char *ctx)
    {
      ah_invalid_argument_if(num_taps == 0)
        << ctx << ": num_taps must be positive";
      ah_invalid_argument_if((num_taps & 1) == 0)
        << ctx << ": num_taps must be odd for a Type-I linear-phase design";
      ah_invalid_argument_if(sample_rate <= Real(0))
        << ctx << ": sample rate " << sample_rate << " must be positive";
      ah_invalid_argument_if(bands.is_empty() or (bands.size() & 1) != 0)
        << ctx << ": bands must contain an even number of band edges";
      ah_invalid_argument_if(desired.size() != bands.size())
        << ctx << ": desired size " << desired.size()
        << " does not match band edge count " << bands.size();

      const size_t band_count = bands.size() / 2;
      ah_invalid_argument_if(not weights.is_empty() and weights.size() != band_count)
        << ctx << ": weights size " << weights.size()
        << " does not match band count " << band_count;

      const Real nyquist = sample_rate / Real(2);
      const Real edge_tol = scaled_tolerance(sample_rate, Real(1024));

      ah_invalid_argument_if(std::abs(bands[0]) > edge_tol)
        << ctx << ": first band edge " << bands[0] << " must start at 0";
      ah_invalid_argument_if(std::abs(bands[bands.size() - 1] - nyquist) > edge_tol)
        << ctx << ": last band edge " << bands[bands.size() - 1]
        << " must reach Nyquist " << nyquist;

      Array<Real> normalized_weights;
      normalized_weights.reserve(band_count);
      if (weights.is_empty())
        for (size_t i = 0; i < band_count; ++i)
          normalized_weights.append(Real(1));
      else
        for (size_t i = 0; i < band_count; ++i)
          {
            ah_invalid_argument_if(weights[i] <= Real(0))
              << ctx << ": weight " << weights[i]
              << " at band " << i << " must be positive";
            normalized_weights.append(weights[i]);
          }

      Array<Real> omega = Array<Real>::create(bands.size());
      Real previous_edge = Real(0);
      for (size_t i = 0; i < bands.size(); ++i)
        {
          const Real edge = i == 0 ? Real(0) : (i + 1 == bands.size() ? nyquist : bands[i]);
          ah_invalid_argument_if(edge < -edge_tol or edge > nyquist + edge_tol)
            << ctx << ": band edge " << edge << " lies outside [0, Nyquist]";
          ah_invalid_argument_if(i > 0 and edge + edge_tol < previous_edge)
            << ctx << ": band edges must be non-decreasing";
          if ((i & 1) == 1)
            ah_invalid_argument_if(edge <= previous_edge + edge_tol)
              << ctx << ": band " << (i / 2)
              << " has non-positive width";

          omega(i) = Real(2) * std::numbers::pi_v<Real> * edge / sample_rate;
          previous_edge = edge;
        }

      const size_t order = (num_taps - 1) / 2;
      const size_t dimension = order + 1;
      Array<Real> system = Array<Real>::create(dimension * dimension);
      Array<Real> rhs = Array<Real>::create(dimension);
      for (size_t i = 0; i < system.size(); ++i)
        system(i) = Real(0);
      for (size_t i = 0; i < rhs.size(); ++i)
        rhs(i) = Real(0);

      for (size_t band = 0; band < band_count; ++band)
        {
          const Real omega_lo = omega[band * 2];
          const Real omega_hi = omega[band * 2 + 1];
          const Real desired_lo = desired[band * 2];
          const Real desired_hi = desired[band * 2 + 1];
          const Real weight = normalized_weights[band];

          for (size_t row = 0; row < dimension; ++row)
            {
              rhs(row) += weight * integrate_linear_cos_basis(omega_lo,
                                                              omega_hi,
                                                              desired_lo,
                                                              desired_hi,
                                                              row);
              for (size_t col = row; col < dimension; ++col)
                {
                  const Real contribution =
                      weight * integrate_cos_product(omega_lo,
                                                     omega_hi,
                                                     row,
                                                     col);
                  system(row * dimension + col) += contribution;
                  if (col != row)
                    system(col * dimension + row) += contribution;
                }
            }
        }

      const Array<Real> cosine_series =
          solve_dense_system(system, rhs, dimension, ctx);
      Array<Real> coeffs = Array<Real>::create(num_taps);
      for (size_t i = 0; i < coeffs.size(); ++i)
        coeffs(i) = Real(0);

      const size_t center = order;
      coeffs(center) = cosine_series[0];
      for (size_t k = 1; k < cosine_series.size(); ++k)
        {
          const Real tap = cosine_series[k] / Real(2);
          coeffs(center - k) = tap;
          coeffs(center + k) = tap;
        }

      return coeffs;
    }

    [[nodiscard]] static WeightedFrequencyGrid
    build_weighted_frequency_grid(const size_t num_taps,
                                  const Array<Real> & bands,
                                  const Array<Real> & desired,
                                  const Real sample_rate,
                                  const Array<Real> & weights,
                                  const size_t grid_density,
                                  const char *ctx)
    {
      ah_invalid_argument_if(num_taps == 0)
        << ctx << ": num_taps must be positive";
      ah_invalid_argument_if((num_taps & 1) == 0)
        << ctx << ": num_taps must be odd for a Type-I linear-phase design";
      ah_invalid_argument_if(sample_rate <= Real(0))
        << ctx << ": sample rate " << sample_rate << " must be positive";
      ah_invalid_argument_if(grid_density < 8)
        << ctx << ": grid density " << grid_density
        << " must be at least 8";
      ah_invalid_argument_if(bands.is_empty() or (bands.size() & 1) != 0)
        << ctx << ": bands must contain an even number of band edges";
      ah_invalid_argument_if(desired.size() != bands.size())
        << ctx << ": desired size " << desired.size()
        << " does not match band edge count " << bands.size();

      const size_t band_count = bands.size() / 2;
      ah_invalid_argument_if(not weights.is_empty() and weights.size() != band_count)
        << ctx << ": weights size " << weights.size()
        << " does not match band count " << band_count;

      const Real nyquist = sample_rate / Real(2);
      const Real edge_tol = scaled_tolerance(sample_rate, Real(1024));
      ah_invalid_argument_if(std::abs(bands[0]) > edge_tol)
        << ctx << ": first band edge " << bands[0] << " must start at 0";
      ah_invalid_argument_if(std::abs(bands[bands.size() - 1] - nyquist) > edge_tol)
        << ctx << ": last band edge " << bands[bands.size() - 1]
        << " must reach Nyquist " << nyquist;

      WeightedFrequencyGrid grid;
      const size_t half_order = (num_taps - 1) / 2;
      const size_t target_points = grid_density * (half_order + 2);

      Real previous_edge = Real(0);
      for (size_t band = 0; band < band_count; ++band)
        {
          const Real low = bands[band * 2];
          const Real high = bands[band * 2 + 1];
          ah_invalid_argument_if(low < -edge_tol or high > nyquist + edge_tol)
            << ctx << ": band edges must lie in [0, Nyquist]";
          ah_invalid_argument_if(low + edge_tol < previous_edge)
            << ctx << ": band edges must be non-decreasing";
          ah_invalid_argument_if(high <= low + edge_tol)
            << ctx << ": band " << band << " has non-positive width";

          const Real desired_low = desired[band * 2];
          const Real desired_high = desired[band * 2 + 1];
          const Real weight = weights.is_empty() ? Real(1) : weights[band];
          ah_invalid_argument_if(weight <= Real(0))
            << ctx << ": weight " << weight << " at band " << band
            << " must be positive";

          const Real omega_low =
              Real(2) * std::numbers::pi_v<Real> * low / sample_rate;
          const Real omega_high =
              Real(2) * std::numbers::pi_v<Real> * high / sample_rate;
          const Real omega_width = omega_high - omega_low;
          size_t sample_count =
              static_cast<size_t>(std::ceil(target_points
                                            * omega_width
                                            / std::numbers::pi_v<Real>));
          sample_count = std::max<size_t>(sample_count, 2);

          for (size_t i = 0; i < sample_count; ++i)
            {
              if (band > 0 and i == 0)
                continue;

              const Real t =
                  sample_count == 1 ?
                    Real(0) :
                    static_cast<Real>(i)
                    / static_cast<Real>(sample_count - 1);
              const Real omega = omega_low + t * omega_width;
              const Real desired_value =
                  desired_low + t * (desired_high - desired_low);
              grid.omega.append(omega);
              grid.desired.append(desired_value);
              grid.weight.append(weight);
            }

          previous_edge = high;
        }

      ah_runtime_error_if(grid.omega.size() < half_order + 2)
        << ctx << ": weighted frequency grid is too small";
      return grid;
    }

    [[nodiscard]] static Real
    evaluate_cosine_series(const Array<Real> & coefficients,
                           const Real omega) noexcept
    {
      Real value = Real(0);
      for (size_t k = 0; k < coefficients.size(); ++k)
        value += coefficients[k] * std::cos(static_cast<Real>(k) * omega);
      return value;
    }

    [[nodiscard]] static Array<size_t>
    evenly_spaced_extrema(const size_t grid_size,
                          const size_t extremal_count)
    {
      Array<size_t> extrema;
      extrema.reserve(extremal_count);
      for (size_t i = 0; i < extremal_count; ++i)
        {
          const Real position =
              extremal_count == 1 ?
                Real(0) :
                static_cast<Real>(i)
                * static_cast<Real>(grid_size - 1)
                / static_cast<Real>(extremal_count - 1);
          extrema.append(static_cast<size_t>(std::llround(position)));
        }
      return extrema;
    }

    [[nodiscard]] static Array<size_t>
    choose_remez_extrema(const Array<Real> & weighted_error,
                         const size_t required_count,
                         const char *ctx)
    {
      ah_invalid_argument_if(weighted_error.size() < required_count)
        << ctx << ": weighted error grid has only " << weighted_error.size()
        << " points for " << required_count << " required extrema";

      struct Candidate
      {
        size_t index = 0;
        Real magnitude = Real(0);
        int sign = 0;
      };

      Array<Candidate> candidates;
      candidates.reserve(weighted_error.size());
      for (size_t i = 0; i < weighted_error.size(); ++i)
        {
          const Real current = weighted_error[i];
          const Real current_abs = std::abs(current);
          if (current_abs <= scaled_tolerance(current_abs, Real(4096)))
            continue;

          const Real left_abs =
              i == 0 ? Real(-1) : std::abs(weighted_error[i - 1]);
          const Real right_abs =
              i + 1 == weighted_error.size() ? Real(-1) : std::abs(weighted_error[i + 1]);
          if (current_abs + scaled_tolerance(current_abs, Real(256)) < left_abs
              or current_abs + scaled_tolerance(current_abs, Real(256)) < right_abs)
            continue;

          Candidate candidate;
          candidate.index = i;
          candidate.magnitude = current_abs;
          candidate.sign = current >= Real(0) ? 1 : -1;
          candidates.append(candidate);
        }

      if (candidates.size() < required_count)
        return evenly_spaced_extrema(weighted_error.size(), required_count);

      Array<Candidate> compressed;
      compressed.reserve(candidates.size());
      compressed.append(candidates[0]);
      for (size_t i = 1; i < candidates.size(); ++i)
        {
          if (candidates[i].sign == compressed[compressed.size() - 1].sign)
            {
              if (candidates[i].magnitude > compressed[compressed.size() - 1].magnitude)
                compressed(compressed.size() - 1) = candidates[i];
            }
          else
            compressed.append(candidates[i]);
        }

      if (compressed.size() < required_count)
        return evenly_spaced_extrema(weighted_error.size(), required_count);
      if (compressed.size() == required_count)
        {
          Array<size_t> extrema;
          extrema.reserve(required_count);
          for (size_t i = 0; i < compressed.size(); ++i)
            extrema.append(compressed[i].index);
          return extrema;
        }

      size_t best_start = 0;
      Real best_floor = Real(-1);
      Real best_sum = Real(-1);
      for (size_t start = 0; start + required_count <= compressed.size(); ++start)
        {
          Real floor = std::numeric_limits<Real>::infinity();
          Real sum = Real(0);
          for (size_t i = 0; i < required_count; ++i)
            {
              floor = std::min(floor, compressed[start + i].magnitude);
              sum += compressed[start + i].magnitude;
            }

          if (floor > best_floor
              or (std::abs(floor - best_floor)
                  <= scaled_tolerance(floor, Real(4096))
                  and sum > best_sum))
            {
              best_floor = floor;
              best_sum = sum;
              best_start = start;
            }
        }

      Array<size_t> extrema;
      extrema.reserve(required_count);
      for (size_t i = 0; i < required_count; ++i)
        extrema.append(compressed[best_start + i].index);
      return extrema;
    }

    [[nodiscard]] static Array<Real>
    solve_remez_cosine_series(const Array<Real> & omega,
                              const Array<Real> & desired,
                              const Array<Real> & weight,
                              const Array<size_t> & extrema,
                              const size_t half_order,
                              const char *ctx)
    {
      const size_t dimension = half_order + 2;
      ah_invalid_argument_if(extrema.size() != dimension)
        << ctx << ": expected " << dimension << " extrema but got "
        << extrema.size();

      Array<Real> matrix = Array<Real>::create(dimension * dimension);
      Array<Real> rhs = Array<Real>::create(dimension);
      for (size_t i = 0; i < matrix.size(); ++i)
        matrix(i) = Real(0);
      for (size_t i = 0; i < rhs.size(); ++i)
        rhs(i) = Real(0);

      for (size_t row = 0; row < dimension; ++row)
        {
          const size_t index = extrema[row];
          for (size_t col = 0; col <= half_order; ++col)
            matrix(row * dimension + col) =
                std::cos(static_cast<Real>(col) * omega[index]);
          matrix(row * dimension + half_order + 1) =
              ((row & 1) == 0 ? Real(1) : Real(-1)) / weight[index];
          rhs(row) = desired[index];
        }

      return solve_dense_system(matrix, rhs, dimension, ctx);
    }

    [[nodiscard]] static Array<Real>
    remez_impl(const size_t num_taps,
               const Array<Real> & bands,
               const Array<Real> & desired,
               const Real sample_rate,
               const Array<Real> & weights,
               const size_t grid_density,
               const size_t max_iterations,
               const char *ctx)
    {
      ah_invalid_argument_if(max_iterations == 0)
        << ctx << ": max_iterations must be positive";

      const WeightedFrequencyGrid grid =
          build_weighted_frequency_grid(num_taps,
                                        bands,
                                        desired,
                                        sample_rate,
                                        weights,
                                        grid_density,
                                        ctx);
      const size_t half_order = (num_taps - 1) / 2;
      const size_t required_extrema = half_order + 2;
      Array<size_t> extrema =
          evenly_spaced_extrema(grid.omega.size(), required_extrema);

      Array<Real> cosine = Array<Real>::create(half_order + 1);
      for (size_t i = 0; i < cosine.size(); ++i)
        cosine(i) = Real(0);

      for (size_t iter = 0; iter < max_iterations; ++iter)
        {
          const Array<Real> solution =
              solve_remez_cosine_series(grid.omega,
                                        grid.desired,
                                        grid.weight,
                                        extrema,
                                        half_order,
                                        ctx);
          for (size_t i = 0; i < cosine.size(); ++i)
            cosine(i) = solution[i];

          Array<Real> weighted_error = Array<Real>::create(grid.omega.size());
          for (size_t i = 0; i < grid.omega.size(); ++i)
            {
              const Real approx =
                  evaluate_cosine_series(cosine, grid.omega[i]);
              weighted_error(i) =
                  grid.weight[i] * (approx - grid.desired[i]);
            }

          const Array<size_t> next_extrema =
              choose_remez_extrema(weighted_error,
                                   required_extrema,
                                   ctx);
          bool unchanged = next_extrema.size() == extrema.size();
          for (size_t i = 0; unchanged and i < extrema.size(); ++i)
            unchanged = next_extrema[i] == extrema[i];
          extrema = next_extrema;
          if (unchanged)
            break;
        }

      Array<Real> coeffs = Array<Real>::create(num_taps);
      for (size_t i = 0; i < coeffs.size(); ++i)
        coeffs(i) = Real(0);

      const size_t center = half_order;
      coeffs(center) = cosine[0];
      for (size_t k = 1; k < cosine.size(); ++k)
        {
          const Real tap = cosine[k] / Real(2);
          coeffs(center - k) = tap;
          coeffs(center + k) = tap;
        }

      return coeffs;
    }

    [[nodiscard]] static Real
    interpolate_crossing(const Real x0,
                         const Real y0,
                         const Real x1,
                         const Real y1,
                         const Real target) noexcept
    {
      const Real dy = y1 - y0;
      if (std::abs(dy) <= std::numeric_limits<Real>::epsilon())
        return x0;
      const Real t = (target - y0) / dy;
      return x0 + t * (x1 - x0);
    }

    [[nodiscard]] static Real
    interpolate_value_at(const Real x0,
                         const Real y0,
                         const Real x1,
                         const Real y1,
                         const Real x) noexcept
    {
      const Real dx = x1 - x0;
      if (std::abs(dx) <= std::numeric_limits<Real>::epsilon())
        return y0;
      const Real t = (x - x0) / dx;
      return y0 + t * (y1 - y0);
    }

    [[nodiscard]] static Real
    align_phase_near_reference(const Real raw_phase,
                               const Real reference_phase) noexcept
    {
      const Real turns =
          std::round((reference_phase - raw_phase)
                     / (Real(2) * std::numbers::pi_v<Real>));
      return raw_phase + turns * Real(2) * std::numbers::pi_v<Real>;
    }

    template <typename Evaluator>
    [[nodiscard]] static Real
    refine_scalar_crossing(const Evaluator & evaluator,
                           const Real x0,
                           const Real y0,
                           const Real x1,
                           const Real y1,
                           const Real target)
    {
      Real left_x = x0;
      Real right_x = x1;
      Real left_y = y0 - target;
      Real right_y = y1 - target;

      if (std::abs(left_y) <= std::numeric_limits<Real>::epsilon())
        return left_x;
      if (std::abs(right_y) <= std::numeric_limits<Real>::epsilon())
        return right_x;
      if ((left_y < Real(0) and right_y < Real(0))
          or (left_y > Real(0) and right_y > Real(0)))
        return interpolate_crossing(x0, y0, x1, y1, target);

      const Real xtol =
          scaled_tolerance(std::max(std::abs(x0), std::abs(x1)), Real(4096));
      for (size_t iter = 0; iter < 64; ++iter)
        {
          const Real mid_x = (left_x + right_x) / Real(2);
          const Real mid_y = evaluator(mid_x) - target;
          if (std::abs(mid_y) <= std::numeric_limits<Real>::epsilon()
              or std::abs(right_x - left_x) <= xtol)
            return mid_x;

          if ((left_y < Real(0) and mid_y < Real(0))
              or (left_y > Real(0) and mid_y > Real(0)))
            {
              left_x = mid_x;
              left_y = mid_y;
            }
          else
            {
              right_x = mid_x;
              right_y = mid_y;
            }
        }

      return (left_x + right_x) / Real(2);
    }

    [[nodiscard]] static Complex
    evaluate_sos_transfer_at(const Array<BiquadSection> & sections,
                             const Real omega,
                             const char *ctx)
    {
      ah_invalid_argument_if(sections.is_empty())
        << ctx << ": at least one biquad section is required";

      Complex response(Real(1), Real(0));
      for (size_t i = 0; i < sections.size(); ++i)
        response *= evaluate_transfer_at(sections[i].numerator(),
                                         sections[i].denominator(),
                                         omega,
                                         ctx);
      return response;
    }

    [[nodiscard]] static PhaseMarginInfo
    phase_margin_impl(const FrequencyResponse & response)
    {
      ah_invalid_argument_if(response.omega.size() != response.response.size())
        << "FFT::phase_margin: omega size " << response.omega.size()
        << " does not match response size " << response.response.size();

      PhaseMarginInfo info;
      if (response.omega.size() < 2)
        return info;

      const Array<Real> magnitude = response.magnitude();
      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      bool found = false;
      Real best_margin = std::numeric_limits<Real>::infinity();
      Real best_omega = std::numeric_limits<Real>::quiet_NaN();

      for (size_t i = 1; i < response.omega.size(); ++i)
        {
          const Real left = magnitude[i - 1] - Real(1);
          const Real right = magnitude[i] - Real(1);
          if ((left < Real(0) and right < Real(0))
              or (left > Real(0) and right > Real(0)))
            continue;

          const Real omega = interpolate_crossing(response.omega[i - 1],
                                                  left,
                                                  response.omega[i],
                                                  right,
                                                  Real(0));
          const Real phi = interpolate_value_at(response.omega[i - 1],
                                                phase[i - 1],
                                                response.omega[i],
                                                phase[i],
                                                omega);
          const Real margin = std::numbers::pi_v<Real> + phi;
          if (not found or margin < best_margin)
            {
              best_margin = margin;
              best_omega = omega;
              found = true;
            }
        }

      if (found)
        {
          info.radians = best_margin;
          info.degrees = best_margin * Real(180) / std::numbers::pi_v<Real>;
          info.crossover_omega = best_omega;
          info.found = true;
        }

      return info;
    }

    /** @brief implementation helper for Gain Margin calculation. */
    [[nodiscard]] static GainMarginInfo
    gain_margin_impl(const FrequencyResponse & response)
    {
      ah_invalid_argument_if(response.omega.size() != response.response.size())
        << "FFT::gain_margin: omega size " << response.omega.size()
        << " does not match response size " << response.response.size();

      GainMarginInfo info;
      if (response.omega.size() < 2)
        return info;

      const Array<Real> magnitude = response.magnitude();
      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      bool found = false;
      Real best_ratio = std::numeric_limits<Real>::infinity();
      Real best_omega = std::numeric_limits<Real>::quiet_NaN();

      for (size_t i = 1; i < response.omega.size(); ++i)
        {
          const Real p0 = phase[i - 1];
          const Real p1 = phase[i];
          const Real lower = std::min(p0, p1);
          const Real upper = std::max(p0, p1);
          const auto first_target =
              static_cast<long long>(std::ceil((-upper - std::numbers::pi_v<Real>)
                                               / (Real(2)
                                                  * std::numbers::pi_v<Real>)));
          const auto last_target =
              static_cast<long long>(std::floor((-lower - std::numbers::pi_v<Real>)
                                                / (Real(2)
                                                   * std::numbers::pi_v<Real>)));

          for (long long m = first_target; m <= last_target; ++m)
            {
              const Real target =
                  -(Real(2) * static_cast<Real>(m) + Real(1))
                  * std::numbers::pi_v<Real>;
              const Real omega = interpolate_crossing(response.omega[i - 1],
                                                      p0,
                                                      response.omega[i],
                                                      p1,
                                                      target);
              const Real mag = interpolate_value_at(response.omega[i - 1],
                                                    magnitude[i - 1],
                                                    response.omega[i],
                                                    magnitude[i],
                                                    omega);
              const Real ratio = std::abs(mag) <= std::numeric_limits<Real>::epsilon() ?
                                   std::numeric_limits<Real>::infinity() :
                                   Real(1) / std::abs(mag);
              if (not found or ratio < best_ratio)
                {
                  best_ratio = ratio;
                  best_omega = omega;
                  found = true;
                }
            }
        }

      if (found)
        {
          info.ratio = best_ratio;
          info.decibels = std::isinf(best_ratio) ?
                            std::numeric_limits<Real>::infinity() :
                            Real(20) * std::log10(best_ratio);
          info.crossover_omega = best_omega;
          info.found = true;
        }

      return info;
    }

    /** @brief implementation of refined Phase Margin calculation using analytic evaluation. */
    template <typename Evaluator>
    [[nodiscard]] static PhaseMarginInfo
    phase_margin_refined_impl(const FrequencyResponse & response,
                              const Evaluator & evaluator)
    {
      ah_invalid_argument_if(response.omega.size() != response.response.size())
        << "FFT::phase_margin: omega size " << response.omega.size()
        << " does not match response size " << response.response.size();

      PhaseMarginInfo info;
      if (response.omega.size() < 2)
        return info;

      const Array<Real> magnitude = response.magnitude();
      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      bool found = false;
      Real best_margin = std::numeric_limits<Real>::infinity();
      Real best_omega = std::numeric_limits<Real>::quiet_NaN();

      for (size_t i = 1; i < response.omega.size(); ++i)
        {
          const Real left = magnitude[i - 1] - Real(1);
          const Real right = magnitude[i] - Real(1);
          if ((left < Real(0) and right < Real(0))
              or (left > Real(0) and right > Real(0)))
            continue;

          const Real omega =
              refine_scalar_crossing([&](const Real w)
                                       {
                                         return std::abs(evaluator(w));
                                       },
                                     response.omega[i - 1],
                                     magnitude[i - 1],
                                     response.omega[i],
                                     magnitude[i],
                                     Real(1));
          const Real phase_ref =
              interpolate_value_at(response.omega[i - 1],
                                   phase[i - 1],
                                   response.omega[i],
                                   phase[i],
                                   omega);
          const Real phi =
              align_phase_near_reference(std::arg(evaluator(omega)), phase_ref);
          const Real margin = std::numbers::pi_v<Real> + phi;
          if (not found or margin < best_margin)
            {
              best_margin = margin;
              best_omega = omega;
              found = true;
            }
        }

      if (found)
        {
          info.radians = best_margin;
          info.degrees = best_margin * Real(180) / std::numbers::pi_v<Real>;
          info.crossover_omega = best_omega;
          info.found = true;
        }

      return info;
    }

    /** @brief implementation of refined Gain Margin calculation using analytic evaluation. */
    template <typename Evaluator>
    [[nodiscard]] static GainMarginInfo
    gain_margin_refined_impl(const FrequencyResponse & response,
                             const Evaluator & evaluator)
    {
      ah_invalid_argument_if(response.omega.size() != response.response.size())
        << "FFT::gain_margin: omega size " << response.omega.size()
        << " does not match response size " << response.response.size();

      GainMarginInfo info;
      if (response.omega.size() < 2)
        return info;

      const Array<Real> magnitude = response.magnitude();
      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      bool found = false;
      Real best_ratio = std::numeric_limits<Real>::infinity();
      Real best_omega = std::numeric_limits<Real>::quiet_NaN();

      for (size_t i = 1; i < response.omega.size(); ++i)
        {
          const Real p0 = phase[i - 1];
          const Real p1 = phase[i];
          const Real lower = std::min(p0, p1);
          const Real upper = std::max(p0, p1);
          const auto first_target =
              static_cast<long long>(std::ceil((-upper - std::numbers::pi_v<Real>)
                                               / (Real(2)
                                                  * std::numbers::pi_v<Real>)));
          const auto last_target =
              static_cast<long long>(std::floor((-lower - std::numbers::pi_v<Real>)
                                                / (Real(2)
                                                   * std::numbers::pi_v<Real>)));

          for (long long m = first_target; m <= last_target; ++m)
            {
              const Real target =
                  -(Real(2) * static_cast<Real>(m) + Real(1))
                  * std::numbers::pi_v<Real>;
              const Real omega =
                  refine_scalar_crossing([&](const Real w)
                                           {
                                             const Real reference =
                                                 interpolate_value_at(response.omega[i - 1],
                                                                      p0,
                                                                      response.omega[i],
                                                                      p1,
                                                                      w);
                                             return align_phase_near_reference(std::arg(evaluator(w)),
                                                                               reference);
                                           },
                                         response.omega[i - 1],
                                         p0,
                                         response.omega[i],
                                         p1,
                                         target);
              const Real mag = std::abs(evaluator(omega));
              const Real ratio =
                  std::abs(mag) <= std::numeric_limits<Real>::epsilon() ?
                    std::numeric_limits<Real>::infinity() :
                    Real(1) / std::abs(mag);
              if (not found or ratio < best_ratio)
                {
                  best_ratio = ratio;
                  best_omega = omega;
                  found = true;
                }
            }
        }

      if (found)
        {
          info.ratio = best_ratio;
          info.decibels = std::isinf(best_ratio) ?
                            std::numeric_limits<Real>::infinity() :
                            Real(20) * std::log10(best_ratio);
          info.crossover_omega = best_omega;
          info.found = true;
        }

      return info;
    }

    /** @brief implementation of phase unwrapping (removes +/- 2*pi jumps). */
    [[nodiscard]] static Array<Real>
    unwrap_phase(const Array<Real> & phase)
    {
      if (phase.is_empty())
        return {};

      Array<Real> output = Array<Real>::create(phase.size());
      output(0) = phase[0];
      Real offset = Real(0);
      for (size_t i = 1; i < phase.size(); ++i)
        {
          const Real delta = phase[i] - phase[i - 1];
          if (delta > std::numbers::pi_v<Real>)
            offset -= Real(2) * std::numbers::pi_v<Real>;
          else if (delta < -std::numbers::pi_v<Real>)
            offset += Real(2) * std::numbers::pi_v<Real>;
          output(i) = phase[i] + offset;
        }
      return output;
    }

    /** @brief implementation of numeric group delay calculation. */
    [[nodiscard]] static Array<Real>
    group_delay_impl(const FrequencyResponse & response)
    {
      ah_invalid_argument_if(response.omega.size() != response.response.size())
        << "FFT::group_delay: omega size " << response.omega.size()
        << " does not match response size " << response.response.size();

      if (response.omega.is_empty())
        return {};

      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      Array<Real> delay = Array<Real>::create(response.omega.size());
      if (response.omega.size() == 1)
        {
          delay(0) = Real(0);
          return delay;
        }

      for (size_t i = 0; i < response.omega.size(); ++i)
        {
          const size_t left = i == 0 ? 0 : i - 1;
          const size_t right = i + 1 == response.omega.size() ? i : i + 1;
          const Real domega = response.omega[right] - response.omega[left];
          if (std::abs(domega) <= std::numeric_limits<Real>::epsilon())
            delay(i) = Real(0);
          else
            delay(i) = -(phase[right] - phase[left]) / domega;
        }

      return delay;
    }

    /** @brief implementation of numeric phase delay calculation. */
    [[nodiscard]] static Array<Real>
    phase_delay_impl(const FrequencyResponse & response)
    {
      ah_invalid_argument_if(response.omega.size() != response.response.size())
        << "FFT::phase_delay: omega size " << response.omega.size()
        << " does not match response size " << response.response.size();

      if (response.omega.is_empty())
        return {};

      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      const Array<Real> group = group_delay_impl(response);
      Array<Real> delay = Array<Real>::create(response.omega.size());
      const Real tol = (response.omega[response.omega.size() - 1] + Real(1))
                       * Real(128) * std::numeric_limits<Real>::epsilon();

      for (size_t i = 0; i < response.omega.size(); ++i)
        delay(i) = std::abs(response.omega[i]) <= tol ? group[i] : -phase[i] / response.omega[i];

      return delay;
    }

    /** @brief Rationale: Validates and normalizes ISTFT parameters, checking
     *         window constraints and FFT size.
     */
    static void
    validate_istft_configuration(const Array<Real> & analysis_window,
                                 const Array<Real> & synthesis_window,
                                 const size_t fft_size,
                                 const ISTFTOptions & options,
                                 const char *ctx)
    {
      ah_invalid_argument_if(analysis_window.is_empty())
        << ctx << ": analysis window must be non-empty";
      ah_invalid_argument_if(synthesis_window.is_empty())
        << ctx << ": synthesis window must be non-empty";
      ah_invalid_argument_if(analysis_window.size() != synthesis_window.size())
        << ctx << ": analysis window size " << analysis_window.size()
        << " does not match synthesis window size "
        << synthesis_window.size();
      ah_invalid_argument_if(options.hop_size == 0)
        << ctx << ": hop size must be positive";
      ah_invalid_argument_if(fft_size == 0)
        << ctx << ": FFT size must be positive";
      ah_invalid_argument_if(fft_size < analysis_window.size())
        << ctx << ": frame FFT size " << fft_size
        << " is smaller than window size " << analysis_window.size();

      validate_overlap_constraints(analysis_window,
                                   synthesis_window,
                                   options.hop_size,
                                   options.validate_nola,
                                   options.validate_cola,
                                   ctx);
    }

    /** @brief implementation of the Inverse Short-Time Fourier Transform. */
    [[nodiscard]] static Array<Real>
    istft_impl(const Array<Array<Complex>> & spectrogram,
               const Array<Real> & analysis_window,
               const Array<Real> & synthesis_window,
               const ISTFTOptions & options,
               ThreadPool *pool,
               const size_t chunk_size)
    {
      if (spectrogram.is_empty())
        return {};

      const size_t fft_size = spectrogram[0].size();
      validate_istft_configuration(analysis_window,
                                   synthesis_window,
                                   fft_size,
                                   options,
                                   "FFT::istft");

      for (size_t i = 1; i < spectrogram.size(); ++i)
        ah_invalid_argument_if(spectrogram[i].size() != fft_size)
          << "FFT::istft: frame " << i << " has size "
          << spectrogram[i].size() << " but expected " << fft_size;

      const size_t raw_length = (spectrogram.size() - 1) * options.hop_size
                                + analysis_window.size();
      const size_t center_pad = options.centered ? analysis_window.size() / 2 : 0;
      const size_t output_length = options.signal_length == 0 ?
                                     (options.centered and raw_length >= 2 * center_pad ?
                                        raw_length - 2 * center_pad :
                                        raw_length) :
                                     options.signal_length;
      ah_invalid_argument_if(center_pad + output_length > raw_length)
        << "FFT::istft: requested signal length " << output_length
        << " exceeds overlap-add length " << raw_length
        << " after centered trimming";

      Plan plan(fft_size);
      Array<Real> output = Array<Real>::create(raw_length);
      Array<Real> normalization = Array<Real>::create(raw_length);
      for (size_t i = 0; i < raw_length; ++i)
        {
          output(i) = Real(0);
          normalization(i) = Real(0);
        }

      for (size_t frame_idx = 0; frame_idx < spectrogram.size(); ++frame_idx)
        {
          const Array<Real> frame = (pool != nullptr) ?
                                      plan.pinverse_transform_real(*pool, spectrogram[frame_idx], chunk_size) :
                                      plan.inverse_transform_real(spectrogram[frame_idx]);
          const size_t offset = frame_idx * options.hop_size;

          for (size_t i = 0; i < analysis_window.size(); ++i)
            {
              output(offset + i) += frame[i] * synthesis_window[i];
              normalization(offset + i) += analysis_window[i] * synthesis_window[i];
            }
        }

      const Real tol = (max_abs_value(normalization) + Real(1))
                       * Real(256) * std::numeric_limits<Real>::epsilon();
      Array<Real> normalized = Array<Real>::create(raw_length);
      for (size_t i = 0; i < raw_length; ++i)
        {
          if (std::abs(normalization[i]) <= tol)
            {
              ah_domain_error_if(std::abs(output[i]) > tol)
                << "FFT::istft: overlap-add normalization vanished at sample " << i;
              normalized(i) = Real(0);
            }
          else
            normalized(i) = output[i] / normalization[i];
        }

      return options.centered ?
               slice_copy(normalized, center_pad, output_length) :
               slice_copy(normalized, 0, output_length);
    }

    /** @brief Returns a copy of @p input zero-padded to size @p n. */
    [[nodiscard]] static Array<Complex>
    zero_padded_copy(const Array<Complex> & input, const size_t n)
    {
      ah_invalid_argument_if(input.size() > n)
        << "FFT::zero_padded_copy: target size " << n
        << " is smaller than input size " << input.size();

      Array<Complex> output;
      output.reserve(n);
      for (size_t i = 0; i < input.size(); ++i)
        output.append(input[i]);
      for (size_t i = input.size(); i < n; ++i)
        output.append(Complex(Real(0), Real(0)));
      return output;
    }

    /** @brief Returns a copy of @p input zero-padded to size @p n. */
    [[nodiscard]] static Array<Real>
    zero_padded_copy(const Array<Real> & input, const size_t n)
    {
      ah_invalid_argument_if(input.size() > n)
        << "FFT::zero_padded_copy: target size " << n
        << " is smaller than input size " << input.size();

      Array<Real> output;
      output.reserve(n);
      for (size_t i = 0; i < input.size(); ++i)
        output.append(input[i]);
      for (size_t i = input.size(); i < n; ++i)
        output.append(Real(0));
      return output;
    }

    /** @brief Trims @p input to size @p n. */
    static void
    trim_to_size(Array<Complex> & input, const size_t n)
    {
      ah_invalid_argument_if(n > input.size())
        << "FFT::trim_to_size: target size " << n
        << " exceeds input size " << input.size();

      while (input.size() > n)
        static_cast<void>(input.remove_last());
    }

    template <typename T, typename Container>
      requires requires(const Container & c)
        {
          std::begin(c);
          std::end(c);
          requires std::convertible_to<decltype(*std::begin(c)), T>;
        }
    [[nodiscard]] static Array<T>
    to_array(const Container & input)
    {
      Array<T> output;

      if constexpr (requires { input.size(); })
        output.reserve(input.size());

      for (auto it = std::begin(input); it != std::end(input); ++it)
        output.append(static_cast<T>(*it));

      return output;
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    build_complex_input(const Container & input)
    {
      return to_array<Complex>(input);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Real>
    build_real_input(const Container & input)
    {
      return to_array<Real>(input);
    }

    [[nodiscard]] static size_t
    tensor_element_count(const Array<size_t> & shape,
                         const char *ctx)
    {
      ah_invalid_argument_if(shape.is_empty())
        << ctx << ": tensor shape must be non-empty";

      size_t total = 1;
      for (size_t i = 0; i < shape.size(); ++i)
        {
          ah_invalid_argument_if(shape[i] == 0)
            << ctx << ": shape dimension " << i << " must be positive";
          total = saturating_product(total, shape[i]);
        }
      return total;
    }

    [[nodiscard]] static Array<size_t>
    row_major_strides(const Array<size_t> & shape,
                      const char *ctx)
    {
      static_cast<void>(tensor_element_count(shape, ctx));
      Array<size_t> strides = Array<size_t>::create(shape.size());
      size_t stride = 1;
      for (size_t i = shape.size(); i > 0; --i)
        {
          strides(i - 1) = stride;
          stride = saturating_product(stride, shape[i - 1]);
        }
      return strides;
    }

    [[nodiscard]] static size_t
    tensor_max_offset(const Array<size_t> & shape,
                      const Array<size_t> & strides,
                      const char *ctx)
    {
      ah_invalid_argument_if(shape.size() != strides.size())
        << ctx << ": shape rank " << shape.size()
        << " does not match strides rank " << strides.size();

      size_t max_offset = 0;
      for (size_t i = 0; i < shape.size(); ++i)
        {
          ah_invalid_argument_if(shape[i] == 0)
            << ctx << ": shape dimension " << i << " must be positive";
          max_offset += saturating_product(shape[i] - 1, strides[i]);
        }
      return max_offset;
    }

    /** @brief Rationale: Validates a tensor layout and infers row-major strides
     *         if they are not provided. Ensures the layout is compatible with
     *         the actual flat buffer size.
     */
    [[nodiscard]] static TensorLayout
    normalize_tensor_layout(const Array<Complex> & data,
                            const TensorLayout & layout,
                            const char *ctx)
    {
      TensorLayout normalized;
      normalized.shape = layout.shape;
      ah_invalid_argument_if(normalized.shape.is_empty())
        << ctx << ": tensor shape must be non-empty";

      const size_t logical_size = tensor_element_count(normalized.shape, ctx);
      if (layout.strides.is_empty())
        {
          normalized.strides = row_major_strides(normalized.shape, ctx);
          ah_invalid_argument_if(data.size() != logical_size)
            << ctx << ": flat data size " << data.size()
            << " does not match row-major tensor size " << logical_size;
          return normalized;
        }

      normalized.strides = layout.strides;
      ah_invalid_argument_if(normalized.strides.size() != normalized.shape.size())
        << ctx << ": strides rank " << normalized.strides.size()
        << " does not match shape rank " << normalized.shape.size();

      const size_t max_offset =
          tensor_max_offset(normalized.shape, normalized.strides, ctx);
      ah_invalid_argument_if(data.is_empty() or max_offset >= data.size())
        << ctx << ": tensor layout touches offset " << max_offset
        << " but flat buffer size is " << data.size();
      return normalized;
    }

    /** @brief Rationale: Validates axis indices against the tensor rank and
     *         ensures no axis is specified more than once.
     */
    [[nodiscard]] static Array<size_t>
    normalize_axes(const Array<size_t> & axes,
                   const size_t rank,
                   const char *ctx)
    {
      ah_invalid_argument_if(axes.is_empty())
        << ctx << ": at least one axis is required";

      Array<bool> used(rank, false);
      Array<size_t> normalized;
      normalized.reserve(axes.size());
      for (size_t i = 0; i < axes.size(); ++i)
        {
          ah_out_of_range_error_if(axes[i] >= rank)
            << ctx << ": axis " << axes[i] << " is out of range for rank "
            << rank;
          ah_invalid_argument_if(used[axes[i]])
            << ctx << ": axis " << axes[i] << " appears more than once";
          used(axes[i]) = true;
          normalized.append(axes[i]);
        }

      return normalized;
    }

    /** @brief Rationale: Generates the base memory offsets for all 1D slices
     *         along a specific axis in a multidimensional tensor.
     */
    [[nodiscard]] static Array<size_t>
    axis_base_offsets(const Array<size_t> & shape,
                      const Array<size_t> & strides,
                      const size_t axis)
    {
      Array<size_t> other_dims;
      other_dims.reserve(shape.size());
      size_t slice_count = 1;
      for (size_t i = 0; i < shape.size(); ++i)
        if (i != axis)
          {
            other_dims.append(i);
            slice_count = saturating_product(slice_count, shape[i]);
          }

      Array<size_t> offsets;
      offsets.reserve(slice_count);
      if (other_dims.is_empty())
        {
          offsets.append(0);
          return offsets;
        }

      Array<size_t> counters = Array<size_t>::create(other_dims.size());
      for (size_t i = 0; i < counters.size(); ++i)
        counters(i) = 0;

      bool done = false;
      while (not done)
        {
          size_t offset = 0;
          for (size_t i = 0; i < other_dims.size(); ++i)
            offset += counters[i] * strides[other_dims[i]];
          offsets.append(offset);

          for (size_t rev = other_dims.size(); rev > 0; --rev)
            {
              const size_t idx = rev - 1;
              ++counters(idx);
              if (counters[idx] < shape[other_dims[idx]])
                break;

              counters(idx) = 0;
              if (idx == 0)
                done = true;
            }
        }

      return offsets;
    }

    /** @brief Rationale: Extracts a non-contiguous slice of data along a tensor
     *         axis into a contiguous array for FFT processing.
     */
    [[nodiscard]] static Array<Complex>
    gather_axis_slice(const Array<Complex> & data,
                      const size_t base_offset,
                      const size_t axis_length,
                      const size_t axis_stride)
    {
      Array<Complex> slice = Array<Complex>::create(axis_length);
      for (size_t i = 0; i < axis_length; ++i)
        slice(i) = data[base_offset + i * axis_stride];
      return slice;
    }

    /** @brief Rationale: Writes back a transformed contiguous slice into its
     *         original (potentially non-contiguous) memory locations.
     */
    static void
    scatter_axis_slice(Array<Complex> & data,
                       const size_t base_offset,
                       const size_t axis_stride,
                       const Array<Complex> & slice)
    {
      for (size_t i = 0; i < slice.size(); ++i)
        data(base_offset + i * axis_stride) = slice[i];
    }

    /** @brief Rationale: Internal implementation of a 1D FFT along a single tensor
     *         axis. It gathers slices, transforms them, and scatters them back.
     */
    static void
    transform_axis_impl(Array<Complex> & data,
                        const TensorLayout & layout,
                        const size_t axis,
                        const bool invert,
                        ThreadPool *pool = nullptr,
                        const size_t chunk_size = 0)
    {
      const TensorLayout normalized =
          normalize_tensor_layout(data, layout, "FFT::transform_axis");
      const Array<size_t> axes =
          normalize_axes(Array<size_t>({axis}),
                         normalized.shape.size(),
                         "FFT::transform_axis");
      const size_t axis_index = axes[0];
      const size_t axis_length = normalized.shape[axis_index];
      const size_t axis_stride = normalized.strides[axis_index];
      const Array<size_t> offsets =
          axis_base_offsets(normalized.shape, normalized.strides, axis_index);
      Plan plan(axis_length);

      if (offsets.size() == 1)
        {
          Array<Complex> slice =
              gather_axis_slice(data, offsets[0], axis_length, axis_stride);
          if (pool != nullptr and pool->num_threads() > 1)
            plan.ptransform(*pool, slice, invert, chunk_size);
          else
            plan.transform(slice, invert);
          scatter_axis_slice(data, offsets[0], axis_stride, slice);
          return;
        }

      auto transform_one =
          [&data, &offsets, axis_length, axis_stride, invert, &plan](const size_t i)
        {
          Array<Complex> slice =
              gather_axis_slice(data, offsets[i], axis_length, axis_stride);
          plan.transform(slice, invert);
          scatter_axis_slice(data, offsets[i], axis_stride, slice);
        };

      if (pool != nullptr
          and pool->num_threads() > 1
          and offsets.size() > 1)
        parallel_for_index(*pool, 0, offsets.size(), transform_one, chunk_size);
      else
        for (size_t i = 0; i < offsets.size(); ++i)
          transform_one(i);
    }

    /** @brief Rationale: Internal implementation of a multi-axis tensor transform.
     *         Applies the 1D transform sequentially along each requested axis.
     */
    static void
    transform_axes_impl(Array<Complex> & data,
                        const TensorLayout & layout,
                        const Array<size_t> & axes,
                        const bool invert,
                        ThreadPool *pool = nullptr,
                        const size_t chunk_size = 0)
    {
      const TensorLayout normalized =
          normalize_tensor_layout(data, layout, "FFT::transform_axes");
      const Array<size_t> normalized_axes =
          normalize_axes(axes, normalized.shape.size(), "FFT::transform_axes");
      for (size_t i = 0; i < normalized_axes.size(); ++i)
        transform_axis_impl(data,
                            normalized,
                            normalized_axes[i],
                            invert,
                            pool,
                            chunk_size);
    }

    /** @brief Rationale: Validates that an Array of Arrays represents a rectangular
     *         matrix and returns its {rows, cols} shape.
     */
    [[nodiscard]] static Array<size_t>
    matrix_shape(const Array<Array<Complex>> & input,
                 const char *ctx)
    {
      ah_invalid_argument_if(input.is_empty()) << ctx << ": matrix must be non-empty";

      const size_t rows = input.size();
      const size_t cols = input[0].size();
      ah_invalid_argument_if(cols == 0) << ctx << ": matrix rows must be non-empty";

      for (size_t i = 1; i < rows; ++i)
        ah_invalid_argument_if(input[i].size() != cols)
          << ctx << ": row " << i << " has size " << input[i].size()
          << " but expected " << cols;

      return {rows, cols};
    }

    /** @brief Rationale: Flattens a 2D matrix into a 1D row-major array. */
    [[nodiscard]] static Array<Complex>
    flatten_matrix_row_major(const Array<Array<Complex>> & input,
                             const char *ctx)
    {
      const Array<size_t> shape = matrix_shape(input, ctx);
      Array<Complex> flat;
      flat.reserve(shape[0] * shape[1]);
      for (size_t row = 0; row < shape[0]; ++row)
        for (size_t col = 0; col < shape[1]; ++col)
          flat.append(input[row][col]);
      return flat;
    }

    /** @brief Rationale: Reshapes a 1D row-major array back into a 2D matrix. */
    [[nodiscard]] static Array<Array<Complex>>
    reshape_matrix_row_major(const Array<Complex> & input,
                             const size_t rows,
                             const size_t cols,
                             const char *ctx)
    {
      ah_invalid_argument_if(rows == 0 or cols == 0)
        << ctx << ": matrix shape must be positive";
      ah_invalid_argument_if(input.size() != rows * cols)
        << ctx << ": flat size " << input.size()
        << " does not match matrix shape " << rows << "x" << cols;

      Array<Array<Complex>> output(rows, Array<Complex>());
      for (size_t row = 0; row < rows; ++row)
        {
          output(row) = Array<Complex>::create(cols);
          for (size_t col = 0; col < cols; ++col)
            output(row)(col) = input[row * cols + col];
        }
      return output;
    }

    /** @brief Rationale: Validates that a nested Array represents a 3D tensor
     *         and returns its {d0, d1, d2} shape.
     */
    [[nodiscard]] static Array<size_t>
    tensor3_shape(const Array<Array<Array<Complex>>> & input,
                  const char *ctx)
    {
      ah_invalid_argument_if(input.is_empty())
        << ctx << ": tensor must be non-empty";
      const size_t dim0 = input.size();
      const size_t dim1 = input[0].size();
      ah_invalid_argument_if(dim1 == 0)
        << ctx << ": tensor middle dimension must be positive";
      const size_t dim2 = input[0][0].size();
      ah_invalid_argument_if(dim2 == 0)
        << ctx << ": tensor innermost dimension must be positive";

      for (size_t i = 0; i < dim0; ++i)
        {
          ah_invalid_argument_if(input[i].size() != dim1)
            << ctx << ": slab " << i << " has size " << input[i].size()
            << " but expected " << dim1;
          for (size_t j = 0; j < dim1; ++j)
            ah_invalid_argument_if(input[i][j].size() != dim2)
              << ctx << ": slab " << i << ", row " << j
              << " has size " << input[i][j].size()
              << " but expected " << dim2;
        }

      return {dim0, dim1, dim2};
    }

    /** @brief Rationale: Flattens a 3D tensor into a 1D row-major array. */
    [[nodiscard]] static Array<Complex>
    flatten_tensor3_row_major(const Array<Array<Array<Complex>>> & input,
                              const char *ctx)
    {
      const Array<size_t> shape = tensor3_shape(input, ctx);
      Array<Complex> flat;
      flat.reserve(shape[0] * shape[1] * shape[2]);
      for (size_t i = 0; i < shape[0]; ++i)
        for (size_t j = 0; j < shape[1]; ++j)
          for (size_t k = 0; k < shape[2]; ++k)
            flat.append(input[i][j][k]);
      return flat;
    }

    /** @brief Rationale: Reshapes a 1D row-major array back into a 3D tensor. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    reshape_tensor3_row_major(const Array<Complex> & input,
                              const size_t dim0,
                              const size_t dim1,
                              const size_t dim2,
                              const char *ctx)
    {
      ah_invalid_argument_if(dim0 == 0 or dim1 == 0 or dim2 == 0)
        << ctx << ": tensor shape must be positive";
      ah_invalid_argument_if(input.size() != dim0 * dim1 * dim2)
        << ctx << ": flat size " << input.size()
        << " does not match tensor shape";

      Array<Array<Array<Complex>>> output(dim0, Array<Array<Complex>>());
      size_t index = 0;
      for (size_t i = 0; i < dim0; ++i)
        {
          output(i) = Array<Array<Complex>>(dim1, Array<Complex>());
          for (size_t j = 0; j < dim1; ++j)
            {
              output(i)(j) = Array<Complex>::create(dim2);
              for (size_t k = 0; k < dim2; ++k, ++index)
                output(i)(j)(k) = input[index];
            }
        }
      return output;
    }

    /** @brief Rationale: Internal implementation for transposing between
     *         multichannel spectrogram layouts (e.g. Channel-major to Frame-major).
     */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    transpose_spectrogram_layout_impl(const Array<Array<Array<Complex>>> & input,
                                      const SpectrogramLayout source,
                                      const SpectrogramLayout target,
                                      const char *ctx)
    {
      if (source == target or input.is_empty())
        return input;

      if (source == SpectrogramLayout::channel_frame_bin)
        {
          const size_t channels = input.size();
          const size_t frames = input[0].size();
          const size_t bins =
              frames == 0 ? 0 : input[0][0].size();

          for (size_t ch = 0; ch < channels; ++ch)
            {
              ah_invalid_argument_if(input[ch].size() != frames)
                << ctx << ": channel " << ch << " has " << input[ch].size()
                << " frames but expected " << frames;
              for (size_t frame = 0; frame < frames; ++frame)
                ah_invalid_argument_if(input[ch][frame].size() != bins)
                  << ctx << ": channel " << ch << ", frame " << frame
                  << " has " << input[ch][frame].size()
                  << " bins but expected " << bins;
            }

          Array<Array<Array<Complex>>> output(frames, Array<Array<Complex>>());
          for (size_t frame = 0; frame < frames; ++frame)
            {
              output(frame) = Array<Array<Complex>>(channels, Array<Complex>());
              for (size_t ch = 0; ch < channels; ++ch)
                output(frame)(ch) = input[ch][frame];
            }
          return output;
        }

      const size_t frames = input.size();
      const size_t channels = input[0].size();
      const size_t bins = channels == 0 ? 0 : input[0][0].size();
      for (size_t frame = 0; frame < frames; ++frame)
        {
          ah_invalid_argument_if(input[frame].size() != channels)
            << ctx << ": frame " << frame << " has " << input[frame].size()
            << " channels but expected " << channels;
          for (size_t ch = 0; ch < channels; ++ch)
            ah_invalid_argument_if(input[frame][ch].size() != bins)
              << ctx << ": frame " << frame << ", channel " << ch
              << " has " << input[frame][ch].size()
              << " bins but expected " << bins;
        }

      Array<Array<Array<Complex>>> output(channels, Array<Array<Complex>>());
      for (size_t ch = 0; ch < channels; ++ch)
        {
          output(ch) = Array<Array<Complex>>(frames, Array<Complex>());
          for (size_t frame = 0; frame < frames; ++frame)
            output(ch)(frame) = input[frame][ch];
        }
      return output;
    }

    /** @brief Lifts a real-valued input array to the complex domain.
     *
     *  Creates a new Array of complex numbers where the real part is taken
     *  from the input and the imaginary part is set to zero.
     *
     *  @param input The input array of real values.
     *  @return An Array of complex values.
     */
    [[nodiscard]] static Array<Complex>
    lift_real_input(const Array<Real> & input)
    {
      return to_array<Complex>(input);
    }

    /** @brief Rationale: Optimized FFT for real signals of size N (power-of-two).
     *         Computes an N/2 complex FFT and uses symmetry to extract the
     *         full N-point complex spectrum.
     */
    [[nodiscard]] static Array<Complex>
    transform_real_optimized(const Array<Real> & input,
                             ThreadPool *pool = nullptr,
                             const size_t chunk_size = 0)
    {
      const size_t n = input.size();
      ah_invalid_argument_if(n == 0)
        << "FFT::transform: input must be non-empty";

      if (n == 1)
        return {Complex(input[0], Real(0))};

      if (not is_power_of_two(n))
        {
          Array<Complex> spectrum = lift_real_input(input);
          transform_any_size_impl(spectrum, false, pool, chunk_size);
          return spectrum;
        }

      const size_t half = n / 2;
      Array<Complex> packed;
      packed.reserve(half);
      for (size_t i = 0; i < half; ++i)
        packed.append(Complex(input[2 * i], input[2 * i + 1]));

      transform_impl(packed, false, pool, chunk_size);

      Array<Complex> spectrum = Array<Complex>::create(n);
      const Complex half_i(Real(0), Real(-0.5));
      const Real base_angle = -Real(2) * std::numbers::pi_v<Real>
                              / static_cast<Real>(n);
      auto recombine = [&packed, &spectrum, half_i, half, base_angle](const size_t k)
        {
          const size_t mirror = (half - k) % half;
          const Complex zk = packed[k];
          const Complex zmk = std::conj(packed[mirror]);
          const Complex even = (zk + zmk) * Real(0.5);
          const Complex odd = half_i * (zk - zmk);
          const Complex twiddle = twiddle_at(base_angle, k);
          spectrum(k) = even + twiddle * odd;
          spectrum(k + half) = even - twiddle * odd;
        };

      if (pool != nullptr and pool->num_threads() > 1 and half > 1)
        parallel_for_index(*pool, 0, half, recombine, chunk_size);
      else
        for (size_t k = 0; k < half; ++k)
          recombine(k);

      return spectrum;
    }

    /** @brief Rationale: Optimized Inverse FFT for real signals of size N (power-of-two).
     *         Processes an N-point complex spectrum into an N/2 complex array,
     *         applies IFFT, and extracts N real samples.
     */
    template <typename InversePackedTransform>
    [[nodiscard]] static Array<Real>
    inverse_transform_real_optimized_impl(const Array<Complex> & input,
                                          const char *ctx,
                                          ThreadPool *pool,
                                          const size_t chunk_size,
                                          InversePackedTransform inverse_packed)
    {
      const size_t n = input.size();
      ah_invalid_argument_if(n == 0)
        << ctx << ": input must be non-empty";

      if (n == 1)
        {
          validate_real_spectrum(input, ctx);
          return {input[0].real()};
        }

      ah_invalid_argument_if(not is_power_of_two(n))
        << ctx << ": input size " << n << " must be a power of two";

      validate_real_spectrum(input, ctx);

      const size_t half = n / 2;
      Array<Complex> packed_spectrum = Array<Complex>::create(half);
      const Real base_angle = -Real(2) * std::numbers::pi_v<Real>
                              / static_cast<Real>(n);

      auto build_packed_spectrum = [&input, &packed_spectrum, half, base_angle]
      (const size_t k)
        {
          const Complex even = (input[k] + input[k + half]) * Real(0.5);
          const Complex odd = (input[k] - input[k + half])
                              * std::conj(twiddle_at(base_angle, k))
                              * Real(0.5);
          packed_spectrum(k) = even + Complex(-odd.imag(), odd.real());
        };

      if (pool != nullptr and pool->num_threads() > 1 and half > 1)
        parallel_for_index(*pool, 0, half, build_packed_spectrum, chunk_size);
      else
        for (size_t k = 0; k < half; ++k)
          build_packed_spectrum(k);

      inverse_packed(packed_spectrum, pool, chunk_size);

      Array<Real> output = Array<Real>::create(n);
      auto unpack_output = [&packed_spectrum, &output](const size_t i)
        {
          output(2 * i) = packed_spectrum[i].real();
          output(2 * i + 1) = packed_spectrum[i].imag();
        };

      if (pool != nullptr and pool->num_threads() > 1 and half > 1)
        parallel_for_index(*pool, 0, half, unpack_output, chunk_size);
      else
        for (size_t i = 0; i < half; ++i)
          unpack_output(i);

      return output;
    }

    /** @brief Rationale: Efficiently multiplies two real sequences using a single
     *         N-point complex FFT by packing one sequence into the real part
     *         and the other into the imaginary part.
     */
    [[nodiscard]] static Array<Real>
    multiply_real_optimized(const Array<Real> & a, const Array<Real> & b,
                            ThreadPool *pool = nullptr,
                            const size_t chunk_size = 0)
    {
      if (a.is_empty() or b.is_empty())
        return {};

      ah_invalid_argument_if(a.size() > std::numeric_limits<size_t>::max() - b.size() + 1)
        << "FFT::multiply: product size exceeds size_t capacity";

      const size_t required = a.size() + b.size() - 1;
      const size_t n = next_power_of_two(required);
      const Array<Real> pa = zero_padded_copy(a, n);
      const Array<Real> pb = zero_padded_copy(b, n);

      Array<Complex> packed;
      packed.reserve(n);
      for (size_t i = 0; i < n; ++i)
        packed.append(Complex(pa[i], pb[i]));

      transform_impl(packed, false, pool, chunk_size);
      const Array<Complex> packed_spectrum = packed;

      const Complex half_i(Real(0), Real(-0.5));
      auto build_product = [&packed, &packed_spectrum, half_i, n](const size_t k)
        {
          const size_t mirror = (n - k) % n;
          const Complex pk = packed_spectrum[k];
          const Complex pm = std::conj(packed_spectrum[mirror]);
          const Complex fa = (pk + pm) * Real(0.5);
          const Complex fb = half_i * (pk - pm);
          packed(k) = fa * fb;
        };

      if (pool != nullptr and pool->num_threads() > 1 and n > 1)
        parallel_for_index(*pool, 0, n, build_product, chunk_size);
      else
        for (size_t k = 0; k < n; ++k)
          build_product(k);

      transform_impl(packed, true, pool, chunk_size);
      trim_to_size(packed, required);
      return project_real_output(packed, "FFT::multiply");
    }

    /** @brief Rationale: Linear convolution of two complex sequences via FFT. */
    [[nodiscard]] static Array<Complex>
    multiply_complex_impl(const Array<Complex> & a, const Array<Complex> & b,
                          ThreadPool *pool = nullptr,
                          const size_t chunk_size = 0)
    {
      if (a.is_empty() or b.is_empty())
        return {};

      ah_invalid_argument_if(a.size() > std::numeric_limits<size_t>::max() - b.size() + 1)
        << "FFT::multiply: product size exceeds size_t capacity";

      const size_t required = a.size() + b.size() - 1;
      const size_t n = next_power_of_two(required);

      Array<Complex> fa = zero_padded_copy(a, n);
      Array<Complex> fb = zero_padded_copy(b, n);

      transform_impl(fa, false, pool, chunk_size);
      transform_impl(fb, false, pool, chunk_size);

      auto pointwise_product = [&fa, &fb](const size_t i)
        {
          fa(i) *= fb[i];
        };

      if (pool != nullptr and pool->num_threads() > 1 and n > 1)
        parallel_for_index(*pool, 0, n, pointwise_product, chunk_size);
      else
        for (size_t i = 0; i < n; ++i)
          pointwise_product(i);

      transform_impl(fa, true, pool, chunk_size);

      trim_to_size(fa, required);
      return fa;
    }

    /** @brief Projects a complex-valued output array back to the real domain.
     *
     *  Extracts the real parts of the complex numbers in the input array.
     *  It verifies that the imaginary parts are within a negligible tolerance
     *  relative to the magnitude of the values.
     *
     *  @param input The input array of complex values.
     *  @param ctx A context string for error reporting (usually the caller name).
     *  @return An Array of real values.
     *  @throw std::domain_error if any imaginary part exceeds the tolerance.
     */
    [[nodiscard]] static Array<Real>
    project_real_output(const Array<Complex> & input, const char *ctx)
    {
      Array<Real> output;
      output.reserve(input.size());
      const Real stages = transform_stages(input.size());

      for (size_t i = 0; i < input.size(); ++i)
        {
          const Real imag = std::abs(input[i].imag());
          const Real scale = std::abs(input[i].real()) + std::abs(input[i].imag()) + Real(1);
          const Real tolerance = Real(128) * stages
                                 * std::numeric_limits<Real>::epsilon() * scale;

          ah_domain_error_if(imag > tolerance)
            << ctx << ": coefficient " << i
            << " has non-negligible imaginary part " << input[i].imag();

          output.append(input[i].real());
        }

      return output;
    }

    /** @brief Performs bit-reversal permutation in-place.
     *
     *  This is a required preprocessing step for the iterative Cooley-Tukey FFT.
     *  It rearranges the elements of the array such that the element at index
     *  'i' is moved to the index formed by reversing the bits of 'i'.
     *
     *  @param a The array to be permuted.
     */
    static void
    bit_reverse(Array<Complex> & a) noexcept
    {
      const size_t n = a.size();
      for (size_t i = 1, j = 0; i < n; ++i)
        {
          size_t bit = n >> 1;
          for (; j & bit; bit >>= 1)
            j ^= bit;
          j ^= bit;
          if (i < j)
            std::swap(a(i), a(j));
        }
    }

    /** @brief Calculates the smallest power of two greater than or equal to n.
     *
     *  Used to determine the required size for FFT-based convolution.
     *
     *  @param n The input size.
     *  @return The next power of two.
     *  @throw std::invalid_argument if n is zero or if an overflow occurs.
     */
    [[nodiscard]] static size_t
    next_power_of_two(const size_t n)
    {
      ah_invalid_argument_if(n == 0)
        << "FFT::next_power_of_two: input size must be positive";

      size_t power = 1;
      while (power < n)
        {
          ah_invalid_argument_if(power > std::numeric_limits<size_t>::max() / 2)
            << "FFT::next_power_of_two: size overflow for input " << n;
          power <<= 1;
        }

      return power;
    }

    /** @brief implementation of the power-of-two iterative FFT using mixed radix-2/4. */
    static void
    transform_impl(Array<Complex> & a, const bool invert,
                   ThreadPool *pool = nullptr,
                   const size_t chunk_size = 0)
    {
      const size_t n = a.size();
      ah_invalid_argument_if(n == 0)
        << "FFT::transform: input must be non-empty";

      if (n == 1)
        return;

      ah_invalid_argument_if(not is_power_of_two(n))
        << "FFT::transform: input size " << n << " must be a power of two";

      bit_reverse(a);

      size_t log_n = 0;
      for (size_t tmp = n; tmp > 1; tmp >>= 1)
        ++log_n;

      size_t r4_start_len;

      // If odd number of radix-2 stages, do one radix-2 pass first
      // (twiddle for len=2 is always 1)
      if (log_n & 1)
        {
          const size_t blocks = n >> 1;
          auto r2_block = [&a](const size_t block)
            {
              const size_t base = block << 1;
              const Complex u = a[base];
              const Complex v = a[base + 1];
              a(base) = u + v;
              a(base + 1) = u - v;
            };

          if (pool != nullptr and pool->num_threads() > 1 and blocks > 1)
            parallel_for_index(*pool, 0, blocks, r2_block, chunk_size);
          else
            for (size_t block = 0; block < blocks; ++block)
              r2_block(block);

          r4_start_len = 8;
        }
      else
        r4_start_len = 4;

      // Radix-4 stages: each fuses two radix-2 stages.
      // Block size quadruples each iteration.
      for (size_t len = r4_start_len; len <= n; len <<= 2)
        {
          const size_t quarter = len >> 2;
          const size_t blocks = n / len;
          const Real angle = (invert ? Real(2) : Real(-2))
                             * std::numbers::pi_v<Real>
                             / static_cast<Real>(len);
          const Complex w_step = std::polar(Real(1), angle);

          auto r4_block = [&a, quarter, angle, w_step, invert]
          (const size_t block)
            {
              const size_t base = block * (quarter << 2);
              Complex w1(Real(1), Real(0));
              for (size_t j = 0; j < quarter; ++j)
                {
                  const Complex w2 = w1 * w1;
                  const Complex w3 = w2 * w1;

                  const size_t i0 = base + j;
                  const size_t i1 = i0 + quarter;
                  const size_t i2 = i1 + quarter;
                  const size_t i3 = i2 + quarter;

                  const Complex a0 = a[i0];
                  const Complex a1 = a[i1] * w2;
                  const Complex a2 = a[i2] * w1;
                  const Complex a3 = a[i3] * w3;

                  const Complex p = a0 + a1;
                  const Complex q = a0 - a1;
                  const Complex r = a2 + a3;
                  const Complex s = a2 - a3;

                  // -i*s (forward) or +i*s (inverse), no real multiplications
                  const Complex js = invert ? Complex(-s.imag(), s.real()) : Complex(s.imag(), -s.real());

                  a(i0) = p + r;
                  a(i1) = q + js;
                  a(i2) = p - r;
                  a(i3) = q - js;

                  if (j + 1 < quarter)
                    {
                      const size_t next_j = j + 1;
                      if (next_j % twiddle_refresh_period == 0)
                        w1 = twiddle_at(angle, next_j);
                      else
                        w1 *= w_step;
                    }
                }
            };

          if (pool != nullptr and pool->num_threads() > 1 and blocks > 1)
            parallel_for_index(*pool, 0, blocks, r4_block, chunk_size);
          else
            for (size_t block = 0; block < blocks; ++block)
              r4_block(block);
        }

      if (invert)
        {
          const Real inv_n = Real(1) / static_cast<Real>(n);
          for (size_t i = 0; i < n; ++i)
            a(i) *= inv_n;
        }
    }

    /** @brief Rationale: Factors a number into small primes (2, 3, 4, 5) to
     *         determine if a composite FFT kernel is applicable.
     */
    [[nodiscard]] static Array<size_t>
    factor_small_radices(size_t n)
    {
      Array<size_t> factors;
      while (n > 1)
        {
          size_t factor = 0;
          if (n % 4 == 0)
            factor = 4;
          else if (n % 5 == 0)
            factor = 5;
          else if (n % 3 == 0)
            factor = 3;
          else if (n % 2 == 0)
            factor = 2;
          else
            break;

          factors.append(factor);
          n /= factor;
        }

      if (n != 1)
        factors.empty();

      return factors;
    }

  public:
    /** @brief Checks if a given number is a power of two.
     *
     *  @param n The number to check.
     *  @return true if n is a power of two, false otherwise.
     */
    [[nodiscard]] static constexpr bool
    is_power_of_two(const size_t n) noexcept
    {
      return n > 0 and (n & (n - 1)) == 0;
    }

    /** @brief Returns the human-readable name of a SIMD backend. */
    [[nodiscard]] static constexpr const char *
    simd_backend_name(const SimdBackend backend) noexcept
    {
      switch (backend)
        {
        case SimdBackend::neon:
          return "neon";
        case SimdBackend::avx2:
          return "avx2";
        case SimdBackend::scalar:
        default:
          return "scalar";
        }
    }

    /** @brief Returns the human-readable name of a SIMD preference. */
    [[nodiscard]] static constexpr const char *
    simd_preference_name(const SimdPreference preference) noexcept
    {
      switch (preference)
        {
        case SimdPreference::neon_only:
          return "neon";
        case SimdPreference::avx2_only:
          return "avx2";
        case SimdPreference::scalar_only:
          return "scalar";
        case SimdPreference::automatic:
        default:
          return "auto";
        }
    }

    /** @brief Returns whether the AVX2 kernel was compiled for this precision. */
    [[nodiscard]] static constexpr bool
    avx2_kernel_compiled() noexcept
    {
# if ALEPH_FFT_HAS_X86_AVX2_DISPATCH
      return std::same_as<Real, double>;
# else
      return false;
# endif
    }

    /** @brief Returns whether the NEON kernel was compiled for this precision. */
    [[nodiscard]] static constexpr bool
    neon_kernel_compiled() noexcept
    {
# if ALEPH_FFT_HAS_ARM_NEON_DISPATCH
      if constexpr (std::same_as<Real, float>)
        return true;
#   if ALEPH_FFT_HAS_ARM_NEON_DOUBLE
      if constexpr (std::same_as<Real, double>)
        return true;
#   endif
# endif
      return false;
    }

    /** @brief Returns whether the runtime CPU can execute the AVX2 `double`
     *         kernel when it has been compiled in.
     */
    [[nodiscard]] static bool
    avx2_dispatch_available() noexcept
    {
# if ALEPH_FFT_HAS_X86_AVX2_DISPATCH
      if constexpr (std::same_as<Real, double>)
        {
          static const bool available = []() noexcept
            {
              __builtin_cpu_init();
              return static_cast<bool>(__builtin_cpu_supports("avx2"));
            }();
          return available;
        }
# endif
      return false;
    }

    /** @brief Returns whether the runtime CPU can execute the NEON kernel when
     *         it has been compiled in.
     */
    [[nodiscard]] static bool
    neon_dispatch_available() noexcept
    {
# if ALEPH_FFT_HAS_ARM_NEON_DISPATCH
      if constexpr (std::same_as<Real, float>)
        {



#   if defined(__linux__)
      static const bool available = []() noexcept
        {



#     if defined(__aarch64__) and defined(HWCAP_ASIMD)
      return (getauxval(AT_HWCAP) & HWCAP_ASIMD) != 0;
#     elif defined(__arm__) and defined(HWCAP_NEON)
      return (getauxval(AT_HWCAP) & HWCAP_NEON) != 0;
#     else
      return true;
#     endif
          }();
      return available;
#   else
      return true;
#   endif
        }

#   if ALEPH_FFT_HAS_ARM_NEON_DOUBLE
      if constexpr (std::same_as<Real, double>)
        {



#     if defined(__linux__) and defined(HWCAP_ASIMD)
      static const bool available = []() noexcept
        {
          return (getauxval(AT_HWCAP) & HWCAP_ASIMD) != 0;
        }();
      return available;
#     else
      return true;
#     endif
        }
#   endif
# endif
      return false;
    }

    /** @brief Returns the best SIMD backend supported by the current hardware
     *         among the kernels compiled for this precision.
     */
    [[nodiscard]] static SimdBackend
    detected_simd_backend() noexcept
    {
      if (avx2_dispatch_available())
        return SimdBackend::avx2;
      if (neon_dispatch_available())
        return SimdBackend::neon;
      return SimdBackend::scalar;
    }

    /** @brief Returns the detected hardware SIMD backend name. */
    [[nodiscard]] static const char *
    detected_simd_backend_name() noexcept
    {
      return simd_backend_name(detected_simd_backend());
    }

    /** @brief Returns the runtime SIMD preference requested via environment.
     *
     *  `ALEPH_FFT_SIMD=auto|scalar|avx2` is preferred. Legacy compatibility is
     *  preserved through `ALEPH_FFT_ENABLE_AVX2=1` and
     *  `ALEPH_FFT_DISABLE_AVX2=1`.
     */
    [[nodiscard]] static SimdPreference
    simd_preference() noexcept
    {
      static const SimdPreference preference = []() noexcept
        {
          const char *disabled = std::getenv("ALEPH_FFT_DISABLE_AVX2");
          if (disabled != nullptr and disabled[0] != '\0' and disabled[0] != '0')
            return SimdPreference::scalar_only;

          if (const char *mode = std::getenv("ALEPH_FFT_SIMD");
            mode != nullptr and mode[0] != '\0')
            {
              const std::string_view value(mode);
              if (value == "scalar")
                return SimdPreference::scalar_only;
              if (value == "avx2")
                return SimdPreference::avx2_only;
              if (value == "neon")
                return SimdPreference::neon_only;
              return SimdPreference::automatic;
            }

          const char *enabled = std::getenv("ALEPH_FFT_ENABLE_AVX2");
          if (enabled != nullptr and enabled[0] != '\0' and enabled[0] != '0')
            return SimdPreference::avx2_only;

          return SimdPreference::automatic;
        }();
      return preference;
    }

    /** @brief Returns the default SIMD backend used by standalone plan
     *         transforms for this precision.
     */
    [[nodiscard]] static SimdBackend
    simd_backend() noexcept
    {
      switch (simd_preference())
        {
        case SimdPreference::avx2_only:
          return detected_simd_backend() == SimdBackend::avx2 ? SimdBackend::avx2 : SimdBackend::scalar;
        case SimdPreference::neon_only:
          return detected_simd_backend() == SimdBackend::neon ? SimdBackend::neon : SimdBackend::scalar;
        case SimdPreference::scalar_only:
        case SimdPreference::automatic:
        default:
          return SimdBackend::scalar;
        }
    }

    /** @brief Returns the active default SIMD backend name for this precision. */
    [[nodiscard]] static const char *
    simd_backend_name() noexcept
    {
      return simd_backend_name(simd_backend());
    }

    /** @brief Returns the SIMD backend selected for throughput-oriented batch
     *         plan paths under the current runtime policy.
     */
    [[nodiscard]] static SimdBackend
    batched_plan_simd_backend() noexcept
    {
      if (simd_preference() == SimdPreference::scalar_only)
        return SimdBackend::scalar;

      const SimdBackend detected = detected_simd_backend();
      switch (simd_preference())
        {
        case SimdPreference::avx2_only:
          return detected == SimdBackend::avx2 ? SimdBackend::avx2 : SimdBackend::scalar;
        case SimdPreference::neon_only:
          return detected == SimdBackend::neon ? SimdBackend::neon : SimdBackend::scalar;
        case SimdPreference::automatic:
          return detected;
        case SimdPreference::scalar_only:
        default:
          return SimdBackend::scalar;
        }
    }

    /** @brief Returns the active batch-plan SIMD backend name. */
    [[nodiscard]] static const char *
    batched_plan_simd_backend_name() noexcept
    {
      return simd_backend_name(batched_plan_simd_backend());
    }

    /** @brief Returns the requested SIMD policy name. */
    [[nodiscard]] static const char *
    simd_preference_name() noexcept
    {
      return simd_preference_name(simd_preference());
    }

    /** @brief Returns whether AVX2 dispatch is supported at runtime. */
    [[nodiscard]] static bool
    avx2_runtime_available() noexcept
    {
      return avx2_dispatch_available();
    }

    /** @brief Returns whether NEON dispatch is supported at runtime. */
    [[nodiscard]] static bool
    neon_runtime_available() noexcept
    {
      return neon_dispatch_available();
    }

    /** @brief Precomputed FFT plan for repeated transforms of the same size.
     *
     *  A Plan precomputes the state needed for a given transform size N.
     *  Power-of-two sizes cache the bit-reversal permutation and radix-2/radix-4
     *  twiddle tables. Other sizes cache either a mixed-radix factorization or
     *  the Bluestein convolution kernel. Subsequent calls to @ref transform
     *  reuse these tables instead of rebuilding them.
     *
     *  This is beneficial when multiple transforms of the same size are
     *  performed (e.g., streaming audio, sliding-window spectral analysis).
     *
     *  @note Construction cost is O(N). Each transform execution is O(N log N).
     *
     *  @code
     *  FFT<double>::Plan plan(1024);
     *  plan.transform(signal_a, false);  // forward, in-place
     *  plan.transform(signal_b, false);  // reuses precomputed tables
     *  auto spectrum = plan.transformed(signal_c);  // functional variant
     *  @endcode
     */
    class Plan
    {
      enum class Strategy
      {
        empty,
        power_of_two,
        mixed_radix,
        bluestein
      };

      size_t n_ = 0;
      Strategy strategy_ = Strategy::empty;
      size_t log_n_ = 0;
      Array<size_t> bit_rev_;
      Array<Complex> twiddles_;
      std::shared_ptr<const Plan> half_plan_;
      Array<size_t> factors_;
      Array<Complex> roots_;
      size_t bluestein_size_ = 0;
      Array<Complex> bluestein_chirp_;
      Array<Complex> bluestein_kernel_forward_;
      Array<Complex> bluestein_kernel_inverse_;
      std::shared_ptr<const Plan> bluestein_plan_;

      void
      initialize_power_of_two_plan()
      {
        strategy_ = Strategy::power_of_two;

        log_n_ = 0;
        for (size_t tmp = n_; tmp > 1; tmp >>= 1)
          ++log_n_;

        bit_rev_ = Array<size_t>::create(n_);
        bit_rev_(0) = 0;
        for (size_t i = 1, j = 0; i < n_; ++i)
          {
            size_t bit = n_ >> 1;
            for (; j & bit; bit >>= 1)
              j ^= bit;
            j ^= bit;
            bit_rev_(i) = j;
          }

        twiddles_ = Array<Complex>::create(n_ - 1);
        for (size_t stage = 0; stage < log_n_; ++stage)
          {
            const size_t half = static_cast<size_t>(1) << stage;
            const size_t len = half << 1;
            const Real angle = Real(-2) * std::numbers::pi_v<Real>
                               / static_cast<Real>(len);
            const size_t offset = half - 1;
            for (size_t j = 0; j < half; ++j)
              twiddles_(offset + j) = std::polar(Real(1),
                                                 angle * static_cast<Real>(j));
          }

        half_plan_ = std::make_shared<Plan>(n_ / 2);
      }

      void
      initialize_mixed_radix_plan()
      {
        strategy_ = Strategy::mixed_radix;
        roots_ = Array<Complex>::create(n_);
        for (size_t i = 0; i < n_; ++i)
          roots_(i) = std::polar(Real(1),
                                 Real(-2) * std::numbers::pi_v<Real>
                                 * static_cast<Real>(i)
                                 / static_cast<Real>(n_));
      }

      void
      initialize_bluestein_plan()
      {
        strategy_ = Strategy::bluestein;
        bluestein_size_ = FFT::next_power_of_two(2 * n_ - 1);
        bluestein_plan_ = std::make_shared<Plan>(bluestein_size_);

        bluestein_chirp_ = Array<Complex>::create(n_);
        for (size_t i = 0; i < n_; ++i)
          {
            const Real value = static_cast<Real>(i);
            const Real phase = std::numbers::pi_v<Real> * value * value
                               / static_cast<Real>(n_);
            bluestein_chirp_(i) = std::polar(Real(1), phase);
          }

        Array<Complex> forward_kernel = Array<Complex>::create(bluestein_size_);
        Array<Complex> inverse_kernel = Array<Complex>::create(bluestein_size_);
        for (size_t i = 0; i < bluestein_size_; ++i)
          {
            forward_kernel(i) = Complex(Real(0), Real(0));
            inverse_kernel(i) = Complex(Real(0), Real(0));
          }

        forward_kernel(0) = Complex(Real(1), Real(0));
        inverse_kernel(0) = Complex(Real(1), Real(0));
        for (size_t i = 1; i < n_; ++i)
          {
            forward_kernel(i) = bluestein_chirp_[i];
            forward_kernel(bluestein_size_ - i) = bluestein_chirp_[i];

            inverse_kernel(i) = std::conj(bluestein_chirp_[i]);
            inverse_kernel(bluestein_size_ - i) = std::conj(bluestein_chirp_[i]);
          }

        bluestein_kernel_forward_ = forward_kernel;
        bluestein_kernel_inverse_ = inverse_kernel;
        bluestein_plan_->transform(bluestein_kernel_forward_, false);
        bluestein_plan_->transform(bluestein_kernel_inverse_, false);
      }

      void apply_bit_reversal(Array<Complex> & a) const noexcept
      {
        for (size_t i = 0; i < n_; ++i)
          if (i < bit_rev_(i))
            std::swap(a(i), a(bit_rev_(i)));
      }

# if ALEPH_FFT_HAS_X86_AVX2_DISPATCH
      static ALEPH_FFT_AVX2_TARGET __m256d
      avx2_load_complex_pair(const Complex *ptr) requires std::same_as<Real, double>
      {
        static_assert(sizeof(Complex) == sizeof(double) * 2);
        return _mm256_loadu_pd(reinterpret_cast<const double *>(ptr));
      }

      static ALEPH_FFT_AVX2_TARGET void
      avx2_store_complex_pair(Complex *ptr,
                              const __m256d value) requires std::same_as<Real, double>
      {
        static_assert(sizeof(Complex) == sizeof(double) * 2);
        _mm256_storeu_pd(reinterpret_cast<double *>(ptr), value);
      }

      [[nodiscard]] static ALEPH_FFT_AVX2_TARGET __m256d
      avx2_conjugate_pair(const __m256d value) requires std::same_as<Real, double>
      {
        const __m256d mask = _mm256_set_pd(-0.0, 0.0, -0.0, 0.0);
        return _mm256_xor_pd(value, mask);
      }

      [[nodiscard]] static ALEPH_FFT_AVX2_TARGET __m256d
      avx2_complex_mul_pair(const __m256d lhs,
                            const __m256d rhs) requires std::same_as<Real, double>
      {
        const __m256d rhs_real = _mm256_movedup_pd(rhs);
        const __m256d rhs_imag = _mm256_permute_pd(rhs, 0xF);
        const __m256d lhs_swapped = _mm256_permute_pd(lhs, 0x5);
        return _mm256_addsub_pd(_mm256_mul_pd(lhs, rhs_real),
                                _mm256_mul_pd(lhs_swapped, rhs_imag));
      }

      [[nodiscard]] static ALEPH_FFT_AVX2_TARGET __m256d
      avx2_rotate_js_pair(const __m256d value,
                          const bool invert) requires std::same_as<Real, double>
      {
        const __m256d swapped = _mm256_permute_pd(value, 0x5);
        const __m256d forward_mask = _mm256_set_pd(-0.0, 0.0, -0.0, 0.0);
        const __m256d inverse_mask = _mm256_set_pd(0.0, -0.0, 0.0, -0.0);
        return _mm256_xor_pd(swapped, invert ? inverse_mask : forward_mask);
      }

      static ALEPH_FFT_AVX2_TARGET void
      avx2_apply_r2_block(Array<Complex> & a, const size_t base)
        requires std::same_as<Real, double>
      {
        const __m256d uv = avx2_load_complex_pair(&a[base]);
        const __m128d u = _mm256_castpd256_pd128(uv);
        const __m128d v = _mm256_extractf128_pd(uv, 1);
        const __m128d sum = _mm_add_pd(u, v);
        const __m128d diff = _mm_sub_pd(u, v);
        avx2_store_complex_pair(&a[base], _mm256_set_m128d(diff, sum));
      }

      static ALEPH_FFT_AVX2_TARGET void
      avx2_apply_r2_stage(Array<Complex> & a, const size_t blocks)
        requires std::same_as<Real, double>
      {
        for (size_t block = 0; block < blocks; ++block)
          avx2_apply_r2_block(a, block << 1);
      }

      ALEPH_FFT_AVX2_TARGET void
      avx2_apply_r4_block(Array<Complex> & a,
                          const size_t quarter,
                          const size_t tw_w1_off,
                          const size_t tw_w2_off,
                          const bool invert,
                          const size_t block) const
        requires std::same_as<Real, double>
      {
        const size_t base = block * (quarter << 2);
        size_t j = 0;
        for (; j + 1 < quarter; j += 2)
          {
            __m256d w1 = avx2_load_complex_pair(&twiddles_[tw_w1_off + j]);
            __m256d w2 = avx2_load_complex_pair(&twiddles_[tw_w2_off + j]);
            if (invert)
              {
                w1 = avx2_conjugate_pair(w1);
                w2 = avx2_conjugate_pair(w2);
              }
            const __m256d w3 = avx2_complex_mul_pair(w1, w2);

            const size_t i0 = base + j;
            const size_t i1 = i0 + quarter;
            const size_t i2 = i1 + quarter;
            const size_t i3 = i2 + quarter;

            const __m256d a0 = avx2_load_complex_pair(&a[i0]);
            const __m256d a1 = avx2_complex_mul_pair(avx2_load_complex_pair(&a[i1]), w2);
            const __m256d a2 = avx2_complex_mul_pair(avx2_load_complex_pair(&a[i2]), w1);
            const __m256d a3 = avx2_complex_mul_pair(avx2_load_complex_pair(&a[i3]), w3);

            const __m256d p = _mm256_add_pd(a0, a1);
            const __m256d q = _mm256_sub_pd(a0, a1);
            const __m256d r = _mm256_add_pd(a2, a3);
            const __m256d s = _mm256_sub_pd(a2, a3);
            const __m256d js = avx2_rotate_js_pair(s, invert);

            avx2_store_complex_pair(&a[i0], _mm256_add_pd(p, r));
            avx2_store_complex_pair(&a[i1], _mm256_add_pd(q, js));
            avx2_store_complex_pair(&a[i2], _mm256_sub_pd(p, r));
            avx2_store_complex_pair(&a[i3], _mm256_sub_pd(q, js));
          }

        for (; j < quarter; ++j)
          {
            const Complex tw1 = twiddles_(tw_w1_off + j);
            const Complex tw2 = twiddles_(tw_w2_off + j);
            const Complex w1 = invert ? std::conj(tw1) : tw1;
            const Complex w2 = invert ? std::conj(tw2) : tw2;
            const Complex w3 = w1 * w2;

            const size_t i0 = base + j;
            const size_t i1 = i0 + quarter;
            const size_t i2 = i1 + quarter;
            const size_t i3 = i2 + quarter;

            const Complex a0 = a[i0];
            const Complex a1 = a[i1] * w2;
            const Complex a2 = a[i2] * w1;
            const Complex a3 = a[i3] * w3;

            const Complex p = a0 + a1;
            const Complex q = a0 - a1;
            const Complex r = a2 + a3;
            const Complex s = a2 - a3;

            const Complex js = invert ? Complex(-s.imag(), s.real()) : Complex(s.imag(), -s.real());

            a(i0) = p + r;
            a(i1) = q + js;
            a(i2) = p - r;
            a(i3) = q - js;
          }
      }

      static ALEPH_FFT_AVX2_TARGET void
      avx2_scale_output(Array<Complex> & a,
                        const Real inv_n) requires std::same_as<Real, double>
      {
        const __m256d scale = _mm256_set1_pd(inv_n);
        size_t i = 0;
        for (; i + 1 < a.size(); i += 2)
          avx2_store_complex_pair(&a[i],
                                  _mm256_mul_pd(avx2_load_complex_pair(&a[i]), scale));
        for (; i < a.size(); ++i)
          a(i) *= inv_n;
      }

      ALEPH_FFT_AVX2_TARGET void
      avx2_apply_r4_stage(Array<Complex> & a,
                          const size_t quarter,
                          const size_t blocks,
                          const size_t tw_w1_off,
                          const size_t tw_w2_off,
                          const bool invert) const
        requires std::same_as<Real, double>
      {
        for (size_t block = 0; block < blocks; ++block)
          avx2_apply_r4_block(a, quarter, tw_w1_off, tw_w2_off, invert, block);
      }
# endif

# if ALEPH_FFT_HAS_ARM_NEON_DISPATCH
      struct NeonComplexPairFloat
      {
        float32x2_t re;
        float32x2_t im;
      };

#   if ALEPH_FFT_HAS_ARM_NEON_DOUBLE
      struct NeonComplexPairDouble
      {
        float64x2_t re;
        float64x2_t im;
      };
#   endif

      [[nodiscard]] static NeonComplexPairFloat
      neon_load_complex_pair(const Complex *ptr) requires std::same_as<Real, float>
      {
        static_assert(sizeof(Complex) == sizeof(float) * 2);
        const float32x2x2_t interleaved =
            vld2_f32(reinterpret_cast<const float *>(ptr));
        return {interleaved.val[0], interleaved.val[1]};
      }

      static void
      neon_store_complex_pair(Complex *ptr,
                              const NeonComplexPairFloat value) requires std::same_as<Real, float>
      {
        static_assert(sizeof(Complex) == sizeof(float) * 2);
        const float32x2x2_t interleaved = {value.re, value.im};
        vst2_f32(reinterpret_cast<float *>(ptr), interleaved);
      }

      [[nodiscard]] static NeonComplexPairFloat
      neon_conjugate_pair(const NeonComplexPairFloat value) requires std::same_as<Real, float>
      {
        return {value.re, vneg_f32(value.im)};
      }

      [[nodiscard]] static NeonComplexPairFloat
      neon_complex_mul_pair(const NeonComplexPairFloat lhs,
                            const NeonComplexPairFloat rhs) requires std::same_as<Real, float>
      {
        return {
              vsub_f32(vmul_f32(lhs.re, rhs.re), vmul_f32(lhs.im, rhs.im)),
              vadd_f32(vmul_f32(lhs.re, rhs.im), vmul_f32(lhs.im, rhs.re))
            };
      }

      [[nodiscard]] static NeonComplexPairFloat
      neon_add_pair(const NeonComplexPairFloat lhs,
                    const NeonComplexPairFloat rhs) requires std::same_as<Real, float>
      {
        return {vadd_f32(lhs.re, rhs.re), vadd_f32(lhs.im, rhs.im)};
      }

      [[nodiscard]] static NeonComplexPairFloat
      neon_sub_pair(const NeonComplexPairFloat lhs,
                    const NeonComplexPairFloat rhs) requires std::same_as<Real, float>
      {
        return {vsub_f32(lhs.re, rhs.re), vsub_f32(lhs.im, rhs.im)};
      }

      [[nodiscard]] static NeonComplexPairFloat
      neon_rotate_js_pair(const NeonComplexPairFloat value,
                          const bool invert) requires std::same_as<Real, float>
      {
        return invert ?
                 NeonComplexPairFloat{vneg_f32(value.im), value.re} :
                 NeonComplexPairFloat{value.im, vneg_f32(value.re)};
      }

#   if ALEPH_FFT_HAS_ARM_NEON_DOUBLE
      [[nodiscard]] static NeonComplexPairDouble
      neon_load_complex_pair(const Complex *ptr) requires std::same_as<Real, double>
      {
        static_assert(sizeof(Complex) == sizeof(double) * 2);
        const float64x2x2_t interleaved =
            vld2q_f64(reinterpret_cast<const double *>(ptr));
        return {interleaved.val[0], interleaved.val[1]};
      }

      static void
      neon_store_complex_pair(Complex *ptr,
                              const NeonComplexPairDouble value) requires std::same_as<Real, double>
      {
        static_assert(sizeof(Complex) == sizeof(double) * 2);
        const float64x2x2_t interleaved = {value.re, value.im};
        vst2q_f64(reinterpret_cast<double *>(ptr), interleaved);
      }

      [[nodiscard]] static NeonComplexPairDouble
      neon_conjugate_pair(const NeonComplexPairDouble value) requires std::same_as<Real, double>
      {
        return {value.re, vnegq_f64(value.im)};
      }

      [[nodiscard]] static NeonComplexPairDouble
      neon_complex_mul_pair(const NeonComplexPairDouble lhs,
                            const NeonComplexPairDouble rhs) requires std::same_as<Real, double>
      {
        return {
              vsubq_f64(vmulq_f64(lhs.re, rhs.re), vmulq_f64(lhs.im, rhs.im)),
              vaddq_f64(vmulq_f64(lhs.re, rhs.im), vmulq_f64(lhs.im, rhs.re))
            };
      }

      [[nodiscard]] static NeonComplexPairDouble
      neon_add_pair(const NeonComplexPairDouble lhs,
                    const NeonComplexPairDouble rhs) requires std::same_as<Real, double>
      {
        return {vaddq_f64(lhs.re, rhs.re), vaddq_f64(lhs.im, rhs.im)};
      }

      [[nodiscard]] static NeonComplexPairDouble
      neon_sub_pair(const NeonComplexPairDouble lhs,
                    const NeonComplexPairDouble rhs) requires std::same_as<Real, double>
      {
        return {vsubq_f64(lhs.re, rhs.re), vsubq_f64(lhs.im, rhs.im)};
      }

      [[nodiscard]] static NeonComplexPairDouble
      neon_rotate_js_pair(const NeonComplexPairDouble value,
                          const bool invert) requires std::same_as<Real, double>
      {
        return invert ?
                 NeonComplexPairDouble{vnegq_f64(value.im), value.re} :
                 NeonComplexPairDouble{value.im, vnegq_f64(value.re)};
      }
#   endif

      void
      neon_apply_r4_block(Array<Complex> & a,
                          const size_t quarter,
                          const size_t tw_w1_off,
                          const size_t tw_w2_off,
                          const bool invert,
                          const size_t block) const
      {
        const size_t base = block * (quarter << 2);
        size_t j = 0;
        for (; j + 1 < quarter; j += 2)
          {
            auto w1 = neon_load_complex_pair(&twiddles_[tw_w1_off + j]);
            auto w2 = neon_load_complex_pair(&twiddles_[tw_w2_off + j]);
            if (invert)
              {
                w1 = neon_conjugate_pair(w1);
                w2 = neon_conjugate_pair(w2);
              }
            const auto w3 = neon_complex_mul_pair(w1, w2);

            const size_t i0 = base + j;
            const size_t i1 = i0 + quarter;
            const size_t i2 = i1 + quarter;
            const size_t i3 = i2 + quarter;

            const auto a0 = neon_load_complex_pair(&a[i0]);
            const auto a1 = neon_complex_mul_pair(neon_load_complex_pair(&a[i1]), w2);
            const auto a2 = neon_complex_mul_pair(neon_load_complex_pair(&a[i2]), w1);
            const auto a3 = neon_complex_mul_pair(neon_load_complex_pair(&a[i3]), w3);

            const auto p = neon_add_pair(a0, a1);
            const auto q = neon_sub_pair(a0, a1);
            const auto r = neon_add_pair(a2, a3);
            const auto s = neon_sub_pair(a2, a3);
            const auto js = neon_rotate_js_pair(s, invert);

            neon_store_complex_pair(&a[i0], neon_add_pair(p, r));
            neon_store_complex_pair(&a[i1], neon_add_pair(q, js));
            neon_store_complex_pair(&a[i2], neon_sub_pair(p, r));
            neon_store_complex_pair(&a[i3], neon_sub_pair(q, js));
          }

        for (; j < quarter; ++j)
          {
            const Complex tw1 = twiddles_(tw_w1_off + j);
            const Complex tw2 = twiddles_(tw_w2_off + j);
            const Complex w1 = invert ? std::conj(tw1) : tw1;
            const Complex w2 = invert ? std::conj(tw2) : tw2;
            const Complex w3 = w1 * w2;

            const size_t i0 = base + j;
            const size_t i1 = i0 + quarter;
            const size_t i2 = i1 + quarter;
            const size_t i3 = i2 + quarter;

            const Complex a0 = a[i0];
            const Complex a1 = a[i1] * w2;
            const Complex a2 = a[i2] * w1;
            const Complex a3 = a[i3] * w3;

            const Complex p = a0 + a1;
            const Complex q = a0 - a1;
            const Complex r = a2 + a3;
            const Complex s = a2 - a3;

            const Complex js = invert ? Complex(-s.imag(), s.real()) : Complex(s.imag(), -s.real());

            a(i0) = p + r;
            a(i1) = q + js;
            a(i2) = p - r;
            a(i3) = q - js;
          }
      }

      static void
      neon_scale_output(Array<Complex> & a, const Real inv_n)
      {
        size_t i = 0;
        if constexpr (std::same_as<Real, float>)
          {
            const float32x2_t scale = vdup_n_f32(inv_n);
            for (; i + 1 < a.size(); i += 2)
              {
                auto pair = neon_load_complex_pair(&a[i]);
                pair.re = vmul_f32(pair.re, scale);
                pair.im = vmul_f32(pair.im, scale);
                neon_store_complex_pair(&a[i], pair);
              }
          }
#   if ALEPH_FFT_HAS_ARM_NEON_DOUBLE
      else if constexpr (std::same_as<Real, double>)
      {
        const float64x2_t scale = vdupq_n_f64(inv_n);
        for (; i + 1 < a.size(); i += 2)
          {
            auto pair = neon_load_complex_pair(&a[i]);
            pair.re = vmulq_f64(pair.re, scale);
            pair.im = vmulq_f64(pair.im, scale);
            neon_store_complex_pair(&a[i], pair);
          }
      }
#   endif
      for (; i<a.size();++i)
      a (i)*= inv_n;
      }

      void
      neon_apply_r4_stage(Array<Complex> & a,
                          const size_t quarter,
                          const size_t blocks,
                          const size_t tw_w1_off,
                          const size_t tw_w2_off,
                          const bool invert) const
      {
        for (size_t block = 0; block < blocks; ++block)
          neon_apply_r4_block(a, quarter, tw_w1_off, tw_w2_off, invert, block);
      }
# endif

      [[nodiscard]] bool
      automatic_batch_simd_candidate(const size_t batch_size) const noexcept
      {
        return batch_size >= 4
               and strategy_ == Strategy::power_of_two
               and n_ >= 128
               and (log_n_ & 1) == 1;
      }

      [[nodiscard]] bool
      should_use_avx2(ThreadPool *pool,
                      const bool prefer_simd) const noexcept
      {
        if constexpr (not std::same_as<Real, double>)
          return false;

        if (strategy_ != Strategy::power_of_two or (log_n_ & 1) == 0)
          return false;

        if (pool != nullptr and pool->num_threads() > 1)
          return false;

        if (FFT::detected_simd_backend() != SimdBackend::avx2)
          return false;

        switch (FFT::simd_preference())
          {
          case SimdPreference::scalar_only:
          case SimdPreference::neon_only:
            return false;
          case SimdPreference::avx2_only:
            return true;
          case SimdPreference::automatic:
          default:
            return prefer_simd;
          }
      }

      [[nodiscard]] bool
      should_use_neon(ThreadPool *pool,
                      const bool prefer_simd) const noexcept
      {
        if (strategy_ != Strategy::power_of_two or (log_n_ & 1) == 0)
          return false;

        if (pool != nullptr and pool->num_threads() > 1)
          return false;

        if (FFT::detected_simd_backend() != SimdBackend::neon)
          return false;

        switch (FFT::simd_preference())
          {
          case SimdPreference::scalar_only:
          case SimdPreference::avx2_only:
            return false;
          case SimdPreference::neon_only:
            return true;
          case SimdPreference::automatic:
          default:
            return prefer_simd;
          }
      }

      void apply_butterflies(Array<Complex> & a, const bool invert,
                             ThreadPool *pool,
                             const size_t chunk_size,
                             const bool prefer_simd) const
      {
        const bool use_avx2 = should_use_avx2(pool, prefer_simd);
        [[maybe_unused]] const bool use_neon = should_use_neon(pool, prefer_simd);

# if ALEPH_FFT_HAS_X86_AVX2_DISPATCH
        if constexpr (std::same_as<Real, double>)
          if (use_avx2 and (pool == nullptr or pool->num_threads() <= 1))
            {
              size_t stage = 0;

              if (log_n_ & 1)
                {
                  avx2_apply_r2_stage(a, n_ >> 1);
                  stage = 1;
                }

              while (stage + 1 < log_n_)
                {
                  const size_t s_low = stage;
                  const size_t s_high = stage + 1;
                  const size_t quarter = static_cast<size_t>(1) << s_low;
                  const size_t blocks = n_ >> (s_high + 1);
                  const size_t tw_w1_off = (static_cast<size_t>(1) << s_high) - 1;
                  const size_t tw_w2_off = quarter - 1;
                  avx2_apply_r4_stage(a, quarter, blocks,
                                      tw_w1_off, tw_w2_off, invert);
                  stage += 2;
                }

              if (invert)
                avx2_scale_output(a, Real(1) / static_cast<Real>(n_));
              return;
            }
# endif

# if ALEPH_FFT_HAS_ARM_NEON_DISPATCH
        if (use_neon and (pool == nullptr or pool->num_threads() <= 1))
          {
            size_t stage = 0;

            if (log_n_ & 1)
              {
                const size_t blocks = n_ >> 1;
                for (size_t block = 0; block < blocks; ++block)
                  {
                    const size_t base = block << 1;
                    const Complex u = a[base];
                    const Complex v = a[base + 1];
                    a(base) = u + v;
                    a(base + 1) = u - v;
                  }
                stage = 1;
              }

            while (stage + 1 < log_n_)
              {
                const size_t s_low = stage;
                const size_t s_high = stage + 1;
                const size_t quarter = size_t(1) << s_low;
                const size_t blocks = n_ >> (s_high + 1);
                const size_t tw_w1_off = (size_t(1) << s_high) - 1;
                const size_t tw_w2_off = quarter - 1;
                neon_apply_r4_stage(a, quarter, blocks,
                                    tw_w1_off, tw_w2_off, invert);
                stage += 2;
              }

            if (invert)
              neon_scale_output(a, Real(1) / static_cast<Real>(n_));
            return;
          }
# endif

        size_t stage = 0;

        // If odd number of stages, one radix-2 pass first (twiddle = 1)
        if (log_n_ & 1)
          {
            const size_t blocks = n_ >> 1;
            auto r2_block = [&a](const size_t block)
              {
                const size_t base = block << 1;
                const Complex u = a[base];
                const Complex v = a[base + 1];
                a(base) = u + v;
                a(base + 1) = u - v;
              };

            if (pool != nullptr and pool->num_threads() > 1 and blocks > 1)
              parallel_for_index(*pool, 0, blocks, r2_block, chunk_size);
            else
              for (size_t block = 0; block < blocks; ++block)
                r2_block(block);

            stage = 1;
          }

        // Radix-4 stages: each fuses two radix-2 stages.
        // w1 and w2 are looked up directly from the precomputed table;
        // w3 = w1 * w2 requires only one multiplication.
        while (stage + 1 < log_n_)
          {
            const size_t s_low = stage;
            const size_t s_high = stage + 1;
            const size_t quarter = static_cast<size_t>(1) << s_low;
            const size_t blocks = n_ >> (s_high + 1);
            const size_t tw_w1_off = (static_cast<size_t>(1) << s_high) - 1;
            const size_t tw_w2_off = quarter - 1;

            auto r4_block = [this, &a, quarter, tw_w1_off, tw_w2_off, invert]
            (const size_t block)
              {
                const size_t base = block * (quarter << 2);
                for (size_t j = 0; j < quarter; ++j)
                  {
                    const Complex tw1 = twiddles_(tw_w1_off + j);
                    const Complex tw2 = twiddles_(tw_w2_off + j);
                    const Complex w1 = invert ? std::conj(tw1) : tw1;
                    const Complex w2 = invert ? std::conj(tw2) : tw2;
                    const Complex w3 = w1 * w2;

                    const size_t i0 = base + j;
                    const size_t i1 = i0 + quarter;
                    const size_t i2 = i1 + quarter;
                    const size_t i3 = i2 + quarter;

                    const Complex a0 = a[i0];
                    const Complex a1 = a[i1] * w2;
                    const Complex a2 = a[i2] * w1;
                    const Complex a3 = a[i3] * w3;

                    const Complex p = a0 + a1;
                    const Complex q = a0 - a1;
                    const Complex r = a2 + a3;
                    const Complex s = a2 - a3;

                    const Complex js = invert ? Complex(-s.imag(), s.real()) : Complex(s.imag(), -s.real());

                    a(i0) = p + r;
                    a(i1) = q + js;
                    a(i2) = p - r;
                    a(i3) = q - js;
                  }
              };

            if (pool != nullptr and pool->num_threads() > 1 and blocks > 1)
              parallel_for_index(*pool, 0, blocks, r4_block, chunk_size);
            else
              for (size_t block = 0; block < blocks; ++block)
                r4_block(block);

            stage += 2;
          }

        if (invert)
          {
            const Real inv_n = Real(1) / static_cast<Real>(n_);
            for (size_t i = 0; i < n_; ++i)
              a(i) *= inv_n;
          }
      }

      void
      apply_transform(Array<Complex> & a,
                      const bool invert,
                      ThreadPool *pool,
                      const size_t chunk_size,
                      const bool prefer_simd) const
      {
        ah_invalid_argument_if(a.size() != n_)
          << "FFT::Plan::transform: input size " << a.size()
          << " does not match plan size " << n_;

        if (n_ <= 1)
          return;

        switch (strategy_)
          {
          case Strategy::power_of_two:
            apply_bit_reversal(a);
            apply_butterflies(a, invert, pool, chunk_size, prefer_simd);
            break;
          case Strategy::mixed_radix:
            apply_mixed_radix_transform(a, invert);
            break;
          case Strategy::bluestein:
            apply_bluestein_transform(a, invert, pool, chunk_size);
            break;
          case Strategy::empty:
            break;
          }
      }

      [[nodiscard]] Complex
      root_for_length(const size_t length,
                      const size_t exponent,
                      const bool invert) const
      {
        if (length <= 1)
          return Complex(Real(1), Real(0));

        const size_t stride = n_ / length;
        const size_t index = ((exponent % length) * stride) % n_;
        const Complex root = roots_[index];
        return invert ? std::conj(root) : root;
      }

      [[nodiscard]] Array<Complex>
      mixed_radix_transform_recursive(const Array<Complex> & input,
                                      const size_t factor_index,
                                      const size_t current_size,
                                      const bool invert) const
      {
        if (current_size <= 1)
          return input;

        const size_t factor = factors_[factor_index];
        const size_t sub_size = current_size / factor;

        Array<Array<Complex>> subtransforms;
        subtransforms.reserve(factor);
        for (size_t q = 0; q < factor; ++q)
          {
            Array<Complex> subsequence;
            subsequence.reserve(sub_size);
            for (size_t t = 0; t < sub_size; ++t)
              subsequence.append(input[factor * t + q]);
            subtransforms.append(mixed_radix_transform_recursive(subsequence,
                                                                 factor_index + 1,
                                                                 sub_size,
                                                                 invert));
          }

        Array<Complex> output = Array<Complex>::create(current_size);
        for (size_t k1 = 0; k1 < sub_size; ++k1)
          for (size_t p = 0; p < factor; ++p)
            {
              Complex sum(Real(0), Real(0));
              const size_t target = k1 + sub_size * p;
              for (size_t q = 0; q < factor; ++q)
                sum += subtransforms[q][k1]
                    * root_for_length(current_size, q * target, invert);
              output(target) = sum;
            }

        return output;
      }

      void
      apply_mixed_radix_transform(Array<Complex> & a, const bool invert) const
      {
        Array<Complex> output = mixed_radix_transform_recursive(a, 0, n_, invert);
        if (invert)
          {
            const Real inv_n = Real(1) / static_cast<Real>(n_);
            for (size_t i = 0; i < n_; ++i)
              output(i) *= inv_n;
          }
        a = std::move(output);
      }

      void
      apply_bluestein_transform(Array<Complex> & a,
                                const bool invert,
                                ThreadPool *pool,
                                const size_t chunk_size) const
      {
        Array<Complex> work = Array<Complex>::create(bluestein_size_);

        auto initialize = [this, &a, &work, invert](const size_t i)
          {
            if (i >= n_)
              {
                work(i) = Complex(Real(0), Real(0));
                return;
              }

            const Complex chirp = invert ? bluestein_chirp_[i] : std::conj(bluestein_chirp_[i]);
            work(i) = a[i] * chirp;
          };

        if (pool != nullptr and pool->num_threads() > 1 and bluestein_size_ > 1)
          parallel_for_index(*pool, 0, bluestein_size_, initialize, chunk_size);
        else
          for (size_t i = 0; i < bluestein_size_; ++i)
            initialize(i);

        if (pool != nullptr)
          bluestein_plan_->ptransform(*pool, work, false, chunk_size);
        else
          bluestein_plan_->transform(work, false);

        const Array<Complex> & kernel = invert ? bluestein_kernel_inverse_ : bluestein_kernel_forward_;

        auto pointwise = [&work, &kernel](const size_t i)
          {
            work(i) *= kernel[i];
          };

        if (pool != nullptr and pool->num_threads() > 1 and bluestein_size_ > 1)
          parallel_for_index(*pool, 0, bluestein_size_, pointwise, chunk_size);
        else
          for (size_t i = 0; i < bluestein_size_; ++i)
            pointwise(i);

        if (pool != nullptr)
          bluestein_plan_->ptransform(*pool, work, true, chunk_size);
        else
          bluestein_plan_->transform(work, true);

        auto finalize = [this, &a, &work, invert](const size_t i)
          {
            const Complex chirp = invert ? bluestein_chirp_[i] : std::conj(bluestein_chirp_[i]);
            a(i) = work[i] * chirp;
            if (invert)
              a(i) /= static_cast<Real>(n_);
          };

        if (pool != nullptr and pool->num_threads() > 1 and n_ > 1)
          parallel_for_index(*pool, 0, n_, finalize, chunk_size);
        else
          for (size_t i = 0; i < n_; ++i)
            finalize(i);
      }

      [[nodiscard]] bool
      supports_power_of_two_real_optimization() const noexcept
      {
        return strategy_ == Strategy::power_of_two
               and n_ > 1
               and half_plan_ != nullptr;
      }

    public:
      /** @brief Constructs an empty plan (size 0). */
      Plan() = default;

      /** @brief Constructs a plan for transforms of size @p n.
       *
       *  Precomputes the bit-reversal permutation and all twiddle factors.
       *  Construction cost is O(N).
       *
       *  @param n Transform size (must be positive).
       *  @throw std::invalid_argument if n is zero.
       */
      explicit Plan(const size_t n) : n_(n)
      {
        ah_invalid_argument_if(n == 0)
          << "FFT::Plan: size must be positive";

        if (n == 1)
          {
            strategy_ = Strategy::empty;
            return;
          }

        if (is_power_of_two(n_))
          {
            initialize_power_of_two_plan();
            return;
          }

        factors_ = FFT::factor_small_radices(n_);
        if (not factors_.is_empty())
          {
            initialize_mixed_radix_plan();
            return;
          }

        initialize_bluestein_plan();
      }

      /** @brief Returns the transform size this plan was built for. */
      [[nodiscard]] size_t size() const noexcept { return n_; }

      /** @brief Computes the FFT or IFFT in-place using precomputed tables.
       *
       *  @param a Array of complex values (size must equal plan size).
       *  @param invert If true, compute the inverse transform (with 1/N scaling).
       *  @throw std::invalid_argument if a.size() differs from plan size.
       */
      void transform(Array<Complex> & a, const bool invert) const
      {
        apply_transform(a, invert, nullptr, 0, false);
      }

      /** @brief Computes the FFT or IFFT returning a new array.
       *
       *  @param input Input array (size must equal plan size).
       *  @param invert If true, compute the inverse transform.
       *  @return A new array containing the transformed values.
       */
      [[nodiscard]] Array<Complex>
      transformed(const Array<Complex> & input,
                  const bool invert = false) const
      {
        Array<Complex> output = input;
        transform(output, invert);
        return output;
      }

      /** @brief Returns the inverse transform as a new array. */
      [[nodiscard]] Array<Complex>
      inverse_transform(const Array<Complex> & input) const
      {
        return transformed(input, true);
      }

      /** @brief Returns the IFFT projected back to real values.
       *
       *  @throw std::domain_error if the result has non-negligible
       *         imaginary parts.
       */
      [[nodiscard]] Array<Real>
      inverse_transform_real(const Array<Complex> & input) const
      {
        ah_invalid_argument_if(input.size() != n_)
          << "FFT::Plan::inverse_transform_real: input size " << input.size()
          << " does not match plan size " << n_;

        if (n_ == 0)
          return {};

        if (supports_power_of_two_real_optimization())
          return FFT::inverse_transform_real_optimized_impl(
                                                            input, "FFT::Plan::inverse_transform_real", nullptr, 0,
                                                            [this](Array<Complex> & packed, ThreadPool *, const size_t)
                                                              {
                                                                half_plan_->transform(packed, true);
                                                              });

        FFT::validate_real_spectrum(input, "FFT::Plan::inverse_transform_real");
        Array<Complex> output = input;
        transform(output, true);
        return FFT::project_real_output(output, "FFT::Plan::inverse_transform_real");
      }

      /** @brief Parallel IFFT projected back to real values. */
      [[nodiscard]] Array<Real>
      pinverse_transform_real(ThreadPool & pool, const Array<Complex> & input,
                              const size_t chunk_size = 0) const
      {
        ah_invalid_argument_if(input.size() != n_)
          << "FFT::Plan::pinverse_transform_real: input size "
          << input.size() << " does not match plan size " << n_;

        if (n_ == 0)
          return {};

        if (supports_power_of_two_real_optimization())
          return FFT::inverse_transform_real_optimized_impl(
                                                            input, "FFT::Plan::inverse_transform_real", &pool,
                                                            chunk_size,
                                                            [this](Array<Complex> & packed, ThreadPool *inner_pool,
                                                                   const size_t inner_chunk_size)
                                                              {
                                                                if (inner_pool != nullptr)
                                                                  half_plan_->ptransform(*inner_pool, packed, true,
                                                                    inner_chunk_size);
                                                                else
                                                                  half_plan_->transform(packed, true);
                                                              });

        FFT::validate_real_spectrum(input, "FFT::Plan::inverse_transform_real");
        Array<Complex> output = input;
        ptransform(pool, output, true, chunk_size);
        return FFT::project_real_output(output, "FFT::Plan::inverse_transform_real");
      }

      /** @brief Parallel in-place transform using a thread pool.
       *
       *  @param pool Thread pool for parallel butterfly stages.
       *  @param a Array of complex values (size must equal plan size).
       *  @param invert If true, compute the inverse transform.
       *  @param chunk_size Minimum iterations per parallel chunk (0 = auto).
       */
      void ptransform(ThreadPool & pool, Array<Complex> & a,
                      const bool invert,
                      const size_t chunk_size = 0) const
      {
        apply_transform(a, invert, &pool, chunk_size, false);
      }

      /** @brief Parallel transform returning a new array. */
      [[nodiscard]] Array<Complex>
      ptransformed(ThreadPool & pool, const Array<Complex> & input,
                   const bool invert = false,
                   const size_t chunk_size = 0) const
      {
        Array<Complex> output = input;
        ptransform(pool, output, invert, chunk_size);
        return output;
      }

      /** @brief Parallel inverse transform returning a new array. */
      [[nodiscard]] Array<Complex>
      pinverse_transform(ThreadPool & pool, const Array<Complex> & input,
                         const size_t chunk_size = 0) const
      {
        return ptransformed(pool, input, true, chunk_size);
      }

      /** @brief In-place batch transform for equal-length complex inputs. */
      void
      transform_batch(Array<Array<Complex>> & batch,
                      const bool invert,
                      const bool prefer_simd = true) const
      {
        const bool effective_prefer_simd =
            prefer_simd and automatic_batch_simd_candidate(batch.size());
        for (size_t i = 0; i < batch.size(); ++i)
          {
            ah_invalid_argument_if(batch[i].size() != n_)
              << "FFT::Plan::transform_batch: batch item " << i
              << " has size " << batch[i].size()
              << " but plan size is " << n_;
            apply_transform(batch(i), invert, nullptr, 0, effective_prefer_simd);
          }
      }

      /** @brief Parallel batch transform for equal-length complex inputs.
       *
       *  When the batch has more than one item, parallelism is applied across
       *  batch elements and each individual transform runs sequentially to
       *  avoid nested thread-pool oversubscription.
       */
      void
      ptransform_batch(ThreadPool & pool,
                       Array<Array<Complex>> & batch,
                       const bool invert,
                       const size_t chunk_size = 0,
                       const bool prefer_simd = true) const
      {
        const bool effective_prefer_simd =
            prefer_simd and automatic_batch_simd_candidate(batch.size());
        for (size_t i = 0; i < batch.size(); ++i)
          ah_invalid_argument_if(batch[i].size() != n_)
            << "FFT::Plan::ptransform_batch: batch item " << i
            << " has size " << batch[i].size()
            << " but plan size is " << n_;

        if (batch.is_empty())
          return;

        if (batch.size() == 1 or pool.num_threads() <= 1)
          {
            for (size_t i = 0; i < batch.size(); ++i)
              apply_transform(batch(i),
                              invert,
                              &pool,
                              chunk_size,
                              effective_prefer_simd);
            return;
          }

        auto transform_one = [this, &batch, invert, effective_prefer_simd](const size_t i)
          {
            apply_transform(batch(i), invert, nullptr, 0, effective_prefer_simd);
          };
        parallel_for_index(pool, 0, batch.size(), transform_one, chunk_size);
      }

      /** @brief Returns the batch FFT/IFFT as a new array of arrays. */
      [[nodiscard]] Array<Array<Complex>>
      transformed_batch(const Array<Array<Complex>> & input,
                        const bool invert = false,
                        const bool prefer_simd = true) const
      {
        Array<Array<Complex>> output = input;
        transform_batch(output, invert, prefer_simd);
        return output;
      }

      /** @brief Returns the parallel batch FFT/IFFT as a new array of arrays. */
      [[nodiscard]] Array<Array<Complex>>
      ptransformed_batch(ThreadPool & pool,
                         const Array<Array<Complex>> & input,
                         const bool invert = false,
                         const size_t chunk_size = 0,
                         const bool prefer_simd = true) const
      {
        Array<Array<Complex>> output = input;
        ptransform_batch(pool, output, invert, chunk_size, prefer_simd);
        return output;
      }

      /** @brief Returns the inverse batch transform as a new array of arrays. */
      [[nodiscard]] Array<Array<Complex>>
      inverse_transform_batch(const Array<Array<Complex>> & input) const
      {
        return transformed_batch(input, true);
      }

      /** @brief Returns the parallel inverse batch transform. */
      [[nodiscard]] Array<Array<Complex>>
      pinverse_transform_batch(ThreadPool & pool,
                               const Array<Array<Complex>> & input,
                               const size_t chunk_size = 0,
                               const bool prefer_simd = true) const
      {
        return ptransformed_batch(pool, input, true, chunk_size, prefer_simd);
      }

      /** @brief Returns the inverse batch transform projected back to real
       *         values.
       */
      [[nodiscard]] Array<Array<Real>>
      inverse_transform_real_batch(const Array<Array<Complex>> & input) const
      {
        Array<Array<Real>> output(input.size(), Array<Real>());
        for (size_t i = 0; i < input.size(); ++i)
          {
            ah_invalid_argument_if(input[i].size() != n_)
              << "FFT::Plan::inverse_transform_real_batch: batch item " << i
              << " has size " << input[i].size()
              << " but plan size is " << n_;
            output(i) = inverse_transform_real(input[i]);
          }
        return output;
      }

      /** @brief Returns the parallel inverse batch transform projected back to
       *         real values.
       */
      [[nodiscard]] Array<Array<Real>>
      pinverse_transform_real_batch(ThreadPool & pool,
                                    const Array<Array<Complex>> & input,
                                    const size_t chunk_size = 0,
                                    const bool prefer_simd [[maybe_unused]] = true) const
      {
        for (size_t i = 0; i < input.size(); ++i)
          ah_invalid_argument_if(input[i].size() != n_)
            << "FFT::Plan::pinverse_transform_real_batch: batch item " << i
            << " has size " << input[i].size()
            << " but plan size is " << n_;

        if (input.is_empty())
          return {};

        Array<Array<Real>> output(input.size(), Array<Real>());
        if (input.size() == 1 or pool.num_threads() <= 1)
          {
            for (size_t i = 0; i < input.size(); ++i)
              output(i) = pinverse_transform_real(pool, input[i], chunk_size);
            return output;
          }

        auto inverse_one = [this, &input, &output](const size_t i)
          {
            output(i) = inverse_transform_real(input[i]);
          };
        parallel_for_index(pool, 0, input.size(), inverse_one, chunk_size);
        return output;
      }

      /** @brief Computes the Real-to-Complex FFT (RFFT).
       *
       *  Returns the compact spectrum (size N/2 + 1) for a real input.
       *
       *  @param input Real input array (size must equal plan size).
       *  @return Compact complex spectrum.
       */
      [[nodiscard]] Array<Complex>
      rfft(const Array<Real> & input) const
      {
        ah_invalid_argument_if(input.size() != n_)
          << "FFT::Plan::rfft: input size " << input.size()
          << " does not match plan size " << n_;
        return FFT::compact_real_spectrum(FFT::project_to_plan_spectrum(*this, input));
      }

      /** @brief Parallel Real-to-Complex FFT (RFFT). */
      [[nodiscard]] Array<Complex>
      prfft(ThreadPool & pool, const Array<Real> & input,
            const size_t chunk_size = 0) const
      {
        ah_invalid_argument_if(input.size() != n_)
          << "FFT::Plan::prfft: input size " << input.size()
          << " does not match plan size " << n_;
        return FFT::compact_real_spectrum(FFT::project_to_plan_spectrum(*this,
                                                                        input,
                                                                        &pool,
                                                                        chunk_size));
      }

      /** @brief Computes the Complex-to-Real Inverse FFT (IRFFT).
       *
       *  @param spectrum Compact complex spectrum (size N/2 + 1).
       *  @return Real signal of size N.
       */
      [[nodiscard]] Array<Real>
      irfft(const Array<Complex> & spectrum) const
      {
        return inverse_transform_real(FFT::expand_real_spectrum(spectrum,
                                                                n_,
                                                                "FFT::Plan::irfft"));
      }

      /** @brief Parallel Complex-to-Real Inverse FFT (IRFFT). */
      [[nodiscard]] Array<Real>
      pirfft(ThreadPool & pool, const Array<Complex> & spectrum,
             const size_t chunk_size = 0) const
      {
        return pinverse_transform_real(pool,
                                       FFT::expand_real_spectrum(spectrum,
                                                                 n_,
                                                                 "FFT::Plan::pirfft"),
                                       chunk_size);
      }

      /** @brief Computes a batch of Real-to-Complex FFTs. */
      [[nodiscard]] Array<Array<Complex>>
      rfft_batch(const Array<Array<Real>> & input) const
      {
        return FFT::compact_real_batch_spectra(
                                               FFT::project_to_plan_batch_spectra(*this, input));
      }

      /** @brief Computes a parallel batch of Real-to-Complex FFTs. */
      [[nodiscard]] Array<Array<Complex>>
      prfft_batch(ThreadPool & pool,
                  const Array<Array<Real>> & input,
                  const size_t chunk_size = 0) const
      {
        return FFT::compact_real_batch_spectra(
                                               FFT::project_to_plan_batch_spectra(*this, input, &pool, chunk_size));
      }

      /** @brief Computes a batch of Complex-to-Real Inverse FFTs. */
      [[nodiscard]] Array<Array<Real>>
      irfft_batch(const Array<Array<Complex>> & spectra) const
      {
        return inverse_transform_real_batch(FFT::expand_real_batch_spectra(
                                                                           spectra, n_, "FFT::Plan::irfft_batch"));
      }

      /** @brief Computes a parallel batch of Complex-to-Real Inverse FFTs. */
      [[nodiscard]] Array<Array<Real>>
      pirfft_batch(ThreadPool & pool,
                   const Array<Array<Complex>> & spectra,
                   const size_t chunk_size = 0) const
      {
        return pinverse_transform_real_batch(pool,
                                             FFT::expand_real_batch_spectra(
                                                                            spectra,
                                                                            n_,
                                                                            "FFT::Plan::pirfft_batch"),
                                             chunk_size);
      }
    };

  private:
    [[nodiscard]] static Array<Complex>
    project_to_plan_spectrum(const Plan & plan,
                             const Array<Real> & input,
                             ThreadPool *pool = nullptr,
                             const size_t chunk_size = 0)
    {
      Array<Complex> lifted = lift_real_input(input);
      if (pool != nullptr)
        plan.ptransform(*pool, lifted, false, chunk_size);
      else
        plan.transform(lifted, false);
      return lifted;
    }

    [[nodiscard]] static Array<Array<Complex>>
    project_to_plan_batch_spectra(const Plan & plan,
                                  const Array<Array<Real>> & input,
                                  ThreadPool *pool = nullptr,
                                  const size_t chunk_size = 0)
    {
      Array<Array<Complex>> lifted;
      lifted.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        lifted.append(lift_real_input(input[i]));

      if (pool != nullptr)
        plan.ptransform_batch(*pool, lifted, false, chunk_size);
      else
        plan.transform_batch(lifted, false);
      return lifted;
    }

    [[nodiscard]] static Array<Complex>
    compact_real_spectrum(const Array<Complex> & full_spectrum)
    {
      ah_invalid_argument_if(full_spectrum.is_empty())
        << "FFT::compact_real_spectrum: spectrum must be non-empty";

      Array<Complex> compact;
      compact.reserve(full_spectrum.size() / 2 + 1);
      for (size_t i = 0; i <= full_spectrum.size() / 2; ++i)
        compact.append(full_spectrum[i]);
      return compact;
    }

    [[nodiscard]] static Array<Array<Complex>>
    compact_real_batch_spectra(const Array<Array<Complex>> & full_spectra)
    {
      Array<Array<Complex>> compact;
      compact.reserve(full_spectra.size());
      for (size_t i = 0; i < full_spectra.size(); ++i)
        compact.append(compact_real_spectrum(full_spectra[i]));
      return compact;
    }

    [[nodiscard]] static size_t
    resolve_irfft_signal_size(const Array<Complex> & spectrum,
                              const size_t signal_size,
                              const char *ctx)
    {
      ah_invalid_argument_if(spectrum.is_empty())
        << ctx << ": compact spectrum must be non-empty";

      if (signal_size != 0)
        {
          ah_invalid_argument_if(spectrum.size() != signal_size / 2 + 1)
            << ctx << ": compact spectrum size " << spectrum.size()
            << " does not match signal size " << signal_size;
          return signal_size;
        }

      if (spectrum.size() == 1)
        return 1;

      ah_invalid_argument_if(spectrum.size() - 1
                             > std::numeric_limits<size_t>::max() / 2)
        << ctx << ": inferred signal size overflows size_t";
      return 2 * (spectrum.size() - 1);
    }

    [[nodiscard]] static Array<Complex>
    expand_real_spectrum(const Array<Complex> & spectrum,
                         const size_t signal_size,
                         const char *ctx)
    {
      const size_t n = resolve_irfft_signal_size(spectrum, signal_size, ctx);
      const size_t half = n / 2;

      Array<Complex> full = Array<Complex>::create(n);
      for (size_t i = 0; i < n; ++i)
        full(i) = Complex(Real(0), Real(0));

      for (size_t i = 0; i <= half; ++i)
        full(i) = spectrum[i];
      for (size_t i = half + 1; i < n; ++i)
        full(i) = std::conj(full[n - i]);

      return full;
    }

    [[nodiscard]] static Array<Array<Complex>>
    expand_real_batch_spectra(const Array<Array<Complex>> & spectra,
                              const size_t signal_size,
                              const char *ctx)
    {
      Array<Array<Complex>> full;
      full.reserve(spectra.size());
      for (size_t i = 0; i < spectra.size(); ++i)
        full.append(expand_real_spectrum(spectra[i], signal_size, ctx));
      return full;
    }

    [[nodiscard]] static Array<Array<Real>>
    project_real_batch_output(const Array<Array<Complex>> & input,
                              const char *ctx)
    {
      Array<Array<Real>> output;
      output.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output.append(project_real_output(input[i], ctx));
      return output;
    }

    static void
    transform_any_size_impl(Array<Complex> & a,
                            const bool invert,
                            ThreadPool *pool = nullptr,
                            const size_t chunk_size = 0)
    {
      const size_t n = a.size();
      ah_invalid_argument_if(n == 0) << "FFT::transform: input must be non-empty";

      if (n == 1)
        return;

      if (is_power_of_two(n))
        {
          transform_impl(a, invert, pool, chunk_size);
          return;
        }

      const Plan plan(n);
      if (pool != nullptr)
        plan.ptransform(*pool, a, invert, chunk_size);
      else
        plan.transform(a, invert);
    }

    [[nodiscard]] static Array<Real>
    inverse_transform_real_general_impl(const Array<Complex> & input,
                                        const char *ctx,
                                        ThreadPool *pool,
                                        const size_t chunk_size)
    {
      validate_real_spectrum(input, ctx);
      Array<Complex> output = input;
      transform_any_size_impl(output, true, pool, chunk_size);
      return project_real_output(output, ctx);
    }

  public:
    /** @brief Computes the Fast Fourier Transform (FFT) in-place.
     *
     *  Implements a mixed radix-2/radix-4 iterative Cooley-Tukey algorithm.
     *  Power-of-two sizes use the optimized radix-2/radix-4 kernel, while
     *  other sizes dispatch to a reusable arbitrary-length plan.
     *
     *  If 'invert' is true, it computes the Inverse Fast Fourier Transform (IFFT),
     *  including the 1/N scaling factor.
     *
     *  @param a The array of complex values to transform. Modified in-place.
     *  @param invert Whether to perform the inverse transform.
     *  @throw std::invalid_argument if the size of 'a' is zero.
     */
    static void
    transform(Array<Complex> & a, const bool invert)
    {
      transform_any_size_impl(a, invert);
    }

    /** @brief Parallel in-place FFT/IFFT using a thread pool.
     *
     *  Equivalent to @ref transform but distributes work across `pool`.
     *  The transform is applied in-place to the complex array `a`.
     *
     *  @param pool      Thread pool to use for parallel execution.
     *  @param a         Array of complex values to transform in-place.
     *  @param invert    If `true`, computes the inverse transform (IFFT).
     *  @param chunk_size Minimum number of elements per worker task
     *                   (0 = automatic).
     *  @throw std::invalid_argument if the size of `a` is zero.
     *  @throw std::bad_alloc if memory allocation fails.
     */
    static void
    ptransform(ThreadPool & pool, Array<Complex> & a, const bool invert,
               const size_t chunk_size = 0)
    {
      transform_any_size_impl(a, invert, &pool, chunk_size);
    }

    /** @brief Computes the FFT or IFFT and returns a new array.
     *
     *  Functional wrapper around the in-place @ref transform method.
     *  Overloads are also available for generic iterable containers of
     *  `std::complex` values.
     *
     *  @param input The input array of complex values.
     *  @param invert Whether to perform the inverse transform.
     *  @return A new array containing the transformed values.
     *  @throw std::invalid_argument if the input size is zero.
     */
    [[nodiscard]] static Array<Complex>
    transformed(const Array<Complex> & input, const bool invert = false)
    {
      Array<Complex> output = input;
      transform(output, invert);
      return output;
    }

    /** @brief Parallel FFT/IFFT that returns a new array.
     *
     *  Functional parallel wrapper around @ref ptransform.
     *
     *  @param pool      Thread pool to use for parallel execution.
     *  @param input     Input array of complex values.
     *  @param invert    If `true`, computes the inverse transform.
     *  @param chunk_size Minimum elements per worker task (0 = automatic).
     *  @return A new array containing the transformed values.
     *  @throw std::invalid_argument if the input size is zero.
     *  @throw std::bad_alloc if memory allocation fails.
     */
    [[nodiscard]] static Array<Complex>
    ptransformed(ThreadPool & pool, const Array<Complex> & input,
                 const bool invert = false, const size_t chunk_size = 0)
    {
      Array<Complex> output = input;
      ptransform(pool, output, invert, chunk_size);
      return output;
    }

    /** @brief In-place batch transform for equal-length complex inputs. */
    static void
    transform_batch(Array<Array<Complex>> & batch, const bool invert)
    {
      if (batch.is_empty())
        return;

      Plan plan(batch[0].size());
      plan.transform_batch(batch, invert);
    }

    /** @brief Parallel in-place batch transform for equal-length inputs. */
    static void
    ptransform_batch(ThreadPool & pool,
                     Array<Array<Complex>> & batch,
                     const bool invert,
                     const size_t chunk_size = 0)
    {
      if (batch.is_empty())
        return;

      Plan plan(batch[0].size());
      plan.ptransform_batch(pool, batch, invert, chunk_size);
    }

    /** @brief Functional batch FFT/IFFT wrapper. */
    [[nodiscard]] static Array<Array<Complex>>
    transformed_batch(const Array<Array<Complex>> & input,
                      const bool invert = false)
    {
      Array<Array<Complex>> output = input;
      transform_batch(output, invert);
      return output;
    }

    /** @brief Parallel functional batch FFT/IFFT wrapper. */
    [[nodiscard]] static Array<Array<Complex>>
    ptransformed_batch(ThreadPool & pool,
                       const Array<Array<Complex>> & input,
                       const bool invert = false,
                       const size_t chunk_size = 0)
    {
      Array<Array<Complex>> output = input;
      ptransform_batch(pool, output, invert, chunk_size);
      return output;
    }

    /** @brief Functional batch IFFT wrapper. */
    [[nodiscard]] static Array<Array<Complex>>
    inverse_transform_batch(const Array<Array<Complex>> & input)
    {
      return transformed_batch(input, true);
    }

    /** @brief Parallel functional batch IFFT wrapper. */
    [[nodiscard]] static Array<Array<Complex>>
    pinverse_transform_batch(ThreadPool & pool,
                             const Array<Array<Complex>> & input,
                             const size_t chunk_size = 0)
    {
      return ptransformed_batch(pool, input, true, chunk_size);
    }

    /** @brief Functional compact real FFT for equal-length real batches. */
    [[nodiscard]] static Array<Array<Complex>>
    rfft_batch(const Array<Array<Real>> & input)
    {
      if (input.is_empty())
        return {};

      Plan plan(input[0].size());
      return plan.rfft_batch(input);
    }

    /** @brief Parallel compact real FFT for equal-length real batches. */
    [[nodiscard]] static Array<Array<Complex>>
    prfft_batch(ThreadPool & pool,
                const Array<Array<Real>> & input,
                const size_t chunk_size = 0)
    {
      if (input.is_empty())
        return {};

      Plan plan(input[0].size());
      return plan.prfft_batch(pool, input, chunk_size);
    }

    /** @brief Functional compact real inverse FFT for equal-length batches. */
    [[nodiscard]] static Array<Array<Real>>
    irfft_batch(const Array<Array<Complex>> & spectra,
                const size_t signal_size)
    {
      if (spectra.is_empty())
        return {};

      Plan plan(signal_size);
      return plan.irfft_batch(spectra);
    }

    /** @brief Parallel compact real inverse FFT for equal-length batches. */
    [[nodiscard]] static Array<Array<Real>>
    pirfft_batch(ThreadPool & pool,
                 const Array<Array<Complex>> & spectra,
                 const size_t signal_size,
                 const size_t chunk_size = 0)
    {
      if (spectra.is_empty())
        return {};

      Plan plan(signal_size);
      return plan.pirfft_batch(pool, spectra, chunk_size);
    }

    /** @brief Builds a default row-major tensor layout for a flat buffer. */
    [[nodiscard]] static TensorLayout
    row_major_layout(const Array<size_t> & shape)
    {
      TensorLayout layout;
      layout.shape = shape;
      layout.strides = row_major_strides(shape, "FFT::row_major_layout");
      return layout;
    }

    /** @brief In-place 1-D FFT/IFFT along one tensor axis of a flat buffer. */
    static void
    transform_axis(Array<Complex> & data,
                   const TensorLayout & layout,
                   const size_t axis,
                   const bool invert)
    {
      transform_axis_impl(data, layout, axis, invert);
    }

    /** @brief Parallel in-place 1-D FFT/IFFT along one tensor axis. */
    static void
    ptransform_axis(ThreadPool & pool,
                    Array<Complex> & data,
                    const TensorLayout & layout,
                    const size_t axis,
                    const bool invert,
                    const size_t chunk_size = 0)
    {
      transform_axis_impl(data, layout, axis, invert, &pool, chunk_size);
    }

    /** @brief In-place FFT/IFFT along multiple tensor axes of a flat buffer. */
    static void
    transform_axes(Array<Complex> & data,
                   const TensorLayout & layout,
                   const Array<size_t> & axes,
                   const bool invert)
    {
      transform_axes_impl(data, layout, axes, invert);
    }

    /** @brief Parallel in-place FFT/IFFT along multiple tensor axes. */
    static void
    ptransform_axes(ThreadPool & pool,
                    Array<Complex> & data,
                    const TensorLayout & layout,
                    const Array<size_t> & axes,
                    const bool invert,
                    const size_t chunk_size = 0)
    {
      transform_axes_impl(data, layout, axes, invert, &pool, chunk_size);
    }

    /** @brief Functional tensor FFT/IFFT wrapper for flat buffers. */
    [[nodiscard]] static Array<Complex>
    transformed_axes(const Array<Complex> & input,
                     const TensorLayout & layout,
                     const Array<size_t> & axes,
                     const bool invert = false)
    {
      Array<Complex> output = input;
      transform_axes(output, layout, axes, invert);
      return output;
    }

    /** @brief Parallel functional tensor FFT/IFFT wrapper for flat buffers. */
    [[nodiscard]] static Array<Complex>
    ptransformed_axes(ThreadPool & pool,
                      const Array<Complex> & input,
                      const TensorLayout & layout,
                      const Array<size_t> & axes,
                      const bool invert = false,
                      const size_t chunk_size = 0)
    {
      Array<Complex> output = input;
      ptransform_axes(pool, output, layout, axes, invert, chunk_size);
      return output;
    }

    /** @brief Functional 2-D FFT/IFFT wrapper for rectangular complex matrices. */
    [[nodiscard]] static Array<Array<Complex>>
    transformed2d(const Array<Array<Complex>> & input,
                  const bool invert = false)
    {
      const Array<size_t> shape = matrix_shape(input, "FFT::transformed2d");
      Array<Complex> flat = flatten_matrix_row_major(input, "FFT::transformed2d");
      transform_axes(flat, row_major_layout(shape), Array<size_t>({0, 1}), invert);
      return reshape_matrix_row_major(flat,
                                      shape[0],
                                      shape[1],
                                      "FFT::transformed2d");
    }

    /** @brief Parallel functional 2-D FFT/IFFT wrapper for matrices. */
    [[nodiscard]] static Array<Array<Complex>>
    ptransformed2d(ThreadPool & pool,
                   const Array<Array<Complex>> & input,
                   const bool invert = false,
                   const size_t chunk_size = 0)
    {
      const Array<size_t> shape = matrix_shape(input, "FFT::ptransformed2d");
      Array<Complex> flat = flatten_matrix_row_major(input, "FFT::ptransformed2d");
      ptransform_axes(pool,
                      flat,
                      row_major_layout(shape),
                      Array<size_t>({0, 1}),
                      invert,
                      chunk_size);
      return reshape_matrix_row_major(flat,
                                      shape[0],
                                      shape[1],
                                      "FFT::ptransformed2d");
    }

    /** @brief Functional inverse 2-D FFT wrapper. */
    [[nodiscard]] static Array<Array<Complex>>
    inverse_transform2d(const Array<Array<Complex>> & input)
    {
      return transformed2d(input, true);
    }

    /** @brief Parallel functional inverse 2-D FFT wrapper. */
    [[nodiscard]] static Array<Array<Complex>>
    pinverse_transform2d(ThreadPool & pool,
                         const Array<Array<Complex>> & input,
                         const size_t chunk_size = 0)
    {
      return ptransformed2d(pool, input, true, chunk_size);
    }

    /** @brief Functional 2-D batched FFT/IFFT wrapper over a matrix stack. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    transformed2d_batch(const Array<Array<Array<Complex>>> & input,
                        const bool invert = false)
    {
      const Array<size_t> shape = tensor3_shape(input, "FFT::transformed2d_batch");
      Array<Complex> flat = flatten_tensor3_row_major(input,
                                                      "FFT::transformed2d_batch");
      transform_axes(flat, row_major_layout(shape), Array<size_t>({1, 2}), invert);
      return reshape_tensor3_row_major(flat,
                                       shape[0],
                                       shape[1],
                                       shape[2],
                                       "FFT::transformed2d_batch");
    }

    /** @brief Parallel functional 2-D batched FFT/IFFT wrapper. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    ptransformed2d_batch(ThreadPool & pool,
                         const Array<Array<Array<Complex>>> & input,
                         const bool invert = false,
                         const size_t chunk_size = 0)
    {
      const Array<size_t> shape = tensor3_shape(input, "FFT::ptransformed2d_batch");
      Array<Complex> flat = flatten_tensor3_row_major(input,
                                                      "FFT::ptransformed2d_batch");
      ptransform_axes(pool,
                      flat,
                      row_major_layout(shape),
                      Array<size_t>({1, 2}),
                      invert,
                      chunk_size);
      return reshape_tensor3_row_major(flat,
                                       shape[0],
                                       shape[1],
                                       shape[2],
                                       "FFT::ptransformed2d_batch");
    }

    /** @brief Functional 3-D FFT/IFFT wrapper for rectangular complex tensors. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    transformed3d(const Array<Array<Array<Complex>>> & input,
                  const bool invert = false)
    {
      const Array<size_t> shape = tensor3_shape(input, "FFT::transformed3d");
      Array<Complex> flat = flatten_tensor3_row_major(input, "FFT::transformed3d");
      transform_axes(flat,
                     row_major_layout(shape),
                     Array<size_t>({0, 1, 2}),
                     invert);
      return reshape_tensor3_row_major(flat,
                                       shape[0],
                                       shape[1],
                                       shape[2],
                                       "FFT::transformed3d");
    }

    /** @brief Parallel functional 3-D FFT/IFFT wrapper for tensors. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    ptransformed3d(ThreadPool & pool,
                   const Array<Array<Array<Complex>>> & input,
                   const bool invert = false,
                   const size_t chunk_size = 0)
    {
      const Array<size_t> shape = tensor3_shape(input, "FFT::ptransformed3d");
      Array<Complex> flat = flatten_tensor3_row_major(input, "FFT::ptransformed3d");
      ptransform_axes(pool,
                      flat,
                      row_major_layout(shape),
                      Array<size_t>({0, 1, 2}),
                      invert,
                      chunk_size);
      return reshape_tensor3_row_major(flat,
                                       shape[0],
                                       shape[1],
                                       shape[2],
                                       "FFT::ptransformed3d");
    }

    /** @brief Functional inverse 3-D FFT wrapper. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    inverse_transform3d(const Array<Array<Array<Complex>>> & input)
    {
      return transformed3d(input, true);
    }

    /** @brief Parallel functional inverse 3-D FFT wrapper. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    pinverse_transform3d(ThreadPool & pool,
                         const Array<Array<Array<Complex>>> & input,
                         const size_t chunk_size = 0)
    {
      return ptransformed3d(pool, input, true, chunk_size);
    }

    /** @brief Converts multichannel spectrograms between channel-major and
     *         frame-major layouts.
     */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    transpose_spectrogram_layout(const Array<Array<Array<Complex>>> & input,
                                 const SpectrogramLayout source,
                                 const SpectrogramLayout target)
    {
      return transpose_spectrogram_layout_impl(input,
                                               source,
                                               target,
                                               "FFT::transpose_spectrogram_layout");
    }

    /** @brief Multichannel STFT with explicit channel-major or frame-major
     *         output layout.
     */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    multichannel_stft(const Array<Array<Real>> & signals,
                      const Array<Real> & window,
                      const STFTOptions & options = {},
                      const SpectrogramLayout layout =
                          SpectrogramLayout::channel_frame_bin)
    {
      const Array<Array<Array<Complex>>> channel_major =
          batched_stft(signals, window, options);
      return layout == SpectrogramLayout::channel_frame_bin ?
               channel_major :
               transpose_spectrogram_layout(channel_major,
                                            SpectrogramLayout::channel_frame_bin,
                                            layout);
    }

    /** @brief Parallel multichannel STFT with selectable output layout. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    pmultichannel_stft(ThreadPool & pool,
                       const Array<Array<Real>> & signals,
                       const Array<Real> & window,
                       const STFTOptions & options = {},
                       const SpectrogramLayout layout =
                           SpectrogramLayout::channel_frame_bin,
                       const size_t chunk_size = 0)
    {
      const Array<Array<Array<Complex>>> channel_major =
          pbatched_stft(pool, signals, window, options, chunk_size);
      return layout == SpectrogramLayout::channel_frame_bin ?
               channel_major :
               transpose_spectrogram_layout(channel_major,
                                            SpectrogramLayout::channel_frame_bin,
                                            layout);
    }

    /** @brief Multichannel ISTFT from either channel-major or frame-major
     *         spectrogram layouts.
     */
    [[nodiscard]] static Array<Array<Real>>
    multichannel_istft(const Array<Array<Array<Complex>>> & spectrograms,
                       const Array<Real> & analysis_window,
                       const Array<Real> & synthesis_window,
                       const ISTFTOptions & options = {},
                       const Array<size_t> & signal_lengths = {},
                       const SpectrogramLayout layout =
                           SpectrogramLayout::channel_frame_bin)
    {
      const Array<Array<Array<Complex>>> channel_major =
          layout == SpectrogramLayout::channel_frame_bin ?
            spectrograms :
            transpose_spectrogram_layout(spectrograms,
                                         layout,
                                         SpectrogramLayout::channel_frame_bin);
      return batched_istft(channel_major,
                           analysis_window,
                           synthesis_window,
                           options,
                           signal_lengths);
    }

    /** @brief Parallel multichannel ISTFT from either supported layout. */
    [[nodiscard]] static Array<Array<Real>>
    pmultichannel_istft(ThreadPool & pool,
                        const Array<Array<Array<Complex>>> & spectrograms,
                        const Array<Real> & analysis_window,
                        const Array<Real> & synthesis_window,
                        const ISTFTOptions & options = {},
                        const Array<size_t> & signal_lengths = {},
                        const SpectrogramLayout layout =
                            SpectrogramLayout::channel_frame_bin,
                        const size_t chunk_size = 0)
    {
      const Array<Array<Array<Complex>>> channel_major =
          layout == SpectrogramLayout::channel_frame_bin ?
            spectrograms :
            transpose_spectrogram_layout(spectrograms,
                                         layout,
                                         SpectrogramLayout::channel_frame_bin);
      return pbatched_istft(pool,
                            channel_major,
                            analysis_window,
                            synthesis_window,
                            options,
                            signal_lengths,
                            chunk_size);
    }

    [[nodiscard]] static Array<Complex>
    transform_padded(const Array<Complex> & input)
    {
      Array<Complex> output = zero_padded_copy(input, next_power_of_two(input.size()));
      transform(output, false);
      return output;
    }

    [[nodiscard]] static Array<Complex>
    ptransform_padded(ThreadPool & pool, const Array<Complex> & input,
                      const size_t chunk_size = 0)
    {
      Array<Complex> output = zero_padded_copy(input, next_power_of_two(input.size()));
      ptransform(pool, output, false, chunk_size);
      return output;
    }

    /** @brief Zero-pad and forward-FFT a generic complex iterable.
     *
     *  Accepts any container of `std::complex<Real>` values (e.g.
     *  `std::vector<Complex>`, `DynList<Complex>`).  The input is copied
     *  to an internal `Array<Complex>` via @ref build_complex_input before
     *  the transform is applied.
     *
     *  @tparam Container Any iterable whose elements are `std::complex<Real>`.
     *  @param input The input container.
     *  @return A new `Array<Complex>` zero-padded to a power-of-two length
     *          and transformed.
     *  @throw std::invalid_argument if the input is empty.
     *  @throw std::bad_alloc if memory allocation fails.
     */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform_padded(const Container & input)
    {
      return transform_padded(build_complex_input(input));
    }

    /** @brief Parallel zero-pad and forward-FFT for a generic complex iterable.
     *
     *  @tparam Container Any iterable whose elements are `std::complex<Real>`.
     *  @param pool      Thread pool for parallel execution.
     *  @param input     The input container.
     *  @param chunk_size Minimum elements per worker task (0 = automatic).
     *  @return A new `Array<Complex>` zero-padded and transformed.
     *  @throw std::invalid_argument if the input is empty.
     *  @throw std::bad_alloc if memory allocation fails.
     */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform_padded(ThreadPool & pool, const Container & input,
                      const size_t chunk_size = 0)
    {
      return ptransform_padded(pool, build_complex_input(input), chunk_size);
    }

    /** @brief FFT/IFFT for a generic complex iterable.
     *
     *  Accepts any container of `std::complex<Real>` values.  The input is
     *  converted to `Array<Complex>` via @ref build_complex_input before
     *  the transform is applied.
     *
     *  @tparam Container Any iterable whose elements are `std::complex<Real>`.
     *  @param input  The input container.
     *  @param invert If `true`, computes the inverse transform (IFFT).
     *  @return A new `Array<Complex>` containing the transformed values.
     *  @throw std::invalid_argument if the input is empty.
     *  @throw std::bad_alloc if memory allocation fails.
     */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    transformed(const Container & input, const bool invert = false)
    {
      return transformed(build_complex_input(input), invert);
    }

    /** @brief Parallel FFT/IFFT for a generic complex iterable.
     *
     *  @tparam Container Any iterable whose elements are `std::complex<Real>`.
     *  @param pool      Thread pool for parallel execution.
     *  @param input     The input container.
     *  @param invert    If `true`, computes the inverse transform.
     *  @param chunk_size Minimum elements per worker task (0 = automatic).
     *  @return A new `Array<Complex>` containing the transformed values.
     *  @throw std::invalid_argument if the input is empty.
     *  @throw std::bad_alloc if memory allocation fails.
     */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransformed(ThreadPool & pool, const Container & input,
                 const bool invert = false, const size_t chunk_size = 0)
    {
      return ptransformed(pool, build_complex_input(input), invert, chunk_size);
    }

    /** @brief Computes the FFT of a real-valued input sequence.
     *
     *  The input sequence is first lifted to the complex domain (imaginary
     *  parts set to zero) before the transform is applied. Generic iterable
     *  containers of real values are accepted through a separate overload.
     *
     *  @param input The input array of real values.
     *  @return A new array of complex values containing the frequency spectrum.
     *  @throw std::invalid_argument if the input size is zero.
     */
    [[nodiscard]] static Array<Complex>
    transform(const Array<Real> & input)
    {
      return transform_real_optimized(input);
    }

    /** @brief Parallel real-input FFT.
     *
     *  Parallel equivalent of @ref transform(const Array<Real>&).
     *
     *  @param pool      Thread pool for parallel execution.
     *  @param input     Input array of real values.
     *  @param chunk_size Minimum elements per worker task (0 = automatic).
     *  @return A new `Array<Complex>` containing the frequency spectrum.
     *  @throw std::invalid_argument if the input is empty.
     *  @throw std::bad_alloc if memory allocation fails.
     */
    [[nodiscard]] static Array<Complex>
    ptransform(ThreadPool & pool, const Array<Real> & input,
               const size_t chunk_size = 0)
    {
      return transform_real_optimized(input, &pool, chunk_size);
    }

    /** @brief Computes the compact real FFT of size floor(N/2) + 1.
     *
     *  The returned layout stores bins `0..floor(N/2)` only. For odd-length
     *  signals, callers must pass the original signal length back to
     *  @ref irfft because the compact layout alone is ambiguous.
     */
    [[nodiscard]] static Array<Complex>
    rfft(const Array<Real> & input)
    {
      return compact_real_spectrum(transform(input));
    }

    /** @brief Parallel version of @ref rfft(const Array<Real>&). */
    [[nodiscard]] static Array<Complex>
    prfft(ThreadPool & pool, const Array<Real> & input,
          const size_t chunk_size = 0)
    {
      return compact_real_spectrum(ptransform(pool, input, chunk_size));
    }

    /** @brief Computes the FFT after zero-padding the input to the next power of two. */
    [[nodiscard]] static Array<Complex>
    transform_padded(const Array<Real> & input)
    {
      return transform(zero_padded_copy(input, next_power_of_two(input.size())));
    }

    /** @brief Parallel version of @ref transform_padded(const Array<Real>&). */
    [[nodiscard]] static Array<Complex>
    ptransform_padded(ThreadPool & pool, const Array<Real> & input,
                      const size_t chunk_size = 0)
    {
      return ptransform(pool, zero_padded_copy(input, next_power_of_two(input.size())),
                        chunk_size);
    }

    /** @brief Forward FFT for complex-valued containers. */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform(const Container & input)
    {
      return transformed(input, false);
    }

    /** @brief Parallel version of @ref transform(const Container&). */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform(ThreadPool & pool, const Container & input,
               const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, false, chunk_size);
    }

    /** @brief Forward FFT with padding for real-valued containers. */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform_padded(const Container & input)
    {
      return transform_padded(build_real_input(input));
    }

    /** @brief Parallel version of @ref transform_padded(const Container&). */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform_padded(ThreadPool & pool, const Container & input,
                      const size_t chunk_size = 0)
    {
      return ptransform_padded(pool, build_real_input(input), chunk_size);
    }

    /** @brief Forward FFT for real-valued containers. */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform(const Container & input)
    {
      return transform(build_real_input(input));
    }

    /** @brief Parallel version of @ref transform(const Container&). */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform(ThreadPool & pool, const Container & input,
               const size_t chunk_size = 0)
    {
      return ptransform(pool, build_real_input(input), chunk_size);
    }

    /** @brief Compact real FFT for real-valued containers. */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    rfft(const Container & input)
    {
      return rfft(build_real_input(input));
    }

    /** @brief Parallel version of @ref rfft(const Container&). */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    prfft(ThreadPool & pool, const Container & input,
          const size_t chunk_size = 0)
    {
      return prfft(pool, build_real_input(input), chunk_size);
    }

    /** @brief DSP alias for the forward FFT. */
    [[nodiscard]] static Array<Complex>
    spectrum(const Array<Complex> & input)
    {
      return transformed(input, false);
    }

    /** @brief Parallel version of @ref spectrum(const Array<Complex>&). */
    [[nodiscard]] static Array<Complex>
    pspectrum(ThreadPool & pool, const Array<Complex> & input,
              const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, false, chunk_size);
    }

    /** @brief DSP alias for the real forward FFT. */
    [[nodiscard]] static Array<Complex>
    spectrum(const Array<Real> & input)
    {
      return transform(input);
    }

    /** @brief Parallel version of @ref spectrum(const Array<Real>&). */
    [[nodiscard]] static Array<Complex>
    pspectrum(ThreadPool & pool, const Array<Real> & input,
              const size_t chunk_size = 0)
    {
      return ptransform(pool, input, chunk_size);
    }

    /** @brief DSP alias for complex-valued container forward FFT. */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    spectrum(const Container & input)
    {
      return transformed(input, false);
    }

    /** @brief Parallel version of @ref spectrum(const Container&). */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    pspectrum(ThreadPool & pool, const Container & input,
              const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, false, chunk_size);
    }

    /** @brief DSP alias for real-valued container forward FFT. */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    spectrum(const Container & input)
    {
      return transform(input);
    }

    /** @brief Parallel version of @ref spectrum(const Container&). */
    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    pspectrum(ThreadPool & pool, const Container & input,
              const size_t chunk_size = 0)
    {
      return ptransform(pool, input, chunk_size);
    }

    /** @brief Computes the Inverse Fast Fourier Transform (IFFT).
     *
     *  Functional wrapper for the inverse transform. Generic iterable
     *  containers of `std::complex` values are accepted through a separate
     *  overload.
     *
     *  @param input The frequency-domain complex values.
     *  @return A new array of complex values in the time/spatial domain.
     *  @throw std::invalid_argument if the input size is zero.
     */
    [[nodiscard]] static Array<Complex>
    inverse_transform(const Array<Complex> & input)
    {
      return transformed(input, true);
    }

    /** @brief Parallel version of @ref inverse_transform(const Array<Complex>&). */
    [[nodiscard]] static Array<Complex>
    pinverse_transform(ThreadPool & pool, const Array<Complex> & input,
                       const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, true, chunk_size);
    }

    /** @brief Forward IFFT for complex-valued containers. */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    inverse_transform(const Container & input)
    {
      return transformed(input, true);
    }

    /** @brief Parallel version of @ref inverse_transform(const Container&). */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    pinverse_transform(ThreadPool & pool, const Container & input,
                       const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, true, chunk_size);
    }

    /** @brief Computes the IFFT and projects the result back to real values.
     *
     *  Useful when the result is expected to be real (e.g., after processing
     *  the spectrum of a real signal). Generic iterable containers of
     *  `std::complex` values are accepted through a separate overload.
     *
     *  @param input The frequency-domain complex values.
     *  @return A new array of real values.
     *  @throw std::invalid_argument if the input size is zero.
     *  @throw std::domain_error if the result has non-negligible imaginary parts.
     */
    [[nodiscard]] static Array<Real>
    inverse_transform_real(const Array<Complex> & input)
    {
      return is_power_of_two(input.size()) ?
               inverse_transform_real_optimized_impl(input,
                                                     "FFT::inverse_transform_real", nullptr, 0,
                                                     [](Array<Complex> & packed, ThreadPool *inner_pool,
                                                        const size_t inner_chunk_size)
                                                       {
                                                         transform_impl(packed, true, inner_pool, inner_chunk_size);
                                                       }) :
               inverse_transform_real_general_impl(input,
                                                   "FFT::inverse_transform_real",
                                                   nullptr,
                                                   0);
    }

    /** @brief Parallel version of @ref inverse_transform_real(const Array<Complex>&). */
    [[nodiscard]] static Array<Real>
    pinverse_transform_real(ThreadPool & pool, const Array<Complex> & input,
                            const size_t chunk_size = 0)
    {
      return is_power_of_two(input.size()) ?
               inverse_transform_real_optimized_impl(input,
                                                     "FFT::inverse_transform_real", &pool, chunk_size,
                                                     [](Array<Complex> & packed, ThreadPool *inner_pool,
                                                        const size_t inner_chunk_size)
                                                       {
                                                         transform_impl(packed, true, inner_pool, inner_chunk_size);
                                                       }) :
               inverse_transform_real_general_impl(input,
                                                   "FFT::inverse_transform_real",
                                                   &pool,
                                                   chunk_size);
    }

    /** @brief Reconstructs a real signal from a compact `rfft()` spectrum.
     *
     *  If @p signal_size is zero, the method infers the original length as
     *  `2 * (spectrum.size() - 1)`. Odd original lengths therefore require
     *  an explicit @p signal_size.
     */
    [[nodiscard]] static Array<Real>
    irfft(const Array<Complex> & spectrum, const size_t signal_size = 0)
    {
      return inverse_transform_real(expand_real_spectrum(spectrum,
                                                         signal_size,
                                                         "FFT::irfft"));
    }

    /** @brief Parallel version of @ref irfft(const Array<Complex>&, size_t). */
    [[nodiscard]] static Array<Real>
    pirfft(ThreadPool & pool,
           const Array<Complex> & spectrum,
           const size_t signal_size = 0,
           const size_t chunk_size = 0)
    {
      return pinverse_transform_real(pool,
                                     expand_real_spectrum(spectrum,
                                                          signal_size,
                                                          "FFT::pirfft"),
                                     chunk_size);
    }

    /** @brief Inverse real transform for complex-valued containers. */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    inverse_transform_real(const Container & input)
    {
      return inverse_transform_real(build_complex_input(input));
    }

    /** @brief Parallel version of @ref inverse_transform_real(const Container&). */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    pinverse_transform_real(ThreadPool & pool, const Container & input,
                            const size_t chunk_size = 0)
    {
      return pinverse_transform_real(pool, build_complex_input(input), chunk_size);
    }

    /** @brief Inverse real transform (compact) for complex-valued containers. */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    irfft(const Container & spectrum, const size_t signal_size = 0)
    {
      return irfft(build_complex_input(spectrum), signal_size);
    }

    /** @brief Parallel version of @ref irfft(const Container&, size_t). */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    pirfft(ThreadPool & pool,
           const Container & spectrum,
           const size_t signal_size = 0,
           const size_t chunk_size = 0)
    {
      return pirfft(pool,
                    build_complex_input(spectrum),
                    signal_size,
                    chunk_size);
    }

    /** @brief Returns |X[k]| for each frequency bin in an FFT output. */
    [[nodiscard]] static Array<Real>
    magnitude_spectrum(const Array<Complex> & input)
    {
      Array<Real> output;
      output.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output.append(std::abs(input[i]));
      return output;
    }

    /** @brief Magnitude spectrum for complex-valued containers. */
    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    magnitude_spectrum(const Container & input)
    {
      return magnitude_spectrum(build_complex_input(input));
    }

    /** @brief Returns |X[k]|^2 for each frequency bin in an FFT output. */
    [[nodiscard]] static Array<Real>
    power_spectrum(const Array<Complex> & input)
    {
      Array<Real> output;
      output.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output.append(std::norm(input[i]));
      return output;
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    power_spectrum(const Container & input)
    {
      return power_spectrum(build_complex_input(input));
    }

    /** @brief Returns arg(X[k]) for each frequency bin in an FFT output. */
    [[nodiscard]] static Array<Real>
    phase_spectrum(const Array<Complex> & input)
    {
      Array<Real> output;
      output.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output.append(std::arg(input[i]));
      return output;
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    phase_spectrum(const Container & input)
    {
      return phase_spectrum(build_complex_input(input));
    }

    /** @brief Returns a Hann window of length @p n. */
    [[nodiscard]] static Array<Real>
    hann_window(const size_t n)
    {
      return cosine_sum_window(n, Real(0.5), Real(0.5));
    }

    /** @brief Returns a Hamming window of length @p n. */
    [[nodiscard]] static Array<Real>
    hamming_window(const size_t n)
    {
      return cosine_sum_window(n, Real(0.54), Real(0.46));
    }

    /** @brief Returns a Blackman window of length @p n. */
    [[nodiscard]] static Array<Real>
    blackman_window(const size_t n)
    {
      return cosine_sum_window(n, Real(0.42), Real(0.5), Real(0.08));
    }

    /** @brief Returns the Kaiser beta that corresponds to an attenuation goal. */
    [[nodiscard]] static Real
    kaiser_beta(const Real attenuation_db)
    {
      ah_invalid_argument_if(attenuation_db < Real(0))
        << "FFT::kaiser_beta: attenuation " << attenuation_db
        << " must be non-negative";

      if (attenuation_db > Real(50))
        return Real(0.1102) * (attenuation_db - Real(8.7));
      if (attenuation_db >= Real(21))
        return Real(0.5842) * std::pow(attenuation_db - Real(21), Real(0.4))
               + Real(0.07886) * (attenuation_db - Real(21));
      return Real(0);
    }

    /** @brief Returns a Kaiser window of length @p n. */
    [[nodiscard]] static Array<Real>
    kaiser_window(const size_t n,
                  const Real beta)
    {
      ah_invalid_argument_if(beta < Real(0))
        << "FFT::kaiser_window: beta " << beta << " must be non-negative";

      if (n == 0)
        return {};
      if (n == 1)
        return {Real(1)};

      const Real denom = modified_bessel_i0(beta);
      const Real half = static_cast<Real>(n - 1) / Real(2);
      Array<Real> window;
      window.reserve(n);
      for (size_t i = 0; i < n; ++i)
        {
          const Real x = (static_cast<Real>(i) - half) / half;
          const Real arg = beta * std::sqrt(std::max(Real(0), Real(1) - x * x));
          window.append(modified_bessel_i0(arg) / denom);
        }
      return window;
    }

    /** @brief Applies a real window sample-by-sample to a real signal. */
    [[nodiscard]] static Array<Real>
    apply_window(const Array<Real> & signal, const Array<Real> & window)
    {
      ah_invalid_argument_if(signal.size() != window.size())
        << "FFT::apply_window: signal size " << signal.size()
        << " does not match window size " << window.size();

      Array<Real> output = Array<Real>::create(signal.size());
      for (size_t i = 0; i < signal.size(); ++i)
        output(i) = signal[i] * window[i];
      return output;
    }

    /** @brief Applies a real window sample-by-sample to a complex signal. */
    [[nodiscard]] static Array<Complex>
    apply_window(const Array<Complex> & signal, const Array<Real> & window)
    {
      ah_invalid_argument_if(signal.size() != window.size())
        << "FFT::apply_window: signal size " << signal.size()
        << " does not match window size " << window.size();

      Array<Complex> output = Array<Complex>::create(signal.size());
      for (size_t i = 0; i < signal.size(); ++i)
        output(i) = signal[i] * window[i];
      return output;
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static Array<Real>
    apply_window(const SignalContainer & signal, const WindowContainer & window)
    {
      return apply_window(build_real_input(signal), build_real_input(window));
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Complex_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static Array<Complex>
    apply_window(const SignalContainer & signal, const WindowContainer & window)
    {
      return apply_window(build_complex_input(signal), build_real_input(window));
    }

    /** @brief Applies a Hann window of matching size to a real signal. */
    [[nodiscard]] static Array<Real>
    apply_hann_window(const Array<Real> & signal)
    {
      return apply_window(signal, hann_window(signal.size()));
    }

    /** @brief Applies a Hamming window of matching size to a real signal. */
    [[nodiscard]] static Array<Real>
    apply_hamming_window(const Array<Real> & signal)
    {
      return apply_window(signal, hamming_window(signal.size()));
    }

    /** @brief Applies a Blackman window of matching size to a real signal. */
    [[nodiscard]] static Array<Real>
    apply_blackman_window(const Array<Real> & signal)
    {
      return apply_window(signal, blackman_window(signal.size()));
    }

    /** @brief Applies a Hann window of matching size to a complex signal. */
    [[nodiscard]] static Array<Complex>
    apply_hann_window(const Array<Complex> & signal)
    {
      return apply_window(signal, hann_window(signal.size()));
    }

    /** @brief Applies a Hamming window of matching size to a complex signal. */
    [[nodiscard]] static Array<Complex>
    apply_hamming_window(const Array<Complex> & signal)
    {
      return apply_window(signal, hamming_window(signal.size()));
    }

    /** @brief Applies a Blackman window of matching size to a complex signal. */
    [[nodiscard]] static Array<Complex>
    apply_blackman_window(const Array<Complex> & signal)
    {
      return apply_window(signal, blackman_window(signal.size()));
    }

    /** @brief FIR low-pass design via the window method. */
    [[nodiscard]] static Array<Real>
    firwin_lowpass(const size_t num_taps,
                   const Real cutoff_frequency,
                   const Real sample_rate,
                   const Array<Real> & window)
    {
      return firwin_lowpass_impl(num_taps,
                                 cutoff_frequency,
                                 sample_rate,
                                 window,
                                 "FFT::firwin_lowpass");
    }

    /** @brief FIR low-pass design via the window method using a Kaiser window. */
    [[nodiscard]] static Array<Real>
    firwin_lowpass(const size_t num_taps,
                   const Real cutoff_frequency,
                   const Real sample_rate,
                   const Real attenuation_db)
    {
      return firwin_lowpass(num_taps,
                            cutoff_frequency,
                            sample_rate,
                            kaiser_window(num_taps, kaiser_beta(attenuation_db)));
    }

    /** @brief FIR high-pass design via spectral inversion of a low-pass design. */
    [[nodiscard]] static Array<Real>
    firwin_highpass(const size_t num_taps,
                    const Real cutoff_frequency,
                    const Real sample_rate,
                    const Array<Real> & window)
    {
      Array<Real> coeffs =
          firwin_lowpass_impl(num_taps,
                              cutoff_frequency,
                              sample_rate,
                              window,
                              "FFT::firwin_highpass");
      const size_t center = (num_taps - 1) / 2;
      for (size_t i = 0; i < coeffs.size(); ++i)
        coeffs(i) = -coeffs[i];
      coeffs(center) += Real(1);
      normalize_fir_at_omega(coeffs,
                             std::numbers::pi_v<Real>,
                             "FFT::firwin_highpass");
      return coeffs;
    }

    /** @brief FIR high-pass design using a Kaiser window. */
    [[nodiscard]] static Array<Real>
    firwin_highpass(const size_t num_taps,
                    const Real cutoff_frequency,
                    const Real sample_rate,
                    const Real attenuation_db)
    {
      return firwin_highpass(num_taps,
                             cutoff_frequency,
                             sample_rate,
                             kaiser_window(num_taps, kaiser_beta(attenuation_db)));
    }

    /** @brief FIR band-pass design via the window method. */
    [[nodiscard]] static Array<Real>
    firwin_bandpass(const size_t num_taps,
                    const Real low_cutoff_frequency,
                    const Real high_cutoff_frequency,
                    const Real sample_rate,
                    const Array<Real> & window)
    {
      ah_invalid_argument_if(low_cutoff_frequency <= Real(0))
        << "FFT::firwin_bandpass: low cutoff " << low_cutoff_frequency
        << " must be positive";
      ah_invalid_argument_if(high_cutoff_frequency <= low_cutoff_frequency)
        << "FFT::firwin_bandpass: high cutoff " << high_cutoff_frequency
        << " must be greater than low cutoff " << low_cutoff_frequency;
      ah_invalid_argument_if(high_cutoff_frequency >= sample_rate / Real(2))
        << "FFT::firwin_bandpass: high cutoff " << high_cutoff_frequency
        << " must be smaller than Nyquist " << (sample_rate / Real(2));

      Array<Real> low =
          firwin_lowpass_impl(num_taps,
                              high_cutoff_frequency,
                              sample_rate,
                              window,
                              "FFT::firwin_bandpass");
      Array<Real> high =
          firwin_lowpass_impl(num_taps,
                              low_cutoff_frequency,
                              sample_rate,
                              window,
                              "FFT::firwin_bandpass");
      Array<Real> coeffs = Array<Real>::create(num_taps);
      for (size_t i = 0; i < num_taps; ++i)
        coeffs(i) = low[i] - high[i];

      const Real center_frequency = (low_cutoff_frequency + high_cutoff_frequency)
                                    / (Real(2) * sample_rate);
      normalize_fir_at_omega(coeffs,
                             Real(2) * std::numbers::pi_v<Real> * center_frequency,
                             "FFT::firwin_bandpass");
      return coeffs;
    }

    /** @brief FIR band-pass design using a Kaiser window. */
    [[nodiscard]] static Array<Real>
    firwin_bandpass(const size_t num_taps,
                    const Real low_cutoff_frequency,
                    const Real high_cutoff_frequency,
                    const Real sample_rate,
                    const Real attenuation_db)
    {
      return firwin_bandpass(num_taps,
                             low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             kaiser_window(num_taps, kaiser_beta(attenuation_db)));
    }

    /** @brief FIR band-stop design via the window method. */
    [[nodiscard]] static Array<Real>
    firwin_bandstop(const size_t num_taps,
                    const Real low_cutoff_frequency,
                    const Real high_cutoff_frequency,
                    const Real sample_rate,
                    const Array<Real> & window)
    {
      Array<Real> coeffs =
          firwin_bandpass(num_taps,
                          low_cutoff_frequency,
                          high_cutoff_frequency,
                          sample_rate,
                          window);
      const size_t center = (num_taps - 1) / 2;
      for (size_t i = 0; i < coeffs.size(); ++i)
        coeffs(i) = -coeffs[i];
      coeffs(center) += Real(1);
      normalize_fir_at_omega(coeffs, Real(0), "FFT::firwin_bandstop");
      return coeffs;
    }

    /** @brief FIR band-stop design using a Kaiser window. */
    [[nodiscard]] static Array<Real>
    firwin_bandstop(const size_t num_taps,
                    const Real low_cutoff_frequency,
                    const Real high_cutoff_frequency,
                    const Real sample_rate,
                    const Real attenuation_db)
    {
      return firwin_bandstop(num_taps,
                             low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             kaiser_window(num_taps, kaiser_beta(attenuation_db)));
    }

    /** @brief FIR design by weighted least squares over piecewise-linear bands. */
    [[nodiscard]] static Array<Real>
    firls(const size_t num_taps,
          const Array<Real> & bands,
          const Array<Real> & desired,
          const Real sample_rate,
          const Array<Real> & weights = {})
    {
      return firls_impl(num_taps,
                        bands,
                        desired,
                        sample_rate,
                        weights,
                        "FFT::firls");
    }

    template <typename BandContainer, typename DesiredContainer>
      requires Is_Real_Container<BandContainer>
               and Is_Real_Container<DesiredContainer>
    [[nodiscard]] static Array<Real>
    firls(const size_t num_taps,
          const BandContainer & bands,
          const DesiredContainer & desired,
          const Real sample_rate)
    {
      return firls(num_taps,
                   build_real_input(bands),
                   build_real_input(desired),
                   sample_rate,
                   Array<Real>());
    }

    template <typename BandContainer, typename DesiredContainer,
              typename WeightContainer>
      requires Is_Real_Container<BandContainer>
               and Is_Real_Container<DesiredContainer>
               and Is_Real_Container<WeightContainer>
    [[nodiscard]] static Array<Real>
    firls(const size_t num_taps,
          const BandContainer & bands,
          const DesiredContainer & desired,
          const Real sample_rate,
          const WeightContainer & weights)
    {
      return firls(num_taps,
                   build_real_input(bands),
                   build_real_input(desired),
                   sample_rate,
                   build_real_input(weights));
    }

    /** @brief FIR equiripple design via a dense-grid Remez exchange. */
    [[nodiscard]] static Array<Real>
    remez(const size_t num_taps,
          const Array<Real> & bands,
          const Array<Real> & desired,
          const Real sample_rate,
          const Array<Real> & weights = {},
          const size_t grid_density = 32,
          const size_t max_iterations = 64)
    {
      return remez_impl(num_taps,
                        bands,
                        desired,
                        sample_rate,
                        weights,
                        grid_density,
                        max_iterations,
                        "FFT::remez");
    }

    template <typename BandContainer, typename DesiredContainer>
      requires Is_Real_Container<BandContainer>
               and Is_Real_Container<DesiredContainer>
    [[nodiscard]] static Array<Real>
    remez(const size_t num_taps,
          const BandContainer & bands,
          const DesiredContainer & desired,
          const Real sample_rate)
    {
      return remez(num_taps,
                   build_real_input(bands),
                   build_real_input(desired),
                   sample_rate,
                   Array<Real>());
    }

    template <typename BandContainer, typename DesiredContainer,
              typename WeightContainer>
      requires Is_Real_Container<BandContainer>
               and Is_Real_Container<DesiredContainer>
               and Is_Real_Container<WeightContainer>
    [[nodiscard]] static Array<Real>
    remez(const size_t num_taps,
          const BandContainer & bands,
          const DesiredContainer & desired,
          const Real sample_rate,
          const WeightContainer & weights,
          const size_t grid_density = 32,
          const size_t max_iterations = 64)
    {
      return remez(num_taps,
                   build_real_input(bands),
                   build_real_input(desired),
                   sample_rate,
                   build_real_input(weights),
                   grid_density,
                   max_iterations);
    }

    /** @brief Polyphase upsample-filter-downsample for real signals. */
    [[nodiscard]] static Array<Real>
    upfirdn(const Array<Real> & signal,
            const Array<Real> & coeffs,
            const size_t up = 1,
            const size_t down = 1)
    {
      ah_invalid_argument_if(up == 0)
        << "FFT::upfirdn: up factor must be positive";
      ah_invalid_argument_if(down == 0)
        << "FFT::upfirdn: down factor must be positive";
      ah_invalid_argument_if(coeffs.is_empty())
        << "FFT::upfirdn: FIR coefficients must be non-empty";
      if (signal.is_empty())
        return {};

      ah_invalid_argument_if(signal.size() > std::numeric_limits<size_t>::max() / up)
        << "FFT::upfirdn: upsampled signal length overflows size_t";
      const size_t convolved_length = signal.size() * up + coeffs.size() - 1;
      const size_t output_size = (convolved_length + down - 1) / down;

      Array<Real> filtered = Array<Real>::create(convolved_length);
      for (size_t i = 0; i < convolved_length; ++i)
        filtered(i) = Real(0);

      for (size_t n = 0; n < signal.size(); ++n)
        {
          const size_t base = n * up;
          for (size_t k = 0; k < coeffs.size(); ++k)
            filtered(base + k) += signal[n] * coeffs[k];
        }

      Array<Real> output = Array<Real>::create(output_size);
      for (size_t i = 0; i < output_size; ++i)
        output(i) = filtered[i * down];
      return output;
    }

    template <typename SignalContainer, typename CoeffContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<CoeffContainer>
    [[nodiscard]] static Array<Real>
    upfirdn(const SignalContainer & signal,
            const CoeffContainer & coeffs,
            const size_t up = 1,
            const size_t down = 1)
    {
      return upfirdn(build_real_input(signal),
                     build_real_input(coeffs),
                     up,
                     down);
    }

    /** @brief Polyphase resampling with explicit FIR coefficients. */
    [[nodiscard]] static Array<Real>
    resample_poly(const Array<Real> & signal,
                  const size_t up,
                  const size_t down,
                  const Array<Real> & coeffs)
    {
      ah_invalid_argument_if(up == 0)
        << "FFT::resample_poly: up factor must be positive";
      ah_invalid_argument_if(down == 0)
        << "FFT::resample_poly: down factor must be positive";
      ah_invalid_argument_if(coeffs.is_empty())
        << "FFT::resample_poly: FIR coefficients must be non-empty";
      if (signal.is_empty())
        return {};

      const size_t gcd = std::gcd(up, down);
      const size_t reduced_up = up / gcd;
      const size_t reduced_down = down / gcd;
      const Array<Real> raw = upfirdn(signal, coeffs, reduced_up, reduced_down);

      const size_t half_len = effective_coeff_length(coeffs) / 2;
      const size_t start = (half_len + reduced_down - 1) / reduced_down;
      const size_t expected_size =
          (signal.size() * reduced_up + reduced_down - 1) / reduced_down;

      ah_runtime_error_if(start > raw.size())
        << "FFT::resample_poly: internal trim offset exceeds raw output";

      const size_t available = raw.size() - start;
      const size_t trimmed_size = std::min(expected_size, available);
      Array<Real> output = slice_copy(raw, start, trimmed_size);
      if (output.size() == expected_size)
        return output;

      Array<Real> padded = Array<Real>::create(expected_size);
      for (size_t i = 0; i < expected_size; ++i)
        padded(i) = i < output.size() ? output[i] : Real(0);
      return padded;
    }

    /** @brief Polyphase resampling with an internally designed Kaiser FIR. */
    [[nodiscard]] static Array<Real>
    resample_poly(const Array<Real> & signal,
                  const size_t up,
                  const size_t down,
                  const ResamplePolyOptions & options = {})
    {
      ah_invalid_argument_if(options.taps_per_phase == 0)
        << "FFT::resample_poly: taps_per_phase must be positive";
      ah_invalid_argument_if(options.attenuation_db <= Real(0))
        << "FFT::resample_poly: attenuation " << options.attenuation_db
        << " must be strictly positive";
      if (signal.is_empty())
        return {};

      const size_t gcd = std::gcd(up, down);
      const size_t reduced_up = up / gcd;
      const size_t reduced_down = down / gcd;
      const size_t rate = std::max(reduced_up, reduced_down);
      const size_t num_taps = 2 * options.taps_per_phase * rate + 1;
      const Real effective_rate = static_cast<Real>(reduced_up);
      const Real cutoff = effective_rate / (Real(2) * static_cast<Real>(rate));

      Array<Real> coeffs =
          firwin_lowpass(num_taps,
                         cutoff,
                         effective_rate,
                         options.attenuation_db);
      for (size_t i = 0; i < coeffs.size(); ++i)
        coeffs(i) *= static_cast<Real>(reduced_up);

      return resample_poly(signal, reduced_up, reduced_down, coeffs);
    }

    template <typename SignalContainer, typename CoeffContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<CoeffContainer>
    [[nodiscard]] static Array<Real>
    resample_poly(const SignalContainer & signal,
                  const size_t up,
                  const size_t down,
                  const CoeffContainer & coeffs)
    {
      return resample_poly(build_real_input(signal),
                           up,
                           down,
                           build_real_input(coeffs));
    }

    template <typename SignalContainer>
      requires Is_Real_Container<SignalContainer>
    [[nodiscard]] static Array<Real>
    resample_poly(const SignalContainer & signal,
                  const size_t up,
                  const size_t down,
                  const ResamplePolyOptions & options = {})
    {
      return resample_poly(build_real_input(signal), up, down, options);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Real>
    apply_hann_window(const Container & signal)
    {
      return apply_hann_window(build_real_input(signal));
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Real>
    apply_hamming_window(const Container & signal)
    {
      return apply_hamming_window(build_real_input(signal));
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Real>
    apply_blackman_window(const Container & signal)
    {
      return apply_blackman_window(build_real_input(signal));
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    apply_hann_window(const Container & signal)
    {
      return apply_hann_window(build_complex_input(signal));
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    apply_hamming_window(const Container & signal)
    {
      return apply_hamming_window(build_complex_input(signal));
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    apply_blackman_window(const Container & signal)
    {
      return apply_blackman_window(build_complex_input(signal));
    }

    /** @brief Returns the FFT of a real signal after applying a window. */
    [[nodiscard]] static Array<Complex>
    windowed_spectrum(const Array<Real> & signal, const Array<Real> & window)
    {
      return spectrum(apply_window(signal, window));
    }

    /** @brief Returns the FFT of a complex signal after applying a window. */
    [[nodiscard]] static Array<Complex>
    windowed_spectrum(const Array<Complex> & signal, const Array<Real> & window)
    {
      return spectrum(apply_window(signal, window));
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static Array<Complex>
    windowed_spectrum(const SignalContainer & signal,
                      const WindowContainer & window)
    {
      return windowed_spectrum(build_real_input(signal), build_real_input(window));
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Complex_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static Array<Complex>
    windowed_spectrum(const SignalContainer & signal,
                      const WindowContainer & window)
    {
      return windowed_spectrum(build_complex_input(signal), build_real_input(window));
    }

    /** @brief Returns the sum of squared window samples. */
    [[nodiscard]] static Real
    window_energy(const Array<Real> & window)
    {
      ah_invalid_argument_if(window.is_empty()) << "FFT::window_energy: window must be non-empty";
      return sum_squares(window);
    }

    /** @brief Returns the average window gain. */
    [[nodiscard]] static Real
    window_coherent_gain(const Array<Real> & window)
    {
      ah_invalid_argument_if(window.is_empty())
        << "FFT::window_coherent_gain: window must be non-empty";
      return sum_values(window) / static_cast<Real>(window.size());
    }

    /** @brief Returns the equivalent noise bandwidth of a window in bins. */
    [[nodiscard]] static Real
    window_enbw(const Array<Real> & window)
    {
      ah_invalid_argument_if(window.is_empty())
        << "FFT::window_enbw: window must be non-empty";
      const Real coherent_sum = sum_values(window);
      ah_domain_error_if(std::abs(coherent_sum) <= std::numeric_limits<Real>::epsilon())
        << "FFT::window_enbw: coherent gain is zero";
      return static_cast<Real>(window.size()) * sum_squares(window)
             / (coherent_sum * coherent_sum);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Real
    window_energy(const Container & window)
    {
      return window_energy(build_real_input(window));
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Real
    window_coherent_gain(const Container & window)
    {
      return window_coherent_gain(build_real_input(window));
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Real
    window_enbw(const Container & window)
    {
      return window_enbw(build_real_input(window));
    }

    /** @brief Returns the frame start offsets used by @ref frame_signal. */
    [[nodiscard]] static Array<size_t>
    frame_offsets(const size_t signal_size,
                  const size_t frame_size,
                  const size_t hop_size,
                  const bool pad_end = true)
    {
      return frame_offsets_impl(signal_size,
                                frame_size,
                                hop_size,
                                pad_end,
                                "FFT::frame_offsets");
    }

    /** @brief Overlap-adds a frame sequence with a fixed hop size. */
    [[nodiscard]] static Array<Real>
    overlap_add_frames(const Array<Array<Real>> & frames,
                       const size_t hop_size,
                       const size_t signal_length = 0)
    {
      ah_invalid_argument_if(hop_size == 0)
        << "FFT::overlap_add_frames: hop size must be positive";
      if (frames.is_empty())
        return {};

      const size_t frame_size = frames[0].size();
      ah_invalid_argument_if(frame_size == 0)
        << "FFT::overlap_add_frames: frames must be non-empty";
      for (size_t i = 1; i < frames.size(); ++i)
        ah_invalid_argument_if(frames[i].size() != frame_size)
          << "FFT::overlap_add_frames: frame " << i
          << " has size " << frames[i].size()
          << " but expected " << frame_size;

      const size_t total_length = hop_size * (frames.size() - 1) + frame_size;
      ah_invalid_argument_if(signal_length > total_length)
        << "FFT::overlap_add_frames: requested signal length "
        << signal_length << " exceeds overlap-add extent " << total_length;

      Array<Real> output = Array<Real>::create(total_length);
      for (size_t i = 0; i < total_length; ++i)
        output(i) = Real(0);

      for (size_t frame = 0; frame < frames.size(); ++frame)
        {
          const size_t offset = frame * hop_size;
          for (size_t i = 0; i < frame_size; ++i)
            output(offset + i) += frames[frame][i];
        }

      return signal_length == 0 ?
               output :
               slice_copy(output, 0, signal_length);
    }

    /** @brief Welch one-sided PSD estimate using a custom window. */
    [[nodiscard]] static PowerSpectralDensity
    welch(const Array<Real> & signal,
          const Array<Real> & window,
          const Real sample_rate,
          const WelchOptions & options = {})
    {
      const char *ctx = "FFT::welch";
      const size_t fft_size = resolve_welch_fft_size(options, window.size(), ctx);
      const Array<Array<Real>> frames = prepare_welch_frames(signal, window, options, ctx);
      const Real energy = window_energy(window);
      ah_domain_error_if(energy <= Real(0))
        << ctx << ": window energy must be strictly positive";

      Plan plan(fft_size);
      Array<Real> density = Array<Real>::create(fft_size / 2 + 1);
      for (size_t k = 0; k < density.size(); ++k)
        density(k) = Real(0);

      for (size_t i = 0; i < frames.size(); ++i)
        {
          Array<Real> padded = zero_padded_copy(frames[i], fft_size);
          const Array<Complex> spectrum = plan.rfft(padded);
          for (size_t k = 0; k < spectrum.size(); ++k)
            {
              Real value = std::norm(spectrum[k]);
              apply_one_sided_density_scaling(value, k, fft_size);
              density(k) += value;
            }
        }

      const Real scale = Real(1)
                         / (sample_rate
                            * energy
                            * static_cast<Real>(frames.size()));
      for (size_t k = 0; k < density.size(); ++k)
        density(k) *= scale;

      return {one_sided_frequency_grid(fft_size, sample_rate, ctx), density};
    }

    /** @brief Welch one-sided PSD estimate using a Hann window. */
    [[nodiscard]] static PowerSpectralDensity
    welch(const Array<Real> & signal,
          const size_t frame_size,
          const Real sample_rate,
          const WelchOptions & options = {})
    {
      return welch(signal, hann_window(frame_size), sample_rate, options);
    }

    /** @brief One-sided cross-spectral density estimate using a custom window. */
    [[nodiscard]] static CrossSpectralDensity
    csd(const Array<Real> & x,
        const Array<Real> & y,
        const Array<Real> & window,
        const Real sample_rate,
        const WelchOptions & options = {})
    {
      const auto ctx = "FFT::csd";
      ah_invalid_argument_if(x.size() != y.size())
        << ctx << ": signal sizes " << x.size()
        << " and " << y.size() << " do not match";

      const size_t fft_size = resolve_welch_fft_size(options, window.size(), ctx);
      const Array<Array<Real>> x_frames = prepare_welch_frames(x, window, options, ctx);
      const Array<Array<Real>> y_frames = prepare_welch_frames(y, window, options, ctx);
      ah_runtime_error_if(x_frames.size() != y_frames.size())
        << ctx << ": frame count mismatch after preparation";

      const Real energy = window_energy(window);
      ah_domain_error_if(energy <= Real(0))
        << ctx << ": window energy must be strictly positive";

      Plan plan(fft_size);
      Array<Complex> density = Array<Complex>::create(fft_size / 2 + 1);
      for (size_t k = 0; k < density.size(); ++k)
        density(k) = Complex(Real(0), Real(0));

      for (size_t i = 0; i < x_frames.size(); ++i)
        {
          const Array<Complex> x_spectrum = plan.rfft(zero_padded_copy(x_frames[i], fft_size));
          const Array<Complex> y_spectrum = plan.rfft(zero_padded_copy(y_frames[i], fft_size));
          for (size_t k = 0; k < density.size(); ++k)
            {
              Complex value = x_spectrum[k] * std::conj(y_spectrum[k]);
              apply_one_sided_density_scaling(value, k, fft_size);
              density(k) += value;
            }
        }

      const Real scale = Real(1)
                         / (sample_rate
                            * energy
                            * static_cast<Real>(x_frames.size()));
      for (size_t k = 0; k < density.size(); ++k)
        density(k) *= scale;

      return {one_sided_frequency_grid(fft_size, sample_rate, ctx), density};
    }

    /** @brief One-sided cross-spectral density estimate using a Hann window. */
    [[nodiscard]] static CrossSpectralDensity
    csd(const Array<Real> & x,
        const Array<Real> & y,
        const size_t frame_size,
        const Real sample_rate,
        const WelchOptions & options = {})
    {
      return csd(x, y, hann_window(frame_size), sample_rate, options);
    }

    /** @brief Magnitude-squared coherence estimate using Welch averages. */
    [[nodiscard]] static CoherenceEstimate
    coherence(const Array<Real> & x,
              const Array<Real> & y,
              const Array<Real> & window,
              const Real sample_rate,
              const WelchOptions & options = {})
    {
      const CrossSpectralDensity pxy = csd(x, y, window, sample_rate, options);
      const PowerSpectralDensity pxx = welch(x, window, sample_rate, options);
      const PowerSpectralDensity pyy = welch(y, window, sample_rate, options);

      ah_runtime_error_if(pxy.density.size() != pxx.density.size()
                          or pxy.density.size() != pyy.density.size())
        << "FFT::coherence: inconsistent spectrum sizes";

      CoherenceEstimate output;
      output.frequency = pxy.frequency;
      output.magnitude_squared = Array<Real>::create(pxy.density.size());
      const Real tol = Real(1024) * std::numeric_limits<Real>::epsilon();
      for (size_t k = 0; k < pxy.density.size(); ++k)
        {
          const Real denom = pxx.density[k] * pyy.density[k];
          output.magnitude_squared(k) =
              denom <= tol ? Real(0) : std::norm(pxy.density[k]) / denom;
        }
      return output;
    }

    /** @brief Magnitude-squared coherence estimate using a Hann window. */
    [[nodiscard]] static CoherenceEstimate
    coherence(const Array<Real> & x,
              const Array<Real> & y,
              const size_t frame_size,
              const Real sample_rate,
              const WelchOptions & options = {})
    {
      return coherence(x, y, hann_window(frame_size), sample_rate, options);
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static PowerSpectralDensity
    welch(const SignalContainer & signal,
          const WindowContainer & window,
          const Real sample_rate,
          const WelchOptions & options = {})
    {
      return welch(build_real_input(signal),
                   build_real_input(window),
                   sample_rate,
                   options);
    }

    template <typename ContainerX, typename ContainerY, typename WindowContainer>
      requires Is_Real_Container<ContainerX>
               and Is_Real_Container<ContainerY>
               and Is_Real_Container<WindowContainer>
    [[nodiscard]] static CrossSpectralDensity
    csd(const ContainerX & x,
        const ContainerY & y,
        const WindowContainer & window,
        const Real sample_rate,
        const WelchOptions & options = {})
    {
      return csd(build_real_input(x),
                 build_real_input(y),
                 build_real_input(window),
                 sample_rate,
                 options);
    }

    template <typename ContainerX, typename ContainerY, typename WindowContainer>
      requires Is_Real_Container<ContainerX>
               and Is_Real_Container<ContainerY>
               and Is_Real_Container<WindowContainer>
    [[nodiscard]] static CoherenceEstimate
    coherence(const ContainerX & x,
              const ContainerY & y,
              const WindowContainer & window,
              const Real sample_rate,
              const WelchOptions & options = {})
    {
      return coherence(build_real_input(x),
                       build_real_input(y),
                       build_real_input(window),
                       sample_rate,
                       options);
    }

    /** @brief Returns the overlap-add normalization profile for one hop period. */
    [[nodiscard]] static Array<Real>
    window_overlap_profile(const Array<Real> & analysis_window,
                           const Array<Real> & synthesis_window,
                           const size_t hop_size)
    {
      return overlap_profile(analysis_window,
                             synthesis_window,
                             hop_size,
                             "FFT::window_overlap_profile");
    }

    /** @brief Returns true when the window pair satisfies NOLA. */
    [[nodiscard]] static bool
    satisfies_nola(const Array<Real> & analysis_window,
                   const Array<Real> & synthesis_window,
                   const size_t hop_size)
    {
      return overlap_profile_has_nola(window_overlap_profile(analysis_window,
                                                             synthesis_window,
                                                             hop_size));
    }

    /** @brief Returns true when the window pair satisfies COLA. */
    [[nodiscard]] static bool
    satisfies_cola(const Array<Real> & analysis_window,
                   const Array<Real> & synthesis_window,
                   const size_t hop_size)
    {
      return overlap_profile_has_cola(window_overlap_profile(analysis_window,
                                                             synthesis_window,
                                                             hop_size));
    }

    /** @brief Returns true when one window satisfies NOLA by itself. */
    [[nodiscard]] static bool
    satisfies_nola(const Array<Real> & window, const size_t hop_size)
    {
      return satisfies_nola(window, window, hop_size);
    }

    /** @brief Returns true when one window satisfies COLA by itself. */
    [[nodiscard]] static bool
    satisfies_cola(const Array<Real> & window, const size_t hop_size)
    {
      return satisfies_cola(window, window, hop_size);
    }

    /** @brief Splits a real signal into frames using a fixed hop size.
     *
     *  If @p pad_end is true, the last frame is zero-padded to @p frame_size.
     *  Otherwise, only complete frames are returned.
     */
    [[nodiscard]] static Array<Array<Real>>
    frame_signal(const Array<Real> & signal,
                 const size_t frame_size,
                 const size_t hop_size,
                 const bool pad_end = true)
    {
      Array<Array<Real>> frames;
      if (signal.is_empty())
        return frames;

      const Array<size_t> offsets =
          frame_offsets_impl(signal.size(),
                             frame_size,
                             hop_size,
                             pad_end,
                             "FFT::frame_signal");

      for (size_t item = 0; item < offsets.size(); ++item)
        {
          const size_t offset = offsets[item];
          const size_t length = std::min(frame_size, signal.size() - offset);

          Array<Real> frame = Array<Real>::create(frame_size);
          for (size_t i = 0; i < frame_size; ++i)
            frame(i) = Real(0);
          for (size_t i = 0; i < length; ++i)
            frame(i) = signal[offset + i];

          frames.append(frame);
        }

      return frames;
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Array<Real>>
    frame_signal(const Container & signal,
                 const size_t frame_size,
                 const size_t hop_size,
                 const bool pad_end = true)
    {
      return frame_signal(build_real_input(signal), frame_size, hop_size, pad_end);
    }

    /** @brief Computes a basic STFT for a real signal using a custom window.
     *
     *  Each frame is windowed and transformed with zero-padding to the next
     *  power-of-two FFT size when needed.
     */
    [[nodiscard]] static Array<Array<Complex>>
    stft(const Array<Real> & signal,
         const Array<Real> & window,
         const size_t hop_size,
         const bool pad_end = true)
    {
      STFTOptions options;
      options.hop_size = hop_size;
      options.pad_end = pad_end;
      return stft_impl(signal, window, options);
    }

    /** @brief Computes a basic STFT for a real signal using a Hann window. */
    [[nodiscard]] static Array<Array<Complex>>
    stft(const Array<Real> & signal,
         const size_t frame_size,
         const size_t hop_size,
         const bool pad_end = true)
    {
      return stft(signal, hann_window(frame_size), hop_size, pad_end);
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static Array<Array<Complex>>
    stft(const SignalContainer & signal,
         const WindowContainer & window,
         const size_t hop_size,
         const bool pad_end = true)
    {
      return stft(build_real_input(signal), build_real_input(window), hop_size, pad_end);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Array<Complex>>
    stft(const Container & signal,
         const size_t frame_size,
         const size_t hop_size,
         const bool pad_end = true)
    {
      return stft(build_real_input(signal), frame_size, hop_size, pad_end);
    }

    /** @brief Computes an STFT with explicit analysis options. */
    [[nodiscard]] static Array<Array<Complex>>
    stft(const Array<Real> & signal,
         const Array<Real> & window,
         const STFTOptions & options)
    {
      return stft_impl(signal, window, options);
    }

    /** @brief Parallel STFT using explicit analysis options. */
    [[nodiscard]] static Array<Array<Complex>>
    pstft(ThreadPool & pool,
          const Array<Real> & signal,
          const Array<Real> & window,
          const STFTOptions & options,
          const size_t chunk_size = 0)
    {
      return stft_impl(signal, window, options, &pool, chunk_size);
    }

    /** @brief Computes a Hann-window STFT with explicit analysis options. */
    [[nodiscard]] static Array<Array<Complex>>
    stft(const Array<Real> & signal,
         const size_t frame_size,
         const STFTOptions & options)
    {
      return stft(signal, hann_window(frame_size), options);
    }

    /** @brief Parallel Hann-window STFT with explicit analysis options. */
    [[nodiscard]] static Array<Array<Complex>>
    pstft(ThreadPool & pool,
          const Array<Real> & signal,
          const size_t frame_size,
          const STFTOptions & options,
          const size_t chunk_size = 0)
    {
      return pstft(pool, signal, hann_window(frame_size), options, chunk_size);
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static Array<Array<Complex>>
    stft(const SignalContainer & signal,
         const WindowContainer & window,
         const STFTOptions & options)
    {
      return stft(build_real_input(signal), build_real_input(window), options);
    }

    template <typename SignalContainer, typename WindowContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<WindowContainer>
    [[nodiscard]] static Array<Array<Complex>>
    pstft(ThreadPool & pool,
          const SignalContainer & signal,
          const WindowContainer & window,
          const STFTOptions & options,
          const size_t chunk_size = 0)
    {
      return pstft(pool,
                   build_real_input(signal),
                   build_real_input(window),
                   options,
                   chunk_size);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Array<Complex>>
    stft(const Container & signal,
         const size_t frame_size,
         const STFTOptions & options)
    {
      return stft(build_real_input(signal), frame_size, options);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Array<Complex>>
    pstft(ThreadPool & pool,
          const Container & signal,
          const size_t frame_size,
          const STFTOptions & options,
          const size_t chunk_size = 0)
    {
      return pstft(pool, build_real_input(signal), frame_size, options, chunk_size);
    }

    /** @brief Reconstructs a real signal from an STFT using overlap-add.
     *
     *  The STFT frames are assumed to come from a real signal transformed with
     *  the analysis window @p analysis_window. Each inverse frame is overlap-
     *  added after multiplying by @p synthesis_window and normalized by the
     *  accumulated window products.
     */
    [[nodiscard]] static Array<Real>
    istft(const Array<Array<Complex>> & spectrogram,
          const Array<Real> & analysis_window,
          const Array<Real> & synthesis_window,
          const size_t hop_size,
          const size_t signal_length = 0)
    {
      ISTFTOptions options;
      options.hop_size = hop_size;
      options.signal_length = signal_length;
      return istft_impl(spectrogram,
                        analysis_window,
                        synthesis_window,
                        options,
                        nullptr,
                        0);
    }

    /** @brief Reconstructs a real signal using the same window for analysis
     *         and synthesis.
     */
    [[nodiscard]] static Array<Real>
    istft(const Array<Array<Complex>> & spectrogram,
          const Array<Real> & window,
          const size_t hop_size,
          const size_t signal_length = 0)
    {
      return istft(spectrogram, window, window, hop_size, signal_length);
    }

    /** @brief Reconstructs a real signal using a Hann window pair. */
    [[nodiscard]] static Array<Real>
    istft(const Array<Array<Complex>> & spectrogram,
          const size_t frame_size,
          const size_t hop_size,
          const size_t signal_length = 0)
    {
      const Array<Real> window = hann_window(frame_size);
      return istft(spectrogram, window, window, hop_size, signal_length);
    }

    /** @brief Parallel STFT inversion using a thread pool. */
    [[nodiscard]] static Array<Real>
    pistft(ThreadPool & pool,
           const Array<Array<Complex>> & spectrogram,
           const Array<Real> & analysis_window,
           const Array<Real> & synthesis_window,
           const size_t hop_size,
           const size_t signal_length = 0,
           const size_t chunk_size = 0)
    {
      ISTFTOptions options;
      options.hop_size = hop_size;
      options.signal_length = signal_length;
      return istft_impl(spectrogram,
                        analysis_window,
                        synthesis_window,
                        options,
                        &pool,
                        chunk_size);
    }

    /** @brief Parallel STFT inversion using a shared analysis/synthesis window. */
    [[nodiscard]] static Array<Real>
    pistft(ThreadPool & pool,
           const Array<Array<Complex>> & spectrogram,
           const Array<Real> & window,
           const size_t hop_size,
           const size_t signal_length = 0,
           const size_t chunk_size = 0)
    {
      return pistft(pool,
                    spectrogram,
                    window,
                    window,
                    hop_size,
                    signal_length,
                    chunk_size);
    }

    /** @brief Parallel STFT inversion using a Hann window pair. */
    [[nodiscard]] static Array<Real>
    pistft(ThreadPool & pool,
           const Array<Array<Complex>> & spectrogram,
           const size_t frame_size,
           const size_t hop_size,
           const size_t signal_length = 0,
           const size_t chunk_size = 0)
    {
      const Array<Real> window = hann_window(frame_size);
      return pistft(pool,
                    spectrogram,
                    window,
                    window,
                    hop_size,
                    signal_length,
                    chunk_size);
    }

    /** @brief Reconstructs a real signal using explicit ISTFT options. */
    [[nodiscard]] static Array<Real>
    istft(const Array<Array<Complex>> & spectrogram,
          const Array<Real> & analysis_window,
          const Array<Real> & synthesis_window,
          const ISTFTOptions & options)
    {
      return istft_impl(spectrogram,
                        analysis_window,
                        synthesis_window,
                        options,
                        nullptr,
                        0);
    }

    /** @brief Reconstructs a real signal using one shared analysis/synthesis window. */
    [[nodiscard]] static Array<Real>
    istft(const Array<Array<Complex>> & spectrogram,
          const Array<Real> & window,
          const ISTFTOptions & options)
    {
      return istft(spectrogram, window, window, options);
    }

    /** @brief Reconstructs a Hann-window STFT with explicit options. */
    [[nodiscard]] static Array<Real>
    istft(const Array<Array<Complex>> & spectrogram,
          const size_t frame_size,
          const ISTFTOptions & options)
    {
      const Array<Real> window = hann_window(frame_size);
      return istft(spectrogram, window, window, options);
    }

    /** @brief Parallel ISTFT with explicit options. */
    [[nodiscard]] static Array<Real>
    pistft(ThreadPool & pool,
           const Array<Array<Complex>> & spectrogram,
           const Array<Real> & analysis_window,
           const Array<Real> & synthesis_window,
           const ISTFTOptions & options,
           const size_t chunk_size = 0)
    {
      return istft_impl(spectrogram,
                        analysis_window,
                        synthesis_window,
                        options,
                        &pool,
                        chunk_size);
    }

    /** @brief Parallel ISTFT using a shared analysis/synthesis window. */
    [[nodiscard]] static Array<Real>
    pistft(ThreadPool & pool,
           const Array<Array<Complex>> & spectrogram,
           const Array<Real> & window,
           const ISTFTOptions & options,
           const size_t chunk_size = 0)
    {
      return pistft(pool, spectrogram, window, window, options, chunk_size);
    }

    /** @brief Parallel Hann-window ISTFT with explicit options. */
    [[nodiscard]] static Array<Real>
    pistft(ThreadPool & pool,
           const Array<Array<Complex>> & spectrogram,
           const size_t frame_size,
           const ISTFTOptions & options,
           const size_t chunk_size = 0)
    {
      const Array<Real> window = hann_window(frame_size);
      return pistft(pool, spectrogram, window, window, options, chunk_size);
    }

    /** @brief Stateful STFT processor for chunked real-time analysis. */
    class STFTProcessor
    {
      Array<Real> window_;
      STFTOptions options_;
      size_t fft_size_ = 0;
      size_t center_pad_ = 0;
      Plan plan_;
      Array<Real> pending_;
      bool configured_ = false;
      bool has_seen_input_ = false;
      bool finalized_ = false;

      void
      require_configured(const char *ctx) const
      {
        ah_runtime_error_if(not configured_) << ctx << ": processor is not configured";
      }

      void
      initialize_pending()
      {
        pending_.empty();
        pending_.reserve(center_pad_);
        for (size_t i = 0; i < center_pad_; ++i)
          pending_.append(Real(0));
      }

      void
      append_samples(const Array<Real> & block)
      {
        ah_invalid_argument_if(pending_.size()
                               > std::numeric_limits<size_t>::max() - block.size())
          << "FFT::STFTProcessor::process_block: pending buffer overflow";
        FFT::append_all(pending_, block);
      }

      [[nodiscard]] Array<Array<Complex>>
      emit_ready_frames(ThreadPool *pool,
                        const size_t chunk_size,
                        const bool allow_partial_frames)
      {
        Array<Array<Real>> frames;
        while (not pending_.is_empty())
          {
            if (pending_.size() < window_.size() and not allow_partial_frames)
              break;

            frames.append(FFT::build_padded_frame_from_prefix(pending_,
                                                              window_.size()));
            FFT::drop_prefix(pending_, std::min(options_.hop_size, pending_.size()));
          }

        return FFT::transform_stft_frames(frames,
                                          window_,
                                          fft_size_,
                                          plan_,
                                          pool,
                                          chunk_size);
      }

    public:
      /** @brief Default constructor for an unconfigured processor. */
      STFTProcessor() = default;

      /** @brief Construct an STFT processor with a fixed analysis window.
       *
       *  @param window Analysis window (e.g. Hann, Hamming).
       *  @param options STFT parameters (hop size, centered, etc.).
       */
      explicit STFTProcessor(const Array<Real> & window,
                             const STFTOptions & options)
        : window_(window),
          options_(options),
          fft_size_(FFT::validate_stft_options(window,
                                               options,
                                               "FFT::STFTProcessor")),
          center_pad_(options.centered ? window.size() / 2 : 0),
          plan_(fft_size_),
          configured_(true)
      {
        initialize_pending();
      }

      /** @brief Construct an STFT processor using a Hann window.
       *
       *  @param frame_size Analysis window size.
       *  @param options STFT parameters.
       */
      explicit STFTProcessor(const size_t frame_size,
                             const STFTOptions & options)
        : STFTProcessor(FFT::hann_window(frame_size), options)
      {}

      /** @brief Construct an STFT processor from any real container.
       *
       *  @tparam WindowContainer Container type (e.g. std::vector<double>).
       *  @param window Analysis window.
       *  @param options STFT parameters.
       */
      template <typename WindowContainer>
        requires Is_Real_Container<WindowContainer>
      explicit STFTProcessor(const WindowContainer & window,
                             const STFTOptions & options)
        : STFTProcessor(FFT::build_real_input(window), options)
      {}

      [[nodiscard]] size_t frame_size() const noexcept
      {
        return window_.size();
      }

      [[nodiscard]] bool configured() const noexcept
      {
        return configured_;
      }

      [[nodiscard]] size_t hop_size() const noexcept
      {
        return options_.hop_size;
      }

      [[nodiscard]] size_t fft_size() const noexcept
      {
        return fft_size_;
      }

      [[nodiscard]] bool centered() const noexcept
      {
        return options_.centered;
      }

      [[nodiscard]] bool pad_end() const noexcept
      {
        return options_.pad_end;
      }

      [[nodiscard]] bool finalized() const noexcept
      {
        return finalized_;
      }

      [[nodiscard]] const Array<Real> &window() const noexcept
      {
        return window_;
      }

      void reset()
      {
        require_configured("FFT::STFTProcessor::reset");
        has_seen_input_ = false;
        finalized_ = false;
        initialize_pending();
      }

      [[nodiscard]] Array<Array<Complex>>
      process_block(const Array<Real> & block)
      {
        require_configured("FFT::STFTProcessor::process_block");
        ah_runtime_error_if(finalized_)
          << "FFT::STFTProcessor::process_block: processor already flushed";

        if (block.is_empty())
          return {};

        has_seen_input_ = true;
        append_samples(block);
        return emit_ready_frames(nullptr, 0, false);
      }

      [[nodiscard]] Array<Array<Complex>>
      pprocess_block(ThreadPool & pool,
                     const Array<Real> & block,
                     const size_t chunk_size = 0)
      {
        require_configured("FFT::STFTProcessor::pprocess_block");
        ah_runtime_error_if(finalized_)
          << "FFT::STFTProcessor::process_block: processor already flushed";

        if (block.is_empty())
          return {};

        has_seen_input_ = true;
        append_samples(block);
        return emit_ready_frames(&pool, chunk_size, false);
      }

      [[nodiscard]] Array<Array<Complex>>
      flush()
      {
        require_configured("FFT::STFTProcessor::flush");
        if (finalized_)
          return {};

        finalized_ = true;
        if (not has_seen_input_)
          {
            pending_.empty();
            return {};
          }

        if (center_pad_ != 0)
          {
            Array<Real> right_pad = Array<Real>::create(center_pad_);
            for (size_t i = 0; i < center_pad_; ++i)
              right_pad(i) = Real(0);
            append_samples(right_pad);
          }

        Array<Array<Complex>> tail =
            emit_ready_frames(nullptr, 0, options_.pad_end);
        pending_.empty();
        return tail;
      }

      [[nodiscard]] Array<Array<Complex>>
      pflush(ThreadPool & pool, const size_t chunk_size = 0)
      {
        require_configured("FFT::STFTProcessor::pflush");
        if (finalized_)
          return {};

        finalized_ = true;
        if (not has_seen_input_)
          {
            pending_.empty();
            return {};
          }

        if (center_pad_ != 0)
          {
            Array<Real> right_pad = Array<Real>::create(center_pad_);
            for (size_t i = 0; i < center_pad_; ++i)
              right_pad(i) = Real(0);
            append_samples(right_pad);
          }

        Array<Array<Complex>> tail =
            emit_ready_frames(&pool, chunk_size, options_.pad_end);
        pending_.empty();
        return tail;
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Array<Complex>>
      process_block(const Container & block)
      {
        return process_block(FFT::build_real_input(block));
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Array<Complex>>
      pprocess_block(ThreadPool & pool,
                     const Container & block,
                     const size_t chunk_size = 0)
      {
        return pprocess_block(pool, FFT::build_real_input(block), chunk_size);
      }
    };

    /** @brief Stateful ISTFT processor for chunked frame-by-frame synthesis. */
    class ISTFTProcessor
    {
      Array<Real> analysis_window_;
      Array<Real> synthesis_window_;
      ISTFTOptions options_;
      size_t fft_size_ = 0;
      size_t center_pad_ = 0;
      Plan plan_;
      Array<Real> pending_output_;
      Array<Real> pending_norm_;
      size_t frames_processed_ = 0;
      size_t emitted_raw_samples_ = 0;
      size_t left_trim_remaining_ = 0;
      size_t emitted_signal_samples_ = 0;
      bool configured_ = false;
      bool has_seen_frame_ = false;
      bool finalized_ = false;

      void
      require_configured(const char *ctx) const
      {
        ah_runtime_error_if(not configured_)
          << ctx << ": processor is not configured";
      }

      void
      initialize_state()
      {
        pending_output_.empty();
        pending_norm_.empty();
        frames_processed_ = 0;
        emitted_raw_samples_ = 0;
        left_trim_remaining_ = center_pad_;
        emitted_signal_samples_ = 0;
        has_seen_frame_ = false;
        finalized_ = false;
      }

      void
      ensure_pending_size(const size_t size)
      {
        if (pending_output_.size() >= size)
          return;

        pending_output_.reserve(size);
        pending_norm_.reserve(size);
        while (pending_output_.size() < size)
          {
            pending_output_.append(Real(0));
            pending_norm_.append(Real(0));
          }
      }

      void
      accumulate_frame(const Array<Real> & frame)
      {
        ah_invalid_argument_if(options_.hop_size != 0
                               and frames_processed_
                               > std::numeric_limits<size_t>::max()
                               / options_.hop_size)
          << "FFT::ISTFTProcessor::process_frame: frame offset overflow";

        const size_t frame_offset = frames_processed_ * options_.hop_size;
        ah_runtime_error_if(frame_offset < emitted_raw_samples_)
          << "FFT::ISTFTProcessor::process_frame: invalid overlap-add state";

        const size_t offset = frame_offset - emitted_raw_samples_;
        ensure_pending_size(offset + analysis_window_.size());

        for (size_t i = 0; i < analysis_window_.size(); ++i)
          {
            pending_output_(offset + i) += frame[i] * synthesis_window_[i];
            pending_norm_(offset + i) += analysis_window_[i] * synthesis_window_[i];
          }

        ++frames_processed_;
      }

      [[nodiscard]] Array<Real>
      normalize_prefix(const size_t count)
      {
        ah_invalid_argument_if(count > pending_output_.size())
          << "FFT::ISTFTProcessor::normalize_prefix: count " << count
          << " exceeds pending size " << pending_output_.size();

        if (count == 0)
          return {};

        const Real tol = (max_abs_value(pending_norm_) + Real(1))
                         * Real(256) * std::numeric_limits<Real>::epsilon();
        Array<Real> normalized = Array<Real>::create(count);
        for (size_t i = 0; i < count; ++i)
          {
            if (std::abs(pending_norm_[i]) <= tol)
              {
                ah_domain_error_if(std::abs(pending_output_[i]) > tol)
                  << "FFT::ISTFTProcessor: overlap-add normalization vanished at sample "
                  << (emitted_raw_samples_ + i);
                normalized(i) = Real(0);
              }
            else
              normalized(i) = pending_output_[i] / pending_norm_[i];
          }

        FFT::drop_prefix(pending_output_, count);
        FFT::drop_prefix(pending_norm_, count);
        emitted_raw_samples_ += count;
        return normalized;
      }

      [[nodiscard]] Array<Real>
      emit_samples(const Array<Real> & normalized,
                   const bool final_flush)
      {
        if (normalized.is_empty())
          return {};

        const size_t start = std::min(left_trim_remaining_, normalized.size());
        left_trim_remaining_ -= start;
        size_t end = normalized.size();

        if (final_flush and options_.centered and options_.signal_length == 0)
          end = end > center_pad_ ? end - center_pad_ : start;

        if (start >= end)
          return {};

        size_t available = end - start;
        if (options_.signal_length != 0)
          {
            if (emitted_signal_samples_ >= options_.signal_length)
              return {};

            available = std::min(available,
                                 options_.signal_length - emitted_signal_samples_);
          }

        Array<Real> emitted;
        emitted.reserve(available);
        for (size_t i = 0; i < available; ++i)
          emitted.append(normalized[start + i]);
        emitted_signal_samples_ += emitted.size();
        return emitted;
      }

      [[nodiscard]] Array<Real>
      drain_ready_samples(const bool final_flush)
      {
        size_t ready_raw = 0;
        if (final_flush)
          ready_raw = pending_output_.size();
        else
          {
            const size_t next_offset = frames_processed_ * options_.hop_size;
            ah_runtime_error_if(next_offset < emitted_raw_samples_)
              << "FFT::ISTFTProcessor: invalid finalized-sample accounting";
            ready_raw = next_offset - emitted_raw_samples_;
            const size_t right_hold =
                options_.centered and options_.signal_length == 0 ? center_pad_ : 0;
            ready_raw = ready_raw > right_hold ? ready_raw - right_hold : 0;
          }

        return emit_samples(normalize_prefix(ready_raw), final_flush);
      }

      [[nodiscard]] Array<Real>
      process_frame_impl(const Array<Complex> & spectrum,
                         ThreadPool *pool,
                         const size_t chunk_size)
      {
        require_configured("FFT::ISTFTProcessor::process_frame");
        ah_runtime_error_if(finalized_)
          << "FFT::ISTFTProcessor::process_frame: processor already flushed";
        ah_invalid_argument_if(spectrum.size() != fft_size_)
          << "FFT::ISTFTProcessor::process_frame: frame size " << spectrum.size()
          << " does not match configured FFT size " << fft_size_;

        has_seen_frame_ = true;
        const Array<Real> frame = pool != nullptr ?
                                    plan_.pinverse_transform_real(*pool, spectrum, chunk_size) :
                                    plan_.inverse_transform_real(spectrum);
        accumulate_frame(frame);
        return drain_ready_samples(false);
      }

    public:
      /** @brief Default constructor for an unconfigured inverse processor. */
      ISTFTProcessor() = default;

      /** @brief Construct an inverse STFT processor with custom windows.
       *
       *  @param fft_size Fixed FFT transform size.
       *  @param analysis_window Analysis window used during STFT.
       *  @param synthesis_window Synthesis window for reconstruction.
       *  @param options Overlap-add parameters.
       */
      explicit ISTFTProcessor(const size_t fft_size,
                              const Array<Real> & analysis_window,
                              const Array<Real> & synthesis_window,
                              const ISTFTOptions & options)
        : analysis_window_(analysis_window),
          synthesis_window_(synthesis_window),
          options_(options),
          fft_size_(fft_size),
          center_pad_(options.centered ? analysis_window.size() / 2 : 0),
          plan_(fft_size),
          configured_(true)
      {
        FFT::validate_istft_configuration(analysis_window_,
                                          synthesis_window_,
                                          fft_size_,
                                          options_,
                                          "FFT::ISTFTProcessor");
        initialize_state();
      }

      /** @brief Construct an inverse STFT processor with a single window.
       *
       *  @param fft_size Fixed FFT transform size.
       *  @param window Window used for both analysis and synthesis.
       *  @param options Overlap-add parameters.
       */
      explicit ISTFTProcessor(const size_t fft_size,
                              const Array<Real> & window,
                              const ISTFTOptions & options)
        : ISTFTProcessor(fft_size, window, window, options)
      {}

      /** @brief Construct an inverse STFT processor with a Hann window.
       *
       *  @param fft_size Fixed FFT transform size.
       *  @param frame_size Hann window size.
       *  @param options Overlap-add parameters.
       */
      explicit ISTFTProcessor(const size_t fft_size,
                              const size_t frame_size,
                              const ISTFTOptions & options)
        : ISTFTProcessor(fft_size,
                         FFT::hann_window(frame_size),
                         FFT::hann_window(frame_size),
                         options)
      {}

      /** @brief Construct an inverse STFT processor from any real containers.
       *
       *  @tparam AnalysisContainer Type for analysis window.
       *  @tparam SynthesisContainer Type for synthesis window.
       *  @param fft_size Fixed FFT transform size.
       *  @param analysis_window Analysis window.
       *  @param synthesis_window Synthesis window.
       *  @param options Overlap-add parameters.
       */
      template <typename AnalysisContainer, typename SynthesisContainer>
        requires Is_Real_Container<AnalysisContainer>
                 and Is_Real_Container<SynthesisContainer>
      explicit ISTFTProcessor(const size_t fft_size,
                              const AnalysisContainer & analysis_window,
                              const SynthesisContainer & synthesis_window,
                              const ISTFTOptions & options)
        : ISTFTProcessor(fft_size,
                         FFT::build_real_input(analysis_window),
                         FFT::build_real_input(synthesis_window),
                         options)
      {}

      [[nodiscard]] size_t frame_size() const noexcept
      {
        return analysis_window_.size();
      }

      [[nodiscard]] bool configured() const noexcept
      {
        return configured_;
      }

      [[nodiscard]] size_t fft_size() const noexcept
      {
        return fft_size_;
      }

      [[nodiscard]] size_t hop_size() const noexcept
      {
        return options_.hop_size;
      }

      [[nodiscard]] bool centered() const noexcept
      {
        return options_.centered;
      }

      [[nodiscard]] bool finalized() const noexcept
      {
        return finalized_;
      }

      void reset()
      {
        require_configured("FFT::ISTFTProcessor::reset");
        initialize_state();
      }

      [[nodiscard]] Array<Real>
      process_frame(const Array<Complex> & spectrum)
      {
        return process_frame_impl(spectrum, nullptr, 0);
      }

      [[nodiscard]] Array<Real>
      pprocess_frame(ThreadPool & pool,
                     const Array<Complex> & spectrum,
                     const size_t chunk_size = 0)
      {
        return process_frame_impl(spectrum, &pool, chunk_size);
      }

      [[nodiscard]] Array<Real>
      process_block(const Array<Array<Complex>> & spectrogram_block)
      {
        Array<Real> emitted;
        for (size_t i = 0; i < spectrogram_block.size(); ++i)
          FFT::append_all(emitted, process_frame(spectrogram_block[i]));
        return emitted;
      }

      [[nodiscard]] Array<Real>
      pprocess_block(ThreadPool & pool,
                     const Array<Array<Complex>> & spectrogram_block,
                     const size_t chunk_size = 0)
      {
        Array<Real> emitted;
        for (size_t i = 0; i < spectrogram_block.size(); ++i)
          FFT::append_all(emitted,
                          pprocess_frame(pool, spectrogram_block[i], chunk_size));
        return emitted;
      }

      [[nodiscard]] Array<Real>
      flush()
      {
        require_configured("FFT::ISTFTProcessor::flush");
        if (finalized_)
          return {};

        finalized_ = true;
        if (not has_seen_frame_)
          {
            pending_output_.empty();
            pending_norm_.empty();
            return {};
          }

        Array<Real> tail = drain_ready_samples(true);
        pending_output_.empty();
        pending_norm_.empty();
        return tail;
      }

      [[nodiscard]] Array<Real>
      pflush(ThreadPool &, const size_t = 0)
      {
        require_configured("FFT::ISTFTProcessor::pflush");
        return flush();
      }

      template <typename Container>
        requires Is_Complex_Container<Container>
      [[nodiscard]] Array<Real>
      process_frame(const Container & spectrum)
      {
        return process_frame(FFT::build_complex_input(spectrum));
      }

      template <typename Container>
        requires Is_Complex_Container<Container>
      [[nodiscard]] Array<Real>
      pprocess_frame(ThreadPool & pool,
                     const Container & spectrum,
                     const size_t chunk_size = 0)
      {
        return pprocess_frame(pool, FFT::build_complex_input(spectrum), chunk_size);
      }
    };

    /** @brief Stateful multichannel STFT processor with one analyzer per channel. */
    class BatchedSTFTProcessor
    {
      Array<STFTProcessor> processors_;

      void
      validate_batch_size(const size_t size, const char *ctx) const
      {
        ah_invalid_argument_if(size != processors_.size())
          << ctx << ": batch size " << size
          << " does not match configured channel count " << processors_.size();
      }

    public:
      explicit BatchedSTFTProcessor(const size_t num_channels,
                                    const Array<Real> & window,
                                    const STFTOptions & options)
      {
        ah_invalid_argument_if(num_channels == 0)
          << "FFT::BatchedSTFTProcessor: at least one channel is required";
        processors_.reserve(num_channels);
        for (size_t i = 0; i < num_channels; ++i)
          processors_.append(STFTProcessor(window, options));
      }

      explicit BatchedSTFTProcessor(const size_t num_channels,
                                    const size_t frame_size,
                                    const STFTOptions & options)
        : BatchedSTFTProcessor(num_channels, FFT::hann_window(frame_size), options)
      {}

      template <typename WindowContainer>
        requires Is_Real_Container<WindowContainer>
      explicit BatchedSTFTProcessor(const size_t num_channels,
                                    const WindowContainer & window,
                                    const STFTOptions & options)
        : BatchedSTFTProcessor(num_channels,
                               FFT::build_real_input(window),
                               options)
      {}

      [[nodiscard]] size_t num_channels() const noexcept
      {
        return processors_.size();
      }

      [[nodiscard]] STFTProcessor &
      channel(const size_t index)
      {
        ah_out_of_range_error_if(index >= processors_.size())
          << "FFT::BatchedSTFTProcessor::channel: index " << index
          << " out of range for " << processors_.size() << " channels";
        return processors_[index];
      }

      [[nodiscard]] const STFTProcessor &
      channel(const size_t index) const
      {
        ah_out_of_range_error_if(index >= processors_.size())
          << "FFT::BatchedSTFTProcessor::channel: index " << index
          << " out of range for " << processors_.size() << " channels";
        return processors_[index];
      }

      void reset()
      {
        for (size_t i = 0; i < processors_.size(); ++i)
          processors_(i).reset();
      }

      [[nodiscard]] Array<Array<Array<Complex>>>
      process_block(const Array<Array<Real>> & block)
      {
        validate_batch_size(block.size(), "FFT::BatchedSTFTProcessor::process_block");
        Array<Array<Array<Complex>>> output(processors_.size(),
                                            Array<Array<Complex>>());
        for (size_t i = 0; i < processors_.size(); ++i)
          output(i) = processors_[i].process_block(block[i]);
        return output;
      }

      [[nodiscard]] Array<Array<Array<Complex>>>
      pprocess_block(ThreadPool & pool,
                     const Array<Array<Real>> & block,
                     const size_t chunk_size = 0)
      {
        validate_batch_size(block.size(),
                            "FFT::BatchedSTFTProcessor::pprocess_block");
        Array<Array<Array<Complex>>> output(processors_.size(),
                                            Array<Array<Complex>>());
        parallel_for_index(pool,
                           0,
                           processors_.size(),
                           [this, &block, &pool, chunk_size, &output](const size_t i)
                             {
                               output(i) =
                                   processors_[i].pprocess_block(pool,
                                                                 block[i],
                                                                 chunk_size);
                             },
                           chunk_size);
        return output;
      }

      [[nodiscard]] Array<Array<Array<Complex>>>
      flush()
      {
        Array<Array<Array<Complex>>> output(processors_.size(),
                                            Array<Array<Complex>>());
        for (size_t i = 0; i < processors_.size(); ++i)
          output(i) = processors_[i].flush();
        return output;
      }

      [[nodiscard]] Array<Array<Array<Complex>>>
      pflush(ThreadPool & pool, const size_t chunk_size = 0)
      {
        Array<Array<Array<Complex>>> output(processors_.size(),
                                            Array<Array<Complex>>());
        parallel_for_index(pool,
                           0,
                           processors_.size(),
                           [this, &pool, chunk_size, &output](const size_t i)
                             {
                               output(i) = processors_[i].pflush(pool, chunk_size);
                             },
                           chunk_size);
        return output;
      }
    };

    /** @brief Stateful multichannel ISTFT processor with one synthesizer per channel. */
    class BatchedISTFTProcessor
    {
      Array<ISTFTProcessor> processors_;

      [[nodiscard]] static ISTFTOptions
      channel_options(const ISTFTOptions & options,
                      const Array<size_t> & signal_lengths,
                      const size_t index)
      {
        ISTFTOptions channel = options;
        if (not signal_lengths.is_empty())
          channel.signal_length = signal_lengths[index];
        return channel;
      }

      void
      validate_batch_size(const size_t size, const char *ctx) const
      {
        ah_invalid_argument_if(size != processors_.size())
          << ctx << ": batch size " << size
          << " does not match configured channel count " << processors_.size();
      }

    public:
      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const Array<Real> & analysis_window,
                                     const Array<Real> & synthesis_window,
                                     const ISTFTOptions & options)
        : BatchedISTFTProcessor(num_channels,
                                fft_size,
                                analysis_window,
                                synthesis_window,
                                options,
                                Array<size_t>())
      {}

      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const Array<Real> & analysis_window,
                                     const Array<Real> & synthesis_window,
                                     const ISTFTOptions & options,
                                     const Array<size_t> & signal_lengths)
      {
        ah_invalid_argument_if(num_channels == 0)
          << "FFT::BatchedISTFTProcessor: at least one channel is required";
        ah_invalid_argument_if(not signal_lengths.is_empty()
                               and signal_lengths.size() != num_channels)
          << "FFT::BatchedISTFTProcessor: signal length count "
          << signal_lengths.size()
          << " does not match channel count " << num_channels;
        processors_.reserve(num_channels);
        for (size_t i = 0; i < num_channels; ++i)
          processors_.append(ISTFTProcessor(fft_size,
                                            analysis_window,
                                            synthesis_window,
                                            channel_options(options,
                                                            signal_lengths,
                                                            i)));
      }

      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const Array<Real> & window,
                                     const ISTFTOptions & options)
        : BatchedISTFTProcessor(num_channels,
                                fft_size,
                                window,
                                window,
                                options,
                                Array<size_t>())
      {}

      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const Array<Real> & window,
                                     const ISTFTOptions & options,
                                     const Array<size_t> & signal_lengths)
        : BatchedISTFTProcessor(num_channels,
                                fft_size,
                                window,
                                window,
                                options,
                                signal_lengths)
      {}

      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const size_t frame_size,
                                     const ISTFTOptions & options)
        : BatchedISTFTProcessor(num_channels,
                                fft_size,
                                FFT::hann_window(frame_size),
                                FFT::hann_window(frame_size),
                                options,
                                Array<size_t>())
      {}

      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const size_t frame_size,
                                     const ISTFTOptions & options,
                                     const Array<size_t> & signal_lengths)
        : BatchedISTFTProcessor(num_channels,
                                fft_size,
                                FFT::hann_window(frame_size),
                                FFT::hann_window(frame_size),
                                options,
                                signal_lengths)
      {}

      template <typename AnalysisContainer, typename SynthesisContainer>
        requires Is_Real_Container<AnalysisContainer>
                 and Is_Real_Container<SynthesisContainer>
      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const AnalysisContainer & analysis_window,
                                     const SynthesisContainer & synthesis_window,
                                     const ISTFTOptions & options)
        : BatchedISTFTProcessor(num_channels,
                                fft_size,
                                FFT::build_real_input(analysis_window),
                                FFT::build_real_input(synthesis_window),
                                options,
                                Array<size_t>())
      {}

      template <typename AnalysisContainer, typename SynthesisContainer>
        requires Is_Real_Container<AnalysisContainer>
                 and Is_Real_Container<SynthesisContainer>
      explicit BatchedISTFTProcessor(const size_t num_channels,
                                     const size_t fft_size,
                                     const AnalysisContainer & analysis_window,
                                     const SynthesisContainer & synthesis_window,
                                     const ISTFTOptions & options,
                                     const Array<size_t> & signal_lengths)
        : BatchedISTFTProcessor(num_channels,
                                fft_size,
                                FFT::build_real_input(analysis_window),
                                FFT::build_real_input(synthesis_window),
                                options,
                                signal_lengths)
      {}

      [[nodiscard]] size_t num_channels() const noexcept
      {
        return processors_.size();
      }

      [[nodiscard]] ISTFTProcessor &
      channel(const size_t index)
      {
        ah_out_of_range_error_if(index >= processors_.size())
          << "FFT::BatchedISTFTProcessor::channel: index " << index
          << " out of range for " << processors_.size() << " channels";
        return processors_[index];
      }

      [[nodiscard]] const ISTFTProcessor &
      channel(const size_t index) const
      {
        ah_out_of_range_error_if(index >= processors_.size())
          << "FFT::BatchedISTFTProcessor::channel: index " << index
          << " out of range for " << processors_.size() << " channels";
        return processors_[index];
      }

      void reset()
      {
        for (size_t i = 0; i < processors_.size(); ++i)
          processors_(i).reset();
      }

      [[nodiscard]] Array<Array<Real>>
      process_block(const Array<Array<Array<Complex>>> & block)
      {
        validate_batch_size(block.size(),
                            "FFT::BatchedISTFTProcessor::process_block");
        Array<Array<Real>> output(processors_.size(), Array<Real>());
        for (size_t i = 0; i < processors_.size(); ++i)
          output(i) = processors_[i].process_block(block[i]);
        return output;
      }

      [[nodiscard]] Array<Array<Real>>
      pprocess_block(ThreadPool & pool,
                     const Array<Array<Array<Complex>>> & block,
                     const size_t chunk_size = 0)
      {
        validate_batch_size(block.size(),
                            "FFT::BatchedISTFTProcessor::pprocess_block");
        Array<Array<Real>> output(processors_.size(), Array<Real>());
        parallel_for_index(pool,
                           0,
                           processors_.size(),
                           [this, &block, &pool, chunk_size, &output](const size_t i)
                             {
                               output(i) =
                                   processors_[i].pprocess_block(pool,
                                                                 block[i],
                                                                 chunk_size);
                             },
                           chunk_size);
        return output;
      }

      [[nodiscard]] Array<Array<Real>>
      flush()
      {
        Array<Array<Real>> output(processors_.size(), Array<Real>());
        for (size_t i = 0; i < processors_.size(); ++i)
          output(i) = processors_[i].flush();
        return output;
      }

      [[nodiscard]] Array<Array<Real>>
      pflush(ThreadPool & pool, const size_t chunk_size = 0)
      {
        Array<Array<Real>> output(processors_.size(), Array<Real>());
        parallel_for_index(pool,
                           0,
                           processors_.size(),
                           [this, &pool, chunk_size, &output](const size_t i)
                             {
                               output(i) = processors_[i].pflush(pool, chunk_size);
                             },
                           chunk_size);
        return output;
      }
    };

    /** @brief Batched STFT over a collection of real signals. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    batched_stft(const Array<Array<Real>> & signals,
                 const Array<Real> & window,
                 const STFTOptions & options)
    {
      if (signals.is_empty())
        return {};

      Array<Array<Array<Complex>>> output(signals.size(), Array<Array<Complex>>());
      for (size_t i = 0; i < signals.size(); ++i)
        output(i) = stft_impl(signals[i], window, options);
      return output;
    }

    /** @brief Parallel batched STFT across signals. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    pbatched_stft(ThreadPool & pool,
                  const Array<Array<Real>> & signals,
                  const Array<Real> & window,
                  const STFTOptions & options,
                  const size_t chunk_size = 0)
    {
      if (signals.is_empty())
        return {};

      Array<Array<Array<Complex>>> output(signals.size(), Array<Array<Complex>>());
      const bool parallelize = pool.num_threads() > 1 and signals.size() > 1;
      auto transform_one = [&signals, &window, &options, &output](const size_t i)
        {
          output(i) = stft_impl(signals[i], window, options);
        };

      if (parallelize)
        parallel_for_index(pool, 0, signals.size(), transform_one, chunk_size);
      else
        for (size_t i = 0; i < signals.size(); ++i)
          output(i) = stft_impl(signals[i], window, options, &pool, chunk_size);

      return output;
    }

    /** @brief Hann-window batched STFT. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    batched_stft(const Array<Array<Real>> & signals,
                 const size_t frame_size,
                 const STFTOptions & options)
    {
      return batched_stft(signals, hann_window(frame_size), options);
    }

    /** @brief Parallel Hann-window batched STFT. */
    [[nodiscard]] static Array<Array<Array<Complex>>>
    pbatched_stft(ThreadPool & pool,
                  const Array<Array<Real>> & signals,
                  const size_t frame_size,
                  const STFTOptions & options,
                  const size_t chunk_size = 0)
    {
      return pbatched_stft(pool,
                           signals,
                           hann_window(frame_size),
                           options,
                           chunk_size);
    }

    /** @brief Batched ISTFT with optional per-signal output lengths. */
    [[nodiscard]] static Array<Array<Real>>
    batched_istft(const Array<Array<Array<Complex>>> & spectrograms,
                  const Array<Real> & analysis_window,
                  const Array<Real> & synthesis_window,
                  const ISTFTOptions & options,
                  const Array<size_t> & signal_lengths = {})
    {
      if (spectrograms.is_empty())
        return {};

      ah_invalid_argument_if(not signal_lengths.is_empty()
                             and signal_lengths.size() != spectrograms.size())
        << "FFT::batched_istft: signal_lengths size "
        << signal_lengths.size()
        << " does not match batch size " << spectrograms.size();

      Array<Array<Real>> output(spectrograms.size(), Array<Real>());
      for (size_t i = 0; i < spectrograms.size(); ++i)
        {
          ISTFTOptions local_options = options;
          if (not signal_lengths.is_empty())
            local_options.signal_length = signal_lengths[i];
          output(i) = istft_impl(spectrograms[i],
                                 analysis_window,
                                 synthesis_window,
                                 local_options,
                                 nullptr,
                                 0);
        }
      return output;
    }

    /** @brief Parallel batched ISTFT across signals. */
    [[nodiscard]] static Array<Array<Real>>
    pbatched_istft(ThreadPool & pool,
                   const Array<Array<Array<Complex>>> & spectrograms,
                   const Array<Real> & analysis_window,
                   const Array<Real> & synthesis_window,
                   const ISTFTOptions & options,
                   const Array<size_t> & signal_lengths = {},
                   const size_t chunk_size = 0)
    {
      if (spectrograms.is_empty())
        return {};

      ah_invalid_argument_if(not signal_lengths.is_empty()
                             and signal_lengths.size() != spectrograms.size())
        << "FFT::pbatched_istft: signal_lengths size "
        << signal_lengths.size()
        << " does not match batch size " << spectrograms.size();

      Array<Array<Real>> output(spectrograms.size(), Array<Real>());
      const bool parallelize = pool.num_threads() > 1 and spectrograms.size() > 1;
      auto reconstruct_one =
          [&spectrograms, &analysis_window, &synthesis_window, &options,
            &signal_lengths, &output](const size_t i)
        {
          ISTFTOptions local_options = options;
          if (not signal_lengths.is_empty())
            local_options.signal_length = signal_lengths[i];
          output(i) = istft_impl(spectrograms[i],
                                 analysis_window,
                                 synthesis_window,
                                 local_options,
                                 nullptr,
                                 0);
        };

      if (parallelize)
        parallel_for_index(pool, 0, spectrograms.size(), reconstruct_one, chunk_size);
      else
        for (size_t i = 0; i < spectrograms.size(); ++i)
          {
            ISTFTOptions local_options = options;
            if (not signal_lengths.is_empty())
              local_options.signal_length = signal_lengths[i];
            output(i) = istft_impl(spectrograms[i],
                                   analysis_window,
                                   synthesis_window,
                                   local_options,
                                   &pool,
                                   chunk_size);
          }

      return output;
    }

    /** @brief Batched ISTFT using a shared analysis/synthesis window. */
    [[nodiscard]] static Array<Array<Real>>
    batched_istft(const Array<Array<Array<Complex>>> & spectrograms,
                  const Array<Real> & window,
                  const ISTFTOptions & options,
                  const Array<size_t> & signal_lengths = {})
    {
      return batched_istft(spectrograms,
                           window,
                           window,
                           options,
                           signal_lengths);
    }

    /** @brief Parallel batched ISTFT using a shared analysis/synthesis window. */
    [[nodiscard]] static Array<Array<Real>>
    pbatched_istft(ThreadPool & pool,
                   const Array<Array<Array<Complex>>> & spectrograms,
                   const Array<Real> & window,
                   const ISTFTOptions & options,
                   const Array<size_t> & signal_lengths = {},
                   const size_t chunk_size = 0)
    {
      return pbatched_istft(pool,
                            spectrograms,
                            window,
                            window,
                            options,
                            signal_lengths,
                            chunk_size);
    }

    /** @brief Hann-window batched ISTFT. */
    [[nodiscard]] static Array<Array<Real>>
    batched_istft(const Array<Array<Array<Complex>>> & spectrograms,
                  const size_t frame_size,
                  const ISTFTOptions & options,
                  const Array<size_t> & signal_lengths = {})
    {
      const Array<Real> window = hann_window(frame_size);
      return batched_istft(spectrograms, window, window, options, signal_lengths);
    }

    /** @brief Parallel Hann-window batched ISTFT. */
    [[nodiscard]] static Array<Array<Real>>
    pbatched_istft(ThreadPool & pool,
                   const Array<Array<Array<Complex>>> & spectrograms,
                   const size_t frame_size,
                   const ISTFTOptions & options,
                   const Array<size_t> & signal_lengths = {},
                   const size_t chunk_size = 0)
    {
      const Array<Real> window = hann_window(frame_size);
      return pbatched_istft(pool,
                            spectrograms,
                            window,
                            window,
                            options,
                            signal_lengths,
                            chunk_size);
    }

    /** @brief Stateful direct-form II transposed IIR filter. */
    class LFilter
    {
      IIRCoefficients coeffs_;
      Array<Real> state_;
      bool configured_ = false;

      void
      require_configured(const char *ctx) const
      {
        ah_runtime_error_if(not configured_)
          << ctx << ": filter is not configured";
      }

      void
      initialize_state()
      {
        const size_t order = coeffs_.denominator.size() == 0 ? 0 : coeffs_.denominator.size() - 1;
        state_ = Array<Real>::create(order);
        for (size_t i = 0; i < order; ++i)
          state_(i) = Real(0);
      }

    public:
      /** @brief Default constructor for an unconfigured LFilter. */
      LFilter() = default;

      /** @brief Construct a linear filter from numerator and denominator arrays.
       *
       *  The filter is normalized such that a[0] = 1.
       *
       *  @param numerator Feed-forward coefficients (B).
       *  @param denominator Feed-back coefficients (A).
       */
      explicit LFilter(const Array<Real> & numerator,
                       const Array<Real> & denominator)
        : coeffs_(FFT::normalize_iir_coefficients(numerator,
                                                  denominator,
                                                  "FFT::LFilter")),
          configured_(true)
      {
        initialize_state();
      }

      /** @brief Construct a linear filter from IIRCoefficients.
       *  @param coeffs IIR coefficients. */
      explicit LFilter(const IIRCoefficients & coeffs)
        : LFilter(coeffs.numerator, coeffs.denominator)
      {}

      /** @brief Construct a linear filter from a single BiquadSection.
       *  @param section Biquad section. */
      explicit LFilter(const BiquadSection & section)
        : LFilter(section.numerator(), section.denominator())
      {}

      /** @brief Construct a linear filter from any real containers.
       *
       *  @tparam NumContainer Type for numerator.
       *  @tparam DenContainer Type for denominator.
       *  @param numerator Feed-forward coefficients.
       *  @param denominator Feed-back coefficients.
       */
      template <typename NumContainer, typename DenContainer>
        requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
      explicit LFilter(const NumContainer & numerator,
                       const DenContainer & denominator)
        : LFilter(build_real_input(numerator), build_real_input(denominator))
      {}

      /** @brief Returns the filter order (number of feedback coefficients - 1). */
      [[nodiscard]] size_t order() const noexcept
      {
        return coeffs_.denominator.is_empty() ? 0 : coeffs_.denominator.size() - 1;
      }

      /** @brief Returns whether the filter has been initialized with coefficients. */
      [[nodiscard]] bool configured() const noexcept
      {
        return configured_;
      }

      /** @brief Returns the active normalized filter coefficients. */
      [[nodiscard]] const IIRCoefficients &coefficients() const noexcept
      {
        return coeffs_;
      }

      /** @brief Returns the current internal delay-line state. */
      [[nodiscard]] const Array<Real> &state() const noexcept
      {
        return state_;
      }

      /** @brief Resets the delay-line state to zero. */
      void reset()
      {
        require_configured("FFT::LFilter::reset");
        for (size_t i = 0; i < state_.size(); ++i)
          state_(i) = Real(0);
      }

      /** @brief Manually overrides the delay-line state. */
      void set_state(const Array<Real> & new_state)
      {
        require_configured("FFT::LFilter::set_state");
        ah_invalid_argument_if(new_state.size() != state_.size())
          << "FFT::LFilter::set_state: state size " << new_state.size()
          << " does not match filter order " << state_.size();
        state_ = new_state;
      }

      /** @brief Processes a signal block through the stateful filter. */
      [[nodiscard]] Array<Real>
      filter(const Array<Real> & signal)
      {
        require_configured("FFT::LFilter::filter");
        return FFT::iir_filter_impl(signal,
                                    coeffs_.numerator,
                                    coeffs_.denominator,
                                    state_,
                                    "FFT::LFilter::filter",
                                    &state_);
      }

      /** @brief Processes a real-valued container through the stateful filter. */
      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      filter(const Container & signal)
      {
        return filter(build_real_input(signal));
      }
    };

    /** @brief Stateful cascade of second-order sections. */
    class SOSFilter
    {
      Array<BiquadSection> sections_;
      Array<IIRCoefficients> coeffs_;
      Array<Array<Real>> states_;
      bool configured_ = false;

      void
      require_configured(const char *ctx) const
      {
        ah_runtime_error_if(not configured_)
          << ctx << ": filter is not configured";
      }

      void
      initialize_states()
      {
        states_ = {};
        states_.reserve(coeffs_.size());
        for (size_t i = 0; i < coeffs_.size(); ++i)
          {
            Array<Real> state = Array<Real>::create(coeffs_[i].denominator.size() - 1);
            for (size_t j = 0; j < state.size(); ++j)
              state(j) = Real(0);
            states_.append(state);
          }
      }

    public:
      /** @brief Default constructor for an unconfigured SOSFilter. */
      SOSFilter() = default;

      /** @brief Construct a cascade of second-order sections (SOS).
       *  @param sections Array of biquad sections. */
      explicit SOSFilter(const Array<BiquadSection> & sections)
        : sections_(sections),
          configured_(true)
      {
        ah_invalid_argument_if(sections_.is_empty())
          << "FFT::SOSFilter: at least one biquad section is required";

        coeffs_.reserve(sections_.size());
        for (size_t i = 0; i < sections_.size(); ++i)
          coeffs_.append(FFT::normalize_iir_coefficients(sections_[i].numerator(),
                                                         sections_[i].denominator(),
                                                         "FFT::SOSFilter"));
        initialize_states();
      }

      /** @brief Construct a cascade of SOS from any biquad container.
       *  @tparam SectionsContainer Container of BiquadSection.
       *  @param sections Biquad sections. */
      template <typename SectionsContainer>
        requires Is_Biquad_Container<SectionsContainer>
      explicit SOSFilter(const SectionsContainer & sections)
        : SOSFilter(to_array<BiquadSection>(sections))
      {}

      /** @brief Returns the number of sections in the cascade. */
      [[nodiscard]] size_t num_sections() const noexcept
      {
        return sections_.size();
      }

      /** @brief Returns whether the filter has been configured. */
      [[nodiscard]] bool configured() const noexcept
      {
        return configured_;
      }

      /** @brief Returns the current delay-line state of section @p index. */
      [[nodiscard]] const Array<Real> &
      state(const size_t index) const
      {
        ah_out_of_range_error_if(index >= states_.size())
          << "FFT::SOSFilter::state: index " << index
          << " out of range for " << states_.size() << " sections";
        return states_[index];
      }

      /** @brief Resets all internal delay lines to zero. */
      void reset()
      {
        require_configured("FFT::SOSFilter::reset");
        for (size_t i = 0; i < states_.size(); ++i)
          for (size_t j = 0; j < states_[i].size(); ++j)
            states_(i)(j) = Real(0);
      }

      /** @brief Processes a signal block through the stateful SOS cascade. */
      [[nodiscard]] Array<Real>
      filter(const Array<Real> & signal)
      {
        require_configured("FFT::SOSFilter::filter");
        Array<Real> stage = signal;
        for (size_t i = 0; i < coeffs_.size(); ++i)
          {
            Array<Real> final_state;
            stage = FFT::iir_filter_impl(stage,
                                         coeffs_[i].numerator,
                                         coeffs_[i].denominator,
                                         states_[i],
                                         "FFT::SOSFilter::filter",
                                         &final_state);
            states_(i) = final_state;
          }
        return stage;
      }

      /** @brief Processes a real-valued container through the stateful SOS cascade. */
      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      filter(const Container & signal)
      {
        return filter(build_real_input(signal));
      }
    };

    /** @brief Stateful bank of direct-form II transposed IIR filters. */
    class LFilterBank
    {
      Array<LFilter> filters_;

      /** @brief Rationale: Internal safety guard. */
      void
      validate_channel_count(const size_t count, const char *ctx) const
      {
        ah_invalid_argument_if(count != filters_.size())
          << ctx << ": batch size " << count
          << " does not match configured channel count " << filters_.size();
      }

    public:
      explicit LFilterBank(const size_t num_channels,
                           const Array<Real> & numerator,
                           const Array<Real> & denominator)
      {
        ah_invalid_argument_if(num_channels == 0)
          << "FFT::LFilterBank: at least one channel is required";
        filters_.reserve(num_channels);
        for (size_t i = 0; i < num_channels; ++i)
          filters_.append(LFilter(numerator, denominator));
      }

      explicit LFilterBank(const size_t num_channels,
                           const IIRCoefficients & coeffs)
        : LFilterBank(num_channels, coeffs.numerator, coeffs.denominator)
      {}

      explicit LFilterBank(const size_t num_channels,
                           const BiquadSection & section)
        : LFilterBank(num_channels, section.numerator(), section.denominator())
      {}

      template <typename NumContainer, typename DenContainer>
        requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
      explicit LFilterBank(const size_t num_channels,
                           const NumContainer & numerator,
                           const DenContainer & denominator)
        : LFilterBank(num_channels,
                      FFT::build_real_input(numerator),
                      FFT::build_real_input(denominator))
      {}

      [[nodiscard]] size_t num_channels() const noexcept
      {
        return filters_.size();
      }

      [[nodiscard]] LFilter &
      channel(const size_t index)
      {
        ah_out_of_range_error_if(index >= filters_.size())
          << "FFT::LFilterBank::channel: index " << index
          << " out of range for " << filters_.size() << " channels";
        return filters_[index];
      }

      [[nodiscard]] const LFilter &
      channel(const size_t index) const
      {
        ah_out_of_range_error_if(index >= filters_.size())
          << "FFT::LFilterBank::channel: index " << index
          << " out of range for " << filters_.size() << " channels";
        return filters_[index];
      }

      void reset()
      {
        for (size_t i = 0; i < filters_.size(); ++i)
          filters_(i).reset();
      }

      [[nodiscard]] Array<Real>
      filter_channel(const size_t index,
                     const Array<Real> & signal)
      {
        return channel(index).filter(signal);
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      filter_channel(const size_t index,
                     const Container & signal)
      {
        return channel(index).filter(signal);
      }

      [[nodiscard]] Array<Array<Real>>
      filter(const Array<Array<Real>> & signals)
      {
        validate_channel_count(signals.size(), "FFT::LFilterBank::filter");
        Array<Array<Real>> output(filters_.size(), Array<Real>());
        for (size_t i = 0; i < filters_.size(); ++i)
          output(i) = filters_[i].filter(signals[i]);
        return output;
      }

      [[nodiscard]] Array<Array<Real>>
      pfilter(ThreadPool & pool,
              const Array<Array<Real>> & signals,
              const size_t chunk_size = 0)
      {
        validate_channel_count(signals.size(), "FFT::LFilterBank::pfilter");
        Array<Array<Real>> output(filters_.size(), Array<Real>());
        parallel_for_index(pool,
                           0,
                           filters_.size(),
                           [this, &signals, &output](const size_t i)
                             {
                               output(i) = filters_[i].filter(signals[i]);
                             },
                           chunk_size);
        return output;
      }
    };

    /** @brief Stateful bank of SOS cascades, one per channel. */
    class SOSFilterBank
    {
      Array<SOSFilter> filters_;

      /** @brief Rationale: Internal safety guard. */
      void
      validate_channel_count(const size_t count, const char *ctx) const
      {
        ah_invalid_argument_if(count != filters_.size())
          << ctx << ": batch size " << count
          << " does not match configured channel count " << filters_.size();
      }

    public:
      explicit SOSFilterBank(const size_t num_channels,
                             const Array<BiquadSection> & sections)
      {
        ah_invalid_argument_if(num_channels == 0)
          << "FFT::SOSFilterBank: at least one channel is required";
        filters_.reserve(num_channels);
        for (size_t i = 0; i < num_channels; ++i)
          filters_.append(SOSFilter(sections));
      }

      template <typename SectionsContainer>
        requires Is_Biquad_Container<SectionsContainer>
      explicit SOSFilterBank(const size_t num_channels,
                             const SectionsContainer & sections)
        : SOSFilterBank(num_channels, to_array<BiquadSection>(sections))
      {}

      [[nodiscard]] size_t num_channels() const noexcept
      {
        return filters_.size();
      }

      [[nodiscard]] SOSFilter &
      channel(const size_t index)
      {
        ah_out_of_range_error_if(index >= filters_.size())
          << "FFT::SOSFilterBank::channel: index " << index
          << " out of range for " << filters_.size() << " channels";
        return filters_[index];
      }

      [[nodiscard]] const SOSFilter &
      channel(const size_t index) const
      {
        ah_out_of_range_error_if(index >= filters_.size())
          << "FFT::SOSFilterBank::channel: index " << index
          << " out of range for " << filters_.size() << " channels";
        return filters_[index];
      }

      void reset()
      {
        for (size_t i = 0; i < filters_.size(); ++i)
          filters_(i).reset();
      }

      [[nodiscard]] Array<Real>
      filter_channel(const size_t index,
                     const Array<Real> & signal)
      {
        return channel(index).filter(signal);
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      filter_channel(const size_t index,
                     const Container & signal)
      {
        return channel(index).filter(signal);
      }

      [[nodiscard]] Array<Array<Real>>
      filter(const Array<Array<Real>> & signals)
      {
        validate_channel_count(signals.size(), "FFT::SOSFilterBank::filter");
        Array<Array<Real>> output(filters_.size(), Array<Real>());
        for (size_t i = 0; i < filters_.size(); ++i)
          output(i) = filters_[i].filter(signals[i]);
        return output;
      }

      [[nodiscard]] Array<Array<Real>>
      pfilter(ThreadPool & pool,
              const Array<Array<Real>> & signals,
              const size_t chunk_size = 0)
      {
        validate_channel_count(signals.size(), "FFT::SOSFilterBank::pfilter");
        Array<Array<Real>> output(filters_.size(), Array<Real>());
        parallel_for_index(pool,
                           0,
                           filters_.size(),
                           [this, &signals, &output](const size_t i)
                             {
                               output(i) = filters_[i].filter(signals[i]);
                             },
                           chunk_size);
        return output;
      }
    };

    /** @brief One-shot causal IIR filtering. */
    [[nodiscard]] static Array<Real>
    lfilter(const Array<Real> & signal,
            const Array<Real> & numerator,
            const Array<Real> & denominator,
            const Array<Real> & initial_state = {})
    {
      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, "FFT::lfilter");
      return iir_filter_impl(signal,
                             coeffs.numerator,
                             coeffs.denominator,
                             initial_state,
                             "FFT::lfilter");
    }

    [[nodiscard]] static Array<Real>
    lfilter(const Array<Real> & signal,
            const IIRCoefficients & coeffs,
            const Array<Real> & initial_state = {})
    {
      return lfilter(signal, coeffs.numerator, coeffs.denominator, initial_state);
    }

    [[nodiscard]] static Array<Real>
    lfilter(const Array<Real> & signal,
            const BiquadSection & section,
            const Array<Real> & initial_state = {})
    {
      return lfilter(signal, section.numerator(), section.denominator(), initial_state);
    }

    template <typename SignalContainer, typename NumContainer, typename DenContainer>
      requires Is_Real_Container<SignalContainer>
               and Is_Real_Container<NumContainer>
               and Is_Real_Container<DenContainer>
    [[nodiscard]] static Array<Real>
    lfilter(const SignalContainer & signal,
            const NumContainer & numerator,
            const DenContainer & denominator,
            const Array<Real> & initial_state = {})
    {
      return lfilter(build_real_input(signal),
                     build_real_input(numerator),
                     build_real_input(denominator),
                     initial_state);
    }

    /** @brief One-shot batched causal IIR filtering across real channels. */
    [[nodiscard]] static Array<Array<Real>>
    batched_lfilter(const Array<Array<Real>> & signals,
                    const Array<Real> & numerator,
                    const Array<Real> & denominator,
                    const Array<Array<Real>> & initial_states = {})
    {
      if (signals.is_empty())
        return {};

      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, "FFT::batched_lfilter");
      ah_invalid_argument_if(not initial_states.is_empty()
                             and initial_states.size() != signals.size())
        << "FFT::batched_lfilter: initial state count "
        << initial_states.size()
        << " does not match channel count " << signals.size();

      Array<Array<Real>> output(signals.size(), Array<Real>());
      for (size_t i = 0; i < signals.size(); ++i)
        output(i) = iir_filter_impl(signals[i],
                                    coeffs.numerator,
                                    coeffs.denominator,
                                    initial_states.is_empty() ? Array<Real>() : initial_states[i],
                                    "FFT::batched_lfilter");
      return output;
    }

    [[nodiscard]] static Array<Array<Real>>
    pbatched_lfilter(ThreadPool & pool,
                     const Array<Array<Real>> & signals,
                     const Array<Real> & numerator,
                     const Array<Real> & denominator,
                     const Array<Array<Real>> & initial_states = {},
                     const size_t chunk_size = 0)
    {
      if (signals.is_empty())
        return {};

      const IIRCoefficients coeffs =
          normalize_iir_coefficients(numerator, denominator, "FFT::pbatched_lfilter");
      ah_invalid_argument_if(not initial_states.is_empty()
                             and initial_states.size() != signals.size())
        << "FFT::pbatched_lfilter: initial state count "
        << initial_states.size()
        << " does not match channel count " << signals.size();

      Array<Array<Real>> output(signals.size(), Array<Real>());
      parallel_for_index(pool,
                         0,
                         signals.size(),
                         [&signals, &coeffs, &initial_states, &output](const size_t i)
                           {
                             output(i) = iir_filter_impl(signals[i],
                                                         coeffs.numerator,
                                                         coeffs.denominator,
                                                         initial_states.is_empty() ? Array<Real>() : initial_states[i],
                                                         "FFT::pbatched_lfilter");
                           },
                         chunk_size);
      return output;
    }

    template <typename SignalsContainer, typename NumContainer, typename DenContainer>
      requires Is_Real_Batch_Container<SignalsContainer>
               and Is_Real_Container<NumContainer>
               and Is_Real_Container<DenContainer>
    [[nodiscard]] static Array<Array<Real>>
    batched_lfilter(const SignalsContainer & signals,
                    const NumContainer & numerator,
                    const DenContainer & denominator,
                    const Array<Array<Real>> & initial_states = {})
    {
      Array<Array<Real>> batch;
      for (const auto & signal: signals)
        batch.append(build_real_input(signal));
      return batched_lfilter(batch,
                             build_real_input(numerator),
                             build_real_input(denominator),
                             initial_states);
    }

    /** @brief One-shot causal filtering of a cascade of second-order sections. */
    [[nodiscard]] static Array<Real>
    sosfilt(const Array<Real> & signal,
            const Array<BiquadSection> & sections)
    {
      return SOSFilter(sections).filter(signal);
    }

    template <typename SignalContainer, typename SectionsContainer>
      requires Is_Real_Container<SignalContainer>
               and Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<Real>
    sosfilt(const SignalContainer & signal,
            const SectionsContainer & sections)
    {
      return sosfilt(build_real_input(signal), to_array<BiquadSection>(sections));
    }

    /** @brief One-shot batched causal SOS filtering across real channels. */
    [[nodiscard]] static Array<Array<Real>>
    batched_sosfilt(const Array<Array<Real>> & signals,
                    const Array<BiquadSection> & sections)
    {
      if (signals.is_empty())
        return {};

      Array<Array<Real>> output(signals.size(), Array<Real>());
      for (size_t i = 0; i < signals.size(); ++i)
        output(i) = sosfilt(signals[i], sections);
      return output;
    }

    [[nodiscard]] static Array<Array<Real>>
    pbatched_sosfilt(ThreadPool & pool,
                     const Array<Array<Real>> & signals,
                     const Array<BiquadSection> & sections,
                     const size_t chunk_size = 0)
    {
      if (signals.is_empty())
        return {};

      Array<Array<Real>> output(signals.size(), Array<Real>());
      parallel_for_index(pool,
                         0,
                         signals.size(),
                         [&signals, &sections, &output](const size_t i)
                           {
                             output(i) = sosfilt(signals[i], sections);
                           },
                         chunk_size);
      return output;
    }

    template <typename SignalsContainer, typename SectionsContainer>
      requires Is_Real_Batch_Container<SignalsContainer>
               and Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<Array<Real>>
    batched_sosfilt(const SignalsContainer & signals,
                    const SectionsContainer & sections)
    {
      Array<Array<Real>> batch;
      for (const auto & signal: signals)
        batch.append(build_real_input(signal));
      return batched_sosfilt(batch, to_array<BiquadSection>(sections));
    }

    /** @brief Samples the discrete-time transfer response on the unit circle. */
    [[nodiscard]] static FrequencyResponse
    freqz(const Array<Real> & numerator,
          const Array<Real> & denominator,
          const size_t num_points = 512,
          const bool whole = false)
    {
      return freqz_impl(numerator, denominator, num_points, whole, "FFT::freqz");
    }

    /** @brief FIR frequency response. */
    [[nodiscard]] static FrequencyResponse
    freqz(const Array<Real> & numerator,
          const size_t num_points = 512,
          const bool whole = false)
    {
      return freqz(numerator, Array<Real>({Real(1)}), num_points, whole);
    }

    [[nodiscard]] static FrequencyResponse
    freqz(const IIRCoefficients & coeffs,
          const size_t num_points = 512,
          const bool whole = false)
    {
      return freqz(coeffs.numerator, coeffs.denominator, num_points, whole);
    }

    [[nodiscard]] static FrequencyResponse
    freqz(const BiquadSection & section,
          const size_t num_points = 512,
          const bool whole = false)
    {
      return freqz(section.numerator(), section.denominator(), num_points, whole);
    }

    [[nodiscard]] static FrequencyResponse
    freqz(const Array<BiquadSection> & sections,
          const size_t num_points = 512,
          const bool whole = false)
    {
      ah_invalid_argument_if(sections.is_empty())
        << "FFT::freqz: at least one biquad section is required";
      ah_invalid_argument_if(num_points == 0)
        << "FFT::freqz: number of frequency samples must be positive";

      FrequencyResponse output;
      output.omega = Array<Real>::create(num_points);
      output.response = Array<Complex>::create(num_points);

      const Real max_omega = whole ? Real(2) * std::numbers::pi_v<Real> : std::numbers::pi_v<Real>;
      const Real denom = whole ? static_cast<Real>(num_points) : static_cast<Real>(std::max(size_t(1), num_points - 1));

      for (size_t i = 0; i < num_points; ++i)
        {
          const Real omega = num_points == 1 ? Real(0) : max_omega * static_cast<Real>(i) / denom;
          Complex response(Real(1), Real(0));
          for (size_t j = 0; j < sections.size(); ++j)
            response *= evaluate_transfer_at(sections[j].numerator(),
                                             sections[j].denominator(),
                                             omega,
                                             "FFT::freqz(SOS)");
          output.omega(i) = omega;
          output.response(i) = response;
        }

      return output;
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static FrequencyResponse
    freqz(const NumContainer & numerator,
          const DenContainer & denominator,
          const size_t num_points = 512,
          const bool whole = false)
    {
      return freqz(build_real_input(numerator),
                   build_real_input(denominator),
                   num_points,
                   whole);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static FrequencyResponse
    freqz(const SectionsContainer & sections,
          const size_t num_points = 512,
          const bool whole = false)
    {
      return freqz(to_array<BiquadSection>(sections), num_points, whole);
    }

    /** @brief Returns the finite zeros of a transfer numerator. */
    [[nodiscard]] static Array<Complex>
    zeros(const Array<Real> & numerator)
    {
      const Array<Real> trimmed = trim_leading_zeros_copy(numerator);
      if (trimmed.is_empty())
        return {};
      const auto [deflated, extracted_roots] =
          extract_repeated_unit_roots(trimmed, "FFT::zeros");
      Array<Complex> roots =
          polynomial_roots_impl(deflated, "FFT::zeros");
      append_all(roots, extracted_roots);
      return roots;
    }

    /** @brief Returns the poles of a transfer denominator. */
    [[nodiscard]] static Array<Complex>
    poles(const Array<Real> & denominator)
    {
      const IIRCoefficients normalized =
          normalize_iir_coefficients(Array<Real>({Real(1)}),
                                     denominator,
                                     "FFT::poles");
      return polynomial_roots_impl(normalized.denominator, "FFT::poles");
    }

    [[nodiscard]] static Array<Complex>
    zeros(const IIRCoefficients & coeffs)
    {
      return zeros(coeffs.numerator);
    }

    [[nodiscard]] static Array<Complex>
    poles(const IIRCoefficients & coeffs)
    {
      return poles(coeffs.denominator);
    }

    [[nodiscard]] static Array<Complex>
    zeros(const BiquadSection & section)
    {
      return zeros(section.numerator());
    }

    [[nodiscard]] static Array<Complex>
    poles(const BiquadSection & section)
    {
      return poles(section.denominator());
    }

    [[nodiscard]] static Array<Complex>
    zeros(const Array<BiquadSection> & sections)
    {
      Array<Complex> output;
      for (size_t i = 0; i < sections.size(); ++i)
        append_all(output, zeros(sections[i]));
      return output;
    }

    [[nodiscard]] static Array<Complex>
    poles(const Array<BiquadSection> & sections)
    {
      Array<Complex> output;
      for (size_t i = 0; i < sections.size(); ++i)
        append_all(output, poles(sections[i]));
      return output;
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    zeros(const Container & numerator)
    {
      return zeros(build_real_input(numerator));
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    poles(const Container & denominator)
    {
      return poles(build_real_input(denominator));
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<Complex>
    zeros(const SectionsContainer & sections)
    {
      return zeros(to_array<BiquadSection>(sections));
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<Complex>
    poles(const SectionsContainer & sections)
    {
      return poles(to_array<BiquadSection>(sections));
    }

    /** @brief Greedily pairs zeros and poles by nearest distance. */
    [[nodiscard]] static Array<PoleZeroPair>
    pair_poles_and_zeros(const Array<Complex> & zeros,
                         const Array<Complex> & poles)
    {
      return pole_zero_pairs_impl(zeros, poles);
    }

    [[nodiscard]] static Array<PoleZeroPair>
    pair_poles_and_zeros(const Array<Real> & numerator,
                         const Array<Real> & denominator)
    {
      return pair_poles_and_zeros(zeros(numerator), poles(denominator));
    }

    [[nodiscard]] static Array<PoleZeroPair>
    pair_poles_and_zeros(const IIRCoefficients & coeffs)
    {
      return pair_poles_and_zeros(coeffs.numerator, coeffs.denominator);
    }

    [[nodiscard]] static Array<PoleZeroPair>
    pair_poles_and_zeros(const BiquadSection & section)
    {
      return pair_poles_and_zeros(section.numerator(), section.denominator());
    }

    [[nodiscard]] static Array<PoleZeroPair>
    pair_poles_and_zeros(const Array<BiquadSection> & sections)
    {
      return pair_poles_and_zeros(zeros(sections), poles(sections));
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static Array<PoleZeroPair>
    pair_poles_and_zeros(const NumContainer & numerator,
                         const DenContainer & denominator)
    {
      return pair_poles_and_zeros(build_real_input(numerator),
                                  build_real_input(denominator));
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<PoleZeroPair>
    pair_poles_and_zeros(const SectionsContainer & sections)
    {
      return pair_poles_and_zeros(to_array<BiquadSection>(sections));
    }

    /** @brief Returns the minimum pole/zero distance in a greedy pairing. */
    [[nodiscard]] static Real
    minimum_pole_zero_distance(const Array<Complex> & zeros,
                               const Array<Complex> & poles)
    {
      return minimum_cancellation_distance_impl(pair_poles_and_zeros(zeros, poles));
    }

    [[nodiscard]] static Real
    minimum_pole_zero_distance(const Array<Real> & numerator,
                               const Array<Real> & denominator)
    {
      return minimum_pole_zero_distance(zeros(numerator), poles(denominator));
    }

    [[nodiscard]] static Real
    minimum_pole_zero_distance(const IIRCoefficients & coeffs)
    {
      return minimum_pole_zero_distance(coeffs.numerator, coeffs.denominator);
    }

    [[nodiscard]] static Real
    minimum_pole_zero_distance(const BiquadSection & section)
    {
      return minimum_pole_zero_distance(section.numerator(), section.denominator());
    }

    [[nodiscard]] static Real
    minimum_pole_zero_distance(const Array<BiquadSection> & sections)
    {
      return minimum_pole_zero_distance(zeros(sections), poles(sections));
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static Real
    minimum_pole_zero_distance(const NumContainer & numerator,
                               const DenContainer & denominator)
    {
      return minimum_pole_zero_distance(build_real_input(numerator),
                                        build_real_input(denominator));
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Real
    minimum_pole_zero_distance(const SectionsContainer & sections)
    {
      return minimum_pole_zero_distance(to_array<BiquadSection>(sections));
    }

    /** @brief Signed stability margin relative to the unit circle. */
    [[nodiscard]] static Real
    stability_margin(const Array<Real> & denominator)
    {
      return Real(1) - max_root_radius(poles(denominator));
    }

    [[nodiscard]] static Real
    stability_margin(const IIRCoefficients & coeffs)
    {
      return stability_margin(coeffs.denominator);
    }

    [[nodiscard]] static Real
    stability_margin(const BiquadSection & section)
    {
      return stability_margin(section.denominator());
    }

    [[nodiscard]] static Real
    stability_margin(const Array<BiquadSection> & sections)
    {
      if (sections.is_empty())
        return Real(1);

      Real margin = std::numeric_limits<Real>::infinity();
      for (size_t i = 0; i < sections.size(); ++i)
        margin = std::min(margin, stability_margin(sections[i]));
      return margin;
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Real
    stability_margin(const Container & denominator)
    {
      return stability_margin(build_real_input(denominator));
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Real
    stability_margin(const SectionsContainer & sections)
    {
      return stability_margin(to_array<BiquadSection>(sections));
    }

    /** @brief Checks BIBO stability from denominator roots. */
    [[nodiscard]] static bool
    is_stable(const Array<Real> & denominator)
    {
      const Real tol = Real(1024) * std::numeric_limits<Real>::epsilon();
      return stability_margin(denominator) > tol;
    }

    [[nodiscard]] static bool
    is_stable(const IIRCoefficients & coeffs)
    {
      return is_stable(coeffs.denominator);
    }

    [[nodiscard]] static bool
    is_stable(const BiquadSection & section)
    {
      return is_stable(section.denominator());
    }

    [[nodiscard]] static bool
    is_stable(const Array<BiquadSection> & sections)
    {
      for (size_t i = 0; i < sections.size(); ++i)
        if (not is_stable(sections[i]))
          return false;
      return true;
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static bool
    is_stable(const Container & denominator)
    {
      return is_stable(build_real_input(denominator));
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static bool
    is_stable(const SectionsContainer & sections)
    {
      return is_stable(to_array<BiquadSection>(sections));
    }

    /** @brief Detects near pole/zero cancellations under a tolerance. */
    [[nodiscard]] static bool
    has_near_pole_zero_cancellation(const Array<Complex> & zeros,
                                    const Array<Complex> & poles,
                                    const Real tolerance)
    {
      const Array<PoleZeroPair> pairs = pair_poles_and_zeros(zeros, poles);
      for (size_t i = 0; i < pairs.size(); ++i)
        if (pairs[i].is_cancellation(tolerance))
          return true;
      return false;
    }

    [[nodiscard]] static bool
    has_near_pole_zero_cancellation(const Array<Real> & numerator,
                                    const Array<Real> & denominator,
                                    const Real tolerance)
    {
      return has_near_pole_zero_cancellation(zeros(numerator),
                                             poles(denominator),
                                             tolerance);
    }

    [[nodiscard]] static bool
    has_near_pole_zero_cancellation(const IIRCoefficients & coeffs,
                                    const Real tolerance)
    {
      return has_near_pole_zero_cancellation(coeffs.numerator,
                                             coeffs.denominator,
                                             tolerance);
    }

    [[nodiscard]] static bool
    has_near_pole_zero_cancellation(const BiquadSection & section,
                                    const Real tolerance)
    {
      return has_near_pole_zero_cancellation(section.numerator(),
                                             section.denominator(),
                                             tolerance);
    }

    [[nodiscard]] static bool
    has_near_pole_zero_cancellation(const Array<BiquadSection> & sections,
                                    const Real tolerance)
    {
      return has_near_pole_zero_cancellation(zeros(sections),
                                             poles(sections),
                                             tolerance);
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static bool
    has_near_pole_zero_cancellation(const NumContainer & numerator,
                                    const DenContainer & denominator,
                                    const Real tolerance)
    {
      return has_near_pole_zero_cancellation(build_real_input(numerator),
                                             build_real_input(denominator),
                                             tolerance);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static bool
    has_near_pole_zero_cancellation(const SectionsContainer & sections,
                                    const Real tolerance)
    {
      return has_near_pole_zero_cancellation(to_array<BiquadSection>(sections),
                                             tolerance);
    }

    static void
    validate_stable(const Array<Real> & denominator)
    {
      const Real tol = Real(1024) * std::numeric_limits<Real>::epsilon();
      const Array<Complex> roots = poles(denominator);
      const Real margin = stability_margin(denominator);
      ah_domain_error_if(margin <= tol)
        << "FFT::validate_stable: stability margin " << margin
        << " is not strictly positive";
      for (size_t i = 0; i < roots.size(); ++i)
        ah_domain_error_if(std::abs(roots[i]) >= Real(1) - tol)
          << "FFT::validate_stable: unstable pole at " << roots[i]
          << " with magnitude " << std::abs(roots[i]);
    }

    static void
    validate_stable(const IIRCoefficients & coeffs)
    {
      validate_stable(coeffs.denominator);
    }

    static void
    validate_stable(const BiquadSection & section)
    {
      validate_stable(section.denominator());
    }

    static void
    validate_stable(const Array<BiquadSection> & sections)
    {
      for (size_t i = 0; i < sections.size(); ++i)
        validate_stable(sections[i]);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    static void
    validate_stable(const Container & denominator)
    {
      validate_stable(build_real_input(denominator));
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    static void
    validate_stable(const SectionsContainer & sections)
    {
      validate_stable(to_array<BiquadSection>(sections));
    }

    static void
    validate_stable(const Array<Real> & denominator,
                    const Real min_margin)
    {
      const Real margin = stability_margin(denominator);
      ah_domain_error_if(margin < min_margin)
        << "FFT::validate_stable: stability margin " << margin
        << " is smaller than required margin " << min_margin;
      validate_stable(denominator);
    }

    static void
    validate_stable(const IIRCoefficients & coeffs,
                    const Real min_margin)
    {
      validate_stable(coeffs.denominator, min_margin);
    }

    static void
    validate_stable(const BiquadSection & section,
                    const Real min_margin)
    {
      validate_stable(section.denominator(), min_margin);
    }

    static void
    validate_stable(const Array<BiquadSection> & sections,
                    const Real min_margin)
    {
      const Real margin = stability_margin(sections);
      ah_domain_error_if(margin < min_margin)
        << "FFT::validate_stable: SOS stability margin "
        << margin
        << " is smaller than required margin " << min_margin;
      validate_stable(sections);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    static void
    validate_stable(const Container & denominator,
                    const Real min_margin)
    {
      validate_stable(build_real_input(denominator), min_margin);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    static void
    validate_stable(const SectionsContainer & sections,
                    const Real min_margin)
    {
      validate_stable(to_array<BiquadSection>(sections), min_margin);
    }

    static void
    validate_no_near_pole_zero_cancellation(const Array<Complex> & zeros,
                                            const Array<Complex> & poles,
                                            const Real tolerance)
    {
      const Array<PoleZeroPair> pairs = pair_poles_and_zeros(zeros, poles);
      for (size_t i = 0; i < pairs.size(); ++i)
        if (pairs[i].is_cancellation(tolerance))
          ah_domain_error_if(true)
            << "FFT::validate_no_near_pole_zero_cancellation: zero "
            << pairs[i].zero << " and pole " << pairs[i].pole
            << " are only " << pairs[i].distance << " apart";
    }

    static void
    validate_no_near_pole_zero_cancellation(const Array<Real> & numerator,
                                            const Array<Real> & denominator,
                                            const Real tolerance)
    {
      validate_no_near_pole_zero_cancellation(zeros(numerator),
                                              poles(denominator),
                                              tolerance);
    }

    static void
    validate_no_near_pole_zero_cancellation(const IIRCoefficients & coeffs,
                                            const Real tolerance)
    {
      validate_no_near_pole_zero_cancellation(coeffs.numerator,
                                              coeffs.denominator,
                                              tolerance);
    }

    static void
    validate_no_near_pole_zero_cancellation(const BiquadSection & section,
                                            const Real tolerance)
    {
      validate_no_near_pole_zero_cancellation(section.numerator(),
                                              section.denominator(),
                                              tolerance);
    }

    static void
    validate_no_near_pole_zero_cancellation(const Array<BiquadSection> & sections,
                                            const Real tolerance)
    {
      validate_no_near_pole_zero_cancellation(zeros(sections),
                                              poles(sections),
                                              tolerance);
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    static void
    validate_no_near_pole_zero_cancellation(const NumContainer & numerator,
                                            const DenContainer & denominator,
                                            const Real tolerance)
    {
      validate_no_near_pole_zero_cancellation(build_real_input(numerator),
                                              build_real_input(denominator),
                                              tolerance);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    static void
    validate_no_near_pole_zero_cancellation(const SectionsContainer & sections,
                                            const Real tolerance)
    {
      validate_no_near_pole_zero_cancellation(to_array<BiquadSection>(sections),
                                              tolerance);
    }

    /** @brief Estimates group delay from a sampled frequency response. */
    [[nodiscard]] static Array<Real>
    group_delay(const FrequencyResponse & response)
    {
      return group_delay_impl(response);
    }

    /** @brief Estimates phase delay from a sampled frequency response. */
    [[nodiscard]] static Array<Real>
    phase_delay(const FrequencyResponse & response)
    {
      return phase_delay_impl(response);
    }

    [[nodiscard]] static Array<Real>
    group_delay(const Array<Real> & numerator,
                const Array<Real> & denominator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return analytic_group_delay_impl(numerator,
                                       denominator,
                                       num_points,
                                       whole,
                                       "FFT::group_delay");
    }

    [[nodiscard]] static Array<Real>
    phase_delay(const Array<Real> & numerator,
                const Array<Real> & denominator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return analytic_phase_delay_impl(numerator,
                                       denominator,
                                       num_points,
                                       whole,
                                       "FFT::phase_delay");
    }

    [[nodiscard]] static Array<Real>
    group_delay(const Array<Real> & numerator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return analytic_group_delay_impl(numerator,
                                       Array<Real>({Real(1)}),
                                       num_points,
                                       whole,
                                       "FFT::group_delay");
    }

    [[nodiscard]] static Array<Real>
    phase_delay(const Array<Real> & numerator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return analytic_phase_delay_impl(numerator,
                                       Array<Real>({Real(1)}),
                                       num_points,
                                       whole,
                                       "FFT::phase_delay");
    }

    [[nodiscard]] static Array<Real>
    group_delay(const IIRCoefficients & coeffs,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return group_delay(coeffs.numerator, coeffs.denominator, num_points, whole);
    }

    [[nodiscard]] static Array<Real>
    phase_delay(const IIRCoefficients & coeffs,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return phase_delay(coeffs.numerator, coeffs.denominator, num_points, whole);
    }

    [[nodiscard]] static Array<Real>
    group_delay(const BiquadSection & section,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return group_delay(section.numerator(),
                         section.denominator(),
                         num_points,
                         whole);
    }

    [[nodiscard]] static Array<Real>
    phase_delay(const BiquadSection & section,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return phase_delay(section.numerator(),
                         section.denominator(),
                         num_points,
                         whole);
    }

    [[nodiscard]] static Array<Real>
    group_delay(const Array<BiquadSection> & sections,
                const size_t num_points = 512,
                const bool whole = false)
    {
      const FrequencyResponse response = freqz(sections, num_points, whole);
      return analytic_sos_group_delay_impl(sections,
                                           response,
                                           "FFT::group_delay(SOS)");
    }

    [[nodiscard]] static Array<Real>
    phase_delay(const Array<BiquadSection> & sections,
                const size_t num_points = 512,
                const bool whole = false)
    {
      const FrequencyResponse response = freqz(sections, num_points, whole);
      const Array<Real> phase = unwrap_phase(phase_spectrum(response.response));
      const Array<Real> group =
          analytic_sos_group_delay_impl(sections,
                                        response,
                                        "FFT::phase_delay(SOS)");
      Array<Real> delay = Array<Real>::create(response.omega.size());
      const Real tol = (response.omega[response.omega.size() - 1] + Real(1))
                       * Real(128) * std::numeric_limits<Real>::epsilon();

      for (size_t i = 0; i < response.omega.size(); ++i)
        delay(i) = std::abs(response.omega[i]) <= tol ? group[i] : -phase[i] / response.omega[i];

      return delay;
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static Array<Real>
    group_delay(const NumContainer & numerator,
                const DenContainer & denominator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return group_delay(build_real_input(numerator),
                         build_real_input(denominator),
                         num_points,
                         whole);
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static Array<Real>
    phase_delay(const NumContainer & numerator,
                const DenContainer & denominator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return phase_delay(build_real_input(numerator),
                         build_real_input(denominator),
                         num_points,
                         whole);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Real>
    group_delay(const Container & numerator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return group_delay(build_real_input(numerator), num_points, whole);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Real>
    phase_delay(const Container & numerator,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return phase_delay(build_real_input(numerator), num_points, whole);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<Real>
    group_delay(const SectionsContainer & sections,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return group_delay(to_array<BiquadSection>(sections), num_points, whole);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<Real>
    phase_delay(const SectionsContainer & sections,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return phase_delay(to_array<BiquadSection>(sections), num_points, whole);
    }

    /** @brief Estimates phase margin around the unity-gain crossover. */
    [[nodiscard]] static PhaseMarginInfo
    phase_margin(const FrequencyResponse & response)
    {
      return phase_margin_impl(response);
    }

    /** @brief Estimates gain margin around the -pi phase crossover. */
    [[nodiscard]] static GainMarginInfo
    gain_margin(const FrequencyResponse & response)
    {
      return gain_margin_impl(response);
    }

    [[nodiscard]] static PhaseMarginInfo
    phase_margin(const Array<Real> & numerator,
                 const Array<Real> & denominator,
                 const size_t num_points = 1024,
                 const bool whole = false)
    {
      const size_t seed_points = std::max(num_points, size_t(4097));
      const FrequencyResponse response = freqz(numerator,
                                               denominator,
                                               seed_points,
                                               whole);
      return phase_margin_refined_impl(response,
                                       [&](const Real omega)
                                         {
                                           return evaluate_transfer_at(numerator,
                                                                       denominator,
                                                                       omega,
                                                                       "FFT::phase_margin");
                                         });
    }

    [[nodiscard]] static GainMarginInfo
    gain_margin(const Array<Real> & numerator,
                const Array<Real> & denominator,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      const size_t seed_points = std::max(num_points, size_t(4097));
      const FrequencyResponse response = freqz(numerator,
                                               denominator,
                                               seed_points,
                                               whole);
      return gain_margin_refined_impl(response,
                                      [&](const Real omega)
                                        {
                                          return evaluate_transfer_at(numerator,
                                                                      denominator,
                                                                      omega,
                                                                      "FFT::gain_margin");
                                        });
    }

    [[nodiscard]] static PhaseMarginInfo
    phase_margin(const IIRCoefficients & coeffs,
                 const size_t num_points = 1024,
                 const bool whole = false)
    {
      return phase_margin(coeffs.numerator, coeffs.denominator, num_points, whole);
    }

    [[nodiscard]] static GainMarginInfo
    gain_margin(const IIRCoefficients & coeffs,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      return gain_margin(coeffs.numerator, coeffs.denominator, num_points, whole);
    }

    [[nodiscard]] static PhaseMarginInfo
    phase_margin(const BiquadSection & section,
                 const size_t num_points = 1024,
                 const bool whole = false)
    {
      return phase_margin(section.numerator(),
                          section.denominator(),
                          num_points,
                          whole);
    }

    [[nodiscard]] static GainMarginInfo
    gain_margin(const BiquadSection & section,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      return gain_margin(section.numerator(),
                         section.denominator(),
                         num_points,
                         whole);
    }

    [[nodiscard]] static PhaseMarginInfo
    phase_margin(const Array<BiquadSection> & sections,
                 const size_t num_points = 1024,
                 const bool whole = false)
    {
      const size_t seed_points = std::max(num_points, size_t(4097));
      const FrequencyResponse response = freqz(sections, seed_points, whole);
      return phase_margin_refined_impl(response,
                                       [&](const Real omega)
                                         {
                                           return evaluate_sos_transfer_at(sections,
                                                                           omega,
                                                                           "FFT::phase_margin(SOS)");
                                         });
    }

    [[nodiscard]] static GainMarginInfo
    gain_margin(const Array<BiquadSection> & sections,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      const size_t seed_points = std::max(num_points, size_t(4097));
      const FrequencyResponse response = freqz(sections, seed_points, whole);
      return gain_margin_refined_impl(response,
                                      [&](const Real omega)
                                        {
                                          return evaluate_sos_transfer_at(sections,
                                                                          omega,
                                                                          "FFT::gain_margin(SOS)");
                                        });
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static PhaseMarginInfo
    phase_margin(const NumContainer & numerator,
                 const DenContainer & denominator,
                 const size_t num_points = 1024,
                 const bool whole = false)
    {
      return phase_margin(build_real_input(numerator),
                          build_real_input(denominator),
                          num_points,
                          whole);
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static GainMarginInfo
    gain_margin(const NumContainer & numerator,
                const DenContainer & denominator,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      return gain_margin(build_real_input(numerator),
                         build_real_input(denominator),
                         num_points,
                         whole);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static PhaseMarginInfo
    phase_margin(const SectionsContainer & sections,
                 const size_t num_points = 1024,
                 const bool whole = false)
    {
      return phase_margin(to_array<BiquadSection>(sections), num_points, whole);
    }

    template <typename SectionsContainer>
      requires Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static GainMarginInfo
    gain_margin(const SectionsContainer & sections,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      return gain_margin(to_array<BiquadSection>(sections), num_points, whole);
    }

    /** @brief Bilinear transform of an analog transfer function. */
    [[nodiscard]] static IIRCoefficients
    bilinear_transform(const Array<Real> & analog_numerator,
                       const Array<Real> & analog_denominator,
                       const Real sample_rate)
    {
      return bilinear_transform_impl(analog_numerator,
                                     analog_denominator,
                                     sample_rate,
                                     "FFT::bilinear_transform");
    }

    template <typename NumContainer, typename DenContainer>
      requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
    [[nodiscard]] static IIRCoefficients
    bilinear_transform(const NumContainer & analog_numerator,
                       const DenContainer & analog_denominator,
                       const Real sample_rate)
    {
      return bilinear_transform(build_real_input(analog_numerator),
                                build_real_input(analog_denominator),
                                sample_rate);
    }

    /** @brief Digital Butterworth low-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    butterworth_lowpass(const size_t order,
                        const Real cutoff_frequency,
                        const Real sample_rate)
    {
      return design_prototype_sections(butterworth_prototype(order,
                                                             "FFT::butterworth_lowpass"),
                                       order,
                                       cutoff_frequency,
                                       sample_rate,
                                       false,
                                       "FFT::butterworth_lowpass");
    }

    /** @brief Digital Butterworth high-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    butterworth_highpass(const size_t order,
                         const Real cutoff_frequency,
                         const Real sample_rate)
    {
      return design_prototype_sections(butterworth_prototype(order,
                                                             "FFT::butterworth_highpass"),
                                       order,
                                       cutoff_frequency,
                                       sample_rate,
                                       true,
                                       "FFT::butterworth_highpass");
    }

    /** @brief Digital Butterworth band-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    butterworth_bandpass(const size_t order,
                         const Real low_cutoff_frequency,
                         const Real high_cutoff_frequency,
                         const Real sample_rate)
    {
      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             "FFT::butterworth_bandpass");
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      return design_transformed_sections(butterworth_prototype(order,
                                                               "FFT::butterworth_bandpass"),
                                         {Real(1), Real(0), center * center},
                                         {bandwidth, Real(0)},
                                         sample_rate,
                                         "FFT::butterworth_bandpass");
    }

    /** @brief Digital Butterworth band-stop design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    butterworth_bandstop(const size_t order,
                         const Real low_cutoff_frequency,
                         const Real high_cutoff_frequency,
                         const Real sample_rate)
    {
      return design_bandstop_sections_without_numerator_roots(butterworth_prototype(order, "FFT::butterworth_bandstop"),
                                                              order,
                                                              low_cutoff_frequency,
                                                              high_cutoff_frequency,
                                                              sample_rate,
                                                              "FFT::butterworth_bandstop");
    }

    /** @brief Digital Chebyshev-I low-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev1_lowpass(const size_t order,
                       const Real ripple_db,
                       const Real cutoff_frequency,
                       const Real sample_rate)
    {
      return design_prototype_sections(chebyshev1_prototype(order,
                                                            ripple_db,
                                                            "FFT::chebyshev1_lowpass"),
                                       order,
                                       cutoff_frequency,
                                       sample_rate,
                                       false,
                                       "FFT::chebyshev1_lowpass");
    }

    /** @brief Digital Chebyshev-I high-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev1_highpass(const size_t order,
                        const Real ripple_db,
                        const Real cutoff_frequency,
                        const Real sample_rate)
    {
      return design_prototype_sections(chebyshev1_prototype(order,
                                                            ripple_db,
                                                            "FFT::chebyshev1_highpass"),
                                       order,
                                       cutoff_frequency,
                                       sample_rate,
                                       true,
                                       "FFT::chebyshev1_highpass");
    }

    /** @brief Digital Chebyshev-I band-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev1_bandpass(const size_t order,
                        const Real ripple_db,
                        const Real low_cutoff_frequency,
                        const Real high_cutoff_frequency,
                        const Real sample_rate)
    {
      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             "FFT::chebyshev1_bandpass");
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      return design_transformed_sections(chebyshev1_prototype(order,
                                                              ripple_db,
                                                              "FFT::chebyshev1_bandpass"),
                                         {Real(1), Real(0), center * center},
                                         {bandwidth, Real(0)},
                                         sample_rate,
                                         "FFT::chebyshev1_bandpass");
    }

    /** @brief Digital Chebyshev-I band-stop design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev1_bandstop(const size_t order,
                        const Real ripple_db,
                        const Real low_cutoff_frequency,
                        const Real high_cutoff_frequency,
                        const Real sample_rate)
    {
      return design_bandstop_sections_without_numerator_roots(
                                                              chebyshev1_prototype(order,
                                                                ripple_db,
                                                                "FFT::chebyshev1_bandstop"),
                                                              order,
                                                              low_cutoff_frequency,
                                                              high_cutoff_frequency,
                                                              sample_rate,
                                                              "FFT::chebyshev1_bandstop");
    }

    /** @brief Digital Chebyshev-II low-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev2_lowpass(const size_t order,
                       const Real attenuation_db,
                       const Real cutoff_frequency,
                       const Real sample_rate)
    {
      return design_low_high_sections(chebyshev2_prototype(order,
                                                           attenuation_db,
                                                           "FFT::chebyshev2_lowpass"),
                                      cutoff_frequency,
                                      sample_rate,
                                      false,
                                      "FFT::chebyshev2_lowpass");
    }

    /** @brief Digital Chebyshev-II high-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev2_highpass(const size_t order,
                        const Real attenuation_db,
                        const Real cutoff_frequency,
                        const Real sample_rate)
    {
      return design_low_high_sections(chebyshev2_prototype(order,
                                                           attenuation_db,
                                                           "FFT::chebyshev2_highpass"),
                                      cutoff_frequency,
                                      sample_rate,
                                      true,
                                      "FFT::chebyshev2_highpass");
    }

    /** @brief Digital Chebyshev-II band-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev2_bandpass(const size_t order,
                        const Real attenuation_db,
                        const Real low_cutoff_frequency,
                        const Real high_cutoff_frequency,
                        const Real sample_rate)
    {
      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             "FFT::chebyshev2_bandpass");
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      return design_transformed_sections(chebyshev2_prototype(order,
                                                              attenuation_db,
                                                              "FFT::chebyshev2_bandpass"),
                                         {Real(1), Real(0), center * center},
                                         {bandwidth, Real(0)},
                                         sample_rate,
                                         "FFT::chebyshev2_bandpass");
    }

    /** @brief Digital Chebyshev-II band-stop design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    chebyshev2_bandstop(const size_t order,
                        const Real attenuation_db,
                        const Real low_cutoff_frequency,
                        const Real high_cutoff_frequency,
                        const Real sample_rate)
    {
      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             "FFT::chebyshev2_bandstop");
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      return design_transformed_sections(chebyshev2_prototype(order,
                                                              attenuation_db,
                                                              "FFT::chebyshev2_bandstop"),
                                         {bandwidth, Real(0)},
                                         {Real(1), Real(0), center * center},
                                         sample_rate,
                                         "FFT::chebyshev2_bandstop");
    }

    /** @brief Digital Bessel low-pass design returned as SOS.
     *
     *  The analog prototype is normalized so that the digital cutoff matches
     *  the -3 dB point after bilinear mapping.
     */
    [[nodiscard]] static Array<BiquadSection>
    bessel_lowpass(const size_t order,
                   const Real cutoff_frequency,
                   const Real sample_rate)
    {
      return design_low_high_sections(bessel_prototype(order,
                                                       "FFT::bessel_lowpass"),
                                      cutoff_frequency,
                                      sample_rate,
                                      false,
                                      "FFT::bessel_lowpass");
    }

    /** @brief Digital Bessel high-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    bessel_highpass(const size_t order,
                    const Real cutoff_frequency,
                    const Real sample_rate)
    {
      return design_low_high_sections(bessel_prototype(order,
                                                       "FFT::bessel_highpass"),
                                      cutoff_frequency,
                                      sample_rate,
                                      true,
                                      "FFT::bessel_highpass");
    }

    /** @brief Digital Bessel band-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    bessel_bandpass(const size_t order,
                    const Real low_cutoff_frequency,
                    const Real high_cutoff_frequency,
                    const Real sample_rate)
    {
      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             "FFT::bessel_bandpass");
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      return design_transformed_sections(bessel_prototype(order,
                                                          "FFT::bessel_bandpass"),
                                         {Real(1), Real(0), center * center},
                                         {bandwidth, Real(0)},
                                         sample_rate,
                                         "FFT::bessel_bandpass");
    }

    /** @brief Digital Bessel band-stop design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    bessel_bandstop(const size_t order,
                    const Real low_cutoff_frequency,
                    const Real high_cutoff_frequency,
                    const Real sample_rate)
    {
      return design_bandstop_sections_without_numerator_roots(bessel_prototype(order, "FFT::bessel_bandstop"),
                                                              order,
                                                              low_cutoff_frequency,
                                                              high_cutoff_frequency,
                                                              sample_rate,
                                                              "FFT::bessel_bandstop");
    }

    /** @brief Digital elliptic/Cauer low-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    elliptic_lowpass(const size_t order,
                     const Real ripple_db,
                     const Real attenuation_db,
                     const Real cutoff_frequency,
                     const Real sample_rate)
    {
      return design_low_high_sections(elliptic_prototype(order,
                                                         ripple_db,
                                                         attenuation_db,
                                                         "FFT::elliptic_lowpass"),
                                      cutoff_frequency,
                                      sample_rate,
                                      false,
                                      "FFT::elliptic_lowpass");
    }

    /** @brief Digital elliptic/Cauer high-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    elliptic_highpass(const size_t order,
                      const Real ripple_db,
                      const Real attenuation_db,
                      const Real cutoff_frequency,
                      const Real sample_rate)
    {
      return design_low_high_sections(elliptic_prototype(order,
                                                         ripple_db,
                                                         attenuation_db,
                                                         "FFT::elliptic_highpass"),
                                      cutoff_frequency,
                                      sample_rate,
                                      true,
                                      "FFT::elliptic_highpass");
    }

    /** @brief Digital elliptic/Cauer band-pass design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    elliptic_bandpass(const size_t order,
                      const Real ripple_db,
                      const Real attenuation_db,
                      const Real low_cutoff_frequency,
                      const Real high_cutoff_frequency,
                      const Real sample_rate)
    {
      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             "FFT::elliptic_bandpass");
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      return design_transformed_sections(elliptic_prototype(order,
                                                            ripple_db,
                                                            attenuation_db,
                                                            "FFT::elliptic_bandpass"),
                                         {Real(1), Real(0), center * center},
                                         {bandwidth, Real(0)},
                                         sample_rate,
                                         "FFT::elliptic_bandpass");
    }

    /** @brief Digital elliptic/Cauer band-stop design returned as SOS. */
    [[nodiscard]] static Array<BiquadSection>
    elliptic_bandstop(const size_t order,
                      const Real ripple_db,
                      const Real attenuation_db,
                      const Real low_cutoff_frequency,
                      const Real high_cutoff_frequency,
                      const Real sample_rate)
    {
      const auto [warped_low, warped_high] =
          prewarp_band_edges(low_cutoff_frequency,
                             high_cutoff_frequency,
                             sample_rate,
                             "FFT::elliptic_bandstop");
      const Real bandwidth = warped_high - warped_low;
      const Real center = std::sqrt(warped_low * warped_high);
      return design_transformed_sections(elliptic_prototype(order,
                                                            ripple_db,
                                                            attenuation_db,
                                                            "FFT::elliptic_bandstop"),
                                         {bandwidth, Real(0)},
                                         {Real(1), Real(0), center * center},
                                         sample_rate,
                                         "FFT::elliptic_bandstop");
    }

    /** @brief Zero-phase FIR filtering via forward-backward convolution.
     *
     *  This implementation uses reflect padding, a forward causal FIR pass,
     *  time reversal, and a second FIR pass. The output length matches the
     *  input length.
     */
    [[nodiscard]] static Array<Real>
    filtfilt(const Array<Real> & signal,
             const Array<Real> & coeffs,
             const size_t block_size = 0)
    {
      if (signal.is_empty() or coeffs.is_empty())
        return {};

      if (coeffs.size() == 1)
        {
          Array<Real> output = Array<Real>::create(signal.size());
          const Real gain = coeffs[0] * coeffs[0];
          for (size_t i = 0; i < signal.size(); ++i)
            output(i) = signal[i] * gain;
          return output;
        }

      const size_t pad_len = default_filtfilt_pad_length(signal.size(), coeffs.size());
      const Array<Real> padded = reflect_pad_signal(signal, pad_len);
      const OverlapAdd convolver(coeffs, block_size);

      const Array<Real> forward = prefix_copy(convolver.convolve(padded), padded.size());
      const Array<Real> reversed = reverse_copy(forward);
      const Array<Real> backward = prefix_copy(convolver.convolve(reversed), padded.size());
      const Array<Real> zero_phase = reverse_copy(backward);

      return slice_copy(zero_phase, pad_len, signal.size());
    }

    /** @brief Parallel zero-phase FIR filtering via forward-backward convolution. */
    [[nodiscard]] static Array<Real>
    pfiltfilt(ThreadPool & pool,
              const Array<Real> & signal,
              const Array<Real> & coeffs,
              const size_t block_size = 0,
              const size_t chunk_size = 0)
    {
      if (signal.is_empty() or coeffs.is_empty())
        return {};

      if (coeffs.size() == 1)
        {
          Array<Real> output = Array<Real>::create(signal.size());
          const Real gain = coeffs[0] * coeffs[0];
          for (size_t i = 0; i < signal.size(); ++i)
            output(i) = signal[i] * gain;
          return output;
        }

      const size_t pad_len = default_filtfilt_pad_length(signal.size(), coeffs.size());
      const Array<Real> padded = reflect_pad_signal(signal, pad_len);
      const OverlapAdd convolver(coeffs, block_size);

      const Array<Real> forward = prefix_copy(convolver.pconvolve(pool, padded, chunk_size),
                                              padded.size());
      const Array<Real> reversed = reverse_copy(forward);
      const Array<Real> backward = prefix_copy(convolver.pconvolve(pool, reversed, chunk_size),
                                               padded.size());
      const Array<Real> zero_phase = reverse_copy(backward);

      return slice_copy(zero_phase, pad_len, signal.size());
    }

    template <typename SignalContainer, typename CoeffContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<CoeffContainer>
    [[nodiscard]] static Array<Real>
    filtfilt(const SignalContainer & signal,
             const CoeffContainer & coeffs,
             const size_t block_size = 0)
    {
      return filtfilt(build_real_input(signal), build_real_input(coeffs), block_size);
    }

    template <typename SignalContainer, typename CoeffContainer>
      requires Is_Real_Container<SignalContainer> and Is_Real_Container<CoeffContainer>
    [[nodiscard]] static Array<Real>
    pfiltfilt(ThreadPool & pool,
              const SignalContainer & signal,
              const CoeffContainer & coeffs,
              const size_t block_size = 0,
              const size_t chunk_size = 0)
    {
      return pfiltfilt(pool,
                       build_real_input(signal),
                       build_real_input(coeffs),
                       block_size,
                       chunk_size);
    }

    /** @brief Zero-phase IIR filtering using transfer-function coefficients.
     *
     *  The denominator is interpreted in the standard DSP form
     *  `a[0] y[n] + a[1] y[n-1] + ... = b[0] x[n] + ...`.
     *  Coefficients are normalized by `a[0]` internally and the recursion uses
     *  transposed direct-form II with steady-state endpoint initialization.
     */
    [[nodiscard]] static Array<Real>
    filtfilt(const Array<Real> & signal,
             const Array<Real> & numerator,
             const Array<Real> & denominator)
    {
      return iir_filtfilt_impl(signal,
                               numerator,
                               denominator,
                               "FFT::filtfilt(IIR)");
    }

    /** @brief Zero-phase IIR filtering with explicit coefficient groups. */
    [[nodiscard]] static Array<Real>
    filtfilt(const Array<Real> & signal,
             const IIRCoefficients & coeffs)
    {
      return filtfilt(signal, coeffs.numerator, coeffs.denominator);
    }

    /** @brief Zero-phase filtering of one biquad section. */
    [[nodiscard]] static Array<Real>
    filtfilt(const Array<Real> & signal,
             const BiquadSection & section)
    {
      return filtfilt(signal, section.numerator(), section.denominator());
    }

    /** @brief Zero-phase filtering of a cascade of biquad sections. */
    [[nodiscard]] static Array<Real>
    filtfilt(const Array<Real> & signal,
             const Array<BiquadSection> & sections)
    {
      return sos_filtfilt_impl(signal, sections, "FFT::filtfilt(SOS)");
    }

    template <typename SignalContainer, typename NumContainer, typename DenContainer>
      requires Is_Real_Container<SignalContainer>
               and Is_Real_Container<NumContainer>
               and Is_Real_Container<DenContainer>
    [[nodiscard]] static Array<Real>
    filtfilt(const SignalContainer & signal,
             const NumContainer & numerator,
             const DenContainer & denominator)
    {
      return filtfilt(build_real_input(signal),
                      build_real_input(numerator),
                      build_real_input(denominator));
    }

    template <typename SignalContainer>
      requires Is_Real_Container<SignalContainer>
    [[nodiscard]] static Array<Real>
    filtfilt(const SignalContainer & signal,
             const IIRCoefficients & coeffs)
    {
      return filtfilt(build_real_input(signal), coeffs);
    }

    template <typename SignalContainer>
      requires Is_Real_Container<SignalContainer>
    [[nodiscard]] static Array<Real>
    filtfilt(const SignalContainer & signal,
             const BiquadSection & section)
    {
      return filtfilt(build_real_input(signal), section);
    }

    template <typename SignalContainer, typename SectionsContainer>
      requires Is_Real_Container<SignalContainer>
               and Is_Biquad_Container<SectionsContainer>
    [[nodiscard]] static Array<Real>
    filtfilt(const SignalContainer & signal,
             const SectionsContainer & sections)
    {
      return filtfilt(build_real_input(signal),
                      to_array<BiquadSection>(sections));
    }

    /** @brief Reusable overlap-add convolver for long real sequences.
     *
     *  The kernel spectrum is precomputed once. Input signals are processed in
     *  blocks of size @ref block_size(), each block transformed with an FFT of
     *  size @ref fft_size() and accumulated in the output via overlap-add.
     */
    class OverlapAdd
    {
      Array<Real> kernel_;
      size_t block_size_ = 0;
      size_t fft_size_ = 0;
      Plan plan_;
      Array<Complex> kernel_spectrum_;
      Array<Real> overlap_;
      bool has_pending_tail_ = false;

      [[nodiscard]] static size_t
      default_block_size(const size_t kernel_size)
      {
        ah_invalid_argument_if(kernel_size == 0)
          << "FFT::OverlapAdd: kernel size must be positive";
        return next_power_of_two(std::max(kernel_size, size_t(1)));
      }

      /** @brief Rationale: Prepares a chunk of signal for FFT by zero-padding it
       *         to the internal @ref fft_size().
       */
      [[nodiscard]] Array<Complex>
      build_signal_block(const Array<Real> & signal,
                         const size_t offset,
                         const size_t length) const
      {
        Array<Complex> block = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          block(i) = Complex(Real(0), Real(0));

        for (size_t i = 0; i < length; ++i)
          block(i) = Complex(signal[offset + i], Real(0));

        return block;
      }

      /** @brief Rationale: Applies the precomputed FIR kernel in the frequency
       *         domain via element-wise multiplication.
       */
      void
      pointwise_multiply(Array<Complex> & spectrum,
                         ThreadPool *pool,
                         const size_t chunk_size) const
      {
        auto pointwise = [this, &spectrum](const size_t i)
          {
            spectrum(i) *= kernel_spectrum_[i];
          };

        if (pool != nullptr and pool->num_threads() > 1 and fft_size_ > 1)
          parallel_for_index(*pool, 0, fft_size_, pointwise, chunk_size);
        else
          for (size_t i = 0; i < fft_size_; ++i)
            pointwise(i);
      }

      /** @brief Rationale: Resets the overlap buffer between independent convolution runs. */
      void
      clear_overlap()
      {
        for (size_t i = 0; i < overlap_.size(); ++i)
          overlap_(i) = Real(0);
        has_pending_tail_ = false;
      }

      /** @brief Rationale: Implements the core Overlap-Add logic for a single block:
       *         1. FFT of current signal block.
       *         2. Pointwise multiplication with kernel spectrum.
       *         3. Inverse FFT to time domain.
       *         4. Add previous overlap and emit result.
       *         5. Update overlap buffer with the new tail.
       */
      [[nodiscard]] Array<Real>
      process_chunk_impl(const Array<Real> & block,
                         ThreadPool *pool,
                         const size_t chunk_size)
      {
        if (block.is_empty())
          return {};

        ah_invalid_argument_if(block.size() > block_size_)
          << "FFT::OverlapAdd::process_block: block size " << block.size()
          << " exceeds configured block size " << block_size_;

        Array<Complex> spectrum = build_signal_block(block, 0, block.size());

        if (pool != nullptr)
          plan_.ptransform(*pool, spectrum, false, chunk_size);
        else
          plan_.transform(spectrum, false);

        pointwise_multiply(spectrum, pool, chunk_size);

        const Array<Real> convolved = (pool != nullptr) ?
                                        plan_.pinverse_transform_real(*pool, spectrum, chunk_size) :
                                        plan_.inverse_transform_real(spectrum);

        Array<Real> emitted = Array<Real>::create(block.size());
        for (size_t i = 0; i < block.size(); ++i)
          {
            const Real overlap = i < overlap_.size() ? overlap_[i] : Real(0);
            emitted(i) = convolved[i] + overlap;
          }

        if (not overlap_.is_empty())
          {
            Array<Real> next_overlap = Array<Real>::create(overlap_.size());
            for (size_t i = 0; i < overlap_.size(); ++i)
              {
                const size_t src = block.size() + i;
                const Real from_current = src < convolved.size() ? convolved[src] : Real(0);
                const Real from_previous = src < overlap_.size() ? overlap_[src] : Real(0);
                next_overlap(i) = from_current + from_previous;
              }
            overlap_ = next_overlap;
          }

        has_pending_tail_ = true;
        return emitted;
      }

      /** @brief implementation of one-shot linear convolution. */
      [[nodiscard]] Array<Real>
      convolve_impl(const Array<Real> & signal,
                    ThreadPool *pool,
                    const size_t chunk_size) const
      {
        if (signal.is_empty())
          return {};

        ah_invalid_argument_if(signal.size()
                               > std::numeric_limits<size_t>::max()
                               - kernel_.size() + 1)
          << "FFT::OverlapAdd::convolve: product size exceeds size_t capacity";

        Array<Real> output = Array<Real>::create(signal.size() + kernel_.size() - 1);
        for (size_t i = 0; i < output.size(); ++i)
          output(i) = Real(0);

        for (size_t offset = 0; offset < signal.size(); offset += block_size_)
          {
            const size_t length = std::min(block_size_, signal.size() - offset);
            Array<Complex> spectrum = build_signal_block(signal, offset, length);

            if (pool != nullptr)
              plan_.ptransform(*pool, spectrum, false, chunk_size);
            else
              plan_.transform(spectrum, false);

            pointwise_multiply(spectrum, pool, chunk_size);

            const Array<Real> block = (pool != nullptr) ?
                                        plan_.pinverse_transform_real(*pool, spectrum, chunk_size) :
                                        plan_.inverse_transform_real(spectrum);

            const size_t valid = length + kernel_.size() - 1;
            for (size_t i = 0; i < valid; ++i)
              output(offset + i) += block[i];
          }

        return output;
      }

    public:
      explicit OverlapAdd(const Array<Real> & kernel,
                          const size_t block_size = 0)
        : kernel_(kernel)
      {
        ah_invalid_argument_if(kernel_.is_empty())
          << "FFT::OverlapAdd: kernel must be non-empty";

        block_size_ = block_size == 0 ? default_block_size(kernel_.size()) : block_size;
        ah_invalid_argument_if(block_size_ == 0)
          << "FFT::OverlapAdd: block size must be positive";

        ah_invalid_argument_if(block_size_
                               > std::numeric_limits<size_t>::max()
                               - kernel_.size() + 1)
          << "FFT::OverlapAdd: FFT size overflow";

        fft_size_ = next_power_of_two(block_size_ + kernel_.size() - 1);
        plan_ = Plan(fft_size_);
        overlap_ = Array<Real>::create(kernel_.size() - 1);
        clear_overlap();

        Array<Complex> padded_kernel = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          padded_kernel(i) = Complex(Real(0), Real(0));
        for (size_t i = 0; i < kernel_.size(); ++i)
          padded_kernel(i) = Complex(kernel_[i], Real(0));

        kernel_spectrum_ = padded_kernel;
        plan_.transform(kernel_spectrum_, false);
      }

      [[nodiscard]] size_t block_size() const noexcept { return block_size_; }
      [[nodiscard]] size_t fft_size() const noexcept { return fft_size_; }
      [[nodiscard]] const Array<Real> &kernel() const noexcept { return kernel_; }
      [[nodiscard]] size_t overlap_size() const noexcept { return overlap_.size(); }

      void reset()
      {
        clear_overlap();
      }

      [[nodiscard]] Array<Real>
      convolve(const Array<Real> & signal) const
      {
        return convolve_impl(signal, nullptr, 0);
      }

      [[nodiscard]] Array<Real>
      pconvolve(ThreadPool & pool, const Array<Real> & signal,
                const size_t chunk_size = 0) const
      {
        return convolve_impl(signal, &pool, chunk_size);
      }

      [[nodiscard]] Array<Real>
      process_block(const Array<Real> & block)
      {
        if (block.is_empty())
          return {};

        Array<Real> emitted;
        emitted.reserve(block.size());
        for (size_t offset = 0; offset < block.size(); offset += block_size_)
          {
            const size_t length = std::min(block_size_, block.size() - offset);
            Array<Real> chunk;
            chunk.reserve(length);
            for (size_t i = 0; i < length; ++i)
              chunk.append(block[offset + i]);

            const Array<Real> partial = process_chunk_impl(chunk, nullptr, 0);
            for (size_t i = 0; i < partial.size(); ++i)
              emitted.append(partial[i]);
          }

        return emitted;
      }

      [[nodiscard]] Array<Real>
      pprocess_block(ThreadPool & pool, const Array<Real> & block,
                     const size_t chunk_size = 0)
      {
        if (block.is_empty())
          return {};

        Array<Real> emitted;
        emitted.reserve(block.size());
        for (size_t offset = 0; offset < block.size(); offset += block_size_)
          {
            const size_t length = std::min(block_size_, block.size() - offset);
            Array<Real> chunk;
            chunk.reserve(length);
            for (size_t i = 0; i < length; ++i)
              chunk.append(block[offset + i]);

            const Array<Real> partial = process_chunk_impl(chunk, &pool, chunk_size);
            for (size_t i = 0; i < partial.size(); ++i)
              emitted.append(partial[i]);
          }

        return emitted;
      }

      [[nodiscard]] Array<Real>
      flush()
      {
        if (not has_pending_tail_ or overlap_.is_empty())
          {
            has_pending_tail_ = false;
            return {};
          }

        Array<Real> tail = overlap_;
        clear_overlap();
        return tail;
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      convolve(const Container & signal) const
      {
        return convolve(build_real_input(signal));
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      pconvolve(ThreadPool & pool, const Container & signal,
                const size_t chunk_size = 0) const
      {
        return pconvolve(pool, build_real_input(signal), chunk_size);
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      process_block(const Container & block)
      {
        return process_block(build_real_input(block));
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      pprocess_block(ThreadPool & pool, const Container & block,
                     const size_t chunk_size = 0)
      {
        return pprocess_block(pool, build_real_input(block), chunk_size);
      }
    };

    /** @brief Multichannel overlap-add convolver with one shared kernel FFT.
     *
     *  All channels reuse the same transform plan and kernel spectrum. This
     *  avoids recomputing the kernel FFT per channel and enables batched FFT
     *  execution when many channels share the same FIR kernel.
     */
    class OverlapAddBank
    {
      Array<Real> kernel_;
      size_t block_size_ = 0;
      size_t fft_size_ = 0;
      Plan plan_;
      Array<Complex> kernel_spectrum_;
      Array<Array<Real>> overlaps_;
      Array<bool> has_pending_tail_;

      /** @brief Rationale: Selects a block size that is at least as large as the
       *         kernel to ensure efficient FFT processing (defaults to power of two).
       */
      [[nodiscard]] static size_t
      default_block_size(const size_t kernel_size)
      {
        ah_invalid_argument_if(kernel_size == 0)
          << "FFT::OverlapAddBank: kernel size must be positive";
        return next_power_of_two(std::max(kernel_size, size_t(1)));
      }

      /** @brief Rationale: Internal safety guard. */
      void
      validate_channel_count(const size_t count, const char *ctx) const
      {
        ah_invalid_argument_if(count != overlaps_.size())
          << ctx << ": batch size " << count
          << " does not match configured channel count " << overlaps_.size();
      }

      /** @brief Returns the maximum signal length in a batch. */
      [[nodiscard]] static size_t
      max_batch_length(const Array<Array<Real>> & signals) noexcept
      {
        size_t length = 0;
        for (size_t i = 0; i < signals.size(); ++i)
          length = std::max(length, signals[i].size());
        return length;
      }

      /** @brief Returns a contiguous slice of @p input. */
      [[nodiscard]] static Array<Real>
      slice_chunk(const Array<Real> & input,
                  const size_t offset,
                  const size_t length)
      {
        Array<Real> chunk;
        chunk.reserve(length);
        for (size_t i = 0; i < length; ++i)
          chunk.append(input[offset + i]);
        return chunk;
      }

      /** @brief implementation helper for preparing a signal block for FFT. */
      [[nodiscard]] Array<Complex>
      build_signal_block(const Array<Real> & signal,
                         const size_t offset = 0,
                         const size_t length = std::numeric_limits<size_t>::max()) const
      {
        ah_invalid_argument_if(offset > signal.size())
          << "FFT::OverlapAddBank::build_signal_block: offset " << offset
          << " exceeds signal size " << signal.size();
        const size_t effective_length =
            std::min(length, signal.size() - offset);
        ah_invalid_argument_if(effective_length > block_size_)
          << "FFT::OverlapAddBank::build_signal_block: block size "
          << effective_length << " exceeds configured block size " << block_size_;

        Array<Complex> block = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          block(i) = Complex(Real(0), Real(0));
        for (size_t i = 0; i < effective_length; ++i)
          block(i) = Complex(signal[offset + i], Real(0));
        return block;
      }

      /** @brief implementation helper for preparing a batch of signal blocks. */
      [[nodiscard]] Array<Array<Complex>>
      build_signal_batch(const Array<Array<Real>> & block) const
      {
        Array<Array<Complex>> lifted(block.size(), Array<Complex>());
        for (size_t channel = 0; channel < block.size(); ++channel)
          lifted(channel) = build_signal_block(block[channel]);
        return lifted;
      }

      /** @brief implementation helper for preparing a subset of signal blocks. */
      [[nodiscard]] Array<Array<Complex>>
      build_signal_batch(const Array<Array<Real>> & source,
                         const Array<size_t> & channels,
                         const Array<size_t> & lengths,
                         const size_t offset = 0) const
      {
        Array<Array<Complex>> lifted(channels.size(), Array<Complex>());
        for (size_t item = 0; item < channels.size(); ++item)
          lifted(item) = build_signal_block(source[channels[item]],
                                            offset,
                                            lengths[item]);
        return lifted;
      }

      /** @brief Rationale: Parallel pointwise multiplication of multiple signal
       *         spectra with the shared kernel spectrum.
       */
      void
      pointwise_multiply_batch(Array<Array<Complex>> & spectra,
                               ThreadPool *pool,
                               const size_t chunk_size) const
      {
        auto multiply_one = [this, &spectra](const size_t channel)
          {
            for (size_t bin = 0; bin < fft_size_; ++bin)
              spectra(channel)(bin) *= kernel_spectrum_[bin];
          };

        if (pool != nullptr and pool->num_threads() > 1 and spectra.size() > 1)
          parallel_for_index(*pool, 0, spectra.size(), multiply_one, chunk_size);
        else
          for (size_t channel = 0; channel < spectra.size(); ++channel)
            multiply_one(channel);
      }

      /** @brief Rationale: Resets the overlap buffer for a single channel. */
      void
      clear_channel_overlap(const size_t channel)
      {
        for (size_t i = 0; i < overlaps_[channel].size(); ++i)
          overlaps_(channel)(i) = Real(0);
        has_pending_tail_(channel) = false;
      }

      void
      clear_overlaps()
      {
        for (size_t channel = 0; channel < overlaps_.size(); ++channel)
          clear_channel_overlap(channel);
      }

      [[nodiscard]] Array<Array<Real>>
      process_chunk_batch_impl(const Array<Array<Real>> & block,
                               ThreadPool *pool,
                               const size_t chunk_size)
      {
        validate_channel_count(block.size(), "FFT::OverlapAddBank::process_block");

        const size_t active_count = FFT::count_non_empty_real_batch(block);
        Array<size_t> active_channels;
        Array<size_t> active_lengths;
        active_channels.reserve(active_count);
        active_lengths.reserve(active_count);
        for (size_t channel = 0; channel < block.size(); ++channel)
          if (not block[channel].is_empty())
            {
              active_channels.append(channel);
              active_lengths.append(block[channel].size());
            }

        Array<Array<Real>> emitted(block.size(), Array<Real>());
        if (active_channels.is_empty())
          return emitted;

        Array<Array<Complex>> spectra = build_signal_batch(block,
                                                           active_channels,
                                                           active_lengths);
        const bool parallelize_fft =
            FFT::should_parallelize_batch_work(pool, active_channels.size(), fft_size_);
        if (parallelize_fft)
          plan_.ptransform_batch(*pool, spectra, false, chunk_size, false);
        else
          plan_.transform_batch(spectra, false, false);

        pointwise_multiply_batch(spectra, parallelize_fft ? pool : nullptr, chunk_size);

        const Array<Array<Real>> convolved = parallelize_fft ?
                                               plan_.pinverse_transform_real_batch(*pool, spectra, chunk_size) :
                                               plan_.inverse_transform_real_batch(spectra);

        for (size_t item = 0; item < active_channels.size(); ++item)
          {
            const size_t channel = active_channels[item];
            emitted(channel) = Array<Real>::create(active_lengths[item]);
            for (size_t i = 0; i < active_lengths[item]; ++i)
              {
                const Real overlap = i < overlaps_[channel].size() ? overlaps_[channel][i] : Real(0);
                emitted(channel)(i) = convolved[item][i] + overlap;
              }

            if (not overlaps_[channel].is_empty())
              {
                Array<Real> next_overlap = Array<Real>::create(overlaps_[channel].size());
                for (size_t i = 0; i < overlaps_[channel].size(); ++i)
                  {
                    const size_t src = active_lengths[item] + i;
                    const Real from_current = src < convolved[item].size() ? convolved[item][src] : Real(0);
                    const Real from_previous = src < overlaps_[channel].size() ? overlaps_[channel][src] : Real(0);
                    next_overlap(i) = from_current + from_previous;
                  }
                overlaps_(channel) = next_overlap;
              }

            has_pending_tail_(channel) = true;
          }

        return emitted;
      }

      [[nodiscard]] Array<Array<Real>>
      convolve_impl(const Array<Array<Real>> & signals,
                    ThreadPool *pool,
                    const size_t chunk_size) const
      {
        if (signals.is_empty())
          return {};

        Array<Array<Real>> output(signals.size(), Array<Real>());
        for (size_t channel = 0; channel < signals.size(); ++channel)
          {
            if (signals[channel].is_empty())
              continue;
            output(channel) =
                Array<Real>::create(signals[channel].size() + kernel_.size() - 1);
            for (size_t i = 0; i < output[channel].size(); ++i)
              output(channel)(i) = Real(0);
          }

        const size_t max_length = max_batch_length(signals);
        for (size_t offset = 0; offset < max_length; offset += block_size_)
          {
            Array<size_t> active_channels;
            Array<size_t> active_lengths;
            active_channels.reserve(signals.size());
            active_lengths.reserve(signals.size());
            for (size_t channel = 0; channel < signals.size(); ++channel)
              {
                if (offset >= signals[channel].size())
                  continue;

                const size_t length =
                    std::min(block_size_, signals[channel].size() - offset);
                active_channels.append(channel);
                active_lengths.append(length);
              }

            if (active_channels.is_empty())
              continue;

            const bool parallelize_fft =
                FFT::should_parallelize_batch_work(pool, active_channels.size(), fft_size_);
            Array<Array<Complex>> spectra = build_signal_batch(signals,
                                                               active_channels,
                                                               active_lengths,
                                                               offset);
            if (parallelize_fft)
              plan_.ptransform_batch(*pool, spectra, false, chunk_size, false);
            else
              plan_.transform_batch(spectra, false, false);

            pointwise_multiply_batch(spectra,
                                     parallelize_fft ? pool : nullptr,
                                     chunk_size);

            const Array<Array<Real>> blocks = parallelize_fft ?
                                                plan_.pinverse_transform_real_batch(*pool, spectra, chunk_size) :
                                                plan_.inverse_transform_real_batch(spectra);

            for (size_t item = 0; item < active_channels.size(); ++item)
              {
                const size_t channel = active_channels[item];
                const size_t valid = active_lengths[item] + kernel_.size() - 1;
                for (size_t i = 0; i < valid; ++i)
                  output(channel)(offset + i) += blocks[item][i];
              }
          }

        return output;
      }

    public:
      explicit OverlapAddBank(const size_t num_channels,
                              const Array<Real> & kernel,
                              const size_t block_size = 0)
        : kernel_(kernel)
      {
        ah_invalid_argument_if(num_channels == 0)
          << "FFT::OverlapAddBank: at least one channel is required";
        ah_invalid_argument_if(kernel_.is_empty())
          << "FFT::OverlapAddBank: kernel must be non-empty";

        block_size_ = block_size == 0 ? default_block_size(kernel_.size()) : block_size;
        ah_invalid_argument_if(block_size_ == 0)
          << "FFT::OverlapAddBank: block size must be positive";
        ah_invalid_argument_if(block_size_
                               > std::numeric_limits<size_t>::max()
                               - kernel_.size() + 1)
          << "FFT::OverlapAddBank: FFT size overflow";

        fft_size_ = next_power_of_two(block_size_ + kernel_.size() - 1);
        plan_ = Plan(fft_size_);

        overlaps_.reserve(num_channels);
        has_pending_tail_.reserve(num_channels);
        for (size_t channel = 0; channel < num_channels; ++channel)
          {
            overlaps_.append(Array<Real>::create(kernel_.size() - 1));
            has_pending_tail_.append(false);
          }
        clear_overlaps();

        Array<Complex> padded_kernel = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          padded_kernel(i) = Complex(Real(0), Real(0));
        for (size_t i = 0; i < kernel_.size(); ++i)
          padded_kernel(i) = Complex(kernel_[i], Real(0));

        kernel_spectrum_ = padded_kernel;
        plan_.transform(kernel_spectrum_, false);
      }

      [[nodiscard]] size_t num_channels() const noexcept
      {
        return overlaps_.size();
      }

      [[nodiscard]] size_t block_size() const noexcept { return block_size_; }
      [[nodiscard]] size_t fft_size() const noexcept { return fft_size_; }
      [[nodiscard]] size_t overlap_size() const noexcept { return kernel_.size() - 1; }
      [[nodiscard]] const Array<Real> &kernel() const noexcept { return kernel_; }

      void reset()
      {
        clear_overlaps();
      }

      [[nodiscard]] Array<Array<Real>>
      convolve(const Array<Array<Real>> & signals) const
      {
        validate_channel_count(signals.size(), "FFT::OverlapAddBank::convolve");
        return convolve_impl(signals, nullptr, 0);
      }

      [[nodiscard]] Array<Array<Real>>
      pconvolve(ThreadPool & pool,
                const Array<Array<Real>> & signals,
                const size_t chunk_size = 0) const
      {
        validate_channel_count(signals.size(), "FFT::OverlapAddBank::pconvolve");
        return convolve_impl(signals, &pool, chunk_size);
      }

      [[nodiscard]] Array<Array<Real>>
      process_block(const Array<Array<Real>> & block)
      {
        validate_channel_count(block.size(), "FFT::OverlapAddBank::process_block");
        const size_t max_length = max_batch_length(block);
        Array<Array<Real>> emitted(block.size(), Array<Real>());
        for (size_t channel = 0; channel < emitted.size(); ++channel)
          emitted(channel).reserve(block[channel].size());
        for (size_t offset = 0; offset < max_length; offset += block_size_)
          {
            Array<Array<Real>> chunk_batch(block.size(), Array<Real>());
            bool any_pending = false;
            for (size_t channel = 0; channel < block.size(); ++channel)
              {
                if (offset >= block[channel].size())
                  continue;
                const size_t length =
                    std::min(block_size_, block[channel].size() - offset);
                chunk_batch(channel) = slice_chunk(block[channel], offset, length);
                any_pending = true;
              }

            if (not any_pending)
              break;

            const Array<Array<Real>> partial =
                process_chunk_batch_impl(chunk_batch, nullptr, 0);
            for (size_t channel = 0; channel < emitted.size(); ++channel)
              for (size_t i = 0; i < partial[channel].size(); ++i)
                emitted(channel).append(partial[channel][i]);
          }

        return emitted;
      }

      [[nodiscard]] Array<Array<Real>>
      pprocess_block(ThreadPool & pool,
                     const Array<Array<Real>> & block,
                     const size_t chunk_size = 0)
      {
        validate_channel_count(block.size(), "FFT::OverlapAddBank::pprocess_block");
        const size_t max_length = max_batch_length(block);
        Array<Array<Real>> emitted(block.size(), Array<Real>());
        for (size_t channel = 0; channel < emitted.size(); ++channel)
          emitted(channel).reserve(block[channel].size());
        for (size_t offset = 0; offset < max_length; offset += block_size_)
          {
            Array<Array<Real>> chunk_batch(block.size(), Array<Real>());
            bool any_pending = false;
            for (size_t channel = 0; channel < block.size(); ++channel)
              {
                if (offset >= block[channel].size())
                  continue;
                const size_t length =
                    std::min(block_size_, block[channel].size() - offset);
                chunk_batch(channel) = slice_chunk(block[channel], offset, length);
                any_pending = true;
              }

            if (not any_pending)
              break;

            const Array<Array<Real>> partial =
                process_chunk_batch_impl(chunk_batch, &pool, chunk_size);
            for (size_t channel = 0; channel < emitted.size(); ++channel)
              for (size_t i = 0; i < partial[channel].size(); ++i)
                emitted(channel).append(partial[channel][i]);
          }

        return emitted;
      }

      [[nodiscard]] Array<Array<Real>>
      flush()
      {
        Array<Array<Real>> tail(overlaps_.size(), Array<Real>());
        for (size_t channel = 0; channel < overlaps_.size(); ++channel)
          {
            if (has_pending_tail_[channel] and not overlaps_[channel].is_empty())
              tail(channel) = overlaps_[channel];
            clear_channel_overlap(channel);
          }
        return tail;
      }

      [[nodiscard]] Array<Array<Real>>
      pflush(ThreadPool & pool, const size_t chunk_size = 0)
      {
        Array<Array<Real>> tail(overlaps_.size(), Array<Real>());
        auto flush_one = [this, &tail](const size_t channel)
          {
            if (has_pending_tail_[channel] and not overlaps_[channel].is_empty())
              tail(channel) = overlaps_[channel];
            clear_channel_overlap(channel);
          };

        if (pool.num_threads() > 1 and overlaps_.size() > 1)
          parallel_for_index(pool, 0, overlaps_.size(), flush_one, chunk_size);
        else
          for (size_t channel = 0; channel < overlaps_.size(); ++channel)
            flush_one(channel);

        return tail;
      }
    };

    /** @brief Reusable overlap-save convolver for streaming real FIR filtering.
     *
     *  The configured block size controls both algorithmic latency and the
     *  number of valid samples emitted per internal FFT block.
     */
    class OverlapSave
    {
      Array<Real> kernel_;
      size_t block_size_ = 0;
      size_t fft_size_ = 0;
      Plan plan_;
      Array<Complex> kernel_spectrum_;
      Array<Real> history_;
      Array<Real> pending_;
      size_t total_input_size_ = 0;
      size_t emitted_output_size_ = 0;

      /** @brief Rationale: Heuristic for choosing a block size that provides a good
       *         balance between algorithmic latency and FFT efficiency.
       */
      [[nodiscard]] static size_t
      default_block_size(const size_t kernel_size)
      {
        ah_invalid_argument_if(kernel_size == 0)
          << "FFT::OverlapSave: kernel size must be positive";
        return next_power_of_two(std::max(size_t(2), kernel_size * 2))
               - kernel_size + 1;
      }

      /** @brief Rationale: Resets the state for a new filtering process. */
      void
      clear_history()
      {
        for (size_t i = 0; i < history_.size(); ++i)
          history_(i) = Real(0);
      }

      /** @brief Rationale: Maintains the sliding window of historical samples
       *         required to correctly compute the linear convolution prefix
       *         of the next block.
       */
      void
      update_history(const Array<Real> & padded_chunk)
      {
        if (history_.is_empty())
          return;

        if (padded_chunk.size() >= history_.size())
          {
            const size_t start = padded_chunk.size() - history_.size();
            for (size_t i = 0; i < history_.size(); ++i)
              history_(i) = padded_chunk[start + i];
            return;
          }

        Array<Real> next_history = Array<Real>::create(history_.size());
        const size_t preserved = history_.size() - padded_chunk.size();
        for (size_t i = 0; i < preserved; ++i)
          next_history(i) = history_[history_.size() - preserved + i];
        for (size_t i = 0; i < padded_chunk.size(); ++i)
          next_history(preserved + i) = padded_chunk[i];
        history_ = next_history;
      }

      /** @brief Rationale: Prepares a frequency-domain segment by combining
       *         historical samples with the current signal chunk.
       */
      [[nodiscard]] Array<Complex>
      build_segment_spectrum(const Array<Real> & chunk) const
      {
        ah_invalid_argument_if(chunk.size() > block_size_)
          << "FFT::OverlapSave::process_block: block size " << chunk.size()
          << " exceeds configured block size " << block_size_;

        Array<Real> padded_chunk = Array<Real>::create(block_size_);
        for (size_t i = 0; i < block_size_; ++i)
          padded_chunk(i) = i < chunk.size() ? chunk[i] : Real(0);

        Array<Complex> segment = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          segment(i) = Complex(Real(0), Real(0));
        for (size_t i = 0; i < history_.size(); ++i)
          segment(i) = Complex(history_[i], Real(0));
        for (size_t i = 0; i < padded_chunk.size(); ++i)
          segment(history_.size() + i) = Complex(padded_chunk[i], Real(0));
        return segment;
      }

      /** @brief Internal helper for zero-padding chunks. */
      [[nodiscard]] Array<Real>
      padded_chunk_copy(const Array<Real> & chunk) const
      {
        Array<Real> padded_chunk = Array<Real>::create(block_size_);
        for (size_t i = 0; i < block_size_; ++i)
          padded_chunk(i) = i < chunk.size() ? chunk[i] : Real(0);
        return padded_chunk;
      }

      /** @brief Rationale: Implements the core Overlap-Save logic for one block:
       *         1. Combine History + Chunk.
       *         2. FFT to frequency domain.
       *         3. Pointwise multiply with kernel spectrum.
       *         4. Inverse FFT to time domain.
       *         5. Discard corrupted (circular) samples and emit valid results.
       *         6. Update history for next block.
       */
      [[nodiscard]] Array<Real>
      process_chunk_impl(const Array<Real> & chunk)
      {
        if (chunk.is_empty())
          return {};

        Array<Real> padded_chunk = padded_chunk_copy(chunk);
        Array<Complex> segment = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          segment(i) = Complex(Real(0), Real(0));
        for (size_t i = 0; i < history_.size(); ++i)
          segment(i) = Complex(history_[i], Real(0));
        for (size_t i = 0; i < padded_chunk.size(); ++i)
          segment(history_.size() + i) = Complex(padded_chunk[i], Real(0));

        plan_.transform(segment, false);
        for (size_t i = 0; i < fft_size_; ++i)
          segment(i) *= kernel_spectrum_[i];
        const Array<Real> convolved = plan_.inverse_transform_real(segment);

        update_history(padded_chunk);
        emitted_output_size_ += block_size_;
        return slice_copy(convolved, kernel_.size() - 1, block_size_);
      }

    public:
      explicit OverlapSave(const Array<Real> & kernel,
                           const size_t block_size = 0)
        : kernel_(kernel)
      {
        ah_invalid_argument_if(kernel_.is_empty())
          << "FFT::OverlapSave: kernel must be non-empty";

        block_size_ = block_size == 0 ? default_block_size(kernel_.size()) : block_size;
        ah_invalid_argument_if(block_size_ == 0)
          << "FFT::OverlapSave: block size must be positive";
        ah_invalid_argument_if(block_size_
                               > std::numeric_limits<size_t>::max()
                               - kernel_.size() + 1)
          << "FFT::OverlapSave: FFT size overflow";

        fft_size_ = next_power_of_two(block_size_ + kernel_.size() - 1);
        plan_ = Plan(fft_size_);
        history_ = Array<Real>::create(kernel_.size() - 1);
        clear_history();

        Array<Complex> padded_kernel = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          padded_kernel(i) = Complex(Real(0), Real(0));
        for (size_t i = 0; i < kernel_.size(); ++i)
          padded_kernel(i) = Complex(kernel_[i], Real(0));
        kernel_spectrum_ = padded_kernel;
        plan_.transform(kernel_spectrum_, false);
      }

      /** @brief Returns the processing block size (the hop size). */
      [[nodiscard]] size_t block_size() const noexcept { return block_size_; }

      /** @brief Returns the FFT size used internally (block_size + kernel_size - 1 padded to power of 2). */
      [[nodiscard]] size_t fft_size() const noexcept { return fft_size_; }

      /** @brief Returns the current FIR kernel. */
      [[nodiscard]] const Array<Real> &kernel() const noexcept { return kernel_; }

      /** @brief Returns the internal overlap history size. */
      [[nodiscard]] size_t overlap_size() const noexcept { return history_.size(); }

      /** @brief Resets the internal filter state and history. */
      void reset()
      {
        clear_history();
        pending_ = {};
        total_input_size_ = 0;
        emitted_output_size_ = 0;
      }

      /** @brief Processes an input block and returns the corresponding filtered output.
       *
       *  Input samples are buffered until a full block is available. Multiple blocks
       *  might be emitted if the input is large.
       *
       *  @param block Input signal block.
       *  @return Filtered output samples available so far.
       */
      [[nodiscard]] Array<Real>
      process_block(const Array<Real> & block)
      {
        if (block.is_empty())
          return {};

        total_input_size_ += block.size();
        append_all(pending_, block);

        Array<Real> emitted;
        while (pending_.size() >= block_size_)
          {
            const Array<Real> chunk = slice_copy(pending_, 0, block_size_);
            append_all(emitted, process_chunk_impl(chunk));
            pending_ = slice_copy(pending_,
                                  block_size_,
                                  pending_.size() - block_size_);
          }

        return emitted;
      }

      /** @brief Flushes all buffered samples and returns the final tail of the convolution.
       *
       *  Zero-padding is applied to complete the final processing block if necessary.
       *  The filter is reset after flushing.
       *
       *  @return The remaining filtered samples.
       */
      [[nodiscard]] Array<Real>
      flush()
      {
        if (total_input_size_ == 0)
          {
            reset();
            return {};
          }

        const size_t target_output_size = total_input_size_ + kernel_.size() - 1;
        const size_t emitted_before = emitted_output_size_;
        Array<Real> emitted;

        if (not pending_.is_empty())
          {
            append_all(emitted, process_chunk_impl(pending_));
            pending_ = {};
          }

        while (emitted_output_size_ < target_output_size)
          {
            Array<Real> zeros = Array<Real>::create(block_size_);
            for (size_t i = 0; i < block_size_; ++i)
              zeros(i) = Real(0);
            append_all(emitted, process_chunk_impl(zeros));
          }

        const size_t required = target_output_size - emitted_before;
        Array<Real> output = slice_copy(emitted, 0, required);
        reset();
        return output;
      }

      /** @brief Convolves the input signal with the kernel (one-shot convenience). */
      [[nodiscard]] Array<Real>
      convolve(const Array<Real> & signal)
      {
        reset();
        Array<Real> output = process_block(signal);
        append_all(output, flush());
        return output;
      }

      /** @brief Processes an input block from a real-valued container. */
      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      process_block(const Container & block)
      {
        return process_block(build_real_input(block));
      }

      /** @brief Convolves an input signal from a real-valued container. */
      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      convolve(const Container & signal)
      {
        return convolve(build_real_input(signal));
      }
    };

    /** @brief Uniform partitioned FIR convolver for low-latency streaming.
     *
     *  The partition size controls algorithmic latency directly: each processed
     *  partition emits that many time-domain samples.
     */
    class PartitionedConvolver
    {
      Array<Real> kernel_;
      size_t partition_size_ = 0;
      size_t fft_size_ = 0;
      Plan plan_;
      Array<Array<Complex>> kernel_partitions_;
      Array<Array<Complex>> input_history_;
      size_t head_ = 0;
      Array<Real> overlap_;
      Array<Real> pending_;
      size_t total_input_size_ = 0;
      size_t emitted_output_size_ = 0;

      /** @brief Rationale: Selects a partition size that minimizes latency while
       *         maintaining FFT efficiency (capped at 256 for low-latency).
       */
      [[nodiscard]] static size_t
      default_partition_size(const size_t kernel_size)
      {
        ah_invalid_argument_if(kernel_size == 0)
          << "FFT::PartitionedConvolver: kernel size must be positive";
        return next_power_of_two(std::min(kernel_size, size_t(256)));
      }

      /** @brief Rationale: Resets all delay-line history and overlap buffers. */
      void
      clear_state()
      {
        for (size_t i = 0; i < input_history_.size(); ++i)
          for (size_t j = 0; j < input_history_[i].size(); ++j)
            input_history_(i)(j) = Complex(Real(0), Real(0));
        for (size_t i = 0; i < overlap_.size(); ++i)
          overlap_(i) = Real(0);
        pending_ = {};
        head_ = 0;
        total_input_size_ = 0;
        emitted_output_size_ = 0;
      }

      /** @brief implementation helper for creating zero-filled complex spectra. */
      [[nodiscard]] Array<Complex>
      zero_spectrum() const
      {
        Array<Complex> spectrum = Array<Complex>::create(fft_size_);
        for (size_t i = 0; i < fft_size_; ++i)
          spectrum(i) = Complex(Real(0), Real(0));
        return spectrum;
      }

      /** @brief implementation helper for preparing a padded partition. */
      [[nodiscard]] Array<Real>
      padded_partition(const Array<Real> & chunk) const
      {
        Array<Real> padded = Array<Real>::create(partition_size_);
        for (size_t i = 0; i < partition_size_; ++i)
          padded(i) = i < chunk.size() ? chunk[i] : Real(0);
        return padded;
      }

      /** @brief Rationale: Implements the core frequency-domain partitioned
       *         convolution logic:
       *         1. FFT of current signal partition.
       *         2. Update rotating input history.
       *         3. Sum element-wise products of all history/kernel pairs.
       *         4. Inverse FFT and overlap-add.
       */
      [[nodiscard]] Array<Real>
      process_partition_impl(const Array<Real> & chunk)
      {
        if (chunk.is_empty())
          return {};

        const Array<Real> padded = padded_partition(chunk);
        Array<Complex> input = zero_spectrum();
        for (size_t i = 0; i < padded.size(); ++i)
          input(i) = Complex(padded[i], Real(0));
        plan_.transform(input, false);
        input_history_(head_) = input;

        Array<Complex> accumulated = zero_spectrum();
        for (size_t part = 0; part < kernel_partitions_.size(); ++part)
          {
            const size_t history_index =
                (head_ + kernel_partitions_.size() - part) % kernel_partitions_.size();
            for (size_t bin = 0; bin < fft_size_; ++bin)
              accumulated(bin) += input_history_[history_index][bin]
                  * kernel_partitions_[part][bin];
          }

        const Array<Real> convolved = plan_.inverse_transform_real(accumulated);
        Array<Real> emitted = Array<Real>::create(partition_size_);
        for (size_t i = 0; i < partition_size_; ++i)
          emitted(i) = convolved[i] + overlap_[i];
        for (size_t i = 0; i < partition_size_; ++i)
          overlap_(i) = convolved[partition_size_ + i];

        head_ = (head_ + 1) % kernel_partitions_.size();
        emitted_output_size_ += partition_size_;
        return emitted;
      }

    public:
      explicit PartitionedConvolver(const Array<Real> & kernel,
                                    const size_t partition_size = 0)
        : kernel_(kernel)
      {
        ah_invalid_argument_if(kernel_.is_empty())
          << "FFT::PartitionedConvolver: kernel must be non-empty";

        partition_size_ =
            partition_size == 0 ? default_partition_size(kernel_.size()) : partition_size;
        ah_invalid_argument_if(partition_size_ == 0)
          << "FFT::PartitionedConvolver: partition size must be positive";

        fft_size_ = next_power_of_two(partition_size_ * 2);
        plan_ = Plan(fft_size_);
        overlap_ = Array<Real>::create(partition_size_);
        for (size_t i = 0; i < overlap_.size(); ++i)
          overlap_(i) = Real(0);

        const size_t num_partitions =
            (kernel_.size() + partition_size_ - 1) / partition_size_;
        kernel_partitions_.reserve(num_partitions);
        input_history_.reserve(num_partitions);
        for (size_t part = 0; part < num_partitions; ++part)
          {
            Array<Complex> partition = zero_spectrum();
            const size_t offset = part * partition_size_;
            const size_t length =
                std::min(partition_size_, kernel_.size() - offset);
            for (size_t i = 0; i < length; ++i)
              partition(i) = Complex(kernel_[offset + i], Real(0));
            plan_.transform(partition, false);
            kernel_partitions_.append(partition);
            input_history_.append(zero_spectrum());
          }

        clear_state();
      }

      [[nodiscard]] size_t partition_size() const noexcept
      {
        return partition_size_;
      }

      [[nodiscard]] size_t fft_size() const noexcept
      {
        return fft_size_;
      }

      [[nodiscard]] const Array<Real> &kernel() const noexcept
      {
        return kernel_;
      }

      void reset()
      {
        clear_state();
      }

      [[nodiscard]] Array<Real>
      process_block(const Array<Real> & block)
      {
        if (block.is_empty())
          return {};

        total_input_size_ += block.size();
        append_all(pending_, block);

        Array<Real> emitted;
        while (pending_.size() >= partition_size_)
          {
            const Array<Real> chunk = slice_copy(pending_, 0, partition_size_);
            append_all(emitted, process_partition_impl(chunk));
            pending_ = slice_copy(pending_,
                                  partition_size_,
                                  pending_.size() - partition_size_);
          }

        return emitted;
      }

      [[nodiscard]] Array<Real>
      flush()
      {
        if (total_input_size_ == 0)
          {
            reset();
            return {};
          }

        const size_t target_output_size = total_input_size_ + kernel_.size() - 1;
        const size_t emitted_before = emitted_output_size_;
        Array<Real> emitted;

        if (not pending_.is_empty())
          {
            append_all(emitted, process_partition_impl(pending_));
            pending_ = {};
          }

        while (emitted_output_size_ < target_output_size)
          {
            Array<Real> zeros = Array<Real>::create(partition_size_);
            for (size_t i = 0; i < partition_size_; ++i)
              zeros(i) = Real(0);
            append_all(emitted, process_partition_impl(zeros));
          }

        const size_t required = target_output_size - emitted_before;
        Array<Real> output = slice_copy(emitted, 0, required);
        reset();
        return output;
      }

      [[nodiscard]] Array<Real>
      convolve(const Array<Real> & signal)
      {
        reset();
        Array<Real> output = process_block(signal);
        append_all(output, flush());
        return output;
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      process_block(const Container & block)
      {
        return process_block(build_real_input(block));
      }

      template <typename Container>
        requires Is_Real_Container<Container>
      [[nodiscard]] Array<Real>
      convolve(const Container & signal)
      {
        return convolve(build_real_input(signal));
      }
    };

    /** @brief Convenience wrapper for long real convolution via overlap-add. */
    [[nodiscard]] static Array<Real>
    overlap_add_convolution(const Array<Real> & signal,
                            const Array<Real> & kernel,
                            const size_t block_size = 0)
    {
      if (signal.is_empty() or kernel.is_empty())
        return {};
      return OverlapAdd(kernel, block_size).convolve(signal);
    }

    /** @brief Parallel convenience wrapper for overlap-add convolution. */
    /** @brief Parallel version of @ref overlap_add_convolution(const Array<Real>&, const Array<Real>&, size_t). */
    [[nodiscard]] static Array<Real>
    poverlap_add_convolution(ThreadPool & pool,
                             const Array<Real> & signal,
                             const Array<Real> & kernel,
                             const size_t block_size = 0,
                             const size_t chunk_size = 0)
    {
      if (signal.is_empty() or kernel.is_empty())
        return {};
      return OverlapAdd(kernel, block_size).pconvolve(pool, signal, chunk_size);
    }

    /** @brief Multichannel overlap-add convolution with a shared kernel FFT. */
    [[nodiscard]] static Array<Array<Real>>
    overlap_add_convolution_batch(const Array<Array<Real>> & signals,
                                  const Array<Real> & kernel,
                                  const size_t block_size = 0)
    {
      if (signals.is_empty() or kernel.is_empty())
        return {};
      return OverlapAddBank(signals.size(), kernel, block_size).convolve(signals);
    }

    /** @brief Parallel version of @ref overlap_add_convolution_batch. */
    [[nodiscard]] static Array<Array<Real>>
    poverlap_add_convolution_batch(ThreadPool & pool,
                                   const Array<Array<Real>> & signals,
                                   const Array<Real> & kernel,
                                   const size_t block_size = 0,
                                   const size_t chunk_size = 0)
    {
      if (signals.is_empty() or kernel.is_empty())
        return {};
      return OverlapAddBank(signals.size(), kernel, block_size)
          .pconvolve(pool, signals, chunk_size);
    }

    /** @brief Convenience wrapper for overlap-save real convolution. */
    [[nodiscard]] static Array<Real>
    overlap_save_convolution(const Array<Real> & signal,
                             const Array<Real> & kernel,
                             const size_t block_size = 0)
    {
      if (signal.is_empty() or kernel.is_empty())
        return {};
      return OverlapSave(kernel, block_size).convolve(signal);
    }

    /** @brief Convenience wrapper for low-latency partitioned convolution. */
    [[nodiscard]] static Array<Real>
    partitioned_convolution(const Array<Real> & signal,
                            const Array<Real> & kernel,
                            const size_t partition_size = 0)
    {
      if (signal.is_empty() or kernel.is_empty())
        return {};
      return PartitionedConvolver(kernel, partition_size).convolve(signal);
    }

    /** @brief Overlap-add convolution for real-valued containers. */
    template <typename Container1, typename Container2>
      requires Is_Real_Container<Container1> and Is_Real_Container<Container2>
    [[nodiscard]] static Array<Real>
    overlap_add_convolution(const Container1 & signal,
                            const Container2 & kernel,
                            const size_t block_size = 0)
    {
      return overlap_add_convolution(build_real_input(signal),
                                     build_real_input(kernel),
                                     block_size);
    }

    /** @brief Parallel version of @ref overlap_add_convolution(const Container1&, const Container2&, size_t). */
    template <typename Container1, typename Container2>
      requires Is_Real_Container<Container1> and Is_Real_Container<Container2>
    [[nodiscard]] static Array<Real>
    poverlap_add_convolution(ThreadPool & pool,
                             const Container1 & signal,
                             const Container2 & kernel,
                             const size_t block_size = 0,
                             const size_t chunk_size = 0)
    {
      return poverlap_add_convolution(pool,
                                      build_real_input(signal),
                                      build_real_input(kernel),
                                      block_size,
                                      chunk_size);
    }

    /** @brief Overlap-save convolution for real-valued containers. */
    template <typename Container1, typename Container2>
      requires Is_Real_Container<Container1> and Is_Real_Container<Container2>
    [[nodiscard]] static Array<Real>
    overlap_save_convolution(const Container1 & signal,
                             const Container2 & kernel,
                             const size_t block_size = 0)
    {
      return overlap_save_convolution(build_real_input(signal),
                                      build_real_input(kernel),
                                      block_size);
    }

    /** @brief Partitioned convolution for real-valued containers. */
    template <typename Container1, typename Container2>
      requires Is_Real_Container<Container1> and Is_Real_Container<Container2>
    [[nodiscard]] static Array<Real>
    partitioned_convolution(const Container1 & signal,
                            const Container2 & kernel,
                            const size_t partition_size = 0)
    {
      return partitioned_convolution(build_real_input(signal),
                                     build_real_input(kernel),
                                     partition_size);
    }

    /** @brief Multiplies two complex-valued sequences using convolution.
     *
     *  This method computes the linear convolution of two arrays by:
     *  1. Padding both inputs to a power-of-two size (N >= size(a) + size(b) - 1).
     *  2. Transforming both inputs to the frequency domain (FFT).
     *  3. Performing element-wise multiplication.
     *  4. Transforming the result back to the time/spatial domain (IFFT).
     *
     *  Generic iterable containers of `std::complex` values are accepted
     *  through a separate overload.
     *
     *  This is equivalent to multiplying two polynomials whose coefficients
     *  are given by the input arrays.
     *
     *  @param a The first complex sequence (e.g., polynomial coefficients).
     *  @param b The second complex sequence.
     *  @return The result of the convolution (size will be a.size() + b.size() - 1).
     *  @throw std::invalid_argument if the resulting size overflows size_t.
     */
    [[nodiscard]] static Array<Complex>
    multiply(const Array<Complex> & a, const Array<Complex> & b)
    {
      return multiply_complex_impl(a, b);
    }

    /** @brief Parallel version of @ref multiply(const Array<Complex>&, const Array<Complex>&). */
    [[nodiscard]] static Array<Complex>
    pmultiply(ThreadPool & pool, const Array<Complex> & a, const Array<Complex> & b,
              const size_t chunk_size = 0)
    {
      return multiply_complex_impl(a, b, &pool, chunk_size);
    }

    /** @brief Multiplication for complex-valued containers. */
    template <typename Container1, typename Container2>
      requires Is_Complex_Container<Container1> and Is_Complex_Container<Container2>
    [[nodiscard]] static Array<Complex>
    multiply(const Container1 & a, const Container2 & b)
    {
      return multiply(build_complex_input(a), build_complex_input(b));
    }

    /** @brief Parallel version of @ref multiply(const Container1&, const Container2&). */
    template <typename Container1, typename Container2>
      requires Is_Complex_Container<Container1> and Is_Complex_Container<Container2>
    [[nodiscard]] static Array<Complex>
    pmultiply(ThreadPool & pool, const Container1 & a, const Container2 & b,
              const size_t chunk_size = 0)
    {
      return pmultiply(pool, build_complex_input(a), build_complex_input(b), chunk_size);
    }

    /** @brief Multiplies two real-valued sequences using convolution.
     *
     *  Computes the linear convolution of two real arrays using the FFT-based
     *  multiplication algorithm. Generic iterable containers of real values
     *  convertible to `Real` are accepted through a separate overload.
     *
     *  @param a The first real sequence.
     *  @param b The second real sequence.
     *  @return The result of the convolution (size will be a.size() + b.size() - 1).
     *  @throw std::invalid_argument if the resulting size overflows size_t.
     *  @throw std::domain_error if the intermediate result has non-negligible
     *                           imaginary parts (numerical instability).
     */
    [[nodiscard]] static Array<Real>
    multiply(const Array<Real> & a, const Array<Real> & b)
    {
      return multiply_real_optimized(a, b);
    }

    /** @brief Parallel version of @ref multiply(const Array<Real>&, const Array<Real>&). */
    [[nodiscard]] static Array<Real>
    pmultiply(ThreadPool & pool, const Array<Real> & a, const Array<Real> & b,
              const size_t chunk_size = 0)
    {
      return multiply_real_optimized(a, b, &pool, chunk_size);
    }

    /** @brief Multiplication for real-valued containers. */
    template <typename Container1, typename Container2>
      requires Is_Real_Container<Container1> and Is_Real_Container<Container2>
    [[nodiscard]] static Array<Real>
    multiply(const Container1 & a, const Container2 & b)
    {
      return multiply(build_real_input(a), build_real_input(b));
    }

    /** @brief Parallel version of @ref multiply(const Container1&, const Container2&). */
    template <typename Container1, typename Container2>
      requires Is_Real_Container<Container1> and Is_Real_Container<Container2>
    [[nodiscard]] static Array<Real>
    pmultiply(ThreadPool & pool, const Container1 & a, const Container2 & b,
              const size_t chunk_size = 0)
    {
      return pmultiply(pool, build_real_input(a), build_real_input(b), chunk_size);
    }
  };
} // namespace Aleph

# endif // FFT_H
