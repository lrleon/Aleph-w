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
 * @file blossom_weighted_mwmatching.H
 * @brief Internal weighted matching core used by Blossom_Weighted.H.
 *
 * This file contains an adapted version of the MIT-licensed reference
 * implementation by Joris van Rantwijk (repository:
 * https://git.jorisvr.nl/joris/maximum-weight-matching).
 *
 * Adaptations for Aleph include:
 * - Namespace nesting under Aleph internals.
 * - Error reporting via Aleph ah-errors macros.
 * - Header/style integration.
 */

# ifndef ALEPH_BLOSSOM_WEIGHTED_MWMATCHING_H
# define ALEPH_BLOSSOM_WEIGHTED_MWMATCHING_H

# include <algorithm>
# include <cassert>
# include <cmath>
# include <cstddef>
# include <limits>
# include <tuple>
# include <utility>

# include <ah-errors.H>
# include <ahFunctional.H>
# include <ahSort.H>
# include <tpl_array.H>
# include <tpl_agraph.H>
# include <tpl_dynDlist.H>
# include <tpl_dynListQueue.H>
# include <tpl_dynListStack.H>

namespace Aleph::blossom_weighted_detail::mwmatching
{
  /* **************************************************
   * **        public definitions                    **
   * ************************************************** */

  /** @brief Type representing the unique ID of a vertex.
   *
   *  Vertices must be indexed by consecutive non-negative integers starting from 0.
   */
  using VertexId = unsigned int;

  /** @brief Type representing a pair of vertices.
   *
   *  Used to represent edges and matched pairs.
   */
  using VertexPair = std::pair<VertexId, VertexId>;


  /** @brief Type representing a weighted edge.
   *
   *  @tparam WeightType Numeric type used to represent edge weights (long long, double, etc.).
   */
  template <typename WeightType>
  struct Edge
  {
    static_assert(std::numeric_limits<WeightType>::is_specialized,
                  "Edge weight must be a numeric type");

    /** @brief Incident vertices (u, v). */
    VertexPair vt;

    /** @brief Edge weight. */
    WeightType weight;

    /** @brief Default constructor. Initializes to (0,0) weight 0. */
    Edge()
      : vt(0, 0), weight(0)
    {}

    /** @brief Construct from a VertexPair and weight. */
    Edge(VertexPair vt, WeightType w)
      : vt(std::move(vt)), weight(w)
    {}

    /** @brief Construct from vertex IDs and weight. */
    Edge(VertexId x, VertexId y, WeightType w)
      : vt(x, y), weight(w)
    {}
  };


  namespace impl
  {
    /* **************************************************
     * **        private definitions                   **
     * ************************************************** */

    /** Value used to mark an invalid or undefined vertex. */
    constexpr VertexId NO_VERTEX = std::numeric_limits<VertexId>::max();


    /** Top-level blossoms may be labeled "S" or "T" or unlabeled. */
    enum BlossomLabel { LABEL_NONE = 0, LABEL_S = 1, LABEL_T = 2 };


    /* **************************************************
     * **        private helper functions              **
     * ************************************************** */

    /** Return a pair of vertices in flipped order. */
    inline VertexPair flip_vertex_pair(const VertexPair & vt)
    {
      return std::make_pair(vt.second, vt.first);
    }


    /**
     * Check that the input is a valid graph.
     *
     * The graph may not have self-edges or multiple edges between
     * the same pair of vertices. Edge weights must be within a valid range.
     *
     * This function takes time O(m * log(m)).
     *
     * @param  edges  Array of weighted edges defining the graph.
     * @throw  std::invalid_argument  If the input graph is not valid.
     */
    template <typename WeightType>
    void check_input_graph(const Array<Edge<WeightType>> & edges)
    {
      constexpr VertexId max_num_vertex = std::numeric_limits<VertexId>::max();

      for (const Edge<WeightType> & edge: edges)
        {
          // Check that vertex IDs are valid.
          ah_invalid_argument_if(edge.vt.first >= max_num_vertex or edge.vt.second >= max_num_vertex)
            << "Vertex ID out of range";

          // Check that the edge weight is a finite number.
          ah_invalid_argument_if(not std::numeric_limits<WeightType>::is_integer and
                                 not std::isfinite(edge.weight))
                << "Edge weights must be finite numbers";

          // Check that edge weight will not cause overflow.
          ah_invalid_argument_if(edge.weight > std::numeric_limits<WeightType>::max() / 4)
            << "Edge weight exceeds maximum supported value";
        }

      // Check that the graph has no self-edges.
      for (const Edge<WeightType> & edge: edges)
        ah_invalid_argument_if(edge.vt.first == edge.vt.second)
            << "Self-edges are not supported";

      // Check that the graph does not contain duplicate edges.
      Array<VertexPair> edge_endpoints;
      edge_endpoints.reserve(edges.size());
      for (const Edge<WeightType> & edge: edges)
        {
          VertexPair vt = edge.vt;
          if (vt.first > vt.second)
            std::swap(vt.first, vt.second);
          edge_endpoints.append(vt);
        }

      in_place_sort(edge_endpoints);
      ah_invalid_argument_if(not all_unique(edge_endpoints))
        << "Duplicate edges are not supported";
    }


    /* **************************************************
     * **        struct Problem_Data                   **
     * ************************************************** */

    /** @brief Representation of the input graph for the internal solver.
     *
     *  Builds an internal Aleph adjacency graph for efficient incident arc iteration.
     */
    template <typename WeightType>
    struct Problem_Data
    {
      using EdgeT = Edge<WeightType>;
      using Internal_Node = Graph_Anode<VertexId>;
      using Internal_Arc = Graph_Aarc<const EdgeT *>;
      using Internal_Graph = Array_Graph<Internal_Node, Internal_Arc>;

      /** @brief Pre-filtered edges (non-negative only). */
      const Array<EdgeT> edges;

      /** @brief Number of vertices in the graph. */
      const VertexId num_vertex;

      /** @brief Adjacency graph used to iterate over incident edges. */
      Internal_Graph adjacent_graph;

      /** @brief ID to node mapping. */
      Array<Internal_Node *> nodes;

      /** @brief Initialize data and build the internal graph. */
      explicit Problem_Data(const Array<EdgeT> & edges_in)
        : edges(remove_negative_weight_edges(edges_in)),
          num_vertex(count_num_vertex(edges)),
          adjacent_graph(),
          nodes()
      {
        build_adjacent_graph();
      }

      // Prevent copying.
      Problem_Data(const Problem_Data &) = delete;

      Problem_Data &operator=(const Problem_Data &) = delete;

      /** @internal Filter edges with negative weight. */
      static Array<EdgeT>
      remove_negative_weight_edges(const Array<EdgeT> & edges_in)
      {
        Array<EdgeT> real_edges;
        real_edges.reserve(edges_in.size());

        for (const Edge<WeightType> & edge: edges_in)
          if (edge.weight >= 0)
            real_edges.append(edge);

        return real_edges;
      }

      /** @internal Count unique vertex IDs in the edge list. */
      static VertexId count_num_vertex(const Array<EdgeT> & edges)
      {
        VertexId num_vertex = 0;
        for (const Edge<WeightType> & edge: edges)
          {
            const VertexId m = std::max(edge.vt.first, edge.vt.second);
            assert(m < std::numeric_limits<VertexId>::max());
            num_vertex = std::max(num_vertex, m + 1);
          }

        return num_vertex;
      }

      /** @internal Construct the adjacency structure. */
      void build_adjacent_graph()
      {
        nodes.reserve(num_vertex);
        for (VertexId i = 0; i < num_vertex; ++i)
          nodes.append(adjacent_graph.insert_node(i));

        for (const EdgeT & edge: edges)
          {
            assert(edge.vt.first < num_vertex);
            assert(edge.vt.second < num_vertex);
            adjacent_graph.insert_arc(nodes[edge.vt.first],
                                      nodes[edge.vt.second],
                                      &edge);
          }
      }

      /** @brief Iterates over edges incident to vertex @p x. */
      template <class Op>
      void for_adjacent_edges(const VertexId x, Op op) const
      {
        assert(x < num_vertex);
        for (Node_Arc_Iterator<Internal_Graph> it(nodes[x]); it.has_curr(); it.next_ne())
          {
            Internal_Arc *arc = it.get_curr();
            op(arc->get_info());
          }
      }
    };


    /* **************************************************
     * **        struct Blossom                        **
     * ************************************************** */

    // Forward declaration.
    template <typename WeightType>
    struct NonTrivialBlossom;


    /** @brief Represents a blossom in the algorithm.
     *
     *  A blossom is either a single vertex (trivial) or an odd-length 
     *  alternating cycle of sub-blossoms.
     *
     *  @tparam WeightType   Numeric type for weights.
     */
    template <typename WeightType>
    struct Blossom
    {
      /** @brief Parent in the blossom hierarchy, or null if top-level. */
      NonTrivialBlossom<WeightType> *parent;

      /** @brief Index of the base vertex (the unmatched vertex in the blossom). */
      VertexId base_vertex;

      /** @brief Label S or T if this is part of the current alternating forest. */
      BlossomLabel label;

      /** @brief Flag to distinguish between Blossom and NonTrivialBlossom. */
      bool is_nontrivial_blossom;

      /** @brief Tree edge attaching this blossom to its parent in the alternating forest. */
      VertexPair tree_edge;

      /** @brief Best edge for dual variable calculation (least slack). */
      const Edge<WeightType> *best_edge;

    protected:
      /** @internal Initialize base class. */
      Blossom(const VertexId base_vertex, const bool is_nontrivial_blossom)
        : parent(nullptr),
          base_vertex(base_vertex),
          label(LABEL_NONE),
          is_nontrivial_blossom(is_nontrivial_blossom),
          best_edge(nullptr)
      {}

    public:
      /** @internal Default constructor. */
      Blossom()
        : Blossom(0, false)
      {}

      /** @internal Initialize a trivial (single-vertex) blossom. */
      explicit Blossom(VertexId base_vertex)
        : Blossom(base_vertex, false)
      {}

      /** @brief Safe downcast to NonTrivialBlossom. */
      NonTrivialBlossom<WeightType> * nontrivial()
      {
        return is_nontrivial_blossom ?
                 static_cast<NonTrivialBlossom<WeightType> *>(this) :
                 nullptr;
      }

      /** @brief Safe downcast to NonTrivialBlossom (const version). */
      const NonTrivialBlossom<WeightType> * nontrivial() const
      {
        return is_nontrivial_blossom ?
                 static_cast<const NonTrivialBlossom<WeightType> *>(this) :
                 nullptr;
      }
    };


    /* **************************************************
     * **        struct NonTrivialBlossom              **
     * ************************************************** */

    /** @brief Represents a non-trivial blossom (an odd cycle of sub-blossoms).
     *
     *  Non-trivial blossoms have associated dual variables in the LPP.
     */
    template <typename WeightType>
    struct NonTrivialBlossom : public Blossom<WeightType>
    {
      struct SubBlossom
      {
        /** Pointer to sub-blossom. */
        Blossom<WeightType> *blossom;

        /** Edge connecting this sub-blossom to the next in the cycle. */
        VertexPair edge;
      };

      /** @brief Ring of sub-blossoms forming this blossom. */
      DynDlist<SubBlossom> subblossoms;

      /** @brief Dual variable value for this blossom. */
      WeightType dual_var;

      /** @internal Edge tracking for dual updates. */
      DynDlist<const Edge<WeightType> *> best_edge_set;

      // Needed by DynDlist internals (sentinel node).
      NonTrivialBlossom()
        : Blossom<WeightType>(0, true),
          dual_var(0)
      {}

      /** @brief Construct a new non-trivial blossom. */
      template <class Blossom_Container, class Edge_Container>
      NonTrivialBlossom(const Blossom_Container & subblossoms,
                        const Edge_Container & edges)
        : Blossom<WeightType>((*subblossoms.begin())->base_vertex, true),
          dual_var(0)
      {
        assert(subblossoms.size() == edges.size());
        assert(subblossoms.size() % 2 == 1);
        assert(subblossoms.size() >= 3);

        auto blossom_it = subblossoms.begin();
        auto edge_it = edges.begin();
        while (blossom_it != subblossoms.end())
          {
            this->subblossoms.append(SubBlossom{});
            this->subblossoms.get_last().blossom = *blossom_it;
            this->subblossoms.get_last().edge = *edge_it;
            ++blossom_it;
            ++edge_it;
          }
      }

      /** @internal Helper to find a subblossom index. */
      VertexId find_subblossom_pos(Blossom<WeightType> *subblossom) const
      {
        VertexId pos = 0;
        for (auto it = subblossoms.get_it(); it.has_curr(); it.next_ne(), ++pos)
          if (it.get_curr().blossom == subblossom)
            return pos;
        assert(false);
        return 0;
      }
    };


    /** @brief Iterate over all base vertices contained within a blossom. */
    template <typename WeightType, typename Func>
    inline void for_vertices_in_blossom(const Blossom<WeightType> *blossom, Func func)
    {
      const NonTrivialBlossom<WeightType> *ntb = blossom->nontrivial();
      if (ntb)
        {
          // Visit all vertices in the non-trivial blossom.
          // Use an explicit stack to avoid deep call chains.
          Array<const NonTrivialBlossom<WeightType> *> stack;
          stack.append(ntb);

          while (not stack.is_empty())
            {
              const NonTrivialBlossom<WeightType> *b = stack.get_last();
              (void) stack.remove_last();

              for (const auto & sub: b->subblossoms)
                {
                  ntb = sub.blossom->nontrivial();
                  if (ntb)
                    stack.append(ntb);
                  else
                    func(sub.blossom->base_vertex);
                }
            }
        }
      else
        {
          // A trivial blossom contains just one vertex.
          func(blossom->base_vertex);
        }
    }


    /** @brief Represents a sequence of edges along an alternating path. */
    struct AlternatingPath
    {
      DynDlist<VertexPair> edges;
    };


    /* **************************************************
     * **        struct MatchingContext                **
     * ************************************************** */

    /** @brief Encapsulates the complete state of the weighted matching algorithm.
     *
     *  Includes dual variables, blossom hierarchy, and the current matching state.
     */
    template <typename WeightType>
    class MatchingContext
    {
    public:
      using EdgeT = Edge<WeightType>;
      using BlossomT = Blossom<WeightType>;
      using NonTrivialBlossomT = NonTrivialBlossom<WeightType>;

      /** @brief A candidate step for dual variable modification. */
      struct DeltaStep
      {
        /** Type of delta step (1-4). */
        int kind;

        /** Value of the dual update. */
        WeightType value;

        /** Edge causing the update (types 2, 3). */
        VertexPair edge;

        /** Blossom causing the update (type 4). */
        NonTrivialBlossomT *blossom;
      };

      /** @brief Integer scaling factor for weight calculations. */
      static constexpr WeightType weight_factor =
          std::numeric_limits<WeightType>::is_integer ? 2 : 1;

      /** @brief Internal graph representation. */
      const Problem_Data<WeightType> graph;

      /** @brief The current mate of each vertex, or NO_VERTEX. */
      Array<VertexId> vertex_mate;

      /** @brief All trivial (single vertex) blossoms. */
      Array<BlossomT> trivial_blossom;

      /** @brief Currently active non-trivial blossoms. */
      DynDlist<NonTrivialBlossomT> nontrivial_blossom;

      /** @brief Maps each vertex to its highest-level containing blossom. */
      Array<BlossomT *> vertex_top_blossom;

      /** @brief Dual variable@f$y_i@f$for each vertex. */
      Array<WeightType> vertex_dual;

      /** @internal Least-slack edge tracker. */
      Array<const EdgeT *> vertex_best_edge;

      /** @internal Alternating tree BFS queue. */
      DynListQueue<VertexId> queue;

      /** @internal Temporary marker array. */
      Array<bool> vertex_marker;

      /** @internal Temporary vertex list. */
      Array<VertexId> marked_vertex;

      /** @brief Initialize context from an edge array. */
      explicit MatchingContext(const Array<EdgeT> & edges_in)
        : graph(edges_in)
      {
        // Initially, all vertices are unmatched.
        vertex_mate.reserve(graph.num_vertex);
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          vertex_mate.append(NO_VERTEX);

        // Create a trivial blossom for each vertex.
        trivial_blossom.reserve(graph.num_vertex);
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          trivial_blossom.append(BlossomT(x));

        // Initially, all vertices are trivial top-level blossoms.
        vertex_top_blossom.reserve(graph.num_vertex);
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          vertex_top_blossom.append(&trivial_blossom[x]);

        // Vertex duals are initialized to half the maximum edge weight.
        WeightType max_weight = 0;
        for (const EdgeT & edge: graph.edges)
          max_weight = std::max(max_weight, edge.weight);

        WeightType init_vertex_dual = max_weight * (weight_factor / 2);
        vertex_dual.reserve(graph.num_vertex);
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          vertex_dual.append(init_vertex_dual);

        // Initialize "vertex_best_edge".
        vertex_best_edge.reserve(graph.num_vertex);
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          vertex_best_edge.append(nullptr);

        // Allocate temporary arrays for path tracing.
        vertex_marker.reserve(graph.num_vertex);
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          vertex_marker.append(false);

        marked_vertex.reserve(graph.num_vertex);
      }

      // Prevent copying.
      MatchingContext(const MatchingContext &) = delete;

      MatchingContext &operator=(const MatchingContext &) = delete;

      /* **********  General support routines:  ********** */

      /** Calculate edge slack. */
      WeightType edge_slack(const EdgeT & edge) const
      {
        VertexId x = edge.vt.first;
        VertexId y = edge.vt.second;
        assert(vertex_top_blossom[x] != vertex_top_blossom[y]);
        return vertex_dual[x] + vertex_dual[y] - weight_factor * edge.weight;
      }

      /*
       * Least-slack edge tracking:
       *
       * To calculate delta steps, the matching algorithm needs to find
       *  - the least-slack edge between any S-vertex and an unlabeled vertex;
       *  - the least-slack edge between any pair of top-level S-blossoms.
       *
       * For each unlabeled vertex and each T-vertex, we keep track of the
       * least-slack edge to any S-vertex. Tracking for unlabeled vertices
       * serves to provide the least-slack edge for the delta step.
       * Tracking for T-vertices is done because such vertices can turn into
       * unlabeled vertices if they are part of a T-blossom that gets expanded.
       *
       * For each top-level S-blossom, we keep track of the least-slack edge
       * to any S-vertex not in the same blossom.
       *
       * Furthermore, for each top-level S-blossom, we keep a list of
       * least-slack edges to other top-level S-blossoms. For any pair of
       * top-level S-blossoms, the least-slack edge between them is contained
       * in the edge list of at least one of the blossoms. An edge list may
       * contain multiple edges to the same S-blossom. Such redundant edges are
       * pruned during blossom merging to limit the number of tracked edges.
       *
       * Note: For a given vertex or blossom, the identity of the least-slack
       * edge to any S-blossom remains unchanged during a delta step.
       * Although the delta step changes edge slacks, it changes the slack
       * of every edge to an S-vertex by the same amount. Therefore the edge
       * that had least slack before the delta step, will still have least slack
       * after the delta step.
       */

      /**
       * Reset least-slack edge tracking.
       *
       * This function takes time O(n + m).
       */
      void lset_reset()
      {
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          vertex_best_edge[x] = nullptr;

        for (BlossomT & blossom: trivial_blossom)
          blossom.best_edge = nullptr;

        for (NonTrivialBlossomT & blossom: nontrivial_blossom)
          {
            blossom.best_edge = nullptr;
            blossom.best_edge_set.empty();
          }
      }

      /**
       * Add edge "e" from an S-vertex to unlabeled vertex or T-vertex "y".
       *
       * This function takes time O(1) per call.
       * This function is called O(m) times per stage.
       */
      void lset_add_vertex_edge(VertexId y, const EdgeT *edge, WeightType slack)
      {
        const EdgeT *cur_best_edge = vertex_best_edge[y];
        if (cur_best_edge == nullptr or slack < edge_slack(*cur_best_edge))
          vertex_best_edge[y] = edge;
      }

      /**
       * Return the index and slack of the least-slack edge between
       * any S-vertex and unlabeled vertex.
       *
       * This function takes time O(n) per call.
       * This function takes total time O(n**2) per stage.
       *
       * @return Pair (edge, slack) if there is a least-slack edge,
       *         or (nullptr, 0) if there is no suitable edge.
       */
      std::pair<const EdgeT *, WeightType> lset_get_best_vertex_edge()
      {
        const EdgeT *best_edge = nullptr;
        WeightType best_slack = 0;

        for (VertexId x = 0; x < graph.num_vertex; ++x)
          if (vertex_top_blossom[x]->label == LABEL_NONE)
            {
              const EdgeT *edge = vertex_best_edge[x];
              if (edge != nullptr)
                {
                  WeightType slack = edge_slack(*edge);
                  if (best_edge == nullptr or slack < best_slack)
                    {
                      best_edge = edge;
                      best_slack = slack;
                    }
                }
            }

        return std::make_pair(best_edge, best_slack);
      }

      /** Start tracking edges for a new S-blossom. */
      static void lset_new_blossom(BlossomT *blossom)
      {
        assert(blossom->best_edge == nullptr);
        assert((blossom->nontrivial() == nullptr)
               or blossom->nontrivial()->best_edge_set.is_empty());
      }

      /**
       * Add edge "e" between the specified S-blossom and another S-blossom.
       *
       * This function takes time O(1) per call.
       * This function is called O(m) times per stage.
       */
      void lset_add_blossom_edge(BlossomT *blossom,
                                 const EdgeT *edge,
                                 WeightType slack)
      {
        const EdgeT *cur_best_edge = blossom->best_edge;
        if (cur_best_edge == nullptr or slack < edge_slack(*cur_best_edge))
          blossom->best_edge = edge;

        if (NonTrivialBlossomT *ntb = blossom->nontrivial())
          ntb->best_edge_set.append(edge);
      }

      /**
       * Update least-slack edge tracking after merging sub-blossoms
       * into a new S-blossom.
       *
       * This function takes total time O(n**2) per stage.
       */
      void lset_merge_blossoms(NonTrivialBlossomT *blossom)
      {
        assert(blossom->best_edge == nullptr);
        assert(blossom->best_edge_set.is_empty());

        // Collect edges from the sub-blossoms that used to be S-blossoms.
        for (auto & subblossom_item: blossom->subblossoms)
          if (BlossomT *sub = subblossom_item.blossom; sub->label == LABEL_S)
            {
              if (NonTrivialBlossomT *ntb = sub->nontrivial())
                {
                  // Take least-slack edge set from this subblossom.
                  blossom->best_edge_set.append(std::move(ntb->best_edge_set));
                }
              else
                {
                  // Trivial blossoms don't maintain a least-slack edge set.
                  // Just consider all incident edges.
                  graph.for_adjacent_edges(sub->base_vertex,
                                           [this,blossom](const EdgeT *edge)
                                             {
                                               // Only take edges between different S-blossoms.
                                               VertexId x = edge->vt.first;
                                               VertexId y = edge->vt.second;
                                               BlossomT *bx = vertex_top_blossom[x];
                                               BlossomT *by = vertex_top_blossom[y];
                                               if (bx != by
                                                   and (bx->label == LABEL_S)
                                                   and (by->label == LABEL_S))
                                                 {
                                                   blossom->best_edge_set.append(edge);
                                                 }
                                             });
                }
            }

        // Build a temporary array holding the least-slack edge index to
        // each top-level S-blossom. This array is indexed by the base vertex
        // of the blossoms.
        Array<std::pair<const EdgeT *, WeightType>> best_edge_to_blossom;
        best_edge_to_blossom.reserve(graph.num_vertex);
        for (VertexId i = 0; i < graph.num_vertex; ++i)
          best_edge_to_blossom.append(std::make_pair(nullptr, WeightType{}));

        for (const EdgeT *edge: blossom->best_edge_set)
          {
            BlossomT *bx = vertex_top_blossom[edge->vt.first];
            BlossomT *by = vertex_top_blossom[edge->vt.second];
            assert(bx == blossom or by == blossom);

            // Ignore internal edges.
            if (bx != by)
              {
                bx = (bx == blossom) ? by : bx;

                // Only consider edges to S-blossoms.
                if (bx->label == LABEL_S)
                  {
                    // Keep only the least-slack edge to blossom "bx".
                    WeightType slack = edge_slack(*edge);
                    VertexId bx_base = bx->base_vertex;
                    if (auto & best_edge_item = best_edge_to_blossom[bx_base]; (best_edge_item.first == nullptr)
                                                                               or (slack < best_edge_item.second))
                      {
                        best_edge_item.first = edge;
                        best_edge_item.second = slack;
                      }
                  }
              }
          };

        // Rebuild a compact list of least-slack edges.
        // Also find the overall least-slack edge to any other S-blossom.
        WeightType best_slack = 0;
        blossom->best_edge_set.empty();
        for (auto & best_edge_item: best_edge_to_blossom)
          {
            const EdgeT *edge = best_edge_item.first;
            if (edge != nullptr)
              {
                blossom->best_edge_set.append(edge);
                WeightType slack = best_edge_item.second;
                if (blossom->best_edge == nullptr or slack < best_slack)
                  {
                    blossom->best_edge = edge;
                    best_slack = slack;
                  }
              }
          }
      }

      /**
       * Return the index and slack of the least-slack edge between
       * any pair of top-level S-blossoms.
       *
       * This function takes time O(n) per call.
       * This function takes total time O(n**2) per stage.
       *
       * @return Pair (edge, slack) if there is a least-slack edge,
       *         or (nullptr, 0) if there is no suitable edge.
       */
      std::pair<const EdgeT *, WeightType> lset_get_best_blossom_edge()
      {
        const EdgeT *best_edge = nullptr;
        WeightType best_slack = 0;

        auto consider_blossom = [this,&best_edge,&best_slack](BlossomT *blossom)
          {
            if (blossom->parent == nullptr and (blossom->label == LABEL_S))
              {
                const EdgeT *edge = blossom->best_edge;
                if (edge != nullptr)
                  {
                    WeightType slack = edge_slack(*edge);
                    if (best_edge == nullptr or (slack < best_slack))
                      {
                        best_edge = edge;
                        best_slack = slack;
                      }
                  }
              }
          };

        for (BlossomT & blossom: trivial_blossom)
          consider_blossom(&blossom);
        for (BlossomT & blossom: nontrivial_blossom)
          consider_blossom(&blossom);

        return std::make_pair(best_edge, best_slack);
      }

      /* **********  Creating and expanding blossoms:  ********** */

      /**
       * Trace back through the alternating trees from vertices "x" and "y".
       *
       * If both vertices are part of the same alternating tree, this function
       * discovers a new blossom. In this case it returns an alternating path
       * through the blossom that starts and ends in the same sub-blossom.
       *
       * If the vertices are part of different alternating trees, this function
       * discovers an augmenting path. In this case it returns an alternating
       * path that starts and ends in an unmatched vertex.
       *
       * This function takes time O(k) to discover a blossom, where "k" is the
       * number of sub-blossoms, or time O(n) to discover an augmenting path.
       */
      AlternatingPath trace_alternating_paths(VertexId x, VertexId y)
      {
        assert(vertex_top_blossom[x] != vertex_top_blossom[y]);

        // Initialize a path containing only the edge (x, y).
        AlternatingPath path;
        path.edges.append(std::make_pair(x, y));

        // "first_common" is the first common ancestor of "x" and "y"
        // in the alternating tree, or "nullptr" if no common ancestor
        // has been found.
        BlossomT *first_common = nullptr;

        // Alternate between tracing the path from "x" and the path from "y".
        // This ensures that the search time is bounded by the size of any
        // newly found blossom.
        while (x != NO_VERTEX or y != NO_VERTEX)
          {
            // Trace path from vertex "x".
            if (x != NO_VERTEX)
              {
                // Stop if we found a common ancestor.
                BlossomT *bx = vertex_top_blossom[x];
                if (vertex_marker[bx->base_vertex])
                  {
                    first_common = bx;
                    break;
                  }

                // Mark blossom as potential common ancestor.
                vertex_marker[bx->base_vertex] = true;
                marked_vertex.append(bx->base_vertex);

                // Trace back to the parent in the alternating tree.
                x = bx->tree_edge.first;
                if (x != NO_VERTEX)
                  // While tracing from x, prepend so the left half of the path
                  // remains in forward order.
                  path.edges.insert(bx->tree_edge);
              }

            // Trace path from vertex "y".
            if (y != NO_VERTEX)
              {
                // Stop if we found a common ancestor.
                BlossomT *by = vertex_top_blossom[y];
                if (vertex_marker[by->base_vertex])
                  {
                    first_common = by;
                    break;
                  }

                // Mark blossom as potential common ancestor.
                vertex_marker[by->base_vertex] = true;
                marked_vertex.append(by->base_vertex);

                // Trace back to the parent in the alternating tree.
                y = by->tree_edge.first;
                if (y != NO_VERTEX)
                  // While tracing from y, append so the right half stays in
                  // forward order from left to right.
                  path.edges.append(std::make_pair(by->tree_edge.second, y));
              }
          }

        // Remove all markers we placed.
        for (const VertexId k: marked_vertex)
          vertex_marker[k] = false;
        marked_vertex.empty();

        // If we found a common ancestor, trim the paths so they end there.
        if (first_common)
          {
            assert(first_common->label == LABEL_S);
            while (vertex_top_blossom[path.edges.get_first().first] != first_common)
              (void) path.edges.remove_first();
            while (vertex_top_blossom[path.edges.get_last().second] != first_common)
              (void) path.edges.remove_last();
          }

        // Any alternating path between S-blossoms must have odd length.
        assert(path.edges.size() % 2 == 1);

        return path;
      }

      /**
       * Create a new blossom from an alternating cycle.
       *
       * Assign label S to the new blossom.
       * Relabel all T-sub-blossoms as S and add their vertices to the queue.
       *
       * This function takes total time O(n**2) per stage.
       */
      void make_blossom(const AlternatingPath & path)
      {
        assert(path.edges.size() % 2 == 1);
        assert(path.edges.size() >= 3);

        // First pass:
        // Build the ordered ring of sub-blossoms from edge.first.
        // We keep this pass separate from the cycle check below to keep the
        // first/second roles explicit and easy to audit.
        Array<BlossomT *> subblossoms;
        subblossoms.reserve(path.edges.size());
        for (VertexPair edge: path.edges)
          subblossoms.append(vertex_top_blossom[edge.first]);

        // Second pass:
        // Validate that each edge.second lands in the next sub-blossom in the
        // ring (with wrap-around at the end).
        VertexId pos = 0;
        for (VertexPair edge: path.edges)
          {
            pos = (pos + 1) % subblossoms.size();
            assert(vertex_top_blossom[edge.second] == subblossoms[pos]);
          }

        // Create the new blossom object.
        nontrivial_blossom.append(NonTrivialBlossomT(subblossoms, path.edges));
        NonTrivialBlossomT *blossom = &nontrivial_blossom.get_last();

        // Link the subblossoms to their new parent.
        for (BlossomT *sub: subblossoms)
          sub->parent = blossom;

        // Mark vertices as belonging to the new blossom.
        for_vertices_in_blossom(blossom, [this,blossom](VertexId x)
                                  {
                                    vertex_top_blossom[x] = blossom;
                                  });

        // Assign label S to the new blossom and link to the alternating tree.
        assert(subblossoms.get_first()->label == LABEL_S);
        blossom->label = LABEL_S;
        blossom->tree_edge = subblossoms.get_first()->tree_edge;

        // Consider vertices inside former T-sub-blossoms which now
        // became S-vertices; add them to the queue.
        for (BlossomT *sub: subblossoms)
          if (sub->label == LABEL_T)
            for_vertices_in_blossom(sub, [this](VertexId x)
                                      {
                                        queue.put(x);
                                      });

        // Merge least-slack edges for the S-sub-blossoms.
        lset_merge_blossoms(blossom);
      }

      /** Erase the specified non-trivial blossom. */
      void erase_nontrivial_blossom(NonTrivialBlossomT *blossom)
      {
        for (auto it = nontrivial_blossom.get_it(); it.has_curr(); it.next_ne())
          if (&it.get_curr() == blossom)
            {
              (void) it.del();
              return;
            }
        assert(false);
      }

      /**
       * Expand the specified T-blossom.
       *
       * This function takes time O(n).
       */
      void expand_t_blossom(NonTrivialBlossomT *blossom)
      {
        assert(blossom->parent == nullptr);
        assert(blossom->label == LABEL_T);

        // Convert sub-blossoms into top-level blossoms.
        for (const auto & sub: blossom->subblossoms)
          {
            BlossomT *sub_blossom = sub.blossom;
            assert(sub_blossom->parent == blossom);
            assert(sub_blossom->label == LABEL_NONE);
            sub_blossom->parent = nullptr;
            for_vertices_in_blossom(sub_blossom,
                                    [this,sub_blossom](VertexId x)
                                      {
                                        vertex_top_blossom[x] = sub_blossom;
                                      });
          }

        // The expanded blossom was part of an alternating tree.
        // We must now reconstruct the part of the alternating tree
        // that ran through this blossom by linking some of the sub-blossoms
        // into the tree.

        // Find the sub-blossom that was attached to the parent node
        // in the alternating tree.
        BlossomT *entry = vertex_top_blossom[blossom->tree_edge.second];

        // Assign label T to that blossom and link to the alternating tree.
        entry->label = LABEL_T;
        entry->tree_edge = blossom->tree_edge;

        // Find the position of this sub-blossom within the expanding blossom.
        const VertexId entry_pos = blossom->find_subblossom_pos(entry);
        auto sub_it = blossom->subblossoms.get_it();
        for (VertexId i = 0; i < entry_pos; ++i)
          sub_it.next_ne();

        if (entry_pos % 2 == 0)
          {
            // Walk backward to the base.
            while (sub_it.get_pos() != 0)
              {
                sub_it.prev();
                assign_label_s(sub_it.get_curr().edge.first);

                assert(sub_it.get_pos() != 0);
                sub_it.prev();
                auto & cur = sub_it.get_curr();
                cur.blossom->label = LABEL_T;
                cur.blossom->tree_edge = flip_vertex_pair(cur.edge);
              }
          }
        else
          {
            // Walk forward to the base.
            while (sub_it.has_curr())
              {
                assign_label_s(sub_it.get_curr().edge.second);
                sub_it.next();

                assert(sub_it.has_curr());
                VertexPair tree_edge = sub_it.get_curr().edge;
                sub_it.next();

                BlossomT *sub_blossom = sub_it.has_curr() ?
                                          sub_it.get_curr().blossom :
                                          blossom->subblossoms.get_first().blossom;
                sub_blossom->label = LABEL_T;
                sub_blossom->tree_edge = tree_edge;
              }
          }

        // Delete the expanded blossom.
        erase_nontrivial_blossom(blossom);
      }

      /**
       * Expand the specified unlabeled blossom.
       *
       * This function takes time O(n).
       */
      void expand_unlabeled_blossom(NonTrivialBlossomT *blossom)
      {
        assert(blossom->parent == nullptr);
        assert(blossom->label == LABEL_NONE);

        // Convert sub-blossoms into top-level blossoms.
        for (const auto & sub: blossom->subblossoms)
          {
            BlossomT *sub_blossom = sub.blossom;
            assert(sub_blossom->parent == blossom);
            assert(sub_blossom->label == LABEL_NONE);
            sub_blossom->parent = nullptr;
            for_vertices_in_blossom(sub_blossom,
                                    [this,sub_blossom](VertexId x)
                                      {
                                        vertex_top_blossom[x] = sub_blossom;
                                      });
          }

        // Delete the expanded blossom.
        erase_nontrivial_blossom(blossom);
      }

      /* **********  Augmenting:  ********** */

      /**
       * Augment along an alternating path through the specified blossom,
       * from sub-blossom "entry" to the base vertex of the blossom.
       *
       * Modify the blossom to reflect that sub-blossom "entry" contains
       * the base vertex after augmenting.
       */
      void augment_blossom_rec(NonTrivialBlossomT *blossom,
                               BlossomT *entry,
                               DynListStack<std::pair<NonTrivialBlossomT *, BlossomT *>> & rec_stack)
      {
        const VertexId entry_pos = blossom->find_subblossom_pos(entry);
        auto sub_it = blossom->subblossoms.get_it();
        for (VertexId i = 0; i < entry_pos; ++i)
          sub_it.next_ne();
        while ((sub_it.get_pos() != 0) and sub_it.has_curr())
          {
            VertexId x, y;
            BlossomT *bx = nullptr;
            BlossomT *by = nullptr;

            if (entry_pos % 2 == 0)
              {
                // Walk backward to the base.
                sub_it.prev();
                by = sub_it.get_curr().blossom;
                assert(sub_it.get_pos() != 0);
                sub_it.prev();
                bx = sub_it.get_curr().blossom;
                std::tie(x, y) = sub_it.get_curr().edge;
              }
            else
              {
                // Walk forward to the base.
                sub_it.next();
                assert(sub_it.has_curr());
                std::tie(x, y) = sub_it.get_curr().edge;
                bx = sub_it.get_curr().blossom;
                sub_it.next();
                by = sub_it.has_curr() ?
                       sub_it.get_curr().blossom :
                       blossom->subblossoms.get_first().blossom;
              }

            // Pull this edge into the matching.
            vertex_mate[x] = y;
            vertex_mate[y] = x;

            // Augment through any non-trivial subblossoms touching this edge.
            NonTrivialBlossomT *bx_ntb = bx->nontrivial();
            if (bx_ntb != nullptr)
              rec_stack.put(std::make_pair(bx_ntb, &trivial_blossom[x]));

            NonTrivialBlossomT *by_ntb = by->nontrivial();
            if (by_ntb != nullptr)
              rec_stack.put(std::make_pair(by_ntb, &trivial_blossom[y]));
          }

        // Re-orient the blossom (rotate so "entry" is first).
        if (entry_pos != 0)
          {
            Array<typename NonTrivialBlossomT::SubBlossom> ring;
            ring.reserve(blossom->subblossoms.size());
            for (auto it = blossom->subblossoms.get_it(); it.has_curr(); it.next_ne())
              ring.append(it.get_curr());
            const size_t n = ring.size();
            DynDlist<typename NonTrivialBlossomT::SubBlossom> rotated;
            for (size_t k = 0; k < n; ++k)
              rotated.append(ring[(entry_pos + k) % n]);
            blossom->subblossoms.swap(rotated);
          }

        // Update the base vertex.
        // We can pull the new base vertex from the entry sub-blossom
        // since its augmentation has already finished.
        blossom->base_vertex = entry->base_vertex;
      }

      /**
       * Augment along an alternating path through the specified blossom,
       * from sub-blossom "entry" to the base vertex of the blossom.
       *
       * Recursively handle sub-blossoms as needed.
       *
       * This function takes time O(n).
       */
      void augment_blossom(NonTrivialBlossomT *blossom, BlossomT *entry)
      {
        // Use an explicit stack to avoid deep recursion.
        DynListStack<std::pair<NonTrivialBlossomT *, BlossomT *>> rec_stack;
        rec_stack.put(std::make_pair(blossom, entry));

        while (not rec_stack.is_empty())
          {
            NonTrivialBlossomT *outer_blossom;
            BlossomT *inner_entry;
            std::tie(outer_blossom, inner_entry) = rec_stack.top();

            NonTrivialBlossomT *inner_blossom = inner_entry->parent;
            assert(inner_blossom != nullptr);

            if (inner_blossom != outer_blossom)
              {
                // After augmenting "inner_blossom",
                // continue by augmenting its parent.
                rec_stack.top() = std::make_pair(outer_blossom, inner_blossom);
              }
            else
              {
                // After augmenting "inner_blossom",
                // this entire "outer_blossom" will be finished.
                (void) rec_stack.get();
              }

            // Augment "inner_blossom".
            augment_blossom_rec(inner_blossom, inner_entry, rec_stack);
          }
      }

      /**
       * Augment the matching through the specified augmenting path.
       *
       * This function takes time O(n).
       */
      void augment_matching(const AlternatingPath & path)
      {
        // Check that the path starts and ends in an unmatched blossom.
        assert(path.edges.size() % 2 == 1);
        assert(vertex_mate[vertex_top_blossom[path.edges.get_first().first]->base_vertex] == NO_VERTEX);
        assert(vertex_mate[vertex_top_blossom[path.edges.get_last().second]->base_vertex] == NO_VERTEX);

        // Process the unmatched edges on the augmenting path.
        auto edge_it = path.edges.begin();
        auto edge_end = path.edges.end();
        while (edge_it != edge_end)
          {
            VertexId x = edge_it->first;
            VertexId y = edge_it->second;

            // Augment any non-trivial blossoms that touch this edge.
            BlossomT *bx = vertex_top_blossom[x];
            NonTrivialBlossomT *bx_ntb = bx->nontrivial();
            if (bx_ntb != nullptr)
              augment_blossom(bx_ntb, &trivial_blossom[x]);

            BlossomT *by = vertex_top_blossom[y];
            NonTrivialBlossomT *by_ntb = by->nontrivial();
            if (by_ntb != nullptr)
              augment_blossom(by_ntb, &trivial_blossom[y]);

            // Pull this edge into the matching.
            vertex_mate[x] = y;
            vertex_mate[y] = x;

            // Move forward through the augmenting path to
            // the next edge to be matched.
            // Edges along an augmenting path alternate unmatched/matched;
            // only unmatched edges are pulled into the new matching.
            ++edge_it;
            if (edge_it == edge_end)
              break;
            ++edge_it;
          }
      }

      /* **********  Labels and alternating tree:  ********** */

      /**
       * Assign label S to the unlabeled blossom that contains vertex "x".
       *
       * If vertex "x" is matched, it becomes attached to the alternating tree
       * via its matched edge. If vertex "x" is unmatched, it becomes the root
       * of an alternating tree.
       *
       * All vertices in the newly labeled blossom are added to the scan queue.
       *
       * @pre "x" is an unlabeled vertex, either unmatched or matched to
       *      a T-vertex via a tight edge.
       */
      void assign_label_s(VertexId x)
      {
        // Assign label S to the blossom that contains vertex "x".
        BlossomT *bx = vertex_top_blossom[x];
        assert(bx->label == LABEL_NONE);
        bx->label = LABEL_S;

        VertexId y = vertex_mate[x];
        if (y == NO_VERTEX)
          {
            // Vertex "x" is unmatched.
            // It must be either a top-level vertex or the base vertex of
            // a top-level blossom.
            assert(bx->base_vertex == x);

            // Mark the blossom as root of an alternating tree.
            bx->tree_edge = std::make_pair(NO_VERTEX, x);
          }
        else
          {
            // Vertex "x" is matched to T-vertex "y".
            assert(vertex_top_blossom[y]->label == LABEL_T);

            // Attach the blossom to the alternating tree via vertex "y".
            bx->tree_edge = std::make_pair(y, x);
          }

        // Start least-slack edge tracking for the S-blossom.
        lset_new_blossom(bx);

        // Add all vertices inside the newly labeled S-blossom to the queue.
        for_vertices_in_blossom(bx, [this](VertexId v)
                                  {
                                    queue.put(v);
                                  });
      }

      /**
       * Assign label T to the unlabeled blossom that contains vertex "y".
       *
       * Attach it to the alternating tree via edge (x, y).
       * Then immediately assign label S to the mate of vertex "y".
       *
       * Note that this function may expand blossoms that contain vertex "y".
       *
       * @pre "x" is an S-vertex.
       * @pre "y" is an unlabeled, matched vertex.
       * @pre There is a tight edge between vertices "x" and "y".
       */
      void assign_label_t(VertexId x, VertexId y)
      {
        assert(vertex_top_blossom[x]->label == LABEL_S);

        BlossomT *by = vertex_top_blossom[y];
        assert(by->label == LABEL_NONE);

        // If "y" is part of a zero-dual blossom, expand it.
        // This would otherwise likely happen through a zero-delta4 step,
        // so we can just do it now and avoid a substage.
        NonTrivialBlossomT *ntb = by->nontrivial();
        while (ntb != nullptr and ntb->dual_var == 0)
          {
            expand_unlabeled_blossom(ntb);
            by = vertex_top_blossom[y];
            assert(by->label == LABEL_NONE);
            ntb = by->nontrivial();
          }

        // Assign label T to the top-level blossom that contains vertex "y".
        by->label = LABEL_T;
        by->tree_edge = std::make_pair(x, y);

        // Assign label S to the blossom that is mated to the T-blossom.
        const VertexId z = vertex_mate[by->base_vertex];
        assert(z != NO_VERTEX);
        assign_label_s(z);
      }

      /**
       * Add the edge between S-vertices "x" and "y".
       *
       * If the edge connects blossoms that are part of the same alternating
       * tree, this function creates a new S-blossom and returns false.
       *
       * If the edge connects two different alternating trees, an augmenting
       * path has been discovered. In this case the matching is augmented.
       *
       * @return True if the matching was augmented; otherwise false.
       */
      bool add_s_to_s_edge(VertexId x, VertexId y)
      {
        assert(vertex_top_blossom[x]->label == LABEL_S);
        assert(vertex_top_blossom[y]->label == LABEL_S);

        // Trace back through the alternating trees from "x" and "y".
        AlternatingPath path = trace_alternating_paths(x, y);

        // If the path is a cycle, create a new blossom.
        // Otherwise the path is an augmenting path.
        // Note that an alternating starts and ends in the same blossom,
        // but not necessarily in the same vertex within that blossom.
        VertexId p = path.edges.get_first().first;
        VertexId q = path.edges.get_last().second;
        if (vertex_top_blossom[p] == vertex_top_blossom[q])
          {
            make_blossom(path);
            return false;
          }
        augment_matching(path);
        return true;
      }

      /**
       * Scan queued S-vertices to expand the alternating trees.
       *
       * The scan proceeds until either an augmenting path is found,
       * or the queue of S-vertices becomes empty.
       * If an augmenting path is found, it is used to augment the matching.
       *
       * New blossoms may be created during the scan.
       *
       * @return True if the matching was augmented; otherwise false.
       */
      bool substage_scan()
      {
        // Process the queue of S-vertices to be scanned.
        // This loop runs through O(n) iterations per stage.
        while (not queue.is_empty())
          {
            // Take one vertex from the queue.
            VertexId x = queue.get();

            assert(vertex_top_blossom[x]->label == LABEL_S);

            // Scan the edges that are incident on "x".
            // This loop runs through O(m) iterations per stage.
            bool found_augmenting = false;
            graph.for_adjacent_edges(x, [this,x,&found_augmenting](const EdgeT *edge)
                                       {
                                         if (found_augmenting)
                                           return;

                                         // "x" is one endpoint of this incident edge;
                                         // recover the opposite endpoint.
                                         VertexId y = (edge->vt.first != x) ? edge->vt.first : edge->vt.second;

                                         // Note: The top-level blossom of vertex "x" may change
                                         // during this loop, so we need to refresh it in each pass.
                                         BlossomT *bx = vertex_top_blossom[x];

                                         // Ignore edges that are internal to a blossom.
                                         if (bx == vertex_top_blossom[y])
                                           return;

                                         const BlossomLabel ylabel = vertex_top_blossom[y]->label;

                                         // Check whether this edge is tight (has zero slack).
                                         // Only tight edges may be part of an alternating tree.
                                         WeightType slack = edge_slack(*edge);
                                         if (slack <= 0)
                                           {
                                             if (ylabel == LABEL_NONE)
                                               {
                                                 // Found a tight edge to an unlabeled blossom.
                                                 // Assign label T to the blossom that contains "y".
                                                 assign_label_t(x, y);
                                               }
                                             else if (ylabel == LABEL_S)
                                               {
                                                 // Found a tight edge between two S-blossoms.
                                                 // Find either a new blossom or an augmenting path.
                                                 if (add_s_to_s_edge(x, y))
                                                   found_augmenting = true;
                                               }
                                           }
                                         else if (ylabel == LABEL_S)
                                           {
                                             // Found a non-tight edge between two S-blossoms.
                                             // Pass it to the least-slack edge tracker.
                                             lset_add_blossom_edge(bx, edge, slack);
                                           }

                                         if (ylabel != LABEL_S)
                                           {
                                             // Found an to a T-vertex or unlabeled vertex "y".
                                             // Pass it to the least-slack edge tracker.
                                             // Tight edges must also be tracked in this way.
                                             lset_add_vertex_edge(y, edge, slack);
                                           }
                                       });

            if (found_augmenting)
              return true;
          }

        // No further S vertices to scan, and no augmenting path found.
        return false;
      }

      /* **********  Delta steps:  ********** */

      /**
       * Calculate a delta step in the dual LPP problem.
       *
       * This function returns the minimum of the 4 types of delta values,
       * and the type of delta which obtain the minimum, and the edge or
       * blossom that produces the minimum delta, if applicable.
       *
       * This function takes time O(n).
       *
       * @pre There is at least one S-vertex.
       *
       * @return Tuple (delta_type, delta_value, delta_edge, delta_blossom)
       */
      DeltaStep substage_calc_dual_delta()
      {
        DeltaStep delta;
        delta.blossom = nullptr;

        // Compute delta1: minimum dual variable of any S-vertex.
        delta.kind = 1;
        delta.value = std::numeric_limits<WeightType>::max();
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          if (vertex_top_blossom[x]->label == LABEL_S)
            delta.value = std::min(delta.value, vertex_dual[x]);

        // Compute delta2: minimum slack of any edge between an S-vertex and
        // an unlabeled vertex.
        const EdgeT *edge;
        WeightType slack;
        std::tie(edge, slack) = lset_get_best_vertex_edge();
        if (edge != nullptr and slack <= delta.value)
          {
            delta.kind = 2;
            delta.value = slack;
            delta.edge = edge->vt;
          }

        // Compute delta3: half minimum slack of any edge between two
        // top-level S-blossoms.
        std::tie(edge, slack) = lset_get_best_blossom_edge();
        if (edge != nullptr and slack / 2 <= delta.value)
          {
            delta.kind = 3;
            delta.value = slack / 2;
            delta.edge = edge->vt;
          }

        // Compute delta4: half minimum dual of a top-level T-blossom.
        for (NonTrivialBlossomT & blossom: nontrivial_blossom)
          if (not blossom.parent and blossom.label == LABEL_T)
            if (blossom.dual_var / 2 <= delta.value)
              {
                delta.kind = 4;
                delta.value = blossom.dual_var / 2;
                delta.blossom = &blossom;
              }

        return delta;
      }

      /** Apply a delta step to the dual LPP variables. */
      void substage_apply_delta_step(WeightType delta)
      {
        // Apply delta to dual variables of all vertices.
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          if (const BlossomLabel xlabel = vertex_top_blossom[x]->label; xlabel == LABEL_S)
            {
              // S-vertex: subtract delta from dual variable.
              vertex_dual[x] -= delta;
            }
          else if (xlabel == LABEL_T)
            {
              // T-vertex: add delta to dual variable.
              vertex_dual[x] += delta;
            }

        // Apply delta to dual variables of top-level non-trivial blossoms.
        for (NonTrivialBlossomT & blossom: nontrivial_blossom)
          if (blossom.parent == nullptr)
            {
              if (blossom.label == LABEL_S)
                {
                  // S-blossom: add 2*delta to dual variable.
                  blossom.dual_var += 2 * delta;
                }
              else if (blossom.label == LABEL_T)
                {
                  // T-blossom: subtract 2*delta from dual variable.
                  blossom.dual_var -= 2 * delta;
                }
            }
      }

      /* **********  Main algorithm:  ********** */

      /**
       * Reset data which are only valid during a stage.
       *
       * Mark all blossoms as unlabeled, clear the queue,
       * reset least-slack edge tracking.
       */
      void reset_stage()
      {
        // Remove blossom labels.
        for (BlossomT & blossom: trivial_blossom)
          blossom.label = LABEL_NONE;
        for (BlossomT & blossom: nontrivial_blossom)
          blossom.label = LABEL_NONE;

        // Clear the S-vertex queue.
        queue.clear();

        // Reset least-slack edge tracking.
        lset_reset();
      }

      /**
       * Run one stage of the matching algorithm.
       *
       * The stage searches a maximum-weight augmenting path.
       * If this path is found, it is used to augment the matching,
       * thereby increasing the number of matched edges by 1.
       * If no such path is found, the matching must already be optimal.
       *
       * This function takes time O(n**2).
       *
       * @return True if the matching was successfully augmented;
       *         false if no further improvement is possible.
       */
      bool run_stage()
      {
        // Assign label S to all unmatched vertices and put them in the queue.
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          if (vertex_mate[x] == NO_VERTEX)
            assign_label_s(x);

        // Stop if all vertices are matched.
        // No further improvement is possible in that case.
        // This avoids messy calculations of delta steps without any S-vertex.
        if (queue.is_empty())
          return false;

        // Each pass through the following loop is a "substage".
        // The substage tries to find an augmenting path.
        // If an augmenting path is found, we augment the matching and end
        // the stage. Otherwise we update the dual LPP problem and enter the
        // next substage, or stop if no further improvement is possible.
        //
        // This loop runs through at most O(n) iterations per stage.
        bool augmented = false;
        while (true)
          {
            // Grow alternating trees.
            // End the stage if an augmenting path is found.
            augmented = substage_scan();
            if (augmented)
              break;

            // Calculate delta step in the dual LPP problem.
            DeltaStep delta = substage_calc_dual_delta();

            // Apply the delta step to the dual variables.
            substage_apply_delta_step(delta.value);

            if (delta.kind == 2)
              {
                // Use the edge from S-vertex to unlabeled vertex that got
                // unlocked through the delta update.
                VertexId x = delta.edge.first;
                VertexId y = delta.edge.second;
                if (vertex_top_blossom[x]->label != LABEL_S)
                  std::swap(x, y);
                assign_label_t(x, y);
              }
            else if (delta.kind == 3)
              {
                // Use the S-to-S edge that got unlocked by the delta update.
                // This may reveal an augmenting path.
                VertexId x = delta.edge.first;
                VertexId y = delta.edge.second;
                augmented = add_s_to_s_edge(x, y);
                if (augmented)
                  break;
              }
            else if (delta.kind == 4)
              {
                // Expand the T-blossom that reached dual value 0 through
                // the delta update.
                assert(delta.blossom);
                expand_t_blossom(delta.blossom);
              }
            else
              {
                // No further improvement possible. End the stage.
                assert(delta.kind == 1);
                break;
              }
          }

        // Remove all labels, clear queue.
        reset_stage();

        // Return True if the matching was augmented.
        return augmented;
      }

      /** Run the matching algorithm. */
      void run()
      {
        // Improve the solution until no further improvement is possible.
        //
        // Each successful pass through this loop increases the number
        // of matched edges by 1.
        //
        // This loop runs through at most (n/2 + 1) iterations.
        // Each iteration takes time O(n**2).
        while (run_stage());
      }
    };


    /* **************************************************
     * **          struct MatchingVerifier             **
     * ************************************************** */

    /** Helper class to verify that an optimal solution has been found. */
    template <typename WeightType>
    class MatchingVerifier
    {
    public:
      MatchingVerifier(const MatchingContext<WeightType> & ctx)
        : ctx(ctx),
          graph(ctx.graph),
          edge_duals()
      {
        edge_duals.reserve(ctx.graph.edges.size());
        for (size_t i = 0; i < ctx.graph.edges.size(); ++i)
          edge_duals.append(0);
      }

      /**
       * Verify that the optimum solution has been found.
       *
       * This function takes time O(n**2).
       *
       * @return True if the solution is optimal; otherwise false.
       */
      bool verify()
      {
        return verify_vertex_mate()
               and verify_vertex_duals()
               and verify_blossom_duals()
               and verify_blossoms_and_calc_edge_duals()
               and verify_edge_slack();
      }

    private:
      using EdgeT = Edge<WeightType>;
      using BlossomT = Blossom<WeightType>;
      using NonTrivialBlossomT = NonTrivialBlossom<WeightType>;

      static bool checked_add(WeightType & result, WeightType a, WeightType b)
      {
        if (a > std::numeric_limits<WeightType>::max() - b)
          return true;
        result = a + b;
        return false;
      }

      /** Convert edge pointer to its index in the array "edges". */
      std::size_t edge_index(const EdgeT *edge)
      {
        return edge - &graph.edges.base();
      }

      /** Check that the array "vertex_mate" is consistent. */
      bool verify_vertex_mate()
      {
        // Count matched vertices and check symmetry of "vertex_mate".
        VertexId num_matched_vertex = 0;
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          {
            VertexId y = ctx.vertex_mate[x];
            if (y != NO_VERTEX)
              {
                ++num_matched_vertex;
                if (ctx.vertex_mate[y] != x)
                  return false;
              }
          }

        // Count matched edges.
        VertexId num_matched_edge = 0;
        for (const EdgeT & edge: graph.edges)
          if (ctx.vertex_mate[edge.vt.first] == edge.vt.second)
            ++num_matched_edge;

        // Check that all matched vertices correspond to matched edges.
        return (num_matched_vertex == 2 * num_matched_edge);
      }

      /**
       * Check that vertex dual variables are non-negative,
       * and all unmatched vertices have zero dual.
       */
      bool verify_vertex_duals()
      {
        for (VertexId x = 0; x < graph.num_vertex; ++x)
          {
            if (ctx.vertex_dual[x] < 0)
              return false;
            if (ctx.vertex_mate[x] == NO_VERTEX and ctx.vertex_dual[x] != 0)
              return false;
          }
        return true;
      }

      /** Check that blossom dual variables are non-negative. */
      bool verify_blossom_duals()
      {
        for (const NonTrivialBlossomT & blossom: ctx.nontrivial_blossom)
          if (blossom.dual_var < 0)
            return false;
        return true;
      }

      /**
       * Helper function for verifying the solution.
       *
       * Descend down the blossom tree to find edges that are contained
       * in blossoms.
       *
       * On the way down, keep track of the sum of dual variables of
       * containing blossoms. Add blossom duals to edges that are contained
       * inside blossoms.
       *
       * On the way up, keep track of the total number of matched edges
       * in subblossoms. Check that all blossoms with non-zero dual variables
       * are "full".
       *
       * @return True if successful;
       *         false if a blossom with non-zero dual is not full;
       *         false if blossom dual calculations cause numeric overflow.
       */
      bool check_blossom(const NonTrivialBlossomT *blossom)
      {
        // For each vertex "x",
        // "vertex_depth[x]" is the depth of the smallest blossom on
        // the current descent path that contains "x".
        Array<VertexId> vertex_depth;
        vertex_depth.reserve(graph.num_vertex);
        for (VertexId i = 0; i < graph.num_vertex; ++i)
          vertex_depth.append(0);

        // At each depth, keep track of the sum of blossom duals
        // along the current descent path.
        Array<WeightType> path_sum_dual;
        path_sum_dual.append(0);

        // At each depth, keep track of the number of matched edges
        // along the current ascent path.
        Array<VertexId> path_num_matched;
        path_num_matched.append(0);

        // Use an explicit stack to avoid deep recursion.
        // Second item tracks the next sub-blossom index to process.
        DynListStack<std::pair<const NonTrivialBlossomT *, size_t>> stack;
        stack.put(std::make_pair(blossom, size_t{0}));

        while (! stack.is_empty())
          {
            VertexId depth = stack.size();
            auto & stack_elem = stack.top();
            blossom = stack_elem.first;
            size_t sub_index = stack_elem.second;

            if (sub_index == 0)
              {
                // We just entered this sub-blossom.
                // Update the depth of all vertices in this sub-blossom.
                for_vertices_in_blossom(blossom,
                                        [&vertex_depth,depth](VertexId x)
                                          {
                                            vertex_depth[x] = depth;
                                          });

                // Calculate the sum of blossom duals at the new depth.
                path_sum_dual.append(path_sum_dual.get_last());
                if (checked_add(path_sum_dual.get_last(),
                                path_sum_dual.get_last(),
                                blossom->dual_var))
                  return false;

                // Initialize the number of matched edges at the new depth.
                path_num_matched.append(0);

                if (blossom->subblossoms.size() < 3)
                  return false;
              }

            if (sub_index < blossom->subblossoms.size())
              {
                auto it = blossom->subblossoms.get_it();
                for (size_t i = 0; i < sub_index; ++i)
                  it.next_ne();
                auto & sub_item = it.get_curr();
                ++stack_elem.second;

                // Examine the current sub-blossom.
                BlossomT *sub = sub_item.blossom;
                NonTrivialBlossomT *ntb = sub->nontrivial();
                if (ntb)
                  {
                    // Prepare to descend into this sub-blossom.
                    stack.put(std::make_pair(ntb, size_t{0}));
                  }
                else
                  {
                    // Handle single vertex.
                    // For each incident edge, find the smallest blossom
                    // that contains it.
                    VertexId x = sub->base_vertex;
                    bool failed = false;
                    graph.for_adjacent_edges(x, [this,x,&failed,&vertex_depth,&path_sum_dual,&path_num_matched](
                                             const EdgeT *edge)
                                               {
                                                 if (failed)
                                                   return;

                                                 // Only consider edges pointing out from "x".
                                                 // This avoids processing the same undirected edge
                                                 // twice during verification.
                                                 if (edge->vt.first != x)
                                                   return;

                                                 VertexId y = edge->vt.second;
                                                 VertexId edge_depth = vertex_depth[y];
                                                 if (edge_depth > 0)
                                                   {
                                                     // Found the smallest blossom that contains this edge.
                                                     // Add the duals of the containing blossoms.
                                                     if (checked_add(edge_duals[edge_index(edge)],
                                                                     edge_duals[edge_index(edge)],
                                                                     path_sum_dual[edge_depth]))
                                                       {
                                                         failed = true;
                                                         return;
                                                       }

                                                     // Update the number of matched edges in the blossom.
                                                     if (ctx.vertex_mate[x] == y)
                                                       path_num_matched[edge_depth] += 1;
                                                   }
                                               });

                    if (failed)
                      return false;
                  }
              }
            else
              {
                // We are leaving the current sub-blossom.

                // Count the number of vertices inside this blossom.
                VertexId blossom_num_vertex = 0;
                for_vertices_in_blossom(blossom,
                                        [&blossom_num_vertex](VertexId)
                                          {
                                            ++blossom_num_vertex;
                                          });

                // Check that the blossom is "full".
                // A blossom is full if all except one of its vertices
                // are matched to another vertex within the blossom.
                if (const VertexId blossom_num_matched = path_num_matched[depth];
                  blossom_num_vertex != 2 * blossom_num_matched + 1)
                  return false;

                // Update the number of matched edges in the parent blossom.
                path_num_matched[depth - 1] += path_num_matched[depth];

                // Push vertices in this sub-blossom back to the parent.
                for_vertices_in_blossom(blossom, [&vertex_depth,depth](VertexId x)
                                          {
                                            vertex_depth[x] = depth - 1;
                                          });

                // Trim the descending path.
                (void) path_sum_dual.remove_last();
                (void) path_num_matched.remove_last();

                // Remove the current blossom from the stack.
                (void) stack.get();
              }
          }

        return true;
      }

      /**
       * Check that all blossoms are full.
       * Also calculate the sum of dual variables for every edge.
       */
      bool verify_blossoms_and_calc_edge_duals()
      {
        // For each edge, calculate the sum of its vertex duals.
        for (const EdgeT & edge: graph.edges)
          if (checked_add(edge_duals[edge_index(&edge)],
                          ctx.vertex_dual[edge.vt.first],
                          ctx.vertex_dual[edge.vt.second]))
            return false;

        // Descend down each top-level blossom.
        // Check that blossoms are full.
        // Add blossom duals to the edges contained inside the blossoms.
        // This takes total time O(n**2).
        for (const NonTrivialBlossomT & blossom: ctx.nontrivial_blossom)
          if (blossom.parent == nullptr)
            if (not check_blossom(&blossom))
              return false;

        return true;
      }

      /**
       * Check that all edges have non-negative slack,
       * and check that all matched edges have zero slack.
       *
       * @pre Edge duals must be calculated before calling this function.
       */
      bool verify_edge_slack()
      {
        for (const EdgeT & edge: graph.edges)
          {
            WeightType duals = edge_duals[edge_index(&edge)];
            WeightType weight = ctx.weight_factor * edge.weight;

            if (weight > duals)
              return false;
            WeightType slack = duals - weight;

            if (ctx.vertex_mate[edge.vt.first] == edge.vt.second)
              if (slack != 0)
                return false;
          }
        return true;
      }

    private:
      /** Reference to the MatchingContext instance. */
      const MatchingContext<WeightType> & ctx;

      /** Reference to the input graph. */
      const Problem_Data<WeightType> & graph;

      /**
       * For each edge, the sum of duals of its incident vertices
       * and duals of all blossoms that contain the edge.
       */
      Array<WeightType> edge_duals;
    };
  } // namespace impl


  /* **************************************************
   * **        public functions                      **
   * ************************************************** */

  /** @brief Compute a maximum-weighted matching in a general undirected graph.
   *
   *  This function implements the main Edmonds Blossom algorithm for weighted 
   *  matching. It finds a set of edges such that no two edges share a vertex 
   *  and the sum of weights is maximized.
   *
   *  The input graph must be provided as an array of weighted edges.
   *  - Vertices must be indexed 0 to@f$n-1@f$.
   *  - Self-loops are not allowed.
   *  - Negative weight edges are ignored.
   *
   *  @tparam WeightType   Type used for edge weights (integral or floating-point).
   *  @param[in] edges     Array of weighted edges.
   *
   *  @return Array of matched vertex pairs.
   *
   *  @exception std::invalid_argument If the input graph is invalid (e.g. duplicate edges, self-loops).
   *
   *  @par Complexity
   *  - Time:@f$O(n^3)@f$where@f$n@f$is the number of vertices.
   *  - Space:@f$O(n + m)@f$where@f$m@f$is the number of edges.
   */
  template <typename WeightType>
  Array<VertexPair> maximum_weight_matching(
      const Array<Edge<WeightType>> & edges)
  {
    // Check that the input meets all constraints.
    impl::check_input_graph(edges);

    // Run matching algorithm.
    impl::MatchingContext<WeightType> matching(edges);
    matching.run();

    // Verify that the solution is optimal (works only for integer weights).
    if (std::numeric_limits<WeightType>::is_integer)
      assert(impl::MatchingVerifier<WeightType>(matching).verify());

    // Extract the matched edges.
    Array<VertexPair> solution;
    solution.reserve(matching.graph.edges.size());
    for (const Edge<WeightType> & edge: matching.graph.edges)
      if (matching.vertex_mate[edge.vt.first] == edge.vt.second)
        solution.append(edge.vt);

    return solution;
  }


  /** @brief Adjust edge weights to prioritize maximum cardinality matching.
   *
   *  This function modifies the weights of the input edges such that any 
   *  maximum weight matching in the modified graph is also a maximum 
   *  cardinality matching in the original graph. Among matchings of 
   *  maximum cardinality, it will find one with maximum weight.
   *
   *  This is useful when the goal is to find the "best" maximum matching.
   *
   *  @tparam WeightType   Type used for edge weights.
   *  @param[in] edges_in  Original weighted edges.
   *
   *  @return A new array of edges with adjusted weights.
   *
   *  @exception std::invalid_argument If adjustment would cause numeric overflow.
   */
  template <typename WeightType>
  Array<Edge<WeightType>> adjust_weights_for_maximum_cardinality_matching(
      const Array<Edge<WeightType>> & edges_in)
  {
    // For integer types, min() is the most-negative representable value.
    // For floating-point types, min() is the smallest *positive* normalized
    // value — not the most negative — so we must use lowest() instead.
    const WeightType min_safe_weight =
        (std::numeric_limits<WeightType>::is_integer
             ? std::numeric_limits<WeightType>::min()
             : std::numeric_limits<WeightType>::lowest()) / 4;
    const WeightType max_safe_weight = std::numeric_limits<WeightType>::max() / 4;

    // Copy edges.
    Array<Edge<WeightType>> edges(edges_in);

    // Don't worry about empty graphs.
    if (edges.is_empty())
      return edges;

    // Count number of vertices.
    // Determine minimum and maximum edge weight.
    VertexId num_vertex = 0;
    WeightType min_weight = edges.get_first().weight;
    WeightType max_weight = min_weight;

    constexpr VertexId max_num_vertex = std::numeric_limits<VertexId>::max();
    for (const Edge<WeightType> & edge: edges)
      {
        const VertexId m = std::max(edge.vt.first, edge.vt.second);
        ah_invalid_argument_if(m >= max_num_vertex)
            << "Vertex ID out of range";
        num_vertex = std::max(num_vertex, m + 1);

        if (not std::numeric_limits<WeightType>::is_integer)
          ah_invalid_argument_if(! std::isfinite(edge.weight))
                << "Edge weights must be finite numbers";

        min_weight = std::min(min_weight, edge.weight);
        max_weight = std::max(max_weight, edge.weight);
      }

    // Calculate weight range and required weight adjustment.
    ah_invalid_argument_if(min_weight < min_safe_weight or max_weight > max_safe_weight)
        << "Edge weight exceeds maximum supported value";

    WeightType weight_range = max_weight - min_weight;

    ah_invalid_argument_if(weight_range > max_safe_weight / num_vertex)
        << "Adjusted edge weight exceeds maximum supported value";

    // Do nothing if the weights already ensure maximum-cardinality.
    if (min_weight > 0 and min_weight >= num_vertex * weight_range)
      return edges;

    WeightType delta;
    if (weight_range > 0)
      {
        // Increase weights to make minimum edge weight large enough
        // to improve any non-maximum-cardinality matching.
        delta = num_vertex * weight_range - min_weight;
      }
    else
      {
        // All weights are the same. Increase weights to make them positive.
        delta = 1 - min_weight;
      }

    assert(delta >= 0);

    ah_invalid_argument_if(delta > max_safe_weight - max_weight)
        << "Adjusted edge weight exceeds maximum supported value";

    // Increase all edge weights by "delta".
    for (Edge<WeightType> & edge: edges)
      edge.weight += delta;

    return edges;
  }
} // namespace Aleph::blossom_weighted_detail::mwmatching


#endif // ALEPH_BLOSSOM_WEIGHTED_MWMATCHING_H
