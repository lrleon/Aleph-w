
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
# ifndef TPL_INDEXARC_H
# define TPL_INDEXARC_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace Aleph
{


  /** Construye un índice de arcos para su rápida búsqueda y
      recuperación dado sus nodos. 

      IndexArc indiza los arcos de un grafo según el par de nodos que el
      arco relaciona. La idea es implementar la detección de existencia
      de arco y su recuperación en tiempo logarítmico.

      La clase recibe los siguientes parámetros tipo:
      -# GT: el tipo de grafo basado en List_Graph.
      -# Tree: el tipo de árbol binario de búsqueda usado internamente
      para indizar las claves. Por omisión se usan treaps.

      @warning Para que la compilación correcta es imperativo que el
      exista el constructor GT::Arc(void * src, void * tgt,
      GT::Arc_Type). Del mismo modo, también debe existir el constructor
      por omisión GT::Arc_Type().

      @see IndexNode Index_Graph

      @ingroup Grafos
      @author Leandro Rabindranath León (lrleon en ula punto ve)
      @author Alejandro Mujica (aledrums en gmail punto com)
  */
  template <
    class GT,
    template <class /* Key */, class /* Compare */> class Tree = Rand_Tree,
    class SA = Dft_Show_Arc<GT>
    >
class IndexArc
{
private:
			
  typedef typename GT::Arc GT_Arc;
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc_Type GT_Arc_Type;
  typedef typename GT::Node_Type GT_Node_Type;

  typedef std::pair<void*, void*> Node_Pair;

  struct Cmp_Arc
  {
    bool operator () (GT_Arc * a1, GT_Arc * a2) const
    {
      if (a1->src_node < a2->src_node)
	return true;

      return not (a2->src_node < a1->src_node) and a1->tgt_node < a2->tgt_node;
    }
  };

  GT & g;
  DynSetTree <GT_Arc *, Tree, Cmp_Arc> index;
  SA & sa;

public:

      /// Inserta en el índice el arco a.
  GT_Arc * insert(GT_Arc * e)
  {
    return *index.put(e);
  }

      /** Busca un arco que conecte a dos nodos.

	  @param[in] src nodo origen.
	  @param[in] tgt nodo destino.
	  @return puntero al arco si éste existe o nullptr de lo contrario.
      */
  GT_Arc * search(void * src, void * tgt)
  {
    GT_Arc arc;
    arc.src_node = src; 
    arc.tgt_node = tgt;
    
    GT_Arc ** ret_val = index.search(&arc);

    if (ret_val != nullptr)
      return *ret_val;
 
    if (g.is_digraph())
      return nullptr;

    std::swap(arc.src_node, arc.tgt_node);
    ret_val = index.search(&arc);

    if (ret_val == nullptr)
      return nullptr;

    assert(((src == (*ret_val)->src_node) and (tgt == (*ret_val)->tgt_node)) or 
      ((tgt == (*ret_val)->src_node) and (src == (*ret_val)->tgt_node)));

    return *ret_val; 
  }

    /** Busca un arco que conecte a dos nodos.

	  @param[in] src nodo origen.
	  @param[in] tgt nodo destino.
	  @return puntero al arco si éste existe o nullptr de lo contrario.
      */
  GT_Arc * search_directed(void * src, void * tgt)
  {
    GT_Arc arc;
    arc.src_node = src; 
    arc.tgt_node = tgt;
    
    GT_Arc ** ret_val = index.search(&arc);
    
    return ret_val != nullptr ? *ret_val : nullptr;
  }

      /** Busca un arco según puntero.

	  @param[in] a arco que se desea buscar.
	  @return puntero al arco si éste existe o nullptr de lo contrario.
      */
  GT_Arc * search(GT_Arc * a)
  {
    return search(a->src_node, a->tgt_node);
  }

      /** Crea un nuevo arco entre dos nodos, lo inserta en el grafo y
	  luego en el índice.

	  @param[in] src nodo origen.
	  @param[in] tgt nodo destino.
	  @param[in] info información a copiarse como contenido del
	  nodo. Por omisión se copia el contenido que arroje el
	  constructor GT::Arc_Type().
	  @return puntero al arco creado.
	  @throw bad_alloc si no hay suficiente memoria.
       */
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt, 
			   const GT_Arc_Type & info)
    throw (std::exception, std::domain_error)
  {
    GT_Arc * a = search(src, tgt);

    if (a != nullptr)
      throw std::domain_error("There is already in arc between these nodes");

    a = g.insert_arc(src, tgt, info);
    insert(a);
    
    return a;
  }

      /// \overload  insert_in_graph
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt, 
			   GT_Arc_Type && info = GT_Arc_Type())
    throw (std::exception, std::domain_error)
  {
    GT_Arc * a = search(src, tgt);

    if (a != nullptr)
      throw std::domain_error("There is already in arc between these nodes");

    a = g.insert_arc(src, tgt, std::forward<GT_Arc_Type>(info));
    insert(a);
    
    return a;
  }

      /// Elimina del índice el arco e.
  void remove(GT_Arc * e)
  {
    index.remove(e);
  }

      /// Elimina del índice y del grafo el arco a.
  void remove_from_graph(GT_Arc * a)
  {
    remove(a);
    g.remove_arc(a);
  }

      /// Elimina todos los arcos del índice.
  void clear_index()
  {
    index.empty();
  }

        /// Inserta todos los arcos del grafo en el índice.
  void build_index()
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
	GT_Arc * a = it.get_curr_ne();
	if (search(a) != a)
	  insert(a);
      }
  }

private:

  void init()
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      insert(it.get_curr_ne());
  }

public:

  /** Crea un índice a partir de los arcos insertado en el grafo.

      @param[in] __g el grafo
      @param[in] with_init si el valor es true, entonces los arcos de
      __g son insertados en el índice; de lo contrario, el índice
      permanece vacío
      @throw bad_alloc si no hay suficiente memoria.
  */
  IndexArc(GT & __g, bool with_init = true, SA && __sa = SA()) 
    : g(__g), sa(__sa)
  {
    if (with_init)
      init();
  }

  /** Crea un índice a partir de los arcos insertado en el grafo.

      @param[in] __g el grafo
      @param[in] with_init si el valor es true, entonces los arcos de
      __g son insertados en el índice; de lo contrario, el índice
      permanece vacío
      @throw bad_alloc si no hay suficiente memoria.
  */
  IndexArc(GT & __g, bool with_init, SA & __sa) : g(__g), sa(__sa)
  {
    if (with_init)
      init();
  }

      /// Retorna la cantidad de arcos que contiene el índice.
  size_t size() const { return index.size(); }
};

}

# endif

