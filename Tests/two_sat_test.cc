
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

# include <gtest/gtest.h>
# include <random>
# include <limits>

# include <Two_Sat.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>

using namespace Aleph;

// ---------------------------------------------------------------------------
// Helper: verify that an assignment satisfies a list of clauses.
// Each clause is a pair of signed 1-based variables.
// ---------------------------------------------------------------------------
static bool verify_assignment(const Array<bool> & assignment,
                              const Array<std::pair<long, long>> & clauses)
{
  auto lit_val = [&](long v) -> bool {
    return v > 0 ? assignment(static_cast<size_t>(v - 1))
                 : not assignment(static_cast<size_t>(-v - 1));
  };

  for (size_t i = 0; i < clauses.size(); ++i)
    {
      auto [a, b] = clauses(i);
      if (not lit_val(a) and not lit_val(b))
        return false;
    }
  return true;
}

// ---------------------------------------------------------------------------
// Basic tests
// ---------------------------------------------------------------------------

TEST(TwoSat, EmptyFormula)
{
  Two_Sat<> sat(3);

  auto [ok, assignment] = sat.solve();
  EXPECT_TRUE(ok);
  EXPECT_EQ(assignment.size(), 3U);
}

TEST(TwoSat, SingleClause)
{
  // (x0 OR x1)
  Two_Sat<> sat(2);
  sat.add_clause(sat.pos_lit(0), sat.pos_lit(1));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);

  // At least one must be true
  EXPECT_TRUE(assignment(0) or assignment(1));
}

TEST(TwoSat, UnitClauseTrue)
{
  // Force x0 = true
  Two_Sat<> sat(1);
  sat.set_true(sat.pos_lit(0));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(assignment(0));
}

TEST(TwoSat, UnitClauseFalse)
{
  // Force x0 = false
  Two_Sat<> sat(1);
  sat.set_false(sat.pos_lit(0));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_FALSE(assignment(0));
}

TEST(TwoSat, Contradiction)
{
  // x0 AND ~x0 is UNSAT
  Two_Sat<> sat(1);
  sat.set_true(sat.pos_lit(0));
  sat.set_true(sat.neg_lit(0));

  auto [ok, assignment] = sat.solve();
  EXPECT_FALSE(ok);
}

TEST(TwoSat, SimpleImplication)
{
  // x0 -> x1, x0 forced true => x1 must be true
  Two_Sat<> sat(2);
  sat.set_true(sat.pos_lit(0));
  sat.add_implication(sat.pos_lit(0), sat.pos_lit(1));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(assignment(0));
  EXPECT_TRUE(assignment(1));
}

TEST(TwoSat, ImplicationChain)
{
  // x0 -> x1 -> x2 -> x3, x0 = true => all true
  Two_Sat<> sat(4);
  sat.set_true(sat.pos_lit(0));
  sat.add_implication(sat.pos_lit(0), sat.pos_lit(1));
  sat.add_implication(sat.pos_lit(1), sat.pos_lit(2));
  sat.add_implication(sat.pos_lit(2), sat.pos_lit(3));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  for (size_t i = 0; i < 4; ++i)
    EXPECT_TRUE(assignment(i)) << "variable " << i;
}

TEST(TwoSat, ImplicationChainContradiction)
{
  // x0 -> x1 -> x2, x0 = true, x2 = false => UNSAT
  Two_Sat<> sat(3);
  sat.set_true(sat.pos_lit(0));
  sat.add_implication(sat.pos_lit(0), sat.pos_lit(1));
  sat.add_implication(sat.pos_lit(1), sat.pos_lit(2));
  sat.set_false(sat.pos_lit(2));

  EXPECT_FALSE(sat.is_satisfiable());
}

TEST(TwoSat, Equivalence)
{
  // x0 <-> x1, x0 = true => x1 = true
  Two_Sat<> sat(2);
  sat.set_true(sat.pos_lit(0));
  sat.add_equiv(sat.pos_lit(0), sat.pos_lit(1));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(assignment(0));
  EXPECT_TRUE(assignment(1));
}

TEST(TwoSat, Xor)
{
  // x0 XOR x1: exactly one is true
  Two_Sat<> sat(2);
  sat.add_xor(sat.pos_lit(0), sat.pos_lit(1));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_NE(assignment(0), assignment(1));
}

TEST(TwoSat, XorContradiction)
{
  // x0 XOR x1, but also x0 = true, x1 = true => UNSAT
  Two_Sat<> sat(2);
  sat.add_xor(sat.pos_lit(0), sat.pos_lit(1));
  sat.set_true(sat.pos_lit(0));
  sat.set_true(sat.pos_lit(1));

  EXPECT_FALSE(sat.is_satisfiable());
}

// ---------------------------------------------------------------------------
// Signed variable API
// ---------------------------------------------------------------------------

TEST(TwoSat, SignedApi)
{
  // (x0 OR x1) AND (~x0 OR x2) AND (~x1 OR ~x2)
  Two_Sat<> sat(3);
  sat.add_clause_signed(1, 2);
  sat.add_clause_signed(-1, 3);
  sat.add_clause_signed(-2, -3);

  Array<std::pair<long, long>> clauses = {{1, 2}, {-1, 3}, {-2, -3}};

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(verify_assignment(assignment, clauses));
}

TEST(TwoSat, SignedApiUnsat)
{
  // (x0) AND (~x0)
  Two_Sat<> sat(1);
  sat.add_clause_signed(1, 1);
  sat.add_clause_signed(-1, -1);

  EXPECT_FALSE(sat.is_satisfiable());
}

// ---------------------------------------------------------------------------
// At most one
// ---------------------------------------------------------------------------

TEST(TwoSat, AtMostOne)
{
  // At most one of x0, x1, x2 is true.
  Two_Sat<> sat(3);
  Array<size_t> lits = {sat.pos_lit(0), sat.pos_lit(1), sat.pos_lit(2)};
  sat.add_at_most_one(lits);

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);

  int count = 0;
  for (size_t i = 0; i < 3; ++i)
    if (assignment(i))
      ++count;
  EXPECT_LE(count, 1);
}

TEST(TwoSat, AtMostOneWithForced)
{
  // At most one of x0, x1, x2 is true; x0 forced true.
  // => x1 = false, x2 = false
  Two_Sat<> sat(3);
  Array<size_t> lits = {sat.pos_lit(0), sat.pos_lit(1), sat.pos_lit(2)};
  sat.add_at_most_one(lits);
  sat.set_true(sat.pos_lit(0));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(assignment(0));
  EXPECT_FALSE(assignment(1));
  EXPECT_FALSE(assignment(2));
}

TEST(TwoSat, AtMostOneConflict)
{
  // At most one of x0, x1, x2; but x0 and x1 forced true => UNSAT
  Two_Sat<> sat(3);
  Array<size_t> lits = {sat.pos_lit(0), sat.pos_lit(1), sat.pos_lit(2)};
  sat.add_at_most_one(lits);
  sat.set_true(sat.pos_lit(0));
  sat.set_true(sat.pos_lit(1));

  EXPECT_FALSE(sat.is_satisfiable());
}

TEST(TwoSat, AtMostOneEdgeCases)
{
  Two_Sat<> sat(3);

  // Singleton: at most one of {x0} is true. Always SAT.
  Array<size_t> singleton = {sat.pos_lit(0)};
  sat.add_at_most_one(singleton);
  EXPECT_TRUE(sat.is_satisfiable());

  // Duplicate: at most one of {x1, x1} is true. Equivalent to at most one {x1}.
  // Should NOT add (~x1 or ~x1) which would force x1 to be false.
  Two_Sat<> sat2(3);
  Array<size_t> duplicates = {sat2.pos_lit(1), sat2.pos_lit(1)};
  sat2.add_at_most_one(duplicates);
  sat2.set_true(sat2.pos_lit(1));
  EXPECT_TRUE(sat2.is_satisfiable());

  // Out of range validation in add_at_most_one.
  EXPECT_THROW(sat.add_at_most_one({sat.pos_lit(5)}), std::domain_error);
}

// ---------------------------------------------------------------------------
// Classical examples
// ---------------------------------------------------------------------------

TEST(TwoSat, GraphColoring2Colors)
{
  // 2-color a triangle (3 nodes, 3 edges) => UNSAT
  // Variables: x0, x1, x2 (true = color A, false = color B)
  // Edge (i,j): xi XOR xj (different colors)
  Two_Sat<> sat(3);
  sat.add_xor(sat.pos_lit(0), sat.pos_lit(1)); // edge 0-1
  sat.add_xor(sat.pos_lit(1), sat.pos_lit(2)); // edge 1-2
  sat.add_xor(sat.pos_lit(0), sat.pos_lit(2)); // edge 0-2

  EXPECT_FALSE(sat.is_satisfiable());
}

TEST(TwoSat, GraphColoring2ColorsPath)
{
  // 2-color a path 0-1-2-3 => SAT (bipartite)
  Two_Sat<> sat(4);
  sat.add_xor(sat.pos_lit(0), sat.pos_lit(1));
  sat.add_xor(sat.pos_lit(1), sat.pos_lit(2));
  sat.add_xor(sat.pos_lit(2), sat.pos_lit(3));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);

  // Adjacent nodes must differ
  EXPECT_NE(assignment(0), assignment(1));
  EXPECT_NE(assignment(1), assignment(2));
  EXPECT_NE(assignment(2), assignment(3));
}

TEST(TwoSat, AllClausesSame)
{
  // All clauses are (x0 OR x0) => x0 = true
  Two_Sat<> sat(2);
  sat.add_clause(sat.pos_lit(0), sat.pos_lit(0));

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(assignment(0));
}

// ---------------------------------------------------------------------------
// Bounds checking
// ---------------------------------------------------------------------------

TEST(TwoSat, BoundsChecking)
{
  Two_Sat<> sat(3);

  EXPECT_THROW(sat.add_clause(sat.pos_lit(3), sat.pos_lit(0)),
               std::domain_error);
  EXPECT_THROW(sat.add_clause(sat.pos_lit(0), sat.neg_lit(5)),
               std::domain_error);
  EXPECT_THROW(sat.add_clause_signed(0, 1), std::domain_error);
  EXPECT_THROW(sat.add_clause_signed(1, 0), std::domain_error);
  EXPECT_THROW(sat.add_clause_signed(std::numeric_limits<long>::min(), 1),
               std::domain_error);
  EXPECT_THROW(sat.add_clause_signed(1, std::numeric_limits<long>::min()),
               std::domain_error);
}

// ---------------------------------------------------------------------------
// Stress test with brute force verification
// ---------------------------------------------------------------------------

TEST(TwoSat, StressBruteForce)
{
  // Test with small enough n to brute-force all 2^n assignments
  constexpr size_t N = 8;
  constexpr size_t NUM_CLAUSES = 20;
  constexpr size_t NUM_TRIALS = 50;

  std::mt19937 rng(2024);
  std::uniform_int_distribution<size_t> var_dist(0, N - 1);
  std::uniform_int_distribution<int> sign_dist(0, 1);

  for (size_t trial = 0; trial < NUM_TRIALS; ++trial)
    {
      Two_Sat<> sat(N);
      Array<std::pair<long, long>> clauses;

      for (size_t c = 0; c < NUM_CLAUSES; ++c)
        {
          long a = static_cast<long>(var_dist(rng)) + 1;
          long b = static_cast<long>(var_dist(rng)) + 1;
          if (sign_dist(rng)) a = -a;
          if (sign_dist(rng)) b = -b;
          sat.add_clause_signed(a, b);
          clauses.append(std::pair<long, long>(a, b));
        }

      auto [ok, assignment] = sat.solve();

      // Brute-force check
      bool brute_sat = false;
      for (unsigned mask = 0; mask < (1U << N); ++mask)
        {
          Array<bool> bf = Array<bool>::create(N);
          for (size_t i = 0; i < N; ++i)
            bf(i) = ((mask >> i) & 1) != 0;

          if (verify_assignment(bf, clauses))
            {
              brute_sat = true;
              break;
            }
        }

      EXPECT_EQ(ok, brute_sat)
        << "Trial " << trial << ": solver says "
        << (ok ? "SAT" : "UNSAT") << " but brute-force says "
        << (brute_sat ? "SAT" : "UNSAT");

      if (ok)
        EXPECT_TRUE(verify_assignment(assignment, clauses))
          << "Trial " << trial << ": assignment does not satisfy formula";
    }
}

// ---------------------------------------------------------------------------
// Test with different graph types
// ---------------------------------------------------------------------------

TEST(TwoSat, WithListSDigraph)
{
  using DG = List_SDigraph<Graph_Snode<Empty_Class>, Graph_Sarc<Empty_Class>>;
  Two_Sat<DG> sat(3);

  // (x0 OR x1) AND (~x0 OR x2) AND (~x1 OR ~x2)
  sat.add_clause(sat.pos_lit(0), sat.pos_lit(1));
  sat.add_clause(sat.neg_lit(0), sat.pos_lit(2));
  sat.add_clause(sat.neg_lit(1), sat.neg_lit(2));

  Array<std::pair<long, long>> clauses = {{1, 2}, {-1, 3}, {-2, -3}};

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(verify_assignment(assignment, clauses));
}

TEST(TwoSat, WithArrayDigraph)
{
  using DG = Array_Digraph<Graph_Anode<Empty_Class>, Graph_Aarc<Empty_Class>>;
  Two_Sat<DG> sat(3);

  sat.add_clause(sat.pos_lit(0), sat.pos_lit(1));
  sat.add_clause(sat.neg_lit(0), sat.pos_lit(2));
  sat.add_clause(sat.neg_lit(1), sat.neg_lit(2));

  Array<std::pair<long, long>> clauses = {{1, 2}, {-1, 3}, {-2, -3}};

  auto [ok, assignment] = sat.solve();
  ASSERT_TRUE(ok);
  EXPECT_TRUE(verify_assignment(assignment, clauses));
}

// ---------------------------------------------------------------------------
// Larger test
// ---------------------------------------------------------------------------

TEST(TwoSat, LargerSatisfiable)
{
  // Build a satisfiable instance: pick a random assignment and generate
  // clauses that are consistent with it.
  constexpr size_t N = 500;
  constexpr size_t M = 2000;

  std::mt19937 rng(42);
  std::uniform_int_distribution<size_t> var_dist(0, N - 1);
  std::uniform_int_distribution<int> sign_dist(0, 1);

  // Generate a "target" assignment
  Array<bool> target = Array<bool>::create(N);
  for (size_t i = 0; i < N; ++i)
    target(i) = sign_dist(rng) != 0;

  Two_Sat<> sat(N);
  Array<std::pair<size_t, size_t>> clauses;

  for (size_t c = 0; c < M; ++c)
    {
      size_t v1 = var_dist(rng);
      size_t v2 = var_dist(rng);

      // Create a clause satisfied by `target`
      size_t l1 = target(v1) ? sat.pos_lit(v1) : sat.neg_lit(v1);
      size_t l2 = target(v2) ? sat.pos_lit(v2) : sat.neg_lit(v2);
      sat.add_clause(l1, l2);
      clauses.append(std::pair<size_t, size_t>(l1, l2));
    }

  auto [ok, assignment] = sat.solve();
  EXPECT_TRUE(ok);

  for (size_t c = 0; c < clauses.size(); ++c)
    {
      auto [l1, l2] = clauses[c];
      bool val1 = sat.is_pos_lit(l1) ? assignment[sat.lit_var(l1)]
                                     : not assignment[sat.lit_var(l1)];
      bool val2 = sat.is_pos_lit(l2) ? assignment[sat.lit_var(l2)]
                                     : not assignment[sat.lit_var(l2)];
      EXPECT_TRUE(val1 or val2);
    }
}

TEST(TwoSat, LargerUnsatisfiable)
{
  // x0 -> x1 -> x2 -> ... -> x_{n-1} -> ~x0
  // AND x0 = true
  // This forces all variables true, but then ~x0 must be true => contradiction
  constexpr size_t N = 200;
  Two_Sat<> sat(N);
  sat.set_true(sat.pos_lit(0));

  for (size_t i = 0; i + 1 < N; ++i)
    sat.add_implication(sat.pos_lit(i), sat.pos_lit(i + 1));

  // Last variable implies ~x0
  sat.add_implication(sat.pos_lit(N - 1), sat.neg_lit(0));

  EXPECT_FALSE(sat.is_satisfiable());
}

// ---------------------------------------------------------------------------
// Literal helpers
// ---------------------------------------------------------------------------

TEST(TwoSat, LiteralHelpers)
{
  EXPECT_EQ(Two_Sat<>::pos_lit(0), 0U);
  EXPECT_EQ(Two_Sat<>::neg_lit(0), 1U);
  EXPECT_EQ(Two_Sat<>::pos_lit(3), 6U);
  EXPECT_EQ(Two_Sat<>::neg_lit(3), 7U);

  EXPECT_EQ(Two_Sat<>::negate_lit(0), 1U);
  EXPECT_EQ(Two_Sat<>::negate_lit(1), 0U);
  EXPECT_EQ(Two_Sat<>::negate_lit(6), 7U);
  EXPECT_EQ(Two_Sat<>::negate_lit(7), 6U);

  EXPECT_EQ(Two_Sat<>::lit_var(0), 0U);
  EXPECT_EQ(Two_Sat<>::lit_var(1), 0U);
  EXPECT_EQ(Two_Sat<>::lit_var(6), 3U);
  EXPECT_EQ(Two_Sat<>::lit_var(7), 3U);

  EXPECT_TRUE(Two_Sat<>::is_pos_lit(0));
  EXPECT_FALSE(Two_Sat<>::is_pos_lit(1));
  EXPECT_TRUE(Two_Sat<>::is_pos_lit(6));
  EXPECT_FALSE(Two_Sat<>::is_pos_lit(7));

  EXPECT_EQ(Two_Sat<>::signed_to_lit(1), 0U);   // +1 -> pos_lit(0)
  EXPECT_EQ(Two_Sat<>::signed_to_lit(-1), 1U);  // -1 -> neg_lit(0)
  EXPECT_EQ(Two_Sat<>::signed_to_lit(3), 4U);   // +3 -> pos_lit(2)
  EXPECT_EQ(Two_Sat<>::signed_to_lit(-3), 5U);  // -3 -> neg_lit(2)
  EXPECT_THROW(Two_Sat<>::signed_to_lit(0), std::domain_error);
  EXPECT_THROW(Two_Sat<>::signed_to_lit(std::numeric_limits<long>::min()),
               std::domain_error);
}
