
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
# ifndef TPL_DYN_GRAPH_H
# define TPL_DYN_GRAPH_H

# include <tpl_agraph.H>

using namespace Aleph;

namespace Aleph
{

  /**

     @author Alejandro Mujica
     @ingroup Grafos
   */
      template <class GT>
  class Dyn_Graph
  {
  public:

    typedef typename GT::Node Node;

    typedef typename GT::Arc Arc;

    typedef typename Node::Node_Info Node_Info;

    typedef typename Arc::Arc_Info Arc_Info;

  private:

    static Node * info_to_node(Node_Info & info)
    {
      Node * node_zero = 0;
      long off_set = (long) &node_zero->get_info();
      long ptr_info = (long) &info;
      return (Node *) (ptr_info - off_set); 
    }

    static Arc * info_to_arc(Arc_Info & info)
    {
      Arc * arc_zero = 0;
      long off_set = (long) &arc_zero->get_info();
      long ptr_info = (long) &info;
      return (Arc *) (ptr_info - off_set); 
    }

    GT graph;

  public:

    /// Constructor por omisión
    Dyn_Graph()
      : graph()
    {
      // Empty
    }

    Dyn_Graph(const Dyn_Graph & g)
      : graph(g.graph)
    {
      // Empty
    }

    Dyn_Graph(Dyn_Graph && g)
      : graph(std::move(g.graph))
    {
      // Empty
    }


    /** Inserta un nodo en el grafo.
     *
     * @param info Información contenida por el nodo.
     * @return Referencia a la info insertada.
     */
    Node_Info & insert_node(const Node_Info & info)
    {
      Node * node = graph.insert_node(new Node(info));
      return node->get_info();
    }

    /** Inserta un nodo en el grafo con info proveniente de un rvalue.
     *
     * @param info Información contenida por el nodo.
     * @return Referencia a la info insertada.
     */
    Node_Info & insert_node(Node_Info && info = Node_Info())
    {
      Node * node = graph.insert_node(new Node(std::move(info)));
      return node->get_info();
    }

    /** Inserta un arco en el grafo.
     *
     * @param src_info Referencia a la info contenida del nodo que se
     * quiere como fuente.
     * @param tgt_info Referencia a la info contenida del nodo que se
     * quiere como destino.
     * @param info Información contenida por el arco. 
     * @return Referencia a la info insertada, en este caso inicializada por
     *         omisión.
     */
    Arc_Info & insert_arc(Node_Info & src_info, Node_Info & tgt_info,
			  const Arc_Info & info)
    {
      Node * src = info_to_node(src_info);
      Node * tgt = info_to_node(tgt_info);
      Arc * arc = graph.insert_arc(src, tgt);
      arc->get_info() = info;
      return arc->get_info();
    }

    /** Inserta un arco en el grafo con info proveniente de un rvalue.
     *
     * @param src_info Referencia a la info contenida del nodo que se
     quiere como fuente.
     @param tgt_info Referencia a la info contenida del nodo que se
     quiere como destino.

     @param info Información contenida por el arco. 
     @return Referencia a la info insertada, en este caso inicializada por
     omisión.
     */
    Arc_Info & insert_arc(Node_Info & src_info, Node_Info & tgt_info,
			  Arc_Info && info = Arc_Info())
    {
      Node * src = info_to_node(src_info);
      Node * tgt = info_to_node(tgt_info);
      Arc * arc = graph.insert_arc(src, tgt);
      arc->get_info() = std::move(info);
      return arc->get_info();
    }

    /** Consulta la info del nodo fuente de un arco dado.
     *
     * @param info Referencia a la info contenida por el arco.
     * @return Referencia a la info contenida por el nodo fuente.
     */
    Node_Info & get_src_node(Arc_Info & info)
    {
      Arc * arc = info_to_arc(info);
      Node * src = graph.get_src_node(arc);
      return src->get_info();
    }

    /** Consulta la info del nodo destino de un arco dado.
     
      @param info Referencia a la info contenida por el arco.
      @return Referencia a la info contenida por el nodo destino.
     */
    Node_Info & get_tgt_node(Arc_Info & info)
    {
      Arc * arc = info_to_arc(info);
      Node * tgt = graph.get_tgt_node(arc);
      return tgt->get_info();
    }

    Node_Info & get_connected_node(Node_Info & node_info, Arc_Info & arc_info)
    {
      Node * node = info_to_node(node_info);
      Arc * arc = info_to_arc(arc_info);
      Node * connected_node(node, arc);
      return connected_node->get_info();
    }

    /** Elimina un arco del grafo.
     *
     * @param info Referencia a la información almacenada por el arco.
     */
    void remove_arc(Arc_Info & info)
    {
      Arc * arc = info_to_arc(info);
      graph.remove_arc(arc);
    }

    /** Elimina un nodo del grafo.
     *
     * @param info Referencia a la información almacenada por el nodo.
     */
    void remove_node(Node_Info & info)
    {
      Node * node = info_to_node(info);
      graph.remove_node(node);
    }

    /** Retorna el numero de nodos del grafo.
     *
     * @return Cantidad de nodos del grafo.
     */
    const size_t & get_num_nodes() const
    {
      return graph.get_num_nodes();
    }

    /** Retorna el numero de arcos del grafo.
     *
     * @return Cantidad de arcos del grafo.
     */
    const size_t & get_num_arcs() const
    {
      return graph.get_num_arcs();
    }

    /** Retorna el numero de arcos de un nodo dato.
     *
     * @param info Referencia a la información almacenada en el nodo deseado.
     * @return Cantidad de arcos adyacentes al nodo.
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

