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

/** @file quadtree.H
 *  @brief QuadTree spatial data structure for efficient 2D point indexing.
 *
 *  This file provides QuadTree, a hierarchical spatial data structure that
 *  recursively subdivides 2D space into quadrants. It enables efficient
 *  spatial queries including point location, range searches, and nearest
 *  neighbor queries.
 *
 *  @author Alejandro Mujica
 *  @ingroup Geometry
 */

# ifndef QUADTREE_H
# define QUADTREE_H

# include <quadnode.H>

/** @brief QuadTree - Hierarchical spatial index for 2D points.

    QuadTree is a tree data structure where each internal node has exactly
    four children corresponding to quadrants: NW, NE, SW, and SE. The tree
    adapts to point distribution by subdividing regions that exceed a
    specified point capacity.

    @par Key Characteristics:
    - **Adaptive subdivision**: Regions split only when exceeding capacity
    - **Hierarchical structure**: Tree depth adapts to point distribution
    - **Efficient spatial queries**: O(log n) for well-distributed data
    - **Automatic balancing**: Merges empty quadrants after deletions

    @par Spatial Decomposition:
    ```
    Root Region
    +-------------------+
    |        |          |
    |   NW   |    NE    |
    +--------+----------+
    |   SW   |    SE    |
    |        |          |
    +-------------------+
    ```

    @par Complexity:
    - Insert: O(log n) average, O(depth) worst
    - Search: O(log n) average, O(depth) worst
    - Remove: O(log n) average, O(depth) worst
    - Range query: O(log n + k) where k is result size
    - Space: O(n) for n points

    @par Use Cases:
    ✓ Geographic information systems (GIS)
    ✓ Collision detection in games
    ✓ Image processing and compression
    ✓ Nearest neighbor searches
    ✓ Range queries (e.g., "find all points in rectangle")

    @par Example:
    @code
    // Create a quadtree for region [0, 100] × [0, 100]
    // with max 4 points per node
    QuadTree tree(0, 100, 0, 100, 4);

    // Insert points
    tree.insert(Point(25, 25));
    tree.insert(Point(75, 75));
    tree.insert(Point(25, 75));
    tree.insert(Point(75, 25));

    // Search for a point
    Point * found = tree.search(Point(25, 25));

    // Check if point is in the tree
    if (tree.contains(Point(50, 50)))
      std::cout << "Point found" << '\n';

    // Remove a point
    tree.remove(Point(25, 25));

    // Traverse all nodes
    tree.for_each([](QuadNode * node) {
      std::cout << "Node at level " << LEVEL(node) << '\n';
    });
    @endcode

    @note The tree automatically subdivides nodes when they exceed
          max_num_points_per_node and merges when they fall below it.

    @see QuadNode Node structure for the quadtree.
    @see K2Tree Alternative 2D tree structure (kd-tree variant).
    @author Alejandro Mujica
    @ingroup Geometry
 */
class QuadTree 
{
public:
  using Node = QuadNode;

private:
  Node * root;  ///< Root node of the tree

  /// Maximum points per leaf node before splitting
  size_t max_num_points_per_node;

  /** @brief Subdivide a leaf node into four children.
   *
   *  Creates four child nodes (NW, NE, SW, SE) and redistributes the
   *  node's points among them. Only applies to leaf nodes.
   *
   *  @param node Leaf node to split
   */
  void split(Node * node)
  {
    assert(node->is_leaf()); // Only applies to leaf nodes

    size_t next_level = LEVEL(node) + 1;

    NW_CHILD(node) =
      new QuadNode(node->get_min_x(), node->get_mid_x(),
                    node->get_min_y(), node->get_mid_y(), node);
    LEVEL(NW_CHILD(node)) = next_level; 

    NE_CHILD(node) =
      new QuadNode(node->get_mid_x(), node->get_max_x(),
                    node->get_min_y(), node->get_mid_y(), node);
    LEVEL(NE_CHILD(node)) = next_level;

    SW_CHILD(node) =
      new QuadNode(node->get_min_x(), node->get_mid_x(),
                    node->get_mid_y(), node->get_max_y(), node);
    LEVEL(SW_CHILD(node)) = next_level;

    SE_CHILD(node) =
      new QuadNode(node->get_mid_x(), node->get_max_x(),
                    node->get_mid_y(), node->get_max_y(), node);
    LEVEL(SE_CHILD(node)) = next_level;

    COLOR(node) = Node::Color::Gray;

    DynList<Point> & points = node->get_points_set();
    
    while (not points.is_empty())
      {
        Point point = points.remove_first();

        QuadNode * child = node->get_child_to(point);

        child->add_point(point);
      }
  }

  /** @brief Merge four child nodes back into their parent.
   *
   *  Deletes the four child nodes and transfers all their points to the
   *  parent node. Applies only to internal nodes whose children are all leaves.
   *
   *  @param node Internal node whose children should be merged
   */
  void join(Node * node)
  {
    assert(not node->is_leaf()); // node is not a leaf
    // All four children of node must be leaves
    assert(NW_CHILD(node)->is_leaf());
    assert(NE_CHILD(node)->is_leaf());
    assert(SW_CHILD(node)->is_leaf());
    assert(SE_CHILD(node)->is_leaf());

    DynList<Point> & points = node->get_points_set();

    DynList<Point> & nw_points = NW_CHILD(node)->get_points_set();
 
    while(not nw_points.is_empty())
      points.append(nw_points.remove_first());

    auto & ne_points = NE_CHILD(node)->get_points_set();
 
    while(not ne_points.is_empty())
      points.append(ne_points.remove_first());

    auto & sw_points = SW_CHILD(node)->get_points_set();
 
    while(not sw_points.is_empty())
      points.append(sw_points.remove_first());

    auto & se_points = SE_CHILD(node)->get_points_set();
 
    while(not se_points.is_empty())
      points.append(se_points.remove_first());

    delete NW_CHILD(node);
    NW_CHILD(node) = nullptr;
  
    delete NE_CHILD(node);
    NE_CHILD(node) = nullptr;

    delete SW_CHILD(node);
    SW_CHILD(node) = nullptr;

    delete SE_CHILD(node);
    SE_CHILD(node) = nullptr;

    COLOR(node) = points.is_empty() ? Node::Color::White : Node::Color::Black;
  }

  /// @brief Recursive insert helper.
  Point * insert(Node *& r, const Point & p)
  {
    assert(r->contains(p));

    if (r->is_leaf())
      {
        if (r->get_num_points() < max_num_points_per_node)
          return &r->add_point(p);
        else
          split(r);
      }
	
    Node * node = r->get_child_to(p);

    return insert(node, p);
  }

  /// @brief Recursively delete all nodes.
  void empty(Node *& r) noexcept
  {
    if (r == nullptr)
      return;

    empty(NW_CHILD(r));
    empty(NE_CHILD(r));
    empty(SW_CHILD(r));
    empty(SE_CHILD(r));

    delete r;
    r = nullptr;
  }
 
  /// @brief Apply operation to all nodes in the tree.
  template <class Op>
  void operate_on_nodes(Node * r, Op & op)
  {
    if (r == nullptr)
      return;

    op(r);

    operate_on_nodes<Op> (NW_CHILD(r), op);
    operate_on_nodes<Op> (NE_CHILD(r), op);
    operate_on_nodes<Op> (SW_CHILD(r), op);
    operate_on_nodes<Op> (SE_CHILD(r), op);
  }

  /// @brief Deep copy a tree structure.
  void copy_tree(QuadNode * src, QuadNode *& tgt,
                 QuadNode * tgt_parent = nullptr)
  {
    if (src == nullptr)
      return;

    tgt = new QuadNode(src->get_min_x(), src->get_max_x(),
                        src->get_min_y(), src->get_max_y(), tgt_parent);

    tgt->get_points_set() = src->get_points_set();

    COLOR(tgt) = COLOR(src);
    LEVEL(tgt) = LEVEL(src);

    copy_tree(NW_CHILD(src), NW_CHILD(tgt), tgt);
    copy_tree(NE_CHILD(src), NE_CHILD(tgt), tgt);
    copy_tree(SW_CHILD(src), SW_CHILD(tgt), tgt);
    copy_tree(SE_CHILD(src), SE_CHILD(tgt), tgt);
  }

public:

  /** @brief Default constructor - creates an uninitialized tree.
   *
   *  @warning You must call set_max_num_points_per_node() and ensure
   *           the root has proper bounds before using the tree.
   */
  QuadTree()
    : root(new Node), max_num_points_per_node(1)
  {
    // Empty
  }
  
  /** @brief Construct a quadtree with specified region and capacity.
   *
   *  @param min_x Minimum X coordinate of the region
   *  @param max_x Maximum X coordinate of the region
   *  @param min_y Minimum Y coordinate of the region
   *  @param max_y Maximum Y coordinate of the region
   *  @param _max_num_points_per_node Maximum points per leaf before splitting (default: 1)
   */
  QuadTree(const Geom_Number & min_x, const Geom_Number & max_x,
	   const Geom_Number & min_y, const Geom_Number & max_y,
	   const size_t & _max_num_points_per_node = 1)
    : root(new Node(min_x, max_x, min_y, max_y)),
      max_num_points_per_node(_max_num_points_per_node)
  {
    // Empty
  }
  
  /** @brief Copy constructor - creates a deep copy of the tree.
   *
   *  @param tree Tree to copy
   */
  QuadTree(const QuadTree & tree)
    : root(nullptr), max_num_points_per_node(tree.max_num_points_per_node)
  {
    copy_tree(tree.root, root);
  }

  /** @brief Move constructor - transfers ownership.
   *
   *  @param tree Tree to move from
   */
  QuadTree(QuadTree && tree) noexcept
    : root(tree.root), max_num_points_per_node(tree.max_num_points_per_node)
  {
    tree.root = nullptr;
  }

  /** @brief Copy assignment operator - creates a deep copy.
   *
   *  @param tree Tree to assign from
   *  @return Reference to this
   */
  QuadTree & operator = (const QuadTree & tree)
  {
    if (this == &tree)
      return *this;

    empty(root);
    max_num_points_per_node = tree.max_num_points_per_node;
    copy_tree(tree.root, root);

    return *this;
  }

  /** @brief Move assignment operator - transfers ownership.
   *
   *  @param tree Tree to move from
   *  @return Reference to this
   */
  QuadTree & operator = (QuadTree && tree) noexcept
  {
    if (this != &tree)
      {
        empty(root);
        root = tree.root;
        max_num_points_per_node = tree.max_num_points_per_node;
        tree.root = nullptr;
      }
    return *this;
  }

  /// @brief Destructor - frees all nodes.
  ~QuadTree()
  {
    empty(root);
  }

  /// @brief Get the root node.
  [[nodiscard]] Node * get_root() noexcept
  {
    return root;
  }

  /// @brief Get the root node (const version).
  [[nodiscard]] const Node * get_root() const noexcept
  {
    return root;
  }

  /// @brief Set the maximum points per leaf node.
  void set_max_num_points_per_node(const size_t & _max_num_points_per_node) noexcept
  {
    max_num_points_per_node = _max_num_points_per_node;
  }

  /// @brief Get the maximum points per leaf node.
  [[nodiscard]] size_t get_max_num_points_per_node() const noexcept
  {
    return max_num_points_per_node;
  }

  /** @brief Check if a point is within the tree's region.
   *
   *  @param p Point to check
   *  @return true if the point is within the tree's root region
   */
  [[nodiscard]] bool contains(const Point & p) const noexcept
  {
    return root->contains(p);
  }

  /** @brief Insert a point into the tree.
   *
   *  If the point is outside the tree's region, insertion fails.
   *  If the point already exists, it will still be inserted (duplicates allowed).
   *
   *  @param p Point to insert
   *  @return Pointer to the inserted point, or nullptr if outside region
   */
  Point * insert(const Point & p)
  {
    if (not root->contains(p))
      return nullptr;

    return insert(root, p);
  }

  /** @brief Insert a point given by coordinates.
   *
   *  @param x X coordinate
   *  @param y Y coordinate
   *  @return Pointer to the inserted point, or nullptr if outside region
   */
  Point * insert(const Geom_Number & x, const Geom_Number & y)
  {
    return insert(Point(x, y));
  }
  
  /** @brief Search for a point in the tree.
   *
   *  @param p Point to search for
   *  @return Pointer to the found point, or nullptr if not found
   */
  [[nodiscard]] Point * search(const Point & p) noexcept
  {
    if (not root->contains(p))
      return nullptr;

    Node * aux = root;

    while (not aux->is_leaf())
      aux = aux->get_child_to(p);

    return aux->search_point(p);
  }

  /** @brief Find the leaf node containing a point.
   *
   *  @param p Point to locate
   *  @return Pointer to the leaf node containing the point, or nullptr if not found
   */
  [[nodiscard]] Node * search_container_node(const Point & p) noexcept
  {
    if (not root->contains(p))
      return nullptr;

    Node * aux = root;

    while (not aux->is_leaf())
      aux = aux->get_child_to(p);

    if (aux->search_point(p) == nullptr)
      return nullptr;

    return aux;
  }

  /** @brief Remove a point from the tree.
   *
   *  Removes the point and potentially merges nodes if the total point
   *  count falls below the threshold.
   *
   *  @param p Point to remove
   */
  void remove(const Point & p)
  {
    Node * aux = root;

    while (not aux->is_leaf())
      aux = aux->get_child_to(p);

    if (not aux->remove_point(p))
      return;

    Node * parent = PARENT(aux);

    if (parent == nullptr)
      return;

    if (NW_CHILD(parent)->get_num_points() +
        NE_CHILD(parent)->get_num_points() +
        SW_CHILD(parent)->get_num_points() +
        SE_CHILD(parent)->get_num_points() <= max_num_points_per_node)
      join(parent);
  }

  /// @brief Remove all points from the tree, keeping only the root.
  void empty()
  {
    empty(NW_CHILD(root));
    empty(NE_CHILD(root));
    empty(SW_CHILD(root));
    empty(SE_CHILD(root));
    root->empty();
  }

  /** @brief Apply an operation to each node in the tree.
   *
   *  Performs preorder traversal, visiting internal nodes before their children.
   *
   *  @tparam Op Function object type with signature void(Node*)
   *  @param op Operation to apply
   */
  template <class Op>
  void for_each(Op & op)
  {
    operate_on_nodes <Op> (root, op);
  }

  /// @brief Apply operation to each node (rvalue reference version).
  template <class Op>
  void for_each(Op && op = Op())
  { 
    operate_on_nodes <Op> (root, op);
  }
};

# endif // QUADTREE_H
