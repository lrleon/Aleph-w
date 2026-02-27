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
# include <vector>
# include <set>
# include <string>
# include <random>

# include <reservoir-sampling.H>
# include <count-min-sketch.H>
# include <hyperloglog.H>
# include <minhash.H>
# include <simhash.H>

using namespace Aleph;

TEST(StreamingAlgorithms, ReservoirSampling)
{
  std::vector<int> data;
  for (int i = 0; i < 1000; ++i) data.push_back(i);

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
  std::set<int> unique_vals;
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
  EXPECT_LT(cms.estimate("cherry"), 10u); // should be reasonably close
  
  EXPECT_EQ(cms.estimate("dragonfruit"), 0u);
}

TEST(StreamingAlgorithms, HyperLogLog)
{
  HyperLogLog<int> hll(10); // 1024 registers, ~3.2% error
  
  const int n = 5000;
  for (int i = 0; i < n; ++i)
    hll.update(i % 1000); // 1000 unique elements

  double est = hll.estimate();
  EXPECT_NEAR(est, 1000.0, 1000.0 * 0.05); // allow 5% error
}

TEST(StreamingAlgorithms, MinHash)
{
  MinHash<int> mh1(256);
  MinHash<int> mh2(256);
  MinHash<int> mh3(256);

  // Set 1: [0..99]
  // Set 2: [50..149] -> Intersection 50, Union 150 -> Jaccard = 1/3
  // Set 3: [200..299] -> Jaccard with 1 = 0
  
  for (int i = 0; i < 100; ++i) mh1.update(i);
  for (int i = 50; i < 150; ++i) mh2.update(i);
  for (int i = 200; i < 300; ++i) mh3.update(i);

  double sim12 = mh1.similarity(mh2);
  double sim13 = mh1.similarity(mh3);

  EXPECT_NEAR(sim12, 1.0/3.0, 0.1);
  EXPECT_NEAR(sim13, 0.0, 0.05);
}

TEST(StreamingAlgorithms, SimHash)
{
  SimHash<std::string> sh1, sh2, sh3;

  const std::vector<std::string> doc1 = {"this", "is", "a", "test", "document", "with", "some", "words"};
  const std::vector<std::string> doc2 = {"this", "is", "a", "test", "document", "with", "other", "words"}; // one word diff
  const std::vector<std::string> doc3 = {"completely", "unrelated", "content", "nothing", "matches", "here", "at", "all"};

  for (const auto & w : doc1) sh1.update(w);
  for (const auto & w : doc2) sh2.update(w);
  for (const auto & w : doc3) sh3.update(w);

  uint64_t f1 = sh1.get_fingerprint();
  uint64_t f2 = sh2.get_fingerprint();
  uint64_t f3 = sh3.get_fingerprint();

  double sim12 = SimHash<std::string>::similarity(f1, f2);
  double sim13 = SimHash<std::string>::similarity(f1, f3);

  EXPECT_GT(sim12, 0.8);
  EXPECT_LT(sim13, 0.7);
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
