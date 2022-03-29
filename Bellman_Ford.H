

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

  bool operator () (const GT & g, typename GT::Node * s,
		    Path<GT> & path, Distance & d, SA & sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(s, path);
  }

  bool operator () (const GT & g, typename GT::Node * s,
		    Path<GT> & path, Distance && d = Distance(), 
		    SA && sa = SA()) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(s, path);
  }

  Path<GT> operator () (const GT & g, typename GT::Node * s, 
			Distance & d, SA & sa, double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      search_negative_cycle(s, it_factor);
  }

  Path<GT> operator () (const GT & g, typename GT::Node * s, 
			Distance && d = Distance(), SA && sa = SA(), 
			double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      search_negative_cycle(s, it_factor);
  }

  Path<GT> operator () (const GT & g, Distance & d, SA & sa, 
			double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
    search_negative_cycle(it_factor);
  }

  Path<GT> 
  operator () (const GT & g, Distance && d = Distance(), SA && sa = SA(), 
	       double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
    search_negative_cycle(it_factor);
  }
};


# undef NI
# undef IDX
# undef ACU
} // end namespace Aleph

# endif // BELLMAN_FORD_H
