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
 * @file sparse_table_example.cc
 * @brief Sparse Table applied to competitive programming and sensor analysis
 *
 * This example demonstrates the three Sparse Table variants through
 * realistic scenarios.
 *
 * SCENARIO 1 — Sensor Monitoring (Sparse_Table — range minimum)
 * ==============================================================
 * An industrial plant has a row of temperature sensors sampled once
 * per second.  After the sampling window closes, operations needs to
 * answer many queries of the form "what was the coldest reading
 * between second l and second r?"  The data is static (the window is
 * already closed), so a Sparse Table gives O(1) per query after an
 * O(n log n) build.
 *
 * SCENARIO 2 — Sports Leaderboard (Max_Sparse_Table — range maximum)
 * ===================================================================
 * A swimming competition records lap times for each swimmer.  The
 * commentator wants to quickly find the best (fastest) lap in any
 * contiguous block of laps.  Since lower time = better, we actually
 * want range minimum on times; but to showcase Max_Sparse_Table we
 * track scores (higher = better) assigned by judges.
 *
 * SCENARIO 3 — GCD Queries (Gen_Sparse_Table — custom idempotent op)
 * ====================================================================
 * A number-theory teaching tool: given a sequence of integers, answer
 * range-GCD queries in O(1).  GCD is idempotent (gcd(a, a) = a), so
 * it fits the Sparse Table perfectly.
 *
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o sparse_table_example sparse_table_example.cc
 * ./sparse_table_example
 */

# include <cstdio>
# include <iostream>
# include <iomanip>
# include <numeric>
# include <cassert>

# include <tpl_sparse_table.H>

using namespace std;
using namespace Aleph;

// =====================================================================
// SCENARIO 1 — Sensor Monitoring (range minimum)
// =====================================================================
//
// A chemical reactor has 20 temperature sensors sampled once.  After
// shutdown the engineer needs:
//   "What was the minimum temperature in sensors 3..10?"
//   "What was the minimum across the entire bank?"
//
static void scenario_sensor_monitoring()
{
  cout << "============================================================\n"
       << " SCENARIO 1: Sensor Monitoring (Sparse_Table — range min)\n"
       << "============================================================\n\n";

  // Temperatures in °C from 20 sensors
  Sparse_Table<double> temps = {
    72.3, 71.8, 73.1, 69.5, 70.2, 68.9, 74.0, 71.1,
    67.5, 70.8, 72.0, 73.5, 66.2, 69.0, 71.4, 75.1,
    68.3, 70.0, 72.7, 69.8
  };

  cout << "Sensor readings (°C):\n\n";
  cout << "  Sensor  Temperature\n"
       << "  ------  -----------\n";

  for (size_t i = 0; i < temps.size(); ++i)
    cout << "  " << setw(6) << i
         << "  " << setw(8) << fixed << setprecision(1)
         << temps.get(i) << " °C\n";

  cout << "\nTable info: " << temps.size() << " elements, "
       << temps.num_levels() << " levels\n";

  // -- Queries --
  struct Query { size_t l; size_t r; const char *label; };
  Query queries[] = {
    {  0,  4, "Sensors 0-4 (left bank)"      },
    {  3, 10, "Sensors 3-10 (center section)" },
    { 10, 19, "Sensors 10-19 (right bank)"    },
    {  0, 19, "All sensors (full bank)"        },
    { 12, 12, "Sensor 12 alone"                },
    {  5,  8, "Sensors 5-8 (hot zone)"         },
  };

  cout << "\nRange minimum queries:\n\n";
  cout << "  Range       Min °C   Description\n"
       << "  ----------  -------  ----------------------------\n";

  for (auto & q : queries)
    {
      double mn = temps.query(q.l, q.r);
      cout << "  [" << setw(2) << q.l << ", " << setw(2) << q.r << "]"
           << "  " << setw(7) << fixed << setprecision(1) << mn
           << "  " << q.label << "\n";
    }

  // Verify: brute-force check for one query
  double brute_min = temps.get(3);
  for (size_t i = 4; i <= 10; ++i)
    brute_min = min(brute_min, temps.get(i));
  double st_min = temps.query(3, 10);
  assert(brute_min == st_min);
  cout << "\n  ✓ Brute-force verification passed for [3, 10]\n";
}

// =====================================================================
// SCENARIO 2 — Sports Leaderboard (range maximum)
// =====================================================================
//
// Judges score 15 gymnastic routines.  The broadcaster wants to
// quickly find the highest score in any contiguous block.
//
static void scenario_sports_leaderboard()
{
  cout << "\n\n============================================================\n"
       << " SCENARIO 2: Sports Leaderboard (Max_Sparse_Table — range max)\n"
       << "============================================================\n\n";

  Max_Sparse_Table<double> scores = {
    9.1, 8.7, 9.4, 8.9, 9.6, 9.0, 8.5, 9.8,
    9.2, 8.8, 9.5, 9.3, 8.6, 9.7, 9.1
  };

  const char *athletes[] = {
    "Simone B.", "Kohei U.",  "Nadia C.",  "Daiki H.",
    "Gabby D.",  "Yul M.",   "Marian D.", "Larisa L.",
    "Nastia L.", "Vitaly S.", "Olga K.",   "Li Ning",
    "Mary Lou",  "Sawao K.", "Nellie K."
  };

  cout << "Routine scores:\n\n";
  cout << "  #   Athlete      Score\n"
       << "  --  -----------  -----\n";

  for (size_t i = 0; i < scores.size(); ++i)
    cout << "  " << setw(2) << i
         << "  " << setw(11) << left << athletes[i] << right
         << "  " << setw(5) << fixed << setprecision(1)
         << scores.get(i) << "\n";

  struct Query { size_t l; size_t r; const char *label; };
  Query queries[] = {
    {  0,  4, "First group (0-4)"     },
    {  5,  9, "Second group (5-9)"    },
    { 10, 14, "Third group (10-14)"   },
    {  0, 14, "Overall best"           },
    {  3,  7, "Mid-competition (3-7)"  },
    {  7,  7, "Single routine (#7)"    },
  };

  cout << "\nRange maximum queries:\n\n";
  cout << "  Range       Max     Description\n"
       << "  ----------  ------  ----------------------\n";

  for (auto & q : queries)
    {
      double mx = scores.query(q.l, q.r);
      cout << "  [" << setw(2) << q.l << ", " << setw(2) << q.r << "]"
           << "  " << setw(5) << fixed << setprecision(1) << mx
           << "   " << q.label << "\n";
    }

  // Verify
  assert(scores.query(0, 14) == 9.8);
  cout << "\n  ✓ Overall best = 9.8 (Larisa L.) — verified\n";
}

// =====================================================================
// SCENARIO 3 — Range GCD (custom idempotent operation)
// =====================================================================
//
// Given an array of positive integers, answer range-GCD queries.
// gcd is idempotent: gcd(a, a) = a.
//
static void scenario_range_gcd()
{
  cout << "\n\n============================================================\n"
       << " SCENARIO 3: Range GCD (Gen_Sparse_Table — custom op)\n"
       << "============================================================\n\n";

  struct Gcd_Op
  {
    int operator()(const int & a, const int & b) const noexcept
    {
      return gcd(a, b);
    }
  };

  Gen_Sparse_Table<int, Gcd_Op> st = {12, 18, 24, 36, 60, 48, 30, 90, 15, 45};

  cout << "Array: ";
  for (size_t i = 0; i < st.size(); ++i)
    cout << (i ? ", " : "") << st.get(i);
  cout << "\n\n";

  cout << "Table info: " << st.size() << " elements, "
       << st.num_levels() << " levels\n\n";

  struct Query { size_t l; size_t r; };
  Query queries[] = {
    {0, 2}, {0, 9}, {3, 5}, {1, 4}, {6, 9}, {4, 4}, {0, 5}, {7, 9},
  };

  cout << "Range GCD queries:\n\n";
  cout << "  Range     GCD   Values\n"
       << "  --------  ----  ------\n";

  for (auto & q : queries)
    {
      int g = st.query(q.l, q.r);

      // Show the sub-array
      cout << "  [" << q.l << ", " << q.r << "]"
           << setw(6 - (q.r > 9 ? 1 : 0) - (q.l > 9 ? 1 : 0)) << ""
           << setw(4) << g << "  {";
      for (size_t i = q.l; i <= q.r; ++i)
        cout << (i > q.l ? ", " : "") << st.get(i);
      cout << "}\n";

      // Brute-force verification
      int brute = st.get(q.l);
      for (size_t i = q.l + 1; i <= q.r; ++i)
        brute = gcd(brute, st.get(i));
      assert(brute == g);
    }

  cout << "\n  ✓ All GCD queries verified against brute-force\n";
}

// =====================================================================
// SCENARIO 4 — Construction from different container types
// =====================================================================
static void scenario_construction()
{
  cout << "\n\n============================================================\n"
       << " SCENARIO 4: Construction from different containers\n"
       << "============================================================\n\n";

  // From Array<T>
  Array<int> arr = {5, 3, 7, 1, 9, 2, 8, 4, 6};
  Sparse_Table<int> st_arr(arr);
  cout << "From Array<int>:   min[0,8] = " << st_arr.query(0, 8) << "\n";

  // From std::vector<T>
  vector<int> vec = {5, 3, 7, 1, 9, 2, 8, 4, 6};
  Sparse_Table<int> st_vec(vec);
  cout << "From vector<int>:  min[0,8] = " << st_vec.query(0, 8) << "\n";

  // From DynList<T>
  DynList<int> dl = {5, 3, 7, 1, 9, 2, 8, 4, 6};
  Sparse_Table<int> st_dl(dl);
  cout << "From DynList<int>: min[0,8] = " << st_dl.query(0, 8) << "\n";

  // From initializer_list
  Sparse_Table<int> st_il = {5, 3, 7, 1, 9, 2, 8, 4, 6};
  cout << "From init-list:    min[0,8] = " << st_il.query(0, 8) << "\n";

  assert(st_arr.query(0, 8) == 1);
  assert(st_vec.query(0, 8) == 1);
  assert(st_dl.query(0, 8) == 1);
  assert(st_il.query(0, 8) == 1);

  // values() reconstruction
  auto vals = st_arr.values();
  cout << "\nReconstructed values: ";
  for (size_t i = 0; i < vals.size(); ++i)
    cout << (i ? ", " : "") << vals(i);
  cout << "\n";

  cout << "\n  ✓ All construction methods produce identical results\n";
}

// =====================================================================

int main()
{
  scenario_sensor_monitoring();
  scenario_sports_leaderboard();
  scenario_range_gcd();
  scenario_construction();

  cout << "\n";
  return 0;
}
