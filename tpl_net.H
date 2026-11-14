
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

/** @file tpl_net.H
 *  @brief Network flow graph structures.
 *
 *  Defines graph structures for network flow problems with
 *  capacities, flows, and source/sink management.
 *
 *  ## Features
 *  - Capacity and flow on arcs
 *  - Source and sink node management
 *  - Residual graph computation
 *
 *  @see tpl_maxflow.H Maximum flow algorithms
 *  @see tpl_netcost.H Minimum cost flow
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_NET_H
#define TPL_NET_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <tpl_array.H>
#include <tpl_sort_utils.H>
#include <tpl_dynDlist.H>
#include <tpl_dynListStack.H>
#include <tpl_dynBinHeap.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynSetHash.H>
#include <tpl_random_queue.H>
#include <tpl_graph_utils.H>
#include <tpl_find_path.H>
#include <graph-traverse.H>
#include <ah-errors.H>
#include <cookie_guard.H>

namespace Aleph {
using std::get;

/** @brief Arc info extension that stores capacity and flow values.
 *
 * @tparam Arc_Info Base arc info type.
 * @tparam Flow_Type Numeric type for capacity/flow.
 *
 * @ingroup Networks
 */
template <typename Arc_Info, typename Flow_Type>
struct Net_Arc_Info : public Arc_Info
{
  /// Capacity value.
  Flow_Type cap = Flow_Type{};

  /// Flow value.
  Flow_Type flow = Flow_Type{};

  /// Default constructor.
  Net_Arc_Info() = default;

  /// Construct from base info, capacity and flow.
  Net_Arc_Info(const Arc_Info &info, Flow_Type __cap, Flow_Type __flow)
    : Arc_Info(info), cap(__cap), flow(__flow)
  {
    // empty
  }
};

namespace net_detail {
/** @brief Whether `float` can be the type of a flow: it can, but it is
    deprecated, and this overload, which a static assertion of Net_Arc
    calls, warns it.

    The tolerances of the floating flows are relative to the data and to
    the epsilon of the type, which for `float`, 1.2e-7, is 2^29 times that
    of `double`: so the far magnitude that hides the real improvements
    comes with ordinary data, as an arc of cost 500 hides from the cycle
    canceling the cycles of cost -1 among the others on a network of 100
    nodes, without notice; the tolerance of check_node(), 1e-9, is below
    its epsilon; and its integers stay exact only up to 2^24. Use `double`:
    `float` is rejected in 8.0.0. The warning comes from this header, so
    that a pragma around the code that uses the network does not silence
    it; including the headers between `#pragma GCC diagnostic push`,
    `#pragma GCC diagnostic ignored "-Wdeprecated-declarations"` and
    `#pragma GCC diagnostic pop` does, while moving to `double`.

    @return true.
*/
[[deprecated("float as the type of a flow is deprecated: its precision is below what the "
             "tolerances of the floating flows need; use double. It is rejected in 8.0.0")]]
constexpr bool flow_type_ok(const float *) noexcept
{
  return true;
}

/** @brief Whether any type but `float` can be the type of a flow, as a
    static assertion of Net_Arc asks: it can (see flow_type_ok(const float *)).

    @return true.
*/
constexpr bool flow_type_ok(const void *) noexcept
{
  return true;
}
}  // namespace net_detail

/** @brief Arc of a flow network implemented with adjacency lists.
 *
 * Net_Arc models a capacitated network arc with Arc_Info and a numeric
 * Flow_Type for capacity and flow: an integer type, which compares
 * exactly, or `double`. `float` is deprecated, and warns when the arc is
 * instantiated: its precision is below what the tolerances of the
 * floating flows need (see net_detail::flow_type_ok()); it is rejected in
 * 8.0.0.
 *
 * This class is only intended as the arc type for Net_Graph; do not
 * instantiate it directly. Use Net_Graph::insert_arc() instead.
 *
 * @see Array_Graph Net_Graph Net_Node
 * @ingroup Networks
 */
template <typename Arc_Info, typename F_Type = double>
struct Net_Arc : public Graph_Aarc<Arc_Info>  // Arc type for flow networks.
{
  using Base = Graph_Aarc<Arc_Info>;

  // A float flow warns that it is deprecated
  static_assert(net_detail::flow_type_ok(static_cast<const F_Type *>(nullptr)));

  using Flow_Type = F_Type;

  /// Capacity value.
  Flow_Type cap = 0;

  /// Flow value.
  Flow_Type flow = 0;

  /// Return true if flow satisfies 0 <= flow <= cap.
  bool check_arc() const noexcept
  {
    return flow >= 0 and flow <= cap;
  }

  /// Construct from arc info.
  Net_Arc(const Arc_Info &info) : Base(info)
  { /* empty */
  }

  /// Copy constructor.
  Net_Arc(const Net_Arc &arc) : Base(arc.arc_info), cap(arc.cap), flow(arc.flow)
  {
    // empty
  }

  /// Default constructor.
  Net_Arc()
  { /* empty */
  }

  /// Copy assignment.
  Net_Arc &operator = (const Net_Arc &arc)
  {
    if (this == &arc)
      return *this;

    *static_cast<Base *>(this) = arc;
    cap = arc.cap;
    flow = arc.flow;

    return *this;
  }
};

/** @brief Return true if arc `a` is residual with respect to `src`. */
template <class Net>
bool is_residual(typename Net::Node *src, typename Net::Arc *a) noexcept
{
  assert(a->src_node == src or a->tgt_node == src);
  return a->tgt_node == src;
}

/** @brief Arc filter for residual traversal in flow networks.
 *
 * @ingroup Networks
 */
template <class Net>
struct Net_Filt
{
  typename Net::Node *p = nullptr;

  void *cookie = nullptr;

  /// Store an opaque cookie for compatibility with other iterators.
  void set_cookie(void *__cookie) noexcept
  {
    cookie = __cookie;
  }

  /// Build a filter rooted at node `s`.
  Net_Filt(typename Net::Node *s = nullptr) noexcept : p(s)
  { /* empty */
  }

  /// Return true if arc `a` has residual capacity with respect to `p`.
  bool operator () (typename Net::Arc *a) const noexcept
  {
    assert(p);
    assert(a->src_node == p or a->tgt_node == p);
    auto src = static_cast<typename Net::Node *>(a->src_node);
    if (src == p)
      return a->cap - a->flow > 0;  // normal arc

    assert(is_residual<Net>(p, a));
    return a->flow > 0;  // residual arc
  }

  /// Filter hook used by some iterator adapters.
  bool operator () (const Net &, typename Net::Arc *arc) noexcept
  {
    return (*this)(arc);
  }

  /// Return the opposite endpoint of arc `a` with respect to `p`.
  typename Net::Node *get_node(typename Net::Arc *a) const noexcept
  {
    assert(p);
    assert(a->src_node == p or a->tgt_node == p);
    return static_cast<typename Net::Node *>(a->src_node == p ? a->tgt_node : a->src_node);
  }
};

template <class Net>
using __Net_Iterator = Digraph_Iterator<Net, Net_Filt<Net>>;

/// Residual-graph iterator adapter for flow networks.
template <class Net, class Show_Arc = Dft_Show_Arc<Net>>
using Net_Iterator = Filter_Iterator<typename Net::Node *, __Net_Iterator<Net>, Show_Arc>;

/** @brief Return the remaining flow of `a` as seen from `src`.
 *
 * If `a` is residual with respect to `src`, the remaining flow is the
 * current flow. Otherwise, it is the residual capacity.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type remaining_flow(typename Net::Node *src, typename Net::Arc *a) noexcept
{
  return is_residual<Net>(src, a) ? a->flow : a->cap - a->flow;
}

/// Node type for flow networks.
template <typename Node_Info = Empty_Class>
using Net_Node = Graph_Anode<Node_Info>;

/** @brief Flow network implemented with adjacency lists.
 *
 * Net_Graph models a capacitated network, the main data structure
 * for maximum-flow algorithms and related graph routines.
 *
 * Template parameters:
 * - NodeT: node type (must derive from Net_Node).
 * - ArcT: arc type (must derive from Net_Arc).
 *
 * @ingroup Networks
 */
template <class NodeT = Net_Node<Empty_Class>, class ArcT = Net_Arc<Empty_Class, double>>
struct Net_Graph : public Array_Graph<NodeT, ArcT>
{
  using Net = Net_Graph<NodeT, ArcT>;

  using Base = Array_Graph<NodeT, ArcT>;

  using Base::Base;
  using Base::insert_node;

  using Graph = Base;

  /// Arc type.
  using Arc = ArcT;

  /// Node type.
  using Node = NodeT;

  /// Capacity/flow numeric type.
  using Flow_Type = typename Arc::Flow_Type;

  /// Node info type.
  using Node_Type = typename Node::Node_Type;

  /// Arc info type.
  using Arc_Type = typename Arc::Arc_Type;

  mutable Flow_Type Infinity;

  /// Return arcs outgoing from `p` (as a DynList).
  DynList<Arc *> out_arcs(Node *p) const noexcept
  {
    return Aleph::out_arcs<Net_Graph>(p);
  }

  /// Return nodes reachable from `p` through outgoing arcs.
  DynList<Node *> out_nodes(Node *p) const noexcept
  {
    return out_pairs<Net_Graph>(p).template maps<Node *>([](const ArcPair<Net_Graph> &p)
      {
        return get<1>(p);
      });
  }

  /// Return arcs incoming to `p` (as a DynList).
  DynList<Arc *> in_arcs(Node *p) const noexcept
  {
    return Aleph::in_arcs<Net_Graph>(p);
  }

  /// Return nodes that can reach `p` through incoming arcs.
  DynList<Node *> in_nodes(Node *p) const noexcept
  {
    return in_pairs<Net_Graph>(p).template maps<Node *>([](const ArcPair<Net_Graph> &p)
      {
        return get<1>(p);
      });
  }

  /// Return total incoming capacity of `node`.
  [[nodiscard]] Flow_Type get_in_cap(Node *node) const noexcept
  {
    Flow_Type sum = Flow_Type{0};
    for (_In_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
      sum += it.get_curr()->cap;
    return sum;
  }

  /// Return total outgoing capacity of `node`.
  [[nodiscard]] Flow_Type get_out_cap(Node *node) const noexcept
  {
    Flow_Type sum = Flow_Type{0};
    for (_Out_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
      sum += it.get_curr()->cap;
    return sum;
  }

  /// Return the in-degree of `p` (number of incoming arcs).
  size_t get_in_degree(Node *p) const noexcept
  {
    return this->in_degree(p);
  }

  /// Return the out-degree of `p` (number of outgoing arcs).
  size_t get_out_degree(Node *p) const noexcept
  {
    return this->out_degree(p);
  }

  /// Return total outgoing flow of `node`.
  [[nodiscard]] Flow_Type get_out_flow(Node *node) const noexcept
  {
    Flow_Type sum = Flow_Type{0};
    for (_Out_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
      sum += it.get_curr()->flow;
    return sum;
  }

  /// Return total incoming flow of `node`.
  [[nodiscard]] Flow_Type get_in_flow(Node *node) const noexcept
  {
    Flow_Type sum = Flow_Type{0};
    for (_In_Iterator<Net_Graph> it(node); it.has_curr(); it.next_ne())
      sum += it.get_curr()->flow;
    return sum;
  }

  /// Return true if `node` is a source.
  bool is_source(Node *node) const noexcept
  {
    return src_nodes.contains(node);
  }

  /// Return true if `node` is a sink.
  bool is_sink(Node *node) const noexcept
  {
    return sink_nodes.contains(node);
  }

  /// Return true if the network has a single source.
  [[nodiscard]] constexpr bool is_single_source() const noexcept
  {
    return src_nodes.size() == 1;
  }

  /// Return true if the network has a single sink.
  [[nodiscard]] constexpr bool is_single_sink() const noexcept
  {
    return sink_nodes.size() == 1;
  }

  /// Return true if `p` is connected (used for validation).
  bool is_connected(Node *p) const noexcept
  {
    return get_in_degree(p) != 0 or get_out_degree(p) != 0;
  }

  /** @brief The tolerance of rounding of the flow of a node whose flows
      into and out of it are `in` and `out`.

      For a floating flow, 1e-9 of the larger of them: the scale of the
      node is its own flows, so that a small flow next to a large
      capacity elsewhere keeps its precision. Zero for an integer flow.
  */
  static Flow_Type node_tolerance(const Flow_Type &in, const Flow_Type &out) noexcept
  {
    if constexpr (std::is_floating_point_v<Flow_Type>)
      return std::max(std::abs(in), std::abs(out)) * 1e-9;
    else
      return Flow_Type{0};
  }

  /** @brief Return true if `node` satisfies flow conservation constraints.

      A floating flow keeps them but for rounding, within
      node_tolerance() of the flows of the node. Flows that are equal
      balance, also infinite ones, as those of a node of a path of arcs
      of infinite capacity; an infinite flow and a finite one do not.
  */
  bool check_node(Node *node) const noexcept
  {
    if (not is_connected(node))
      return false;

    auto out_flow = get_out_flow(node);
    auto in_flow = get_in_flow(node);

    // Use tolerance for floating-point comparison. Equal flows balance,
    // also infinite ones, whose difference, inf - inf, is NaN
    const auto eps = check_tolerance(in_flow, out_flow);
    const auto nearly_zero = [eps](auto x)
      {
        return std::abs(x) <= eps;
      };
    const auto nearly_equal = [eps](auto a, auto b)
      {
        return a == b or std::abs(a - b) <= eps;
      };

    if (is_sink(node))
      return nearly_zero(out_flow) and in_flow >= -eps;

    if (is_source(node))
      return nearly_zero(in_flow) and out_flow >= -eps;

    return nearly_equal(out_flow, in_flow);
  }

private:
  DynSetTree<Node *> src_nodes;
  DynSetTree<Node *> sink_nodes;

  // The super source and the super sink that make_super_source() and
  // make_super_sink() inserted, if any, which unmake_super_source() and
  // unmake_super_sink() remove
  Node *super_source_node = nullptr;
  Node *super_sink_node = nullptr;

  // The tolerance of check_node() for a node whose flows into and out of
  // it are in and out: node_tolerance(), but zero if one of them is
  // infinite. That of an infinite flow, infinite too, took any imbalance,
  // as an infinite flow into a node that sends 2.5; with zero, the flows
  // balance only if they are equal, as the infinite flows into and out
  // of a node of a path of arcs of infinite capacity
  static Flow_Type check_tolerance(const Flow_Type &in, const Flow_Type &out) noexcept
  {
    if constexpr (std::is_floating_point_v<Flow_Type>)
      if (std::isinf(in) or std::isinf(out))
        return Flow_Type{0};
    return node_tolerance(in, out);
  }

  // The sum of the tolerances of check_node() of every node: as each
  // keeps its flow but for its own, the flow out of the sources and that
  // into the sinks differ, at most, by it. O(V + E)
  Flow_Type nodes_tolerance() const
  {
    Flow_Type sum{0};
    for (Node_Iterator<Net_Graph> it(*this); it.has_curr(); it.next_ne())
      sum += check_tolerance(get_in_flow(it.get_curr()), get_out_flow(it.get_curr()));
    return sum;
  }

  // Whether the flow out of the sources and that into the sinks are the
  // same: exactly, or, for a floating type, but for rounding: within the
  // tolerance of check_node() of the totals, or within the sum of those
  // of the nodes, which is computed only if the first does not hold.
  // Equal totals are the same, also infinite ones, whose difference is
  // NaN; an infinite total and a finite one are not
  bool same_totals(const Flow_Type &out, const Flow_Type &in) const
  {
    if constexpr (std::is_floating_point_v<Flow_Type>)
      {
        if (out == in)
          return true;
        const Flow_Type diff = std::abs(out - in);
        return diff <= check_tolerance(out, in) or diff <= nodes_tolerance();
      }
    else
      return out == in;
  }

  /// Return total outgoing flow from all sources.
  Flow_Type total_source_out_flow() const
  {
    Flow_Type sum = Flow_Type{};
    for (auto it = src_nodes.get_it(); it.has_curr(); it.next_ne())
      sum += get_out_flow(it.get_curr());
    return sum;
  }

  /// Return total incoming flow to all sinks.
  Flow_Type total_sink_in_flow() const
  {
    Flow_Type sum = Flow_Type{};
    for (auto it = sink_nodes.get_it(); it.has_curr(); it.next_ne())
      sum += get_in_flow(it.get_curr());
    return sum;
  }

  /// Insert `p` into source/sink sets with rollback on failure.
  Node *register_node(Node *p)
  {
    try
      {
        src_nodes.insert(p);
        sink_nodes.insert(p);
      }
    catch (...)
      {
        src_nodes.remove(p);
        sink_nodes.remove(p);
        Graph::remove_node(p);
        throw;
      }
    return p;
  }

public:
  /// Return the set of source nodes.
  const DynSetTree<Node *> &get_src_nodes() const noexcept
  {
    return src_nodes;
  }

  /// Return the set of sink nodes.
  const DynSetTree<Node *> &get_sink_nodes() const noexcept
  {
    return sink_nodes;
  }

public:
  /// Convert a multi-source network into a single super-source network.
  /// Throws if there is no source node or on allocation failure.
  void make_super_source()
  {
    if (src_nodes.size() == 1)
      return;

    ah_domain_error_if(src_nodes.size() == 0) << "network has no source nodes (it has cycles)";

    DynList<Node *> sources;
    for (auto it = src_nodes.get_it(); it.has_curr(); it.next_ne())
      sources.append(it.get_curr());

    Node *super_source = insert_node();
    try
      {
        for (auto it = sources.get_it(); it.has_curr(); it.next_ne())
          insert_arc(super_source, it.get_curr(), get_out_cap(it.get_curr()));
      }
    catch (...)
      {
        remove_node(super_source);  // with the arcs that it got
        throw;
      }

    super_source_node = super_source;
    with_super_source = true;
  }

  /** @brief Restore a super-source network to its original multi-source
      form, by removing the super source that make_super_source()
      inserted.

      @throw bad_alloc as remove_node(), if a source cannot go back to the
      set of sources. Then the super-source stays, maybe with fewer
      arcs, and another call goes on: the network keeps the super source,
      which is then one of several sources, and never takes another for
      it.
  */
  void unmake_super_source()
  {
    if (not with_super_source)
      return;

    if (super_source_node == nullptr)  // the flag was set by hand
      {
        with_super_source = false;
        return;
      }

    remove_node(super_source_node);  // which clears it and the flag
  }

  /// Convert a multi-sink network into a single super-sink network.
  /// Throws if there is no sink node or on allocation failure.
  void make_super_sink()
  {
    if (sink_nodes.size() == 1)
      return;

    ah_domain_error_if(sink_nodes.size() == 0) << "network has no sink nodes (it has cycles)";

    DynList<Node *> sinks;
    for (auto it = sink_nodes.get_it(); it.has_curr(); it.next_ne())
      sinks.append(it.get_curr());

    Node *super_sink = insert_node();
    try
      {
        for (auto it = sinks.get_it(); it.has_curr(); it.next_ne())
          insert_arc(it.get_curr(), super_sink, get_in_cap(it.get_curr()));
      }
    catch (...)
      {
        remove_node(super_sink);  // with the arcs that it got
        throw;
      }
    super_sink_node = super_sink;
    with_super_sink = true;
  }

  /** @brief Restore a super-sink network to its original multi-sink form,
      by removing the super sink that make_super_sink() inserted.

      @throw bad_alloc as unmake_super_source(); then another call goes
      on likewise.
  */
  void unmake_super_sink()
  {
    if (not with_super_sink)
      return;

    if (super_sink_node == nullptr)  // the flag was set by hand
      {
        with_super_sink = false;
        return;
      }

    remove_node(super_sink_node);  // which clears it and the flag
  }

  /** @brief Convert a multi-source/multi-sink network into
      super-source/super-sink.

      @throw domain_error if the network has no source or no sink.
      @throw bad_alloc if there is not enough memory. Then the network is
      as it was.
  */
  void make_super_nodes()
  {
    const bool had_super_source = with_super_source;
    make_super_source();
    try
      {
        make_super_sink();
      }
    catch (...)
      {
        if (not had_super_source)
          unmake_super_source();
        throw;
      }
  }

  /// Restore a super-source/super-sink network to its original form.
  void unmake_super_nodes()
  {
    unmake_super_source();
    unmake_super_sink();
  }

  /// Return an arbitrary source node.
  Node *get_source() const
  {
    return src_nodes.get_item();
  }

  /// Return an arbitrary sink node.
  Node *get_sink() const
  {
    return sink_nodes.get_item();
  }

  /** @brief Insert a new node by copying `node_info`.
   *
   * @param[in] node_info Info to copy into the node.
   * @return Pointer to the inserted node.
   * @throw bad_alloc If there is not enough memory.
   */
  Node *insert_node(const Node_Type &node_info)
  {
    return register_node(Graph::insert_node(node_info));
  }

  /// Insert a new node with default info.
  Node *insert_node()
  {
    return register_node(Graph::insert_node(Node_Type()));
  }

  /// Insert a new node by moving `info`.
  Node *insert_node(Node_Type &&info)
  {
    return register_node(Graph::insert_node(std::move(info)));
  }

  /// Construct a node in-place and insert it into the network.
  template <typename... Args>
  Node *emplace_node(Args &&...args)
  {
    return insert_node(Node_Type(std::forward<Args>(args)...));
  }

  /** @brief Insert the node @p p, allocated by the caller.
   *
   * @param[in] p Node to insert.
   * @return Pointer to the inserted node.
   * @throw bad_alloc If there is not enough memory; then @p p is removed
   * from the network and freed.
   */
  Node *insert_node(Node *p) override
  {
    Graph::insert_node(p);
    return register_node(p);
  }

  /** @brief Insert a capacitated arc with an initial flow.
   *
   * @param[in] src_node Source node.
   * @param[in] tgt_node Target node.
   * @param[in] cap Capacity value.
   * @param[in] flow Flow value.
   * @param[in] arc_info Arc info to store.
   * @return Pointer to the inserted arc.
   * @throw bad_alloc If there is not enough memory.
   * @throw overflow_error If `flow` exceeds `cap` or is negative. The
   * network is then as it was.
   */
  Arc *insert_arc(Node *src_node, Node *tgt_node, const Flow_Type &cap, const Flow_Type &flow,
                  const typename Arc::Arc_Type &arc_info = Arc_Type())
  {
    // As Net_Arc::check_arc(), before the arc enters the network
    ah_overflow_error_if(not (flow >= 0 and flow <= cap)) << "flow is greater than capacity";

    auto arc = Graph::insert_arc(src_node, tgt_node, arc_info);  // base insertion

    src_nodes.remove(tgt_node);  // update sources/sinks
    sink_nodes.remove(src_node);

    arc->cap = cap;
    arc->flow = flow;

    return arc;
  }

  template <typename... Args>
  /// Construct arc info in-place and insert the arc.
  Arc *emplace_arc(Node *src_node, Node *tgt_node, const Flow_Type &cap, const Flow_Type &flow,
                   Args &&...args)
  {
    return insert_arc(src_node, tgt_node, cap, flow, Arc_Type(std::forward<Args>(args)...));
  }

  /** @brief Connect a previously disconnected arc.
   *
   * The arc must already belong to the graph; no verification is performed.
   * Multigraphs are supported (no check for existing arcs).
   *
   * @param[in] arc Arc to connect.
   * @return Pointer to the connected arc.
   */
  Arc *connect_arc(Arc *arc)
  {
    Graph::connect_arc(arc);

    auto src = this->get_src_node(arc);
    auto tgt = this->get_tgt_node(arc);

    src_nodes.remove(tgt);   // target is no longer a source
    sink_nodes.remove(src);  // source is no longer a sink

    return arc;
  }

  /** @brief Insert an arc with capacity `cap` and zero flow. */
  Arc *insert_arc(Node *src_node, Node *tgt_node, const Flow_Type &cap)
  {
    return insert_arc(src_node, tgt_node, cap, 0, Arc_Type());
  }

  /** @brief Insert an arc with zero capacity and flow.
   *
   * This overload is disabled when Arc_Type equals Flow_Type to avoid
   * a signature clash with the capacity-only overload. It is also
   * disabled for arithmetic types to ensure that integer literals
   * match the capacity overload instead.
   */
  Arc *insert_arc(Node *src_node, Node *tgt_node, const Arc_Type &arc_info = Arc_Type())
    requires(not std::is_same_v<Arc_Type, Flow_Type> and not std::is_arithmetic_v<Arc_Type>)
  {
    return insert_arc(src_node, tgt_node, 0, 0, arc_info);
  }

  /** @brief Insert an arc with zero capacity and flow, for an arc info
   * that is arithmetic or of the type of the flow.
   *
   * The overload above is disabled for those types, and copy_graph(),
   * which the copy constructor uses, inserts the arcs with two
   * arguments.
   */
  Arc *insert_arc(Node *src_node, Node *tgt_node)
    requires(std::is_same_v<Arc_Type, Flow_Type> or std::is_arithmetic_v<Arc_Type>)
  {
    return insert_arc(src_node, tgt_node, 0, 0, Arc_Type());
  }

private:
  // Puts in the sets of sources and of sinks the nodes that arc leaves as
  // one when it goes. It is done before the arc goes, so that if a set
  // cannot take a node, the network is as it was.
  void register_ends_of_leaving(Arc *arc)
  {
    auto src = this->get_src_node(arc);
    auto tgt = this->get_tgt_node(arc);
    const bool new_source =
      get_in_degree(tgt) == 1 and src_nodes.insert(tgt) != nullptr;  // target becomes a source
    try
      {
        if (get_out_degree(src) == 1)
          sink_nodes.insert(src);  // source becomes a sink
      }
    catch (...)
      {
        if (new_source)
          src_nodes.remove(tgt);
        throw;
      }
  }

public:
  /** @brief Remove arc `arc` from the network.
   *
   * @throw bad_alloc if the set of sources or that of sinks cannot take
   * an end of the arc that becomes one. The network is then as it was.
   */
  void remove_arc(Arc *arc) override
  {
    register_ends_of_leaving(arc);
    Graph::remove_arc(arc);  // base removal
  }

  /** @brief Disconnect arc `arc` from the graph without deleting it.
   *
   * @throw bad_alloc as remove_arc().
   */
  void disconnect_arc(Arc *arc)
  {
    register_ends_of_leaving(arc);
    Graph::disconnect_arc(arc);  // base disconnection
  }

  /** @brief Remove node `p` and all its arcs from the network.
   *
   * @throw bad_alloc as remove_arc(), for the arcs of p. Then the arcs
   * that went are gone, p and the others are in the network, and the
   * sets of sources and sinks are right.
   */
  void remove_node(Node *p) override
  {
    const bool super_source = p == super_source_node;
    const bool super_sink = p == super_sink_node;
    Graph::remove_node(p);  // base removal
    src_nodes.remove(p);
    sink_nodes.remove(p);
    if (super_source)  // the network has no super source any more
      {
        super_source_node = nullptr;
        with_super_source = false;
      }
    if (super_sink)
      {
        super_sink_node = nullptr;
        with_super_sink = false;
      }
  }

  /// Copy-construct a network. Throws bad_alloc on allocation failure.
  Net_Graph(const Net_Graph &net)
    : Array_Graph<NodeT, ArcT>::Array_Graph(),
      Infinity(std::numeric_limits<typename Arc::Flow_Type>::max()),
      with_super_source(net.with_super_source), with_super_sink(net.with_super_sink)
  {
    copy_graph(*this, net, false);  // copy without mapping

    using Pair = std::pair<typename Net_Graph::Arc *, typename Net_Graph::Arc *>;
    zip(this->arcs(), net.arcs())
      .for_each([](const Pair &p)
      {
        auto atgt = p.first;
        auto asrc = p.second;
        atgt->cap = asrc->cap;
        atgt->flow = asrc->flow;
      });

    // The copy has the nodes of net in their order: the super nodes are
    // those at their places
    if (net.super_source_node != nullptr or net.super_sink_node != nullptr)
      {
        Node_Iterator<Net_Graph> jt(*this);
        for (Node_Iterator<Net_Graph> it(net); it.has_curr(); it.next_ne(), jt.next_ne())
          {
            if (it.get_curr() == net.super_source_node)
              super_source_node = jt.get_curr();
            if (it.get_curr() == net.super_sink_node)
              super_sink_node = jt.get_curr();
          }
      }
  }

  /// Swap contents with another network. O(1) operation.
  void swap(Net_Graph &other) noexcept
  {
    Graph::common_swap(other);  // swap num_nodes, num_arcs, digraph, cookie
    Graph::get_node_dlink().swap(other.get_node_dlink());
    Graph::get_arc_dlink().swap(other.get_arc_dlink());
    src_nodes.swap(other.src_nodes);
    sink_nodes.swap(other.sink_nodes);
    std::swap(Infinity, other.Infinity);
    std::swap(with_super_source, other.with_super_source);
    std::swap(with_super_sink, other.with_super_sink);
    std::swap(super_source_node, other.super_source_node);
    std::swap(super_sink_node, other.super_sink_node);
  }

  /// Move-construct a network. O(1) operation.
  Net_Graph(Net_Graph &&other) noexcept
    : Infinity(std::numeric_limits<typename Arc::Flow_Type>::max()), with_super_source(false),
      with_super_sink(false)
  {
    swap(other);
  }

  /// Move-assign a network. O(1) operation.
  Net_Graph &operator = (Net_Graph &&other) noexcept
  {
    if (this != &other)
      swap(other);
    return *this;
  }

  /// Copy-assign a network. Throws bad_alloc on allocation failure.
  Net_Graph &operator = (const Net_Graph &net)
  {
    if (this == &net)
      return *this;

    Net_Graph tmp(net);                     // copy construct
    tmp.get_cookie() = this->get_cookie();  // the network keeps its cookie
    swap(tmp);                              // swap with copy
    return *this;
  }

  /// Set the capacity of an arc.
  void set_cap(Arc *arc, const Flow_Type &cap)
  {
    ah_out_of_range_error_if(cap < arc->flow) << "capacity value is smaller than flow";

    arc->cap = cap;
  }

  /// Set the flow of an arc. Throws if `flow` exceeds capacity.
  void set_flow(Arc *arc, const Flow_Type &flow)
  {
    ah_out_of_range_error_if(flow > arc->cap) << "flow value is greater than capacity";

    arc->flow = flow;
  }

  /// Return the flow value of an arc.
  const Flow_Type &get_flow(Arc *arc) const noexcept
  {
    return arc->flow;
  }

  /// Return the capacity value of an arc.
  const Flow_Type &get_cap(Arc *arc) const noexcept
  {
    return arc->cap;
  }

  /// Reset all arc flows to zero.
  void reset()
  {
    for (Arc_Iterator<Net_Graph> it(*this); it.has_curr(); it.next_ne())
      it.get_curr()->flow = 0;
  }

  /** @brief Validate flow-conservation and capacity constraints.
   *
   * A floating flow is valid but for rounding: every node keeps its flow
   * within node_tolerance() of its own flows, and the flow out of the
   * sources and that into the sinks differ at most by the sum of those
   * tolerances. Equal flows, also infinite ones, as those of a path of
   * arcs of infinite capacity, balance; an infinite flow and a finite one
   * do not.
   *
   * @return true if the network is valid; false otherwise.
   */
  bool check_network() const
  {
    if (src_nodes.is_empty() or sink_nodes.is_empty())
      return false;

    const bool flow_balanced = same_totals(total_source_out_flow(), total_sink_in_flow());

    return this->nodes().all(
             [this](Node *p)
      {
        return check_node(p);
      }) and
      this->arcs().all(
        [](Arc *a)
      {
        return a->check_arc();
      }) and
      flow_balanced;
  }

  /** @brief Return the total flow value of the network: the flow out of
      its sources, which a valid flow also takes into its sinks, as
      check_network() takes them. */
  Flow_Type flow_value() const
  {
    ah_domain_error_if(src_nodes.is_empty() or sink_nodes.is_empty())
      << "network has no source or sink nodes";

    const auto total_out = total_source_out_flow();
    assert(same_totals(total_out, total_sink_in_flow()));
    return total_out;
  }

  /// True if the network has a super-source.
  bool with_super_source;

  /// True if the network has a super-sink.
  bool with_super_sink;

  /// Default constructor.
  Net_Graph()
    : Infinity(std::numeric_limits<typename Arc::Flow_Type>::max()), with_super_source(false),
      with_super_sink(false)
  { /* empty */
  }

  /** @brief Stream a path with arc (cap,flow) tuples. */
  friend std::ostream &operator << (std::ostream &s, const Path<Net_Graph> &path)
  {
    if (path.is_empty())
      return s << "Path is Empty";

    const Net_Graph &net = path.get_graph();
    typename Path<Net_Graph>::Iterator it(path);
    s << it.get_current_node()->get_info();
    for (; it.has_current_arc(); it.next_ne())
      {
        typename Net_Graph::Arc *a = it.get_current_arc_ne();
        s << "(" << a->cap << "," << a->flow << ")"
          << net.get_connected_node(a, it.get_current_node_ne())->get_info();
      }
    return s;
  }
};

/** @brief Arc entry for a semi-path.
 *
 * Second field is true for a forward arc, false for a residual arc.
 *
 * @ingroup Networks
 */
template <class Net>
using Parc = std::tuple<typename Net::Arc *, bool>;  // second field marks forward.

/** @brief Semi-path tuple:
 * - bool: whether a path was found
 * - Flow_Type: slack value of the path
 * - DynList<Parc<Net>>: arc list with direction flags
 *
 * @ingroup Networks
 */
template <class Net>
using SemiPath = std::tuple<bool, typename Net::Flow_Type, DynList<Parc<Net>>>;

/// Print a semi-path to stdout.
template <class Net>
inline void print(const DynList<Parc<Net>> &sp)
{
  if (sp.is_empty())
    {
      std::cout << "Semi path is Empty";
      return;
    }

  for (auto it = sp.get_it(); it.has_curr(); it.next_ne())
    {
      const Parc<Net> &pa = it.get_curr();
      auto a = get<0>(pa);
      auto s = static_cast<typename Net::Node *>(a->src_node);
      auto t = static_cast<typename Net::Node *>(a->tgt_node);
      std::cout << s->get_info() << "(" << a->flow << "," << a->cap << ")" << t->get_info() << " "
                << (get<1>(pa) ? "Normal" : "Reduced") << '\n';
    }
}

namespace net_detail {
// Adds slack to the flow of arc. A floating slack that covers what the
// arc has left takes the flow to the capacity, exactly, so that the arc
// that bounds a path is saturated, as it would be without rounding,
// and no arc goes beyond it.
template <class Arc, typename Flow_Type>
inline void add_flow(Arc *arc, const Flow_Type &slack) noexcept
{
  if constexpr (std::is_floating_point_v<Flow_Type>)
    if (arc->cap - arc->flow <= slack)
      {
        arc->flow = arc->cap;
        return;
      }
  arc->flow += slack;
}

// The largest excess of a node of degree arcs, whose flows into and
// out of it are in and out, that is a residue of rounding, and not flow
// to push: for a floating flow, the errors that its sums of flows, and
// the pushes of a discharge, may make, an epsilon of the larger of in
// and out by each of its arcs and two more; zero for an integer flow. A
// push-relabel algorithm that pushed it could move it around forever,
// as no arc may take it. The scale is the node's own flows, not the
// capacities of the network nor those of the node: a node may receive a
// small flow through a small arc beside a large one, and that flow is
// not a residue; nor is 21 beside 1e15, though 256 epsilons of 1e15
// are 57.
template <typename Flow_Type>
inline Flow_Type node_residue(const Flow_Type &in, const Flow_Type &out, size_t arcs) noexcept
{
  if constexpr (std::is_floating_point_v<Flow_Type>)
    return std::numeric_limits<Flow_Type>::epsilon() * Flow_Type(arcs + 2) * std::max(in, out);
  else
    {
      (void) in;
      (void) out;
      (void) arcs;
      return Flow_Type{0};
    }
}

// Whether node p of net, which a preflow algorithm has just pushed
// flow into, does not send more than it receives, beyond the
// tolerance of check_node(); for an integer flow, whether it has an
// excess
template <class Net>
bool not_short(const Net &net, typename Net::Node *p)
{
  const auto in = net.get_in_flow(p), out = net.get_out_flow(p);
  if constexpr (std::is_floating_point_v<typename Net::Flow_Type>)
    return in - out >= -Net::node_tolerance(in, out);
  else
    return in > out;
}

// Subtracts slack from the flow of arc, along an augmenting path. A
// floating flow that is left next to zero, or below it, relative to
// the numbers of the subtraction, is zero, as the slack of the path
// was computed on another arc, or the flow added up rounding errors.
// The preflows do not take it: they push with push_excess().
template <class Arc, typename Flow_Type>
inline void subtract_flow(Arc *arc, const Flow_Type &slack) noexcept
{
  if constexpr (std::is_floating_point_v<Flow_Type>)
    {
      const Flow_Type before = arc->flow;
      arc->flow -= slack;
      if (arc->flow <= 256 * std::numeric_limits<Flow_Type>::epsilon() * std::max(before, slack))
        arc->flow = 0;
    }
  else
    arc->flow -= slack;
}

/** A sum of floating flows that keeps the error of its rounding, as
    hi + lo, by the error-free addition of Knuth: a small flow beside a
    large one, as 0.53 beside 7e13, which the sum rounds away, comes
    back when the large one goes. The preflows and HLPP sum so the
    flows of their nodes. It needs strict floating arithmetic, which
    -ffast-math and /fp:fast are not: with them a floating flow does not
    compile. */
template <typename Flow_Type>
struct Flow_Sum
{
#if defined(__FAST_MATH__) || defined(_M_FP_FAST)
  static_assert(not std::is_floating_point_v<Flow_Type>,
                "The floating flows of tpl_net.H need strict floating "
                "arithmetic, which -ffast-math and /fp:fast are not");
#endif

  Flow_Type hi{0};  ///< The sum, rounded
  Flow_Type lo{0};  ///< What its rounding lost

  /// Adds x
  void add(const Flow_Type &x) noexcept
  {
    const Flow_Type s = hi + x;
    const Flow_Type b = s - hi;
    lo += (hi - (s - b)) + (x - b);
    hi = s;
  }

  /// Replaces the term from, which the sum has, by to
  void replace(const Flow_Type &from, const Flow_Type &to) noexcept
  {
    add(-from);
    add(to);
  }

  /// The sum, rounded once
  [[nodiscard]] Flow_Type value() const noexcept
  {
    return hi + lo;
  }

  /// This sum less other, rounded once but for the errors of lo
  [[nodiscard]] Flow_Type minus(const Flow_Sum &other) const noexcept
  {
    Flow_Sum d = *this;
    d.add(-other.hi);
    return d.hi + (d.lo - other.lo);
  }
};

// Sums into in and out the flows into and out of node p of net
template <class Net>
void node_flows(const Net &net, typename Net::Node *p, Flow_Sum<typename Net::Flow_Type> &in,
                Flow_Sum<typename Net::Flow_Type> &out) noexcept
{
  for (typename Net::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr_ne();
      if (net.get_src_node(arc) == p)
        out.add(arc->flow);
      if (net.get_tgt_node(arc) == p)
        in.add(arc->flow);
    }
}

// Pushes the excess of a node whose flows into and out of it are in
// and out along arc, forward or back, as much as the arc takes, for a
// floating flow: the flow of the arc becomes what keeps the node in
// balance, in - (out - flow) forward and out - (in - flow) back, or the
// capacity, or zero, if that is beyond them. The sums keep what their
// rounding loses, so that a small flow beside a large one is not lost,
// as 0.05 left by the return of 1e15. Updates in and out by what the
// arc took.
template <class Arc, typename Flow_Type>
inline void push_excess(Arc *arc, bool forward, Flow_Sum<Flow_Type> &in,
                        Flow_Sum<Flow_Type> &out) noexcept
{
  const Flow_Type before = arc->flow;
  Flow_Sum<Flow_Type> &with = forward ? out : in;  // the sum that has arc
  const Flow_Sum<Flow_Type> &other = forward ? in : out;
  with.add(-before);
  const Flow_Type balanced = other.minus(with);
  if (forward)
    arc->flow = balanced >= arc->cap ? arc->cap : balanced;
  else
    arc->flow = balanced <= Flow_Type{0} ? Flow_Type{0} : balanced;
  with.add(arc->flow);
}

/** Returns the dust of a floating flow that a preflow algorithm left
    in net, from source to sink: what each node but them keeps beyond
    the tolerance of check_node() goes back toward the source, along
    arcs with flow from it, and what each sends beyond it that it does
    not receive goes forward toward the sink, along arcs with flow to
    it; what a node still keeps or lacks beyond that tolerance, as on
    an island of arcs with flow apart from both, goes along a shortest
    path of arcs with flow to the first node that takes it; and when
    nothing reaches the sink, what still leaves the source, which the
    rounding of the sums of the nodes may hide, is taken from the arcs
    of the source with the least flow, if it is within the tolerances
    of the nodes (and the other way). What a node keeps or lacks within
    its tolerance stays: it is the rounding of the sums of its flows,
    as 0.1 + 0.8 in and their rounded sum out, and returning it lowered
    by an ulp an arc that the flow saturates, as one of a minimum cut,
    so that a path of residual capacity went from the source to the
    sink. Flows only go down. Afterwards every node keeps its flow
    within the tolerance of check_node(), and a network whose maximum
    flow is 0 gives 0. Nothing for an
    integer flow, and, but a pass over the nodes, nothing if no node
    keeps or lacks any flow. O((b + 1)(V + E) + d log d), where b, the
    number of searches, is about that of the nodes that the first moves
    leave beyond their tolerance, at most V + E, and d is the degree of
    the source or the sink. The augmenting paths return theirs too. */
template <class Net>
void return_dust(Net &net, typename Net::Node *source, typename Net::Node *sink)
{
  using Flow_Type = typename Net::Flow_Type;
  if constexpr (std::is_floating_point_v<Flow_Type>)
    {
      using Node = typename Net::Node;
      using Arc = typename Net::Arc;

      // Nothing to do, and nothing to allocate, if every node but the
      // ends keeps its flow within the tolerance of check_node(), as it
      // sums the flows, where it stays, and the totals agree, or both
      // carry flow: a pass over the arcs, the least that the maximum
      // flows of a floating flow pay for their residues
      {
        bool within = true;
        Flow_Type sent{0}, received{0};
        for (Node_Iterator<Net> it(net); within and it.has_curr(); it.next_ne())
          {
            Node *p = it.get_curr_ne();
            Flow_Type in{0}, out{0};
            for (typename Net::Node_Arc_Iterator a(p); a.has_curr(); a.next_ne())
              {
                Arc *arc = a.get_curr_ne();
                if (net.get_src_node(arc) == p)
                  out += arc->flow;
                if (net.get_tgt_node(arc) == p)
                  in += arc->flow;
              }
            if (p == source)
              sent = out - in;
            else if (p == sink)
              received = in - out;
            else
              within = std::abs(in - out) <= Net::node_tolerance(in, out);
          }
        if (within and (sent == received or (sent != Flow_Type{0} and received != Flow_Type{0})))
          return;
      }

      struct Info
      {
        Flow_Sum<Flow_Type> in;  // which keep what their rounding loses
        Flow_Sum<Flow_Type> out;
        long from_source = -1;  // depth from the source, by arcs with flow
        long to_sink = -1;      // depth to the sink, by arcs with flow
        Arc *parent = nullptr;  // by which a search reached it
        size_t stamp = 0;       // the last search that reached it
      };
      // The storage first, so that it outlives the saver of cookies,
      // and all of it before any flow moves
      auto info = Array<Info>::create(net.vsize());
      Array<Node *> order, queue;  // of the searches
      order.reserve(net.vsize());
      queue.reserve(net.vsize());
      Array<Arc *> end_arcs;  // of the source or the sink
      end_arcs.reserve(std::max(net.get_num_arcs(source), net.get_num_arcs(sink)));
      Cookie_Saver<Net> saver(net, true, false);
      {
        size_t i = 0;
        for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne(), ++i)
          NODE_COOKIE(it.get_curr_ne()) = &info(i);
      }
      auto at = [](Node *p) -> Info &
        {
          return *static_cast<Info *>(NODE_COOKIE(p));
        };
      for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
        {
          Arc *a = it.get_curr_ne();
          at(net.get_src_node(a)).out.add(a->flow);
          at(net.get_tgt_node(a)).in.add(a->flow);
        }

      // Lowers the flow of a by d, at most its flow
      auto lower = [&](Arc *a, const Flow_Type &d)
        {
          const Flow_Type before = a->flow;
          a->flow = d >= before ? Flow_Type{0} : before - d;
          at(net.get_src_node(a)).out.replace(before, a->flow);
          at(net.get_tgt_node(a)).in.replace(before, a->flow);
        };
      auto between = [source, sink](Node *p)
        {
          return p != source and p != sink;
        };
      auto excess = [&](Node *p)
        {
          return at(p).in.minus(at(p).out);
        };

      // The nodes by depth from start, along arcs with flow, forward if
      // forward, else backward; none goes on from stop
      auto search = [&](Node *start, Node *stop, bool forward, long Info::*depth)
        {
          order.clear();
          at(start).*depth = 0;
          order.append(start);
          for (size_t k = 0; k < order.size(); ++k)
            {
              Node *u = order(k);
              if (u == stop)
                continue;
              for (typename Net::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
                {
                  Arc *a = it.get_curr_ne();
                  Node *from = forward ? net.get_src_node(a) : net.get_tgt_node(a);
                  Node *v = forward ? net.get_tgt_node(a) : net.get_src_node(a);
                  if (from == u and a->flow > Flow_Type{0} and at(v).*depth < 0)
                    {
                      at(v).*depth = at(u).*depth + 1;
                      order.append(v);
                    }
                }
            }
        };

      // Whether p keeps or lacks more than the tolerance of check_node()
      // of its flows: what it keeps or lacks within it stays, as the
      // rounding of their sums
      auto tolerance_of = [&](Node *p)
        {
          return Net::node_tolerance(at(p).in.value(), at(p).out.value());
        };
      auto off = [&](Node *p)
        {
          return std::abs(excess(p)) > tolerance_of(p);
        };

      // What each node keeps beyond its tolerance goes back, the
      // deepest first, along arcs from nodes nearer to the source, which
      // come later
      search(source, sink, true, &Info::from_source);
      for (size_t k = order.size(); k-- > 1;)
        if (Node *v = order(k); between(v) and off(v))
          for (typename Net::Node_Arc_Iterator it(v); it.has_curr() and excess(v) > Flow_Type{0};
               it.next_ne())
            {
              Arc *a = it.get_curr_ne();
              Node *u = net.get_src_node(a);
              if (net.get_tgt_node(a) == v and u != v and a->flow > Flow_Type{0} and
                  at(u).from_source >= 0 and at(u).from_source < at(v).from_source)
                lower(a, excess(v));
            }

      // What each node lacks beyond it goes forward likewise, toward
      // the sink
      search(sink, source, false, &Info::to_sink);
      for (size_t k = order.size(); k-- > 1;)
        if (Node *v = order(k); between(v) and off(v))
          for (typename Net::Node_Arc_Iterator it(v); it.has_curr() and excess(v) < Flow_Type{0};
               it.next_ne())
            {
              Arc *a = it.get_curr_ne();
              Node *w = net.get_tgt_node(a);
              if (net.get_src_node(a) == v and w != v and a->flow > Flow_Type{0} and
                  at(w).to_sink >= 0 and at(w).to_sink < at(v).to_sink)
                lower(a, -excess(v));
            }

      // What is left beyond the tolerance of check_node() of its node
      // goes, an excess backward and a lack forward, along a shortest
      // path of arcs with flow, to the first node that takes it: the
      // source or the sink, a node with the opposite beyond its own
      // tolerance, a node of flows large enough to take it within their
      // tolerance, or else the first node with some of the opposite. So
      // an island of arcs with flow apart from both ends, and a path of
      // dust from the source into a circulation, in which rounding hid
      // it, go, and no move goes around a cycle. What a node keeps or
      // lacks within its tolerance stays: it is the rounding of the sums
      // of its flows. Each search takes a path to zero at one of its
      // arcs, settles its node, or settles the node that takes it; the
      // searches are bounded all the same
      size_t stamp = 0;
      auto settle = [&]
        {
          size_t budget = net.vsize() + net.esize();
          for (Node_Iterator<Net> nt(net); nt.has_curr() and budget > 0; nt.next_ne())
            for (Node *v = nt.get_curr_ne(); between(v) and off(v) and budget > 0; --budget)
              {
                const bool keeps = excess(v) > Flow_Type{0};
                const Flow_Type d = std::abs(excess(v));
                auto opposite = [&](Node *w)
                  {
                    const Flow_Type e = excess(w);
                    return between(w) and (keeps ? e < Flow_Type{0} : e > Flow_Type{0});
                  };
                auto takes_all = [&](Node *w)
                  {
                    return not between(w) or std::abs(excess(w)) + d <= tolerance_of(w);
                  };
                // A search from v, backward if it keeps, else forward,
                // to the first node that takes all, or has the opposite
                // beyond its tolerance, or else to the first that has
                // some of the opposite, which, as the flow decomposes
                // into paths and cycles, a search finds
                ++stamp;
                at(v).stamp = stamp;
                queue.clear();
                queue.append(v);
                Node *w = nullptr, *some = nullptr;
                for (size_t k = 0; k < queue.size() and w == nullptr; ++k)
                  for (typename Net::Node_Arc_Iterator it(queue(k)); it.has_curr(); it.next_ne())
                    {
                      Arc *a = it.get_curr_ne();
                      Node *near = keeps ? net.get_tgt_node(a) : net.get_src_node(a);
                      Node *x = keeps ? net.get_src_node(a) : net.get_tgt_node(a);
                      if (near != queue(k) or x == near or not (a->flow > Flow_Type{0}) or
                          at(x).stamp == stamp)
                        continue;
                      at(x).stamp = stamp;
                      at(x).parent = a;
                      if (takes_all(x) or (opposite(x) and off(x)))
                        {
                          w = x;
                          break;
                        }
                      if (some == nullptr and opposite(x))
                        some = x;
                      queue.append(x);
                    }
                if (w == nullptr)
                  w = some;
                if (w == nullptr)
                  break;  // nothing takes it
                auto next = [&](Node *x)
                  {
                    Arc *a = at(x).parent;
                    return keeps ? net.get_tgt_node(a) : net.get_src_node(a);
                  };
                Flow_Type m = d;  // what goes: at most the flow of the path
                for (Node *x = w; x != v; x = next(x))
                  m = std::min(m, at(x).parent->flow);
                if (not takes_all(w))  // only what it has of the opposite
                  m = std::min(m, std::abs(excess(w)));
                for (Node *x = w; x != v;)
                  {
                    Arc *a = at(x).parent;
                    x = next(x);
                    lower(a, m);
                  }
              }
        };
      settle();

      // When nothing reaches the sink, what still leaves the source, or
      // the other way, which the rounding of the sums of the nodes
      // between them hides, is dust too, if it is within the tolerances
      // of those nodes: the arcs of the source, or of the sink, with the
      // least flow lose it, so that a maximum flow of 0 gives 0. A
      // difference of the totals when both carry flow is what rounding
      // hides between them, and it stays, within the tolerance that
      // check_network() takes
      Flow_Type tolerance{0};
      for (const Info &x : info)
        tolerance += Net::node_tolerance(x.in.value(), x.out.value());
      const Flow_Type sent = at(source).out.minus(at(source).in);
      const Flow_Type received = at(sink).in.minus(at(sink).out);
      const Flow_Type apart = sent - received;
      if ((sent == Flow_Type{0} or received == Flow_Type{0}) and apart != Flow_Type{0} and
          std::abs(apart) <= tolerance)
        {
          Node *end = apart > Flow_Type{0} ? source : sink;
          for (typename Net::Node_Arc_Iterator it(end); it.has_curr(); it.next_ne())
            if (Arc *a = it.get_curr_ne(); a->flow > Flow_Type{0} and
                (end == source ? net.get_src_node(a) : net.get_tgt_node(a)) == end and
                net.get_src_node(a) != net.get_tgt_node(a))
              end_arcs.append(a);
          introsort(end_arcs, [](Arc *a, Arc *b)
            {
              return a->flow < b->flow;
            });
          Flow_Type left = std::abs(apart);
          for (Arc *a : end_arcs)
            {
              if (not (left > Flow_Type{0}))
                break;
              const Flow_Type before = a->flow;
              lower(a, left);
              left -= before - a->flow;
            }
          settle();  // what that left, next to the source or the sink
        }
    }
  else
    {
      (void) net;
      (void) source;
      (void) sink;
    }
}
}  // namespace net_detail

/** @brief Increase flow along an augmenting path.
 *
 * With a floating type of flow, a flow that rounding leaves next to the
 * capacity of its arc, or next to zero, is set to it.
 *
 * @param[in,out] net Flow network to update.
 * @param[in] path Augmenting path.
 * @return Slack value used to increase the flow.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type increase_flow(Net &net, const Path<Net> &path)
{
  typename Net::Flow_Type slack = net.Infinity;  // bottleneck
  using Tuple = std::tuple<typename Net::Node *, typename Net::Arc *>;

  // Compute the bottleneck of the augmenting path.
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next_ne())
    {
      Tuple t = it.get_tuple_ne();
      auto p = get<0>(t);
      auto arc = get<1>(t);
      const auto w = remaining_flow<Net>(p, arc);
      if (w < slack)
        slack = w;
    }

  // Increase flow along the augmenting path.
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next_ne())
    {
      auto t = it.get_tuple_ne();
      auto p = get<0>(t);
      auto arc = get<1>(t);

      if (is_residual<Net>(p, arc))
        net_detail::subtract_flow(arc, slack);
      else
        net_detail::add_flow(arc, slack);

      assert(arc->check_arc());
    }

  return slack;
}

/** @brief Increase flow along a semi-path by a given slack.
 *
 * With a floating type of flow, a flow that rounding leaves next to the
 * capacity of its arc, or next to zero, is set to it. A node may then
 * keep or lack a residue of rounding beyond the tolerance of
 * check_node(), when its flows are such residues, which came from arcs of
 * larger flows: aumenting_path_maximum_flow() returns them at its end.
 *
 * @param[in,out] net Flow network to update.
 * @param[in] semi_path Semi-path arc list.
 * @param[in] slack Slack to apply (not validated).
 *
 * @ingroup Networks
 */
template <class Net>
void increase_flow(Net &net, const DynList<Parc<Net>> &semi_path, const typename Net::Flow_Type slack)
{
  (void) net;  // May be unused when NDEBUG disables assert().

  // Increase flow along the semi-path.
  for (auto it = semi_path.get_it(); it.has_curr(); it.next_ne())
    {
      auto p = it.get_curr();
      auto arc = get<0>(p);
      if (get<1>(p))  // forward arc
        net_detail::add_flow(arc, slack);
      else
        net_detail::subtract_flow(arc, slack);

      assert(arc->check_arc());
    }
  if constexpr (not std::is_floating_point_v<typename Net::Flow_Type>)
    assert(net.check_network());
}

/** @brief Decrease flow along a semi-path by a given slack.
 *
 * This reverses the effect of increase_flow by negating forward/residual
 * arc directions. With a floating type of flow, a flow that rounding
 * leaves next to the capacity of its arc, or next to zero, is set to it.
 *
 * @param[in,out] net Flow network to update.
 * @param[in] semi_path Semi-path arc list.
 * @param[in] slack Slack to apply (not validated).
 *
 * @ingroup Networks
 */
template <class Net>
void decrease_flow(Net &net, const DynList<Parc<Net>> &semi_path, const typename Net::Flow_Type slack)
{
  (void) net;

  // Decrease flow: reverse the direction logic from increase_flow
  for (auto it = semi_path.get_it(); it.has_curr(); it.next_ne())
    {
      auto p = it.get_curr();
      auto arc = get<0>(p);
      if (get<1>(p))  // forward arc: decrease instead of increase
        net_detail::subtract_flow(arc, slack);
      else  // residual arc: increase instead of decrease
        net_detail::add_flow(arc, slack);

      assert(arc->check_arc());
    }
}

/** @brief Augmenting-path search over a directed network.
 *
 * The network is modeled with a graph class (not a digraph class).
 *
 * @ingroup Networks
 */
template <class Net, template <typename T> class Q>
class Find_Aumenting_Path
{
  const Net &net;

  // Return end node if a path is found. The cookie of each node reached
  // keeps the arc by which it was reached, which the paths take: with
  // parallel arcs, another one could have less slack than min_slack.
  typename Net::Node *search(typename Net::Node *start, typename Net::Node *end,
                             typename Net::Flow_Type min_slack)
  {
    using Itor = Net_Iterator<Net>;
    net.reset_nodes();
    net.reset_bit_arcs();  // only their states: their cookies stay, as
                           // those of an auxiliary network to the original

    start->set_state(Processed);
    Q<typename Net::Arc *> q;
    for (Itor it(start); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        if (remaining_flow<Net>(start, a) < min_slack)
          continue;
        auto tgt = net.get_connected_node(a, start);
        tgt->set_state(Processing);
        a->set_state(Processing);
        q.put(a);
      }

    typename Net::Node *curr = nullptr;
    while (not q.is_empty())
      {
        auto arc = q.get();
        assert(arc->state() == Processing);
        arc->set_state(Processed);

        auto s = net.get_src_node(arc);
        auto t = net.get_tgt_node(arc);

        if (s->state() == Processed and t->state() == Processed)
          continue;

        curr = s->state() == Processed ? t : s;
        assert(curr->state() == Processing);
        curr->set_state(Processed);
        NODE_COOKIE(curr) = arc;

        if (curr == end)
          return curr;

        for (Itor it(curr); it.has_curr(); it.next_ne())
          {
            auto a = it.get_curr();

            // Skip arcs already processed or already in queue
            if (a->state() != Unprocessed)
              continue;

            if (remaining_flow<Net>(curr, a) < min_slack)
              continue;

            auto tgt = net.get_connected_node(a, curr);
            if (tgt->state() == Processed)
              {
                a->set_state(Processed);
                continue;
              }

            a->set_state(Processing);
            q.put(a);
            tgt->set_state(Processing);
          }
      }  // end while

    return nullptr;
  }

  /// Build a Path from `start` to `end` with minimum slack.
  Path<Net> find(typename Net::Node *start, typename Net::Node *end,
                 const typename Net::Flow_Type &min_slack = typename Net::Flow_Type{0})
  {
    auto curr = search(start, end, min_slack);

    Path<Net> ret(net);
    if (not curr)
      return ret;

    assert(curr == end);

    ret.insert(curr);
    while (curr != start)
      {
        auto arc = static_cast<typename Net::Arc *>(NODE_COOKIE(curr));
        ret.insert(arc);  // the arc that the search took, of the parallels
        curr = net.get_connected_node(arc, curr);
      }

    return ret;
  }

  /// Build a SemiPath from `start` to `end` with minimum slack.
  SemiPath<Net> find_path(typename Net::Node *start, typename Net::Node *end,
                          typename Net::Flow_Type min_slack = typename Net::Flow_Type{0})
  {
    auto t = search(start, end, min_slack);
    if (not t)
      return std::make_tuple(false, typename Net::Flow_Type{0}, DynList<Parc<Net>>());

    assert(t == end);

    DynList<Parc<Net>> semi_path;
    auto m = std::numeric_limits<typename Net::Flow_Type>::max();
    while (t != start)
      {
        auto a = static_cast<typename Net::Arc *>(NODE_COOKIE(t));
        auto s = net.get_connected_node(a, t);
        bool normal = a->tgt_node == t;
        auto slack = normal ? a->cap - a->flow : a->flow;
        m = std::min(m, slack);
        semi_path.insert(std::make_tuple(a, normal));
        t = s;
      }

    return std::make_tuple(true, m, std::move(semi_path));
  }

public:
  /// Find an augmenting semi-path with at least `min_slack`.
  SemiPath<Net> find_aum_path(typename Net::Flow_Type min_slack = 0.0)
  {
    return find_path(net.get_source(), net.get_sink(), min_slack);
  }

  /// Find a decrementing semi-path with at least `min_slack`.
  SemiPath<Net> find_dec_path(typename Net::Flow_Type min_slack = 0.0)
  {
    return find_path(net.get_sink(), net.get_source(), min_slack);
  }

  /// Construct a finder for `net`.
  Find_Aumenting_Path(const Net &__g) : net(__g)
  {
    // empty
  }

  /// Find a concrete Path with at least `min_slack`.
  Path<Net> operator () (typename Net::Node *start, typename Net::Node *end,
                         typename Net::Flow_Type min_slack = 0)
  {
    return find(start, end, min_slack);
  }

  /// Find an augmenting semi-path from the network source to sink.
  SemiPath<Net> operator () (typename Net::Flow_Type min_slack = 0)
  {
    return find_aum_path(min_slack);
  }

  /** @brief Fill `semi_path` and return the slack value.
   *
   * Returns 0 if no path is found.
   */
  typename Net::Flow_Type semi_path(typename Net::Node *start, typename Net::Node *end,
                                    DynList<Parc<Net>> &semi_path,
                                    const typename Net::Flow_Type &min_slack = 0)
  {
    semi_path.empty();

    auto result = find_path(start, end, min_slack);
    if (not get<0>(result))
      return typename Net::Flow_Type{};

    semi_path = std::move(get<2>(result));
    return get<1>(result);
  }
};

/** @brief Augmenting-path finder using DFS (stack).

    @warning With a floating flow, its paths may not end on networks of
    some hundreds of nodes with capacities not exact in binary, as tenths:
    see aumenting_path_maximum_flow().
*/
template <class Net>
using Find_Aumenting_Path_DFS = Find_Aumenting_Path<Net, DynListStack>;

/// Augmenting-path finder using BFS (queue).
template <class Net>
using Find_Aumenting_Path_BFS = Find_Aumenting_Path<Net, DynListQueue>;

/** @brief Find an augmenting path using DFS or BFS based on `Q`.
 *
 * @param[in] net Flow network.
 * @param[in] min_slack Minimum slack required.
 *
 * @ingroup Networks
 */
template <class Net, template <typename T> class Q>
Path<Net> find_aumenting_path(const Net &net, const typename Net::Flow_Type &min_slack)
{
  auto s = net.get_source();
  auto t = net.get_sink();
  return Find_Aumenting_Path<Net, Q>(net)(s, t, min_slack);
}

/** @brief Find an augmenting path using DFS. */
template <class Net>
Path<Net> find_aumenting_path_dfs(const Net &net, const typename Net::Flow_Type &min_slack)
{
  return find_aumenting_path<Net, DynListStack>(net, min_slack);
}

/** @brief Find an augmenting path using BFS. */
template <class Net>
Path<Net> find_aumenting_path_bfs(const Net &net, const typename Net::Flow_Type &min_slack)
{
  return find_aumenting_path<Net, DynListQueue>(net, min_slack);
}

/** @brief Find an augmenting semi-path using DFS or BFS based on `Q`.
 *
 * @param[in] net Flow network.
 * @param[in] slack Minimum slack required.
 * @return Semi-path tuple (found, slack, arc list).
 *
 * @ingroup Networks
 */
template <class Net, template <typename T> class Q>
SemiPath<Net> find_aumenting_semi_path(const Net &net, const typename Net::Flow_Type &slack)
{
  return Find_Aumenting_Path<Net, Q>(net).find_aum_path(slack);
}

/** @brief Find an augmenting semi-path using DFS. */
template <class Net>
SemiPath<Net> find_aumenting_semi_path_dfs(const Net &net, const typename Net::Flow_Type &slack)
{
  return find_aumenting_semi_path<Net, DynListStack>(net, slack);
}

/** @brief Find an augmenting semi-path using BFS. */
template <class Net>
SemiPath<Net> find_aumenting_semi_path_bfs(const Net &net, const typename Net::Flow_Type &slack)
{
  return find_aumenting_semi_path<Net, DynListQueue>(net, slack);
}

/** @brief Find a decrementing semi-path using DFS or BFS based on `Q`.
 *
 * @param[in] net Flow network.
 * @param[in] slack Minimum slack required.
 * @return Semi-path tuple (found, slack, arc list).
 *
 * @ingroup Networks
 */
template <class Net, template <typename T> class Q>
SemiPath<Net> find_decrementing_path(const Net &net, const typename Net::Flow_Type &slack)
{
  return Find_Aumenting_Path<Net, Q>(net).find_dec_path(slack);
}

/** @brief Find a decrementing semi-path using DFS. */
template <class Net>
SemiPath<Net> find_decrementing_path_dfs(const Net &net, const typename Net::Flow_Type &slack)
{
  return find_decrementing_path<Net, DynListStack>(net, slack);
}

/** @brief Find a decrementing semi-path using BFS. */
template <class Net>
SemiPath<Net> find_decrementing_path_bfs(const Net &net, const typename Net::Flow_Type &slack)
{
  return find_decrementing_path<Net, DynListQueue>(net, slack);
}

/// Residual node used by preflow-push algorithms.
template <class Net>
struct PP_Res_Node : public Net_Node<Empty_Class>
{
  using Base = Net_Node<Empty_Class>;

  typename Net::Flow_Type in_flow = 0;
  typename Net::Flow_Type out_flow = 0;

  /// Default constructor
  PP_Res_Node() : Base() {}

  /// Constructor from node info
  PP_Res_Node(const Empty_Class &info) : Base(info) {}

  /// Move constructor from node info
  PP_Res_Node(Empty_Class &&info) : Base(std::move(info)) {}
};

/** Arc of the residual network of the preflow-push algorithms. Each arc of
    the network has two: its image, whose `img` points to it, and the
    reverse one, the residual, whose `img` is null; `dup` points from each
    to the other. */
template <class Net>
struct __Res_Arc : public Net_Arc<Empty_Class, typename Net::Flow_Type>
{
  using Base = Net_Arc<Empty_Class, typename Net::Flow_Type>;

  typename Net::Arc *img = nullptr;  // nullptr indicates residual arc
  __Res_Arc *dup = nullptr;

  /// Default constructor
  __Res_Arc() : Base() {}

  /// Constructor from arc info
  __Res_Arc(const Empty_Class &info) : Base(info) {}

  /// Move constructor from arc info
  __Res_Arc(Empty_Class &&info) : Base(std::move(info)) {}

  /// Return true if this is a residual arc.
  bool is_residual() const
  {
    return img == nullptr;
  }
};

/// Residual network for augmenting-path algorithms.
template <class Net>
using AP_Res_Net = Array_Digraph<Net_Node<Empty_Class>, __Res_Arc<Net>>;

/// Residual network for preflow-push algorithms.
template <class Net>
using PP_Res_Net = Array_Digraph<PP_Res_Node<Net>, __Res_Arc<Net>>;

/// Create residual arcs for `a` in the residual network.
template <class Net>
inline void create_residual_arc(const Net &net, PP_Res_Net<Net> &rnet, typename Net::Arc *a)
{
  using Rnet = PP_Res_Net<Net>;
  auto s = net.get_src_node(a);
  auto t = net.get_tgt_node(a);

  assert(NODE_COOKIE(s) and NODE_COOKIE(t));

  auto src = static_cast<typename Rnet::Node *>(NODE_COOKIE(s));
  auto tgt = static_cast<typename Rnet::Node *>(NODE_COOKIE(t));

  auto arc = rnet.insert_arc(src, tgt);
  auto dup = rnet.insert_arc(tgt, src);

  arc->img = a;  // mark it as not residual
  arc->cap = a->cap;
  arc->flow = a->flow;
  arc->dup = dup;

  dup->cap = arc->cap;
  dup->flow = arc->cap - arc->flow;
  dup->dup = arc;
}

/// Residual arc filter: keep arcs with residual capacity.
template <class Rnet>
struct Res_F
{
  Res_F(typename Rnet::Node *) noexcept {}
  Res_F() noexcept {}

  bool operator () (typename Rnet::Arc *a) const
  {
    return a->cap > a->flow;
  }
};

/** @brief Build the residual network for preflow-push algorithms.
 *
 * Returns the residual network and the mapped source/sink nodes.
 */
template <class Net>
inline std::tuple<PP_Res_Net<Net>, typename PP_Res_Net<Net>::Node *, typename PP_Res_Net<Net>::Node *>
preflow_create_residual_net(Net &net)
{
  using Rnet = PP_Res_Net<Net>;
  net.reset_nodes();
  Rnet rnet;

  for (typename Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
    {
      auto p = it.get_curr();
      auto q = rnet.insert_node();
      q->in_flow = net.get_in_flow(p);
      q->out_flow = net.get_out_flow(p);
      // Directly store pointers in cookies using reinterpret_cast to
      // preserve exact addresses without multiple inheritance adjustment
      NODE_COOKIE(p) = reinterpret_cast<void *>(q);
      NODE_COOKIE(q) = reinterpret_cast<void *>(p);
    }

  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
    create_residual_arc(net, rnet, it.get_curr());

  return std::make_tuple(std::move(rnet),
                         static_cast<typename Rnet::Node *>(NODE_COOKIE(net.get_source())),
                         static_cast<typename Rnet::Node *>(NODE_COOKIE(net.get_sink())));
}

/// Propagate residual arc flows back to their original arcs.
template <class Rnet>
inline void update_flow(const Rnet &rnet)
{
  for (typename Rnet::Arc_Iterator it(rnet); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      auto img = arc->img;
      if (img == nullptr)
        continue;
      img->flow = arc->flow;
    }
}

/** @brief Maximize flow using repeated augmenting-path searches.
 *
 * With a floating type of flow, the residues of rounding that the paths
 * leave go back at the end, as those of the preflows (see
 * net_detail::return_dust()): every node keeps its flow within the
 * tolerance of check_node(). What a node keeps within it stays, as the
 * rounding of the sums of its flows: returning it lowered by an ulp arcs
 * that the paths had filled, and min_cut() put the sink with the source.
 *
 * @warning With a floating flow, the paths of a depth-first search, as
 * those of Find_Aumenting_Path_DFS, take the residues of rounding that
 * capacities not exact in binary, as tenths, leave on the arcs, of the
 * order of an ulp, and spread them, so that the slacks of the paths go
 * down to 1e-17: they may not end, as on a network of 400 nodes in
 * tenths. ford_fulkerson_maximum_flow() takes the shortest paths then,
 * those of Find_Aumenting_Path_BFS, which end after O(V E) of them.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @return Maximized flow value.
 *
 * @ingroup Networks
 */
template <class Net, template <class> class Find_Path>
typename Net::Flow_Type aumenting_path_maximum_flow(Net &net)
{
  ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
    << "Network is not single source and single sink";

  while (true)  // while an augmenting path exists
    {
      SemiPath<Net> semi_path = Find_Path<Net>(net)();
      if (not get<0>(semi_path))
        break;

      // Skip paths with zero slack.
      // This can happen with parallel arcs where one arc is saturated
      // but the path finder still finds a path through it.
      auto slack = get<1>(semi_path);
      if (slack <= typename Net::Flow_Type{0})
        break;

      increase_flow<Net>(net, get<2>(semi_path), slack);
    }

  // A slack that rounding left on an arc of larger flows, next to one of
  // smaller capacity, may go along a path as a residue, whose nodes then
  // lose their balance when other paths take their flows to zero
  net_detail::return_dust(net, net.get_source(), net.get_sink());
  assert(net.get_source() == net.get_sink() or net.check_network());

  return net.get_out_flow(net.get_source());
}

/** @brief Maximize flow using the Ford-Fulkerson algorithm.
 *
 * With an integer flow, it takes the augmenting paths of a depth-first
 * search. With a floating flow, it takes the shortest ones, those of
 * edmonds_karp_maximum_flow(): rounding leaves residues of the order of
 * an ulp on the arcs, which the paths of a depth-first search take and
 * spread, taking the flows off the multiples of the data, so that the
 * augmentations, bounded only by the value of the flow over the least of
 * its units, did not end on networks of some hundreds of nodes with
 * capacities in tenths. The number of shortest augmentations is O(V E),
 * whatever the capacities.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @return Maximized flow value.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type ford_fulkerson_maximum_flow(Net &net)
{
  if constexpr (std::is_floating_point_v<typename Net::Flow_Type>)
    return aumenting_path_maximum_flow<Net, Find_Aumenting_Path_BFS>(net);
  else
    return aumenting_path_maximum_flow<Net, Find_Aumenting_Path_DFS>(net);
}

/** @brief Functor wrapper for ford_fulkerson_maximum_flow().
 *
 * @ingroup Networks
 */
template <class Net>
struct Ford_Fulkerson_Maximum_Flow
{
  /// Invoke ford_fulkerson_maximum_flow().
  typename Net::Flow_Type operator () (Net &net) const
  {
    return ford_fulkerson_maximum_flow(net);
  }
};

/** @brief Maximize flow using the Edmonds-Karp algorithm.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @return Maximized flow value.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type edmonds_karp_maximum_flow(Net &net)
{
  return aumenting_path_maximum_flow<Net, Find_Aumenting_Path_BFS>(net);
}

/** @brief Functor wrapper for edmonds_karp_maximum_flow().
 *
 * @ingroup Networks
 */
template <class Net>
struct Edmonds_Karp_Maximum_Flow
{
  /// Invoke edmonds_karp_maximum_flow().
  typename Net::Flow_Type operator () (Net &net) const
  {
    return edmonds_karp_maximum_flow<Net>(net);
  }
};

/// Return true if `p` has excess flow in `net`.
template <class Net>
static inline bool is_node_active(const Net &net, typename Net::Node *p)
{
  return net.get_in_flow(p) != net.get_out_flow(p);
}

/// Return true if residual node `p` has excess flow.
template <class Rnet>
static inline bool is_node_active(typename Rnet::Node *p)
{
  return p->in_flow != p->out_flow;
}

/// Access the height label stored in NODE_COUNTER.
template <class Net>
static inline long &node_height(typename Net::Node *p)
{
  return NODE_COUNTER(p);
}

/// Initialize node heights with BFS distance to the sink.
template <class Net>
static inline void init_height_in_nodes(Net &net)
{
  Graph_Traverse_BFS<Net, Node_Arc_Iterator<Net>>(net).exec(
    net.get_sink(), [&net](typename Net::Node *p, typename Net::Arc *a)
    {
      if (a)
        node_height<Net>(p) = node_height<Net>(net.get_tgt_node(a)) + 1;
      return true;
    });
}

/// Enqueue an active node if it is not already enqueued.
template <class Q_Type>
static inline void put_in_active_queue(Q_Type &q, typename Q_Type::Item_Type &p)
{
  if (NODE_BITS(p).get_bit(Aleph::Maximum_Flow))
    return;  // the node is already into the queue
  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, true);
  q.put(p);
}

/// Dequeue an active node and clear its queue mark.
template <class Q_Type>
static inline typename Q_Type::Item_Type get_from_active_queue(Q_Type &q)
{
  auto p = q.get();
  assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));
  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);
  return p;
}

/// Remove a node from the active queue and clear its queue mark.
template <class Q_Type>
static inline void remove_from_active_queue(Q_Type &q, typename Q_Type::Item_Type &p)
{
  assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));
  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);
  q.remove(p);
}

namespace net_detail {
// For a floating flow, a path from the source to the sink of arcs of
// infinite capacity makes the maximum flow infinite. If there is one, it
// takes an infinite flow, and the function returns true: the preflows
// end then, as their initial preflow would fill an arc of infinite
// capacity out of the source with infinity, which a node that cannot
// send it on cannot send back either, inf - inf being NaN. Only arcs of
// infinite capacity are searched, and only if one leaves the source:
// otherwise, the cost is that of a pass over the arcs of the source
template <class Net>
bool fill_infinite_path(Net &net)
{
  using Flow_Type = typename Net::Flow_Type;
  if constexpr (std::numeric_limits<Flow_Type>::has_infinity)
    {
      constexpr Flow_Type inf = std::numeric_limits<Flow_Type>::infinity();
      auto source = net.get_source();
      bool leaves = false;
      for (typename Net::Node_Arc_Iterator it(source); it.has_curr() and not leaves; it.next_ne())
        if (auto a = it.get_curr(); net.get_src_node(a) == source and a->cap == inf)
          leaves = true;
      if (not leaves)
        return false;

      SemiPath<Net> semi_path = Find_Aumenting_Path_BFS<Net>(net)(inf);
      if (not get<0>(semi_path))
        return false;
      increase_flow<Net>(net, get<2>(semi_path), inf);
      return true;
    }
  else
    {
      (void) net;
      return false;
    }
}

// The flow that the initial preflow gives an arc of infinite capacity
// out of the source, when no path of such arcs goes to the sink: a bound
// of the maximum flow, the capacity of a cut: that of the arcs into the
// sink, if all of them are finite, or else the sum of all the finite
// capacities, as the nodes that arcs of infinite capacity reach from the
// source make a cut without them. max() if the sum goes beyond it
template <class Net>
typename Net::Flow_Type infinite_arc_preflow(const Net &net)
{
  using Flow_Type = typename Net::Flow_Type;
  constexpr Flow_Type inf = std::numeric_limits<Flow_Type>::infinity();
  auto sink = net.get_sink();
  Flow_Type bound{0};
  bool finite = true;
  for (typename Net::Node_Arc_Iterator it(sink); it.has_curr() and finite; it.next_ne())
    if (auto a = it.get_curr(); net.get_tgt_node(a) == sink)
      {
        if (a->cap == inf)
          finite = false;
        else
          bound += a->cap;
      }
  if (not finite)
    {
      bound = Flow_Type{0};
      for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
        if (auto a = it.get_curr(); a->cap != inf)
          bound += a->cap;
    }
  return std::min(bound, std::numeric_limits<Flow_Type>::max());
}
}  // namespace net_detail

/** @brief Generic preflow-push maximum flow (vertex-oriented).
 *
 * Q_Type is the active-node container and must support:
 * - get(): remove a node
 * - put(p): insert a node
 *
 * The Maximum_Flow bit is used to track whether a node is in the queue.
 *
 * With a floating flow, the flows of the node that a discharge pushes
 * from are summed keeping what their rounding loses, so that a small
 * flow beside a large one is not lost; an excess of a node up to a
 * residue of rounding of its own flows, an epsilon of the larger of its
 * flows in and out by each of its arcs and two more, is not pushed; and
 * at the end what the nodes keep or lack of it goes back toward the
 * source, or forward toward the sink, so that every node keeps its flow
 * within the tolerance of check_node(), as check_network() takes it,
 * and a network whose maximum flow is 0 gives 0.
 *
 * An arc of infinite capacity out of the source starts with a bound of
 * the maximum flow, the capacity of the arcs into the sink, if they are
 * finite, or else the sum of the finite capacities: filled with
 * infinity, a node that could not send it on could not send it back
 * either, as inf - inf is NaN, and the flow came out infinite on networks
 * whose maximum flow is finite. A path of arcs of infinite capacity from
 * the source to the sink makes the maximum flow infinite: its arcs take
 * an infinite flow, and the function returns it, as the augmenting paths
 * do.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @param[in] q the queue of the active nodes, empty, which the push takes;
 *            by default Q_Type(), and with a seed for the random order of
 *            random_preflow_maximum_flow(net, seed).
 * @return Maximized flow value.
 *
 * @see fifo_preflow_maximum_flow() heap_preflow_maximum_flow() random_preflow_maximum_flow()
 * @ingroup Networks
 */
template <class Net, class Q_Type>
typename Net::Flow_Type generic_preflow_vertex_push_maximum_flow(Net &net, Q_Type q = Q_Type())
{
  ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
    << "Network is not single source and single sink";

  // A path of arcs of infinite capacity makes the maximum flow infinite
  if (net_detail::fill_infinite_path(net))
    return net.flow_value();

  net.reset_nodes();          // especially assures that counters are in zero
  init_height_in_nodes(net);  // set height to minimum distance in nodes to sink

  auto source = net.get_source();
  auto sink = net.get_sink();
  node_height<Net>(source) = net.vsize();  // except the source
  constexpr auto Max = std::numeric_limits<long>::max();

  using Itor = __Net_Iterator<Net>;
  using Flow_Type = typename Net::Flow_Type;
  // An arc of infinite capacity out of the source takes a bound of the
  // maximum flow, computed once if one needs it
  [[maybe_unused]] Flow_Type bound{0};
  [[maybe_unused]] bool bounded = false;
  for (Itor it(source); it.has_curr(); it.next_ne())  // initial preflow
    {
      auto arc = it.get_curr();
      arc->flow = arc->cap;                               // saturate arc
      if constexpr (std::numeric_limits<Flow_Type>::has_infinity)
        if (arc->cap == std::numeric_limits<Flow_Type>::infinity())
          {
            if (not bounded)
              {
                bound = net_detail::infinite_arc_preflow(net);
                bounded = true;
              }
            arc->flow = bound;
          }
      if (auto tgt = net.get_tgt_node(arc); tgt != sink)  // never active
        put_in_active_queue(q, tgt);
    }

  while (not q.is_empty())  // while there are active nodes
    {
      auto src = get_from_active_queue(q);
      // With a floating flow, the flows of src, which keep what their
      // rounding loses; an excess up to residue is a residue of rounding
      // of them, and is not pushed
      [[maybe_unused]] net_detail::Flow_Sum<Flow_Type> in, out;
      Flow_Type excess, residue;
      if constexpr (std::is_floating_point_v<Flow_Type>)
        {
          net_detail::node_flows(net, src, in, out);
          excess = in.minus(out);
          residue = net_detail::node_residue(in.value(), out.value(), net.get_num_arcs(src));
        }
      else
        {
          excess = net.get_in_flow(src) - net.get_out_flow(src);
          residue = Flow_Type{0};
        }
      long m = Max;
      bool was_eligible_arc = false;
      for (Itor it(src); it.has_curr() and excess > residue; it.next_ne())
        {
          auto arc = it.get_curr();
          auto tgt = net.get_connected_node(arc, src);
          if (node_height<Net>(src) != node_height<Net>(tgt) + 1)
            {
              m = std::min(m, node_height<Net>(tgt));
              continue;
            }

          was_eligible_arc = true;
          if constexpr (std::is_floating_point_v<Flow_Type>)
            {
              // The excess follows the flows that src keeps, which the
              // arc changed: a small push beside a large excess would be
              // lost in it, and src would then push back too much
              net_detail::push_excess(arc, not is_residual<Net>(src, arc), in, out);
              excess = in.minus(out);
            }
          else
            {
              Flow_Type flow_to_push;
              if (is_residual<Net>(src, arc))
                {
                  flow_to_push = std::min(arc->flow, excess);
                  arc->flow -= flow_to_push;
                }
              else
                {
                  flow_to_push = std::min(arc->cap - arc->flow, excess);
                  arc->flow += flow_to_push;
                }
              excess -= flow_to_push;
            }
          if (tgt != source and tgt != sink)
            {
              assert(net_detail::not_short(net, tgt));
              put_in_active_queue(q, tgt);
            }
        }

      if (excess > residue)  // is src still active
        {
          if (not was_eligible_arc)
            node_height<Net>(src) = m + 1;
          put_in_active_queue(q, src);
        }
    }

  // What a floating flow keeps as residues of rounding goes back
  net_detail::return_dust(net, source, sink);

  // A network of one node, its source and sink, has no arcs, which
  // check_network() does not take
  assert(source == sink or net.check_network());

  return net.flow_value();
}

/// Initialize node heights in a residual network from `sink`.
template <class Rnet>
inline void init_height_in_nodes(Rnet &rnet, typename Rnet::Node *sink)
{
  Graph_Traverse_BFS<Rnet, Node_Arc_Iterator<Rnet>>(rnet).exec(
    sink, [&rnet](typename Rnet::Node *p, typename Rnet::Arc *a)
    {
      if (a)
        node_height<Rnet>(p) = node_height<Rnet>(rnet.get_src_node(a)) + 1;
      return true;
    });
}

/** @brief Preflow-push maximum flow using a FIFO queue of active nodes.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @return Maximized flow value.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type fifo_preflow_maximum_flow(Net &net)
{
  using Node = typename Net::Node;
  return generic_preflow_vertex_push_maximum_flow<Net, DynListQueue<Node *>>(net);
}

/** @brief Functor wrapper for fifo_preflow_maximum_flow().
 *
 * @ingroup Networks
 */
template <class Net>
struct Fifo_Preflow_Maximum_Flow
{
  /// Invoke fifo_preflow_maximum_flow().
  typename Net::Flow_Type operator () (Net &net) const
  {
    return fifo_preflow_maximum_flow(net);
  }
};

/// Compare nodes by height (higher first).
template <class Net>
struct Compare_Height
{
  bool operator () (typename Net::Node *n1, typename Net::Node *n2) const
  {
    return node_height<Net>(n1) > node_height<Net>(n2);
  }
};

/** @brief Preflow-push maximum flow using a height-ordered heap.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @return Maximized flow value.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type heap_preflow_maximum_flow(Net &net)
{
  using Node = typename Net::Node;
  return generic_preflow_vertex_push_maximum_flow<Net, DynBinHeap<Node *, Compare_Height<Net>>>(net);
}

/** @brief Functor wrapper for heap_preflow_maximum_flow().
 *
 * @ingroup Networks
 */
template <class Net>
struct Heap_Preflow_Maximum_Flow
{
  /// Invoke heap_preflow_maximum_flow().
  typename Net::Flow_Type operator () (Net &net) const
  {
    return heap_preflow_maximum_flow(net);
  }
};

/** @brief Preflow-push maximum flow using a random active-node queue.
 *
 * The order of the active nodes takes its seed from the time, so that two
 * runs may push the flow along other arcs; the overload with a seed
 * repeats them.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @return Maximized flow value.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type random_preflow_maximum_flow(Net &net)
{
  using Node = typename Net::Node;
  return generic_preflow_vertex_push_maximum_flow<Net, Random_Set<Node *>>(net);
}

/** @brief Preflow-push maximum flow using a random active-node queue
 * whose order takes the seed `seed`.
 *
 * Two runs on equal networks with the same seed push the same flows.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @param[in] seed the seed of the order of the active nodes.
 * @return Maximized flow value.
 *
 * @ingroup Networks
 */
template <class Net>
typename Net::Flow_Type random_preflow_maximum_flow(Net &net, const unsigned long seed)
{
  using Node = typename Net::Node;
  Random_Set<Node *> q;
  q.set_seed(seed);
  return generic_preflow_vertex_push_maximum_flow<Net, Random_Set<Node *>>(net, std::move(q));
}

/** @brief Functor wrapper for random_preflow_maximum_flow().
 *
 * @ingroup Networks
 */
template <class Net>
struct Random_Preflow_Maximum_Flow
{
  /// Invoke random_preflow_maximum_flow().
  typename Net::Flow_Type operator () (Net &net) const
  {
    return random_preflow_maximum_flow(net);
  }
};

/** @brief Compute max flow and the corresponding minimum cut.
 *
 * @param[in,out] net Flow network (must have a single source and sink).
 * @param[out] vs Set of nodes in V_s.
 * @param[out] vt Set of nodes in V_t.
 * @param[out] cuts Arcs from V_s to V_t (the cut).
 * @param[out] cutt Arcs from V_t to V_s.
 * @return Max flow value (equal to min-cut capacity).
 *
 * @ingroup Networks
 */
template <class Net, template <class> class Maxflow>
typename Net::Flow_Type min_cut(Net &net, DynSetTree<typename Net::Node *> &vs,
                                DynSetTree<typename Net::Node *> &vt,
                                DynList<typename Net::Arc *> &cuts,
                                DynList<typename Net::Arc *> &cutt)
{
  Maxflow<Net>()(net);  // compute max flow

  typename Net::Node *source = net.get_source();

  // Traverse the residual network from source: reachable nodes go to vs.
  (Graph_Traverse_BFS<Net, Net_Iterator<Net>>(net))(source, [&vs](typename Net::Node *p)
    {
      vs.insert(p);
      return true;
    });

  // Compute vt by complement.
  const size_t size_vt = net.get_num_nodes() - vs.size();
  for (Node_Iterator<Net> it(net); it.has_curr() and vt.size() < size_vt; it.next_ne())
    if (auto p = it.get_curr(); p->state() == Unprocessed)
      vt.insert(p);

  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      if (arc->flow == 0)                           // residual candidate
        if (vt.contains(net.get_src_node(arc)) and  // vt -> vs
            vs.contains(net.get_tgt_node(arc)))
          {
            cutt.insert(arc);
            continue;
          }

      if (arc->flow == arc->cap)                    // saturated candidate
        if (vs.contains(net.get_src_node(arc)) and  // vs -> vt
            vt.contains(net.get_tgt_node(arc)))
          cuts.insert(arc);
    }

  return net.flow_value();
}

/** @brief Functor wrapper for min_cut().
 *
 * @ingroup Networks
 */
template <class Net, template <class> class Maxflow = Heap_Preflow_Maximum_Flow>
struct Min_Cut
{
  /// Invoke min_cut().
  typename Net::Flow_Type operator () (Net &net, DynSetTree<typename Net::Node *> &vs,
                                       DynSetTree<typename Net::Node *> &vt,
                                       DynList<typename Net::Arc *> &cuts,
                                       DynList<typename Net::Arc *> &cutt)
  {
    return min_cut<Net, Maxflow>(net, vs, vt, cuts, cutt);
  }
};
}  // end namespace Aleph

#endif  // TPL_NET_H
