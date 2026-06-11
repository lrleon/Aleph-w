/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file drossel_schwabl.cc
 * @brief Quantitative Drossel-Schwabl forest-fire reproduction.
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <reproductions/ca_reproduction_support.H>
#include <tpl_ca_stochastic_rules.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Reproductions;

#ifndef ALEPH_REPRODUCTIONS_SOURCE_DIR
# define ALEPH_REPRODUCTIONS_SOURCE_DIR "reproductions"
#endif

namespace
{

using State = std::uint8_t;
using Neighbours = std::array<std::size_t, 8>;

constexpr State empty = static_cast<State>(Forest_Cell::EMPTY);
constexpr State tree = static_cast<State>(Forest_Cell::TREE);
constexpr State burning = static_cast<State>(Forest_Cell::BURNING);

/** @brief Compact deterministic kernel for the publication-scale run.
 *
 * `Forest_Fire_Rule` intentionally constructs a fresh standard RNG per cell
 * so generic engines remain bit-identical across schedules. The weekly
 * reproduction evaluates 3.6 billion cells on one thread, so it consumes the
 * same `(master, step, coord)` SplitMix stream directly. Fixed-size
 * `std::vector` buffers are deliberate here: this local tooling kernel needs
 * contiguous indexed storage and O(1) buffer swaps in its inner loop.
 */
class Fast_Forest_Fire
{
public:
  /** @brief Build an empty periodic forest.
   * @param side square grid side length.
   * @param p_growth per-step empty-cell growth probability.
   * @param p_lightning per-step spontaneous tree ignition probability.
   * @param master_seed deterministic SplitMix root seed.
   */
  Fast_Forest_Fire(const ca_size_t side,
                   const double p_growth,
                   const double p_lightning,
                   const std::uint64_t master_seed)
    : side_(side), p_growth_(p_growth), p_lightning_(p_lightning),
      master_seed_(master_seed), current_(side * side, empty),
      next_(side * side, empty), neighbours_(side * side),
      coord_hashes_(side * side)
  {
    for (ca_size_t row = 0; row < side_; ++row)
      for (ca_size_t column = 0; column < side_; ++column)
        {
          const std::size_t index = row * side_ + column;
          std::size_t k = 0;
          for (ca_index_t dr = -1; dr <= 1; ++dr)
            for (ca_index_t dc = -1; dc <= 1; ++dc)
              {
                if (dr == 0 and dc == 0)
                  continue;
                const ca_size_t r = static_cast<ca_size_t>(
                  (static_cast<ca_index_t>(row) + static_cast<ca_index_t>(side_) + dr)
                  % static_cast<ca_index_t>(side_));
                const ca_size_t c = static_cast<ca_size_t>(
                  (static_cast<ca_index_t>(column) + static_cast<ca_index_t>(side_) + dc)
                  % static_cast<ca_index_t>(side_));
                neighbours_[index][k++] = r * side_ + c;
              }
          coord_hashes_[index] = cell_key_from_coord<2>(
            {static_cast<ca_index_t>(row), static_cast<ca_index_t>(column)});
        }
  }

  /** @brief Advance one synchronous Drossel-Schwabl step. */
  void step()
  {
    const std::uint64_t step_hash
      = splitmix64(static_cast<std::uint64_t>(step_) + 0xa5a5a5a5a5a5a5a5ull);
    for (std::size_t index = 0; index < current_.size(); ++index)
      {
        if (current_[index] == burning)
          {
            next_[index] = empty;
            continue;
          }
        if (current_[index] == tree)
          {
            next_[index]
              = has_burning_neighbour(current_, neighbours_[index])
                or draw(index, step_hash) < p_lightning_ ? burning : tree;
            continue;
          }
        next_[index] = draw(index, step_hash) < p_growth_ ? tree : empty;
      }
    current_.swap(next_);
    ++step_;
  }

  /** @brief Return the active compact frame.
   * @return cell-state buffer.
   */
  [[nodiscard]] const std::vector<State> &frame() const noexcept
  {
    return current_;
  }

  /** @brief Return the precomputed Moore-neighbour table.
   * @return eight neighbours for every cell index.
   */
  [[nodiscard]] const std::vector<Neighbours> &neighbours() const noexcept
  {
    return neighbours_;
  }

private:
  ca_size_t side_;
  double p_growth_;
  double p_lightning_;
  std::uint64_t master_seed_;
  std::size_t step_ = 0;
  std::vector<State> current_;
  std::vector<State> next_;
  std::vector<Neighbours> neighbours_;
  std::vector<std::uint64_t> coord_hashes_;

  /** @brief Test whether one neighbourhood contains fire.
   * @param frame state buffer.
   * @param neighbours eight candidate indexes.
   * @return `true` when at least one neighbour is burning.
   */
  [[nodiscard]] static bool has_burning_neighbour(const std::vector<State> &frame,
                                                  const Neighbours &neighbours)
  {
    for (const std::size_t index : neighbours)
      if (frame[index] == burning)
        return true;
    return false;
  }

  /** @brief Produce a reproducible cell-local unit draw.
   * @param index cell index.
   * @param step_hash precomputed hash of the current step.
   * @return uniform value in `[0, 1)`.
   */
  [[nodiscard]] double draw(const std::size_t index,
                            const std::uint64_t step_hash) const noexcept
  {
    const std::uint64_t bits = splitmix64(master_seed_ ^ step_hash ^ coord_hashes_[index]);
    return static_cast<double>(bits >> 11) * (1.0 / 9007199254740992.0);
  }
};

/** @brief Test whether one neighbourhood contained fire in a prior frame.
 * @param frame prior state buffer.
 * @param neighbours eight candidate indexes.
 * @return `true` when at least one neighbour is burning.
 */
bool has_burning_neighbour(const std::vector<State> &frame,
                           const Neighbours &neighbours)
{
  for (const std::size_t index : neighbours)
    if (frame[index] == burning)
      return true;
  return false;
}

/** @brief Append tree-cluster sizes selected by spontaneous lightning.
 * @param before prior frame.
 * @param after current frame.
 * @param neighbours precomputed Moore-neighbour table.
 * @param[in,out] sizes sampled cluster sizes.
 */
void append_ignited_cluster_sizes(const std::vector<State> &before,
                                  const std::vector<State> &after,
                                  const std::vector<Neighbours> &neighbours,
                                  std::vector<std::size_t> &sizes)
{
  std::vector<unsigned char> seen(before.size(), 0);
  std::vector<std::size_t> queue;
  for (std::size_t origin = 0; origin < before.size(); ++origin)
    {
      if (seen[origin] != 0 or before[origin] != tree
          or after[origin] != burning
          or has_burning_neighbour(before, neighbours[origin]))
        continue;

      queue.clear();
      queue.push_back(origin);
      seen[origin] = 1;
      std::size_t count = 0;
      for (std::size_t head = 0; head < queue.size(); ++head)
        {
          const std::size_t index = queue[head];
          ++count;
          for (const std::size_t next : neighbours[index])
            if (seen[next] == 0 and before[next] == tree)
              {
                seen[next] = 1;
                queue.push_back(next);
              }
        }
      sizes.push_back(count);
    }
}

/** @brief Write observed and `1/s`-weighted cluster counts.
 * @param path output CSV path.
 * @param sizes ignition-selected cluster sizes.
 */
void write_cluster_histogram_csv(const std::filesystem::path &path,
                                 const std::vector<std::size_t> &sizes)
{
  if (const auto parent = path.parent_path(); not parent.empty())
    std::filesystem::create_directories(parent);
  std::map<std::size_t, std::size_t> histogram;
  for (const std::size_t size : sizes)
    ++histogram[size];
  std::ofstream out(path);
  if (not out)
    throw std::runtime_error("Cannot write Drossel-Schwabl histogram");
  out << "size,ignitions,cluster_number_weight\n";
  for (const auto &[size, ignitions] : histogram)
    out << size << ',' << ignitions << ','
        << static_cast<double>(ignitions) / static_cast<double>(size) << '\n';
}

}  // namespace

int main(int argc, char **argv)
{
  const bool quick = argc == 2 and std::string(argv[1]) == "--quick";
  if (argc > 1 and not quick)
    {
      std::cerr << "Usage: drossel_schwabl [--quick]\n";
      return 1;
    }
  const ca_size_t side = quick ? 64 : 256;
  const std::size_t warmup_steps = quick ? 500 : 5000;
  const std::size_t measured_steps = quick ? 2000 : 50000;
  constexpr double p_growth = 0.005;
  constexpr double p_lightning = 0.0001;
  constexpr std::uint64_t seed = 0xD20551992ull;

  Fast_Forest_Fire model(side, p_growth, p_lightning, seed);
  for (std::size_t step = 0; step < warmup_steps; ++step)
    model.step();

  std::vector<std::size_t> sizes;
  for (std::size_t step = 0; step < measured_steps; ++step)
    {
      const std::vector<State> before = model.frame();
      model.step();
      append_ignited_cluster_sizes(before, model.frame(), model.neighbours(), sizes);
      if (not quick and (step + 1) % 5000 == 0)
        std::cout << "Drossel-Schwabl progress: " << step + 1 << '/'
                  << measured_steps << " measured steps\n";
    }

  // Lightning samples a cluster with probability proportional to its size.
  // Weight by 1/s to recover the cluster-number distribution n(s). The
  // 4..1024 scaling window excludes the finite-grid cutoff above 1024.
  const Linear_Fit fit
    = fit_log_log_histogram(sizes, 4, 1024, quick ? 12 : 18, 1.0);
  const std::filesystem::path root = ALEPH_REPRODUCTIONS_SOURCE_DIR;
  write_cluster_histogram_csv(root / "results" / "drossel_schwabl_fire_sizes.csv", sizes);
  std::ofstream summary(root / "results" / "drossel_schwabl_summary.csv");
  if (not summary)
    {
      std::cerr << "Cannot write Drossel-Schwabl summary\n";
      return 1;
    }
  summary << "side,warmup_steps,measured_steps,p_growth,p_lightning,fires,slope,r_squared,fit_points\n"
          << side << ',' << warmup_steps << ',' << measured_steps << ','
          << p_growth << ',' << p_lightning << ',' << sizes.size() << ','
          << std::setprecision(12) << fit.slope << ',' << fit.r_squared << ','
          << fit.points << '\n';

  std::cout << "Drossel-Schwabl: slope=" << std::fixed << std::setprecision(4)
            << fit.slope << " r2=" << fit.r_squared << " fires=" << sizes.size() << '\n';
  if (quick)
    return 0;
  if (fit.slope < -2.2 or fit.slope > -1.8)
    {
      std::cerr << "Drossel-Schwabl fire-size slope outside [-2.2, -1.8]\n";
      return 2;
    }
  return 0;
}
