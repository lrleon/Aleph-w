/**
 * @file matrix_example.C
 * @brief Example demonstrating sparse matrices and vectors in Aleph-w
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
### Dense Matrix
 * - Stores ALL elements (even zeros)
 * - Memory: O(rows × cols)
 * - Access: O(1) - direct indexing
 * - **Best for**: Dense data, small matrices
 *
### Sparse Matrix (This Example)
 * - Stores ONLY non-zero entries
 * - Memory: O(nonzero_entries)
 * - Access: O(log n) - search in sparse structure
 * - **Best for**: Sparse data, large matrices with few non-zeros
 *
### When to Use Sparse?
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
## Key Features
 *
### Sparse Storage
 *
 * - **Efficient storage**: Only non-zero entries stored
 * - **Automatic cleanup**: Near-zero entries removed (epsilon tolerance)
 * - **Memory efficient**: O(nonzeros) instead of O(rows×cols)
 * - **Flexible**: Can handle very large matrices
 *
### Domain-Based Indexing
 *
 * Unlike traditional matrices (indexed 0..n-1), Aleph-w matrices support:
 * - **String indices**: `matrix["row_name"]["col_name"]`
 * - **Custom types**: Any comparable type as index
 * - **Named dimensions**: Rows/columns have meaningful names
 *
 * **Example**:
 * ```cpp
 * Matrix<double> sales;
 * sales["January"]["ProductA"] = 1000.0;
 * sales["February"]["ProductB"] = 2000.0;
 * ```
 *
### Operations Supported
 *
 * - **Element access**: Get/set individual elements
 * - **Arithmetic**: Add, subtract, multiply matrices/vectors
 * - **Scalar operations**: Multiply by scalar
 * - **Row/column extraction**: Get entire row or column as vector
 * - **Transposition**: Swap rows and columns
 *
## Applications
 *
### Scientific Computing
 * - **Linear systems**: Sparse linear algebra
 * - **Finite element methods**: Sparse stiffness matrices
 * - **Graph algorithms**: Adjacency matrices (usually sparse)
 *
### Data Analysis
 * - **Feature matrices**: Machine learning (many zeros)
 * - **Transaction data**: User-item matrices (sparse)
 * - **Time series**: Sparse temporal data
 *
### Business Applications
 * - **Sales data**: Products × Time periods
 * - **Resource allocation**: Resources × Tasks
 * - **Financial modeling**: Instruments × Time periods
 *
## Complexity
 *
 * | Operation | Dense | Sparse | Notes |
 * |-----------|-------|--------|-------|
 * | Storage | O(n²) | O(nonzeros) | Sparse wins for sparse data |
 * | Access | O(1) | O(log n) | Dense faster, but sparse uses less memory |
 * | Addition | O(n²) | O(nonzeros) | Sparse much faster |
 * | Multiplication | O(n³) | O(nonzeros₁ × nonzeros₂) | Depends on sparsity |
 *
## Domain-Based Indexing Benefits
 *
### Readability
 * ```cpp
 * // Traditional (unclear)
 * matrix[0][5] = 100;
 *
 * // Domain-based (clear)
 * matrix["January"]["Sales"] = 100;
 * ```
 *
### Flexibility
 * - Add new rows/columns dynamically
 * - No need to pre-allocate all dimensions
 * - Easy to work with real-world data
 *
## Usage
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
