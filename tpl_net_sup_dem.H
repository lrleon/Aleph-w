
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


/** @file tpl_net_sup_dem.H
 *  @brief Supply-demand network flow algorithms.
 *
 *  This file provides classes for modeling and analyzing supply-demand
 *  networks. In such networks, nodes have supply values (positive = source,
 *  negative = sink/demand). The goal is to determine if it's possible to
 *  satisfy all demands from the available supplies.
 *
 *  ## Key Concepts
 *
 *  - **Supply node**: A node with positive supply_flow that provides flow
 *  - **Demand node**: A node with negative supply_flow that consumes flow
 *  - **Feasibility**: Whether all demands can be satisfied from supplies
 *
 *  @par Example: Basic supply-demand network
 *  @code
 *  using NetSD = Net_Sup_Dem_Graph<Net_Sup_Dem_Node<string, int>,
 *                                   Net_Arc<Empty_Class, int>>;
 *  NetSD net;
 *
 *  // Create nodes with supply/demand values
 *  auto supplier = net.insert_node("Factory", 100);   // supplies 100 units
 *  auto consumer = net.insert_node("Customer", -50);  // demands 50 units
 *  auto transit  = net.insert_node("Warehouse", 0);   // transit node
 *
 *  // Connect nodes with arcs (capacity)
 *  net.insert_arc(supplier, transit, 80);
 *  net.insert_arc(transit, consumer, 60);
 *
 *  // Compute auxiliary network and check feasibility
 *  net.compute_aux_net();
 *  if (net.is_feasible())
 *    cout << "All demands can be satisfied!\n";
 *  @endcode
 *
 *  @par Example: Multi-source multi-sink network
 *  @code
 *  NetSD distribution_net;
 *  
 *  // Multiple suppliers
 *  auto warehouse1 = distribution_net.insert_node("WH1", 200);
 *  auto warehouse2 = distribution_net.insert_node("WH2", 150);
 *  
 *  // Multiple consumers
 *  auto store1 = distribution_net.insert_node("Store1", -120);
 *  auto store2 = distribution_net.insert_node("Store2", -180);
 *  
 *  // Distribution hub
 *  auto hub = distribution_net.insert_node("Hub", 0);
 *  
 *  // Connect with capacities
 *  distribution_net.insert_arc(warehouse1, hub, 150);
 *  distribution_net.insert_arc(warehouse2, hub, 120);
 *  distribution_net.insert_arc(hub, store1, 120);
 *  distribution_net.insert_arc(hub, store2, 180);
 *  @endcode
 *
 *  @see tpl_net.H Base network flow classes
 *  @see tpl_netcapgraph.H Capacity graph algorithms
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_NET_SUP_DEM_H
#define TPL_NET_SUP_DEM_H

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>
#include <tpl_net.H>
#include <ah-errors.H>

namespace Aleph
{

/**
 * @brief Node with supply/demand flow value.
 *
 * Extends Net_Node with a supply_flow attribute that indicates:
 * - Positive value: the node supplies (produces) that amount of flow
 * - Negative value: the node demands (consumes) the absolute amount
 * - Zero: the node is a transit node (neither supplies nor demands)
 *
 * @tparam Node_Info Type of information stored in the node
 * @tparam F_Type Type representing flow values (default: long)
 *
 * @ingroup Graphs
 */
template <typename Node_Info, typename F_Type = long>
class Net_Sup_Dem_Node : public Graph_Anode<Node_Info>
{
public:
  /// Type representing flow values
  using Flow_Type = F_Type;
  
  /// Base class type
  using Base = Graph_Anode<Node_Info>;

  /// Supply flow value: positive = supply, negative = demand, zero = transit
  Flow_Type supply_flow = Flow_Type{0};

  /// The most that the node may supply; unlimited unless set
  Flow_Type out_cap = std::numeric_limits<Flow_Type>::max();

  /// The most that the node may demand; unlimited unless set
  Flow_Type in_cap = std::numeric_limits<Flow_Type>::max();

  /// Outgoing flow, for the user: the network does not write it
  Flow_Type out_flow = Flow_Type{0};

  /// Incoming flow, for the user: the network does not write it
  Flow_Type in_flow = Flow_Type{0};

  /**
   * @brief Get the supply/demand flow value.
   * @return Reference to the supply_flow attribute
   */
  [[nodiscard]] Flow_Type& get_supply_flow() noexcept { return supply_flow; }

  /**
   * @brief Get the supply/demand flow value (const version).
   * @return Const reference to the supply_flow attribute
   */
  [[nodiscard]] const Flow_Type& get_supply_flow() const noexcept 
  { 
    return supply_flow; 
  }

  /// Default constructor: no supply, unlimited capacities and no flows.
  Net_Sup_Dem_Node() : Base() {}

  /**
   * @brief Construct node with info.
   *
   * Creates a node with the given information, no supply, unlimited
   * capacities and no flows.
   *
   * @param node_info Information to store in the node
   */
  explicit Net_Sup_Dem_Node(const Node_Info& node_info)
    : Base(node_info)
  {}

  /**
   * @brief Copy constructor from pointer.
   *
   * Creates a copy of the node pointed to, including its supply_flow value.
   *
   * @param node Pointer to the node to copy
   */
  Net_Sup_Dem_Node(Net_Sup_Dem_Node* node)
    : Base(*node), 
      supply_flow(node->supply_flow),
      out_cap(node->out_cap),
      in_cap(node->in_cap),
      out_flow(node->out_flow),
      in_flow(node->in_flow)
  {}
};


/**
 * @brief Network graph with supply and demand nodes.
 *
 * A supply-demand network is a flow network where some nodes have
 * supply values (sources) and others have demand values (sinks).
 * The goal is typically to determine if all demands can be satisfied
 * from available supplies (feasibility check).
 *
 * ## How it works
 *
 * 1. Create nodes with supply/demand values using insert_node()
 * 2. Connect nodes with arcs having capacities
 * 3. Call compute_aux_net() to create the auxiliary capacitated network
 * 4. Run a max-flow algorithm on the network
 * 5. Call is_feasible() to check if all demands are satisfied
 * 6. Call free_aux_net() to release auxiliary structures
 *
 * ## Implementation Details
 *
 * The auxiliary network is constructed by:
 * - Creating a super-source connected to all supply nodes
 * - Creating a super-sink connected from all demand nodes
 * - Arc capacities from super-source equal supply values
 * - Arc capacities to super-sink equal demand values (absolute)
 *
 * @tparam NodeT Node type (default: Net_Sup_Dem_Node with double flow)
 * @tparam ArcT Arc type (default: Net_Arc with double flow)
 *
 * @ingroup Graphs
 */
template <class NodeT = Net_Sup_Dem_Node<Empty_Class, double>,
          class ArcT = Net_Arc<Empty_Class, double>>
class Net_Sup_Dem_Graph : public Net_Graph<NodeT, ArcT>
{
public:
  /// The base network class type
  using Net_Class = Net_Graph<NodeT, ArcT>;
  /// Arc type
  using Arc = ArcT;
  /// Node type
  using Node = NodeT;
  /// Type representing capacity and flow values
  using Flow_Type = typename Arc::Flow_Type;
  /// Type of information stored in nodes
  using Node_Type = typename Node::Node_Type;
  /// Type of information stored in arcs
  using Arc_Type = typename Arc::Arc_Type;
  /// Auxiliary network type (same as this class)
  using Aux_Net = Net_Sup_Dem_Graph;

private:
  Node* super_source = nullptr;  ///< Super-source for auxiliary network
  Node* super_sink = nullptr;    ///< Super-sink for auxiliary network

  /** @brief Whether the flow fills an arc of the super-source or of the
             super-sink: whether its node ships all its supply, or
             receives all its demand.

      Exactly for an integer flow. For a floating flow, within the
      tolerance that check_node() gives the flows of the arc, 1e-9 of
      its capacity: the sums that bring the flow to the arc round, as
      2.4 - 1.1, an ulp below 1.3 in binary, so that a demand of 1.3 that
      a supply of 2.4 meets after one of 1.1 comes out an ulp short. The
      scale is the arc's own, so that a real deficit stays beyond it, also
      that of a supply or a demand far smaller than the others. Only an
      infinite flow fills an infinite capacity, that of an infinite supply
      or demand of a node of infinite capacities: its tolerance, infinite
      too, would take any flow, and inf - inf is NaN.

      @param[in] a Arc from the super-source or to the super-sink.
      @return true if the flow of `a` fills its capacity.
  */
  static bool filled(const Arc *a) noexcept
  {
    if constexpr (std::is_floating_point_v<Flow_Type>)
      return a->flow == a->cap
        or (std::isfinite(a->cap)
            and std::abs(a->cap - a->flow) <= Net_Class::node_tolerance(a->cap, a->flow));
    else
      return a->flow == a->cap;
  }

public:
  /// @name Constructors and Destructor
  /// @{

  /// Default constructor
  Net_Sup_Dem_Graph() = default;

  /**
   * @brief Copy constructor.
   *
   * The copy has the supplies, capacities and flows of the nodes, and
   * the auxiliary network, if other has it.
   *
   * @throw std::bad_alloc if memory allocation fails
   */
  Net_Sup_Dem_Graph(const Net_Sup_Dem_Graph & other)
    : Net_Class(other)
  {
    // The copy of the network builds the nodes from their infos, in the
    // order of other
    Node_Iterator<Net_Sup_Dem_Graph> it(*this);
    for (Node_Iterator<Net_Sup_Dem_Graph> jt(other); jt.has_curr();
         it.next_ne(), jt.next_ne())
      {
        Node * p = it.get_curr();
        const Node * q = jt.get_curr();
        p->supply_flow = q->supply_flow;
        p->out_cap = q->out_cap;
        p->in_cap = q->in_cap;
        p->out_flow = q->out_flow;
        p->in_flow = q->in_flow;
        if (q == other.super_source)
          super_source = p;
        if (q == other.super_sink)
          super_sink = p;
      }
  }

  /// Move constructor. O(1).
  Net_Sup_Dem_Graph(Net_Sup_Dem_Graph && other) noexcept
    : Net_Class(std::move(other)),
      super_source(std::exchange(other.super_source, nullptr)),
      super_sink(std::exchange(other.super_sink, nullptr))
  {
    // empty
  }

  /**
   * @brief Copy assignment, as the copy constructor; the network keeps
   * its cookie.
   *
   * @throw std::bad_alloc if memory allocation fails. Then the network
   *        is as it was.
   */
  Net_Sup_Dem_Graph & operator = (const Net_Sup_Dem_Graph & other)
  {
    if (this != &other)
      {
        Net_Sup_Dem_Graph tmp(other);
        tmp.get_cookie() = this->get_cookie();
        swap(tmp);
      }
    return *this;
  }

  /// Move assignment. O(1).
  Net_Sup_Dem_Graph & operator = (Net_Sup_Dem_Graph && other) noexcept
  {
    if (this != &other)
      swap(other);
    return *this;
  }

  /// Swaps the networks, with their super nodes. O(1).
  void swap(Net_Sup_Dem_Graph & other) noexcept
  {
    Net_Class::swap(other);
    std::swap(super_source, other.super_source);
    std::swap(super_sink, other.super_sink);
  }

  /// @}

  // Bring base class insert_node overloads into scope; those below with
  // an info hide that of Net_Graph, which a call with an info would find
  // as good
  using Net_Class::insert_node;

  /// @name Node Insertion
  /// @{

  /**
   * @brief Insert a node with info and supply/demand value.
   *
   * Creates and inserts a new node with the given information and
   * supply/demand value.
   *
   * @param node_info Information to store in the node
   * @param supply Supply value (positive = supply, negative = demand)
   * @return Pointer to the newly created node
   * @throw std::bad_alloc if memory allocation fails
   */
  Node* insert_node(const Node_Type& node_info, const Flow_Type& supply)
  {
    Node* p = Net_Class::insert_node(node_info);
    p->supply_flow = supply;
    return p;
  }

  /**
   * @brief Insert a node with info and no supply nor demand.
   *
   * @param node_info Information to store in the node
   * @return Pointer to the newly created node
   * @throw std::bad_alloc if memory allocation fails
   */
  Node* insert_node(const Node_Type& node_info)
  {
    return insert_node(node_info, Flow_Type{0});
  }

  /**
   * @brief Insert a node with supply/demand value only.
   *
   * Creates and inserts a new node with default info and the given
   * supply/demand value.
   *
   * @param supply Supply value (positive = supply, negative = demand)
   * @return Pointer to the newly created node
   * @throw std::bad_alloc if memory allocation fails
   */
  Node* insert_node(const Flow_Type& supply)
    requires (not std::is_same_v<Node_Type, Flow_Type>)
  {
    return insert_node(Node_Type(), supply);
  }

  /// @}

  /// @name Auxiliary Network Management
  /// @{

  /**
   * @brief Check if the auxiliary network has been computed.
   * @return true if auxiliary network exists
   */
  [[nodiscard]] bool exist_aux_net() const noexcept
  {
    return super_source != nullptr or super_sink != nullptr;
  }

  /**
   * @brief Compute the auxiliary capacitated network.
   *
   * Constructs the auxiliary network needed for max-flow computation.
   * This creates:
   * - A super-source connected to all supply nodes (supply_flow > 0)
   * - A super-sink connected from all demand nodes (supply_flow < 0)
   *
   * If there are both, the other sources and sinks of the network, as a
   * node without arcs in that supplies nothing, are joined to them with
   * arcs of capacity 0, so that the network has one source and one sink
   * for a maximum flow. The auxiliary network is built in-place on this
   * graph.
   *
   * @return Pointer to this network (with auxiliary structures added)
   * @throw std::domain_error if auxiliary network already exists
   * @throw std::range_error if any supply exceeds node capacity
   * @throw std::bad_alloc if memory allocation fails
   */
  Net_Sup_Dem_Graph* compute_aux_net()
  {
    ah_domain_error_if(exist_aux_net())
      << "Auxiliary net is already computed";

    super_source = Net_Class::insert_node();  // auxiliary source
    super_sink = Net_Class::insert_node();    // auxiliary sink

    // Connect super-source/sink based on supply/demand values
    for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
      {
        Node* p = it.get_curr();
        if (p->supply_flow > 0)  // supply node?
          {
            ah_range_error_if(p->out_cap < p->supply_flow)
              << "Supply flow in node at " << static_cast<void*>(p)
              << " is greater than out capacity";
            this->insert_arc(super_source, p, p->supply_flow);
          }
        else if (p->supply_flow < 0)  // demand node?
          {
            ah_range_error_if(p->in_cap < -p->supply_flow)
              << "Supply flow in node at " << static_cast<void*>(p)
              << " is smaller than in capacity";
            this->insert_arc(p, super_sink, -p->supply_flow);
          }
      }

    // Remove super-source if no supply nodes
    if (this->get_out_degree(super_source) == 0)
      {
        this->remove_node(super_source);
        super_source = nullptr;
      }

    // Remove super-sink if no demand nodes
    if (this->get_in_degree(super_sink) == 0)
      {
        this->remove_node(super_sink);
        super_sink = nullptr;
      }

    if (super_source == nullptr or super_sink == nullptr)
      return this;

    // The other sources and sinks, through arcs of capacity 0
    DynList<Node*> sources, sinks;
    for (auto it = this->get_src_nodes().get_it(); it.has_curr(); it.next_ne())
      if (it.get_curr() != super_source)
        sources.append(it.get_curr());
    for (auto it = this->get_sink_nodes().get_it(); it.has_curr(); it.next_ne())
      if (it.get_curr() != super_sink)
        sinks.append(it.get_curr());
    for (auto it = sources.get_it(); it.has_curr(); it.next_ne())
      this->insert_arc(super_source, it.get_curr(), Flow_Type{0});
    for (auto it = sinks.get_it(); it.has_curr(); it.next_ne())
      this->insert_arc(it.get_curr(), super_sink, Flow_Type{0});

    return this;
  }

  /**
   * @brief Get the auxiliary network if it exists.
   * @return Pointer to this network if auxiliary exists, nullptr otherwise
   */
  [[nodiscard]] Net_Sup_Dem_Graph* get_aux_net() noexcept
  {
    return exist_aux_net() ? this : nullptr;
  }

  /**
   * @brief Get the super-source node.
   * @return Pointer to super-source, or nullptr if not computed
   */
  [[nodiscard]] Node* get_super_source() const noexcept { return super_source; }

  /**
   * @brief Get the super-sink node.
   * @return Pointer to super-sink, or nullptr if not computed
   */
  [[nodiscard]] Node* get_super_sink() const noexcept { return super_sink; }

  /**
   * @brief Free the auxiliary network structures.
   *
   * Removes the super-source and super-sink nodes and their arcs.
   * Also clears the residual network if it exists.
   *
   * @throw std::domain_error if auxiliary network doesn't exist
   */
  void free_aux_net()
  {
    ah_domain_error_if(not exist_aux_net())
      << "Auxiliary net has not been computed";

    if (super_source != nullptr)
      {
        this->remove_node(super_source);
        super_source = nullptr;
      }

    if (super_sink != nullptr)
      {
        this->remove_node(super_sink);
        super_sink = nullptr;
      }
  }

  /// @}

  /// @name Feasibility Analysis
  /// @{

  /**
   * @brief Check if the current flow is feasible.
   *
   * A flow is feasible if every supply node ships its supply and every
   * demand node receives its demand: if the flow fills the arcs from the
   * super-source and those to the super-sink, exactly for an integer
   * flow, and within the tolerance of check_node() of each arc, 1e-9 of
   * its capacity, for a floating one: the sums of a floating flow round,
   * so that the demands of 1.1 and 1.3 that a supply of 2.4 meets come
   * out an ulp short, while a real deficit, also of a supply or a demand
   * far smaller than the others, stays beyond it. Without a super-source
   * or without a super-sink, nothing is shipped, and it is not.
   *
   * @return true if flow is feasible, false otherwise
   * @throw std::domain_error if auxiliary network hasn't been computed
   *
   * @note You must run a max-flow algorithm before calling this method
   */
  [[nodiscard]] bool is_feasible() const
  {
    ah_domain_error_if(not exist_aux_net())
      << "Auxiliary net has not been computed";

    if (super_source == nullptr or super_sink == nullptr)
      return false;

    for (typename Net_Class::Node_Arc_Iterator it(super_source); it.has_curr(); it.next_ne())
      if (not filled(it.get_curr()))
        return false;  // a supply node does not ship all its supply

    for (typename Net_Class::Node_Arc_Iterator it(super_sink); it.has_curr(); it.next_ne())
      if (not filled(it.get_curr()))
        return false;  // a demand node does not receive all its demand

    return true;
  }

  /**
   * @brief Get lists of nodes with unsatisfied supply/demand.
   *
   * As is_feasible(), it judges the flow of the auxiliary network: a
   * supply node that does not ship all its supply is one whose arc from
   * the super-source the flow does not fill, and a demand node that does
   * not receive all its demand, one whose arc to the super-sink it does
   * not fill, with the same tolerance for a floating flow, so that the
   * lists are empty if and only if is_feasible(). The fields `out_flow`
   * and `in_flow` of the nodes, which the network does not write, do not
   * count.
   *
   * @param[out] supply_list Nodes that do not ship all their supply
   * @param[out] demand_list Nodes that do not receive all their demand
   * @throw std::domain_error if the auxiliary net has not been computed
   * @throw std::bad_alloc if memory allocation fails for the lists
   *
   * @note You must run a max-flow algorithm before calling this method
   */
  void non_feasible_nodes(DynDlist<Node*>& supply_list,
                          DynDlist<Node*>& demand_list)
  {
    ah_domain_error_if(not exist_aux_net())
      << "Auxiliary net has not been computed";

    if (super_source != nullptr)
      for (typename Net_Class::Node_Arc_Iterator it(super_source); it.has_curr(); it.next_ne())
        if (Arc* a = it.get_curr(); not filled(a))
          supply_list.append(this->get_tgt_node(a));

    if (super_sink != nullptr)
      for (typename Net_Class::Node_Arc_Iterator it(super_sink); it.has_curr(); it.next_ne())
        if (Arc* a = it.get_curr(); not filled(a))
          demand_list.append(this->get_src_node(a));
  }

  /// @}

  /// @name Supply/Demand Modification
  /// @{

  /**
   * @brief Set the supply/demand value for a node.
   *
   * Changes the supply_flow attribute of the given node. Note that
   * after modifying supply values, you must recompute the max-flow
   * to check feasibility.
   *
   * @param p Pointer to the node
   * @param supply New supply value (positive = supply, negative = demand)
   * @throw std::range_error if supply exceeds node's out capacity
   * @throw std::range_error if demand exceeds node's in capacity
   */
  void set_supply_flow(Node* p, const Flow_Type& supply)
  {
    ah_range_error_if(supply > 0 and p->out_cap < supply)
      << "Supply flow in node at " << static_cast<void*>(p)
      << " is greater than out capacity";
    ah_range_error_if(supply < 0 and p->in_cap < -supply)
      << "Supply flow in node at " << static_cast<void*>(p)
      << " is smaller than in capacity";

    p->supply_flow = supply;
  }

  /**
   * @brief Get the supply/demand value for a node.
   *
   * @param p Pointer to the node
   * @return The supply_flow value of the node
   */
  [[nodiscard]] Flow_Type get_supply_flow(Node* p) const noexcept
  {
    return p->supply_flow;
  }

  /// @}

  /// @name Query Methods
  /// @{

  /**
   * @brief Count the number of supply nodes.
   * @return Number of nodes with positive supply_flow
   */
  [[nodiscard]] size_t count_supply_nodes() const noexcept
  {
    size_t count = 0;
    for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
      if (it.get_curr()->supply_flow > 0)
        ++count;
    return count;
  }

  /**
   * @brief Count the number of demand nodes.
   * @return Number of nodes with negative supply_flow
   */
  [[nodiscard]] size_t count_demand_nodes() const noexcept
  {
    size_t count = 0;
    for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
      if (it.get_curr()->supply_flow < 0)
        ++count;
    return count;
  }

  /**
   * @brief Calculate total supply in the network.
   * @return Sum of all positive supply_flow values
   */
  [[nodiscard]] Flow_Type total_supply() const noexcept
  {
    Flow_Type total = 0;
    for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
      {
        const Flow_Type& sf = it.get_curr()->supply_flow;
        if (sf > 0)
          total += sf;
      }
    return total;
  }

  /**
   * @brief Calculate total demand in the network.
   * @return Sum of absolute values of all negative supply_flow values
   */
  [[nodiscard]] Flow_Type total_demand() const noexcept
  {
    Flow_Type total = 0;
    for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
      {
        const Flow_Type& sf = it.get_curr()->supply_flow;
        if (sf < 0)
          total -= sf;  // Add absolute value
      }
    return total;
  }

  /**
   * @brief Check if the network is balanced.
   *
   * A network is balanced if total supply equals total demand.
   *
   * @return true if total_supply() == total_demand()
   */
  [[nodiscard]] bool is_balanced() const noexcept
  {
    return total_supply() == total_demand();
  }

  /// @}
};

} // end namespace Aleph

#endif // TPL_NET_SUP_DEM_H
