
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
# ifndef TPL_INDEXNODE_H
# define TPL_INDEXNODE_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace Aleph
{

  template <class GT>
struct Dft_Node_Cmp
{
      bool
  operator () (typename GT::Node * p1, typename GT::Node * p2) const
  {
    return p1->get_info() < p2->get_info();
  }
};

  /** Construye un índice de nodos para su rápida búsqueda y recuperación.

      IndexNode indiza los nodos de un grafo según un clave dada
      definida por el usuario.

      La clase recibe los siguientes parámetros tipo:
      -# GT: el tipo de grafo basado en List_Graph.
      -# Compare: clase de comparación para la clave de indización. El
      contrato de esta clase es intrumentar el operador () así:  
      \code
          template <class GT>  
	struct Dft_Node_Cmp  
	{  
            bool  
          operator () (typename GT::Node * p1, typename GT::Node * p2) const 
          {  
	    // acceso a los nodos y comparación según el campo deseado 
          }  
      }; 
      \endcode
      IndexNode recibe esta clase por omisión programada para comparar
      el valor retornado por get_info() sobre cada nodo. Para ello, el
      operador < del tipo GT::Node_Type debe estar implementado.
      -# SN: clase filtro para el iterador de nodos en caso de que se construya
      -# Tree: el tipo de árbol binario de búsqueda usado internamente
      para indizar las claves. Por omisión se usan treaps.

      @see IndexArc Index_Graph

      @ingroup Grafos
      @author Leandro Rabindranath León (lrleon en ula punto ve)
      @author Alejandro Mujica (aledrums en gmail punto com)
  */
  template
  <class GT, 
   class Compare = Dft_Node_Cmp<GT>,
   template <class /* Key */, class /* Compare */> class Tree = Treap,
   class SN = Dft_Show_Node<GT> >  
class IndexNode
{
private:

  typedef typename GT::Node GT_Node;
  typedef typename GT::Node_Type GT_Node_Type;
	
  DynSetTree<GT_Node*, Tree, Compare> index;

  GT & g;
  SN & sn;
		
public:

      /** Inserta el nodo p en índice.

	  @param[in] p puntero al nodo que se desea guardar en el índice
	  @throw bad_alloc si no hay suficiente memoria
      */
  GT_Node * insert(GT_Node * p)
  {
    index.put(p);
    return p;
  }

      /** Crea un nuevo nodo con contenido genérico info, lo inserta en
	  el grafo y luego en el índice

	  @param[in] info Contenido asociado al nuevo nodo.
	  @return puntero al nodo
	  @throw bad_alloc si no hay suficiente memoria
      */
  GT_Node * insert_in_graph(const GT_Node_Type & info)
  {
    GT_Node * ret_val = g.insert_node(info);

    try
      {
	insert(ret_val);

	return ret_val;
      }
    catch (...)
      {
	g.remove_node(ret_val);
	throw;
      }
  }

  /// \overload insert_in_graph
  GT_Node * insert_in_graph(GT_Node_Type && info = GT_Node_Type())
  {
    GT_Node * ret_val = g.insert_node(std::forward<GT_Node_Type>(info));

    try
      {
	insert(ret_val);

	return ret_val;
      }
    catch (...)
      {
	g.remove_node(ret_val);
	throw;
      }
  }

      /** Busca un nodo según el contenido de p.

	  Tome en cuenta que el criterio de búsqueda está determinado
	  por la clase parámetro de comparación Compare.

	  @param[in] p el nodo cuya información se desea buscar.
	  @return puntero al nodo si es encontrado o nulo de lo
	  contrario.
      */
  GT_Node * search(GT_Node * p)
  {
    GT_Node ** ret_val = index.search(p);

    if (ret_val == nullptr)
      return nullptr;

    return *ret_val;
  }

      /** Busca un nodo según la información info.

	  Tome en cuenta que el criterio de búsqueda está determinado
	  por la clase parámetro de comparación Compare.

	  @param[in] info información según la cual se desea realizar la
	  búsqueda (la cual depende de la clase parámetro de comparación
	  Compare. 
	  @return puntero al nodo si es encontrado o nulo de lo
	  contrario.
      */
  GT_Node * search(const GT_Node_Type & info)
  {
    GT_Node dummy_node(info);
    GT_Node * dummy_node_ptr = &dummy_node;

    return search(dummy_node_ptr);
  }

      /// Elimina del índice el nodo con dirección p.
  void remove(GT_Node * p)
  {
    index.remove(p);
  }

      /** Elimina el nodo p del grafo y del ìndice.

	  @param[in] p puntero al nodo a eliminar
	  @throw domain_error si el nodo està insertado en el índice (lo
	  que posiblemente signifique que tampoco en el grafo).
   */
  void remove_from_graph(GT_Node * p) throw(std::exception, std::domain_error)
  {
    index.find(p); // dispara excepción si p no está en el índice
    index.remove(p);
    g.remove_node(p);
  }

      /// Borra el índice; todos los nodos son eliminados.
  void clear_index()
  {
    index.empty();
  }

      /// Inserta todos los nodos del grafo en el índice.
  void build_index()
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      {
	GT_Node * p = it.get_curr();

	if (search(p) != p)
	  insert(p);
      }
  }

      /// Elimina todos los nodos del grafo y del índice.
  void clear_graph()
  {
    clear_index();
    g.clear_graph();
  }

private: 

  void init()
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      insert(it.get_curr());
  }

public:

  /** Crea un índice a partir de los nodos insertado en el grafo.

      @param[in] __g el grafo
      \param[in] build_index indica si se debe construir el índice
      durante la construcción 
      @throw bad_alloc si no hay suficiente memoria.
  */
  IndexNode(GT & __g, SN && __sn = SN()) : g(__g), sn(__sn)
  {
    init();
  }

  IndexNode(GT & __g, SN & __sn) : g(__g), sn(__sn) 
  {
    init();
  }

      /// Retorna la cantidad de arcos que contiene el índice.
  size_t size() const { return index.size(); }
};

}

# endif

