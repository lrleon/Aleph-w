
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
 * @file mo_on_trees_example.cc
 * @brief Three creative scenarios demonstrating Mo's algorithm on trees.
 *
 * SCENARIO 1 — "Rainforest Biodiversity" (List_Graph, subtree queries)
 * =====================================================================
 * A botanist catalogues plant species in a hierarchical forest canopy.
 * Each node represents a canopy section labelled by its dominant species.
 * Subtree queries count how many distinct species appear in each
 * section and all sub-sections beneath it.
 *
 * SCENARIO 2 — "Network Latency Analysis" (List_SGraph, path queries)
 * ====================================================================
 * A tree-shaped data-centre network has routers labelled by latency
 * class.  Path queries between pairs of routers count the number of
 * distinct latency classes along the route.
 *
 * SCENARIO 3 — "Corporate Org Chart" (Array_Graph, path + subtree)
 * =================================================================
 * An org chart tree stores department IDs.  Subtree queries find
 * distinct departments under a VP; path queries count distinct
 * departments between two employees.  Demonstrates Array_Graph.
 */

# include <tpl_mo_on_trees.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>
# include <tpl_dynSetHash.H>

# include <cassert>
# include <cstdio>
# include <vector>

using namespace Aleph;

// ================================================================
// Brute-force helpers for verification
// ================================================================

// Brute-force distinct count over a subtree rooted at subtree_root,
// where tree_root defines the parent/child orientation.
template <class GT>
static size_t brute_subtree_distinct(const GT & g,
                                     typename GT::Node * tree_root,
                                     typename GT::Node * subtree_root)
{
  using Node = typename GT::Node;
  // 1. Compute parent map via DFS from tree_root
  DynMapHash<Node*, Node*> parent;
  parent.insert(tree_root, nullptr);
  std::vector<std::pair<Node*, Node*>> stk;
  stk.push_back({tree_root, nullptr});
  while (!stk.empty())
    {
      auto [cur, par] = stk.back();
      stk.pop_back();
      for (auto it = typename GT::Node_Arc_Iterator(cur);
           it.has_curr(); it.next_ne())
        {
          auto * a = it.get_curr();
          auto * nb = g.get_connected_node(a, cur);
          if (nb != par)
            {
              parent.insert(nb, cur);
              stk.push_back({nb, cur});
            }
        }
    }
  // 2. DFS subtree_root following only children (not parent)
  DynSetHash<typename Node::Node_Type> seen;
  std::vector<Node*> stk2;
  stk2.push_back(subtree_root);
  while (!stk2.empty())
    {
      auto * cur = stk2.back();
      stk2.pop_back();
      seen.insert(cur->get_info());
      for (auto it = typename GT::Node_Arc_Iterator(cur);
           it.has_curr(); it.next_ne())
        {
          auto * a = it.get_curr();
          auto * nb = g.get_connected_node(a, cur);
          if (nb != parent.find(cur))
            stk2.push_back(nb);
        }
    }
  return seen.size();
}

// Brute-force distinct count on the path u→v in a tree
template <class GT>
static size_t brute_path_distinct(const GT & g,
                                  typename GT::Node * root,
                                  typename GT::Node * u,
                                  typename GT::Node * v)
{
  using Node = typename GT::Node;
  // Find path via DFS from root, then extract u→v
  DynMapHash<Node*, Node*> parent;
  parent.insert(root, nullptr);
  std::vector<std::pair<Node*, Node*>> stk;
  stk.push_back({root, nullptr});
  while (!stk.empty())
    {
      auto [cur, par] = stk.back();
      stk.pop_back();
      for (auto it = typename GT::Node_Arc_Iterator(cur);
           it.has_curr(); it.next_ne())
        {
          auto * a = it.get_curr();
          auto * nb = g.get_connected_node(a, cur);
          if (nb != par)
            {
              parent.insert(nb, cur);
              stk.push_back({nb, cur});
            }
        }
    }
  // Collect ancestors of u
  DynSetHash<Node*> ancestors_u;
  for (auto * p = u; p; p = parent.find(p))
    ancestors_u.insert(p);
  // Find LCA
  Node * lca_node = v;
  while (!ancestors_u.has(lca_node))
    lca_node = parent.find(lca_node);
  // Collect values on path u → lca → v
  DynSetHash<typename Node::Node_Type> vals;
  for (auto * p = u; p != lca_node; p = parent.find(p))
    vals.insert(p->get_info());
  for (auto * p = v; p != lca_node; p = parent.find(p))
    vals.insert(p->get_info());
  vals.insert(lca_node->get_info());
  return vals.size();
}

// ================================================================
// Scenario 1: Rainforest Biodiversity (List_Graph, subtree queries)
// ================================================================

static void rainforest_biodiversity()
{
  printf("=== SCENARIO 1: Rainforest Biodiversity (List_Graph) ===\n\n");
  printf("A hierarchical forest canopy: each section stores its\n");
  printf("dominant species ID.  Subtree queries count distinct\n");
  printf("species beneath each section.\n\n");

  /*        Tree structure (species IDs):
   *
   *                  3 (root)
   *                / | \
   *               1   4   1
   *              / \     |
   *             5   9   3
   *            /
   *           2
   */

  using G = List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class>>;
  G g;

  auto * r  = g.insert_node(3);   // root
  auto * a  = g.insert_node(1);
  auto * b  = g.insert_node(4);
  auto * c  = g.insert_node(1);
  auto * d  = g.insert_node(5);
  auto * e  = g.insert_node(9);
  auto * f  = g.insert_node(3);
  auto * h  = g.insert_node(2);

  g.insert_arc(r, a);
  g.insert_arc(r, b);
  g.insert_arc(r, c);
  g.insert_arc(a, d);
  g.insert_arc(a, e);
  g.insert_arc(c, f);
  g.insert_arc(d, h);

  printf("  Nodes: %zu,  Edges: %zu\n\n", g.vsize(), g.esize());

  Distinct_Count_Mo_On_Trees<G> mot(g, r);

  auto ans = mot.subtree_solve({r, a, b, c, d});

  printf("%-25s  %s\n", "Subtree Root (species)", "Distinct species");
  printf("%-25s  %s\n", "------------------------", "----------------");
  printf("%-25s  %zu\n", "root  (3)", ans(0));
  printf("%-25s  %zu\n", "a     (1)", ans(1));
  printf("%-25s  %zu\n", "b     (4)", ans(2));
  printf("%-25s  %zu\n", "c     (1)", ans(3));
  printf("%-25s  %zu\n", "d     (5)", ans(4));

  assert(ans(0) == brute_subtree_distinct(g, r, r));
  assert(ans(1) == brute_subtree_distinct(g, r, a));
  assert(ans(2) == brute_subtree_distinct(g, r, b));
  assert(ans(3) == brute_subtree_distinct(g, r, c));
  assert(ans(4) == brute_subtree_distinct(g, r, d));

  printf("\nAll assertions passed!\n\n");
}

// ================================================================
// Scenario 2: Network Latency Analysis (List_SGraph, path queries)
// ================================================================

static void network_latency()
{
  printf("=== SCENARIO 2: Network Latency Analysis (List_SGraph) ===\n\n");
  printf("A tree-shaped data-centre network.  Each router has a\n");
  printf("latency class (1-5).  Path queries count distinct latency\n");
  printf("classes between pairs of routers.\n\n");

  /*        Network tree (latency class):
   *
   *              2 (R0)
   *            /     \
   *          3(R1)    2(R2)
   *         / \         \
   *       1(R3) 5(R4)   3(R5)
   *       |              |
   *      4(R6)          1(R7)
   */

  using G = List_SGraph<Graph_Snode<int>, Graph_Sarc<Empty_Class>>;
  G g;

  auto * r0 = g.insert_node(2);
  auto * r1 = g.insert_node(3);
  auto * r2 = g.insert_node(2);
  auto * r3 = g.insert_node(1);
  auto * r4 = g.insert_node(5);
  auto * r5 = g.insert_node(3);
  auto * r6 = g.insert_node(4);
  auto * r7 = g.insert_node(1);

  g.insert_arc(r0, r1);
  g.insert_arc(r0, r2);
  g.insert_arc(r1, r3);
  g.insert_arc(r1, r4);
  g.insert_arc(r2, r5);
  g.insert_arc(r3, r6);
  g.insert_arc(r5, r7);

  printf("  Nodes: %zu,  Edges: %zu\n\n", g.vsize(), g.esize());

  Distinct_Count_Mo_On_Trees<G> mot(g, r0);

  auto ans = mot.path_solve({
      {r6, r4},   // R6→R3→R1→R4: classes {4,1,3,5} = 4
      {r6, r7},   // R6→R3→R1→R0→R2→R5→R7: {4,1,3,2,2,3,1} → {1,2,3,4}
      {r0, r0},   // trivial: just R0 → {2} = 1
      {r3, r5},   // R3→R1→R0→R2→R5: {1,3,2,2,3} → {1,2,3}
      {r1, r2}    // R1→R0→R2: {3,2,2} → {2,3}
  });

  printf("%-20s  %s\n", "Path (routers)", "Distinct classes");
  printf("%-20s  %s\n", "-------------------", "----------------");
  printf("%-20s  %zu\n", "R6 → R4", ans(0));
  printf("%-20s  %zu\n", "R6 → R7", ans(1));
  printf("%-20s  %zu\n", "R0 → R0", ans(2));
  printf("%-20s  %zu\n", "R3 → R5", ans(3));
  printf("%-20s  %zu\n", "R1 → R2", ans(4));

  assert(ans(0) == brute_path_distinct(g, r0, r6, r4));
  assert(ans(1) == brute_path_distinct(g, r0, r6, r7));
  assert(ans(2) == brute_path_distinct(g, r0, r0, r0));
  assert(ans(3) == brute_path_distinct(g, r0, r3, r5));
  assert(ans(4) == brute_path_distinct(g, r0, r1, r2));

  printf("\nAll assertions passed!\n\n");
}

// ================================================================
// Scenario 3: Corporate Org Chart (Array_Graph, subtree + path)
// ================================================================

static void org_chart()
{
  printf("=== SCENARIO 3: Corporate Org Chart (Array_Graph) ===\n\n");
  printf("Org chart: each node stores a department ID.\n");
  printf("Subtree queries: distinct departments under a VP.\n");
  printf("Path queries: distinct departments between employees.\n\n");

  /*        Org chart (dept IDs):
   *
   *            10 (CEO)
   *           /    \
   *        20(VP1)  30(VP2)
   *        / \        \
   *     10(D1) 20(D2)  30(D3)
   *     |
   *    40(D4)
   */

  using G = Array_Graph<Graph_Anode<int>, Graph_Aarc<Empty_Class>>;
  G g;

  auto * ceo = g.insert_node(10);
  auto * vp1 = g.insert_node(20);
  auto * vp2 = g.insert_node(30);
  auto * d1  = g.insert_node(10);
  auto * d2  = g.insert_node(20);
  auto * d3  = g.insert_node(30);
  auto * d4  = g.insert_node(40);

  g.insert_arc(ceo, vp1);
  g.insert_arc(ceo, vp2);
  g.insert_arc(vp1, d1);
  g.insert_arc(vp1, d2);
  g.insert_arc(vp2, d3);
  g.insert_arc(d1, d4);

  printf("  Nodes: %zu,  Edges: %zu\n\n", g.vsize(), g.esize());

  Distinct_Count_Mo_On_Trees<G> mot(g, ceo);

  // Subtree queries
  auto sub_ans = mot.subtree_solve({ceo, vp1, vp2});

  printf("--- Subtree queries ---\n");
  printf("%-20s  %s\n", "Root", "Distinct depts");
  printf("%-20s  %s\n", "-------------------", "--------------");
  printf("%-20s  %zu\n", "CEO  (10)", sub_ans(0));
  printf("%-20s  %zu\n", "VP1  (20)", sub_ans(1));
  printf("%-20s  %zu\n", "VP2  (30)", sub_ans(2));

  assert(sub_ans(0) == brute_subtree_distinct(g, ceo, ceo));
  assert(sub_ans(1) == brute_subtree_distinct(g, ceo, vp1));
  assert(sub_ans(2) == brute_subtree_distinct(g, ceo, vp2));

  // Path queries
  auto path_ans = mot.path_solve({
      {d4, d2},    // D4→D1→VP1→D2: {40,10,20,20} → {10,20,40}
      {d4, d3},    // D4→D1→VP1→CEO→VP2→D3: {40,10,20,10,30,30} → {10,20,30,40}
      {vp1, vp2}   // VP1→CEO→VP2: {20,10,30} → {10,20,30}
  });

  printf("\n--- Path queries ---\n");
  printf("%-20s  %s\n", "Path", "Distinct depts");
  printf("%-20s  %s\n", "-------------------", "--------------");
  printf("%-20s  %zu\n", "D4 → D2", path_ans(0));
  printf("%-20s  %zu\n", "D4 → D3", path_ans(1));
  printf("%-20s  %zu\n", "VP1 → VP2", path_ans(2));

  assert(path_ans(0) == brute_path_distinct(g, ceo, d4, d2));
  assert(path_ans(1) == brute_path_distinct(g, ceo, d4, d3));
  assert(path_ans(2) == brute_path_distinct(g, ceo, vp1, vp2));

  printf("\nAll assertions passed!\n\n");
}

// ================================================================
// Scenario 4: File-system inode types (Tree_Node)
// ================================================================

static void filesystem_inodes()
{
  printf("=== SCENARIO 4: File-system Inode Types (Tree_Node) ===\n\n");
  printf("A directory tree where each node stores a file-type ID.\n");
  printf("Subtree queries count distinct file types under a dir.\n");
  printf("Path queries count distinct types between two files.\n\n");

  using TN = Tree_Node<int>;

  //          root(1)
  //         /   |    \                 .
  //      a(2)  b(1)  c(3)
  //     / \          |
  //  d(4) e(2)      f(1)
  //  |
  // g(5)

  auto * root = new TN(1);
  auto * a    = new TN(2);
  auto * b    = new TN(1);
  auto * c    = new TN(3);
  auto * d    = new TN(4);
  auto * e    = new TN(2);
  auto * f    = new TN(1);
  auto * g    = new TN(5);

  root->insert_rightmost_child(a);
  root->insert_rightmost_child(b);
  root->insert_rightmost_child(c);
  a->insert_rightmost_child(d);
  a->insert_rightmost_child(e);
  c->insert_rightmost_child(f);
  d->insert_rightmost_child(g);

  printf("  Nodes: 8\n\n");

  Distinct_Count_Mo_On_Tree_Node<int> mot(root);

  // --- Subtree queries ---
  auto sub_ans = mot.subtree_solve({root, a, c, d});

  printf("--- Subtree queries ---\n");
  printf("Root                  Distinct types\n");
  printf("-------------------   --------------\n");
  printf("root (1)              %zu\n", sub_ans(0));
  printf("a    (2)              %zu\n", sub_ans(1));
  printf("c    (3)              %zu\n", sub_ans(2));
  printf("d    (4)              %zu\n", sub_ans(3));

  // root subtree: {1,2,1,3,4,2,1,5} → distinct = 5
  assert(sub_ans(0) == 5);
  // a subtree: {2,4,2,5} → distinct {2,4,5} = 3
  assert(sub_ans(1) == 3);
  // c subtree: {3,1} → distinct = 2
  assert(sub_ans(2) == 2);
  // d subtree: {4,5} → distinct = 2
  assert(sub_ans(3) == 2);

  // --- Path queries ---
  auto path_ans = mot.path_solve({{g, f}, {e, b}, {d, c}});

  printf("\n--- Path queries ---\n");
  printf("Path                  Distinct types\n");
  printf("-------------------   --------------\n");
  printf("g → f               %zu\n", path_ans(0));
  printf("e → b               %zu\n", path_ans(1));
  printf("d → c               %zu\n", path_ans(2));

  // g→f: g(5)→d(4)→a(2)→root(1)→c(3)→f(1) → {5,4,2,1,3} = 5
  assert(path_ans(0) == 5);
  // e→b: e(2)→a(2)→root(1)→b(1) → {2,1} = 2
  assert(path_ans(1) == 2);
  // d→c: d(4)→a(2)→root(1)→c(3) → {4,2,1,3} = 4
  assert(path_ans(2) == 4);

  printf("\nAll assertions passed!\n\n");

  // Clean up
  destroy_tree(root);
}

// ================================================================
// Main
// ================================================================

int main()
{
  printf("Mo's Algorithm on Trees — Offline Subtree & Path Queries\n");
  printf("========================================================\n\n");

  rainforest_biodiversity();
  network_latency();
  org_chart();
  filesystem_inodes();

  printf("All scenarios completed successfully.\n");
  return 0;
}
