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
 *  Provides an iterative radix-2 Cooley-Tukey FFT with support for:
 *  - Forward and inverse transforms over complex arrays
 *  - Real input sequences lifted automatically to the complex domain
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
# include <numbers>
# include <type_traits>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Fast Fourier Transform (FFT) implementation.
   *
   *  The FFT class provides a template-based implementation of the Fast Fourier
   *  Transform using the iterative radix-2 Cooley-Tukey algorithm. This
   *  implementation supports:
   *  - Forward and inverse transforms for complex and real-valued sequences.
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

  private:
    template <typename T>
    struct Is_Std_Complex : std::false_type
    {
    };

    template <typename T>
    struct Is_Std_Complex<std::complex<T>> : std::true_type
    {
    };

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
      requires requires (const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      }
    [[nodiscard]] static Array<Complex>
    build_complex_input(const Container & input)
    {
      return to_array<Complex>(input);
    }

    template <typename Container>
      requires requires (const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires std::convertible_to<decltype(*std::begin(c)), Real>;
        requires not Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      }
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

      for (size_t i = 0; i < input.size(); ++i)
        {
          const Real imag = std::abs(input[i].imag());
          const Real scale = std::abs(input[i].real()) + std::abs(input[i].imag()) + Real(1);
          const Real tolerance = Real(128) * std::numeric_limits<Real>::epsilon() * scale;

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
    bit_reverse(Array<Complex> & a)
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

    /** @brief Computes the Fast Fourier Transform (FFT) in-place.
     *
     *  Implements the iterative radix-2 Cooley-Tukey algorithm.
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
      const size_t n = a.size();
      ah_invalid_argument_if(n == 0)
        << "FFT::transform: input must be non-empty";

      if (n == 1)
        return;

      ah_invalid_argument_if(not is_power_of_two(n))
        << "FFT::transform: input size " << n << " must be a power of two";

      bit_reverse(a);

      for (size_t len = 2; len <= n; len <<= 1)
        {
          const Real angle = (invert ? Real(2) : Real(-2))
                             * std::numbers::pi_v<Real>
                             / static_cast<Real>(len);
          const Complex wlen = std::polar(Real(1), angle);

          for (size_t i = 0; i < n; i += len)
            {
              Complex w(Real(1), Real(0));
              for (size_t j = 0; j < len / 2; ++j)
                {
                  const Complex u = a[i + j];
                  const Complex v = a[i + j + len / 2] * w;
                  a(i + j) = u + v;
                  a(i + j + len / 2) = u - v;
                  w *= wlen;
                }
            }
        }

      if (invert)
        {
          const Real inv_n = Real(1) / static_cast<Real>(n);
          for (size_t i = 0; i < n; ++i)
            a(i) *= inv_n;
        }
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

    template <typename Container>
      requires requires (const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      }
    [[nodiscard]] static Array<Complex>
    transformed(const Container & input, const bool invert = false)
    {
      return transformed(build_complex_input(input), invert);
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
      Array<Complex> output = lift_real_input(input);
      transform(output, false);
      return output;
    }

    template <typename Container>
      requires requires (const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      }
    [[nodiscard]] static Array<Complex>
    transform(const Container & input)
    {
      return transformed(input, false);
    }

    template <typename Container>
      requires requires (const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires std::convertible_to<decltype(*std::begin(c)), Real>;
        requires not Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      }
    [[nodiscard]] static Array<Complex>
    transform(const Container & input)
    {
      return transform(build_real_input(input));
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

    template <typename Container>
      requires requires (const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      }
    [[nodiscard]] static Array<Complex>
    inverse_transform(const Container & input)
    {
      return transformed(input, true);
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
      return project_real_output(inverse_transform(input),
                                 "FFT::inverse_transform_real");
    }

    template <typename Container>
      requires requires (const Container & c)
      {
        std::begin(c);
        std::end(c);
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(c))>>::value;
      }
    [[nodiscard]] static Array<Real>
    inverse_transform_real(const Container & input)
    {
      return inverse_transform_real(build_complex_input(input));
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
      if (a.is_empty() or b.is_empty())
        return {};

      ah_invalid_argument_if(a.size() > std::numeric_limits<size_t>::max() - b.size() + 1)
        << "FFT::multiply: product size exceeds size_t capacity";

      const size_t required = a.size() + b.size() - 1;
      const size_t n = next_power_of_two(required);

      Array<Complex> fa = a;
      Array<Complex> fb = b;
      fa.reserve(n);
      fb.reserve(n);

      for (size_t i = a.size(); i < n; ++i)
        fa.append(Complex(Real(0), Real(0)));
      for (size_t i = b.size(); i < n; ++i)
        fb.append(Complex(Real(0), Real(0)));

      transform(fa, false);
      transform(fb, false);

      for (size_t i = 0; i < n; ++i)
        fa(i) *= fb[i];

      transform(fa, true);

      Array<Complex> result;
      result.reserve(required);
      for (size_t i = 0; i < required; ++i)
        result.append(fa[i]);
      return result;
    }

    template <typename Container1, typename Container2>
      requires requires (const Container1 & a, const Container2 & b)
      {
        std::begin(a);
        std::end(a);
        std::begin(b);
        std::end(b);
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(a))>>::value;
        requires Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(b))>>::value;
      }
    [[nodiscard]] static Array<Complex>
    multiply(const Container1 & a, const Container2 & b)
    {
      return multiply(build_complex_input(a), build_complex_input(b));
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
      return project_real_output(multiply(lift_real_input(a), lift_real_input(b)),
                                 "FFT::multiply");
    }

    template <typename Container1, typename Container2>
      requires requires (const Container1 & a, const Container2 & b)
      {
        std::begin(a);
        std::end(a);
        std::begin(b);
        std::end(b);
        requires std::convertible_to<decltype(*std::begin(a)), Real>;
        requires std::convertible_to<decltype(*std::begin(b)), Real>;
        requires not Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(a))>>::value;
        requires not Is_Std_Complex<std::remove_cvref_t<decltype(*std::begin(b))>>::value;
      }
    [[nodiscard]] static Array<Real>
    multiply(const Container1 & a, const Container2 & b)
    {
      return multiply(build_real_input(a), build_real_input(b));
    }
  };
} // namespace Aleph

# endif // FFT_H
