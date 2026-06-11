/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file btw_sandpile.cc
 * @brief Quantitative Bak-Tang-Wiesenfeld avalanche-size reproduction.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include <reproductions/ca_reproduction_support.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Reproductions;

#ifndef ALEPH_REPRODUCTIONS_SOURCE_DIR
# define ALEPH_REPRODUCTIONS_SOURCE_DIR "reproductions"
#endif

int main()
{
  constexpr ca_size_t side = 64;
  constexpr std::size_t warmup_drops = 20000;
  constexpr std::size_t measured_drops = 100000;
  constexpr std::uint64_t seed = 0x4254571987ull;

  static_assert(not BTW_Sandpile::update_scheme::requires_double_buffer);

  BTW_Sandpile sandpile(side, seed);
  for (std::size_t i = 0; i < warmup_drops; ++i)
    (void) sandpile.drop_random();

  std::vector<std::size_t> sizes;
  std::vector<std::size_t> durations;
  sizes.reserve(measured_drops);
  durations.reserve(measured_drops);
  for (std::size_t i = 0; i < measured_drops; ++i)
    {
      const Avalanche avalanche = sandpile.drop_random();
      if (avalanche.size != 0)
        {
          sizes.push_back(avalanche.size);
          durations.push_back(avalanche.duration);
        }
    }

  const Linear_Fit fit = fit_log_log_histogram(sizes, 4, 2048, 18);
  const std::filesystem::path root = ALEPH_REPRODUCTIONS_SOURCE_DIR;
  write_histogram_csv(root / "results" / "btw_avalanche_sizes.csv", sizes);
  write_histogram_csv(root / "results" / "btw_avalanche_durations.csv", durations);

  std::ofstream summary(root / "results" / "btw_summary.csv");
  if (not summary)
    {
      std::cerr << "Cannot write BTW summary\n";
      return 1;
    }
  summary << "side,warmup_drops,measured_drops,nonzero_avalanches,slope,r_squared,fit_points\n"
          << side << ',' << warmup_drops << ',' << measured_drops << ','
          << sizes.size() << ',' << std::setprecision(12) << fit.slope << ','
          << fit.r_squared << ',' << fit.points << '\n';

  std::cout << "BTW sandpile: slope=" << std::fixed << std::setprecision(4)
            << fit.slope << " r2=" << fit.r_squared
            << " nonzero_avalanches=" << sizes.size() << '\n';
  if (fit.slope < -1.2 or fit.slope > -0.8)
    {
      std::cerr << "BTW avalanche-size slope outside [-1.2, -0.8]\n";
      return 2;
    }
  return 0;
}
