

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

    template <class Mat> void 
find_min_path(Mat & p, const long & src_idx, const long & tgt_idx,
              Path<typename Mat::Graph_Type> & path)
{
  typedef typename Mat::Graph_Type GT;
  GT & g                  = p.get_list_graph();
  typename GT::Node * src = p(src_idx);
  path.set_graph(g, src);
  for (int i = src_idx, j; j != tgt_idx; i = j)
    {
      j = p(i, tgt_idx);
      path.append(p(j));
    }
}

# endif // MAT_PATH_H
