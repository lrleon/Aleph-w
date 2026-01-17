/**
 * @file tpl_net_sup_dem_example.cc
 * @brief Educational examples for supply-demand network flow problems
 * 
 * WHAT IS SUPPLY-DEMAND FLOW?
 * ============================
 * Network flow where:
 * - SUPPLY nodes: Produce goods (positive supply value)
 * - DEMAND nodes: Consume goods (negative supply value)
 * - TRANSIT nodes: Neither produce nor consume (zero supply)
 * 
 * GOAL: Route flow from suppliers to consumers respecting capacity constraints
 * 
 * KEY CONCEPTS:
 * =============
 * - Conservation: Flow in = Flow out (except at sources/sinks)
 * - Capacity: Arc flow ≤ arc capacity
 * - Balance: Total supply = Total demand (for feasibility)
 * 
 * REAL-WORLD APPLICATIONS:
 * ========================
 * - Supply chain logistics
 * - Power grid distribution
 * - Water distribution networks
 * - Transportation planning
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_net_sup_dem_example tpl_net_sup_dem_example.cc
 * ./tpl_net_sup_dem_example
 */

#include <iostream>
#include <tpl_net_sup_dem.H>
#include <tpl_net.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Supply-Demand Networks: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Basic Distribution Network
  // =========================================================================
  {
    cout << "--- Example 1: Simple Distribution Problem ---\n\n";
    
    // STEP 1: Define network type
    using NetSD = Net_Sup_Dem_Graph<Net_Sup_Dem_Node<string, int>,
                                     Net_Arc<Empty_Class, int>>;
    NetSD network;
    
    cout << "SCENARIO: Warehouse distribution to stores\n";
    cout << "=========================================\n\n";
    
    // STEP 2: Create supply nodes (warehouses)
    cout << "SUPPLY NODES (Warehouses):\n";
    auto warehouse1 = network.insert_node("Warehouse1", 100);  // supplies 100 units
    auto warehouse2 = network.insert_node("Warehouse2", 150);  // supplies 150 units
    cout << "  Warehouse1: supplies 100 units\n";
    cout << "  Warehouse2: supplies 150 units\n";
    cout << "  Total supply: 250 units\n\n";
    
    // STEP 3: Create demand nodes (stores)
    cout << "DEMAND NODES (Stores):\n";
    auto store1 = network.insert_node("Store1", -80);   // demands 80 units
    auto store2 = network.insert_node("Store2", -100);  // demands 100 units
    auto store3 = network.insert_node("Store3", -70);   // demands 70 units
    cout << "  Store1: demands 80 units\n";
    cout << "  Store2: demands 100 units\n";
    cout << "  Store3: demands 70 units\n";
    cout << "  Total demand: 250 units\n\n";
    
    // STEP 4: Create transit nodes (distribution centers)
    cout << "TRANSIT NODES (Distribution Centers):\n";
    auto hub = network.insert_node("Hub", 0);  // zero supply (just passes flow)
    cout << "  Hub: transit point (no supply/demand)\n\n";
    
    // STEP 5: Connect with capacity constraints
    cout << "NETWORK CONNECTIONS (with capacities):\n";
    network.insert_arc(warehouse1, hub, 100);   // capacity 100
    network.insert_arc(warehouse2, hub, 150);   // capacity 150
    network.insert_arc(hub, store1, 80);        // capacity 80
    network.insert_arc(hub, store2, 100);       // capacity 100
    network.insert_arc(hub, store3, 70);        // capacity 70
    
    cout << "  Warehouse1 -> Hub: capacity 100\n";
    cout << "  Warehouse2 -> Hub: capacity 150\n";
    cout << "  Hub -> Store1: capacity 80\n";
    cout << "  Hub -> Store2: capacity 100\n";
    cout << "  Hub -> Store3: capacity 70\n\n";
    
    // STEP 6: Check balance
    cout << "FEASIBILITY CHECK:\n";
    cout << "  Total supply: 250 units\n";
    cout << "  Total demand: 250 units\n";
    cout << "  Network is BALANCED ✓\n\n";
    
    cout << "KEY INSIGHT: Balanced network (supply = demand) has feasible solution\n";
    cout << "             Flow can satisfy all demands from available supply\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Multi-Source Multi-Sink
  // =========================================================================
  {
    cout << "--- Example 2: Complex Distribution Network ---\n\n";
    
    using NetSD = Net_Sup_Dem_Graph<Net_Sup_Dem_Node<string, int>,
                                     Net_Arc<Empty_Class, int>>;
    NetSD network;
    
    cout << "SCENARIO: Multiple factories → Multiple customers\n";
    cout << "================================================\n\n";
    
    // Multiple suppliers
    auto factory_a = network.insert_node("Factory_A", 200);
    auto factory_b = network.insert_node("Factory_B", 180);
    auto factory_c = network.insert_node("Factory_C", 220);
    
    cout << "Factories (suppliers):\n";
    cout << "  Factory A: 200 units\n";
    cout << "  Factory B: 180 units\n";
    cout << "  Factory C: 220 units\n";
    cout << "  Total: 600 units\n\n";
    
    // Multiple consumers
    auto customer1 = network.insert_node("Customer1", -150);
    auto customer2 = network.insert_node("Customer2", -200);
    auto customer3 = network.insert_node("Customer3", -120);
    auto customer4 = network.insert_node("Customer4", -130);
    
    cout << "Customers (demanders):\n";
    cout << "  Customer 1: 150 units\n";
    cout << "  Customer 2: 200 units\n";
    cout << "  Customer 3: 120 units\n";
    cout << "  Customer 4: 130 units\n";
    cout << "  Total: 600 units\n\n";
    
    // Distribution hubs
    auto hub_north = network.insert_node("Hub_North", 0);
    auto hub_south = network.insert_node("Hub_South", 0);
    
    cout << "Distribution hubs (transit):\n";
    cout << "  Hub North: 0 (transit only)\n";
    cout << "  Hub South: 0 (transit only)\n\n";
    
    // Create connections
    cout << "Network topology:\n";
    cout << "  Factories → Hubs → Customers\n";
    cout << "  Multiple paths for redundancy\n\n";
    
    // Factory to hubs
    network.insert_arc(factory_a, hub_north, 150);
    network.insert_arc(factory_a, hub_south, 100);
    network.insert_arc(factory_b, hub_north, 100);
    network.insert_arc(factory_b, hub_south, 120);
    network.insert_arc(factory_c, hub_north, 120);
    network.insert_arc(factory_c, hub_south, 150);
    
    // Hubs to customers
    network.insert_arc(hub_north, customer1, 150);
    network.insert_arc(hub_north, customer2, 120);
    network.insert_arc(hub_south, customer2, 100);
    network.insert_arc(hub_south, customer3, 120);
    network.insert_arc(hub_south, customer4, 130);
    
    cout << "ADVANTAGES OF MULTI-HUB DESIGN:\n";
    cout << "  ✓ Redundancy (multiple paths)\n";
    cout << "  ✓ Load balancing across hubs\n";
    cout << "  ✓ Geographic optimization\n";
    cout << "  ✓ Fault tolerance\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Unbalanced Network (Excess Supply)
  // =========================================================================
  {
    cout << "--- Example 3: Unbalanced Network ---\n\n";
    
    using NetSD = Net_Sup_Dem_Graph<Net_Sup_Dem_Node<string, int>,
                                     Net_Arc<Empty_Class, int>>;
    NetSD network;
    
    cout << "SCENARIO: Supply > Demand\n";
    cout << "=========================\n\n";
    
    // More supply than demand
    auto supplier = network.insert_node("Supplier", 500);     // supply 500
    auto customer = network.insert_node("Customer", -300);    // demand 300
    
    cout << "Supply: 500 units\n";
    cout << "Demand: 300 units\n";
    cout << "Excess: 200 units\n\n";
    
    network.insert_arc(supplier, customer, 500);
    
    cout << "HANDLING EXCESS SUPPLY:\n";
    cout << "  Option 1: Add dummy demand node (sink excess)\n";
    cout << "  Option 2: Model as upper bound on supply\n";
    cout << "  Option 3: Storage/inventory node\n\n";
    
    // Add dummy sink for excess
    auto storage = network.insert_node("Storage", -200);  // absorb excess
    network.insert_arc(supplier, storage, 200);
    
    cout << "Solution: Added storage node\n";
    cout << "  Supplier -> Customer: 300 units\n";
    cout << "  Supplier -> Storage: 200 units\n";
    cout << "  Network now balanced ✓\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Capacity Constraints Matter
  // =========================================================================
  {
    cout << "--- Example 4: Capacity Bottlenecks ---\n\n";
    
    using NetSD = Net_Sup_Dem_Graph<Net_Sup_Dem_Node<string, int>,
                                     Net_Arc<Empty_Class, int>>;
    NetSD network;
    
    cout << "SCENARIO: Balanced but infeasible due to capacity\n";
    cout << "================================================\n\n";
    
    auto source = network.insert_node("Source", 100);
    auto sink = network.insert_node("Sink", -100);
    auto bottleneck = network.insert_node("Bottleneck", 0);
    
    cout << "Network: Source -> Bottleneck -> Sink\n";
    cout << "  Source supply: 100\n";
    cout << "  Sink demand: 100\n";
    cout << "  Balanced! ✓\n\n";
    
    // BUT: capacity is too small!
    network.insert_arc(source, bottleneck, 100);
    network.insert_arc(bottleneck, sink, 50);  // BOTTLENECK!
    
    cout << "Arc capacities:\n";
    cout << "  Source -> Bottleneck: 100 (OK)\n";
    cout << "  Bottleneck -> Sink: 50 (TOO SMALL!)\n\n";
    
    cout << "PROBLEM: Capacity bottleneck!\n";
    cout << "  Can only send 50 units through bottleneck\n";
    cout << "  Cannot satisfy demand of 100\n";
    cout << "  Network is INFEASIBLE ✗\n\n";
    
    cout << "LESSON: Balanced supply/demand ≠ feasible solution\n";
    cout << "        Must also check capacity constraints!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Real-World Power Grid
  // =========================================================================
  {
    cout << "--- Example 5: Power Grid Distribution ---\n\n";
    
    cout << "REAL-WORLD APPLICATION: Electric Power Grid\n";
    cout << "==========================================\n\n";
    
    cout << "NODES:\n";
    cout << "  * Power plants (supply nodes)\n";
    cout << "    - Coal plant: 500 MW\n";
    cout << "    - Solar farm: 200 MW\n";
    cout << "    - Wind farm: 150 MW\n";
    cout << "\n";
    cout << "  * Cities (demand nodes)\n";
    cout << "    - City A: -300 MW\n";
    cout << "    - City B: -250 MW\n";
    cout << "    - City C: -200 MW\n";
    cout << "\n";
    cout << "  * Substations (transit nodes)\n";
    cout << "    - Balance load\n";
    cout << "    - Transform voltage\n";
    cout << "    - No generation or consumption\n\n";
    
    cout << "ARCS (Transmission Lines):\n";
    cout << "  * Capacity = power line rating (MW)\n";
    cout << "  * Cost = transmission loss/distance\n";
    cout << "  * Redundancy for reliability\n\n";
    
    cout << "OPTIMIZATION GOALS:\n";
    cout << "  1. Meet all demand (feasibility)\n";
    cout << "  2. Minimize transmission losses (cost)\n";
    cout << "  3. Respect line capacities (constraints)\n";
    cout << "  4. Load balancing across plants\n\n";
    
    cout << "ALGORITHMS:\n";
    cout << "  * Check feasibility: Max-flow algorithm\n";
    cout << "  * Minimize cost: Min-cost flow\n";
    cout << "  * Handle failures: Network simplex\n\n";
  }
  
  cout << "=== SUMMARY: Supply-Demand Networks ===\n";
  cout << "\n1. NODE TYPES:\n";
  cout << "   Supply (source):  positive value (produces)\n";
  cout << "   Demand (sink):    negative value (consumes)\n";
  cout << "   Transit:          zero value (passes flow)\n";
  cout << "\n2. FEASIBILITY CONDITIONS:\n";
  cout << "   ✓ Total supply = Total demand (balance)\n";
  cout << "   ✓ Capacities allow required flows\n";
  cout << "   ✓ Network is connected\n";
  cout << "\n3. COMMON PATTERNS:\n";
  cout << "   * Hub-and-spoke: Central distribution\n";
  cout << "   * Multi-tier: Factory→Hub→Store\n";
  cout << "   * Redundant paths: Fault tolerance\n";
  cout << "\n4. REAL-WORLD APPLICATIONS:\n";
  cout << "   * Supply chain logistics\n";
  cout << "   * Power grid distribution\n";
  cout << "   * Water/gas networks\n";
  cout << "   * Transportation planning\n";
  cout << "   * Telecommunications\n";
  cout << "\n5. KEY ALGORITHMS:\n";
  cout << "   Feasibility: Max-flow (source to sink)\n";
  cout << "   Optimal: Min-cost flow\n";
  cout << "   Complexity: Polynomial time\n";
  cout << "\n6. DESIGN PRINCIPLES:\n";
  cout << "   * Balance supply and demand\n";
  cout << "   * Size capacities appropriately\n";
  cout << "   * Add redundancy for reliability\n";
  cout << "   * Use transit nodes for routing flexibility\n";
  
  return 0;
}
