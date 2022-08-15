
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
# ifndef TPL_SPANNING_TREE_H
# define TPL_SPANNING_TREE_H

# include <tpl_graph.H>

namespace Aleph {


    /** Calcula un árbol abarcador en profundidad de un grafo.

          Esta clase toma un grafo g, efectúa un recorrido en profundidad
          a partir de un nodo seleccionado y construye el árbol abarcador
          según el orden de visita dado por el recorrido.  

          La clase toma dos parámetros tipo:
          -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
          -# SA: clase que se encarga de mostrar el arco. Internamente, la
          función emplea el iterador filtro Node_Arc_Iterator (basado
          en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
          el la clase que determina si el arco debe o no mostrarse al
          recorrido. 

          @ingroup Grafos
          @see find_breadth_first_spanning_tree() Tree_Node
          @see graph_to_tree_node() 
        */
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Find_Depth_First_Spanning_Tree
{
  SA   sa;
  GT * gptr = nullptr;
  GT * tptr = nullptr;

  bool build_tree(typename GT::Node * gnode, typename GT::Arc * garc, 
		  typename GT::Node * tnode)
  {
    NODE_BITS(gnode).set_bit(Spanning_Tree, true); // marcar nodo 
    ARC_BITS(garc).set_bit(Spanning_Tree, true);   // marcar arco

    auto tree_tgt_node = tptr->insert_node(gnode->get_info());
    GT::map_nodes(gnode, tree_tgt_node);

    auto tarc = tptr->insert_arc(tnode, tree_tgt_node, garc->get_info()); 
    GT::map_arcs(garc, tarc);

    tnode = tree_tgt_node; 
    if (tptr->get_num_nodes() == gptr->get_num_nodes()) // ¿grafo abarcado?
      return true; // tree ya contiene el árbol abarcador

    assert(tptr->get_num_nodes() > tptr->get_num_arcs()); // debe ser acíclico

    for (Node_Arc_Iterator<GT, SA> i(gnode, sa); 
	 i.has_curr() and tptr->get_num_nodes() < gptr->get_num_nodes();
	 i.next_ne())
      {
        auto arc = i.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Spanning_Tree)) 
          continue;

        auto arc_tgt_node = i.get_tgt_node();
        if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
          continue; // destino ya visitado desde otro arco

        if (build_tree(arc_tgt_node, arc, tnode))
	  return false;
      }

    return false;
  }

  bool build_tree(GT & g, typename GT::Node * gnode, GT & tree)
  {
    gptr = &g;
    tptr = &tree;

    gptr->reset_nodes();
    gptr->reset_arcs(); 

    clear_graph(*tptr); // asegurar que árbol destino esté vacío
  
    NODE_BITS(gnode).set_bit(Spanning_Tree, true); // marcar gnode
  
    auto tnode = tree.insert_node(gnode->get_info()); 
    GT::map_nodes(gnode, tnode);
  
    for (Node_Arc_Iterator<GT, SA> i(gnode, sa); 
	 i.has_curr() and tptr->get_num_nodes() < gptr->get_num_nodes(); 
	 i.next_ne())
      {
	auto arc = i.get_current_arc_ne();
	if (IS_ARC_VISITED(arc, Spanning_Tree)) 
	  continue;

	auto arc_tgt_node = i.get_tgt_node();
	if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
	  continue; // destino ya visitado desde otro arco

	if (build_tree(arc_tgt_node, arc, tnode))
	  return false;
      }

    return true;
  }

public:

  Find_Depth_First_Spanning_Tree(SA __sa = SA()) : sa(__sa) { /* empty */ }

  /** Invoca a la construcción de árbol abarcador en profundidad.

      Luego de la operación el parámetro tree contiene el árbol
      abarcador en cuestión con sus arcos y nodos mapeados al grafo g.

      Durante la ejecución, el algoritmo marca los nodos y
      arcos visitados con el bit el spanning_tree.

      El orden de visita del grafo no necesariamente es el mismo que el
      de la primitiva depth_first_traversal().

      @param[in] g el grafo sobre el cual se desea construir el árbol
      abarcador en profundidad.
      @param[out] tree grafo donde se colocará el árbol abarcador de
      profundidad. 
      @return si el grafo es conexo y existe un árbol abarcador, se
      retorna un puntero al nodo desde donde se inicia la búsqueda en
      profundidad; de lo contrario se retorna nullptr.
      @throw bad_alloc si no hay memoria para construir el árbol abarcador.
  */
  typename GT::Node * operator () (GT & g, GT & tree)
  {
    auto start = g.get_first_node();
    if (not build_tree(g, start, tree)) 
      return nullptr;

    return start;
  }

    /** Invoca a la construcción de árbol abarcador en profundidad a
	partir de un nodo dado.

	Luego de la operación el parámetro tree contiene el árbol
	abarcador en cuestión con sus arcos y nodos mapeados al grafo
	g. 

	Durante la ejecución, el algoritmo marca los nodos y
	arcos visitados con el bit el spanning_tree.
	
	El orden de visita del grafo no necesariamente es el mismo que
	el de la primitiva depth_first_traversal(). 

	@param[in] g el grafo sobre el cual se desea construir el árbol
	abarcador en profundidad.
	@param[in] gnode nodo desde el cual se deseaq iniciar la
	búsqueda en profundidad.
	@param[out] tree grafo donde se colocará el árbol abarcador de
	profundidad. 
	@return puntero al nodo desde donde se inicia la búsqueda en
	profundidad; de lo contrario se retorna nullptr.
	@throw bad_alloc si no hay memoria para construir el árbol
	abarcador. 
    */
  typename GT::Node * operator () (GT & g, typename GT::Node * gnode, GT & tree)
  {
    this->build_tree(g, gnode, tree);
    return (typename GT::Node *) NODE_COOKIE(gnode);
  }
};



/** Calcula un árbol abarcador en amplitud de un grafo a partir 
           de un nodo.

           Esta clase toma un grafo g, efectúa un recorrido en
           amplitud a partir de un nodo dado y construye el árbol
           abarcador según el orden de visita dado por el recorrido.

           La clase toma dos parámetros tipo:
           -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
           -# SA: clase que se encarga de mostrar el arco. Internamente, la
           clase emplea el iterador filtro Node_Arc_Iterator (basado
           en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
           el la clase que determina si el arco debe o no mostrarse al
           recorrido. 


           @throw bad_alloc si no hay memoria para construir el árbol abarcador
           o para la cola interna que se utiliza para el recorrido en amplitud.
           @ingroup Grafos
           @see find_depth_first_spanning_tree() Tree_Node
           @see graph_to_tree_node() 
        */
      template <class GT, class SA = Dft_Show_Arc<GT> > 
class Find_Breadth_First_Spanning_Tree
{
  SA & sa;

  void build_tree(GT & g, typename GT::Node * gp, GT & tree)
  {
    g.reset_bit_nodes(Spanning_Tree);
    g.reset_bit_arcs(Spanning_Tree);

    clear_graph(tree); 

    unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
    tree.insert_node(tp_auto.get());
    GT::map_nodes(gp, tp_auto.release());

    DynListQueue<typename GT::Arc*> q; // insertar en cola arcos gp
    for (Node_Arc_Iterator<GT, SA> i(gp, sa); i.has_curr(); i.next_ne())
      q.put(i.get_current_arc_ne());

    NODE_BITS(gp).set_bit(Spanning_Tree, true); 

    while (not q.is_empty()) 
      {
	auto garc = q.get(); 
	ARC_BITS(garc).set_bit(Spanning_Tree, true); 
	auto gsrc = g.get_src_node(garc);  
	auto gtgt = g.get_tgt_node(garc);

	if (IS_NODE_VISITED(gsrc, Spanning_Tree) and 
	    IS_NODE_VISITED(gtgt, Spanning_Tree))
	  continue; // los dos nodos de garc ya fueron visitados

	if (IS_NODE_VISITED(gtgt, Spanning_Tree)) // ¿gtgt visitado?
	  std::swap(gsrc, gtgt); // sí, intercámbielo con gsrc

	auto tsrc = mapped_node<GT>(gsrc);
	NODE_BITS(gtgt).set_bit(Spanning_Tree, true); // gtgt visitado

            // crear copia de gtgt, insertarlo en tree y mapearlo
	unique_ptr<typename GT::Node> ttgt_auto(new typename GT::Node(gtgt));
	tree.insert_node(ttgt_auto.get());
	auto ttgt = ttgt_auto.release();
	GT::map_nodes(gtgt, ttgt);

            // insertar nuevo arco en tree y mapearlo
	auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
	GT::map_arcs(garc, tarc);
	if (tree.get_num_nodes() == g.get_num_nodes()) // ¿abarca a g?
	  break; 

            // insertar en cola arcos de gtgt
	for (Node_Arc_Iterator<GT, SA> i(gtgt, sa); i.has_curr(); i.next_ne())
	  {
	    auto current_arc = i.get_current_arc_ne();
	    if (IS_ARC_VISITED(current_arc, Spanning_Tree)) 
	      continue;

                // revise nodos de arcos para ver si han sido visitados
	    if (IS_NODE_VISITED(g.get_src_node(current_arc), Spanning_Tree) and 
		IS_NODE_VISITED(g.get_tgt_node(current_arc), Spanning_Tree))
	      continue; // nodos ya visitados ==> no meter arco
	    q.put(current_arc);
	  }
      }
  }

public:

  Find_Breadth_First_Spanning_Tree(SA && __sa = SA()) : sa(__sa) { /* empty */ }

  Find_Breadth_First_Spanning_Tree(SA & __sa) : sa(__sa) { /* empty */ }

    /** Invoca a la construcción de un árbol abarcador el amplitud.

        Luego de la operación el parámetro tree contiene el árbol
        abarcador en cuestión con sus arcos y nodos mapeados al grafo
        g. 

        Durante la ejecución, el algoritmo marca los nodos y
        arcos visitados con el bit el spanning_tree.

        El orden de visita del grafo no necesariamente es el mismo que
        el de la primitiva breadth_first_traversal(). 

        @param[in] g el grafo sobre el cual se desea construir el árbol
        abarcador en amplitud.
        @param[in] gnode puntero al nodo origen de la búsqueda en
        amplitud. 
        @param[out] tree grafo donde se colocará el árbol abarcador de
        amplitud. 
    */
    void operator () (GT & g, typename GT::Node * gnode, GT & tree) 
    {
      find_breadth_first_spanning_tree <GT, SA> (g, gnode, tree);
    }
  };


      /** Construye un árbol abarcador de un grafo representado con arreglos.

          Esta clase toma un grafo g y un árbol abarcador
          representado mediante los arreglos de nodos pred y de arcos
          arcs y construye en tree el árbol abarcador.

         @ingroup Grafos
       */
      template <class GT>
class Build_Spanning_Tree
{
public:

    /** Invoca la construcción del árbol abarcador mediante arreglos.

          @param[in] g el grafo sobre el cual se desea construir el árbol
          abarcador alterno.
          @param[out] tree el árbol abarcador alterno obtenido de los
          arreglos pred y arcs.
          @param[in] pred el arreglo de punteros a nodos.
          @param[in] arcs el arreglo de punteros a arcos.
          @param[in] with_map indica si el árbol abarcador tree debe
          mapearse con el grafo g.
          @throw bad_alloc si no hay suficiente memoria para construir el
          árbol abarcado tree.
     */
  GT operator () (const DynArray<typename GT::Arc> *  arcs) const
  {
    return build_spanning_tree(arcs);
  }
};


} // end namespace Aleph

# endif //  TPL_SPANNING_TREE_H
