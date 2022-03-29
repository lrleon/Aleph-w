

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

  template <class Op>
  DynList<Arc*> filter_out_arcs(Node * p, Op & op) const noexcept(noexcept(op))
  {
    DynList<Arc*> ret;
    for_each_out_arc(p, [&ret, &op] (auto a) 
		     {
		       if (op(a))
			 ret.append(a);
		     });
    return ret;
  }

  /// \overload filter_out_arcs(Node * p, Op & op)
  template <class Op>
  auto filter_out_arcs(Node * p, Op && op = Op()) const noexcept(noexcept(op))
  {
    return filter_out_arcs(p, op);
  }
};



# endif // GRAPH_DRY_H
