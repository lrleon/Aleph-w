

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

  void compute_blocks(DynDlist<GT> &                block_list, 
		      GT &                          cut_graph, 
		      DynDlist<typename GT::Arc*> & cross_arc_list)
  {
    if (state < Cut_Nodes_Computed)
      throw std::logic_error("Cut nodes have not been computed");

    if (state == Cut_Nodes_Computed)
      paint_subgraphs();

    const long & num_colors = curr_color;
    
    DynArray<GT*> blocks; // bloques en un arreglo para rápido
			  // acceso. Que esté o no vacío indica si ha
			  // sido o no procesado.
    blocks.reserve(num_colors);

        // crear lista de componentes vacíos ordenador por color i
    for (int i = 0; i < num_colors; ++i)
      blocks.access(i) = &block_list.append(GT());

        // Recorrer los nodos y copiar y mapear según color
    for (typename GT::Node_Iterator it(*gptr); it.has_curr(); it.next_ne())
      {
	auto p = it.get_curr_ne();
	if (IS_NODE_VISITED(p, Build_Subtree))
	  continue;

	if (is_a_cut_node <GT> (p))
	  continue;

	const long color = get_color<GT>(p);

	GT & sg = *blocks.access(color - 1);

	create_and_map_node(p, color, sg);

	map_subgraph(sg, p, color);	
      }

    map_cut_graph(cut_graph, cross_arc_list);
  }
};

} // end namespace Aleph

# endif // TPL_CUT_NODES_H
