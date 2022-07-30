
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef GEOM_ALGORITHMS_H
# define GEOM_ALGORITHMS_H

# include <polygon.H>
# include <htlist.H>
# include <tpl_dynSetTree.H>
# include <tpl_sort_utils.H>

// Triangulation algorithms

/** Triangulation algorithm by cutting ears.

    @author Alejandro J. Mujica
    @ingroup Geometry
*/
class CuttingEarsTriangulation
{
  using EarsSet = DynSetTree<const Vertex*, Treap_Rk>;
  
public:

  bool diagonalie(const Polygon & p, const Segment & s)
  {
    for (Polygon::Segment_Iterator it(p); it.has_curr(); it.next_ne())
      {
	Segment curr = it.get_current_segment();
		
	if ((curr.get_src_point() != s.get_src_point()) and
	    (curr.get_tgt_point() != s.get_src_point()) and
	    (curr.get_src_point() != s.get_tgt_point()) and
	    (curr.get_tgt_point() != s.get_tgt_point()) and
	    s.intersects_with(curr))
	  return false;
      }
    return true;
  }

  bool in_cone(const Polygon & p, const Vertex & a, const Vertex & b)
  {
    // a0 -> a -> a1 son vértices consecutivos
    const Vertex & a0 = p.get_prev_vertex(a);
    const Vertex & a1 = p.get_next_vertex(a);
    
    if (a0.is_to_left_on_from(a, a1))
      return a0.is_to_left_from(a, b) and a1.is_to_left_from(b, a);
    
    return not (a1.is_to_left_on_from(a, b) and
		a0.is_to_left_on_from(b, a));
  }
  
  bool diagonal(const Polygon & p, const Vertex & a, const Vertex & b)
  {
    return in_cone(p, a, b) and in_cone(p, b, a) and
      diagonalie(p, Segment(a, b));
  }

  EarsSet init_ears(const Polygon & p)
  {
    EarsSet ret;

    for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
      {
	Vertex & curr = it.get_current_vertex();
	const Vertex & prev = p.get_prev_vertex(curr);
	const Vertex & next = p.get_next_vertex(curr);
	if (diagonal(p, prev, next))
	  ret.insert(&curr);
      }

    return ret;
  }
  
public:

  DynList<Triangle> operator () (Polygon & p)
  {
    if (p.size() < 3)
      throw std::domain_error("Polygon has less than 3 vertexs");

    EarsSet ears = init_ears(p);

    DynList<Triangle> ret;

    while (p.size() > 3)
      {
	const Vertex * curr = ears.remove_pos(0);

	const Vertex & prev = p.get_prev_vertex(*curr);
	const Vertex & prev_prev = p.get_prev_vertex(prev);
	const Vertex & next = p.get_next_vertex(*curr);
	const Vertex & next_next = p.get_next_vertex(next);

	if (diagonal(p, prev_prev, next))
	  ears.insert(&prev);
	else
	  ears.remove(&prev);
	
	if (diagonal(p, prev, next_next))
	  ears.insert(&next);
	else
	  ears.remove(&next);
	
	ret.append(Triangle(prev, *curr, next));
	
	p.remove_vertex(*curr);
      }
    
    assert(p.size() == 3);

    const Vertex & a = p.get_first_vertex();
    const Vertex & b = a.next_vertex();
    const Vertex & c = b.next_vertex();

    ret.append(Triangle(a, b, c));

    return ret;
  }
};

// Convex hull algorithms

/** Builds a convex hull from a points set by brute force.

    @author Alejandro J. Mujica
    @ingroup Geometry
*/
class BruteForceConvexHull
{
  struct CmpSegment
  {
    bool cmp_point(const Point & p1, const Point & p2) const
    {
      if (p1.get_x() < p2.get_x())
	return true;
      
      return not (p2.get_x() < p1.get_x()) and p1.get_y() < p2.get_y();
    }
    
    bool operator () (const Segment & s1, const Segment & s2)
    {
      if (cmp_point(s1.get_src_point(), s2.get_src_point()))
	return true;
      
      return not (cmp_point(s2.get_src_point(), s1.get_src_point())) and
	cmp_point(s1.get_tgt_point(), s2.get_tgt_point());
    }
  };

  using SegmentSet = DynSetTree<Segment, Treap_Rk, CmpSegment>;

  using PointIt = DynList<Point>::Iterator;

  bool are_all_points_on_left(DynList<Point> & l, const Segment & s)
  {
    for (PointIt it(l); it.has_curr(); it.next_ne())
      {
	const Point & p = it.get_curr();
	
	if (p.is_to_right_from(s))
	  return false;
      }
    
    return true;
  }
  
  SegmentSet extreme_edges(DynList<Point> & point_set)
  {
    SegmentSet ret;
  
    for (PointIt i(point_set); i.has_curr(); i.next_ne())
      {
	const Point & p_i = i.get_curr();
	
	for (PointIt j(point_set); j.has_curr(); j.next_ne())
	  {
	    const Point & p_j = j.get_curr();
	    
	    if (p_i == p_j)
	      continue;
	    
	    Segment s(p_i, p_j);
	    
	    if (are_all_points_on_left(point_set, s))
	      ret.insert(s);
	  }	  
      }
    
    return ret;
  }
  
public:
  Polygon operator () (DynList<Point> & point_set)
  {
    Polygon ret;

    SegmentSet extremes = extreme_edges(point_set);

    Segment first_segment = extremes.remove_pos(0);
    ret.add_vertex(first_segment.get_src_point());
    ret.add_vertex(first_segment.get_tgt_point());

    while (true)
      {
	const Vertex & last_vertex = ret.get_last_vertex();
	
	Segment * ptr = extremes.find_ptr([&last_vertex](const Segment & s) {
	    return s.get_src_point() == last_vertex;
	  });

	assert(ptr != nullptr);

	if (ptr->get_tgt_point() == ret.get_first_vertex())
	  break;
	
	ret.add_vertex(ptr->get_tgt_point());

	extremes.remove(*ptr);
      }

    ret.close();
    return ret;
  }
};

/** Builds a convex hull from a points set by gift wrapping algorithm.

    @author Alejandro J. Mujica
    @ingroup Geometry
*/
class GiftWrappingConvexHull
{
  Point * get_lowest_point(DynList<Point> & point_set)
  {
    DynList<Point>::Iterator it(point_set);
    Point * ret = &it.get_curr();
    it.next();

    for ( /* nothing */; it.has_curr(); it.next_ne())
      {
	Point & p = it.get_curr();

	if (p.get_y() < ret->get_y())
	  ret = &p;
      }

    return ret;
  }
  
public:
  Polygon operator () (DynList<Point> & point_set)
  {
    Polygon ret;

    Point * lowest_point = get_lowest_point(point_set);
    Point * p_i = lowest_point;
    ret.add_vertex(*lowest_point);

    Segment last_segment = Segment(Point(0, 0), Point(1, 0));

    while (true)
      {
	Point * p_k = nullptr;

	double min_angle = std::numeric_limits<double>::max();

	for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
	  {
	    Point & p_j = it.get_curr();

	    if (p_j == *p_i)
	      continue;

	    double angle =
	      Segment(*p_i, p_j).counterclockwise_angle_with(last_segment);

	    if (angle < min_angle)
	      {
		min_angle = angle;
		p_k = &p_j;
	      }
	  }

	assert(p_k != nullptr);

	if (p_k == lowest_point)
	  break;

	ret.add_vertex(*p_k);

	last_segment = ret.get_last_segment();

	p_i = p_k;
      }

    ret.close();
    return ret;
  }
};

/** Builds a convex hull from a points set by quick hull algorithm.

    @author Alejandro J. Mujica
    @ingroup Geometry
*/
class QuickHull
{
  Point get_fartest_point(DynList<Point> & point_set, const Segment & s)
  {
    Geom_Number max_distance = 0;
    Point ret;
    
    for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
      {
	const Point & p = it.get_curr();

	Segment s1 = s.get_perpendicular(p);

	Geom_Number sz = s1.size();

	if (sz > max_distance)
	  {
	    ret = p;
	    max_distance = sz;
	  }
      }

    return ret;
  }

  std::pair<DynList<Point>, DynList<Point>>
    get_right_points(DynList<Point> & point_set,
		     const Point & a, const Point & b, const Point & c)
  {
    std::pair<DynList<Point>, DynList<Point>> ret;

    while (not point_set.is_empty())
      {
	Point p = point_set.remove_first();

	if (p != a and p != c and p.is_to_right_from(a, c))
	  {
	    ret.first.append(p);
	    continue;
	  }

	if (p != c and p != b and p.is_to_right_from(c, b))
	  ret.second.append(p);
      }

    return ret;
  }
  
  DynList<Point> quick_hull(DynList<Point> & point_set, const Point & a,
			    const Point & b)
  {
    if (point_set.is_empty())
      return DynList<Point>();
    
    Point c = get_fartest_point(point_set, Segment(a, b));

    auto r = get_right_points(point_set, a, b, c);

    DynList<Point> ret = quick_hull(r.first, a, c);
    DynList<Point> tmp = quick_hull(r.second, c, b);
    ret.append(c);
    ret.concat(tmp);

    return ret;
  }
  
  std::pair<Point, Point> search_extremes(DynList<Point> & point_set)
  {
    DynList<Point>::Iterator it(point_set);
    Point leftmost = it.get_curr();
    Point rightmost = it.get_curr();
    it.next();

    for ( /* nothing */; it.has_curr(); it.next_ne())
      {
	const Point & p = it.get_curr();

	if (p.get_x() < leftmost.get_x())
	  leftmost = p;

	if (p.get_x() > rightmost.get_x())
	  rightmost = p;
      }

    return make_pair(leftmost, rightmost);
  }

  std::pair<DynList<Point>, DynList<Point>>
    partition(DynList<Point> & point_set, const Point & a, const Point & b)
  {
    std::pair<DynList<Point>, DynList<Point>> ret;
    
    for (DynList<Point>::Iterator it(point_set); it.has_curr(); it.next_ne())
      {
	const Point & p = it.get_curr();

	if (p.is_to_right_from(a, b))
	  ret.first.append(p);
	else
	  ret.second.append(p);
      }

    return ret;
  }
  
public:
  Polygon operator () (DynList<Point> & point_set)
  {
    Polygon ret;

    auto e = search_extremes(point_set);
    auto p = partition(point_set, e.first, e.second);

    DynList<Point> s1 = quick_hull(p.first, e.first, e.second);
    DynList<Point> s2 = quick_hull(p.second, e.second, e.first);

    DynList<Point> convex_set;
    convex_set.append(e.first);
    convex_set.concat(s1);
    convex_set.append(e.second);
    convex_set.concat(s2);

    for (DynList<Point>::Iterator it(convex_set); it.has_curr(); it.next_ne())
      ret.add_vertex(it.get_curr());
    
    ret.close();
    return ret;
  }
};

# endif // GEOM_ALGORITHMS_H
