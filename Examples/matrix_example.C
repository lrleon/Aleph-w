/**
 * @file matrix_example.C
 * @brief Comprehensive example demonstrating sparse matrices and vectors in Aleph-w
 *
 * This program demonstrates sparse Matrix and Vector classes from
 * `al-matrix.H` and `al-vector.H`. Unlike dense matrices (which store
 * all elements), sparse matrices only store non-zero entries, making
 * them memory-efficient for sparse data. Additionally, they support
 * domain-based indexing, allowing rows and columns to be indexed by
 * any type (not just integers).
 *
 * ## Sparse vs Dense Matrices
 *
 * ### Dense Matrix
 * - Stores ALL elements (even zeros)
 * - Memory: O(rows × cols)
 * - Access: O(1) - direct indexing
 * - **Best for**: Dense data, small matrices
 *
 * ### Sparse Matrix (This Example)
 * - Stores ONLY non-zero entries
 * - Memory: O(nonzero_entries)
 * - Access: O(log n) - search in sparse structure
 * - **Best for**: Sparse data, large matrices with few non-zeros
 *
 * ### When to Use Sparse?
 *
 * Use sparse when:
 * - **Sparsity > 90%**: Most entries are zero
 * - **Large matrices**: Memory savings significant
 * - **Domain-based indexing**: Need named rows/columns
 *
 * **Example**: 1000×1000 matrix with only 1000 non-zeros:
 * - Dense: 1,000,000 elements = 8 MB (for doubles)
 * - Sparse: 1,000 elements = 8 KB (huge savings!)
 *
 * ## Key Features Demonstrated
 *
 * ### Sparse Storage
 *
 * - **Efficient storage**: Only non-zero entries stored
 * - **Automatic cleanup**: Near-zero entries removed (epsilon tolerance)
 * - **Memory efficient**: O(nonzeros) instead of O(rows×cols)
 * - **Flexible**: Can handle very large matrices
 *
 * ### Domain-Based Indexing
 *
 * Unlike traditional matrices (indexed 0..n-1), Aleph-w matrices support:
 * - **String indices**: `matrix["row_name"]["col_name"]`
 * - **Custom types**: Any comparable type as index
 * - **Named dimensions**: Rows/columns have meaningful names
 *
 * **Example**:
 * ```cpp
 * Matrix<string, string, double> sales(products, stores);
 * sales.set_entry("Laptop", "BOG", 150);
 * sales.set_entry("Phone", "MED", 450);
 * ```
 *
 * ### Operations Demonstrated
 *
 * - **Element access**: `get_entry()`, `set_entry()`
 * - **Arithmetic**: `+`, `-`, `+=`, `-=` for matrices/vectors
 * - **Scalar operations**: `mult_by_scalar()`, `scalar * matrix`
 * - **Row/column extraction**: `get_row_vector()`, `get_col_vector()`
 * - **Row/column setting**: `set_vector_as_row()`, `set_vector_as_col()`
 * - **Transposition**: `transpose()`
 * - **Identity matrix**: `identity()` (for square matrices)
 * - **Matrix multiplication**: `vector_matrix_mult()`, `matrix_vector_mult()`
 * - **Matrix-vector multiplication**: `operator*`, `mult_matrix_vector_sparse()`,
 *   `mult_matrix_vector_dot_product()`, `mult_matrix_vector_linear_comb()`
 * - **Vector-matrix multiplication**: `operator*`, `mult_vector_matrix_linear_comb()`
 * - **Outer product**: `outer_product(v1, v2)`
 * - **Comparison**: `==`, `!=`, `equal_to()` (with epsilon tolerance)
 * - **Initializer list construction**: Direct matrix initialization
 * - **Conversion**: `to_rowlist()`, `to_collist()`, `to_str()`
 *
 * ## Applications
 *
 * ### Scientific Computing
 * - **Linear systems**: Sparse linear algebra (demonstrated in demo_linear_system)
 * - **Finite element methods**: Sparse stiffness matrices
 * - **Graph algorithms**: Adjacency matrices (demonstrated in demo_adjacency_matrix)
 *
 * ### Data Analysis
 * - **Feature matrices**: Machine learning (many zeros)
 * - **Transaction data**: User-item matrices (sparse)
 * - **Time series**: Sparse temporal data
 *
 * ### Business Applications
 * - **Sales data**: Products × Stores (demonstrated in demo_named_matrix)
 * - **Resource allocation**: Resources × Tasks
 * - **Financial modeling**: Instruments × Time periods
 *
 * ## Complexity
 *
 * | Operation | Dense | Sparse | Notes |
 * |-----------|-------|--------|-------|
 * | Storage | O(n²) | O(nnz) | Sparse wins for sparse data |
 * | Access | O(1) | O(1) avg | Hash-based storage |
 * | Addition | O(n²) | O(nnz) | Sparse much faster |
 * | M×v mult | O(n²) | O(nnz) | Sparse iteration available |
 * | M×M mult | O(n³) | O(nnz₁×nnz₂) | Depends on sparsity |
 *
 * ## Demos Included
 *
 * 1. **Sparse Vector Basics** - Vector creation and element access
 * 2. **String-Indexed Vectors** - Domain-based indexing with strings
 * 3. **Sparse Matrix Basics** - Matrix creation and storage
 * 4. **Named Row/Column Matrix** - Real-world sales data example
 * 5. **Vector Arithmetic** - Addition, subtraction, scalar ops
 * 6. **Graph Adjacency Matrix** - Practical graph representation
 * 7. **Epsilon Tolerance** - Near-zero handling
 * 8. **Initializer List Construction** - Direct matrix initialization
 * 9. **Matrix Transpose** - Row/column swapping
 * 10. **Identity Matrix** - Creating identity for square matrices
 * 11. **Matrix-Vector Multiplication** - Multiple methods compared
 * 12. **Matrix-Matrix Multiplication** - Two approaches demonstrated
 * 13. **Outer Product** - Vector outer product
 * 14. **Matrix Comparison** - Equality with epsilon tolerance
 * 15. **Matrix Arithmetic** - Full arithmetic operations
 * 16. **Row/Column Operations** - Extract/set rows and columns
 * 17. **Linear System Example** - Practical Ax=b verification
 *
 * ## Usage
 *
 * ```bash
 * # Run all matrix demonstrations
 * ./matrix_example
 * ```
 *
 * @see al-matrix.H Sparse Matrix class
 * @see al-vector.H Sparse Vector class
 * @see al-domain.H Domain-based indexing
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <memory>

#include <tclap/CmdLine.h>

#include <al-vector.H>
#include <al-matrix.H>
#include <al-domain.H>
#include <htlist.H>
#include <ahFunctional.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Helper functions
// =============================================================================

void print_section(const string& title)
{
  cout << "\n" << string(60, '=') << "\n";
  cout << "  " << title << "\n";
  cout << string(60, '=') << "\n\n";
}

void print_subsection(const string& title)
{
  cout << "\n--- " << title << " ---\n";
}

// =============================================================================
// 1. Sparse Vector Basics
// =============================================================================

void demo_sparse_vector()
{
  print_section("SPARSE VECTOR BASICS");
  
  // Create a domain for the vector indices
  print_subsection("Creating a Sparse Vector");
  
  // Create domain with elements 0-9
  auto domain = make_shared<AlDomain<int>>();
  for (int i = 0; i < 10; i++)
    (void)domain->insert(i);
  
  // Create sparse vector with this domain
  Vector<int, double> v(domain);
  
  cout << "Created sparse vector with domain {0, 1, ..., 9}" << endl;
  cout << "Initial storage: only non-zero elements are stored" << endl;
  
  // Set some values
  print_subsection("Setting Values");
  v.set_entry(0, 1.5);
  v.set_entry(3, 2.7);
  v.set_entry(7, -4.2);
  
  cout << "Set v[0] = 1.5, v[3] = 2.7, v[7] = -4.2" << endl;
  cout << "\nNon-zero entries:" << endl;
  for (auto it = v.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  v[" << entry.first << "] = " << entry.second << endl;
  }
  
  // Access values
  print_subsection("Accessing Values");
  cout << "v[0] = " << v.get_entry(0) << endl;
  cout << "v[3] = " << v.get_entry(3) << endl;
  cout << "v[5] = " << v.get_entry(5) << " (not set, returns 0)" << endl;
  
  // Count non-zero entries
  size_t nnz = 0;
  for (auto it = v.get_it(); it.has_curr(); it.next())
    nnz++;
  cout << "\nNumber of non-zero entries: " << nnz << endl;
  cout << "Memory savings: only storing " << nnz << " of 10 possible values" << endl;
}

// =============================================================================
// 2. String-Indexed Vectors
// =============================================================================

void demo_string_indexed_vector()
{
  print_section("STRING-INDEXED VECTORS");
  
  cout << "Vectors can be indexed by ANY type, not just integers!" << endl;
  
  // Create a domain with Colombian city names
  auto cities = make_shared<AlDomain<string>>();
  (void)cities->insert("Bogota");
  (void)cities->insert("Medellin");
  (void)cities->insert("Cali");
  (void)cities->insert("Barranquilla");
  (void)cities->insert("Cartagena");
  
  // Create a population vector (in millions)
  Vector<string, double> population(cities);
  population.set_entry("Bogota", 7.4);
  population.set_entry("Medellin", 2.5);
  population.set_entry("Cali", 2.2);
  population.set_entry("Barranquilla", 1.2);
  // Cartagena not set - will be 0
  
  cout << "\nColombian city populations (millions):" << endl;
  for (auto it = population.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  " << setw(15) << left << entry.first 
         << ": " << entry.second << "M" << endl;
  }
  
  cout << "\nCartagena (not set): " << population.get_entry("Cartagena") << "M" << endl;
  
  // Scalar multiplication
  print_subsection("Scalar Operations");
  auto doubled = population * 2.0;
  cout << "Population * 2:" << endl;
  for (auto it = doubled.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  " << setw(15) << left << entry.first 
         << ": " << entry.second << "M" << endl;
  }
}

// =============================================================================
// 3. Sparse Matrix Basics
// =============================================================================

void demo_sparse_matrix()
{
  print_section("SPARSE MATRIX BASICS");
  
  // Create row and column domains
  auto rows = make_shared<AlDomain<int>>();
  auto cols = make_shared<AlDomain<int>>();
  
  for (int i = 0; i < 5; i++)
  {
    (void)rows->insert(i);
    (void)cols->insert(i);
  }
  
  // Create 5x5 sparse matrix
  Matrix<int, int, double> M(rows, cols);
  
  print_subsection("Creating a Sparse 5x5 Matrix");
  cout << "Matrix is 5x5 but only non-zero entries are stored" << endl;
  
  // Set diagonal and a few off-diagonal entries
  M.set_entry(0, 0, 1.0);
  M.set_entry(1, 1, 2.0);
  M.set_entry(2, 2, 3.0);
  M.set_entry(3, 3, 4.0);
  M.set_entry(4, 4, 5.0);
  // Some off-diagonal
  M.set_entry(0, 1, 0.5);
  M.set_entry(1, 2, 0.5);
  
  cout << "\nNon-zero entries:" << endl;
  // Print diagonal
  for (int i = 0; i < 5; i++)
    cout << "  M[" << i << "][" << i << "] = " << M.get_entry(i, i) << endl;
  cout << "  M[0][1] = " << M.get_entry(0, 1) << endl;
  cout << "  M[1][2] = " << M.get_entry(1, 2) << endl;
  
  // Count entries manually
  size_t nnz = 7;  // We set 7 entries above
  cout << "\nStored entries: " << nnz << " out of " << (5*5) 
       << " possible (" << (100.0 * nnz / 25) << "% fill)" << endl;
}

// =============================================================================
// 4. Named Row/Column Matrix (Practical Example)
// =============================================================================

void demo_named_matrix()
{
  print_section("NAMED ROW/COLUMN MATRIX");
  
  cout << "Sparse matrices are perfect for real-world data with named dimensions" << endl;
  
  // Sales data: products vs stores
  auto products = make_shared<AlDomain<string>>();
  (void)products->insert("Laptop");
  (void)products->insert("Phone");
  (void)products->insert("Tablet");
  (void)products->insert("Monitor");
  
  auto stores = make_shared<AlDomain<string>>();
  (void)stores->insert("BOG");  // Bogota
  (void)stores->insert("MED");  // Medellin
  (void)stores->insert("CAL");  // Cali
  (void)stores->insert("BAQ");  // Barranquilla
  
  Matrix<string, string, double> sales(products, stores);
  
  // Set some sales data (not all products sold in all stores)
  sales.set_entry("Laptop", "BOG", 150);
  sales.set_entry("Laptop", "MED", 120);
  sales.set_entry("Phone", "BOG", 500);
  sales.set_entry("Phone", "MED", 450);
  sales.set_entry("Phone", "CAL", 300);
  sales.set_entry("Phone", "BAQ", 200);
  sales.set_entry("Tablet", "BOG", 80);
  sales.set_entry("Monitor", "MED", 50);
  
  cout << "\nSales data (only non-zero entries stored):" << endl;
  cout << string(45, '-') << endl;
  
  // Print some entries manually
  cout << "  Laptop @ BOG: " << sales.get_entry("Laptop", "BOG") << " units" << endl;
  cout << "  Laptop @ MED: " << sales.get_entry("Laptop", "MED") << " units" << endl;
  cout << "  Phone @ BOG:  " << sales.get_entry("Phone", "BOG") << " units" << endl;
  cout << "  Phone @ MED:  " << sales.get_entry("Phone", "MED") << " units" << endl;
  cout << "  Phone @ CAL:  " << sales.get_entry("Phone", "CAL") << " units" << endl;
  cout << "  Phone @ BAQ:  " << sales.get_entry("Phone", "BAQ") << " units" << endl;
  cout << "  Tablet @ BOG: " << sales.get_entry("Tablet", "BOG") << " units" << endl;
  cout << "  Monitor @ MED:" << sales.get_entry("Monitor", "MED") << " units" << endl;
  
  // Get a column (all products for a store)
  print_subsection("Column Extraction: Bogota Store Sales");
  auto bog_sales = sales.get_col_vector("BOG");
  cout << "Products sold in Bogota:" << endl;
  for (auto it = bog_sales.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  " << setw(10) << left << entry.first 
         << ": " << entry.second << " units" << endl;
  }
  
  // Get a row (all stores for a product)
  print_subsection("Row Extraction: Phone Sales");
  auto phone_sales = sales.get_row_vector("Phone");
  cout << "Phone sales by store:" << endl;
  for (auto it = phone_sales.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  " << setw(5) << left << entry.first 
         << ": " << entry.second << " units" << endl;
  }
}

// =============================================================================
// 5. Vector Arithmetic
// =============================================================================

void demo_vector_arithmetic()
{
  print_section("VECTOR ARITHMETIC");
  
  // Create two vectors
  auto domain = make_shared<AlDomain<int>>();
  for (int i = 0; i < 5; i++)
    (void)domain->insert(i);
  
  Vector<int, double> a(domain);
  Vector<int, double> b(domain);
  
  a.set_entry(0, 1.0);
  a.set_entry(1, 2.0);
  a.set_entry(2, 3.0);
  
  b.set_entry(1, 1.5);
  b.set_entry(2, 2.5);
  b.set_entry(3, 3.5);
  
  auto print_vec = [](const string& name, const Vector<int, double>& v) {
    cout << name << ": ";
    for (auto it = v.get_it(); it.has_curr(); it.next())
    {
      auto e = it.get_curr();
      cout << "(" << e.first << ":" << e.second << ") ";
    }
    cout << endl;
  };
  
  print_vec("a", a);
  print_vec("b", b);
  
  // Addition
  print_subsection("Vector Addition");
  auto sum = a + b;
  print_vec("a + b", sum);
  cout << "  Note: sparse + sparse = sparse (union of indices)" << endl;
  
  // Scalar multiplication
  print_subsection("Scalar Multiplication");
  auto scaled = a * 2.0;
  print_vec("a * 2", scaled);
  
  // Subtraction
  print_subsection("Vector Subtraction");
  auto diff = a - b;
  print_vec("a - b", diff);
}

// =============================================================================
// 6. Practical Application: Graph Adjacency Matrix
// =============================================================================

void demo_adjacency_matrix()
{
  print_section("PRACTICAL: GRAPH ADJACENCY MATRIX");
  
  cout << "Sparse matrices are ideal for representing graphs" << endl;
  cout << "Most graphs are sparse (few edges vs all possible edges)" << endl;
  
  // Create nodes domain
  auto nodes = make_shared<AlDomain<string>>();
  (void)nodes->insert("A");
  (void)nodes->insert("B");
  (void)nodes->insert("C");
  (void)nodes->insert("D");
  (void)nodes->insert("E");
  
  // Adjacency matrix with weights
  Matrix<string, string, double> adj(nodes, nodes);
  
  // Add edges (directed graph)
  adj.set_entry("A", "B", 4);
  adj.set_entry("A", "C", 2);
  adj.set_entry("B", "C", 1);
  adj.set_entry("B", "D", 5);
  adj.set_entry("C", "D", 8);
  adj.set_entry("C", "E", 10);
  adj.set_entry("D", "E", 2);
  
  cout << "\nGraph edges (weighted):" << endl;
  cout << "  A -> B : weight " << adj.get_entry("A", "B") << endl;
  cout << "  A -> C : weight " << adj.get_entry("A", "C") << endl;
  cout << "  B -> C : weight " << adj.get_entry("B", "C") << endl;
  cout << "  B -> D : weight " << adj.get_entry("B", "D") << endl;
  cout << "  C -> D : weight " << adj.get_entry("C", "D") << endl;
  cout << "  C -> E : weight " << adj.get_entry("C", "E") << endl;
  cout << "  D -> E : weight " << adj.get_entry("D", "E") << endl;
  
  // Count edges
  size_t num_edges = 7;  // We set 7 edges above
  
  size_t num_nodes = 5;
  size_t max_edges = num_nodes * num_nodes;
  
  cout << "\nGraph statistics:" << endl;
  cout << "  Nodes: " << num_nodes << endl;
  cout << "  Edges: " << num_edges << endl;
  cout << "  Density: " << fixed << setprecision(1) 
       << (100.0 * num_edges / max_edges) << "%" << endl;
  cout << "  Memory: storing only " << num_edges 
       << " values instead of " << max_edges << endl;
  
  // Find outgoing edges from a node
  print_subsection("Outgoing Edges from Node B");
  auto b_out = adj.get_row_vector("B");
  for (auto it = b_out.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  B -> " << entry.first << " : " << entry.second << endl;
  }
}

// =============================================================================
// 7. Epsilon Tolerance
// =============================================================================

void demo_epsilon()
{
  print_section("EPSILON TOLERANCE");
  
  cout << "Sparse vectors/matrices automatically handle near-zero values" << endl;
  
  auto domain = make_shared<AlDomain<int>>();
  for (int i = 0; i < 5; i++)
    (void)domain->insert(i);
  
  Vector<int, double> v(domain);
  
  // Get current epsilon
  cout << "\nDefault epsilon: " << v.get_epsilon() << endl;
  
  v.set_entry(0, 1.0);
  v.set_entry(1, 0.0001);  // Very small
  v.set_entry(2, 0.0);     // Exact zero
  
  cout << "\nAfter setting v[0]=1.0, v[1]=0.0001, v[2]=0.0:" << endl;
  for (auto it = v.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  v[" << entry.first << "] = " << entry.second << endl;
  }
  
  cout << "\nNote: v[2]=0.0 is not stored (it's zero)" << endl;
  cout << "v[1]=0.0001 is stored because |0.0001| > epsilon" << endl;
  
  // Change epsilon
  print_subsection("Changing Epsilon");
  v.set_epsilon(0.001);
  cout << "New epsilon: " << v.get_epsilon() << endl;
  
  // Now set a value smaller than epsilon
  v.set_entry(3, 0.0005);
  cout << "\nAfter setting v[3]=0.0005 (< epsilon):" << endl;
  for (auto it = v.get_it(); it.has_curr(); it.next())
  {
    auto entry = it.get_curr();
    cout << "  v[" << entry.first << "] = " << entry.second << endl;
  }
  cout << "\nv[3] was not stored because 0.0005 < 0.001 (epsilon)" << endl;
}

// =============================================================================
// 8. Initializer List Construction
// =============================================================================

void demo_initializer_list()
{
  print_section("INITIALIZER LIST CONSTRUCTION");
  
  cout << "Matrices can be constructed directly from initializer lists" << endl;
  cout << "(similar to how you'd write a matrix on paper)" << endl;
  
  // Create domains for a 3x3 matrix
  auto rows = make_shared<AlDomain<int>>();
  auto cols = make_shared<AlDomain<int>>();
  for (int i = 0; i < 3; i++)
  {
    (void)rows->insert(i);
    (void)cols->insert(i);
  }
  
  // Create matrix with initializer list
  Matrix<int, int, double> A(rows, cols, {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
  });
  
  cout << "\nMatrix A (from initializer list):" << endl;
  cout << A.to_str() << endl;
  
  // Sparse matrix - zeros are not stored
  print_subsection("Sparse Initializer List");
  Matrix<int, int, double> B(rows, cols, {
    {1, 0, 0},
    {0, 2, 0},
    {0, 0, 3}
  });
  
  cout << "\nDiagonal matrix B (zeros not stored internally):" << endl;
  cout << B.to_str() << endl;
  cout << "\nNote: only 3 entries are stored (the diagonal)" << endl;
}

// =============================================================================
// 9. Matrix Transpose
// =============================================================================

void demo_transpose()
{
  print_section("MATRIX TRANSPOSE");
  
  cout << "The transpose() method swaps rows and columns" << endl;
  
  // Create a 2x3 matrix
  auto rows = make_shared<AlDomain<string>>();
  auto cols = make_shared<AlDomain<string>>();
  (void)rows->insert("r0");
  (void)rows->insert("r1");
  (void)cols->insert("c0");
  (void)cols->insert("c1");
  (void)cols->insert("c2");
  
  Matrix<string, string, double> M(rows, cols);
  M.set_entry("r0", "c0", 1); M.set_entry("r0", "c1", 2); M.set_entry("r0", "c2", 3);
  M.set_entry("r1", "c0", 4); M.set_entry("r1", "c1", 5); M.set_entry("r1", "c2", 6);
  
  cout << "\nOriginal matrix M (2x3):" << endl;
  cout << M.to_str() << endl;
  
  auto Mt = M.transpose();
  cout << "\nTranspose M^T (3x2):" << endl;
  cout << Mt.to_str() << endl;
  
  cout << "\nProperty: M[r][c] = M^T[c][r]" << endl;
  cout << "  M[r0][c2] = " << M.get_entry("r0", "c2") << endl;
  cout << "  M^T[c2][r0] = " << Mt.get_entry("c2", "r0") << endl;
}

// =============================================================================
// 10. Identity Matrix
// =============================================================================

void demo_identity()
{
  print_section("IDENTITY MATRIX");
  
  cout << "The identity() method creates I (only for square matrices)" << endl;
  cout << "Property: A * I = I * A = A" << endl;
  
  auto domain = make_shared<AlDomain<int>>();
  for (int i = 0; i < 4; i++)
    (void)domain->insert(i);
  
  Matrix<int, int, double> A(domain, domain, {
    {2, 3, 0, 0},
    {0, 1, 4, 0},
    {0, 0, 5, 6},
    {7, 0, 0, 8}
  });
  
  cout << "\nMatrix A:" << endl;
  cout << A.to_str() << endl;
  
  auto I = A.identity();
  cout << "\nIdentity matrix I:" << endl;
  cout << I.to_str() << endl;
  
  cout << "\nIdentity is sparse: only diagonal entries stored" << endl;
}

// =============================================================================
// 11. Matrix-Vector Multiplication Methods
// =============================================================================

void demo_matrix_vector_mult()
{
  print_section("MATRIX-VECTOR MULTIPLICATION");
  
  cout << "Multiple methods available for M * v:" << endl;
  cout << "  - Linear combination (default)" << endl;
  cout << "  - Dot product" << endl;
  cout << "  - Sparse iteration" << endl;
  
  auto rows = make_shared<AlDomain<int>>();
  auto cols = make_shared<AlDomain<int>>();
  for (int i = 0; i < 3; i++)
  {
    (void)rows->insert(i);
    (void)cols->insert(i);
  }
  
  Matrix<int, int, double> M(rows, cols, {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
  });
  
  Vector<int, double> v(cols);
  v.set_entry(0, 1);
  v.set_entry(1, 0);  // Sparse: only 2 non-zero entries
  v.set_entry(2, 2);
  
  cout << "\nMatrix M:" << endl;
  cout << M.to_str() << endl;
  
  cout << "\nVector v: (0:1, 2:2) -- note v[1]=0 not stored" << endl;
  
  // Method 1: Linear combination (operator*)
  auto r1 = M * v;  // Uses mult_matrix_vector_linear_comb
  cout << "\nM * v (linear combination):" << endl;
  for (auto it = r1.get_it(); it.has_curr(); it.next())
  {
    auto e = it.get_curr();
    cout << "  [" << e.first << "] = " << e.second << endl;
  }
  
  // Method 2: Dot product
  auto r2 = M.mult_matrix_vector_dot_product(v);
  cout << "\nM * v (dot product):" << endl;
  for (auto it = r2.get_it(); it.has_curr(); it.next())
  {
    auto e = it.get_curr();
    cout << "  [" << e.first << "] = " << e.second << endl;
  }
  
  // Method 3: Sparse iteration
  auto r3 = M.mult_matrix_vector_sparse(v);
  cout << "\nM * v (sparse):" << endl;
  for (auto it = r3.get_it(); it.has_curr(); it.next())
  {
    auto e = it.get_curr();
    cout << "  [" << e.first << "] = " << e.second << endl;
  }
  
  cout << "\nAll methods produce same result (choose based on sparsity)" << endl;
  
  // Vector * Matrix
  print_subsection("Vector-Matrix Multiplication (v * M)");
  Vector<int, double> u(rows);
  u.set_entry(0, 1.5);
  u.set_entry(2, 3);
  
  cout << "\nVector u: (0:1.5, 2:3)" << endl;
  auto r4 = u * M;  // Uses mult_vector_matrix_linear_comb
  cout << "\nu * M:" << endl;
  for (auto it = r4.get_it(); it.has_curr(); it.next())
  {
    auto e = it.get_curr();
    cout << "  [" << e.first << "] = " << e.second << endl;
  }
}

// =============================================================================
// 12. Matrix-Matrix Multiplication
// =============================================================================

void demo_matrix_mult()
{
  print_section("MATRIX-MATRIX MULTIPLICATION");
  
  cout << "Two methods for A * B:" << endl;
  cout << "  - vector_matrix_mult: row_i * B for each row" << endl;
  cout << "  - matrix_vector_mult: A * col_j for each column" << endl;
  
  // IMPORTANT: For multiplication A*B, column domain of A must be
  // identical (same shared_ptr) to row domain of B
  auto rowsA = make_shared<AlDomain<int>>();
  auto shared_domain = make_shared<AlDomain<int>>();  // shared between A cols and B rows
  auto colsB = make_shared<AlDomain<int>>();
  
  for (int i = 0; i < 2; i++)
    (void)rowsA->insert(i);
  for (int i = 0; i < 3; i++)
    (void)shared_domain->insert(i);
  for (int i = 0; i < 2; i++)
    (void)colsB->insert(i);
  
  // A is 2x3
  Matrix<int, int, double> A(rowsA, shared_domain, {
    {1, 2, 3},
    {4, 5, 6}
  });
  
  // B is 3x2 (rows domain = A's column domain)
  Matrix<int, int, double> B(shared_domain, colsB, {
    {7, 8},
    {9, 10},
    {11, 12}
  });
  
  cout << "\nMatrix A (2x3):" << endl;
  cout << A.to_str() << endl;
  
  cout << "\nMatrix B (3x2):" << endl;
  cout << B.to_str() << endl;
  
  // Method 1: vector_matrix_mult
  auto C1 = A.vector_matrix_mult(B);
  cout << "\nA * B (vector_matrix_mult):" << endl;
  cout << C1.to_str() << endl;
  
  // Method 2: matrix_vector_mult
  auto C2 = A.matrix_vector_mult(B);
  cout << "\nA * B (matrix_vector_mult):" << endl;
  cout << C2.to_str() << endl;
  
  cout << "\nBoth methods yield the same result" << endl;
  cout << "Verified: C1 == C2 ? " << (C1 == C2 ? "YES" : "NO") << endl;
}

// =============================================================================
// 13. Outer Product
// =============================================================================

void demo_outer_product()
{
  print_section("OUTER PRODUCT");
  
  cout << "The outer product of vectors u and v produces a matrix M" << endl;
  cout << "where M[i][j] = u[i] * v[j]" << endl;
  
  auto dom_u = make_shared<AlDomain<string>>();
  (void)dom_u->insert("x");
  (void)dom_u->insert("y");
  (void)dom_u->insert("z");
  
  auto dom_v = make_shared<AlDomain<string>>();
  (void)dom_v->insert("a");
  (void)dom_v->insert("b");
  
  Vector<string, double> u(dom_u);
  u.set_entry("x", 1);
  u.set_entry("y", 2);
  u.set_entry("z", 3);
  
  Vector<string, double> v(dom_v);
  v.set_entry("a", 4);
  v.set_entry("b", 5);
  
  cout << "\nVector u: x=1, y=2, z=3" << endl;
  cout << "Vector v: a=4, b=5" << endl;
  
  auto M = outer_product(u, v);
  cout << "\nOuter product u ⊗ v:" << endl;
  cout << M.to_str() << endl;
  
  cout << "\nVerification:" << endl;
  cout << "  M[y][a] = u[y] * v[a] = 2 * 4 = " << M.get_entry("y", "a") << endl;
  cout << "  M[z][b] = u[z] * v[b] = 3 * 5 = " << M.get_entry("z", "b") << endl;
}

// =============================================================================
// 14. Matrix Comparison
// =============================================================================

void demo_comparison()
{
  print_section("MATRIX COMPARISON");
  
  cout << "Matrices can be compared with == and != operators" << endl;
  cout << "Comparison uses epsilon tolerance for floating-point values" << endl;
  
  auto domain = make_shared<AlDomain<int>>();
  for (int i = 0; i < 2; i++)
    (void)domain->insert(i);
  
  Matrix<int, int, double> A(domain, domain, {
    {1.0, 2.0},
    {3.0, 4.0}
  });
  
  Matrix<int, int, double> B(domain, domain, {
    {1.0, 2.0},
    {3.0, 4.0}
  });
  
  Matrix<int, int, double> C(domain, domain, {
    {1.0, 2.0},
    {3.0, 4.001}  // Slightly different
  });
  
  cout << "\nMatrix A:" << endl;
  cout << A.to_str() << endl;
  
  cout << "\nMatrix B (same as A):" << endl;
  cout << B.to_str() << endl;
  
  cout << "\nMatrix C (A[1][1] is 4.001):" << endl;
  cout << C.to_str() << endl;
  
  cout << "\nComparisons:" << endl;
  cout << "  A == B ? " << (A == B ? "YES" : "NO") << endl;
  cout << "  A == C ? " << (A == C ? "YES" : "NO") << endl;
  cout << "  A != C ? " << (A != C ? "YES" : "NO") << endl;
  
  // Epsilon-sensitive comparison
  print_subsection("Epsilon-Sensitive Comparison");
  Matrix<int, int, double> D(domain, domain, {
    {1.0, 2.0},
    {3.0, 4.0 + 1e-8}  // Within default epsilon (1e-7)
  });
  
  cout << "\nMatrix D has A[1][1] = 4.0 + 1e-8 (within epsilon=1e-7)" << endl;
  cout << "  A == D ? " << (A == D ? "YES" : "NO") << " (within epsilon tolerance)" << endl;
}

// =============================================================================
// 15. Matrix Arithmetic Operations
// =============================================================================

void demo_matrix_arithmetic()
{
  print_section("MATRIX ARITHMETIC OPERATIONS");
  
  cout << "Supported: addition (+, +=), subtraction (-, -=), scalar mult (*)" << endl;
  
  auto domain = make_shared<AlDomain<int>>();
  for (int i = 0; i < 2; i++)
    (void)domain->insert(i);
  
  Matrix<int, int, double> A(domain, domain, {
    {1, 2},
    {3, 4}
  });
  
  Matrix<int, int, double> B(domain, domain, {
    {5, 6},
    {7, 8}
  });
  
  cout << "\nMatrix A:" << endl;
  cout << A.to_str() << endl;
  
  cout << "\nMatrix B:" << endl;
  cout << B.to_str() << endl;
  
  // Addition
  auto sum = A + B;
  cout << "\nA + B:" << endl;
  cout << sum.to_str() << endl;
  
  // Subtraction
  auto diff = A - B;
  cout << "\nA - B:" << endl;
  cout << diff.to_str() << endl;
  
  // Scalar multiplication
  auto scaled = 2.5 * A;
  cout << "\n2.5 * A:" << endl;
  cout << scaled.to_str() << endl;
  
  // In-place modification
  print_subsection("In-Place Operations");
  Matrix<int, int, double> C = A;
  C += B;
  cout << "\nC = A; C += B:" << endl;
  cout << C.to_str() << endl;
  
  C.mult_by_scalar(0.5);
  cout << "\nC.mult_by_scalar(0.5):" << endl;
  cout << C.to_str() << endl;
}

// =============================================================================
// 16. Row and Column Operations
// =============================================================================

void demo_row_col_operations()
{
  print_section("ROW AND COLUMN OPERATIONS");
  
  cout << "Methods for working with rows and columns:" << endl;
  cout << "  - get_row_vector(), get_col_vector()" << endl;
  cout << "  - set_vector_as_row(), set_vector_as_col()" << endl;
  cout << "  - to_rowlist(), to_collist()" << endl;
  cout << "  - get_row_as_list(), get_col_as_list()" << endl;
  
  auto rows = make_shared<AlDomain<string>>();
  auto cols = make_shared<AlDomain<string>>();
  (void)rows->insert("A");
  (void)rows->insert("B");
  (void)cols->insert("X");
  (void)cols->insert("Y");
  (void)cols->insert("Z");
  
  Matrix<string, string, double> M(rows, cols);
  M.set_entry("A", "X", 1); M.set_entry("A", "Y", 2); M.set_entry("A", "Z", 3);
  M.set_entry("B", "X", 4); M.set_entry("B", "Y", 5); M.set_entry("B", "Z", 6);
  
  cout << "\nMatrix M:" << endl;
  cout << M.to_str() << endl;
  
  // Get row as vector
  print_subsection("Get Row as Vector");
  auto row_A = M.get_row_vector("A");
  cout << "Row 'A' as vector:" << endl;
  for (auto it = row_A.get_it(); it.has_curr(); it.next())
  {
    auto e = it.get_curr();
    cout << "  [" << e.first << "] = " << e.second << endl;
  }
  
  // Get column as vector
  print_subsection("Get Column as Vector");
  auto col_Y = M.get_col_vector("Y");
  cout << "Column 'Y' as vector:" << endl;
  for (auto it = col_Y.get_it(); it.has_curr(); it.next())
  {
    auto e = it.get_curr();
    cout << "  [" << e.first << "] = " << e.second << endl;
  }
  
  // Set a row from vector
  print_subsection("Set Row from Vector");
  Vector<string, double> new_row(cols);
  new_row.set_entry("X", 10);
  new_row.set_entry("Y", 20);
  new_row.set_entry("Z", 30);
  
  M.set_vector_as_row("B", new_row);
  cout << "After setting row 'B' to (10, 20, 30):" << endl;
  cout << M.to_str() << endl;
  
  // Convert to list of row vectors
  print_subsection("Convert to List of Rows");
  auto row_list = M.to_rowlist();
  cout << "Matrix as list of " << row_list.size() << " row vectors" << endl;
}

// =============================================================================
// 17. Practical: Linear System Example
// =============================================================================

void demo_linear_system()
{
  print_section("PRACTICAL: LINEAR EQUATIONS");
  
  cout << "Using sparse matrices to represent linear systems" << endl;
  cout << "\nSystem:  2x + 3y = 13" << endl;
  cout << "         4x -  y = 5" << endl;
  cout << "Solution: x=2, y=3" << endl;
  
  auto vars = make_shared<AlDomain<string>>();
  (void)vars->insert("x");
  (void)vars->insert("y");
  
  auto eqs = make_shared<AlDomain<string>>();
  (void)eqs->insert("eq1");
  (void)eqs->insert("eq2");
  
  // Coefficient matrix A
  Matrix<string, string, double> A(eqs, vars);
  A.set_entry("eq1", "x", 2);  A.set_entry("eq1", "y", 3);
  A.set_entry("eq2", "x", 4);  A.set_entry("eq2", "y", -1);
  
  cout << "\nCoefficient matrix A:" << endl;
  cout << A.to_str() << endl;
  
  // Solution vector
  Vector<string, double> solution(vars);
  solution.set_entry("x", 2);
  solution.set_entry("y", 3);
  
  cout << "\nSolution vector: x=2, y=3" << endl;
  
  // Verify: A * solution = b
  auto b = A * solution;
  cout << "\nVerification A * solution:" << endl;
  for (auto it = b.get_it(); it.has_curr(); it.next())
  {
    auto e = it.get_curr();
    cout << "  " << e.first << " = " << e.second << endl;
  }
  cout << "\nExpected: eq1=13, eq2=5 ✓" << endl;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Sparse Matrix and Vector example for Aleph-w.\n"
      "Demonstrates domain-based indexing and efficient sparse storage.",
      ' ', "1.0"
    );
    
    cmd.parse(argc, argv);
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "    ALEPH-W SPARSE MATRIX AND VECTOR EXAMPLE\n";
    cout << "============================================================\n";
    
    demo_sparse_vector();
    demo_string_indexed_vector();
    demo_sparse_matrix();
    demo_named_matrix();
    demo_vector_arithmetic();
    demo_adjacency_matrix();
    demo_epsilon();
    demo_initializer_list();
    demo_transpose();
    demo_identity();
    demo_matrix_vector_mult();
    demo_matrix_mult();
    demo_outer_product();
    demo_comparison();
    demo_matrix_arithmetic();
    demo_row_col_operations();
    demo_linear_system();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Sparse Matrix and Vector demo completed!\n";
    cout << string(60, '=') << "\n\n";
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
    return 1;
  }
  catch (exception& e)
  {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
}
