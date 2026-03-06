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
 *  @brief Fast Fourier Transform (FFT) for complex and real-valued sequences.
 *
 *  Provides an iterative mixed radix-2/radix-4 Cooley-Tukey FFT with support for:
 *  - Forward and inverse transforms over complex arrays
 *  - Optimized sequential transforms and convolution for real-valued inputs
 *  - Separate concurrent APIs backed by `ThreadPool`
 *  - Linear convolution for both real and complex coefficient arrays
 *
 *  This header is suitable for spectral analysis, polynomial multiplication
 *  over the reals, filtering, and other frequency-domain computations.
 *
 *  @example fft_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef FFT_H
# define FFT_H

# include <algorithm>
# include <cmath>
# include <complex>
# include <concepts>
# include <iterator>
# include <limits>
# include <memory>
# include <numbers>
# include <type_traits>

# include <ah-errors.H>
# include <thread_pool.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Fast Fourier Transform (FFT) implementation.
   *
   *  The FFT class provides a template-based implementation of the Fast Fourier
   *  Transform using a mixed radix-2/radix-4 Cooley-Tukey algorithm. This
   *  implementation supports:
   *  - Forward and inverse transforms for complex and real-valued sequences.
   *  - An optimized sequential path for real FFT and real convolution.
   *  - A reusable @ref Plan object that precomputes twiddle factors and
   *    bit-reversal tables for repeated transforms of the same size.
   *  - Separate concurrent `p...` APIs that accept an explicit `ThreadPool`.
   *  - Generic iterable containers whose elements are real values or
   *    `std::complex` numbers convertible to the selected `Real` precision.
   *  - Bit-reversal permutation for in-place computation.
   *  - Efficient linear convolution (polynomial multiplication) using FFT.
   *
   *  The core algorithm has a time complexity of O(N log N), where N is the
   *  number of elements in the sequence. For the core @ref transform method,
   *  N must be a power of two.
   *
   *  @tparam Real The floating-point type to use for computations (e.g.,
   *               double, float, long double). Defaults to double.
   */
  template <std::floating_point Real = double>
  class FFT
  {
  public:
    using value_type = Real;
    using Complex = std::complex<Real>;
    class Plan;
    
    /** @brief Generic IIR transfer-function coefficients. */
    struct IIRCoefficients
    {
      Array<Real> numerator;
      Array<Real> denominator;
    };

    /** @brief One second-order section of an IIR cascade. */
    struct BiquadSection
    {
      Real b0 = Real(1);
      Real b1 = Real(0);
      Real b2 = Real(0);
      Real a0 = Real(1);
      Real a1 = Real(0);
      Real a2 = Real(0);

      [[nodiscard]] Array<Real>
      numerator() const
      {
        return {b0, b1, b2};
      }

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

  private:
    template <typename T>
    struct Is_Std_Complex : std::false_type
    {
    };

    template <typename T>
    struct Is_Std_Complex<std::complex<T>> : std::true_type
    {
    };

    template <typename Container>
    static constexpr bool Is_Complex_Container = requires (const Container & c)
    {
      std::begin(c);
      std::end(c);
      requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
    };

    template <typename Container>
    static constexpr bool Is_Real_Container = requires (const Container & c)
    {
      std::begin(c);
      std::end(c);
      requires std::convertible_to<decltype(*std::begin(c)), Real>;
      requires not Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
    };

    template <typename Container>
    static constexpr bool Is_Biquad_Container = requires (const Container & c)
    {
      std::begin(c);
      std::end(c);
      requires std::convertible_to<std::remove_cvref_t<decltype(*std::begin(c))>,
                                   BiquadSection>;
    };

    template <typename Container>
    static constexpr bool Is_Real_Batch_Container = requires (const Container & c)
    {
      std::begin(c);
      std::end(c);
      requires Is_Real_Container<std::remove_cvref_t<decltype(*std::begin(c))>>;
    };

    static constexpr size_t twiddle_refresh_period = 32;

    [[nodiscard]] static Complex
    twiddle_at(const Real angle, const size_t index)
    {
      return std::polar(Real(1), angle * static_cast<Real>(index));
    }

    [[nodiscard]] static Real
    transform_stages(const size_t n) noexcept
    {
      return std::max(Real(1),
                      std::log2(static_cast<Real>(std::max(n, size_t(2)))));
    }

    [[nodiscard]] static Real
    real_projection_tolerance(const Complex & value, const size_t n) noexcept
    {
      const Real scale = std::abs(value.real()) + std::abs(value.imag()) + Real(1);
      return Real(128) * transform_stages(n)
             * std::numeric_limits<Real>::epsilon() * scale;
    }

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

    static void
    validate_real_spectrum(const Array<Complex> & input, const char * ctx)
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
      ah_domain_error_if(std::abs(input[half].imag())
                         > real_projection_tolerance(input[half], n))
        << ctx << ": coefficient " << half
        << " has non-negligible imaginary part " << input[half].imag();

      for (size_t k = 1; k < half; ++k)
        {
          const Complex expected = std::conj(input[n - k]);
          ah_domain_error_if(std::abs(input[k] - expected)
                             > hermitian_tolerance(input[k], input[n - k], n))
            << ctx << ": spectrum is not Hermitian at k=" << k;
        }
    }

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

    template <typename T>
    [[nodiscard]] static Array<T>
    slice_copy(const Array<T> & input,
               const size_t offset,
               const size_t length)
    {
      ah_invalid_argument_if(offset > input.size())
        << "FFT::slice_copy: offset " << offset
        << " exceeds input size " << input.size();
      ah_invalid_argument_if(length > input.size() - offset)
        << "FFT::slice_copy: length " << length
        << " exceeds slice capacity " << (input.size() - offset);

      Array<T> output;
      output.reserve(length);
      for (size_t i = 0; i < length; ++i)
        output.append(input[offset + i]);
      return output;
    }

    [[nodiscard]] static size_t
    default_filtfilt_pad_length(const size_t signal_size,
                                const size_t coeff_size) noexcept
    {
      if (signal_size <= 1 or coeff_size <= 1)
        return 0;

      const size_t taps_minus_one = coeff_size - 1;
      const size_t suggested = taps_minus_one
                               > std::numeric_limits<size_t>::max() / 3
        ? std::numeric_limits<size_t>::max()
        : taps_minus_one * 3;
      return std::min(suggested, signal_size - 1);
    }

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

    [[nodiscard]] static Array<Real>
    overlap_profile(const Array<Real> & analysis_window,
                    const Array<Real> & synthesis_window,
                    const size_t hop_size,
                    const char * ctx)
    {
      ah_invalid_argument_if(analysis_window.is_empty())
        << ctx << ": analysis window must be non-empty";
      ah_invalid_argument_if(synthesis_window.is_empty())
        << ctx << ": synthesis window must be non-empty";
      ah_invalid_argument_if(analysis_window.size() != synthesis_window.size())
        << ctx << ": analysis window size " << analysis_window.size()
        << " does not match synthesis window size "
        << synthesis_window.size();
      ah_invalid_argument_if(hop_size == 0)
        << ctx << ": hop size must be positive";

      Array<Real> profile = Array<Real>::create(hop_size);
      for (size_t i = 0; i < hop_size; ++i)
        profile(i) = Real(0);

      for (size_t i = 0; i < analysis_window.size(); ++i)
        profile(i % hop_size) += analysis_window[i] * synthesis_window[i];

      return profile;
    }

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

    static void
    validate_overlap_constraints(const Array<Real> & analysis_window,
                                 const Array<Real> & synthesis_window,
                                 const size_t hop_size,
                                 const bool validate_nola,
                                 const bool validate_cola,
                                 const char * ctx)
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

    [[nodiscard]] static Real
    max_abs_value(const Array<Real> & input) noexcept
    {
      Real max_value = Real(0);
      for (size_t i = 0; i < input.size(); ++i)
        max_value = std::max(max_value, std::abs(input[i]));
      return max_value;
    }

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

    [[nodiscard]] static Array<Real>
    scaled_copy(const Array<Real> & input, const Real factor)
    {
      Array<Real> output = Array<Real>::create(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output(i) = input[i] * factor;
      return output;
    }

    [[nodiscard]] static IIRCoefficients
    normalize_iir_coefficients(const Array<Real> & numerator,
                               const Array<Real> & denominator,
                               const char * ctx)
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
                       const char * ctx)
    {
      if (n == 0)
        return {};

      ah_invalid_argument_if(matrix.size() != n * n)
        << ctx << ": matrix size " << matrix.size()
        << " does not match a dense " << n << "x" << n << " system";
      ah_invalid_argument_if(rhs.size() != n)
        << ctx << ": rhs size " << rhs.size()
        << " does not match system size " << n;

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
                     const char * ctx)
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
                    const char * ctx,
                    Array<Real> * final_state = nullptr)
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
                      const char * ctx)
    {
      if (signal.is_empty() or numerator.is_empty() or denominator.is_empty())
        return {};

      const IIRCoefficients coeffs =
        normalize_iir_coefficients(numerator, denominator, ctx);
      const size_t order = coeffs.denominator.size() - 1;

      if (order == 0)
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
                      const char * ctx)
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
        << "FFT::drop_prefix: count " << count
        << " exceeds input size " << input.size();

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
                          const char * ctx)
    {
      ah_invalid_argument_if(window.is_empty())
        << ctx << ": window must be non-empty";
      ah_invalid_argument_if(options.hop_size == 0)
        << ctx << ": hop size must be positive";

      const size_t fft_size = options.fft_size == 0
        ? next_power_of_two(window.size())
        : options.fft_size;
      ah_invalid_argument_if(fft_size < window.size())
        << ctx << ": FFT size " << fft_size
        << " is smaller than window size " << window.size();
      ah_invalid_argument_if(not is_power_of_two(fft_size))
        << ctx << ": FFT size " << fft_size << " must be a power of two";

      validate_overlap_constraints(window,
                                   window,
                                   options.hop_size,
                                   options.validate_nola,
                                   options.validate_cola,
                                   ctx);
      return fft_size;
    }

    [[nodiscard]] static Array<Complex>
    transform_stft_frame(const Array<Real> & frame,
                         const Array<Real> & window,
                         const size_t fft_size,
                         const Plan & plan,
                         ThreadPool * pool,
                         const size_t chunk_size,
                         const bool allow_plan_parallelism)
    {
      Array<Complex> spectrum =
        lift_real_input(zero_padded_copy(apply_window(frame, window), fft_size));

      if (pool != nullptr and allow_plan_parallelism)
        plan.ptransform(*pool, spectrum, false, chunk_size);
      else
        plan.transform(spectrum, false);

      return spectrum;
    }

    [[nodiscard]] static Array<Array<Complex>>
    transform_stft_frames(const Array<Array<Real>> & frames,
                          const Array<Real> & window,
                          const size_t fft_size,
                          const Plan & plan,
                          ThreadPool * pool,
                          const size_t chunk_size)
    {
      if (frames.is_empty())
        return {};

      Array<Array<Complex>> spectrogram(frames.size(), Array<Complex>());
      const bool parallelize_across_frames =
        pool != nullptr and pool->num_threads() > 1 and frames.size() > 1;

      auto transform_one = [&frames, &window, fft_size, &plan, pool, chunk_size,
                            parallelize_across_frames, &spectrogram](const size_t i)
      {
        spectrogram(i) = transform_stft_frame(frames[i],
                                              window,
                                              fft_size,
                                              plan,
                                              pool,
                                              chunk_size,
                                              not parallelize_across_frames);
      };

      if (parallelize_across_frames)
        parallel_for_index(*pool, 0, frames.size(), transform_one, chunk_size);
      else
        for (size_t i = 0; i < frames.size(); ++i)
          transform_one(i);

      return spectrogram;
    }

    [[nodiscard]] static Array<Array<Complex>>
    stft_impl(const Array<Real> & signal,
              const Array<Real> & window,
              const STFTOptions & options,
              ThreadPool * pool = nullptr,
              const size_t chunk_size = 0)
    {
      if (signal.is_empty())
        return {};

      const size_t fft_size =
        validate_stft_options(window, options, "FFT::stft");

      const size_t center_pad = options.centered ? window.size() / 2 : 0;
      const Array<Real> prepared_signal = options.centered
        ? zero_pad_edges(signal, center_pad, center_pad)
        : signal;
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
                         const char * ctx)
    {
      const IIRCoefficients coeffs =
        normalize_iir_coefficients(numerator, denominator, ctx);
      Complex num(Real(0), Real(0));
      Complex den(Real(0), Real(0));

      for (size_t k = 0; k < coeffs.numerator.size(); ++k)
        num += coeffs.numerator[k]
               * std::polar(Real(1), -omega * static_cast<Real>(k));
      for (size_t k = 0; k < coeffs.denominator.size(); ++k)
        den += coeffs.denominator[k]
               * std::polar(Real(1), -omega * static_cast<Real>(k));

      const Real den_tol = (max_abs_value(coeffs.denominator) + Real(1))
                           * Real(128) * std::numeric_limits<Real>::epsilon();
      ah_domain_error_if(std::abs(den) <= den_tol)
        << ctx << ": transfer response is singular at omega=" << omega;

      return num / den;
    }

    [[nodiscard]] static FrequencyResponse
    freqz_impl(const Array<Real> & numerator,
               const Array<Real> & denominator,
               const size_t num_points,
               const bool whole,
               const char * ctx)
    {
      ah_invalid_argument_if(num_points == 0)
        << ctx << ": number of frequency samples must be positive";

      FrequencyResponse output;
      output.omega = Array<Real>::create(num_points);
      output.response = Array<Complex>::create(num_points);

      const Real max_omega = whole
        ? Real(2) * std::numbers::pi_v<Real>
        : std::numbers::pi_v<Real>;
      const Real denom = whole
        ? static_cast<Real>(num_points)
        : static_cast<Real>(std::max(size_t(1), num_points - 1));

      for (size_t i = 0; i < num_points; ++i)
        {
          const Real omega = num_points == 1
            ? Real(0)
            : max_omega * static_cast<Real>(i) / denom;
          output.omega(i) = omega;
          output.response(i) = evaluate_transfer_at(numerator,
                                                    denominator,
                                                    omega,
                                                    ctx);
        }

      return output;
    }

    [[nodiscard]] static Array<Complex>
    polynomial_roots_impl(const Array<Real> & coefficients,
                          const char * ctx)
    {
      const size_t n = effective_coeff_length(coefficients);
      if (n <= 1)
        return {};

      const Real leading = coefficients[0];
      const Real tol = (max_abs_value(coefficients) + Real(1))
                       * Real(1024) * std::numeric_limits<Real>::epsilon();
      ah_domain_error_if(std::abs(leading) <= tol)
        << ctx << ": leading polynomial coefficient must be non-zero";

      Array<Complex> poly = Array<Complex>::create(n);
      for (size_t i = 0; i < n; ++i)
        poly(i) = Complex(coefficients[i] / leading, Real(0));

      const size_t degree = n - 1;
      Real radius = Real(1);
      for (size_t i = 1; i < n; ++i)
        radius = std::max(radius, Real(1) + std::abs(poly[i]));

      Array<Complex> roots = Array<Complex>::create(degree);
      for (size_t i = 0; i < degree; ++i)
        {
          const Real angle = Real(2) * std::numbers::pi_v<Real>
                             * static_cast<Real>(i) / static_cast<Real>(degree);
          roots(i) = std::polar(radius, angle);
        }

      bool converged = false;
      for (size_t iter = 0; iter < 128; ++iter)
        {
          converged = true;
          for (size_t i = 0; i < degree; ++i)
            {
              Complex value = poly[0];
              for (size_t k = 1; k < poly.size(); ++k)
                value = value * roots[i] + poly[k];

              Complex denom(Real(1), Real(0));
              for (size_t j = 0; j < degree; ++j)
                if (j != i)
                  denom *= roots[i] - roots[j];

              ah_domain_error_if(std::abs(denom) <= tol)
                << ctx << ": root solver encountered a degenerate iteration";

              const Complex delta = value / denom;
              roots(i) -= delta;
              if (std::abs(delta)
                  > tol * (Real(1) + std::abs(roots[i])))
                converged = false;
            }

          if (converged)
            break;
        }

      ah_domain_error_if(not converged)
        << ctx << ": root solver did not converge";

      return roots;
    }

    [[nodiscard]] static Array<Real>
    trim_leading_zeros_copy(const Array<Real> & input) noexcept
    {
      if (input.is_empty())
        return {};

      const Real tol = (max_abs_value(input) + Real(1))
                       * Real(64) * std::numeric_limits<Real>::epsilon();
      size_t first = 0;
      while (first < input.size() and std::abs(input[first]) <= tol)
        ++first;

      if (first == input.size())
        return {};

      Array<Real> trimmed;
      trimmed.reserve(input.size() - first);
      for (size_t i = first; i < input.size(); ++i)
        trimmed.append(input[i]);
      return trimmed;
    }

    struct TransferTerms
    {
      Complex numerator = Complex(Real(0), Real(0));
      Complex denominator = Complex(Real(0), Real(0));
      Complex numerator_derivative = Complex(Real(0), Real(0));
      Complex denominator_derivative = Complex(Real(0), Real(0));
    };

    [[nodiscard]] static TransferTerms
    evaluate_transfer_terms_at(const Array<Real> & numerator,
                               const Array<Real> & denominator,
                               const Real omega,
                               const char * ctx)
    {
      const IIRCoefficients coeffs =
        normalize_iir_coefficients(numerator, denominator, ctx);
      TransferTerms terms;

      for (size_t k = 0; k < coeffs.numerator.size(); ++k)
        {
          const Complex z = std::polar(Real(1), -omega * static_cast<Real>(k));
          const Complex dw(Real(0), -static_cast<Real>(k));
          terms.numerator += coeffs.numerator[k] * z;
          terms.numerator_derivative += coeffs.numerator[k] * dw * z;
        }

      for (size_t k = 0; k < coeffs.denominator.size(); ++k)
        {
          const Complex z = std::polar(Real(1), -omega * static_cast<Real>(k));
          const Complex dw(Real(0), -static_cast<Real>(k));
          terms.denominator += coeffs.denominator[k] * z;
          terms.denominator_derivative += coeffs.denominator[k] * dw * z;
        }

      const Real den_tol = (max_abs_value(coeffs.denominator) + Real(1))
                           * Real(128) * std::numeric_limits<Real>::epsilon();
      ah_domain_error_if(std::abs(terms.denominator) <= den_tol)
        << ctx << ": transfer response is singular at omega=" << omega;

      return terms;
    }

    [[nodiscard]] static Array<Real>
    analytic_group_delay_impl(const Array<Real> & numerator,
                              const Array<Real> & denominator,
                              const size_t num_points,
                              const bool whole,
                              const char * ctx)
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
          if (std::abs(terms.numerator) <= tol)
            delay(i) = numeric_fallback[i];
          else
            delay(i) = -std::imag(terms.numerator_derivative / terms.numerator
                                  - terms.denominator_derivative
                                    / terms.denominator);
        }

      return delay;
    }

    [[nodiscard]] static Array<Real>
    analytic_phase_delay_impl(const Array<Real> & numerator,
                              const Array<Real> & denominator,
                              const size_t num_points,
                              const bool whole,
                              const char * ctx)
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
        delay(i) = std::abs(response.omega[i]) <= tol
          ? group[i]
          : -phase[i] / response.omega[i];

      return delay;
    }

    [[nodiscard]] static Real
    max_root_radius(const Array<Complex> & roots) noexcept
    {
      Real radius = Real(0);
      for (size_t i = 0; i < roots.size(); ++i)
        radius = std::max(radius, static_cast<Real>(std::abs(roots[i])));
      return radius;
    }

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

    [[nodiscard]] static Real
    minimum_cancellation_distance_impl(const Array<PoleZeroPair> & pairs) noexcept
    {
      Real best = std::numeric_limits<Real>::infinity();
      for (size_t i = 0; i < pairs.size(); ++i)
        if (pairs[i].has_zero and pairs[i].has_pole)
          best = std::min(best, pairs[i].distance);
      return best;
    }

    struct RootGroup
    {
      Array<Complex> roots;
      Complex center = Complex(Real(0), Real(0));
    };

    struct AnalogPrototype
    {
      Array<Complex> poles;
      Real gain = Real(1);
    };

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

    [[nodiscard]] static Real
    integer_power(const Real base, const size_t exponent) noexcept
    {
      Real result = Real(1);
      for (size_t i = 0; i < exponent; ++i)
        result *= base;
      return result;
    }

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

    [[nodiscard]] static Array<Real>
    bilinear_substitute_polynomial(const Array<Real> & analog,
                                   const size_t order,
                                   const Real sample_rate,
                                   const char * ctx)
    {
      ah_invalid_argument_if(analog.is_empty())
        << ctx << ": analog polynomial must be non-empty";
      ah_invalid_argument_if(sample_rate <= Real(0))
        << ctx << ": sample rate " << sample_rate << " must be positive";

      const size_t degree = analog.size() - 1;
      ah_invalid_argument_if(degree > order)
        << ctx << ": analog degree " << degree
        << " exceeds requested bilinear order " << order;

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

    [[nodiscard]] static Array<Real>
    real_polynomial_from_roots(const Array<Complex> & roots,
                               const char * ctx)
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

    [[nodiscard]] static Array<RootGroup>
    root_groups_impl(const Array<Complex> & roots,
                     const char * ctx)
    {
      if (roots.is_empty())
        return {};

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
            << ctx << ": complex root " << roots[i]
            << " does not have a matching conjugate";

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
            << ctx << ": root " << roots[i]
            << " was not paired into a real section";
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

    [[nodiscard]] static BiquadSection
    section_from_coefficients(const Array<Real> & numerator,
                              const Array<Real> & denominator,
                              const char * ctx)
    {
      const IIRCoefficients coeffs =
        normalize_iir_coefficients(numerator, denominator, ctx);
      ah_invalid_argument_if(coeffs.denominator.size() > 3)
        << ctx << ": section denominator order "
        << (coeffs.denominator.size() - 1)
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

    [[nodiscard]] static IIRCoefficients
    bilinear_transform_impl(const Array<Real> & analog_numerator,
                            const Array<Real> & analog_denominator,
                            const Real sample_rate,
                            const char * ctx)
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

    [[nodiscard]] static Real
    prewarp_frequency(const Real cutoff_frequency,
                      const Real sample_rate,
                      const char * ctx)
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

    [[nodiscard]] static AnalogPrototype
    butterworth_prototype(const size_t order,
                          const char * ctx)
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
                         const char * ctx)
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

    [[nodiscard]] static Array<BiquadSection>
    design_prototype_sections(const AnalogPrototype & prototype,
                              const size_t order,
                              const Real cutoff_frequency,
                              const Real sample_rate,
                              const bool highpass,
                              const char * ctx)
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
      const Real first_section_gain = highpass
        ? prototype.gain / prototype_constant
        : prototype.gain * integer_power(warped_cutoff, order);

      Array<Complex> transformed_poles = Array<Complex>::create(order);
      for (size_t i = 0; i < order; ++i)
        transformed_poles(i) = highpass
          ? Complex(warped_cutoff, Real(0)) / prototype.poles[i]
          : Complex(warped_cutoff, Real(0)) * prototype.poles[i];

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
          const long long first_target =
            static_cast<long long>(std::ceil((-upper - std::numbers::pi_v<Real>)
                                             / (Real(2)
                                                * std::numbers::pi_v<Real>)));
          const long long last_target =
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
              const Real ratio = std::abs(mag) <= std::numeric_limits<Real>::epsilon()
                ? std::numeric_limits<Real>::infinity()
                : Real(1) / std::abs(mag);
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
          info.decibels = std::isinf(best_ratio)
            ? std::numeric_limits<Real>::infinity()
            : Real(20) * std::log10(best_ratio);
          info.crossover_omega = best_omega;
          info.found = true;
        }

      return info;
    }

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
        delay(i) = std::abs(response.omega[i]) <= tol
          ? group[i]
          : -phase[i] / response.omega[i];

      return delay;
    }

    static void
    validate_istft_configuration(const Array<Real> & analysis_window,
                                 const Array<Real> & synthesis_window,
                                 const size_t fft_size,
                                 const ISTFTOptions & options,
                                 const char * ctx)
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
      ah_invalid_argument_if(not is_power_of_two(fft_size))
        << ctx << ": FFT size " << fft_size << " must be a power of two";

      validate_overlap_constraints(analysis_window,
                                   synthesis_window,
                                   options.hop_size,
                                   options.validate_nola,
                                   options.validate_cola,
                                   ctx);
    }

    [[nodiscard]] static Array<Real>
    istft_impl(const Array<Array<Complex>> & spectrogram,
               const Array<Real> & analysis_window,
               const Array<Real> & synthesis_window,
               const ISTFTOptions & options,
               ThreadPool * pool,
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
      const size_t output_length = options.signal_length == 0
        ? (options.centered and raw_length >= 2 * center_pad
            ? raw_length - 2 * center_pad
            : raw_length)
        : options.signal_length;
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
          const Array<Real> frame = (pool != nullptr)
            ? plan.pinverse_transform_real(*pool, spectrogram[frame_idx], chunk_size)
            : plan.inverse_transform_real(spectrogram[frame_idx]);
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

      return options.centered
        ? slice_copy(normalized, center_pad, output_length)
        : slice_copy(normalized, 0, output_length);
    }

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
      requires requires (const Container & c)
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

    [[nodiscard]] static Array<Complex>
    transform_real_optimized(const Array<Real> & input,
                             ThreadPool * pool = nullptr,
                             const size_t chunk_size = 0)
    {
      const size_t n = input.size();
      ah_invalid_argument_if(n == 0)
        << "FFT::transform: input must be non-empty";

      if (n == 1)
        return {Complex(input[0], Real(0))};

      ah_invalid_argument_if(not is_power_of_two(n))
        << "FFT::transform: input size " << n << " must be a power of two";

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

    template <typename InversePackedTransform>
    [[nodiscard]] static Array<Real>
    inverse_transform_real_optimized_impl(const Array<Complex> & input,
                                          const char * ctx,
                                          ThreadPool * pool,
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

    [[nodiscard]] static Array<Real>
    multiply_real_optimized(const Array<Real> & a, const Array<Real> & b,
                            ThreadPool * pool = nullptr,
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

    [[nodiscard]] static Array<Complex>
    multiply_complex_impl(const Array<Complex> & a, const Array<Complex> & b,
                          ThreadPool * pool = nullptr,
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
    project_real_output(const Array<Complex> & input, const char * ctx)
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

    static void
    transform_impl(Array<Complex> & a, const bool invert,
                   ThreadPool * pool = nullptr,
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
                const Complex js = invert
                  ? Complex(-s.imag(), s.real())
                  : Complex(s.imag(), -s.real());

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

    /** @brief Precomputed FFT plan for repeated transforms of the same size.
     *
     *  A Plan precomputes the bit-reversal permutation table and all twiddle
     *  factors for a given transform size N (which must be a power of two).
     *  Subsequent calls to @ref transform reuse these tables, avoiding the
     *  cost of recomputing @c std::polar at every butterfly stage.
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
      size_t n_ = 0;
      size_t log_n_ = 0;
      Array<size_t> bit_rev_;
      Array<Complex> twiddles_;
      std::shared_ptr<const Plan> half_plan_;

      void apply_bit_reversal(Array<Complex> & a) const noexcept
      {
        for (size_t i = 0; i < n_; ++i)
          if (i < bit_rev_(i))
            std::swap(a(i), a(bit_rev_(i)));
      }

      void apply_butterflies(Array<Complex> & a, const bool invert,
                             ThreadPool * pool,
                             const size_t chunk_size) const
      {
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
            const size_t quarter = size_t(1) << s_low;
            const size_t blocks = n_ >> (s_high + 1);
            const size_t tw_w1_off = (size_t(1) << s_high) - 1;
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

                  const Complex js = invert
                    ? Complex(-s.imag(), s.real())
                    : Complex(s.imag(), -s.real());

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

    public:
      /** @brief Constructs an empty plan (size 0). */
      Plan() = default;

      /** @brief Constructs a plan for transforms of size @p n.
       *
       *  Precomputes the bit-reversal permutation and all twiddle factors.
       *  Construction cost is O(N).
       *
       *  @param n Transform size (must be a power of two, or 1).
       *  @throw std::invalid_argument if n is zero or not a power of two.
       */
      explicit Plan(const size_t n) : n_(n)
      {
        ah_invalid_argument_if(n == 0)
          << "FFT::Plan: size must be positive";

        if (n == 1)
          return;

        ah_invalid_argument_if(not is_power_of_two(n))
          << "FFT::Plan: size " << n << " must be a power of two";

        log_n_ = 0;
        for (size_t tmp = n; tmp > 1; tmp >>= 1)
          ++log_n_;

        bit_rev_ = Array<size_t>::create(n);
        bit_rev_(0) = 0;
        for (size_t i = 1, j = 0; i < n; ++i)
          {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
              j ^= bit;
            j ^= bit;
            bit_rev_(i) = j;
          }

        twiddles_ = Array<Complex>::create(n - 1);
        for (size_t stage = 0; stage < log_n_; ++stage)
          {
            const size_t half = size_t(1) << stage;
            const size_t len = half << 1;
            const Real angle = Real(-2) * std::numbers::pi_v<Real>
                               / static_cast<Real>(len);
            const size_t offset = half - 1;
            for (size_t j = 0; j < half; ++j)
              twiddles_(offset + j) = std::polar(Real(1),
                                                 angle * static_cast<Real>(j));
          }

        half_plan_ = std::make_shared<Plan>(n / 2);
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
        ah_invalid_argument_if(a.size() != n_)
          << "FFT::Plan::transform: input size " << a.size()
          << " does not match plan size " << n_;

        if (n_ <= 1)
          return;

        apply_bit_reversal(a);
        apply_butterflies(a, invert, nullptr, 0);
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

        return FFT::inverse_transform_real_optimized_impl(
            input, "FFT::Plan::inverse_transform_real", nullptr, 0,
            [this](Array<Complex> & packed, ThreadPool *, const size_t)
            {
              half_plan_->transform(packed, true);
            });
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

        return FFT::inverse_transform_real_optimized_impl(
            input, "FFT::Plan::inverse_transform_real", &pool, chunk_size,
            [this](Array<Complex> & packed, ThreadPool * inner_pool,
                   const size_t inner_chunk_size)
            {
              if (inner_pool != nullptr)
                half_plan_->ptransform(*inner_pool, packed, true,
                                       inner_chunk_size);
              else
                half_plan_->transform(packed, true);
            });
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
        ah_invalid_argument_if(a.size() != n_)
          << "FFT::Plan::ptransform: input size " << a.size()
          << " does not match plan size " << n_;

        if (n_ <= 1)
          return;

        apply_bit_reversal(a);
        apply_butterflies(a, invert, &pool, chunk_size);
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
    };

    /** @brief Computes the Fast Fourier Transform (FFT) in-place.
     *
     *  Implements a mixed radix-2/radix-4 iterative Cooley-Tukey algorithm.
     *  The input array 'a' must have a size that is a power of two.
     *
     *  If 'invert' is true, it computes the Inverse Fast Fourier Transform (IFFT),
     *  including the 1/N scaling factor.
     *
     *  @param a The array of complex values to transform. Modified in-place.
     *  @param invert Whether to perform the inverse transform.
     *  @throw std::invalid_argument if the size of 'a' is zero or not a power of two.
     */
    static void
    transform(Array<Complex> & a, const bool invert)
    {
      transform_impl(a, invert);
    }

    static void
    ptransform(ThreadPool & pool, Array<Complex> & a, const bool invert,
               const size_t chunk_size = 0)
    {
      transform_impl(a, invert, &pool, chunk_size);
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
     *  @throw std::invalid_argument if the input size is not a power of two.
     */
    [[nodiscard]] static Array<Complex>
    transformed(const Array<Complex> & input, const bool invert = false)
    {
      Array<Complex> output = input;
      transform(output, invert);
      return output;
    }

    [[nodiscard]] static Array<Complex>
    ptransformed(ThreadPool & pool, const Array<Complex> & input,
                 const bool invert = false, const size_t chunk_size = 0)
    {
      Array<Complex> output = input;
      ptransform(pool, output, invert, chunk_size);
      return output;
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

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform_padded(const Container & input)
    {
      return transform_padded(build_complex_input(input));
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform_padded(ThreadPool & pool, const Container & input,
                      const size_t chunk_size = 0)
    {
      return ptransform_padded(pool, build_complex_input(input), chunk_size);
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    transformed(const Container & input, const bool invert = false)
    {
      return transformed(build_complex_input(input), invert);
    }

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
     *  @throw std::invalid_argument if the input size is not a power of two.
     */
    [[nodiscard]] static Array<Complex>
    transform(const Array<Real> & input)
    {
      return transform_real_optimized(input);
    }

    [[nodiscard]] static Array<Complex>
    ptransform(ThreadPool & pool, const Array<Real> & input,
               const size_t chunk_size = 0)
    {
      return transform_real_optimized(input, &pool, chunk_size);
    }

    [[nodiscard]] static Array<Complex>
    transform_padded(const Array<Real> & input)
    {
      return transform(zero_padded_copy(input, next_power_of_two(input.size())));
    }

    [[nodiscard]] static Array<Complex>
    ptransform_padded(ThreadPool & pool, const Array<Real> & input,
                      const size_t chunk_size = 0)
    {
      return ptransform(pool, zero_padded_copy(input, next_power_of_two(input.size())),
                        chunk_size);
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform(const Container & input)
    {
      return transformed(input, false);
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform(ThreadPool & pool, const Container & input,
               const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, false, chunk_size);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform_padded(const Container & input)
    {
      return transform_padded(build_real_input(input));
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform_padded(ThreadPool & pool, const Container & input,
                      const size_t chunk_size = 0)
    {
      return ptransform_padded(pool, build_real_input(input), chunk_size);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    transform(const Container & input)
    {
      return transform(build_real_input(input));
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    ptransform(ThreadPool & pool, const Container & input,
               const size_t chunk_size = 0)
    {
      return ptransform(pool, build_real_input(input), chunk_size);
    }

    /** @brief DSP alias for the forward FFT. */
    [[nodiscard]] static Array<Complex>
    spectrum(const Array<Complex> & input)
    {
      return transformed(input, false);
    }

    [[nodiscard]] static Array<Complex>
    pspectrum(ThreadPool & pool, const Array<Complex> & input,
              const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, false, chunk_size);
    }

    [[nodiscard]] static Array<Complex>
    spectrum(const Array<Real> & input)
    {
      return transform(input);
    }

    [[nodiscard]] static Array<Complex>
    pspectrum(ThreadPool & pool, const Array<Real> & input,
              const size_t chunk_size = 0)
    {
      return ptransform(pool, input, chunk_size);
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    spectrum(const Container & input)
    {
      return transformed(input, false);
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    pspectrum(ThreadPool & pool, const Container & input,
              const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, false, chunk_size);
    }

    template <typename Container>
      requires Is_Real_Container<Container>
    [[nodiscard]] static Array<Complex>
    spectrum(const Container & input)
    {
      return transform(input);
    }

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
     *  @throw std::invalid_argument if the input size is not a power of two.
     */
    [[nodiscard]] static Array<Complex>
    inverse_transform(const Array<Complex> & input)
    {
      return transformed(input, true);
    }

    [[nodiscard]] static Array<Complex>
    pinverse_transform(ThreadPool & pool, const Array<Complex> & input,
                       const size_t chunk_size = 0)
    {
      return ptransformed(pool, input, true, chunk_size);
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Complex>
    inverse_transform(const Container & input)
    {
      return transformed(input, true);
    }

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
     *  @throw std::invalid_argument if the input size is not a power of two.
     *  @throw std::domain_error if the result has non-negligible imaginary parts.
     */
    [[nodiscard]] static Array<Real>
    inverse_transform_real(const Array<Complex> & input)
    {
      return inverse_transform_real_optimized_impl(
          input, "FFT::inverse_transform_real", nullptr, 0,
          [](Array<Complex> & packed, ThreadPool * inner_pool,
             const size_t inner_chunk_size)
          {
            transform_impl(packed, true, inner_pool, inner_chunk_size);
          });
    }

    [[nodiscard]] static Array<Real>
    pinverse_transform_real(ThreadPool & pool, const Array<Complex> & input,
                            const size_t chunk_size = 0)
    {
      return inverse_transform_real_optimized_impl(
          input, "FFT::inverse_transform_real", &pool, chunk_size,
          [](Array<Complex> & packed, ThreadPool * inner_pool,
             const size_t inner_chunk_size)
          {
            transform_impl(packed, true, inner_pool, inner_chunk_size);
          });
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    inverse_transform_real(const Container & input)
    {
      return inverse_transform_real(build_complex_input(input));
    }

    template <typename Container>
      requires Is_Complex_Container<Container>
    [[nodiscard]] static Array<Real>
    pinverse_transform_real(ThreadPool & pool, const Container & input,
                            const size_t chunk_size = 0)
    {
      return pinverse_transform_real(pool, build_complex_input(input), chunk_size);
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
     *  Otherwise only complete frames are returned.
     */
    [[nodiscard]] static Array<Array<Real>>
    frame_signal(const Array<Real> & signal,
                 const size_t frame_size,
                 const size_t hop_size,
                 const bool pad_end = true)
    {
      ah_invalid_argument_if(frame_size == 0)
        << "FFT::frame_signal: frame size must be positive";
      ah_invalid_argument_if(hop_size == 0)
        << "FFT::frame_signal: hop size must be positive";

      Array<Array<Real>> frames;
      if (signal.is_empty())
        return frames;

      for (size_t offset = 0; offset < signal.size(); offset += hop_size)
        {
          const size_t length = std::min(frame_size, signal.size() - offset);
          if (not pad_end and length < frame_size)
            break;

          Array<Real> frame = Array<Real>::create(frame_size);
          for (size_t i = 0; i < frame_size; ++i)
            frame(i) = Real(0);
          for (size_t i = 0; i < length; ++i)
            frame(i) = signal[offset + i];

          frames.append(frame);

          if (not pad_end and offset + frame_size >= signal.size())
            break;
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
      require_configured(const char * ctx) const
      {
        ah_runtime_error_if(not configured_)
          << ctx << ": processor is not configured";
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
      emit_ready_frames(ThreadPool * pool,
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
      STFTProcessor() = default;

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

      explicit STFTProcessor(const size_t frame_size,
                             const STFTOptions & options)
        : STFTProcessor(FFT::hann_window(frame_size), options)
      {
      }

      template <typename WindowContainer>
        requires Is_Real_Container<WindowContainer>
      explicit STFTProcessor(const WindowContainer & window,
                             const STFTOptions & options)
        : STFTProcessor(FFT::build_real_input(window), options)
      {
      }

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

      [[nodiscard]] const Array<Real> & window() const noexcept
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
      require_configured(const char * ctx) const
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

        size_t start = std::min(left_trim_remaining_, normalized.size());
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
                         ThreadPool * pool,
                         const size_t chunk_size)
      {
        require_configured("FFT::ISTFTProcessor::process_frame");
        ah_runtime_error_if(finalized_)
          << "FFT::ISTFTProcessor::process_frame: processor already flushed";
        ah_invalid_argument_if(spectrum.size() != fft_size_)
          << "FFT::ISTFTProcessor::process_frame: frame size " << spectrum.size()
          << " does not match configured FFT size " << fft_size_;

        has_seen_frame_ = true;
        const Array<Real> frame = (pool != nullptr)
          ? plan_.pinverse_transform_real(*pool, spectrum, chunk_size)
          : plan_.inverse_transform_real(spectrum);
        accumulate_frame(frame);
        return drain_ready_samples(false);
      }

    public:
      ISTFTProcessor() = default;

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

      explicit ISTFTProcessor(const size_t fft_size,
                              const Array<Real> & window,
                              const ISTFTOptions & options)
        : ISTFTProcessor(fft_size, window, window, options)
      {
      }

      explicit ISTFTProcessor(const size_t fft_size,
                              const size_t frame_size,
                              const ISTFTOptions & options)
        : ISTFTProcessor(fft_size,
                         FFT::hann_window(frame_size),
                         FFT::hann_window(frame_size),
                         options)
      {
      }

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
      {
      }

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
      validate_batch_size(const size_t size, const char * ctx) const
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
      {
      }

      template <typename WindowContainer>
        requires Is_Real_Container<WindowContainer>
      explicit BatchedSTFTProcessor(const size_t num_channels,
                                    const WindowContainer & window,
                                    const STFTOptions & options)
        : BatchedSTFTProcessor(num_channels,
                               FFT::build_real_input(window),
                               options)
      {
      }

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
      validate_batch_size(const size_t size, const char * ctx) const
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
      {
      }

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
      {
      }

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
      {
      }

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
      {
      }

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
      {
      }

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
      {
      }

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
      {
      }

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
      require_configured(const char * ctx) const
      {
        ah_runtime_error_if(not configured_)
          << ctx << ": filter is not configured";
      }

      void
      initialize_state()
      {
        const size_t order = coeffs_.denominator.size() == 0
          ? 0
          : coeffs_.denominator.size() - 1;
        state_ = Array<Real>::create(order);
        for (size_t i = 0; i < order; ++i)
          state_(i) = Real(0);
      }

    public:
      LFilter() = default;

      explicit LFilter(const Array<Real> & numerator,
                       const Array<Real> & denominator)
        : coeffs_(FFT::normalize_iir_coefficients(numerator,
                                                  denominator,
                                                  "FFT::LFilter")),
          configured_(true)
      {
        initialize_state();
      }

      explicit LFilter(const IIRCoefficients & coeffs)
        : LFilter(coeffs.numerator, coeffs.denominator)
      {
      }

      explicit LFilter(const BiquadSection & section)
        : LFilter(section.numerator(), section.denominator())
      {
      }

      template <typename NumContainer, typename DenContainer>
        requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
      explicit LFilter(const NumContainer & numerator,
                       const DenContainer & denominator)
        : LFilter(build_real_input(numerator), build_real_input(denominator))
      {
      }

      [[nodiscard]] size_t order() const noexcept
      {
        return coeffs_.denominator.is_empty() ? 0 : coeffs_.denominator.size() - 1;
      }

      [[nodiscard]] bool configured() const noexcept
      {
        return configured_;
      }

      [[nodiscard]] const IIRCoefficients & coefficients() const noexcept
      {
        return coeffs_;
      }

      [[nodiscard]] const Array<Real> & state() const noexcept
      {
        return state_;
      }

      void reset()
      {
        require_configured("FFT::LFilter::reset");
        for (size_t i = 0; i < state_.size(); ++i)
          state_(i) = Real(0);
      }

      void set_state(const Array<Real> & new_state)
      {
        require_configured("FFT::LFilter::set_state");
        ah_invalid_argument_if(new_state.size() != state_.size())
          << "FFT::LFilter::set_state: state size " << new_state.size()
          << " does not match filter order " << state_.size();
        state_ = new_state;
      }

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
      require_configured(const char * ctx) const
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
      SOSFilter() = default;

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

      template <typename SectionsContainer>
        requires Is_Biquad_Container<SectionsContainer>
      explicit SOSFilter(const SectionsContainer & sections)
        : SOSFilter(to_array<BiquadSection>(sections))
      {
      }

      [[nodiscard]] size_t num_sections() const noexcept
      {
        return sections_.size();
      }

      [[nodiscard]] bool configured() const noexcept
      {
        return configured_;
      }

      [[nodiscard]] const Array<Real> &
      state(const size_t index) const
      {
        ah_out_of_range_error_if(index >= states_.size())
          << "FFT::SOSFilter::state: index " << index
          << " out of range for " << states_.size() << " sections";
        return states_[index];
      }

      void reset()
      {
        require_configured("FFT::SOSFilter::reset");
        for (size_t i = 0; i < states_.size(); ++i)
          for (size_t j = 0; j < states_[i].size(); ++j)
            states_(i)(j) = Real(0);
      }

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

      void
      validate_channel_count(const size_t count, const char * ctx) const
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
      {
      }

      explicit LFilterBank(const size_t num_channels,
                           const BiquadSection & section)
        : LFilterBank(num_channels, section.numerator(), section.denominator())
      {
      }

      template <typename NumContainer, typename DenContainer>
        requires Is_Real_Container<NumContainer> and Is_Real_Container<DenContainer>
      explicit LFilterBank(const size_t num_channels,
                           const NumContainer & numerator,
                           const DenContainer & denominator)
        : LFilterBank(num_channels,
                      FFT::build_real_input(numerator),
                      FFT::build_real_input(denominator))
      {
      }

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

      void
      validate_channel_count(const size_t count, const char * ctx) const
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
      {
      }

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
                                    initial_states.is_empty()
                                      ? Array<Real>()
                                      : initial_states[i],
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
                                                       initial_states.is_empty()
                                                         ? Array<Real>()
                                                         : initial_states[i],
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
      for (const auto & signal : signals)
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
      for (const auto & signal : signals)
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

      const Real max_omega = whole
        ? Real(2) * std::numbers::pi_v<Real>
        : std::numbers::pi_v<Real>;
      const Real denom = whole
        ? static_cast<Real>(num_points)
        : static_cast<Real>(std::max(size_t(1), num_points - 1));

      for (size_t i = 0; i < num_points; ++i)
        {
          const Real omega = num_points == 1
            ? Real(0)
            : max_omega * static_cast<Real>(i) / denom;
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
      return polynomial_roots_impl(trimmed, "FFT::zeros");
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
      return Real(1) - max_root_radius(poles(sections));
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
      return group_delay(freqz(sections, num_points, whole));
    }

    [[nodiscard]] static Array<Real>
    phase_delay(const Array<BiquadSection> & sections,
                const size_t num_points = 512,
                const bool whole = false)
    {
      return phase_delay(freqz(sections, num_points, whole));
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
      return phase_margin(freqz(numerator, denominator, num_points, whole));
    }

    [[nodiscard]] static GainMarginInfo
    gain_margin(const Array<Real> & numerator,
                const Array<Real> & denominator,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      return gain_margin(freqz(numerator, denominator, num_points, whole));
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
      return phase_margin(freqz(sections, num_points, whole));
    }

    [[nodiscard]] static GainMarginInfo
    gain_margin(const Array<BiquadSection> & sections,
                const size_t num_points = 1024,
                const bool whole = false)
    {
      return gain_margin(freqz(sections, num_points, whole));
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

      void
      pointwise_multiply(Array<Complex> & spectrum,
                         ThreadPool * pool,
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

      void
      clear_overlap()
      {
        for (size_t i = 0; i < overlap_.size(); ++i)
          overlap_(i) = Real(0);
        has_pending_tail_ = false;
      }

      [[nodiscard]] Array<Real>
      process_chunk_impl(const Array<Real> & block,
                         ThreadPool * pool,
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

        const Array<Real> convolved = (pool != nullptr)
          ? plan_.pinverse_transform_real(*pool, spectrum, chunk_size)
          : plan_.inverse_transform_real(spectrum);

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
                const Real from_current = src < convolved.size()
                  ? convolved[src] : Real(0);
                const Real from_previous = src < overlap_.size()
                  ? overlap_[src] : Real(0);
                next_overlap(i) = from_current + from_previous;
              }
            overlap_ = next_overlap;
          }

        has_pending_tail_ = true;
        return emitted;
      }

      [[nodiscard]] Array<Real>
      convolve_impl(const Array<Real> & signal,
                    ThreadPool * pool,
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

            const Array<Real> block = (pool != nullptr)
              ? plan_.pinverse_transform_real(*pool, spectrum, chunk_size)
              : plan_.inverse_transform_real(spectrum);

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

        block_size_ = block_size == 0 ? default_block_size(kernel_.size())
                                      : block_size;
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
      [[nodiscard]] const Array<Real> & kernel() const noexcept { return kernel_; }
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

    [[nodiscard]] static Array<Complex>
    pmultiply(ThreadPool & pool, const Array<Complex> & a, const Array<Complex> & b,
              const size_t chunk_size = 0)
    {
      return multiply_complex_impl(a, b, &pool, chunk_size);
    }

    template <typename Container1, typename Container2>
      requires Is_Complex_Container<Container1> and Is_Complex_Container<Container2>
    [[nodiscard]] static Array<Complex>
    multiply(const Container1 & a, const Container2 & b)
    {
      return multiply(build_complex_input(a), build_complex_input(b));
    }

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

    [[nodiscard]] static Array<Real>
    pmultiply(ThreadPool & pool, const Array<Real> & a, const Array<Real> & b,
              const size_t chunk_size = 0)
    {
      return multiply_real_optimized(a, b, &pool, chunk_size);
    }

    template <typename Container1, typename Container2>
      requires Is_Real_Container<Container1> and Is_Real_Container<Container2>
    [[nodiscard]] static Array<Real>
    multiply(const Container1 & a, const Container2 & b)
    {
      return multiply(build_real_input(a), build_real_input(b));
    }

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
