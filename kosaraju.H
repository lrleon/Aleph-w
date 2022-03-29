

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

template <class GT> inline 
DynList<DynList<typename GT::Node*>> 
kosaraju_connected_components(const GT & g)
{
  DynList<DynList<typename GT::Node*>> list;

  g.reset_nodes();
  g.reset_arcs();
  DynArray<typename GT::Node*> df; // arreglo de df por sufijo

      // recorre en profundidad y marca en sufijo los nodos
  for (auto it = g.get_node_it(); it.has_curr() and df.size() < g.vsize(); 
       it.next_ne())
    __dfp(g, it.get_curr_ne(), df);

  GT gi = invert_digraph(g); // gi es el grafo invertido de g

  for (int i = df.size() - 1; i >= 0; i--)
    {
      auto gp = df.access(i);
      auto bp = mapped_node<GT>(gp);
      if (IS_NODE_VISITED(bp, Depth_First))
        continue;

      auto & blk = list.append(DynList<typename GT::Node*>());
      
      __dfp(gi, bp, blk);
    }

  return list;
} 



} // end namespace Aleph

# endif // KOSARAJU_H
