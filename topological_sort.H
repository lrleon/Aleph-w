
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
# ifndef TOPOLOGICAL_SORT_H
# define TOPOLOGICAL_SORT_H

# include <tpl_dynListQueue.H>
# include <tpl_graph.H>

namespace Aleph {


    /** Calcula un ordenamiento topológico para un digrafo en modo sufijo. 

        Topological_Sort recorre recursivamente en profundidad y
        en sufijo los nodos de un grafo y efectúa un ordenamiento topológico
        cuyo resultado lo almacena un la lista dada en parámetro.

        @ingroup Grafos
     */
    template <class GT, 
	      template <typename, class> class Itor = Out_Iterator,
	      class SA = Dft_Show_Arc<GT>> 
class Topological_Sort
{
  SA & sa;
  const GT * gptr;

public:

  Topological_Sort(SA && __sa = SA()) 
  noexcept(std::is_nothrow_move_assignable<SA>::value)
    : sa(__sa), gptr(nullptr) { /* empty */ }

  Topological_Sort(SA & __sa) 
  noexcept(std::is_nothrow_copy_assignable<SA>::value)
    : sa(__sa), gptr(nullptr)  { /* empty */ }

private:
 
      template <template <class> class List>
  void topological_sort(typename GT::Node * curr, 
			List<typename GT::Node*> & list)
  {
    assert(gptr != nullptr);

    if (IS_NODE_VISITED(curr, Depth_First))
      return;

    NODE_BITS(curr).set_bit(Depth_First, 1); // marcarlo como visitado

    const auto & n = gptr->get_num_nodes();

      // visitar recursivamente en sufijo los nodos adyacentes a curr
    for (Itor<GT,SA> it(curr,sa); it.has_curr() and list.size() < n; it.next_ne()) 
      topological_sort(it.get_tgt_node_ne(), list);

    list.insert(curr); // inserción sufija de nodo que devino sumidero
  }

public:

    /** Retorna un ordenamiento topológico por sufijo.

        @param[in] g el digrafo acíclico.
        @return una lista de nodos correspondiente al ordenamiento
        topológico. 
        @throw bad_alloc si no hay suficiente memoria para insertar en
        list.
        @note La rutina no verifica si g es en efecto un digrafo acíclico. 
   */
  template <template <class> class List>
  List<typename GT::Node*> perform (const GT & g)
  {
    g.reset_bit_nodes(Depth_First); // iniciar bit de marca de visita en nodos

    gptr = &g;
    List<typename GT::Node*> list;

        // recorrer todos los nodos
    const auto & n = gptr->get_num_nodes();
    for (auto it = g.get_node_it(); it.has_curr() and list.size() < n; 
	 it.next_ne())
    {
      auto curr = it.get_current_node_ne();
      if (not IS_NODE_VISITED(curr, Depth_First))
        topological_sort(curr, list);
    }
    
    return list;
  }

  /// Sobrecarga de () sólo por razones de compatibilidad con versiones
  /// anteriores
  void operator () (const GT & g, DynDlist<typename GT::Node*> & list) 
  {
    perform<DynDlist>(g).swap(list);
  }
};

    /** Calcula un ordenamiento topológico para el digrafo mediante un
        recorrido en amplitud. 

        Q_Topdological_Sort recorre  en amplitud
        los nodos de un digrafo g y efectúa un ordenamiento topológico
        cuyo resultado lo almacena en una lista dada.

        @ingroup Grafos
     */
   template <class GT, 
	     template <typename, class> class Itor = Out_Iterator,
	     class SA = Dft_Show_Arc<GT>> 
class Q_Topological_Sort
{
  SA & sa;

public:

  Q_Topological_Sort(SA && __sa = SA()) 
  noexcept(std::is_nothrow_move_assignable<SA>::value)
    : sa(__sa) { /* empty */ }

  Q_Topological_Sort(SA & __sa) 
  noexcept(std::is_nothrow_copy_assignable<SA>::value)
    : sa(__sa) { /* empty */ }

  /** Retorna un ordenamiento topológico en amplitud.
      
      @param[in] g el digrafo acíclico.
      @return lista de nodos correspondiente al ordenamiento
      topológico. 
      @throw bad_alloc si no hay suficiente memoria para insertar en
      list o en la cola interna que se requiere para recorrer el
      digrafo en amplitud.
      @throw domain_error si g no es un digrafo.
      @note La rutina no verifica si g es en efecto un digrafo acíclico. 
  */
      template <template <class> class List>
  List<typename GT::Node*> perform (const GT & g)
  {
    g.reset_counter_nodes();

    List<typename GT::Node*> list;

        // recorra todos los arcos y contar grados de entrada
    for (Arc_Iterator<GT,SA> it(g, sa); it.has_curr(); it.next_ne()) 
      NODE_COUNTER(it.get_tgt_node_ne())++; 

      // buscar nodos con grado de entrada 0 y meterlos en cola
    DynListQueue<typename GT::Node*> q; // cola de fuentes
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      {
	auto p = it.get_current_node_ne();
	if (NODE_COUNTER(p) == 0) // ¿es un nodo fuente?
	  q.put(p); // sí ==> colocarlo en la cola
      }

    while (not q.is_empty()) 
      {
	auto p = q.get(); // saque último fuente

	assert(NODE_COUNTER(p) == 0);

	list.append(p); // insértelo en el orden topológico

          // decrementar grado de entrada de cada nodo adyacente a p
	for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
	  {
	    auto tgt = it.get_tgt_node_ne();
	    if (--NODE_COUNTER(tgt) == 0) // ¿tgt deviene fuente?
	      q.put(tgt); // sí ==> colóquelo en la cola
	  }
      }

    return list;
  }

  /** Retorna el ordenamiento topológico en amplitud por rangos.
      
      @param[in] g el digrafo acíclico.
      @param[out] ranks lista de rangos correspondiente al ordenamiento
      topológico. Cada rango especifica un conjunto de nodos que
      pueden mirarse en paralelo.
      @throw bad_alloc si no hay suficiente memoria para insertar en
      list o en la cola interna que se requiere para recorrer el
      digrafo en amplitud.
      @throw domain_error si g no es un digrafo.
      @note La rutina no verifica si g es en efecto un digrafo acíclico. 
  */
  template <template <class> class RankList = DynList, 
	    template <class> class List = DynList>
  RankList<List<typename GT::Node*>> ranks(const GT & g)
  {
    g.reset_counter_nodes();

        // recorra todos los nodos para contabilizar grado de entrada
    for (typename GT::Node_Iterator i(g); i.has_curr(); i.next_ne()) 
      for (Itor<GT, SA> j(i.get_current_node_ne(), sa);
	   j.has_curr(); j.next_ne())
	NODE_COUNTER(j.get_tgt_node())++;

        // revisar nodos con grado de entrada 0 y meterlos en cola
    DynListQueue<typename GT::Node*> q; // cola de fuentes
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
	auto p = it.get_current_node_ne();
	if (NODE_COUNTER(p) == 0) // ¿es un nodo fuente?
	  q.put(p); // sí ==> colocarlo en la cola
      }
    
    RankList<List<typename GT::Node*>> ranks; // valor de retorno
    while (not q.is_empty()) 
      {
	List<typename GT::Node*> rank;
	DynListQueue<typename GT::Node*> aq;

	while (not q.is_empty()) // saca todos los nodos del nivel i
	  {
	    auto p = q.get(); // saque último fuente
	    rank.append(p); // insértelo en el rango topológico

                // decrementar grado entrada de cada nodo adyacente a p
	    for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
	      {
		auto tgt = it.get_tgt_node_ne();
		if (--NODE_COUNTER(tgt) == 0) // ¿tgt deviene fuente?
		  aq.put(tgt); // sí ==> colóquelo en cola auxiliar
	      }
	  }

	ranks.append(std::move(rank));
	q.swap(aq);
	assert(aq.is_empty());
      }

    return ranks; 
  }

  void operator () (const GT & g, DynDlist<DynList<typename GT::Node *>> & list)
  {
    this->ranks<>(g).swap(list);
  }

  void operator () (const GT & g, DynList<DynList<typename GT::Node *>> & list)
  {
    this->ranks<DynList>(g).swap(list);
  }

      /// Sobrecarga de () por razones compatibilidad
  void operator () (const GT & g, DynDlist<typename GT::Node*> & list) 
  {
    this->perform<DynDlist>(g).swap(list);
  }
};

} // end namespace Aleph

# endif // TOPOLOGICAL_SORT_H
