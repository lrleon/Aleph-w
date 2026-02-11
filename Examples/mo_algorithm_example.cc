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
 * @file mo_algorithm_example.cc
 * @brief Four creative scenarios demonstrating Mo's algorithm.
 *
 * SCENARIO 1 — "Biodiversity Survey"
 * ====================================
 * Count distinct species observed by a wildlife camera trap across
 * different time windows.  Uses Distinct_Count_Mo.
 *
 * SCENARIO 2 — "Powerful Array"
 * ===============================
 * Classic competitive programming problem: compute sum(cnt[x]^2 * x)
 * for several ranges.  Uses Powerful_Array_Mo.
 *
 * SCENARIO 3 — "Election Polling"
 * =================================
 * Find the most reported candidate (mode) across polling windows.
 * Uses Range_Mode_Mo.
 *
 * SCENARIO 4 — "Network Packet Analysis" (Custom Policy)
 * ========================================================
 * Sum of squared packet sizes in time windows.  Demonstrates writing
 * a custom MoPolicy from scratch.
 */

# include <tpl_mo_algorithm.H>

# include <cassert>
# include <cstdio>
# include <unordered_map>
# include <unordered_set>

using namespace Aleph;

// ================================================================
// Scenario 1: Biodiversity Survey
// ================================================================

static void biodiversity_survey()
{
  printf("=== SCENARIO 1: Biodiversity Survey ===\n\n");
  printf("A wildlife camera trap records species IDs over 12 hours.\n");
  printf("We want to know how many distinct species appeared in each\n");
  printf("time window.\n\n");

  // Species IDs observed each hour (0-indexed)
  //            h0  h1  h2  h3  h4  h5  h6  h7  h8  h9  h10 h11
  Distinct_Count_Mo<int> mo =
      {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8};

  printf("Hour:    ");
  int data[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8};
  for (int i = 0; i < 12; ++i)
    printf("%3d", data[i]);
  printf("\n\n");

  // Queries: different time windows
  auto ans = mo.solve({
      {0, 3},   // early morning
      {4, 8},   // midday
      {0, 11},  // full day
      {2, 5},   // late morning
      {9, 11}   // evening
  });

  printf("%-20s  %s\n", "Time Window", "Distinct Species");
  printf("%-20s  %s\n", "-------------------", "----------------");
  printf("%-20s  %zu\n", "[h0, h3]  morning", ans(0));
  printf("%-20s  %zu\n", "[h4, h8]  midday", ans(1));
  printf("%-20s  %zu\n", "[h0, h11] full day", ans(2));
  printf("%-20s  %zu\n", "[h2, h5]  late morn", ans(3));
  printf("%-20s  %zu\n", "[h9, h11] evening", ans(4));

  // Verify against brute force
  auto brute = [&](size_t l, size_t r) -> size_t {
    std::unordered_set<int> s;
    for (size_t i = l; i <= r; ++i)
      s.insert(data[i]);
    return s.size();
  };
  assert(ans(0) == brute(0, 3));
  assert(ans(1) == brute(4, 8));
  assert(ans(2) == brute(0, 11));
  assert(ans(3) == brute(2, 5));
  assert(ans(4) == brute(9, 11));

  printf("\nAll assertions passed!\n\n");
}

// ================================================================
// Scenario 2: Powerful Array
// ================================================================

static void powerful_array()
{
  printf("=== SCENARIO 2: Powerful Array ===\n\n");
  printf("Given array a[], for a range [l,r] compute:\n");
  printf("  sum over distinct x in a[l..r] of cnt(x)^2 * x\n\n");

  //                          0  1  2  3  4  5
  Powerful_Array_Mo<int> mo = {1, 2, 1, 1, 2, 3};

  printf("Array: [1, 2, 1, 1, 2, 3]\n\n");

  auto ans = mo.solve({
      {0, 0},  // {1:1} => 1
      {0, 2},  // {1:2, 2:1} => 4*1 + 1*2 = 6
      {0, 5},  // {1:3, 2:2, 3:1} => 9*1 + 4*2 + 1*3 = 20
      {3, 5},  // {1:1, 2:1, 3:1} => 1+2+3 = 6
      {1, 4}   // {2:2, 1:2} => 4*2 + 4*1 = 12
  });

  printf("%-15s  %s\n", "Range", "Power");
  printf("%-15s  %s\n", "-----------", "-----");
  printf("%-15s  %lld\n", "[0,0]", ans(0));
  printf("%-15s  %lld\n", "[0,2]", ans(1));
  printf("%-15s  %lld\n", "[0,5]", ans(2));
  printf("%-15s  %lld\n", "[3,5]", ans(3));
  printf("%-15s  %lld\n", "[1,4]", ans(4));

  assert(ans(0) == 1LL);
  assert(ans(1) == 6LL);
  assert(ans(2) == 20LL);
  assert(ans(3) == 6LL);
  assert(ans(4) == 12LL);

  printf("\nAll assertions passed!\n\n");
}

// ================================================================
// Scenario 3: Election Polling
// ================================================================

static void election_polling()
{
  printf("=== SCENARIO 3: Election Polling ===\n\n");
  printf("Voters report their preferred candidate (1-4) over time.\n");
  printf("Find the most popular candidate in each polling window.\n\n");

  // Candidate IDs reported by voters over time
  //                        v0 v1 v2 v3 v4 v5 v6 v7 v8 v9
  Range_Mode_Mo<int> mo = {2, 3, 2, 1, 2, 3, 3, 1, 4, 3};

  printf("Voter:     ");
  int data[] = {2, 3, 2, 1, 2, 3, 3, 1, 4, 3};
  for (int i = 0; i < 10; ++i)
    printf("%3d", data[i]);
  printf("\n\n");

  auto ans = mo.solve({
      {0, 9},  // all voters
      {0, 4},  // first half
      {5, 9},  // second half
      {2, 6}   // middle window
  });

  printf("%-20s  %-10s %s\n", "Window", "Freq", "Candidate");
  printf("%-20s  %-10s %s\n", "-------------------", "----", "---------");

  const char * labels[] = {"[v0,v9] all", "[v0,v4] first",
                           "[v5,v9] second", "[v2,v6] middle"};
  for (size_t i = 0; i < 4; ++i)
    printf("%-20s  %-10zu %d\n", labels[i], ans(i).first, ans(i).second);

  // Verify frequencies via brute force
  auto brute_freq = [&](size_t l, size_t r) -> size_t {
    std::unordered_map<int, size_t> freq;
    for (size_t i = l; i <= r; ++i)
      ++freq[data[i]];
    size_t mx = 0;
    for (auto & [v, f] : freq)
      mx = std::max(mx, f);
    return mx;
  };
  assert(ans(0).first == brute_freq(0, 9));
  assert(ans(1).first == brute_freq(0, 4));
  assert(ans(2).first == brute_freq(5, 9));
  assert(ans(3).first == brute_freq(2, 6));

  printf("\nAll assertions passed!\n\n");
}

// ================================================================
// Scenario 4: Custom Policy — Network Packet Analysis
// ================================================================

/// Custom Mo policy: sum of squared packet sizes in a window.
struct Packet_Sum_Sq_Policy
{
  using answer_type = long long;

  long long sum_sq = 0;

  void init(const Array<int> &, size_t) { sum_sq = 0; }

  void add(const Array<int> & data, size_t idx)
  {
    long long x = data(idx);
    sum_sq += x * x;
  }

  void remove(const Array<int> & data, size_t idx)
  {
    long long x = data(idx);
    sum_sq -= x * x;
  }

  answer_type answer() const { return sum_sq; }
};

static void network_packet_analysis()
{
  printf("=== SCENARIO 4: Custom Policy — Network Packet Analysis ===\n\n");
  printf("Packet sizes (bytes) captured over 10 time slots.\n");
  printf("Query: sum of squared sizes in each window (for variance\n");
  printf("analysis / anomaly detection).\n\n");

  // Packet sizes (in hundreds of bytes)
  Gen_Mo_Algorithm<int, Packet_Sum_Sq_Policy> mo =
      {15, 8, 22, 3, 17, 11, 9, 25, 6, 14};

  int data[] = {15, 8, 22, 3, 17, 11, 9, 25, 6, 14};

  printf("Slot:   ");
  for (int i = 0; i < 10; ++i)
    printf("%5d", data[i]);
  printf("\n\n");

  auto ans = mo.solve({
      {0, 9},  // all
      {0, 4},  // first half
      {5, 9},  // second half
      {2, 7}   // burst window
  });

  printf("%-20s  %s\n", "Window", "Sum of Squares");
  printf("%-20s  %s\n", "-------------------", "--------------");

  const char * labels[] = {"[0,9]  all", "[0,4]  first",
                           "[5,9]  second", "[2,7]  burst"};
  for (size_t i = 0; i < 4; ++i)
    printf("%-20s  %lld\n", labels[i], ans(i));

  // Verify via brute force
  auto brute_ssq = [&](size_t l, size_t r) -> long long {
    long long s = 0;
    for (size_t i = l; i <= r; ++i)
      s += (long long)data[i] * data[i];
    return s;
  };

  assert(ans(0) == brute_ssq(0, 9));
  assert(ans(1) == brute_ssq(0, 4));
  assert(ans(2) == brute_ssq(5, 9));
  assert(ans(3) == brute_ssq(2, 7));

  printf("\nAll assertions passed!\n\n");
}

// ================================================================
// Main
// ================================================================

int main()
{
  printf("Mo's Algorithm — Offline Range Queries\n");
  printf("======================================\n\n");

  biodiversity_survey();
  powerful_array();
  election_polling();
  network_packet_analysis();

  printf("All scenarios completed successfully.\n");
  return 0;
}
