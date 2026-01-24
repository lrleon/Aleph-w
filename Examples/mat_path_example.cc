/**
 * @file mat_path_example.cc
 * @brief Educational examples for path reconstruction from distance matrices
 * 
 * WHAT IS PATH RECONSTRUCTION?
 * ============================
 * After running Floyd-Warshall all-pairs shortest paths algorithm,
 * you get a distance matrix D[i][j] = shortest distance from i to j
 * But how do you get the ACTUAL PATH?
 * 
 * SOLUTION: Path matrix P[i][j]
 * ==============================
 * During Floyd-Warshall, maintain P[i][j] = intermediate node on path
 * Then recursively reconstruct the path from P matrix
 * 
 * WHY IT MATTERS:
 * ===============
 * Distance alone isn't enough - you need the route!
 * GPS needs: "Turn left, then right" not just "10 miles away"
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o mat_path_example mat_path_example.cc
 * ./mat_path_example
 */

#include <iostream>
#include <tpl_graph.H>
#include <tpl_dynArray.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Path Reconstruction from Matrices: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Understanding Distance vs Path Matrices
  // =========================================================================
  {
    cout << "--- Example 1: Distance Matrix vs Path Matrix ---\n\n";
    
    cout << "SCENARIO: Simple road network\n";
    cout << "=============================\n\n";
    
    cout << "Cities: A(0), B(1), C(2), D(3)\n\n";
    
    cout << "Direct roads:\n";
    cout << "  A -> B: 10 miles\n";
    cout << "  A -> C: 5 miles\n";
    cout << "  C -> B: 2 miles\n";
    cout << "  C -> D: 8 miles\n";
    cout << "  B -> D: 4 miles\n\n";
    
    const int INF = 9999;
    const int N = 4;
    
    // Initial distance matrix (direct connections only)
    int dist[N][N] = {
      {0,   10,  5,   INF},  // A
      {INF, 0,   INF, 4},    // B
      {INF, 2,   0,   8},    // C
      {INF, INF, INF, 0}     // D
    };
    
    cout << "Initial Distance Matrix (direct connections):\n";
    cout << "     A    B    C    D\n";
    for (int i = 0; i < N; ++i)
    {
      char names[] = {'A', 'B', 'C', 'D'};
      cout << "  " << names[i] << " ";
      for (int j = 0; j < N; ++j)
      {
        if (dist[i][j] == INF)
          cout << " INF ";
        else
          cout << "  " << dist[i][j] << "  ";
      }
      cout << "\n";
    }
    
    cout << "\nAFTER FLOYD-WARSHALL:\n\n";
    
    cout << "Final Distance Matrix (shortest paths):\n";
    cout << "  A -> D: 11 miles (via C then B)\n";
    cout << "  Direct would be INF, but A->C->B->D = 5+2+4 = 11\n\n";
    
    cout << "PATH MATRIX stores intermediate nodes:\n";
    cout << "  path[A][D] = C (go through C first)\n";
    cout << "  path[C][D] = B (then go through B)\n";
    cout << "  path[B][D] = -1 (direct connection)\n\n";
    
    cout << "KEY INSIGHT: Distance tells HOW FAR, path matrix tells WHICH WAY\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Reconstructing a Path
  // =========================================================================
  {
    cout << "--- Example 2: Step-by-Step Path Reconstruction ---\n\n";
    
    cout << "GOAL: Find path from A to D\n";
    cout << "===========================\n\n";
    
    // Path matrix (built during Floyd-Warshall)
    const int N = 4;
    int path[N][N] = {
      {-1, -1, -1,  2},   // A: to reach D, go through C first
      {-1, -1, -1, -1},   // B: direct connections
      {-1, -1, -1,  1},   // C: to reach D, go through B first
      {-1, -1, -1, -1}    // D: no outgoing paths
    };
    
    cout << "Path matrix:\n";
    cout << "     A   B   C   D\n";
    for (int i = 0; i < N; ++i)
    {
      char names[] = {'A', 'B', 'C', 'D'};
      cout << "  " << names[i] << " ";
      for (int j = 0; j < N; ++j)
      {
        if (path[i][j] == -1)
          cout << " -  ";
        else
          cout << " " << (char)('A' + path[i][j]) << "  ";
      }
      cout << "\n";
    }
    
    cout << "\nRECONSTRUCTION ALGORITHM:\n";
    cout << "========================\n\n";
    
    int src = 0;  // A
    int dst = 3;  // D
    
    cout << "find_path(A, D):\n\n";
    
    cout << "Step 1: Check path[A][D] = " << (char)('A' + path[src][dst]) << "\n";
    cout << "        Intermediate node is C\n";
    cout << "        Recursively find: A -> C, C -> D\n\n";
    
    cout << "Step 2: find_path(A, C):\n";
    cout << "        path[A][C] = -1 (direct)\n";
    cout << "        Add edge: A -> C\n\n";
    
    cout << "Step 3: find_path(C, D):\n";
    cout << "        path[C][D] = B\n";
    cout << "        Intermediate node is B\n";
    cout << "        Recursively find: C -> B, B -> D\n\n";
    
    cout << "Step 4: find_path(C, B):\n";
    cout << "        path[C][B] = -1 (direct)\n";
    cout << "        Add edge: C -> B\n\n";
    
    cout << "Step 5: find_path(B, D):\n";
    cout << "        path[B][D] = -1 (direct)\n";
    cout << "        Add edge: B -> D\n\n";
    
    cout << "FINAL PATH: A -> C -> B -> D\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Complete Working Example
  // =========================================================================
  {
    cout << "--- Example 3: Full Floyd-Warshall with Path Recovery ---\n\n";
    
    const int N = 5;
    const int INF = 99999;
    
    cout << "Network: 5 cities with highways\n\n";
    
    // Distance matrix
    int dist[N][N];
    int path[N][N];
    
    // Initialize
    for (int i = 0; i < N; ++i)
    {
      for (int j = 0; j < N; ++j)
      {
        if (i == j)
          dist[i][j] = 0;
        else
          dist[i][j] = INF;
        path[i][j] = -1;  // No intermediate node initially
      }
    }
    
    // Add edges
    dist[0][1] = 10;  dist[1][0] = 10;  // 0 <-> 1
    dist[0][2] = 5;   dist[2][0] = 5;   // 0 <-> 2
    dist[1][3] = 2;   dist[3][1] = 2;   // 1 <-> 3
    dist[2][3] = 9;   dist[3][2] = 9;   // 2 <-> 3
    dist[2][4] = 3;   dist[4][2] = 3;   // 2 <-> 4
    dist[3][4] = 4;   dist[4][3] = 4;   // 3 <-> 4
    
    cout << "Initial distances (direct connections):\n";
    cout << "  0 <-> 1: 10\n";
    cout << "  0 <-> 2: 5\n";
    cout << "  1 <-> 3: 2\n";
    cout << "  2 <-> 3: 9\n";
    cout << "  2 <-> 4: 3\n";
    cout << "  3 <-> 4: 4\n\n";
    
    // Floyd-Warshall with path reconstruction
    cout << "Running Floyd-Warshall...\n";
    for (int k = 0; k < N; ++k)
    {
      for (int i = 0; i < N; ++i)
      {
        for (int j = 0; j < N; ++j)
        {
          if (dist[i][k] + dist[k][j] < dist[i][j])
          {
            dist[i][j] = dist[i][k] + dist[k][j];
            path[i][j] = k;  // Store intermediate node
          }
        }
      }
    }
    
    cout << "Done!\n\n";
    
    // Show shortest distances
    cout << "Shortest Distance Matrix:\n";
    cout << "     0   1   2   3   4\n";
    for (int i = 0; i < N; ++i)
    {
      cout << "  " << i << " ";
      for (int j = 0; j < N; ++j)
      {
        if (dist[i][j] == INF)
          cout << " INF";
        else
          cout << "  " << dist[i][j] << " ";
      }
      cout << "\n";
    }
    
    cout << "\nEXAMPLE QUERIES:\n\n";
    
    // Query 1: 0 to 4
    cout << "Shortest path from 0 to 4:\n";
    cout << "  Distance: " << dist[0][4] << "\n";
    cout << "  Path: 0 -> 2 -> 4 (via " << path[0][4] << ")\n";
    cout << "  (Direct: 0->2=5, 2->4=3, Total=8)\n\n";
    
    // Query 2: 0 to 3
    cout << "Shortest path from 0 to 3:\n";
    cout << "  Distance: " << dist[0][3] << "\n";
    if (path[0][3] != -1)
      cout << "  Path: 0 -> ... -> 3 (via " << path[0][3] << ")\n";
    else
      cout << "  Path: Direct 0 -> 3\n";
    cout << "\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Practical Application - GPS Routing
  // =========================================================================
  {
    cout << "--- Example 4: GPS Navigation System ---\n\n";
    
    cout << "REAL-WORLD APPLICATION:\n";
    cout << "======================\n\n";
    
    cout << "USER QUERY: 'Navigate from Home to Airport'\n\n";
    
    cout << "SYSTEM PROCESS:\n";
    cout << "1. Look up distance matrix: Home -> Airport = 45 min\n";
    cout << "2. Look up path matrix: Route goes through Downtown\n";
    cout << "3. Recursively reconstruct:\n";
    cout << "   a. Home -> Downtown: via Highway-1\n";
    cout << "   b. Downtown -> Airport: via Airport Rd\n\n";
    
    cout << "NAVIGATION INSTRUCTIONS:\n";
    cout << "  1. Head east on Main St\n";
    cout << "  2. Turn right onto Highway-1 (10 mi)\n";
    cout << "  3. Take exit 15 for Downtown (15 mi)\n";
    cout << "  4. Continue on Airport Rd (20 mi)\n";
    cout << "  5. Arrive at Airport (45 min total)\n\n";
    
    cout << "WHY PATH MATRIX IS ESSENTIAL:\n";
    cout << "  ✓ Gives turn-by-turn directions\n";
    cout << "  ✓ Shows intermediate waypoints\n";
    cout << "  ✓ Allows route alternatives\n";
    cout << "  ✓ Enables traffic rerouting\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: When No Path Exists
  // =========================================================================
  {
    cout << "--- Example 5: Handling Unreachable Destinations ---\n\n";
    
    cout << "PROBLEM: What if no path exists?\n";
    cout << "=================================\n\n";
    
    cout << "Example: Island A and Island B (no bridge)\n\n";
    
    const int N = 2;
    const int INF = 99999;
    
    int dist[N][N] = {
      {0, INF},
      {INF, 0}
    };
    (void)dist;
    
    cout << "Distance matrix:\n";
    cout << "     A    B\n";
    cout << "  A  0   INF\n";
    cout << "  B INF   0\n\n";
    
    cout << "DETECTION:\n";
    cout << "  if (dist[A][B] == INF):\n";
    cout << "    No path exists!\n\n";
    
    cout << "USER MESSAGE:\n";
    cout << "  'Cannot reach destination'\n";
    cout << "  'No route available'\n";
    cout << "  'Destination unreachable'\n\n";
    
    cout << "PRACTICAL HANDLING:\n";
    cout << "  * Suggest alternative destinations\n";
    cout << "  * Show nearby accessible locations\n";
    cout << "  * Offer multi-modal transport (ferry, flight)\n\n";
  }
  
  cout << "=== SUMMARY: Path Matrix Reconstruction ===\n";
  cout << "\n1. WHY PATH MATRIX?\n";
  cout << "   Distance matrix: Tells HOW FAR\n";
  cout << "   Path matrix: Tells WHICH WAY\n";
  cout << "   Both needed for practical routing\n";
  cout << "\n2. HOW IT WORKS:\n";
  cout << "   During Floyd-Warshall:\n";
  cout << "     When improving path i->j through k:\n";
  cout << "     path[i][j] = k (store intermediate node)\n";
  cout << "   After Floyd-Warshall:\n";
  cout << "     Recursively reconstruct using path matrix\n";
  cout << "\n3. RECONSTRUCTION ALGORITHM:\n";
  cout << "   function find_path(i, j):\n";
  cout << "     if path[i][j] == -1:\n";
  cout << "       return [i, j]  // Direct edge\n";
  cout << "     else:\n";
  cout << "       k = path[i][j]\n";
  cout << "       return find_path(i,k) + find_path(k,j)\n";
  cout << "\n4. COMPLEXITY:\n";
  cout << "   Floyd-Warshall: O(V^3)\n";
  cout << "   Path reconstruction: O(V) per query\n";
  cout << "   Preprocessing once, query many times\n";
  cout << "\n5. REAL-WORLD APPLICATIONS:\n";
  cout << "   ✓ GPS navigation systems\n";
  cout << "   ✓ Network routing protocols\n";
  cout << "   ✓ Flight planning systems\n";
  cout << "   ✓ Supply chain logistics\n";
  cout << "   ✓ Game pathfinding\n";
  cout << "\n6. KEY PROPERTIES:\n";
  cout << "   * Works for all pairs simultaneously\n";
  cout << "   * Handles negative weights (not negative cycles)\n";
  cout << "   * Simple recursive implementation\n";
  cout << "   * Space: O(V^2) for path matrix\n";
  cout << "\n7. BEST PRACTICES:\n";
  cout << "   * Always build path matrix with distance matrix\n";
  cout << "   * Check for INF before reconstructing\n";
  cout << "   * Cache reconstructed paths if queried often\n";
  cout << "   * Handle edge cases (no path, same node)\n";
  
  return 0;
}
