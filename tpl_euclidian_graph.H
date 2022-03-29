

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

    }

    Point & add_point(typename __Euclidian_Graph::Node * node)
    {
      if (node == nullptr)
        throw std::domain_error("node is nullptr");
      points.append(node->get_position());
      Point & p = points.top();
      if (points.size() == 1)
        ptr_west_point = ptr_north_point = ptr_east_point = ptr_south_point = 
	  &points.access(0);
      else
        {
          if (p.get_x() < ptr_west_point->get_x())
            ptr_west_point = &p;
          if (p.get_y() > ptr_north_point->get_y())
            ptr_north_point = &p;
          if (p.get_x() > ptr_east_point->get_x())
            ptr_east_point = &p;
          if (p.get_y() < ptr_south_point->get_y())
            ptr_south_point = &p;
        }
      return p;
    }

    const Point & get_west_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_west_point;
    }

    const Point & get_north_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_north_point;
    }

    const Point & get_east_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_east_point;
    }

    const Point & get_south_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_south_point;
    }

    const Geom_Number get_width() const
    {
      if (points.size() < 1)
        return Geom_Number(0);
      return ptr_east_point->get_x() - ptr_west_point->get_x();
    }

    const Geom_Number get_height() const
    {
      if (points.size() < 1)
        return Geom_Number(0);
      return ptr_north_point->get_y() - ptr_south_point->get_y();
    }

    const Geom_Number & get_x_node_ratio() const { return x_node_ratio; }

    void set_x_node_ratio(const Geom_Number & _x_node_ratio) 
    { x_node_ratio = _x_node_ratio; }

    const Geom_Number & get_y_node_ratio() const { return y_node_ratio; }

    void set_y_node_ratio(const Geom_Number & _y_node_ratio) 
    { y_node_ratio = _y_node_ratio; }

    const Geom_Number & get_x_scale() const { return x_scale; }

    void set_x_scale(const Geom_Number & _x_scale) { x_scale = _x_scale; }

    const Geom_Number & get_y_scale() const { return y_scale; }

    void set_y_scale(const Geom_Number & _y_scale) { y_scale = _y_scale; }

  }; // End class Abstract_Euclidian_Plane

} // End namespace Aleph

# endif // TPL_EUCLIDIAN_GRAPH_H

