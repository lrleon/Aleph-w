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

TEST(FFT, RealInverseRestoresSignal)
{
  Array<double> signal = {0.5, -1.25, 3.5, 2.0, -0.75, 4.25, 1.0, -2.0};

  const auto spectrum = FFTD::transform(signal);
  const auto restored = FFTD::inverse_transform_real(spectrum);

  expect_real_array_near(restored, signal, 1e-8);
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
