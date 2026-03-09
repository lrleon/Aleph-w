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
 * @file fft_benchmark.cc
 * @brief Baseline benchmark and validation utility for fft.H.
 */

# include <algorithm>
# include <chrono>
# include <cmath>
# include <complex>
# include <cstdlib>
# include <iomanip>
# include <iostream>
# include <limits>
# include <numbers>
# include <random>
# include <sstream>
# include <string>
# include <thread>

# include <fft.H>
# include <tpl_array.H>

using namespace Aleph;
using Clock = std::chrono::steady_clock;

namespace
{
  using FFTD = FFT<double>;
  using ComplexD = FFTD::Complex;

  volatile long double benchmark_sink = 0;

  struct Options
  {
    bool validate = false;
    bool json = false;
    size_t repetitions = 64;
    size_t warmup = 2;
    size_t samples = 7;
    size_t threads = 0;
    size_t batch_count = 16;
    unsigned seed = 20260306u;
    Array<size_t> sizes = {64, 128, 256, 300, 512, 1024, 1536, 2048};
  };

  struct TimingStats
  {
    double mean_us = 0.0;
    double median_us = 0.0;
    double min_us = 0.0;
    double max_us = 0.0;
    double stddev_us = 0.0;
  };

  struct BenchmarkRow
  {
    std::string name;
    size_t size = 0;
    size_t items_per_call = 1;
    size_t repetitions = 0;
    TimingStats sequential;
    TimingStats parallel;
  };

  struct ValidationMetrics
  {
    long double max_forward_error = 0;
    long double max_roundtrip_error = 0;
    long double max_real_roundtrip_error = 0;
    long double max_compact_roundtrip_error = 0;
  };

  template <typename Real>
  [[nodiscard]] constexpr ValidationMetrics
  validation_envelope() noexcept
  {
    if constexpr (std::is_same_v<Real, float>)
      return {1.0e-4L, 1.0e-4L, 1.0e-4L, 1.0e-4L};
    else if constexpr (std::is_same_v<Real, double>)
      return {1.0e-12L, 1.0e-12L, 1.0e-12L, 1.0e-12L};
    else
      return {1.0e-15L, 1.0e-15L, 1.0e-15L, 1.0e-15L};
  }

  [[nodiscard]] constexpr bool
  is_within_envelope(const ValidationMetrics & metrics,
                     const ValidationMetrics & envelope) noexcept
  {
    return metrics.max_forward_error <= envelope.max_forward_error
           and metrics.max_roundtrip_error <= envelope.max_roundtrip_error
           and metrics.max_real_roundtrip_error <= envelope.max_real_roundtrip_error
           and metrics.max_compact_roundtrip_error <= envelope.max_compact_roundtrip_error;
  }

  [[nodiscard]] Array<size_t>
  parse_sizes(const std::string & csv)
  {
    Array<size_t> sizes;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ','))
      {
        if (item.empty())
          continue;
        const unsigned long long parsed = std::stoull(item);
        sizes.append(static_cast<size_t>(parsed));
      }
    return sizes;
  }

  [[nodiscard]] Options
  parse_options(const int argc, char ** argv)
  {
    Options options;
    for (int i = 1; i < argc; ++i)
      {
        const std::string arg = argv[i];
        if (arg == "--validate")
          options.validate = true;
        else if (arg == "--json")
          options.json = true;
        else if (arg == "--repetitions" and i + 1 < argc)
          options.repetitions = static_cast<size_t>(std::stoull(argv[++i]));
        else if (arg == "--warmup" and i + 1 < argc)
          options.warmup = static_cast<size_t>(std::stoull(argv[++i]));
        else if (arg == "--samples" and i + 1 < argc)
          options.samples = static_cast<size_t>(std::stoull(argv[++i]));
        else if (arg == "--threads" and i + 1 < argc)
          options.threads = static_cast<size_t>(std::stoull(argv[++i]));
        else if (arg == "--batch-count" and i + 1 < argc)
          options.batch_count = static_cast<size_t>(std::stoull(argv[++i]));
        else if (arg == "--seed" and i + 1 < argc)
          options.seed = static_cast<unsigned>(std::stoul(argv[++i]));
        else if (arg == "--sizes" and i + 1 < argc)
          options.sizes = parse_sizes(argv[++i]);
        else if (arg == "--help")
          {
            std::cout
              << "Usage: fft_benchmark [--validate] [--json] [--repetitions N]"
              << " [--warmup N] [--samples N] [--threads N] [--batch-count N]"
              << " [--seed N] [--sizes a,b,c]\n";
            std::exit(0);
          }
        else
          {
            std::cerr << "Unknown option: " << arg << "\n";
            std::exit(1);
          }
      }

    if (options.sizes.is_empty())
      {
        std::cerr << "At least one benchmark size is required\n";
        std::exit(1);
      }

    for (size_t i = 0; i < options.sizes.size(); ++i)
      if (options.sizes[i] == 0)
        {
          std::cerr << "Benchmark sizes must be positive\n";
          std::exit(1);
        }

    if (options.repetitions == 0)
      {
        std::cerr << "Repetitions must be positive\n";
        std::exit(1);
      }

    if (options.samples == 0)
      {
        std::cerr << "Samples must be positive\n";
        std::exit(1);
      }

    if (options.batch_count == 0)
      {
        std::cerr << "Batch count must be positive\n";
        std::exit(1);
      }

    return options;
  }

  [[nodiscard]] size_t
  resolve_thread_count(const size_t requested)
  {
    if (requested != 0)
      return requested;

    const size_t detected = static_cast<size_t>(std::thread::hardware_concurrency());
    return std::max<size_t>(1, detected);
  }

  template <typename Real>
  [[nodiscard]] Array<std::complex<Real>>
  make_complex_signal(const size_t n, std::mt19937_64 & rng)
  {
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    Array<std::complex<Real>> signal;
    signal.reserve(n);
    for (size_t i = 0; i < n; ++i)
      signal.append(std::complex<Real>(static_cast<Real>(dist(rng)),
                                       static_cast<Real>(dist(rng))));
    return signal;
  }

  template <typename Real>
  [[nodiscard]] Array<Array<std::complex<Real>>>
  make_complex_batch(const size_t batch_count,
                     const size_t n,
                     std::mt19937_64 & rng)
  {
    Array<Array<std::complex<Real>>> batch;
    batch.reserve(batch_count);
    for (size_t item = 0; item < batch_count; ++item)
      batch.append(make_complex_signal<Real>(n, rng));
    return batch;
  }

  template <typename Real>
  [[nodiscard]] Array<Real>
  make_real_signal(const size_t n, std::mt19937_64 & rng)
  {
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    Array<Real> signal;
    signal.reserve(n);
    for (size_t i = 0; i < n; ++i)
      signal.append(static_cast<Real>(dist(rng)));
    return signal;
  }

  template <typename Real>
  [[nodiscard]] Array<Array<Real>>
  make_real_batch(const size_t batch_count,
                  const size_t n,
                  std::mt19937_64 & rng)
  {
    Array<Array<Real>> batch;
    batch.reserve(batch_count);
    for (size_t item = 0; item < batch_count; ++item)
      batch.append(make_real_signal<Real>(n, rng));
    return batch;
  }

  template <typename Real>
  [[nodiscard]] Array<std::complex<Real>>
  naive_dft(const Array<std::complex<Real>> & input, const bool invert)
  {
    using Complex = std::complex<Real>;

    Array<Complex> output;
    output.reserve(input.size());
    const Real sign = invert ? Real(1) : Real(-1);
    for (size_t k = 0; k < input.size(); ++k)
      {
        Complex sum(Real(0), Real(0));
        for (size_t t = 0; t < input.size(); ++t)
          {
            const Real angle = sign * Real(2) * std::numbers::pi_v<Real>
                               * static_cast<Real>(k)
                               * static_cast<Real>(t)
                               / static_cast<Real>(input.size());
            sum += input[t] * std::polar(Real(1), angle);
          }

        if (invert)
          sum /= static_cast<Real>(input.size());
        output.append(sum);
      }
    return output;
  }

  template <typename Real>
  [[nodiscard]] long double
  max_complex_error(const Array<std::complex<Real>> & lhs,
                    const Array<std::complex<Real>> & rhs)
  {
    long double error = 0;
    for (size_t i = 0; i < lhs.size(); ++i)
      error = std::max(error,
                       static_cast<long double>(std::abs(lhs[i] - rhs[i])));
    return error;
  }

  template <typename Real>
  [[nodiscard]] long double
  max_real_error(const Array<Real> & lhs, const Array<Real> & rhs)
  {
    long double error = 0;
    for (size_t i = 0; i < lhs.size(); ++i)
      error = std::max(error,
                       static_cast<long double>(std::abs(lhs[i] - rhs[i])));
    return error;
  }

  template <typename Real>
  [[nodiscard]] Array<std::complex<Real>>
  lift_real(const Array<Real> & input)
  {
    Array<std::complex<Real>> output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
      output.append(std::complex<Real>(input[i], Real(0)));
    return output;
  }

  template <typename Fn>
  [[nodiscard]] Array<double>
  collect_us_per_call_samples(const size_t warmup,
                              const size_t samples,
                              const size_t repetitions,
                              Fn && fn)
  {
    for (size_t sample = 0; sample < warmup; ++sample)
      for (size_t i = 0; i < repetitions; ++i)
        fn();

    Array<double> values;
    values.reserve(samples);
    for (size_t sample = 0; sample < samples; ++sample)
      {
        const auto t0 = Clock::now();
        for (size_t i = 0; i < repetitions; ++i)
          fn();
        const auto elapsed =
          std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
              Clock::now() - t0).count();
        values.append(elapsed / static_cast<double>(repetitions));
      }
    return values;
  }

  [[nodiscard]] TimingStats
  summarize_timings(const Array<double> & values)
  {
    ah_invalid_argument_if(values.is_empty())
      << "summarize_timings: at least one sample is required";

    TimingStats stats;
    Array<double> sorted = values;
    std::sort(&sorted[0], &sorted[0] + sorted.size());

    double sum = 0.0;
    for (size_t i = 0; i < values.size(); ++i)
      sum += values[i];

    stats.mean_us = sum / static_cast<double>(values.size());
    stats.min_us = sorted[0];
    stats.max_us = sorted[sorted.size() - 1];
    if (sorted.size() % 2 == 1)
      stats.median_us = sorted[sorted.size() / 2];
    else
      stats.median_us = 0.5 * (sorted[sorted.size() / 2 - 1]
                               + sorted[sorted.size() / 2]);

    double squared = 0.0;
    for (size_t i = 0; i < values.size(); ++i)
      {
        const double delta = values[i] - stats.mean_us;
        squared += delta * delta;
      }
    stats.stddev_us = std::sqrt(squared / static_cast<double>(values.size()));
    return stats;
  }

  template <typename Fn>
  [[nodiscard]] TimingStats
  measure_timing_stats(const size_t warmup,
                       const size_t samples,
                       const size_t repetitions,
                       Fn && fn)
  {
    return summarize_timings(collect_us_per_call_samples(warmup,
                                                         samples,
                                                         repetitions,
                                                         std::forward<Fn>(fn)));
  }

  [[nodiscard]] Array<double>
  make_fir_kernel(const size_t taps)
  {
    Array<double> kernel;
    kernel.reserve(taps);
    const double denom = static_cast<double>(std::max<size_t>(1, taps));
    for (size_t i = 0; i < taps; ++i)
      {
        const double phase = 2.0 * std::numbers::pi * static_cast<double>(i) / denom;
        kernel.append(0.42 - 0.5 * std::cos(phase) + 0.08 * std::cos(2.0 * phase));
      }
    return kernel;
  }

  [[nodiscard]] Array<BenchmarkRow>
  run_benchmarks(const Options & options)
  {
    std::mt19937_64 rng(options.seed);
    const size_t num_threads = resolve_thread_count(options.threads);
    ThreadPool pool(num_threads);
    Array<BenchmarkRow> rows;
    rows.reserve(options.sizes.size() * 10);

    for (size_t i = 0; i < options.sizes.size(); ++i)
      {
        const size_t n = options.sizes[i];
        const Array<ComplexD> complex_signal = make_complex_signal<double>(n, rng);
        const Array<Array<ComplexD>> complex_batch =
          make_complex_batch<double>(options.batch_count, n, rng);
        const Array<double> real_signal = make_real_signal<double>(n, rng);
        const Array<Array<double>> real_batch =
          make_real_batch<double>(options.batch_count, n, rng);
        const Array<Array<double>> long_real_batch =
          make_real_batch<double>(options.batch_count, n * 4, rng);
        const Array<double> long_signal = make_real_signal<double>(n * 4, rng);
        const Array<double> fir = make_fir_kernel(std::min<size_t>(129, std::max<size_t>(17, n / 8 + 1)));
        const FFTD::Plan batch_plan(n);
        const Array<double> lfilter_numerator = {0.067455, 0.134911, 0.067455};
        const Array<double> lfilter_denominator = {1.0, -1.14298, 0.412802};

        const size_t stft_fft_size = n;
        const size_t window_size = std::min<size_t>(std::max<size_t>(16, n / 2), n);
        const Array<double> window = FFTD::hann_window(window_size);
        FFTD::STFTOptions stft_options;
        stft_options.hop_size = std::max<size_t>(1, window_size / 4);
        stft_options.fft_size = stft_fft_size;
        stft_options.pad_end = true;
        stft_options.validate_nola = true;

        const size_t fir_block_size = std::max<size_t>(64, n / 2);

        BenchmarkRow complex_row;
        complex_row.name = "complex_fft";
        complex_row.size = n;
        complex_row.repetitions = options.repetitions;
        complex_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&complex_signal]()
                               {
                                 const auto output = FFTD::transformed(complex_signal, false);
                                 benchmark_sink += output[0].real();
                               });
        complex_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&complex_signal, &pool]()
                               {
                                 const auto output = FFTD::ptransformed(pool, complex_signal, false);
                                 benchmark_sink += output[0].real();
                               });
        rows.append(complex_row);

        BenchmarkRow batched_complex_row;
        batched_complex_row.name = "batched_complex_fft";
        batched_complex_row.size = n;
        batched_complex_row.items_per_call = options.batch_count;
        batched_complex_row.repetitions = std::max<size_t>(size_t(1),
                                                           options.repetitions / 2);
        batched_complex_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               batched_complex_row.repetitions,
                               [&complex_batch, &batch_plan]()
                               {
                                 const auto output = batch_plan.transformed_batch(complex_batch, false);
                                 benchmark_sink += output[0][0].real();
                               });
        batched_complex_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               batched_complex_row.repetitions,
                               [&complex_batch, &batch_plan, &pool]()
                               {
                                 const auto output = batch_plan.ptransformed_batch(pool,
                                                                                   complex_batch,
                                                                                   false);
                                 benchmark_sink += output[0][0].real();
                               });
        rows.append(batched_complex_row);

        BenchmarkRow batched_real_row;
        batched_real_row.name = "batched_real_rfft";
        batched_real_row.size = n;
        batched_real_row.items_per_call = options.batch_count;
        batched_real_row.repetitions = std::max<size_t>(size_t(1),
                                                        options.repetitions / 2);
        batched_real_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               batched_real_row.repetitions,
                               [&real_batch, &batch_plan]()
                               {
                                 const auto output = batch_plan.rfft_batch(real_batch);
                                 benchmark_sink += output[0][0].real();
                               });
        batched_real_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               batched_real_row.repetitions,
                               [&real_batch, &batch_plan, &pool]()
                               {
                                 const auto output = batch_plan.prfft_batch(pool,
                                                                            real_batch);
                                 benchmark_sink += output[0][0].real();
                               });
        rows.append(batched_real_row);

        BenchmarkRow real_row;
        real_row.name = "real_fft";
        real_row.size = n;
        real_row.repetitions = options.repetitions;
        real_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&real_signal]()
                               {
                                 const auto output = FFTD::transform(real_signal);
                                 benchmark_sink += output[0].real();
                               });
        real_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&real_signal, &pool]()
                               {
                                 const auto output = FFTD::ptransform(pool, real_signal);
                                 benchmark_sink += output[0].real();
                               });
        rows.append(real_row);

        BenchmarkRow compact_row;
        compact_row.name = "compact_rfft";
        compact_row.size = n;
        compact_row.repetitions = options.repetitions;
        compact_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&real_signal]()
                               {
                                 const auto output = FFTD::rfft(real_signal);
                                 benchmark_sink += output[0].real();
                               });
        compact_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&real_signal, &pool]()
                               {
                                 const auto output = FFTD::prfft(pool, real_signal);
                                 benchmark_sink += output[0].real();
                               });
        rows.append(compact_row);

        BenchmarkRow convolution_row;
        convolution_row.name = "convolution";
        convolution_row.size = n;
        convolution_row.repetitions = options.repetitions;
        convolution_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&long_signal, &fir]()
                               {
                                 const auto output = FFTD::multiply(long_signal, fir);
                                 benchmark_sink += output[0];
                               });
        convolution_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               options.repetitions,
                               [&long_signal, &fir, &pool]()
                               {
                                 const auto output = FFTD::pmultiply(pool, long_signal, fir);
                                 benchmark_sink += output[0];
                               });
        rows.append(convolution_row);

        BenchmarkRow overlap_add_bank_row;
        overlap_add_bank_row.name = "overlap_add_bank";
        overlap_add_bank_row.size = n;
        overlap_add_bank_row.items_per_call = options.batch_count;
        overlap_add_bank_row.repetitions =
          std::max<size_t>(size_t(1), options.repetitions / 4);
        FFTD::OverlapAddBank overlap_bank(options.batch_count, fir, fir_block_size);
        overlap_add_bank_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               overlap_add_bank_row.repetitions,
                               [&long_real_batch, &overlap_bank]()
                               {
                                 const auto output = overlap_bank.convolve(long_real_batch);
                                 benchmark_sink += output[0][0];
                               });
        overlap_add_bank_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               overlap_add_bank_row.repetitions,
                               [&long_real_batch, &overlap_bank, &pool]()
                               {
                                 const auto output = overlap_bank.pconvolve(pool,
                                                                            long_real_batch);
                                 benchmark_sink += output[0][0];
                               });
        rows.append(overlap_add_bank_row);

        BenchmarkRow stft_row;
        stft_row.name = "stft";
        stft_row.size = stft_fft_size;
        stft_row.repetitions = std::max<size_t>(size_t(1), options.repetitions / 4);
        stft_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               stft_row.repetitions,
                               [&long_signal, &window, &stft_options]()
                               {
                                 const auto output = FFTD::stft(long_signal, window, stft_options);
                                 benchmark_sink += static_cast<long double>(output.size());
                               });
        stft_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               stft_row.repetitions,
                               [&long_signal, &window, &stft_options, &pool]()
                               {
                                 const auto output = FFTD::pstft(pool, long_signal, window, stft_options);
                                 benchmark_sink += static_cast<long double>(output.size());
                               });
        rows.append(stft_row);

        BenchmarkRow filter_row;
        filter_row.name = "filtfilt";
        filter_row.size = n;
        filter_row.repetitions = std::max<size_t>(size_t(1), options.repetitions / 4);
        filter_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               filter_row.repetitions,
                               [&long_signal, &fir, fir_block_size]()
                               {
                                 const auto output = FFTD::filtfilt(long_signal, fir, fir_block_size);
                                 benchmark_sink += output[0];
                               });
        filter_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               filter_row.repetitions,
                               [&long_signal, &fir, fir_block_size, &pool]()
                               {
                                 const auto output = FFTD::pfiltfilt(pool,
                                                                     long_signal,
                                                                     fir,
                                                                     fir_block_size);
                                 benchmark_sink += output[0];
                               });
        rows.append(filter_row);

        BenchmarkRow lfilter_bank_row;
        lfilter_bank_row.name = "lfilter_bank";
        lfilter_bank_row.size = n;
        lfilter_bank_row.items_per_call = options.batch_count;
        lfilter_bank_row.repetitions =
          std::max<size_t>(size_t(1), options.repetitions / 4);
        FFTD::LFilterBank sequential_bank(options.batch_count,
                                          lfilter_numerator,
                                          lfilter_denominator);
        FFTD::LFilterBank parallel_bank(options.batch_count,
                                        lfilter_numerator,
                                        lfilter_denominator);
        lfilter_bank_row.sequential =
          measure_timing_stats(options.warmup,
                               options.samples,
                               lfilter_bank_row.repetitions,
                               [&real_batch, &sequential_bank]()
                               {
                                 sequential_bank.reset();
                                 const auto output = sequential_bank.filter(real_batch);
                                 benchmark_sink += output[0][0];
                               });
        lfilter_bank_row.parallel =
          measure_timing_stats(options.warmup,
                               options.samples,
                               lfilter_bank_row.repetitions,
                               [&real_batch, &parallel_bank, &pool]()
                               {
                                 parallel_bank.reset();
                                 const auto output = parallel_bank.pfilter(pool, real_batch);
                                 benchmark_sink += output[0][0];
                               });
        rows.append(lfilter_bank_row);
      }

    return rows;
  }

  template <typename Real>
  [[nodiscard]] ValidationMetrics
  compute_validation_metrics(const unsigned seed)
  {
    using FFTT = FFT<Real>;
    using Complex = typename FFTT::Complex;

    std::mt19937_64 rng(seed);
    ValidationMetrics metrics;

    for (const size_t n : {size_t(3), size_t(5), size_t(8), size_t(10),
                           size_t(17), size_t(32)})
      {
        const Array<Complex> complex_signal = make_complex_signal<Real>(n, rng);
        const Array<Real> real_signal = make_real_signal<Real>(n, rng);

        const auto expected = naive_dft(complex_signal, false);
        const auto obtained = FFTT::transformed(complex_signal, false);
        metrics.max_forward_error = std::max(metrics.max_forward_error,
                                             max_complex_error(obtained, expected));

        Array<Complex> roundtrip = complex_signal;
        FFTT::transform(roundtrip, false);
        FFTT::transform(roundtrip, true);
        metrics.max_roundtrip_error = std::max(metrics.max_roundtrip_error,
                                               max_complex_error(roundtrip,
                                                                 complex_signal));

        const auto full_spectrum = FFTT::transform(real_signal);
        const auto restored_real = FFTT::inverse_transform_real(full_spectrum);
        metrics.max_real_roundtrip_error = std::max(metrics.max_real_roundtrip_error,
                                                    max_real_error(restored_real,
                                                                   real_signal));

        const auto compact_spectrum = FFTT::rfft(real_signal);
        const auto restored_compact = FFTT::irfft(compact_spectrum, n);
        metrics.max_compact_roundtrip_error =
          std::max(metrics.max_compact_roundtrip_error,
                   max_real_error(restored_compact, real_signal));
      }

    return metrics;
  }

  void
  print_timing_stats_json(const TimingStats & stats)
  {
    std::cout << "{"
              << "\"mean_us\": " << std::fixed << std::setprecision(6) << stats.mean_us << ", "
              << "\"median_us\": " << stats.median_us << ", "
              << "\"min_us\": " << stats.min_us << ", "
              << "\"max_us\": " << stats.max_us << ", "
              << "\"stddev_us\": " << stats.stddev_us
              << "}";
  }

  void
  print_benchmarks(const Array<BenchmarkRow> & rows, const Options & options)
  {
    if (options.json)
      {
        std::cout << "{\n"
                  << "  \"schema_version\": 1,\n"
                  << "  \"mode\": \"benchmark\",\n"
                  << "  \"metadata\": {\n"
                  << "    \"seed\": " << options.seed << ",\n"
                  << "    \"threads\": " << resolve_thread_count(options.threads) << ",\n"
                  << "    \"simd_backend\": \"" << FFTD::simd_backend_name() << "\",\n"
                  << "    \"batched_plan_simd_backend\": \""
                  << FFTD::batched_plan_simd_backend_name() << "\",\n"
                  << "    \"detected_simd_backend\": \""
                  << FFTD::detected_simd_backend_name() << "\",\n"
                  << "    \"avx2_kernel_compiled\": "
                  << (FFTD::avx2_kernel_compiled() ? "true" : "false") << ",\n"
                  << "    \"neon_kernel_compiled\": "
                  << (FFTD::neon_kernel_compiled() ? "true" : "false") << ",\n"
                  << "    \"avx2_runtime_available\": "
                  << (FFTD::avx2_runtime_available() ? "true" : "false") << ",\n"
                  << "    \"neon_runtime_available\": "
                  << (FFTD::neon_runtime_available() ? "true" : "false") << ",\n"
                  << "    \"simd_preference\": \"" << FFTD::simd_preference_name() << "\",\n"
                  << "    \"batch_count\": " << options.batch_count << ",\n"
                  << "    \"warmup_samples\": " << options.warmup << ",\n"
                  << "    \"samples\": " << options.samples << "\n"
                  << "  },\n"
                  << "  \"rows\": [\n";
        for (size_t i = 0; i < rows.size(); ++i)
          {
            const auto & row = rows[i];
            const double speedup_mean = row.parallel.mean_us > 0.0
              ? row.sequential.mean_us / row.parallel.mean_us
              : 0.0;
            const double speedup_median = row.parallel.median_us > 0.0
              ? row.sequential.median_us / row.parallel.median_us
              : 0.0;

            std::cout << "    {\n"
                      << "      \"case\": \"" << row.name << "\",\n"
                      << "      \"size\": " << row.size << ",\n"
                      << "      \"items_per_call\": " << row.items_per_call << ",\n"
                      << "      \"repetitions_per_sample\": " << row.repetitions << ",\n"
                      << "      \"metric_unit\": \"microseconds_per_call\",\n"
                      << "      \"sequential\": ";
            print_timing_stats_json(row.sequential);
            std::cout << ",\n"
                      << "      \"parallel\": ";
            print_timing_stats_json(row.parallel);
            std::cout << ",\n"
                      << "      \"speedup\": {"
                      << "\"mean\": " << speedup_mean << ", "
                      << "\"median\": " << speedup_median
                      << "}\n"
                      << "    }";
            if (i + 1 != rows.size())
              std::cout << ",";
            std::cout << "\n";
          }
        std::cout << "  ]\n}" << std::endl;
        return;
      }

    std::cout << "FFT baseline benchmark (microseconds per call)\n";
    std::cout << "Configuration: seed=" << options.seed
              << ", threads=" << resolve_thread_count(options.threads)
              << ", simd=" << FFTD::simd_backend_name()
              << ", batch_simd=" << FFTD::batched_plan_simd_backend_name()
              << ", detected_simd=" << FFTD::detected_simd_backend_name()
              << ", simd_pref=" << FFTD::simd_preference_name()
              << ", batch_count=" << options.batch_count
              << ", warmup=" << options.warmup
              << ", samples=" << options.samples
              << "\n";
    std::cout << std::left << std::setw(14) << "Case"
              << std::right << std::setw(10) << "Size"
              << std::setw(10) << "Batch"
              << std::setw(12) << "Rep/S"
              << std::setw(16) << "SeqMean"
              << std::setw(16) << "ParMean"
              << std::setw(16) << "SeqMedian"
              << std::setw(16) << "ParMedian"
              << std::setw(12) << "Speedup"
              << "\n";
    std::cout << std::string(108, '-') << "\n";
    for (size_t i = 0; i < rows.size(); ++i)
      {
        const auto & row = rows[i];
        const double speedup = row.parallel.mean_us > 0.0
          ? row.sequential.mean_us / row.parallel.mean_us
          : 0.0;
        std::cout << std::left << std::setw(14) << row.name
                  << std::right << std::setw(10) << row.size
                  << std::setw(10) << row.items_per_call
                  << std::setw(12) << row.repetitions
                  << std::setw(16) << std::fixed << std::setprecision(3) << row.sequential.mean_us
                  << std::setw(16) << row.parallel.mean_us
                  << std::setw(16) << row.sequential.median_us
                  << std::setw(16) << row.parallel.median_us
                  << std::setw(12) << speedup
                  << "\n";
      }
    std::cout << "Benchmark sink: " << static_cast<long double>(benchmark_sink) << "\n";
  }

  [[nodiscard]] bool
  print_validation(const bool json, const unsigned seed)
  {
    const auto float_metrics = compute_validation_metrics<float>(seed);
    const auto double_metrics = compute_validation_metrics<double>(seed);
    const auto long_double_metrics = compute_validation_metrics<long double>(seed);
    const auto float_envelope = validation_envelope<float>();
    const auto double_envelope = validation_envelope<double>();
    const auto long_double_envelope = validation_envelope<long double>();
    const bool valid = is_within_envelope(float_metrics, float_envelope)
                       and is_within_envelope(double_metrics, double_envelope)
                       and is_within_envelope(long_double_metrics,
                                              long_double_envelope);

    auto print_metrics = [](const char * label, const ValidationMetrics & metrics)
    {
      std::cout << std::left << std::setw(12) << label
                << std::right << std::setw(18) << std::setprecision(6)
                << std::scientific << metrics.max_forward_error
                << std::setw(18) << metrics.max_roundtrip_error
                << std::setw(18) << metrics.max_real_roundtrip_error
                << std::setw(18) << metrics.max_compact_roundtrip_error
                << "\n";
    };

    if (json)
      {
        auto emit = [](const char * label, const ValidationMetrics & metrics, const bool trailing)
        {
          std::cout << "    \"" << label << "\": {"
                    << "\"max_forward_error\": " << std::scientific << metrics.max_forward_error << ", "
                    << "\"max_roundtrip_error\": " << metrics.max_roundtrip_error << ", "
                    << "\"max_real_roundtrip_error\": " << metrics.max_real_roundtrip_error << ", "
                    << "\"max_compact_roundtrip_error\": " << metrics.max_compact_roundtrip_error
                    << "}";
          if (trailing)
            std::cout << ",";
          std::cout << "\n";
        };

        std::cout << "{\n"
                  << "  \"schema_version\": 1,\n"
                  << "  \"mode\": \"validation\",\n"
                  << "  \"valid\": " << (valid ? "true" : "false") << ",\n"
                  << "  \"metadata\": {\n"
                  << "    \"seed\": " << seed << "\n"
                  << "  },\n"
                  << "  \"validation\": {\n";
        emit("float", float_metrics, true);
        emit("double", double_metrics, true);
        emit("long_double", long_double_metrics, false);
        std::cout << "  }\n}" << std::endl;
        return valid;
      }

    std::cout << "FFT validation envelope (max absolute error, seed=" << seed << ")\n";
    std::cout << std::left << std::setw(12) << "Precision"
              << std::right << std::setw(18) << "Forward"
              << std::setw(18) << "RoundTrip"
              << std::setw(18) << "RealIFFT"
              << std::setw(18) << "CompactIRFFT"
              << "\n";
    std::cout << std::string(84, '-') << "\n";
    print_metrics("float", float_metrics);
    print_metrics("double", double_metrics);
    print_metrics("long double", long_double_metrics);
    return valid;
  }
}

int
main(int argc, char ** argv)
{
  const Options options = parse_options(argc, argv);

  if (options.validate)
    {
      if (not print_validation(options.json, options.seed))
        return 1;
    }
  else
    print_benchmarks(run_benchmarks(options), options);

  return 0;
}
