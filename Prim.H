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

/** @file Prim.H
 *  @brief Prim's algorithm for minimum spanning trees.
 *
 *  This file implements Prim's algorithm for finding the Minimum Spanning
 *  Tree (MST) of a connected, undirected, weighted graph. The algorithm
 *  grows the MST one vertex at a time, always adding the minimum-weight
 *  edge that connects a vertex in the tree to a vertex outside.
 *
 *  ## Algorithm Overview
 *
 *  1. Start with an arbitrary vertex
 *  2. Maintain a priority queue of edges crossing the cut
 *  3. Extract minimum-weight edge, add its endpoint to the tree
 *  4. Add new crossing edges to the queue
 *  5. Repeat until all vertices are in the tree
 *
 *  ## Key Features
 *
 *  - Finds minimum spanning tree for connected graphs
 *  - Uses binary heap for efficient edge selection
 *  - Supports custom distance/weight functions
 *  - Works with both List_Graph and Array_Graph
 *
 *  ## Complexity
 *
 *  | Implementation | Time | Space |
 *  |----------------|------|-------|
 *  | Binary Heap | O(E log V) | O(V) |
 *
 *  ## Comparison with Kruskal
 *
 *  | Aspect | Prim | Kruskal |
 *  |--------|------|---------|
 *  | Best for | Dense graphs | Sparse graphs |
 *  | Data structure | Priority queue | Union-Find |
 *  | Edge processing | One at a time | All sorted first |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<Node, Arc> g;
 *  // ... build connected graph ...
 *
 *  List_Graph<Node, Arc> mst;
 *  Prim_Min_Spanning_Tree<decltype(g)> prim;
 *  prim(g, mst);
 *
 *  // mst now contains the minimum spanning tree
 *  ```
 *
 *  @see Kruskal.H Alternative MST algorithm (better for sparse graphs)
 *  @see tpl_graph_utils.H For graph traversal utilities
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef PRIM_H
# define PRIM_H

# include <tpl_agraph.H>
# include <tpl_graph_utils.H>
# include <archeap.H>
# include <ah-errors.H>
# include <cookie_guard.H>

namespace Aleph
{
  using namespace Aleph;

  template <class GT>
  struct Prim_Info
  {
    typename GT::Node *tree_node = nullptr; // imagen en el árbol abarcador
    void *heap_node = nullptr; // puntero en el heap exclusivo

    Prim_Info() : tree_node(nullptr), heap_node(nullptr)
    { /* empty */
    }
  };

# define PRIMINFO(p) static_cast<Prim_Info<GT>*>(NODE_COOKIE(p))
# define TREENODE(p) (PRIMINFO(p)->tree_node)
# define HEAPNODE(p) (PRIMINFO(p)->heap_node)

  template <class GT, class Distance>
  struct Prim_Heap_Info
  {
    // Returns reference to the heap_node pointer stored in Prim_Info
    // The actual type is determined by ArcHeap, but stored as void*
    void *& operator ()(typename GT::Node *p)
    {
      return PRIMINFO(p)->heap_node;
    }
  };

  template <class GT, class Distance>
  struct Simple_Prim_Heap
  {
    // Returns reference to the cookie pointer used as heap node storage
    void *& operator ()(typename GT::Node *p)
    {
      return p->cookie;
    }
  };

  template <class GT>
  struct Init_Prim_Info
  {
    GT & tree;

    Init_Prim_Info(GT & __tree) : tree(__tree)
    { /* empty */
    }

    void operator ()(const GT & g, typename GT::Node *p)
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Prim_Info<GT>;
      TREENODE(p) = tree.insert_node(p->get_info());
    }
  };

  template <class GT>
  struct Uninit_Prim_Info
  {
    void operator ()(const GT &, typename GT::Node *p)
    {
      const Prim_Info<GT> *aux = PRIMINFO(p);
      GT::map_nodes(p, TREENODE(p));
      delete aux;
    }
  };


  /** Calcula el árbol abarcador mínimo de un grafo según el
      algoritmo de Prim.

      Esta clase emplea el algoritmo de Prim para
      calcular el árbol abarcador mínimo de un grafo y almacenarlo
      en otro grafo.

      El árbol abarcador mínimo tree completamente mapeado con el grafo.

      El algoritmo utiliza una cola interna cuya longitud máxima es
      proporcional número de nodos del grafo.

      El algoritmo de Prim es el recomendado para grafos densos.

      El procedimiento es parametrizado con las siguientes
      especificaciones:
      -# GT: el tipo de grafo basado en List_Graph.
      -# Distance<GT>: La clase de lectura del peso del arco  que debe
      exportar los siguientes atributos:
        -# typedef Distance<GT>::Distance_Type: el tipo de dato que
        representa un peso en un arco.
        -# Distance<GT>::Distance_Type operator()(typename GT::Arc *a):
        que retorna el valor del peso en el arco a.
        -# Distance<GT>::Max_Distance: constante estática
        correspondiente al valor de distancia máximo que un algoritmo
        consideraría como valor infinito.
        -# typename Distance<GT>::Zero_Distance: constante estática
        correspondiente al elemento neutro de la suma. Tradicionalmente,
        en la inmensa mayoría de casos, este será el cero.
        .
      -# Compare<GT>: clase que realiza la comparación entre dos
         pesos y cuyo prototipo es:
         - bool operator () (const typename
           Distance<GT>::Distance_Type & op1,
           const typename Distance<GT>::Distance_Type & op2) const
         . Por omisión, esta clase implanta el operador relacional
         menor que.
      -# SA: filtro de arcos

      @see Kruskal_Min_Spanning_Tree
      @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Prim_Min_Spanning_Tree
  {
    typedef Prim_Heap_Info<GT, Distance> Acc_Heap;

    typedef Simple_Prim_Heap<GT, Distance> Acc_Simple_Heap;

    typedef ArcHeap<GT, Distance, Acc_Heap> Heap;

    typedef ArcHeap<GT, Distance, Acc_Simple_Heap> Simple_Heap;

    Distance dist;
    SA sa;

  public:
    /** Constructor.

        \param[inout] __dist acceso a la distancia de cada arco
        \param[inout] __cmp comparación entre distancias de arco
        \param[inout] __sa filtro de iterador de arcos
    */
    Prim_Min_Spanning_Tree(Distance __dist = Distance(), SA __sa = SA())
      : dist(__dist), sa(__sa)
    {
      // empty
    }

  private:
    void paint_min_spanning_tree(const GT & g, typename GT::Node *first)
    {
      ah_domain_error_if(g.is_digraph()) << "g is a digraph";

      g.reset_nodes();
      g.reset_arcs();

      NODE_BITS(first).set_bit(Aleph::Spanning_Tree, true); // visitado

      Simple_Heap heap(dist, Acc_Simple_Heap());
      for (Node_Arc_Iterator<GT, SA> it(first, sa); it.has_curr(); it.next_ne())
        {
          typename GT::Arc *arc = it.get_current_arc_ne();
          heap.put_arc(arc, it.get_tgt_node_ne());
        }

      const size_t V1 = g.get_num_nodes() - 1;
      size_t count = 0;

      while (count < V1 and not heap.is_empty())
        { // obtenga siguiente menor arco
          typename GT::Arc *min_arc = heap.get_min_arc();
          if (IS_ARC_VISITED(min_arc, Aleph::Spanning_Tree))
            continue;

          ARC_BITS(min_arc).set_bit(Aleph::Spanning_Tree, true);

          typename GT::Node *src = g.get_src_node(min_arc);
          typename GT::Node *tgt = g.get_tgt_node(min_arc);
          if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            continue; // este arco cerraría un ciclo en el árbol

          typename GT::Node *tgt_node =
              IS_NODE_VISITED(src, Aleph::Spanning_Tree) ? tgt : src;

          NODE_BITS(tgt_node).set_bit(Aleph::Spanning_Tree, true);

          // insertar en heap arcos no visitados de tgt_node
          for (Node_Arc_Iterator<GT, SA> it(tgt_node, sa); it.has_curr();
               it.next_ne())
            {
              typename GT::Arc *arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              typename GT::Node *tgt = it.get_tgt_node_ne();
              if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
                continue; // nodo visitado ==> causará ciclo

              heap.put_arc(arc, tgt);
            }

          ++count;
          ARC_BITS(min_arc).set_bit(Aleph::Spanning_Tree, true);
        }
    }

    void min_spanning_tree(const GT & g, typename GT::Node *first, GT & tree)
    {
      ah_domain_error_if(g.is_digraph()) << "g is a digraph";

      clear_graph(tree);

      Init_Prim_Info<GT> init(tree);
      Operate_On_Nodes<GT, Init_Prim_Info<GT>>()(g, init);
      g.reset_arcs();

      // Scope guard for exception safety - ensures Uninit is always called
      Scope_Guard cleanup_guard(g, [](const GT & graph) {
        Operate_On_Nodes<GT, Uninit_Prim_Info<GT>>()(graph);
      });

      NODE_BITS(first).set_bit(Aleph::Spanning_Tree, true); // visitado

      Heap heap(dist, Acc_Heap());
      // meter en heap arcos iniciales del primer nodo
      for (Node_Arc_Iterator<GT, SA> it(first, sa); it.has_curr(); it.next_ne())
        {
          typename GT::Arc *arc = it.get_current_arc_ne();
          heap.put_arc(arc, it.get_tgt_node_ne());
        }

      const size_t V1 = g.get_num_nodes() - 1;

      while (tree.get_num_arcs() < V1 and not heap.is_empty())
        { // obtenga siguiente menor arco
          typename GT::Arc *min_arc = heap.get_min_arc();
          if (IS_ARC_VISITED(min_arc, Aleph::Spanning_Tree))
            continue;

          ARC_BITS(min_arc).set_bit(Aleph::Spanning_Tree, true);

          typename GT::Node *src = g.get_src_node(min_arc);
          typename GT::Node *tgt = g.get_tgt_node(min_arc);
          if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            continue; // este arco cerraría un ciclo en el árbol

          typename GT::Node *tgt_node =
              IS_NODE_VISITED(src, Aleph::Spanning_Tree) ? tgt : src;

          NODE_BITS(tgt_node).set_bit(Aleph::Spanning_Tree, true);

          // insertar en heap arcos no visitados de tgt_node
          for (Node_Arc_Iterator<GT, SA> it(tgt_node, sa); it.has_curr();
               it.next_ne())
            {
              typename GT::Arc *arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              typename GT::Node *tgt = it.get_tgt_node_ne();
              if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
                continue; // nodo visitado ==> causará ciclo

              heap.put_arc(arc, tgt);
            }

          // insertar nuevo arco en tree y mapearlo
          typename GT::Arc *tree_arc =
              tree.insert_arc(TREENODE(src), TREENODE(tgt), min_arc->get_info());
          GT::map_arcs(min_arc, tree_arc);
        }

      // cleanup_guard destructor will call Uninit_Prim_Info for all nodes
    }

  public:
    /** Invoca al cálculo del árbol abarcador mínimo por el algoritmo de
        Prim.

        @param[in] g el grafo al cual se desea calcular el árbol
        abarcador mínimo.
        @param[out] tree el grafo donde se desea guardar el árbol
        abarcador mínimo resultado. Este grafo es limpiado antes del
        comienzo del algoritmo.
        @throw bad_alloc si no hay suficiente memoria para construir
        tree. En este caso el valor de tree es indeterminado y no está
        limpio.
    */
    void operator ()(const GT & g, GT & tree)
    {
      min_spanning_tree(g, g.get_first_node(), tree);
    }

    /** Invoca al cálculo del árbol abarcador mínimo por el algoritmo de
        Prim.

        @param[in] g el grafo al cual se desea calcular el árbol
        abarcador mínimo.
        @param[in] start nodo desde el cual se inicia elalgoritmo.
        @param[out] tree el grafo donde se desea guardar el árbol
        abarcador mínimo resultado. Este grafo es limpiado antes del
        comienzo del algoritmo.
        @throw bad_alloc si no hay suficiente memoria para construir
        tree. En este caso el valor de tree es indeterminado y no está
        limpio.
    */
    void operator ()(const GT & g, typename GT::Node *start, GT & tree)
    {
      min_spanning_tree(g, start, tree);
    }

    /// overload ()
    void operator ()(const GT & g)
    {
      paint_min_spanning_tree(g, g.get_first_node());
    }

    void operator ()(const GT & g, typename GT::Node *start)
    {
      paint_min_spanning_tree(g, start);
    }
  };


# undef HEAPNODE
# undef TREENODE
# undef PRIMINFO
} // end namespace Aleph

# endif // PRIM_H
