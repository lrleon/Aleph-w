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

  /// Outgoing capacity constraint
  Flow_Type out_cap = Flow_Type{0};

  /// Incoming capacity constraint
  Flow_Type in_cap = Flow_Type{0};

  /// Tracked outgoing flow
  Flow_Type out_flow = Flow_Type{0};

  /// Tracked incoming flow
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

  /// Default constructor. Initializes all values to zero.
  Net_Sup_Dem_Node() : Base() {}

  /**
   * @brief Construct node with info.
   * 
   * Creates a node with the given information. Capacity and flow
   * attributes are initialized to zero.
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

public:
  /// @name Constructors and Destructor
  /// @{

  /// Default constructor
  Net_Sup_Dem_Graph() = default;

  /// Destructor. Frees auxiliary network if it exists.
  ~Net_Sup_Dem_Graph()
  {
    if (exist_aux_net())
      free_aux_net();
  }

  /// @}

  // Bring base class insert_node overloads into scope
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
  Node* insert_node(const Node_Type& node_info, const Flow_Type& supply = 0)
  {
    Node* p = Net_Class::insert_node(node_info);
    p->supply_flow = supply;
    return p;
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
   * The auxiliary network is built in-place on this graph.
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

    super_source = insert_node(0);  // auxiliary source
    super_sink = insert_node(0);    // auxiliary sink

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
   * A flow is feasible if:
   * - All supply nodes have out_flow >= supply_flow
   * - All demand nodes have in_flow >= |supply_flow|
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

    for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
      {
        Node* p = it.get_curr();
        const Flow_Type& supply_flow = p->supply_flow;

        if (supply_flow == 0)
          continue;

        if (supply_flow > 0 and p->out_flow < supply_flow)
          return false;  // Supply node not providing enough flow

        if (supply_flow < 0 and p->in_flow < -supply_flow)
          return false;  // Demand node not receiving enough flow
      }

    return true;
  }

  /**
   * @brief Get lists of nodes with unsatisfied supply/demand.
   *
   * Traverses all nodes and identifies those where the flow doesn't
   * satisfy the supply or demand requirements.
   *
   * @param[out] supply_list Nodes with out_flow < supply_flow
   * @param[out] demand_list Nodes with in_flow < |supply_flow|
   * @throw std::bad_alloc if memory allocation fails for the lists
   */
  void non_feasible_nodes(DynDlist<Node*>& supply_list,
                          DynDlist<Node*>& demand_list)
  {
    for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_curr(); it.next_ne())
      {
        Node* p = it.get_curr();
        const Flow_Type& supply_flow = p->supply_flow;

        if (supply_flow == 0)
          continue;

        if (supply_flow > 0 and p->out_flow < supply_flow)
          supply_list.append(p);
        else if (supply_flow < 0 and p->in_flow < -supply_flow)
          demand_list.append(p);
      }
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
