

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

template <typename __Graph_Node = Graph_Anode<int>, 
	  typename __Graph_Arc  = Graph_Aarc<int>>
class Array_Digraph : public Array_Graph<__Graph_Node, __Graph_Arc>
{
  using GT = Array_Graph<__Graph_Node, __Graph_Arc>;

public:

  using Node = __Graph_Node;

  using Arc = __Graph_Arc;

  Array_Digraph() noexcept
  {
    this->digraph = true; 
  }
  
  Array_Digraph(const Array_Digraph & dg)
    : Array_Graph<Node, Arc>((Array_Digraph&) dg)
  {
    this->digraph = true; 
  }

  Array_Digraph & operator = (const Array_Digraph<Node, Arc> & g)
  {
    if (this == &g) 
      return *this;

    copy_graph(*this, g, false);

    return *this;
  }

  Array_Digraph(Array_Digraph && dg) noexcept : GT()
  {
    this->digraph = true; 
    this->swap(dg);
  }

  Array_Digraph & operator = (Array_Digraph<Node, Arc> && g) noexcept
  {
    this->swap(g);

    return *this;
  }
};


} // namespace Aleph {

# endif //  TPL_AGRAPH_H
