
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


/** @file quadnode.H
 *  @brief QuadTree node implementation for spatial data structures.
 *
 *  This file provides QuadNode, a node class for quadtree spatial indexing.
 *  Each node represents a rectangular region and can be subdivided into
 *  four quadrants (NW, NE, SW, SE) for efficient 2D point storage and queries.
 *
 *  @author Alejandro Mujica
 *  @ingroup Geometry
 */

# ifndef QUADNODE_H
# define QUADNODE_H

# include <htlist.H>
# include <point.H>
# include <ah-errors.H>

using namespace Aleph;

# define PARENT(p) ((p)->get_parent())
# define NW_CHILD(p) ((p)->get_nw_child())
# define NE_CHILD(p) ((p)->get_ne_child())
# define SW_CHILD(p) ((p)->get_sw_child())
# define SE_CHILD(p) ((p)->get_se_child())
# define COLOR(p) ((p)->get_color())
# define LEVEL(p) ((p)->get_level())

/** @brief Node for QuadTree spatial data structure.

    QuadNode represents a rectangular region in 2D space that can be recursively
    subdivided into four quadrants (northwest, northeast, southwest, southeast).
    Each node can store multiple points and maintains pointers to its parent
    and four potential children.

    @par Node States (Colors):
    - **White**: Leaf node with no points (empty)
    - **Black**: Leaf node with at least one point
    - **Gray**: Internal node (has children)

    @par Spatial Subdivision:
    When a leaf node exceeds its point capacity, it splits into four children:
    ```
    +-------+-------+
    | NW    | NE    |
    +-------+-------+
    | SW    | SE    |
    +-------+-------+
    ```

    @par Complexity:
    - Point insertion: O(log n) expected (depends on tree depth)
    - Point search: O(log n) expected
    - Point removal: O(log n) expected
    - Neighbor finding: O(log n)
    - Space: O(n) for n points

    @author Alejandro Mujica
    @ingroup Geometry
 */
class QuadNode
{
public:

  /** @brief Node color states for quadtree nodes.
   *
   *  Colors indicate the state of a node:
   *  - **White**: Leaf node with no points (empty region)
   *  - **Black**: Leaf node containing at least one point
   *  - **Gray**: Internal node (non-leaf, has children)
   */
  enum class Color
  {
    White,  ///< Empty leaf node
    Gray,   ///< Internal node with children
    Black,  ///< Leaf node with points
    Num_Colors
  };

  /** @brief Quadrant directions for child nodes.
   *
   *  Identifies which quadrant a child node occupies:
   *  - **NW**: Northwest (top-left)
   *  - **NE**: Northeast (top-right)
   *  - **SW**: Southwest (bottom-left)
   *  - **SE**: Southeast (bottom-right)
   */
  enum class Quad
  {
    NW,  ///< Northwest quadrant
    NE,  ///< Northeast quadrant
    SW,  ///< Southwest quadrant
    SE,  ///< Southeast quadrant
    Num_Quads
  };

  /** @brief Cardinal directions for neighbor navigation.
   *
   *  Used to identify neighbors in four cardinal directions.
   */
  enum class Side
  {
    North,  ///< Northern neighbor
    South,  ///< Southern neighbor
    East,   ///< Eastern neighbor
    West,   ///< Western neighbor
    Num_Sides
  };

private:
  DynList<Point> points; ///< Points stored in this node (only for leaves)

  QuadNode * parent;   ///< Parent node (nullptr for root)
  QuadNode * nw_child; ///< Northwest child
  QuadNode * ne_child; ///< Northeast child
  QuadNode * sw_child; ///< Southwest child
  QuadNode * se_child; ///< Southeast child

  Color color;         ///< Node state (White/Gray/Black)
  unsigned long level; ///< Depth in the tree (0 for root)

  // Region boundaries
  Geom_Number min_x;   ///< Minimum X coordinate
  Geom_Number max_x;   ///< Maximum X coordinate
  Geom_Number min_y;   ///< Minimum Y coordinate
  Geom_Number max_y;   ///< Maximum Y coordinate

  /// @brief Find the northern neighbor of a node.
  static QuadNode * get_north_neighbor(QuadNode * v) noexcept
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_sw_child())
      return NW_CHILD(PARENT(v));

    if (v->is_se_child())
      return NE_CHILD(PARENT(v));

    QuadNode * u = get_north_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_nw_child())
      return SW_CHILD(u);
    else
    return SE_CHILD(u);
  }

  /// @brief Find the southern neighbor of a node.
  static QuadNode * get_south_neighbor(QuadNode * v) noexcept
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_nw_child())
      return SW_CHILD(PARENT(v));

    if (v->is_ne_child())
      return SE_CHILD(PARENT(v));

    QuadNode * u = get_south_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_sw_child())
      return NW_CHILD(u);
    else
      return NE_CHILD(u);
  }

  /// @brief Find the eastern neighbor of a node.
  static QuadNode * get_east_neighbor(QuadNode * v) noexcept
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_nw_child())
      return NE_CHILD(PARENT(v));

    if (v->is_sw_child())
      return SE_CHILD(PARENT(v));

    QuadNode * u = get_east_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_ne_child())
      return NW_CHILD(u);
    else
      return SW_CHILD(u);
  }

  /// @brief Find the western neighbor of a node.
  static QuadNode * get_west_neighbor(QuadNode * v) noexcept
  {
    if (PARENT(v) == nullptr)
      return nullptr;

    if (v->is_ne_child())
      return NW_CHILD(PARENT(v));

    if (v->is_se_child())
      return SW_CHILD(PARENT(v));

    QuadNode * u = get_west_neighbor(PARENT(v));

    if (u == nullptr or u->is_leaf())
      return u;
    else if (v->is_nw_child())
      return NE_CHILD(u);
    else
      return SE_CHILD(u);
  }

  /// @brief Recursively collect leaf neighbors on a given side.
  static void get_neighbors_by_side(QuadNode * node,
                                    const Side & side,
				    DynList<QuadNode *> & neighbors)
  {
    if (node == nullptr)
      return;

    if (node->is_leaf())
      {
        neighbors.append(node);
        return;
      }

    switch(side)
      {
      case Side::West:
        get_neighbors_by_side(NW_CHILD(node), side, neighbors);
        get_neighbors_by_side(SW_CHILD(node), side, neighbors);
        break;
      case Side::East:
        get_neighbors_by_side(NE_CHILD(node), side, neighbors);
        get_neighbors_by_side(SE_CHILD(node), side, neighbors);
        break;
      case Side::North:
        get_neighbors_by_side(NW_CHILD(node), side, neighbors);
        get_neighbors_by_side(NE_CHILD(node), side, neighbors);
        break;
      case Side::South:
        get_neighbors_by_side(SW_CHILD(node), side, neighbors);
        get_neighbors_by_side(SE_CHILD(node), side, neighbors);
        break;
      default:
        ah_domain_error_if(true) << "Invalid side value";
    }
}

/// @brief Recursively count all points in a subtree.
static size_t count_points(QuadNode * node) noexcept
{
  if (node == nullptr)
    return 0;

  if (node->is_leaf())
    return node->points.size();

  size_t num_points = 0;

  num_points += count_points(NW_CHILD(node));
  num_points += count_points(NE_CHILD(node));
  num_points += count_points(SW_CHILD(node));
  num_points += count_points(SE_CHILD(node));

  return num_points;
}
  
public:

  /// @brief Default constructor - creates a white (empty) node.
  QuadNode() noexcept
    : points(), parent(nullptr), nw_child(nullptr), ne_child(nullptr),
      sw_child(nullptr), se_child(nullptr), color(Color::White), level(0),
      min_x(0), max_x(0), min_y(0), max_y(0)
  {
    // Empty
  }
  
  /** @brief Construct a node with specified region bounds.
   *
   *  @param _min_x Minimum X coordinate of the region
   *  @param _max_x Maximum X coordinate of the region
   *  @param _min_y Minimum Y coordinate of the region
   *  @param _max_y Maximum Y coordinate of the region
   *  @param _parent Parent node (nullptr for root)
   */
  QuadNode(const Geom_Number & _min_x, const Geom_Number & _max_x,
	   const Geom_Number & _min_y, const Geom_Number & _max_y,
	   QuadNode * _parent = nullptr) noexcept
    : points(), parent(_parent), nw_child(nullptr), ne_child(nullptr),
      sw_child(nullptr), se_child(nullptr), color(Color::White), level(0),
      min_x(_min_x), max_x(_max_x), min_y(_min_y), max_y(_max_y)
  {
    // Empty
  }

  QuadNode(const QuadNode &) = delete;

  /** @brief Set the region boundaries for this node.
   *
   *  @param _min_x Minimum X coordinate
   *  @param _max_x Maximum X coordinate
   *  @param _min_y Minimum Y coordinate
   *  @param _max_y Maximum Y coordinate
   */
  void set_region(const Geom_Number & _min_x,
		  const Geom_Number & _max_x,
                  const Geom_Number & _min_y,
                  const Geom_Number & _max_y) noexcept
  {
    min_x = _min_x;
    max_x = _max_x;
    min_y = _min_y;
    max_y = _max_y;
  }

  /// @brief Get reference to parent node (for reading/writing via PARENT macro).
  [[nodiscard]] QuadNode *& get_parent() noexcept { return parent; }

  /// @brief Get reference to NW child (for reading/writing via NW_CHILD macro).
  [[nodiscard]] QuadNode *& get_nw_child() noexcept { return nw_child; }

  /// @brief Get reference to NE child (for reading/writing via NE_CHILD macro).
  [[nodiscard]] QuadNode *& get_ne_child() noexcept { return ne_child; }

  /// @brief Get reference to SW child (for reading/writing via SW_CHILD macro).
  [[nodiscard]] QuadNode *& get_sw_child() noexcept { return sw_child; }

  /// @brief Get reference to SE child (for reading/writing via SE_CHILD macro).
  [[nodiscard]] QuadNode *& get_se_child() noexcept { return se_child; }

  /// @brief Get reference to color (for reading/writing via COLOR macro).
  [[nodiscard]] Color & get_color() noexcept { return color; }

  /// @brief Get reference to level (for reading/writing via LEVEL macro).
  [[nodiscard]] unsigned long & get_level() noexcept { return level; }

  /// @brief Check if this node is a leaf (has no children).
  [[nodiscard]] bool is_leaf() const noexcept { return color != Color::Gray; }

  /// @brief Check if this node is the NW child of its parent.
  [[nodiscard]] bool is_nw_child() const noexcept
  {
    if (parent == nullptr)
      return false;
    return NW_CHILD(parent) == this;
  }

  /// @brief Check if this node is the NE child of its parent.
  [[nodiscard]] bool is_ne_child() const noexcept
  {
    if (parent == nullptr)
      return false;
    return NE_CHILD(parent) == this;
  }

  /// @brief Check if this node is the SW child of its parent.
  [[nodiscard]] bool is_sw_child() const noexcept
  {
    if (parent == nullptr)
      return false;
    return SW_CHILD(parent) == this;
  }

  /// @brief Check if this node is the SE child of its parent.
  [[nodiscard]] bool is_se_child() const noexcept
  {
    if (parent == nullptr)
      return false;
    return SE_CHILD(parent) == this;
  }

  /** @brief Check if a point is contained in this node's region.
   *
   *  A point is contained if min_x <= x < max_x and min_y <= y < max_y.
   *  Note: Upper bounds are exclusive.
   *
   *  @param p The point to test
   *  @return true if the point is within the region
   */
  [[nodiscard]] bool contains(const Point & p) const noexcept
  {
    return p.get_x() >= min_x and p.get_x() < max_x and
           p.get_y() >= min_y and p.get_y() < max_y;
  }

  /** @brief Find which child node should contain a given point.
   *
   *  Requires that this node is not a leaf and that the point is
   *  within this node's region.
   *
   *  @param p Point to locate
   *  @return Pointer to the child node that contains p
   *  @throw std::domain_error if this node doesn't contain p
   */
  [[nodiscard]] QuadNode * get_child_to(const Point & p) const
  {
    assert(nw_child != nullptr);
    assert(ne_child != nullptr);
    assert(sw_child != nullptr);
    assert(se_child != nullptr);

    ah_domain_error_if(not contains(p))
      << "This node does not contain that point";

    if (nw_child->contains(p))
      return nw_child;

    if (ne_child->contains(p))
      return ne_child;

    if (sw_child->contains(p))
      return sw_child;

    if (se_child->contains(p))
      return se_child;

    return nullptr; // To avoid compiler warnings
  }

  /** @brief Add a point to this node.
   *
   *  Appends the point to the node's point list and marks it as Black.
   *  Should only be called on leaf nodes.
   *
   *  @param p The point to add
   *  @return Reference to the added point
   */
  Point & add_point(const Point & p)
  {
    color = Color::Black;
    return points.append(p);
  }

  /// @brief Get total number of points in this subtree.
  [[nodiscard]] size_t get_num_points() noexcept
  {
    return count_points(this);
  }

  /// @brief Get minimum X coordinate of this region.
  [[nodiscard]] const Geom_Number & get_min_x() const noexcept { return min_x; }

  /// @brief Get maximum X coordinate of this region.
  [[nodiscard]] const Geom_Number & get_max_x() const noexcept { return max_x; }

  /// @brief Get minimum Y coordinate of this region.
  [[nodiscard]] const Geom_Number & get_min_y() const noexcept { return min_y; }

  /// @brief Get maximum Y coordinate of this region.
  [[nodiscard]] const Geom_Number & get_max_y() const noexcept { return max_y; }

  /// @brief Get width of this region.
  [[nodiscard]] Geom_Number get_width() const noexcept { return max_x - min_x; }

  /// @brief Get height of this region.
  [[nodiscard]] Geom_Number get_height() const noexcept { return max_y - min_y; }

  /// @brief Get midpoint X coordinate.
  [[nodiscard]] Geom_Number get_mid_x() const noexcept { return (min_x + max_x) / 2; }

  /// @brief Get midpoint Y coordinate.
  [[nodiscard]] Geom_Number get_mid_y() const noexcept { return (min_y + max_y) / 2; }

  /** @brief Search for a point in this node.
   *
   *  Linear search through the node's point list.
   *
   *  @param p Point to search for
   *  @return Pointer to the found point, or nullptr if not found
   */
  [[nodiscard]] Point * search_point(const Point & p) noexcept
  {
    for (Point & point : points)
      {
        if (point == p)
          return &point;
      }
    return nullptr;
  }

  /** @brief Remove a point from this node.
   *
   *  @param p Point to remove
   *  @return true if the point was found and removed, false otherwise
   */
  bool remove_point(const Point & p)
  {
    for (DynList<Point>::iterator it(points); it.has_curr(); it.next_ne())
      {
        const Point & curr = it.get_curr_ne();
	
        if (curr == p)
          {
            it.del();
            return true;
          }
      }
    return false;
  }

  /// @brief Remove all points from this node and mark it as White.
  void empty() noexcept
  {
    points.empty();
    color = Color::White;
  }

  /** @brief Find all adjacent leaf nodes.
   *
   *  Searches in all four cardinal directions and returns neighboring
   *  leaf nodes. Internal nodes are traversed to find their adjacent leaves.
   *
   *  @return List of pointers to neighboring leaf nodes
   */
  [[nodiscard]] DynList<QuadNode *> get_neighbors()
  {
    DynList<QuadNode *> neighbors;
    
    QuadNode * north_neighbor = get_north_neighbor(this);
    get_neighbors_by_side(north_neighbor, Side::South, neighbors);

    QuadNode * south_neighbor = get_south_neighbor(this);
    get_neighbors_by_side(south_neighbor, Side::North, neighbors);

    QuadNode * east_neighbor = get_east_neighbor(this);
    get_neighbors_by_side(east_neighbor, Side::West, neighbors);

    QuadNode * west_neighbor = get_west_neighbor(this);
    get_neighbors_by_side(west_neighbor, Side::East, neighbors);

    return neighbors;
  }

  /// @brief Get reference to the points list (for direct manipulation).
  [[nodiscard]] DynList<Point> & get_points_set() noexcept { return points; }

  /** @brief Apply operation to each point in this node.
   *
   *  @tparam Op Function object type with signature void(Point&)
   *  @param op Operation to apply
   */
  template <class Op>
  void for_each_point(Op & op)
  {
    for (Point & point : points)
      op(point);
  };

  /// @brief Apply operation to each point (rvalue reference version).
  template <class Op>
  void for_each_point(Op && op = Op())
  {
    for_each_point<Op>(op);
  }

  QuadNode & operator = (const QuadNode &) = delete;
};

# endif // QUADNODE_H
