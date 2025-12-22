/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
# ifndef KARGER_H
# define KARGER_H

# include <cmath>
# include <limits>
# include <htlist.H>
# include <tpl_sgraph.H>
# include <tpl_dynSetTree.H>
# include <generate_graph.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{
  /**
      Karger's randomized minimum cut algorithm.

      This class implements Karger's algorithm for finding a minimum
      cut in an undirected graph. A minimum cut is a partition of the
      graph's vertices into two non-empty sets such that the number
      of edges crossing the partition is minimized.

      The algorithm works by repeatedly contracting randomly selected
      edges until only two "super-nodes" remain. The edges between
      these super-nodes form a cut. By running multiple iterations,
      the algorithm finds the minimum cut with high probability.

      Template parameters:
      - GT: Graph type (must be based on List_Graph with node/arc types).
      - SA: Arc filter that determines which arcs to consider. Must provide
            `bool operator()(Arc*)` returning true if the arc should be
            included. Defaults to `Dft_Show_Arc<GT>` which includes all arcs.

      Time complexity:
      - Standard version: O(n^2 * m) per iteration, O(n^4) for n^2 iterations
      - Fast version (Karger-Stein): O(n^2 log^3 n)

      @note This algorithm is designed for undirected graphs.
      @note The fast version is incomplete and returns approximate results
            for small graphs (n <= 6).

      @see https://en.wikipedia.org/wiki/Karger%27s_algorithm
      @ingroup Grafos
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Karger_Min_Cut
  {
    // Each node contains the list of contracted nodes
    using Knode = Graph_Node<DynList<typename GT::Node *>>;
    using Karc = Graph_Arc<typename GT::Arc *>;
    using Kgraph = List_Graph<Knode, Karc>;

    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    unsigned long seed;
    gsl_rng *r;
    SA sa;

    void build_kgraph(GT & g, Kgraph & kg, DynSetTreapRk<Karc *> & arcs)
    {
      clear_graph(kg);
      arcs.empty();
      g.reset_nodes();
      g.reset_arcs();

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          auto q = kg.insert_node();
          q->get_info().append(p);
          g.map_nodes(p, q);
        }

      // Use arc filter SA to determine which arcs to include
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          auto s = mapped_node<GT, Kgraph>(g.get_src_node(a));
          auto t = mapped_node<GT, Kgraph>(g.get_tgt_node(a));
          auto ka = kg.insert_arc(s, t, a);
          arcs.insert(ka);
        }
    }

    void update_arcs(Kgraph & kg, Knode *p, Knode *t, Knode *cp,
                     DynSetTreapRk<Karc *> & arcs)
    {
      for (typename Kgraph::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
        {
          auto pa = it.get_curr();
          auto tgt = it.get_tgt_node_ne();

          arcs.remove(pa); // remove from index; removed from graph when nodes deleted
          if (tgt == t)
            continue; // parallel arc ==> ignore

          auto ka = kg.insert_arc(cp, tgt, pa->get_info());
          arcs.insert(ka);
        }
    }

    void contract(Kgraph & kg, const unsigned long & left_num_nodes,
                  DynSetTreapRk<Karc *> & arcs)
    {
      while (kg.get_num_nodes() > left_num_nodes)
        { // randomly select an arc from kg
          auto num_arc = gsl_rng_uniform_int(r, kg.get_num_arcs());
          auto a = arcs.select(num_arc); // arc to remove
          auto s = kg.get_src_node(a); // nodes to "contract"
          auto t = kg.get_tgt_node(a);

          arcs.remove(a); // remove from kg and index
          kg.remove_arc(a);

          auto cp = kg.insert_node(); // new contracted node representing s-t

          update_arcs(kg, s, t, cp, arcs);
          update_arcs(kg, t, s, cp, arcs);

          cp->get_info().swap(s->get_info());
          cp->get_info().append(t->get_info());

          kg.remove_node(s);
          kg.remove_node(t);
        }
    }

    int karger_min_cut(GT & g,
                       DynList<typename GT::Node *> & vs,
                       DynList<typename GT::Node *> & vt,
                       DynList<typename GT::Arc *> & cut,
                       const size_t num_iter)
    {
      ah_domain_error_if(g.get_num_arcs() == 0) << "Graph has not arcs";

      auto min_cut = numeric_limits<size_t>::max();

      for (int i = 0; i < num_iter; ++i)
        {
          Kgraph kg;
          DynSetTreapRk<Karc *> arcs; // arcs for fast random selection
          build_kgraph(g, kg, arcs);

          contract(kg, 2, arcs);

          auto cut_size = kg.get_num_arcs();

          if (cut_size >= min_cut)
            continue;

          min_cut = cut_size;

          // update minimum cut
          cut.empty();

          // traverse the arcs of the super nodes (these are the cut edges)
          for (typename Kgraph::Arc_Iterator it(kg); it.has_curr(); it.next_ne())
            {
              auto ka = it.get_curr();
              assert(kg.get_src_node(ka) != kg.get_tgt_node(ka));

              cut.append(ka->get_info());
            }

          auto ka = kg.get_first_arc();
          auto S = kg.get_src_node(ka);
          auto T = kg.get_tgt_node(ka);

          assert(S->get_info().size() + T->get_info().size() ==
                 g.get_num_nodes());

          vs.empty();
          vt.empty();
          vs.swap(S->get_info());
          vt.swap(T->get_info());
        }
      return min_cut;
    }

    int __fast_karger_min_cut(Kgraph & kg, DynSetTreapRk<Karc *> & arcs)
    {
      const auto & n = kg.get_num_nodes();

      if (n <= 6)
        {
          // For small graphs, return current arc count as cut estimate
          // TODO: implement exact brute-force enumeration
          return kg.get_num_arcs();
        }

      const size_t t = std::ceil(1 + 1.0 * n / std::sqrt(2));

      Kgraph h1(kg);
      DynSetTreapRk<Karc *> arcs1(arcs);
      contract(h1, t, arcs1);
      const int cut1 = __fast_karger_min_cut(h1, arcs1);

      Kgraph h2(kg);
      DynSetTreapRk<Karc *> arcs2(arcs);
      contract(h2, t, arcs2);
      const int cut2 = __fast_karger_min_cut(h2, arcs2);

      if (cut1 < cut2)
        {
          kg.swap(h1);
          arcs.swap(arcs1);
          return cut1;
        }

      kg.swap(h2);
      arcs.swap(arcs2);
      return cut2;
    }

    int fast_karger_min_cut(GT & g,
                            DynList<typename GT::Node *> & vs,
                            DynList<typename GT::Node *> & vt,
                            DynList<typename GT::Arc *> & cut)
    {
      Kgraph kg;
      DynSetTreapRk<Karc *> arcs; // arcs for fast random selection
      build_kgraph(g, kg, arcs);

      const int min_cut = __fast_karger_min_cut(kg, arcs);

      for (typename Kgraph::Arc_Iterator it(kg); it.has_curr(); it.next_ne())
        {
          auto ka = it.get_curr();
          cut.append(ka->get_info());
        }

      auto ka = kg.get_first_arc();
      auto S = kg.get_src_node(ka);
      auto T = kg.get_tgt_node(ka);

      vs.swap(S->get_info());
      vt.swap(T->get_info());

      return min_cut;
    }

  public:
    /** Construct a Karger minimum cut solver.

        @param[in] _seed Random seed for the algorithm. Defaults to
                   current time. Using the same seed produces
                   reproducible results.
        @param[in] _sa Arc filter to determine which arcs to consider.
                   Defaults to Dft_Show_Arc<GT> which includes all arcs.
     */
    Karger_Min_Cut(const unsigned long _seed = time(nullptr), SA _sa = SA())
      : seed(_seed), r(gsl_rng_alloc(gsl_rng_mt19937)), sa(_sa)
    {
      gsl_rng_set(r, seed % gsl_rng_max(r));
    }

    /// Destructor. Frees the random number generator.
    ~Karger_Min_Cut()
    {
      gsl_rng_free(r);
    }

    /** Compute minimum cut with a specified number of iterations.

        @param[in] g The undirected graph.
        @param[out] vs Nodes in the first partition (S).
        @param[out] vt Nodes in the second partition (T).
        @param[out] cut Arcs crossing the minimum cut.
        @param[in] num_iter Number of iterations to run.
        @return The size of the minimum cut found.
        @throw std::domain_error if graph has no arcs.
     */
    int operator ()(GT & g,
                    DynList<typename GT::Node *> & vs,
                    DynList<typename GT::Node *> & vt,
                    DynList<typename GT::Arc *> & cut,
                    const size_t num_iter)
    {
      return karger_min_cut(g, vs, vt, cut, num_iter);
    }

    /** Compute minimum cut with default number of iterations.

        Uses approximately n^2 iterations where n is the number of nodes,
        which gives a high probability of finding the true minimum cut.

        @param[in] g The undirected graph.
        @param[out] vs Nodes in the first partition (S).
        @param[out] vt Nodes in the second partition (T).
        @param[out] cut Arcs crossing the minimum cut.
        @return The size of the minimum cut found.
        @throw std::domain_error if graph has no arcs.
     */
    int operator ()(GT & g,
                    DynList<typename GT::Node *> & vs,
                    DynList<typename GT::Node *> & vt,
                    DynList<typename GT::Arc *> & cut)
    {
      const size_t & n = g.get_num_nodes();
      // 1.05 * n^2 iterations gives high probability of finding min cut
      const auto num_iter = static_cast<size_t>(1.05 * n * n);
      return karger_min_cut(g, vs, vt, cut, num_iter);
    }
  };
} // namespace Aleph

# endif // KARGER_H
