/**
 * @file tpl_multicommodity_example.cc
 * @brief Educational examples for multi-commodity flow networks
 * 
 * WHAT IS MULTI-COMMODITY FLOW?
 * ==============================
 * Extension of max-flow where MULTIPLE types of flow (commodities)
 * share the SAME network infrastructure
 * 
 * Each commodity has:
 * - Its own source and sink nodes
 * - Its own demand/supply
 * - But shares arc capacities with other commodities!
 * 
 * KEY CHALLENGE:
 * ==============
 * Multiple commodities compete for LIMITED capacity
 * Must route all commodities without exceeding capacities
 * NP-hard in general, requires optimization algorithms
 * 
 * REAL-WORLD EXAMPLES:
 * ====================
 * - Telecommunications: Multiple calls share same network
 * - Transportation: Different goods share roads/railways
 * - Data centers: Multiple data streams share bandwidth
 * - Supply chains: Multiple products through warehouses
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_multicommodity_example tpl_multicommodity_example.cc
 * ./tpl_multicommodity_example
 */

#include <iostream>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Multi-Commodity Flow: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Two-Commodity Problem
  // =========================================================================
  {
    cout << "--- Example 1: Two Data Streams Sharing Network ---\n\n";
    
    cout << "SCENARIO: Data center with two applications\n";
    cout << "=========================================\n\n";
    
    cout << "COMMODITY 1: Video streaming\n";
    cout << "  Source: Server_A\n";
    cout << "  Sink:   CDN_1\n";
    cout << "  Demand: 100 Mbps\n\n";
    
    cout << "COMMODITY 2: Database backup\n";
    cout << "  Source: Server_B\n";
    cout << "  Sink:   CDN_2\n";
    cout << "  Demand: 80 Mbps\n\n";
    
    cout << "SHARED NETWORK:\n";
    cout << "  Server_A ----\n";
    cout << "               \\\n";
    cout << "                Router (100 Mbps capacity)\n";
    cout << "               /\n";
    cout << "  Server_B ----\n";
    cout << "               \\\n";
    cout << "                Switch (100 Mbps capacity)\n";
    cout << "               /     \\\n";
    cout << "         CDN_1         CDN_2\n\n";
    
    cout << "CONSTRAINT: Router has only 100 Mbps total capacity\n";
    cout << "  Commodity 1 wants: 100 Mbps\n";
    cout << "  Commodity 2 wants: 80 Mbps\n";
    cout << "  Total demand: 180 Mbps\n";
    cout << "  Available: 100 Mbps\n";
    cout << "  CONFLICT! Cannot satisfy both fully\n\n";
    
    cout << "POSSIBLE SOLUTIONS:\n";
    cout << "  1. Priority: Give commodity 1 full 100 Mbps, commodity 2 gets 0\n";
    cout << "  2. Fair share: Split 50/50 → 50 Mbps each\n";
    cout << "  3. Weighted: 60% to commodity 1, 40% to commodity 2\n";
    cout << "  4. Time-sharing: Alternate between commodities\n\n";
    
    cout << "KEY INSIGHT: Must balance competing demands\n";
    cout << "             Unlike single-commodity where solution is clear\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Feasible vs Infeasible
  // =========================================================================
  {
    cout << "--- Example 2: Feasibility Check ---\n\n";
    
    cout << "FEASIBLE CASE:\n";
    cout << "==============\n\n";
    
    cout << "Network:\n";
    cout << "  S1 --100--> Hub --150--> T1\n";
    cout << "               |\n";
    cout << "  S2 --80----->+--120--> T2\n\n";
    
    cout << "Commodity 1: S1 → T1, demand 60\n";
    cout << "Commodity 2: S2 → T2, demand 70\n\n";
    
    cout << "Check capacities:\n";
    cout << "  S1 → Hub: Need 60, have 100 ✓\n";
    cout << "  S2 → Hub: Need 70, have 80 ✓\n";
    cout << "  Hub → T1: Need 60, have 150 ✓\n";
    cout << "  Hub → T2: Need 70, have 120 ✓\n\n";
    
    cout << "RESULT: FEASIBLE ✓\n";
    cout << "  Both commodities can be routed\n\n";
    
    cout << "INFEASIBLE CASE:\n";
    cout << "================\n\n";
    
    cout << "Same network, different demands:\n";
    cout << "Commodity 1: S1 → T1, demand 90\n";
    cout << "Commodity 2: S2 → T2, demand 80\n\n";
    
    cout << "Check capacities:\n";
    cout << "  S1 → Hub: Need 90, have 100 ✓\n";
    cout << "  S2 → Hub: Need 80, have 80 ✓\n";
    cout << "  Hub outgoing: Need 90+80=170, have 150+120=270 ✓\n";
    cout << "  BUT: Hub is single node with limited throughput!\n\n";
    
    cout << "RESULT: May be INFEASIBLE\n";
    cout << "  Depends on node capacity constraints\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Routing Alternatives
  // =========================================================================
  {
    cout << "--- Example 3: Multiple Paths ---\n\n";
    
    cout << "ADVANTAGE: Multiple commodities can use DIFFERENT paths\n";
    cout << "======================================================\n\n";
    
    cout << "Network:\n";
    cout << "         Path 1 (direct)\n";
    cout << "  S1 ========100========> T1\n";
    cout << "    \\                    /\n";
    cout << "     \\                  /\n";
    cout << "  50  \\    Hub (relay)  / 50\n";
    cout << "       \\      |        /\n";
    cout << "        \\     |50     /\n";
    cout << "         \\    |      /\n";
    cout << "  S2 ====\\===+=====/ T2\n";
    cout << "      80  \\       /  80\n";
    cout << "           Path 2\n\n";
    
    cout << "Commodity 1: S1 → T1, demand 60\n";
    cout << "Commodity 2: S2 → T2, demand 70\n\n";
    
    cout << "SMART ROUTING:\n";
    cout << "  Commodity 1: Use direct path (S1 → T1)\n";
    cout << "    Flow: 60 through 100-capacity link ✓\n\n";
    
    cout << "  Commodity 2: Use relay path (S2 → Hub → T2)\n";
    cout << "    Flow: 70 split between two 50-capacity links\n";
    cout << "    Send 50 through S2→Hub, then Hub→T2\n";
    cout << "    Send 20 through direct S2→T2\n\n";
    
    cout << "KEY STRATEGY: Route different commodities on different paths\n";
    cout << "             Maximize utilization of all network resources\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Cost Optimization
  // =========================================================================
  {
    cout << "--- Example 4: Minimizing Cost ---\n\n";
    
    cout << "PROBLEM: Not just feasibility, but OPTIMAL routing\n";
    cout << "=================================================\n\n";
    
    cout << "Each arc has:\n";
    cout << "  * Capacity: Maximum flow\n";
    cout << "  * Cost per unit: Price to send 1 unit\n\n";
    
    cout << "EXAMPLE:\n";
    cout << "  Path A: Capacity 100, Cost $2/unit\n";
    cout << "  Path B: Capacity 50,  Cost $1/unit (cheaper but limited)\n\n";
    
    cout << "STRATEGY:\n";
    cout << "  1. Fill cheap path B first (50 units at $1 = $50)\n";
    cout << "  2. Use expensive path A for overflow (50 units at $2 = $100)\n";
    cout << "  3. Total: 100 units for $150\n\n";
    
    cout << "NAIVE APPROACH:\n";
    cout << "  Use only path A: 100 units at $2 = $200\n";
    cout << "  Wastes $50!\n\n";
    
    cout << "OPTIMIZATION GOAL:\n";
    cout << "  Minimize: Sum of (flow * cost) across all arcs\n";
    cout << "  Subject to:\n";
    cout << "    - Flow conservation\n";
    cout << "    - Capacity constraints\n";
    cout << "    - Commodity demands satisfied\n\n";
    
    cout << "ALGORITHM: Linear programming or min-cost flow\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Real-World Telecommunications
  // =========================================================================
  {
    cout << "--- Example 5: Telephone Network ---\n\n";
    
    cout << "REAL-WORLD: Telephone calls sharing network\n";
    cout << "=========================================\n\n";
    
    cout << "COMMODITIES: Individual phone calls\n";
    cout << "  * Call 1: NYC → LA\n";
    cout << "  * Call 2: Boston → SF\n";
    cout << "  * Call 3: NYC → Chicago\n";
    cout << "  * ... (thousands of simultaneous calls)\n\n";
    
    cout << "SHARED RESOURCES:\n";
    cout << "  * Fiber optic cables (limited bandwidth)\n";
    cout << "  * Routing switches (limited capacity)\n";
    cout << "  * Cross-country trunk lines\n\n";
    
    cout << "CHALLENGES:\n";
    cout << "  1. DYNAMIC: Calls start/end continuously\n";
    cout << "     Must reroute in real-time\n\n";
    
    cout << "  2. QUALITY: Each call needs minimum bandwidth\n";
    cout << "     Cannot just 'share' bandwidth arbitrarily\n\n";
    
    cout << "  3. RELIABILITY: Calls shouldn't drop\n";
    cout << "     Need backup routes if link fails\n\n";
    
    cout << "  4. COST: Long-distance links are expensive\n";
    cout << "     Minimize total routing cost\n\n";
    
    cout << "SOLUTION APPROACH:\n";
    cout << "  * Online algorithms: Handle calls as they arrive\n";
    cout << "  * Load balancing: Distribute across network\n";
    cout << "  * Admission control: Reject if no capacity\n";
    cout << "  * Dynamic rerouting: Adapt to failures\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 6: Supply Chain with Multiple Products
  // =========================================================================
  {
    cout << "--- Example 6: Multi-Product Distribution ---\n\n";
    
    cout << "SCENARIO: Logistics company\n";
    cout << "==========================\n\n";
    
    cout << "PRODUCTS (commodities):\n";
    cout << "  * Electronics: Factory_A → Store_X (50 tons)\n";
    cout << "  * Furniture:   Factory_B → Store_Y (80 tons)\n";
    cout << "  * Food:        Factory_C → Store_Z (30 tons)\n\n";
    
    cout << "SHARED WAREHOUSES:\n";
    cout << "  * Hub_1: Capacity 100 tons (receives from all factories)\n";
    cout << "  * Hub_2: Capacity 80 tons  (distributes to all stores)\n\n";
    
    cout << "TRUCK FLEET: Limited capacity\n";
    cout << "  * Route A: 70 tons/day\n";
    cout << "  * Route B: 50 tons/day\n";
    cout << "  * Route C: 60 tons/day\n\n";
    
    cout << "OPTIMIZATION:\n";
    cout << "  1. Which products use which routes?\n";
    cout << "  2. How to mix products on trucks?\n";
    cout << "  3. Minimize delivery time or cost?\n\n";
    
    cout << "CONSTRAINT: Different products CAN share trucks\n";
    cout << "            But total weight cannot exceed capacity\n\n";
  }
  
  cout << "=== SUMMARY: Multi-Commodity Flow ===\n";
  cout << "\n1. DEFINITION:\n";
  cout << "   Multiple flow types (commodities) sharing same network\n";
  cout << "   Each commodity has own source/sink/demand\n";
  cout << "   Arc capacities are SHARED across commodities\n";
  cout << "\n2. KEY DIFFERENCE vs SINGLE-COMMODITY:\n";
  cout << "   Single: One source-sink pair, clear optimal solution\n";
  cout << "   Multi:  Many source-sink pairs, must balance conflicts\n";
  cout << "\n3. COMPLEXITY:\n";
  cout << "   Single-commodity: Polynomial time (max-flow)\n";
  cout << "   Multi-commodity:  NP-hard in general\n";
  cout << "                     Requires LP or approximation\n";
  cout << "\n4. PROBLEM VARIANTS:\n";
  cout << "   * Feasibility: Can all demands be met?\n";
  cout << "   * Min-cost: Minimize total routing cost\n";
  cout << "   * Max-throughput: Maximize total flow\n";
  cout << "   * Fair allocation: Balance between commodities\n";
  cout << "\n5. REAL-WORLD APPLICATIONS:\n";
  cout << "   ✓ Telecommunications (calls, data streams)\n";
  cout << "   ✓ Transportation (goods, passengers)\n";
  cout << "   ✓ Supply chains (multiple products)\n";
  cout << "   ✓ Computer networks (packet routing)\n";
  cout << "   ✓ Power grids (multiple generators/consumers)\n";
  cout << "\n6. SOLUTION APPROACHES:\n";
  cout << "   * Linear Programming (exact, polynomial for fixed k)\n";
  cout << "   * Approximation algorithms (fast, near-optimal)\n";
  cout << "   * Heuristics (practical, no guarantees)\n";
  cout << "   * Column generation (for large instances)\n";
  cout << "\n7. KEY TRADE-OFFS:\n";
  cout << "   Complexity vs Optimality\n";
  cout << "   Speed vs Solution Quality\n";
  cout << "   Fairness vs Efficiency\n";
  cout << "   Static vs Dynamic routing\n";
  
  return 0;
}
