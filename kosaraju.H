
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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
# ifndef KOSARAJU_H
# define KOSARAJU_H

# include <tpl_graph_utils.H>

namespace Aleph {

     template <class GT> inline static
void __dfp(const GT & g, typename GT::Node * p, 
	   DynArray<typename GT::Node*> & df)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true); 

      // recorre en profundidad los arcos de p 
  for (auto it = g.get_out_it(p); it.has_curr(); it.next_ne())
    {
      auto a = it.get_current_arc_ne();
      if (IS_ARC_VISITED(a, Depth_First))
        continue;

      ARC_BITS(a).set_bit(Depth_First, true); 

      __dfp(g, it.get_tgt_node(), df);
    }
  df.append(p); // asignación de df(p) en sufijo
  NODE_COUNTER(p) = df.size();
}

    /* Recorre el grafo invertido e inserta los nodos alcanzables en el
       sub-grafo blk. A cada bloque le coloca el color color

     */
     template <class GT> inline static
void __dfp(const GT & g, typename GT::Node * p, GT & blk, const int & color)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true); 

  auto q = blk.insert_node(p->get_info());
  NODE_COUNTER(q) = color;
  GT::map_nodes(p, q);

  for (auto it = g.get_out_it(p); it.has_curr(); it.next_ne())
    {
      auto a = it.get_current_arc_ne();
      if (IS_ARC_VISITED(a, Depth_First))
        continue;
      ARC_BITS(a).set_bit(Depth_First, true); 

      __dfp(g, it.get_tgt_node(), blk, color);
    }
}

    /** Calcula los componentes fuertemente conexos de un grafo según el
        algoritmo de Kosaraju. 

        kosaraju_connected_components() toma un digrafo g,
        "supuestamente débilmente conexo", calcula sus subgrafos o componentes
        inconexos y los almacena en una lista dinámica list de subdigrafos
        mapeados, tanto en los nodos como en los arcos. El énfasis en el
        supuesto carácter conexo del grafo se debe a que si el grafo no
        fuese conexo, entonces la lista resultante contendría un solo
        elemento correspondiente al digrafo copia mapeado de g. Si este
        fuese el caso, entonces es preferible hacerlo con la función
        copy_graph().

        La función se basa en el algoritmo de Tarjan.

        La función emplea dos parámetros tipo:
        -# GT: la clase digrafo.
        -# SA: el filtro de arcos que emplea el iterador interno.

        La función se vale del bit build_subtree para marcar nodos y
        arcos visitados.

        El algoritmo internamente descubre los componentes mediante
        búsquedas en profundidad.

        @param[in] g el grafo.
        @param[out] blk_list lista de sub-grafos mapeados a g
        correspondiente a los componentes fuertemente conexos de g.
        @param[out] arc_list lista de arcos de cruce entre los componentes.

        @see tarjan_connected_components()
        @ingroup Grafos
     */
     template <class GT> inline 
void kosaraju_connected_components(const GT &                     g,
                                   DynList <GT> &                blk_list,
                                   DynList<typename GT::Arc *> & arc_list)
{
  g.reset_nodes();
  g.reset_arcs();

  DynArray<typename GT::Node*> df; // arreglo de df por sufijo

      // recorre en profundidad y marca en sufijo los nodos
  for (auto it = g.get_node_it(); it.has_curr() and df.size() < g.vsize();
       it.next_ne())
    __dfp(g, it.get_curr(), df);

  GT gi = invert_digraph(g); // gi es el grafo invertido de g

  DynArray<GT*> array; // areglo de sub-grafos

  for (int i = df.size() - 1, color = 0; i >= 0; i--)
    {
      auto gp = df.access(i);
      auto bp = mapped_node<GT>(gp);
      if (IS_NODE_VISITED(bp, Depth_First))
        continue;
      
      GT & blk = blk_list.append(GT());
      array[color] = &blk;
      
      __dfp(gi, bp, blk, color++); // recorre el grafo inverso 
                                            // inserta los nodos en blk 
      assert(NODE_COLOR(mapped_node<GT>(bp)) == color - 1);
    }

  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto a  = it.get_curr();
      auto gs = g.get_src_node(a);
      auto gt = g.get_tgt_node(a);

      auto bs = mapped_node<GT>(mapped_node<GT>(gs));
      auto bt = mapped_node<GT>(mapped_node<GT>(gt));

      const long & color = NODE_COLOR(bs);

      if (color == NODE_COLOR(bt))
        {
          typename GT::Arc * ba = array.access(color)->insert_arc(bs, bt);
          GT::map_arcs(a, ba);
        }
      else 
        arc_list.append(a); 
    }
} 


    /* Recorre el grafo invertido e inserta los nodos alcanzables en la
       lista list. A cada nodo le coloca el color color

     */
     template <class GT> inline static
void __dfp(const GT & g, 
	   typename GT::Node * p, 
	   DynList<typename GT::Node *> & list)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true); 

  list.append(mapped_node<GT>(p));

  for (auto it = g.get_out_it(p); it.has_curr(); it.next_ne())
    {
      auto a = it.get_current_arc_ne();
      if (IS_ARC_VISITED(a, Depth_First))
        continue;
      ARC_BITS(a).set_bit(Depth_First, true); 

      __dfp(g, it.get_tgt_node(), list);
    }
}

    /** Calcula los componentes fuertemente conexos de un grafo según el
        algoritmo de Kosaraju. 

         kosaraju_connected_components() toma un digrafo g,
        "supuestamente débilmente conexo", calcula sus subgrafos o componentes
        inconexos y los almacena en una lista dinámica list de subdigrafos
        mapeados, tanto en los nodos como en los arcos. El énfasis en el
        supuesto carácter conexo del grafo se debe a que si el grafo no
        fuese conexo, entonces la lista resultante contendría un solo
        elemento correspondiente al digrafo copia mapeado de g. Si este
        fuese el caso, entonces es preferible hacerlo con la función
        copy_graph().

        La función se basa en el algoritmo de Kosaraju, el cual no es el
        más eficiente conocido pero sí es el más simple.

        La función emplea dos parámetros tipo:
        -# GT: la clase digrafo.
        -# SA: el filtro de arcos que emplea el iterador interno.

        La función se vale del bit build_subtree para marcar nodos y
        arcos visitados.

        El algoritmo internamente descubre los componentes mediante
        búsquedas en profundidad.

        La diferencia y ventaja de esta rutina respecto a la versiòn que
        calcula los bloques directos es su rapidez; los bloque son
        comprendidos en listas de listas de nodos y no en lista de
        subgrafos. 

        @param[in] g el grafo.
        @param[out] list lista de listas de nodos que conforman los
        componentes. Cada lista es una listas de los nodos del componente.

        @see tarjan_connected_components()
        @ingroup Grafos
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
    __dfp(g, it.get_curr(), df);

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
