
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


/** @file tpl_netcost.H
    @brief Maximum flow minimum cost network algorithms.

    This file provides data structures and algorithms for solving the
    maximum flow minimum cost problem on capacitated networks.

    The main algorithm implemented is cycle-canceling, which first computes
    a maximum flow using Ford-Fulkerson (or another max-flow algorithm),
    then iteratively finds and cancels negative-cost cycles in the residual
    network using Bellman-Ford until no more negative cycles exist.

    @ingroup Networks
 *  @author Leandro Rabindranath León
*/

# ifndef TPL_NETCOST_H
# define TPL_NETCOST_H

# include <limits>
# include <chrono>
# include <tpl_net.H>
# include <tpl_dynMapTree.H>
# include <tpl_find_path.H>
# include <tpl_cut_nodes.H>
# include <Bellman_Ford.H>
# include <generate_graph.H>
# include <ah-errors.H>

namespace Aleph
{
  /** Node type alias for cost networks.

      Uses the same node type as regular flow networks.

      @tparam Node_Info Type of information stored in the node.
      @ingroup Networks
  */
  template <typename Node_Info = Empty_Class>
  using Net_Cost_Node = Net_Node<Node_Info>;


  /** Arc type for maximum flow minimum cost networks.

      Net_Cost_Arc models an arc belonging to a flow network with
      associated costs per unit of flow.

      This class extends Net_Arc by adding a cost field that represents
      the cost per unit of flow through the arc. For residual arcs,
      the cost is negative (representing the refund of cost when
      reducing flow).

      @tparam Arc_Info Type representing attributes associated with the arc.
      @tparam F_Type Numeric type for capacity, flow, and cost values.
                     Must be the same type used for node flow accumulations.

      @see Net_Arc Net_Cost_Graph
      @ingroup Networks
  */
  template <typename Arc_Info = Empty_Class, typename F_Type = double>
  struct Net_Cost_Arc : public Net_Arc<Arc_Info, F_Type>
  {
    using Base = Net_Arc<Arc_Info, F_Type>;
    using Base::Base;

    /// Type representing flow, capacity, and cost values.
    using Flow_Type = F_Type;

    /// Cost per unit of flow (negative for residual arcs).
    Flow_Type cost = 0;

    /// Default constructor.
    Net_Cost_Arc() = default;

    /// Copy constructor.
    Net_Cost_Arc(const Net_Cost_Arc & a)
      : Base(a), cost(a.cost)
    { /* empty */
    }

    /// Copy assignment operator.
    Net_Cost_Arc &operator =(const Net_Cost_Arc & a)
    {
      if (&a == this)
        return *this;
      Base::operator=(a);
      cost = a.cost;
      return *this;
    }

    /// Return the cost of the current flow through this arc.
    Flow_Type flow_cost() const noexcept { return this->flow * cost; }
  };


  /** Capacitated flow network with costs associated to arcs.

      This type, derived from Net_Graph, models a capacitated network
      where each arc has an associated cost per unit of flow. This model,
      extending the capacitated network concept, considerably expands the
      range of combinatorial optimization problems by posing a max-min
      optimization. In this case, maximum flow at minimum cost.

      Being a derivation of Net_Graph, its interface is very similar, with
      the addition of parameters and methods to handle the cost of each arc.

      @tparam NodeT Node type (should be Net_Cost_Node or compatible).
      @tparam ArcT Arc type (should be Net_Cost_Arc or compatible).

      @see max_flow_min_cost_by_cycle_canceling Max_Flow_Min_Cost_By_Cycle_Canceling
      @ingroup Networks
  */
  template <class NodeT = Net_Cost_Node<Empty_Class>,
            class ArcT = Net_Cost_Arc<Empty_Class, double>>
  struct Net_Cost_Graph : public Net_Graph<NodeT, ArcT>
  {
    using Base = Net_Graph<NodeT, ArcT>;

    /// The underlying flow network type.
    using Net = Net_Graph<NodeT, ArcT>;

    /// Self type alias.
    using Net_MFMC = Net_Cost_Graph<NodeT, ArcT>;

    /// Arc type.
    using Arc = ArcT;

    /// Node type.
    using Node = NodeT;

    /// Type representing capacity, flow, and cost values.
    using Flow_Type = typename Arc::Flow_Type;

    /// Type of attribute stored in a node.
    using Node_Type = typename Node::Node_Type;

    /// Type of attribute stored in an arc.
    using Arc_Type = typename Arc::Arc_Type;

    /// Default constructor.
    Net_Cost_Graph() = default;

    /** Copy constructor.

        Creates a deep copy of the network, including all nodes, arcs,
        and their cost values.

        @param[in] net Network to copy.
    */
    Net_Cost_Graph(const Net_Cost_Graph & net) : Base(net)
    {
      zip(this->arcs(), net.arcs()).for_each([](const auto & p)
                                               {
                                                 auto atgt = p.first;
                                                 auto asrc = p.second;
                                                 atgt->cost = asrc->cost;
                                               });
    }

    /// Move constructor.
    Net_Cost_Graph(Net_Cost_Graph &&) = default;

    /// Copy assignment operator (uses copy-and-swap idiom).
    Net_Cost_Graph &operator =(const Net_Cost_Graph & net)
    {
      if (this == &net)
        return *this;
      Net_Cost_Graph tmp(net);  // copy construct (copies capacity, flow, and cost)
      this->swap(tmp);          // swap with copy
      return *this;
    }

    /// Move assignment operator.
    Net_Cost_Graph &operator =(Net_Cost_Graph &&) = default;

    /** Return a modifiable reference to the cost of an arc.

        @param[in] a Pointer to the arc.
        @return Reference to the arc's cost field.
    */
    Flow_Type &get_cost(Arc *a) noexcept { return a->cost; }

    /** Return the cost of an arc (const version).

        @param[in] a Pointer to the arc.
        @return The arc's cost value.
    */
    Flow_Type get_cost(Arc *a) const noexcept { return a->cost; }

    /** Compute the cost of the flow through an arc.

        @param[in] a Pointer to the arc.
        @return The product of flow and cost for this arc.
    */
    static Flow_Type arc_flow_cost(Arc *a) noexcept { return a->flow_cost(); }

    /** Create and insert an arc in a flow network with costs.

        The created arc has zero initial flow.

        @param[in] src_node Source node.
        @param[in] tgt_node Target node.
        @param[in] cap Capacity value of the arc.
        @param[in] __cost Cost per unit of flow.
        @return Pointer to the new arc.
        @throw bad_alloc If there is not enough memory.
    */
    virtual Arc * insert_arc(Node *src_node, Node *tgt_node,
                             const Flow_Type & cap, const Flow_Type & __cost)
    {
      Arc *a = Net::insert_arc(src_node, tgt_node, cap, 0, Arc_Type());
      a->cost = __cost;
      return a;
    }

    /** Create and insert an arc with arc info using perfect forwarding.

        @tparam Args Types of arguments to forward to Arc_Type constructor.
        @param[in] src_node Source node.
        @param[in] tgt_node Target node.
        @param[in] cap Capacity value.
        @param[in] __cost Cost per unit of flow.
        @param[in] args Arguments to forward to Arc_Type constructor.
        @return Pointer to the new arc.
    */
    template <typename... Args>
    Arc * emplace_arc(Node *src_node, Node *tgt_node,
                      const Flow_Type & cap, const Flow_Type & __cost,
                      Args &&... args)
    {
      auto a = Net::insert_arc(src_node, tgt_node, cap, 0,
                               Arc_Type(std::forward<Args>(args)...));
      a->cost = __cost;
      return a;
    }

    /** Insert arc (internal use only).

        Used by internal algorithms. Creates an arc with zero cost.

        @param[in] src_node Source node.
        @param[in] tgt_node Target node.
        @return Pointer to the new arc.
        @warning For internal use only. Do not call directly.
    */
    virtual Arc * insert_arc(Node *src_node, Node *tgt_node)
    {
      Arc *a = Net::insert_arc(src_node, tgt_node, Arc_Type());
      a->cost = 0;
      return a;
    }

    /** Compute the total cost of flow circulating through the network.

        Sums up the flow cost (flow * cost) for all arcs in the network.

        @return Total flow cost.
    */
    Flow_Type flow_cost() const
    {
      Flow_Type total = 0;
      for (Arc_Iterator<Net_MFMC> it(*this); it.has_curr(); it.next_ne())
        {
          Arc *a = it.get_curr();
          total += a->flow_cost();
        }
      return total;
    }

    /** Compute outgoing flow parameters for a node.

        @param[in] p Node to analyze.
        @return Tuple of (total capacity, total flow, total cost) for
                all outgoing arcs.
    */
    auto out_pars(Node *p)
    {
      Flow_Type cap_sum = 0, flow_sum = 0, cost_sum = 0;
      for (_Out_Iterator<Net> it(p); it.has_curr(); it.next_ne())
        {
          Arc *a = it.get_curr();
          cap_sum += a->cap;
          flow_sum += a->flow;
          cost_sum += a->cost;
        }
      return std::make_tuple(cap_sum, flow_sum, cost_sum);
    }

    /** Compute incoming flow parameters for a node.

        @param[in] p Node to analyze.
        @return Tuple of (total capacity, total flow, total cost) for
                all incoming arcs.
    */
    auto in_pars(Node *p)
    {
      Flow_Type cap_sum = 0, flow_sum = 0, cost_sum = 0;
      for (_In_Iterator<Net> it(p); it.has_curr(); it.next_ne())
        {
          Arc *a = it.get_curr();
          cap_sum += a->cap;
          flow_sum += a->flow;
          cost_sum += a->cost;
        }
      return std::make_tuple(cap_sum, flow_sum, cost_sum);
    }
  };


  /** Arc filter for residual networks.

      Filters arcs based on residual capacity (cap > flow).

      @tparam Net Network type.
      @ingroup Networks
  */
  template <class Net>
  struct Res_Filt
  {
    /// Constructor (node parameter ignored).
    Res_Filt(typename Net::Node *) noexcept {}

    /// Default constructor.
    Res_Filt() noexcept = default;

    /** Check if arc has residual capacity.

        @param[in] a Arc to check.
        @return true if cap > flow, false otherwise.
    */
    bool operator ()(typename Net::Arc *a) const noexcept
    {
      return a->cap > a->flow;
    }
  };


  /** Cost distance functor for Bellman-Ford on residual networks.

      Provides the cost of an arc as its "distance" for shortest path
      algorithms.

      @tparam Net Network type.
      @ingroup Networks
  */
  template <class Net>
  struct Rcost
  {
    Rcost() noexcept = default;

    using Distance_Type = typename Net::Flow_Type;

    /** Return the cost of an arc.

        @param[in] a Arc to get cost from.
        @return The arc's cost value.
    */
    typename Net::Flow_Type operator ()(typename Net::Arc *a) const noexcept
    {
      return a->cost;
    }

    /** Reset arc to zero state.

        Sets capacity to max, flow and cost to zero.

        @param[in] a Arc to reset.
    */
    static void set_zero(typename Net::Arc *a) noexcept
    {
      a->cap = std::numeric_limits<Distance_Type>::max();
      a->flow = 0;
      a->cost = 0;
    }
  };


  /** Residual arc type with mirror pointer.

      Extends Net_Cost_Arc with a flag indicating if this is a residual
      arc and a pointer to the mirror arc in the opposite direction.

      @tparam Ftype Flow/cost numeric type.
      @ingroup Networks
  */
  template <typename Ftype>
  struct Res_Arc : public Net_Cost_Arc<Empty_Class, Ftype>
  {
    using Base = Net_Cost_Arc<Empty_Class, Ftype>;
    using Base::Base;

    /// True if this is a residual (backward) arc.
    bool is_residual = false;

    /// Pointer to the mirror arc in the opposite direction.
    Res_Arc *img = nullptr;
  };


  /** Residual network type alias.

      @tparam Ftype Flow/cost numeric type.
      @ingroup Networks
  */
  template <typename Ftype>
  using Residual_Net = Net_Cost_Graph<Net_Node<Empty_Class>, Res_Arc<Ftype>>;


  /** Create a pair of residual arcs in the residual network.

      For each arc in the original network, creates two arcs in the
      residual network: a forward arc with the same cost and a backward
      (residual) arc with negative cost.

      @tparam Res_Net Residual network type.
      @param[in,out] residual_net The residual network to insert arcs into.
      @param[in] src Source node in residual network.
      @param[in] tgt Target node in residual network.
      @param[in] cap Capacity of the arc.
      @param[in] flow Current flow on the arc.
      @param[in] cost Cost per unit of flow.
      @return Pointer to the forward arc.

      @pre flow <= cap and cost >= 0
      @ingroup Networks
  */
  template <class Res_Net>
  typename Res_Net::Arc *
  create_residual_arc(Res_Net & residual_net,
                      typename Res_Net::Node *src,
                      typename Res_Net::Node *tgt,
                      const typename Res_Net::Flow_Type cap,
                      const typename Res_Net::Flow_Type flow,
                      const typename Res_Net::Flow_Type cost)
  {
    assert(flow <= cap and cost >= 0);

    auto arc = residual_net.insert_arc(src, tgt, cap, cost);
    auto rarc = residual_net.insert_arc(tgt, src, cap, -cost);

    arc->is_residual = false;
    arc->flow = flow;
    arc->img = rarc;

    rarc->is_residual = true;
    rarc->img = arc;
    rarc->flow = arc->cap - arc->flow;

    assert(arc->cap == cap and arc->flow == flow and arc->cost == cost);
    assert(rarc->cap == cap and rarc->flow == cap - flow and rarc->cost == -cost);

    return arc;
  }


  /** Build a residual network from a flow network.

      Creates a residual network with forward and backward arcs for each
      arc in the original network. Also builds a mapping from original
      arcs to residual arcs.

      @tparam Net Original network type.
      @param[in] net The original flow network.
      @param[out] rnet The residual network to populate.
      @param[out] arcs Mapping from original arcs to residual arcs.
      @return Pointer to the source node in the residual network.

      @throw domain_error If network is not single source and single sink.
      @ingroup Networks
  */
  template <class Net>
  typename Residual_Net<typename Net::Flow_Type>::Node *
  build_residual_net(const Net & net,
                     Residual_Net<typename Net::Flow_Type> & rnet,
                     DynMapTree<void *, void *> & arcs)
  {
    ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
      << "Network is not single source and single sink";

    using Rnet = Residual_Net<typename Net::Flow_Type>;

    // Copy nodes from net to residual network
    for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        auto q = rnet.insert_node();
        map_nodes<Net, Rnet>(p, q);
      }

    // Create residual arcs and build mapping
    for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      {
        auto ga = it.get_curr();
        auto gsrc = static_cast<typename Net::Node *>(ga->src_node);
        auto gtgt = static_cast<typename Net::Node *>(ga->tgt_node);

        auto rsrc = mapped_node<Net, Rnet>(gsrc);
        auto rtgt = mapped_node<Net, Rnet>(gtgt);
        auto ra = create_residual_arc(rnet, rsrc, rtgt,
                                      ga->cap, ga->flow, ga->cost);
        arcs.insert(ga, ra);
      }

    assert(check_residual_net(rnet));

    return static_cast<typename Rnet::Node *>(NODE_COOKIE(net.get_source()));
  }


  /** Verify residual network consistency.

      Checks that each arc's mirror pointer forms a valid bidirectional link.

      @tparam Res_Net Residual network type.
      @param[in] net The residual network to check.
      @return true if all mirror pointers are consistent.
      @ingroup Networks
  */
  template <class Res_Net>
  bool check_residual_net(const Res_Net & net)
  {
    return net.all_arcs([](typename Res_Net::Arc *a)
                          {
                            return a->img != nullptr and a->img->img == a;
                          });
  }


  /** Cancel a negative cycle by augmenting flow.

      Finds the minimum slack (residual capacity) along the cycle and
      augments flow by that amount, which cancels the negative cycle.

      @tparam Res_Net Residual network type.
      @param[in] path The cycle path to cancel.

      @pre path must be a non-empty cycle with positive residual capacity
           on all arcs.
      @ingroup Networks
  */
  template <class Res_Net>
  void cancel_cycle(const Res_Net &, const Path<Res_Net> & path)
  {
    ah_domain_error_if(path.is_empty() or not path.is_cycle())
      << "Path is empty or not a cycle";

    using Ftype = typename Res_Net::Flow_Type;

    // Determine minimum slack (bottleneck capacity) of the cycle
    Ftype slack = std::numeric_limits<Ftype>::max();
    path.for_each_arc([&slack](typename Res_Net::Arc *a)
                        {
                          assert(a->cap - a->flow > 0);
                          slack = std::min(slack, a->cap - a->flow);
                        });

    // Cancel the cycle by augmenting flow
    path.for_each_arc([slack](typename Res_Net::Arc *a)
                        {
                          auto img = a->img;
                          assert(img->img == a);
                          assert(a->cap == img->cap);
                          a->flow += slack;
                          img->flow -= slack;
                        });
  }


  /** Transfer flow values from residual network back to original.

      Updates the original network's flow values based on the residual
      network's flow values after cycle canceling.

      @tparam Net Original network type.
      @param[in] arcs Mapping from original arcs to residual arcs.
      @ingroup Networks
  */
  template <class Net>
  void residual_to_net(const DynMapTree<void *, void *> & arcs)
  {
    using Rnet = Residual_Net<typename Net::Flow_Type>;
    arcs.for_each([](std::pair<void *, void *> p)
                    {
                      auto net_arc = static_cast<typename Net::Arc *>(p.first);
                      auto res_arc = static_cast<typename Rnet::Arc *>(p.second);
                      net_arc->flow = res_arc->flow;
                    });
  }


  /** Compute maximum flow at minimum cost using cycle canceling.

      This algorithm first computes a maximum flow using a max-flow algorithm
      (Ford-Fulkerson by default), then iteratively finds and cancels negative
      cost cycles in the residual network using the Bellman-Ford algorithm
      until no more negative cycles exist.

      The algorithm guarantees that the final flow is:
      1. A maximum flow (same value as produced by the max-flow algorithm)
      2. Has minimum total cost among all maximum flows

      @tparam Net Network type (must be Net_Cost_Graph or compatible).
      @tparam Max_Flow_Algo Maximum flow algorithm template (default: Ford_Fulkerson_Maximum_Flow).

      @param[in,out] net The network to compute max-flow min-cost on.
                         Flow values are modified in place.
      @param[in] it_factor Initial iteration factor for Bellman-Ford's
                           early cycle detection (default: 0.4).
      @param[in] step Step increment for iteration threshold (default: 10).

      @return Tuple containing:
              - Number of negative cycles cancelled
              - Final iteration factor used

      @throw bad_alloc If there is not enough memory.
      @throw domain_error If network is not single source and single sink.

      @par Complexity
      O(V * E^2 * C * U) where V is vertices, E is edges, C is max cost,
      and U is max capacity. In practice, often much faster due to early
      cycle detection.

      @par Example
      @code
      Net_Cost_Graph<> net;
      // ... build network with costs ...
      auto [cycles, factor] = max_flow_min_cost_by_cycle_canceling(net);
      double total_cost = net.flow_cost();
      @endcode

      @see Max_Flow_Min_Cost_By_Cycle_Canceling
      @ingroup Networks
  */
  template <class Net,
            template <class> class Max_Flow_Algo = Ford_Fulkerson_Maximum_Flow>
  std::tuple<size_t, double>
  max_flow_min_cost_by_cycle_canceling(Net & net,
                                       double it_factor = 0.4,
                                       size_t step = 10)
  {
    Max_Flow_Algo<Net>()(net); // First compute maximum flow

    using Rnet = Residual_Net<typename Net::Flow_Type>;
    using BF = Bellman_Ford<Rnet, Rcost<Rnet>, Arc_Iterator,
                            Out_Iterator, Res_Filt<Rnet>>;

    // Build residual network
    Rnet rnet;
    DynMapTree<void *, void *> arcs_map;
    typename Rnet::Node *source = build_residual_net(net, rnet, arcs_map);

    size_t count = 0;
    bool found_cycle = true;

    // Main loop: find and cancel negative cycles
    while (found_cycle)
      {
        // Search for negative cycle from source
        auto [cycle, iterations] =
            BF(rnet).search_negative_cycle(source, it_factor, step);

        if (cycle.is_empty())
          {
            // No cycle found from source, try global search
            auto [global_cycle, global_iter] =
                BF(rnet).search_negative_cycle(it_factor, step);

            if (global_cycle.is_empty())
              found_cycle = false;
            else
              {
                cancel_cycle(rnet, global_cycle);
                ++count;
              }
          }
        else
          { // Update iteration factor based on when cycle was found
            it_factor = static_cast<double>(iterations) / net.vsize();
            cancel_cycle(rnet, cycle);
            ++count;
          }
      }

    // Transfer results back to original network
    residual_to_net<Net>(arcs_map);

    return std::make_tuple(count, it_factor);
  }


  /** Functor wrapper for maximum flow minimum cost algorithm.

      Provides a callable object interface to the cycle-canceling algorithm.

      @tparam Net Network type.
      @tparam Max_Flow_Algo Maximum flow algorithm template.

      @see max_flow_min_cost_by_cycle_canceling
      @ingroup Networks
  */
  template <class Net,
            template <class> class Max_Flow_Algo = Ford_Fulkerson_Maximum_Flow>
  struct Max_Flow_Min_Cost_By_Cycle_Canceling
  {
    /** Execute the algorithm.

        @param[in,out] net Network to process.
        @param[in] it_factor Initial iteration factor (default: 0.4).
        @param[in] step Step increment (default: 10).
        @return Tuple of (cycles cancelled, final iteration factor).
    */
    std::tuple<size_t, double> operator ()(Net & net,
                                           const double it_factor = 0.4,
                                           const size_t step = 10)
    {
      return max_flow_min_cost_by_cycle_canceling<Net, Max_Flow_Algo>
          (net, it_factor, step);
    }
  };


  /** Output a flow network to Graphviz format.

      Generates a DOT format representation of the network showing
      flow/capacity/cost labels on arcs.

      @tparam Net Network type.
      @param[in] net The network to output.
      @param[out] out Output stream.
      @ingroup Networks
  */
  template <class Net>
  void print_net_cost(const Net & net, std::ostream & out)
  {
    long i = 0;
    net.nodes().for_each([&i](typename Net::Node *p)
                           {
                             NODE_COUNTER(p) = i++;
                           });

    struct Show_Node
    {
      void operator ()(const Net &, typename Net::Node *p, std::ostream & o)
      {
        o << "label = \"(" << p->get_info() << "," << NODE_COUNTER(p) << ")\"";
      }
    };

    struct Show_Arc
    {
      void operator ()(const Net &, typename Net::Arc *a, std::ostream & o)
      {
        o << "label = \"" << a->flow << "/" << a->cap << "/" << a->cost << "\"";
      }
    };

    To_Graphviz<Net, Show_Node, Show_Arc>().digraph(net, out);
  }


  /** Output a residual network to Graphviz format.

      Generates a DOT format representation showing residual arcs in red.

      @tparam Net Original network type (used for Flow_Type).
      @param[in] net The residual network to output.
      @param[out] out Output stream.
      @ingroup Networks
  */
  template <class Net>
  void print_residual_net(const Residual_Net<typename Net::Flow_Type> & net,
                          std::ostream & out)
  {
    using Rnet = Residual_Net<typename Net::Flow_Type>;

    long i = 0;
    net.nodes().for_each([&i](typename Rnet::Node *p)
                           {
                             NODE_COUNTER(p) = i++;
                           });

    struct Show_Node
    {
      void operator ()(const Rnet &, typename Rnet::Node *p, std::ostream & o)
      {
        o << "label = \"" << NODE_COUNTER(p) << "\"";
      }
    };

    struct Show_Arc
    {
      void operator ()(const Rnet &, typename Rnet::Arc *a, std::ostream & o)
      {
        o << "label = \"" << a->flow << "/" << a->cap << "/" << a->cost << "\"";
        if (a->is_residual)
          o << " color = red";
      }
    };

    To_Graphviz<Rnet, Show_Node, Show_Arc, Dft_Show_Node<Rnet>,
                Res_Filt<Rnet>>().digraph(net, out);
  }


  // =========================================================================
  // Network Simplex Algorithm Implementation
  // =========================================================================

  /** Feasible spanning tree classification.

      Tuple of (empty arcs, full arcs, partial arcs) where:
      - Empty: flow = 0
      - Full: flow = capacity
      - Partial: 0 < flow < capacity

      @tparam Net Network type.
      @ingroup Networks
  */
  template <class Net>
  using Feasible_Tree = std::tuple<DynList<typename Net::Arc *>,
                                   DynList<typename Net::Arc *>,
                                   DynList<typename Net::Arc *>>;


  /** Build feasible spanning tree classification.

      Classifies all arcs into empty (flow=0), full (flow=cap), and partial.

      @tparam Net Network type.
      @param[in] net Network to classify.
      @return Tuple of (empty, full, partial) arc lists.
      @ingroup Networks
  */
  template <class Net>
  inline Feasible_Tree<Net> build_feasible_spanning_tree(const Net & net)
  {
    using Arc = typename Net::Arc;
    DynList<Arc *> empty, full, partial;

    for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      {
        if (auto a = it.get_curr(); a->flow == 0)
          empty.append(a);
        else if (a->flow == a->cap)
          full.append(a);
        else
          partial.append(a);
      }

    return std::make_tuple(std::move(empty), std::move(full), std::move(partial));
  }


  /** Get arcs with partial flow (0 < flow < cap).

      @tparam Net Network type.
      @param[in] net Network to scan.
      @return List of arcs with partial flow.
      @ingroup Networks
  */
  template <class Net>
  inline DynList<typename Net::Arc *> get_partial_arcs(const Net & net)
  {
    DynList<typename Net::Arc *> ret;
    for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      if (auto a = it.get_curr(); a->flow > 0 and a->flow < a->cap)
        ret.append(a);
    return ret;
  }


  /** Node information for Network Simplex algorithm.

      Stores tree structure and dual variables (potentials) for each node.
      The tree is represented with parent pointers and a mark for
      efficient tree membership testing.

      @tparam Ftype Flow/cost numeric type.
      @ingroup Networks
  */
  template <typename Ftype>
  struct Simplex_Node_Info
  {
    /// Node potential (dual variable pi).
    Ftype potential = 0;

    /// Parent node in the spanning tree (nullptr for root).
    void *parent = nullptr;

    /// Arc connecting this node to its parent.
    void *parent_arc = nullptr;

    /// Depth in the spanning tree (root has depth 0).
    long depth = 0;

    /// True if parent_arc is oriented from parent towards this node.
    bool arc_from_parent = true;

    /// Mark used for LCA computation.
    long mark = 0;
  };


  /** Arc state in Network Simplex.

      Indicates whether an arc is in the basis (tree) and its bound state.
      @ingroup Networks
  */
  enum class Simplex_Arc_State : unsigned char
  {
    Lower,  ///< Non-basic arc at lower bound (flow = 0).
    Upper,  ///< Non-basic arc at upper bound (flow = cap).
    Tree    ///< Basic arc (in spanning tree).
  };


  /** Arc information for Network Simplex algorithm.

      @tparam Ftype Flow/cost numeric type.
      @ingroup Networks
  */
  template <typename Ftype>
  struct Simplex_Arc_Info
  {
    /// Arc state (Lower, Upper, or Tree).
    Simplex_Arc_State state = Simplex_Arc_State::Lower;
    /// Skip this arc temporarily (failed pivot, will retry later).
    bool skip = false;
    /// Lower bound on flow (default 0).
    Ftype lower_bound = Ftype{0};
  };


  /** Execution statistics for Network Simplex algorithm.

      Tracks performance metrics and algorithm behavior during solving.
      @ingroup Networks
  */
  struct NetworkSimplexStats
  {
    size_t total_pivots = 0;         ///< Total pivot operations
    size_t phase1_pivots = 0;        ///< Pivots in Phase I (feasibility)
    size_t phase2_pivots = 0;        ///< Pivots in Phase II (optimization)
    size_t degenerate_pivots = 0;    ///< Pivots with zero flow change
    size_t tree_arcs = 0;            ///< Number of arcs in spanning tree
    double phase1_time_ms = 0.0;     ///< Phase I elapsed time in milliseconds
    double phase2_time_ms = 0.0;     ///< Phase II elapsed time in milliseconds
    double total_time_ms = 0.0;      ///< Total elapsed time in milliseconds
    size_t initial_partial_arcs = 0; ///< Arcs with partial flow before Phase I
    size_t forced_into_tree = 0;     ///< Arcs forced into tree in Phase I

    void reset() noexcept
    {
      total_pivots = 0;
      phase1_pivots = 0;
      phase2_pivots = 0;
      degenerate_pivots = 0;
      tree_arcs = 0;
      phase1_time_ms = 0.0;
      phase2_time_ms = 0.0;
      total_time_ms = 0.0;
      initial_partial_arcs = 0;
      forced_into_tree = 0;
    }
  };


  /** Network Simplex algorithm for minimum cost flow.

      This class implements the Network Simplex algorithm, a specialized
      version of the Simplex method for minimum cost network flow problems.
      It maintains a spanning tree of basic arcs and iteratively improves
      the solution through pivoting operations.

      ## Algorithm Overview

      The algorithm uses a two-phase approach:

      ### Phase I: Establish Basic Feasible Solution
      After Ford-Fulkerson, some arcs may have partial flow (0 < flow < cap).
      For a valid basic solution, these arcs MUST be in the spanning tree
      (non-basic arcs must be at their bounds). Phase I forces all partial-flow
      arcs into the tree through pivots.

      ### Phase II: Cost Optimization
      Standard Network Simplex that iteratively:
      1. Finds an entering arc with negative reduced cost
      2. Identifies the cycle created by adding this arc
      3. Determines the leaving arc (blocking arc in the cycle)
      4. Performs the pivot: updates flows and tree structure

      ## Implementation Notes

      This implementation follows the presentation in Sedgewick's "Algorithms"
      with some adaptations for the Aleph-w graph representation and the
      addition of Phase I to handle the basic feasibility requirement.

      The algorithm assumes the network already has a feasible flow (typically
      from a max-flow algorithm like Ford-Fulkerson). It then optimizes the
      cost while maintaining the same flow value.

      @tparam Net Network type (must be Net_Cost_Graph or compatible).

      @see max_flow_min_cost_by_network_simplex
      @see max_flow_min_cost_by_cycle_canceling
      @ingroup Networks
  */
  template <class Net>
  class Network_Simplex
  {
  public:
    using Node = typename Net::Node;
    using Arc = typename Net::Arc;
    using Flow_Type = typename Net::Flow_Type;
    using Node_Info = Simplex_Node_Info<Flow_Type>;
    using Arc_Info = Simplex_Arc_Info<Flow_Type>;

  private:
    Net &net;
    DynArray<Node_Info> node_info;
    DynArray<Arc_Info> arc_info;
    DynMapTree<Node *, size_t> node_to_idx;
    DynMapTree<Arc *, size_t> arc_to_idx;
    Node *root = nullptr;
    size_t num_pivots = 0;
    long lca_mark = 0;  // Counter for LCA marking
    mutable NetworkSimplexStats stats;
    bool support_lower_bounds = false;

    static constexpr Flow_Type Inf = std::numeric_limits<Flow_Type>::max();

    /// Epsilon for floating-point comparisons.
    static Flow_Type eps()
    {
      if constexpr (std::is_floating_point_v<Flow_Type>)
        return std::numeric_limits<Flow_Type>::epsilon() * 1000;
      else
        return 0;
    }

    /// Get node info reference.
    Node_Info &ninfo(Node *p) { return node_info(node_to_idx.find(p)); }

    /// Get node info const reference.
    const Node_Info &ninfo(Node *p) const { return node_info(node_to_idx.find(p)); }

    /// Get arc info reference.
    Arc_Info &ainfo(Arc *a) { return arc_info(arc_to_idx.find(a)); }

    /// Get arc info const reference.
    const Arc_Info &ainfo(Arc *a) const { return arc_info(arc_to_idx.find(a)); }

    /// Get parent node.
    Node *parent(Node *p) const { return static_cast<Node *>(ninfo(p).parent); }

    /// Get parent arc.
    Arc *parent_arc(Node *p) const { return static_cast<Arc *>(ninfo(p).parent_arc); }

    /// Get depth.
    long depth(Node *p) const { return ninfo(p).depth; }

    /// Get potential.
    Flow_Type potential(Node *p) const { return ninfo(p).potential; }

    /// Check if value is effectively zero.
    bool is_zero(Flow_Type x) const
    {
      if constexpr (std::is_floating_point_v<Flow_Type>)
        return std::abs(x) <= eps();
      else
        return x == 0;
    }

    /// Compute reduced cost of an arc.
    /// Reduced cost = c_ij - pi_i + pi_j
    /// For optimality: reduced cost >= 0 for arcs at lower bound (flow = 0)
    ///                 reduced cost <= 0 for arcs at upper bound (flow = cap)
    Flow_Type reduced_cost(Arc *a) const
    {
      auto src = static_cast<Node *>(a->src_node);
      auto tgt = static_cast<Node *>(a->tgt_node);
      return a->cost - ninfo(src).potential + ninfo(tgt).potential;
    }

    /// Initialize data structures.
    void init_structures()
    {
      node_info.cut(0);
      arc_info.cut(0);
      node_to_idx.empty();
      arc_to_idx.empty();

      // Map nodes to indices
      size_t idx = 0;
      for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          node_to_idx.insert(p, idx);
          node_info.touch(idx) = Node_Info{};
          ++idx;
        }

      // Map arcs to indices - DO NOT classify yet, let build_spanning_tree handle it
      idx = 0;
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          arc_to_idx.insert(a, idx);
          arc_info.touch(idx) = Arc_Info{};
          // Initially mark all arcs as Lower; build_spanning_tree will fix this
          arc_info(idx).state = Simplex_Arc_State::Lower;
          ++idx;
        }
    }

    /// Check if arc has partial flow (strictly between bounds).
    bool is_partial_flow(Arc *a) const
    {
      return a->flow > eps() and a->flow < a->cap - eps();
    }

    /** Build initial spanning tree from source.

        Creates a spanning tree rooted at the source node and computes
        initial potentials. Prioritizes arcs with partial flow.

        Non-tree arcs are classified as Lower or Upper based on their
        current flow relative to capacity.
    */
    void build_spanning_tree()
    {
      root = net.get_source();

      // Reset all nodes
      for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          auto &ni = ninfo(p);
          ni.parent = nullptr;
          ni.parent_arc = nullptr;
          ni.depth = -1;
          ni.potential = 0;
          ni.mark = 0;
        }

      // Reset all arcs to Lower
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        ainfo(it.get_curr()).state = Simplex_Arc_State::Lower;

      const size_t total_nodes = net.vsize();
      DynListQueue<Node *> queue;
      queue.put(root);
      ninfo(root).depth = 0;
      ninfo(root).potential = 0;

      size_t nodes_in_tree = 1;

      // Helper to add a node to tree via an arc
      auto add_to_tree = [&](Node *p, Arc *a, Node *other) -> bool {
        if (ninfo(other).depth >= 0)
          return false;  // Already in tree

        const auto &pi = ninfo(p);
        auto &oi = ninfo(other);
        oi.parent = p;
        oi.parent_arc = a;
        oi.depth = pi.depth + 1;

        auto src = static_cast<Node *>(a->src_node);
        if (src == p)
          {
            oi.arc_from_parent = true;
            oi.potential = pi.potential - a->cost;
          }
        else
          {
            oi.arc_from_parent = false;
            oi.potential = pi.potential + a->cost;
          }

        ainfo(a).state = Simplex_Arc_State::Tree;
        ++nodes_in_tree;
        queue.put(other);
        return true;
      };

      // Build tree with multiple priority passes
      while (not queue.is_empty() and nodes_in_tree < total_nodes)
        {
          auto p = queue.get();

          // Collect all candidate arcs with priorities
          struct ArcPriority {
            Arc* arc;
            Node* other;
            int priority;  // 0 = partial flow, 1 = with flow, 2 = no flow
          };
          DynList<ArcPriority> candidates;

          for (Node_Arc_Iterator<Net> it(p); it.has_curr(); it.next_ne())
            {
              auto a = it.get_curr();
              auto other = net.get_connected_node(a, p);
              if (ninfo(other).depth >= 0)
                continue;  // Already in tree

              int prio;
              if (is_partial_flow(a))
                prio = 0;  // Highest priority
              else if (a->flow > eps())
                prio = 1;  // Medium priority
              else
                prio = 2;  // Lowest priority

              candidates.append(ArcPriority{a, other, prio});
            }

          // Sort by priority (partial flow first)
          // Since DynList doesn't have sort, process in order
          for (int target_prio = 0; target_prio <= 2; ++target_prio)
            {
              for (auto& ap : candidates)
                {
                  if (ap.priority == target_prio)
                    add_to_tree(p, ap.arc, ap.other);
                }
            }
        }

      // Classify remaining non-tree arcs
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          if (ainfo(a).state == Simplex_Arc_State::Tree)
            continue;

          if (a->flow <= eps())
            ainfo(a).state = Simplex_Arc_State::Lower;
          else if (a->flow >= a->cap - eps())
            ainfo(a).state = Simplex_Arc_State::Upper;
          else
            {
              // Partial flow arc NOT in tree - classify based on reduced cost
              auto rc = reduced_cost(a);
              ainfo(a).state = (rc < -eps()) ?
                  Simplex_Arc_State::Upper : Simplex_Arc_State::Lower;
            }
        }
    }

    /** Find entering arc using most negative reduced cost.

        Scans all non-tree arcs and selects the one with the largest
        violation of the optimality conditions:
        - Arc at Lower bound (flow=0): should have reduced_cost >= 0
        - Arc at Upper bound (flow=cap): should have reduced_cost <= 0

        This implementation checks BOTH directions for each arc:
        - Forward direction: increase flow (requires flow < cap, benefits if rc < 0)
        - Backward direction: decrease flow (requires flow > 0, benefits if rc > 0)

        This is essential for finding all negative cycles, including those
        that require reversing flow through arcs with flow > 0.

        @return Entering arc, or nullptr if optimal.
    */
    Arc *find_entering_arc()
    {
      Arc *best = nullptr;
      Flow_Type best_violation = eps();  // Threshold to avoid tiny violations
      bool best_is_increase = true;

      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          auto &ai = ainfo(a);

          if (ai.state == Simplex_Arc_State::Tree)
            continue;

          if (ai.skip)  // Skip arcs that failed in this round
            continue;

          const auto rc = reduced_cost(a);
          const bool can_increase = (a->flow < a->cap - eps());
          const bool can_decrease = (a->flow > eps());

          // Check FORWARD direction: increase flow (rc < 0 is good)
          if (can_increase and rc < -best_violation)
            {
              best_violation = -rc;
              best = a;
              best_is_increase = true;
            }

          // Check BACKWARD direction: decrease flow (rc > 0 is good)
          // This is equivalent to using the "reverse arc" in residual network
          if (can_decrease and rc > best_violation)
            {
              best_violation = rc;
              best = a;
              best_is_increase = false;
            }
        }

      // Update state based on direction
      if (best != nullptr)
        {
          ainfo(best).state = best_is_increase ?
              Simplex_Arc_State::Lower : Simplex_Arc_State::Upper;
        }

      return best;
    }

    /** Find the lowest common ancestor of two nodes.

        Uses marking to efficiently find the LCA in O(d) where d is depth.

        @param[in] u First node.
        @param[in] v Second node.
        @return LCA of u and v.
    */
    Node *find_lca(Node *u, Node *v)
    {
      ++lca_mark;  // Use new mark

      // Mark path from u to root
      Node *p = u;
      while (p != nullptr)
        {
          ninfo(p).mark = lca_mark;
          p = parent(p);
        }

      // Walk from v towards root until we hit a marked node
      p = v;
      while (p != nullptr and ninfo(p).mark != lca_mark)
        p = parent(p);

      return p;
    }

    /** Augment flow around the cycle and find leaving arc.

        When entering arc is added, it creates a cycle. This function:
        1. Finds the bottleneck (minimum slack) in the cycle
        2. Updates flows around the cycle
        3. Identifies the leaving arc

        For a valid cycle that allows flow redistribution, we need arcs
        going in both directions (considering the entering arc). If all
        arcs in the cycle go in the same "global" direction (e.g., all
        from source towards sink), then we cannot redistribute flow
        without changing the total flow value.

        @param[in] entering The entering arc.
        @param[out] delta Flow change amount.
        @param[out] leaving The leaving arc (becomes non-basic).
        @param[out] leaving_goes_lower True if leaving arc goes to lower bound.
        @return true if a valid augmentation was performed.
    */
    bool augment_and_find_leaving(Arc *entering, Flow_Type &delta,
                                  Arc *&leaving, bool &leaving_goes_lower)
    {
      auto u = static_cast<Node *>(entering->src_node);
      auto v = static_cast<Node *>(entering->tgt_node);
      Node *lca = find_lca(u, v);

      assert(lca != nullptr);

      // Determine if we're increasing or decreasing flow on entering arc
      const bool increase_on_entering = (ainfo(entering).state == Simplex_Arc_State::Lower);

      // Count arcs in the cycle and check if it's a valid cycle
      // A valid cycle must have at least one arc where flow increases
      // and at least one where flow decreases (in the cycle direction)
      size_t arcs_with_increase = 0;
      size_t arcs_with_decrease = 0;
      size_t total_cycle_arcs = 1;  // Start with entering arc

      if (increase_on_entering)
        ++arcs_with_increase;
      else
        ++arcs_with_decrease;


      // Walk from u to lca and count
      Node *p = u;
      while (p != lca)
        {
          bool from_parent = ninfo(p).arc_from_parent;
          bool flow_increases = (increase_on_entering == from_parent);

          if (flow_increases)
            ++arcs_with_increase;
          else
            ++arcs_with_decrease;

          ++total_cycle_arcs;
          p = parent(p);
        }

      // Walk from v to lca and count
      p = v;
      while (p != lca)
        {
          bool from_parent = ninfo(p).arc_from_parent;
          bool flow_increases = (increase_on_entering != from_parent);

          if (flow_increases)
            ++arcs_with_increase;
          else
            ++arcs_with_decrease;

          ++total_cycle_arcs;
          p = parent(p);
        }

      // If the cycle only has one arc (the entering arc itself) or
      // all arcs go in the same direction, it's not a valid cycle
      // for flow redistribution
      if (total_cycle_arcs <= 1 or arcs_with_increase == 0 or arcs_with_decrease == 0)
        {
          delta = 0;
          leaving = entering;
          leaving_goes_lower = not increase_on_entering;
          return false;  // No valid pivot possible
        }

      // Initialize with entering arc's slack
      if (increase_on_entering)
        delta = entering->cap - entering->flow;
      else
        delta = entering->flow;

      leaving = entering;
      leaving_goes_lower = increase_on_entering;

      // Walk from u to lca
      // Direction: lca → ... → u (following tree direction)
      // If increase_on_entering, flow increases in arcs pointing towards u
      p = u;
      while (p != lca)
        {
          Arc *pa = parent_arc(p);
          bool from_parent = ninfo(p).arc_from_parent;
          // from_parent=true means arc goes parent→child (towards u)
          // So flow increases when both are true or both are false
          bool flow_increases = (increase_on_entering == from_parent);

          Flow_Type slack;
          bool goes_lower;

          if (flow_increases)
            {
              slack = pa->cap - pa->flow;
              goes_lower = false;
            }
          else
            {
              slack = pa->flow;
              goes_lower = true;
            }

          // Prefer arc that goes to lower bound in case of tie (Bland's rule variant)
          if (slack < delta or (slack == delta and goes_lower and not leaving_goes_lower))
            {
              delta = slack;
              leaving = pa;
              leaving_goes_lower = goes_lower;
            }

          p = parent(p);
        }

      // Walk from v to lca
      // Direction: v → ... → lca (against tree direction)
      // If increase_on_entering, flow decreases in arcs pointing towards v
      p = v;
      while (p != lca)
        {
          Arc *pa = parent_arc(p);
          bool from_parent = ninfo(p).arc_from_parent;
          // from_parent=true means arc goes parent→child (towards v)
          // Since we go against tree direction, flow decreases when from_parent=true
          bool flow_increases = (increase_on_entering != from_parent);

          Flow_Type slack;
          bool goes_lower;

          if (flow_increases)
            {
              slack = pa->cap - pa->flow;
              goes_lower = false;
            }
          else
            {
              slack = pa->flow;
              goes_lower = true;
            }

          if (slack < delta or (slack == delta and goes_lower and not leaving_goes_lower))
            {
              delta = slack;
              leaving = pa;
              leaving_goes_lower = goes_lower;
            }

          p = parent(p);
        }

      // Allow degenerate pivots (delta=0) if we found a valid leaving arc
      // This restructures the tree without changing flow, potentially enabling
      // future pivots that were previously blocked.
      if (delta >= 0 and leaving != nullptr)
        {
          // Update entering arc
          if (increase_on_entering)
            entering->flow += delta;
          else
            entering->flow -= delta;

          // Update u-side (same logic as above)
          p = u;
          while (p != lca)
            {
              Arc *pa = parent_arc(p);
              bool from_parent = ninfo(p).arc_from_parent;
              bool flow_increases = (increase_on_entering == from_parent);

              if (flow_increases)
                pa->flow += delta;
              else
                pa->flow -= delta;

              p = parent(p);
            }

          // Update v-side (same logic as above)
          p = v;
          while (p != lca)
            {
              Arc *pa = parent_arc(p);
              bool from_parent = ninfo(p).arc_from_parent;
              bool flow_increases = (increase_on_entering != from_parent);

              if (flow_increases)
                pa->flow += delta;
              else
                pa->flow -= delta;

              p = parent(p);
            }

          return true;  // Successful pivot (including degenerate with delta=0)
        }

      return false;  // Failed - no valid cycle found
    }

    /** Update tree structure after pivot.

        Replaces the leaving arc with the entering arc and updates
        parent pointers, depths, and potentials.

        @param[in] entering The entering arc.
        @param[in] leaving The leaving arc.
        @param[in] leaving_goes_lower True if leaving goes to lower bound.
    */
    void pivot_tree(Arc *entering, Arc *leaving, bool leaving_goes_lower)
    {
      // Handle degenerate pivot where entering = leaving
      if (entering == leaving)
        {
          auto &ai = ainfo(entering);
          ai.state = leaving_goes_lower ?
              Simplex_Arc_State::Lower : Simplex_Arc_State::Upper;
          return;
        }

      // Set leaving arc state
      ainfo(leaving).state = leaving_goes_lower ?
          Simplex_Arc_State::Lower : Simplex_Arc_State::Upper;

      // Set entering arc as tree arc
      ainfo(entering).state = Simplex_Arc_State::Tree;

      // Find which end of leaving arc becomes the new subtree root
      // One of the endpoints must have leaving as its parent_arc
      auto leaving_src = static_cast<Node *>(leaving->src_node);
      auto leaving_tgt = static_cast<Node *>(leaving->tgt_node);

      Node *subtree_root;
      bool src_has_leaving = (parent_arc(leaving_src) == leaving);
      bool tgt_has_leaving = (parent_arc(leaving_tgt) == leaving);

      if (not src_has_leaving and not tgt_has_leaving)
        return;  // Invalid pivot

      if (src_has_leaving)
        subtree_root = leaving_src;
      else
        subtree_root = leaving_tgt;

      auto entering_src = static_cast<Node *>(entering->src_node);
      auto entering_tgt = static_cast<Node *>(entering->tgt_node);

      // Determine which end of entering arc is in the subtree
      Node *in_subtree, *out_subtree;
      {
        Node *p = entering_src;
        bool src_in = false;
        while (p != nullptr)
          {
            if (p == subtree_root)
              {
                src_in = true;
                break;
              }
            p = parent(p);
          }

        if (src_in)
          {
            in_subtree = entering_src;
            out_subtree = entering_tgt;
          }
        else
          {
            in_subtree = entering_tgt;
            out_subtree = entering_src;
          }
      }

      // Reverse path from subtree_root to in_subtree
      DynList<Node *> path;
      Node *p = in_subtree;
      while (p != subtree_root and p != nullptr)
        {
          path.insert(p);  // Prepend
          p = parent(p);
        }
      if (p == subtree_root)
        path.insert(subtree_root);
      else
        return;  // in_subtree not descendant of subtree_root

      // Reverse parent relationships along the path
      // Path is: [subtree_root, X, Y, ..., in_subtree] (built by prepending)
      //
      // Original tree (parent pointers):
      //   in_subtree --> Y --> X --> subtree_root --> (via leaving to rest of tree)
      //   (each node's parent is to its right)
      //
      // After reversal (parent pointers):
      //   subtree_root --> X --> Y --> in_subtree --> out_subtree --> ...
      //   (each node's parent is to its right, but in_subtree connects via entering)
      //
      // For each pair (curr, next) in [subtree_root, X, Y, ...]:
      // - Original: next.parent = curr, so parent_arc(next) = arc between them
      // - New: curr.parent = next, using the same arc
      for (auto it = path.get_it(); it.has_curr(); )
        {
          Node *curr = it.get_curr();
          it.next();

          if (not it.has_curr())
            break;

          Node *next = it.get_curr();

          // Original: next was child of curr, so parent_arc(next) = arc between curr and next
          Arc *arc_between = parent_arc(next);
          auto &curr_info = ninfo(curr);
          curr_info.parent = next;
          curr_info.parent_arc = arc_between;

          auto arc_src = static_cast<Node *>(arc_between->src_node);
          curr_info.arc_from_parent = (arc_src == next);
        }

      // Link in_subtree to out_subtree via entering arc
      auto &sub_info = ninfo(in_subtree);
      sub_info.parent = out_subtree;
      sub_info.parent_arc = entering;
      sub_info.arc_from_parent = (static_cast<Node *>(entering->src_node) == out_subtree);

      // Update depths and potentials using BFS from in_subtree
      DynListQueue<Node *> queue;
      queue.put(in_subtree);

      auto &out_info = ninfo(out_subtree);
      sub_info.depth = out_info.depth + 1;
      if (sub_info.arc_from_parent)
        sub_info.potential = out_info.potential - entering->cost;
      else
        sub_info.potential = out_info.potential + entering->cost;

      while (not queue.is_empty())
        {
          Node *curr = queue.get();
          const auto &curr_info = ninfo(curr);

          // Find children of curr (nodes whose parent is curr)
          for (typename Net::Node_Iterator nit(net); nit.has_curr(); nit.next_ne())
            {
              Node *child = nit.get_curr();
              if (child == curr)
                continue;

              if (parent(child) != curr)
                continue;

              auto &ci = ninfo(child);
              ci.depth = curr_info.depth + 1;

              Arc *pa = parent_arc(child);
              if (ci.arc_from_parent)
                ci.potential = curr_info.potential - pa->cost;
              else
                ci.potential = curr_info.potential + pa->cost;

              queue.put(child);
            }
        }
    }

  public:
    /** Construct Network Simplex solver.

        @param[in,out] network The network to optimize.
    */
    explicit Network_Simplex(Net &network) : net(network) {}

    /** Set lower bound for an arc.

        Sets the minimum flow that must pass through an arc.
        Default lower bound is 0.

        @param[in] a Arc to set lower bound for.
        @param[in] lower_bound Minimum flow value.

        @note Must be called before run().
    */
    void set_lower_bound(Arc *a, Flow_Type lower_bound)
    {
      support_lower_bounds = true;
      ainfo(a).lower_bound = lower_bound;
    }

    /** Get lower bound for an arc.

        @param[in] a Arc to query.
        @return Lower bound value (0 if not set).
    */
    [[nodiscard]] Flow_Type get_lower_bound(Arc *a) const
    {
      return ainfo(a).lower_bound;
    }

    /** Check if arc flow is at lower bound.

        @param[in] a Arc to check.
        @return true if flow equals lower bound.
    */
    [[nodiscard]] bool is_at_lower_bound(Arc *a) const
    {
      return std::abs(a->flow - ainfo(a).lower_bound) < eps();
    }

    /** Check if arc flow is at upper bound (capacity).

        @param[in] a Arc to check.
        @return true if flow equals capacity.
    */
    [[nodiscard]] bool is_at_upper_bound(Arc *a) const
    {
      return std::abs(a->flow - a->cap) < eps();
    }

    /** Get residual capacity (room to increase flow).

        @param[in] a Arc to query.
        @return Available capacity above current flow.
    */
    [[nodiscard]] Flow_Type residual_capacity(Arc *a) const
    {
      return a->cap - a->flow;
    }

    /** Get residual lower (room to decrease flow).

        @param[in] a Arc to query.
        @return Available decrease to lower bound.
    */
    [[nodiscard]] Flow_Type residual_lower(Arc *a) const
    {
      return a->flow - ainfo(a).lower_bound;
    }


  public:
    /** Execute Network Simplex algorithm.

        Computes minimum cost flow. If the network already has a feasible
        flow (e.g., from Ford-Fulkerson), it optimizes starting from there.

        @param[in] unused Kept for API compatibility.
        @return Number of pivots performed.
    */
    size_t run([[maybe_unused]] Flow_Type unused = 0)
    {
      ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
          << "Network is not single source and single sink";

      stats.reset();
      auto total_start = std::chrono::high_resolution_clock::now();

      init_structures();
      build_spanning_tree();

      // Count initial partial-flow arcs
      stats.initial_partial_arcs = count_non_tree_partial_arcs();

      // PHASE I: Establish valid basic feasible solution
      auto phase1_start = std::chrono::high_resolution_clock::now();
      size_t phase1_pivots = force_partial_arcs_into_tree();
      auto phase1_end = std::chrono::high_resolution_clock::now();
      stats.phase1_time_ms = std::chrono::duration<double, std::milli>(
        phase1_end - phase1_start).count();
      stats.phase1_pivots = phase1_pivots;
      stats.forced_into_tree = phase1_pivots;

      // PHASE II: Optimize cost using standard Network Simplex
      auto phase2_start = std::chrono::high_resolution_clock::now();
      num_pivots = phase1_pivots;
      size_t failed_attempts = 0;
      const size_t max_pivots = net.vsize() * net.esize() * 10 + 1000;
      const size_t max_failures = net.esize() + 1;

      while (num_pivots < max_pivots and failed_attempts < max_failures)
        {
          Arc *entering = find_entering_arc();

          if (entering == nullptr)
            break;  // Optimal!

          Flow_Type delta;
          Arc *leaving;
          bool leaving_goes_lower;

          bool success = augment_and_find_leaving(entering, delta, leaving, leaving_goes_lower);

          if (success)
            {
              if (std::abs(delta) < eps())
                ++stats.degenerate_pivots;

              pivot_tree(entering, leaving, leaving_goes_lower);
              ++num_pivots;
              failed_attempts = 0;

              // Reset skip flags after successful pivot
              for (typename Net::Arc_Iterator ait(net); ait.has_curr(); ait.next_ne())
                ainfo(ait.get_curr()).skip = false;
            }
          else
            {
              ainfo(entering).skip = true;
              ++failed_attempts;
            }
        }

      auto phase2_end = std::chrono::high_resolution_clock::now();
      stats.phase2_time_ms = std::chrono::duration<double, std::milli>(
        phase2_end - phase2_start).count();
      stats.phase2_pivots = num_pivots - phase1_pivots;

      auto total_end = std::chrono::high_resolution_clock::now();
      stats.total_time_ms = std::chrono::duration<double, std::milli>(
        total_end - total_start).count();
      stats.total_pivots = num_pivots;

      // Count tree arcs
      stats.tree_arcs = 0;
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        if (ainfo(it.get_curr()).state == Simplex_Arc_State::Tree)
          ++stats.tree_arcs;

      return num_pivots;
    }

    /// Return number of pivots performed in last run.
    [[nodiscard]] size_t get_num_pivots() const { return num_pivots; }

    /// Return execution statistics from last run.
    [[nodiscard]] const NetworkSimplexStats & get_stats() const noexcept { return stats; }

    /// Print execution statistics to stdout.
    void print_stats() const
    {
      std::cout << "=== Network Simplex Statistics ===\n"
                << "Total pivots: " << stats.total_pivots << "\n"
                << "  Phase I pivots: " << stats.phase1_pivots << "\n"
                << "  Phase II pivots: " << stats.phase2_pivots << "\n"
                << "  Degenerate pivots: " << stats.degenerate_pivots << "\n"
                << "Tree arcs: " << stats.tree_arcs << " (expected " << (net.vsize() - 1) << ")\n"
                << "Initial partial-flow arcs: " << stats.initial_partial_arcs << "\n"
                << "Arcs forced into tree: " << stats.forced_into_tree << "\n"
                << "Timing:\n"
                << "  Phase I: " << stats.phase1_time_ms << " ms\n"
                << "  Phase II: " << stats.phase2_time_ms << " ms\n"
                << "  Total: " << stats.total_time_ms << " ms\n";
    }

    /** Phase I: Force all partial-flow arcs into the spanning tree.

        For a valid basic feasible solution in Network Simplex, all arcs
        with flow strictly between bounds (0 < flow < cap) MUST be in the
        spanning tree. Non-basic arcs must be at their bounds.

        This method identifies arcs with partial flow that are not in the
        tree and forces pivots to include them. Each pivot replaces one
        tree arc with the entering arc, maintaining exactly N-1 tree arcs.

        @return Number of forced pivots performed.
    */
    size_t force_partial_arcs_into_tree()
    {
      size_t forced_pivots = 0;
      bool made_progress = true;

      while (made_progress)
        {
          made_progress = false;

          // Find a non-tree arc with partial flow
          for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
            {
              auto a = it.get_curr();
              if (ainfo(a).state == Simplex_Arc_State::Tree)
                continue;

              if (not is_partial_flow(a))
                continue;

              // Found a partial-flow arc not in tree - force it in
              // Mark it as Lower (to increase flow direction)
              ainfo(a).state = Simplex_Arc_State::Lower;

              Flow_Type delta;
              Arc *leaving;
              bool leaving_goes_lower;

              // Try to find a valid cycle and leaving arc
              bool success = augment_and_find_leaving(a, delta, leaving, leaving_goes_lower);

              if (success and leaving != nullptr and leaving != a)
                {
                  // Verify leaving arc is in tree
                  if (ainfo(leaving).state != Simplex_Arc_State::Tree)
                    {
                      ainfo(a).state = Simplex_Arc_State::Lower;
                      continue;
                    }

                  pivot_tree(a, leaving, leaving_goes_lower);
                  ++forced_pivots;
                  made_progress = true;
                  break;
                }
              else
                {
                  // Try the other direction (Upper = decrease flow)
                  ainfo(a).state = Simplex_Arc_State::Upper;
                  success = augment_and_find_leaving(a, delta, leaving, leaving_goes_lower);

                  if (success and leaving != nullptr and leaving != a)
                    {
                      pivot_tree(a, leaving, leaving_goes_lower);
                      ++forced_pivots;
                      made_progress = true;
                      break;
                    }
                  else
                    {
                      // Cannot add this arc - restore to appropriate bound
                      if (a->flow <= eps())
                        ainfo(a).state = Simplex_Arc_State::Lower;
                      else if (a->flow >= a->cap - eps())
                        ainfo(a).state = Simplex_Arc_State::Upper;
                      else
                        ainfo(a).state = Simplex_Arc_State::Lower;  // Should not happen
                    }
                }
            }
        }

      return forced_pivots;
    }

    /** Check if current solution is a valid basic feasible solution.

        A valid BFS requires:
        1. All tree arcs can have any flow
        2. All non-tree arcs must be at their bounds (flow=0 or flow=cap)

        @return true if valid BFS, false otherwise.
    */
    bool is_valid_basic_solution() const
    {
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          if (ainfo(a).state == Simplex_Arc_State::Tree)
            continue;

          // Non-tree arc must be at bounds
          if (is_partial_flow(a))
            return false;
        }
      return true;
    }

    /** Count arcs with partial flow not in tree. */
    size_t count_non_tree_partial_arcs() const
    {
      size_t count = 0;
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          if (ainfo(a).state != Simplex_Arc_State::Tree and is_partial_flow(a))
            ++count;
        }
      return count;
    }

    /** Verify that tree arcs have zero reduced cost.

        In a valid basic solution, all tree arcs must have reduced cost = 0.
        This is a consistency check for the potential values.

        @return true if all tree arcs have rc=0, false otherwise.
    */
    bool verify_tree_reduced_costs() const
    {
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          if (ainfo(a).state != Simplex_Arc_State::Tree)
            continue;

          auto rc = reduced_cost(a);
          if (std::abs(rc) > eps() * 100)  // Allow small tolerance
            return false;
        }
      return true;
    }

    /** Verify tree integrity - all parent_arcs should be Tree arcs.

        @return true if tree structure is consistent.
    */
    [[nodiscard]] bool verify_tree_integrity() const
    {
      for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          if (p == root)
            continue;

          auto pa = parent_arc(p);
          if (pa == nullptr)
            return false;

          if (ainfo(pa).state != Simplex_Arc_State::Tree)
            return false;
        }
      return true;
    }

    /** Print diagnostic information about current state. */
    void print_diagnostics() const
    {
      std::cout << "\n=== Network Simplex Diagnostics ===\n";

      // Count arc types
      size_t tree_arcs = 0, lower_arcs = 0, upper_arcs = 0;
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          switch (ainfo(it.get_curr()).state)
            {
            case Simplex_Arc_State::Tree: ++tree_arcs; break;
            case Simplex_Arc_State::Lower: ++lower_arcs; break;
            case Simplex_Arc_State::Upper: ++upper_arcs; break;
            }
        }

      std::cout << "Arcs: Tree=" << tree_arcs << " Lower=" << lower_arcs
                << " Upper=" << upper_arcs << " Total=" << net.esize() << "\n";
      std::cout << "Expected tree arcs: " << (net.vsize() - 1) << "\n";

      // Verify tree reduced costs
      bool tree_rc_ok = verify_tree_reduced_costs();
      std::cout << "Tree arcs rc=0: " << (tree_rc_ok ? "YES" : "NO") << "\n";

      // Check for optimality violations
      size_t violations = 0;
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto a = it.get_curr();
          auto state = ainfo(a).state;
          auto rc = reduced_cost(a);

          if (state == Simplex_Arc_State::Lower)
            {
              // Lower: flow=0, can increase. Optimal if rc >= 0
              if (rc < -eps() and a->flow < a->cap - eps())
                {
                  ++violations;
                  std::cout << "  VIOLATION: Lower arc with rc=" << rc
                            << " flow=" << a->flow << "/" << a->cap << "\n";
                }
            }
          else if (state == Simplex_Arc_State::Upper)
            {
              // Upper: flow=cap, can decrease. Optimal if rc <= 0
              if (rc > eps() and a->flow > eps())
                {
                  ++violations;
                  std::cout << "  VIOLATION: Upper arc with rc=" << rc
                            << " flow=" << a->flow << "/" << a->cap << "\n";
                }
            }
        }

      std::cout << "Optimality violations: " << violations << "\n";
      std::cout << "==================================\n";
    }
  };


  /** Compute maximum flow at minimum cost using Network Simplex.

      This algorithm first computes a maximum flow using a max-flow algorithm
      (Ford-Fulkerson by default), then optimizes cost using the Network
      Simplex algorithm with a two-phase approach:

      - **Phase I**: Establishes a valid basic feasible solution by forcing
        all arcs with partial flow (0 < flow < cap) into the spanning tree.
      - **Phase II**: Standard Network Simplex optimization that finds arcs
        with negative reduced cost and performs pivots until optimal.

      @tparam Net Network type (must be Net_Cost_Graph or compatible).
      @tparam Max_Flow_Algo Maximum flow algorithm template.

      @param[in,out] net The network to compute max-flow min-cost on.
                         Flow values are modified in place.

      @return Number of simplex pivots performed (Phase I + Phase II).

      @throw bad_alloc If there is not enough memory.
      @throw domain_error If network is not single source and single sink.

      @par Complexity
      Expected O(n * m * log(n * C)) with good pivot rules, where n=nodes,
      m=arcs, C=max cost. Worst case is pseudo-polynomial.

      @par Example
      @code
      Net_Cost_Graph<> net;
      // ... build network with costs ...
      size_t pivots = max_flow_min_cost_by_network_simplex(net);
      double total_cost = net.flow_cost();
      @endcode

      @see max_flow_min_cost_by_cycle_canceling Network_Simplex
      @ingroup Networks
  */
  template <class Net,
            template <class> class Max_Flow_Algo = Ford_Fulkerson_Maximum_Flow>
  size_t max_flow_min_cost_by_network_simplex(Net & net)
  {
    Max_Flow_Algo<Net>()(net);  // First compute maximum flow
    Network_Simplex<Net> simplex(net);
    return simplex.run();
  }


  /** Functor wrapper for Network Simplex algorithm.

      @tparam Net Network type.
      @tparam Max_Flow_Algo Maximum flow algorithm template.

      @see max_flow_min_cost_by_network_simplex
      @ingroup Networks
  */
  template <class Net,
            template <class> class Max_Flow_Algo = Ford_Fulkerson_Maximum_Flow>
  struct Max_Flow_Min_Cost_By_Network_Simplex
  {
    /** Execute the algorithm.

        @param[in,out] net Network to process.
        @return Number of simplex pivots performed.
    */
    size_t operator ()(Net & net)
    {
      return max_flow_min_cost_by_network_simplex<Net, Max_Flow_Algo>(net);
    }
  };

} // end namespace Aleph

# endif // TPL_NETCOST_H
