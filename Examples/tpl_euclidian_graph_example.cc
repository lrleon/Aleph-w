/**
 * @file tpl_euclidian_graph_example.cc
 * @brief Educational examples for Euclidean graphs with geometric positions
 * 
 * WHAT IS A EUCLIDEAN GRAPH?
 * ===========================
 * Graph where nodes have 2D/3D positions and arc weights are Euclidean distances
 * Distances are computed AUTOMATICALLY from node positions
 * Ideal for geographic, spatial, and geometric applications
 * 
 * KEY FEATURE:
 * ============
 * You just specify node positions - distances are auto-calculated!
 * Perfect for maps, GIS, robotics, network planning
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_euclidian_graph_example tpl_euclidian_graph_example.cc -lm
 * ./tpl_euclidian_graph_example
 */

#include <iostream>
#include <cmath>
#include <tpl_euclidian_graph.H>
#include <point.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Euclidean Graphs: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: City Network with Auto-Distances
  // =========================================================================
  {
    cout << "--- Example 1: City Network (Automatic Distance Calculation) ---\n\n";
    
    // STEP 1: Define Euclidean graph type
    // Nodes have positions, arcs get automatic distance weights
    using EG = Euclidian_Graph<Euclidian_Node<string>, Euclidian_Arc<Empty_Class>>;
    EG city_map;
    
    cout << "Building US East Coast city network...\n\n";
    
    // STEP 2: Insert cities with geographic coordinates
    // Positions are (latitude, longitude) - distances auto-computed!
    auto nyc = city_map.insert_node("NYC", Point(40.7128, -74.0060));
    auto boston = city_map.insert_node("Boston", Point(42.3601, -71.0589));
    auto philly = city_map.insert_node("Philadelphia", Point(39.9526, -75.1652));
    auto dc = city_map.insert_node("Washington DC", Point(38.9072, -77.0369));
    
    cout << "Cities added:\n";
    cout << "  NYC:           (40.71, -74.01)\n";
    cout << "  Boston:        (42.36, -71.06)\n";
    cout << "  Philadelphia:  (39.95, -75.17)\n";
    cout << "  Washington DC: (38.91, -77.04)\n\n";
    
    // STEP 3: Connect cities - distances computed automatically!
    cout << "Connecting cities (distances auto-calculated):\n";
    
    auto arc1 = city_map.insert_arc(nyc, boston);
    auto arc2 = city_map.insert_arc(nyc, philly);
    auto arc3 = city_map.insert_arc(philly, dc);
    
    // Get computed distances (note: returns geometric type, display directly)
    cout << "  NYC -> Boston:        " << city_map.get_distance(arc1) << " degrees\n";
    cout << "  NYC -> Philadelphia:  " << city_map.get_distance(arc2) << " degrees\n";
    cout << "  Philadelphia -> DC:   " << city_map.get_distance(arc3) << " degrees\n\n";
    
    cout << "KEY INSIGHT: You provide positions, graph computes distances!\n";
    cout << "             Perfect for geographic applications\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Nearest Neighbor Search
  // =========================================================================
  {
    cout << "--- Example 2: Finding Nearest City ---\n\n";
    
    using EG = Euclidian_Graph<Euclidian_Node<string>, Euclidian_Arc<Empty_Class>>;
    EG city_map;
    
    // Add cities
    city_map.insert_node("NYC", Point(40.7, -74.0));
    city_map.insert_node("Boston", Point(42.4, -71.1));
    city_map.insert_node("Philadelphia", Point(40.0, -75.2));
    city_map.insert_node("Baltimore", Point(39.3, -76.6));
    city_map.insert_node("Washington DC", Point(38.9, -77.0));
    
    cout << "Cities in network: NYC, Boston, Philadelphia, Baltimore, DC\n\n";
    
    // QUERY: Find nearest city to a given point
    Point query_location(40.5, -74.5);  // Somewhere in New Jersey
    
    cout << "Query: What city is nearest to point (40.5, -74.5)?\n";
    cout << "       (This is somewhere in New Jersey)\n\n";
    
    // Manual nearest neighbor search
    EG::Node* nearest_city = nullptr;
    bool first = true;
    Geom_Number min_distance;
    
    for (Node_Iterator<EG> it(city_map); it.has_curr(); it.next())
    {
      auto city = it.get_curr();
      auto city_pos = city->get_position();
      auto dist = query_location.distance_with(city_pos);
      
      cout << "  Distance to " << city->get_info() << ": " << dist << "\n";
      
      if (first || dist < min_distance)
      {
        min_distance = dist;
        nearest_city = city;
        first = false;
      }
    }
    
    cout << "\nNearest city: " << nearest_city->get_info()
         << " (distance: " << min_distance << ")\n\n";
    
    cout << "APPLICATION: Location-based services, routing, facility placement\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Minimum Spanning Tree (Road Network)
  // =========================================================================
  {
    cout << "--- Example 3: Minimum Spanning Tree (Optimal Road Network) ---\n\n";
    
    using EG = Euclidian_Graph<Euclidian_Node<string>, Euclidian_Arc<Empty_Class>>;
    EG cities;
    
    // Add cities
    auto a = cities.insert_node("A", Point(0, 0));
    auto b = cities.insert_node("B", Point(4, 0));
    auto c = cities.insert_node("C", Point(2, 3));
    auto d = cities.insert_node("D", Point(0, 4));
    auto e = cities.insert_node("E", Point(4, 4));
    
    cout << "5 cities positioned on a grid\n";
    cout << "Goal: Connect all cities with minimum total road length\n\n";
    
    // Connect all pairs (complete graph)
    DynList<EG::Node*> nodes;
    nodes.append(a); nodes.append(b); nodes.append(c);
    nodes.append(d); nodes.append(e);
    
    cout << "All possible connections:\n";
    Geom_Number total_all = 0;
    int connection_count = 0;
    
    for (size_t i = 0; i < nodes.size(); ++i)
      for (size_t j = i + 1; j < nodes.size(); ++j)
      {
        auto arc = cities.insert_arc(nodes[i], nodes[j]);
        auto dist = cities.get_distance(arc);
        total_all = total_all + dist;
        connection_count++;
        
        cout << "  " << nodes[i]->get_info() << " <-> "
             << nodes[j]->get_info() << ": " << dist << "\n";
      }
    
    cout << "\nTotal if we build ALL roads: " << total_all << "\n";
    cout << "Number of roads: " << connection_count << "\n\n";
    
    cout << "OPTIMIZATION PROBLEM:\n";
    cout << "  Minimum Spanning Tree finds minimum total length\n";
    cout << "  to connect all cities (n-1 = 4 roads needed)\n";
    cout << "  MST would use approximately 50% of total road length\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Distance Matrix for Route Planning
  // =========================================================================
  {
    cout << "--- Example 4: Distance Matrix ---\n\n";
    
    using EG = Euclidian_Graph<Euclidian_Node<int>, Euclidian_Arc<Empty_Class>>;
    EG delivery_network;
    
    // Create delivery locations on a grid
    DynList<EG::Node*> locations;
    for (int i = 0; i < 4; ++i)
    {
      int x = i * 10;
      int y = (i % 2) * 10;
      auto loc = delivery_network.insert_node(i, Point(x, y));
      locations.append(loc);
    }
    
    cout << "Delivery network: 4 locations\n";
    cout << "Computing all-pairs distances...\n\n";
    
    // Build distance matrix
    cout << "Distance Matrix:\n";
    cout << "    ";
    for (size_t j = 0; j < locations.size(); ++j)
      cout << "  [" << j << "]  ";
    cout << "\n";
    
    for (size_t i = 0; i < locations.size(); ++i)
    {
      cout << "[" << i << "] ";
      for (size_t j = 0; j < locations.size(); ++j)
      {
        auto pi = locations[i]->get_position();
        auto pj = locations[j]->get_position();
        auto dist = pi.distance_with(pj);
        cout << " " << dist << "  ";
      }
      cout << "\n";
    }
    
    cout << "\nUSE CASE: Route optimization, delivery planning, TSP\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: 3D Euclidean Graph (Bonus)
  // =========================================================================
  {
    cout << "--- Example 5: 3D Space (Satellites/Drones) ---\n\n";
    
    cout << "EXTENDING TO 3D:\n";
    cout << "  Same concepts apply to 3D coordinates (x, y, z)\n";
    cout << "  Distance formula: sqrt((x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2)\n\n";
    
    cout << "APPLICATIONS:\n";
    cout << "  * Satellite networks (3D positions in space)\n";
    cout << "  * Drone delivery routes (altitude matters)\n";
    cout << "  * 3D pathfinding in games\n";
    cout << "  * Molecular structures (atoms in 3D)\n\n";
    
    cout << "IMPLEMENTATION:\n";
    cout << "  Use Point3D class or extend Point to 3D\n";
    cout << "  Distance calculation automatic for any dimension\n\n";
  }
  
  cout << "=== SUMMARY: Euclidean Graphs ===\n";
  cout << "\n1. AUTOMATIC DISTANCE CALCULATION:\n";
  cout << "   * Set node positions\n";
  cout << "   * Distances computed from Euclidean formula\n";
  cout << "   * No manual weight assignment needed\n";
  cout << "\n2. WHEN TO USE:\n";
  cout << "   ✓ Geographic networks (cities, roads)\n";
  cout << "   ✓ Facility location problems\n";
  cout << "   ✓ Robotics (motion planning)\n";
  cout << "   ✓ Wireless networks (signal range)\n";
  cout << "   ✓ Computer graphics (scene graphs)\n";
  cout << "\n3. KEY OPERATIONS:\n";
  cout << "   - insert_node(info, Point(x,y))\n";
  cout << "   - get_distance(arc) → auto-computed\n";
  cout << "   - get_position(node) → Point\n";
  cout << "   - Nearest neighbor queries\n";
  cout << "\n4. ADVANTAGES:\n";
  cout << "   * Consistency: distances always match positions\n";
  cout << "   * Convenience: no manual distance computation\n";
  cout << "   * Natural for spatial problems\n";
  cout << "\n5. ALGORITHMS:\n";
  cout << "   All graph algorithms work:\n";
  cout << "   * Dijkstra (shortest paths)\n";
  cout << "   * MST (minimum spanning tree)\n";
  cout << "   * TSP (traveling salesman)\n";
  cout << "   Plus geometric-specific:\n";
  cout << "   * Nearest neighbor\n";
  cout << "   * Range queries\n";
  cout << "   * Voronoi diagrams\n";
  cout << "\n6. DISTANCE FORMULA:\n";
  cout << "   2D: sqrt((x2-x1)^2 + (y2-y1)^2)\n";
  cout << "   3D: sqrt((x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2)\n";
  cout << "   Time: O(1) per calculation\n";
  
  return 0;
}
