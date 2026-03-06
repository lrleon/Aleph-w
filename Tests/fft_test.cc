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
 * @file fft_test.cc
 * @brief Exhaustive tests for fft.H.
 */

# include <gtest/gtest.h>

# include <cmath>
# include <complex>
# include <numbers>
# include <random>
# include <stdexcept>
# include <vector>

# include <fft.H>
# include <tpl_array.H>

using namespace Aleph;

namespace
{
  using FFTD = FFT<double>;
  using Complex = FFTD::Complex;

  constexpr double eps = 1e-9;

  void expect_complex_near(const Complex & lhs, const Complex & rhs,
                           const double tol = eps)
  {
    EXPECT_NEAR(lhs.real(), rhs.real(), tol);
    EXPECT_NEAR(lhs.imag(), rhs.imag(), tol);
  }

  void expect_complex_array_near(const Array<Complex> & lhs,
                                 const Array<Complex> & rhs,
                                 const double tol = eps)
  {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (size_t i = 0; i < lhs.size(); ++i)
      expect_complex_near(lhs[i], rhs[i], tol);
  }

  void expect_real_array_near(const Array<double> & lhs,
                              const Array<double> & rhs,
                              const double tol = eps)
  {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (size_t i = 0; i < lhs.size(); ++i)
      EXPECT_NEAR(lhs[i], rhs[i], tol);
  }

  void expect_spectrogram_near(const Array<Array<Complex>> & lhs,
                               const Array<Array<Complex>> & rhs,
                               const double tol = eps)
  {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (size_t i = 0; i < lhs.size(); ++i)
      expect_complex_array_near(lhs[i], rhs[i], tol);
  }

  void append_spectrogram(Array<Array<Complex>> & dst,
                          const Array<Array<Complex>> & src)
  {
    for (size_t i = 0; i < src.size(); ++i)
      dst.append(src[i]);
  }

  void append_real_samples(Array<double> & dst,
                           const Array<double> & src)
  {
    for (size_t i = 0; i < src.size(); ++i)
      dst.append(src[i]);
  }

  void expect_complex_unordered_near(const Array<Complex> & lhs,
                                     const Array<Complex> & rhs,
                                     const double tol = eps)
  {
    ASSERT_EQ(lhs.size(), rhs.size());

    std::vector<bool> matched(rhs.size(), false);
    for (size_t i = 0; i < lhs.size(); ++i)
      {
        bool found = false;
        for (size_t j = 0; j < rhs.size(); ++j)
          {
            if (matched[j])
              continue;

            if (std::abs(lhs[i] - rhs[j]) <= tol)
              {
                matched[j] = true;
                found = true;
                break;
              }
          }

        EXPECT_TRUE(found) << "Unexpected complex root " << lhs[i];
      }
  }

  Array<Complex> lift_real_input(const Array<double> & input)
  {
    Array<Complex> lifted;
    lifted.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
      lifted.append(Complex(input[i], 0.0));
    return lifted;
  }

  Array<Complex> naive_dft(const Array<Complex> & input, const bool invert)
  {
    const size_t n = input.size();
    Array<Complex> output;
    output.reserve(n);

    const double sign = invert ? 1.0 : -1.0;
    for (size_t k = 0; k < n; ++k)
      {
        Complex sum(0.0, 0.0);
        for (size_t t = 0; t < n; ++t)
          {
            const double angle = sign * 2.0 * std::numbers::pi *
                                 static_cast<double>(k * t) /
                                 static_cast<double>(n);
            sum += input[t] * std::polar(1.0, angle);
          }

        if (invert)
          sum /= static_cast<double>(n);

        output.append(sum);
      }

    return output;
  }

  Array<Complex> naive_convolution(const Array<Complex> & a,
                                   const Array<Complex> & b)
  {
    if (a.is_empty() or b.is_empty())
      return {};

    Array<Complex> output = Array<Complex>::create(a.size() + b.size() - 1);
    for (size_t i = 0; i < output.size(); ++i)
      output(i) = Complex(0.0, 0.0);

    for (size_t i = 0; i < a.size(); ++i)
      for (size_t j = 0; j < b.size(); ++j)
        output(i + j) += a[i] * b[j];

    return output;
  }

  Array<double> naive_convolution(const Array<double> & a,
                                  const Array<double> & b)
  {
    if (a.is_empty() or b.is_empty())
      return {};

    Array<double> output = Array<double>::create(a.size() + b.size() - 1);
    for (size_t i = 0; i < output.size(); ++i)
      output(i) = 0.0;

    for (size_t i = 0; i < a.size(); ++i)
      for (size_t j = 0; j < b.size(); ++j)
        output(i + j) += a[i] * b[j];

    return output;
  }

  Array<double> reverse_real_array(const Array<double> & input)
  {
    Array<double> output;
    output.reserve(input.size());
    for (size_t i = input.size(); i > 0; --i)
      output.append(input[i - 1]);
    return output;
  }

  Array<double> naive_prefix_filter(const Array<double> & signal,
                                    const Array<double> & coeffs)
  {
    if (signal.is_empty() or coeffs.is_empty())
      return {};

    Array<double> output = Array<double>::create(signal.size());
    for (size_t i = 0; i < signal.size(); ++i)
      {
        double sum = 0.0;
        const size_t limit = std::min(i + 1, coeffs.size());
        for (size_t k = 0; k < limit; ++k)
          sum += coeffs[k] * signal[i - k];
        output(i) = sum;
      }

    return output;
  }

  size_t naive_filtfilt_pad_length(const size_t signal_size,
                                   const size_t coeff_size)
  {
    if (signal_size <= 1 or coeff_size <= 1)
      return 0;

    const size_t taps_minus_one = coeff_size - 1;
    const size_t suggested = taps_minus_one > std::numeric_limits<size_t>::max() / 3
      ? std::numeric_limits<size_t>::max()
      : taps_minus_one * 3;
    return std::min(suggested, signal_size - 1);
  }

  Array<double> reflect_pad_real(const Array<double> & signal,
                                 const size_t pad_len)
  {
    if (pad_len == 0)
      return signal;

    Array<double> output;
    output.reserve(signal.size() + 2 * pad_len);

    for (size_t i = 0; i < pad_len; ++i)
      output.append(signal[pad_len - i]);
    for (size_t i = 0; i < signal.size(); ++i)
      output.append(signal[i]);
    for (size_t i = 0; i < pad_len; ++i)
      output.append(signal[signal.size() - 2 - i]);

    return output;
  }

  Array<double> slice_real_array(const Array<double> & input,
                                 const size_t offset,
                                 const size_t length)
  {
    Array<double> output;
    output.reserve(length);
    for (size_t i = 0; i < length; ++i)
      output.append(input[offset + i]);
    return output;
  }

  Array<double> naive_filtfilt(const Array<double> & signal,
                               const Array<double> & coeffs)
  {
    if (signal.is_empty() or coeffs.is_empty())
      return {};

    if (coeffs.size() == 1)
      {
        Array<double> output = Array<double>::create(signal.size());
        const double gain = coeffs[0] * coeffs[0];
        for (size_t i = 0; i < signal.size(); ++i)
          output(i) = signal[i] * gain;
        return output;
      }

    const size_t pad_len = naive_filtfilt_pad_length(signal.size(), coeffs.size());
    const Array<double> padded = reflect_pad_real(signal, pad_len);
    const Array<double> forward = naive_prefix_filter(padded, coeffs);
    const Array<double> backward = naive_prefix_filter(reverse_real_array(forward), coeffs);
    return slice_real_array(reverse_real_array(backward), pad_len, signal.size());
  }

  struct RefIIRCoefficients
  {
    Array<double> numerator;
    Array<double> denominator;
  };

  double max_abs_real_array(const Array<double> & input)
  {
    double max_value = 0.0;
    for (size_t i = 0; i < input.size(); ++i)
      max_value = std::max(max_value, std::abs(input[i]));
    return max_value;
  }

  size_t effective_coeff_length_ref(const Array<double> & input)
  {
    if (input.is_empty())
      return 0;

    const double tol = (max_abs_real_array(input) + 1.0) * 64.0
                       * std::numeric_limits<double>::epsilon();
    size_t n = input.size();
    while (n > 1 and std::abs(input[n - 1]) <= tol)
      --n;
    return n;
  }

  RefIIRCoefficients normalize_iir_reference(const Array<double> & numerator,
                                             const Array<double> & denominator)
  {
    const size_t num_length = effective_coeff_length_ref(numerator);
    const size_t den_length = effective_coeff_length_ref(denominator);
    EXPECT_GT(num_length, 0u);
    EXPECT_GT(den_length, 0u);

    const double a0 = denominator[0];
    const double tol = (max_abs_real_array(denominator) + 1.0) * 64.0
                       * std::numeric_limits<double>::epsilon();
    EXPECT_GT(std::abs(a0), tol);

    const size_t order = std::max(num_length, den_length) - 1;
    RefIIRCoefficients coeffs;
    coeffs.numerator = Array<double>::create(order + 1);
    coeffs.denominator = Array<double>::create(order + 1);
    for (size_t i = 0; i <= order; ++i)
      {
        coeffs.numerator(i) = 0.0;
        coeffs.denominator(i) = 0.0;
      }

    for (size_t i = 0; i < num_length; ++i)
      coeffs.numerator(i) = numerator[i] / a0;
    for (size_t i = 0; i < den_length; ++i)
      coeffs.denominator(i) = denominator[i] / a0;
    coeffs.denominator(0) = 1.0;
    return coeffs;
  }

  Array<double> solve_dense_system_ref(Array<double> matrix,
                                       Array<double> rhs,
                                       const size_t n)
  {
    if (n == 0)
      return {};

    auto coeff = [&matrix, n](const size_t row, const size_t col) -> double &
    {
      return matrix(row * n + col);
    };

    double max_entry = 0.0;
    for (size_t i = 0; i < matrix.size(); ++i)
      max_entry = std::max(max_entry, std::abs(matrix[i]));
    for (size_t i = 0; i < rhs.size(); ++i)
      max_entry = std::max(max_entry, std::abs(rhs[i]));

    const double tol = (max_entry + 1.0) * 128.0
                       * std::numeric_limits<double>::epsilon();

    for (size_t col = 0; col < n; ++col)
      {
        size_t pivot_row = col;
        double pivot_abs = std::abs(coeff(col, col));
        for (size_t row = col + 1; row < n; ++row)
          {
            const double candidate = std::abs(coeff(row, col));
            if (candidate > pivot_abs)
              {
                pivot_abs = candidate;
                pivot_row = row;
              }
          }

        EXPECT_GT(pivot_abs, tol);

        if (pivot_row != col)
          {
            for (size_t k = col; k < n; ++k)
              std::swap(coeff(col, k), coeff(pivot_row, k));
            std::swap(rhs(col), rhs(pivot_row));
          }

        const double pivot = coeff(col, col);
        for (size_t row = col + 1; row < n; ++row)
          {
            const double factor = coeff(row, col) / pivot;
            coeff(row, col) = 0.0;
            for (size_t k = col + 1; k < n; ++k)
              coeff(row, k) -= factor * coeff(col, k);
            rhs(row) -= factor * rhs[col];
          }
      }

    Array<double> solution = Array<double>::create(n);
    for (size_t row = n; row > 0; --row)
      {
        const size_t i = row - 1;
        double sum = rhs[i];
        for (size_t col = i + 1; col < n; ++col)
          sum -= coeff(i, col) * solution[col];
        solution(i) = sum / coeff(i, i);
      }

    return solution;
  }

  Array<double> iir_steady_state_ref(const Array<double> & numerator,
                                     const Array<double> & denominator)
  {
    const size_t order = denominator.size() - 1;
    if (order == 0)
      return {};

    Array<double> system = Array<double>::create(order * order);
    for (size_t i = 0; i < system.size(); ++i)
      system(i) = 0.0;

    auto coeff = [&system, order](const size_t row, const size_t col) -> double &
    {
      return system(row * order + col);
    };

    for (size_t i = 0; i < order; ++i)
      coeff(i, i) = 1.0;
    for (size_t i = 0; i < order; ++i)
      {
        coeff(i, 0) += denominator[i + 1];
        if (i + 1 < order)
          coeff(i, i + 1) -= 1.0;
      }

    Array<double> rhs = Array<double>::create(order);
    for (size_t i = 0; i < order; ++i)
      rhs(i) = numerator[i + 1] - denominator[i + 1] * numerator[0];
    return solve_dense_system_ref(system, rhs, order);
  }

  Array<double> scale_real_array(const Array<double> & input, const double factor)
  {
    Array<double> output = Array<double>::create(input.size());
    for (size_t i = 0; i < input.size(); ++i)
      output(i) = input[i] * factor;
    return output;
  }

  Array<double> naive_iir_filter(const Array<double> & signal,
                                 const Array<double> & numerator,
                                 const Array<double> & denominator,
                                 const Array<double> & initial_state = {})
  {
    if (signal.is_empty())
      return {};

    const size_t order = denominator.size() - 1;
    Array<double> output = Array<double>::create(signal.size());
    if (order == 0)
      {
        for (size_t i = 0; i < signal.size(); ++i)
          output(i) = numerator[0] * signal[i];
        return output;
      }

    Array<double> state = Array<double>::create(order);
    for (size_t i = 0; i < order; ++i)
      state(i) = initial_state.is_empty() ? 0.0 : initial_state[i];

    for (size_t n = 0; n < signal.size(); ++n)
      {
        const double x = signal[n];
        const double y = numerator[0] * x + state[0];
        output(n) = y;
        for (size_t i = 0; i + 1 < order; ++i)
          state(i) = state[i + 1] + numerator[i + 1] * x
                     - denominator[i + 1] * y;
        state(order - 1) = numerator[order] * x - denominator[order] * y;
      }

    return output;
  }

  Array<double> naive_iir_filtfilt(const Array<double> & signal,
                                   const Array<double> & numerator,
                                   const Array<double> & denominator)
  {
    if (signal.is_empty() or numerator.is_empty() or denominator.is_empty())
      return {};

    const auto coeffs = normalize_iir_reference(numerator, denominator);
    const size_t order = coeffs.denominator.size() - 1;

    if (order == 0)
      {
        Array<double> output = Array<double>::create(signal.size());
        const double gain = coeffs.numerator[0] * coeffs.numerator[0];
        for (size_t i = 0; i < signal.size(); ++i)
          output(i) = signal[i] * gain;
        return output;
      }

    const size_t pad_len = naive_filtfilt_pad_length(signal.size(),
                                                     coeffs.denominator.size());
    const Array<double> padded = reflect_pad_real(signal, pad_len);
    const Array<double> zi = iir_steady_state_ref(coeffs.numerator,
                                                  coeffs.denominator);

    const Array<double> forward = naive_iir_filter(padded,
                                                   coeffs.numerator,
                                                   coeffs.denominator,
                                                   scale_real_array(zi, padded[0]));
    const Array<double> reversed = reverse_real_array(forward);
    const Array<double> backward = naive_iir_filter(reversed,
                                                    coeffs.numerator,
                                                    coeffs.denominator,
                                                    scale_real_array(zi, reversed[0]));
    return slice_real_array(reverse_real_array(backward), pad_len, signal.size());
  }

  Array<double> naive_sos_filtfilt(const Array<double> & signal,
                                   const Array<FFTD::BiquadSection> & sections)
  {
    if (signal.is_empty() or sections.is_empty())
      return {};

    Array<RefIIRCoefficients> normalized_sections;
    normalized_sections.reserve(sections.size());
    size_t total_order = 0;
    for (size_t i = 0; i < sections.size(); ++i)
      {
        const auto coeffs = normalize_iir_reference(sections[i].numerator(),
                                                    sections[i].denominator());
        total_order += coeffs.denominator.size() - 1;
        normalized_sections.append(coeffs);
      }

    Array<double> stage =
      reflect_pad_real(signal, naive_filtfilt_pad_length(signal.size(), total_order + 1));

    for (size_t i = 0; i < normalized_sections.size(); ++i)
      {
        const auto & coeffs = normalized_sections[i];
        stage = naive_iir_filter(stage,
                                 coeffs.numerator,
                                 coeffs.denominator,
                                 scale_real_array(iir_steady_state_ref(coeffs.numerator,
                                                                       coeffs.denominator),
                                                  stage[0]));
      }

    stage = reverse_real_array(stage);
    for (size_t i = 0; i < normalized_sections.size(); ++i)
      {
        const auto & coeffs = normalized_sections[i];
        stage = naive_iir_filter(stage,
                                 coeffs.numerator,
                                 coeffs.denominator,
                                 scale_real_array(iir_steady_state_ref(coeffs.numerator,
                                                                       coeffs.denominator),
                                                  stage[0]));
      }

    const size_t pad_len = naive_filtfilt_pad_length(signal.size(), total_order + 1);
    return slice_real_array(reverse_real_array(stage), pad_len, signal.size());
  }

  template <std::floating_point Real>
  void expect_typed_complex_array_near(const Array<std::complex<Real>> & lhs,
                                       const Array<std::complex<Real>> & rhs,
                                       const Real tol)
  {
    ASSERT_EQ(lhs.size(), rhs.size());
    for (size_t i = 0; i < lhs.size(); ++i)
      EXPECT_LE(std::abs(lhs[i] - rhs[i]), tol);
  }
}

TEST(FFT, TransformRejectsInvalidInput)
{
  Array<Complex> empty;
  EXPECT_THROW(FFTD::transform(empty, false), std::invalid_argument);

  Array<Complex> bad = {Complex(1.0, 0.0), Complex(2.0, 0.0), Complex(3.0, 0.0)};
  EXPECT_THROW(FFTD::transform(bad, false), std::invalid_argument);
}

TEST(FFT, PowerOfTwoPredicate)
{
  EXPECT_FALSE(FFTD::is_power_of_two(0));
  EXPECT_TRUE(FFTD::is_power_of_two(1));
  EXPECT_TRUE(FFTD::is_power_of_two(8));
  EXPECT_FALSE(FFTD::is_power_of_two(12));
}

TEST(FFT, ComplexRoundTrip)
{
  Array<Complex> signal = {
    Complex(1.5, -0.5),
    Complex(-2.0, 3.0),
    Complex(0.25, 4.5),
    Complex(7.0, -1.0),
    Complex(-3.5, 2.25),
    Complex(1.0, 0.0),
    Complex(2.0, -2.0),
    Complex(-1.25, 0.75)
  };

  Array<Complex> work = signal;
  FFTD::transform(work, false);
  FFTD::transform(work, true);

  expect_complex_array_near(work, signal, 1e-8);
}

TEST(FFT, MatchesNaiveDFTForComplexSignals)
{
  std::mt19937_64 rng(20260305);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(2), size_t(4), size_t(8), size_t(16)})
    {
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      const auto expected = naive_dft(signal, false);
      const auto obtained = FFTD::transformed(signal, false);
      expect_complex_array_near(obtained, expected, 1e-8);
    }
}

TEST(FFT, RealTransformMatchesLiftedComplexTransform)
{
  Array<double> signal = {1.0, 2.0, -1.0, 0.5, 3.0, -2.0, 4.0, -0.75};

  const auto real_spectrum = FFTD::transform(signal);
  const auto complex_spectrum = FFTD::transformed(lift_real_input(signal), false);

  expect_complex_array_near(real_spectrum, complex_spectrum, 1e-8);
}

TEST(FFT, AcceptsGenericComplexContainers)
{
  std::vector<Complex> signal = {
    Complex(1.5, -0.5),
    Complex(-2.0, 3.0),
    Complex(0.25, 4.5),
    Complex(7.0, -1.0),
    Complex(-3.5, 2.25),
    Complex(1.0, 0.0),
    Complex(2.0, -2.0),
    Complex(-1.25, 0.75)
  };

  const Array<Complex> as_array(signal.begin(), signal.end());
  const auto spectrum_from_vector = FFTD::transform(signal);
  const auto spectrum_from_array = FFTD::transformed(as_array, false);
  expect_complex_array_near(spectrum_from_vector, spectrum_from_array, 1e-8);

  std::vector<Complex> spectrum_vector(spectrum_from_vector.begin(),
                                       spectrum_from_vector.end());
  const auto restored = FFTD::inverse_transform(spectrum_vector);
  expect_complex_array_near(restored, as_array, 1e-8);
}

TEST(FFT, AcceptsGenericRealContainers)
{
  std::vector<double> signal = {1.0, 2.0, -1.0, 0.5, 3.0, -2.0, 4.0, -0.75};
  Array<double> as_array(signal.begin(), signal.end());

  const auto spectrum_from_vector = FFTD::transform(signal);
  const auto spectrum_from_array = FFTD::transform(as_array);
  expect_complex_array_near(spectrum_from_vector, spectrum_from_array, 1e-8);

  const auto restored = FFTD::inverse_transform_real(spectrum_from_vector);
  expect_real_array_near(restored, as_array, 1e-8);
}

TEST(FFT, SpectrumAliasesMatchTransform)
{
  ThreadPool pool(4);

  Array<double> real_signal = {1.0, -0.5, 3.0, 2.5, -1.0, 4.0, 0.75, -2.25};
  const auto expected_real = FFTD::transform(real_signal);
  const auto obtained_real = FFTD::spectrum(real_signal);
  expect_complex_array_near(obtained_real, expected_real, 1e-12);

  const auto expected_real_parallel = FFTD::ptransform(pool, real_signal);
  const auto obtained_real_parallel = FFTD::pspectrum(pool, real_signal);
  expect_complex_array_near(obtained_real_parallel, expected_real_parallel, 1e-12);

  Array<Complex> complex_signal = lift_real_input(real_signal);
  const auto expected_complex = FFTD::transformed(complex_signal, false);
  const auto obtained_complex = FFTD::spectrum(complex_signal);
  expect_complex_array_near(obtained_complex, expected_complex, 1e-12);

  std::vector<Complex> complex_vector(complex_signal.begin(), complex_signal.end());
  const auto expected_complex_parallel = FFTD::ptransformed(pool, complex_vector, false);
  const auto obtained_complex_parallel = FFTD::pspectrum(pool, complex_vector);
  expect_complex_array_near(obtained_complex_parallel, expected_complex_parallel, 1e-12);
}

TEST(FFT, MagnitudeAndPowerSpectrumUtilities)
{
  Array<Complex> bins = {
    Complex(3.0, 4.0),
    Complex(-1.0, 0.0),
    Complex(0.0, -2.0),
    Complex(0.5, -0.5)
  };

  const auto magnitude = FFTD::magnitude_spectrum(bins);
  const auto power = FFTD::power_spectrum(bins);

  Array<double> expected_magnitude = {5.0, 1.0, 2.0, std::sqrt(0.5)};
  Array<double> expected_power = {25.0, 1.0, 4.0, 0.5};

  expect_real_array_near(magnitude, expected_magnitude, 1e-12);
  expect_real_array_near(power, expected_power, 1e-12);

  std::vector<Complex> bins_vector(bins.begin(), bins.end());
  expect_real_array_near(FFTD::magnitude_spectrum(bins_vector), expected_magnitude, 1e-12);
  expect_real_array_near(FFTD::power_spectrum(bins_vector), expected_power, 1e-12);
}

TEST(FFT, PhaseSpectrumUtility)
{
  Array<Complex> bins = {
    Complex(1.0, 0.0),
    Complex(0.0, 1.0),
    Complex(-1.0, 0.0),
    Complex(0.0, -1.0)
  };

  const auto phase = FFTD::phase_spectrum(bins);
  ASSERT_EQ(phase.size(), 4u);
  EXPECT_NEAR(phase[0], 0.0, 1e-12);
  EXPECT_NEAR(phase[1], std::numbers::pi / 2.0, 1e-12);
  EXPECT_NEAR(phase[2], std::numbers::pi, 1e-12);
  EXPECT_NEAR(phase[3], -std::numbers::pi / 2.0, 1e-12);

  std::vector<Complex> bins_vector(bins.begin(), bins.end());
  expect_real_array_near(FFTD::phase_spectrum(bins_vector), phase, 1e-12);
}

TEST(FFT, PaddedTransformHandlesNonPowerOfTwoInputs)
{
  std::vector<double> signal = {1.0, -0.5, 2.0, 0.25, -1.0, 3.0};
  Array<double> padded(signal.begin(), signal.end());
  padded.append(0.0);
  padded.append(0.0);

  const auto obtained = FFTD::transform_padded(signal);
  const auto expected = FFTD::transform(padded);

  ASSERT_EQ(obtained.size(), 8u);
  expect_complex_array_near(obtained, expected, 1e-8);
}

TEST(FFT, ParallelComplexTransformMatchesSequential)
{
  ThreadPool pool(4);
  Array<Complex> signal = {
    Complex(1.5, -0.5),
    Complex(-2.0, 3.0),
    Complex(0.25, 4.5),
    Complex(7.0, -1.0),
    Complex(-3.5, 2.25),
    Complex(1.0, 0.0),
    Complex(2.0, -2.0),
    Complex(-1.25, 0.75)
  };

  const auto expected = FFTD::transformed(signal, false);
  const auto obtained = FFTD::ptransformed(pool, signal, false);
  expect_complex_array_near(obtained, expected, 1e-8);

  Array<Complex> work = signal;
  FFTD::ptransform(pool, work, false);
  expect_complex_array_near(work, expected, 1e-8);
}

TEST(FFT, ParallelRealTransformMatchesSequential)
{
  ThreadPool pool(4);
  Array<double> signal = {2.0, -1.0, 0.5, 3.0, -2.5, 1.25, 4.0, -0.75};

  const auto expected = FFTD::transform(signal);
  const auto obtained = FFTD::ptransform(pool, signal);
  expect_complex_array_near(obtained, expected, 1e-8);
}

TEST(FFT, RealInverseRestoresSignal)
{
  Array<double> signal = {0.5, -1.25, 3.5, 2.0, -0.75, 4.25, 1.0, -2.0};

  const auto spectrum = FFTD::transform(signal);
  const auto restored = FFTD::inverse_transform_real(spectrum);

  expect_real_array_near(restored, signal, 1e-8);
}

TEST(FFT, RealInverseRejectsNonHermitianSpectrum)
{
  Array<Complex> invalid = {
    Complex(10.0, 0.0),
    Complex(1.0, 2.0),
    Complex(-3.0, 0.0),
    Complex(1.5, -0.5)
  };

  EXPECT_THROW(FFTD::inverse_transform_real(invalid), std::domain_error);
}

TEST(FFT, RealSpectrumHasHermitianSymmetry)
{
  Array<double> signal = {2.0, -1.0, 0.5, 3.0, -2.5, 1.25, 4.0, -0.75};
  const auto spectrum = FFTD::transform(signal);

  ASSERT_EQ(spectrum.size(), 8u);
  expect_complex_near(spectrum[0], std::conj(spectrum[0]), 1e-8);
  expect_complex_near(spectrum[4], std::conj(spectrum[4]), 1e-8);

  for (size_t k = 1; k < 4; ++k)
    expect_complex_near(spectrum[k], std::conj(spectrum[8 - k]), 1e-8);
}

TEST(FFT, KnownSignalsProduceExpectedSpectra)
{
  Array<Complex> impulse = {
    Complex(1.0, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0),
    Complex(0.0, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0)
  };
  const auto impulse_spectrum = FFTD::transformed(impulse, false);
  for (size_t i = 0; i < impulse_spectrum.size(); ++i)
    expect_complex_near(impulse_spectrum[i], Complex(1.0, 0.0), 1e-8);

  Array<double> alternating = {1.0, -1.0, 1.0, -1.0};
  const auto alternating_spectrum = FFTD::transform(alternating);
  ASSERT_EQ(alternating_spectrum.size(), 4u);
  expect_complex_near(alternating_spectrum[0], Complex(0.0, 0.0), 1e-8);
  expect_complex_near(alternating_spectrum[1], Complex(0.0, 0.0), 1e-8);
  expect_complex_near(alternating_spectrum[2], Complex(4.0, 0.0), 1e-8);
  expect_complex_near(alternating_spectrum[3], Complex(0.0, 0.0), 1e-8);
}

TEST(FFT, ComplexConvolutionMatchesNaive)
{
  Array<Complex> a = {
    Complex(1.0, 2.0),
    Complex(-3.0, 0.5),
    Complex(2.5, -1.5)
  };
  Array<Complex> b = {
    Complex(0.5, -1.0),
    Complex(4.0, 2.0)
  };

  const auto expected = naive_convolution(a, b);
  const auto obtained = FFTD::multiply(a, b);
  expect_complex_array_near(obtained, expected, 1e-8);
}

TEST(FFT, ParallelComplexConvolutionMatchesSequential)
{
  ThreadPool pool(4);
  Array<Complex> a = {
    Complex(1.0, 2.0),
    Complex(-3.0, 0.5),
    Complex(2.5, -1.5)
  };
  Array<Complex> b = {
    Complex(0.5, -1.0),
    Complex(4.0, 2.0)
  };

  const auto expected = FFTD::multiply(a, b);
  const auto obtained = FFTD::pmultiply(pool, a, b);
  expect_complex_array_near(obtained, expected, 1e-8);
}

TEST(FFT, GenericComplexConvolutionMatchesArrayVersion)
{
  std::vector<Complex> a = {
    Complex(1.0, 2.0),
    Complex(-3.0, 0.5),
    Complex(2.5, -1.5)
  };
  Array<Complex> b = {
    Complex(0.5, -1.0),
    Complex(4.0, 2.0)
  };

  const Array<Complex> a_array(a.begin(), a.end());
  const auto expected = FFTD::multiply(a_array, b);
  const auto obtained = FFTD::multiply(a, b);
  expect_complex_array_near(obtained, expected, 1e-8);
}

TEST(FFT, RealConvolutionMatchesNaive)
{
  Array<double> a = {1.0, 2.0, 3.0, -1.0};
  Array<double> b = {4.0, -1.0, 0.5};

  const auto expected = naive_convolution(a, b);
  const auto obtained = FFTD::multiply(a, b);
  expect_real_array_near(obtained, expected, 1e-8);
}

TEST(FFT, ParallelRealConvolutionMatchesSequential)
{
  ThreadPool pool(4);
  Array<double> a = {1.0, 2.0, 3.0, -1.0};
  Array<double> b = {4.0, -1.0, 0.5};

  const auto expected = FFTD::multiply(a, b);
  const auto obtained = FFTD::pmultiply(pool, a, b);
  expect_real_array_near(obtained, expected, 1e-8);
}

TEST(FFT, GenericRealConvolutionMatchesArrayVersion)
{
  std::vector<double> a = {1.0, 2.0, 3.0, -1.0};
  Array<double> b = {4.0, -1.0, 0.5};

  const Array<double> a_array(a.begin(), a.end());
  const auto expected = FFTD::multiply(a_array, b);
  const auto obtained = FFTD::multiply(a, b);
  expect_real_array_near(obtained, expected, 1e-8);
}

TEST(FFT, RandomizedConvolutionCrossCheck)
{
  std::mt19937_64 rng(424242);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  for (int trial = 0; trial < 20; ++trial)
    {
      const size_t n = static_cast<size_t>(trial % 6 + 1);
      const size_t m = static_cast<size_t>((trial * 3) % 5 + 1);

      Array<double> ra;
      Array<double> rb;
      ra.reserve(n);
      rb.reserve(m);
      for (size_t i = 0; i < n; ++i)
        ra.append(dist(rng));
      for (size_t i = 0; i < m; ++i)
        rb.append(dist(rng));

      const auto expected_real = naive_convolution(ra, rb);
      const auto obtained_real = FFTD::multiply(ra, rb);
      expect_real_array_near(obtained_real, expected_real, 1e-8);

      Array<Complex> ca;
      Array<Complex> cb;
      ca.reserve(n);
      cb.reserve(m);
      for (size_t i = 0; i < n; ++i)
        ca.append(Complex(dist(rng), dist(rng)));
      for (size_t i = 0; i < m; ++i)
        cb.append(Complex(dist(rng), dist(rng)));

      const auto expected_complex = naive_convolution(ca, cb);
      const auto obtained_complex = FFTD::multiply(ca, cb);
      expect_complex_array_near(obtained_complex, expected_complex, 1e-8);
    }
}

TEST(FFT, EmptyConvolutionProducesEmptyResult)
{
  EXPECT_TRUE(FFTD::multiply(Array<double>(), Array<double>({1.0, 2.0})).is_empty());
  EXPECT_TRUE(FFTD::multiply(Array<Complex>(), Array<Complex>({Complex(1.0, 0.0)})).is_empty());
}

TEST(FFT, FloatRoundTrip)
{
  using FFTF = FFT<float>;
  using ComplexF = FFTF::Complex;

  Array<ComplexF> signal = {
    ComplexF(1.5f, -0.5f),
    ComplexF(-2.0f, 3.0f),
    ComplexF(0.25f, 4.5f),
    ComplexF(7.0f, -1.0f),
    ComplexF(-3.5f, 2.25f),
    ComplexF(1.0f, 0.0f),
    ComplexF(2.0f, -2.0f),
    ComplexF(-1.25f, 0.75f)
  };

  Array<ComplexF> work = signal;
  FFTF::transform(work, false);
  FFTF::transform(work, true);

  expect_typed_complex_array_near(work, signal, 2e-4f);
}

TEST(FFT, LongDoubleRoundTrip)
{
  using FFTL = FFT<long double>;
  using ComplexL = FFTL::Complex;

  Array<ComplexL> signal = {
    ComplexL(1.5L, -0.5L),
    ComplexL(-2.0L, 3.0L),
    ComplexL(0.25L, 4.5L),
    ComplexL(7.0L, -1.0L),
    ComplexL(-3.5L, 2.25L),
    ComplexL(1.0L, 0.0L),
    ComplexL(2.0L, -2.0L),
    ComplexL(-1.25L, 0.75L)
  };

  Array<ComplexL> work = signal;
  FFTL::transform(work, false);
  FFTL::transform(work, true);

  expect_typed_complex_array_near(work, signal, 1e-12L);
}

// ---------------------------------------------------------------------------
// Plan tests
// ---------------------------------------------------------------------------

TEST(FFTPlan, ConstructionRejectsInvalidSizes)
{
  EXPECT_THROW(FFTD::Plan(0), std::invalid_argument);
  EXPECT_THROW(FFTD::Plan(3), std::invalid_argument);
  EXPECT_THROW(FFTD::Plan(6), std::invalid_argument);
  EXPECT_NO_THROW(FFTD::Plan(1));
  EXPECT_NO_THROW(FFTD::Plan(2));
  EXPECT_NO_THROW(FFTD::Plan(8));
  EXPECT_NO_THROW(FFTD::Plan(1024));
}

TEST(FFTPlan, SizeReturnsConstructedSize)
{
  FFTD::Plan p1(1);
  EXPECT_EQ(p1.size(), 1u);

  FFTD::Plan p8(8);
  EXPECT_EQ(p8.size(), 8u);

  FFTD::Plan p1024(1024);
  EXPECT_EQ(p1024.size(), 1024u);

  FFTD::Plan empty;
  EXPECT_EQ(empty.size(), 0u);
}

TEST(FFTPlan, TransformRejectsSizeMismatch)
{
  FFTD::Plan plan(8);
  Array<Complex> wrong_size = {Complex(1.0, 0.0), Complex(2.0, 0.0)};
  EXPECT_THROW(plan.transform(wrong_size, false), std::invalid_argument);
}

TEST(FFTPlan, MatchesStaticTransform)
{
  std::mt19937_64 rng(99887766);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(1), size_t(2), size_t(4), size_t(8),
                         size_t(16), size_t(64), size_t(256), size_t(1024)})
    {
      FFTD::Plan plan(n);
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      auto from_static = FFTD::transformed(signal, false);
      auto from_plan = plan.transformed(signal, false);
      expect_complex_array_near(from_static, from_plan, 1e-9);
    }
}

TEST(FFTPlan, RoundTrip)
{
  std::mt19937_64 rng(55443322);
  std::uniform_real_distribution<double> dist(-10.0, 10.0);

  for (const size_t n : {size_t(2), size_t(8), size_t(64), size_t(512)})
    {
      FFTD::Plan plan(n);
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      Array<Complex> work = signal;
      plan.transform(work, false);
      plan.transform(work, true);

      expect_complex_array_near(work, signal, 1e-8);
    }
}

TEST(FFTPlan, ReusePlanForMultipleTransforms)
{
  std::mt19937_64 rng(11223344);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  FFTD::Plan plan(256);

  for (int trial = 0; trial < 10; ++trial)
    {
      Array<Complex> signal;
      signal.reserve(256);
      for (size_t i = 0; i < 256; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      Array<Complex> work = signal;
      plan.transform(work, false);
      plan.transform(work, true);

      expect_complex_array_near(work, signal, 1e-8);
    }
}

TEST(FFTPlan, InverseTransformReal)
{
  Array<double> signal = {0.5, -1.25, 3.5, 2.0, -0.75, 4.25, 1.0, -2.0};
  Array<Complex> lifted;
  lifted.reserve(8);
  for (size_t i = 0; i < signal.size(); ++i)
    lifted.append(Complex(signal[i], 0.0));

  FFTD::Plan plan(8);
  auto spectrum = plan.transformed(lifted, false);
  auto restored = plan.inverse_transform_real(spectrum);

  expect_real_array_near(restored, signal, 1e-8);
}

TEST(FFTPlan, ParallelInverseTransformReal)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(31415926);
  std::uniform_real_distribution<double> dist(-4.0, 4.0);

  const size_t n = 256;
  Array<double> signal;
  signal.reserve(n);
  for (size_t i = 0; i < n; ++i)
    signal.append(dist(rng));

  FFTD::Plan plan(n);
  const auto spectrum = FFTD::transform(signal);
  const auto sequential = plan.inverse_transform_real(spectrum);
  const auto parallel = plan.pinverse_transform_real(pool, spectrum);

  expect_real_array_near(parallel, sequential, 1e-8);
  expect_real_array_near(parallel, signal, 1e-8);
}

TEST(FFTPlan, MatchesNaiveDFTLargeN)
{
  std::mt19937_64 rng(77665544);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  const size_t n = 64;
  FFTD::Plan plan(n);
  Array<Complex> signal;
  signal.reserve(n);
  for (size_t i = 0; i < n; ++i)
    signal.append(Complex(dist(rng), dist(rng)));

  const auto expected = naive_dft(signal, false);
  const auto obtained = plan.transformed(signal, false);
  expect_complex_array_near(obtained, expected, 1e-6);
}

TEST(FFTPlan, ParallelMatchesSequential)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(33221100);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(8), size_t(64), size_t(256), size_t(1024)})
    {
      FFTD::Plan plan(n);
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      const auto sequential = plan.transformed(signal, false);
      const auto parallel = plan.ptransformed(pool, signal, false);
      expect_complex_array_near(sequential, parallel, 1e-9);

      const auto seq_inv = plan.transformed(signal, true);
      const auto par_inv = plan.ptransformed(pool, signal, true);
      expect_complex_array_near(seq_inv, par_inv, 1e-9);
    }
}

TEST(FFTPlan, FloatPlan)
{
  using FFTF = FFT<float>;
  using ComplexF = FFTF::Complex;

  FFTF::Plan plan(8);
  Array<ComplexF> signal = {
    ComplexF(1.5f, -0.5f), ComplexF(-2.0f, 3.0f),
    ComplexF(0.25f, 4.5f), ComplexF(7.0f, -1.0f),
    ComplexF(-3.5f, 2.25f), ComplexF(1.0f, 0.0f),
    ComplexF(2.0f, -2.0f), ComplexF(-1.25f, 0.75f)
  };

  Array<ComplexF> work = signal;
  plan.transform(work, false);
  plan.transform(work, true);

  expect_typed_complex_array_near(work, signal, 2e-4f);
}

TEST(FFTPlan, ParsevalTheorem)
{
  std::mt19937_64 rng(44556677);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(8), size_t(64), size_t(256), size_t(1024)})
    {
      FFTD::Plan plan(n);
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      double energy_time = 0.0;
      for (size_t i = 0; i < n; ++i)
        energy_time += std::norm(signal[i]);

      const auto spectrum = plan.transformed(signal, false);

      double energy_freq = 0.0;
      for (size_t i = 0; i < n; ++i)
        energy_freq += std::norm(spectrum[i]);

      EXPECT_NEAR(energy_time, energy_freq / static_cast<double>(n), 1e-6)
        << "Parseval failed for N=" << n;
    }
}

TEST(FFTPlan, LinearityProperty)
{
  std::mt19937_64 rng(66778899);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  const size_t n = 128;
  FFTD::Plan plan(n);

  Array<Complex> x, y;
  x.reserve(n);
  y.reserve(n);
  for (size_t i = 0; i < n; ++i)
    {
      x.append(Complex(dist(rng), dist(rng)));
      y.append(Complex(dist(rng), dist(rng)));
    }

  const double alpha = 3.7;
  const double beta = -2.1;

  Array<Complex> z;
  z.reserve(n);
  for (size_t i = 0; i < n; ++i)
    z.append(alpha * x[i] + beta * y[i]);

  const auto fft_z = plan.transformed(z, false);
  const auto fft_x = plan.transformed(x, false);
  const auto fft_y = plan.transformed(y, false);

  Array<Complex> expected;
  expected.reserve(n);
  for (size_t i = 0; i < n; ++i)
    expected.append(alpha * fft_x[i] + beta * fft_y[i]);

  expect_complex_array_near(fft_z, expected, 1e-8);
}

// ---------------------------------------------------------------------------
// Large-N tests (exercise twiddle refresh, numerical stability at scale)
// ---------------------------------------------------------------------------

TEST(FFTLargeN, MatchesNaiveDFTUpTo256)
{
  std::mt19937_64 rng(10203040);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(32), size_t(64), size_t(128), size_t(256)})
    {
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      const auto expected = naive_dft(signal, false);
      const auto obtained = FFTD::transformed(signal, false);

      for (size_t i = 0; i < n; ++i)
        {
          EXPECT_NEAR(obtained[i].real(), expected[i].real(), 1e-6)
            << "N=" << n << " bin=" << i;
          EXPECT_NEAR(obtained[i].imag(), expected[i].imag(), 1e-6)
            << "N=" << n << " bin=" << i;
        }
    }
}

TEST(FFTLargeN, ComplexRoundTrip)
{
  std::mt19937_64 rng(50607080);
  std::uniform_real_distribution<double> dist(-10.0, 10.0);

  for (const size_t n : {size_t(256), size_t(1024), size_t(4096), size_t(16384)})
    {
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      Array<Complex> work = signal;
      FFTD::transform(work, false);
      FFTD::transform(work, true);

      for (size_t i = 0; i < n; ++i)
        {
          EXPECT_NEAR(work[i].real(), signal[i].real(), 1e-7)
            << "N=" << n << " i=" << i;
          EXPECT_NEAR(work[i].imag(), signal[i].imag(), 1e-7)
            << "N=" << n << " i=" << i;
        }
    }
}

TEST(FFTLargeN, RealRoundTrip)
{
  std::mt19937_64 rng(90807060);
  std::uniform_real_distribution<double> dist(-10.0, 10.0);

  for (const size_t n : {size_t(256), size_t(1024), size_t(4096)})
    {
      Array<double> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(dist(rng));

      const auto spectrum = FFTD::transform(signal);
      const auto restored = FFTD::inverse_transform_real(spectrum);

      ASSERT_EQ(restored.size(), n);
      for (size_t i = 0; i < n; ++i)
        EXPECT_NEAR(restored[i], signal[i], 1e-7)
          << "N=" << n << " i=" << i;
    }
}

TEST(FFTLargeN, ParsevalTheorem)
{
  std::mt19937_64 rng(12121212);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(256), size_t(1024), size_t(4096), size_t(16384)})
    {
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      double energy_time = 0.0;
      for (size_t i = 0; i < n; ++i)
        energy_time += std::norm(signal[i]);

      const auto spectrum = FFTD::transformed(signal, false);

      double energy_freq = 0.0;
      for (size_t i = 0; i < n; ++i)
        energy_freq += std::norm(spectrum[i]);

      EXPECT_NEAR(energy_time, energy_freq / static_cast<double>(n), 1e-4)
        << "Parseval failed for N=" << n;
    }
}

TEST(FFTLargeN, DCComponentEqualsSumOfSignal)
{
  std::mt19937_64 rng(34343434);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  for (const size_t n : {size_t(8), size_t(64), size_t(512), size_t(4096)})
    {
      Array<Complex> signal;
      signal.reserve(n);
      Complex sum_signal(0.0, 0.0);
      for (size_t i = 0; i < n; ++i)
        {
          Complex val(dist(rng), dist(rng));
          signal.append(val);
          sum_signal += val;
        }

      const auto spectrum = FFTD::transformed(signal, false);
      expect_complex_near(spectrum[0], sum_signal, 1e-6);
    }
}

TEST(FFTLargeN, HermitianSymmetryLargeRealSignals)
{
  std::mt19937_64 rng(56565656);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(64), size_t(256), size_t(1024)})
    {
      Array<double> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(dist(rng));

      const auto spectrum = FFTD::transform(signal);
      ASSERT_EQ(spectrum.size(), n);

      EXPECT_NEAR(spectrum[0].imag(), 0.0, 1e-8)
        << "DC should be real, N=" << n;
      EXPECT_NEAR(spectrum[n / 2].imag(), 0.0, 1e-8)
        << "Nyquist should be real, N=" << n;

      for (size_t k = 1; k < n / 2; ++k)
        {
          EXPECT_NEAR(spectrum[k].real(), spectrum[n - k].real(), 1e-8)
            << "Re symmetry failed at k=" << k << " N=" << n;
          EXPECT_NEAR(spectrum[k].imag(), -spectrum[n - k].imag(), 1e-8)
            << "Im symmetry failed at k=" << k << " N=" << n;
        }
    }
}

TEST(FFTLargeN, RealTransformMatchesComplexLiftLargeN)
{
  std::mt19937_64 rng(78787878);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(64), size_t(256), size_t(1024)})
    {
      Array<double> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(dist(rng));

      const auto real_spectrum = FFTD::transform(signal);
      const auto complex_spectrum = FFTD::transformed(lift_real_input(signal), false);
      expect_complex_array_near(real_spectrum, complex_spectrum, 1e-7);
    }
}

TEST(FFTLargeN, ParallelLargeN)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(11111111);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(256), size_t(1024), size_t(4096), size_t(8192)})
    {
      Array<Complex> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(Complex(dist(rng), dist(rng)));

      const auto sequential = FFTD::transformed(signal, false);
      const auto parallel = FFTD::ptransformed(pool, signal, false);

      for (size_t i = 0; i < n; ++i)
        {
          EXPECT_NEAR(sequential[i].real(), parallel[i].real(), 1e-9)
            << "N=" << n << " i=" << i;
          EXPECT_NEAR(sequential[i].imag(), parallel[i].imag(), 1e-9)
            << "N=" << n << " i=" << i;
        }
    }
}

TEST(FFTLargeN, ParallelRealLargeN)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(22222222);
  std::uniform_real_distribution<double> dist(-5.0, 5.0);

  for (const size_t n : {size_t(256), size_t(1024), size_t(4096)})
    {
      Array<double> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(dist(rng));

      const auto sequential = FFTD::transform(signal);
      const auto parallel = FFTD::ptransform(pool, signal);
      expect_complex_array_near(sequential, parallel, 1e-8);
    }
}

TEST(FFTLargeN, ParallelMultiplyLargeN)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(33333333);
  std::uniform_real_distribution<double> dist(-2.0, 2.0);

  for (const size_t n : {size_t(128), size_t(512), size_t(2048)})
    {
      Array<double> a, b;
      a.reserve(n);
      b.reserve(n);
      for (size_t i = 0; i < n; ++i)
        {
          a.append(dist(rng));
          b.append(dist(rng));
        }

      const auto sequential = FFTD::multiply(a, b);
      const auto parallel = FFTD::pmultiply(pool, a, b);
      expect_real_array_near(sequential, parallel, 1e-6);
    }
}

TEST(FFTLargeN, ConvolutionNaiveCrossCheckLargeN)
{
  std::mt19937_64 rng(44444444);
  std::uniform_real_distribution<double> dist(-2.0, 2.0);

  for (const size_t n : {size_t(50), size_t(100), size_t(200)})
    {
      const size_t m = n / 2 + 1;
      Array<double> a, b;
      a.reserve(n);
      b.reserve(m);
      for (size_t i = 0; i < n; ++i)
        a.append(dist(rng));
      for (size_t i = 0; i < m; ++i)
        b.append(dist(rng));

      const auto expected = naive_convolution(a, b);
      const auto obtained = FFTD::multiply(a, b);
      expect_real_array_near(obtained, expected, 1e-6);
    }
}

// ---------------------------------------------------------------------------
// Edge cases for real transform
// ---------------------------------------------------------------------------

TEST(FFT, RealTransformN1)
{
  Array<double> signal = {42.0};
  const auto spectrum = FFTD::transform(signal);
  ASSERT_EQ(spectrum.size(), 1u);
  expect_complex_near(spectrum[0], Complex(42.0, 0.0), 1e-12);
}

TEST(FFT, RealTransformN2)
{
  Array<double> signal = {3.0, 7.0};
  const auto spectrum = FFTD::transform(signal);
  ASSERT_EQ(spectrum.size(), 2u);
  expect_complex_near(spectrum[0], Complex(10.0, 0.0), 1e-12);
  expect_complex_near(spectrum[1], Complex(-4.0, 0.0), 1e-12);

  const auto restored = FFTD::inverse_transform_real(spectrum);
  expect_real_array_near(restored, signal, 1e-12);
}

TEST(FFT, RealTransformN4)
{
  Array<double> signal = {1.0, 0.0, -1.0, 0.0};
  const auto spectrum = FFTD::transform(signal);
  ASSERT_EQ(spectrum.size(), 4u);
  expect_complex_near(spectrum[0], Complex(0.0, 0.0), 1e-12);
  expect_complex_near(spectrum[1], Complex(2.0, 0.0), 1e-12);
  expect_complex_near(spectrum[2], Complex(0.0, 0.0), 1e-12);
  expect_complex_near(spectrum[3], Complex(2.0, 0.0), 1e-12);
}

// ---------------------------------------------------------------------------
// Float stress tests
// ---------------------------------------------------------------------------

TEST(FFTFloatStress, LargeNRoundTrip)
{
  using FFTF = FFT<float>;
  using ComplexF = FFTF::Complex;

  std::mt19937_64 rng(55555555);
  std::uniform_real_distribution<float> dist(-5.0f, 5.0f);

  for (const size_t n : {size_t(64), size_t(256), size_t(1024)})
    {
      Array<ComplexF> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(ComplexF(dist(rng), dist(rng)));

      Array<ComplexF> work = signal;
      FFTF::transform(work, false);
      FFTF::transform(work, true);

      const float tol = 5e-3f * std::log2(static_cast<float>(n));
      expect_typed_complex_array_near(work, signal, tol);
    }
}

TEST(FFTFloatStress, ParsevalTheorem)
{
  using FFTF = FFT<float>;
  using ComplexF = FFTF::Complex;

  std::mt19937_64 rng(66666666);
  std::uniform_real_distribution<float> dist(-3.0f, 3.0f);

  for (const size_t n : {size_t(64), size_t(256), size_t(1024)})
    {
      Array<ComplexF> signal;
      signal.reserve(n);
      for (size_t i = 0; i < n; ++i)
        signal.append(ComplexF(dist(rng), dist(rng)));

      float energy_time = 0.0f;
      for (size_t i = 0; i < n; ++i)
        energy_time += std::norm(signal[i]);

      auto spectrum = FFTF::transformed(signal, false);
      float energy_freq = 0.0f;
      for (size_t i = 0; i < n; ++i)
        energy_freq += std::norm(spectrum[i]);

      const float tol = energy_time * 1e-3f;
      EXPECT_NEAR(energy_time, energy_freq / static_cast<float>(n), tol)
        << "Float Parseval failed for N=" << n;
    }
}

TEST(FFTFloatStress, FloatMultiply)
{
  using FFTF = FFT<float>;

  Array<float> a = {1.0f, 2.0f, 3.0f, -1.0f};
  Array<float> b = {4.0f, -1.0f, 0.5f};

  const auto result = FFTF::multiply(a, b);
  ASSERT_EQ(result.size(), 6u);

  // 1*4=4, 1*(-1)+2*4=7, 1*0.5+2*(-1)+3*4=10.5,
  // 2*0.5+3*(-1)+(-1)*4=-6, 3*0.5+(-1)*(-1)=2.5, (-1)*0.5=-0.5
  Array<float> expected = {4.0f, 7.0f, 10.5f, -6.0f, 2.5f, -0.5f};
  for (size_t i = 0; i < expected.size(); ++i)
    EXPECT_NEAR(result[i], expected[i], 1e-3f) << "i=" << i;
}

TEST(FFTFloatStress, HighDynamicRange)
{
  // With extreme dynamic range (1e+12 vs 1e-12), FFT butterflies add/subtract
  // these values, losing precision on small entries.  Tolerance must be
  // proportional to the largest magnitude in the signal.
  Array<Complex> signal;
  signal.reserve(8);
  signal.append(Complex(1e+12, 0.0));
  signal.append(Complex(1e-12, 0.0));
  signal.append(Complex(-1e+12, 0.0));
  signal.append(Complex(1e-12, 0.0));
  signal.append(Complex(1e+6, 0.0));
  signal.append(Complex(-1e+6, 0.0));
  signal.append(Complex(1e-6, 0.0));
  signal.append(Complex(-1e-6, 0.0));

  double max_magnitude = 0.0;
  for (size_t i = 0; i < signal.size(); ++i)
    max_magnitude = std::max(max_magnitude, std::abs(signal[i]));

  Array<Complex> work = signal;
  FFTD::transform(work, false);
  FFTD::transform(work, true);

  // Use Parseval as the correctness check instead of element-wise comparison:
  // round-trip preserves total energy even when individual small elements
  // are lost to floating-point cancellation.
  double energy_original = 0.0;
  double energy_restored = 0.0;
  for (size_t i = 0; i < signal.size(); ++i)
    {
      energy_original += std::norm(signal[i]);
      energy_restored += std::norm(work[i]);
    }
  EXPECT_NEAR(energy_original, energy_restored, energy_original * 1e-10);

  // Elements whose magnitude is comparable to the max should round-trip well
  const double tol = max_magnitude * 1e-8;
  for (size_t i = 0; i < signal.size(); ++i)
    if (std::abs(signal[i]) > max_magnitude * 1e-6)
      {
        EXPECT_NEAR(work[i].real(), signal[i].real(), tol) << "i=" << i;
        EXPECT_NEAR(work[i].imag(), signal[i].imag(), tol) << "i=" << i;
      }
}

TEST(FFTFloatStress, HighDynamicRangeLargeN)
{
  std::mt19937_64 rng(77777777);

  const size_t n = 1024;
  Array<Complex> signal;
  signal.reserve(n);
  for (size_t i = 0; i < n; ++i)
    {
      const double scale = (i % 3 == 0) ? 1e+10 : ((i % 3 == 1) ? 1e-10 : 1.0);
      std::uniform_real_distribution<double> dist(-scale, scale);
      signal.append(Complex(dist(rng), dist(rng)));
    }

  double energy_time = 0.0;
  for (size_t i = 0; i < n; ++i)
    energy_time += std::norm(signal[i]);

  const auto spectrum = FFTD::transformed(signal, false);

  double energy_freq = 0.0;
  for (size_t i = 0; i < n; ++i)
    energy_freq += std::norm(spectrum[i]);

  const double rel_error = std::abs(energy_time - energy_freq / static_cast<double>(n))
                           / energy_time;
  EXPECT_LT(rel_error, 1e-6) << "Parseval with high dynamic range, N=" << n;
}

// ---------------------------------------------------------------------------
// Convolution algebraic properties
// ---------------------------------------------------------------------------

TEST(FFTConvolution, Commutativity)
{
  std::mt19937_64 rng(88888888);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  for (int trial = 0; trial < 5; ++trial)
    {
      const size_t na = static_cast<size_t>(trial + 3);
      const size_t nb = static_cast<size_t>(trial + 2);

      Array<double> a, b;
      a.reserve(na);
      b.reserve(nb);
      for (size_t i = 0; i < na; ++i)
        a.append(dist(rng));
      for (size_t i = 0; i < nb; ++i)
        b.append(dist(rng));

      const auto ab = FFTD::multiply(a, b);
      const auto ba = FFTD::multiply(b, a);
      expect_real_array_near(ab, ba, 1e-9);
    }
}

TEST(FFTConvolution, IdentityElement)
{
  Array<double> a = {1.0, 2.0, 3.0, -1.0, 0.5};
  Array<double> identity = {1.0};

  const auto result = FFTD::multiply(a, identity);
  expect_real_array_near(result, a, 1e-12);
}

TEST(FFTConvolution, ShiftByDelayedImpulse)
{
  Array<double> a = {1.0, 2.0, 3.0, -1.0};
  Array<double> delay2 = {0.0, 0.0, 1.0};

  const auto result = FFTD::multiply(a, delay2);
  ASSERT_EQ(result.size(), 6u);

  Array<double> expected = {0.0, 0.0, 1.0, 2.0, 3.0, -1.0};
  expect_real_array_near(result, expected, 1e-12);
}

TEST(FFTConvolution, ScalingByConstant)
{
  Array<double> a = {1.0, -2.0, 3.0, 0.5};
  Array<double> scale = {3.5};

  const auto result = FFTD::multiply(a, scale);
  ASSERT_EQ(result.size(), 4u);

  for (size_t i = 0; i < a.size(); ++i)
    EXPECT_NEAR(result[i], a[i] * 3.5, 1e-10) << "i=" << i;
}

TEST(FFTConvolution, ComplexCommutativity)
{
  std::mt19937_64 rng(99999999);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  Array<Complex> a, b;
  for (size_t i = 0; i < 5; ++i)
    a.append(Complex(dist(rng), dist(rng)));
  for (size_t i = 0; i < 4; ++i)
    b.append(Complex(dist(rng), dist(rng)));

  const auto ab = FFTD::multiply(a, b);
  const auto ba = FFTD::multiply(b, a);
  expect_complex_array_near(ab, ba, 1e-9);
}

TEST(FFTConvolution, SingleElementInputs)
{
  Array<double> a = {5.0};
  Array<double> b = {3.0};
  const auto result = FFTD::multiply(a, b);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_NEAR(result[0], 15.0, 1e-12);
}

TEST(FFTConvolution, ConvolutionTheoremDirect)
{
  std::mt19937_64 rng(12345678);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  const size_t na = 5;
  const size_t nb = 4;
  const size_t required = na + nb - 1;
  const size_t n = 8;

  Array<Complex> a, b;
  for (size_t i = 0; i < na; ++i)
    a.append(Complex(dist(rng), dist(rng)));
  for (size_t i = 0; i < nb; ++i)
    b.append(Complex(dist(rng), dist(rng)));

  auto conv_result = FFTD::multiply(a, b);

  Array<Complex> a_padded, b_padded;
  a_padded.reserve(n);
  b_padded.reserve(n);
  for (size_t i = 0; i < na; ++i)
    a_padded.append(a[i]);
  for (size_t i = na; i < n; ++i)
    a_padded.append(Complex(0.0, 0.0));
  for (size_t i = 0; i < nb; ++i)
    b_padded.append(b[i]);
  for (size_t i = nb; i < n; ++i)
    b_padded.append(Complex(0.0, 0.0));

  auto fa = FFTD::transformed(a_padded, false);
  auto fb = FFTD::transformed(b_padded, false);

  Array<Complex> product;
  product.reserve(n);
  for (size_t i = 0; i < n; ++i)
    product.append(fa[i] * fb[i]);

  auto from_theorem = FFTD::transformed(product, true);

  for (size_t i = 0; i < required; ++i)
    expect_complex_near(conv_result[i], from_theorem[i], 1e-8);
}

TEST(FFTWindows, CanonicalSmallWindows)
{
  const auto hann = FFTD::hann_window(5);
  const auto hamming = FFTD::hamming_window(5);
  const auto blackman = FFTD::blackman_window(5);

  Array<double> expected_hann = {0.0, 0.5, 1.0, 0.5, 0.0};
  Array<double> expected_hamming = {0.08, 0.54, 1.0, 0.54, 0.08};
  Array<double> expected_blackman = {0.0, 0.34, 1.0, 0.34, 0.0};

  expect_real_array_near(hann, expected_hann, 1e-12);
  expect_real_array_near(hamming, expected_hamming, 1e-12);
  expect_real_array_near(blackman, expected_blackman, 1e-12);
}

TEST(FFTWindows, DegenerateSizes)
{
  EXPECT_TRUE(FFTD::hann_window(0).is_empty());
  EXPECT_TRUE(FFTD::hamming_window(0).is_empty());
  EXPECT_TRUE(FFTD::blackman_window(0).is_empty());

  Array<double> singleton = {1.0};
  expect_real_array_near(FFTD::hann_window(1), singleton, 1e-12);
  expect_real_array_near(FFTD::hamming_window(1), singleton, 1e-12);
  expect_real_array_near(FFTD::blackman_window(1), singleton, 1e-12);
}

TEST(FFTWindows, ApplyWindowRealAndComplex)
{
  Array<double> signal = {2.0, -1.0, 0.5, 4.0};
  Array<double> window = {0.0, 0.5, 1.0, 0.25};
  Array<double> expected_real = {0.0, -0.5, 0.5, 1.0};

  const auto windowed_real = FFTD::apply_window(signal, window);
  expect_real_array_near(windowed_real, expected_real, 1e-12);

  Array<Complex> complex_signal = {
    Complex(2.0, -2.0),
    Complex(-1.0, 3.0),
    Complex(0.5, 0.5),
    Complex(4.0, -1.0)
  };
  Array<Complex> expected_complex = {
    Complex(0.0, 0.0),
    Complex(-0.5, 1.5),
    Complex(0.5, 0.5),
    Complex(1.0, -0.25)
  };

  const auto windowed_complex = FFTD::apply_window(complex_signal, window);
  expect_complex_array_near(windowed_complex, expected_complex, 1e-12);

  std::vector<double> signal_vec(signal.begin(), signal.end());
  std::vector<double> window_vec(window.begin(), window.end());
  expect_real_array_near(FFTD::apply_window(signal_vec, window_vec), expected_real, 1e-12);

  expect_real_array_near(FFTD::apply_hann_window(signal),
                         FFTD::apply_window(signal, FFTD::hann_window(signal.size())),
                         1e-12);
  expect_complex_array_near(FFTD::apply_hamming_window(complex_signal),
                            FFTD::apply_window(complex_signal,
                                               FFTD::hamming_window(complex_signal.size())),
                            1e-12);
}

TEST(FFTWindows, ApplyWindowRejectsMismatchedSizes)
{
  Array<double> signal = {1.0, 2.0, 3.0};
  Array<double> short_window = {0.5, 1.0};
  Array<Complex> complex_signal = {
    Complex(1.0, 0.0),
    Complex(2.0, 1.0),
    Complex(3.0, -1.0)
  };

  EXPECT_THROW(FFTD::apply_window(signal, short_window), std::invalid_argument);
  EXPECT_THROW(FFTD::apply_window(complex_signal, short_window), std::invalid_argument);
}

TEST(FFTWindows, WindowedSpectrumMatchesManualPipeline)
{
  Array<double> signal = {1.0, -0.5, 2.0, 0.25, -1.0, 3.0, 0.5, -2.0};
  const auto window = FFTD::hann_window(signal.size());

  const auto manual_real = FFTD::spectrum(FFTD::apply_window(signal, window));
  const auto helper_real = FFTD::windowed_spectrum(signal, window);
  expect_complex_array_near(helper_real, manual_real, 1e-12);

  Array<Complex> complex_signal = lift_real_input(signal);
  const auto manual_complex = FFTD::spectrum(FFTD::apply_window(complex_signal, window));
  const auto helper_complex = FFTD::windowed_spectrum(complex_signal, window);
  expect_complex_array_near(helper_complex, manual_complex, 1e-12);

  std::vector<double> signal_vec(signal.begin(), signal.end());
  std::vector<double> window_vec(window.begin(), window.end());
  expect_complex_array_near(FFTD::windowed_spectrum(signal_vec, window_vec),
                            manual_real, 1e-12);
}

TEST(FFTSTFT, FrameSignalExactAndPadded)
{
  Array<double> signal = {1.0, 2.0, 3.0, 4.0, 5.0};

  const auto padded = FFTD::frame_signal(signal, 4, 2, true);
  ASSERT_EQ(padded.size(), 3u);
  expect_real_array_near(padded[0], Array<double>({1.0, 2.0, 3.0, 4.0}), 1e-12);
  expect_real_array_near(padded[1], Array<double>({3.0, 4.0, 5.0, 0.0}), 1e-12);
  expect_real_array_near(padded[2], Array<double>({5.0, 0.0, 0.0, 0.0}), 1e-12);

  const auto exact = FFTD::frame_signal(signal, 4, 2, false);
  ASSERT_EQ(exact.size(), 1u);
  expect_real_array_near(exact[0], Array<double>({1.0, 2.0, 3.0, 4.0}), 1e-12);
}

TEST(FFTSTFT, MatchesManualFramePipeline)
{
  Array<double> signal = {1.0, -0.5, 2.0, 0.25, -1.0, 3.0, 0.5};
  Array<double> window = {1.0, 0.5, 0.5, 1.0};

  const auto frames = FFTD::frame_signal(signal, window.size(), 2, true);
  const auto spectrogram = FFTD::stft(signal, window, 2, true);

  ASSERT_EQ(spectrogram.size(), frames.size());
  for (size_t i = 0; i < frames.size(); ++i)
    {
      const auto expected = FFTD::transform_padded(FFTD::apply_window(frames[i], window));
      expect_complex_array_near(spectrogram[i], expected, 1e-12);
    }

  const auto hann_stft = FFTD::stft(signal, 4, 2, true);
  ASSERT_EQ(hann_stft.size(), frames.size());
  ASSERT_EQ(hann_stft[0].size(), 4u);
}

TEST(FFTSTFT, RejectsInvalidParameters)
{
  Array<double> signal = {1.0, 2.0, 3.0};
  Array<double> window = {1.0, 0.5};

  EXPECT_THROW(FFTD::frame_signal(signal, 0, 1, true), std::invalid_argument);
  EXPECT_THROW(FFTD::frame_signal(signal, 2, 0, true), std::invalid_argument);
  EXPECT_THROW(FFTD::stft(signal, Array<double>(), 1, true), std::invalid_argument);
  EXPECT_NO_THROW(FFTD::stft(signal, window, 1, true));
}

TEST(FFTSTFT, IstftReconstructsWindowedFrames)
{
  ThreadPool pool(4);
  Array<double> signal = {1.0, -0.5, 2.0, 0.25, -1.0, 3.0, 0.5, -2.0, 1.25};
  Array<double> analysis_window = {1.0, 0.75, 0.5, 1.0};
  Array<double> synthesis_window = {0.5, 1.0, 1.0, 0.5};

  const auto spectrogram = FFTD::stft(signal, analysis_window, 2, true);
  const auto reconstructed =
    FFTD::istft(spectrogram, analysis_window, synthesis_window, 2, signal.size());
  const auto same_window =
    FFTD::istft(spectrogram, analysis_window, 2, signal.size());
  const auto parallel =
    FFTD::pistft(pool, spectrogram, analysis_window, synthesis_window, 2,
                 signal.size());

  expect_real_array_near(reconstructed, signal, 1e-10);
  expect_real_array_near(same_window, signal, 1e-10);
  expect_real_array_near(parallel, signal, 1e-10);
}

TEST(FFTSTFT, IstftRejectsInvalidParameters)
{
  Array<double> analysis_window = {1.0, 0.5, 0.5, 1.0};
  Array<double> synthesis_window = {1.0, 1.0, 1.0, 1.0};
  Array<Array<Complex>> spectrogram = {
    Array<Complex>({Complex(1.0, 0.0), Complex(2.0, 0.0),
                    Complex(3.0, 0.0), Complex(4.0, 0.0)}),
    Array<Complex>({Complex(0.5, 0.0), Complex(1.0, 0.0),
                    Complex(1.5, 0.0), Complex(2.0, 0.0)})
  };

  EXPECT_THROW(FFTD::istft(spectrogram, Array<double>(), synthesis_window, 2),
               std::invalid_argument);
  EXPECT_THROW(FFTD::istft(spectrogram, analysis_window, Array<double>(), 2),
               std::invalid_argument);
  EXPECT_THROW(FFTD::istft(spectrogram, analysis_window, synthesis_window, 0),
               std::invalid_argument);
  EXPECT_THROW(FFTD::istft(spectrogram, analysis_window, synthesis_window, 2, 16),
               std::invalid_argument);

  Array<Array<Complex>> mismatched = spectrogram;
  mismatched.append(Array<Complex>({Complex(1.0, 0.0), Complex(0.0, 0.0)}));
  EXPECT_THROW(FFTD::istft(mismatched, analysis_window, synthesis_window, 2),
               std::invalid_argument);

  Array<Array<Complex>> too_small = {
    Array<Complex>({Complex(1.0, 0.0), Complex(0.0, 0.0)})
  };
  EXPECT_THROW(FFTD::istft(too_small, analysis_window, synthesis_window, 1),
               std::invalid_argument);
}

TEST(FFTSTFT, CenteredOptionsAndOverlapConstraints)
{
  std::mt19937_64 rng(20260305);
  std::uniform_real_distribution<double> dist(-1.0, 1.0);

  Array<double> signal;
  signal.reserve(23);
  for (size_t i = 0; i < 23; ++i)
    signal.append(dist(rng));

  const auto window = FFTD::hann_window(8);
  FFTD::STFTOptions stft_options;
  stft_options.hop_size = 4;
  stft_options.centered = true;
  stft_options.pad_end = true;
  stft_options.fft_size = 16;
  stft_options.validate_nola = true;

  FFTD::ISTFTOptions istft_options;
  istft_options.hop_size = 4;
  istft_options.centered = true;
  istft_options.signal_length = signal.size();
  istft_options.validate_nola = true;

  const auto spectrogram = FFTD::stft(signal, window, stft_options);
  ASSERT_FALSE(spectrogram.is_empty());
  EXPECT_EQ(spectrogram[0].size(), 16u);

  const auto reconstructed = FFTD::istft(spectrogram, window, istft_options);
  expect_real_array_near(reconstructed, signal, 1e-9);

  Array<double> rectangular = {1.0, 1.0, 1.0, 1.0};
  const auto profile = FFTD::window_overlap_profile(rectangular, rectangular, 2);
  expect_real_array_near(profile, Array<double>({2.0, 2.0}), 1e-12);
  EXPECT_TRUE(FFTD::satisfies_nola(rectangular, 2));
  EXPECT_TRUE(FFTD::satisfies_cola(rectangular, 2));

  Array<double> bad_window = {1.0, 0.0, 1.0, 0.0};
  EXPECT_FALSE(FFTD::satisfies_nola(bad_window, 2));

  FFTD::STFTOptions bad_options;
  bad_options.hop_size = 2;
  bad_options.validate_nola = true;
  EXPECT_THROW(FFTD::stft(signal, bad_window, bad_options), std::domain_error);
}

TEST(FFTSTFT, ParallelAndStreamingProcessorMatchOffline)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(20260306);
  std::uniform_real_distribution<double> dist(-1.0, 1.0);

  Array<double> signal;
  signal.reserve(29);
  for (size_t i = 0; i < 29; ++i)
    signal.append(dist(rng));

  const auto window = FFTD::hann_window(8);
  FFTD::STFTOptions options;
  options.hop_size = 4;
  options.centered = true;
  options.pad_end = true;
  options.fft_size = 16;
  options.validate_nola = true;

  const auto offline = FFTD::stft(signal, window, options);
  const auto parallel = FFTD::pstft(pool, signal, window, options);
  expect_spectrogram_near(parallel, offline, 1e-12);

  FFTD::STFTProcessor processor(window, options);
  Array<Array<Complex>> streamed;
  for (size_t offset = 0; offset < signal.size();)
    {
      const size_t length = std::min(size_t(5 + (offset % 4)),
                                     signal.size() - offset);
      Array<double> chunk;
      chunk.reserve(length);
      for (size_t i = 0; i < length; ++i)
        chunk.append(signal[offset + i]);

      append_spectrogram(streamed, processor.process_block(chunk));
      offset += length;
    }
  append_spectrogram(streamed, processor.flush());
  expect_spectrogram_near(streamed, offline, 1e-12);
  EXPECT_THROW(processor.process_block(Array<double>({1.0})), std::runtime_error);

  processor.reset();
  streamed = {};
  append_spectrogram(streamed, processor.process_block(signal));
  append_spectrogram(streamed, processor.flush());
  expect_spectrogram_near(streamed, offline, 1e-12);

  FFTD::STFTProcessor parallel_processor(window, options);
  Array<Array<Complex>> parallel_streamed;
  for (size_t offset = 0; offset < signal.size();)
    {
      const size_t length = std::min(size_t(3 + (offset % 5)),
                                     signal.size() - offset);
      Array<double> chunk;
      chunk.reserve(length);
      for (size_t i = 0; i < length; ++i)
        chunk.append(signal[offset + i]);

      append_spectrogram(parallel_streamed,
                         parallel_processor.pprocess_block(pool, chunk));
      offset += length;
    }
  append_spectrogram(parallel_streamed, parallel_processor.pflush(pool));
  expect_spectrogram_near(parallel_streamed, offline, 1e-12);

  FFTD::STFTProcessor empty_processor(window, options);
  EXPECT_TRUE(empty_processor.flush().is_empty());
}

TEST(FFTSTFT, BatchedStftAndIstftMatchScalarPaths)
{
  ThreadPool pool(4);
  Array<Array<double>> signals = {
    Array<double>({1.0, -0.5, 0.25, 2.0, -1.0, 0.5, 1.5, -0.75, 0.25}),
    Array<double>({-1.0, 0.75, 0.5, -0.25, 1.25, -1.5, 0.0, 0.5, 1.0, -0.5,
                   0.25}),
    Array<double>({0.25, -0.125, 0.75, 1.5, -0.5, 0.0, 0.5})
  };

  const auto window = FFTD::hann_window(8);
  FFTD::STFTOptions stft_options;
  stft_options.hop_size = 4;
  stft_options.centered = true;
  stft_options.pad_end = true;
  stft_options.fft_size = 16;
  stft_options.validate_nola = true;

  const auto batch = FFTD::batched_stft(signals, window, stft_options);
  const auto parallel_batch = FFTD::pbatched_stft(pool, signals, window, stft_options);
  ASSERT_EQ(batch.size(), signals.size());
  ASSERT_EQ(parallel_batch.size(), signals.size());
  for (size_t i = 0; i < signals.size(); ++i)
    {
      const auto expected = FFTD::stft(signals[i], window, stft_options);
      expect_spectrogram_near(batch[i], expected, 1e-12);
      expect_spectrogram_near(parallel_batch[i], expected, 1e-12);
    }

  FFTD::ISTFTOptions istft_options;
  istft_options.hop_size = 4;
  istft_options.centered = true;
  istft_options.validate_nola = true;

  Array<size_t> lengths;
  lengths.reserve(signals.size());
  for (size_t i = 0; i < signals.size(); ++i)
    lengths.append(signals[i].size());

  const auto reconstructed =
    FFTD::batched_istft(batch, window, istft_options, lengths);
  const auto parallel_reconstructed =
    FFTD::pbatched_istft(pool, batch, window, istft_options, lengths);

  ASSERT_EQ(reconstructed.size(), signals.size());
  ASSERT_EQ(parallel_reconstructed.size(), signals.size());
  for (size_t i = 0; i < signals.size(); ++i)
    {
      expect_real_array_near(reconstructed[i], signals[i], 1e-10);
      expect_real_array_near(parallel_reconstructed[i], signals[i], 1e-10);
    }

  EXPECT_THROW(FFTD::batched_istft(batch,
                                   window,
                                   istft_options,
                                   Array<size_t>({signals[0].size()})),
               std::invalid_argument);
}

TEST(FFTSTFT, ISTFTProcessorMatchesOffline)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(20260307);
  std::uniform_real_distribution<double> dist(-1.0, 1.0);

  Array<double> signal;
  signal.reserve(27);
  for (size_t i = 0; i < 27; ++i)
    signal.append(dist(rng));

  const auto window = FFTD::hann_window(8);
  FFTD::STFTOptions stft_options;
  stft_options.hop_size = 4;
  stft_options.centered = true;
  stft_options.pad_end = true;
  stft_options.fft_size = 16;
  stft_options.validate_nola = true;

  FFTD::ISTFTOptions istft_options;
  istft_options.hop_size = 4;
  istft_options.centered = true;
  istft_options.validate_nola = true;

  const auto spectrogram = FFTD::stft(signal, window, stft_options);
  const auto expected = FFTD::istft(spectrogram, window, istft_options);

  FFTD::ISTFTProcessor processor(spectrogram[0].size(), window, istft_options);
  Array<double> streamed;
  for (size_t offset = 0; offset < spectrogram.size();)
    {
      const size_t length = std::min(size_t(2 + (offset % 3)),
                                     spectrogram.size() - offset);
      Array<Array<Complex>> chunk;
      for (size_t i = 0; i < length; ++i)
        chunk.append(spectrogram[offset + i]);

      const auto partial = processor.process_block(chunk);
      for (size_t i = 0; i < partial.size(); ++i)
        streamed.append(partial[i]);
      offset += length;
    }

  const auto tail = processor.flush();
  for (size_t i = 0; i < tail.size(); ++i)
    streamed.append(tail[i]);

  expect_real_array_near(streamed, expected, 1e-10);
  EXPECT_THROW(processor.process_frame(spectrogram[0]), std::runtime_error);

  processor.reset();
  streamed = processor.process_block(spectrogram);
  const auto full_tail = processor.flush();
  for (size_t i = 0; i < full_tail.size(); ++i)
    streamed.append(full_tail[i]);
  expect_real_array_near(streamed, expected, 1e-10);

  FFTD::ISTFTProcessor parallel_processor(spectrogram[0].size(), window, istft_options);
  Array<double> parallel_streamed;
  for (size_t offset = 0; offset < spectrogram.size();)
    {
      const size_t length = std::min(size_t(1 + (offset % 4)),
                                     spectrogram.size() - offset);
      Array<Array<Complex>> chunk;
      for (size_t i = 0; i < length; ++i)
        chunk.append(spectrogram[offset + i]);

      const auto partial = parallel_processor.pprocess_block(pool, chunk);
      for (size_t i = 0; i < partial.size(); ++i)
        parallel_streamed.append(partial[i]);
      offset += length;
    }

  const auto parallel_tail = parallel_processor.pflush(pool);
  for (size_t i = 0; i < parallel_tail.size(); ++i)
    parallel_streamed.append(parallel_tail[i]);
  expect_real_array_near(parallel_streamed, expected, 1e-10);

  FFTD::ISTFTProcessor empty_processor(spectrogram[0].size(), window, istft_options);
  EXPECT_TRUE(empty_processor.flush().is_empty());
}

TEST(FFTSTFT, BatchedProcessorsMatchOfflineAndParallelPaths)
{
  ThreadPool pool(4);
  Array<Array<double>> signals = {
    Array<double>({1.0, -0.5, 0.25, 2.0, -1.0, 0.5, 1.5, -0.75, 0.25, 0.0}),
    Array<double>({-1.0, 0.75, 0.5, -0.25, 1.25, -1.5, 0.0, 0.5, 1.0}),
    Array<double>({0.25, -0.125, 0.75, 1.5, -0.5, 0.0, 0.5, -0.75})
  };
  const Array<size_t> chunk_pattern = {3, 2, 4, 1};
  const Array<size_t> frame_pattern = {1, 2, 1, 3};

  const auto window = FFTD::hann_window(8);
  FFTD::STFTOptions stft_options;
  stft_options.hop_size = 4;
  stft_options.centered = true;
  stft_options.pad_end = true;
  stft_options.fft_size = 16;
  stft_options.validate_nola = true;

  FFTD::ISTFTOptions istft_options;
  istft_options.hop_size = 4;
  istft_options.centered = true;
  istft_options.validate_nola = true;

  Array<size_t> lengths;
  lengths.reserve(signals.size());
  for (size_t i = 0; i < signals.size(); ++i)
    lengths.append(signals[i].size());

  const auto offline = FFTD::batched_stft(signals, window, stft_options);
  ASSERT_EQ(offline.size(), signals.size());

  FFTD::BatchedSTFTProcessor analyzer(signals.size(), window, stft_options);
  FFTD::BatchedSTFTProcessor parallel_analyzer(signals.size(), window, stft_options);
  Array<Array<Array<Complex>>> streamed(signals.size(), Array<Array<Complex>>());
  Array<Array<Array<Complex>>> parallel_streamed(signals.size(), Array<Array<Complex>>());
  Array<size_t> offsets = {0, 0, 0};

  for (size_t step = 0;; ++step)
    {
      bool any_pending = false;
      Array<Array<double>> block;
      for (size_t channel = 0; channel < signals.size(); ++channel)
        {
          const size_t remaining = signals[channel].size() - offsets[channel];
          const size_t length = std::min(chunk_pattern[step % chunk_pattern.size()],
                                         remaining);
          Array<double> chunk;
          chunk.reserve(length);
          for (size_t i = 0; i < length; ++i)
            chunk.append(signals[channel][offsets[channel] + i]);
          offsets(channel) += length;
          any_pending = any_pending or length != 0;
          block.append(chunk);
        }

      if (not any_pending)
        break;

      const auto emitted = analyzer.process_block(block);
      const auto parallel_emitted = parallel_analyzer.pprocess_block(pool, block);
      for (size_t channel = 0; channel < signals.size(); ++channel)
        {
          append_spectrogram(streamed(channel), emitted[channel]);
          append_spectrogram(parallel_streamed(channel), parallel_emitted[channel]);
        }
    }

  const auto tail = analyzer.flush();
  const auto parallel_tail = parallel_analyzer.pflush(pool);
  for (size_t channel = 0; channel < signals.size(); ++channel)
    {
      append_spectrogram(streamed(channel), tail[channel]);
      append_spectrogram(parallel_streamed(channel), parallel_tail[channel]);
      expect_spectrogram_near(streamed[channel], offline[channel], 1e-12);
      expect_spectrogram_near(parallel_streamed[channel], offline[channel], 1e-12);
    }

  FFTD::BatchedISTFTProcessor inverse(signals.size(),
                                      offline[0][0].size(),
                                      window,
                                      istft_options,
                                      lengths);
  FFTD::BatchedISTFTProcessor parallel_inverse(signals.size(),
                                               offline[0][0].size(),
                                               window,
                                               istft_options,
                                               lengths);
  Array<Array<double>> reconstructed(signals.size(), Array<double>());
  Array<Array<double>> parallel_reconstructed(signals.size(), Array<double>());
  Array<size_t> frame_offsets = {0, 0, 0};

  for (size_t step = 0;; ++step)
    {
      bool any_pending = false;
      Array<Array<Array<Complex>>> block;
      for (size_t channel = 0; channel < offline.size(); ++channel)
        {
          const size_t remaining = offline[channel].size() - frame_offsets[channel];
          const size_t length = std::min(frame_pattern[step % frame_pattern.size()],
                                         remaining);
          Array<Array<Complex>> chunk;
          for (size_t i = 0; i < length; ++i)
            chunk.append(offline[channel][frame_offsets[channel] + i]);
          frame_offsets(channel) += length;
          any_pending = any_pending or length != 0;
          block.append(chunk);
        }

      if (not any_pending)
        break;

      const auto emitted = inverse.process_block(block);
      const auto parallel_emitted = parallel_inverse.pprocess_block(pool, block);
      for (size_t channel = 0; channel < signals.size(); ++channel)
        {
          append_real_samples(reconstructed(channel), emitted[channel]);
          append_real_samples(parallel_reconstructed(channel), parallel_emitted[channel]);
        }
    }

  const auto reconstructed_tail = inverse.flush();
  const auto parallel_reconstructed_tail = parallel_inverse.pflush(pool);
  for (size_t channel = 0; channel < signals.size(); ++channel)
    {
      append_real_samples(reconstructed(channel), reconstructed_tail[channel]);
      append_real_samples(parallel_reconstructed(channel),
                          parallel_reconstructed_tail[channel]);
      expect_real_array_near(reconstructed[channel], signals[channel], 1e-10);
      expect_real_array_near(parallel_reconstructed[channel], signals[channel], 1e-10);
    }
}

TEST(FFTSTFT, DefaultConstructedProcessorsRemainUnconfigured)
{
  FFTD::STFTProcessor analyzer;
  FFTD::ISTFTProcessor inverse;

  EXPECT_FALSE(analyzer.configured());
  EXPECT_FALSE(inverse.configured());
  EXPECT_THROW(analyzer.reset(), std::runtime_error);
  EXPECT_THROW(analyzer.process_block(Array<double>({1.0, 2.0})), std::runtime_error);
  EXPECT_THROW(analyzer.flush(), std::runtime_error);
  EXPECT_THROW(inverse.reset(), std::runtime_error);
  EXPECT_THROW(inverse.process_frame(Array<Complex>({Complex(1.0, 0.0)})),
               std::runtime_error);
  EXPECT_THROW(inverse.flush(), std::runtime_error);
}

TEST(FFTFiltFilt, IdentityAndConstantSignal)
{
  Array<double> signal = {3.5, 3.5, 3.5, 3.5, 3.5, 3.5};
  Array<double> identity = {1.0};
  Array<double> smoother = {0.25, 0.5, 0.25};

  expect_real_array_near(FFTD::filtfilt(signal, identity), signal, 1e-12);
  expect_real_array_near(FFTD::filtfilt(signal, smoother), signal, 1e-12);
}

TEST(FFTFiltFilt, MatchesNaiveReflectiveReference)
{
  std::mt19937_64 rng(56473829);
  std::uniform_real_distribution<double> dist(-2.0, 2.0);

  Array<double> signal;
  signal.reserve(41);
  for (size_t i = 0; i < 41; ++i)
    signal.append(dist(rng));

  Array<double> coeffs = {0.1, 0.2, 0.4, 0.2, 0.1};

  const auto expected = naive_filtfilt(signal, coeffs);
  const auto obtained = FFTD::filtfilt(signal, coeffs, 16);
  expect_real_array_near(obtained, expected, 1e-7);
}

TEST(FFTFiltFilt, ParallelAndContainerOverloads)
{
  ThreadPool pool(4);
  Array<double> signal = {1.0, -1.0, 2.0, -2.0, 0.5, 3.0, -0.5, 1.5};
  Array<double> coeffs = {0.2, 0.6, 0.2};

  const auto sequential = FFTD::filtfilt(signal, coeffs, 8);
  const auto parallel = FFTD::pfiltfilt(pool, signal, coeffs, 8);
  expect_real_array_near(parallel, sequential, 1e-8);

  std::vector<double> signal_vec(signal.begin(), signal.end());
  std::vector<double> coeffs_vec(coeffs.begin(), coeffs.end());
  expect_real_array_near(FFTD::filtfilt(signal_vec, coeffs_vec, 8), sequential, 1e-8);
}

TEST(FFTFiltFilt, EmptyInputsReturnEmpty)
{
  Array<double> empty;
  Array<double> coeffs = {0.25, 0.5, 0.25};
  Array<double> signal = {1.0, 2.0, 3.0};

  EXPECT_TRUE(FFTD::filtfilt(empty, coeffs).is_empty());
  EXPECT_TRUE(FFTD::filtfilt(signal, empty).is_empty());
}

TEST(FFTFiltFiltIIR, IdentityAndConstantSignal)
{
  Array<double> signal = {2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5};
  Array<double> numerator = {0.075, 0.15, 0.075};
  Array<double> denominator = {1.0, -0.9, 0.2};

  expect_real_array_near(FFTD::filtfilt(signal,
                                        Array<double>({1.0}),
                                        Array<double>({1.0})),
                         signal, 1e-12);
  expect_real_array_near(FFTD::filtfilt(signal, numerator, denominator),
                         signal, 1e-10);
}

TEST(FFTFiltFiltIIR, MatchesNaiveReferenceAndBiquadWrapper)
{
  std::mt19937_64 rng(11235813);
  std::uniform_real_distribution<double> dist(-2.0, 2.0);

  Array<double> signal;
  signal.reserve(57);
  for (size_t i = 0; i < 57; ++i)
    signal.append(dist(rng));

  Array<double> numerator = {0.075, 0.15, 0.075};
  Array<double> denominator = {1.0, -0.9, 0.2};
  FFTD::BiquadSection section{0.075, 0.15, 0.075, 1.0, -0.9, 0.2};

  const auto expected = naive_iir_filtfilt(signal, numerator, denominator);
  const auto obtained = FFTD::filtfilt(signal, numerator, denominator);
  const auto wrapped = FFTD::filtfilt(signal, section);

  expect_real_array_near(obtained, expected, 1e-10);
  expect_real_array_near(wrapped, expected, 1e-10);

  std::vector<double> signal_vec(signal.begin(), signal.end());
  std::vector<double> numerator_vec(numerator.begin(), numerator.end());
  std::vector<double> denominator_vec(denominator.begin(), denominator.end());
  expect_real_array_near(FFTD::filtfilt(signal_vec, numerator_vec, denominator_vec),
                         expected, 1e-10);
}

TEST(FFTFiltFiltIIR, CascadeSectionsMatchReference)
{
  std::mt19937_64 rng(42424242);
  std::uniform_real_distribution<double> dist(-1.5, 1.5);

  Array<double> signal;
  signal.reserve(64);
  for (size_t i = 0; i < 64; ++i)
    signal.append(dist(rng));

  Array<FFTD::BiquadSection> sections = {
    FFTD::BiquadSection{0.075, 0.15, 0.075, 1.0, -0.9, 0.2},
    FFTD::BiquadSection{0.14, 0.28, 0.14, 1.0, -0.5, 0.06}
  };

  const auto expected = naive_sos_filtfilt(signal, sections);
  const auto obtained = FFTD::filtfilt(signal, sections);
  expect_real_array_near(obtained, expected, 1e-10);

  std::vector<FFTD::BiquadSection> sections_vec(sections.begin(), sections.end());
  expect_real_array_near(FFTD::filtfilt(signal, sections_vec), expected, 1e-10);
}

TEST(FFTIIRUtilities, LFilterStatefulMatchesOneShot)
{
  std::mt19937_64 rng(17171717);
  std::uniform_real_distribution<double> dist(-1.0, 1.0);

  Array<double> signal;
  signal.reserve(48);
  for (size_t i = 0; i < 48; ++i)
    signal.append(dist(rng));

  Array<double> numerator = {0.075, 0.15, 0.075};
  Array<double> denominator = {1.0, -0.9, 0.2};

  const auto expected = FFTD::lfilter(signal, numerator, denominator);

  FFTD::LFilter filter(numerator, denominator);
  EXPECT_EQ(filter.order(), 2u);

  Array<double> streamed;
  for (size_t offset = 0; offset < signal.size(); offset += 11)
    {
      const size_t length = std::min(size_t(11), signal.size() - offset);
      Array<double> chunk;
      chunk.reserve(length);
      for (size_t i = 0; i < length; ++i)
        chunk.append(signal[offset + i]);

      const auto partial = filter.filter(chunk);
      for (size_t i = 0; i < partial.size(); ++i)
        streamed.append(partial[i]);
    }

  expect_real_array_near(streamed, expected, 1e-12);

  filter.reset();
  expect_real_array_near(filter.filter(signal), expected, 1e-12);
}

TEST(FFTIIRUtilities, SOSFilterStatefulMatchesOneShot)
{
  std::mt19937_64 rng(31313131);
  std::uniform_real_distribution<double> dist(-1.0, 1.0);

  Array<double> signal;
  signal.reserve(52);
  for (size_t i = 0; i < 52; ++i)
    signal.append(dist(rng));

  Array<FFTD::BiquadSection> sections = {
    FFTD::BiquadSection{0.075, 0.15, 0.075, 1.0, -0.9, 0.2},
    FFTD::BiquadSection{0.14, 0.28, 0.14, 1.0, -0.5, 0.06}
  };

  const auto expected = FFTD::sosfilt(signal, sections);
  FFTD::SOSFilter filter(sections);
  EXPECT_EQ(filter.num_sections(), 2u);

  Array<double> streamed;
  for (size_t offset = 0; offset < signal.size(); offset += 9)
    {
      const size_t length = std::min(size_t(9), signal.size() - offset);
      Array<double> chunk;
      chunk.reserve(length);
      for (size_t i = 0; i < length; ++i)
        chunk.append(signal[offset + i]);

      const auto partial = filter.filter(chunk);
      for (size_t i = 0; i < partial.size(); ++i)
        streamed.append(partial[i]);
    }

  expect_real_array_near(streamed, expected, 1e-12);

  filter.reset();
  expect_real_array_near(filter.filter(signal), expected, 1e-12);
}

TEST(FFTIIRUtilities, BatchedLFilterAndBankMatchScalarPaths)
{
  ThreadPool pool(4);
  Array<Array<double>> signals = {
    Array<double>({1.0, -0.5, 0.25, 2.0, -1.0, 0.5, 1.5, -0.75, 0.25}),
    Array<double>({-1.0, 0.75, 0.5, -0.25, 1.25, -1.5, 0.0, 0.5}),
    Array<double>({0.25, -0.125, 0.75, 1.5, -0.5, 0.0, 0.5})
  };
  Array<double> numerator = {0.075, 0.15, 0.075};
  Array<double> denominator = {1.0, -0.9, 0.2};
  Array<size_t> chunk_pattern = {2, 3, 1, 4};

  Array<Array<double>> expected(signals.size(), Array<double>());
  for (size_t i = 0; i < signals.size(); ++i)
    expected(i) = FFTD::lfilter(signals[i], numerator, denominator);

  const auto batch = FFTD::batched_lfilter(signals, numerator, denominator);
  const auto parallel_batch =
    FFTD::pbatched_lfilter(pool, signals, numerator, denominator);
  for (size_t i = 0; i < signals.size(); ++i)
    {
      expect_real_array_near(batch[i], expected[i], 1e-12);
      expect_real_array_near(parallel_batch[i], expected[i], 1e-12);
    }

  FFTD::LFilterBank bank(signals.size(), numerator, denominator);
  FFTD::LFilterBank parallel_bank(signals.size(), numerator, denominator);
  Array<Array<double>> streamed(signals.size(), Array<double>());
  Array<Array<double>> parallel_streamed(signals.size(), Array<double>());
  Array<size_t> offsets = {0, 0, 0};

  for (size_t step = 0;; ++step)
    {
      bool any_pending = false;
      Array<Array<double>> block;
      for (size_t channel = 0; channel < signals.size(); ++channel)
        {
          const size_t remaining = signals[channel].size() - offsets[channel];
          const size_t length = std::min(chunk_pattern[step % chunk_pattern.size()],
                                         remaining);
          Array<double> chunk;
          chunk.reserve(length);
          for (size_t i = 0; i < length; ++i)
            chunk.append(signals[channel][offsets[channel] + i]);
          offsets(channel) += length;
          any_pending = any_pending or length != 0;
          block.append(chunk);
        }

      if (not any_pending)
        break;

      const auto emitted = bank.filter(block);
      const auto parallel_emitted = parallel_bank.pfilter(pool, block);
      for (size_t channel = 0; channel < signals.size(); ++channel)
        {
          append_real_samples(streamed(channel), emitted[channel]);
          append_real_samples(parallel_streamed(channel), parallel_emitted[channel]);
        }
    }

  for (size_t i = 0; i < signals.size(); ++i)
    {
      expect_real_array_near(streamed[i], expected[i], 1e-12);
      expect_real_array_near(parallel_streamed[i], expected[i], 1e-12);
    }

  bank.reset();
  const auto rerun = bank.filter(signals);
  for (size_t i = 0; i < signals.size(); ++i)
    expect_real_array_near(rerun[i], expected[i], 1e-12);
}

TEST(FFTIIRUtilities, BatchedSOSFilterAndBankMatchScalarPaths)
{
  ThreadPool pool(4);
  Array<Array<double>> signals = {
    Array<double>({1.0, -0.5, 0.25, 2.0, -1.0, 0.5, 1.5, -0.75, 0.25}),
    Array<double>({-1.0, 0.75, 0.5, -0.25, 1.25, -1.5, 0.0, 0.5}),
    Array<double>({0.25, -0.125, 0.75, 1.5, -0.5, 0.0, 0.5})
  };
  Array<FFTD::BiquadSection> sections = {
    FFTD::BiquadSection{0.075, 0.15, 0.075, 1.0, -0.9, 0.2},
    FFTD::BiquadSection{0.14, 0.28, 0.14, 1.0, -0.5, 0.06}
  };
  Array<size_t> chunk_pattern = {3, 1, 2, 4};

  Array<Array<double>> expected(signals.size(), Array<double>());
  for (size_t i = 0; i < signals.size(); ++i)
    expected(i) = FFTD::sosfilt(signals[i], sections);

  const auto batch = FFTD::batched_sosfilt(signals, sections);
  const auto parallel_batch = FFTD::pbatched_sosfilt(pool, signals, sections);
  for (size_t i = 0; i < signals.size(); ++i)
    {
      expect_real_array_near(batch[i], expected[i], 1e-12);
      expect_real_array_near(parallel_batch[i], expected[i], 1e-12);
    }

  FFTD::SOSFilterBank bank(signals.size(), sections);
  FFTD::SOSFilterBank parallel_bank(signals.size(), sections);
  Array<Array<double>> streamed(signals.size(), Array<double>());
  Array<Array<double>> parallel_streamed(signals.size(), Array<double>());
  Array<size_t> offsets = {0, 0, 0};

  for (size_t step = 0;; ++step)
    {
      bool any_pending = false;
      Array<Array<double>> block;
      for (size_t channel = 0; channel < signals.size(); ++channel)
        {
          const size_t remaining = signals[channel].size() - offsets[channel];
          const size_t length = std::min(chunk_pattern[step % chunk_pattern.size()],
                                         remaining);
          Array<double> chunk;
          chunk.reserve(length);
          for (size_t i = 0; i < length; ++i)
            chunk.append(signals[channel][offsets[channel] + i]);
          offsets(channel) += length;
          any_pending = any_pending or length != 0;
          block.append(chunk);
        }

      if (not any_pending)
        break;

      const auto emitted = bank.filter(block);
      const auto parallel_emitted = parallel_bank.pfilter(pool, block);
      for (size_t channel = 0; channel < signals.size(); ++channel)
        {
          append_real_samples(streamed(channel), emitted[channel]);
          append_real_samples(parallel_streamed(channel), parallel_emitted[channel]);
        }
    }

  for (size_t i = 0; i < signals.size(); ++i)
    {
      expect_real_array_near(streamed[i], expected[i], 1e-12);
      expect_real_array_near(parallel_streamed[i], expected[i], 1e-12);
    }

  bank.reset();
  const auto rerun = bank.filter(signals);
  for (size_t i = 0; i < signals.size(); ++i)
    expect_real_array_near(rerun[i], expected[i], 1e-12);
}

TEST(FFTIIRUtilities, FreqzMatchesAnalyticAndCascadeProduct)
{
  Array<double> fir = {0.5, 0.5};
  const auto fir_response = FFTD::freqz(fir, 5, false);
  ASSERT_EQ(fir_response.omega.size(), 5u);
  ASSERT_EQ(fir_response.response.size(), 5u);
  expect_complex_near(fir_response.response[0], Complex(1.0, 0.0), 1e-12);
  expect_complex_near(fir_response.response[4], Complex(0.0, 0.0), 1e-12);

  FFTD::BiquadSection s1{0.075, 0.15, 0.075, 1.0, -0.9, 0.2};
  FFTD::BiquadSection s2{0.14, 0.28, 0.14, 1.0, -0.5, 0.06};
  Array<FFTD::BiquadSection> sections = {s1, s2};

  const auto r1 = FFTD::freqz(s1, 9, false);
  const auto r2 = FFTD::freqz(s2, 9, false);
  const auto rs = FFTD::freqz(sections, 9, false);
  for (size_t i = 0; i < rs.response.size(); ++i)
    expect_complex_near(rs.response[i], r1.response[i] * r2.response[i], 1e-12);
}

TEST(FFTIIRUtilities, RootsStabilityAndDelayMetrics)
{
  Array<double> numerator = {0.5, 0.5};
  Array<double> denominator = {1.0, -0.9, 0.2};
  FFTD::IIRCoefficients coeffs{numerator, denominator};
  FFTD::BiquadSection section{0.5, 0.5, 0.0, 1.0, -0.9, 0.2};

  expect_complex_unordered_near(FFTD::zeros(numerator),
                                Array<Complex>({Complex(-1.0, 0.0)}),
                                1e-8);
  expect_complex_unordered_near(FFTD::poles(denominator),
                                Array<Complex>({Complex(0.5, 0.0),
                                                Complex(0.4, 0.0)}),
                                1e-8);
  EXPECT_TRUE(FFTD::zeros(Array<double>({0.0, 0.0, 1.0})).is_empty());

  EXPECT_TRUE(FFTD::is_stable(denominator));
  EXPECT_TRUE(FFTD::is_stable(coeffs));
  EXPECT_TRUE(FFTD::is_stable(section));
  EXPECT_FALSE(FFTD::is_stable(Array<double>({1.0, -1.1})));
  EXPECT_NO_THROW(FFTD::validate_stable(denominator));
  EXPECT_THROW(FFTD::validate_stable(Array<double>({1.0, -1.1})),
               std::domain_error);

  Array<double> delay_line = {0.0, 0.0, 1.0};
  const auto response = FFTD::freqz(delay_line, 65, false);
  const auto group = FFTD::group_delay(response);
  const auto phase = FFTD::phase_delay(response);
  const auto wrapped_group = FFTD::group_delay(delay_line, 65, false);
  const auto wrapped_phase = FFTD::phase_delay(delay_line, 65, false);

  expect_real_array_near(group, wrapped_group, 1e-9);
  expect_real_array_near(phase, wrapped_phase, 1e-9);

  for (size_t i = 1; i + 1 < group.size(); ++i)
    {
      EXPECT_NEAR(group[i], 2.0, 1e-6);
      EXPECT_NEAR(phase[i], 2.0, 1e-6);
    }
}

TEST(FFTIIRUtilities, PairingMarginsAndCancellationChecks)
{
  Array<double> numerator = {1.0, 0.59, -0.41};
  Array<double> denominator = {1.0, -0.9, 0.2};

  const auto pairs = FFTD::pair_poles_and_zeros(numerator, denominator);
  ASSERT_EQ(pairs.size(), 2u);
  EXPECT_TRUE(pairs[0].has_zero);
  EXPECT_TRUE(pairs[0].has_pole);
  EXPECT_NEAR(FFTD::minimum_pole_zero_distance(numerator, denominator), 0.01, 1e-8);
  EXPECT_NEAR(FFTD::stability_margin(denominator), 0.5, 1e-8);

  EXPECT_TRUE(FFTD::has_near_pole_zero_cancellation(numerator, denominator, 0.02));
  EXPECT_FALSE(FFTD::has_near_pole_zero_cancellation(numerator, denominator, 0.005));
  EXPECT_THROW(FFTD::validate_no_near_pole_zero_cancellation(numerator,
                                                             denominator,
                                                             0.02),
               std::domain_error);
  EXPECT_NO_THROW(FFTD::validate_no_near_pole_zero_cancellation(numerator,
                                                                denominator,
                                                                0.005));

  EXPECT_NO_THROW(FFTD::validate_stable(denominator, 0.4));
  EXPECT_THROW(FFTD::validate_stable(denominator, 0.6), std::domain_error);
}

TEST(FFTIIRUtilities, GainAndPhaseMarginsDetectCrossovers)
{
  FFTD::FrequencyResponse gain_crossover_response;
  gain_crossover_response.omega = {0.0, 1.0, 2.0};
  gain_crossover_response.response = {
    std::polar(2.0, 0.0),
    std::polar(1.0, -std::numbers::pi / 2.0),
    std::polar(0.5, -3.0 * std::numbers::pi / 4.0)
  };

  const auto phase_margin = FFTD::phase_margin(gain_crossover_response);
  EXPECT_TRUE(phase_margin.found);
  EXPECT_NEAR(phase_margin.crossover_omega, 1.0, 1e-12);
  EXPECT_NEAR(phase_margin.radians, std::numbers::pi / 2.0, 1e-12);
  EXPECT_NEAR(phase_margin.degrees, 90.0, 1e-10);

  FFTD::FrequencyResponse phase_crossover_response;
  phase_crossover_response.omega = {0.0, 1.0, 2.0};
  phase_crossover_response.response = {
    std::polar(2.0, -std::numbers::pi / 2.0),
    std::polar(0.5, -std::numbers::pi),
    std::polar(0.25, -3.0 * std::numbers::pi / 2.0)
  };

  const auto gain_margin = FFTD::gain_margin(phase_crossover_response);
  EXPECT_TRUE(gain_margin.found);
  EXPECT_NEAR(gain_margin.crossover_omega, 1.0, 1e-12);
  EXPECT_NEAR(gain_margin.ratio, 2.0, 1e-12);
  EXPECT_NEAR(gain_margin.decibels, 20.0 * std::log10(2.0), 1e-12);
}

TEST(FFTIIRUtilities, DefaultConstructedFiltersRemainUnconfigured)
{
  FFTD::LFilter iir;
  FFTD::SOSFilter sos;

  EXPECT_FALSE(iir.configured());
  EXPECT_FALSE(sos.configured());
  EXPECT_THROW(iir.reset(), std::runtime_error);
  EXPECT_THROW(iir.filter(Array<double>({1.0, 2.0})), std::runtime_error);
  EXPECT_THROW(sos.reset(), std::runtime_error);
  EXPECT_THROW(sos.filter(Array<double>({1.0, 2.0})), std::runtime_error);
}

TEST(FFTIIRDesign, BilinearAndPrototypeDesignsAreStable)
{
  const auto bilinear =
    FFTD::bilinear_transform(Array<double>({1.0}),
                             Array<double>({1.0, 1.0}),
                             8.0);
  ASSERT_EQ(bilinear.numerator.size(), 2u);
  ASSERT_EQ(bilinear.denominator.size(), 2u);
  EXPECT_NEAR(bilinear.numerator[0], 1.0 / 17.0, 1e-12);
  EXPECT_NEAR(bilinear.numerator[1], 1.0 / 17.0, 1e-12);
  EXPECT_NEAR(bilinear.denominator[0], 1.0, 1e-12);
  EXPECT_NEAR(bilinear.denominator[1], -15.0 / 17.0, 1e-12);

  const auto butter_lp = FFTD::butterworth_lowpass(4, 800.0, 8000.0);
  const auto butter_hp = FFTD::butterworth_highpass(4, 800.0, 8000.0);
  const auto cheby_lp = FFTD::chebyshev1_lowpass(4, 1.0, 800.0, 8000.0);
  const auto cheby_hp = FFTD::chebyshev1_highpass(5, 1.0, 800.0, 8000.0);

  EXPECT_TRUE(FFTD::is_stable(butter_lp));
  EXPECT_TRUE(FFTD::is_stable(butter_hp));
  EXPECT_TRUE(FFTD::is_stable(cheby_lp));
  EXPECT_TRUE(FFTD::is_stable(cheby_hp));

  const auto butter_lp_response = FFTD::freqz(butter_lp, 513, false);
  const auto butter_hp_response = FFTD::freqz(butter_hp, 513, false);
  const auto cheby_lp_response = FFTD::freqz(cheby_lp, 513, false);
  const auto cheby_hp_response = FFTD::freqz(cheby_hp, 513, false);

  EXPECT_NEAR(std::abs(butter_lp_response.response[0]), 1.0, 1e-9);
  EXPECT_LT(std::abs(butter_lp_response.response[512]), 1e-3);
  EXPECT_LT(std::abs(butter_hp_response.response[0]), 1e-6);
  EXPECT_NEAR(std::abs(butter_hp_response.response[512]), 1.0, 1e-6);

  EXPECT_NEAR(std::abs(cheby_lp_response.response[0]),
              std::pow(10.0, -1.0 / 20.0),
              5e-6);
  EXPECT_LT(std::abs(cheby_lp_response.response[512]), 1e-3);
  EXPECT_NEAR(std::abs(cheby_hp_response.response[512]), 1.0, 1e-4);
  EXPECT_LT(std::abs(cheby_hp_response.response[0]), 1e-6);

  const auto butter_phase_margin = FFTD::phase_margin(butter_lp, 2049, false);
  EXPECT_TRUE(butter_phase_margin.found);
  EXPECT_GT(butter_phase_margin.degrees, 0.0);

  EXPECT_THROW(FFTD::butterworth_lowpass(0, 800.0, 8000.0), std::invalid_argument);
  EXPECT_THROW(FFTD::butterworth_lowpass(4, 4000.0, 8000.0), std::invalid_argument);
  EXPECT_THROW(FFTD::chebyshev1_lowpass(4, 0.0, 800.0, 8000.0),
               std::invalid_argument);
}

TEST(FFTOverlapAdd, MatchesDirectConvolution)
{
  std::mt19937_64 rng(24681357);
  std::uniform_real_distribution<double> dist(-2.0, 2.0);

  Array<double> signal;
  signal.reserve(257);
  for (size_t i = 0; i < 257; ++i)
    signal.append(dist(rng));

  Array<double> kernel;
  kernel.reserve(37);
  for (size_t i = 0; i < 37; ++i)
    kernel.append(dist(rng));

  const auto expected = FFTD::multiply(signal, kernel);

  for (const size_t block_size : {size_t(0), size_t(8), size_t(31), size_t(64)})
    {
      const auto obtained = FFTD::overlap_add_convolution(signal, kernel, block_size);
      expect_real_array_near(obtained, expected, 1e-7);
    }
}

TEST(FFTOverlapAdd, ReusableConvolverAndParallelPath)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(97531864);
  std::uniform_real_distribution<double> dist(-3.0, 3.0);

  Array<double> signal;
  signal.reserve(513);
  for (size_t i = 0; i < 513; ++i)
    signal.append(dist(rng));

  Array<double> kernel;
  kernel.reserve(29);
  for (size_t i = 0; i < 29; ++i)
    kernel.append(dist(rng));

  FFTD::OverlapAdd convolver(kernel, 63);
  EXPECT_EQ(convolver.block_size(), 63u);
  EXPECT_GE(convolver.fft_size(), 63u + kernel.size() - 1);

  const auto sequential = convolver.convolve(signal);
  const auto parallel = convolver.pconvolve(pool, signal);
  const auto expected = FFTD::multiply(signal, kernel);

  expect_real_array_near(sequential, expected, 1e-7);
  expect_real_array_near(parallel, expected, 1e-7);
  expect_real_array_near(parallel, sequential, 1e-8);

  std::vector<double> signal_vec(signal.begin(), signal.end());
  std::vector<double> kernel_vec(kernel.begin(), kernel.end());
  const auto wrapper_parallel =
      FFTD::poverlap_add_convolution(pool, signal_vec, kernel_vec, 63);
  expect_real_array_near(wrapper_parallel, expected, 1e-7);
}

TEST(FFTOverlapAdd, EmptyInputsAndInvalidKernel)
{
  Array<double> signal = {1.0, 2.0, 3.0};
  Array<double> kernel = {0.25, 0.5, 0.25};
  Array<double> empty;

  EXPECT_TRUE(FFTD::overlap_add_convolution(empty, kernel).is_empty());
  EXPECT_TRUE(FFTD::overlap_add_convolution(signal, empty).is_empty());
  EXPECT_THROW(([] { FFTD::OverlapAdd invalid{Array<double>()}; }()),
               std::invalid_argument);
}

TEST(FFTOverlapAdd, IncrementalProcessBlockMatchesFullConvolution)
{
  std::mt19937_64 rng(12344321);
  std::uniform_real_distribution<double> dist(-2.0, 2.0);

  Array<double> signal;
  signal.reserve(150);
  for (size_t i = 0; i < 150; ++i)
    signal.append(dist(rng));

  Array<double> kernel;
  kernel.reserve(17);
  for (size_t i = 0; i < 17; ++i)
    kernel.append(dist(rng));

  FFTD::OverlapAdd convolver(kernel, 32);
  Array<double> streamed;

  const size_t chunks[] = {13, 7, 64, 5, 61};
  size_t offset = 0;
  for (const size_t chunk_size : chunks)
    {
      Array<double> chunk;
      chunk.reserve(chunk_size);
      for (size_t i = 0; i < chunk_size; ++i)
        chunk.append(signal[offset + i]);
      offset += chunk_size;

      const auto emitted = convolver.process_block(chunk);
      for (size_t i = 0; i < emitted.size(); ++i)
        streamed.append(emitted[i]);
    }

  const auto tail = convolver.flush();
  for (size_t i = 0; i < tail.size(); ++i)
    streamed.append(tail[i]);

  const auto expected = FFTD::multiply(signal, kernel);
  expect_real_array_near(streamed, expected, 1e-7);
}

TEST(FFTOverlapAdd, IncrementalParallelAndReset)
{
  ThreadPool pool(4);
  std::mt19937_64 rng(99884411);
  std::uniform_real_distribution<double> dist(-1.5, 1.5);

  Array<double> signal;
  signal.reserve(96);
  for (size_t i = 0; i < 96; ++i)
    signal.append(dist(rng));

  Array<double> kernel;
  kernel.reserve(11);
  for (size_t i = 0; i < 11; ++i)
    kernel.append(dist(rng));

  FFTD::OverlapAdd convolver(kernel, 24);
  Array<double> first_run;

  for (size_t offset = 0; offset < signal.size(); offset += 19)
    {
      const size_t length = std::min(size_t(19), signal.size() - offset);
      Array<double> chunk;
      chunk.reserve(length);
      for (size_t i = 0; i < length; ++i)
        chunk.append(signal[offset + i]);

      const auto emitted = convolver.pprocess_block(pool, chunk);
      for (size_t i = 0; i < emitted.size(); ++i)
        first_run.append(emitted[i]);
    }
  const auto first_tail = convolver.flush();
  for (size_t i = 0; i < first_tail.size(); ++i)
    first_run.append(first_tail[i]);

  const auto expected = FFTD::multiply(signal, kernel);
  expect_real_array_near(first_run, expected, 1e-7);

  convolver.reset();
  Array<double> second_run;
  const auto full_block = convolver.process_block(signal);
  for (size_t i = 0; i < full_block.size(); ++i)
    second_run.append(full_block[i]);
  const auto second_tail = convolver.flush();
  for (size_t i = 0; i < second_tail.size(); ++i)
    second_run.append(second_tail[i]);

  expect_real_array_near(second_run, expected, 1e-7);
  EXPECT_TRUE(convolver.flush().is_empty());
}
