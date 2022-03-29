

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

  void range(const Rectangle & rect, DynList<Point> * l)
  { 
    if (N == 0)
      return;
    
    range(root, rect, l);
  }

private:

  Node * min_node;
  Geom_Number min_dist2;

  void lr_nearest(Node * root, const Point & p) 
  {
    if (root == nullptr)
      return;
    
    if (root->rect.distance_squared_to(p) > min_dist2)
      return;
    
    Geom_Number d2 = root->point.distance_squared_to(p); // es con el rectángulo
    if (d2 < min_dist2) 
      {
	min_dist2 = d2;
	min_node = root;
      }
    
    if (p.get_x() < root->x()) // is p to left of this
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

  void bu_nearest(Node * root, const Point & p) 
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
        
    if (p.get_y() < root->y())  // is p to below this?
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

      /// Retorna el punto más cercano al punto p
  Point nearest(const Point & p) 
  { 
    if (N == 0)
      return NullPoint;
    
    min_dist2 = 1.0;
    lr_nearest(root, p);
    
    return min_node->point;
  }
};

# endif // TPL_2DTREE_H
