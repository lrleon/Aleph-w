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
 * @file disjoint_sparse_table_example.cc
 * @brief Disjoint Sparse Table applied to real-world scenarios
 *
 * This example demonstrates the Disjoint Sparse Table variants through
 * realistic scenarios, highlighting the key advantage over the classical
 * Sparse Table: support for **non-idempotent** associative operations
 * like addition, multiplication, and XOR.
 *
 * SCENARIO 1 — Sales Analytics (Sum_Disjoint_Sparse_Table — range sum)
 * =====================================================================
 * A retail chain records daily revenue for each store across a quarter.
 * After the quarter ends, the finance team needs to answer many queries
 * of the form "what was the total revenue between day l and day r?"
 * Because sum is associative but NOT idempotent (sum(a, a) = 2a != a),
 * a classical Sparse Table cannot handle this.  The Disjoint Sparse
 * Table answers each query in O(1).
 *
 * SCENARIO 2 — Probability Chains (Product_Disjoint_Sparse_Table)
 * ================================================================
 * A reliability engineer models a series system of n components.  The
 * probability that a subsystem spanning components [l, r] works is the
 * product of individual reliabilities.  Multiplication is associative
 * but not idempotent, making the Disjoint Sparse Table ideal.
 *
 * SCENARIO 3 — XOR Checksums (Gen_Disjoint_Sparse_Table — custom op)
 * ===================================================================
 * A data integrity tool needs range-XOR queries on a block of data.
 * XOR is associative and forms an abelian group.  This scenario
 * illustrates using a custom functor with the generic table.
 *
 * SCENARIO 4 — Parlay Betting (Product_Disjoint_Sparse_Table)
 * =============================================================
 * A sportsbook offers a card of 12 consecutive match-ups.  A bettor
 * can build a "parlay" (accumulator) by picking any contiguous run of
 * matches; the combined payout multiplier is the product of the
 * individual decimal odds.  Product is associative but NOT idempotent
 * (odds * odds = odds² ≠ odds), so a classical Sparse Table cannot
 * handle this, but the Disjoint Sparse Table answers every parlay
 * combination in O(1).
 *
 * SCENARIO 5 — Construction from different containers
 * ====================================================
 * Shows construction from Array, vector, DynList, and initializer_list.
 *
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o disjoint_sparse_table_example \
 *     disjoint_sparse_table_example.cc
 * ./disjoint_sparse_table_example
 */

# include <cstdio>
# include <iostream>
# include <iomanip>
# include <numeric>
# include <cassert>

# include <tpl_disjoint_sparse_table.H>

using namespace std;
using namespace Aleph;

// =====================================================================
// SCENARIO 1 — Sales Analytics (range sum)
// =====================================================================

void scenario_sales_analytics()
{
  cout << "============================================================\n";
  cout << " SCENARIO 1: Sales Analytics (Sum_Disjoint_Sparse_Table)\n";
  cout << "============================================================\n\n";

  // Daily revenue (thousands of dollars) for 15 days
  const vector<int> revenue =
    {120, 95, 140, 88, 175, 63, 210, 155, 102, 180,
     135, 90, 200, 110, 165};

  Sum_Disjoint_Sparse_Table<int> sales(revenue);

  cout << "Daily revenue ($ thousands):\n\n";
  cout << "  Day  Revenue\n";
  cout << "  ---  -------\n";
  for (size_t i = 0; i < revenue.size(); ++i)
    cout << "   " << setw(2) << i << "    $" << setw(3) << revenue[i]
         << "K\n";

  cout << "\nTable info: " << sales.size() << " elements, "
       << sales.num_levels() << " levels\n";

  cout << "\nRange sum queries:\n\n";
  cout << "  Range       Total     Description\n";
  cout << "  ----------  --------  ----------------------------\n";

  struct Query { size_t l, r; const char * desc; };
  Query queries[] = {
    {0,  4, "Week 1 (Mon-Fri)"},
    {5,  9, "Week 2 (Mon-Fri)"},
    {10, 14, "Week 3 (Mon-Fri)"},
    {0, 14, "Entire period"},
    {3,  7, "Mid-period slice"},
    {6,  6, "Day 6 alone"},
  };

  for (const auto & q : queries)
    {
      int total = sales.query(q.l, q.r);
      cout << "  [" << setw(2) << q.l << ", " << setw(2) << q.r << "]"
           << "    $" << setw(4) << total << "K  " << q.desc << "\n";
    }

  // Brute-force verification
  int bf = 0;
  for (size_t i = 0; i <= 14; ++i) bf += revenue[i];
  assert(sales.query(0, 14) == bf);
  cout << "\n  ✓ Brute-force verification passed for full range\n";
  cout << endl;
}

// =====================================================================
// SCENARIO 2 — Probability Chains (range product)
// =====================================================================

void scenario_probability()
{
  cout << "============================================================\n";
  cout << " SCENARIO 2: Reliability Analysis (Product_Disjoint_Sparse_Table)\n";
  cout << "============================================================\n\n";

  // Component reliabilities (probability of working)
  const vector<double> reliability =
    {0.99, 0.95, 0.98, 0.97, 0.93, 0.96, 0.99, 0.94, 0.98, 0.97};

  const vector<string> names =
    {"Power", "CPU", "Memory", "Disk", "Network",
     "Cooling", "PSU", "GPU", "SSD", "Bus"};

  Product_Disjoint_Sparse_Table<double> rel_table(reliability);

  cout << "Component reliabilities:\n\n";
  cout << "  #   Component   Reliability\n";
  cout << "  --  ----------  -----------\n";
  for (size_t i = 0; i < reliability.size(); ++i)
    cout << "  " << setw(2) << i << "  " << setw(10) << left
         << names[i] << right << "  " << fixed << setprecision(4)
         << reliability[i] << "\n";

  cout << "\nSubsystem reliability queries (product of individual):\n\n";
  cout << "  Range       Reliability  Description\n";
  cout << "  ----------  -----------  ----------------------\n";

  struct Query { size_t l, r; const char * desc; };
  Query queries[] = {
    {0,  2, "Core compute (Power-Memory)"},
    {3,  5, "Storage+Net (Disk-Cooling)"},
    {6,  9, "Expansion (PSU-Bus)"},
    {0,  9, "Full system"},
    {1,  4, "CPU through Network"},
    {7,  7, "GPU alone"},
  };

  for (const auto & q : queries)
    {
      double p = rel_table.query(q.l, q.r);
      cout << "  [" << setw(2) << q.l << ", " << setw(2) << q.r << "]"
           << "    " << fixed << setprecision(6) << p
           << "     " << q.desc << "\n";
    }

  // Brute-force verification of full system
  double bf = 1.0;
  for (double r : reliability) bf *= r;
  double computed = rel_table.query(0, 9);
  assert(abs(computed - bf) < 1e-12);
  cout << "\n  ✓ Full system reliability = " << fixed << setprecision(6)
       << computed << " — verified\n";
  cout << endl;
}

// =====================================================================
// SCENARIO 3 — XOR Checksums (custom associative op)
// =====================================================================

struct Xor_Op
{
  constexpr unsigned operator()(unsigned a, unsigned b) const noexcept
  {
    return a ^ b;
  }
};

void scenario_xor_checksum()
{
  cout << "============================================================\n";
  cout << " SCENARIO 3: XOR Checksums (Gen_Disjoint_Sparse_Table)\n";
  cout << "============================================================\n\n";

  const vector<unsigned> data =
    {0xA3, 0x5F, 0x12, 0xB7, 0x8C, 0xE1, 0x3D, 0x74, 0x9A, 0x06};

  Gen_Disjoint_Sparse_Table<unsigned, Xor_Op> xor_tbl(data);

  cout << "Data blocks (hex):\n\n";
  cout << "  Index  Value\n";
  cout << "  -----  -----\n";
  for (size_t i = 0; i < data.size(); ++i)
    cout << "     " << i << "   0x" << hex << uppercase
         << setw(2) << setfill('0') << data[i]
         << setfill(' ') << dec << "\n";

  cout << "\nTable info: " << xor_tbl.size() << " elements, "
       << xor_tbl.num_levels() << " levels\n";

  cout << "\nRange XOR queries:\n\n";
  cout << "  Range     XOR     Values\n";
  cout << "  --------  ------  ------\n";

  struct Query { size_t l, r; };
  Query queries[] = {{0, 2}, {0, 9}, {3, 5}, {1, 4}, {6, 9}, {4, 4}, {0, 5}};

  for (const auto & q : queries)
    {
      unsigned result = xor_tbl.query(q.l, q.r);

      cout << "  [" << q.l << ", " << q.r << "]   0x"
           << hex << uppercase << setw(2) << setfill('0') << result
           << setfill(' ') << dec << "    {";

      for (size_t i = q.l; i <= q.r; ++i)
        {
          if (i > q.l) cout << ", ";
          cout << "0x" << hex << uppercase << setw(2) << setfill('0')
               << data[i] << setfill(' ') << dec;
        }
      cout << "}\n";
    }

  // Brute-force verification
  bool all_ok = true;
  for (const auto & q : queries)
    {
      unsigned bf = 0;
      for (size_t i = q.l; i <= q.r; ++i) bf ^= data[i];
      if (xor_tbl.query(q.l, q.r) != bf)
        all_ok = false;
    }
  assert(all_ok);
  cout << "\n  ✓ All XOR queries verified against brute-force\n";
  cout << endl;
}

// =====================================================================
// SCENARIO 5 — Construction from different containers
// =====================================================================

void scenario_construction()
{
  cout << "============================================================\n";
  cout << " SCENARIO 5: Construction from different containers\n";
  cout << "============================================================\n\n";

  const vector<int> raw = {5, 3, 7, 1, 9, 2, 8, 4, 6};

  // From Array<int>
  Array<int> arr(raw.size());
  for (size_t i = 0; i < raw.size(); ++i)
    arr.append(raw[i]);
  Sum_Disjoint_Sparse_Table<int> from_arr(arr);

  // From std::vector<int>
  Sum_Disjoint_Sparse_Table<int> from_vec(raw);

  // From DynList<int>
  DynList<int> dl;
  for (size_t i = 0; i < raw.size(); ++i)
    dl.append(raw[i]);
  Sum_Disjoint_Sparse_Table<int> from_dl(dl);

  // From initializer_list
  Sum_Disjoint_Sparse_Table<int> from_il = {5, 3, 7, 1, 9, 2, 8, 4, 6};

  int expected = 0;
  for (int v : raw) expected += v;

  cout << "From Array<int>:   sum[0,8] = " << from_arr.query(0, 8) << "\n";
  cout << "From vector<int>:  sum[0,8] = " << from_vec.query(0, 8) << "\n";
  cout << "From DynList<int>: sum[0,8] = " << from_dl.query(0, 8) << "\n";
  cout << "From init-list:    sum[0,8] = " << from_il.query(0, 8) << "\n";

  assert(from_arr.query(0, 8) == expected);
  assert(from_vec.query(0, 8) == expected);
  assert(from_dl.query(0, 8) == expected);
  assert(from_il.query(0, 8) == expected);

  // Reconstruct values
  auto vals = from_vec.values();
  cout << "\nReconstructed values: ";
  for (size_t i = 0; i < vals.size(); ++i)
    {
      if (i > 0) cout << ", ";
      cout << vals(i);
    }
  cout << "\n";

  // Cross-validate a sub-range
  for (size_t l = 0; l < raw.size(); ++l)
    for (size_t r = l; r < raw.size(); ++r)
      assert(from_arr.query(l, r) == from_vec.query(l, r));

  cout << "\n  ✓ All construction methods produce identical results\n";
  cout << endl;
}

// =====================================================================
// SCENARIO 4 — Parlay Betting (range product of odds)
// =====================================================================

void scenario_parlay_betting()
{
  cout << "============================================================\n";
  cout << " SCENARIO 4: Parlay Betting (Product_Disjoint_Sparse_Table)\n";
  cout << "============================================================\n\n";

  // A Saturday match card: 12 events with decimal odds
  // Decimal odds already include the stake: payout = stake × odds
  struct Match { const char * event; double odds; };
  Match card[] = {
    {"Arsenal vs Chelsea",        1.85},  //  0
    {"Real Madrid vs Barcelona",  2.10},  //  1
    {"Bayern vs Dortmund",        1.55},  //  2
    {"PSG vs Lyon",               1.40},  //  3
    {"Juventus vs Inter",         2.25},  //  4
    {"Liverpool vs Man City",     3.10},  //  5
    {"Ajax vs Feyenoord",         1.90},  //  6
    {"Benfica vs Porto",          2.05},  //  7
    {"Milan vs Napoli",           1.75},  //  8
    {"Atletico vs Sevilla",       1.60},  //  9
    {"Tottenham vs Man United",   2.40},  // 10
    {"Celtic vs Rangers",         1.95},  // 11
  };

  const size_t N = sizeof(card) / sizeof(card[0]);

  vector<double> odds(N);
  for (size_t i = 0; i < N; ++i)
    odds[i] = card[i].odds;

  Product_Disjoint_Sparse_Table<double> parlay(odds);

  cout << "Saturday Match Card:\n\n";
  cout << "  #   Match                        Odds\n";
  cout << "  --  ---------------------------  ----\n";
  for (size_t i = 0; i < N; ++i)
    cout << "  " << setw(2) << i << "  " << setw(27) << left
         << card[i].event << right << "  " << fixed << setprecision(2)
         << card[i].odds << "\n";

  cout << "\nTable info: " << parlay.size() << " events, "
       << parlay.num_levels() << " levels\n";

  // Parlay queries: what is the combined multiplier for a range?
  cout << "\nParlay (accumulator) queries — combined payout multiplier:\n\n";
  cout << "  Parlay          Combined   $10 Bet\n";
  cout << "  Range   Legs    Multiplier  Payout\n";
  cout << "  ------  ------  ----------  --------\n";

  struct PQuery { size_t l, r; const char * desc; };
  PQuery queries[] = {
    {0,  1, nullptr},   // double
    {0,  2, nullptr},   // treble
    {0,  4, nullptr},   // 5-fold
    {5,  8, nullptr},   // 4-fold mid-card
    {0, 11, nullptr},   // full card 12-fold
    {3,  3, nullptr},   // single
    {9, 11, nullptr},   // last 3
  };

  for (const auto & q : queries)
    {
      double mult = parlay.query(q.l, q.r);
      size_t legs = q.r - q.l + 1;
      double payout = 10.0 * mult;

      cout << "  [" << setw(2) << q.l << "," << setw(2) << q.r << "]"
           << "  " << setw(2) << legs << "-fold"
           << "  " << setw(10) << fixed << setprecision(2) << mult
           << "  $" << setw(7) << fixed << setprecision(2) << payout
           << "\n";
    }

  // Brute-force verification
  double bf_full = 1.0;
  for (size_t i = 0; i < N; ++i) bf_full *= odds[i];
  double computed_full = parlay.query(0, N - 1);
  assert(abs(computed_full - bf_full) < 1e-6);

  cout << "\n  Full-card 12-fold parlay: $10 bet pays $"
       << fixed << setprecision(2) << 10.0 * computed_full << "\n";

  // Show why classical Sparse Table can't do this:
  // product is NOT idempotent: 2.0 * 2.0 = 4.0 ≠ 2.0
  cout << "\n  Note: product is NOT idempotent (odds × odds ≠ odds),\n"
       << "  so a classical Sparse Table cannot handle parlay queries.\n"
       << "  The Disjoint Sparse Table handles them in O(1).\n";

  cout << "\n  ✓ Full-card parlay verified against brute-force\n";
  cout << endl;
}

// =====================================================================
// SCENARIO 5 — Construction from different containers
// =====================================================================

int main()
{
  scenario_sales_analytics();
  scenario_probability();
  scenario_xor_checksum();
  scenario_parlay_betting();
  scenario_construction();

  return 0;
}
