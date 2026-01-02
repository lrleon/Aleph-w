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

/** @file tpl_find_path.H
 *  @brief Path finding algorithms in graphs.
 *
 *  Finds paths between nodes using DFS, BFS, or specialized
 *  algorithms. For weighted shortest paths, see Dijkstra.H.
 *
 *  ## Features
 *  - DFS path finding
 *  - BFS path finding (shortest in unweighted)
 *  - All paths enumeration
 *
 *  ## Complexity: O(V + E)
 *
 *  @see Dijkstra.H Weighted shortest paths
 *  @see Bellman_Ford.H Handles negative weights
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_FIND_PATH_H
# define TPL_FIND_PATH_H

# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{
  template <class GT>
  struct Dft_Goal_Node
  {
    bool operator ()(typename GT::Node *) const noexcept { return false; }
  };

  /** Busca en profundidad un camino entre un par de nodos.

      Find_Path_Depth_First busca en profundidad un camino entre
      start_node y end_node, a la vez que va construyendo un camino
      equivalente a la profundidad recursiva de la búsqueda. Si el se
      encuentra un camino, entonces el método retorna true y el parámetro
      path alberga el camino en cuestión; de lo contrario, la función
      retorna false y valor del camino es indeterminado.

      La clase toma dos parámetros tipo:
      -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph
      -# SA: clase que se encarga de mostrar el arco. Internamente, la
      función emplea el iterador filtro Node_Arc_Iterator (basado
      en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
      el la clase que determina si el arco debe o no mostrarse al
      recorrido
      .

      @see find_path_breadth_first()
      @see dijkstra_min_spanning_tree() dijkstra_min_path()
      @see bellman_ford_min_spanning_tree() q_bellman_ford_min_spanning_tree()
      @ingroup Grafos
  */
  template <class GT,
            template <class, class> class Itor = Node_Arc_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Find_Path_Depth_First
  {
    SA & sa;
    GT *g_ptr = nullptr;

    template <class Op>
    bool find_path(typename GT::Node *curr, typename GT::Arc *arc,
                   Path<GT> & path, Op & op)
    {
      if (op(curr)) // goal node reached?
        {
          path.append(arc);
          return true;
        }

      if (IS_NODE_VISITED(curr, Find_Path)) // ¿ha sido visitado?
        return false; // sí ==> desde él no hay camino

      path.append(arc); // añadir curr_arc al camino
      NODE_BITS(curr).set_bit(Find_Path, true);

      // buscar recursivamente a través de arcos de curr_node
      for (Itor<GT, SA> i(curr, sa); i.has_curr(); i.next())
        {
          auto next_arc = i.get_curr();
          if (IS_ARC_VISITED(next_arc, Find_Path))
            continue;

          ARC_BITS(next_arc).set_bit(Find_Path, true);
          auto next_node = g_ptr->get_connected_node(next_arc, curr);
          if (find_path(next_node, next_arc, path, op))
            return true; // se encontró camino
        }

      path.remove_last_node();

      return false;
    }

    template <class Op>
    bool find(const GT & g, typename GT::Node *start, Path<GT> & path, Op & op)
    {
      g_ptr = const_cast<GT *>(&g);
      path.set_graph(g, start);

      if (op(start))
        return true;

      g.reset_bit_nodes(Find_Path);
      g.reset_bit_arcs(Find_Path);

      NODE_BITS(start).set_bit(Find_Path, true);

      // explorar recursivamente cada arco de start
      for (Itor<GT, SA> i(start, sa); i.has_curr(); i.next())
        {
          auto arc = i.get_curr();
          ARC_BITS(arc).set_bit(Find_Path, true);

          auto next_node = g.get_connected_node(arc, start);
          if (IS_NODE_VISITED(next_node, Find_Path))
            continue;

          if (find_path(next_node, arc, path, op))
            return true;
        }

      path.remove_last_node();

      return false;
    }

    template <class Op>
    bool find(const GT & g, typename GT::Node *start, Path<GT> & path, Op && op)
    {
      return find(g, start, path, op);
    }

  public:
    Find_Path_Depth_First(SA && __sa = SA())
      : sa(__sa)
    { /* empty */
    }

    Find_Path_Depth_First(SA & __sa)
      : sa(__sa)
    { /* empty */
    }

    /** Invoca a la búsqueda de camino en profundidad.

        @param[in] g el grafo sobre el cual se desea buscar el camino.
        @param[in] start puntero al nodo inicio del camino.
        @param[in] end puntero al nodo destino del camino.
        @param[out] path el camino visto durante la búsqueda en
        profundidad; sólo tiene sentido si el valor de retorno es true.
        @throw bad_alloc si no hay memoria para continuar construyendo
        el camino
    */
    bool operator ()(const GT & g,
                     typename GT::Node *start, typename GT::Node *end,
                     Path<GT> & path)
    {
      return find(g, start, path, [end](auto p) { return p == end; });
    }

    /** Invoca a la búsqueda de camino en profundidad.

        @param[in] g el grafo sobre el cual se desea buscar el camino.
        @param[in] start puntero al nodo inicio del camino.
        @param[in] end puntero al nodo destino del camino.
        @return path reaching `end` if this one was found;  `false` otherwise
        @throw bad_alloc si no hay memoria para continuar construyendo
        el camino
    */
    Path<GT> operator ()(const GT & g,
                         typename GT::Node *start,
                         typename GT::Node *end)
    {
      Path<GT> ret(g);
      find(g, start, ret, [end](auto p) { return p == end; });
      return ret;
    }

    /** Invoca a la búsqueda de camino en profundidad.

        @param[in] g el grafo sobre el cual se desea buscar el camino.
        @param[in] start puntero al nodo inicio del camino.
        @param[in] op functor que implementa el criterio de búsqueda. El
        functor ecibe como parámetro el nodo y debe retornar `true` si el
        nodo satisface el criterio `false` de lo contrario.
        @rreturn path el camino visto durante la búsqueda en
        profundidad; si no existe tal camino entonces el camino es va
        @throw bad_alloc si no hay memoria para continuar construyendo
        el camino
    */
    template <class Op>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op & op)
    {
      Path<GT> ret(g);
      find<Op>(g, start, ret, op);
      return ret;
    }

    template <class Op = Dft_Goal_Node<GT>>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op && op)
    {
      Path<GT> ret(g);
      find<Op>(g, start, ret, op);
      return ret;
    }
  };


  /** Busca en amplitud un camino entre un par de nodos.

Find_Path_Breadth_First busca en amplitud un camino entre
start_node y end_node, a la vez que va construyendo un camino
hacia el nodo destino. Si se encuentra un camino, entonces el
método retorna true y el parámetro path alberga el camino en
cuestión; de lo contrario, la función retorna false y valor del
camino es indeterminado.

La clase toma dos parámetros tipo:
-# GT: el tipo de grafo, el cual debe ser derivado de List_Graph
-# SA: clase que se encarga de mostrar el arco. Internamente, la
función emplea el iterador filtro Node_Arc_Iterator (basado en
Filter_Iterator) para recorrer los arcos de cada nodo. SA es el
la clase que determina si el arco debe o no mostrarse al
recorrido
.

@see find_path_depth_first()
@see dijkstra_min_spanning_tree() dijkstra_min_path()
@see bellman_ford_min_spanning_tree() q_bellman_ford_min_spanning_tree()
@ingroup Grafos
  */
  template <class GT,
            template <typename, class> class Itor = Node_Arc_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Find_Path_Breadth_First
  {
    SA & sa;

    template <class Op>
    bool find_path(const GT & g, typename GT::Node *start,
                   Path<GT> & path, Op & op)
    {
      if (not path.inside_graph(g))
        ah_invalid_argument_if(true) << "Path does not belong to graph";

      path.empty(); // limpiamos cualquier cosa que esté en path
      g.reset_nodes();
      g.reset_arcs();

      DynListQueue<typename GT::Arc *> q;

      for (Itor<GT, SA> i(start, sa); i.has_curr(); i.next())
        q.put(i.get_current_arc());

      NODE_BITS(start).set_bit(Find_Path, true); // márquelo visitado

      typename GT::Node *end = nullptr;

      while (not q.is_empty()) // mientras queden arcos por visitar
        {
          auto arc = q.get();
          auto src = g.get_src_node(arc);
          auto tgt = g.get_tgt_node(arc);

          if (IS_NODE_VISITED(src, Find_Path) and IS_NODE_VISITED(tgt, Find_Path))
            continue;

          if (IS_NODE_VISITED(tgt, Find_Path))
            std::swap(src, tgt);

          ARC_BITS(arc).set_bit(Find_Path, true);
          NODE_BITS(tgt).set_bit(Find_Path, true);
          NODE_COOKIE(tgt) = src;

          if (op(tgt)) // ¿se encontró un camino que satisfaga op?
            {
              end = tgt;
              break;
            }

          for (Itor<GT, SA> i(tgt); i.has_curr(); i.next())
            {
              auto a = i.get_current_arc();
              if (IS_ARC_VISITED(a, Find_Path)) // ¿arco visitado?
                continue; // sí ==> avanzar al siguiente

              // revise nodos del arco para ver si han sido visitados
              if (IS_NODE_VISITED(g.get_src_node(a), Find_Path) and
                  IS_NODE_VISITED(g.get_tgt_node(a), Find_Path))
                continue; // nodos ya visitados ==> no  meter arco

              q.put(a);
            }
        }

      if (not end)
        return false;

      q.empty();
      path.insert(end);
      auto p = end;
      while (p != start)
        {
          p = (typename GT::Node *) NODE_COOKIE(p);
          path.insert(p);
        }

      return true;
    }

    template <class Op>
    bool find_path(const GT & g, typename GT::Node *start,
                   Path<GT> & path, Op && op)
    {
      return find_path(g, start, path, op);
    }

  public:
    Find_Path_Breadth_First(SA & _sa)
      : sa(_sa)
    { /* Empty */
    }

    Find_Path_Breadth_First(SA && _sa = SA())
      : sa(_sa)
    { /* Empty */
    }

    /** Invoca a la búsqueda de camino en amplitud.

    @param[in] g el grafo sobre el cual se desea buscar el camino.
    @param[in] start puntero al nodo inicio del camino.
    @param[out] path el camino visto durante la búsqueda en
    amplitud; sólo tiene sentido si el valor de retorno es true.
    @param[in] op criterio de fin de búsqueda sobre el nodo visitado
    @throw bad_alloc si no hay memoria para continuar construyendo
    el camino o para la cola interna del recorrido en amplitud.
    */
    template <class Op>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op & op)
    {
      Path<GT> ret(g);
      find_path(g, start, ret, op);
      return ret;
    }

    template <class Op>
    Path<GT> operator ()(const GT & g, typename GT::Node *start, Op && op)
    {
      Path<GT> ret(g);
      find_path(g, start, ret, op);
      return ret;
    }

    /** Invoca a la búsqueda de camino en amplitud.

        @param[in] g el grafo sobre el cual se desea buscar el camino.
        @param[in] start puntero al nodo inicio del camino.
        @graph_utilsparam[in] end puntero al nodo destino del camino.
        @param[out] path el camino visto durante la búsqueda en
        amplitud; sólo tiene sentido si el valor de retorno es true.
        @throw bad_alloc si no hay memoria para continuar construyendo
        el camino o para la cola interna del recorrido en amplitud.
    */
    bool operator ()(const GT & g, typename GT::Node *start,
                     typename GT::Node *end, Path<GT> & path)
    {
      return find_path(g, start, path, [end](auto p) { return p == end; });
    }

    /** Invoca a la búsqueda de camino en amplitud.

        @param[in] g el grafo sobre el cual se desea buscar el camino.
        @param[in] start puntero al nodo inicio del camino.
        @param[in] end puntero al nodo destino del camino.
        @return path el camino visto durante la búsqueda en amplitud
        @throw bad_alloc si no hay memoria para continuar construyendo
        el camino o para la cola interna del recorrido en amplitud.
    */
    Path<GT> operator ()(const GT & g,
                         typename GT::Node *start, typename GT::Node *end)
    {
      Path<GT> ret(g);
      find_path(g, start, ret, [end](auto p) { return p == end; });
      return ret;
    }
  };

  /** Búsqueda de caminos sobre grafos dirigidos definidos mediante una
      clase grafo (no digrafo).

     @ingroup Grafos
   */
  template <class GT,
            template <typename, class> class Itor = Out_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Directed_Find_Path
  {
    const GT & g;
    SA & sa;

    template <template <typename T> class Q, class Op>
    Path<GT> find(typename GT::Node *start, Op & op)
    {
      g.reset_nodes();
      g.reset_arcs();

      start->set_state(Processed);

      Q<typename GT::Arc *> q;
      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next())
        {
          auto a = it.get_curr();
          g.get_tgt_node(a)->set_state(Processing);
          a->set_state(Processing);
          q.put(a);
        }

      typename GT::Node *end = nullptr, *curr = nullptr;
      while (not q.is_empty())
        {
          auto arc = q.get();
          assert(arc->state() == Processing);
          arc->set_state(Processed);

          curr = g.get_tgt_node(arc);
          if (curr->state() == Processed)
            continue;

          curr->set_state(Processed);
          NODE_COOKIE(curr) = g.get_src_node(arc);

          if (op(curr))
            {
              end = curr;
              break;
            }

          for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
            {
              auto a = it.get_curr();
              a->set_state(Processing);

              auto tgt = g.get_tgt_node(a);
              if (tgt->state() == Processed)
                continue;

              if (tgt->state() != Processed)
                {
                  q.put(a);
                  tgt->set_state(Processing);
                }
              else
                a->set_state(Processed);
            }
        } // end while

      Path<GT> ret(g);
      if (not end)
        return ret;

      assert(curr == end);

      while (curr != start)
        {
          ret.insert(curr);
          curr = (typename GT::Node *) NODE_COOKIE(curr);
        }
      ret.insert(start);

      return ret;
    }

  public:
    Directed_Find_Path(const GT & __g, SA & __sa) : g(__g), sa(__sa) {}

    Directed_Find_Path(const GT & __g, SA && __sa = SA()) : g(__g), sa(__sa) {}

    template <class Op>
    Path<GT> dfs(typename GT::Node *start, Op & op)
    {
      return find<DynListStack, Op>(start, op);
    }

    template <class Op>
    Path<GT> dfs(typename GT::Node *start, Op && op)
    {
      return find<DynListStack, Op>(start, op);
    }

    template <class Op>
    Path<GT> bfs(typename GT::Node *start, Op & op)
    {
      return find<DynListQueue, Op>(start, op);
    }

    template <class Op>
    Path<GT> bfs(typename GT::Node *start, Op && op)
    {
      return find<DynListQueue, Op>(start, op);
    }

    Path<GT> dfs(typename GT::Node *start, typename GT::Node *end)
    {
      return dfs(start, [end](auto p) { return p == end; });
    }

    Path<GT> bfs(typename GT::Node *start, typename GT::Node *end)
    {
      return bfs(start, [end](auto p) { return p == end; });
    }
  };
} // namespace Aleph

# endif // TPL_FIND_PATH_H
