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

/** @file tpl_multicommodity.H
 *  @brief Multi-commodity flow algorithms.
 *
 *  This file provides data structures and algorithms for solving
 *  multi-commodity flow problems on capacitated networks.
 *
 *  ## Overview
 *
 *  In a multi-commodity flow problem, multiple commodities (types of flow)
 *  share the same network infrastructure. Each commodity has its own
 *  source-sink pair and demand, but all commodities compete for the
 *  shared arc capacities.
 *
 *  ## Problem Formulation
 *
 *  Given:
 *  - Network G = (V, E) with arc capacities u_ij
 *  - K commodities, each with source s_k, sink t_k, demand d_k, cost c_ij^k
 *
 *  Minimize: Σ_k Σ_{(i,j)} c_ij^k * x_ij^k
 *
 *  Subject to:
 *  - Flow conservation: Σ_j x_ij^k - Σ_j x_ji^k = b_i^k  ∀i,k
 *  - Capacity sharing: Σ_k x_ij^k ≤ u_ij  ∀(i,j)
 *  - Non-negativity: x_ij^k ≥ 0  ∀(i,j),k
 *
 *  Where b_i^k = d_k if i=s_k, -d_k if i=t_k, 0 otherwise.
 *
 *  ## Algorithms
 *
 *  - **LP Direct**: Formulate as linear program and solve with Simplex
 *  - **Path-based**: Generate paths and solve restricted LP (future)
 *
 *  ## Complexity
 *
 *  The LP formulation has O(K*E) variables and O(K*V + E) constraints.
 *  Solution time depends on the LP solver used.
 *
 *  @par Example: Two-commodity flow network
 *  @code
 *  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
 *  GT network;
 *  
 *  auto s1 = network.insert_node("Source1");
 *  auto s2 = network.insert_node("Source2");
 *  auto t1 = network.insert_node("Sink1");
 *  auto t2 = network.insert_node("Sink2");
 *  auto hub = network.insert_node("Hub");
 *  
 *  // Shared infrastructure with capacity constraints
 *  network.insert_arc(s1, hub, 100);  // capacity 100
 *  network.insert_arc(s2, hub, 80);
 *  network.insert_arc(hub, t1, 90);
 *  network.insert_arc(hub, t2, 70);
 *  
 *  // Commodity 1: s1 -> t1, demand 60
 *  // Commodity 2: s2 -> t2, demand 50
 *  // Both share hub's capacity
 *  @endcode
 *
 *  @par Example: Telecommunications network
 *  @code
 *  // Multiple data streams sharing network links
 *  // Each commodity = different data stream with source/dest
 *  // Arcs = network links with bandwidth capacity
 *  // Goal: maximize total throughput while respecting capacities
 *  @endcode
 *
 *  @ingroup Networks
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_MULTICOMMODITY_H
# define TPL_MULTICOMMODITY_H

# include <vector>
# include <limits>
# include <chrono>
# include <tpl_agraph.H>
# include <tpl_dynMapTree.H>
# include <Simplex.H>
# include <ah-errors.H>

namespace Aleph
{

  /** @brief Commodity definition for multi-commodity flow.
   *
   *  Represents a single commodity with its source, sink, demand,
   *  and per-arc costs.
   *
   *  @tparam Node Node type of the network.
   *  @tparam Flow_Type Numeric type for flow values.
   *
   *  @ingroup Networks
   */
  template <typename Node, typename Flow_Type = double>
  struct Commodity
  {
    size_t id;           ///< Unique commodity identifier
    Node *source;        ///< Source node for this commodity
    Node *sink;          ///< Sink node for this commodity
    Flow_Type demand;    ///< Required flow amount
    std::string name;    ///< Optional descriptive name

    /// Default constructor
    Commodity() : id(0), source(nullptr), sink(nullptr), demand(0) {}

    /// Full constructor
    Commodity(size_t _id, Node *_src, Node *_sink, Flow_Type _demand,
              const std::string& _name = "")
      : id(_id), source(_src), sink(_sink), demand(_demand), name(_name) {}
  };


  /** @brief Arc information for multi-commodity flow.
   *
   *  Extends basic arc with per-commodity flow tracking and costs.
   *
   *  @tparam Arc_Info Base arc information type.
   *  @tparam Flow_Type Numeric type for flow values.
   *
   *  @ingroup Networks
   */
  template <typename Arc_Info = Empty_Class, typename FT = double>
  struct MCF_Arc : public Graph_Aarc<Arc_Info>
  {
    using Base = Graph_Aarc<Arc_Info>;
    using Flow_Type = FT;  ///< Type for flow values

    Flow_Type capacity = 0;                    ///< Arc capacity (shared)
    Flow_Type base_cost = 0;                   ///< Default cost per unit flow
    std::vector<Flow_Type> commodity_flow;     ///< Flow per commodity
    std::vector<Flow_Type> commodity_cost;     ///< Cost per commodity (optional)

    /// Default constructor
    MCF_Arc() = default;

    /// Constructor from Arc_Info (required by graph framework)
    MCF_Arc(const Arc_Info&) : MCF_Arc() {}

    /// Constructor from Arc_Info rvalue (required by graph framework)
    MCF_Arc(Arc_Info&&) : MCF_Arc() {}

    /// Constructor with capacity and cost
    MCF_Arc(Flow_Type cap, Flow_Type cost = 0)
      : capacity(cap), base_cost(cost) {}

    /// Total flow on this arc (sum of all commodities)
    Flow_Type total_flow() const
    {
      Flow_Type sum = 0;
      for (auto f : commodity_flow)
        sum += f;
      return sum;
    }

    /// Residual capacity
    Flow_Type residual() const { return capacity - total_flow(); }

    /// Get flow for commodity k
    Flow_Type flow(size_t k) const
    {
      return k < commodity_flow.size() ? commodity_flow[k] : Flow_Type{0};
    }

    /// Get cost for commodity k
    Flow_Type cost(size_t k) const
    {
      if (k < commodity_cost.size() and commodity_cost[k] != 0)
        return commodity_cost[k];
      return base_cost;
    }

    /// Set flow for commodity k
    void set_flow(size_t k, Flow_Type f)
    {
      if (k >= commodity_flow.size())
        commodity_flow.resize(k + 1, Flow_Type{0});
      commodity_flow[k] = f;
    }

    /// Set cost for commodity k
    void set_cost(size_t k, Flow_Type c)
    {
      if (k >= commodity_cost.size())
        commodity_cost.resize(k + 1, Flow_Type{0});
      commodity_cost[k] = c;
    }

    /// Initialize for K commodities
    void init_commodities(size_t K)
    {
      commodity_flow.assign(K, Flow_Type{0});
      commodity_cost.assign(K, Flow_Type{0});
    }
  };


  /** @brief Multi-commodity flow network (directed graph).
   *
   *  A directed network that supports multiple commodities sharing arc capacities.
   *  Each commodity has its own source-sink pair and demand.
   *
   *  This class inherits from Array_Digraph to ensure proper directed graph
   *  behavior, which is essential for flow conservation constraints.
   *
   *  @tparam NodeT Node type.
   *  @tparam ArcT Arc type (should be MCF_Arc or compatible).
   *
   *  @ingroup Networks
   */
  template <class NodeT = Graph_Anode<Empty_Class>,
            class ArcT = MCF_Arc<Empty_Class, double>>
  class MCF_Graph : public Array_Digraph<NodeT, ArcT>
  {
  public:
    using Base = Array_Digraph<NodeT, ArcT>;
    using Node = NodeT;
    using Arc = ArcT;
    using Flow_Type = typename Arc::Flow_Type;
    using CommodityType = Commodity<Node, Flow_Type>;

  private:
    std::vector<CommodityType> commodities;
    DynMapTree<Node*, size_t> node_to_idx;

  public:
    /// Default constructor
    MCF_Graph() = default;

    /// Get number of commodities
    size_t num_commodities() const { return commodities.size(); }

    /// Get commodity by index
    const CommodityType& get_commodity(size_t k) const
    {
      ah_out_of_range_error_if(k >= commodities.size())
        << "Commodity index " << k << " out of range";
      return commodities[k];
    }

    /// Get all commodities
    const std::vector<CommodityType>& get_commodities() const
    {
      return commodities;
    }

    /** @brief Add a commodity to the network.
     *
     *  @param[in] source Source node.
     *  @param[in] sink Sink node.
     *  @param[in] demand Required flow amount.
     *  @param[in] name Optional name.
     *  @return Commodity index.
     */
    size_t add_commodity(Node *source, Node *sink, Flow_Type demand,
                         const std::string& name = "")
    {
      size_t id = commodities.size();
      commodities.emplace_back(id, source, sink, demand, name);

      // Initialize commodity flow in all arcs
      for (typename Base::Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        it.get_curr()->init_commodities(id + 1);

      return id;
    }

    /** @brief Insert arc with capacity and cost.
     *
     *  Overload of insert_arc that sets capacity and cost for flow networks.
     *
     *  @param[in] src Source node.
     *  @param[in] tgt Target node.
     *  @param[in] capacity Arc capacity.
     *  @param[in] cost Cost per unit flow.
     *  @return Pointer to created arc.
     */
    Arc* insert_arc(Node *src, Node *tgt, Flow_Type capacity, Flow_Type cost = 0)
    {
      auto arc = Base::insert_arc(src, tgt);
      arc->capacity = capacity;
      arc->base_cost = cost;
      arc->init_commodities(commodities.size());
      return arc;
    }

    /// Build node index mapping (call after all nodes inserted)
    void build_node_index()
    {
      node_to_idx.empty();
      size_t idx = 0;
      for (typename Base::Node_Iterator it(*this); it.has_curr(); it.next_ne())
        node_to_idx.insert(it.get_curr(), idx++);
    }

    /// Get node index
    size_t get_node_index(Node *p) const
    {
      return node_to_idx.find(p);
    }

    /// Total flow cost across all commodities
    Flow_Type total_cost() const
    {
      Flow_Type sum = 0;
      for (typename Base::Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          for (size_t k = 0; k < commodities.size(); ++k)
            sum += arc->flow(k) * arc->cost(k);
        }
      return sum;
    }

    /// Check if all demands are satisfied
    bool demands_satisfied() const
    {
      for (const auto& comm : commodities)
        {
          Flow_Type out_flow = 0;
          for (Out_Iterator<MCF_Graph> it(comm.source); it.has_curr(); it.next_ne())
            out_flow += it.get_curr()->flow(comm.id);

          Flow_Type in_flow = 0;
          for (In_Iterator<MCF_Graph> it(comm.source); it.has_curr(); it.next_ne())
            in_flow += it.get_curr()->flow(comm.id);

          if (std::abs((out_flow - in_flow) - comm.demand) > 1e-6)
            return false;
        }
      return true;
    }

    /// Check capacity constraints
    bool capacities_respected() const
    {
      for (typename Base::Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          if (arc->total_flow() > arc->capacity + 1e-6)
            return false;
        }
      return true;
    }

    /// Print network summary
    void print_summary() const
    {
      std::cout << "=== Multi-commodity Network ===\n"
                << "Nodes: " << this->vsize() << "\n"
                << "Arcs: " << this->esize() << "\n"
                << "Commodities: " << commodities.size() << "\n";

      for (const auto& c : commodities)
        std::cout << "  [" << c.id << "] " << c.name
                  << ": demand=" << c.demand << "\n";

      std::cout << "Total cost: " << total_cost() << "\n";
    }
  };


  /** @brief Multi-commodity flow result.
   *
   *  Contains the solution of a multi-commodity flow problem.
   *
   *  @tparam Flow_Type Numeric type for flow values.
   *
   *  @ingroup Networks
   */
  template <typename Flow_Type = double>
  struct MCF_Result
  {
    enum Status { Optimal, Infeasible, Unbounded, Error };

    Status status = Error;
    Flow_Type total_cost = 0;
    std::vector<Flow_Type> commodity_costs;  ///< Cost per commodity
    double solve_time_ms = 0;
    size_t iterations = 0;

    bool is_optimal() const { return status == Optimal; }
  };


  /** @brief Multi-commodity flow solver using LP formulation.
   *
   *  Solves the MCF problem by formulating it as a linear program
   *  and using the Revised Simplex algorithm.
   *
   *  ## LP Formulation
   *
   *  Variables: x_{ij}^k for each arc (i,j) and commodity k
   *  Total: K * |E| variables
   *
   *  Constraints:
   *  1. Flow conservation at each node for each commodity: K * |V|
   *  2. Capacity constraints on each arc: |E|
   *
   *  Total constraints: K * |V| + |E|
   *
   *  @tparam Net Network type (should be MCF_Graph or compatible).
   *
   *  @see MCF_Graph RevisedSimplex
   *  @ingroup Networks
   */
  template <class Net>
  class MCF_LP_Solver
  {
  public:
    using Node = typename Net::Node;
    using Arc = typename Net::Arc;
    using Flow_Type = typename Net::Flow_Type;
    using Result = MCF_Result<Flow_Type>;

  private:
    Net& net;
    DynMapTree<Arc*, size_t> arc_to_idx;

    // Get variable index for commodity k on arc a
    size_t var_index(size_t k, size_t arc_idx) const
    {
      return k * net.esize() + arc_idx;
    }

  public:
    /** @brief Construct solver for a network.
     *
     *  @param[in] network The multi-commodity network to solve.
     */
    explicit MCF_LP_Solver(Net& network) : net(network)
    {
      // Build arc index
      size_t idx = 0;
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        arc_to_idx.insert(it.get_curr(), idx++);
    }

    /** @brief Solve the multi-commodity flow problem.
     *
     *  Formulates and solves the LP using the standard Simplex algorithm.
     *  The problem is formulated with equality constraints for flow
     *  conservation and inequality constraints for capacity.
     *
     *  @return Result structure with status and cost.
     */
    Result solve()
    {
      Result result;
      auto start_time = std::chrono::high_resolution_clock::now();

      const size_t K = net.num_commodities();
      const size_t E = net.esize();

      if (K == 0 || E == 0)
        {
          result.status = Result::Optimal;
          result.total_cost = 0;
          return result;
        }

      // Build node index
      net.build_node_index();

      // LP dimensions
      const size_t num_vars = K * E;  // x_{ij}^k for each arc and commodity

      // Create LP solver for minimization
      Simplex<Flow_Type> lp(num_vars);
      lp.set_minimize();

      // Set objective: minimize Σ c_ij^k * x_ij^k
      size_t var_idx = 0;
      for (size_t k = 0; k < K; ++k)
        {
          for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne(), ++var_idx)
            {
              auto arc = it.get_curr();
              lp.put_objetive_function_coef(var_idx, arc->cost(k));
            }
        }

      // Build incoming arcs map: for each node, list of arcs entering it
      // (In Array_Digraph, nodes only store outgoing arcs, so we build this manually)
      DynMapTree<Node*, DynList<Arc*>> incoming_arcs;
      for (typename Net::Arc_Iterator ait(net); ait.has_curr(); ait.next_ne())
        {
          auto arc = ait.get_curr();
          auto tgt = static_cast<Node*>(arc->tgt_node);
          incoming_arcs[tgt].append(arc);
        }

      // Flow conservation constraints: Σ_j x_ij^k - Σ_j x_ji^k = b_i^k
      // Use equality constraints
      // Row format: n coefficients + RHS at the end
      // NOTE: We skip the sink node because the constraints are linearly
      // dependent (sum of all = 0). Only N-1 constraints are needed.
      size_t eq_count = 0;
      for (size_t k = 0; k < K; ++k)
        {
          const auto& comm = net.get_commodity(k);

          for (typename Net::Node_Iterator nit(net); nit.has_curr(); nit.next_ne())
            {
              auto node = nit.get_curr();

              // Skip sink node - constraint is redundant
              if (node == comm.sink)
                continue;

              // Determine b_i^k
              Flow_Type b = 0;
              if (node == comm.source)
                b = comm.demand;
              // else: intermediate node, b = 0

              // Build constraint row: [coefficients, RHS]
              std::vector<Flow_Type> row(num_vars + 1, Flow_Type{0});

              // Outgoing arcs (+1 coefficient) - use Node_Arc_Iterator
              for (typename Net::Node_Arc_Iterator ait(node); ait.has_curr(); ait.next_ne())
                {
                  auto arc = ait.get_curr();
                  size_t a_idx = arc_to_idx.find(arc);
                  row[var_index(k, a_idx)] = Flow_Type{1};
                }

              // Incoming arcs (-1 coefficient) - use our pre-built map
              if (incoming_arcs.has(node))
                {
                  auto& in_list = incoming_arcs[node];
                  for (auto it = in_list.get_it(); it.has_curr(); it.next_ne())
                    {
                      auto arc = it.get_curr();
                      size_t a_idx = arc_to_idx.find(arc);
                      row[var_index(k, a_idx)] = Flow_Type{-1};
                    }
                }

              // RHS at the end
              row[num_vars] = b;

              // Add equality constraint (now works correctly with mixed LE constraints)
              lp.put_eq_restriction(row.data());
              ++eq_count;
            }
        }

      // Capacity constraints: Σ_k x_ij^k ≤ u_ij
      for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          size_t a_idx = arc_to_idx.find(arc);

          // Row format: [coefficients, RHS]
          std::vector<Flow_Type> row(num_vars + 1, Flow_Type{0});
          for (size_t k = 0; k < K; ++k)
            row[var_index(k, a_idx)] = Flow_Type{1};

          row[num_vars] = arc->capacity;  // RHS

          lp.put_restriction(row.data());
        }

      // Prepare and solve LP
      lp.prepare_linear_program();
      auto lp_state = lp.solve();

      if (lp_state != Simplex<Flow_Type>::Solved)
        {
          result.status = (lp_state == Simplex<Flow_Type>::Unbounded)
            ? Result::Unbounded : Result::Infeasible;
          return result;
        }

      // Load solution
      lp.load_solution();

      // Extract solution
      result.status = Result::Optimal;
      result.commodity_costs.resize(K, Flow_Type{0});

      var_idx = 0;
      for (size_t k = 0; k < K; ++k)
        {
          for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne(), ++var_idx)
            {
              auto arc = it.get_curr();
              Flow_Type flow = lp.get_solution(var_idx);
              if (flow < 0) flow = 0;  // Numerical cleanup

              arc->set_flow(k, flow);
              result.commodity_costs[k] += flow * arc->cost(k);
            }
        }

      result.total_cost = lp.objetive_value();

      auto end_time = std::chrono::high_resolution_clock::now();
      result.solve_time_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();
      result.iterations = lp.get_stats().iterations;

      return result;
    }

    /** @brief Print LP formulation (for debugging).
     *
     *  @param[in] max_constraints Maximum constraints to print.
     */
    void print_formulation(size_t max_constraints = 10) const
    {
      const size_t K = net.num_commodities();
      const size_t E = net.esize();
      const size_t V = net.vsize();

      std::cout << "=== MCF LP Formulation ===\n"
                << "Variables: " << (K * E) << " (K=" << K << " × E=" << E << ")\n"
                << "Constraints: " << (K * V + E)
                << " (flow: " << (K * V) << ", capacity: " << E << ")\n\n";

      std::cout << "Objective: minimize Σ c_ij^k * x_ij^k\n\n";

      std::cout << "Commodities:\n";
      for (size_t k = 0; k < K; ++k)
        {
          const auto& c = net.get_commodity(k);
          std::cout << "  k=" << k << ": demand=" << c.demand << "\n";
        }
    }
  };


  /** @brief Solve multi-commodity flow problem.
   *
   *  Convenience function to solve MCF on a network.
   *
   *  @tparam Net Network type.
   *  @param[in,out] net Network with commodities defined.
   *  @return Result with status and cost.
   *
   *  @see MCF_LP_Solver
   *  @ingroup Networks
   */
  template <class Net>
  MCF_Result<typename Net::Flow_Type> solve_multicommodity_flow(Net& net)
  {
    MCF_LP_Solver<Net> solver(net);
    return solver.solve();
  }

} // end namespace Aleph

# endif // TPL_MULTICOMMODITY_H
