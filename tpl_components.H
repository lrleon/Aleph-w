
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef TPL_COMPONENTS_H
# define TPL_COMPONENTS_H

# include <tpl_agraph.H>

namespace Aleph {


/** Construye un subgrafo mapeado del grafo g a partir de uno de sus nodos.

        La clase  Build_Subgraph recorre en profundidad el grafo g a
        partir de un nodo origen g_src y construye en una copia mapeada de
        todo el grafo (o subgrafo si g es inconexo) visto en el
        recorrido.

        @see inconnected_components() copy_graph()
        @ingroup Grafos
     */
      template <class GT, class SA = Dft_Show_Arc<GT> > 
class Build_Subgraph
{
  SA sa;
  const GT * gptr = nullptr;
  size_t count = 0;

public:

  Build_Subgraph(SA __sa = SA()) 
  noexcept(std::is_nothrow_move_assignable<SA>::value)
    : sa(__sa) { /* empty */ }

private:

  void build_subgraph(GT & sg, typename GT::Node * g_src)
  {
    if (IS_NODE_VISITED(g_src, Build_Subtree)) 
      return;

    NODE_BITS(g_src).set_bit(Build_Subtree, true); // g_src visitado
    ++count; 

    auto sg_src = mapped_node <GT> (g_src);
    if (sg_src == nullptr) // ¿está mapeado g_src?
      {     // No, cree imagen de g_src en el subgrafo sg y mapee
	sg_src = sg.insert_node(g_src->get_info());
	GT::map_nodes(g_src, sg_src);
      }

    for (Node_Arc_Iterator<GT, SA> i(g_src, sa); // explore desde g_src 
	 i.has_current(); i.next())
      {
        auto arc = i.get_current_arc();
        if (IS_ARC_VISITED(arc, Build_Subtree)) 
          continue; // avance próximo arco

        ARC_BITS(arc).set_bit(Build_Subtree, true); // arc visitado

        auto g_tgt  = i.get_tgt_node(); // destino de arc
        auto sg_tgt = mapped_node <GT> (g_tgt);
        if (sg_tgt == nullptr) // ¿está mapeado en sg?
          {    // no, hay que mapearlo e insertarlo en el subgrafo sg
            sg_tgt = sg.insert_node(g_tgt->get_info()); 
            GT::map_nodes(g_tgt, sg_tgt); 
          }

    	    // tenemos nodos en subgrafo, insertamos arco y mapeamos
        auto sg_arc = sg.insert_arc(sg_src, sg_tgt, arc->get_info());

        GT::map_arcs(arc, sg_arc); 

        build_subgraph(sg, g_tgt);
      }
  }

      template <template <class> class List>
  void build_subgraph(List<typename GT::Node*> & l, typename GT::Node * p)
  {
    if (IS_NODE_VISITED(p, Build_Subtree)) 
      return;

    NODE_BITS(p).set_bit(Build_Subtree, true); // g_src visitado
    ++count; 
    l.append(p);

    for (Node_Arc_Iterator<GT, SA> it(p, sa); // explore desde g_src 
	 count < gptr->get_num_nodes() and it.has_curr(); it.next())
      {
        auto arc = it.get_current_arc();
        if (IS_ARC_VISITED(arc, Build_Subtree)) 
          continue; // avance próximo arco

        ARC_BITS(arc).set_bit(Build_Subtree, true); // arc visitado
        build_subgraph(l, it.get_tgt_node());
      }
  }
    
  public:

    /** Invoca a la construcción de un subgrafo mapeado.

        El método se sirve del bit build_subtree para marcar los nodos y
        arcos ya visitados.

        build_subgraph() es utilizado por el método
        inconnected_components() para mapear los diversos bloques.

        @param[in] g el grafo a mapear
        @param[out] sg un grafo vacío donde colocar la copia mapeada a
        partir de g_src.
        @param[in] g_src el nodo origen desde donde se origina el
        recorrido y mapeo.
        @throw bad_alloc si no hay memoria para construir sg.
        @throw domain_error si sg no es un grafo vacío.
     */
  void operator () (const GT & g, GT & sg, typename GT::Node * g_src) 
  {
    gptr = &g;
    count = 0;
    build_subgraph (sg, g_src);
  }

  GT operator () (const GT & g, typename GT::Node * src) 
  {
    GT sg;
    gptr = &g;
    count = 0;
    build_subgraph(sg, src);
    return sg;
  }

  /** Construye una lista de nodos alcazables desde src.

      El método se sirve del bit build_subtree para marcar los nodos y
      arcos ya visitados.

      build_subgraph() es utilizado por el método
      inconnected_components() para mapear los diversos bloques.

      @param[in] g el grafo a mapear
      @param[out] l lista vacía donde colocar los nodos a partir de src.
      @param[in] src el nodo origen desde donde se origina el
      recorrido y mapeo.
      @throw bad_alloc si no hay memoria para construir sg.
      @throw domain_error si sg no es un grafo vacío.
  */
  void operator () (const GT & g, DynList<typename GT::Node*> & list, 
		    typename GT::Node * src) 
  {
    gptr = &g;
    count = 0;
    build_subgraph<DynList>(list, src);
  }
}; 


      /** Calcula los componentes inconexos de un grafo.
          
          La clase Inconnected_Components toma un grafo g,
          "supuestamente inconexo" calcula sus subgrafos o componentes
          inconexos y los almacena en una lista dinámica de subgrafos
          mapeados, tanto en los nodos como en los arcos. El énfasis en el
          supuesto carácter conexo del grafo se debe a que si el grafo no
          fuese conexo, entonces la lista resultante contendría un solo
          elemento correspondiente al grafo copia mapeado de g. Si este
          fuese el caso, entonces es preferible hacerlo con la función 
          copy_graph().

          La clase toma dos parámetros tipo:
          -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
          -# SA: clase que se encarga de mostrar el arco. Internamente, la
          función emplea el iterador filtro Node_Arc_Iterator (basado
          en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
          el la clase que determina si el arco debe o no mostrarse al
          recorrido. 


          @see copy_graph()
          @ingroup Grafos
       */
      template <class GT, class SA = Dft_Show_Arc<GT> > 
class Inconnected_Components
{
  SA sa;

public:

  Inconnected_Components(SA __sa = SA()) 
  noexcept(std::is_nothrow_move_assignable<SA>::value)
    : sa(__sa) { /* empty */ }

      template <template <class> class List>
  void compute_blocks(const GT & g, List <GT> & list)
  {
    g.reset_nodes(); 
    g.reset_arcs();  
    size_t count = 0; // contador de nodos visitados
    for (typename GT::Node_Iterator it(g); // recorrer nodos de g
         count < g.get_num_nodes() and it.has_curr(); it.next())
      {
        auto curr = it.get_current_node();
        if (IS_NODE_VISITED(curr, Build_Subtree)) 
          continue;

        GT & subgraph = list.append(GT()); // grafo insertado en list

        Build_Subgraph <GT, SA> build(sa);
	build(g, subgraph, curr); 
	
	count += subgraph.get_num_nodes();
      }
  }

      template <template <class> class List>
  void compute_lists(const GT & g, List<List<typename GT::Node*> > & list)
  {
    g.reset_nodes(); 
    g.reset_arcs();  
    size_t count = 0; // contador de nodos visitados
    for (typename GT::Node_Iterator i(g); // recorrer nodos de g
         count < g.get_num_nodes() and i.has_curr(); i.next())
      {
        auto curr = i.get_current_node();
        if (IS_NODE_VISITED(curr, Build_Subtree)) 
          continue;

	// crear subgrafo componente inconexo conectado por curr_node 
        auto & l = list.append(List<typename GT::Node*>()); 

        Build_Subgraph <GT, SA> build(sa);
	build(g, l, curr); 
	
	count += l.size();
      }
  }
    
    /** Invoca a la determinación de los componentes inconexos.

        La función se vale del bit build_subtree para marcar nodos y
        arcos visitados.

        El algoritmo internamente descubre los componentes mediante
        búsquedas en profundidad.
          
        @param[in] g el grafo sobre el cual se desea calcular sus bloques.
        @param[out] list lista de subgrafos mapeados a g con los
        subgrafos o componentes inconexos de g.
        @throw bad_alloc si no hay memoria para construir algún bloque o
        insertar en la lista.
     */
  void operator () (const GT & g, DynList <GT> & list)
  {
    compute_blocks<DynList>(g, list);
  }

  /** Calcula los componentes inconexos por listas de listas de nodos.

      La función se vale del bit build_subtree para marcar nodos y
      arcos visitados.

      El algoritmo internamente descubre los componentes mediante
      búsquedas en profundidad.
          
      @param[in] g el grafo sobre el cual se desea calcular sus bloques.
      @param[out] list lista de listas de nodos; cada lista contiene los
      nodos que son parte de un componente conexo.
      @throw bad_alloc si no hay memoria para construir algún bloque o
      insertar en la lista.
  */
  void operator () (const GT & g, DynList<DynList<typename GT::Node*>> & list)
  {
     compute_lists<DynList>(g, list);
  }
};


} // end namespace Aleph

# endif //  TPL_COMPONENTS_H
