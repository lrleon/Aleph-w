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

#include <gtest/gtest.h>
#include <hash-fct.H>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace Aleph;

namespace
{

enum class HashProfile
{
  Weak,
  General,
  Strong
};

struct Thresholds
{
  double min_uniqueness = 0.0;
  double max_random_chi = std::numeric_limits<double>::infinity();
  double max_prefix_chi = std::numeric_limits<double>::infinity();
  double max_sequential_chi = std::numeric_limits<double>::infinity();
  double min_avalanche = 0.0;
  double max_bit_bias = std::numeric_limits<double>::infinity();
};

struct Candidate
{
  const char * name = "";
  HashProfile profile = HashProfile::Weak;
  std::function<size_t(const std::string &)> hash;
};

struct Metrics
{
  double uniqueness = 0.0;
  double random_chi = 0.0;
  double prefix_chi = 0.0;
  double sequential_chi = 0.0;
  double avalanche = 0.0;
  double max_bit_bias = 0.0;
  double throughput_mib_s = 0.0;
  size_t full_collisions = 0;
  size_t max_random_bucket = 0;
  uint64_t total_calls = 0;
  double latency_ns = 0.0;
};

Thresholds thresholds_for(HashProfile profile)
{
  switch (profile)
    {
    case HashProfile::Weak:
      return {};
    case HashProfile::General:
      return {.min_uniqueness = 0.970,
              .max_random_chi = 4.25,
              .max_prefix_chi = 6.50,
              .max_sequential_chi = 5.50,
              .min_avalanche = 0.34,
              .max_bit_bias = 0.15};
    case HashProfile::Strong:
      return {.min_uniqueness = 0.995,
              .max_random_chi = 2.60,
              .max_prefix_chi = 3.40,
              .max_sequential_chi = 3.20,
              .min_avalanche = 0.42,
              .max_bit_bias = 0.09};
    }

  return {};
}

std::vector<std::string> make_random_binary_samples(size_t count,
                                                    size_t min_len,
                                                    size_t max_len,
                                                    std::uint64_t seed)
{
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
  std::uniform_int_distribution<int> byte_dist(0, 255);
  std::vector<std::string> out;
  out.reserve(count);
  std::unordered_set<std::string> seen;
  seen.reserve(count * 2);

  while (out.size() < count)
    {
      const size_t len = len_dist(rng);
      std::string sample(len, '\0');
      for (size_t j = 0; j < len; ++j)
        sample[j] = static_cast<char>(byte_dist(rng));
      if (seen.insert(sample).second)
        out.push_back(std::move(sample));
    }

  return out;
}

std::vector<std::string> make_prefix_samples(size_t count,
                                             size_t prefix_len,
                                             size_t suffix_len)
{
  std::vector<std::string> out;
  out.reserve(count);
  const std::string prefix(prefix_len, 'A');

  for (size_t i = 0; i < count; ++i)
    {
      std::string sample = prefix;
      for (size_t j = 0; j < suffix_len; ++j)
        sample.push_back(static_cast<char>((i >> (j * 8)) & 0xff));
      out.push_back(std::move(sample));
    }

  return out;
}

std::vector<std::string> make_sequential_samples(size_t count)
{
  std::vector<std::string> out;
  out.reserve(count);

  for (size_t i = 0; i < count; ++i)
    {
      std::string sample(sizeof(std::uint64_t), '\0');
      std::uint64_t value = static_cast<std::uint64_t>(i);
      for (size_t j = 0; j < sizeof(value); ++j)
        sample[j] = static_cast<char>((value >> (j * 8)) & 0xff);
      out.push_back(std::move(sample));
    }

  return out;
}

std::vector<std::string> make_avalanche_samples(size_t count, size_t len,
                                                std::uint64_t seed)
{
  return make_random_binary_samples(count, len, len, seed);
}

std::pair<double, size_t> reduced_chi_square(const Candidate & candidate,
                                             const std::vector<std::string> & samples,
                                             size_t buckets)
{
  std::vector<size_t> counts(buckets, 0);
  for (const auto & sample : samples)
    ++counts[candidate.hash(sample) % buckets];

  const double expected = static_cast<double>(samples.size()) / buckets;
  double chi = 0.0;
  size_t max_bucket = 0;
  for (const size_t value : counts)
    {
      const double diff = static_cast<double>(value) - expected;
      chi += (diff * diff) / expected;
      max_bucket = std::max(max_bucket, value);
    }

  return {chi / static_cast<double>(buckets - 1), max_bucket};
}

double uniqueness_ratio(const Candidate & candidate,
                        const std::vector<std::string> & samples,
                        size_t & collisions)
{
  std::unordered_set<size_t> unique;
  unique.reserve(samples.size() * 2);

  for (const auto & sample : samples)
    unique.insert(candidate.hash(sample));

  collisions = samples.size() - unique.size();
  return static_cast<double>(unique.size()) / samples.size();
}

size_t effective_output_bits(const Candidate & candidate,
                             const std::vector<std::string> & samples)
{
  size_t observed_mask = 0;

  for (const auto & sample : samples)
    observed_mask |= candidate.hash(sample);

  const size_t bits = std::bit_width(observed_mask);
  return std::max<size_t>(bits, 1);
}

double max_output_bit_bias(const Candidate & candidate,
                           const std::vector<std::string> & samples)
{
  const size_t bits = effective_output_bits(candidate, samples);
  std::array<size_t, sizeof(size_t) * 8> ones = {};

  for (const auto & sample : samples)
    {
      const size_t hash = candidate.hash(sample);
      for (size_t bit = 0; bit < bits; ++bit)
        ones[bit] += (hash >> bit) & size_t{1};
    }

  double max_bias = 0.0;
  for (const size_t count : ones)
    {
      const double ratio = static_cast<double>(count) / samples.size();
      max_bias = std::max(max_bias, std::abs(ratio - 0.5));
    }

  return max_bias;
}

double avalanche_ratio(const Candidate & candidate,
                       const std::vector<std::string> & samples)
{
  const size_t output_bits = effective_output_bits(candidate, samples);
  std::uint64_t changed_bits = 0;
  std::uint64_t comparisons = 0;

  for (const auto & base : samples)
    {
      const size_t original = candidate.hash(base);
      const size_t flip_bits = std::min<size_t>(base.size() * 8, 64);

      for (size_t bit = 0; bit < flip_bits; ++bit)
        {
          std::string mutated = base;
          mutated[bit / 8] ^= static_cast<char>(1u << (bit % 8));
          const size_t hashed = candidate.hash(mutated);
          changed_bits += std::popcount(original ^ hashed);
          comparisons += output_bits;
        }
    }

  return comparisons == 0 ? 0.0
    : static_cast<double>(changed_bits) / comparisons;
}

struct ThroughputResult
{
  double mib_s = 0.0;
  uint64_t calls = 0;
  double latency_ns = 0.0;
};

ThroughputResult throughput_metrics(const Candidate & candidate,
                                    const std::vector<std::string> & samples)
{
  constexpr std::uint64_t target_bytes = 32ULL * 1024 * 1024;
  std::uint64_t processed = 0;
  uint64_t calls = 0;
  volatile size_t sink = 0;

  const auto start = std::chrono::steady_clock::now();
  while (processed < target_bytes)
    {
      for (const auto & sample : samples)
        {
          sink ^= candidate.hash(sample);
          processed += sample.size();
          calls++;
          if (processed >= target_bytes)
            break;
        }
    }
  const auto stop = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed = stop - start;
  EXPECT_NE(sink, 0u);

  ThroughputResult res;
  res.mib_s = static_cast<double>(processed) / (1024.0 * 1024.0) / elapsed.count();
  res.calls = calls;
  res.latency_ns = (std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()) / static_cast<double>(calls);
  return res;
}

Metrics measure_metrics(const Candidate & candidate,
                        const std::vector<std::string> & random_samples,
                        const std::vector<std::string> & prefix_samples,
                        const std::vector<std::string> & sequential_samples,
                        const std::vector<std::string> & avalanche_samples)
{
  Metrics metrics;
  metrics.uniqueness = uniqueness_ratio(candidate, random_samples,
                                        metrics.full_collisions);
  metrics.total_calls += random_samples.size();

  const auto [random_chi, max_random_bucket]
    = reduced_chi_square(candidate, random_samples, 1024);
  metrics.random_chi = random_chi;
  metrics.max_random_bucket = max_random_bucket;
  metrics.total_calls += random_samples.size();

  metrics.prefix_chi = reduced_chi_square(candidate, prefix_samples, 1024).first;
  metrics.total_calls += prefix_samples.size();

  metrics.sequential_chi = reduced_chi_square(candidate, sequential_samples, 1024).first;
  metrics.total_calls += sequential_samples.size();

  metrics.avalanche = avalanche_ratio(candidate, avalanche_samples);
  const size_t flip_bits = std::min<size_t>(avalanche_samples[0].size() * 8, 64);
  metrics.total_calls += avalanche_samples.size() * (1 + flip_bits);

  metrics.max_bit_bias = max_output_bit_bias(candidate, random_samples);
  metrics.total_calls += random_samples.size();

  const auto tp = throughput_metrics(candidate, random_samples);
  metrics.throughput_mib_s = tp.mib_s;
  metrics.latency_ns = tp.latency_ns;
  metrics.total_calls += tp.calls;

  return metrics;
}

std::vector<Candidate> all_candidates()
{
  return {
    {"add_hash", HashProfile::Weak,
     [] (const std::string & s) { return add_hash(s.data(), s.size()); }},
    {"xor_hash", HashProfile::Weak,
     [] (const std::string & s) { return xor_hash(s.data(), s.size()); }},
    {"rot_hash", HashProfile::Weak,
     [] (const std::string & s) { return rot_hash(s.data(), s.size()); }},
    {"djb_hash", HashProfile::General,
     [] (const std::string & s) { return djb_hash(s.data(), s.size()); }},
    {"sax_hash", HashProfile::General,
     [] (const std::string & s) { return sax_hash(s.data(), s.size()); }},
    {"fnv_hash", HashProfile::General,
     [] (const std::string & s) { return fnv_hash(s.data(), s.size()); }},
    {"oat_hash", HashProfile::General,
     [] (const std::string & s) { return oat_hash(s.data(), s.size()); }},
    {"jsw_hash", HashProfile::Weak,
     [] (const std::string & s) { return jsw_hash(s.data(), s.size()); }},
    {"elf_hash", HashProfile::Weak,
     [] (const std::string & s) { return elf_hash(s.data(), s.size()); }},
    {"jen_hash", HashProfile::Weak,
     [] (const std::string & s)
       {
         return jen_hash(static_cast<const void *>(s.data()), s.size(),
                         Default_Hash_Seed);
       }},
    {"SuperFastHash", HashProfile::Weak,
     [] (const std::string & s) { return SuperFastHash(s); }},
    {"murmur3hash", HashProfile::Strong,
     [] (const std::string & s) { return murmur3hash(s, 42ul); }},
    {"xxhash64_hash", HashProfile::Strong,
     [] (const std::string & s) { return xxhash64_hash(s.data(), s.size(), 42); }},
    {"wyhash_hash", HashProfile::Strong,
     [] (const std::string & s) { return wyhash_hash(s.data(), s.size(), 42); }},
    {"siphash24_hash", HashProfile::Strong,
     [] (const std::string & s) { return siphash24_hash(s.data(), s.size()); }}
  };
}

std::string format_metrics_table(const std::vector<std::pair<std::string, Metrics>> & rows)
{
  std::ostringstream out;
  out << "\nHash statistics summary\n";
  out << std::left << std::setw(16) << "name"
      << std::right << std::setw(8) << "uniq"
      << std::setw(8) << "chi-r"
      << std::setw(8) << "chi-p"
      << std::setw(8) << "chi-s"
      << std::setw(8) << "avalan"
      << std::setw(8) << "bias"
      << std::setw(10) << "MiB/s"
      << std::setw(12) << "calls"
      << std::setw(10) << "lat-ns"
      << std::setw(8) << "coll"
      << std::setw(8) << "max-bkt"
      << '\n';

  for (const auto & [name, metrics] : rows)
    {
      out << std::left << std::setw(16) << name
          << std::right << std::fixed << std::setprecision(3)
          << std::setw(8) << metrics.uniqueness
          << std::setw(8) << metrics.random_chi
          << std::setw(8) << metrics.prefix_chi
          << std::setw(8) << metrics.sequential_chi
          << std::setw(8) << metrics.avalanche
          << std::setw(8) << metrics.max_bit_bias
          << std::setw(10) << std::setprecision(1) << metrics.throughput_mib_s
          << std::setw(12) << metrics.total_calls
          << std::setw(10) << std::setprecision(2) << metrics.latency_ns
          << std::setw(8) << metrics.full_collisions
          << std::setw(8) << metrics.max_random_bucket
          << '\n';
    }

  return out.str();
}

class HashStatisticalTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    init_jsw(42);
  }

  static const std::vector<std::string> & random_samples()
  {
    static const auto samples = make_random_binary_samples(20000, 1, 64, 0x12345678ULL);
    return samples;
  }

  static const std::vector<std::string> & prefix_samples()
  {
    static const auto samples = make_prefix_samples(20000, 28, 8);
    return samples;
  }

  static const std::vector<std::string> & sequential_samples()
  {
    static const auto samples = make_sequential_samples(20000);
    return samples;
  }

  static const std::vector<std::string> & avalanche_samples()
  {
    static const auto samples = make_avalanche_samples(128, 32, 0xabcdefULL);
    return samples;
  }
};

TEST_F(HashStatisticalTest, DispersionAvalancheAndThroughputProfiles)
{
  std::vector<std::pair<std::string, Metrics>> rows;

  for (const auto & candidate : all_candidates())
    {
      SCOPED_TRACE(candidate.name);
      const Metrics metrics = measure_metrics(candidate,
                                             random_samples(),
                                             prefix_samples(),
                                             sequential_samples(),
                                             avalanche_samples());
      rows.emplace_back(candidate.name, metrics);

      if (candidate.profile == HashProfile::Weak)
        {
          EXPECT_GT(metrics.throughput_mib_s, 0.0);
          continue;
        }

      const Thresholds thresholds = thresholds_for(candidate.profile);
      EXPECT_GE(metrics.uniqueness, thresholds.min_uniqueness)
          << candidate.name << " uniqueness too low";
      EXPECT_LE(metrics.random_chi, thresholds.max_random_chi)
          << candidate.name << " random chi-square too high";
      EXPECT_LE(metrics.prefix_chi, thresholds.max_prefix_chi)
          << candidate.name << " shared-prefix chi-square too high";
      EXPECT_LE(metrics.sequential_chi, thresholds.max_sequential_chi)
          << candidate.name << " sequential chi-square too high";
      EXPECT_GE(metrics.avalanche, thresholds.min_avalanche)
          << candidate.name << " avalanche ratio too low";
      EXPECT_LE(metrics.max_bit_bias, thresholds.max_bit_bias)
          << candidate.name << " output-bit bias too high";
    }

  std::cout << format_metrics_table(rows) << std::flush;

  auto dispersion_score = [] (const Metrics & m)
  {
    return std::abs(1.0 - m.uniqueness) +
           std::abs(1.0 - m.random_chi) +
           std::abs(1.0 - m.prefix_chi) +
           std::abs(1.0 - m.sequential_chi) +
           std::abs(0.5 - m.avalanche) +
           m.max_bit_bias;
  };

  auto dispersion_rows = rows;
  std::sort(dispersion_rows.begin(), dispersion_rows.end(),
            [&] (const auto & a, const auto & b)
            {
              return dispersion_score(a.second) < dispersion_score(b.second);
            });

  std::cout << "\nRanking by Dispersion (lower is better):\n";
  for (size_t i = 0; i < dispersion_rows.size(); ++i)
    std::cout << std::setw(2) << i + 1 << ". "
              << std::left << std::setw(16) << dispersion_rows[i].first
              << " Score: " << std::fixed << std::setprecision(4)
              << dispersion_score(dispersion_rows[i].second) << "\n";

  auto speed_rows = rows;
  std::sort(speed_rows.begin(), speed_rows.end(),
            [] (const auto & a, const auto & b)
            {
              return a.second.throughput_mib_s > b.second.throughput_mib_s;
            });

  std::cout << "\nRanking by Speed (higher is better):\n";
  for (size_t i = 0; i < speed_rows.size(); ++i)
    std::cout << std::setw(2) << i + 1 << ". "
              << std::left << std::setw(16) << speed_rows[i].first
              << " Speed: " << std::fixed << std::setprecision(1) << std::right << std::setw(8)
              << speed_rows[i].second.throughput_mib_s << " MiB/s ("
              << std::fixed << std::setprecision(2) << std::setw(6)
              << speed_rows[i].second.latency_ns << " ns/op)\n";
}

TEST_F(HashStatisticalTest, PairHashCombinersReportOrderingSensitivity)
{
  size_t swapped_pair_dft_collisions = 0;
  size_t swapped_pair_snd_collisions = 0;

  for (std::uint64_t i = 1; i <= 4096; ++i)
    {
      const auto lhs = std::make_pair(i, i + 1);
      const auto rhs = std::make_pair(i + 1, i);
      if (pair_dft_hash_fct(lhs) == pair_dft_hash_fct(rhs))
        ++swapped_pair_dft_collisions;
      if (pair_snd_hash_fct(lhs) == pair_snd_hash_fct(rhs))
        ++swapped_pair_snd_collisions;
    }

  std::cout << "\npair_dft swapped collisions: " << swapped_pair_dft_collisions
            << "\npair_snd swapped collisions: " << swapped_pair_snd_collisions
            << std::endl;

  EXPECT_GT(swapped_pair_dft_collisions, swapped_pair_snd_collisions);
}

TEST_F(HashStatisticalTest, MapHashAdaptorTracksTheKeyHasherExactly)
{
  const std::pair<std::string, int> item{"aleph", 42};
  const auto dft = map_hash_fct<std::string, int>(
      [] (const std::string & key) { return dft_hash_fct(key); }, item);
  const auto snd = map_hash_fct<std::string, int>(
      [] (const std::string & key) { return snd_hash_fct(key); }, item);
  const auto xxh = map_hash_fct<std::string, int>(
      [] (const std::string & key) { return xxhash64_hash(key); }, item);

  EXPECT_EQ(dft, dft_hash_fct(item.first));
  EXPECT_EQ(snd, snd_hash_fct(item.first));
  EXPECT_EQ(xxh, xxhash64_hash(item.first));
}

} // namespace
