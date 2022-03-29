

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

    const size_t & get_num_arcs(Node_Info & info) const
    {
      Node * node = info_to_node(info);
      return graph.get_num_arcs(node);
    }
  };

  # define GRAPH_SPECIALIZATION(G, N, A) \
    template <typename Node_Info, typename Arc_Info>                     \
    class Dyn_##G : public Dyn_Graph<G<N<Node_Info>, A<Arc_Info>>>       \
    {                                                                    \
      typedef G<N<Node_Info>, A<Arc_Info>> Graph_Type;                   \
                                                                         \
    public:                                                              \
      Dyn_##G()                                                          \
        : Dyn_Graph <Graph_Type>() { }                                   \
                                                                         \
      Dyn_##G(const Dyn_##G & g)                                         \
        : Dyn_Graph <Graph_Type>(g) { }                                  \
                                                                         \
      Dyn_##G(Dyn_##G && g)                                              \
        : Dyn_Graph <Graph_Type>(std::move(g)) { }                       \
    };

  GRAPH_SPECIALIZATION(List_Graph, Graph_Node, Graph_Arc)

  GRAPH_SPECIALIZATION(List_Digraph, Graph_Node, Graph_Arc)

} // End namespace Aleph

# endif // TPL_DYN_GRAPH_H

