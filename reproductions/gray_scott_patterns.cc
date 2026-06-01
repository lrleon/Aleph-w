/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file gray_scott_patterns.cc
 * @brief Golden-image Gray-Scott pattern reproduction.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <reproductions/ca_reproduction_support.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Reproductions;

#ifndef ALEPH_REPRODUCTIONS_SOURCE_DIR
# define ALEPH_REPRODUCTIONS_SOURCE_DIR "reproductions"
#endif

int main(int argc, char **argv)
{
  constexpr ca_size_t side = 128;
  constexpr std::size_t steps = 4000;
  constexpr std::uint64_t seed = 0x5eed1234abcddcbaull;
  const bool update_golden = argc == 2 and std::string(argv[1]) == "--update-golden";
  if (argc > 1 and not update_golden)
    {
      std::cerr << "Usage: gray_scott_patterns [--update-golden]\n";
      return 1;
    }

  const std::filesystem::path root = ALEPH_REPRODUCTIONS_SOURCE_DIR;
  const std::filesystem::path generated_dir = root / "figures" / "generated";
  const std::filesystem::path reference_dir = root / "figures" / "reference";
  std::filesystem::create_directories(generated_dir);
  std::filesystem::create_directories(reference_dir);
  std::filesystem::create_directories(root / "results");
  std::ofstream summary(root / "results" / "gray_scott_summary.csv");
  if (not summary)
    {
      std::cerr << "Cannot write Gray-Scott summary\n";
      return 1;
    }
  summary << "preset,side,steps,mean_channel_difference\n";

  for (const Gray_Scott_Preset &preset : gray_scott_presets)
    {
      const Gray_Scott_Lattice frame = run_gray_scott(preset, side, steps, seed);
      const std::filesystem::path generated = generated_dir / (std::string(preset.name) + ".png");
      const std::filesystem::path reference = reference_dir / (std::string(preset.name) + ".png");
      write_gray_scott_png(generated, frame);
      if (update_golden)
        {
          std::filesystem::copy_file(generated, reference,
                                     std::filesystem::copy_options::overwrite_existing);
          std::cout << "Updated " << reference << '\n';
          continue;
        }

      const double difference
        = mean_channel_difference(read_native_png(reference), read_native_png(generated));
      summary << preset.name << ',' << side << ',' << steps << ','
              << std::setprecision(12) << difference << '\n';
      std::cout << "Gray-Scott " << preset.name << ": mean channel difference="
                << std::fixed << std::setprecision(6) << difference << '\n';
      if (difference >= 0.01)
        {
          std::cerr << "Gray-Scott " << preset.name << " differs from golden by at least 1%\n";
          return 2;
        }
    }
  return 0;
}
