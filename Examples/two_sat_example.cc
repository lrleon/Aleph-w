
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
 * @file two_sat_example.cc
 * @brief Examples of the Two_Sat solver.
 *
 * Demonstrates four applications of 2-SAT:
 *
 * 1. **Basic formula** — solving a simple conjunction of 2-literal clauses.
 * 2. **Graph 2-coloring** — deciding if a graph is 2-colorable.
 * 3. **Team assignment** — assigning people to teams with constraints.
 * 4. **Using different graph backends** — List_SDigraph and Array_Digraph.
 *
 * ## What is 2-SAT?
 *
 * 2-SAT is the problem of determining whether a Boolean formula in
 * conjunctive normal form (CNF), where each clause has exactly two
 * literals, is satisfiable.
 *
 * Example formula: (x0 OR x1) AND (~x0 OR x2) AND (~x1 OR ~x2)
 *
 * ## Algorithm
 *
 * The solver builds an **implication graph** and uses Tarjan's
 * SCC algorithm.  Each clause (a OR b) produces two implications:
 * ~a -> b and ~b -> a.  The formula is unsatisfiable iff some
 * variable and its negation lie in the same SCC.
 *
 * - **Time**: O(n + m) where n = variables, m = clauses
 * - **Space**: O(n + m)
 *
 * @see Two_Sat.H
 * @see Tarjan.H
 * @author Leandro Rabindranath Leon
 * @ingroup Examples
 */

# include <iostream>
# include <string>

# include <Two_Sat.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>

using namespace std;
using namespace Aleph;

// =========================================================================
// Example 1: Basic Boolean formula
// =========================================================================

void example_basic_formula()
{
  cout << "==========================================================\n";
  cout << "Example 1: Basic Boolean Formula\n";
  cout << "==========================================================\n\n";

  // Formula: (x0 OR x1) AND (~x0 OR x2) AND (~x1 OR ~x2) AND (x0 OR x2)
  //
  // We use the literal-index API:
  //   pos_lit(i) = positive literal for variable i
  //   neg_lit(i) = negative literal for variable i

  Two_Sat<> sat(3);

  sat.add_clause(sat.pos_lit(0), sat.pos_lit(1));   // x0 v x1
  sat.add_clause(sat.neg_lit(0), sat.pos_lit(2));   // ~x0 v x2
  sat.add_clause(sat.neg_lit(1), sat.neg_lit(2));   // ~x1 v ~x2
  sat.add_clause(sat.pos_lit(0), sat.pos_lit(2));   // x0 v x2

  cout << "Formula:\n"
       << "  (x0 OR x1) AND (~x0 OR x2) AND (~x1 OR ~x2) AND (x0 OR x2)\n\n";

  auto [ok, assignment] = sat.solve();

  if (ok)
    {
      cout << "SATISFIABLE.  Assignment:\n";
      for (size_t i = 0; i < assignment.size(); ++i)
        cout << "  x" << i << " = " << (assignment(i) ? "true" : "false")
             << "\n";
    }
  else
    cout << "UNSATISFIABLE\n";

  cout << "\n";
}

// =========================================================================
// Example 2: Same formula but using the signed 1-based API
// =========================================================================

void example_signed_api()
{
  cout << "==========================================================\n";
  cout << "Example 2: Signed 1-based Variable API\n";
  cout << "==========================================================\n\n";

  // Same formula: (x0 OR x1) AND (~x0 OR x2) AND (~x1 OR ~x2) AND (x0 OR x2)
  //
  // With signed API: variable k (1-based) is +k, its negation is -k.
  // So x0 -> +1, x1 -> +2, x2 -> +3, ~x0 -> -1, etc.

  Two_Sat<> sat(3);

  sat.add_clause_signed(1, 2);     // x0 v x1
  sat.add_clause_signed(-1, 3);    // ~x0 v x2
  sat.add_clause_signed(-2, -3);   // ~x1 v ~x2
  sat.add_clause_signed(1, 3);     // x0 v x2

  cout << "Formula (same as Example 1, using signed API):\n"
       << "  (+1 OR +2) AND (-1 OR +3) AND (-2 OR -3) AND (+1 OR +3)\n\n";

  auto [ok, assignment] = sat.solve();

  if (ok)
    {
      cout << "SATISFIABLE.  Assignment:\n";
      for (size_t i = 0; i < assignment.size(); ++i)
        cout << "  x" << i << " = " << (assignment(i) ? "true" : "false")
             << "\n";
    }
  else
    cout << "UNSATISFIABLE\n";

  cout << "\n";
}

// =========================================================================
// Example 3: Graph 2-coloring
// =========================================================================

void example_2_coloring()
{
  cout << "==========================================================\n";
  cout << "Example 3: Graph 2-Coloring via 2-SAT\n";
  cout << "==========================================================\n\n";

  // A graph is 2-colorable iff it is bipartite.
  //
  // For each edge (u, v), we require xu XOR xv (different colors).
  // xu XOR xv is encoded as two clauses: (xu OR xv) AND (~xu OR ~xv).
  //
  // A) Path graph: 0-1-2-3 => bipartite => SAT
  {
    cout << "Path graph 0 - 1 - 2 - 3:\n";
    Two_Sat<> sat(4);
    sat.add_xor(sat.pos_lit(0), sat.pos_lit(1));  // edge 0-1
    sat.add_xor(sat.pos_lit(1), sat.pos_lit(2));  // edge 1-2
    sat.add_xor(sat.pos_lit(2), sat.pos_lit(3));  // edge 2-3

    auto [ok, assignment] = sat.solve();
    cout << "  2-colorable? " << (ok ? "YES" : "NO") << "\n";
    if (ok)
      {
        cout << "  Coloring:";
        for (size_t i = 0; i < 4; ++i)
          cout << " node" << i << "=" << (assignment(i) ? "A" : "B");
        cout << "\n";
      }
    cout << "\n";
  }

  // B) Triangle graph: 0-1-2-0 => odd cycle => NOT bipartite => UNSAT
  {
    cout << "Triangle 0 - 1 - 2 - 0:\n";
    Two_Sat<> sat(3);
    sat.add_xor(sat.pos_lit(0), sat.pos_lit(1));
    sat.add_xor(sat.pos_lit(1), sat.pos_lit(2));
    sat.add_xor(sat.pos_lit(0), sat.pos_lit(2));

    auto [ok, _] = sat.solve();
    cout << "  2-colorable? " << (ok ? "YES" : "NO") << "\n\n";
  }

  // C) Cycle of length 4: 0-1-2-3-0 => even cycle => bipartite
  {
    cout << "Square 0 - 1 - 2 - 3 - 0:\n";
    Two_Sat<> sat(4);
    sat.add_xor(sat.pos_lit(0), sat.pos_lit(1));
    sat.add_xor(sat.pos_lit(1), sat.pos_lit(2));
    sat.add_xor(sat.pos_lit(2), sat.pos_lit(3));
    sat.add_xor(sat.pos_lit(3), sat.pos_lit(0));

    auto [ok, assignment] = sat.solve();
    cout << "  2-colorable? " << (ok ? "YES" : "NO") << "\n";
    if (ok)
      {
        cout << "  Coloring:";
        for (size_t i = 0; i < 4; ++i)
          cout << " node" << i << "=" << (assignment(i) ? "A" : "B");
        cout << "\n";
      }
    cout << "\n";
  }
}

// =========================================================================
// Example 4: Team assignment with constraints
// =========================================================================

void example_team_assignment()
{
  cout << "==========================================================\n";
  cout << "Example 4: Team Assignment with Constraints\n";
  cout << "==========================================================\n\n";

  // 5 people must be split into two teams (A = true, B = false).
  //
  // Constraints:
  //   - Alice(0) and Bob(1) must be on the same team       => equivalence
  //   - Charlie(2) and Diana(3) cannot both be on team A   => at-most-one
  //   - Eve(4) must be on team A                           => unit
  //   - Eve(4) and Bob(1) must be on different teams       => XOR

  const char * names[] = {"Alice", "Bob", "Charlie", "Diana", "Eve"};

  Two_Sat<> sat(5);

  sat.add_equiv(sat.pos_lit(0), sat.pos_lit(1));             // Alice <-> Bob
  sat.add_clause(sat.neg_lit(2), sat.neg_lit(3));            // ~Charlie v ~Diana
  sat.set_true(sat.pos_lit(4));                              // Eve = true (team A)
  sat.add_xor(sat.pos_lit(4), sat.pos_lit(1));               // Eve XOR Bob

  cout << "Constraints:\n"
       << "  Alice and Bob:     same team\n"
       << "  Charlie and Diana: not both on team A\n"
       << "  Eve:               must be on team A\n"
       << "  Eve and Bob:       different teams\n\n";

  auto [ok, assignment] = sat.solve();

  if (ok)
    {
      cout << "SATISFIABLE.  Assignment:\n";
      for (size_t i = 0; i < 5; ++i)
        cout << "  " << names[i] << " -> team "
             << (assignment(i) ? "A" : "B") << "\n";
    }
  else
    cout << "UNSATISFIABLE: constraints cannot be met\n";

  cout << "\n";
}

// =========================================================================
// Example 5: Different graph backends
// =========================================================================

void example_graph_backends()
{
  cout << "==========================================================\n";
  cout << "Example 5: Different Graph Backends\n";
  cout << "==========================================================\n\n";

  // The same formula solved with three graph backends.
  // Formula: (x0 OR x1) AND (~x0 OR ~x1) (exactly one is true)

  auto run = [](const string & name, auto & sat) {
    sat.add_xor(sat.pos_lit(0), sat.pos_lit(1));

    auto [ok, assignment] = sat.solve();
    cout << "  " << name << ": ";
    if (ok)
      cout << "x0=" << assignment(0) << " x1=" << assignment(1) << "\n";
    else
      cout << "UNSAT\n";
  };

  // 1) Default: List_Digraph (tpl_graph.H)
  {
    Two_Sat<> sat(2);
    run("List_Digraph ", sat);
  }

  // 2) List_SDigraph (tpl_sgraph.H) — singly-linked lists
  {
    using DG = List_SDigraph<Graph_Snode<Empty_Class>,
                             Graph_Sarc<Empty_Class>>;
    Two_Sat<DG> sat(2);
    run("List_SDigraph", sat);
  }

  // 3) Array_Digraph (tpl_agraph.H) — array-based adjacency
  {
    using DG = Array_Digraph<Graph_Anode<Empty_Class>,
                             Graph_Aarc<Empty_Class>>;
    Two_Sat<DG> sat(2);
    run("Array_Digraph", sat);
  }

  cout << "\n";
}

// =========================================================================

int main()
{
  example_basic_formula();
  example_signed_api();
  example_2_coloring();
  example_team_assignment();
  example_graph_backends();

  return 0;
}