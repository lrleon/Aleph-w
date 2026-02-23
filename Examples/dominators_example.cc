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
 * @file dominators_example.cc
 * @brief Demonstrates dominator tree and dominance frontier computation
 *        for SSA construction in a compiler pipeline.
 *
 * Builds a control-flow graph (CFG) with if-else and loop structures,
 * computes the dominator tree using the Lengauer-Tarjan algorithm, and
 * shows where phi-functions would be placed for SSA form.
 */

#include <iostream>
#include <string>
#include <Dominators.H>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

using CFG = List_Digraph<Graph_Node<string>, Graph_Arc<int>>;

/**
 * @brief Demonstrates construction of a control-flow graph and performs dominance analyses.
 *
 * Builds a sample CFG with if-else and loop structure, computes immediate dominators
 * and post-dominators using the Lengauer–Tarjan algorithms, constructs dominator and
 * post-dominator trees, computes dominance and post-dominance frontiers (for SSA
 * phi-placement and CDG construction), reports blocks that need phi-functions, and
 * runs example dominance/post-dominance queries while printing results to stdout.
 *
 * @return int Exit status: 0 on success.
 */
int main()
{
  // ========================================================================
  // 1. Build a control-flow graph
  // ========================================================================
  //
  //  entry
  //    |
  //  cond
  //  |- then
  //  `- else
  //  join
  //    |
  //  loop_header <----+
  //    |      |       |
  //  loop_body |      |
  //    |      |       |
  //    +------+-------+
  //    |
  //   exit

  CFG cfg;

  auto entry       = cfg.insert_node("entry");
  auto cond        = cfg.insert_node("cond");
  auto then_block  = cfg.insert_node("then");
  auto else_block  = cfg.insert_node("else");
  auto join        = cfg.insert_node("join");
  auto loop_header = cfg.insert_node("loop_hdr");
  auto loop_body   = cfg.insert_node("loop_body");
  auto exit_block  = cfg.insert_node("exit");

  cfg.insert_arc(entry, cond);
  cfg.insert_arc(cond, then_block);
  cfg.insert_arc(cond, else_block);
  cfg.insert_arc(then_block, join);
  cfg.insert_arc(else_block, join);
  cfg.insert_arc(join, loop_header);
  cfg.insert_arc(loop_header, loop_body);
  cfg.insert_arc(loop_header, exit_block);
  cfg.insert_arc(loop_body, loop_header);  // back-edge

  cout << "=== Control-Flow Graph ===" << endl;
  cout << "Nodes: " << cfg.get_num_nodes() << endl;
  cout << "Arcs:  " << cfg.get_num_arcs() << endl;
  cout << endl;
  cout << "Edges:" << endl;
  for (auto it = cfg.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      cout << "  " << cfg.get_src_node(arc)->get_info()
           << " -> " << cfg.get_tgt_node(arc)->get_info() << endl;
    }
  cout << endl;

  // ========================================================================
  // 2. Compute immediate dominators
  // ========================================================================

  Lengauer_Tarjan_Dominators<CFG> dom;
  auto idoms = dom.compute_idom(cfg, entry);

  cout << "=== Immediate Dominators ===" << endl;
  for (auto it = idoms.get_it(); it.has_curr(); it.next_ne())
    {
      auto [node, idom] = it.get_curr();
      cout << "  idom(" << node->get_info() << ") = "
           << (idom ? idom->get_info() : "(root)") << endl;
    }
  cout << endl;

  // ========================================================================
  // 3. Build and display dominator tree
  // ========================================================================

  CFG dom_tree;
  dom.build_tree(cfg, entry, dom_tree);

  cout << "=== Dominator Tree ===" << endl;
  cout << "Nodes: " << dom_tree.get_num_nodes() << endl;
  cout << "Arcs:  " << dom_tree.get_num_arcs() << endl;
  cout << endl;
  cout << "Tree edges (parent -> child):" << endl;
  for (auto it = dom_tree.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      cout << "  " << dom_tree.get_src_node(arc)->get_info()
           << " -> " << dom_tree.get_tgt_node(arc)->get_info() << endl;
    }
  cout << endl;

  // ========================================================================
  // 4. Compute dominance frontiers (for SSA phi-function placement)
  // ========================================================================

  auto df = dom.dominance_frontiers(cfg, entry);
  const auto print_frontier = [](auto * node, const auto & frontier)
  {
    (void) node; // kept for symmetry with call sites
    cout << "{";
    bool first = true;
    for (auto fi = frontier.get_it(); fi.has_curr(); fi.next_ne())
      {
        if (not first)
          cout << ", ";
        cout << fi.get_curr()->get_info();
        first = false;
      }
    cout << "}";
  };

  cout << "=== Dominance Frontiers ===" << endl;
  for (auto it = df.get_it(); it.has_curr(); it.next_ne())
    {
      auto & [node, frontier] = it.get_curr();
      cout << "  DF(" << node->get_info() << ") = ";
      print_frontier(node, frontier);
      cout << endl;
    }
  cout << endl;

  // ========================================================================
  // 5. Identify phi-function placement
  // ========================================================================

  cout << "=== SSA Phi-Function Placement ===" << endl;
  cout << "Blocks where phi-functions would be needed:" << endl;

  // In SSA, phi-functions are placed at dominance frontier nodes.
  // A variable defined in block B needs a phi at each node in DF(B).
  for (auto it = df.get_it(); it.has_curr(); it.next_ne())
    {
      auto & [node, frontier] = it.get_curr();
      if (not frontier.is_empty())
        {
          cout << "  Definitions in '" << node->get_info()
               << "' need phi at: ";
          print_frontier(node, frontier);
          cout << endl;
        }
    }
  cout << endl;

  // ========================================================================
  // 6. Dominance queries
  // ========================================================================

  cout << "=== Dominance Queries ===" << endl;
  cout << "  entry dominates exit?  "
       << (dom.dominates(cfg, entry, entry, exit_block) ? "yes" : "no") << endl;
  cout << "  cond dominates join?   "
       << (dom.dominates(cfg, entry, cond, join) ? "yes" : "no") << endl;
  cout << "  then dominates join?   "
       << (dom.dominates(cfg, entry, then_block, join) ? "yes" : "no") << endl;
  cout << "  loop_hdr dominates loop_body? "
       << (dom.dominates(cfg, entry, loop_header, loop_body) ? "yes" : "no") << endl;
  cout << endl;

  // ========================================================================
  // 7. Compute post-dominators (from exit node)
  // ========================================================================

  Lengauer_Tarjan_Post_Dominators<CFG> pdom;
  auto ipdoms = pdom.compute_ipdom(cfg, exit_block);

  cout << "=== Immediate Post-Dominators (exit = " << exit_block->get_info()
       << ") ===" << endl;
  for (auto pit = ipdoms.get_it(); pit.has_curr(); pit.next_ne())
    {
      auto [node, ipdom] = pit.get_curr();
      cout << "  ipdom(" << node->get_info() << ") = "
           << (ipdom ? ipdom->get_info() : "(exit)") << endl;
    }
  cout << endl;

  // ========================================================================
  // 8. Build and display post-dominator tree
  // ========================================================================

  CFG pdom_tree;
  pdom.build_tree(cfg, exit_block, pdom_tree);

  cout << "=== Post-Dominator Tree ===" << endl;
  cout << "Nodes: " << pdom_tree.get_num_nodes() << endl;
  cout << "Arcs:  " << pdom_tree.get_num_arcs() << endl;
  cout << endl;
  cout << "Tree edges (parent -> child):" << endl;
  for (auto pit = pdom_tree.get_arc_it(); pit.has_curr(); pit.next_ne())
    {
      auto arc = pit.get_curr();
      cout << "  " << pdom_tree.get_src_node(arc)->get_info()
           << " -> " << pdom_tree.get_tgt_node(arc)->get_info() << endl;
    }
  cout << endl;

  // ========================================================================
  // 9. Compute post-dominance frontiers (for CDG construction)
  // ========================================================================

  auto pdf = pdom.post_dominance_frontiers(cfg, exit_block);

  cout << "=== Post-Dominance Frontiers ===" << endl;
  for (auto pit = pdf.get_it(); pit.has_curr(); pit.next_ne())
    {
      auto & [node, frontier] = pit.get_curr();
      cout << "  PDF(" << node->get_info() << ") = {";
      bool first = true;
      for (auto fi = frontier.get_it(); fi.has_curr(); fi.next_ne())
        {
          if (not first)
            cout << ", ";
          cout << fi.get_curr()->get_info();
          first = false;
        }
      cout << "}" << endl;
    }
  cout << endl;

  // ========================================================================
  // 10. Post-dominance queries
  // ========================================================================

  cout << "=== Post-Dominance Queries ===" << endl;
  cout << "  exit post-dominates entry?    "
       << (pdom.post_dominates(cfg, exit_block, exit_block, entry) ? "yes" : "no")
       << endl;
  cout << "  loop_hdr post-dominates cond? "
       << (pdom.post_dominates(cfg, exit_block, loop_header, cond) ? "yes" : "no")
       << endl;
  cout << "  then post-dominates entry?    "
       << (pdom.post_dominates(cfg, exit_block, then_block, entry) ? "yes" : "no")
       << endl;

  return 0;
}
