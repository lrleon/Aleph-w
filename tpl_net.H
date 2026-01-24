
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

# ifndef TPL_NET_H
# define TPL_NET_H

# include <limits>
# include <set>
# include <tuple>
# include <type_traits>
# include <utility>

# include <tpl_dynDlist.H>
# include <tpl_dynListStack.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynSetTree.H>
# include <tpl_dynSetHash.H>
# include <tpl_random_queue.H>
# include <tpl_graph_utils.H>
# include <tpl_find_path.H>
# include <graph-traverse.H>
# include <ah-errors.H>


namespace Aleph
{
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
    Net_Arc_Info(const Arc_Info & info, Flow_Type __cap, Flow_Type __flow)
      : Arc_Info(info), cap(__cap), flow(__flow)
    {
      // empty
    }
  };

  /** @brief Arc of a flow network implemented with adjacency lists.
   *
   * Net_Arc models a capacitated network arc with Arc_Info and a numeric
   * Flow_Type for capacity and flow.
   *
   * This class is only intended as the arc type for Net_Graph; do not
   * instantiate it directly. Use Net_Graph::insert_arc() instead.
   *
   * @see Array_Graph Net_Graph Net_Node
   * @ingroup Networks
   */
  template <typename Arc_Info, typename F_Type = double>
  struct Net_Arc : public Graph_Aarc<Arc_Info> // Arc type for flow networks.
  {
    using Base = Graph_Aarc<Arc_Info>;

    using Flow_Type = F_Type;

    /// Capacity value.
    Flow_Type cap = 0;

    /// Flow value.
    Flow_Type flow = 0;

    /// Return true if flow satisfies 0 <= flow <= cap.
    bool check_arc() const noexcept { return flow >= 0 and flow <= cap; }

    /// Construct from arc info.
    Net_Arc(const Arc_Info & info)
      : Base(info)
    { /* empty */
    }

    /// Copy constructor.
    Net_Arc(const Net_Arc & arc)
      : Base(arc.arc_info), cap(arc.cap), flow(arc.flow)
    {
      // empty
    }

    /// Default constructor.
    Net_Arc()
    { /* empty */
    }

    /// Copy assignment.
    Net_Arc &operator =(const Net_Arc & arc)
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
    void set_cookie(void *__cookie) noexcept { cookie = __cookie; }

    /// Build a filter rooted at node `s`.
    Net_Filt(typename Net::Node *s = nullptr) noexcept
      : p(s)
    { /* empty */
    }

    /// Return true if arc `a` has residual capacity with respect to `p`.
    bool operator ()(typename Net::Arc *a) const noexcept
    {
      assert(p);
      assert(a->src_node == p or a->tgt_node == p);
      auto src = static_cast<typename Net::Node *>(a->src_node);
      if (src == p)
        return a->cap - a->flow > 0; // normal arc

      assert(is_residual<Net>(p, a));
      return a->flow > 0; // residual arc
    }

    /// Filter hook used by some iterator adapters.
    bool operator ()(const Net &, typename Net::Arc *arc) noexcept
    {
      return (*this)(arc);
    }

    /// Return the opposite endpoint of arc `a` with respect to `p`.
    typename Net::Node * get_node(typename Net::Arc *a) const noexcept
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
  using Net_Iterator =
  Filter_Iterator<typename Net::Node *, __Net_Iterator<Net>, Show_Arc>;


  /** @brief Return the remaining flow of `a` as seen from `src`.
   *
   * If `a` is residual with respect to `src`, the remaining flow is the
   * current flow. Otherwise, it is the residual capacity.
   *
   * @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type
  remaining_flow(typename Net::Node *src, typename Net::Arc *a) noexcept
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
  template <class NodeT = Net_Node<Empty_Class>,
            class ArcT = Net_Arc<Empty_Class, double>>
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
      return out_pairs<Net_Graph>(p).template maps<Node *>
          ([](const ArcPair<Net_Graph> & p) { return get<1>(p); });
    }

    /// Return arcs incoming to `p` (as a DynList).
    DynList<Arc *> in_arcs(Node *p) const noexcept
    {
      return Aleph::in_arcs<Net_Graph>(p);
    }

    /// Return nodes that can reach `p` through incoming arcs.
    DynList<Node *> in_nodes(Node *p) const noexcept
    {
      return in_pairs<Net_Graph>(p).template maps<Node *>
          ([](const ArcPair<Net_Graph> & p) { return get<1>(p); });
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
    bool is_source(Node *node) const noexcept { return src_nodes.contains(node); }

    /// Return true if `node` is a sink.
    bool is_sink(Node *node) const noexcept { return sink_nodes.contains(node); }

    /// Return true if the network has a single source.
    [[nodiscard]] constexpr bool is_single_source() const noexcept { return src_nodes.size() == 1; }

    /// Return true if the network has a single sink.
    [[nodiscard]] constexpr bool is_single_sink() const noexcept { return sink_nodes.size() == 1; }

    /// Return true if `p` is connected (used for validation).
    bool is_connected(Node *p) const noexcept
    {
      return get_in_degree(p) != 0 or get_out_degree(p) != 0;
    }

    /// Return true if `node` satisfies flow conservation constraints.
    bool check_node(Node *node) const noexcept
    {
      if (not is_connected(node))
        return false;

      auto out_flow = get_out_flow(node);
      auto in_flow = get_in_flow(node);

      // Use tolerance for floating-point comparison
      const auto eps = std::max(std::abs(out_flow), std::abs(in_flow)) * 1e-9;
      const auto nearly_zero = [eps](auto x) { return std::abs(x) <= eps; };
      const auto nearly_equal = [eps](auto a, auto b) { return std::abs(a - b) <= eps; };

      if (is_sink(node))
        return nearly_zero(out_flow) and in_flow >= -eps;

      if (is_source(node))
        return nearly_zero(in_flow) and out_flow >= -eps;

      return nearly_equal(out_flow, in_flow);
    }

  private:
    DynSetTree<Node *> src_nodes;
    DynSetTree<Node *> sink_nodes;

    /// Return total outgoing flow from all sources.
    Flow_Type total_source_out_flow() const
    {
      Flow_Type sum = Flow_Type{};
      for (typename DynSetTree<Node *>::Iterator it(src_nodes);
           it.has_curr(); it.next_ne())
        sum += get_out_flow(it.get_curr());
      return sum;
    }

    /// Return total incoming flow to all sinks.
    Flow_Type total_sink_in_flow() const
    {
      Flow_Type sum = Flow_Type{};
      for (typename DynSetTree<Node *>::Iterator it(sink_nodes);
           it.has_curr(); it.next_ne())
        sum += get_in_flow(it.get_curr());
      return sum;
    }

    /// Insert `p` into source/sink sets with rollback on failure.
    Node * register_node(Node *p)
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
    const DynSetTree<Node *> &get_src_nodes() const noexcept { return src_nodes; }

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

      ah_domain_error_if(src_nodes.size() == 0)
        << "network has no source nodes (it has cycles)";

      DynList<Node *> sources;
      for (typename DynSetTree<Node *>::Iterator it(src_nodes);
           it.has_curr(); it.next_ne())
        sources.append(it.get_curr());

      Node *super_source = insert_node();
      for (typename DynList<Node *>::Iterator it(sources);
           it.has_curr(); it.next_ne())
        insert_arc(super_source, it.get_curr(), get_out_cap(it.get_curr()));

      with_super_source = true;
    }

    /// Restore a super-source network to its original multi-source form.
    void unmake_super_source() noexcept
    {
      if (not with_super_source)
        return;

      assert(src_nodes.size() == 1);

      remove_node(src_nodes.get_item());
      with_super_source = false;
    }

    /// Convert a multi-sink network into a single super-sink network.
    /// Throws if there is no sink node or on allocation failure.
    void make_super_sink()
    {
      if (sink_nodes.size() == 1)
        return;

      ah_domain_error_if(sink_nodes.size() == 0)
        << "network has no sink nodes (it has cycles)";

      DynList<Node *> sinks;
      for (typename DynSetTree<Node *>::Iterator it(sink_nodes);
           it.has_curr(); it.next_ne())
        sinks.append(it.get_curr());

      Node *super_sink = insert_node();
      for (typename DynList<Node *>::Iterator it(sinks);
           it.has_curr(); it.next_ne())
        insert_arc(it.get_curr(), super_sink, get_in_cap(it.get_curr()));
      with_super_sink = true;
    }

    /// Restore a super-sink network to its original multi-sink form.
    void unmake_super_sink() noexcept
    {
      if (not with_super_sink)
        return;

      assert(sink_nodes.size() == 1);

      remove_node(sink_nodes.get_item());
      with_super_sink = false;
    }

    /// Convert a multi-source/multi-sink network into super-source/super-sink.
    void make_super_nodes()
    {
      make_super_source();
      try
        {
          make_super_sink();
        }
      catch (const std::bad_alloc &)
        {
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
    Node * get_source() const { return src_nodes.get_item(); }

    /// Return an arbitrary sink node.
    Node * get_sink() const { return sink_nodes.get_item(); }

    /** @brief Insert a new node by copying `node_info`.
     *
     * @param[in] node_info Info to copy into the node.
     * @return Pointer to the inserted node.
     * @throw bad_alloc If there is not enough memory.
     */
    Node * insert_node(const Node_Type & node_info)
    {
      return register_node(Graph::insert_node(node_info));
    }

    /// Insert a new node with default info.
    Node * insert_node()
    {
      return register_node(Graph::insert_node(Node_Type()));
    }

    /// Insert a new node by moving `info`.
    Node * insert_node(Node_Type && info)
    {
      return register_node(Graph::insert_node(std::move(info)));
    }

    /// Construct a node in-place and insert it into the network.
    template <typename... Args>
    Node * emplace_node(Args &&... args)
    {
      return insert_node(Node_Type(args...));
    }


    /** @brief Insert a node by copying another node.
     *
     * @param[in] p Node to copy.
     * @return Pointer to the inserted node.
     * @throw bad_alloc If there is not enough memory.
     */
    Node * insert_node(Node *p)
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
     * @throw overflow_error If `flow` exceeds `cap`.
     */
    Arc * insert_arc(Node *src_node, Node *tgt_node,
                     const Flow_Type & cap, const Flow_Type & flow,
                     const typename Arc::Arc_Type & arc_info = Arc_Type())
    { // base insertion
      auto arc = Graph::insert_arc(src_node, tgt_node, arc_info);

      src_nodes.remove(tgt_node); // update sources/sinks
      sink_nodes.remove(src_node);

      arc->cap = cap;
      arc->flow = flow;

      ah_overflow_error_if(not arc->check_arc()) << "flow is greater than capacity";

      return arc;
    }

    template <typename... Args>
    /// Construct arc info in-place and insert the arc.
    Arc * emplace_arc(Node *src_node, Node *tgt_node,
                      const Flow_Type & cap, const Flow_Type & flow,
                      Args &&... args)
    {
      return insert_arc(src_node, tgt_node, cap, flow, Arc_Type(args...));
    }

    /** @brief Connect a previously disconnected arc.
     *
     * The arc must already belong to the graph; no verification is performed.
     * Multigraphs are supported (no check for existing arcs).
     *
     * @param[in] arc Arc to connect.
     * @return Pointer to the connected arc.
     */
    Arc * connect_arc(Arc *arc)
    {
      Graph::connect_arc(arc);

      auto src = this->get_src_node(arc);
      auto tgt = this->get_tgt_node(arc);

      src_nodes.remove(tgt); // target is no longer a source
      sink_nodes.remove(src); // source is no longer a sink

      return arc;
    }

    /** @brief Insert an arc with capacity `cap` and zero flow. */
    Arc * insert_arc(Node *src_node, Node *tgt_node, const Flow_Type & cap)
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
    template <typename T = Arc_Type,
              typename = std::enable_if_t<!std::is_same<T, Flow_Type>::value &&
                                          !std::is_arithmetic_v<T>>>
    Arc * insert_arc(Node *src_node, Node *tgt_node,
                     const T & arc_info = Arc_Type())
    {
      return insert_arc(src_node, tgt_node, 0, 0, arc_info);
    }

    /// Remove arc `arc` from the network.
    void remove_arc(Arc *arc) override
    {
      auto src = this->get_src_node(arc);
      auto tgt = this->get_tgt_node(arc);
      if (get_in_degree(tgt) == 1)
        src_nodes.insert(tgt); // target becomes a source

      Graph::remove_arc(arc); // base removal

      if (get_out_degree(src) == 0)
        sink_nodes.insert(src); // source becomes a sink
    }

    /** @brief Disconnect arc `arc` from the graph without deleting it. */
    void disconnect_arc(Arc *arc) noexcept
    {
      auto src = this->get_src_node(arc);
      auto tgt = this->get_tgt_node(arc);
      if (get_in_degree(tgt) == 1)
        src_nodes.insert(tgt); // target becomes a source

      Graph::disconnect_arc(arc); // base disconnection

      if (get_out_degree(src) == 0)
        sink_nodes.insert(src); // source becomes a sink
    }

    /// Remove node `p` and all its arcs from the network.
    void remove_node(Node *p) noexcept override
    {
      Graph::remove_node(p); // base removal
      src_nodes.remove(p);
      sink_nodes.remove(p);
    }

    /// Copy-construct a network. Throws bad_alloc on allocation failure.
    Net_Graph(const Net_Graph & net)
      : Array_Graph<NodeT, ArcT>::Array_Graph(),
        Infinity(std::numeric_limits<typename Arc::Flow_Type>::max()),
        with_super_source(net.with_super_source),
        with_super_sink(net.with_super_sink)
    {
      copy_graph(*this, net, false); // copy without mapping

      using Pair = std::pair<typename Net_Graph::Arc *, typename Net_Graph::Arc *>;
      zip(this->arcs(), net.arcs()).for_each([](const Pair & p)
                                               {
                                                 auto atgt = p.first;
                                                 auto asrc = p.second;
                                                 atgt->cap = asrc->cap;
                                                 atgt->flow = asrc->flow;
                                               });
    }

    /// Swap contents with another network. O(1) operation.
    void swap(Net_Graph & other) noexcept
    {
      Graph::common_swap(other);  // swap num_nodes, num_arcs, digraph, cookie
      Graph::get_node_dlink().swap(other.get_node_dlink());
      Graph::get_arc_dlink().swap(other.get_arc_dlink());
      src_nodes.swap(other.src_nodes);
      sink_nodes.swap(other.sink_nodes);
      std::swap(Infinity, other.Infinity);
      std::swap(with_super_source, other.with_super_source);
      std::swap(with_super_sink, other.with_super_sink);
    }

    /// Move-construct a network. O(1) operation.
    Net_Graph(Net_Graph && other) noexcept
      : Infinity(std::numeric_limits<typename Arc::Flow_Type>::max()),
        with_super_source(false), with_super_sink(false)
    {
      swap(other);
    }

    /// Move-assign a network. O(1) operation.
    Net_Graph & operator=(Net_Graph && other) noexcept
    {
      if (this != &other)
        swap(other);
      return *this;
    }

    /// Copy-assign a network. Throws bad_alloc on allocation failure.
    Net_Graph & operator=(const Net_Graph & net)
    {
      if (this == &net)
        return *this;
      
      Net_Graph tmp(net);  // copy construct
      swap(tmp);           // swap with copy
      return *this;
    }

    /// Set the capacity of an arc.
    void set_cap(Arc *arc, const Flow_Type & cap)
    {
      ah_out_of_range_error_if(cap < arc->flow) << "capacity value is smaller than flow";

      arc->cap = cap;
    }

    /// Set the flow of an arc. Throws if `flow` exceeds capacity.
    void set_flow(Arc *arc, const Flow_Type & flow)
    {
      ah_out_of_range_error_if(flow > arc->cap) << "flow value is greater than capacity";

      arc->flow = flow;
    }

    /// Return the flow value of an arc.
    const Flow_Type &get_flow(Arc *arc) const noexcept { return arc->flow; }

    /// Return the capacity value of an arc.
    const Flow_Type &get_cap(Arc *arc) const noexcept { return arc->cap; }

    /// Reset all arc flows to zero.
    void reset()
    {
      for (Arc_Iterator<Net_Graph> it(*this); it.has_curr(); it.next_ne())
        it.get_curr()->flow = 0;
    }

    /** @brief Validate flow-conservation and capacity constraints.
     *
     * @return true if the network is valid; false otherwise.
     */
    bool check_network() const
    {
      if (src_nodes.is_empty() or sink_nodes.is_empty())
        return false;

      const auto total_out = total_source_out_flow();
      const auto total_in = total_sink_in_flow();

      // Use tolerance for floating-point comparison
      const auto eps = std::max(std::abs(total_out), std::abs(total_in)) * 1e-9;
      const bool flow_balanced = std::abs(total_out - total_in) <= eps;

      return this->nodes().all([this](Node *p) { return check_node(p); }) and
             this->arcs().all([](Arc *a) { return a->check_arc(); }) and
             flow_balanced;
    }

    /** @brief Return the total flow value of the network. */
	    Flow_Type flow_value() const
	    {
	      ah_domain_error_if(src_nodes.is_empty() or sink_nodes.is_empty())
	        << "network has no source or sink nodes";

	      const auto total_out = total_source_out_flow();
	      const auto total_in = total_sink_in_flow();

	      (void)total_in; // May be unused when NDEBUG disables assert().
	      assert(total_out == total_in);
	      return total_out;
	    }

    /// True if the network has a super-source.
    bool with_super_source;

    /// True if the network has a super-sink.
    bool with_super_sink;

    /// Default constructor.
    Net_Graph()
      : Infinity(std::numeric_limits<typename Arc::Flow_Type>::max()),
        with_super_source(false), with_super_sink(false)
    { /* empty */
    }

    /** @brief Stream a path with arc (cap,flow) tuples. */
    friend std::ostream &operator <<(std::ostream & s, const Path<Net_Graph> & path)
    {
      if (path.is_empty())
        return s << "Path is Empty";

      const Net_Graph & net = path.get_graph();
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
  using Parc = std::tuple<typename Net::Arc *, bool>; // second field marks forward.

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
  inline void print(const DynList<Parc<Net>> & sp)
  {
    if (sp.is_empty())
      {
        std::cout << "Semi path is Empty";
        return;
      }

    for (typename DynList<Parc<Net>>::Iterator it(sp); it.has_curr();
         it.next_ne())
      {
        const Parc<Net> & pa = it.get_curr();
        auto a = get<0>(pa);
        auto s = static_cast<typename Net::Node *>(a->src_node);
        auto t = static_cast<typename Net::Node *>(a->tgt_node);
        std::cout << s->get_info() << "(" << a->flow << "," << a->cap << ")"
            << t->get_info() << " " << (get<1>(pa) ? "Normal" : "Reduced")
            << '\n';
      }
  }

  /** @brief Increase flow along an augmenting path.
   *
   * @param[in,out] net Flow network to update.
   * @param[in] path Augmenting path.
   * @return Slack value used to increase the flow.
   *
   * @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type increase_flow(Net & net, const Path<Net> & path)
  {
    typename Net::Flow_Type slack = net.Infinity; // bottleneck
    using Tuple = std::tuple<typename Net::Node *, typename Net::Arc *>;

    // Compute the bottleneck of the augmenting path.
    for (typename Path<Net>::Iterator it(path); it.has_current_arc();
         it.next_ne())
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
          arc->flow -= slack;
        else
          arc->flow += slack;

        assert(arc->check_arc());
      }

    return slack;
  }


  /** @brief Increase flow along a semi-path by a given slack.
   *
   * @param[in,out] net Flow network to update.
   * @param[in] semi_path Semi-path arc list.
   * @param[in] slack Slack to apply (not validated).
   *
   * @ingroup Networks
   */
	  template <class Net>
	  void increase_flow(Net & net,
	                     const DynList<Parc<Net>> & semi_path,
	                     const typename Net::Flow_Type slack)
	  {
	    (void)net; // May be unused when NDEBUG disables assert().

	    // Increase flow along the semi-path.
	    for (typename DynList<Parc<Net>>::Iterator it(semi_path); it.has_curr();
	         it.next_ne())
	      {
	        auto p = it.get_curr();
        auto arc = get<0>(p);
        if (get<1>(p)) // forward arc
          arc->flow += slack;
        else
          arc->flow -= slack;

        assert(arc->check_arc());
      }
    assert(net.check_network());
  }

  /** @brief Decrease flow along a semi-path by a given slack.
   *
   * This reverses the effect of increase_flow by negating forward/residual
   * arc directions.
   *
   * @param[in,out] net Flow network to update.
   * @param[in] semi_path Semi-path arc list.
   * @param[in] slack Slack to apply (not validated).
   *
   * @ingroup Networks
   */
	  template <class Net>
	  void decrease_flow(Net & net, const DynList<Parc<Net>> & semi_path,
	                     const typename Net::Flow_Type slack)
	  {
	    (void)net;

	    // Decrease flow: reverse the direction logic from increase_flow
	    for (typename DynList<Parc<Net>>::Iterator it(semi_path); it.has_curr();
	         it.next_ne())
	      {
	        auto p = it.get_curr();
        auto arc = get<0>(p);
        if (get<1>(p)) // forward arc: decrease instead of increase
          arc->flow -= slack;
        else           // residual arc: increase instead of decrease
          arc->flow += slack;

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
    const Net & net;

    // Return end node if a path is found.
    typename Net::Node * search(typename Net::Node *start,
                                typename Net::Node *end,
                                typename Net::Flow_Type min_slack)
    {
      using Itor = Net_Iterator<Net>;
      net.reset_nodes();
      net.reset_arcs();

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
          NODE_COOKIE(curr) = net.get_connected_node(arc, curr);

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
        } // end while

      return nullptr;
    }

    /// Build a Path from `start` to `end` with minimum slack.
    Path<Net> find(typename Net::Node *start, typename Net::Node *end,
                   const typename Net::Flow_Type & min_slack = typename Net::Flow_Type{0})
    {
      auto curr = search(start, end, min_slack);

      Path<Net> ret(net);
      if (not curr)
        return ret;

      assert(curr == end);

      while (curr != start)
        {
          ret.insert(curr);
          curr = static_cast<typename Net::Node *>(NODE_COOKIE(curr));
        }
      ret.insert(start);

      return ret;
    }

    /// Build a SemiPath from `start` to `end` with minimum slack.
    SemiPath<Net> find_path(typename Net::Node *start,
                            typename Net::Node *end,
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
          auto s = static_cast<typename Net::Node *>(NODE_COOKIE(t));
          // Find arc connecting s and t with positive residual capacity.
          // With parallel arcs, we must pick one that can carry flow.
          typename Net::Arc *a = nullptr;
          typename Net::Arc *fallback = nullptr;
          for (Node_Arc_Iterator<Net> it(t); it.has_curr(); it.next_ne())
            {
              auto arc = it.get_curr();
              if ((arc->src_node == s and arc->tgt_node == t) or
                  (arc->src_node == t and arc->tgt_node == s))
                {
                  if (fallback == nullptr)
                    fallback = arc;  // Keep first match as fallback
                  // Prefer arc with positive residual capacity
                  if (remaining_flow<Net>(s, arc) > 0)
                    {
                      a = arc;
                      break;
                    }
                }
            }
          if (a == nullptr)
            a = fallback;  // Use fallback if no arc with residual capacity
          assert(a != nullptr);
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
    Find_Aumenting_Path(const Net & __g) : net(__g)
    {
      // empty
    }

    /// Find a concrete Path with at least `min_slack`.
    Path<Net> operator ()(typename Net::Node *start,
                          typename Net::Node *end,
                          typename Net::Flow_Type min_slack = 0)
    {
      return find(start, end, min_slack);
    }

    /// Find an augmenting semi-path from the network source to sink.
    SemiPath<Net> operator ()(typename Net::Flow_Type min_slack = 0)
    {
      return find_aum_path(min_slack);
    }

    /** @brief Fill `semi_path` and return the slack value.
     *
     * Returns 0 if no path is found.
     */
    typename Net::Flow_Type
    semi_path(typename Net::Node *start,
              typename Net::Node *end,
              DynList<Parc<Net>> & semi_path,
              const typename Net::Flow_Type & min_slack = 0)
    {
      semi_path.empty();

      auto result = find_path(start, end, min_slack);
      if (not get<0>(result))
        return typename Net::Flow_Type{};

      semi_path = std::move(get<2>(result));
      return get<1>(result);
    }
  };

  /// Augmenting-path finder using DFS (stack).
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
  Path<Net> find_aumenting_path(const Net & net,
                                const typename Net::Flow_Type & min_slack)
  {
    auto s = net.get_source();
    auto t = net.get_sink();
    return Find_Aumenting_Path<Net, Q>(net)(s, t, min_slack);
  }


  /** @brief Find an augmenting path using DFS. */
  template <class Net>
  Path<Net> find_aumenting_path_dfs(const Net & net,
                                    const typename Net::Flow_Type & min_slack)
  {
    return find_aumenting_path<Net, DynListStack>(net, min_slack);
  }


  /** @brief Find an augmenting path using BFS. */
  template <class Net>
  Path<Net> find_aumenting_path_bfs(const Net & net,
                                    const typename Net::Flow_Type & min_slack)
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
  SemiPath<Net> find_aumenting_semi_path(const Net & net,
                                         const typename Net::Flow_Type & slack)
  {
    return Find_Aumenting_Path<Net, Q>(net).find_aum_path(slack);
  }


  /** @brief Find an augmenting semi-path using DFS. */
  template <class Net>
  SemiPath<Net>
  find_aumenting_semi_path_dfs(const Net & net,
                               const typename Net::Flow_Type & slack)
  {
    return find_aumenting_semi_path<Net, DynListStack>(net, slack);
  }


  /** @brief Find an augmenting semi-path using BFS. */
  template <class Net>
  SemiPath<Net>
  find_aumenting_semi_path_bfs(const Net & net,
                               const typename Net::Flow_Type & slack)
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
  SemiPath<Net>
  find_decrementing_path(const Net & net,
                         const typename Net::Flow_Type & slack)
  {
    return Find_Aumenting_Path<Net, Q>(net).find_dec_path(slack);
  }


  /** @brief Find a decrementing semi-path using DFS. */
  template <class Net>
  SemiPath<Net>
  find_decrementing_path_dfs(const Net & net,
                             const typename Net::Flow_Type & slack)
  {
    return find_decrementing_path<Net, DynListStack>(net, slack);
  }


  /** @brief Find a decrementing semi-path using BFS. */
  template <class Net>
  SemiPath<Net>
  find_decrementing_path_bfs(const Net & net,
                             const typename Net::Flow_Type & slack)
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
    PP_Res_Node(const Empty_Class & info) : Base(info) {}

    /// Move constructor from node info
    PP_Res_Node(Empty_Class && info) : Base(std::move(info)) {}
  };

  template <class Net>
  struct __Res_Arc : public Net_Arc<Empty_Class, typename Net::Flow_Type>
  {
    using Base = Net_Arc<Empty_Class, typename Net::Flow_Type>;

    typename Net::Arc *img = nullptr; // nullptr indicates residual arc
    __Res_Arc *dup = nullptr;

    /// Default constructor
    __Res_Arc() : Base() {}

    /// Constructor from arc info
    __Res_Arc(const Empty_Class & info) : Base(info) {}

    /// Move constructor from arc info  
    __Res_Arc(Empty_Class && info) : Base(std::move(info)) {}

    /// Return true if this is a residual arc.
    bool is_residual() const { return img == nullptr; }
  };


  /// Residual network for augmenting-path algorithms.
  template <class Net>
  using AP_Res_Net = Array_Digraph<Net_Node<Empty_Class>, __Res_Arc<Net>>;

  /// Residual network for preflow-push algorithms.
  template <class Net>
  using PP_Res_Net = Array_Digraph<PP_Res_Node<Net>, __Res_Arc<Net>>;

  /// Create residual arcs for `a` in the residual network.
  template <class Net>
  inline
  void create_residual_arc(const Net & net, PP_Res_Net<Net> & rnet,
                           typename Net::Arc *a)
  {
    using Rnet = PP_Res_Net<Net>;
    auto s = net.get_src_node(a);
    auto t = net.get_tgt_node(a);

    assert(NODE_COOKIE(s) and NODE_COOKIE(t));

    auto src = static_cast<typename Rnet::Node *>(NODE_COOKIE(s));
    auto tgt = static_cast<typename Rnet::Node *>(NODE_COOKIE(t));

    auto arc = rnet.insert_arc(src, tgt);
    auto dup = rnet.insert_arc(tgt, src);

    arc->img = a; // mark it as not residual
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

    bool operator ()(typename Rnet::Arc *a) const
    {
      return a->cap > a->flow;
    }
  };

  /** @brief Build the residual network for preflow-push algorithms.
   *
   * Returns the residual network and the mapped source/sink nodes.
   */
  template <class Net>
  inline
  std::tuple<PP_Res_Net<Net>, typename PP_Res_Net<Net>::Node *,
        typename PP_Res_Net<Net>::Node *>
  preflow_create_residual_net(Net & net)
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
        NODE_COOKIE(p) = reinterpret_cast<void*>(q);
        NODE_COOKIE(q) = reinterpret_cast<void*>(p);
      }

    for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      create_residual_arc(net, rnet, it.get_curr());

    return std::make_tuple(std::move(rnet),
                      static_cast<typename Rnet::Node *>(NODE_COOKIE(net.get_source())),
                      static_cast<typename Rnet::Node *>(NODE_COOKIE(net.get_sink())));
  }

  /// Propagate residual arc flows back to their original arcs.
  template <class Rnet>
  inline
  void update_flow(const Rnet & rnet)
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
   * @param[in,out] net Flow network (must have a single source and sink).
   * @return Maximized flow value.
   *
   * @ingroup Networks
   */
  template <class Net,
            template <class> class Find_Path>
  typename Net::Flow_Type aumenting_path_maximum_flow(Net & net)
  {
    ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
      << "Network is not single source and single sink";

    while (true) // while an augmenting path exists
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

    return net.get_out_flow(net.get_source());
  }


  /** @brief Maximize flow using the Ford-Fulkerson algorithm.
   *
   * @param[in,out] net Flow network (must have a single source and sink).
   * @return Maximized flow value.
   *
   * @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type ford_fulkerson_maximum_flow(Net & net)
  {
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
    typename Net::Flow_Type operator ()(Net & net) const
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
  typename Net::Flow_Type edmonds_karp_maximum_flow(Net & net)
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
    typename Net::Flow_Type operator ()(Net & net) const
    {
      return edmonds_karp_maximum_flow<Net>(net);
    }
  };

  /// Return true if `p` has excess flow in `net`.
  template <class Net>
  static inline
  bool is_node_active(const Net & net, typename Net::Node *p)
  {
    return net.get_in_flow(p) != net.get_out_flow(p);
  }

  /// Return true if residual node `p` has excess flow.
  template <class Rnet>
  static inline
  bool is_node_active(typename Rnet::Node *p)
  {
    return p->in_flow != p->out_flow;
  }


  /// Access the height label stored in NODE_COUNTER.
  template <class Net>
  static inline
  long &node_height(typename Net::Node *p) { return NODE_COUNTER(p); }


  /// Initialize node heights with BFS distance to the sink.
  template <class Net>
  static inline
  void init_height_in_nodes(Net & net)
  {
    Graph_Traverse_BFS<Net, Node_Arc_Iterator<Net>>(net).
        exec(net.get_sink(),
             [&net](typename Net::Node *p, typename Net::Arc *a)
               {
                 if (a)
                   node_height<Net>(p) = node_height<Net>(net.get_tgt_node(a)) + 1;
                 return true;
               });
  }


  /// Enqueue an active node if it is not already enqueued.
  template <class Q_Type>
  static inline
  void put_in_active_queue(Q_Type & q, typename Q_Type::Item_Type & p)
  {
    if (NODE_BITS(p).get_bit(Aleph::Maximum_Flow))
      return; // the node is already into the queue
    NODE_BITS(p).set_bit(Aleph::Maximum_Flow, true);
    q.put(p);
  }


  /// Dequeue an active node and clear its queue mark.
  template <class Q_Type>
  static inline
  typename Q_Type::Item_Type get_from_active_queue(Q_Type & q)
  {
    auto p = q.get();
    assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));
    NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);
    return p;
  }


  /// Remove a node from the active queue and clear its queue mark.
  template <class Q_Type>
  static inline
  void remove_from_active_queue(Q_Type & q, typename Q_Type::Item_Type & p)
  {
    assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));
    NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);
    q.remove(p);
  }


  /** @brief Generic preflow-push maximum flow (vertex-oriented).
   *
   * Q_Type is the active-node container and must support:
   * - get(): remove a node
   * - put(p): insert a node
   *
   * The Maximum_Flow bit is used to track whether a node is in the queue.
   *
   * @param[in,out] net Flow network (must have a single source and sink).
   * @return Maximized flow value.
   *
   * @see fifo_preflow_maximum_flow() heap_preflow_maximum_flow() random_preflow_maximum_flow()
   * @ingroup Networks
   */
  template <class Net, class Q_Type>
  typename Net::Flow_Type
  generic_preflow_vertex_push_maximum_flow(Net & net)
  {
    ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
      << "Network is not single source and single sink";

    net.reset_nodes(); // especially assures that counters are in zero
    init_height_in_nodes(net); // set height to minimum distance in nodes to sink

    auto source = net.get_source();
    auto sink = net.get_sink();
    node_height<Net>(source) = net.vsize(); // except the source
    constexpr auto Max = std::numeric_limits<long>::max();

    using Itor = __Net_Iterator<Net>;
    Q_Type q;
    for (Itor it(source); it.has_curr(); it.next_ne()) // initial preflow
      {
        auto arc = it.get_curr();
        arc->flow = arc->cap; // saturate arc
        auto tgt = net.get_tgt_node(arc);
        put_in_active_queue(q, tgt);
      }

    while (not q.is_empty()) // while there are active nodes
      {
        auto src = get_from_active_queue(q);
        auto excess = net.get_in_flow(src) - net.get_out_flow(src);
        long m = Max;
        bool was_eligible_arc = false;
        for (Itor it(src); it.has_curr() and excess > 0; it.next_ne())
          {
            auto arc = it.get_curr();
            auto tgt = net.get_connected_node(arc, src);
            if (node_height<Net>(src) != node_height<Net>(tgt) + 1)
              {
                m = std::min(m, node_height<Net>(tgt));
                continue;
              }

            was_eligible_arc = true;
            typename Net::Flow_Type flow_to_push;
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
            if (tgt != source and tgt != sink)
              {
                assert(is_node_active(net, tgt));
                put_in_active_queue(q, tgt);
              }
          }

        if (excess > 0) // is src still active
          {
            if (not was_eligible_arc)
              node_height<Net>(src) = m + 1;
            put_in_active_queue(q, src);
          }
      }

    assert(net.check_network());

    return net.flow_value();
  }


  /// Initialize node heights in a residual network from `sink`.
  template <class Rnet>
  inline
  void init_height_in_nodes(Rnet & rnet, typename Rnet::Node *sink)
  {
    Graph_Traverse_BFS<Rnet, Node_Arc_Iterator<Rnet>>(rnet).
        exec(sink, [&rnet](typename Rnet::Node *p, typename Rnet::Arc *a)
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
  typename Net::Flow_Type
  fifo_preflow_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    return generic_preflow_vertex_push_maximum_flow
        <Net, DynListQueue<Node *>>(net);
  }

  /** @brief Functor wrapper for fifo_preflow_maximum_flow().
   *
   * @ingroup Networks
   */
  template <class Net>
  struct Fifo_Preflow_Maximum_Flow
  {
    /// Invoke fifo_preflow_maximum_flow().
    typename Net::Flow_Type
    operator ()(Net & net) const
    {
      return fifo_preflow_maximum_flow(net);
    }
  };


  /// Compare nodes by height (higher first).
  template <class Net>
  struct Compare_Height
  {
    bool operator ()(typename Net::Node *n1, typename Net::Node *n2) const
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
  typename Net::Flow_Type
  heap_preflow_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    return generic_preflow_vertex_push_maximum_flow
        <Net, DynBinHeap<Node *, Compare_Height<Net>>>(net);
  }

  /** @brief Functor wrapper for heap_preflow_maximum_flow().
   *
   * @ingroup Networks
   */
  template <class Net>
  struct Heap_Preflow_Maximum_Flow
  {
    /// Invoke heap_preflow_maximum_flow().
    typename Net::Flow_Type operator ()(Net & net) const
    {
      return heap_preflow_maximum_flow(net);
    }
  };

  /** @brief Preflow-push maximum flow using a random active-node queue.
   *
   * @param[in,out] net Flow network (must have a single source and sink).
   * @return Maximized flow value.
   *
   * @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type
  random_preflow_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    return generic_preflow_vertex_push_maximum_flow<Net, Random_Set<Node *>>(net);
  }


  /** @brief Functor wrapper for random_preflow_maximum_flow().
   *
   * @ingroup Networks
   */
  template <class Net>
  struct Random_Preflow_Maximum_Flow
  {
    /// Invoke random_preflow_maximum_flow().
    typename Net::Flow_Type operator ()(Net & net) const
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
  typename Net::Flow_Type min_cut(Net & net,
                                  DynSetTree<typename Net::Node *> & vs,
                                  DynSetTree<typename Net::Node *> & vt,
                                  DynList<typename Net::Arc *> & cuts,
                                  DynList<typename Net::Arc *> & cutt)
  {
    Maxflow<Net>()(net); // compute max flow

    typename Net::Node *source = net.get_source();

    // Traverse the residual network from source: reachable nodes go to vs.
    (Graph_Traverse_BFS<Net, Net_Iterator<Net>>(net))
        (source, [&vs](typename Net::Node *p)
           {
             vs.insert(p);
             return true;
           });

    // Compute vt by complement.
    const size_t size_vt = net.get_num_nodes() - vs.size();
    for (Node_Iterator<Net> it(net); it.has_curr() and
                                     vt.size() < size_vt; it.next_ne())
      {
        if (auto p = it.get_curr(); p->state() == Unprocessed)
          vt.insert(p);
      }

    for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        if (arc->flow == 0) // residual candidate
          if (vt.contains(net.get_src_node(arc)) and // vt -> vs
              vs.contains(net.get_tgt_node(arc)))
            {
              cutt.insert(arc);
              continue;
            }

        if (arc->flow == arc->cap) // saturated candidate
          if (vs.contains(net.get_src_node(arc)) and // vs -> vt
              vt.contains(net.get_tgt_node(arc)))
            cuts.insert(arc);
      }

    return net.flow_value();
  }


  /** @brief Functor wrapper for min_cut().
   *
   * @ingroup Networks
   */
  template <class Net,
            template <class> class Maxflow = Heap_Preflow_Maximum_Flow>
  struct Min_Cut
  {
    /// Invoke min_cut().
    typename Net::Flow_Type operator ()(Net & net,
                                        DynSetTree<typename Net::Node *> & vs,
                                        DynSetTree<typename Net::Node *> & vt,
                                        DynList<typename Net::Arc *> & cuts,
                                        DynList<typename Net::Arc *> & cutt)
    {
      return min_cut<Net, Maxflow>(net, vs, vt, cuts, cutt);
    }
  };
} // end namespace Aleph

# endif // TPL_NET_H
