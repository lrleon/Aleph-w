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


/** @file Karger.H
 *  @brief Karger's randomized min-cut algorithm.
 *
 *  Implements Karger's contraction algorithm for finding minimum
 *  cuts in graphs. Uses random edge contractions to approximate
 *  the minimum cut with high probability.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef KARGER_H
# define KARGER_H

# include <cmath>
# include <limits>
# include <vector>
# include <htlist.H>
# include <tpl_sgraph.H>
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

      Features:
      - Early termination when a cut of size 1 is found
      - Reuses internal graph structures across iterations for efficiency
      - Move-only semantics (non-copyable, moveable)
      - Reproducible results with seed control

      @note This algorithm is designed for undirected graphs.
      @note The graph must be connected (considering the arc filter) and contain no self-loops.
      @note The graph must have at least 2 nodes and 1 arc.

      @par Example: Finding minimum cut
      @code
      using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
      GT g;
      
      // Create a simple graph with known min-cut
      auto a = g.insert_node(1);
      auto b = g.insert_node(2);
      auto c = g.insert_node(3);
      auto d = g.insert_node(4);
      
      // Left side
      g.insert_arc(a, b);
      
      // Right side
      g.insert_arc(c, d);
      
      // Bridge (min-cut)
      g.insert_arc(b, c);
      
      // Find minimum cut
      Karger_Min_Cut<GT> karger;
      DynList<GT::Node*> S, T;
      DynList<GT::Arc*> cut_arcs;
      
      size_t min_cut_size = karger(g, S, T, cut_arcs);
      // min_cut_size will be 1 (the bridge)
      // One valid partition is S={a, b}, T={c, d} (order may vary)
      @endcode

      @par Example: Multiple iterations for better probability
      @code
      Karger_Min_Cut<GT> karger(12345); // Seed for reproducibility
      
      size_t best_cut = numeric_limits<size_t>::max();
      DynList<GT::Node*> best_S, best_T;
      
      // Run multiple iterations
      for (int i = 0; i < 10; ++i)
      {
        DynList<GT::Node*> S, T;
        DynList<GT::Arc*> cut_arcs;
        size_t cut_size = karger(g, S, T, cut_arcs);
        
        if (cut_size < best_cut)
        {
          best_cut = cut_size;
          best_S = S;
          best_T = T;
        }
      }
      @endcode

      @par Example: Using fast Karger-Stein algorithm
      @code
      Karger_Min_Cut<GT> karger;
      DynList<GT::Node*> S, T;
      DynList<GT::Arc*> cut_arcs;
      
      // Fast version: O(n^2 log^3 n)
      size_t cut_size = karger.fast(g, S, T, cut_arcs);
      @endcode

      @see https://en.wikipedia.org/wiki/Karger%27s_algorithm
      @ingroup Graphs
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

    /// Arc index with O(1) operations and deterministic ordering.
    /// Uses arc counter to store position for O(1) swap-remove.
    /// This ensures reproducible results with the same random seed.
    class ArcIndex
    {
      std::vector<Karc *> arcs;

    public:
      void insert(Karc *a)
      {
        ARC_COUNTER(a) = static_cast<long>(arcs.size());
        arcs.push_back(a);
      }

      Karc * select(size_t i) const
      {
        assert(i < arcs.size());
        return arcs[i];
      }

      void remove(Karc *a)
      {
        const long raw_idx = ARC_COUNTER(a);
        if (raw_idx < 0) // Already removed (parallel arc processed twice)
          return;

        auto idx = static_cast<size_t>(raw_idx);
        assert(idx < arcs.size());
        assert(arcs[idx] == a);
        if (idx < arcs.size() - 1)
          {
            arcs[idx] = arcs.back();
            ARC_COUNTER(arcs[idx]) = static_cast<long>(idx);
          }
        arcs.pop_back();
        ARC_COUNTER(a) = -1; // Mark as removed
      }

      void empty() { arcs.clear(); }

      [[nodiscard]] size_t size() const { return arcs.size(); }

      void swap(ArcIndex & other) noexcept { arcs.swap(other.arcs); }
    };


    bool has_self_loop(const GT & g) const
    {
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        if (auto a = it.get_curr(); g.get_src_node(a) == g.get_tgt_node(a))
          return true;
      return false;
    }

    bool is_connected_filtered(const GT & g) const
    {
      if (g.get_num_nodes() == 0)
        return false;

      Depth_First_Traversal<GT, Default_Visit_Op<GT>, SA> dft(sa);
      return dft(g) == g.get_num_nodes();
    }

    void validate_graph(GT & g) const
    {
      ah_domain_error_if(g.get_num_nodes() < 2) << "Graph must have at least 2 nodes";
      ah_domain_error_if(g.get_num_arcs() == 0) << "Graph has no arcs";
      ah_domain_error_if(has_self_loop(g)) << "Graph contains self-loop";
      ah_domain_error_if(not is_connected_filtered(g)) << "Graph is disconnected";
    }

    unsigned long seed;
    gsl_rng *r;
    SA sa;

    /// Build the contracted graph representation from the original graph.
    /// Each node in kg contains a list of original nodes it represents.
    /// @param[in] g The original graph.
    /// @param[out] kg The contracted graph (cleared and rebuilt).
    /// @param[out] arcs Index of arcs for O(1) random selection.
    void build_kgraph(GT & g, Kgraph & kg, ArcIndex & arcs)
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

    /// Update arcs when contracting nodes p and t into cp.
    /// Moves all arcs from node p to the new contracted node cp,
    /// except arcs that connect to t (which become self-loops and are removed).
    /// @param[in,out] kg The contracted graph.
    /// @param[in] p Source node being contracted.
    /// @param[in] t Target node being contracted (arcs to t are skipped).
    /// @param[in] cp New contracted node that replaces p and t.
    /// @param[in,out] arcs Index of arcs (updated to reflect changes).
    void update_arcs(Kgraph & kg, Knode *p, Knode *t, Knode *cp,
                     ArcIndex & arcs)
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

    /// Rebuild the arc index from scratch for the given graph.
    /// Used after copying a graph to ensure pointers point to the copy's arcs.
    void rebuild_arc_index(Kgraph & kg, ArcIndex & arcs)
    {
      arcs.empty();
      for (typename Kgraph::Arc_Iterator it(kg); it.has_curr(); it.next_ne())
        arcs.insert(it.get_curr());
    }

    /// Contract the graph by randomly merging edges until left_num_nodes remain.
    /// This is the core of Karger's algorithm. Each iteration:
    /// 1. Randomly selects an arc
    /// 2. Contracts its endpoints into a single "super-node"
    /// 3. Removes self-loops created by the contraction
    /// @param[in,out] kg The graph to contract.
    /// @param[in] left_num_nodes Stop when this many nodes remain.
    /// @param[in,out] arcs Index of arcs for O(1) random selection.
    void contract(Kgraph & kg, size_t left_num_nodes,
                  ArcIndex & arcs)
    {
      while (kg.get_num_nodes() > left_num_nodes)
        { // randomly select an arc from kg
          assert(arcs.size() == kg.get_num_arcs() &&
                 "Arc index and graph arc count must be in sync");
          auto num_arc = gsl_rng_uniform_int(r, arcs.size());
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

    /// Internal implementation of Karger's minimum cut algorithm.
    /// Runs num_iter independent contractions and returns the best cut found.
    /// @param[in] g The original undirected graph.
    /// @param[out] vs Nodes in partition S of the minimum cut.
    /// @param[out] vt Nodes in partition T of the minimum cut.
    /// @param[out] cut Arcs crossing the minimum cut.
    /// @param[in] num_iter Number of independent iterations to run.
    /// @return Size of the minimum cut found.
    size_t karger_min_cut(GT & g,
                          DynList<typename GT::Node *> & vs,
                          DynList<typename GT::Node *> & vt,
                          DynList<typename GT::Arc *> & cut,
                          const size_t num_iter)
    {
      validate_graph(g);

      auto min_cut = std::numeric_limits<size_t>::max();

      Kgraph kg;
      ArcIndex arcs; // reused across iterations

      for (size_t i = 0; i < num_iter; ++i)
        {
          build_kgraph(g, kg, arcs);

          contract(kg, 2, arcs);

          const size_t cut_size = kg.get_num_arcs();

          if (cut_size >= min_cut)
            continue;

          min_cut = cut_size;

          // Early termination: cut of 1 is optimal for connected graphs
          if (min_cut == 1)
            {
              auto ka = kg.get_first_arc();
              cut.empty();
              cut.append(ka->get_info());
              vs.empty();
              vt.empty();
              vs.swap(kg.get_src_node(ka)->get_info());
              vt.swap(kg.get_tgt_node(ka)->get_info());
              return 1;
            }

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

    /// Karger-Stein fast minimum cut algorithm (recursive version).
    /// Time complexity: O(n^2 log^3 n)
    size_t __fast_karger_min_cut(Kgraph & kg, ArcIndex & arcs)
    {
      const size_t n = kg.get_num_nodes();

      if (n <= 6)
        {
          // Base case: for small graphs, contract to 2 nodes and return cut
          contract(kg, 2, arcs);
          return kg.get_num_arcs();
        }

      // Contract to t = ceil(1 + n/sqrt(2)) nodes
      const auto t = static_cast<size_t>(std::ceil(1.0 + n / std::sqrt(2.0)));

      // Branch 1: copy graph and rebuild arc index for the copy
      Kgraph h1(kg);
      ArcIndex arcs1;
      rebuild_arc_index(h1, arcs1);
      contract(h1, t, arcs1);
      const size_t cut1 = __fast_karger_min_cut(h1, arcs1);

      // Branch 2: copy graph and rebuild arc index for the copy
      Kgraph h2(kg);
      ArcIndex arcs2;
      rebuild_arc_index(h2, arcs2);
      contract(h2, t, arcs2);
      const size_t cut2 = __fast_karger_min_cut(h2, arcs2);

      // Return the better cut
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

    /// Destructor. Frees the random number generator (if not moved).
    ~Karger_Min_Cut()
    {
      if (r)
        gsl_rng_free(r);
    }

    /// Disable copy constructor (class owns gsl_rng pointer)
    Karger_Min_Cut(const Karger_Min_Cut &) = delete;

    /// Disable copy assignment (class owns gsl_rng pointer)
    Karger_Min_Cut &operator=(const Karger_Min_Cut &) = delete;

    /// Move constructor. Transfers ownership of the random number generator.
    Karger_Min_Cut(Karger_Min_Cut && other) noexcept
      : seed(other.seed), r(other.r), sa(std::move(other.sa))
    {
      other.r = nullptr;
    }

    /// Move assignment operator. Transfers ownership of the random number generator.
    Karger_Min_Cut &operator=(Karger_Min_Cut && other) noexcept
    {
      if (this != &other)
        {
          if (r)
            gsl_rng_free(r);
          seed = other.seed;
          r = other.r;
          sa = std::move(other.sa);
          other.r = nullptr;
        }
      return *this;
    }

    /// Get the random seed used by this solver (useful for reproducibility).
    [[nodiscard]] unsigned long get_seed() const noexcept { return seed; }

    /// Change the random seed. Useful for running multiple independent trials.
    /// @param[in] new_seed The new seed value.
    void reseed(const unsigned long new_seed) noexcept
    {
      seed = new_seed;
      if (r)
        gsl_rng_set(r, seed % gsl_rng_max(r));
    }

    /** Compute only the minimum cut size (without partition info).

        This is more efficient when you only need the cut size,
        not the actual partition or cut edges.

        @param[in] g The undirected graph.
        @param[in] num_iter Number of iterations to run.
        @return The size of the minimum cut found.
        @throw std::domain_error if graph is disconnected, contains self-loops, has fewer than 2 nodes, or has no arcs.
     */
    size_t compute_min_cut_size(GT & g, size_t num_iter = 0)
    {
      assert(r != nullptr && "Cannot use moved-from Karger_Min_Cut object");
      validate_graph(g);

      if (num_iter == 0)
        {
          const size_t n = g.get_num_nodes();
          num_iter = static_cast<size_t>(1.05 * n * n);
        }

      size_t min_cut = std::numeric_limits<size_t>::max();

      Kgraph kg;
      ArcIndex arcs;

      for (size_t i = 0; i < num_iter; ++i)
        {
          build_kgraph(g, kg, arcs);
          contract(kg, 2, arcs);

          const size_t cut_size = kg.get_num_arcs();
          if (cut_size < min_cut)
            {
              min_cut = cut_size;
              if (min_cut == 1)
                return 1; // Early termination
            }
        }
      return min_cut;
    }

    /** Compute a minimum cut with a specified number of iterations.

        @param[in] g The undirected graph.
        @param[out] vs Nodes in the first partition (S).
        @param[out] vt Nodes in the second partition (T).
        @param[out] cut Arcs crossing the minimum cut.
        @param[in] num_iter Number of iterations to run.
        @return The size of the minimum cut found.
        @throw std::domain_error if the graph is disconnected, contains self-loops, has fewer than 2 nodes, or has no arcs.
     */
    size_t operator ()(GT & g,
                       DynList<typename GT::Node *> & vs,
                       DynList<typename GT::Node *> & vt,
                       DynList<typename GT::Arc *> & cut,
                       const size_t num_iter)
    {
      assert(r != nullptr && "Cannot use moved-from Karger_Min_Cut object");
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
        @throw std::domain_error if graph is disconnected, contains self-loops, has fewer than 2 nodes, or has no arcs.
     */
    size_t operator ()(GT & g,
                       DynList<typename GT::Node *> & vs,
                       DynList<typename GT::Node *> & vt,
                       DynList<typename GT::Arc *> & cut)
    {
      assert(r != nullptr && "Cannot use moved-from Karger_Min_Cut object");
      const size_t n = g.get_num_nodes();
      // 1.05 * n^2 iterations gives high probability of finding min cut
      const auto num_iter = static_cast<size_t>(1.05 * n * n);
      return karger_min_cut(g, vs, vt, cut, num_iter);
    }

    /** Compute minimum cut using Karger-Stein fast algorithm.

        This version uses the recursive Karger-Stein algorithm which has
        better time complexity: O(n^2 log^3 n) vs O(n^4) for the standard.

        The algorithm is run O(log^2 n) times to achieve high probability
        of finding the true minimum cut.

        @param[in] g The undirected graph.
        @param[out] vs Nodes in the first partition (S).
        @param[out] vt Nodes in the second partition (T).
        @param[out] cut Arcs crossing the minimum cut.
        @param[in] num_iter Number of iterations (default: ceil(log^2 n)).
        @return The size of the minimum cut found.
        @throw std::domain_error if graph is disconnected, contains self-loops, has fewer than 2 nodes, or has no arcs.
     */
    size_t fast(GT & g,
                DynList<typename GT::Node *> & vs,
                DynList<typename GT::Node *> & vt,
                DynList<typename GT::Arc *> & cut,
                size_t num_iter = 0)
    {
      assert(r != nullptr && "Cannot use moved-from Karger_Min_Cut object");
      validate_graph(g);

      // Default iterations: ceil(log^2 n) for high probability
      if (num_iter == 0)
        {
          const double log_n = std::log2(static_cast<double>(g.get_num_nodes()));
          num_iter = static_cast<size_t>(std::ceil(log_n * log_n));
          if (num_iter < 1)
            num_iter = 1;
        }

      size_t best_cut = std::numeric_limits<size_t>::max();

      for (size_t i = 0; i < num_iter; ++i)
        {
          Kgraph kg;
          ArcIndex arcs;
          build_kgraph(g, kg, arcs);

          const size_t current_cut = __fast_karger_min_cut(kg, arcs);

          if (current_cut >= best_cut)
            continue;

          best_cut = current_cut;

          cut.empty();
          for (typename Kgraph::Arc_Iterator it(kg); it.has_curr(); it.next_ne())
            cut.append(it.get_curr()->get_info());

          auto ka = kg.get_first_arc();
          auto S = kg.get_src_node(ka);
          auto T = kg.get_tgt_node(ka);

          vs.empty();
          vt.empty();
          vs.swap(S->get_info());
          vt.swap(T->get_info());

          // Early termination
          if (best_cut == 1)
            return 1;
        }

      return best_cut;
    }
  };
} // namespace Aleph

# endif // KARGER_H
