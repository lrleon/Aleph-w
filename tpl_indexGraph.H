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

/** @file tpl_indexGraph.H
 *  @brief Graph indexing utilities.
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_INDEXGRAPH
# define TPL_INDEXGRAPH

# include <tpl_indexNode.H>
# include <tpl_indexArc.H>

# include <ah-errors.H>

namespace Aleph
{

  /** Construye índices de nodos y arcos para su rápida búsqueda y recuperación.

      Index_Graph indiza los nodos y arcos a efectos de su
      recuperación rápida.

      A efectos de facilitar el uso y hacer su uso más seguro,
      Index_Graph ofrece las operaciones topológicas clásicas de un
      grafo: insert_node(), insert_arc(), etc.

      La clase recibe los siguientes parámetros tipo:
      -# GT: el tipo de grafo basado en List_Graph
      -# Compare: clase de comparación para la clave de indización de
      los nodos. El contrato de esta clase es intrumentar el operador () así:
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

      Por omisión esta clase está programada para comparar
      el valor retornado por get_info() sobre cada nodo. Para ello, el
      operador < del tipo GT::Node_Type debe estar implementado
      -# Tree: el tipo de árbol binario de búsqueda usado internamente
      para indizar las claves. Por omisión se usan treaps
      .

      @see IndexArc IndexNode

      @ingroup Grafos
      @author Leandro Rabindranath León (lrleon en ula punto ve)
      @author Alejandro Mujica (aledrums en gmail punto com)
  */
template <class GT, class Compare = Dft_Node_Cmp<GT>,
          template <class /* Key */, class /* Compare */> class Tree = Treap>
class Index_Graph
{
private:

  typedef typename GT::Arc GT_Arc;
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc_Type GT_Arc_Type;
  typedef typename GT::Node_Type GT_Node_Type;

  IndexNode<GT, Compare, Tree> idx_node;
  IndexArc<GT, Tree>           idx_arc;

public:

      /// Crea un índice del grafo: los nodos y arcos son indizados.
  Index_Graph(GT & g) : idx_node(g), idx_arc(g)
  {
    // empty
  }

  /** Crea un nuevo nodo y lo inserta en grafo y en el índice.

      @param[in] info información a copiarse en el nodo.
      @return puntero al nuevo nodo.
      @throw bad_alloc si no hay suficiente memoria.
   */
  GT_Node * insert_node(const GT_Node_Type & info)
  {
    return idx_node.insert_in_graph(info);
  }

  /** Crea un nuevo arco entre dos nodos y lo inserta en el grafo y en
      el índice.

      @param[in] src nodo origen.
      @param[in] tgt nodo destino.
      @param[in] info información a copiarse en el arco. Por omisión es
      la que dé el constructor GT_Arc_Type().
      @return puntero al nuevo arco.
      @throw bad_alloc si no hay suficiente memoria.
   */
  GT_Arc * insert_arc(GT_Node * src, GT_Node * tgt,
                      const GT_Arc_Type & info = GT_Arc_Type())
  {
    ah_domain_error_if(idx_node.search(src) == nullptr)
        << "src node not in index";

    ah_domain_error_if(idx_node.search(tgt) == nullptr)
        << "tgt node not in index";

    return idx_arc.insert_in_graph(src, tgt, info);
  }

  /** Busca en el índice un nodo.

      @param[in] p puntero al nodo.
      @return puntero al nodo en caso de encontrarse en el índice; nullptr
      de lo contrario.
  */
  GT_Node * search_node(GT_Node * p)
  {
    return idx_node.search(p);
  }

  /** Busca en el índice un nodo.

      @param[in] info información según la cual se buscará el nodo. 
      @return puntero al nodo en caso de encontrarse en el índice; nullptr
      de lo contrario.
      @warning Tome en consideración que la búsqueda se realiza según la
      implementación de la clase de comparación Compare pasada en tiempo
      de instanciación.
  */
  GT_Node * search_node(const GT_Node_Type & info)
  {
    return idx_node.search(info);
  }

  /** Busca en el índice un arco dados dos nodos.

      @param[in] src puntero al nodo origen.
      @param[in] tgt puntero al nodo destino.
      @return puntero al arco en caso de encontrarse en el índice; nullptr
      de lo contrario.
  */
  GT_Arc * search_arc(GT_Node * src, GT_Node * tgt)
  {
    return idx_arc.search(src, tgt);
  }

  /** Elimina el nodo p del grafo y del índice.

      @param[in] p puntero al nodo a eliminar
  */
  void remove_node(GT_Node * p)
  {
    // eliminar del índice los arcos emanantes del nodo (ellos serán
    // eliminados por la eliminación de nodo del grafo)
    for (typename GT::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
      idx_arc.remove(it.get_curr());

    return idx_node.remove_from_graph(p);
  }

  /// Elimina del grafo y del índice el arco a
  void remove_arc(GT_Arc * a)
  {
    return idx_arc.remove_from_graph(a);
  }

  /// Retorna el número de arcos que contiene el índice.
  size_t get_num_arcs() const { return idx_arc.size(); }

  /// Retorna el número de nodos que contiene el índice.
  size_t get_num_nodes() const { return idx_node.size(); }
};

  template <class GT> inline
  bool are_equal(const GT & g1, const GT & g2)
  {
    if (g1.vsize() != g2.vsize() or g1.esize() != g2.esize())
      return false;

    {
      IndexNode<GT> t2(const_cast<GT&>(g2));
      if (not g1.all_nodes([&t2] (auto p)
                           {
                             auto q = t2.search(p);
                             if (q == nullptr)
                               return false;
                             GT::map_nodes(p, q);
                             return true;
                           }))
        return false;
    }

    IndexArc<GT> t2(const_cast<GT&>(g2));
    return g1.all_arcs([idx = &t2, &g1] (auto a)
                       {
                         auto s1 = g1.get_src_node(a);
                         auto t1 = g1.get_tgt_node(a);
                         auto s2 = mapped_node<GT>(s1);
                         auto t2 = mapped_node<GT>(t1);
                         auto a2 = idx->search(s2, t2);
                         if (a2 == nullptr)
                           return false;
                         return a2->get_info() == a->get_info();
                       });
  }

}

# endif // TPL_INDEXGRAPH


