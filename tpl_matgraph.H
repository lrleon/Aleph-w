/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

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

/** @file tpl_matgraph.H
 *  @brief Adjacency matrix representations for graphs.
 *
 *  This file provides several adjacency matrix representations for graphs:
 *
 *  - **Map_Matrix_Graph**: Maps to an existing adjacency list graph.
 *    Entries are pointers to arcs in the list representation.
 *
 *  - **Matrix_Graph**: Stores copies of node/arc attributes from a graph.
 *    Entries contain the arc attribute type.
 *
 *  - **Ady_Mat**: Auxiliary matrix with user-defined entry type.
 *    Useful for algorithms like Floyd-Warshall that need custom data.
 *
 *  - **Bit_Mat_Graph**: Compact bit matrix for connectivity queries.
 *    Uses 1 bit per entry for minimal memory usage.
 *
 *  ## When to Use Adjacency Matrices
 *
 *  Use adjacency matrices when:
 *  - The graph is dense (many edges relative to vertices)
 *  - O(1) edge existence queries are needed
 *  - Algorithms like Floyd-Warshall are applied
 *
 *  For sparse graphs, prefer adjacency list representations.
 *
 *  @see tpl_graph.H Adjacency list graph representation
 *  @see Floyd_Warshall.H Floyd-Warshall algorithm
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_MAT_GRAPH_H
#define TPL_MAT_GRAPH_H

#include <tpl_graph.H>
#include <tpl_sort_utils.H>
#include <ah-errors.H>

namespace Aleph
{

// =============================================================================
// Internal Helper Functions
// =============================================================================

namespace matgraph_detail
{

/**
 * @brief Get node pointer from sorted node array by index.
 * @tparam GT Graph type
 * @param nodes Sorted array of node pointers
 * @param i Index to retrieve
 * @return Pointer to node at index i
 * @throw std::out_of_range if i >= nodes.size()
 */
template <typename GT>
[[nodiscard]] inline typename GT::Node*
get_node(DynArray<typename GT::Node*>& nodes, long i)
{
  ah_out_of_range_error_if(i < 0 or static_cast<size_t>(i) >= nodes.size())
    << "Node index " << i << " out of range [0, " << nodes.size() << ")";
  return nodes.access(i);
}

/**
 * @brief Get node pointer from sorted node array by index (const version).
 * @tparam GT Graph type
 * @param nodes Sorted array of node pointers
 * @param i Index to retrieve
 * @return Pointer to node at index i
 * @throw std::out_of_range if i >= nodes.size()
 */
template <typename GT>
[[nodiscard]] inline typename GT::Node*
get_node(const DynArray<typename GT::Node*>& nodes, long i)
{
  ah_out_of_range_error_if(i < 0 or static_cast<size_t>(i) >= nodes.size())
    << "Node index " << i << " out of range [0, " << nodes.size() << ")";
  return nodes.access(i);
}

/**
 * @brief Find index of node in sorted array using binary search.
 * @tparam GT Graph type
 * @param nodes Sorted array of node pointers
 * @param n Number of nodes
 * @param p Node pointer to find
 * @return Index of node, or -1 if not found
 */
template <typename GT>
[[nodiscard]] inline long
node_index(const DynArray<typename GT::Node*>& nodes, long n, typename GT::Node* p)
{
  return Aleph::binary_search(nodes, p, 0, n - 1);
}

/**
 * @brief Convert 2D matrix indices to 1D array index.
 * @param i Row index
 * @param j Column index
 * @param n Matrix dimension
 * @return Linear index for row-major storage
 * @throw std::out_of_range if indices are out of bounds
 */
[[nodiscard]] inline long
index_array(long i, long j, long n) noexcept
{
  return i + j * n;
}

/**
 * @brief Validate and convert indices to linear index.
 * @param i Row index
 * @param j Column index
 * @param n Matrix dimension
 * @return Linear index
 * @throw std::out_of_range if i or j >= n
 */
[[nodiscard]] inline long
checked_index_array(long i, long j, long n)
{
  ah_out_of_range_error_if(i < 0 or i >= n or j < 0 or j >= n)
    << "Matrix index (" << i << ", " << j << ") out of range [0, " << n << ")";
  return index_array(i, j, n);
}

/**
 * @brief Read matrix entry if it exists.
 * @tparam Entry Entry type
 * @param mat Dynamic array storing matrix
 * @param i Row index
 * @param j Column index
 * @param n Matrix dimension
 * @return Pointer to entry if exists, nullptr otherwise
 */
template <typename Entry>
[[nodiscard]] inline Entry*
read_matrix(const DynArray<Entry>& mat, long i, long j, long n)
{
  const long index = checked_index_array(i, j, n);
  if (not mat.exist(index))
    return nullptr;
  return &const_cast<Entry&>(mat.access(index));
}

/**
 * @brief Write entry to matrix.
 * @tparam Entry Entry type
 * @param mat Dynamic array storing matrix
 * @param i Row index
 * @param j Column index
 * @param n Matrix dimension
 * @param entry Value to write
 */
template <typename Entry>
inline void
write_matrix(DynArray<Entry>& mat, long i, long j, long n, const Entry& entry)
{
  mat[checked_index_array(i, j, n)] = entry;
}

} // namespace matgraph_detail


// =============================================================================
// Map_Matrix_Graph - Mapped Adjacency Matrix
// =============================================================================

/**
 * @brief Adjacency matrix mapped to an adjacency list graph.
 *
 * Map_Matrix_Graph provides an adjacency matrix view of a graph that is
 * represented using adjacency lists. Matrix entries are pointers to arcs
 * in the underlying graph. A nullptr entry indicates no arc exists.
 *
 * ## Key Features
 *
 * - O(1) edge existence check by indices
 * - O(log n) edge check by node pointers (binary search)
 * - Memory proportional to number of arcs (sparse storage)
 * - Read-only view of graph topology
 *
 * ## Usage Example
 *
 * ```cpp
 * List_Graph<int, double> g;
 * // ... build graph ...
 *
 * Map_Matrix_Graph<List_Graph<int,double>> mat(g);
 *
 * // Access by indices
 * auto arc = mat(0, 1);  // Arc from node 0 to node 1
 *
 * // Access by node pointers
 * auto arc2 = mat(src_node, tgt_node);
 *
 * // Get node by index
 * auto node = mat(2);
 * ```
 *
 * @warning Changes to the underlying graph are NOT reflected in the matrix.
 *          Rebuild the matrix after modifying the graph.
 *
 * @warning Not suitable for multigraphs or multidigraphs.
 *
 * @tparam GT Graph type (derived from List_Graph or similar)
 * @tparam SA Arc filter (default: show all arcs)
 *
 * @see Matrix_Graph For a matrix that stores copies of attributes
 * @see Ady_Mat For auxiliary matrices with custom entry types
 * @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Map_Matrix_Graph
{
public:
  /// The associated graph type
  using Graph_Type = GT;

  /// Node attribute type from the graph
  using Node_Type = typename GT::Node_Type;

  /// Arc attribute type from the graph
  using Arc_Type = typename GT::Arc_Type;

  /// Node type from the graph
  using Node = typename GT::Node;

  /// Arc type from the graph
  using Arc = typename GT::Arc;

private:
  struct Mat_Entry
  {
    Arc* arc = nullptr;
    Mat_Entry() = default;
    explicit Mat_Entry(Arc* a) : arc(a) {}
  };

  DynArray<Node*>    nodes;
  GT*                lgraph = nullptr;
  mutable size_t     num_nodes = 0;
  SA                 sa;
  DynArray<Mat_Entry> mat;

public:
  /**
   * @brief Copy graph structure to matrix.
   *
   * Rebuilds the matrix from the given graph. Previous contents are cleared.
   *
   * @param g Graph to copy from
   */
  void copy_list_graph(GT& g);

  /**
   * @brief Construct from adjacency list graph.
   * @param g Source graph
   * @param __sa Arc filter (rvalue reference)
   * @throw std::bad_alloc if memory allocation fails
   */
  Map_Matrix_Graph(GT& g, SA&& __sa = SA())
    : lgraph(&g), num_nodes(g.get_num_nodes()), sa(std::move(__sa))
  {
    copy_list_graph(g);
  }

  /**
   * @brief Construct from adjacency list graph.
   * @param g Source graph
   * @param __sa Arc filter (lvalue reference)
   * @throw std::bad_alloc if memory allocation fails
   */
  Map_Matrix_Graph(GT& g, SA& __sa)
    : lgraph(&g), num_nodes(g.get_num_nodes()), sa(__sa)
  {
    copy_list_graph(g);
  }

  /// Copy constructor
  Map_Matrix_Graph(Map_Matrix_Graph& other)
    : lgraph(other.lgraph), num_nodes(other.num_nodes), sa(other.sa)
  {
    copy_list_graph(*other.lgraph);
  }

  /// Copy assignment
  Map_Matrix_Graph& operator=(Map_Matrix_Graph& other);

  /// Assign from graph
  Map_Matrix_Graph& operator=(GT& g);

  /**
   * @brief Get node pointer by index.
   * @param i Node index
   * @return Pointer to node
   * @throw std::out_of_range if i >= num_nodes
   */
  [[nodiscard]] Node* operator()(long i)
  {
    return matgraph_detail::get_node<GT>(nodes, i);
  }

  /**
   * @brief Get index of node.
   * @param node Node pointer
   * @return Index of node in matrix
   * @note O(log n) binary search
   */
  [[nodiscard]] long operator()(Node* node) const
  {
    return matgraph_detail::node_index<GT>(nodes, num_nodes, node);
  }

  /**
   * @brief Get arc by node pointers.
   * @param src_node Source node
   * @param tgt_node Target node
   * @return Arc pointer, or nullptr if no arc exists
   * @note O(log n) for each node lookup
   */
  [[nodiscard]] Arc* operator()(Node* src_node, Node* tgt_node) const
  {
    Mat_Entry* entry = matgraph_detail::read_matrix<Mat_Entry>(
      mat,
      matgraph_detail::node_index<GT>(nodes, num_nodes, src_node),
      matgraph_detail::node_index<GT>(nodes, num_nodes, tgt_node),
      num_nodes);
    return entry ? entry->arc : nullptr;
  }

  /**
   * @brief Get arc by indices.
   * @param i Source node index
   * @param j Target node index
   * @return Arc pointer, or nullptr if no arc exists
   * @throw std::out_of_range if indices out of bounds
   * @note O(1) access
   */
  [[nodiscard]] Arc* operator()(long i, long j) const
  {
    Mat_Entry* entry = matgraph_detail::read_matrix<Mat_Entry>(mat, i, j, num_nodes);
    return entry ? entry->arc : nullptr;
  }

  /// Get reference to underlying graph
  [[nodiscard]] GT& get_list_graph() noexcept { return *lgraph; }

  /// Get reference to underlying graph (const)
  [[nodiscard]] const GT& get_list_graph() const noexcept { return *lgraph; }

  /// Get number of nodes (matrix dimension)
  [[nodiscard]] size_t get_num_nodes() const noexcept { return num_nodes; }
};

// Implementation

template <class GT, class SA>
Map_Matrix_Graph<GT, SA>&
Map_Matrix_Graph<GT, SA>::operator=(Map_Matrix_Graph& other)
{
  if (this != &other)
    copy_list_graph(*other.lgraph);
  return *this;
}

template <class GT, class SA>
Map_Matrix_Graph<GT, SA>&
Map_Matrix_Graph<GT, SA>::operator=(GT& g)
{
  copy_list_graph(g);
  return *this;
}

template <class GT, class SA>
void
Map_Matrix_Graph<GT, SA>::copy_list_graph(GT& g)
{
  lgraph = &g;
  num_nodes = g.get_num_nodes();
  nodes.cut();
  mat.cut();

  // Copy nodes to array
  long i = 0;
  for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
    nodes[i] = it.get_current_node_ne();

  // Sort for binary search
  quicksort(nodes);

  // Copy arcs to matrix
  for (typename GT::Node_Iterator nit(g); nit.has_curr(); nit.next_ne())
    {
      Node* src = nit.get_current_node_ne();
      const long src_idx = matgraph_detail::node_index<GT>(nodes, num_nodes, src);

      for (Node_Arc_Iterator<GT, SA> ait(src, sa); ait.has_curr(); ait.next_ne())
        {
          Arc* arc = ait.get_current_arc_ne();
          Node* tgt = g.get_connected_node(arc, src);
          const long tgt_idx = matgraph_detail::node_index<GT>(nodes, num_nodes, tgt);
          matgraph_detail::write_matrix<Mat_Entry>(mat, src_idx, tgt_idx, num_nodes, Mat_Entry(arc));
        }
    }
}


// =============================================================================
// Matrix_Graph - Attribute-Copying Adjacency Matrix
// =============================================================================

/**
 * @brief Adjacency matrix storing copies of graph attributes.
 *
 * Matrix_Graph creates a self-contained adjacency matrix by copying the
 * node and arc attributes from an adjacency list graph. Unlike Map_Matrix_Graph,
 * this matrix does not maintain references to the original graph.
 *
 * ## Key Features
 *
 * - Stores actual attribute values, not pointers
 * - Modifiable entries (can add arcs by setting entries)
 * - null_value() indicates absence of arc
 * - Memory proportional to number of arcs (sparse storage)
 *
 * ## Usage Example
 *
 * ```cpp
 * List_Graph<std::string, int> g;
 * // ... build graph ...
 *
 * // -1 indicates no arc
 * Matrix_Graph<List_Graph<std::string,int>> mat(g, -1);
 *
 * // Read arc weight
 * int weight = mat(0, 1);  // Returns -1 if no arc
 *
 * // Add/modify arc
 * mat(0, 2) = 5;  // Set arc weight
 * ```
 *
 * @note Only stores primitive attribute values, not derived node/arc types.
 *
 * @tparam GT Graph type
 * @tparam SA Arc filter (default: show all arcs)
 *
 * @see Map_Matrix_Graph For pointer-based matrix
 * @see Ady_Mat For auxiliary matrices with custom types
 * @ingroup Graphs
 */
template <typename GT, class SA = Dft_Show_Arc<GT>>
class Matrix_Graph
{
public:
  /// Graph type
  using Graph_Type = GT;

  /// Node attribute type
  using Node_Type = typename GT::Node_Type;

  /// Arc attribute type
  using Arc_Type = typename GT::Arc_Type;

  /// Node type
  using Node = typename GT::Node;

  /// Arc type
  using Arc = typename GT::Arc;

private:
  DynArray<Node_Type> nodes;
  DynArray<Arc_Type>  arcs;
  mutable size_t      n = 0;
  mutable Arc_Type    Null_Value{};
  SA                  sa;

  void copy(Matrix_Graph& other)
  {
    if (this == &other)
      return;

    n = other.n;
    Null_Value = other.Null_Value;
    nodes.cut();
    arcs.cut();
    arcs.set_default_initial_value(Null_Value);

    for (size_t i = 0; i < n; ++i)
      {
        nodes[i] = other.nodes[i];
        for (size_t j = 0; j < n; ++j)
          {
            const long idx = matgraph_detail::index_array(i, j, n);
            if (other.arcs.exist(idx))
              arcs.touch(idx) = other.arcs.access(idx);
          }
      }
  }

  void copy(GT& g)
  {
    n = g.get_num_nodes();
    DynArray<typename GT::Node*> ptr;

    long i = 0;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
      ptr[i] = it.get_current_node_ne();

    quicksort(ptr);
    arcs.set_default_initial_value(Null_Value);

    for (size_t idx = 0; idx < n; ++idx)
      {
        typename GT::Node* src = ptr[idx];
        nodes[idx] = src->get_info();
        for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Arc* arc = it.get_current_arc_ne();
            typename GT::Node* tgt = it.get_tgt_node();
            const long j = matgraph_detail::node_index<GT>(ptr, n, tgt);
            arcs[matgraph_detail::index_array(idx, j, n)] = arc->get_info();
          }
      }
  }

public:
  /// Get number of nodes (matrix dimension)
  [[nodiscard]] size_t get_num_nodes() const noexcept { return n; }

  /// Get the null value indicating no arc
  [[nodiscard]] const Arc_Type& null_value() const noexcept { return Null_Value; }

  /**
   * @brief Construct from graph with null value.
   * @param g Source graph
   * @param null Value indicating absence of arc
   * @param __sa Arc filter
   * @throw std::bad_alloc if memory allocation fails
   */
  Matrix_Graph(GT& g, const Arc_Type& null, SA&& __sa = SA())
    : Null_Value(null), sa(std::move(__sa))
  {
    copy(g);
  }

  Matrix_Graph(GT& g, const Arc_Type& null, SA& __sa)
    : Null_Value(null), sa(__sa)
  {
    copy(g);
  }

  /// Copy constructor
  Matrix_Graph(Matrix_Graph& other) { copy(other); }

  /// Copy assignment
  Matrix_Graph& operator=(Matrix_Graph& other)
  {
    copy(other);
    return *this;
  }

  /// Assign from graph
  Matrix_Graph& operator=(GT& g)
  {
    copy(g);
    return *this;
  }

  /**
   * @brief Get arc attribute by indices (const).
   * @param i Source index
   * @param j Target index
   * @return Arc attribute, or null_value() if no arc
   * @throw std::out_of_range if indices out of bounds
   */
  [[nodiscard]] const Arc_Type& operator()(long i, long j) const
  {
    const long idx = matgraph_detail::checked_index_array(i, j, n);
    return arcs.exist(idx) ? arcs.access(idx) : Null_Value;
  }

  /**
   * @brief Get node attribute by index.
   * @param i Node index
   * @return Reference to node attribute
   * @throw std::out_of_range if i >= n
   */
  [[nodiscard]] Node_Type& operator()(long i) const
  {
    ah_out_of_range_error_if(i < 0 or static_cast<size_t>(i) >= n)
      << "Node index " << i << " out of range";
    return const_cast<Node_Type&>(nodes.access(i));
  }

  /**
   * @brief Get/set arc attribute by indices.
   * @param i Source index
   * @param j Target index
   * @return Reference to arc entry (creates if needed)
   */
  [[nodiscard]] Arc_Type& operator()(long i, long j)
  {
    return arcs.touch(matgraph_detail::checked_index_array(i, j, n));
  }
};


// =============================================================================
// Ady_Mat - Auxiliary Adjacency Matrix
// =============================================================================

/**
 * @brief Auxiliary adjacency matrix with custom entry type.
 *
 * Ady_Mat provides an adjacency matrix with a user-defined entry type,
 * useful for algorithms that need temporary storage or computations
 * different from the graph's native attributes.
 *
 * ## Key Features
 *
 * - Custom entry type (not tied to graph arc type)
 * - Associated with an adjacency list graph
 * - Sparse storage (memory proportional to non-null entries)
 * - Operations to iterate over arcs
 *
 * ## Common Uses
 *
 * - Floyd-Warshall shortest path algorithm
 * - Transitive closure computation
 * - Any algorithm needing matrix operations on graphs
 *
 * ## Usage Example
 *
 * ```cpp
 * List_Digraph<std::string, int> g;
 * // ... build graph ...
 *
 * // Matrix storing distances (int) with infinity as null
 * Ady_Mat<decltype(g), int> dist(g, INT_MAX);
 *
 * // Initialize from arcs
 * dist.operate_all_arcs_list_graph<Init_Dist>();
 *
 * // Access entries
 * dist(0, 1) = 5;
 * int d = dist(0, 1);
 * ```
 *
 * @tparam GT Graph type
 * @tparam __Entry_Type Type of matrix entries
 * @tparam SA Arc filter (default: show all arcs)
 *
 * @see Floyd_Warshall.H Usage in Floyd-Warshall algorithm
 * @ingroup Graphs
 */
template <class GT, typename __Entry_Type, class SA = Dft_Show_Arc<GT>>
class Ady_Mat
{
public:
  /// Graph type
  using Graph_Type = GT;

  /// Entry type stored in matrix
  using Entry_Type = __Entry_Type;

  /// Node attribute type from graph
  using Node_Type = typename GT::Node_Type;

  /// Arc attribute type from graph
  using Arc_Type = typename GT::Arc_Type;

  /// Node type from graph
  using Node = typename GT::Node;

  /// Arc type from graph
  using Arc = typename GT::Arc;

private:
  GT*                  lgraph = nullptr;
  DynArray<Node*>      nodes;
  DynArray<Entry_Type> mat;
  mutable size_t       num_nodes = 0;
  mutable Entry_Type   Null_Value{};

  void test_same_graph(const Ady_Mat& other) const
  {
    ah_domain_error_if(lgraph != other.lgraph)
      << "Matrices refer to different graphs";
  }

  void copy_nodes(GT& g)
  {
    lgraph = &g;
    num_nodes = g.get_num_nodes();
    nodes.cut();

    long i = 0;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++i)
      nodes[i] = it.get_current_node_ne();

    quicksort(nodes);
  }

  Ady_Mat& copy(Ady_Mat& other)
  {
    if (this == &other)
      return *this;

    test_same_graph(other);
    Null_Value = other.Null_Value;
    copy_nodes(*other.lgraph);
    return *this;
  }

public:
  /**
   * @brief Get node pointer by index.
   * @param i Node index
   * @return Pointer to node in underlying graph
   * @throw std::out_of_range if index out of bounds
   */
  [[nodiscard]] Node* operator()(long i)
  {
    return matgraph_detail::get_node<GT>(nodes, i);
  }

  /**
   * @brief Get node pointer by index (const).
   * @param i Node index
   * @return Pointer to node in underlying graph
   * @throw std::out_of_range if index out of bounds
   */
  [[nodiscard]] Node* operator()(long i) const
  {
    return matgraph_detail::get_node<GT>(nodes, i);
  }

  /**
   * @brief Get index of node.
   * @param node Node pointer
   * @return Index in matrix
   * @note O(log n) binary search
   */
  [[nodiscard]] long operator()(Node* node) const
  {
    return matgraph_detail::node_index<GT>(nodes, num_nodes, node);
  }

  /**
   * @brief Access entry by indices (modifiable).
   * @param i Row index
   * @param j Column index
   * @return Reference to entry (creates if needed)
   * @throw std::out_of_range if indices out of bounds
   */
  [[nodiscard]] Entry_Type& operator()(long i, long j)
  {
    return mat.touch(matgraph_detail::checked_index_array(i, j, num_nodes));
  }

  /**
   * @brief Access entry by indices (const).
   * @param i Row index
   * @param j Column index
   * @return Entry value, or null_value() if not set
   * @throw std::out_of_range if indices out of bounds
   */
  [[nodiscard]] const Entry_Type& operator()(long i, long j) const
  {
    const long index = matgraph_detail::checked_index_array(i, j, num_nodes);
    return mat.exist(index) ? mat.access(index) : Null_Value;
  }

  /**
   * @brief Access entry by node pointers (modifiable).
   * @param src Source node
   * @param tgt Target node
   * @return Reference to entry
   * @note O(log n) for each node lookup
   */
  [[nodiscard]] Entry_Type& operator()(Node* src, Node* tgt)
  {
    return (*this)(matgraph_detail::node_index<GT>(nodes, num_nodes, src),
                   matgraph_detail::node_index<GT>(nodes, num_nodes, tgt));
  }

  /**
   * @brief Access entry by node pointers (const).
   * @param src Source node
   * @param tgt Target node
   * @return Entry value, or null_value() if not set
   * @note O(log n) for each node lookup
   */
  [[nodiscard]] const Entry_Type& operator()(Node* src, Node* tgt) const
  {
    return (*this)(matgraph_detail::node_index<GT>(nodes, num_nodes, src),
                   matgraph_detail::node_index<GT>(nodes, num_nodes, tgt));
  }

  /// Get reference to underlying graph
  [[nodiscard]] GT& get_list_graph() noexcept { return *lgraph; }

  /// Get const reference to underlying graph
  [[nodiscard]] const GT& get_list_graph() const noexcept { return *lgraph; }

  /// Get the null value
  [[nodiscard]] const Entry_Type& null_value() const noexcept { return Null_Value; }

  /// Get number of nodes (matrix dimension)
  [[nodiscard]] size_t get_num_nodes() const noexcept { return num_nodes; }

  /**
   * @brief Construct from graph without null value.
   *
   * Creates matrix with uninitialized entries. Use set_null_value()
   * to define the null value.
   *
   * @param g Source graph
   * @throw std::bad_alloc if memory allocation fails
   */
  explicit Ady_Mat(GT& g)
    : lgraph(&g), num_nodes(g.get_num_nodes())
  {
    copy_nodes(g);
  }

  /**
   * @brief Construct from graph with null value.
   * @param g Source graph
   * @param null Value indicating no entry
   * @throw std::bad_alloc if memory allocation fails
   */
  Ady_Mat(GT& g, const Entry_Type& null)
    : lgraph(&g), num_nodes(g.get_num_nodes()), Null_Value(null)
  {
    copy_nodes(g);
  }

  /// Set the null value
  void set_null_value(const Entry_Type& null) noexcept { Null_Value = null; }

  /// Copy constructor
  Ady_Mat(Ady_Mat& other) { copy(other); }

  /// Copy assignment
  Ady_Mat& operator=(Ady_Mat& other) { return copy(other); }

  /**
   * @brief Apply operation to all arcs from the graph.
   *
   * Iterates over all arcs in the underlying graph and invokes:
   * ```
   * Operation()(mat, arc, i, j, entry)
   * ```
   *
   * Where:
   * - mat: reference to this matrix
   * - arc: pointer to arc in graph
   * - i, j: indices in matrix
   * - entry: reference to matrix entry
   *
   * @tparam Operation Functor type with above signature
   */
  template <class Operation>
  void operate_all_arcs_list_graph();

  /**
   * @brief Apply operation to all arcs with user data.
   *
   * Same as operate_all_arcs_list_graph() but passes additional pointer:
   * ```
   * Operation()(mat, arc, i, j, entry, ptr)
   * ```
   *
   * @tparam Operation Functor type
   * @param ptr User data pointer
   */
  template <class Operation>
  void operate_all_arcs_list_graph(void* ptr);

  /**
   * @brief Apply operation to all matrix entries.
   *
   * Iterates over all n×n entries and invokes:
   * ```
   * Operation()(mat, src, tgt, i, j, entry)
   * ```
   *
   * @warning This forces all entries to be allocated.
   *
   * @tparam Operation Functor type
   */
  template <class Operation>
  void operate_all_arcs_matrix();

  /**
   * @brief Apply operation to all matrix entries with user data.
   * @tparam Operation Functor type
   * @param ptr User data pointer
   */
  template <class Operation>
  void operate_all_arcs_matrix(void* ptr);
};

// Implementation of operate_* methods

template <class GT, typename __Entry_Type, class SA>
template <class Operation>
void
Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_list_graph()
{
  for (typename GT::Node_Iterator nit(*lgraph); nit.has_curr(); nit.next_ne())
    {
      Node* src = nit.get_current_node_ne();
      const long src_idx = matgraph_detail::node_index<Graph_Type>(nodes, num_nodes, src);

      for (Node_Arc_Iterator<GT, SA> at(src); at.has_curr(); at.next_ne())
        {
          Arc* arc = at.get_current_arc_ne();
          Node* tgt = at.get_tgt_node();
          const long tgt_idx =
            matgraph_detail::node_index<Graph_Type>(nodes, num_nodes, tgt);
          Entry_Type& entry =
            mat.touch(matgraph_detail::index_array(src_idx, tgt_idx, num_nodes));
          Operation()(*this, arc, src_idx, tgt_idx, entry);
        }
    }
}

template <class GT, typename __Entry_Type, class SA>
template <class Operation>
void
Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_list_graph(void* ptr)
{
  for (typename GT::Node_Iterator nit(*lgraph); nit.has_curr(); nit.next_ne())
    {
      Node* src = nit.get_current_node_ne();
      const long src_idx = matgraph_detail::node_index<Graph_Type>(nodes, num_nodes, src);

      for (Node_Arc_Iterator<GT, SA> at(src); at.has_curr(); at.next_ne())
        {
          Arc* arc = at.get_current_arc_ne();
          Node* tgt = lgraph->get_tgt_node(arc);
          const long tgt_idx = matgraph_detail::node_index<Graph_Type>(nodes, num_nodes, tgt);
          Entry_Type& entry =
            mat.touch(matgraph_detail::index_array(src_idx, tgt_idx, num_nodes));
          Operation()(*this, arc, src_idx, tgt_idx, entry, ptr);
        }
    }
}

template <class GT, typename __Entry_Type, class SA>
template <class Operation>
void
Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_matrix()
{
  const long n = num_nodes;
  for (long s = 0; s < n; ++s)
    {
      Node* src_node = matgraph_detail::get_node<GT>(nodes, s);
      for (long t = 0; t < n; ++t)
        {
          Node* tgt_node = matgraph_detail::get_node<GT>(nodes, t);
          Entry_Type& entry = mat.touch(matgraph_detail::index_array(s, t, num_nodes));
          Operation()(*this, src_node, tgt_node, s, t, entry);
        }
    }
}

template <class GT, typename __Entry_Type, class SA>
template <class Operation>
void
Ady_Mat<GT, __Entry_Type, SA>::operate_all_arcs_matrix(void* ptr)
{
  const long n = num_nodes;
  for (long s = 0; s < n; ++s)
    {
      Node* src_node = matgraph_detail::get_node<GT>(nodes, s);
      for (long t = 0; t < n; ++t)
        {
          Node* tgt_node = matgraph_detail::get_node<GT>(nodes, t);
          Entry_Type& entry = mat.touch(matgraph_detail::index_array(s, t, num_nodes));
          Operation()(*this, src_node, tgt_node, s, t, entry, ptr);
        }
    }
}


// =============================================================================
// Bit_Mat_Graph - Bit Matrix for Connectivity
// =============================================================================

/**
 * @brief Bit matrix for graph connectivity.
 *
 * Bit_Mat_Graph provides a compact bit matrix representation for
 * connectivity queries. Each entry uses only 1 bit: 1 indicates an
 * arc exists, 0 indicates no arc.
 *
 * ## Key Features
 *
 * - Minimal memory: n²/8 bytes for n nodes
 * - O(1) edge existence check
 * - Can be created without a graph (just dimension)
 * - Modifiable entries
 *
 * ## Usage Example
 *
 * ```cpp
 * List_Graph<int, int> g;
 * // ... build graph ...
 *
 * Bit_Mat_Graph<decltype(g)> bits(g);
 *
 * // Check connectivity
 * if (bits(0, 1))
 *   std::cout << "Edge exists\n";
 *
 * // Modify
 * bits(0, 2) = 1;  // Add edge
 * bits(0, 1) = 0;  // Remove edge
 * ```
 *
 * @tparam GT Graph type
 * @tparam SA Arc filter
 *
 * @see BitArray Underlying bit array implementation
 * @see warshall_compute_transitive_closure()
 * @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Bit_Mat_Graph
{
public:
  /// Graph type
  using Graph_Type = GT;

  /// Node type
  using Node = typename GT::Node;

  /// Arc type
  using Arc = typename GT::Arc;

private:
  BitArray                     bit_array;
  GT*                          lgraph = nullptr;
  DynArray<typename GT::Node*> nodes;
  mutable size_t               n = 0;

  void copy_list_graph(GT& g)
  {
    n = g.get_num_nodes();
    nodes.cut();

    long i = 0;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      nodes[i++] = static_cast<typename GT::Node*>(it.get_current_node_ne());

    quicksort(nodes);

    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
        typename GT::Node* src = it.get_current_node_ne();
        const size_t src_idx = matgraph_detail::node_index<Graph_Type>(nodes, n, src);

        for (Node_Arc_Iterator<GT, SA> jt(src); jt.has_curr(); jt.next_ne())
          {
            typename GT::Node* tgt = jt.get_tgt_node_ne();
            const size_t tgt_idx = matgraph_detail::node_index<Graph_Type>(nodes, n, tgt);
            bit_array[matgraph_detail::index_array(src_idx, tgt_idx, n)] = 1;
          }
      }
  }

  /// Proxy for bit access
  struct Proxy
  {
    BitArray&    bit_array;
    const size_t bit_index;

    Proxy(Bit_Mat_Graph& bitmat, long i, long j)
      : bit_array(bitmat.bit_array),
        bit_index(matgraph_detail::index_array(i, j, bitmat.n))
    {}

    Proxy& operator=(const Proxy& other)
    {
      bit_array[bit_index] = other.bit_array[other.bit_index];
      return *this;
    }

    Proxy& operator=(int value)
    {
      bit_array[bit_index] = value;
      return *this;
    }

    operator int() const { return bit_array[bit_index]; }
  };

public:
  /// Get number of nodes (matrix dimension)
  [[nodiscard]] size_t get_num_nodes() const noexcept { return n; }

  /// Default constructor (empty matrix)
  Bit_Mat_Graph() = default;

  /**
   * @brief Construct from graph.
   * @param g Source graph
   */
  explicit Bit_Mat_Graph(GT& g)
    : bit_array(g.get_num_nodes() * g.get_num_nodes()), lgraph(&g)
  {
    copy_list_graph(g);
  }

  /// Copy constructor
  Bit_Mat_Graph(const Bit_Mat_Graph& other)
    : bit_array(other.bit_array), lgraph(other.lgraph),
      nodes(other.nodes), n(other.n)
  {}

  /**
   * @brief Construct with dimension only.
   * @param dim Matrix dimension
   */
  explicit Bit_Mat_Graph(size_t dim)
    : bit_array(dim * dim), nodes(dim), n(dim)
  {}

  /**
   * @brief Associate with a graph.
   * @param g Graph to associate
   */
  void set_list_graph(GT& g)
  {
    const size_t new_n = g.get_num_nodes();
    bit_array.set_size(new_n * new_n);
    lgraph = &g;
    copy_list_graph(g);
  }

  /// Get pointer to associated graph (nullptr if none)
  [[nodiscard]] GT* get_list_graph() noexcept { return lgraph; }

  /// Get pointer to associated graph (const)
  [[nodiscard]] const GT* get_list_graph() const noexcept { return lgraph; }

  /// Copy assignment
  Bit_Mat_Graph& operator=(const Bit_Mat_Graph& other)
  {
    if (this != &other)
      {
        lgraph = other.lgraph;
        bit_array = other.bit_array;
        nodes = other.nodes;
        n = other.n;
      }
    return *this;
  }

  /// Assign from graph
  Bit_Mat_Graph& operator=(GT& g)
  {
    if (&g != lgraph)
        set_list_graph(g);
    return *this;
  }

  /**
   * @brief Get node by index.
   * @param i Node index
   * @return Pointer to node
   * @throw std::domain_error if no graph associated
   * @throw std::out_of_range if index out of bounds
   */
  [[nodiscard]] Node* operator()(long i)
  {
    ah_domain_error_if(lgraph == nullptr)
      << "No graph associated with bit matrix";
    return matgraph_detail::get_node<GT>(nodes, i);
  }

  /**
   * @brief Get index of node.
   * @param node Node pointer
   * @return Index in matrix
   * @throw std::domain_error if no graph associated
   */
  [[nodiscard]] long operator()(Node* node) const
  {
    ah_domain_error_if(lgraph == nullptr)
      << "No graph associated with bit matrix";
    return matgraph_detail::node_index<GT>(nodes, n, node);
  }

  /// Access bit by indices
  Proxy operator()(long i, long j)
  {
    return Proxy(*this, i, j);
  }

  /// Access bit by indices (const)
  Proxy operator()(long i, long j) const
  {
    return Proxy(const_cast<Bit_Mat_Graph&>(*this), i, j);
  }

  /// Access bit by node pointers
  Proxy operator()(Node* src_node, Node* tgt_node)
  {
    ah_domain_error_if(lgraph == nullptr)
      << "No graph associated with bit matrix";
    return Proxy(*this,
                 matgraph_detail::node_index<GT>(nodes, n, src_node),
                 matgraph_detail::node_index<GT>(nodes, n, tgt_node));
  }

  /// Access bit by node pointers (const)
  Proxy operator()(Node* src_node, Node* tgt_node) const
  {
    ah_domain_error_if(lgraph == nullptr)
      << "No graph associated with bit matrix";
    return Proxy(const_cast<Bit_Mat_Graph&>(*this),
                 matgraph_detail::node_index<GT>(nodes, n, src_node),
                 matgraph_detail::node_index<GT>(nodes, n, tgt_node));
  }
};


// =============================================================================
// Legacy Helper Functions (for backward compatibility)
// =============================================================================

/// @cond INTERNAL

template <typename GT>
[[nodiscard]] inline typename GT::Node*
get_node(DynArray<typename GT::Node*>& nodes, long i)
{
  return matgraph_detail::get_node<GT>(nodes, i);
}

template <typename GT>
[[nodiscard]] inline long
node_index(const DynArray<typename GT::Node*>& nodes, long n, typename GT::Node* p)
{
  return matgraph_detail::node_index<GT>(nodes, n, p);
}

[[nodiscard]] inline long
index_array(long i, long j, long n)
{
  return matgraph_detail::index_array(i, j, n);
}

template <typename Entry>
[[nodiscard]] inline Entry*
read_matrix(const DynArray<Entry>& mat, long i, long j, long n)
{
  return matgraph_detail::read_matrix(mat, i, j, n);
}

template <typename Entry>
inline void
write_matrix(DynArray<Entry>& mat, long i, long j, long n, const Entry& entry)
{
  matgraph_detail::write_matrix(mat, i, j, n, entry);
}

/// @endcond

} // end namespace Aleph

#endif // TPL_MAT_GRAPH_H
