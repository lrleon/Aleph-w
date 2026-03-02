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
 * @file streaming_algorithms_test.cc
 * @brief Tests for Reservoir Sampling, Count-Min Sketch, HyperLogLog and MinHash.
 */

# include <gtest/gtest.h>
# include <string>
# include <random>
# include <vector>

# include <tpl_dynArray.H>
# include <tpl_hash.H>
# include <tpl_dynList.H>
# include <reservoir-sampling.H>
# include <count-min-sketch.H>
# include <hyperloglog.H>
# include <minhash.H>
# include <simhash.H>

using namespace Aleph;

TEST(StreamingAlgorithms, ReservoirSampling)
{
  DynArray<int> data;
  for (int i = 0; i < 1000; ++i) data.append(i);

  const size_t k = 50;
  auto sample = reservoir_sample(data.begin(), data.end(), k, 12345);

  ASSERT_EQ(sample.size(), k);
  
  // All elements should be in the original range
  for (size_t i = 0; i < sample.size(); ++i)
    {
      EXPECT_GE(sample[i], 0);
      EXPECT_LT(sample[i], 1000);
    }

  // Check unique elements (mostly, since it's random sampling without replacement from unique input)
  DynHashSet<int> unique_vals;
  for (size_t i = 0; i < sample.size(); ++i)
    unique_vals.insert(sample[i]);
  EXPECT_EQ(unique_vals.size(), k);
}

TEST(StreamingAlgorithms, CountMinSketch)
{
  auto cms = Count_Min_Sketch<std::string>::from_error_bounds(0.01, 0.01); 
  
  cms.update("apple", 10);
  cms.update("banana", 5);
  cms.update("cherry", 1);
  
  for (int i = 0; i < 100; ++i) cms.update("apple"); // total apple = 110

  EXPECT_GE(cms.estimate("apple"), 110u);
  EXPECT_GE(cms.estimate("banana"), 5u);
  EXPECT_GE(cms.estimate("cherry"), 1u);
  // Theoretical bound: f + epsilon * total_count = 1 + 0.01 * 121 = 2.21
  // We use a safe bound of 10u to account for small total_count noise.
  EXPECT_LE(cms.estimate("cherry"), 10u); 
  
  // Deterministic property check
  const size_t before = cms.estimate("dragonfruit");
  cms.update("dragonfruit", 5);
  EXPECT_GE(cms.estimate("dragonfruit"), before + 5);

  // Validation tests
  EXPECT_THROW(Count_Min_Sketch<int>::from_error_bounds(0.0, 0.1), std::domain_error);
  EXPECT_THROW(Count_Min_Sketch<int>::from_error_bounds(0.1, 0.0), std::domain_error);
  EXPECT_THROW(Count_Min_Sketch<int>::from_error_bounds(0.1, 1.0), std::domain_error);
}

TEST(StreamingAlgorithms, HyperLogLog)
{
  HyperLogLog<int> hll(10); // 1024 registers, ~3.2% error
  
  const int n = 5000;
  for (int i = 0; i < n; ++i)
    hll.update(i % 1000); // 1000 unique elements

  double est = hll.estimate();
  EXPECT_NEAR(est, 1000.0, 1000.0 * 0.05); // allow 5% error

  // Validation tests
  EXPECT_THROW(HyperLogLog<int>(3), std::domain_error);
  EXPECT_THROW(HyperLogLog<int>(17), std::domain_error);
}

TEST(StreamingAlgorithms, MinHash)
{
  constexpr size_t K = 256;
  MinHash<int> mh1(K);
  MinHash<int> mh2(K);
  MinHash<int> mh3(K);
  MinHash<int> mh1_range(K);

  // Set 1: [0..99]
  // Set 2: [50..149] -> Intersection 50, Union 150 -> Jaccard = 1/3
  // Set 3: [200..299] -> Jaccard with 1 = 0
  DynArray<int> set1, set2, set3;
  set1.reserve(100);
  set2.reserve(100);
  set3.reserve(100);

  for (int i = 0; i < 100; ++i)
    {
      set1.append(i);
      set2.append(i + 50);
      set3.append(i + 200);
    }

  for (int i = 0; i < 100; ++i) mh1.update(i);
  for (int i = 50; i < 150; ++i) mh2.update(i);
  for (int i = 200; i < 300; ++i) mh3.update(i);
  mh1_range.update(set1.begin(), set1.end()); // range overload

  EXPECT_EQ(mh1.size(), K);
  EXPECT_EQ(mh2.size(), K);
  EXPECT_EQ(mh3.size(), K);
  EXPECT_EQ(mh1_range.size(), K);

  // update(Itor, Itor) must be equivalent to repeated point updates.
  EXPECT_DOUBLE_EQ(mh1.similarity(mh1_range), 1.0);
  const auto & sig1 = mh1.get_signature();
  const auto & sig1_range = mh1_range.get_signature();
  const auto * sig_ptr = &mh1_range.get_signature();
  EXPECT_NE(sig_ptr, nullptr);
  EXPECT_EQ(sig_ptr, &mh1_range.get_signature());
  ASSERT_EQ(sig1.size(), K);
  ASSERT_EQ(sig1_range.size(), K);

  bool changed_after_update = false;
  for (size_t i = 0; i < K; ++i)
    {
      EXPECT_EQ(sig1[i], sig1_range[i]);
      if (sig1_range[i] != std::numeric_limits<uint64_t>::max())
        changed_after_update = true;
    }
  EXPECT_TRUE(changed_after_update);

  double sim12 = mh1.similarity(mh2);
  double sim13 = mh1.similarity(mh3);

  EXPECT_NEAR(sim12, 1.0/3.0, 0.1);
  EXPECT_NEAR(sim13, 0.0, 0.05);

  // Merge contract: returns *this and corresponds to union of sets.
  MinHash<int> mh_union_expected(K);
  DynArray<int> union12;
  union12.reserve(150);
  for (int i = 0; i < 150; ++i)
    union12.append(i);
  mh_union_expected.update(union12.begin(), union12.end());

  MinHash<int> mh_merged = mh1;
  MinHash<int> & merge_ret = mh_merged.merge(mh2);
  EXPECT_EQ(&merge_ret, &mh_merged);
  EXPECT_EQ(mh_merged.size(), K);
  EXPECT_DOUBLE_EQ(mh_merged.similarity(mh_union_expected), 1.0);

  const auto & sig2 = mh2.get_signature();
  const auto & sig_merged = mh_merged.get_signature();
  const auto & sig_union = mh_union_expected.get_signature();
  ASSERT_EQ(sig_merged.size(), K);
  ASSERT_EQ(sig_union.size(), K);
  for (size_t i = 0; i < K; ++i)
    {
      EXPECT_EQ(sig_merged[i], std::min(sig1[i], sig2[i]));
      EXPECT_EQ(sig_merged[i], sig_union[i]);
    }

  // clear() restores the empty signature (all max values).
  const Array<uint64_t> sig_before_clear = mh_merged.get_signature();
  mh_merged.clear();
  EXPECT_EQ(mh_merged.size(), K);

  const auto & sig_cleared = mh_merged.get_signature();
  ASSERT_EQ(sig_cleared.size(), K);
  bool changed_after_clear = false;
  for (size_t i = 0; i < K; ++i)
    {
      EXPECT_EQ(sig_cleared[i], std::numeric_limits<uint64_t>::max());
      if (sig_cleared[i] != sig_before_clear[i])
        changed_after_clear = true;
    }
  EXPECT_TRUE(changed_after_clear);

  // In current MinHash contract, clear() == fresh empty signature.
  MinHash<int> fresh(K);
  EXPECT_DOUBLE_EQ(mh_merged.similarity(fresh), 1.0);

  // Signature must change again after a new update from the cleared state.
  const Array<uint64_t> sig_before_reupdate = mh_merged.get_signature();
  mh_merged.update(set2.begin(), set2.end());
  EXPECT_EQ(mh_merged.size(), K);

  const auto & sig_reupdated = mh_merged.get_signature();
  bool changed_after_reupdate = false;
  for (size_t i = 0; i < K; ++i)
    if (sig_reupdated[i] != sig_before_reupdate[i])
      {
        changed_after_reupdate = true;
        break;
      }
  EXPECT_TRUE(changed_after_reupdate);
}

TEST(StreamingAlgorithms, MinHashNewMethods)
{
  // update(Itor, Itor) should produce the same signature as repeated update(val).
  MinHash<int> mhLoop(64);
  MinHash<int> mhRange(64);

  DynArray<int> elems;
  for (int i = 0; i < 50; ++i)
    elems.append(i);

  for (int i = 0; i < 50; ++i)
    mhLoop.update(i);
  mhRange.update(elems.begin(), elems.end());

  EXPECT_EQ(mhLoop.similarity(mhRange), 1.0);

  // size() returns k.
  EXPECT_EQ(mhLoop.size(), 64u);
  EXPECT_EQ(mhRange.size(), 64u);

  // get_signature() returns a stable reference with exactly k entries.
  const Array<uint64_t> & sig = mhLoop.get_signature();
  EXPECT_EQ(sig.size(), 64u);

  // clear() resets the signature; similarity to fresh MinHash is 1.0
  // (both have all-max entries, so all positions agree).
  MinHash<int> fresh(64);
  mhLoop.clear();
  EXPECT_EQ(mhLoop.similarity(fresh), 1.0);

  // get_signature() after clear has all entries == max.
  constexpr uint64_t max64 = std::numeric_limits<uint64_t>::max();
  const Array<uint64_t> & sig2 = mhLoop.get_signature();
  for (size_t i = 0; i < sig2.size(); ++i)
    EXPECT_EQ(sig2[i], max64);

  // merge() combines two signatures: union of [0..49] and [25..74]
  // should give same result as building from [0..74] directly.
  MinHash<int> mhA(64), mhB(64), mhUnion(64);
  for (int i =  0; i < 50; ++i) mhA.update(i);
  for (int i = 25; i < 75; ++i) mhB.update(i);
  for (int i =  0; i < 75; ++i) mhUnion.update(i);

  mhA.merge(mhB);
  // Merged should be close to union; similarity should be high.
  EXPECT_GT(mhA.similarity(mhUnion), 0.8);

  // merge() returns *this (chaining works).
  MinHash<int> mhC(64), mhD(64);
  mhC.update(1);
  mhD.update(2);
  MinHash<int> & ref = mhC.merge(mhD);
  EXPECT_EQ(&ref, &mhC);

  // size() is stable after merge and clear.
  EXPECT_EQ(mhA.size(), 64u);
  mhA.clear();
  EXPECT_EQ(mhA.size(), 64u);
}

TEST(StreamingAlgorithms, SimHash)
{
  SimHash<std::string> sh1, sh2, sh3;

  const DynArray<std::string> doc1 = {"this", "is", "a", "test", "document", "with", "some", "words"};
  const DynArray<std::string> doc2 = {"this", "is", "a", "test", "document", "with", "other", "words"}; // one word diff
  const DynArray<std::string> doc3 = {"completely", "unrelated", "content", "nothing", "matches", "here", "at", "all"};

  for (size_t i = 0; i < doc1.size(); ++i) sh1.update(doc1[i]);
  for (size_t i = 0; i < doc2.size(); ++i) sh2.update(doc2[i]);
  for (size_t i = 0; i < doc3.size(); ++i) sh3.update(doc3[i]);

  uint64_t f1 = sh1.get_fingerprint();
  uint64_t f2 = sh2.get_fingerprint();
  uint64_t f3 = sh3.get_fingerprint();

  double sim12 = SimHash<std::string>::similarity(f1, f2);
  double sim13 = SimHash<std::string>::similarity(f1, f3);

  EXPECT_GT(sim12, 0.8);
  EXPECT_LT(sim13, 0.7);
}

TEST(StreamingAlgorithms, EdgeCases)
{
  // 1. Reservoir sample with k=0, k=1 and empty input
  DynList<int> empty_list;
  EXPECT_EQ(reservoir_sample(empty_list.begin(), empty_list.end(), 0).size(), 0u);
  EXPECT_EQ(reservoir_sample(empty_list.begin(), empty_list.end(), 5).size(), 0u);
  
  DynList<int> singleton = {42};
  auto s1 = reservoir_sample(singleton.begin(), singleton.end(), 1, 123);
  ASSERT_EQ(s1.size(), 1u);
  EXPECT_EQ(s1[0], 42);

  // 2. Count-Min Sketch: invalid params and empty/singleton
  EXPECT_THROW(Count_Min_Sketch<int>::from_error_bounds(0, 0.01), std::domain_error);
  
  Count_Min_Sketch<int> cms(100, 5);
  EXPECT_GE(cms.estimate(42), 0u); // empty: 0
  cms.update(42, 10);
  EXPECT_EQ(cms.estimate(42), 10u);
  
  Count_Min_Sketch<int> cms_other(50, 5); // different width
  EXPECT_THROW(cms.merge(cms_other), std::domain_error);

  // 3. HyperLogLog: empty and singleton
  HyperLogLog<int> hll(10);
  EXPECT_NEAR(hll.estimate(), 0.0, 0.1);
  hll.update(42);
  EXPECT_NEAR(hll.estimate(), 1.0, 0.5);

  // 4. MinHash: empty and singletons
  MinHash<int> mh1(64), mh2(64);
  EXPECT_EQ(mh1.similarity(mh2), 1.0); // empty vs empty
  mh1.update(100);
  EXPECT_LT(mh1.similarity(mh2), 0.1); // singleton vs empty
  mh2.update(100);
  EXPECT_EQ(mh1.similarity(mh2), 1.0); // same singleton

  // 5. SimHash: empty and singletons
  SimHash<int> sh1, sh2;
  EXPECT_EQ(sh1.get_fingerprint(), 0u);
  EXPECT_EQ(SimHash<int>::similarity(sh1.get_fingerprint(), sh2.get_fingerprint()), 1.0);
  sh1.update(42);
  sh2.update(42);
  EXPECT_EQ(sh1.get_fingerprint(), sh2.get_fingerprint());
  EXPECT_EQ(SimHash<int>::similarity(sh1.get_fingerprint(), sh2.get_fingerprint()), 1.0);
}

TEST(ReservoirSampler, overflow_size_t)
{
  // Keep internal state consistent: for n_seen_ >= k, the reservoir must be full.
  Reservoir_Sampler<int> s(5, 42);
  for (int i = 0; i < 5; ++i)
    s.update(i);
  s.set_n_seen_for_testing(std::numeric_limits<size_t>::max());
  EXPECT_THROW(s.update(1), std::overflow_error);

  // Iterator overload delegates to update(), so the same guard fires.
  Reservoir_Sampler<int> s2(5, 42);
  for (int i = 0; i < 5; ++i)
    s2.update(i);
  s2.set_n_seen_for_testing(std::numeric_limits<size_t>::max());
  DynArray<int> one = {99};
  EXPECT_THROW(s2.update(one.begin(), one.end()), std::overflow_error);
}

TEST(ReservoirSampler, overflow_rng_range)
{
  // RNG range exhaustion fires when n_seen_ >= rng_range_, but only after
  // the reservoir is already full (n_seen_ >= k).  Seed the reservoir first
  // via legitimate updates, then jump n_seen_ to rng_range_.
  Reservoir_Sampler<int> s(3, 42);
  for (int i = 0; i < 3; ++i)
    s.update(i);                    // fill reservoir: n_seen_ == 3

  const uint64_t rng_range = s.rng_range();
  s.set_n_seen_for_testing(rng_range); // n_seen_ == rng_range_ >= k
  EXPECT_THROW(s.update(99), std::runtime_error);

  // Same path through the iterator overload.
  Reservoir_Sampler<int> s2(3, 42);
  for (int i = 0; i < 3; ++i)
    s2.update(i);
  s2.set_n_seen_for_testing(rng_range);
  DynArray<int> one2 = {99};
  EXPECT_THROW(s2.update(one2.begin(), one2.end()), std::runtime_error);
}

TEST(StreamingAlgorithms, CMSSketchMerge)
{
  Count_Min_Sketch<int> cms1(size_t(100), size_t(5));
  Count_Min_Sketch<int> cms2(size_t(100), size_t(5));

  cms1.update(42, 10);
  cms2.update(42, 5);
  cms2.update(7, 3);

  cms1.merge(cms2);
  EXPECT_GE(cms1.estimate(42), 15u);
  EXPECT_GE(cms1.estimate(7), 3u);
}
