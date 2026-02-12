
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
# include <optional>
# include <algorithm>
# include <htlist.H>
# include <tpl_array.H>
# include <ahDefs.H> // For Empty_Class
#include <utility>

using Aleph::Empty_Class;

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
    if (auto nearest = tree.nearest(Point(30, 45)))
      std::cout << "Nearest: (" << nearest->get_x() << ", "
                << nearest->get_y() << ")" << '\n';

    // Range query
    Rectangle rect(20, 20, 80, 80);
    DynList<Point> points_in_rect;
    tree.range(rect, &points_in_rect);
    @endcode

    @tparam T Optional user data type associated with points (default: Empty_Class)

    @note The tree does NOT automatically balance.  For sorted insertions,
          use the static `build()` factory which selects medians to produce
          an O(log n)-height tree.

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
    Point point_; ///< The point stored at this node
    Rectangle rect_; ///< The axis-aligned rectangle for this node's region
    Node *lb_; ///< Left/bottom subtree
    Node *rt_; ///< Right/top subtree

    /// @brief Construct a node with a given point.
    Node(Point  p) noexcept
      : point_(std::move(p)), lb_(nullptr), rt_(nullptr)
    {
      // empty
    }

    /// @brief Set the rectangular region for this node.
    void set_rect(const Geom_Number & xmin, const Geom_Number & ymin,
                  const Geom_Number & xmax, const Geom_Number & ymax) noexcept
    {
      rect_.set_rect(xmin, ymin, xmax, ymax);
    }

    /// @brief Set the rectangular region using corner points.
    void set_rect(const Point & pmin, const Point & pmax) noexcept
    {
      rect_.set_rect(pmin.get_x(), pmin.get_y(), pmax.get_x(), pmax.get_y());
    }

    [[nodiscard]] const Geom_Number &xmin() const noexcept { return rect_.get_xmin(); }
    [[nodiscard]] const Geom_Number &ymin() const noexcept { return rect_.get_ymin(); }
    [[nodiscard]] const Geom_Number &xmax() const noexcept { return rect_.get_xmax(); }
    [[nodiscard]] const Geom_Number &ymax() const noexcept { return rect_.get_ymax(); }
    [[nodiscard]] const Geom_Number &x() const noexcept { return point_.get_x(); }
    [[nodiscard]] const Geom_Number &y() const noexcept { return point_.get_y(); }
  };

  Point pmin_; ///< Lower-left corner of reference rectangle
  Point pmax_; ///< Upper-right corner of reference rectangle
  size_t N_; ///< Number of points in the tree
  Node *root_; ///< Root of the tree

public:
  /// @brief Default constructor - creates an empty tree with zero-sized region.
  K2Tree() : pmin_(0, 0), pmax_(0, 0), N_(0), root_(nullptr)
  {
    // empty
  }

  /** @brief Construct a k-d tree with specified bounding region.
   *
   *  @param pmin Lower-left corner of the region
   *  @param pmax Upper-right corner of the region
   */
  K2Tree(const Point & pmin, const Point & pmax)
    : pmin_(pmin), pmax_(pmax), N_(0), root_(nullptr)
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
    : pmin_(xmin, ymin), pmax_(xmax, ymax), N_(0), root_(nullptr)
  {
    // empty
  }

  K2Tree(const K2Tree &) = delete;

  K2Tree &operator =(const K2Tree &) = delete;

  /// @brief Move constructor — transfers ownership of all nodes.
  K2Tree(K2Tree && other) noexcept
    : pmin_(other.pmin_), pmax_(other.pmax_), N_(other.N_), root_(other.root_)
  {
    other.N_ = 0;
    other.root_ = nullptr;
  }

  /// @brief Move assignment operator — transfers ownership of all nodes.
  K2Tree & operator =(K2Tree && other) noexcept
  {
    if (this != &other)
      {
        destroy_tree(root_);
        pmin_ = other.pmin_;
        pmax_ = other.pmax_;
        N_ = other.N_;
        root_ = other.root_;
        other.N_ = 0;
        other.root_ = nullptr;
      }
    return *this;
  }

  /// @brief Check if the tree is empty.
  [[nodiscard]] constexpr bool is_empty() const noexcept { return N_ == 0; }

  /// @brief Get the number of points in the tree.
  [[nodiscard]] constexpr size_t size() const noexcept { return N_; }

private:
  /** @brief Insert into tree with left-right (X) split.
   *
   *  Recursively inserts a point, splitting on X coordinate at this level.
   *
   *  @param root Current node
   *  @param p Point to insert
   *  @return New subtree root, or nullptr if point is duplicate
   */
  Node * lr_insert(Node *root, const Point & p)
  {
    if (root == nullptr)
      return new Node(p);

    Node *ret_val = nullptr;
    if (p.get_x() == root->x())
      {
        if (p.get_y() == root->y())
          return nullptr; // Duplicate point

        ret_val = bu_insert(root->rt_, p);
        if (ret_val != nullptr)
          {
            root->rt_ = ret_val; // Node was inserted
            root->rt_->set_rect(root->x(), root->ymin(),
                               root->xmax(), root->ymax());

            return root;
          }

        return nullptr;
      }

    if (p.get_x() < root->x())
      {
        ret_val = bu_insert(root->lb_, p);
        if (ret_val != nullptr)
          {
            root->lb_ = ret_val;
            root->lb_->set_rect(root->xmin(), root->ymin(),
                               root->x(), root->ymax());

            return root;
          }
        return nullptr;
      }

    ret_val = bu_insert(root->rt_, p);
    if (ret_val != nullptr)
      {
        root->rt_ = ret_val;
        root->rt_->set_rect(root->x(), root->ymin(),
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
  Node * bu_insert(Node *root, const Point & p)
  {
    if (root == nullptr)
      return new Node(p);

    Node *ret_val = nullptr;
    if (p.get_y() == root->y())
      {
        if (p.get_x() == root->x())
          return nullptr; // Duplicate point

        ret_val = lr_insert(root->rt_, p);
        if (ret_val != nullptr)
          {
            root->rt_ = ret_val; // Node was inserted
            root->rt_->set_rect(root->xmin(), root->y(),
                               root->xmax(), root->ymax());

            return root;
          }

        return nullptr;
      }

    if (p.get_y() < root->y())
      {
        ret_val = lr_insert(root->lb_, p);
        if (ret_val != nullptr)
          {
            root->lb_ = ret_val;
            root->lb_->set_rect(root->xmin(), root->ymin(),
                               root->xmax(), root->y());
            return root;
          }
        return nullptr;
      }

    ret_val = lr_insert(root->rt_, p);
    if (ret_val != nullptr)
      {
        root->rt_ = ret_val;
        root->rt_->set_rect(root->xmin(), root->y(),
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
   *  @param p Point to insert.
   *  @return true if the point was inserted, false if it was a duplicate.
   */
  bool insert(const Point & p)
  {
    if (root_ == nullptr)
      {
        root_ = new Node(p);
        root_->set_rect(pmin_, pmax_);
        ++N_;
        return true;
      }

    Node *ret = lr_insert(root_, p);
    if (ret == nullptr)
      return false;

    ++N_;
    return true;
  }

private:
  /// @brief Search with bottom-up (Y) split pattern.
  static Node * bu_search(Node *root, const Point & p) noexcept
  {
    if (root == nullptr)
      return nullptr;

    if (root->y() == p.get_y())
      {
        if (root->x() == p.get_x())
          return root;

        return lr_search(root->rt_, p);
      }

    if (p.get_y() < root->y())
      return lr_search(root->lb_, p);
    return lr_search(root->rt_, p);
  }

  /// @brief Search with left-right (X) split pattern.
  static Node * lr_search(Node *root, const Point & p) noexcept
  {
    if (root == nullptr)
      return nullptr;

    if (root->x() == p.get_x())
      {
        if (root->y() == p.get_y())
          return root;

        return bu_search(root->rt_, p);
      }

    if (p.get_x() < root->x())
      return bu_search(root->lb_, p);
    return bu_search(root->rt_, p);
  }

public:
  /// @brief Check if the tree contains a point.
  [[nodiscard]] bool contains(const Point & p) const noexcept
  {
    return lr_search(root_, p) != nullptr;
  }

private:
  /** @brief Recursively find points within a rectangle.
   *
   *  @param root Current node
   *  @param rect Query rectangle
   *  @param q Output list for matching points
   */
  static void range(Node *root, const Rectangle & rect, DynList<Point> *q)
  {
    if (root == nullptr)
      return;

    if (not root->rect_.intersects(rect))
      return;

    if (const Point & point = root->point_; rect.contains(point))
      q->append(point);

    range(root->lb_, rect, q);
    range(root->rt_, rect, q);
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
  void range(const Rectangle & rect, DynList<Point> *l) const
  {
    if (N_ == 0)
      return;

    range(root_, rect, l);
  }

private:
  /// @brief Thread-local state for nearest-neighbor recursion.
  struct NearestState
  {
    Node *node = nullptr;
    Geom_Number dist2;
  };

  /** @brief Recursively find nearest neighbor (X-split level).
   *
   *  @param root Current node
   *  @param p Query point
   *  @param st Mutable search state
   */
  static void lr_nearest(Node *root, const Point & p,
                         NearestState & st) noexcept
  {
    if (root == nullptr)
      return;

    if (root->rect_.distance_squared_to(p) > st.dist2)
      return;

    if (const Geom_Number d2 = root->point_.distance_squared_to(p); d2 < st.dist2)
      {
        st.dist2 = d2;
        st.node = root;
      }

    if (p.get_x() < root->x()) // Is p to left of this?
      {
        bu_nearest(root->lb_, p, st);
        bu_nearest(root->rt_, p, st);
      }
    else
      {
        bu_nearest(root->rt_, p, st);
        bu_nearest(root->lb_, p, st);
      }
  }

  /** @brief Recursively find nearest neighbor (Y-split level).
   *
   *  @param root Current node
   *  @param p Query point
   *  @param st Mutable search state
   */
  static void bu_nearest(Node *root, const Point & p,
                         NearestState & st) noexcept
  {
    if (root == nullptr)
      return;

    if (root->rect_.distance_squared_to(p) > st.dist2)
      return;

    if (const Geom_Number d2 = root->point_.distance_squared_to(p); d2 < st.dist2)
      {
        st.dist2 = d2;
        st.node = root;
      }

    if (p.get_y() < root->y()) // Is p below this?
      {
        lr_nearest(root->lb_, p, st);
        lr_nearest(root->rt_, p, st);
      }
    else
      {
        lr_nearest(root->rt_, p, st);
        lr_nearest(root->lb_, p, st);
      }
  }

public:
  /** @brief Find the nearest point to a query point.
   *
   *  Performs a nearest neighbor search using branch-and-bound pruning.
   *  This method is const and thread-safe (no mutable members).
   *
   *  @param p Query point
   *  @return The nearest point in the tree; empty if tree is empty.
   */
  [[nodiscard]] std::optional<Point> nearest(const Point & p) const noexcept
  {
    if (N_ == 0)
      return std::nullopt;

    NearestState st;
    st.node = root_;
    st.dist2 = root_->point_.distance_squared_to(p);

    lr_nearest(root_, p, st);

    return st.node != nullptr ? std::optional<Point>(st.node->point_)
                              : std::nullopt;
  }

  /** @brief Apply an operation to every point in the tree (inorder).
   *
   *  @tparam Op Callable with signature compatible with `void(const Point &)`.
   *  @param op The operation to apply.
   */
  template <typename Op>
  void for_each(Op && op) const
  {
    for_each_node(root_, std::forward<Op>(op));
  }

  /** @brief Build a balanced k-d tree from an array of points.
   *
   *  Constructs a balanced tree by recursively selecting medians,
   *  alternating between X and Y coordinates.  This avoids the
   *  degeneration to O(n) height that occurs with sorted insertions.
   *
   *  Duplicate points are removed internally.
   *
   *  @param points Array of points (copied internally).
   *  @param pmin Lower-left corner of the bounding region.
   *  @param pmax Upper-right corner of the bounding region.
   *  @return A balanced K2Tree containing all unique points.
   *
   *  @par Complexity:
   *  - Time:  O(n log n)
   *  - Space: O(n)
   */
  [[nodiscard]] static K2Tree build(Array<Point> points,
                                    const Point & pmin,
                                    const Point & pmax)
  {
    // Sort lexicographically and remove duplicates
    if (points.size() > 1)
      {
        std::sort(&points(0), &points(0) + points.size(),
                  [](const Point & a, const Point & b)
                    {
                      if (a.get_x() < b.get_x()) return true;
                      if (b.get_x() < a.get_x()) return false;
                      return a.get_y() < b.get_y();
                    });

        size_t w = 1;
        for (size_t r = 1; r < points.size(); ++r)
          if (points(r) != points(w - 1))
            points(w++) = points(r);

        while (points.size() > w)
          points.remove_last();
      }

    K2Tree tree(pmin, pmax);
    if (points.is_empty())
      return tree;

    tree.root_ = build_balanced(points, 0, points.size(), true,
                               pmin, pmax);
    tree.N_ = points.size();
    return tree;
  }

private:
  /// @brief Recursively build a balanced subtree selecting medians.
  ///
  /// After nth_element, elements with the same splitting coordinate as
  /// the median are moved to the right partition so that
  /// lr_search/bu_search (which send equal-coord queries right) can
  /// find them.
  static Node * build_balanced(Array<Point> & pts,
                               const size_t lo, const size_t hi,
                               const bool split_x,
                               const Point & rmin, const Point & rmax)
  {
    if (lo >= hi)
      return nullptr;

    const size_t mid = lo + (hi - lo) / 2;

    if (split_x)
      std::nth_element(&pts(lo), &pts(mid), &pts(hi),
                       [](const Point & a, const Point & b)
                         { return a.get_x() < b.get_x(); });
    else
      std::nth_element(&pts(lo), &pts(mid), &pts(hi),
                       [](const Point & a, const Point & b)
                         { return a.get_y() < b.get_y(); });

    // Ensure left partition [lo, root_pos) has only elements with
    // split-coord strictly less than the median, matching the search
    // invariant (equal goes right).
    const Geom_Number mv = split_x ? pts(mid).get_x() : pts(mid).get_y();

    size_t strict_end = lo;
    for (size_t i = lo; i < mid; ++i)
      {
        const Geom_Number v = split_x ? pts(i).get_x() : pts(i).get_y();
        if (v < mv)
          {
            if (i != strict_end)
              {
                Point tmp = pts(i);
                pts(i) = pts(strict_end);
                pts(strict_end) = tmp;
              }
            ++strict_end;
          }
      }

    // If equal-valued elements were found in [strict_end, mid), move
    // the median to strict_end so that those elements fall into the
    // right subtree.
    size_t root_pos = mid;
    if (strict_end < mid)
      {
        const Point median_pt = pts(mid);
        pts(mid) = pts(strict_end);
        pts(strict_end) = median_pt;
        root_pos = strict_end;
      }

    Node *node = new Node(pts(root_pos));
    node->set_rect(rmin, rmax);

    if (split_x)
      {
        node->lb_ = build_balanced(pts, lo, root_pos, false,
                                   rmin, Point(node->x(), rmax.get_y()));
        node->rt_ = build_balanced(pts, root_pos + 1, hi, false,
                                   Point(node->x(), rmin.get_y()), rmax);
      }
    else
      {
        node->lb_ = build_balanced(pts, lo, root_pos, true,
                                   rmin, Point(rmax.get_x(), node->y()));
        node->rt_ = build_balanced(pts, root_pos + 1, hi, true,
                                   Point(rmin.get_x(), node->y()), rmax);
      }

    return node;
  }

  /// @brief Recursive inorder traversal helper for for_each.
  template <typename Op>
  static void for_each_node(const Node *node, Op && op)
  {
    if (node == nullptr)
      return;

    for_each_node(node->lb_, op);
    op(node->point_);
    for_each_node(node->rt_, op);
  }

  /// @brief Recursively delete all nodes.
  void destroy_tree(Node *node) noexcept
  {
    if (node == nullptr)
      return;

    destroy_tree(node->lb_);
    destroy_tree(node->rt_);
    delete node;
  }

public:
  /// @brief Destructor - frees all nodes.
  ~K2Tree()
  {
    destroy_tree(root_);
  }
};

# endif // TPL_2DTREE_H
