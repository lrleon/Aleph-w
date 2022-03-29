

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

      typename Distance::Distance_Type
  get_min_path(const GT & tree, typename GT::Node * end, Path<GT> & path)
  {
    if (ptr_g == nullptr)
      throw std::domain_error("Min path has not been computed");

    auto ts = mapped_node<GT>(s);
    auto te = mapped_node<GT>(end);

    Path<GT> tree_path(tree);
    Total total;
    (Find_Path_Depth_First<GT, Itor, Total> (total)) (tree, ts, te, tree_path);

    path.empty();
    path.init(s);
    typename Path<GT>::Iterator it(tree_path);
    for (it.next(); it.has_curr(); it.next_ne())
      path.append(mapped_node<GT>(it.get_current_node_ne()));

    return total.dist;
  }
};



# undef DNI
# undef PARENT
# undef TREENODE
# undef ACC
# undef HEAPNODE
# undef DAI
# undef ARC_DIST
# undef TREEARC
# undef POT
# undef GRAPHNODE
} // end namespace Aleph
# endif // DIJKSTRA_H
