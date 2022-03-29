

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
; it.has_curr(); it.next_ne())
      {
	const Point & p = it.get_curr_ne();

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
	const Point & p = it.get_curr_ne();

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
      ret.add_vertex(it.get_curr_ne());
    
    ret.close();
    return ret;
  }
};

# endif // GEOM_ALGORITHMS_H
