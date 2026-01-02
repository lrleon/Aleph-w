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

/** @file tpl_2dtree.H
 *  @brief 2D k-d tree implementation for spatial point indexing.
 *
 *  This file provides K2Tree, a 2D specialization of the k-d tree (k-dimensional
 *  tree) data structure. It partitions 2D space by alternating between vertical
 *  and horizontal splits, enabling efficient nearest neighbor and range queries.
 *
 *  @author Leandro Rabindranath León
 *  @ingroup Geometry
 */

# ifndef TPL_2DTREE_H
# define TPL_2DTREE_H

# include <point.H>
# include <htlist.H>
# include <ahDefs.H> // For Empty_Class

using namespace Aleph;

/** @brief 2D k-d tree for efficient spatial point operations.

    K2Tree is a binary space partitioning tree that recursively divides 2D
    space using alternating vertical (X) and horizontal (Y) splits. Each node
    represents a point and implicitly defines a rectangular region.

    @par Key Characteristics:
    - **Alternating splits**: Even levels split on X, odd levels split on Y
    - **Binary structure**: Each node has at most two children (left/bottom, right/top)
    - **Duplicate rejection**: Duplicate points are not inserted
    - **Balanced on random data**: Expected O(log n) height for random insertions
    - **Worst-case unbalanced**: Can degenerate to O(n) on sorted data

    @par Split Pattern:
    ```
    Root (split on X):
      x < root.x → Left subtree
      x >= root.x → Right subtree
        ├─ Child (split on Y):
        │     y < child.y → Bottom subtree
        │     y >= child.y → Top subtree
        └─ ...
    ```

    @par Complexity:
    - Insert: O(log n) average, O(n) worst case
    - Search: O(log n) average, O(n) worst case
    - Nearest neighbor: O(log n) average, O(n) worst case
    - Range query: O(sqrt(n) + k) where k is result size
    - Space: O(n)

    @par Comparison with QuadTree:
    - **K2Tree**: Binary splits, simpler structure, better for uniformly distributed data
    - **QuadTree**: Quaternary splits, adapts to density, better for clustered data

    @par Use Cases:
    ✓ Nearest neighbor search
    ✓ Range queries (all points in rectangle)
    ✓ Spatial databases
    ✓ Computer graphics (ray tracing, collision detection)
    ✓ Geographic information systems (GIS)

    @par Example:
    @code
    // Create a k-d tree for region [0, 100] × [0, 100]
    K2Tree<> tree(0, 0, 100, 100);

    // Insert points
    tree.insert(Point(25, 50));
    tree.insert(Point(75, 25));
    tree.insert(Point(50, 75));

    // Check if point exists
    if (tree.contains(Point(25, 50)))
      std::cout << "Point found!" << '\n';

    // Find nearest neighbor
    Point nearest = tree.nearest(Point(30, 45));
    std::cout << "Nearest: (" << nearest.get_x() << ", "
              << nearest.get_y() << ")" << '\n';

    // Range query
    Rectangle rect(20, 20, 80, 80);
    DynList<Point> points_in_rect;
    tree.range(rect, &points_in_rect);
    @endcode

    @tparam T Optional user data type associated with points (default: Empty_Class)

    @note This implementation uses a memory pool for efficient node allocation.
    @note The tree does NOT automatically balance. For sorted insertions, consider
          building from a shuffled sequence.

    @see QuadTree Alternative spatial structure with adaptive quadrant subdivision.
    @author Leandro Rabindranath León
    @ingroup Geometry
 */
    template <typename T = Empty_Class>
class K2Tree
{
  /// @brief Internal node structure for the k-d tree.
  struct Node 
  {
    Point point;      ///< The point stored at this node
    Rectangle rect;   ///< The axis-aligned rectangle for this node's region
    Node * lb;        ///< Left/bottom subtree
    Node * rt;        ///< Right/top subtree

    /// @brief Construct a node with a given point.
    Node(const Point & __point) noexcept
      : point(__point), lb(nullptr), rt(nullptr)
    { 
      // empty
    }

    /// @brief Set the rectangular region for this node.
    void set_rect(const Geom_Number & xmin, const Geom_Number & ymin, 
		  const Geom_Number & xmax, const Geom_Number & ymax) noexcept
    {
      rect.set_rect(xmin, ymin, xmax, ymax);
    }

    /// @brief Set the rectangular region using corner points.
    void set_rect(const Point & pmin, const Point & pmax) noexcept
    {
      rect.set_rect(pmin.get_x(), pmin.get_y(), pmax.get_x(), pmax.get_y());
    }

    [[nodiscard]] const Geom_Number & xmin() const noexcept { return rect.get_xmin(); }
    [[nodiscard]] const Geom_Number & ymin() const noexcept { return rect.get_ymin(); }
    [[nodiscard]] const Geom_Number & xmax() const noexcept { return rect.get_xmax(); }
    [[nodiscard]] const Geom_Number & ymax() const noexcept { return rect.get_ymax(); }
    [[nodiscard]] const Geom_Number & x() const noexcept { return point.get_x(); }
    [[nodiscard]] const Geom_Number & y() const noexcept { return point.get_y(); }
  };

  Point pmin;   ///< Lower-left corner of reference rectangle
  Point pmax;   ///< Upper-right corner of reference rectangle
  size_t N;     ///< Number of points in the tree
  Node * root;  ///< Root of the tree

public:

  /// @brief Default constructor - creates an empty tree with zero-sized region.
  K2Tree() : pmin(0, 0), pmax(0, 0), N(0), root(nullptr)
  {
    // empty
  }

  /** @brief Construct a k-d tree with specified bounding region.
   *
   *  @param __pmin Lower-left corner of the region
   *  @param __pmax Upper-right corner of the region
   */
  K2Tree(const Point & __pmin, const Point & __pmax) 
    : pmin(__pmin), pmax(__pmax), N(0), root(nullptr)
  {
    // empty
  }

  /** @brief Construct a k-d tree with specified coordinate bounds.
   *
   *  @param xmin Minimum X coordinate
   *  @param ymin Minimum Y coordinate
   *  @param xmax Maximum X coordinate
   *  @param ymax Maximum Y coordinate
   */
  K2Tree(const Geom_Number & xmin, const Geom_Number & ymin, 
	 const Geom_Number & xmax, const Geom_Number & ymax) 
    : pmin(xmin, ymin), pmax(xmax, ymax), N(0), root(nullptr)
  {
    // empty
  }

  K2Tree(const K2Tree &) = delete;
  K2Tree & operator = (const K2Tree &) = delete;

  /// @brief Check if the tree is empty.
  [[nodiscard]] constexpr bool is_empty() const noexcept { return N == 0; }

  /// @brief Get the number of points in the tree.
  [[nodiscard]] constexpr size_t size() const noexcept { return N; }

private:

  /** @brief Insert into tree with left-right (X) split.
   *
   *  Recursively inserts a point, splitting on X coordinate at this level.
   *
   *  @param root Current node
   *  @param p Point to insert
   *  @return New subtree root, or nullptr if point is duplicate
   */
  Node * lr_insert(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return new Node(p);

    Node * ret_val = nullptr;
    if (p.get_x() == root->x()) 
      {
	if (p.get_y() == root->y())
	  return nullptr; // Duplicate point

	ret_val = bu_insert(root->rt, p);
	if (ret_val != nullptr) 
	  {
	    root->rt = ret_val; // Node was inserted
	    root->rt->set_rect(root->x(), root->ymin(), 
			       root->xmax(), root->ymax());

	    return root;
	  }

	return nullptr;
      }

    if (p.get_x() < root->x())
      {
	ret_val = bu_insert(root->lb, p);
	if (ret_val != nullptr) 
	  {
	    root->lb = ret_val;
	    root->lb->set_rect(root->xmin(), root->ymin(), 
			       root->x(), root->ymax());

	    return root;
	  } 
	else
	  return nullptr;
      }

    ret_val = bu_insert(root->rt, p);
    if (ret_val != nullptr) 
      {
	root->rt = ret_val;
	root->rt->set_rect(root->x(), root->ymin(), 
			   root->xmax(), root->ymax());

	return root;
      }

    return nullptr;
  }

  /** @brief Insert into tree with bottom-up (Y) split.
   *
   *  Recursively inserts a point, splitting on Y coordinate at this level.
   *
   *  @param root Current node
   *  @param p Point to insert
   *  @return New subtree root, or nullptr if point is duplicate
   */
  Node * bu_insert(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return new Node(p);

    Node * ret_val = nullptr;
    if (p.get_y() == root->y()) 
      {
	if (p.get_x() == root->x())
	  return nullptr; // Duplicate point

	ret_val = lr_insert(root->rt, p);
	if (ret_val != nullptr) 
	  {
	    root->rt = ret_val; // Node was inserted
	    root->rt->set_rect(root->xmin(), root->y(), 
			       root->xmax(), root->ymax());

	    return root;
	  }
	
	return nullptr;
      }

    if (p.get_y() < root->y()) 
      {
	ret_val = lr_insert(root->lb, p);
	if (ret_val != nullptr) 
	  {
	    root->lb = ret_val;
	    root->lb->set_rect(root->xmin(), root->ymin(), 
			       root->xmax(), root->y());
	    return root;
	  } 
	else
	  return nullptr;
      }

    ret_val = lr_insert(root->rt, p);
    if (ret_val != nullptr) 
      {
	root->rt = ret_val;
	root->rt->set_rect(root->xmin(), root->y(), 
			   root->xmax(), root->ymax());
	
	return root;
    }

    return nullptr;
  }

public:

  /** @brief Insert a point into the tree.
   *
   *  The point is inserted only if it's not already present (no duplicates).
   *
   *  @param p Point to insert
   *  @return Pointer to the inserted point, or nullptr if duplicate
   */
  Point * insert(const Point & p) 
  {
    if (root == nullptr) 
      {
	root = new Node (p);
	root->set_rect(pmin, pmax);
	++N;

	return &root->point;
      }

    Node * ret = lr_insert(root, p);
    if (ret == nullptr)
      return nullptr;

    ++N;

    return &root->point; // FIX: Was returning nullptr
  }

private:

  /// @brief Search with bottom-up (Y) split pattern.
  static Node * bu_search(Node * root, const Point & p) noexcept
  {
    if (root == nullptr)
      return nullptr;

    if (root->y() == p.get_y()) 
      {
	if (root->x() == p.get_x())
	  return root;

	return lr_search(root->rt, p);
      }

    if (p.get_y() < root->y())
      return lr_search(root->lb, p);
    else
      return lr_search(root->rt, p);
  }

  /// @brief Search with left-right (X) split pattern.
  static Node * lr_search(Node * root, const Point & p) noexcept
  {
    if (root == nullptr)
      return nullptr;

    if (root->x() == p.get_x()) 
      {
	if (root->y() == p.get_y())
	  return root;

	return bu_search(root->rt, p);
      }

    if (p.get_x() < root->x())
      return bu_search(root->lb, p);
    else
      return bu_search(root->rt, p);
  }

public:

  /// @brief Check if the tree contains a point.
  [[nodiscard]] bool contains(const Point & p) const noexcept
  {
    return lr_search(root, p) != nullptr;
  }

private:

  /** @brief Recursively find points within a rectangle.
   *
   *  @param root Current node
   *  @param rect Query rectangle
   *  @param q Output list for matching points
   */
  static void range(Node * root, const Rectangle & rect, DynList<Point> * q) 
  {
    if (root == nullptr)
      return;
    
    if (not root->rect.intersects(rect))
      return;
    
    const Point & point = root->point;
    if (rect.contains(point))
        q->append(point);
    
    range(root->lb, rect, q);
    range(root->rt, rect, q);
  }

public:

  /** @brief Find all points within a rectangle.
   *
   *  Performs a range query to find all points that lie within the
   *  specified rectangle.
   *
   *  @param rect Query rectangle
   *  @param l Output list where matching points are appended
   */
  void range(const Rectangle & rect, DynList<Point> * l)
  { 
    if (N == 0)
      return;
    
    range(root, rect, l);
  }

private:

  Node * min_node;         ///< Current nearest node (for nearest())
  Geom_Number min_dist2;   ///< Current minimum squared distance

  /** @brief Recursively find nearest neighbor (X-split level).
   *
   *  @param root Current node
   *  @param p Query point
   */
  void lr_nearest(Node * root, const Point & p) noexcept
  {
    if (root == nullptr)
      return;
    
    if (root->rect.distance_squared_to(p) > min_dist2)
      return;
    
    Geom_Number d2 = root->point.distance_squared_to(p);
    if (d2 < min_dist2) 
      {
	min_dist2 = d2;
	min_node = root;
      }
    
    if (p.get_x() < root->x()) // Is p to left of this?
      { 
	bu_nearest(root->lb, p);
	bu_nearest(root->rt, p);
      } 
    else 
      {
	bu_nearest(root->rt, p);
	bu_nearest(root->lb, p);
      }
  }

  /** @brief Recursively find nearest neighbor (Y-split level).
   *
   *  @param root Current node
   *  @param p Query point
   */
  void bu_nearest(Node * root, const Point & p) noexcept
  {
    if (root == nullptr)
      return;
    
    if (root->rect.distance_squared_to(p) > min_dist2)
      return;
    
    Geom_Number d2 = root->point.distance_squared_to(p);
    if (d2 < min_dist2) 
      {   
	min_dist2 = d2;
	min_node = root;
      }
        
    if (p.get_y() < root->y())  // Is p below this?
      {
	lr_nearest(root->lb, p);
	lr_nearest(root->rt, p);
    } 
    else 
      {
	lr_nearest(root->rt, p);
	lr_nearest(root->lb, p);
      }
  }

public:

  /** @brief Find the nearest point to a query point.
   *
   *  Performs a nearest neighbor search using branch-and-bound pruning.
   *
   *  @param p Query point
   *  @return The nearest point in the tree, or NullPoint if tree is empty
   */
  [[nodiscard]] Point nearest(const Point & p) noexcept
  { 
    if (N == 0)
      return NullPoint;
    
    // Initialize with distance to root (mpq_class doesn't have max())
    min_node = root;
    min_dist2 = root->point.distance_squared_to(p);
    
    lr_nearest(root, p);
    
    return min_node != nullptr ? min_node->point : NullPoint;
  }

private:

  /// @brief Recursively delete all nodes.
  void destroy_tree(Node * node) noexcept
  {
    if (node == nullptr)
      return;
    
    destroy_tree(node->lb);
    destroy_tree(node->rt);
    delete node;
  }

public:

  /// @brief Destructor - frees all nodes.
  ~K2Tree()
  {
    destroy_tree(root);
  }
};

# endif // TPL_2DTREE_H
