/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_reproduction_support.H
 * @brief Internal helpers shared by the cellular-automata reproductions.
 *
 * This header is intentionally local to `reproductions/`: it is scientific
 * tooling, not part of the installed Aleph API. The helpers keep the weekly
 * reproductions and their reduced smoke tests on the same code paths.
 */

#ifndef CA_REPRODUCTION_SUPPORT_H
#define CA_REPRODUCTION_SUPPORT_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
#include <map>
#include <ostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-png.H>
#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_continuous_rules.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_update_scheme.H>

namespace Aleph {
namespace CA {
namespace Reproductions {

/** @brief Least-squares result for a log-log histogram.
 *
 * `points` reports the number of populated logarithmic bins used by the fit.
 */
struct Linear_Fit
{
  double slope = 0.0;       ///< fitted exponent.
  double intercept = 0.0;   ///< fitted log-space intercept.
  double r_squared = 0.0;   ///< coefficient of determination.
  std::size_t points = 0;   ///< populated bins included in the fit.
};

/** @brief Fit a power-law exponent from logarithmically binned samples.
 *
 * Each populated bin contributes its geometric midpoint and probability
 * density (`count / width`) to an ordinary least-squares fit in log space.
 * The omitted global normalization does not affect the slope.
 *
 * @param samples positive integral event sizes.
 * @param min_value smallest sample included in the fit.
 * @param max_value largest sample included in the fit.
 * @param bins number of logarithmic bins.
 * @param sample_weight_power weight each sample `s` by `s^-sample_weight_power`.
 *        The default `0.0` fits directly observed event probabilities.
 * @return fitted slope, intercept, R-squared and populated-bin count.
 * @throws std::domain_error if the fit range is invalid or fewer than two
 *         logarithmic bins are requested.
 * @throws std::runtime_error if fewer than two populated bins remain.
 *
 * @par Complexity
 * O(samples.size() + bins).
 */
inline Linear_Fit fit_log_log_histogram(const std::vector<std::size_t> &samples,
                                        const std::size_t min_value,
                                        const std::size_t max_value,
                                        const std::size_t bins,
                                        const double sample_weight_power = 0.0)
{
  ah_domain_error_if(min_value == 0 or max_value <= min_value or bins < 2
                     or sample_weight_power < 0.0)
    << "fit_log_log_histogram: invalid range or bin count";

  const double lo = std::log(static_cast<double>(min_value));
  const double hi = std::log(static_cast<double>(max_value) + 1.0);
  const double width = (hi - lo) / static_cast<double>(bins);
  std::vector<double> counts(bins, 0.0);
  for (const std::size_t sample : samples)
    if (sample >= min_value and sample <= max_value)
      {
        const double offset = (std::log(static_cast<double>(sample)) - lo) / width;
        const std::size_t bin = (std::min) (bins - 1, static_cast<std::size_t>(offset));
        counts[bin] += std::pow(static_cast<double>(sample), -sample_weight_power);
      }

  std::vector<std::pair<double, double>> points;
  points.reserve(bins);
  for (std::size_t i = 0; i < bins; ++i)
    if (counts[i] != 0.0)
      {
        const double edge_lo = std::exp(lo + static_cast<double>(i) * width);
        const double edge_hi = std::exp(lo + static_cast<double>(i + 1) * width);
        const double x = std::sqrt(edge_lo * edge_hi);
        const double density = static_cast<double>(counts[i]) / (edge_hi - edge_lo);
        points.emplace_back(std::log(x), std::log(density));
      }

  ah_runtime_error_if(points.size() < 2)
    << "fit_log_log_histogram: fewer than two populated bins";

  double mean_x = 0.0;
  double mean_y = 0.0;
  for (const auto &[x, y] : points)
    {
      mean_x += x;
      mean_y += y;
    }
  mean_x /= static_cast<double>(points.size());
  mean_y /= static_cast<double>(points.size());

  double xx = 0.0;
  double xy = 0.0;
  double yy = 0.0;
  for (const auto &[x, y] : points)
    {
      const double dx = x - mean_x;
      const double dy = y - mean_y;
      xx += dx * dx;
      xy += dx * dy;
      yy += dy * dy;
    }
  ah_runtime_error_if(xx == 0.0) << "fit_log_log_histogram: degenerate x range";

  Linear_Fit fit;
  fit.slope = xy / xx;
  fit.intercept = mean_y - fit.slope * mean_x;
  fit.r_squared = yy == 0.0 ? 1.0 : (xy * xy) / (xx * yy);
  fit.points = points.size();
  return fit;
}

/** @brief Write an exact discrete histogram as CSV.
 *
 * @param path output path.
 * @param samples integral samples to count.
 * @throws std::runtime_error if the output cannot be written.
 *
 * @par Complexity
 * O(samples.size() log samples.size()).
 */
inline void write_histogram_csv(const std::filesystem::path &path,
                                const std::vector<std::size_t> &samples)
{
  if (const auto parent = path.parent_path(); not parent.empty())
    std::filesystem::create_directories(parent);
  std::map<std::size_t, std::size_t> histogram;
  for (const std::size_t sample : samples)
    ++histogram[sample];

  std::ofstream out(path);
  ah_runtime_error_if(not out) << "write_histogram_csv: cannot open '" << path.string() << "'";
  out << "size,count\n";
  for (const auto &[size, count] : histogram)
    out << size << ',' << count << '\n';
  ah_runtime_error_if(not out) << "write_histogram_csv: write failed for '" << path.string() << "'";
}

/** @brief Compute Moran's I for two occupied Schelling cell types.
 *
 * Empty cells are excluded. Occupied cells are encoded as `+1` and `-1`;
 * each occupied cell is connected to its four periodic Von-Neumann
 * neighbours when those neighbours are occupied.
 *
 * @tparam Lattice rank-2 lattice type.
 * @param frame toroidal Schelling frame.
 * @param empty_state value representing a vacancy.
 * @param type_a value representing the first population.
 * @param type_b value representing the second population.
 * @return Moran's I, or `0.0` when the statistic is undefined.
 *
 * @par Complexity
 * O(rows * columns).
 */
template <typename Lattice>
[[nodiscard]] double morans_i_binary(const Lattice &frame,
                                     const typename Lattice::state_type empty_state,
                                     const typename Lattice::state_type type_a,
                                     const typename Lattice::state_type type_b)
{
  static_assert(Lattice::rank == 2, "morans_i_binary requires a rank-2 lattice");
  using Coord = typename Lattice::coord_type;

  const ca_size_t rows = frame.size(0);
  const ca_size_t cols = frame.size(1);
  if (rows == 0 or cols == 0)
    return 0.0;
  std::size_t occupied = 0;
  double sum = 0.0;
  for (ca_size_t r = 0; r < rows; ++r)
    for (ca_size_t c = 0; c < cols; ++c)
      {
        const auto value = frame.at(Coord{static_cast<ca_index_t>(r),
                                          static_cast<ca_index_t>(c)});
        if (value == type_a)
          {
            ++occupied;
            sum += 1.0;
          }
        else if (value == type_b)
          {
            ++occupied;
            sum -= 1.0;
          }
        else if (value != empty_state)
          {
            return 0.0;
          }
      }
  if (occupied < 2)
    return 0.0;

  const double mean = sum / static_cast<double>(occupied);
  auto centered = [=](const auto value)
  {
    if (value == type_a)
      return 1.0 - mean;
    if (value == type_b)
      return -1.0 - mean;
    return 0.0;
  };

  double denominator = 0.0;
  double numerator = 0.0;
  std::size_t weight = 0;
  constexpr ca_index_t dr[] = {-1, 0, 1, 0};
  constexpr ca_index_t dc[] = {0, 1, 0, -1};
  for (ca_size_t r = 0; r < rows; ++r)
    for (ca_size_t c = 0; c < cols; ++c)
      {
        const Coord here{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)};
        const auto value = frame.at(here);
        if (value == empty_state)
          continue;
        const double x = centered(value);
        denominator += x * x;
        for (std::size_t k = 0; k < 4; ++k)
          {
            const Coord there{
              static_cast<ca_index_t>((r + rows + static_cast<ca_size_t>(dr[k] + 1) - 1) % rows),
              static_cast<ca_index_t>((c + cols + static_cast<ca_size_t>(dc[k] + 1) - 1) % cols)};
            const auto neighbour = frame.at(there);
            if (neighbour == empty_state)
              continue;
            numerator += x * centered(neighbour);
            ++weight;
          }
      }

  if (weight == 0 or denominator == 0.0)
    return 0.0;
  return (static_cast<double>(occupied) / static_cast<double>(weight))
         * (numerator / denominator);
}

/** @brief Avalanche measurements for one BTW grain drop. */
struct Avalanche
{
  std::size_t size = 0;      ///< number of topplings.
  std::size_t duration = 0;  ///< number of non-empty row-major sweeps.
};

/** @brief Deterministic open-boundary Bak-Tang-Wiesenfeld sandpile.
 *
 * The implementation performs in-place row-major toppling sweeps. It names
 * `Sequential_Update<RowMajor>` as its update scheme because direct BTW
 * toppling writes both the selected cell and its neighbours; a pull-only
 * local rule cannot preserve grains while expressing that operation.
 */
class BTW_Sandpile
{
public:
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  using update_scheme = Sequential_Update<RowMajor>;

  /** @brief Construct an empty square sandpile.
   * @param side grid side length.
   * @param seed seed used for uniformly distributed grain drops.
   * @throws std::domain_error if `side == 0`.
   */
  explicit BTW_Sandpile(const ca_size_t side, const std::uint64_t seed = 0)
    : grid_({side, side}, 0), rng_(seed)
  {
    ah_domain_error_if(side == 0) << "BTW_Sandpile: side must be positive";
  }

  /** @brief Drop one grain at a uniformly selected cell and stabilise.
   * @return avalanche size and duration.
   */
  [[nodiscard]] Avalanche drop_random()
  {
    std::uniform_int_distribution<ca_size_t> pick(0, grid_.size(0) - 1);
    return drop_at(pick(rng_), pick(rng_));
  }

  /** @brief Drop one grain at `(row, column)` and stabilise.
   * @param row zero-based row.
   * @param column zero-based column.
   * @return avalanche size and duration.
   * @throws std::domain_error if the coordinate is out of range.
   */
  [[nodiscard]] Avalanche drop_at(const ca_size_t row, const ca_size_t column)
  {
    ah_domain_error_if(row >= grid_.size(0) or column >= grid_.size(1))
      << "BTW_Sandpile::drop_at: coordinate out of range";
    const Coord_Vec<2> drop{static_cast<ca_index_t>(row),
                            static_cast<ca_index_t>(column)};
    grid_.set(drop, grid_.at(drop) + 1);

    Avalanche avalanche;
    while (true)
      {
        std::size_t sweep_topplings = 0;
        for (ca_size_t r = 0; r < grid_.size(0); ++r)
          for (ca_size_t c = 0; c < grid_.size(1); ++c)
            {
              const Coord_Vec<2> here{static_cast<ca_index_t>(r),
                                      static_cast<ca_index_t>(c)};
              if (grid_.at(here) < 4)
                continue;
              grid_.set(here, grid_.at(here) - 4);
              add_if_inside(static_cast<ca_index_t>(r) - 1, static_cast<ca_index_t>(c));
              add_if_inside(static_cast<ca_index_t>(r) + 1, static_cast<ca_index_t>(c));
              add_if_inside(static_cast<ca_index_t>(r), static_cast<ca_index_t>(c) - 1);
              add_if_inside(static_cast<ca_index_t>(r), static_cast<ca_index_t>(c) + 1);
              ++sweep_topplings;
            }
        if (sweep_topplings == 0)
          break;
        avalanche.size += sweep_topplings;
        ++avalanche.duration;
      }
    return avalanche;
  }

  /** @brief Return the current stable grid.
   * @return const lattice reference.
   */
  [[nodiscard]] const Grid &frame() const noexcept
  {
    return grid_;
  }

  /** @brief Test whether every cell is below the toppling threshold.
   * @return `true` when the frame is stable.
   */
  [[nodiscard]] bool stable() const
  {
    for (ca_size_t r = 0; r < grid_.size(0); ++r)
      for (ca_size_t c = 0; c < grid_.size(1); ++c)
        if (grid_.at({static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}) >= 4)
          return false;
    return true;
  }

private:
  Grid grid_;
  std::mt19937_64 rng_;

  /** @brief Add one grain when `(row, column)` lies inside the open grid.
   * @param row candidate row.
   * @param column candidate column.
   */
  void add_if_inside(const ca_index_t row, const ca_index_t column)
  {
    if (row < 0 or column < 0
        or row >= static_cast<ca_index_t>(grid_.size(0))
        or column >= static_cast<ca_index_t>(grid_.size(1)))
      return;
    const Coord_Vec<2> coord{row, column};
    grid_.set(coord, grid_.at(coord) + 1);
  }
};

/** @brief One named Gray-Scott parameter preset. */
struct Gray_Scott_Preset
{
  const char *name;  ///< filesystem-safe preset name.
  double feed;       ///< feed rate F.
  double kill;       ///< removal rate k.
};

/// Canonical visual presets used by the weekly Gray-Scott reproduction.
inline constexpr std::array<Gray_Scott_Preset, 3> gray_scott_presets{{
  {"spots", 0.0350, 0.0650},
  {"stripes", 0.0250, 0.0500},
  {"mitosis", 0.0367, 0.0649},
}};

using Gray_Scott_Cell = Reaction_Diffusion_Cell<double>;
using Gray_Scott_Lattice = Lattice<Dense_Cell_Storage<Gray_Scott_Cell, 2>, NeumannBoundary>;

/** @brief Build the deterministic finite-amplitude Gray-Scott perturbation.
 * @param side square lattice side length.
 * @param master_seed seed for cell-local perturbations.
 * @return initial Gray-Scott frame.
 * @throws std::domain_error if `side < 8`.
 */
inline Gray_Scott_Lattice make_gray_scott_seed(const ca_size_t side,
                                               const std::uint64_t master_seed)
{
  ah_domain_error_if(side < 8) << "make_gray_scott_seed: side must be at least 8";
  Gray_Scott_Lattice frame({side, side}, Gray_Scott_Cell{1.0, 0.0});
  const ca_size_t patch = (std::max) (ca_size_t{4}, side / 5);
  const ca_size_t begin = side / 2 - patch / 2;
  for (ca_size_t r = begin; r < begin + patch; ++r)
    for (ca_size_t c = begin; c < begin + patch; ++c)
      {
        const Coord_Vec<2> coord{static_cast<ca_index_t>(r),
                                 static_cast<ca_index_t>(c)};
        const std::uint64_t h = cell_seed<2>(master_seed, 0, coord);
        const double unit = static_cast<double>(h >> 11) * (1.0 / 9007199254740992.0);
        const double noise = 0.02 * (unit - 0.5);
        frame.set(coord, Gray_Scott_Cell{0.50 + noise, 0.25 - noise});
      }
  return frame;
}

/** @brief Run one Gray-Scott preset from the shared deterministic seed.
 * @param preset feed and kill rates.
 * @param side square lattice side length.
 * @param steps number of synchronous steps.
 * @param master_seed perturbation seed.
 * @return final Gray-Scott frame.
 */
inline Gray_Scott_Lattice run_gray_scott(const Gray_Scott_Preset &preset,
                                         const ca_size_t side,
                                         const std::size_t steps,
                                         const std::uint64_t master_seed)
{
  Gray_Scott_Rule<double> rule(preset.feed, preset.kill, 0.16, 0.08);
  Synchronous_Engine<Gray_Scott_Lattice, Gray_Scott_Rule<double>, Moore<2, 1>>
    engine(make_gray_scott_seed(side, master_seed), rule);
  engine.run(steps);
  return engine.frame();
}

/** @brief Convert one Gray-Scott state to an RGB colour.
 * @param cell concentration pair.
 * @return palette colour.
 */
inline RGB8 gray_scott_rgb(const Gray_Scott_Cell &cell)
{
  auto byte = [](const double value)
  {
    const double bounded = std::clamp(value, 0.0, 1.0);
    return static_cast<std::uint8_t>(255.0 * bounded + 0.5);
  };
  return RGB8{byte(2.2 * cell.v),
              byte(1.2 * (1.0 - cell.u)),
              byte(1.0 - 1.8 * cell.v)};
}

/** @brief Write a Gray-Scott frame as a native Aleph RGB PNG.
 * @param path output path.
 * @param frame frame to encode.
 * @throws std::runtime_error if the output cannot be written.
 */
inline void write_gray_scott_png(const std::filesystem::path &path,
                                 const Gray_Scott_Lattice &frame)
{
  if (const auto parent = path.parent_path(); not parent.empty())
    std::filesystem::create_directories(parent);
  std::ofstream out(path, std::ios::binary);
  ah_runtime_error_if(not out) << "write_gray_scott_png: cannot open '" << path.string() << "'";
  write_png(out, frame, gray_scott_rgb);
}

/** @brief Decoded subset of the dependency-free native PNG format. */
struct Native_Png
{
  std::uint32_t width = 0;        ///< pixel width.
  std::uint32_t height = 0;       ///< pixel height.
  std::vector<std::uint8_t> raw;  ///< rows with one filter byte plus RGB bytes.
};

/** @brief Decode a PNG emitted by `Aleph::CA::write_png`.
 *
 * Only the dependency-free writer's filter-0, stored-deflate output is
 * accepted. This keeps golden comparisons strict and avoids a PNG dependency.
 *
 * @param in binary PNG stream.
 * @return decoded dimensions and scanline bytes.
 * @throws std::runtime_error if the stream is malformed or unsupported.
 */
inline Native_Png decode_native_png(std::istream &in)
{
  std::vector<std::uint8_t> png((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
  auto be32 = [](const std::vector<std::uint8_t> &bytes, const std::size_t pos)
  {
    return (static_cast<std::uint32_t>(bytes[pos]) << 24)
           | (static_cast<std::uint32_t>(bytes[pos + 1]) << 16)
           | (static_cast<std::uint32_t>(bytes[pos + 2]) << 8)
           | static_cast<std::uint32_t>(bytes[pos + 3]);
  };

  static constexpr std::array<std::uint8_t, 8> signature{
    0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  ah_runtime_error_if(png.size() < signature.size()
                      or not std::equal(signature.begin(), signature.end(), png.begin()))
    << "decode_native_png: invalid PNG signature";

  Native_Png decoded;
  std::vector<std::uint8_t> idat;
  std::size_t pos = signature.size();
  while (pos + 12 <= png.size())
    {
      const std::uint32_t len = be32(png, pos);
      ah_runtime_error_if(pos + 12 + len > png.size())
        << "decode_native_png: truncated chunk";
      const std::string type(reinterpret_cast<const char *>(png.data() + pos + 4), 4);
      const auto payload = png.begin() + static_cast<std::ptrdiff_t>(pos + 8);
      if (type == "IHDR")
        {
          ah_runtime_error_if(len != 13) << "decode_native_png: malformed IHDR";
          decoded.width = be32(png, pos + 8);
          decoded.height = be32(png, pos + 12);
          ah_runtime_error_if(png[pos + 16] != 8 or png[pos + 17] != 2)
            << "decode_native_png: expected 8-bit RGB PNG";
        }
      else if (type == "IDAT")
        {
          idat.insert(idat.end(), payload, payload + len);
        }
      else if (type == "IEND")
        {
          break;
        }
      pos += 12 + len;
    }

  ah_runtime_error_if(idat.size() < 6) << "decode_native_png: IDAT too short";
  std::size_t z = 2;
  while (z + 5 <= idat.size() - 4)
    {
      const bool final = (idat[z] & 1u) != 0;
      ah_runtime_error_if((idat[z] & 0x06u) != 0u)
        << "decode_native_png: expected stored deflate blocks";
      ++z;
      const std::uint16_t len = static_cast<std::uint16_t>(idat[z]
                                                           | (idat[z + 1] << 8));
      const std::uint16_t nlen = static_cast<std::uint16_t>(idat[z + 2]
                                                            | (idat[z + 3] << 8));
      ah_runtime_error_if(static_cast<std::uint16_t>(~len) != nlen)
        << "decode_native_png: invalid stored block length";
      z += 4;
      ah_runtime_error_if(z + len > idat.size() - 4)
        << "decode_native_png: truncated stored block";
      decoded.raw.insert(decoded.raw.end(),
                         idat.begin() + static_cast<std::ptrdiff_t>(z),
                         idat.begin() + static_cast<std::ptrdiff_t>(z + len));
      z += len;
      if (final)
        break;
    }

  const std::size_t expected
    = static_cast<std::size_t>(decoded.height) * (1 + 3 * static_cast<std::size_t>(decoded.width));
  ah_runtime_error_if(decoded.width == 0 or decoded.height == 0 or decoded.raw.size() != expected)
    << "decode_native_png: unexpected scanline size";
  return decoded;
}

/** @brief Read and decode a native Aleph PNG.
 * @param path input path.
 * @return decoded PNG.
 * @throws std::runtime_error if the file cannot be read or decoded.
 */
inline Native_Png read_native_png(const std::filesystem::path &path)
{
  std::ifstream in(path, std::ios::binary);
  ah_runtime_error_if(not in) << "read_native_png: cannot open '" << path.string() << "'";
  return decode_native_png(in);
}

/** @brief Compute normalized mean absolute per-channel PNG difference.
 * @param lhs first decoded PNG.
 * @param rhs second decoded PNG.
 * @return difference in `[0, 1]`, where `0` is pixel-identical.
 * @throws std::domain_error if dimensions differ.
 */
inline double mean_channel_difference(const Native_Png &lhs,
                                      const Native_Png &rhs)
{
  ah_domain_error_if(lhs.width != rhs.width or lhs.height != rhs.height)
    << "mean_channel_difference: dimensions differ";
  const std::size_t stride = 1 + 3 * static_cast<std::size_t>(lhs.width);
  std::uint64_t difference = 0;
  for (std::size_t row = 0; row < lhs.height; ++row)
    {
      ah_runtime_error_if(lhs.raw[row * stride] != 0 or rhs.raw[row * stride] != 0)
        << "mean_channel_difference: expected filter type 0";
      for (std::size_t i = 1; i < stride; ++i)
        difference += static_cast<std::uint64_t>(
          std::abs(static_cast<int>(lhs.raw[row * stride + i])
                   - static_cast<int>(rhs.raw[row * stride + i])));
    }
  const double channels = static_cast<double>(lhs.width) * lhs.height * 3.0;
  return static_cast<double>(difference) / (255.0 * channels);
}

/// Canonical 36-cell Gosper glider gun.
inline constexpr std::array<std::array<std::int64_t, 2>, 36> gosper_gun_cells{{
  {{1, 5}}, {{1, 6}}, {{2, 5}}, {{2, 6}},
  {{11, 5}}, {{11, 6}}, {{11, 7}}, {{12, 4}}, {{12, 8}},
  {{13, 3}}, {{13, 9}}, {{14, 3}}, {{14, 9}},
  {{15, 6}}, {{16, 4}}, {{16, 8}},
  {{17, 5}}, {{17, 6}}, {{17, 7}}, {{18, 6}},
  {{21, 3}}, {{21, 4}}, {{21, 5}},
  {{22, 3}}, {{22, 4}}, {{22, 5}},
  {{23, 2}}, {{23, 6}},
  {{25, 1}}, {{25, 2}}, {{25, 6}}, {{25, 7}},
  {{35, 3}}, {{35, 4}}, {{36, 3}}, {{36, 4}},
}};

/** @brief Visit every live coordinate in the canonical Gosper gun.
 * @tparam F callable accepting `(x, y)`.
 * @param visitor coordinate callback.
 * @param offset_x horizontal offset.
 * @param offset_y vertical offset.
 */
template <typename F>
void for_each_gosper_gun_cell(F &&visitor,
                              const std::int64_t offset_x = 0,
                              const std::int64_t offset_y = 0)
{
  for (const auto &cell : gosper_gun_cells)
    visitor(offset_x + cell[0], offset_y + cell[1]);
}

}  // namespace Reproductions
}  // namespace CA
}  // namespace Aleph

#endif  // CA_REPRODUCTION_SUPPORT_H
