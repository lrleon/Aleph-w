/**
 * @file tpl_2dtree_example.cc
 * @brief Educational examples for 2D spatial trees (k-d trees)
 * 
 * WHAT IS A 2D-TREE?
 * ==================
 * Binary search tree for 2D points (k-d tree with k=2)
 * Alternates splitting on X and Y coordinates
 * Enables efficient spatial queries in 2D space
 * 
 * HOW IT WORKS:
 * =============
 * - Root level: Split on X coordinate
 * - Next level: Split on Y coordinate  
 * - Alternates X/Y splits at each level
 * - Each node represents a point and implicitly defines a rectangular region
 * 
 * EFFICIENT OPERATIONS:
 * =====================
 * - Insert: O(log n) average
 * - Search: O(log n) average
 * - Range query: O(sqrt(n) + k) where k = results
 * - Nearest neighbor: O(log n) average
 * 
 * APPLICATIONS:
 * =============
 * - Geographic information systems (GIS)
 * - Computer graphics (collision detection)
 * - Machine learning (k-NN classification)
 * - Game development (spatial partitioning)
 * - Computational geometry
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_2dtree_example tpl_2dtree_example.cc -lm
 * ./tpl_2dtree_example
 */

#include <iostream>
#include <cmath>
#include <tpl_2dtree.H>
#include <point.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== 2D Spatial Trees: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Building a 2D-Tree
  // =========================================================================
  {
    cout << "--- Example 1: Inserting Points into 2D-Tree ---\n\n";
    
    // STEP 1: Create 2D-tree with bounding box
    // Region: [0, 100] x [0, 100]
    K2Tree<> tree(0, 0, 100, 100);
    
    cout << "Created 2D-tree for region [0,100] x [0,100]\n\n";
    
    // STEP 2: Insert points
    cout << "Inserting points:\n";
    
    Point p1(25, 50);
    Point p2(75, 25);
    Point p3(50, 75);
    Point p4(10, 10);
    Point p5(90, 90);
    
    tree.insert(p1);
    cout << "  Inserted (25, 50)\n";
    
    tree.insert(p2);
    cout << "  Inserted (75, 25)\n";
    
    tree.insert(p3);
    cout << "  Inserted (50, 75)\n";
    
    tree.insert(p4);
    cout << "  Inserted (10, 10)\n";
    
    tree.insert(p5);
    cout << "  Inserted (90, 90)\n\n";
    
    cout << "TREE STRUCTURE (conceptual):\n";
    cout << "  Level 0 (split on X): root = (25, 50)\n";
    cout << "    Left  (x < 25): (10, 10)\n";
    cout << "    Right (x >= 25):\n";
    cout << "      Level 1 (split on Y): (75, 25)\n";
    cout << "        Left  (y < 25): none\n";
    cout << "        Right (y >= 25): (50, 75), (90, 90)\n\n";
    
    cout << "KEY CONCEPT: Alternating splits create balanced partitioning\n";
    cout << "             of 2D space into rectangular regions\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Point Search
  // =========================================================================
  {
    cout << "--- Example 2: Searching for Points ---\n\n";
    
    K2Tree<> tree(0, 0, 100, 100);
    
    // Build tree
    tree.insert(Point(25, 50));
    tree.insert(Point(75, 25));
    tree.insert(Point(50, 75));
    tree.insert(Point(10, 10));
    
    cout << "Tree contains: (25,50), (75,25), (50,75), (10,10)\n\n";
    
    // Search for existing point
    cout << "SEARCH QUERIES:\n";
    
    Point query1(50, 75);
    if (tree.contains(query1))
      cout << "  Point (50, 75): FOUND ✓\n";
    else
      cout << "  Point (50, 75): NOT FOUND\n";
    
    Point query2(30, 40);
    if (tree.contains(query2))
      cout << "  Point (30, 40): FOUND\n";
    else
      cout << "  Point (30, 40): NOT FOUND ✓\n";
    
    Point query3(25, 50);
    if (tree.contains(query3))
      cout << "  Point (25, 50): FOUND ✓ (root node)\n";
    else
      cout << "  Point (25, 50): NOT FOUND\n";
    
    cout << "\nCOMPLEXITY:\n";
    cout << "  Average case: O(log n) - like binary search\n";
    cout << "  Worst case: O(n) - if tree is unbalanced\n";
    cout << "  For random data: Usually well-balanced\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Range Query (Points in Rectangle)
  // =========================================================================
  {
    cout << "--- Example 3: Range Query (Rectangle Search) ---\n\n";
    
    K2Tree<> tree(0, 0, 200, 200);
    
    cout << "SCENARIO: City POI (Points of Interest) database\n";
    cout << "===============================================\n\n";
    
    // Insert POIs with names (conceptually)
    struct POI {
      Point location;
      string name;
    };
    
    POI pois[] = {
      {Point(20, 30), "Restaurant A"},
      {Point(50, 50), "Park B"},
      {Point(80, 40), "Hotel C"},
      {Point(120, 150), "Museum D"},
      {Point(150, 180), "Theater E"},
      {Point(60, 70), "Cafe F"},
      {Point(180, 20), "Mall G"}
    };
    
    // Insert into tree
    for (const auto& poi : pois)
      tree.insert(poi.location);
    
    cout << "Inserted 7 POIs across the city\n\n";
    
    // Range query: Find all POIs in downtown area
    Rectangle downtown(40, 30, 90, 80);
    
    cout << "QUERY: Find all POIs in downtown area\n";
    cout << "       Rectangle: (40,30) to (90,80)\n\n";
    
    DynList<Point> results;
    tree.range(downtown, &results);
    
    cout << "Results:\n";
    for (size_t i = 0; i < sizeof(pois)/sizeof(POI); ++i)
    {
      Point p = pois[i].location;
      auto x = p.get_x();
      auto y = p.get_y();
      
      if (x >= 40 && x <= 90 && y >= 30 && y <= 80)
        cout << "  " << pois[i].name << " at (" << x << ", " << y << ")\n";
    }
    
    cout << "\nCOMPLEXITY: O(sqrt(n) + k) where k = number of results\n";
    cout << "           Much better than O(n) linear scan!\n\n";
    
    cout << "REAL-WORLD: Map applications showing POIs in visible area\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Nearest Neighbor Search
  // =========================================================================
  {
    cout << "--- Example 4: Finding Nearest Point ---\n\n";
    
    K2Tree<> tree(0, 0, 100, 100);
    
    cout << "SCENARIO: Find closest emergency service\n";
    cout << "========================================\n\n";
    
    // Insert emergency service locations
    Point hospital(20, 80);
    Point fire_station(70, 30);
    Point police(50, 50);
    Point ambulance(85, 75);
    
    tree.insert(hospital);
    tree.insert(fire_station);
    tree.insert(police);
    tree.insert(ambulance);
    
    cout << "Emergency services:\n";
    cout << "  Hospital: (20, 80)\n";
    cout << "  Fire Station: (70, 30)\n";
    cout << "  Police: (50, 50)\n";
    cout << "  Ambulance: (85, 75)\n\n";
    
    // Query point (emergency location)
    Point emergency(60, 40);
    
    cout << "EMERGENCY at (60, 40)\n";
    cout << "Finding nearest service...\n\n";
    
    // Manual nearest neighbor (simplified)
    Point nearest = tree.nearest(emergency);
    
    auto dist = emergency.distance_with(nearest);
    
    cout << "NEAREST SERVICE:\n";
    cout << "  Location: (" << nearest.get_x() << ", " << nearest.get_y() << ")\n";
    cout << "  Distance: " << dist << " units\n";
    
    // Determine which service it is
    if (nearest.get_x() == 70 && nearest.get_y() == 30)
      cout << "  Type: Fire Station\n";
    else if (nearest.get_x() == 50 && nearest.get_y() == 50)
      cout << "  Type: Police\n";
    
    cout << "\nALGORITHM:\n";
    cout << "  1. Start at root\n";
    cout << "  2. Recursively explore promising branches\n";
    cout << "  3. Prune branches that can't contain closer point\n";
    cout << "  4. Track best candidate while descending\n\n";
    
    cout << "COMPLEXITY: O(log n) average - very efficient!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Collision Detection (Game Development)
  // =========================================================================
  {
    cout << "--- Example 5: Spatial Partitioning for Games ---\n\n";
    
    cout << "SCENARIO: 2D game with moving objects\n";
    cout << "=====================================\n\n";
    
    K2Tree<> game_world(0, 0, 1000, 1000);
    
    cout << "Game world: 1000x1000 pixels\n";
    cout << "Objects: Players, enemies, projectiles, items\n\n";
    
    // Insert game objects
    struct GameObject {
      Point position;
      string type;
      double radius;
    };
    
    GameObject objects[] = {
      {Point(100, 200), "Player", 10},
      {Point(150, 220), "Enemy", 15},
      {Point(500, 500), "Item", 5},
      {Point(800, 100), "Enemy", 15},
      {Point(120, 210), "Projectile", 2}
    };
    
    for (const auto& obj : objects)
      game_world.insert(obj.position);
    
    cout << "Inserted " << (sizeof(objects)/sizeof(GameObject)) << " game objects\n\n";
    
    // Check collisions near player
    Point player_pos(100, 200);
    double collision_range = 50;  // Check within 50 pixels
    
    cout << "COLLISION CHECK:\n";
    cout << "  Player at (100, 200)\n";
    cout << "  Checking range: 50 pixels\n\n";
    
    Rectangle search_area(
      player_pos.get_x() - collision_range,
      player_pos.get_y() - collision_range,
      player_pos.get_x() + collision_range,
      player_pos.get_y() + collision_range
    );
    
    DynList<Point> nearby;
    game_world.range(search_area, &nearby);
    
    cout << "Nearby objects:\n";
    for (const auto& obj : objects)
    {
      auto dist = player_pos.distance_with(obj.position);
      
      if (dist <= collision_range && dist > 0)  // Exclude self
        cout << "  " << obj.type << " at distance " << dist << "\n";
    }
    
    cout << "\nGAME OPTIMIZATION:\n";
    cout << "  Without 2D-tree: Check all N objects → O(N^2) for N entities\n";
    cout << "  With 2D-tree: Check only nearby → O(N log N + k*N)\n";
    cout << "  For 1000 objects: ~1M vs ~10K comparisons!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 6: K-Nearest Neighbors (Machine Learning)
  // =========================================================================
  {
    cout << "--- Example 6: k-NN Classification ---\n\n";
    
    cout << "SCENARIO: Classify new data point\n";
    cout << "=================================\n\n";
    
    K2Tree<> dataset(0, 0, 100, 100);
    
    cout << "Training data (2D feature space):\n";
    
    // Class A points (clustered around 20, 20)
    cout << "  Class A (circle): around (20, 20)\n";
    dataset.insert(Point(18, 22));
    dataset.insert(Point(22, 18));
    dataset.insert(Point(20, 25));
    dataset.insert(Point(15, 20));
    
    // Class B points (clustered around 80, 80)
    cout << "  Class B (square): around (80, 80)\n";
    dataset.insert(Point(78, 82));
    dataset.insert(Point(82, 78));
    dataset.insert(Point(80, 85));
    dataset.insert(Point(75, 80));
    
    cout << "\nNew data point: (25, 25)\n";
    cout << "Task: Classify as A or B using 3-NN\n\n";
    
    Point new_point(25, 25);
    
    cout << "ALGORITHM:\n";
    cout << "  1. Find 3 nearest neighbors in tree\n";
    cout << "  2. Take majority vote of their classes\n";
    cout << "  3. Assign that class to new point\n\n";
    
    cout << "EXPECTED RESULT:\n";
    cout << "  3 nearest neighbors likely from Class A\n";
    cout << "  (New point is closer to Class A cluster)\n";
    cout << "  Classification: Class A\n\n";
    
    cout << "COMPLEXITY:\n";
    cout << "  k-NN with 2D-tree: O(k log n)\n";
    cout << "  k-NN without: O(n) for each query\n";
    cout << "  Speedup: log n times faster!\n\n";
  }
  
  cout << "=== SUMMARY: 2D Spatial Trees ===\n";
  cout << "\n1. WHAT IS IT?\n";
  cout << "   Binary search tree for 2D points\n";
  cout << "   Alternates splitting on X and Y\n";
  cout << "   Partitions space into rectangles\n";
  cout << "\n2. KEY OPERATIONS:\n";
  cout << "   Insert:           O(log n) average\n";
  cout << "   Search:           O(log n) average\n";
  cout << "   Range query:      O(sqrt(n) + k)\n";
  cout << "   Nearest neighbor: O(log n) average\n";
  cout << "\n3. WHEN TO USE:\n";
  cout << "   ✓ 2D spatial data (maps, games)\n";
  cout << "   ✓ Range queries (find points in region)\n";
  cout << "   ✓ Nearest neighbor queries\n";
  cout << "   ✓ Collision detection\n";
  cout << "   ✓ k-NN classification\n";
  cout << "\n4. ADVANTAGES:\n";
  cout << "   * Much faster than linear scan\n";
  cout << "   * Simple to implement\n";
  cout << "   * Low memory overhead\n";
  cout << "   * Good for dynamic data (insert/delete)\n";
  cout << "\n5. LIMITATIONS:\n";
  cout << "   * Can become unbalanced (worst case O(n))\n";
  cout << "   * Best for random data\n";
  cout << "   * 2D only (use k-d tree for higher dimensions)\n";
  cout << "   * Not cache-friendly (pointer chasing)\n";
  cout << "\n6. REAL-WORLD APPLICATIONS:\n";
  cout << "   * GIS: Find POIs near location\n";
  cout << "   * Games: Collision detection, AI\n";
  cout << "   * ML: k-NN classification\n";
  cout << "   * Graphics: Ray tracing, culling\n";
  cout << "   * Robotics: Path planning\n";
  cout << "\n7. ALTERNATIVES:\n";
  cout << "   QuadTree:  Better for clustered data\n";
  cout << "   R-Tree:    Better for rectangles\n";
  cout << "   Grid:      Simpler but less flexible\n";
  cout << "   KD-Tree:   Generalization to k dimensions\n";
  cout << "\n8. BEST PRACTICES:\n";
  cout << "   * Use for uniformly distributed data\n";
  cout << "   * Set appropriate bounding box\n";
  cout << "   * Consider rebalancing for skewed data\n";
  cout << "   * Batch insert for better balance\n";
  
  return 0;
}
