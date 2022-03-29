

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

  bool operator () (GT & g, SA & sa) const
  {
    if (g.is_digraph()) // sólo es válida para grafos, no digrafos
      throw std::domain_error("test_connectivity() does not work on digraphs");

    if (g.get_num_arcs() < g.get_num_nodes() - 1) 
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);
 
    return traversal(g) == g.get_num_nodes();
  }
};



# endif // TPL_TEST_CONNECTIVITY_H
