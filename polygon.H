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


/** @file polygon.H
 *  @brief 2D polygon representation and geometric operations.
 *
 *  This file provides classes for representing and manipulating 2D polygons:
 *
 *  - **Vertex**: A point that participates in a doubly-linked list of vertices.
 *  - **Polygon**: General (irregular) polygon defined by a sequence of vertices.
 *  - **Regular_Polygon**: Regular polygon defined by center, side length, and vertex count.
 *
 *  ## Key Features
 *
 *  - **Dynamic vertex management**: Add/remove vertices with automatic validation.
 *  - **Self-intersection detection**: Prevents creation of self-intersecting polygons.
 *  - **Colinearity handling**: Automatically merges colinear edges.
 *  - **Containment testing**: Check if a point lies inside a closed polygon.
 *  - **Extreme points tracking**: Efficiently track bounding box vertices.
 *  - **Iterators**: Traverse vertices or edges (segments) of the polygon.
 *  - **Regular polygon support**: Create n-sided regular polygons with rotation.
 *
 *  ## Usage Example
 *
 *  @code{.cpp}
 *  #include <polygon.H>
 *
 *  // Create a square polygon
 *  Polygon square;
 *  square.add_vertex(Point(0, 0));
 *  square.add_vertex(Point(100, 0));
 *  square.add_vertex(Point(100, 100));
 *  square.add_vertex(Point(0, 100));
 *  square.close();
 *
 *  // Check if point is inside
 *  Point p(50, 50);
 *  if (square.contains_to(p))
 *    std::cout << "Point is inside the square\n";
 *
 *  // Create a regular hexagon
 *  Regular_Polygon hexagon(Point(200, 200), 50.0, 6);
 *  std::cout << "Hexagon radius: " << hexagon.radius() << "\n";
 *  @endcode
 *
 *  ## Thread Safety
 *
 *  None of the classes in this file are thread-safe. External synchronization
 *  is required for concurrent access.
 *
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 */

# ifndef POLYGON_H
# define POLYGON_H

# include <dlink.H>
# include <point.H>
# include <ah-errors.H>
# include <ah-dry.H>
# include <ahDry.H>
# include <ah-iterator.H>
# include <string>
# include <utility>

using Aleph::Dlink;

/** @brief A vertex in a polygon's doubly linked vertex list.
 *
 *  The Vertex class combines a 2D point with doubly linked list capabilities.
 *  This allows polygons to store their vertices as an intrusive linked list
 *  while each vertex retains full Point functionality.
 *
 *  ## Design
 *
 *  - Inherits from Point for geometric coordinates.
 *  - Inherits from Dlink for doubly linked list traversal.
 *  - Provides navigation to adjacent vertices in the polygon.
 *
 *  @note Vertex objects are typically managed by a Polygon and should not
 *        be manually linked/unlinked outside Polygon's methods.
 *
 *  @see Polygon
 *  @ingroup Geometry
 */
class Vertex : public Point, public Dlink
{
public:
  /// @brief Default constructor. Creates a vertex at origin (0, 0).
  Vertex() = default;

  /// @brief Construct a vertex from a Point.
  /// @param point The point to copy coordinates from.
  Vertex(const Point & point) : Point(point)
  { /* empty */
  }

  /// @brief Copy constructor.
  /// @param vertex The vertex to copy.
  /// @note Only copies the Point coordinates; does not copy the link state.
  Vertex(const Vertex & vertex) : Point(vertex), Dlink()
  { /* empty */
  }

  /// @brief Copy assignment operator.
  /// @param vertex The vertex to copy.
  /// @return Reference to this vertex.
  /// @note Only copies Point coordinates; does not modify the link state.
  Vertex &operator=(const Vertex & vertex)
  {
    if (this != &vertex)
      Point::operator=(vertex);
    return *this;
  }

  /// @brief Return this vertex as a plain Point value.
  /// @return Copy of the vertex coordinates as Point.
  [[nodiscard]] Point to_point() const
  {
    return static_cast<const Point>(*this);
  }

  /// @brief Convert a Dlink pointer to a Vertex pointer.
  /// @param link Pointer to a Dlink that is actually a Vertex.
  /// @return Pointer to the Vertex.
  /// @warning The caller must ensure that @p the link actually points to a Vertex.
  [[nodiscard]] static Vertex * dlink_to_vertex(Dlink *link)
  {
    return static_cast<Vertex *>(link);
  }

  /// @brief Convert a const Dlink pointer to a const Vertex pointer.
  /// @param link Pointer to a const Dlink that is actually a Vertex.
  /// @return Pointer to the const Vertex.
  [[nodiscard]] static const Vertex * dlink_to_vertex(const Dlink *link)
  {
    return static_cast<const Vertex *>(link);
  }

  /// @brief Get the previous vertex in the polygon.
  /// @return Reference to the previous vertex.
  /// @throws std::domain_error If this is the only vertex in the list.
  /// @pre The vertex must be part of a polygon with at least 2 vertices.
  [[nodiscard]] const Vertex &prev_vertex() const
  {
    assert(not this->is_empty());

    ah_domain_error_if(this->is_unitarian()) << "There is an only vertex";

    return *dlink_to_vertex(this->get_prev());
  }

  /// @brief Get the next vertex in the polygon.
  /// @return Reference to the next vertex.
  /// @throws std::domain_error If this is the only vertex in the list.
  /// @pre The vertex must be part of a polygon with at least 2 vertices.
  [[nodiscard]] const Vertex &next_vertex() const
  {
    assert(not this->is_empty());

    ah_domain_error_if(this->is_unitarian()) << "There is an only vertex";

    return *dlink_to_vertex(this->get_next());
  }

  // TODO: Implement next_segment() and prev_segment() methods
};


class Regular_Polygon;

/** @brief A general (irregular) 2D polygon defined by a sequence of vertices.
 *
 *  The Polygon class represents a 2D polygon as an ordered sequence of vertices
 *  stored in a doubly linked list. The polygon can be open (a polyline) or
 *  closed (a proper polygon).
 *
 *  ## Key Features
 *
 *  - **Dynamic construction**: Vertices can be added one at a time.
 *  - **Self-intersection prevention**: Adding vertices that would cause
 *    self-intersection throws an exception.
 *  - **Colinearity handling**: Colinear vertices are automatically merged
 *    to simplify the polygon.
 *  - **Bounding box tracking**: Extreme points (lowest, highest, leftmost,
 *    rightmost) are maintained incrementally.
 *  - **Point containment**: Test if a point lies inside a closed polygon.
 *
 *  ## Building a Polygon
 *
 *  @code{.cpp}
 *  Polygon pentagon;
 *  pentagon.add_vertex(Point(0, 100));
 *  pentagon.add_vertex(Point(95, 31));
 *  pentagon.add_vertex(Point(59, -81));
 *  pentagon.add_vertex(Point(-59, -81));
 *  pentagon.add_vertex(Point(-95, 31));
 *  pentagon.close();  // Connect last vertex to first
 *  @endcode
 *
 *  ## Limitations
 *
 *  - The containment test assumes the polygon is convex. For non-convex
 *    polygons, use a more sophisticated algorithm.
 *
 *  @see Vertex, Regular_Polygon, Triangle
 *  @ingroup Geometry
 */
class Polygon : public Geom_Object,
                public GenericTraverse<Polygon>,
                public FunctionalMethods<Polygon, Point>,
                public LocateFunctions<Polygon, Point>,
                public StlAlephIterator<Polygon>
{
  Dlink vertex_list_; ///< Doubly linked list of vertices
  size_t num_vertex_; ///< Number of vertices in the polygon
  bool is_closed_; ///< True if the polygon has been closed

  Point lowest_; ///< Vertex with minimum y-coordinate
  Point highest_; ///< Vertex with maximum y-coordinate
  Point leftmost_; ///< Vertex with minimum x-coordinate
  Point rightmost_; ///< Vertex with maximum x-coordinate

  /// @brief Update extreme points after adding a new vertex.
  /// @param point The newly added point.
  void update_extreme_points(const Point & point)
  {
    if (num_vertex_ == 0)
      {
        leftmost_ = rightmost_ = lowest_ = highest_ = point;
        return;
      }

    if (point.get_x() < leftmost_.get_x())
      leftmost_ = point;

    if (point.get_x() > rightmost_.get_x())
      rightmost_ = point;

    if (point.get_y() < lowest_.get_y())
      lowest_ = point;

    if (point.get_y() > highest_.get_y())
      highest_ = point;
  }

  /// @brief Delete all vertices and reset the polygon state.
  void delete_points()
  {
    while (not vertex_list_.is_empty())
      delete Vertex::dlink_to_vertex(vertex_list_.remove_next());

    num_vertex_ = 0;
    is_closed_ = false;
  }

  /// @brief Copy all vertices from another polygon.
  /// @param poly The polygon to copy vertices from.
  void copy_points(const Polygon & poly)
  {
    auto *list = const_cast<Dlink *>(&poly.vertex_list_);

    for (Dlink::Iterator it(list); it.has_curr(); it.next_ne())
      vertex_list_.append(new Vertex(*Vertex::dlink_to_vertex(it.get_curr())));
  }

  /// @brief Copy vertices from a regular polygon.
  /// @param[in] poly The regular polygon to copy.
  void copy_regular_polygon(const Regular_Polygon & poly);

public:

  /// @brief The type of elements iterated over.
  using Item_Type = Point;

  /// @brief Alias for Item_Type.
  using Key_Type = Point;

  /** @brief STL/Aleph-compatible iterator over polygon vertices as Points.
   *
   *  Wraps the internal `Dlink::Iterator` and yields `const Point&`
   *  via `get_curr()`. Compatible with `StlAlephIterator`,
   *  `GenericTraverse`, and `FunctionalMethods`.
   */
  class Iterator
  {
    Dlink::Iterator dit_;

  public:
    Iterator() noexcept = default;

    Iterator(const Polygon & poly) noexcept
      : dit_(poly.vertex_list_) {}

    [[nodiscard]] bool has_curr() const noexcept { return dit_.has_curr(); }

    void next() { dit_.next(); }

    void next_ne() noexcept { dit_.next_ne(); }

    void prev() { dit_.prev(); }

    void prev_ne() noexcept { dit_.prev_ne(); }

    void reset_first() noexcept { dit_.reset_first(); }

    void end() noexcept { dit_.end(); }

    [[nodiscard]] const Point & get_curr() const
    {
      return *Vertex::dlink_to_vertex(dit_.get_curr());
    }

    [[nodiscard]] Dlink * get_pos() const noexcept
    {
      return dit_.get_curr_ne();
    }

    [[nodiscard]] bool operator==(const Iterator & o) const noexcept
    {
      return dit_ == o.dit_;
    }

    [[nodiscard]] bool operator!=(const Iterator & o) const noexcept
    {
      return dit_ != o.dit_;
    }
  };

  /// @brief Default constructor. Creates an empty, open polygon.
  Polygon() : num_vertex_(0), is_closed_(false)
  { /* empty */
  }

  /// @brief Destructor. Frees all vertices.
  ~Polygon()
  {
    delete_points();
  }

  /// @brief Copy constructor.
  /// @param poly The polygon to copy.
  Polygon(const Polygon & poly)
    : Geom_Object(poly),
      num_vertex_(poly.num_vertex_), is_closed_(poly.is_closed_),
      lowest_(poly.lowest_), highest_(poly.highest_),
      leftmost_(poly.leftmost_), rightmost_(poly.rightmost_)
  {
    copy_points(poly);
  }

  /// @brief Move constructor.
  /// @param poly The polygon to move from.
  Polygon(Polygon && poly) noexcept
    : Polygon()
  {
    vertex_list_.swap(poly.vertex_list_);
    std::swap(num_vertex_, poly.num_vertex_);
    std::swap(is_closed_, poly.is_closed_);
    std::swap(lowest_, poly.lowest_);
    std::swap(highest_, poly.highest_);
    std::swap(leftmost_, poly.leftmost_);
    std::swap(rightmost_, poly.rightmost_);
  }

  /// @brief Construct from a Regular_Polygon.
  /// @param poly The regular polygon to convert.
  /// @note The resulting polygon is automatically closed.
  Polygon(const Regular_Polygon & poly)
    : num_vertex_(0), is_closed_(false)
  {
    copy_regular_polygon(poly);
  }

  /// @brief Copy assignment operator.
  /// @param poly The polygon to copy.
  /// @return Reference to this polygon.
  Polygon &operator =(const Polygon & poly)
  {
    if (this == &poly)
      return *this;

    delete_points();

    num_vertex_ = poly.num_vertex_;
    is_closed_ = poly.is_closed_;
    lowest_ = poly.lowest_;
    highest_ = poly.highest_;
    leftmost_ = poly.leftmost_;
    rightmost_ = poly.rightmost_;

    copy_points(poly);

    return *this;
  }

  /// @brief Move assignment operator.
  /// @param poly The polygon to move from.
  /// @return Reference to this polygon.
  Polygon &operator =(Polygon && poly) noexcept
  {
    vertex_list_.swap(poly.vertex_list_);
    std::swap(num_vertex_, poly.num_vertex_);
    std::swap(is_closed_, poly.is_closed_);
    std::swap(lowest_, poly.lowest_);
    std::swap(highest_, poly.highest_);
    std::swap(leftmost_, poly.leftmost_);
    std::swap(rightmost_, poly.rightmost_);

    return *this;
  }

  /// @brief Assignment from a Regular_Polygon.
  /// @param poly The regular polygon to convert.
  /// @return Reference to this polygon.
  Polygon &operator =(const Regular_Polygon & poly)
  {
    delete_points();
    copy_regular_polygon(poly);

    return *this;
  }

  /// @brief Get the vertex with the minimum y-coordinate.
  /// @return Reference to the lowest point.
  [[nodiscard]] const Point &lowest_point() const { return lowest_; }

  /// @brief Get the vertex with the maximum y-coordinate.
  /// @return Reference to the highest point.
  [[nodiscard]] const Point &highest_point() const { return highest_; }

  /// @brief Get the vertex with the minimum x-coordinate.
  /// @return Reference to the leftmost point.
  [[nodiscard]] const Point &leftmost_point() const { return leftmost_; }

  /// @brief Get the vertex with the maximum x-coordinate.
  /// @return Reference to the rightmost point.
  [[nodiscard]] const Point &rightmost_point() const { return rightmost_; }

  /// @brief Check if the polygon is closed.
  /// @return True if closed, false if still open.
  [[nodiscard]] const bool &is_closed() const { return is_closed_; }

  /// @brief Get the number of vertices.
  /// @return Number of vertices in the polygon.
  [[nodiscard]] const size_t &size() const { return num_vertex_; }

  /** @brief Iterator over the vertices of a polygon.
   *
   *  Provides sequential access to all vertices in the polygon.
   *
   *  @code{.cpp}
   *  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next())
   *    std::cout << it.get_current_vertex().to_string() << "\n";
   *  @endcode
   */
  struct Vertex_Iterator : public Dlink::Iterator
  {
    /// @brief Construct iterator from a polygon.
    /// @param poly The polygon to iterate over.
    /// @throws std::domain_error If the polygon has no vertices.
    Vertex_Iterator(const Polygon & poly) : Dlink::Iterator(poly.vertex_list_)
    {
      ah_domain_error_if(poly.vertex_list_.is_empty()) << "Polygon has not any vertex";
    }

    /// @brief Get the current vertex.
    /// @return Reference to the current vertex.
    Vertex &get_current_vertex() const
    {
      return *Vertex::dlink_to_vertex(get_curr());
    }
  };

  /** @brief Iterator over the edges (segments) of a polygon.
   *
   *  Provides sequential access to all edges of the polygon. For a closed
   *  polygon, this includes the edge from the last vertex back to the first.
   *
   *  @code{.cpp}
   *  for (Polygon::Segment_Iterator it(poly); it.has_curr(); it.next())
   *  {
   *    Segment edge = it.get_current_segment();
   *    std::cout << "Edge: " << edge.to_string() << "\n";
   *  }
   *  @endcode
   */
  class Segment_Iterator : public Dlink::Iterator
  {
    Polygon & poly_; ///< Reference to the polygon being iterated

  public:
    /// @brief Construct a segment iterator from a polygon.
    /// @param poly The polygon to iterate over.
    /// @throws std::domain_error If the polygon has fewer than 2 vertices.
    Segment_Iterator(const Polygon & poly)
      : Dlink::Iterator(poly.vertex_list_), poly_(const_cast<Polygon &>(poly))
    {
      ah_domain_error_if(poly_.vertex_list_.is_unitarian_or_empty())
	      << "Polygon has less than two vertex";
    }

    /// @brief Check if there is a current segment.
    /// @return True if a current segment exists.
    /// @note For open polygons, returns false when at the last vertex.
    [[nodiscard]] bool has_curr() const
    {
      if (this->is_in_last())
        return poly_.is_closed() ? true : false;

      return Dlink::Iterator::has_curr();
    }

    /// @brief Get the current segment (edge).
    /// @return The segment from the current vertex to the next.
    /// @throws std::domain_error If at last vertex of an open polygon.
    [[nodiscard]] Segment get_current_segment() const
    {
      ah_domain_error_if(not poly_.is_closed() and this->is_in_last())
	      << "Segment iterator is in the last point and it is not closed";

      const Vertex *src = Vertex::dlink_to_vertex(this->get_curr());

      // Determine target: if at last vertex, wrap to first; otherwise, next
      const Vertex & tgt =
          this->is_in_last() ? poly_.get_first_vertex() : src->next_vertex();

      return {src->to_point(), tgt.to_point()};
    }
  };

  /// @brief Check if a vertex belongs to this polygon.
  /// @param v The vertex to check.
  /// @return True if @p v is a vertex of this polygon.
  [[nodiscard]] bool vertex_belong_polygon(const Vertex & v) const
  {
    for (Vertex_Iterator it(*this); it.has_curr(); it.next_ne())
      if (&it.get_current_vertex() == &v)
        return true;

    return false;
  }

  /// @brief Get the first vertex of the polygon.
  /// @return Reference to the first vertex.
  /// @throws std::domain_error If the polygon has no vertices.
  [[nodiscard]] const Vertex &get_first_vertex() const
  {
    ah_domain_error_if(vertex_list_.is_empty()) << "Polygon has not any vertex";

    return *Vertex::dlink_to_vertex(vertex_list_.get_next());
  }

  /// @brief Get the last vertex of the polygon.
  /// @return Reference to the last vertex.
  /// @throws std::domain_error If the polygon has no vertices.
  [[nodiscard]] const Vertex &get_last_vertex() const
  {
    ah_domain_error_if(vertex_list_.is_empty()) << "Polygon has not any vertex";

    return *Vertex::dlink_to_vertex(vertex_list_.get_prev());
  }

  /// @brief Get the vertex after the given vertex.
  /// @param v A vertex of this polygon.
  /// @return Reference to the next vertex (wraps around for closed polygons).
  /// @note No validation is performed to check if @p v belongs to this polygon.
  [[nodiscard]] const Vertex &get_next_vertex(const Vertex & v) const
  {
    // If v is the last vertex (next points to sentinel), wrap to the first vertex
    return v.get_next() == &vertex_list_ ?
             *Vertex::dlink_to_vertex(vertex_list_.get_next()) :
             v.next_vertex();
  }

  /// @brief Get the vertex before the given vertex.
  /// @param v A vertex of this polygon.
  /// @return Reference to the previous vertex (wraps around for closed polygons).
  /// @note No validation is performed to check if @p v belongs to this polygon.
  [[nodiscard]] const Vertex &get_prev_vertex(const Vertex & v) const
  {
    // If v is the first vertex (prev points to sentinel), wrap to last vertex
    return v.get_prev() == &vertex_list_ ?
             *Vertex::dlink_to_vertex(vertex_list_.get_prev()) :
             v.prev_vertex();
  }

  /// @brief Get the first edge (segment) of the polygon.
  /// @return Segment from first to second vertex.
  /// @throws std::domain_error If the polygon has fewer than 2 vertices.
  [[nodiscard]] Segment get_first_segment() const
  {
    ah_domain_error_if(vertex_list_.is_unitarian_or_empty())
      << "polygon has less than two vertex";

    const Vertex & first_vertex = get_first_vertex();

    return {first_vertex.to_point(), first_vertex.next_vertex().to_point()};
  }

  /// @brief Get the last edge (segment) of the polygon.
  /// @return Segment from second-to-last to last vertex.
  /// @throws std::domain_error If the polygon has fewer than 2 vertices.
  [[nodiscard]] Segment get_last_segment() const
  {
    ah_domain_error_if(vertex_list_.is_unitarian_or_empty())
      << "polygon has less than two vertex";

    const Vertex & last_vertex = get_last_vertex();

    return {last_vertex.prev_vertex().to_point(), last_vertex.to_point()};
  }

  /// @brief Check if a segment intersects with any edge of the polygon.
  /// @param sg The segment to test.
  /// @return True if @p sg intersects with any edge of the polygon.
  [[nodiscard]] bool intersects_with(const Segment & sg) const
  {
    // Traverse all edges and check for intersection
    for (Segment_Iterator it(*this); it.has_curr(); it.next_ne())
      if (const Segment & curr_side = it.get_current_segment(); curr_side.intersects_with(sg))
        return true;

    return false;
  }

  /** @brief Add a vertex to the polygon.
   *
   *  Adds a new vertex at the end of the vertex list. The method performs
   *  several validations:
   *
   *  1. **Closed check**: Cannot add vertices to a closed polygon.
   *  2. **Colinearity**: If the new point is colinear with the last segment,
   *     the last vertex is replaced (edge extension).
   *  3. **Self-intersection**: If the new edge would intersect any existing
   *     edge, an exception is thrown.
   *
   *  @param point The point to add as a vertex.
   *  @throws std::domain_error If the polygon is closed, the point is inside
   *          the last segment, or the new edge would cause self-intersection.
   */
  void add_vertex(const Point & point)
  {
    ah_domain_error_if(is_closed_) << "Polygon is already closed";

    // Check if the new point is colinear with the last segment
    if (num_vertex_ > 1)
      {
        if (const Segment last_sg = get_last_segment(); point.is_colinear_with(last_sg))
          {
            ah_domain_error_if(point.is_inside(last_sg))
	            << "new vertex is inside of last polygon's segment";

            // Colinear point: replace the last vertex with this new one
            auto & last_vertex = const_cast<Vertex &>(get_last_vertex());
            Point & last_point = last_vertex;
            last_point = point;

            update_extreme_points(point);

            return;
          }
      }

    // If we have at least 3 vertices, verify no self-intersection
    if (num_vertex_ >= 3)
      {
        // New edge that would be formed by adding this point
        const Segment new_side(get_last_vertex().to_point(), point);

        // Check for intersection with all edges except the last one
        // (which shares a vertex with the new edge)
        for (Segment_Iterator it(*this); true; it.next_ne())
          {
            const Segment curr_side = it.get_current_segment();

            if (curr_side == get_last_segment())
              break;

            ah_domain_error_if(curr_side.intersects_with(new_side)) << "new side intersects";
          }
      }

    // Insert new vertex
    vertex_list_.append(new Vertex(point));
    update_extreme_points(point);
    ++num_vertex_;
  }

  /** @brief Add a vertex using coordinates.
   *  @param x The x-coordinate.
   *  @param y The y-coordinate.
   *  @see add_vertex(const Point &)
   */
  void add_vertex(const Geom_Number & x, const Geom_Number & y)
  {
    add_vertex(Point(x, y));
  }

  /// @brief Append a vertex (Aleph container protocol).
  /// @param point The point to add as a vertex.
  /// @see add_vertex(const Point &)
  void append(const Point & point) { add_vertex(point); }

  Special_Ctors(Polygon, Point);

  /** @brief Remove a vertex from the polygon.
   *
   *  Removes the specified vertex from the polygon.
   *
   *  @param v The vertex to remove.
   *  @throws std::domain_error If @p v does not belong to this polygon.
   *  @warning This method does not handle colinearity cases that might arise
   *           from removing a vertex (the caller is responsible for validation).
   */
  void remove_vertex(const Vertex & v)
  {
    Vertex *victim = nullptr;
    for (Vertex_Iterator it(*this); it.has_curr(); it.next_ne())
      if (&it.get_current_vertex() == &v)
        {
          victim = &const_cast<Vertex &>(it.get_current_vertex());
          break;
        }

    ah_domain_error_if(victim == nullptr) << "Vertex does not belong to polygon";

    // TODO: Handle colinearity cases

    victim->del();
    --num_vertex_;
    delete victim;
  }

  /** @brief Close the polygon.
   *
   *  Closes the polygon by logically connecting the last vertex to the first.
   *  This enables segment iteration to include the closing edge and allows
   *  containment testing.
   *
   *  @throws std::domain_error If the polygon is already closed or if closing
   *          would cause a self-intersection.
   *  @pre The polygon must have at least 3 vertices to form a valid closed shape.
   */
  void close()
  {
    ah_domain_error_if(is_closed_) << "Polygon is already closed";

    if (num_vertex_ >= 4)
      {
        // Check for intersection with all edges except the first
        // and last (which share endpoints with the closing edge)
        const Segment last_side(get_first_vertex().to_point(), get_last_vertex().to_point());

        Segment_Iterator it(*this);

        for (it.next(); true; it.next_ne())
          {
            const Segment curr_side = it.get_current_segment();

            if (curr_side == get_last_segment())
              break;

            ah_domain_error_if(curr_side.intersects_with(last_side))
	            << "closing causes an intersection";
          }
      }

    is_closed_ = true;
  }

  /** @brief Check if a point is inside the polygon (or on its boundary).
   *
   *  Tests whether the given point lies inside or on the boundary of this
   *  closed simple polygon. Uses the winding number algorithm, which works
   *  correctly for both convex and non-convex polygons.
   *
   *  @param p The point to test.
   *  @return True if @p p is inside or on the boundary.
   *  @throws std::domain_error If the polygon is not closed.
   */
  [[nodiscard]] bool contains(const Point & p) const
  {
    ah_domain_error_if(not is_closed_) << "Polygon is not closed";

    int winding = 0;

    for (Segment_Iterator it(*this); it.has_curr(); it.next_ne())
      {
        const Segment edge = it.get_current_segment();
        const Point & a = edge.get_src_point();
        const Point & b = edge.get_tgt_point();

        if (on_segment(edge, p))
          return true;

        if (a.get_y() <= p.get_y())
          {
            if (b.get_y() > p.get_y() and
                orientation(a, b, p) == Orientation::CCW)
              ++winding;
          }
        else
          {
            if (b.get_y() <= p.get_y() and
                orientation(a, b, p) == Orientation::CW)
              --winding;
          }
      }

    return winding != 0;
  }

  /// @deprecated Use contains() instead.
  [[deprecated("Use contains() instead")]]
  [[nodiscard]] bool contains_to(const Point & p) const { return contains(p); }

  /** @brief Construct a polygon from a Triangle.
   *
   *  Creates a closed 3-vertex polygon from the given triangle.
   *
   *  @param tr The triangle to convert.
   */
  Polygon(const Triangle & tr)
    : num_vertex_(0), is_closed_(false)
  {
    add_vertex(tr.get_p1());
    add_vertex(tr.get_p2());
    add_vertex(tr.get_p3());

    close();
  }
};


/** @brief A regular polygon defined by center, side length, and vertex count.
 *
 *  Regular_Polygon represents an n-sided regular polygon (equilateral and
 *  equiangular) defined parametrically by its center point, the length of
 *  each side, and the number of vertices.
 *
 *  ## Construction
 *
 *  The polygon is constructed using the law of sines to calculate the
 *  circumradius from the side length:
 *
 *  ```
 *  r = side_size * sin(alpha) / sin(beta)
 *  ```
 *
 *  Where:
 *  - `beta = 2π/n` (central angle)
 *  - `alpha = (π - beta)/2` (angle between radius and side)
 *
 *  ## Vertex Numbering
 *
 *  Vertices are numbered 0 to n-1. Vertex 0 is positioned at angle `angle`
 *  from the negative y-axis (i.e., pointing "south" when angle=0).
 *
 *  ## Example
 *
 *  @code{.cpp}
 *  // Create a regular hexagon centered at (100, 100) with side length 50
 *  Regular_Polygon hexagon(Point(100, 100), 50.0, 6);
 *
 *  // Iterate over vertices
 *  for (size_t i = 0; i < hexagon.size(); ++i)
 *    std::cout << hexagon.get_vertex(i).to_string() << "\n";
 *
 *  // Create rotated square (45 degrees = PI/4 radians)
 *  Regular_Polygon diamond(Point(0, 0), 100.0, 4, PI/4);
 *  @endcode
 *
 *  @see Polygon
 *  @ingroup Geometry
 */
class Regular_Polygon : public Geom_Object
{
  Point center_; ///< Center point of the polygon
  double side_size_; ///< Length of each side
  size_t num_vertex_; ///< Number of vertices (sides)
  double angle_; ///< Rotation angle in radians
  double r_; ///< Circumradius (distance from center to vertices)
  double beta_; ///< Central angle between adjacent vertices (2π/n)

public:
  /// @brief Default constructor. Creates an invalid empty polygon.
  Regular_Polygon() : side_size_(0), num_vertex_(0)
  { /* empty */
  }

  /** @brief Construct a regular polygon.
   *
   *  @param c Center point of the polygon.
   *  @param side_sz Length of each side.
   *  @param n Number of vertices (must be >= 3).
   *  @param ang Rotation angle in radians (default 0).
   *  @throws std::domain_error If n < 3.
   */
  Regular_Polygon(Point c,
                  const double & side_sz,
                  const size_t & n,
                  const double & ang = 0)
    : center_(std::move(c)), side_size_(side_sz), num_vertex_(n), angle_(ang), beta_(2 * PI / n)
  {
    ah_domain_error_if(n < 3)
      << "Polygon sides is less than 3";

    // Calculate radius using law of sines with angles between vertices
    // and the angle between radius and side
    const double alpha = (PI - beta_) / 2; // angle between radius and side
    r_ = side_size_ * sin(alpha) / sin(beta_);
  }

  /// @brief Get the side length.
  /// @return Length of each side.
  [[nodiscard]] const double &get_side_size() const { return side_size_; }

  /// @brief Get the center point.
  /// @return Reference to the center point.
  [[nodiscard]] const Point &get_center() const { return center_; }

  /// @brief Get the number of vertices.
  /// @return Number of vertices (and sides).
  [[nodiscard]] const size_t &size() const { return num_vertex_; }

  /// @brief Get the circumradius.
  /// @return Distance from center to each vertex.
  [[nodiscard]] const double &radius() const { return r_; }

  /// @brief Check if the polygon is closed.
  /// @return Always true for regular polygons.
  static bool is_closed() { return true; }

  /** @brief Get the i-th vertex of the polygon.
   *
   *  Vertex 0 is at angle `angle` from the negative y-axis. Subsequent
   *  vertices are placed at equal angular intervals counter-clockwise.
   *
   *  @param i Vertex index (0 to size()-1).
   *  @return The vertex point.
   *  @throws std::out_of_range If i >= size().
   */
  [[nodiscard]] Point get_vertex(const size_t & i) const
  {
    ah_out_of_range_error_if(i >= num_vertex_)
      << "vertex " << i << " is greater than " << num_vertex_;

    // The first segment is the negative vertical from center
    // Initial point is the target of this segment with origin at center
    Segment sg(center_, center_ - Point(0, r_));

    sg.rotate(i * beta_ + angle_);

    return sg.get_tgt_point();
  }

  /// @brief Get the first vertex (index 0).
  /// @return The first vertex point.
  [[nodiscard]] Point get_first_vertex() const { return get_vertex(0); }

  /// @brief Get the last vertex (index size()-1).
  /// @return The last vertex point.
  [[nodiscard]] Point get_last_vertex() const { return get_vertex(num_vertex_ - 1); }

  /// @brief Get the first segment (edge).
  /// @return Segment from vertex 0 to vertex 1.
  [[nodiscard]] Segment get_first_segment() const
  {
    return {get_vertex(0), get_vertex(1)};
  }

  /// @brief Get the last segment (closing edge).
  /// @return Segment from vertex 0 to last vertex.
  [[nodiscard]] Segment get_last_segment() const
  {
    return {get_vertex(0), get_vertex(num_vertex_ - 1)};
  }

  /** @brief Iterator over the vertices of a regular polygon.
   *
   *  This iterator provides the same interface as Polygon::Vertex_Iterator
   *  to enable generic code that works with both polygon types.
   *
   *  @code{.cpp}
   *  Regular_Polygon hex(Point(0, 0), 50, 6);
   *  for (Regular_Polygon::Vertex_Iterator it(hex); it.has_curr(); it.next())
   *    std::cout << it.get_current_vertex().to_string() << "\n";
   *  @endcode
   */
  class Vertex_Iterator
  {
    const Regular_Polygon & poly_; ///< Reference to the polygon
    size_t curr_; ///< Current vertex index
    Vertex vertex_; ///< Cache for compatibility with Polygon::Vertex_Iterator

  public:
    /// @brief Construct iterator from a regular polygon.
    /// @param poly The polygon to iterate over.
    Vertex_Iterator(const Regular_Polygon & poly)
      : poly_(poly), curr_(0)
    {
      // empty
    }

    /// @brief Check if there is a current vertex.
    /// @return True if a current vertex exists.
    [[nodiscard]] bool has_curr() const { return curr_ < poly_.size(); }

    /// @brief Get the current vertex.
    /// @return Reference to the current vertex.
    /// @throws std::overflow_error If no current vertex exists.
    Vertex &get_current_vertex()
    {
      ah_overflow_error_if(not has_curr()) << "Iterator has not current";

      vertex_ = poly_.get_vertex(curr_);

      return vertex_;
    }

    /// @brief Advance to the next vertex (no exception on overflow).
    void next_ne() noexcept { ++curr_; }

    /// @brief Advance to the next vertex.
    /// @throws std::overflow_error If already at end.
    void next()
    {
      ah_overflow_error_if(not has_curr()) << "Iterator has not current";
      next_ne();
    }

    /// @brief Move to previous vertex.
    /// @throws std::underflow_error If already at the beginning.
    void prev()
    {
      --curr_;

      ah_underflow_error_if(not has_curr()) << "Iterator has not current";
    }
  };

  /** @brief Iterator over the edges (segments) of a regular polygon.
   *
   *  This iterator provides the same interface as Polygon::Segment_Iterator
   *  to enable generic code that works with both polygon types.
   *
   *  @code{.cpp}
   *  Regular_Polygon hex(Point(0, 0), 50, 6);
   *  for (Regular_Polygon::Segment_Iterator it(hex); it.has_curr(); it.next())
   *    std::cout << it.get_current_segment().to_string() << "\n";
   *  @endcode
   */
  class Segment_Iterator
  {
    const Regular_Polygon & poly_; ///< Reference to the polygon
    size_t curr_; ///< Current segment index

  public:
    /// @brief Construct segment iterator from a regular polygon.
    /// @param poly The polygon to iterate over.
    Segment_Iterator(const Regular_Polygon & poly)
      : poly_(poly), curr_(0)
    {
      // empty
    }

    /// @brief Check if there is a current segment.
    /// @return True if a current segment exists.
    [[nodiscard]] bool has_curr() const { return curr_ < poly_.size(); }

    /// @brief Get the current segment (edge).
    /// @return The segment from the current vertex to the next.
    /// @throws std::overflow_error If no current segment exists.
    [[nodiscard]] Segment get_current_segment() const
    {
      ah_overflow_error_if(not has_curr()) << "Iterator has not current";

      return {poly_.get_vertex(curr_), poly_.get_vertex((curr_ + 1) % poly_.size())};
    }

    /// @brief Advance to the next segment (no exception on overflow).
    void next_ne() noexcept { ++curr_; }

    /// @brief Advance to the next segment.
    /// @throws std::overflow_error If already at end.
    void next()
    {
      ah_overflow_error_if(not has_curr()) << "Iterator has not current";
      next_ne();
    }

    /// @brief Move to the previous segment.
    /// @throws std::underflow_error If already at the beginning.
    void prev()
    {
      --curr_;

      ah_underflow_error_if(not has_curr()) << "Iterator has not current";
    }
  };

  /// @brief Get the lowest point of the bounding circle.
  /// @return Point at the bottom of the circumscribed circle.
  /// @note For simplicity, returns cardinal points of the circumcircle,
  ///       not the actual extreme vertices.
  [[nodiscard]] Point lowest_point() const { return center_ + Point(0, -r_); }

  /// @brief Get the highest point of the bounding circle.
  /// @return Point at the top of the circumscribed circle.
  [[nodiscard]] Point highest_point() const { return center_ + Point(0, r_); }

  /// @brief Get the leftmost point of the bounding circle.
  /// @return Point at the left of the circumscribed circle.
  [[nodiscard]] Point leftmost_point() const { return center_ + Point(-r_, 0); }

  /// @brief Get the rightmost point of the bounding circle.
  /// @return Point at the right of the circumscribed circle.
  [[nodiscard]] Point rightmost_point() const { return center_ + Point(r_, 0); }
};

/** \overload
 *
 *  Copy vertices from a regular polygon. See declaration for parameter documentation.
 */
inline void Polygon::copy_regular_polygon(const Regular_Polygon & poly)
{
  assert(num_vertex_ == 0 and not is_closed_);

  for (size_t i = 0; i < poly.size(); ++i)
    add_vertex(poly.get_vertex(i));

  close();
}

// ============================================================================
// Stream output operators for polygon classes
// ============================================================================

inline std::ostream & operator<<(std::ostream & o, const Polygon & p)
{
  o << "Polygon(n=" << p.size() << ", "
    << (p.is_closed() ? "closed" : "open") << ", [";
  size_t count = 0;
  for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
    {
      if (count > 0) o << ", ";
      o << it.get_current_vertex();
      ++count;
    }
  o << "])";
  return o;
}

inline std::ostream & operator<<(std::ostream & o, const Regular_Polygon & p)
{
  o << "Regular_Polygon(center=" << p.get_center()
    << ", n=" << p.size()
    << ", radius=" << p.radius() << ")";
  return o;
}

#endif // POLYGON_H
