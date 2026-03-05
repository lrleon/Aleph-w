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

    constexpr size_t n = 8; // Input size must be a power of two for this FFT
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
    const auto spectrum = FFTD::transform(signal);

    print_real_sequence("Time-domain samples x[t]:", signal);

    cout << "Frequency bins X[k] and magnitudes |X[k]|:\n";
    for (size_t k = 0; k < spectrum.size(); ++k)
      {
        cout << "  k=" << k << "  X[k] = ";
        print_complex(spectrum[k]);
        cout << "    |X[k]| = " << fixed << setprecision(4)
             << std::abs(spectrum[k]) << "\n";
      }

    cout << "\nAnalysis:\n";
    cout << "- Bin k=1 shows energy from 1.5 * cos(theta).\n";
    cout << "- Bin k=3 shows energy from 0.75 * sin(3*theta).\n";
    cout << "- For real signals, the spectrum is Hermitian symmetric:\n";
    cout << "  X[k] is the conjugate of X[N-k]. This is why |X[1]| = |X[7]|.\n";
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

  cout << "All demonstrations completed successfully.\n";
  return 0;
}
