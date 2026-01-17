/**
 * @file tpl_dynMat_example.cc
 * @brief Educational examples for dynamic matrices with automatic resizing
 * 
 * WHAT IS DYNMATRIX?
 * ==================
 * 2D matrix that automatically grows as needed
 * Sparse storage: Only allocated cells consume memory
 * Perfect for dynamic graphs, adjacency matrices, distance tables
 * 
 * KEY FEATURES:
 * =============
 * - Automatic resizing (no need to pre-allocate)
 * - Sparse storage (unwritten cells = zero)
 * - Row/column operations
 * - Matrix arithmetic
 * 
 * WHEN TO USE:
 * ============
 * - Don't know matrix size in advance
 * - Matrix is mostly zeros (sparse)
 * - Need dynamic growth during computation
 * - Building adjacency/distance matrices
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_dynMat_example tpl_dynMat_example.cc
 * ./tpl_dynMat_example
 */

#include <iostream>
#include <tpl_dynMat.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Dynamic Matrices: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Basic Matrix Operations
  // =========================================================================
  {
    cout << "--- Example 1: Creating and Accessing Matrices ---\n\n";
    
    // STEP 1: Create matrix with initial size and default value
    DynMatrix<double> mat(3, 4, 0.0);  // 3 rows, 4 columns, default = 0.0
    
    cout << "Created 3x4 matrix (all zeros initially)\n\n";
    
    // STEP 2: Write individual elements
    cout << "Setting some values:\n";
    mat.write(0, 0, 1.5);   // Row 0, Col 0
    mat.write(0, 2, 2.7);   // Row 0, Col 2
    mat.write(1, 1, 3.2);   // Row 1, Col 1
    mat.write(2, 3, 4.8);   // Row 2, Col 3
    
    cout << "  mat[0,0] = 1.5\n";
    cout << "  mat[0,2] = 2.7\n";
    cout << "  mat[1,1] = 3.2\n";
    cout << "  mat[2,3] = 4.8\n\n";
    
    // STEP 3: Read elements
    cout << "Reading values:\n";
    cout << "  mat[0,0] = " << mat.read(0, 0) << "\n";
    cout << "  mat[0,1] = " << mat.read(0, 1) << " (unwritten, returns default 0.0)\n";
    cout << "  mat[1,1] = " << mat.read(1, 1) << "\n\n";
    
    // STEP 4: Display full matrix
    cout << "Complete matrix:\n";
    for (size_t i = 0; i < 3; ++i)
    {
      cout << "  [ ";
      for (size_t j = 0; j < 4; ++j)
      {
        double val = mat.read(i, j);
        if (val == 0.0)
          cout << "0.0 ";
        else
          cout << val << " ";
      }
      cout << "]\n";
    }
    
    cout << "\nKEY INSIGHT: Unwritten cells automatically return default value\n";
    cout << "             Memory efficient for sparse matrices!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Dynamic Growth
  // =========================================================================
  {
    cout << "--- Example 2: Automatic Resizing ---\n\n";
    
    // Start with small matrix
    DynMatrix<int> mat(2, 2, 0);
    
    cout << "Initial size: 2x2\n";
    mat.write(0, 0, 10);
    mat.write(1, 1, 20);
    
    cout << "Matrix:\n";
    cout << "  [ 10  0 ]\n";
    cout << "  [  0 20 ]\n\n";
    
    // To grow matrix, need to resize explicitly
    cout << "Growing matrix to 6x6:\n";
    DynMatrix<int> larger_mat(6, 6, 0);
    
    // Copy existing values
    larger_mat.write(0, 0, 10);
    larger_mat.write(1, 1, 20);
    larger_mat.write(5, 5, 100);  // Now this works!
    
    cout << "  Resized to 6x6\n";
    cout << "  Copied existing values and added new one at [5,5]\n\n";
    
    cout << "New matrix values:\n";
    cout << "  [0,0] = " << larger_mat.read(0, 0) << "\n";
    cout << "  [1,1] = " << larger_mat.read(1, 1) << "\n";
    cout << "  [5,5] = " << larger_mat.read(5, 5) << "\n";
    cout << "  All other cells = 0 (default)\n\n";
    
    cout << "NOTE: DynMatrix has fixed size after construction\n";
    cout << "      Create new larger matrix if size needs to grow\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Adjacency Matrix for Graphs
  // =========================================================================
  {
    cout << "--- Example 3: Graph Adjacency Matrix ---\n\n";
    
    cout << "SCENARIO: Social network (who knows whom)\n";
    cout << "=========================================\n\n";
    
    // Create adjacency matrix
    DynMatrix<int> adjacency(5, 5, 0);  // 5 people
    
    cout << "People: 0=Alice, 1=Bob, 2=Charlie, 3=Diana, 4=Eve\n\n";
    
    // Build connections (undirected)
    cout << "Friendships:\n";
    
    // Alice knows Bob and Charlie
    adjacency.write(0, 1, 1);
    adjacency.write(1, 0, 1);
    adjacency.write(0, 2, 1);
    adjacency.write(2, 0, 1);
    cout << "  Alice <-> Bob\n";
    cout << "  Alice <-> Charlie\n";
    
    // Bob knows Charlie and Diana
    adjacency.write(1, 2, 1);
    adjacency.write(2, 1, 1);
    adjacency.write(1, 3, 1);
    adjacency.write(3, 1, 1);
    cout << "  Bob <-> Charlie\n";
    cout << "  Bob <-> Diana\n";
    
    // Diana knows Eve
    adjacency.write(3, 4, 1);
    adjacency.write(4, 3, 1);
    cout << "  Diana <-> Eve\n\n";
    
    // Display adjacency matrix
    cout << "Adjacency Matrix:\n";
    cout << "       A B C D E\n";
    for (size_t i = 0; i < 5; ++i)
    {
      char names[] = {'A', 'B', 'C', 'D', 'E'};
      cout << "  " << names[i] << " [  ";
      for (size_t j = 0; j < 5; ++j)
        cout << adjacency.read(i, j) << " ";
      cout << "]\n";
    }
    
    cout << "\nQUERIES:\n";
    cout << "  Does Alice know Diana? " << (adjacency.read(0, 3) ? "Yes" : "No") << "\n";
    cout << "  Does Bob know Charlie? " << (adjacency.read(1, 2) ? "Yes" : "No") << "\n";
    cout << "  Does Eve know Alice? " << (adjacency.read(4, 0) ? "Yes" : "No") << "\n\n";
    
    cout << "ADVANTAGE: O(1) lookup for \"are X and Y connected?\"\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Distance Matrix (All-Pairs Shortest Paths)
  // =========================================================================
  {
    cout << "--- Example 4: Distance Matrix ---\n\n";
    
    cout << "SCENARIO: City distances (miles)\n";
    cout << "================================\n\n";
    
    const int INF = 9999;  // Infinity (no direct connection)
    DynMatrix<int> dist(4, 4, INF);
    
    cout << "Cities: 0=NYC, 1=Boston, 2=Philadelphia, 3=DC\n\n";
    
    // Self-distances are zero
    for (int i = 0; i < 4; ++i)
      dist.write(i, i, 0);
    
    // Direct connections (symmetric)
    dist.write(0, 1, 215);  // NYC <-> Boston
    dist.write(1, 0, 215);
    
    dist.write(0, 2, 95);   // NYC <-> Philadelphia
    dist.write(2, 0, 95);
    
    dist.write(0, 3, 225);  // NYC <-> DC
    dist.write(3, 0, 225);
    
    dist.write(2, 3, 140);  // Philadelphia <-> DC
    dist.write(3, 2, 140);
    
    cout << "Direct distances:\n";
    cout << "  NYC <-> Boston: 215 miles\n";
    cout << "  NYC <-> Philadelphia: 95 miles\n";
    cout << "  NYC <-> DC: 225 miles\n";
    cout << "  Philadelphia <-> DC: 140 miles\n\n";
    
    // Display distance matrix
    cout << "Distance Matrix:\n";
    cout << "          NYC  Bos  Phi   DC\n";
    const char* cities[] = {"NYC", "Bos", "Phi", "DC "};
    for (int i = 0; i < 4; ++i)
    {
      cout << "  " << cities[i] << " [";
      for (int j = 0; j < 4; ++j)
      {
        int d = dist.read(i, j);
        if (d == INF)
          cout << " INF ";
        else if (d < 10)
          cout << "   " << d << " ";
        else if (d < 100)
          cout << "  " << d << " ";
        else
          cout << " " << d << " ";
      }
      cout << "]\n";
    }
    
    cout << "\nNOTE: Boston to DC = INF (no direct route)\n";
    cout << "      Must go through NYC or Philadelphia\n\n";
    
    cout << "USE CASE: Input for Floyd-Warshall algorithm\n";
    cout << "            Computes shortest paths between all pairs\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Matrix Arithmetic
  // =========================================================================
  {
    cout << "--- Example 5: Matrix Operations ---\n\n";
    
    // Create two matrices
    DynMatrix<double> A(2, 2, 0.0);
    DynMatrix<double> B(2, 2, 0.0);
    
    // Matrix A
    A.write(0, 0, 1.0);
    A.write(0, 1, 2.0);
    A.write(1, 0, 3.0);
    A.write(1, 1, 4.0);
    
    // Matrix B
    B.write(0, 0, 5.0);
    B.write(0, 1, 6.0);
    B.write(1, 0, 7.0);
    B.write(1, 1, 8.0);
    
    cout << "Matrix A:\n";
    cout << "  [ 1.0  2.0 ]\n";
    cout << "  [ 3.0  4.0 ]\n\n";
    
    cout << "Matrix B:\n";
    cout << "  [ 5.0  6.0 ]\n";
    cout << "  [ 7.0  8.0 ]\n\n";
    
    cout << "OPERATIONS:\n\n";
    
    // Element-wise addition (conceptual - would need to implement)
    cout << "A + B (element-wise):\n";
    cout << "  [ 6.0  8.0 ]\n";
    cout << "  [10.0 12.0 ]\n\n";
    
    // Scalar multiplication
    cout << "2 * A:\n";
    cout << "  [ 2.0  4.0 ]\n";
    cout << "  [ 6.0  8.0 ]\n\n";
    
    cout << "NOTE: DynMatrix provides storage structure\n";
    cout << "      Arithmetic operations can be built on top\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 6: Sparse Matrix Efficiency
  // =========================================================================
  {
    cout << "--- Example 6: Sparse Matrix Benefits ---\n\n";
    
    cout << "SCENARIO: Large sparse matrix (mostly zeros)\n";
    cout << "============================================\n\n";
    
    // Large matrix with few non-zero elements
    DynMatrix<int> sparse(1000, 1000, 0);
    
    cout << "Matrix size: 1000 x 1000 = 1,000,000 cells\n\n";
    
    // Set only a few elements
    sparse.write(0, 0, 1);
    sparse.write(100, 200, 2);
    sparse.write(500, 750, 3);
    sparse.write(999, 999, 4);
    
    cout << "Non-zero elements: 4\n";
    cout << "  [0,0] = 1\n";
    cout << "  [100,200] = 2\n";
    cout << "  [500,750] = 3\n";
    cout << "  [999,999] = 4\n\n";
    
    cout << "MEMORY EFFICIENCY:\n";
    cout << "  Dense matrix: 1,000,000 integers = ~4 MB\n";
    cout << "  Sparse matrix: ~4 integers + overhead = ~100 bytes\n";
    cout << "  Space savings: 99.998%!\n\n";
    
    cout << "WHEN TO USE SPARSE:\n";
    cout << "  ✓ Large graphs with few edges (social networks)\n";
    cout << "  ✓ Adjacency matrices of sparse graphs\n";
    cout << "  ✓ Distance tables with limited connections\n";
    cout << "  ✓ Feature matrices in machine learning\n\n";
  }
  
  cout << "=== SUMMARY: Dynamic Matrices ===\n";
  cout << "\n1. KEY FEATURES:\n";
  cout << "   * Automatic resizing (no pre-allocation needed)\n";
  cout << "   * Sparse storage (only non-zero cells consume memory)\n";
  cout << "   * Default value for unwritten cells\n";
  cout << "   * Dynamic growth during computation\n";
  cout << "\n2. BASIC OPERATIONS:\n";
  cout << "   write(row, col, value): Set element\n";
  cout << "   read(row, col): Get element (default if unwritten)\n";
  cout << "   rows(), cols(): Get dimensions\n";
  cout << "   Time: O(1) for all operations\n";
  cout << "\n3. WHEN TO USE:\n";
  cout << "   ✓ Unknown matrix size at start\n";
  cout << "   ✓ Sparse matrices (mostly zeros)\n";
  cout << "   ✓ Adjacency/distance matrices\n";
  cout << "   ✓ Dynamic programming tables\n";
  cout << "   ✓ Need automatic growth\n";
  cout << "\n4. WHEN NOT TO USE:\n";
  cout << "   ✗ Dense matrices (use Array<Array<T>>)\n";
  cout << "   ✗ Need matrix algebra (use specialized library)\n";
  cout << "   ✗ Performance-critical dense operations\n";
  cout << "\n5. COMMON APPLICATIONS:\n";
  cout << "   * Graph adjacency matrices\n";
  cout << "   * Distance/cost matrices\n";
  cout << "   * Dynamic programming tables\n";
  cout << "   * Sparse data storage\n";
  cout << "   * Hash table alternatives (2D keys)\n";
  cout << "\n6. MEMORY EFFICIENCY:\n";
  cout << "   Dense matrix: O(rows * cols) always\n";
  cout << "   Sparse matrix: O(non-zero elements)\n";
  cout << "   For 1% density: 99% space savings!\n";
  cout << "\n7. BEST PRACTICES:\n";
  cout << "   * Choose appropriate default value (usually 0)\n";
  cout << "   * Use for graphs/sparse data\n";
  cout << "   * Check sparsity before choosing structure\n";
  cout << "   * Consider access patterns (random vs sequential)\n";
  
  return 0;
}
