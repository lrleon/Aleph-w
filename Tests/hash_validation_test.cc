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
 * @file hash_validation_test.cc
 * @brief Advanced hash validation tests for [HASH-007].
 *
 * Goes beyond the regression/smoke tests in hash_fct_test.cc and the basic
 * statistical checks in hash_statistical_test.cc.  Covers:
 *
 *  - Strict Avalanche Criterion (SAC) — full bit-flip matrix
 *  - Bit Independence Criterion (BIC) — pairwise output-bit independence
 *  - Differential uniformity — Hamming distance input → output
 *  - Multi-resolution distribution — chi-square at multiple table sizes
 *  - Adversarial / pathological patterns (sparse keys, cyclic, flooding-like)
 *  - Alignment sensitivity — same data hashed at different memory offsets
 *  - Seed independence — different seeds produce statistically independent
 *    output streams
 */

# include <gtest/gtest.h>
# include <hash-fct.H>

# include <algorithm>
# include <array>
# include <bit>
# include <cmath>
# include <cstdint>
# include <cstring>
# include <iomanip>
# include <iostream>
# include <numeric>
# include <random>
# include <sstream>
# include <string>
# include <string_view>
# include <unordered_set>
# include <vector>

# include <tpl_array.H>

using namespace Aleph;

// ======================================================================
// Infrastructure
// ======================================================================

namespace
{

struct HashFn
{
  const char * name = "";
  size_t (* hash)(const void *, size_t) = nullptr;
  bool is_strong = false;
};

// Wrappers that normalize the interface to (const void *, size_t) → size_t.
namespace Wrap
{
  size_t add    (const void * p, size_t len) { return add_hash(p, len); }
  size_t xor_   (const void * p, size_t len) { return xor_hash(p, len); }
  size_t rot    (const void * p, size_t len) { return rot_hash(p, len); }
  size_t djb    (const void * p, size_t len) { return djb_hash(p, len); }
  size_t sax    (const void * p, size_t len) { return sax_hash(p, len); }
  size_t fnv    (const void * p, size_t len) { return fnv_hash(p, len); }
  size_t oat    (const void * p, size_t len) { return oat_hash(p, len); }
  size_t jsw    (const void * p, size_t len) { return jsw_hash(p, len); }
  size_t elf    (const void * p, size_t len) { return elf_hash(p, len); }
  size_t jen    (const void * p, size_t len)
  { return jen_hash(p, len, Default_Hash_Seed); }
  size_t sfh    (const void * p, size_t len) { return SuperFastHash(p, len); }
  size_t murmur3(const void * p, size_t len)
  { return murmur3hash(std::string(static_cast<const char *>(p), len), 42u); }
  size_t xxh64  (const void * p, size_t len) { return xxhash64_hash(p, len, 42); }
  size_t wyh    (const void * p, size_t len) { return wyhash_hash(p, len, 42); }
  size_t sip24  (const void * p, size_t len) { return siphash24_hash(p, len); }
} // namespace Wrap

// Profile classification mirrors hash_statistical_test.cc.
Array<HashFn> strong_candidates()
{
  return {
    {"murmur3hash",    Wrap::murmur3,  true},
    {"xxhash64_hash",  Wrap::xxh64,    true},
    {"wyhash_hash",    Wrap::wyh,      true},
    {"siphash24_hash", Wrap::sip24,    true},
  };
}

Array<HashFn> general_candidates()
{
  return {
    {"SuperFastHash", Wrap::sfh,  false},
    {"fnv_hash",      Wrap::fnv,  false},
    {"oat_hash",      Wrap::oat,  false},
    {"djb_hash",      Wrap::djb,  false},
  };
}

// Legacy / weak functions — included in the ranking but not subject to
// quality thresholds (their profile is Weak in hash_statistical_test.cc).
Array<HashFn> weak_candidates()
{
  return {
    {"add_hash",  Wrap::add,   false},
    {"xor_hash",  Wrap::xor_,  false},
    {"rot_hash",  Wrap::rot,   false},
    {"sax_hash",  Wrap::sax,   false},
    {"jsw_hash",  Wrap::jsw,   false},
    {"elf_hash",  Wrap::elf,   false},
    {"jen_hash",  Wrap::jen,   false},
  };
}

Array<HashFn> all_tested()
{
  auto out = strong_candidates();
  for (const auto & c : general_candidates())
    out.append(c);
  for (const auto & c : weak_candidates())
    out.append(c);
  return out;
}

// Generate N random keys of fixed length.
std::vector<std::string> random_keys(size_t n, size_t len, std::uint64_t seed)
{
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> byte_dist(0, 255);
  std::vector<std::string> out;
  out.reserve(n);
  for (size_t i = 0; i < n; ++i)
    {
      std::string s(len, '\0');
      for (size_t j = 0; j < len; ++j)
        s[j] = static_cast<char>(byte_dist(rng));
      out.push_back(std::move(s));
    }
  return out;
}

// Effective output bits (how many bits are actually exercised).
size_t effective_bits(const HashFn & fn,
                      const std::vector<std::string> & keys)
{
  size_t mask = 0;
  for (const auto & k : keys)
    mask |= fn.hash(k.data(), k.size());
  return std::max<size_t>(std::bit_width(mask), 1);
}

// ──────────────────────────────────────────────────────────────────────
// Ranking infrastructure
// ──────────────────────────────────────────────────────────────────────

struct RankMetrics
{
  std::string name;
  bool        is_strong    = false;
  bool        is_weak      = false;  // true for known weak-profile functions
  double      sac_max_bias = 1.0;   // lower is better
  double      sac_avg_bias = 1.0;
  double      bic_max_corr = 1.0;   // lower is better
  double      diff_worst   = 1.0;   // max |mean - bits/2| / (bits/2)
  double      multireschi  = 999.0; // max reduced chi-square across resolutions
  double      adv_chi_ctr  = 999.0; // chi on counter keys
  double      adv_chi_pfx  = 999.0; // chi on long-prefix keys
  bool        align_ok     = false;
  double      score        = 999.0; // composite (lower is better)
};

// Lower-is-better composite score used for ranking.
// Weights are chosen so that SAC/BIC/differential carry the most weight
// (quality) while adversarial chi and multi-res chi contribute equally.
double ranking_score(const RankMetrics & m)
{
  // Penalise if alignment is broken: add a large constant.
  const double align_penalty = m.align_ok ? 0.0 : 10.0;

  return 3.0  * m.sac_max_bias
       + 2.0  * m.sac_avg_bias
       + 2.0  * m.bic_max_corr
       + 2.0  * m.diff_worst
       + 1.0  * m.multireschi
       + 0.5  * m.adv_chi_ctr
       + 0.5  * m.adv_chi_pfx
       + align_penalty;
}

// Compute compact ranking metrics for a single function.
RankMetrics compute_rank_metrics(const HashFn & fn)
{
  // Weak-profile set must match all_tested() entries flagged as weak.
  static constexpr std::string_view weak_names[] = {
    "add_hash", "xor_hash", "rot_hash", "sax_hash",
    "jsw_hash", "elf_hash", "jen_hash"
  };

  RankMetrics rm;
  rm.name      = fn.name;
  rm.is_strong = fn.is_strong;
  rm.is_weak   = std::any_of(std::begin(weak_names), std::end(weak_names),
                              [&](std::string_view w){ return w == fn.name; });

  constexpr size_t N_s   = 4096;
  constexpr size_t klen  = 16;

  auto keys = random_keys(N_s, klen, 0x1234ABCD5678ULL);
  const size_t out_bits  = effective_bits(fn, keys);
  const size_t in_bits   = klen * 8;

  // ── SAC ────────────────────────────────────────────────────────────
  {
    std::vector<std::vector<size_t>> mat(in_bits,
                                         std::vector<size_t>(out_bits, 0));
    for (const auto & base : keys)
      {
        const size_t h0 = fn.hash(base.data(), base.size());
        for (size_t i = 0; i < in_bits; ++i)
          {
            std::string mut = base;
            mut[i / 8] ^= static_cast<char>(1u << (i % 8));
            const size_t diff = h0 ^ fn.hash(mut.data(), mut.size());
            for (size_t j = 0; j < out_bits; ++j)
              mat[i][j] += (diff >> j) & 1u;
          }
      }
    double max_bias = 0, sum_bias = 0;
    for (size_t i = 0; i < in_bits; ++i)
      for (size_t j = 0; j < out_bits; ++j)
        {
          const double p    = static_cast<double>(mat[i][j]) / N_s;
          const double bias = std::abs(p - 0.5);
          max_bias = std::max(max_bias, bias);
          sum_bias += bias;
        }
    rm.sac_max_bias = max_bias;
    rm.sac_avg_bias = sum_bias / static_cast<double>(in_bits * out_bits);
  }

  // ── BIC (sampled) ─────────────────────────────────────────────────
  {
    std::mt19937_64 rng(0xB1C3);
    constexpr size_t sampled_i = 16;
    constexpr size_t pairs     = 64;
    double max_corr = 0;
    for (size_t t = 0; t < sampled_i; ++t)
      {
        const size_t i = rng() % in_bits;
        std::vector<size_t> diffs(N_s);
        for (size_t s = 0; s < N_s; ++s)
          {
            const size_t h0 = fn.hash(keys[s].data(), keys[s].size());
            std::string mut = keys[s];
            mut[i / 8] ^= static_cast<char>(1u << (i % 8));
            diffs[s] = h0 ^ fn.hash(mut.data(), mut.size());
          }
        for (size_t p = 0; p < pairs; ++p)
          {
            const size_t j = rng() % out_bits;
            size_t k = rng() % (out_bits - 1);
            if (k >= j) ++k;
            double sj = 0, sk = 0, sjk = 0, sj2 = 0, sk2 = 0;
            for (size_t s = 0; s < N_s; ++s)
              {
                const double bj = (diffs[s] >> j) & 1u;
                const double bk = (diffs[s] >> k) & 1u;
                sj += bj; sk += bk; sjk += bj*bk;
                sj2 += bj*bj; sk2 += bk*bk;
              }
            const double n   = N_s;
            const double num = n*sjk - sj*sk;
            const double den = std::sqrt((n*sj2-sj*sj)*(n*sk2-sk*sk));
            if (den > 1e-12) max_corr = std::max(max_corr, std::abs(num/den));
          }
      }
    rm.bic_max_corr = max_corr;
  }

  // ── Differential uniformity ────────────────────────────────────────
  {
    std::mt19937_64 rng(0xD1FF3);
    const double ideal = static_cast<double>(out_bits) / 2.0;
    double worst = 0;
    for (size_t target : {1u, 4u, 16u})
      {
        double sum = 0;
        for (const auto & base : keys)
          {
            std::string mut = base;
            std::vector<size_t> bits(in_bits);
            std::iota(bits.begin(), bits.end(), 0);
            for (size_t f = 0; f < target and f < bits.size(); ++f)
              {
                std::uniform_int_distribution<size_t> dist(f, bits.size()-1);
                std::swap(bits[f], bits[dist(rng)]);
                const size_t b = bits[f];
                mut[b/8] ^= static_cast<char>(1u << (b%8));
              }
            const size_t h0 = fn.hash(base.data(), base.size());
            const size_t h1 = fn.hash(mut.data(), mut.size());
            sum += std::popcount(h0 ^ h1);
          }
        const double mean = sum / N_s;
        const double rel  = std::abs(mean - ideal) / ideal;
        worst = std::max(worst, rel);
      }
    rm.diff_worst = worst;
  }

  // ── Multi-resolution chi ───────────────────────────────────────────
  {
    constexpr size_t N_chi = 50000;
    auto chi_keys = random_keys(N_chi, klen, 0x4E58);
    const std::array<size_t, 4> buckets = {127, 1024, 4096, 8191};
    double max_chi = 0;
    for (const size_t b : buckets)
      {
        std::vector<size_t> cnt(b, 0);
        for (const auto & k : chi_keys)
          ++cnt[fn.hash(k.data(), k.size()) % b];
        const double exp = static_cast<double>(N_chi) / b;
        double chi = 0;
        for (const size_t c : cnt)
          { const double d = c - exp; chi += d*d/exp; }
        max_chi = std::max(max_chi, chi / (b - 1));
      }
    rm.multireschi = max_chi;
  }

  // ── Adversarial: counter keys ──────────────────────────────────────
  {
    constexpr size_t N_adv = 65536;
    constexpr size_t B_adv = 1024;
    std::vector<size_t> cnt(B_adv, 0);
    for (size_t i = 0; i < N_adv; ++i)
      { std::uint64_t v = i; cnt[fn.hash(&v, sizeof(v)) % B_adv]++; }
    const double exp = static_cast<double>(N_adv) / B_adv;
    double chi = 0;
    for (const size_t c : cnt) { const double d = c - exp; chi += d*d/exp; }
    rm.adv_chi_ctr = chi / (B_adv - 1);
  }

  // ── Adversarial: long-prefix keys ─────────────────────────────────
  {
    constexpr size_t N_adv  = 65536;
    constexpr size_t B_adv  = 1024;
    constexpr size_t pfxlen = 128;
    const std::string pfx(pfxlen, 'Z');
    std::vector<size_t> cnt(B_adv, 0);
    for (size_t i = 0; i < N_adv; ++i)
      {
        std::string k = pfx;
        std::uint32_t s = static_cast<std::uint32_t>(i);
        k.append(reinterpret_cast<const char *>(&s), sizeof(s));
        cnt[fn.hash(k.data(), k.size()) % B_adv]++;
      }
    const double exp = static_cast<double>(N_adv) / B_adv;
    double chi = 0;
    for (const size_t c : cnt) { const double d = c - exp; chi += d*d/exp; }
    rm.adv_chi_pfx = chi / (B_adv - 1);
  }

  // ── Alignment ─────────────────────────────────────────────────────
  {
    constexpr size_t test_len = 37;
    constexpr size_t pad      = 16;
    std::vector<char> buf(test_len + pad);
    std::mt19937_64 rng2(0xA119ULL);
    for (auto & c : buf) c = static_cast<char>(rng2() & 0xff);
    const size_t ref = fn.hash(buf.data(), test_len);
    rm.align_ok = true;
    for (size_t off = 1; off < pad; ++off)
      {
        std::memmove(buf.data() + off, buf.data(), test_len);
        if (fn.hash(buf.data() + off, test_len) != ref)
          { rm.align_ok = false; break; }
        std::memmove(buf.data(), buf.data() + off, test_len);
      }
  }

  rm.score = ranking_score(rm);
  return rm;
}

} // anonymous namespace

// ======================================================================
// Test fixture
// ======================================================================

class HashValidation : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    init_jsw(42);
  }
};

// ======================================================================
// 1. Strict Avalanche Criterion (SAC)
//
// For each input bit i, flip it and measure the probability that each
// output bit j changes.  A perfect hash has P(j flips | i flipped) = 0.5
// for all (i,j).
//
// We compute the full matrix and check:
//   - The per-cell bias max |P - 0.5| over all (i,j)
//   - The per-row mean (should be ~0.5)
//   - The global mean (should be ~0.5)
// ======================================================================

TEST_F(HashValidation, StrictAvalancheCriterion)
{
  constexpr size_t N_samples = 2048;
  constexpr size_t key_len = 16;

  auto keys = random_keys(N_samples, key_len, 0x5AC00ULL + 42);

  const size_t input_bits = key_len * 8;

  std::cout << "\n=== Strict Avalanche Criterion (SAC) ===\n"
            << "Samples: " << N_samples
            << "  Key length: " << key_len << " bytes\n\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      const size_t out_bits = effective_bits(fn, keys);

      // sac_matrix[i][j] = number of times output bit j flipped when input
      //                     bit i was flipped, over all N_samples keys.
      std::vector<std::vector<size_t>> sac_matrix(
        input_bits, std::vector<size_t>(out_bits, 0));

      for (const auto & base : keys)
        {
          const size_t h0 = fn.hash(base.data(), base.size());
          for (size_t i = 0; i < input_bits; ++i)
            {
              std::string mutated = base;
              mutated[i / 8] ^= static_cast<char>(1u << (i % 8));
              const size_t h1 = fn.hash(mutated.data(), mutated.size());
              const size_t diff = h0 ^ h1;
              for (size_t j = 0; j < out_bits; ++j)
                sac_matrix[i][j] += (diff >> j) & 1u;
            }
        }

      // Compute statistics.
      double max_bias = 0.0;
      double sum_bias = 0.0;
      size_t cells = 0;
      double worst_row_mean = 0.0;

      for (size_t i = 0; i < input_bits; ++i)
        {
          double row_sum = 0.0;
          for (size_t j = 0; j < out_bits; ++j)
            {
              const double p = static_cast<double>(sac_matrix[i][j])
                             / N_samples;
              const double bias = std::abs(p - 0.5);
              max_bias = std::max(max_bias, bias);
              sum_bias += bias;
              row_sum += p;
              ++cells;
            }
          const double row_mean = row_sum / out_bits;
          worst_row_mean = std::max(worst_row_mean, std::abs(row_mean - 0.5));
        }

      const double avg_bias = sum_bias / cells;

      std::cout << std::left << std::setw(16) << fn.name
                << "  out_bits=" << std::setw(3) << out_bits
                << "  max_bias=" << std::fixed << std::setprecision(4) << max_bias
                << "  avg_bias=" << avg_bias
                << "  worst_row=" << worst_row_mean << "\n";

      // Strong hashes: max per-cell bias should be < 0.10, average < 0.03.
      EXPECT_LT(max_bias, 0.10)
          << fn.name << " SAC max per-cell bias exceeds 0.10";
      EXPECT_LT(avg_bias, 0.03)
          << fn.name << " SAC average bias exceeds 0.03";
      EXPECT_LT(worst_row_mean, 0.05)
          << fn.name << " SAC worst row mean deviation exceeds 0.05";
    }
}

// ======================================================================
// 2. Bit Independence Criterion (BIC)
//
// For a given input-bit flip, check that *pairs* of output bits flip
// independently.  If output bits j and k are independent under flip of
// input bit i, then Corr(flip_j, flip_k) ≈ 0.
//
// We sample a subset of (i, j, k) triples to keep it tractable.
// ======================================================================

TEST_F(HashValidation, BitIndependenceCriterion)
{
  constexpr size_t N_samples = 4096;
  constexpr size_t key_len = 16;
  constexpr size_t max_pairs = 256; // random (j,k) pairs per input bit i

  auto keys = random_keys(N_samples, key_len, 0xB1CULL);
  const size_t input_bits = key_len * 8;

  std::cout << "\n=== Bit Independence Criterion (BIC) ===\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);
      const size_t out_bits = effective_bits(fn, keys);
      if (out_bits < 4)
        continue;

      // For each input bit, collect the flip vector for all output bits.
      // flip_vectors[i] = vector of size N_samples, each element is the
      // diff bitmask for that sample when input bit i is flipped.
      // We only sample a few input bits to keep it fast.
      std::mt19937_64 rng(0xB1C2ULL);
      constexpr size_t sampled_input_bits = 32;
      std::vector<size_t> input_bit_indices(sampled_input_bits);
      for (size_t idx = 0; idx < sampled_input_bits; ++idx)
        input_bit_indices[idx] = rng() % input_bits;

      double max_corr = 0.0;
      double sum_corr = 0.0;
      size_t n_triples = 0;

      for (const size_t i : input_bit_indices)
        {
          // Collect flip vectors for all samples.
          std::vector<size_t> diffs(N_samples);
          for (size_t s = 0; s < N_samples; ++s)
            {
              const size_t h0 = fn.hash(keys[s].data(), keys[s].size());
              std::string mutated = keys[s];
              mutated[i / 8] ^= static_cast<char>(1u << (i % 8));
              const size_t h1 = fn.hash(mutated.data(), mutated.size());
              diffs[s] = h0 ^ h1;
            }

          // Sample random (j, k) output-bit pairs.
          for (size_t p = 0; p < max_pairs; ++p)
            {
              const size_t j = rng() % out_bits;
              size_t k = rng() % (out_bits - 1);
              if (k >= j)
                ++k;

              // Compute Pearson correlation between bit j flips and bit k flips.
              double sum_jk = 0, sum_j = 0, sum_k = 0;
              double sum_j2 = 0, sum_k2 = 0;
              for (size_t s = 0; s < N_samples; ++s)
                {
                  const double bj = static_cast<double>((diffs[s] >> j) & 1u);
                  const double bk = static_cast<double>((diffs[s] >> k) & 1u);
                  sum_j += bj;
                  sum_k += bk;
                  sum_jk += bj * bk;
                  sum_j2 += bj * bj;
                  sum_k2 += bk * bk;
                }
              const double n = static_cast<double>(N_samples);
              const double num = n * sum_jk - sum_j * sum_k;
              const double den = std::sqrt((n * sum_j2 - sum_j * sum_j) *
                                           (n * sum_k2 - sum_k * sum_k));
              if (den > 1e-12)
                {
                  const double corr = std::abs(num / den);
                  max_corr = std::max(max_corr, corr);
                  sum_corr += corr;
                  ++n_triples;
                }
            }
        }

      const double avg_corr = n_triples > 0 ? sum_corr / n_triples : 0.0;

      std::cout << std::left << std::setw(16) << fn.name
                << "  max_corr=" << std::fixed << std::setprecision(4) << max_corr
                << "  avg_corr=" << avg_corr
                << "  triples=" << n_triples << "\n";

      EXPECT_LT(max_corr, 0.15)
          << fn.name << " BIC max pairwise correlation exceeds 0.15";
      EXPECT_LT(avg_corr, 0.05)
          << fn.name << " BIC average pairwise correlation exceeds 0.05";
    }
}

// ======================================================================
// 3. Differential Uniformity
//
// Hash pairs of inputs at various Hamming distances and verify that the
// output Hamming distance distribution is centered around bits/2
// regardless of the input distance.
// ======================================================================

TEST_F(HashValidation, DifferentialUniformity)
{
  constexpr size_t N_samples = 4096;
  constexpr size_t key_len = 16;

  auto keys = random_keys(N_samples, key_len, 0xD1FFULL);

  std::cout << "\n=== Differential Uniformity ===\n";

  const std::array<size_t, 5> input_distances = {1, 2, 4, 8, 16};

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);
      const size_t out_bits = effective_bits(fn, keys);
      const double ideal_mean = static_cast<double>(out_bits) / 2.0;

      std::cout << std::left << std::setw(16) << fn.name;

      for (const size_t target_dist : input_distances)
        {
          // For each key, flip exactly `target_dist` random bits and measure
          // the output Hamming distance.
          std::mt19937_64 rng(0xD1FF2ULL + target_dist);
          double sum_out_dist = 0.0;
          double sum_sq = 0.0;
          size_t count = 0;

          for (const auto & base : keys)
            {
              const size_t h0 = fn.hash(base.data(), base.size());

              std::string mutated = base;
              // Flip target_dist distinct random bits.
              std::vector<size_t> all_bits(key_len * 8);
              std::iota(all_bits.begin(), all_bits.end(), 0);
              for (size_t f = 0; f < target_dist and f < all_bits.size(); ++f)
                {
                  std::uniform_int_distribution<size_t> dist(f, all_bits.size() - 1);
                  const size_t pick = dist(rng);
                  std::swap(all_bits[f], all_bits[pick]);
                  const size_t bit = all_bits[f];
                  mutated[bit / 8] ^= static_cast<char>(1u << (bit % 8));
                }

              const size_t h1 = fn.hash(mutated.data(), mutated.size());
              const double d = static_cast<double>(std::popcount(h0 ^ h1));
              sum_out_dist += d;
              sum_sq += d * d;
              ++count;
            }

          const double mean = sum_out_dist / count;
          const double variance = (sum_sq / count) - (mean * mean);
          const double stddev = std::sqrt(std::max(variance, 0.0));

          std::cout << "  d=" << target_dist
                    << " mean=" << std::fixed << std::setprecision(2) << mean
                    << " sd=" << stddev;

          // Mean output distance should be near ideal_mean regardless of
          // input distance.  Allow ±15% of ideal.
          EXPECT_NEAR(mean, ideal_mean, ideal_mean * 0.15)
              << fn.name << " differential mean off at input dist " << target_dist;
        }
      std::cout << "\n";
    }
}

// ======================================================================
// 4. Multi-Resolution Distribution (chi-square at multiple bucket counts)
//
// Good hashes should distribute uniformly at any table size, not just
// powers of 2.  We test primes and powers of 2.
// ======================================================================

TEST_F(HashValidation, MultiResolutionDistribution)
{
  constexpr size_t N_keys = 100000;
  auto keys = random_keys(N_keys, 16, 0x4E57ULL);

  const std::array<size_t, 8> bucket_counts =
    {127, 256, 509, 1024, 2039, 4096, 8191, 16384};

  std::cout << "\n=== Multi-Resolution Distribution ===\n"
            << std::left << std::setw(16) << "hash";
  for (const size_t b : bucket_counts)
    std::cout << std::right << std::setw(8) << b;
  std::cout << "\n";

  for (const auto & fn : all_tested())
    {
      SCOPED_TRACE(fn.name);
      std::cout << std::left << std::setw(16) << fn.name;

      for (const size_t buckets : bucket_counts)
        {
          std::vector<size_t> counts(buckets, 0);
          for (const auto & k : keys)
            ++counts[fn.hash(k.data(), k.size()) % buckets];

          const double expected = static_cast<double>(N_keys) / buckets;
          double chi = 0.0;
          for (const size_t c : counts)
            {
              const double diff = static_cast<double>(c) - expected;
              chi += (diff * diff) / expected;
            }
          const double reduced = chi / static_cast<double>(buckets - 1);

          std::cout << std::right << std::fixed << std::setprecision(3)
                    << std::setw(8) << reduced;

          if (fn.is_strong)
            {
              EXPECT_LT(reduced, 2.50)
                  << fn.name << " chi-square too high at " << buckets << " buckets";
            }
        }
      std::cout << "\n";
    }
}

// ======================================================================
// 5. Adversarial Pattern Tests
//
// Inputs designed to stress-test hash quality:
//  a) All-zero keys of varying lengths
//  b) Single-bit keys (only 1 bit set in the whole key)
//  c) Counter keys with small deltas
//  d) Keys differing only in high-order bytes
//  e) Cyclic/repeating patterns
//  f) Long common prefix with short varying suffix
// ======================================================================

TEST_F(HashValidation, AdversarialAllZeroLengths)
{
  std::cout << "\n=== Adversarial: all-zero keys of varying lengths ===\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      // Hash zero-filled keys from length 1..256; they must all be distinct.
      std::unordered_set<size_t> hashes;
      for (size_t len = 1; len <= 256; ++len)
        {
          const std::string key(len, '\0');
          hashes.insert(fn.hash(key.data(), key.size()));
        }

      const double uniqueness = static_cast<double>(hashes.size()) / 256.0;
      std::cout << std::left << std::setw(16) << fn.name
                << "  uniqueness=" << std::fixed << std::setprecision(4)
                << uniqueness << " (" << hashes.size() << "/256)\n";

      EXPECT_GE(uniqueness, 0.98)
          << fn.name << " too many collisions on zero-filled length-varying keys";
    }
}

TEST_F(HashValidation, AdversarialSingleBitKeys)
{
  constexpr size_t key_len = 32;
  const size_t total_bits = key_len * 8;

  std::cout << "\n=== Adversarial: single-bit-set keys ===\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      std::unordered_set<size_t> hashes;
      for (size_t bit = 0; bit < total_bits; ++bit)
        {
          std::string key(key_len, '\0');
          key[bit / 8] = static_cast<char>(1u << (bit % 8));
          hashes.insert(fn.hash(key.data(), key.size()));
        }

      const double uniqueness =
        static_cast<double>(hashes.size()) / total_bits;
      std::cout << std::left << std::setw(16) << fn.name
                << "  uniqueness=" << std::fixed << std::setprecision(4)
                << uniqueness << " (" << hashes.size() << "/" << total_bits << ")\n";

      EXPECT_GE(uniqueness, 0.99)
          << fn.name << " collisions on single-bit-set keys";
    }
}

TEST_F(HashValidation, AdversarialCounterKeys)
{
  // Sequential 64-bit counter packed into 8 bytes: typical worst case for
  // hash tables indexed by auto-increment IDs.
  constexpr size_t N = 100000;
  constexpr size_t buckets = 1024;

  std::cout << "\n=== Adversarial: counter keys ===\n";

  for (const auto & fn : all_tested())
    {
      SCOPED_TRACE(fn.name);

      std::vector<size_t> counts(buckets, 0);
      for (size_t i = 0; i < N; ++i)
        {
          std::uint64_t val = static_cast<std::uint64_t>(i);
          counts[fn.hash(&val, sizeof(val)) % buckets]++;
        }

      const double expected = static_cast<double>(N) / buckets;
      double chi = 0.0;
      for (const size_t c : counts)
        {
          const double diff = static_cast<double>(c) - expected;
          chi += (diff * diff) / expected;
        }
      const double reduced = chi / (buckets - 1);

      std::cout << std::left << std::setw(16) << fn.name
                << "  chi=" << std::fixed << std::setprecision(3) << reduced
                << "\n";

      if (fn.is_strong)
        EXPECT_LT(reduced, 3.0)
            << fn.name << " poor distribution on sequential counter keys";
    }
}

TEST_F(HashValidation, AdversarialHighByteDifference)
{
  // Keys that are identical except in the last byte.
  constexpr size_t key_len = 32;
  constexpr size_t N = 256;

  std::cout << "\n=== Adversarial: differ only in high byte ===\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      std::unordered_set<size_t> hashes;
      for (size_t v = 0; v < N; ++v)
        {
          std::string key(key_len, 'X');
          key[key_len - 1] = static_cast<char>(v);
          hashes.insert(fn.hash(key.data(), key.size()));
        }

      EXPECT_EQ(hashes.size(), N)
          << fn.name << " collision on keys differing only in last byte";
    }
}

TEST_F(HashValidation, AdversarialCyclicPatterns)
{
  // Repeating pattern "ABCD..." of varying length; tests whether the hash
  // can distinguish different-length repetitions of the same cycle.
  constexpr size_t max_len = 512;
  const std::string cycle = "ABCDEFGH";

  std::cout << "\n=== Adversarial: cyclic pattern keys ===\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      std::unordered_set<size_t> hashes;
      for (size_t len = 1; len <= max_len; ++len)
        {
          std::string key;
          key.reserve(len);
          for (size_t i = 0; i < len; ++i)
            key.push_back(cycle[i % cycle.size()]);
          hashes.insert(fn.hash(key.data(), key.size()));
        }

      const double uniqueness =
        static_cast<double>(hashes.size()) / max_len;
      std::cout << std::left << std::setw(16) << fn.name
                << "  uniqueness=" << std::fixed << std::setprecision(4)
                << uniqueness << "\n";

      EXPECT_GE(uniqueness, 0.99)
          << fn.name << " collisions on cyclic-pattern keys";
    }
}

TEST_F(HashValidation, AdversarialLongCommonPrefix)
{
  // 128-byte common prefix, 4-byte varying suffix.  Tests that the hash is
  // not dominated by the prefix.
  constexpr size_t prefix_len = 128;
  constexpr size_t N = 65536;
  constexpr size_t buckets = 1024;

  std::cout << "\n=== Adversarial: long common prefix ===\n";

  const std::string prefix(prefix_len, 'Z');

  for (const auto & fn : all_tested())
    {
      SCOPED_TRACE(fn.name);

      std::vector<size_t> counts(buckets, 0);
      for (size_t i = 0; i < N; ++i)
        {
          std::string key = prefix;
          std::uint32_t suffix = static_cast<std::uint32_t>(i);
          key.append(reinterpret_cast<const char *>(&suffix), sizeof(suffix));
          counts[fn.hash(key.data(), key.size()) % buckets]++;
        }

      const double expected = static_cast<double>(N) / buckets;
      double chi = 0.0;
      for (const size_t c : counts)
        {
          const double diff = static_cast<double>(c) - expected;
          chi += (diff * diff) / expected;
        }
      const double reduced = chi / (buckets - 1);

      std::cout << std::left << std::setw(16) << fn.name
                << "  chi=" << std::fixed << std::setprecision(3) << reduced
                << "\n";

      if (fn.is_strong)
        EXPECT_LT(reduced, 3.0)
            << fn.name
            << " poor distribution on long-prefix adversarial keys";
    }
}

// ======================================================================
// 6. Alignment Sensitivity
//
// Hash the same data at different memory alignments.  A correct hash
// must produce identical output regardless of alignment.
// ======================================================================

TEST_F(HashValidation, AlignmentSensitivity)
{
  constexpr size_t key_len = 37; // intentionally not a power of 2
  // Allocate a buffer with extra room so we can shift the start by 0..15.
  constexpr size_t pad = 16;
  std::vector<char> buf(key_len + pad);

  // Fill with a known pattern.
  std::mt19937_64 rng(0xA119EULL);
  for (auto & c : buf)
    c = static_cast<char>(rng() & 0xff);

  std::cout << "\n=== Alignment Sensitivity ===\n";

  for (const auto & fn : all_tested())
    {
      SCOPED_TRACE(fn.name);

      const size_t ref_hash = fn.hash(buf.data(), key_len);
      bool all_match = true;

      for (size_t offset = 1; offset < pad; ++offset)
        {
          // Copy the key data to a different alignment.
          std::memmove(buf.data() + offset, buf.data(), key_len);
          const size_t h = fn.hash(buf.data() + offset, key_len);
          // Restore original position.
          std::memmove(buf.data(), buf.data() + offset, key_len);

          if (h != ref_hash)
            {
              all_match = false;
              break;
            }
        }

      std::cout << std::left << std::setw(16) << fn.name
                << "  alignment-stable: " << (all_match ? "yes" : "NO")
                << "\n";

      EXPECT_TRUE(all_match)
          << fn.name << " produces different hashes for different alignments";
    }
}

// ======================================================================
// 7. Seed Independence
//
// Different seeds should produce statistically independent output.
// We measure the cross-seed correlation for the same set of keys.
// ======================================================================

TEST_F(HashValidation, SeedIndependence)
{
  constexpr size_t N = 8192;
  constexpr size_t key_len = 16;

  auto keys = random_keys(N, key_len, 0x5EED1ULL);

  std::cout << "\n=== Seed Independence ===\n";

  // Only test seeded functions.
  struct SeededFn
  {
    const char * name;
    size_t (* hash)(const void *, size_t, std::uint64_t);
  };

  const std::array<SeededFn, 3> seeded_fns = {{
    {"xxhash64_hash",  [](const void * p, size_t l, std::uint64_t s) -> size_t
                       { return xxhash64_hash(p, l, s); }},
    {"wyhash_hash",    [](const void * p, size_t l, std::uint64_t s) -> size_t
                       { return wyhash_hash(p, l, s); }},
    {"siphash24_hash", [](const void * p, size_t l, std::uint64_t s) -> size_t
                       { return siphash24_hash(p, l, s, s ^ 0xdeadbeefULL); }},
  }};

  const std::array<std::uint64_t, 4> seeds = {0, 1, 42, 0xffffffffffffffff};

  for (const auto & fn : seeded_fns)
    {
      SCOPED_TRACE(fn.name);

      // Compute hash vectors for each seed.
      std::vector<std::vector<size_t>> hash_vecs(seeds.size(),
                                                  std::vector<size_t>(N));
      for (size_t si = 0; si < seeds.size(); ++si)
        for (size_t ki = 0; ki < N; ++ki)
          hash_vecs[si][ki] = fn.hash(keys[ki].data(), keys[ki].size(),
                                      seeds[si]);

      // Compute Pearson correlation between all seed pairs (on the low 32 bits
      // cast to double to avoid saturation issues).
      double max_corr = 0.0;
      for (size_t a = 0; a < seeds.size(); ++a)
        for (size_t b = a + 1; b < seeds.size(); ++b)
          {
            double sum_a = 0, sum_b = 0, sum_ab = 0;
            double sum_a2 = 0, sum_b2 = 0;
            for (size_t k = 0; k < N; ++k)
              {
                const double va = static_cast<double>(
                  hash_vecs[a][k] & 0xFFFFFFFFu);
                const double vb = static_cast<double>(
                  hash_vecs[b][k] & 0xFFFFFFFFu);
                sum_a += va;
                sum_b += vb;
                sum_ab += va * vb;
                sum_a2 += va * va;
                sum_b2 += vb * vb;
              }
            const double n = static_cast<double>(N);
            const double num = n * sum_ab - sum_a * sum_b;
            const double den = std::sqrt((n * sum_a2 - sum_a * sum_a) *
                                         (n * sum_b2 - sum_b * sum_b));
            if (den > 1e-12)
              max_corr = std::max(max_corr, std::abs(num / den));
          }

      std::cout << std::left << std::setw(16) << fn.name
                << "  max_cross_seed_corr=" << std::fixed << std::setprecision(5)
                << max_corr << "\n";

      EXPECT_LT(max_corr, 0.05)
          << fn.name << " seeds are correlated (|r| > 0.05)";
    }
}

// ======================================================================
// 8. Sparse Key Test
//
// Keys with very few bits set, similar to SMHasher's "Sparse" test.
// Generate all keys with exactly K bits set (K = 1, 2, 3) in a
// fixed-size buffer and check for collisions.
// ======================================================================

TEST_F(HashValidation, SparseKeyCollisions)
{
  constexpr size_t key_len = 8;   // 64 bits total
  const size_t total_bits = key_len * 8;

  std::cout << "\n=== Sparse Key Collisions ===\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      // K = 1: all single-bit keys (64 keys)
      // K = 2: all two-bit keys (64 choose 2 = 2016 keys)
      // K = 3: all three-bit keys (64 choose 3 = 41664 keys)

      for (size_t K : {1, 2, 3})
        {
          std::unordered_set<size_t> hashes;
          size_t key_count = 0;

          if (K == 1)
            {
              for (size_t b0 = 0; b0 < total_bits; ++b0)
                {
                  std::string key(key_len, '\0');
                  key[b0 / 8] |= static_cast<char>(1u << (b0 % 8));
                  hashes.insert(fn.hash(key.data(), key.size()));
                  ++key_count;
                }
            }
          else if (K == 2)
            {
              for (size_t b0 = 0; b0 < total_bits; ++b0)
                for (size_t b1 = b0 + 1; b1 < total_bits; ++b1)
                  {
                    std::string key(key_len, '\0');
                    key[b0 / 8] |= static_cast<char>(1u << (b0 % 8));
                    key[b1 / 8] |= static_cast<char>(1u << (b1 % 8));
                    hashes.insert(fn.hash(key.data(), key.size()));
                    ++key_count;
                  }
            }
          else // K == 3
            {
              for (size_t b0 = 0; b0 < total_bits; ++b0)
                for (size_t b1 = b0 + 1; b1 < total_bits; ++b1)
                  for (size_t b2 = b1 + 1; b2 < total_bits; ++b2)
                    {
                      std::string key(key_len, '\0');
                      key[b0 / 8] |= static_cast<char>(1u << (b0 % 8));
                      key[b1 / 8] |= static_cast<char>(1u << (b1 % 8));
                      key[b2 / 8] |= static_cast<char>(1u << (b2 % 8));
                      hashes.insert(fn.hash(key.data(), key.size()));
                      ++key_count;
                    }
            }

          const double uniqueness =
            static_cast<double>(hashes.size()) / key_count;

          std::cout << std::left << std::setw(16) << fn.name
                    << "  K=" << K
                    << "  keys=" << std::setw(6) << key_count
                    << "  uniq=" << std::fixed << std::setprecision(6)
                    << uniqueness << "\n";

          // With 64-bit output and ≤ 41664 keys, birthday paradox says
          // collision probability is negligible (< 10^-10), so we expect 100%.
          if constexpr (sizeof(size_t) >= 8)
            EXPECT_GE(uniqueness, 0.9999)
                << fn.name << " sparse key collision at K=" << K;
        }
    }
}

// ======================================================================
// 9. Permutation Test
//
// Hash all permutations of a short input and verify uniform distribution.
// For a 4-byte input, there are 4! = 24 permutations of its bytes.
// For a 5-byte input, 5! = 120 permutations.
// ======================================================================

TEST_F(HashValidation, PermutationDistribution)
{
  std::cout << "\n=== Permutation Distribution ===\n";

  // Use a 6-byte key → 720 permutations.
  std::string base = "ABCDEF";
  std::sort(base.begin(), base.end());
  constexpr size_t buckets = 64;

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      std::vector<size_t> counts(buckets, 0);
      size_t n_perms = 0;
      std::string perm = base;

      do
        {
          counts[fn.hash(perm.data(), perm.size()) % buckets]++;
          ++n_perms;
        }
      while (std::next_permutation(perm.begin(), perm.end()));

      const double expected = static_cast<double>(n_perms) / buckets;
      double chi = 0.0;
      for (const size_t c : counts)
        {
          const double diff = static_cast<double>(c) - expected;
          chi += (diff * diff) / expected;
        }
      const double reduced = chi / (buckets - 1);

      // All permutations must produce distinct hashes.
      std::unordered_set<size_t> uniq;
      perm = base;
      do
        { uniq.insert(fn.hash(perm.data(), perm.size())); }
      while (std::next_permutation(perm.begin(), perm.end()));

      std::cout << std::left << std::setw(16) << fn.name
                << "  perms=" << n_perms
                << "  chi=" << std::fixed << std::setprecision(3) << reduced
                << "  uniq=" << uniq.size() << "/" << n_perms << "\n";

      EXPECT_EQ(uniq.size(), n_perms)
          << fn.name << " collision among byte permutations";
    }
}

// ======================================================================
// 10. Window Test
//
// Slide a small "window" of set bits through a zero-filled key and
// check that every position produces a different hash.  This catches
// hashes that ignore certain byte positions.
// ======================================================================

TEST_F(HashValidation, SlidingWindowTest)
{
  constexpr size_t key_len = 32;
  constexpr size_t window_size = 4; // 4 bytes of 0xFF sliding through

  std::cout << "\n=== Sliding Window Test ===\n";

  for (const auto & fn : strong_candidates())
    {
      SCOPED_TRACE(fn.name);

      const size_t positions = key_len - window_size + 1;
      std::unordered_set<size_t> hashes;

      for (size_t pos = 0; pos < positions; ++pos)
        {
          std::string key(key_len, '\0');
          for (size_t w = 0; w < window_size; ++w)
            key[pos + w] = '\xff';
          hashes.insert(fn.hash(key.data(), key.size()));
        }

      const double uniqueness =
        static_cast<double>(hashes.size()) / positions;

      std::cout << std::left << std::setw(16) << fn.name
                << "  positions=" << positions
                << "  uniq=" << hashes.size()
                << "  uniqueness=" << std::fixed << std::setprecision(4)
                << uniqueness << "\n";

      EXPECT_EQ(hashes.size(), positions)
          << fn.name << " collision in sliding window test";
    }
}

// ======================================================================
// 11. Length-Extension Sensitivity
//
// Verify that hash("ABC") != hash("ABC\0") != hash("ABC\0\0").
// Some hash functions that don't fully incorporate the length can be
// fooled by trailing zeros.
// ======================================================================

TEST_F(HashValidation, LengthExtensionSensitivity)
{
  std::cout << "\n=== Length Extension Sensitivity ===\n";

  const std::string base = "HashTestPayload";

  // Weak functions (add_hash, xor_hash, elf_hash, …) are known to be
  // insensitive to trailing NUL bytes by design — the assertion is only
  // applied to strong and general-profile functions.
  for (const auto & fn : all_tested())
    {
      SCOPED_TRACE(fn.name);

      std::unordered_set<size_t> hashes;
      for (size_t extra = 0; extra < 16; ++extra)
        {
          std::string key = base + std::string(extra, '\0');
          hashes.insert(fn.hash(key.data(), key.size()));
        }

      const bool is_weak =
        (std::string_view(fn.name) == "add_hash"  or
         std::string_view(fn.name) == "xor_hash"  or
         std::string_view(fn.name) == "rot_hash"  or
         std::string_view(fn.name) == "sax_hash"  or
         std::string_view(fn.name) == "jsw_hash"  or
         std::string_view(fn.name) == "elf_hash"  or
         std::string_view(fn.name) == "jen_hash");

      std::cout << std::left << std::setw(16) << fn.name
                << "  distinct=" << hashes.size() << "/16"
                << (is_weak ? "  [weak — not asserted]" : "")
                << "\n";

      if (not is_weak)
        EXPECT_EQ(hashes.size(), 16u)
            << fn.name << " fails length-extension sensitivity";
    }
}

// ======================================================================
// Ranking: composite quality score across all hash functions
//
// Metrics collected per function:
//   SAC max/avg bias, BIC max correlation, differential worst deviation,
//   multi-resolution chi-square, adversarial chi (counter + long prefix),
//   alignment stability.
//
// Lower score is better.  Strong-profile functions are expected to rank
// above the general-profile ones.
// ======================================================================

TEST_F(HashValidation, RankingAllHashFunctions)
{
  std::vector<RankMetrics> results;
  for (const auto & fn : all_tested())
    results.push_back(compute_rank_metrics(fn));

  // Sort by composite score (ascending = better).
  std::sort(results.begin(), results.end(),
            [] (const RankMetrics & a, const RankMetrics & b)
            { return a.score < b.score; });

  // ── Header ─────────────────────────────────────────────────────────
  std::cout
    << "\n"
    << "╔══════════════════════════════════════════════════════════════════════════════╗\n"
    << "║          [HASH-007] Hash Function Quality Ranking                          ║\n"
    << "╠══════════════════════════════════════════════════════════════════════════════╣\n"
    << "║ Rank  Name              SAC-max SAC-avg BIC-max Diff-wst MR-chi  Adv-ctr  ║\n"
    << "║                                                          (max)   Adv-pfx  ║\n"
    << "║                                                                  Align    ║\n"
    << "║                                                                  Score    ║\n"
    << "╠══════════════════════════════════════════════════════════════════════════════╣\n";

  for (size_t i = 0; i < results.size(); ++i)
    {
      const auto & m = results[i];
      const char * tag = m.is_strong ? "[Strong]"
                       : m.is_weak  ? "[Weak]  "
                                    : "[Genrl] ";
      std::cout
        << "║ " << std::setw(2) << i + 1
        << ".  " << std::left << std::setw(16) << m.name
        << " " << tag
        << "  " << std::right << std::fixed << std::setprecision(4)
        << std::setw(7) << m.sac_max_bias
        << " " << std::setw(7) << m.sac_avg_bias
        << " " << std::setw(7) << m.bic_max_corr
        << " " << std::setw(8) << m.diff_worst
        << " " << std::setw(7) << m.multireschi
        << " " << std::setw(7) << m.adv_chi_ctr
        << " ║\n"
        << "║ "  << std::string(22, ' ')
        << "          "
        << "                                "
        << std::setw(7) << m.adv_chi_pfx
        << " ║\n"
        << "║ " << std::string(22, ' ')
        << "          "
        << "                                "
        << (m.align_ok ? "  align✓" : "  alignX")
        << " ║\n"
        << "║ " << std::string(22, ' ')
        << "          "
        << "                         Score: "
        << std::setw(7) << m.score
        << " ║\n"
        << "╠══════════════════════════════════════════════════════════════════════════════╣\n";
    }

  std::cout
    << "╚══════════════════════════════════════════════════════════════════════════════╝\n"
    << "\nLower score = better quality.  Strong-profile threshold: score < 1.0\n"
    << "Columns: SAC-max/avg = Strict Avalanche Criterion bias\n"
    << "         BIC-max = Bit Independence max correlation\n"
    << "         Diff-wst = worst differential mean deviation (relative)\n"
    << "         MR-chi = max reduced chi-square across 4 table sizes\n"
    << "         Adv-ctr/pfx = adversarial pattern chi-square\n"
    << "\n========================================\n"
    << "[HASH-007] Advanced validation complete.\n"
    << "========================================\n";

  // Strong functions must rank above true general-profile ones (weak excluded).
  // (There must exist at least one strong fn with better score than any general fn.)
  double best_strong  = std::numeric_limits<double>::max();
  double best_general = std::numeric_limits<double>::max();
  for (const auto & m : results)
    {
      if (m.is_strong)
        best_strong  = std::min(best_strong,  m.score);
      else if (not m.is_weak)
        best_general = std::min(best_general, m.score);
    }
  EXPECT_LT(best_strong, best_general)
      << "Best strong-profile function should outrank best general-profile function";
}