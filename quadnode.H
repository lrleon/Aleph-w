

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

  DynList<QuadNode *> get_neighbors()
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

  DynList<Point> & get_points_set()
  {
    return points;
  }

  template <class Op>
  void for_each_point(Op & op)
  {
    for (Point & point : points)
      op(point);
  };

  template <class Op>
  void for_each_point(Op && op = Op())
  {
    for_each_point<Op>(op);
  }

  QuadNode & operator = (const QuadNode &) = delete;
};

# endif // QUADNODE_H

