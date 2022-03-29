

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

  if (a.get_x() != b.get_x())
    return (((a.get_x() <= c.get_x()) and (c.get_x() <= b.get_x())) or
	    ((a.get_x() >= c.get_x()) and (c.get_x() >= b.get_x())));

  return (((a.get_y() <= c.get_y()) and (c.get_y() <= b.get_y())) or
	  ((a.get_y() >= c.get_y()) and (c.get_y() >= b.get_y())));
}



const bool intersect_improp(const Point & a, const Point & b, 
			     const Point & c, const Point & d)
{
  if (between(a, b, c) or
      between(a, b, d) or
      between(c, d, a) or
      between(c, d, b))
    return true;

  return false;
}
const bool intersectp(const Point & a, const Point & b, 
			     const Point & c, const Point & d)
{
  if (intersect_prop(a, b, c, d))
    return true;

  if (intersect_improp(a, b, c, d))
    return true;
  
  return false;
}

int read_pixels(ifstream &in){
    int c=0, i=0;
    char buffer[]={'\0','\0','\0','\0','\0'};
    while(!in.eof() && isdigit(c=in.get())) buffer[i++] = c;
    int num = atoi(buffer);
    return num;
   
}


# endif // POINT_UTILS_H
