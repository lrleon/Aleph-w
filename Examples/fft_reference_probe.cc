/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library
*/

/**
 * @file fft_reference_probe.cc
 * @brief Emits deterministic FFT reference cases as JSON.
 */

# include <iomanip>
# include <iostream>

# include <fft.H>
# include <tpl_array.H>

using namespace Aleph;

namespace
{
  using FFTD = FFT<double>;
  using Complex = FFTD::Complex;

  void
  print_complex_array(const Array<Complex> & values)
  {
    std::cout << "[";
    for (size_t i = 0; i < values.size(); ++i)
      {
        std::cout << "[" << std::setprecision(17) << values[i].real()
                  << ", " << values[i].imag() << "]";
        if (i + 1 != values.size())
          std::cout << ", ";
      }
    std::cout << "]";
  }

  void
  print_real_array(const Array<double> & values)
  {
    std::cout << "[";
    for (size_t i = 0; i < values.size(); ++i)
      {
        std::cout << std::setprecision(17) << values[i];
        if (i + 1 != values.size())
          std::cout << ", ";
      }
    std::cout << "]";
  }
}

int
main()
{
  Array<Complex> complex_signal = {
    Complex(1.25, -0.5),
    Complex(-2.0, 1.75),
    Complex(0.5, 2.25),
    Complex(3.0, -1.25),
    Complex(-0.75, 0.125),
    Complex(1.5, -2.0),
    Complex(0.0, 0.75),
    Complex(-1.25, 0.5),
    Complex(2.0, -0.25),
    Complex(-0.5, 1.0)
  };

  Array<double> real_signal = {0.25, -1.5, 2.0, 0.0, -0.75, 1.25, 3.0};

  Array<Complex> prime_signal;
  prime_signal.reserve(17);
  for (size_t i = 0; i < 17; ++i)
    prime_signal.append(Complex(0.5 * static_cast<double>(i) - 2.0,
                                (static_cast<double>(i % 5) - 2.0) * 0.75));

  FFTD::Plan plan(17);

  const auto complex_spectrum = FFTD::transformed(complex_signal, false);
  const auto real_full_spectrum = FFTD::transform(real_signal);
  const auto real_compact_spectrum = FFTD::rfft(real_signal);
  const auto real_reconstructed = FFTD::irfft(real_compact_spectrum, real_signal.size());
  const auto prime_spectrum = plan.transformed(prime_signal, false);

  std::cout << "{\n";
  std::cout << "  \"complex_signal\": ";
  print_complex_array(complex_signal);
  std::cout << ",\n  \"complex_spectrum\": ";
  print_complex_array(complex_spectrum);
  std::cout << ",\n  \"real_signal\": ";
  print_real_array(real_signal);
  std::cout << ",\n  \"real_full_spectrum\": ";
  print_complex_array(real_full_spectrum);
  std::cout << ",\n  \"real_compact_spectrum\": ";
  print_complex_array(real_compact_spectrum);
  std::cout << ",\n  \"real_reconstructed\": ";
  print_real_array(real_reconstructed);
  std::cout << ",\n  \"prime_signal\": ";
  print_complex_array(prime_signal);
  std::cout << ",\n  \"prime_spectrum\": ";
  print_complex_array(prime_spectrum);
  std::cout << "\n}\n";

  return 0;
}
