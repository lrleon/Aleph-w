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

/** @file tpl_netcapgraph.H
 *  @brief Network flow graph with capacities.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_NETCAPGRAPH_H
#define TPL_NETCAPGRAPH_H

#include <tpl_net.H>
#include <ah-errors.H>

namespace Aleph {

/** @brief Node with capacity constraint for flow networks.
 *
 * Net_Cap_Node extends the base graph node to include a maximum capacity
 * value that limits the total flow that can pass through this node
 * (both incoming and outgoing).
 *
 * This is useful for modeling networks where nodes themselves have
 * throughput constraints, such as:
 * - Network routers with bandwidth limits
 * - Processing stations with capacity limits
 * - Transportation hubs with handling capacity
 *
 * @tparam Node_Info Type of information stored in the node.
 * @tparam F_Type Numeric type for flow values (default: double).
 *
 * @see Net_Cap_Graph Net_Graph
 * @ingroup Networks
 */
template <typename Node_Info = Empty_Class, typename F_Type = double>
class Net_Cap_Node : public Graph_Anode<Node_Info>
{
  using Base = Graph_Anode<Node_Info>;

public:
  /// Type representing flow and capacity values.
  using Flow_Type = F_Type;
  
  /// Type of information stored in the node.
  using Node_Type = Node_Info;

  /// Maximum flow capacity that can pass through this node.
  Flow_Type max_cap;

  /// Tracked incoming flow (updated by update() after auxiliary net computation).
  Flow_Type in_flow = Flow_Type{0};

  /// Tracked outgoing flow (updated by update() after auxiliary net computation).
  Flow_Type out_flow = Flow_Type{0};

  /// Default constructor. Sets max_cap to the maximum possible value.
  Net_Cap_Node() noexcept
    : Base(), max_cap(std::numeric_limits<Flow_Type>::max())
  {
    // empty
  }

  /** @brief Construct a node with the given information.
   *
   * Capacity and flow attributes are initialized to their default values
   * (max capacity = numeric max, flows = 0).
   *
   * @param node_info Information to store in the node.
   */
  explicit Net_Cap_Node(const Node_Info& node_info) 
    : Base(node_info), 
      max_cap(std::numeric_limits<Flow_Type>::max())
  {
    // empty
  }

  /** @brief Move constructor from node info.
   *
   * @param node_info Information to move into the node.
   */
  explicit Net_Cap_Node(Node_Info&& node_info) noexcept
    : Base(std::move(node_info)),
      max_cap(std::numeric_limits<Flow_Type>::max())
  {
    // empty
  }

  /** @brief Copy constructor from another Net_Cap_Node.
   *
   * Copies all attributes including max_cap but resets flow values.
   *
   * @param node Pointer to the node to copy from.
   */
  Net_Cap_Node(const Net_Cap_Node* node) 
    : Base(node->get_info()),
      max_cap(node->max_cap),
      in_flow(node->in_flow),
      out_flow(node->out_flow)
  {
    // empty
  }

  /** @brief Copy constructor.
   *
   * @param other Node to copy from.
   */
  Net_Cap_Node(const Net_Cap_Node& other)
    : Base(other),
      max_cap(other.max_cap),
      in_flow(other.in_flow),
      out_flow(other.out_flow)
  {
    // empty
  }

  /** @brief Copy assignment operator.
   *
   * @param other Node to copy from.
   * @return Reference to this node.
   */
  Net_Cap_Node& operator=(const Net_Cap_Node& other)
  {
    if (this != &other)
    {
      Base::operator=(other);
      max_cap = other.max_cap;
      in_flow = other.in_flow;
      out_flow = other.out_flow;
    }
    return *this;
  }

  /** @brief Get the maximum capacity of this node.
   *
   * @return The maximum flow that can pass through this node.
   */
  [[nodiscard]] const Flow_Type& get_max_cap() const noexcept
  {
    return max_cap;
  }

  /** @brief Set the maximum capacity of this node.
   *
   * @param cap New capacity value (must be non-negative).
   * @throw std::domain_error if cap is negative.
   */
  void set_max_cap(const Flow_Type& cap)
  {
    ah_domain_error_if(cap < Flow_Type{0})
      << "Node capacity cannot be negative";
    max_cap = cap;
  }
};


/** @brief Capacitated network with node capacity constraints.
 *
 * Net_Cap_Graph models a flow network where nodes have maximum
 * throughput limits in addition to arc capacities. This extends the
 * standard maximum flow model to handle vertex-capacitated networks.
 *
 * @section transformation Network Transformation
 *
 * A node-capacitated network cannot be directly solved by standard
 * maximum flow algorithms. Instead, it must be transformed into an
 * equivalent arc-capacitated network (called the auxiliary network).
 *
 * The transformation works as follows:
 * - Each node v with capacity c(v) is split into two nodes: v_in and v_out
 * - An arc (v_in, v_out) with capacity c(v) is added
 * - All incoming arcs to v now go to v_in
 * - All outgoing arcs from v now come from v_out
 *
 * @section tpl_netcapgraph_usage Usage Pattern
 *
 * @code
 * Net_Cap_Graph<MyNode, MyArc> net;
 * 
 * // Build the network
 * auto n1 = net.insert_node("A", 10);  // Node with capacity 10
 * auto n2 = net.insert_node("B", 5);   // Node with capacity 5
 * net.insert_arc(n1, n2, 15, 0);       // Arc with capacity 15
 * 
 * // Compute auxiliary network
 * auto aux = net.compute_aux_net();
 * 
 * // Run max flow on auxiliary network
 * ford_fulkerson_maximum_flow(*aux);
 * 
 * // Transfer flow values back
 * net.update();
 * 
 * // Clean up
 * net.free_aux_net();
 * @endcode
 *
 * @tparam NodeT Node type (must derive from Net_Cap_Node).
 * @tparam ArcT Arc type (must derive from Net_Arc).
 *
 * @see Net_Graph compute_aux_net() Net_Cap_Node
 * @ingroup Networks
 */
template <class NodeT = Net_Cap_Node<Empty_Class, double>,
          class ArcT = Net_Arc<Empty_Class, double>>
class Net_Cap_Graph : public Net_Graph<NodeT, ArcT>
{
public:
  /// Base network class type.
  using Net_Class = Net_Graph<NodeT, ArcT>;
  
  /// Arc type.
  using Arc = ArcT;
  
  /// Node type.
  using Node = NodeT;
  
  /// Type representing capacity and flow values.
  using Flow_Type = typename Arc::Flow_Type;
  
  /// Type of information stored in nodes.
  using Node_Type = typename Node::Node_Type;
  
  /// Type of information stored in arcs.
  using Arc_Type = typename Arc::Arc_Type;

  /** @brief Insert a new capacitated node into the network.
   *
   * Creates a new node with the given information and capacity,
   * and inserts it into the network.
   *
   * @param node_info Information to store in the node.
   * @param cap Maximum flow capacity for this node. Defaults to
   *            the maximum representable value (unlimited).
   * @return Pointer to the newly created and inserted node.
   * @throw std::bad_alloc if memory allocation fails.
   * @throw std::domain_error if cap is negative.
   */
  Node* insert_node(const Node_Type& node_info, 
                    const Flow_Type& cap = std::numeric_limits<Flow_Type>::max())
  {
    ah_domain_error_if(cap < Flow_Type{0})
      << "Node capacity cannot be negative";
    
    Node* p = Net_Class::insert_node(node_info);
    p->max_cap = cap;
    return p;
  }

  /** @brief Insert a new node with default info and specified capacity.
   *
   * @param cap Maximum flow capacity for this node.
   * @return Pointer to the newly created and inserted node.
   * @throw std::bad_alloc if memory allocation fails.
   * @throw std::domain_error if cap is negative.
   */
  Node* insert_node(const Flow_Type& cap)
  {
    return insert_node(Node_Type{}, cap);
  }

  /** @brief Insert a new node with default info and unlimited capacity.
   *
   * @return Pointer to the newly created and inserted node.
   * @throw std::bad_alloc if memory allocation fails.
   */
  Node* insert_node()
  {
    return insert_node(Node_Type{}, std::numeric_limits<Flow_Type>::max());
  }

  /** @brief Auxiliary network type for solving node-capacitated networks.
   *
   * Aux_Net is the equivalent arc-capacitated network that can be
   * processed by standard maximum flow algorithms.
   *
   * The auxiliary network is created by compute_aux_net(). Each node
   * in Net_Cap_Graph maps to an arc in Aux_Net. The arc's boolean
   * info indicates whether it represents a node (true) or an original
   * arc (false).
   *
   * Mapping between networks:
   * - For a node p in Net_Cap_Graph: NODE_COOKIE(p) points to the
   *   corresponding arc in Aux_Net
   * - For an arc in Aux_Net: ARC_COOKIE(arc) points to either the
   *   original node (if arc->get_info() == true) or the original arc
   *   (if arc->get_info() == false)
   */
  using Aux_Net = Net_Graph<Net_Node<Empty_Class>, 
                            Net_Arc<bool, Flow_Type>>;

private:
  Aux_Net* aux_net = nullptr;

public:
  /** @brief Default constructor.
   *
   * Creates an empty node-capacitated network.
   */
  Net_Cap_Graph() noexcept
    : Net_Class(), aux_net(nullptr)
  {
    // empty
  }

  /** @brief Copy constructor.
   *
   * @param other Network to copy from.
   * @note The auxiliary network is NOT copied.
   */
  Net_Cap_Graph(const Net_Cap_Graph& other)
    : Net_Class(other), aux_net(nullptr)
  {
    // aux_net is not copied; must be recomputed if needed
  }

  /** @brief Move constructor.
   *
   * @param other Network to move from.
   */
  Net_Cap_Graph(Net_Cap_Graph&& other) noexcept
    : Net_Class(std::move(other)), aux_net(other.aux_net)
  {
    other.aux_net = nullptr;
  }

  /** @brief Copy assignment operator.
   *
   * @param other Network to copy from.
   * @return Reference to this network.
   * @note Frees existing auxiliary network before copying.
   */
  Net_Cap_Graph& operator=(const Net_Cap_Graph& other)
  {
    if (this != &other)
    {
      if (aux_net != nullptr)
        free_aux_net();
      Net_Class::operator=(other);
    }
    return *this;
  }

  /** @brief Move assignment operator.
   *
   * @param other Network to move from.
   * @return Reference to this network.
   */
  Net_Cap_Graph& operator=(Net_Cap_Graph&& other) noexcept
  {
    if (this != &other)
    {
      if (aux_net != nullptr)
        free_aux_net();
      Net_Class::operator=(std::move(other));
      aux_net = other.aux_net;
      other.aux_net = nullptr;
    }
    return *this;
  }

  /** @brief Destructor.
   *
   * Automatically frees the auxiliary network if it exists.
   */
  ~Net_Cap_Graph()
  {
    if (aux_net != nullptr)
    {
      clear_graph(*aux_net);
      delete aux_net;
      aux_net = nullptr;
    }
  }

  /** @brief Get pointer to the auxiliary network.
   *
   * @return Pointer to the auxiliary network, or nullptr if not computed.
   */
  [[nodiscard]] Aux_Net* get_aux_net() noexcept
  {
    return aux_net;
  }

  /** @brief Get const pointer to the auxiliary network.
   *
   * @return Const pointer to the auxiliary network, or nullptr if not computed.
   */
  [[nodiscard]] const Aux_Net* get_aux_net() const noexcept
  {
    return aux_net;
  }

  /** @brief Check if auxiliary network has been computed.
   *
   * @return true if auxiliary network exists, false otherwise.
   */
  [[nodiscard]] bool has_aux_net() const noexcept
  {
    return aux_net != nullptr;
  }

  /** @brief Compute the equivalent arc-capacitated auxiliary network.
   *
   * Transforms this node-capacitated network into an equivalent
   * arc-capacitated network that can be solved by standard maximum
   * flow algorithms.
   *
   * The transformation:
   * - Each node v is split into v_in and v_out
   * - Arc (v_in, v_out) with capacity = v.max_cap is added
   * - Original arcs are remapped accordingly
   *
   * Cookie mappings are established for update():
   * - NODE_COOKIE(p) -> arc representing node p
   * - ARC_COOKIE(node_arc) -> original node p
   * - ARC_COOKIE(edge_arc) -> original arc
   *
   * @return Pointer to the computed auxiliary network.
   * @throw std::domain_error if auxiliary network already exists.
   * @throw std::bad_alloc if memory allocation fails.
   *
   * @note Call update() after running max flow on the auxiliary network
   *       to transfer flow values back to this network.
   */
  Aux_Net* compute_aux_net()
  {
    ah_domain_error_if(aux_net != nullptr)
      << "Auxiliary network has already been computed";
    
    aux_net = new Aux_Net;
    
    try
    {
      // Phase 1: Create node pairs and capacity arcs
      for (Node_Iterator<Net_Cap_Graph> it(*this); it.has_curr(); it.next_ne())
      {
        Node* p = it.get_curr();
        
        // Create source and target nodes for this node
        typename Aux_Net::Node* src = aux_net->insert_node();
        typename Aux_Net::Node* tgt = aux_net->insert_node();
        
        // Create arc representing node capacity
        // info = true means this arc represents a node
        typename Aux_Net::Arc* arc = 
          aux_net->insert_arc(src, tgt, p->max_cap, Flow_Type{0}, true);
        
        // Establish bidirectional mapping
        NODE_COOKIE(p) = arc;
        ARC_COOKIE(arc) = p;
      }
      
      // Phase 2: Create arcs corresponding to original edges
      for (Arc_Iterator<Net_Cap_Graph> it(*this); it.has_curr(); it.next_ne())
      {
        Arc* arc = it.get_curr();
        
        // Get the auxiliary arcs for source and target nodes
        typename Aux_Net::Arc* src_arc = 
          static_cast<typename Aux_Net::Arc*>(NODE_COOKIE(this->get_src_node(arc)));
        typename Aux_Net::Arc* tgt_arc = 
          static_cast<typename Aux_Net::Arc*>(NODE_COOKIE(this->get_tgt_node(arc)));
        
        // Connect tgt of src_arc to src of tgt_arc
        // info = false means this arc represents an original edge
        typename Aux_Net::Arc* a = 
          aux_net->insert_arc(aux_net->get_tgt_node(src_arc), 
                              aux_net->get_src_node(tgt_arc),
                              arc->cap, arc->flow, false);
        
        // Establish bidirectional mapping
        ARC_COOKIE(arc) = a;
        ARC_COOKIE(a) = arc;
      }
      
      return aux_net;
    }
    catch (...)
    {
      // Clean up on failure
      if (aux_net != nullptr)
      {
        clear_graph(*aux_net);
        delete aux_net;
        aux_net = nullptr;
      }
      throw;
    }
  }

  /** @brief Update flow values from auxiliary network to this network.
   *
   * After running a maximum flow algorithm on the auxiliary network,
   * call this method to transfer the computed flow values back to
   * the original node-capacitated network.
   *
   * Updates:
   * - Node in_flow and out_flow from node-representing arcs
   * - Arc flow values from edge-representing arcs
   *
   * @throw std::domain_error if auxiliary network has not been computed.
   */
  void update()
  {
    ah_domain_error_if(aux_net == nullptr)
      << "Auxiliary network has not been computed";

    for (typename Aux_Net::Arc_Iterator it(*aux_net); it.has_curr(); it.next_ne())
    {
      typename Aux_Net::Arc* arc = it.get_curr();
      
      if (arc->get_info())  // This arc represents a node
      {
        Node* p = static_cast<Node*>(ARC_COOKIE(arc));
        p->in_flow = arc->flow;
        p->out_flow = arc->flow;
      }
      else  // This arc represents an original edge
      {
        Arc* a = static_cast<Arc*>(ARC_COOKIE(arc));
        a->flow = arc->flow;
      }
    }
  }

  /** @brief Free the auxiliary network.
   *
   * Releases all memory used by the auxiliary network and clears
   * the cookie mappings.
   *
   * @throw std::domain_error if auxiliary network has not been computed.
   */
  void free_aux_net()
  {
    ah_domain_error_if(aux_net == nullptr)
      << "Auxiliary network has not been computed";

    clear_graph(*aux_net);
    delete aux_net;
    aux_net = nullptr;
  }

  /** @brief Get the total flow value of the network.
   *
   * Returns the total flow passing through the network, computed
   * as the sum of flows through all nodes (using their in_flow).
   *
   * @return Total network flow value.
   * @throw std::domain_error if auxiliary network has not been computed.
   *
   * @note update() must be called first to transfer flow values.
   */
  [[nodiscard]] Flow_Type get_total_flow() const
  {
    ah_domain_error_if(aux_net == nullptr)
      << "Auxiliary network has not been computed";

    Flow_Type total = Flow_Type{0};
    for (Node_Iterator<Net_Cap_Graph> it(*this); it.has_curr(); it.next_ne())
    {
      Node* p = it.get_curr();
      total += p->in_flow;
    }
    return total;
  }

  /** @brief Check if all node capacity constraints are satisfied.
   *
   * Verifies that for each node, the flow through it does not
   * exceed its maximum capacity.
   *
   * @return true if all node capacities are respected, false otherwise.
   */
  [[nodiscard]] bool check_node_capacities() const noexcept
  {
    for (Node_Iterator<Net_Cap_Graph> it(*this); it.has_curr(); it.next_ne())
    {
      Node* p = it.get_curr();
      if (p->in_flow > p->max_cap || p->out_flow > p->max_cap)
        return false;
    }
    return true;
  }

  /** @brief Reset all flow values to zero.
   *
   * Resets flows in both arcs and nodes.
   */
  void reset_flows() noexcept
  {
    // Reset arc flows
    for (Arc_Iterator<Net_Cap_Graph> it(*this); it.has_curr(); it.next_ne())
      it.get_curr()->flow = Flow_Type{0};
    
    // Reset node flows  
    for (Node_Iterator<Net_Cap_Graph> it(*this); it.has_curr(); it.next_ne())
    {
      Node* p = it.get_curr();
      p->in_flow = Flow_Type{0};
      p->out_flow = Flow_Type{0};
    }
  }

  /** @brief Get the capacity of a node.
   *
   * @param node Pointer to the node.
   * @return The maximum capacity of the node.
   */
  [[nodiscard]] static Flow_Type get_node_cap(const Node* node) noexcept
  {
    return node->max_cap;
  }

  /** @brief Set the capacity of a node.
   *
   * @param node Pointer to the node.
   * @param cap New capacity value.
   * @throw std::domain_error if cap is negative.
   * @throw std::overflow_error if current flow exceeds new capacity.
   */
  static void set_node_cap(Node* node, const Flow_Type& cap)
  {
    ah_domain_error_if(cap < Flow_Type{0})
      << "Node capacity cannot be negative";
    ah_overflow_error_if(node->in_flow > cap || node->out_flow > cap)
      << "Current flow exceeds new capacity";
    node->max_cap = cap;
  }
};

} // end namespace Aleph

#endif // TPL_NETCAPGRAPH_H
