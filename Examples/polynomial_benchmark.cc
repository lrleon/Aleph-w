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
 * @file polynomial_benchmark.cc
 * @brief Reproducible benchmark and validation utility for tpl_polynomial.H
 *        and tpl_multi_polynomial.H.
 */

# include <algorithm>
# include <chrono>
# include <cmath>
# include <cstdlib>
# include <iomanip>
# include <iostream>
# include <limits>
# include <numeric>
# include <sstream>
# include <stdexcept>
# include <string>

# include <tpl_polynomial.H>
# include <tpl_multi_polynomial.H>

using namespace Aleph;
using Clock = std::chrono::steady_clock;

namespace
{
  using RealPoly = Gen_Polynomial<double>;
  using IntPoly = Gen_Polynomial<long long>;
  using IntMultiPoly = Gen_MultiPolynomial<long long>;
  using LexMultiPoly = Gen_MultiPolynomial<double, Lex_Order>;

  volatile long double benchmark_sink = 0;

  struct Options
  {
    bool validate = false;
    bool json = false;
    size_t repetitions = 128;
    size_t warmup = 2;
    size_t samples = 7;
    unsigned seed = 20260317u;
    Array<size_t> sizes = {16, 64, 256, 1024};
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
    std::string case_name;
    size_t size = 0;
    size_t repetitions = 0;
    TimingStats stats;
  };

  [[nodiscard]] Array<size_t>
  parse_sizes(const std::string &csv)
  {
    Array<size_t> sizes;
    std::stringstream ss(csv);
    std::string token;

    while (std::getline(ss, token, ','))
      {
        if (token.empty())
          continue;
        sizes.append(static_cast<size_t>(std::stoull(token)));
      }

    return sizes;
  }

  [[nodiscard]] Options
  parse_options(const int argc, char **argv)
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
        else if (arg == "--seed" and i + 1 < argc)
          options.seed = static_cast<unsigned>(std::stoul(argv[++i]));
        else if (arg == "--sizes" and i + 1 < argc)
          options.sizes = parse_sizes(argv[++i]);
        else if (arg == "--help")
          {
            std::cout
              << "Usage: polynomial_benchmark [--validate] [--json]"
              << " [--repetitions N] [--warmup N] [--samples N]"
              << " [--seed N] [--sizes a,b,c]\n";
            std::exit(0);
          }
        else
          {
            std::cerr << "Unknown option: " << arg << "\n";
            std::exit(1);
          }
      }

    if (options.repetitions == 0 or options.warmup == 0 or options.samples == 0)
      {
        std::cerr << "Repetitions, warmup, and samples must be positive\n";
        std::exit(1);
      }

    if (options.sizes.is_empty())
      {
        std::cerr << "At least one benchmark size is required\n";
        std::exit(1);
      }

    return options;
  }

  [[nodiscard]] long long
  deterministic_coeff(const unsigned seed, const size_t slot)
  {
    uint64_t x = static_cast<uint64_t>(seed) * 0x9E3779B185EBCA87ULL;
    x += static_cast<uint64_t>(slot + 1) * 0xC2B2AE3D27D4EB4FULL;
    x ^= x >> 33;
    x *= 0xFF51AFD7ED558CCDULL;
    x ^= x >> 33;
    long long value = static_cast<long long>(x % 17ULL) - 8LL;
    if (value >= 0)
      ++value;
    return value;
  }

  template <typename Poly>
  [[nodiscard]] typename Poly::Coeff
  manual_eval_univariate(const Poly &p, const typename Poly::Coeff &x)
  {
    using Coeff = typename Poly::Coeff;
    Coeff result = Coeff{};
    p.for_each_term([&result, &x](size_t exp, const Coeff &coeff)
    {
      result += coeff * polynomial_detail::power(x, exp);
    });
    return result;
  }

  [[nodiscard]] RealPoly
  make_sparse_real_poly(const size_t degree, const unsigned seed, const size_t variant)
  {
    RealPoly p;
    const Array<size_t> exponents = {
      0,
      std::max<size_t>(1, degree / 7 + 1),
      std::max<size_t>(2, degree / 3 + 1),
      std::max<size_t>(3, (2 * degree) / 3 + 1),
      degree
    };

    for (size_t i = 0; i < exponents.size(); ++i)
      p.set_coeff(exponents(i),
                  static_cast<double>(deterministic_coeff(seed + static_cast<unsigned>(variant), i + variant * 11)));

    return p;
  }

  [[nodiscard]] IntPoly
  make_sparse_int_poly(const size_t degree, const unsigned seed, const size_t variant)
  {
    IntPoly p;
    const Array<size_t> exponents = {
      0,
      std::max<size_t>(1, degree / 5 + 1),
      std::max<size_t>(2, degree / 2 + 1),
      degree
    };

    for (size_t i = 0; i < exponents.size(); ++i)
      p.set_coeff(exponents(i), deterministic_coeff(seed + static_cast<unsigned>(variant), i + variant * 7));

    return p;
  }

  template <typename MultiPoly>
  [[nodiscard]] MultiPoly
  make_sparse_multi_poly(const size_t degree, const unsigned seed, const size_t variant)
  {
    MultiPoly p(2);

    const Array<Array<size_t>> exponents = {
      Array<size_t>{degree, 0},
      Array<size_t>{0, degree},
      Array<size_t>{std::max<size_t>(1, degree / 2), std::max<size_t>(1, degree / 3)},
      Array<size_t>{std::max<size_t>(1, degree / 3), std::max<size_t>(1, degree / 2)},
      Array<size_t>{1, 1},
      Array<size_t>{0, 0}
    };

    for (size_t i = 0; i < exponents.size(); ++i)
      p.add_to_coeff(exponents(i),
                     typename MultiPoly::Coeff(
                       deterministic_coeff(seed + static_cast<unsigned>(variant), i + variant * 13)));

    return p;
  }

  [[nodiscard]] Array<LexMultiPoly>
  make_groebner_system(const size_t size)
  {
    auto x = LexMultiPoly::variable(3, 0);
    auto y = LexMultiPoly::variable(3, 1);
    auto z = LexMultiPoly::variable(3, 2);

    Array<LexMultiPoly> gens;
    if (size <= 32)
      {
        gens.append(x * x - y);
        gens.append(x * y - LexMultiPoly(3, 1.0));
        return gens;
      }

    if (size <= 256)
      {
        gens.append(x * y - LexMultiPoly(3, 1.0));
        gens.append(y - z);
        gens.append(x - LexMultiPoly(3, 1.0));
        return gens;
      }

    gens.append(x * y - z);
    gens.append(y * z - LexMultiPoly(3, 1.0));
    gens.append(x - z);
    return gens;
  }

  template <typename Fn>
  [[nodiscard]] TimingStats
  measure(const Options &options, Fn &&fn)
  {
    Array<double> samples;
    samples.reserve(options.samples);

    for (size_t sample = 0; sample < options.samples; ++sample)
      {
        for (size_t warm = 0; warm < options.warmup; ++warm)
          for (size_t rep = 0; rep < options.repetitions; ++rep)
            fn();

        const auto start = Clock::now();
        for (size_t rep = 0; rep < options.repetitions; ++rep)
          fn();
        const auto end = Clock::now();

        const double elapsed_us =
          std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(end - start).count();
        samples.append(elapsed_us / static_cast<double>(options.repetitions));
      }

    std::sort(&samples[0], &samples[0] + samples.size());

    TimingStats stats;
    stats.min_us = samples(0);
    stats.max_us = samples(samples.size() - 1);
    stats.median_us = samples(samples.size() / 2);
    stats.mean_us =
      std::accumulate(&samples[0], &samples[0] + samples.size(), 0.0) / static_cast<double>(samples.size());

    double variance = 0.0;
    for (size_t i = 0; i < samples.size(); ++i)
      {
        const double diff = samples(i) - stats.mean_us;
        variance += diff * diff;
      }
    variance /= static_cast<double>(samples.size());
    stats.stddev_us = std::sqrt(variance);
    return stats;
  }

  [[nodiscard]] bool
  validate_suite(const Options &options)
  {
    bool ok = true;

    for (size_t i = 0; i < options.sizes.size(); ++i)
      {
        const size_t degree = options.sizes(i);

        RealPoly real_a = make_sparse_real_poly(degree, options.seed, 0);
        RealPoly real_b = make_sparse_real_poly(degree, options.seed, 1);
        const double x = 0.875;

        if (std::abs(real_a.eval(x) - manual_eval_univariate(real_a, x)) > 1.0e-10)
          {
            std::cerr << "[validate] univariate eval mismatch at size " << degree << "\n";
            ok = false;
          }

        RealPoly inner({0.0, 0.5});
        RealPoly composed = real_a.compose(inner);
        if (std::abs(composed.eval(0.5) - real_a.eval(inner.eval(0.5))) > 1.0e-10)
          {
            std::cerr << "[validate] univariate compose mismatch at size " << degree << "\n";
            ok = false;
          }

        IntPoly int_a = make_sparse_int_poly(degree, options.seed, 0);
        IntPoly int_b = make_sparse_int_poly(std::max<size_t>(3, degree / 4), options.seed, 1);
        IntPoly product = int_a * int_b;
        auto [q_uni, r_uni] = product.divmod(int_b);
        if ((q_uni * int_b + r_uni) != product)
          {
            std::cerr << "[validate] univariate divmod identity mismatch at size " << degree << "\n";
            ok = false;
          }

        IntMultiPoly multi_a = make_sparse_multi_poly<IntMultiPoly>(std::max<size_t>(2, degree / 8 + 2),
                                                                    options.seed, 0);
        IntMultiPoly multi_b = make_sparse_multi_poly<IntMultiPoly>(std::max<size_t>(2, degree / 10 + 2),
                                                                    options.seed, 1);
        IntMultiPoly multi_product = multi_a * multi_b;
        Array<IntMultiPoly> divisors(1, multi_b);
        auto [q_multi, r_multi] = multi_product.divmod(divisors);
        if (((q_multi(0) * multi_b) + r_multi) != multi_product)
          {
            std::cerr << "[validate] multivariate divmod identity mismatch at size " << degree << "\n";
            ok = false;
          }

        Array<LexMultiPoly> gb = LexMultiPoly::reduced_groebner_basis(make_groebner_system(degree));
        for (size_t g = 0; g < gb.size(); ++g)
          {
            Array<LexMultiPoly> others;
            for (size_t h = 0; h < gb.size(); ++h)
              if (g != h)
                others.append(gb(h));
            if (others.is_empty())
              continue;
            if (gb(g).divmod(others).second != gb(g))
              {
                std::cerr << "[validate] reduced Groebner basis not autoreduced at size "
                          << degree << "\n";
                ok = false;
                break;
              }
          }
      }

    return ok;
  }

  [[nodiscard]] Array<BenchmarkRow>
  run_benchmarks(const Options &options)
  {
    Array<BenchmarkRow> rows;

    for (size_t i = 0; i < options.sizes.size(); ++i)
      {
        const size_t size = options.sizes(i);

        {
          RealPoly p = make_sparse_real_poly(size, options.seed, 0);
          BenchmarkRow row;
          row.case_name = "uni_eval_sparse";
          row.size = size;
          row.repetitions = options.repetitions;
          row.stats = measure(options, [&p]()
          {
            benchmark_sink += p.eval(0.875);
          });
          rows.append(std::move(row));
        }

        {
          RealPoly a = make_sparse_real_poly(size, options.seed, 0);
          RealPoly b = make_sparse_real_poly(size, options.seed, 1);
          BenchmarkRow row;
          row.case_name = "uni_mul_sparse";
          row.size = size;
          row.repetitions = options.repetitions;
          row.stats = measure(options, [&a, &b]()
          {
            RealPoly c = a * b;
            benchmark_sink += static_cast<long double>(c.degree() + c.num_terms());
          });
          rows.append(std::move(row));
        }

        {
          RealPoly outer = make_sparse_real_poly(size, options.seed, 0);
          RealPoly inner({0.0, 0.5});
          BenchmarkRow row;
          row.case_name = "uni_compose_sparse";
          row.size = size;
          row.repetitions = options.repetitions;
          row.stats = measure(options, [&outer, &inner]()
          {
            RealPoly c = outer.compose(inner);
            benchmark_sink += static_cast<long double>(c.degree() + c.num_terms());
          });
          rows.append(std::move(row));
        }

        {
          IntPoly a = make_sparse_int_poly(size, options.seed, 0);
          IntPoly b = make_sparse_int_poly(std::max<size_t>(3, size / 4), options.seed, 1);
          IntPoly dividend = a * b;
          BenchmarkRow row;
          row.case_name = "uni_divmod_sparse_exact";
          row.size = size;
          row.repetitions = options.repetitions;
          row.stats = measure(options, [&dividend, &b]()
          {
            auto [q, r] = dividend.divmod(b);
            benchmark_sink += static_cast<long double>(q.degree() + r.degree());
          });
          rows.append(std::move(row));
        }

        {
          const size_t multi_size = std::max<size_t>(2, size / 8 + 2);
          IntMultiPoly a = make_sparse_multi_poly<IntMultiPoly>(multi_size, options.seed, 0);
          IntMultiPoly b = make_sparse_multi_poly<IntMultiPoly>(multi_size, options.seed, 1);
          BenchmarkRow row;
          row.case_name = "multi_mul_sparse";
          row.size = size;
          row.repetitions = options.repetitions;
          row.stats = measure(options, [&a, &b]()
          {
            IntMultiPoly c = a * b;
            benchmark_sink += static_cast<long double>(c.degree() + c.num_terms());
          });
          rows.append(std::move(row));
        }

        {
          const size_t multi_size = std::max<size_t>(2, size / 10 + 2);
          IntMultiPoly a = make_sparse_multi_poly<IntMultiPoly>(multi_size, options.seed, 0);
          IntMultiPoly b = make_sparse_multi_poly<IntMultiPoly>(multi_size, options.seed, 1);
          IntMultiPoly dividend = a * b;
          Array<IntMultiPoly> divisors(1, b);
          BenchmarkRow row;
          row.case_name = "multi_divmod_exact";
          row.size = size;
          row.repetitions = options.repetitions;
          row.stats = measure(options, [&dividend, &divisors]()
          {
            auto [q, r] = dividend.divmod(divisors);
            benchmark_sink += static_cast<long double>(q(0).degree() + r.degree());
          });
          rows.append(std::move(row));
        }

        {
          Array<LexMultiPoly> gens = make_groebner_system(size);
          BenchmarkRow row;
          row.case_name = "multi_groebner_reduced_lex";
          row.size = size;
          row.repetitions = std::max<size_t>(1, options.repetitions / 8);
          const Options lighter = {false, false, row.repetitions, options.warmup, options.samples, options.seed,
                                   Array<size_t>{size}};
          row.stats = measure(lighter, [&gens]()
          {
            Array<LexMultiPoly> gb = LexMultiPoly::reduced_groebner_basis(gens);
            benchmark_sink += static_cast<long double>(gb.size());
          });
          rows.append(std::move(row));
        }
      }

    return rows;
  }

  void
  print_rows_json(const Array<BenchmarkRow> &rows, const Options &options)
  {
    std::cout << "{\n"
              << "  \"mode\": \"benchmark\",\n"
              << "  \"metadata\": {\n"
              << "    \"seed\": " << options.seed << ",\n"
              << "    \"repetitions\": " << options.repetitions << ",\n"
              << "    \"warmup\": " << options.warmup << ",\n"
              << "    \"samples\": " << options.samples << "\n"
              << "  },\n"
              << "  \"rows\": [\n";

    for (size_t i = 0; i < rows.size(); ++i)
      {
        const BenchmarkRow &row = rows(i);
        std::cout << "    {\n"
                  << "      \"case\": \"" << row.case_name << "\",\n"
                  << "      \"size\": " << row.size << ",\n"
                  << "      \"repetitions\": " << row.repetitions << ",\n"
                  << "      \"mean_us\": " << std::setprecision(12) << row.stats.mean_us << ",\n"
                  << "      \"median_us\": " << row.stats.median_us << ",\n"
                  << "      \"min_us\": " << row.stats.min_us << ",\n"
                  << "      \"max_us\": " << row.stats.max_us << ",\n"
                  << "      \"stddev_us\": " << row.stats.stddev_us << "\n"
                  << "    }" << (i + 1 == rows.size() ? "\n" : ",\n");
      }

    std::cout << "  ]\n"
              << "}\n";
  }

  void
  print_rows_table(const Array<BenchmarkRow> &rows)
  {
    std::cout << "Polynomial benchmark (microseconds per call)\n"
              << std::left << std::setw(28) << "case"
              << std::setw(8) << "size"
              << std::setw(12) << "median"
              << std::setw(12) << "mean"
              << std::setw(12) << "min"
              << std::setw(12) << "max"
              << "stddev\n";

    for (size_t i = 0; i < rows.size(); ++i)
      {
        const BenchmarkRow &row = rows(i);
        std::cout << std::left << std::setw(28) << row.case_name
                  << std::setw(8) << row.size
                  << std::setw(12) << std::fixed << std::setprecision(3) << row.stats.median_us
                  << std::setw(12) << row.stats.mean_us
                  << std::setw(12) << row.stats.min_us
                  << std::setw(12) << row.stats.max_us
                  << row.stats.stddev_us << "\n";
      }

    std::cout << "Benchmark sink: " << static_cast<long double>(benchmark_sink) << "\n";
  }
}

int
main(int argc, char **argv)
{
  try
    {
      const Options options = parse_options(argc, argv);

      if (options.validate)
        {
          const bool ok = validate_suite(options);
          if (not ok)
            return 1;
          if (not options.json)
            std::cout << "polynomial_benchmark: validation passed\n";
        }

      const Array<BenchmarkRow> rows = run_benchmarks(options);
      if (options.json)
        print_rows_json(rows, options);
      else
        print_rows_table(rows);

      return 0;
    }
  catch (const std::exception &e)
    {
      std::cerr << "polynomial_benchmark: " << e.what() << "\n";
      return 1;
    }
}
