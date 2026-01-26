
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


/**
 * @file quadtree_example.C
 * @brief QuadTree: Spatial Data Structure for 2D Points
 * 
 * This example demonstrates the QuadTree, a hierarchical spatial data
 * structure that recursively divides 2D space into four quadrants. QuadTrees
 * are essential for efficient spatial queries and are widely used in computer
 * graphics, GIS, game development, and computational geometry.
 *
 * ## What is a QuadTree?
 *
 * A QuadTree is a tree data structure where:
 * - **Root**: Represents entire 2D space (bounding box)
 * - **Internal nodes**: Have exactly 4 children (quadrants)
 * - **Leaf nodes**: Contain points or are empty
 * - **Subdivision**: Space is recursively divided into 4 equal quadrants
 *
 * ### Structure
 *
 * Each internal node divides space into four quadrants:
 * - **NW (Northwest)**: Top-left quadrant
 * - **NE (Northeast)**: Top-right quadrant
 * - **SW (Southwest)**: Bottom-left quadrant
 * - **SE (Southeast)**: Bottom-right quadrant
 *
 * **Example**: Dividing a 100×100 space:
 * ```
 * Root (0,0 to 100,100)
 * ├── NW (0,0 to 50,50)
 * ├── NE (50,0 to 100,50)
 * ├── SW (0,50 to 50,100)
 * └── SE (50,50 to 100,100)
 * ```
 *
 * ## How It Works
 *
 * ### Insertion
 * 1. Start at root
 * 2. Determine which quadrant point belongs to
 * 3. If quadrant is empty, insert point there
 * 4. If quadrant has points and exceeds capacity, subdivide
 * 5. Recursively insert into appropriate sub-quadrant
 *
 * ### Search
 * 1. Start at root
 * 2. Determine which quadrant(s) intersect query region
 * 3. Recursively search relevant quadrants
 * 4. Return all points in query region
 *
 * ### Range Query
 * - Query: "Find all points in rectangle [x1,y1] to [x2,y2]"
 * - Efficiently prunes irrelevant quadrants
 * - Only searches quadrants that intersect query region
 *
 * ## Complexity Analysis
 *
 * | Operation | Average | Worst Case | Notes |
 * |-----------|---------|------------|-------|
 * | Insert | O(log n) | O(depth) | Depends on distribution |
 * | Search (point) | O(log n) | O(depth) | Point lookup |
 * | Range query | O(log n + k) | O(n) | k = points in range |
 * | Leaf neighbors (QuadNode) | O(log n) | O(depth) | Neighbor navigation between leaf regions |
 * | Remove | O(log n) | O(depth) | May require merging |
 *
 * **Note**: Worst case occurs when points are poorly distributed
 * (e.g., all on a line), causing deep trees.
 *
 * ## Applications
 *
 * ### Geographic Information Systems (GIS)
 * - **Spatial indexing**: Efficiently store and query geographic data
 * - **Map rendering**: Quickly find features in viewport
 * - **Location services**: Find nearby points of interest
 * - **Spatial joins**: Efficiently join spatial datasets
 *
 * ### Game Development
 * - **Collision detection**: Quickly find objects in collision range
 * - **Visibility culling**: Determine what's visible in viewport
 * - **Spatial partitioning**: Organize game world efficiently
 * - **AI pathfinding**: Spatial awareness for NPCs
 *
 * ### Computer Graphics
 * - **Image compression**: Quadtree-based image representation
 * - **Ray tracing**: Accelerate ray-object intersection tests
 * - **Level of detail (LOD)**: Adaptive detail based on distance
 * - **Terrain rendering**: Efficient terrain data structures
 *
 * ### Computational Geometry
 * - **Nearest neighbor**: Find closest point efficiently
 * - **Range searches**: Find points in rectangular region
 * - **Spatial clustering**: Group nearby points
 * - **Voronoi diagrams**: Spatial partitioning
 *
 * ### Scientific Simulations
 * - **N-body simulations**: Barnes-Hut algorithm uses quadtree
 * - **Particle systems**: Efficient neighbor finding
 * - **Fluid dynamics**: Spatial organization of particles
 * - **Molecular dynamics**: Efficient force calculations
 *
 * ## Advantages
 *
 * ✅ **Efficient queries**: O(log n) average case
 * ✅ **Adaptive**: Automatically adjusts to data distribution
 * ✅ **Memory efficient**: Only subdivides where needed
 * ✅ **Simple concept**: Easy to understand and implement
 *
 * ## Disadvantages
 *
 * ❌ **Worst case**: Can degrade to O(n) with bad distribution
 * ❌ **2D only**: Designed for 2D space (use octree for 3D)
 * ❌ **Rebalancing**: May need periodic rebalancing
 *
 * ## Comparison with Other Spatial Structures
 *
 * | Structure | Dimensions | Best For | Complexity |
 * |-----------|-----------|----------|------------|
 * | QuadTree | 2D | Uniform/adaptive | O(log n) avg |
 * | Octree | 3D | 3D spatial data | O(log n) avg |
 * | KD-Tree | k-D | High-dimensional | O(log n) avg |
 * | R-Tree | k-D | Rectangles | O(log n) avg |
 * | Grid | 2D | Uniform distribution | O(1) |
 *
 * ## Usage Example
 *
 * ```cpp
 * QuadTree qt(0, 100, 0, 100, 4);
 *
 * qt.insert(Point(10, 20));
 * qt.insert(Point(30, 40));
 *
 * if (Point *p = qt.search(Point(10, 20)))
 *   std::cout << "Found: " << *p << "\n";
 *
 * qt.remove(Point(10, 20));
 *
 * // Traverse all nodes (inspect leaf nodes and their stored points)
 * qt.for_each([](QuadNode *node) {
 *   if (node->is_leaf())
 *     std::cout << "Leaf with " << node->get_points_set().size() << " points\n";
 * });
 * ```
 *
 * @see quadtree.H QuadTree implementation
 * @see quadnode.H QuadTree node structure
 * @see geom_example.C Other geometric algorithms
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <quadtree.H>
#include <tclap/CmdLine.h>

using namespace std;

/**
 * @brief Print a point
 */
ostream& operator<<(ostream& os, const Point& p)
{
  return os << "(" << p.get_x() << ", " << p.get_y() << ")";
}

/**
 * @brief Demonstrate basic QuadTree operations
 */
void demo_basic_operations()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "QuadTree: Basic Operations" << endl;
  cout << string(60, '=') << endl;
  
  // Create quadtree for region [0, 100] x [0, 100]
  // with max 4 points per node before splitting
  QuadTree tree(0, 100, 0, 100, 4);
  
  cout << "\nCreated QuadTree for region [0, 100] x [0, 100]" << endl;
  cout << "Max points per node before split: 4" << endl;
  
  cout << "\n--- Insertion ---" << endl;
  
  vector<Point> points = {
    Point(25, 25), Point(75, 25), Point(25, 75), Point(75, 75),
    Point(50, 50), Point(10, 10), Point(90, 90), Point(30, 70)
  };
  
  cout << "Inserting points:" << endl;
  for (const auto& p : points)
  {
    tree.insert(p);
    cout << "  Inserted " << p << endl;
  }
  
  cout << "\n--- Search ---" << endl;
  
  vector<Point> to_search = {
    Point(25, 25), Point(50, 50), Point(99, 99), Point(0, 0)
  };
  
  for (const auto& p : to_search)
  {
    Point* found = tree.search(p);
    cout << "  Search " << p << ": " 
         << (found ? "FOUND" : "not found") << endl;
  }
  
  cout << "\n--- Contains ---" << endl;
  cout << "  Contains (25, 25): " << (tree.contains(Point(25, 25)) ? "yes" : "no") << endl;
  cout << "  Contains (99, 99): " << (tree.contains(Point(99, 99)) ? "yes" : "no") << endl;
  
  cout << "\n--- Removal ---" << endl;
  
  cout << "  Removing (50, 50)..." << endl;
  tree.remove(Point(50, 50));
  cout << "  Contains (50, 50) after removal: " 
       << (tree.contains(Point(50, 50)) ? "yes" : "no") << endl;
}

/**
 * @brief Demonstrate tree structure
 */
void demo_tree_structure()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "QuadTree Structure and Subdivision" << endl;
  cout << string(60, '=') << endl;
  
  QuadTree tree(0, 100, 0, 100, 2);  // Max 2 points per node
  
  cout << "\nCreated QuadTree with max 2 points per node" << endl;
  cout << "This will force subdivision earlier." << endl;
  
  cout << "\n--- Step-by-step insertion ---" << endl;
  
  // Insert first two points (no split yet)
  tree.insert(Point(25, 25));
  cout << "After inserting (25, 25): root has 1 point" << endl;
  
  tree.insert(Point(75, 25));
  cout << "After inserting (75, 25): root has 2 points" << endl;
  
  // Third point triggers split
  cout << "\nInserting (25, 75) - will trigger subdivision..." << endl;
  tree.insert(Point(25, 75));
  cout << "Root split into 4 quadrants:" << endl;
  cout << "  NW: [0, 50] x [0, 50]   - contains (25, 25)" << endl;
  cout << "  NE: [50, 100] x [0, 50] - contains (75, 25)" << endl;
  cout << "  SW: [0, 50] x [50, 100] - contains (25, 75)" << endl;
  cout << "  SE: [50, 100] x [50, 100] - empty" << endl;
  
  cout << "\n--- Visualization ---" << endl;
  cout << "+----------+----------+" << endl;
  cout << "|    *     |     *    |" << endl;
  cout << "|  (25,25) | (75,25)  |" << endl;
  cout << "+----------+----------+" << endl;
  cout << "|    *     |          |" << endl;
  cout << "|  (25,75) |  (empty) |" << endl;
  cout << "+----------+----------+" << endl;
  cout << "NW=*, NE=*, SW=*, SE=empty" << endl;
}

/**
 * @brief Practical example: Geographic Points
 */
void demo_geographic_points()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Geographic Points (Cities)" << endl;
  cout << string(60, '=') << endl;
  
  // Simplified coordinate system (not real lat/lon)
  QuadTree tree(0, 1000, 0, 1000, 5);
  
  // City locations (simplified)
  struct City { string name; double x, y; };
  vector<City> cities = {
    {"New York", 800, 400},
    {"Los Angeles", 100, 300},
    {"Chicago", 600, 420},
    {"Houston", 400, 200},
    {"Phoenix", 200, 250},
    {"Philadelphia", 780, 410},
    {"San Antonio", 350, 180},
    {"San Diego", 80, 280},
    {"Dallas", 420, 220},
    {"San Jose", 60, 370}
  };
  
  cout << "\nInserting " << cities.size() << " cities..." << endl;
  for (const auto& city : cities)
  {
    tree.insert(Point(city.x, city.y));
    cout << "  " << city.name << " at (" << city.x << ", " << city.y << ")" << endl;
  }
  
  cout << "\n--- Spatial Queries ---" << endl;
  
  // Check if point exists
  Point nyc(800, 400);
  cout << "\nLooking for a city at (800, 400)..." << endl;
  if (tree.contains(nyc))
    cout << "  Found! (This is New York)" << endl;
  else
    cout << "  Not found" << endl;
  
  // Search for nearby city
  Point unknown(795, 408);
  cout << "\nSearching for exact point (795, 408)..." << endl;
  Point* result = tree.search(unknown);
  cout << "  Result: " << (result ? "found" : "not found (needs exact match)") << endl;
}

/**
 * @brief Practical example: Game Collision Detection
 */
void demo_collision_detection()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Game Collision Detection" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nScenario: 2D game with objects in a 800x600 screen" << endl;
  
  QuadTree tree(0, 800, 0, 600, 4);
  
  // Game objects (enemies, power-ups, etc.)
  vector<Point> objects = {
    Point(100, 100), Point(150, 120),   // Cluster 1
    Point(700, 500), Point(720, 480),   // Cluster 2
    Point(400, 300), Point(420, 310),   // Center
    Point(50, 550), Point(750, 50)      // Corners
  };
  
  cout << "\nPlacing " << objects.size() << " game objects..." << endl;
  for (const auto& obj : objects)
    tree.insert(obj);
  
  cout << "\n--- Collision Query ---" << endl;
  
  Point player(410, 305);
  cout << "Player at " << player << endl;
  
  cout << "\nChecking for collision with exact position..." << endl;
  if (tree.contains(player))
    cout << "  COLLISION! Object at player position." << endl;
  else
    cout << "  No collision at exact position." << endl;
  
  // Check nearby positions
  cout << "\nChecking nearby positions for objects:" << endl;
  vector<Point> nearby = {
    Point(400, 300), Point(420, 310), Point(405, 308)
  };
  for (const auto& pos : nearby)
  {
    bool collision = tree.contains(pos);
    cout << "  " << pos << ": " << (collision ? "COLLISION" : "clear") << endl;
  }
  
  cout << "\n--- Benefits of QuadTree for Games ---" << endl;
  cout << "1. Only check objects in player's quadrant region" << endl;
  cout << "2. O(log n) average lookup instead of O(n)" << endl;
  cout << "3. Scales well with large numbers of objects" << endl;
}

/**
 * @brief Performance demonstration
 */
void demo_performance(int n)
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Performance Analysis (n = " << n << ")" << endl;
  cout << string(60, '=') << endl;
  
  QuadTree tree(0, 10000, 0, 10000, 10);
  
  // Generate random points
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<> dis(0, 10000);
  
  vector<Point> points;
  points.reserve(n);
  for (int i = 0; i < n; ++i)
    points.emplace_back(dis(gen), dis(gen));
  
  cout << "\nGenerated " << n << " random points" << endl;
  
  // Insertion benchmark
  auto start = chrono::high_resolution_clock::now();
  
  for (const auto& p : points)
    tree.insert(p);
  
  auto mid = chrono::high_resolution_clock::now();
  
  // Search benchmark
  int found = 0;
  for (const auto& p : points)
    if (tree.contains(p)) ++found;
  
  auto end = chrono::high_resolution_clock::now();
  
  auto insert_us = chrono::duration_cast<chrono::microseconds>(mid - start).count();
  auto search_us = chrono::duration_cast<chrono::microseconds>(end - mid).count();
  
  cout << "\nResults:" << endl;
  cout << "  Insert " << n << " points: " << insert_us << " us" << endl;
  cout << "  Search " << n << " points: " << search_us << " us" << endl;
  cout << "  Found: " << found << "/" << n << " points" << endl;
  cout << "  Avg insert: " << (insert_us * 1000.0 / n) << " ns/point" << endl;
  cout << "  Avg search: " << (search_us * 1000.0 / n) << " ns/point" << endl;
  
  // Comparison note
  cout << "\n--- Comparison Note ---" << endl;
  cout << "Linear search would require O(" << n << ") comparisons per query" << endl;
  cout << "QuadTree reduces this to O(log n) ~= O(" << log2(n) << ") on average" << endl;
}

/**
 * @brief Demonstrate node traversal
 */
void demo_traversal()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Tree Traversal" << endl;
  cout << string(60, '=') << endl;
  
  QuadTree tree(0, 100, 0, 100, 2);
  
  // Insert points to create structure
  vector<Point> points = {
    Point(25, 25), Point(75, 25), Point(25, 75), Point(75, 75),
    Point(12, 12), Point(37, 37)
  };
  
  cout << "\nBuilding tree with " << points.size() << " points..." << endl;
  for (const auto& p : points)
    tree.insert(p);
  
  cout << "\n--- Traversing all nodes ---" << endl;
  
  int node_count = 0;
  int leaf_count = 0;
  int point_count = 0;
  
  tree.for_each([&](QuadNode* node) {
    ++node_count;
    if (node->is_leaf())
    {
      ++leaf_count;
      point_count += node->get_points_set().size();
    }
    
    string type = node->is_leaf() ? "LEAF" : "INTERNAL";
    size_t points_in_node = node->is_leaf() ? node->get_points_set().size() : 0;
    
    cout << "  Node (level " << LEVEL(node) << ", " << type;
    if (node->is_leaf())
      cout << ", " << points_in_node << " points";
    cout << ")" << endl;
  });
  
  cout << "\nTree statistics:" << endl;
  cout << "  Total nodes: " << node_count << endl;
  cout << "  Leaf nodes: " << leaf_count << endl;
  cout << "  Total points: " << point_count << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("QuadTree Spatial Data Structure Example", ' ', "1.0");
    
    TCLAP::ValueArg<int> nArg("n", "count",
      "Number of points for performance test", false, 10000, "int");
    TCLAP::SwitchArg basicArg("b", "basic",
      "Show basic operations", false);
    TCLAP::SwitchArg structArg("s", "structure",
      "Show tree structure", false);
    TCLAP::SwitchArg geoArg("g", "geographic",
      "Show geographic points example", false);
    TCLAP::SwitchArg gameArg("c", "collision",
      "Show collision detection example", false);
    TCLAP::SwitchArg perfArg("p", "performance",
      "Run performance analysis", false);
    TCLAP::SwitchArg travArg("t", "traversal",
      "Show tree traversal", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(nArg);
    cmd.add(basicArg);
    cmd.add(structArg);
    cmd.add(geoArg);
    cmd.add(gameArg);
    cmd.add(perfArg);
    cmd.add(travArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    int n = nArg.getValue();
    bool runBasic = basicArg.getValue();
    bool runStruct = structArg.getValue();
    bool runGeo = geoArg.getValue();
    bool runGame = gameArg.getValue();
    bool runPerf = perfArg.getValue();
    bool runTrav = travArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runBasic and not runStruct and not runGeo and 
        not runGame and not runPerf and not runTrav)
      runAll = true;
    
    cout << "=== QuadTree: 2D Spatial Data Structure ===" << endl;
    cout << "Hierarchical space partitioning into quadrants" << endl;
    
    if (runAll or runBasic)
      demo_basic_operations();
    
    if (runAll or runStruct)
      demo_tree_structure();
    
    if (runAll or runGeo)
      demo_geographic_points();
    
    if (runAll or runGame)
      demo_collision_detection();
    
    if (runAll or runTrav)
      demo_traversal();
    
    if (runAll or runPerf)
      demo_performance(n);
    
    cout << "\n=== Summary ===" << endl;
    cout << "QuadTree: Efficient 2D spatial indexing" << endl;
    cout << "Operations: O(log n) average, O(depth) worst" << endl;
    cout << "Use cases: GIS, games, graphics, simulations" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

