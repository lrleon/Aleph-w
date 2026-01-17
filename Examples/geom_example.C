
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/**
 * @file geom_example.C
 * @brief Comprehensive example demonstrating Aleph-w's computational geometry
 *
 * This example showcases fundamental geometric algorithms from `geom_algorithms.H`,
 * demonstrating how to solve common problems in computational geometry. The
 * examples use Colombian geographical locations and landmarks for real-world
 * context.
 *
 * ## Computational Geometry Overview
 *
 * Computational geometry deals with algorithms for solving geometric problems,
 * such as finding convex hulls, triangulating polygons, and computing distances.
 * These algorithms are essential in computer graphics, GIS, robotics, and more.
 *
 * ## Algorithms Demonstrated
 *
 * ### 1. Polygon Triangulation
 *
 * **Problem**: Decompose a simple polygon into triangles.
 *
 * **Algorithm**: Ear-Cutting (CuttingEarsTriangulation)
 * - An "ear" is a triangle formed by three consecutive vertices
 * - Repeatedly "cut off" ears until polygon is fully triangulated
 * - **Time**: O(n²) worst case, O(n) for convex polygons
 * - **Space**: O(n)
 *
 * **Applications**:
 * - Computer graphics: Rendering polygons
 * - Mesh generation: Finite element analysis
 * - Game development: Collision detection
 * - GIS: Polygon decomposition
 *
 * ### 2. Convex Hull Algorithms
 *
 * **Problem**: Find the smallest convex polygon containing all points.
 *
 * Three algorithms are demonstrated:
 *
 * #### BruteForceConvexHull
 * - Check all possible edges
 * - **Time**: O(n³)
 * - **Use**: Educational, small datasets
 *
 * #### GiftWrappingConvexHull (Jarvis March)
 * - Start with leftmost point, "wrap" around points
 * - **Time**: O(nh) where h = number of hull points
 * - **Use**: Output-sensitive, good when h is small
 *
 * #### QuickHull
 * - Divide and conquer approach
 * - **Time**: O(n log n) average, O(n²) worst case
 * - **Use**: General purpose, most practical
 *
 * **Applications**:
 * - Collision detection: Bounding shapes
 * - Pattern recognition: Shape analysis
 * - Computer vision: Object detection
 * - Robotics: Path planning
 *
 * ## Complexity Comparison
 *
 * | Algorithm | Time Complexity | Best For |
 * |-----------|----------------|----------|
 * | Polygon Triangulation | O(n²) | Simple polygons |
 * | Brute Force Hull | O(n³) | Small datasets, education |
 * | Gift Wrapping | O(nh) | Few hull points |
 * | QuickHull | O(n log n) avg | General purpose |
 *
 * ## Colombian Geography Theme
 *
 * Examples use real Colombian locations:
 * - Cities: Bogotá, Medellín, Cali, Barranquilla
 * - Landmarks: Mountains, rivers, regions
 * - Makes examples more relatable and interesting
 *
 * ## Geometric Data Structures
 *
 * The example uses:
 * - **Point**: 2D coordinates (x, y)
 * - **Polygon**: Ordered list of points
 * - **PointSet**: Collection of points for hull computation
 *
 * ## Usage
 *
 * ```bash
 * # Run all geometric algorithm demos
 * ./geom_example
 *
 * # Run specific algorithm
 * ./geom_example -s triangulation
 * ./geom_example -s convexhull
 * ```
 *
 * @see geom_algorithms.H Geometric algorithm implementations
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <cstring>

#include <geom_algorithms.H>
#include <tpl_dynArray.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Helper functions
// ============================================================================

void print_header(const string& title)
{
  cout << "\n";
  cout << "+" << string(70, '-') << "+" << endl;
  cout << "| " << left << setw(68) << title << " |" << endl;
  cout << "+" << string(70, '-') << "+" << endl;
}

void print_subheader(const string& subtitle)
{
  cout << "\n  " << subtitle << endl;
  cout << "  " << string(subtitle.length(), '-') << endl;
}

/**
 * @brief Convert Geom_Number to double.
 */
inline double to_double(const Geom_Number& n)
{
  return n.get_d();
}

/**
 * @brief Print a point with optional label.
 */
void print_point(const Point& p, const string& label = "")
{
  cout << "  ";
  if (!label.empty()) cout << label << ": ";
  cout << "(" << fixed << setprecision(2) << to_double(p.get_x()) 
       << ", " << to_double(p.get_y()) << ")" << endl;
}

/**
 * @brief Print a polygon's vertices.
 */
void print_polygon(const Polygon& poly, const string& name)
{
  cout << "\n  " << name << " (" << poly.size() << " vertices):" << endl;
  int i = 0;
  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
  {
    const Vertex& v = it.get_current_vertex();
    cout << "    V" << i++ << ": (" << fixed << setprecision(2) 
         << to_double(v.get_x()) << ", " << to_double(v.get_y()) << ")" << endl;
  }
}

/**
 * @brief Print a triangle.
 */
void print_triangle(const Triangle& t, int index)
{
  cout << "    T" << index << ": ";
  const Point& a = t.get_p1();
  const Point& b = t.get_p2();
  const Point& c = t.get_p3();
  cout << "(" << fixed << setprecision(1) 
       << to_double(a.get_x()) << "," << to_double(a.get_y()) << ") - "
       << "(" << to_double(b.get_x()) << "," << to_double(b.get_y()) << ") - "
       << "(" << to_double(c.get_x()) << "," << to_double(c.get_y()) << ")" << endl;
}

/**
 * @brief Calculate area of a triangle using cross product.
 */
double triangle_area(const Triangle& t)
{
  const Point& a = t.get_p1();
  const Point& b = t.get_p2();
  const Point& c = t.get_p3();
  
  double ax = to_double(b.get_x()) - to_double(a.get_x());
  double ay = to_double(b.get_y()) - to_double(a.get_y());
  double bx = to_double(c.get_x()) - to_double(a.get_x());
  double by = to_double(c.get_y()) - to_double(a.get_y());
  
  return 0.5 * abs(ax * by - ay * bx);
}

// ============================================================================
// Example 1: Basic Polygon Triangulation
// ============================================================================

void demo_triangulation_basic()
{
  print_header("Example 1: Polygon Triangulation - Basic Shapes");
  
  // Create a simple square (represents Plaza de Bolivar, Bogota)
  print_subheader("Square (Plaza de Bolivar)");
  
  Polygon square;
  square.add_vertex(Point(0, 0));
  square.add_vertex(Point(100, 0));
  square.add_vertex(Point(100, 100));
  square.add_vertex(Point(0, 100));
  square.close();
  
  print_polygon(square, "Original polygon");
  
  CuttingEarsTriangulation triangulator;
  DynList<Triangle> triangles = triangulator(square);
  
  cout << "\n  Triangulation result:" << endl;
  int i = 0;
  double total_area = 0;
  for (auto it = triangles.get_it(); it.has_curr(); it.next_ne())
  {
    const Triangle& t = it.get_curr();
    print_triangle(t, i++);
    total_area += triangle_area(t);
  }
  
  cout << "\n  Total triangles: " << triangles.size() << endl;
  cout << "  Total area: " << fixed << setprecision(2) << total_area 
       << " square units" << endl;
  cout << "  Expected area: 10000.00 square units" << endl;
  
  // Create a pentagon (star shape simplified)
  print_subheader("Pentagon");
  
  Polygon pentagon;
  double radius = 50;
  for (int j = 0; j < 5; ++j)
  {
    double angle = 2 * M_PI * j / 5 - M_PI / 2;
    pentagon.add_vertex(Point(radius * cos(angle), radius * sin(angle)));
  }
  pentagon.close();
  
  print_polygon(pentagon, "Pentagon");
  
  DynList<Triangle> pent_triangles = triangulator(pentagon);
  
  cout << "\n  Triangulation result:" << endl;
  i = 0;
  total_area = 0;
  for (auto it = pent_triangles.get_it(); it.has_curr(); it.next_ne())
  {
    const Triangle& t = it.get_curr();
    print_triangle(t, i++);
    total_area += triangle_area(t);
  }
  
  cout << "\n  Total triangles: " << pent_triangles.size() << endl;
  cout << "  Total area: " << fixed << setprecision(2) << total_area 
       << " square units" << endl;
}

// ============================================================================
// Example 2: Triangulation of Complex Polygon
// ============================================================================

void demo_triangulation_complex()
{
  print_header("Example 2: Complex Polygon Triangulation");
  
  // Create an L-shaped polygon (like a building footprint)
  print_subheader("L-shaped polygon (Building footprint)");
  
  Polygon l_shape;
  l_shape.add_vertex(Point(0, 0));
  l_shape.add_vertex(Point(60, 0));
  l_shape.add_vertex(Point(60, 40));
  l_shape.add_vertex(Point(40, 40));
  l_shape.add_vertex(Point(40, 80));
  l_shape.add_vertex(Point(0, 80));
  l_shape.close();
  
  print_polygon(l_shape, "L-shaped polygon");
  
  CuttingEarsTriangulation triangulator;
  DynList<Triangle> triangles = triangulator(l_shape);
  
  cout << "\n  Triangulation result:" << endl;
  int i = 0;
  double total_area = 0;
  for (auto it = triangles.get_it(); it.has_curr(); it.next_ne())
  {
    const Triangle& t = it.get_curr();
    print_triangle(t, i++);
    total_area += triangle_area(t);
  }
  
  cout << "\n  Total triangles: " << triangles.size() << endl;
  cout << "  Total area: " << fixed << setprecision(2) << total_area 
       << " square units" << endl;
  
  // Verify: L-shape area = 60*40 + 40*40 = 2400 + 1600 = 4000
  cout << "  Expected area: 4000.00 square units (60x40 + 40x40)" << endl;
}

// ============================================================================
// Example 3: Convex Hull - Colombian Cities
// ============================================================================

void demo_convex_hull_cities()
{
  print_header("Example 3: Convex Hull - Colombian Cities");
  
  // Approximate coordinates of Colombian cities (scaled for visibility)
  // Using a simplified coordinate system where (0,0) is near Leticia
  
  DynList<Point> cities;
  
  // Major Colombian cities (approximate positions)
  struct CityCoord { const char* name; double x; double y; };
  DynArray<CityCoord> city_data;
  city_data.append({"Bogota", 74, 44});
  city_data.append({"Medellin", 75, 61});
  city_data.append({"Cali", 76, 34});
  city_data.append({"Barranquilla", 74, 109});
  city_data.append({"Cartagena", 75, 104});
  city_data.append({"Cucuta", 72, 77});
  city_data.append({"Bucaramanga", 73, 71});
  city_data.append({"Pereira", 75, 47});
  city_data.append({"Santa Marta", 74, 111});
  city_data.append({"Ibague", 75, 44});
  city_data.append({"Pasto", 77, 12});
  city_data.append({"Manizales", 75, 51});
  city_data.append({"Villavicencio", 73, 41});
  city_data.append({"Armenia", 75, 44});
  city_data.append({"Leticia", 70, 0});       // Amazon
  city_data.append({"Riohacha", 72, 116});    // La Guajira
  
  cout << "\n  Colombian cities included:" << endl;
  for (size_t i = 0; i < city_data.size(); ++i)
  {
    const auto& c = city_data(i);
    cout << "    " << left << setw(15) << c.name 
         << "(" << c.x << ", " << c.y << ")" << endl;
    cities.append(Point(c.x, c.y));
  }
  
  // Make copies for each algorithm
  DynList<Point> cities_bf, cities_gw, cities_qh;
  for (auto it = cities.get_it(); it.has_curr(); it.next_ne())
  {
    cities_bf.append(it.get_curr());
    cities_gw.append(it.get_curr());
    cities_qh.append(it.get_curr());
  }
  
  // Test each convex hull algorithm
  print_subheader("Brute Force Convex Hull O(n^3)");
  
  BruteForceConvexHull bf_hull;
  auto start = chrono::high_resolution_clock::now();
  Polygon hull_bf = bf_hull(cities_bf);
  auto end = chrono::high_resolution_clock::now();
  auto bf_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
  
  cout << "  Hull vertices: " << hull_bf.size() << endl;
  cout << "  Time: " << bf_time << " microseconds" << endl;
  
  print_subheader("Gift Wrapping Convex Hull O(nh)");
  
  GiftWrappingConvexHull gw_hull;
  start = chrono::high_resolution_clock::now();
  Polygon hull_gw = gw_hull(cities_gw);
  end = chrono::high_resolution_clock::now();
  auto gw_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
  
  cout << "  Hull vertices: " << hull_gw.size() << endl;
  cout << "  Time: " << gw_time << " microseconds" << endl;
  
  print_subheader("QuickHull O(n log n) average");
  
  QuickHull qh_hull;
  start = chrono::high_resolution_clock::now();
  Polygon hull_qh = qh_hull(cities_qh);
  end = chrono::high_resolution_clock::now();
  auto qh_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
  
  cout << "  Hull vertices: " << hull_qh.size() << endl;
  cout << "  Time: " << qh_time << " microseconds" << endl;
  
  // Show which cities are on the border
  print_subheader("Cities on the convex hull (border of Colombia)");
  
  cout << "  The convex hull represents the outermost cities:" << endl;
  for (Polygon::Vertex_Iterator it(hull_qh); it.has_curr(); it.next_ne())
  {
    const Vertex& v = it.get_current_vertex();
    // Find the city name
    for (size_t i = 0; i < city_data.size(); ++i)
    {
      const auto& c = city_data(i);
      if (abs(c.x - to_double(v.get_x())) < 0.5 && 
          abs(c.y - to_double(v.get_y())) < 0.5)
      {
        cout << "    - " << c.name << endl;
        break;
      }
    }
  }
}

// ============================================================================
// Example 4: Convex Hull - Random Points Performance
// ============================================================================

void demo_convex_hull_performance()
{
  print_header("Example 4: Convex Hull Algorithm Performance");
  
  cout << "\n  Comparing algorithms on random point sets:" << endl;
  cout << "  " << string(60, '-') << endl;
  
  DynArray<int> sizes;
  sizes.append(10);
  sizes.append(50);
  sizes.append(100);
  sizes.append(200);
  
  cout << "\n  " << setw(8) << "Points" 
       << setw(15) << "Brute Force"
       << setw(15) << "Gift Wrap"
       << setw(15) << "QuickHull" 
       << setw(10) << "Hull Size" << endl;
  cout << "  " << string(60, '-') << endl;
  
  for (size_t s = 0; s < sizes.size(); ++s)
  {
    int n = sizes(s);
    
    // Generate random points
    DynList<Point> points_bf, points_gw, points_qh;
    srand(42);  // Fixed seed for reproducibility
    
    for (int i = 0; i < n; ++i)
    {
      double x = (rand() % 1000) / 10.0;
      double y = (rand() % 1000) / 10.0;
      points_bf.append(Point(x, y));
      points_gw.append(Point(x, y));
      points_qh.append(Point(x, y));
    }
    
    // Time each algorithm
    BruteForceConvexHull bf;
    GiftWrappingConvexHull gw;
    QuickHull qh;
    
    auto start = chrono::high_resolution_clock::now();
    Polygon hull_bf = bf(points_bf);
    auto end = chrono::high_resolution_clock::now();
    auto bf_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    start = chrono::high_resolution_clock::now();
    Polygon hull_gw = gw(points_gw);
    end = chrono::high_resolution_clock::now();
    auto gw_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    start = chrono::high_resolution_clock::now();
    Polygon hull_qh = qh(points_qh);
    end = chrono::high_resolution_clock::now();
    auto qh_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    cout << "  " << setw(8) << n
         << setw(12) << bf_time << " us"
         << setw(12) << gw_time << " us"
         << setw(12) << qh_time << " us"
         << setw(10) << hull_qh.size() << endl;
  }
  
  cout << "\n  Note: Times in microseconds (us)" << endl;
  cout << "  Brute Force grows as O(n^3)" << endl;
  cout << "  Gift Wrapping grows as O(nh) where h = hull size" << endl;
  cout << "  QuickHull grows as O(n log n) on average" << endl;
}

// ============================================================================
// Example 5: Geometric Primitives Demo
// ============================================================================

void demo_geometric_primitives()
{
  print_header("Example 5: Geometric Primitives");
  
  print_subheader("Points and Segments");
  
  // Create some points
  Point bogota(74, 44);
  Point medellin(75, 61);
  Point cali(76, 34);
  
  cout << "  Three major Colombian cities:" << endl;
  print_point(bogota, "Bogota");
  print_point(medellin, "Medellin");
  print_point(cali, "Cali");
  
  // Create segments (routes)
  Segment bogota_medellin(bogota, medellin);
  Segment bogota_cali(bogota, cali);
  
  cout << "\n  Route lengths (approximate):" << endl;
  cout << "    Bogota-Medellin: " << fixed << setprecision(2) 
       << to_double(bogota_medellin.size()) << " units" << endl;
  cout << "    Bogota-Cali: " << to_double(bogota_cali.size()) 
       << " units" << endl;
  
  print_subheader("Point Position Tests");
  
  Point test_point(73, 50);
  cout << "  Test point: (73, 50)" << endl;
  cout << "  Relative to line Bogota-Medellin:" << endl;
  
  if (test_point.is_to_left_from(bogota, medellin))
    cout << "    Point is to the LEFT" << endl;
  else if (test_point.is_to_right_from(bogota, medellin))
    cout << "    Point is to the RIGHT" << endl;
  else
    cout << "    Point is ON the line" << endl;
  
  print_subheader("Triangle Operations");
  
  Triangle triangle(bogota, medellin, cali);
  
  cout << "  Triangle formed by Bogota, Medellin, Cali:" << endl;
  cout << "    Area: " << fixed << setprecision(2) 
       << triangle_area(triangle) << " square units" << endl;
  
  // Check points inside/outside using contains_to
  Point inside(75, 45);   // Somewhere in the middle
  Point outside(80, 80);  // Outside
  
  cout << "\n  Point containment:" << endl;
  cout << "    Point (75, 45): ";
  if (triangle.contains_to(inside))
    cout << "INSIDE the triangle" << endl;
  else
    cout << "OUTSIDE the triangle" << endl;
    
  cout << "    Point (80, 80): ";
  if (triangle.contains_to(outside))
    cout << "INSIDE the triangle" << endl;
  else
    cout << "OUTSIDE the triangle" << endl;
}

// ============================================================================
// Example 6: Practical Application - Coverage Area
// ============================================================================

void demo_coverage_area()
{
  print_header("Example 6: Coverage Area Calculation");
  
  cout << "\n  Scenario: Calculate coverage area of cell towers" << endl;
  cout << "  " << string(50, '-') << endl;
  
  // Cell tower locations (representing a region in Colombia)
  DynList<Point> towers;
  towers.append(Point(0, 0));
  towers.append(Point(30, 10));
  towers.append(Point(50, 0));
  towers.append(Point(55, 25));
  towers.append(Point(45, 50));
  towers.append(Point(20, 55));
  towers.append(Point(5, 40));
  towers.append(Point(25, 30));  // Interior tower
  towers.append(Point(35, 25));  // Interior tower
  
  cout << "\n  Tower locations:" << endl;
  int tower_num = 1;
  for (auto it = towers.get_it(); it.has_curr(); it.next_ne())
  {
    const Point& p = it.get_curr();
    cout << "    Tower " << tower_num++ << ": (" 
         << to_double(p.get_x()) << ", " 
         << to_double(p.get_y()) << ")" << endl;
  }
  
  // Calculate convex hull (coverage boundary)
  QuickHull qh;
  Polygon coverage = qh(towers);
  
  cout << "\n  Coverage boundary (convex hull):" << endl;
  cout << "    Boundary towers: " << coverage.size() << endl;
  cout << "    Interior towers: " << (9 - coverage.size()) << endl;
  
  // Calculate coverage area using triangulation
  Polygon coverage_copy = coverage;  // triangulation consumes the polygon
  CuttingEarsTriangulation triangulator;
  DynList<Triangle> triangles = triangulator(coverage_copy);
  
  double total_area = 0;
  for (auto it = triangles.get_it(); it.has_curr(); it.next_ne())
    total_area += triangle_area(it.get_curr());
  
  cout << "\n  Coverage statistics:" << endl;
  cout << "    Total coverage area: " << fixed << setprecision(2) 
       << total_area << " square km" << endl;
  cout << "    Triangles in mesh: " << triangles.size() << endl;
  
  // Perimeter calculation
  double perimeter = 0;
  for (Polygon::Segment_Iterator it(coverage); it.has_curr(); it.next_ne())
  {
    const Segment& seg = it.get_current_segment();
    perimeter += to_double(seg.size());
  }
  
  cout << "    Perimeter: " << perimeter << " km" << endl;
  cout << "    Compactness ratio: " << (4 * M_PI * total_area) / (perimeter * perimeter)
       << " (1.0 = circle)" << endl;
}

// ============================================================================
// Main
// ============================================================================

static void usage(const char* prog)
{
  cout << "Usage: " << prog << " [-s <triangulation|convexhull|all>] [--help]\n";
  cout << "\nIf no selector is given, all demos are executed.\n";
}

int main(int argc, char* argv[])
{
  string section = "all";
  for (int i = 1; i < argc; ++i)
    {
      const string arg = argv[i];
      if (arg == "--help" || arg == "-h")
        {
          usage(argv[0]);
          return 0;
        }
      if (arg == "-s")
        {
          if (i + 1 >= argc)
            {
              usage(argv[0]);
              return 1;
            }
          section = argv[++i];
          continue;
        }

      cout << "Unknown argument: " << arg << "\n";
      usage(argv[0]);
      return 1;
    }

  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "        ALEPH-W COMPUTATIONAL GEOMETRY EXAMPLE" << endl;
  cout << "        Triangulation and Convex Hull Algorithms" << endl;
  cout << "========================================================================" << endl;

  if (section == "all" || section == "triangulation")
    {
      demo_triangulation_basic();
      demo_triangulation_complex();
    }

  if (section == "all" || section == "convexhull")
    {
      demo_convex_hull_cities();
      demo_convex_hull_performance();
    }

  if (section == "all")
    {
      demo_geometric_primitives();
      demo_coverage_area();
    }

  if (!(section == "all" || section == "triangulation" || section == "convexhull"))
    {
      cout << "Unknown section: " << section << "\n";
      usage(argv[0]);
      return 1;
    }
  
  cout << "\n";
  cout << "========================================================================" << endl;
  cout << "                    Example completed successfully!" << endl;
  cout << "========================================================================" << endl;
  cout << endl;
  
  return 0;
}
