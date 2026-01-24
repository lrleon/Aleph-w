
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file random_graph.H
 *  @brief Random graph generation utilities.
 *
 *  Provides classes for generating random graphs with various
 *  properties: Erdős–Rényi, random connected, Hamiltonian, Eulerian,
 *  and scale-free graphs. Uses GSL for randomness.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef RANDOM_GRAPH_H
# define RANDOM_GRAPH_H

# include <gsl/gsl_rng.h>
# include <memory>
# include <tpl_indexArc.H>
# include <tpl_graph_utils.H>
# include <tpl_components.H>
# include <single_graph.H>
# include <Tarjan.H>
# include <ah-errors.H>

namespace Aleph
{
  /** Default node initializer for random graph generation.

      This functor does nothing; it exists as a default template parameter
      for `Random_Graph` and `Random_Digraph` when no custom initialization
      is required.

      @tparam GT Graph type.
      @ingroup Graphs
   */
  template <class GT>
  struct Dft_Init_Rand_Node
  {
    void operator ()(GT &, typename GT::Node *) const noexcept
    {
      // empty
    }
  };


  /** Default arc initializer for random graph generation.

      This functor does nothing; it exists as a default template parameter
      for `Random_Graph` and `Random_Digraph` when no custom initialization
      is required.

      @tparam GT Graph type.
      @ingroup Graphs
   */
  template <class GT>
  struct Dft_Init_Rand_Arc
  {
    void operator ()(GT &, typename GT::Arc *) const noexcept
    {
      // empty
    }
  };

  // TODO: consider replacing Init_Node and Init_Arc classes with lambdas

  template <class GT, class Init_Node, class Init_Arc>
  class Random_Graph_Base
  {
  protected:
    typedef typename GT::Node GT_Node;
    typedef typename GT::Arc GT_Arc;

    gsl_rng *r;

    Init_Node & init_node;
    Init_Arc & init_arc;

    std::unique_ptr<DynArray<GT_Node *>> nodes; // pointer to save directory
                                            // space when not used

    std::unique_ptr<IndexArc<GT>> idx_arc; // pointer because constructor
                                      // requires the graph
    mutable size_t num_nodes;
    mutable size_t num_arcs;
    mutable unsigned long rand_max;

    GT g;

    bool save_parity; // indicates whether to track parity relations
                       // among nodes (only for building Eulerian
                       // and Hamiltonian graphs)

    virtual void
    update_parity_after_arc_insertion(GT_Node *src, GT_Node *tgt) = 0;

    GT_Arc * insert_arc(GT_Node *src, GT_Node *tgt)
    {
      auto a = idx_arc->insert(g.insert_arc(src, tgt));
      init_arc(g, a);
      update_parity_after_arc_insertion(src, tgt);
      return a;
    }

    /// Select a random node different from `excluded`.
    GT_Node * select_random_node(GT_Node *excluded = nullptr) noexcept
    {
      assert(nodes.get() != nullptr);
      assert(num_nodes > 0);
      assert(excluded == nullptr or num_nodes > 1); // avoid infinite loop

      GT_Node *ret_val = nullptr;
      while (true)
        {
          unsigned long idx = gsl_rng_uniform_int(r, num_nodes);
          ret_val = nodes->access(idx);
          if (excluded == nullptr or ret_val != excluded)
            break;
        }

      return ret_val;
    }

    /// Select a random node from the given list.
    GT_Node * select_random_node(DynList<GT_Node *> & list) noexcept
    {
      const unsigned long k = gsl_rng_uniform_int(r, list.size());
      typename DynList<GT_Node *>::Iterator it(list);
      for (unsigned long i = 0; i < k; ++i, it.next_ne()) {}

      return it.get_curr_ne();
    }

    virtual void create_nodes_and_initialize_arc_index() = 0;

    virtual void connect() = 0;

    void initialize_and_create_nodes(const size_t & __num_nodes,
                                     const size_t & __num_arcs)
    {
      ah_domain_error_if(__num_nodes == 0)
          << "Number of nodes must be greater than 0";

      num_nodes = __num_nodes;

      const size_t num_nodes_2 = num_nodes * num_nodes;
      if (g.is_digraph())
        num_arcs = std::min(__num_arcs, num_nodes_2 - num_nodes);
      else
        num_arcs = std::min(__num_arcs, (num_nodes_2 - num_nodes) / 2);

      create_nodes_and_initialize_arc_index();
    }

    Random_Graph_Base(const unsigned long seed,
                      const Init_Node & __init_node,
                      const Init_Arc & __init_arc)
      : r(gsl_rng_alloc(gsl_rng_mt19937)),
        init_node(const_cast<Init_Node &>(__init_node)),
        init_arc(const_cast<Init_Arc &>(__init_arc)),
        num_nodes(0), num_arcs(0),
        rand_max(gsl_rng_max(r)), save_parity(false)
    {
      ah_bad_alloc_if(r == nullptr);

      gsl_rng_set(r, seed % rand_max);
    }

    virtual ~Random_Graph_Base()
    {
      if (r != nullptr)
        gsl_rng_free(r);
    }

    /// Create a sparse random graph.
    GT create(const size_t & __num_nodes, const size_t & __num_arcs,
              const bool connected)
    {
      initialize_and_create_nodes(__num_nodes, __num_arcs);

      // randomly insert arcs by selecting random node pairs
      for (size_t i = 0; i < num_arcs; ++i)
        {
          auto src = select_random_node();
          auto tgt = select_random_node(src);
          if (idx_arc->search(src, tgt) == nullptr) // arc already exists?
            insert_arc(src, tgt);
        }

      if (connected)
        connect();

      return std::move(g);
    }

    virtual GT create_p(const size_t & __num_nodes, const double & p,
                        bool connected) = 0;

    virtual void make_eulerian() = 0;

    virtual void make_hamiltonian() = 0;

  public:
    /** Create a random Eulerian graph (sparse version).

        This version builds a sparse random graph that is guaranteed to be
        Eulerian; that is, it contains Eulerian cycles.

        The process first generates a sparse random graph, then examines
        the result and creates new arcs so that the output contains an
        Eulerian cycle.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] __num_arcs Number of arcs the graph should have.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
     */
    GT eulerian(const size_t & __num_nodes, const size_t & __num_arcs)
    {
      save_parity = true;
      g = this->create(__num_nodes, __num_arcs, true);
      make_eulerian();

      return std::move(g);
    }

    /** Create a random Eulerian graph (dense version).

        This version builds a random Eulerian graph with probability `p`
        of an arc existing between any pair of nodes.

        Use this routine to generate dense graphs. In this case, use a
        value of `p` greater than or equal to 0.5.

        @warning The procedure can be extremely slow and memory-intensive
        as `__num_nodes` increases.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] p Probability of an arc existing between any pair of nodes.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
        @throw domain_error if `p` is not in the range (0, 1].
     */
    GT eulerian(const size_t & __num_nodes, const double & p)
    {
      save_parity = true;
      g = this->create_p(__num_nodes, p, true);
      make_eulerian();

      return std::move(g);
    }

    /** Create a random Hamiltonian graph.

        This version builds a random graph that is guaranteed to satisfy
        sufficient conditions for being Hamiltonian.

        The process first generates a random graph, then examines the
        result and creates new arcs so that the output satisfies the
        Ore/Dirac theorems for Hamiltonian graphs.

        @warning The procedure can be extremely slow and memory-intensive
        as `__num_nodes` increases.

        @warning The graph is ensured to be Hamiltonian by the Ore and
        Dirac theorems. Note that this is a sufficiency condition, meaning
        the graph is not necessarily minimal. In fact, the generated graph
        tends to be dense.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] p Probability of an arc existing between any pair of
        nodes. Defaults to 0.5, which empirically tends to produce fewer
        arcs and consume less time.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
        @throw domain_error if `p` is not in the range (0, 1].

        @ingroup Graphs
     */
    GT sufficient_hamiltonian(const size_t & __num_nodes,
                              const double & p = 0.5)
    {
      g = this->create_p(__num_nodes, p, true);
      make_hamiltonian();

      return std::move(g);
    }
  };


  /** Random undirected graph generator.

      `Random_Graph` is a class for generating random undirected graphs
      with a specified number of nodes and, depending on the invoked
      operation, a specified number of arcs or arc probability.

      The class exports two generation methods via the function-call
      operator, intended for generating sparse and dense graphs
      respectively.

      For this class to compile, `GT::Node_Type` and `GT::Arc_Type`
      must have default constructors.

      Template parameters:
      - **GT**: The graph type to construct (must be undirected).
      - **Init_Node**: Functor class for initializing node data.
      - **Init_Arc**: Functor class for initializing arc data.

      The `Init_Node` class must have the following structure:
      @code
      struct Init_Node
      {
        void operator () (GT & g, GT::Node * p) { ... }
      };
      @endcode

      The `Init_Arc` class must have the following structure:
      @code
      struct Init_Arc
      {
        void operator () (GT & g, GT::Arc * a) { ... }
      };
      @endcode

      The generation routines include a boolean parameter that
      determines whether the graph should be connected. By default,
      this parameter is `true`. Consider setting it to `false` if
      connectivity is not required, since ensuring connectivity
      requires computing connected components, which adds time and
      memory overhead.

      @author Leandro Rabindranath Leon
      @ingroup Graphs
   */
  template <class GT,
            class Init_Node = Dft_Init_Rand_Node<GT>,
            class Init_Arc = Dft_Init_Rand_Arc<GT>>
  class Random_Graph : public Random_Graph_Base<GT, Init_Node, Init_Arc>
  {
    typedef typename GT::Node GT_Node;
    typedef typename GT::Arc GT_Arc;

    DynSetRandTree<GT_Node *> odd_nodes;  // nodes with odd degree
    DynSetRandTree<GT_Node *> even_nodes; // nodes with even degree

    virtual void update_parity_after_arc_insertion(GT_Node *src, GT_Node *tgt)
    {
      if (not this->save_parity)
        return;

      if (is_even(this->g.get_num_arcs(src)))
        { // was odd before the insertion
          this->odd_nodes.remove(src);
          this->even_nodes.insert(src);
        }
      else
        {
          this->even_nodes.remove(src);
          this->odd_nodes.insert(src);
        }

      if (is_even(this->g.get_num_arcs(tgt)))
        { // was odd before the insertion
          this->odd_nodes.remove(tgt);
          this->even_nodes.insert(tgt);
        }
      else
        {
          this->even_nodes.remove(tgt);
          this->odd_nodes.insert(tgt);
        }
    }

    void create_nodes_and_initialize_arc_index() override
    {
      this->nodes = std::unique_ptr<DynArray<GT_Node *>>
          (new DynArray<GT_Node *>(this->num_nodes));

      this->nodes->reserve(this->num_nodes);

      for (size_t i = 0; i < this->num_nodes; ++i)
        {
          auto p = this->g.insert_node(new GT_Node);
          this->nodes->access(i) = p;
          this->init_node(this->g, p);
          if (this->save_parity)
            {
              this->even_nodes.insert(p);
              NODE_COUNTER(p) = 0;
            }
        }

      this->idx_arc = std::unique_ptr<IndexArc<GT>>(new IndexArc<GT>(this->g));
    }

    void connect() override
    {
      DynList<DynList<GT_Node *>> subgraphs; // list of subgraphs

      Unconnected_Components<GT>()(this->g, subgraphs);

      const size_t & num_subs = subgraphs.size();

      if (num_subs == 1)
        return;

      DynArray<GT_Node *> block_nodes;

      for (typename DynList<DynList<GT_Node *>>::Iterator it(subgraphs);
           it.has_curr(); it.next_ne())
        block_nodes.append(this->select_random_node(it.get_curr_ne()));

      for (size_t i = 1; i < num_subs; ++i)
        {
          auto src = block_nodes.access(i - 1);
          auto tgt = block_nodes.access(i);
          this->insert_arc(src, tgt);
        }
    }

    // Create a random graph where the probability of an arc between
    // any pair of nodes is p
    GT create_p(const size_t & __num_nodes, const double & p,
                const bool connected) override
    {
      ah_domain_error_if(p > 1.0 or p <= 0.0) << "Invalid value for p";

      this->initialize_and_create_nodes(__num_nodes, __num_nodes);

      for (size_t i = 0; i + 1 < this->num_nodes; ++i)
        {
          auto src = this->nodes->access(i);
          for (size_t j = i + 1; j < this->num_nodes; ++j)
            if (gsl_rng_uniform(this->r) <= p) // random draw
              {
                auto tgt = this->nodes->access(j);
                assert(src != tgt);
                this->insert_arc(src, tgt);
              }
        }

      if (connected)
        connect();

      return std::move(this->g);
    }

  public:
    /** Constructor.

        @param[in] seed Seed for the random number generator.
        @param[in] __init_node Node initialization functor.
        @param[in] __init_arc Arc initialization functor.
     */
    Random_Graph(unsigned long seed,
                 const Init_Node & __init_node,
                 const Init_Arc & __init_arc)
      : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
    {
      ah_domain_error_if(this->g.is_digraph())
      << "Building of random digraph through a graph";
    }

    Random_Graph(unsigned long seed = time(nullptr),
                 const Init_Node && __init_node = Init_Node(),
                 const Init_Arc && __init_arc = Init_Arc())
      : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
    {
      ah_domain_error_if(this->g.is_digraph())
      << "Building of random digraph through a graph";
    }


    /** Create a sparse random graph.

        This version builds a random graph with approximately `__num_arcs`
        arcs. The graph contains no parallel arcs.

        The randomization procedure selects two nodes at random and inserts
        an arc between them if one does not already exist. The routine is
        linear, proportional to `__num_arcs`.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] __num_arcs Number of arcs the graph should have.
        @param[in] connected Whether the generated graph should be connected.
        Defaults to `true`.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
     */
    GT operator ()(const size_t & __num_nodes, const size_t & __num_arcs,
                   bool connected = true)
    {
      return this->create(__num_nodes, __num_arcs, connected);
    }

    /** Create a random graph with arc probability.

        This version builds a random graph with probability `p` of an arc
        existing between any pair of nodes.

        Use this routine to generate dense graphs. In this case, use a
        value of `p` greater than or equal to 0.5.

        The procedure is quadratic: all node pairs are enumerated, and for
        each pair `(i, j)` a random draw with probability `p` determines
        whether an arc is created.

        @warning The procedure can be extremely slow and memory-intensive
        as `__num_nodes` increases.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] p Probability of an arc existing between any pair of nodes.
        @param[in] connected Whether the generated graph should be connected.
        Defaults to `true`.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
        @throw domain_error if `p` is not in the range (0, 1].
     */
    GT operator ()(const size_t & __num_nodes, const double & p,
                   bool connected = true)
    {
      return create_p(__num_nodes, p, connected);
    }

  private:
    void make_eulerian() override
    {
      while (this->odd_nodes.size() > 1)
        {
          GT_Node *src = nullptr;
          GT_Node *tgt = nullptr;

          while (true)
            {
              src = this->odd_nodes.select
                  (gsl_rng_uniform_int(this->r, this->odd_nodes.size()));
              do
                tgt = this->odd_nodes.select
                    (gsl_rng_uniform_int(this->r, this->odd_nodes.size()));
              while (tgt == src);

              if (this->idx_arc->search(src, tgt) == nullptr)
                break;
              else if (this->odd_nodes.size() == 2)
                { // select random node that has no arc to src or tgt
                  GT_Node *p = nullptr;
                  do
                    p = this->even_nodes.select
                        (gsl_rng_uniform_int(this->r, this->even_nodes.size()));
                  while (this->idx_arc->search(src, p) != nullptr or
                         this->idx_arc->search(tgt, p) != nullptr);
                  this->insert_arc(src, p);
                  this->insert_arc(p, tgt);

                  return;
                }
            }

          this->insert_arc(src, tgt);
        }

      assert(this->odd_nodes.size() == 0);
    }

    void balance_graph_nodes_degree(GT_Node *src, GT_Node *tgt)
    {
      if (this->idx_arc->search(src, tgt) == nullptr)
        this->insert_arc(src, tgt);

      const size_t & n = this->g.get_num_nodes();

      while (this->g.get_num_arcs(src) + this->g.get_num_arcs(tgt) < n)
        {
          auto p = this->nodes->access(gsl_rng_uniform_int(this->r, n));
          if (p == src or p == tgt)
            continue;

          if (this->idx_arc->search(src, p) == nullptr)
            this->insert_arc(src, p);

          if (this->g.get_num_arcs(src) + this->g.get_num_arcs(tgt) == n)
            break;

          if (this->idx_arc->search(tgt, p) == nullptr)
            this->insert_arc(tgt, p);
        }
    }

    void make_hamiltonian() override
    {
      const size_t & n = this->g.get_num_nodes();
      for (size_t i = 0; i + 1 < n; ++i)
        {
          auto src = this->nodes->access(i);
          for (size_t j = i + 1; j < n; ++j)
            balance_graph_nodes_degree(src, this->nodes->access(j));
        }
    }

  public:
    /** Create a random Eulerian graph (sparse version).

        This version builds a sparse random graph that is guaranteed to be
        Eulerian; that is, it contains Eulerian cycles.

        The process first generates a sparse random graph, then examines
        the result and creates new arcs so that the output contains an
        Eulerian cycle.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] __num_arcs Number of arcs the graph should have.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
     */
    GT eulerian(const size_t & __num_nodes, const size_t & __num_arcs)
    {
      this->save_parity = true;
      this->g = this->create(__num_nodes, __num_arcs, true);
      make_eulerian();

      return std::move(this->g);
    }

    /** Create a random Eulerian graph (dense version).

        This version builds a random Eulerian graph with probability `p`
        of an arc existing between any pair of nodes.

        Use this routine to generate dense graphs. In this case, use a
        value of `p` greater than or equal to 0.5.

        @warning The procedure can be extremely slow and memory-intensive
        as `__num_nodes` increases.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] p Probability of an arc existing between any pair of nodes.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
        @throw domain_error if `p` is not in the range (0, 1].
     */
    GT eulerian(const size_t & __num_nodes, const double & p)
    {
      this->save_parity = true;
      this->g = this->create_p(__num_nodes, p, true);
      make_eulerian();

      return std::move(this->g);
    }

    /** Create a random Hamiltonian graph.

        This version builds a random graph that is guaranteed to satisfy
        sufficient conditions for being Hamiltonian.

        The process first generates a random graph, then examines the
        result and creates new arcs so that the output satisfies the
        Ore/Dirac theorems for Hamiltonian graphs.

        @warning The procedure can be extremely slow and memory-intensive
        as `__num_nodes` increases.

        @warning The graph is ensured to be Hamiltonian by the Ore and
        Dirac theorems. Note that this is a sufficiency condition, meaning
        the graph is not necessarily minimal. In fact, the generated graph
        tends to be dense.

        @param[in] __num_nodes Number of nodes the graph should have.
        @param[in] p Probability of an arc existing between any pair of
        nodes. Defaults to 0.5, which empirically tends to produce fewer
        arcs and consume less time.
        @return The randomly created graph.
        @throw bad_alloc if there is not enough memory.
        @throw domain_error if `p` is not in the range (0, 1].

        @ingroup Graphs
     */
    GT sufficient_hamiltonian(const size_t & __num_nodes,
                              const double & p = 0.5)
    {
      this->g = this->create_p(__num_nodes, p, true);
      make_hamiltonian();

      return std::move(this->g);
    }
  };


  /** Random directed graph (digraph) generator.

      `Random_Digraph` is a class for generating random directed graphs
      with a specified number of nodes and, depending on the invoked
      operation, a specified number of arcs or arc probability.

      The class exports two generation methods via the function-call
      operator, intended for generating sparse and dense digraphs
      respectively.

      Template parameters:
      - **GT**: The graph type to construct (must be directed).
      - **Init_Node**: Functor class for initializing node data.
      - **Init_Arc**: Functor class for initializing arc data.

      @author Leandro Rabindranath Leon
      @ingroup Graphs
   */
  template <class GT,
            class Init_Node = Dft_Init_Rand_Node<GT>,
            class Init_Arc = Dft_Init_Rand_Arc<GT>>
  class Random_Digraph : public Random_Graph_Base<GT, Init_Node, Init_Arc>
  {
    typedef typename GT::Node GT_Node;
    typedef typename GT::Arc GT_Arc;

    DynSetRandTree<GT_Node *> greater; // nodes with out-degree > in-degree
    DynSetRandTree<GT_Node *> smaller; // nodes with out-degree < in-degree
    DynSetRandTree<GT_Node *> equal;   // nodes with out-degree == in-degree

    bool verify_tables()
    {
      const size_t & n = this->nodes->size();

      if (n != this->g.get_num_nodes())
        std::cout << "Warning num of nodes of graph does not match with array "
            << this->g.get_num_nodes() << "!=" << n << std::endl;

      size_t total = greater.size() + smaller.size() + equal.size();
      if (total != this->g.get_num_nodes())
        std::cout << "Inconsistency with nodes parity" << std::endl
            << "greater = " << greater.size() << std::endl
            << "smaller = " << smaller.size() << std::endl
            << "equal   = " << equal.size() << std::endl
            << "total   = " << total << std::endl
            << "|V|     = " << this->g.get_num_nodes();

      for (size_t i = 0; i < n; ++i)
        {
          auto p = this->nodes->access(i);

          const long & in_sz = NODE_COUNTER(p);
          const size_t & out_sz = this->g.get_num_arcs(p);

          if (in_sz == out_sz)
            {
              if (smaller.search(p) != nullptr)
                std::cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
                    << " in smaller table" << std::endl;

              if (greater.search(p) != nullptr)
                std::cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
                    << " in greater table" << std::endl;

              if (equal.search(p) == nullptr)
                {
                  std::cout << "node of same in/out degree is not in equal table"
                      << std::endl;

                  return false;
                }
            }
          else if (in_sz > out_sz)
            {
              if (greater.search(p) != nullptr)
                std::cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
                    << " in greater table" << std::endl;

              if (equal.search(p) != nullptr)
                std::cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
                    << std::endl;

              if (smaller.search(p) == nullptr)
                {
                  std::cout << "node with " << in_sz << "/" << out_sz << " not found "
                      << "smaller table" << std::endl;

                  return false;
                }
            }
          else
            {
              if (smaller.search(p) != nullptr)
                std::cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
                    << " in smaller table" << std::endl;

              if (equal.search(p) != nullptr)
                std::cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
                    << std::endl;

              if (greater.search(p) == nullptr)
                {
                  std::cout << "node with " << in_sz << "/" << out_sz << " not found "
                      << "greater table" << std::endl;

                  return false;
                }
            }
        }

      return true;
    }

    // This call is made right after inserting a new arc src-->tgt.
    // This implies that out(src) is updated, but in(tgt) is not.
    virtual void update_parity_after_arc_insertion(GT_Node *src, GT_Node *tgt)
    {
      if (not this->save_parity)
        return;

      const size_t & src_out_degree = this->g.get_num_arcs(src);
      const long & src_in_degree = NODE_COUNTER(src);

      if (src_out_degree == src_in_degree)
        { // src is in greater ==> remove it and insert into equal
          assert(this->smaller.search(src) != nullptr);
          this->smaller.remove(src);
          this->equal.insert(src);
        }
      else if (src_out_degree > src_in_degree)
        if (src_out_degree == src_in_degree + 1)
          {
            assert(this->equal.search(src) != nullptr);
            this->equal.remove(src);
            this->greater.insert(src);
          }
        else
          assert(this->greater.search(src) != nullptr);
      else // src_out_degree < src_in_degree
        assert(this->smaller.search(src) != nullptr);

      const size_t & tgt_out_degree = this->g.get_num_arcs(tgt);
      const long tgt_in_degree = ++NODE_COUNTER(tgt);

      if (tgt_out_degree == tgt_in_degree)
        {
          assert(this->greater.search(tgt));
          this->greater.remove(tgt);
          this->equal.insert(tgt);
        }
      else if (tgt_out_degree > tgt_in_degree)
        assert(this->greater.search(tgt));
      else // (tgt_out_degree < tgt_in_degree)
        {
          if (tgt_in_degree - 1 == tgt_out_degree)
            { // tgt is in equal ==> remove it
              assert(this->equal.search(tgt) != nullptr);
              this->smaller.insert(tgt);
              this->equal.remove(tgt);
            }
          else
            assert(this->smaller.search(tgt) != nullptr);
        }
    }

    void create_nodes_and_initialize_arc_index() override
    {
      this->nodes = std::unique_ptr<DynArray<GT_Node *>>
          (new DynArray<GT_Node *>(this->num_nodes));

      this->nodes->reserve(this->num_nodes);

      for (size_t i = 0; i < this->num_nodes; ++i)
        {
          typename GT::Node *p = this->g.insert_node(new GT_Node);
          this->nodes->access(i) = p;
          this->init_node(this->g, p);

          if (this->save_parity)
            {
              NODE_COUNTER(p) = 0;
              this->equal.insert(p);
            }
        }

      this->idx_arc = std::unique_ptr<IndexArc<GT>>(new IndexArc<GT>(this->g));
    }

    void connect() override
    {
      DynList<DynList<typename GT::Node *>> blk_list; // disconnected subgraphs

      { // save in-degrees since Tarjan's algorithm will modify them
        DynArray<int> in_degree;
        in_degree.reserve(this->g.get_num_nodes());

        typename GT::Node_Iterator it(this->g);
        for (int i = 0; it.has_curr(); it.next_ne(), ++i)
          in_degree.access(i) = NODE_COUNTER(it.get_curr_ne());

        Tarjan_Connected_Components<GT>()(this->g, blk_list);

        it.reset_first(); // restore in-degrees
        for (size_t i = 0; it.has_curr(); it.next_ne(), ++i)
          NODE_COUNTER(it.get_curr_ne()) = in_degree.access(i);
      }

      const size_t & num_blocks = blk_list.size();

      if (num_blocks == 1)
        return;

      // each node in this list is a randomly selected node from block i
      DynArray<typename GT::Node *> b1;
      b1.reserve(num_blocks);
      DynArray<typename GT::Node *> b2;
      b2.reserve(num_blocks); {
        typename DynList<DynList<GT_Node *>>::Iterator it(blk_list);
        for (size_t i = 0; it.has_curr(); it.next_ne(), ++i)
          { // select two random nodes from the current component
            DynList<typename GT::Node *> & list = it.get_curr_ne();
            b1.access(i) = this->select_random_node(list);
            b2.access(i) = this->select_random_node(list);
          }
      }

      for (size_t i = 0; i + 1 < num_blocks; ++i)
        {
          auto src = b1.access(i); // node in block i
          auto tgt = b1.access((i + 1) % num_blocks); // node in block i + 1

          if (this->idx_arc->search_directed(src, tgt) == nullptr)
            this->insert_arc(src, tgt);

          src = b2.access(i); // node in block i
          tgt = b2.access((i + 1) % num_blocks); // node in block i + 1

          if (this->idx_arc->search_directed(tgt, src) == nullptr)
            this->insert_arc(tgt, src);
        }
    }

    // Create a random graph where the probability of an arc between
    // any pair of nodes is p
    GT create_p(const size_t & __num_nodes, const double & p,
                bool connected) override
    {
      ah_domain_error_if(p > 1.0 or p <= 0.0) << "Invalid value for p";

      this->initialize_and_create_nodes(__num_nodes, __num_nodes);

      for (size_t i = 0; i < this->num_nodes; ++i)
        {
          auto src = this->nodes->access(i);
          for (size_t j = 0; j < this->num_nodes; ++j)
            if (i != j and gsl_rng_uniform(this->r) <= p)
              {
                auto tgt = this->nodes->access(j);
                assert(this->idx_arc->search_directed(src, tgt) == nullptr);
                this->insert_arc(src, tgt);
              }
        }

      if (connected)
        connect();

      return std::move(this->g);
    }

  public:
    /** Constructor.

        @param[in] seed Seed for the random number generator.
        @param[in] __init_node Node initialization functor.
        @param[in] __init_arc Arc initialization functor.
     */
    Random_Digraph(unsigned long seed,
                   const Init_Node & __init_node,
                   const Init_Arc & __init_arc)
      : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
    {
      this->g.set_digraph(true);
    }

    Random_Digraph(unsigned long seed = time(nullptr),
                   const Init_Node && __init_node = Init_Node(),
                   const Init_Arc && __init_arc = Init_Arc())
      : Random_Digraph(seed, __init_node, __init_arc)
    {
      // empty
    }

    Random_Digraph(const Init_Node & __init_node,
                   const Init_Arc & __init_arc)
      : Random_Digraph(time(nullptr), __init_node, __init_arc)
    {
      // empty
    }

    ~Random_Digraph()
    {
      this->g.set_digraph(false);
    }

    /** Create a sparse random digraph.

        This version builds a random digraph with approximately `__num_arcs`
        arcs. The digraph contains no parallel arcs.

        The randomization procedure selects two nodes at random and inserts
        an arc between them if one does not already exist. The routine is
        linear, proportional to `__num_arcs`.

        @param[in] __num_nodes Number of nodes the digraph should have.
        @param[in] __num_arcs Number of arcs the digraph should have.
        @param[in] connected Whether the generated digraph should be strongly
        connected. Defaults to `true`.
        @return The randomly created digraph.
        @throw bad_alloc if there is not enough memory.
     */
    GT operator ()(const size_t & __num_nodes, const size_t & __num_arcs,
                   bool connected = true)
    {
      return this->create(__num_nodes, __num_arcs, connected);
    }

    /** Create a random digraph with arc probability.

        This version builds a random digraph with probability `p` of an arc
        existing between any ordered pair of nodes.

        Use this routine to generate dense digraphs. In this case, use a
        value of `p` greater than or equal to 0.5.

        The procedure is quadratic: all ordered node pairs are enumerated,
        and for each pair `(i, j)` a random draw with probability `p`
        determines whether an arc is created.

        @warning The procedure can be extremely slow and memory-intensive
        as `__num_nodes` increases.

        @param[in] __num_nodes Number of nodes the digraph should have.
        @param[in] p Probability of an arc existing between any ordered
        pair of nodes.
        @param[in] connected Whether the generated digraph should be strongly
        connected. Defaults to `true`.
        @return The randomly created digraph.
        @throw bad_alloc if there is not enough memory.
        @throw domain_error if `p` is not in the range (0, 1].
     */
    GT operator ()(const size_t & __num_nodes, const double & p,
                   bool connected = true)
    {
      return this->create_p(__num_nodes, p, connected);
    }

  private:
    void make_eulerian() override
    {
      GT_Node *src = nullptr;
      GT_Node *tgt = nullptr;

      while (this->greater.size() > 0 and this->smaller.size() > 0)
        {
          do
            {
              tgt = this->greater.select
                  (gsl_rng_uniform_int(this->r, this->greater.size()));
              src = this->smaller.select
                  (gsl_rng_uniform_int(this->r, this->smaller.size()));
            }
          while (src == tgt);

          if (this->idx_arc->search_directed(src, tgt) == nullptr)
            this->insert_arc(src, tgt);
          else
            {
              auto mid =
                  this->equal.select(gsl_rng_uniform_int(this->r,
                                                         this->equal.size()));

              while (this->idx_arc->search_directed(src, mid) != nullptr or
                     this->idx_arc->search_directed(mid, tgt) != nullptr)
                mid = this->equal.select
                    (gsl_rng_uniform_int(this->r, this->equal.size()));

              this->insert_arc(src, mid);
              this->insert_arc(mid, tgt);
            }
        }
    }

    void balance_digraph_node(GT_Node *p)
    {
      const size_t & n = this->g.get_num_nodes();
      const size_t n2 = n / 2;

      while (not (this->g.get_num_arcs(p) >= n2 and NODE_COUNTER(p) >= n2))
        {
          auto q = this->nodes->access(gsl_rng_uniform_int(this->r, n));
          if (q == p)
            continue;

          if (this->idx_arc->search_directed(p, q) == nullptr)
            {
              this->insert_arc(p, q);
              NODE_COUNTER(q)++;
            }

          if (this->idx_arc->search_directed(q, p) == nullptr)
            {
              this->insert_arc(q, p);
              NODE_COUNTER(p)++;
            }
        }
    }

    // Balance both nodes so they satisfy the Hamiltonian condition.
    // If arc src-->tgt exists, balance each node independently.
    void balance_digraph_nodes_degree(GT_Node *src, GT_Node *tgt)
    {
      if (this->idx_arc->search_directed(src, tgt) != nullptr)
        {
          balance_digraph_node(src);
          balance_digraph_node(tgt);

          return;
        }

      const size_t & n = this->g.get_num_nodes();

      while (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) < n)
        {
          auto p = this->nodes->access(gsl_rng_uniform_int(this->r, n));
          if (p == src or p == tgt)
            continue;

          if (this->idx_arc->search_directed(src, p) == nullptr)
            {
              this->insert_arc(src, p);
              NODE_COUNTER(p)++;

              if (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) == n)
                break;
            }

          if (this->idx_arc->search_directed(p, tgt) == nullptr)
            {
              this->insert_arc(p, tgt);
              NODE_COUNTER(tgt)++;
            }
        }

      assert(this->g.get_num_arcs(src) + NODE_COUNTER(tgt) >= n);
    }

    void make_hamiltonian() override
    {
      this->g.reset_counter_nodes();

      // compute in-degree for each node
      for (typename GT::Arc_Iterator it(this->g); it.has_curr(); it.next_ne())
        NODE_COUNTER(it.get_tgt_node_ne())++;

      const size_t & n = this->g.get_num_nodes();

      for (size_t i = 0; i < n; ++i)
        {
          auto src = this->nodes->access(i);
          for (size_t j = 0; j < n; ++j)
            {
              if (i == j)
                continue;

              auto tgt = this->nodes->access(j);
              balance_digraph_nodes_degree(src, tgt);
            }
        }
    }
  };
}

# endif // RANDOM_GRAPH_H
