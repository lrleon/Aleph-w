/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef DIJKSTRA_H
# define DIJKSTRA_H

# include <ahFunction.H>
# include <archeap.H>
# include <tpl_find_path.H>
# include <tpl_agraph.H>

namespace Aleph {

    // conversión de cookie a Node_Info 
# define DNassert(p) ((Node_Info*) NODE_COOKIE((p)))

    // Acceso al nodo del árbol en el grafo
# define TREENODE(p) (((Tree_Node_Info*)DNassert(p))->tree_node)

# define ACC(p) (DNassert(p)->dist) // Acceso a la distancia acumulada
# define HEAPNODE(p) (DNassert(p)->heap_node)
# define PARENT(p) (DNassert(p)->ret_node)

# define DAassert(p) ((Arc_Info*) ARC_COOKIE(p))
# define ARC_DIST(p) (Distance () (p))
# define TREEARC(p) (((Tree_Arc_Info*)DAassert(p))->tree_arc)
# define POT(p) (DAassert(p)->pot)
# define GRAPHNODE(p) (static_cast<typename GT::Node*>(NODE_COOKIE(p)))



  /** @brief Spanning tree calculation of all shortest paths from
      a given node according to Dijkstra's algorithm.

      @details This class handles Dijkstra's algorithm to compute a tree
      containing of all shortest paths from a node `s` of a graph `g`.

      The algorithm uses an internal queue whose length is proportional
      to the number of nodes in the graph.

      Dijkstra's algorithm does not work for graphs with negative weights.

      The class receives the following template parameters:
      -# `GT`: graph type.
      -# `Distance <GT>`: class reading the weight. It should export the
          following attributes: 
        -# `typedef Distance<GT>::Distance_Type`: Data type. It
	  represents a weight on an arc  
	-# Distance <GT> Distance_Type :: operator () (typename GT :: Arc * a):
           that returns the value of the peso in the bow
        .
      -# `SA`: Arcs filter for internal iterator
      .

      @see Find_Path_Depth_First Floyd_All_Shortest_Paths Bellman_Ford_Min_Spanning_Tree  

  @ingroup Grafos
*/
  template <class GT, 
	    class Distance = Dft_Dist<GT>,
	    template <typename, class> class Itor = Node_Arc_Iterator,
	    class SA       = Dft_Show_Arc<GT>>
class Dijkstra_Min_Paths
{
  // Aunque fundamentalmente el algoritmo es el mismo, hay dos enfoques
  // para calcular el camino mínimo. El primero es pintar el árbol
  // abarcador de todos los caminos mínimos a partir de un nodo
  // start. Por pintar se entiende que la solución se encuentra dentro
  // del mismo grafo y que se distingue mediante marcas.
  //
  // El otro enfoque consiste en construir un árbol abarcador separado.
  //
  // Según un enfoque u otro, el grafo se inicializa con información
  // distinta. Así, las clases que comiencen con el prefijo "Tree" son
  // clases que atañen a la solucipon que construye un árbol abarcador
  // separado. 

  // Información a colocar en el arco para la versión que pinta
  struct Arc_Info
  {
    typename Distance::Distance_Type pot; // potencial del arco
  };

  // Información a colocar en el arco para la versión que construye árbol
  struct Tree_Arc_Info : public Arc_Info
  {
    typename GT::Arc * tree_arc; // imagen en árbol 
    typename Distance::Distance_Type pot; // potencial del arco
  };

  // Wrapper de acceso a la distancia (mediante la clase parámetro Distance)
  struct Get_Potential_Arc : public Distance
  {
    Get_Potential_Arc() noexcept { /* empty */ }

    Get_Potential_Arc(Distance & d) noexcept : Distance(d) { /* empty */ }

    typename Distance::Distance_Type 
    operator () (typename GT::Arc *a) const noexcept
    {
      auto arc_info = (Arc_Info*) ARC_COOKIE(a);
      return arc_info->pot;
    }
  };

  // Información a colocar en el nodo para la versión que pinta
  struct Node_Info
  {
    typename Distance::Distance_Type dist      = 0;      // distancia acumulada
    void *                           heap_node = nullptr;
    void *                           ret_node  = nullptr; // padre en árbol
  };

  // Información a colocar en el nodo para la versión que construye árbol
  struct Tree_Node_Info : public Node_Info
  {
    typename GT::Node *              tree_node = nullptr; // nodo árbol abarcador
    typename Distance::Distance_Type dist;      // distancia acumulada
    void *                           heap_node = nullptr;
  };

  // Acceso al heap de arcos
  struct Dijkstra_Heap_Info
  {
        typedef typename 
    ArcHeap<GT, Get_Potential_Arc, Dijkstra_Heap_Info>::Node Node;

    Node *& operator () (typename GT::Node * p) const noexcept
    {
      return (Node*&) HEAPNODE(p); 
    }
  }; 

  // Inicialización de nodo para la versión que pinta
  struct Initialize_Node
  {
    void operator () (const GT & g, typename GT::Node * p) const noexcept
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Node_Info; 
    }
  };

  // Liberación de memoria de nodo para la versión que pinta
  struct Destroy_Node
  {
    void operator () (const GT &, typename GT::Node * p) const noexcept
    {
      void * tmp = PARENT(p);
      delete DNassert(p); //bloque a liberar
      NODE_COOKIE(p) = tmp;
    }
  };

  // Inicialización de arco para la versión que pinta
  struct Initialize_Arc
  {
    void operator () (const GT & g, typename GT::Arc * a) const noexcept
    {
      g.reset_bit(a, Aleph::Spanning_Tree);
      ARC_COOKIE(a) = new Arc_Info;
      POT(a) = 0;
    }
  };

  // Liberación de memoria de arco para la versión que pinta
  struct Destroy_Arc
  {
    void operator () (const GT &, typename GT::Arc * ga) const noexcept
    {
      delete DAassert(ga); 
    }
  };

  // Inicialización de memoria de nodo para la versión que construye árbol
  struct Initialize_Tree_Node
  {
    void operator () (const GT & g, typename GT::Node * p) const noexcept
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Tree_Node_Info; 
    }
  };

  // Liberación de memoria de nodo y mapeo para versión que construye árbol
  struct Destroy_Tree_Node
  {
    void operator () (const GT &, typename GT::Node * p) const noexcept
    {
      auto aux = (Tree_Node_Info *) DNassert(p); //bloque a liberar
      auto tp = TREENODE(p); // imagen en árbol abarcador
      if (tp != nullptr) // ¿está este nodo incluído en el árbol abarcador? 
	{
	  NODE_COOKIE(p) = NODE_COOKIE(tp) = nullptr;
	  GT::map_nodes (p, tp);
	}
      else
	NODE_COOKIE(p) = nullptr;

      delete aux; 
    }
  };

  // Inicialización de arco para la versión que construye árbol
  struct Initialize_Tree_Arc
  {
    void operator () (const GT & g, typename GT::Arc * a) const noexcept
    {
      g.reset_bit(a, Aleph::Spanning_Tree);
      ARC_COOKIE(a) = new Tree_Arc_Info;
      POT(a) = 0;
      TREEARC(a) = nullptr;
    }
  };

  // Liberación de memoria y mapeo para la versión que construye árbol
  struct Destroy_Tree_Arc
  {
    void operator () (const GT &, typename GT::Arc * ga) const noexcept
    {
      auto aux = (Tree_Arc_Info *) DAassert(ga); 
      typename GT::Arc * ta = TREEARC(ga);
      if (ta != nullptr) // ¿pertenece este arco al árbol abarcador?
	{    
	  assert(IS_ARC_VISITED(ga, Aleph::Spanning_Tree));
	  GT::map_arcs (ga, ta); // sí ==> mapearlo
	}

      delete aux;
    }
  };

  typedef Dijkstra_Heap_Info Heap_Info;

  typedef ArcHeap<GT, Get_Potential_Arc, Heap_Info> Heap;

  SA                  sa;
  Get_Potential_Arc   get_pot;
  Heap                heap;
  bool                painted = false;
  GT *                ptr_g = nullptr;
  typename GT::Node * s = nullptr;

public:

  // Constructores
  
  /** Constructor

      \param[in] dist acceso a la distancia de arco.
      \param[in] cmp comparador entre la distancias de arcos.
      \param[in] __sa filtro de arcos para los iteradores.
   */
  Dijkstra_Min_Paths(Distance dist = Distance(),  
		     SA         __sa = SA())
  noexcept(std::is_nothrow_move_assignable<SA>::value)
    : sa(__sa), get_pot(dist), heap(get_pot, Heap_Info()), 
      painted(false), ptr_g(nullptr), s(nullptr)
  {
    // empty
  }

private:

      template <class IN, class IA>
  void init(const GT & g, typename GT::Node * start)
  {
    heap.empty();

    ptr_g = &const_cast<GT&>(g);
    s     = start;

    Operate_On_Nodes<GT, IN> () (g);

    (Operate_On_Arcs <GT, IA> (sa)) (g);
  }

     template <class DN, class DA>
  void uninit()
  {
    Operate_On_Nodes <GT, DN> () (*ptr_g);

    (Operate_On_Arcs <GT, DA, SA> (sa)) (*ptr_g);
  }

public:

  /** Calcula el árbol abarcador de todos los caminos mínimos que parten
      desde el nodo start.

      @param[in] g el grafo.
      @param[in] start el nodo inicio de todos los caminos mínimos
      @param[out] tree el árbol abarcador de todos los caminos mínimos
      que comienzan por start mapeado mediante los cookies al grafo g.
      @throw bad_alloc si no hay suficiente memoria
   */
  typename GT::Node *
  compute_min_paths_tree(const GT & g, typename GT::Node * start, GT & tree)
  {
    init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);

    clear_graph(tree); // limpiar árbol abarcador destino 

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start)                   = 0; 
    auto ret = TREENODE(start) = tree.insert_node(start->get_info());
    NODE_COOKIE(TREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	auto arc = it.get_current_arc();
	POT(arc) = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const auto & n = g.get_num_nodes();

    while (tree.get_num_nodes() < n)  // mientras tree no abarque a g
      {
	auto garc  = heap.get_min_arc(); 
	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	auto gsrc = g.get_src_node(garc);
	auto gtgt = g.get_tgt_node(garc);

	      // ¿Están los dos nodos visitados?
	if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  continue; // insertar arco causaría ciclo

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);	  

	if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  std::swap(gsrc, gtgt);

	NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

	auto ttgt = tree.insert_node(gtgt->get_info());
	TREENODE(gtgt) = ttgt;
	auto tsrc = TREENODE(gsrc);

	auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
	TREEARC(garc) = tarc;

	ACC(gtgt) = ACC(gsrc) + ARC_DIST(garc); // total dist nodo inicial
	const auto & acc = ACC(gtgt);

	// por cada arco calcular potencial e insertarlo en heap
	for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next()) 
	  {
	    auto arc = it.get_current_arc();
	    if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
	      continue;

	    auto tgt = it.get_tgt_node();
	    if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree)) 
	      continue; // causaría ciclo ==> no se mete en heap  

	    POT(arc) = acc + ARC_DIST(arc); // calcula potencial
	    heap.put_arc(arc, tgt);
	  }
      } 

    uninit<Destroy_Tree_Node, Destroy_Tree_Arc> ();

    return ret;
  }

  /** Calcula el árbol abarcador parcial de todos los caminos mínimos
      que parten desde el nodo start y que contiene al camino start-end.

      compute_partial_min_paths_tree() construye el árbol abarcador de
      caminos mínimos que parten desde start pero el cálculo se detiene
      cuando se encuentra el nodo end.

      @param[in] g el grafo.
      @param[in] start el nodo inicio de todos los caminos mínimos
      @param[in] end 
      @param[out] tree el árbol abarcador de todos los caminos mínimos
      que comienzan por start mapeado mediante los cookies al grafo g.
      @throw bad_alloc si no hay suficiente memoria
  */
  void compute_partial_min_paths_tree(const GT & g, 
				      typename GT::Node * start,
				      typename GT::Node * end, 
				      GT & tree)
  {
    init <Initialize_Tree_Node, Initialize_Tree_Arc> (g, start);
    clear_graph(tree);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start)                   = 0; 
    TREENODE(start)              = tree.insert_node(start->get_info());
    NODE_COOKIE(TREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	auto arc = it.get_current_arc();
	POT(arc) = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const auto & n = g.get_num_nodes();

    while (tree.get_num_nodes() < n and not heap.is_empty()) // tree no abarque
      {
	auto garc  = heap.get_min_arc(); 
	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	auto gsrc = g.get_src_node(garc);
	auto gtgt = g.get_tgt_node(garc);

	// ¿Están los dos nodos visitados?
	if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  continue; // insertar arco causaría ciclo

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

	if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
	  std::swap(gsrc, gtgt);

	auto ttgt      = tree.insert_node(gtgt->get_info());
	TREENODE(gtgt) = ttgt;
	NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

	auto tarc = // insertar nuevo arco en tree 
	  tree.insert_arc(TREENODE(gsrc), TREENODE(gtgt), garc->get_info());
	TREEARC(garc) = tarc;

	if (gtgt == end) // ¿está end_node en árbol abarcador? 
	  break; // sí ==> camino mínimo ya está en árbol abarcador

	ACC(gtgt) = ACC(gsrc) + ARC_DIST(garc); // total dist nodo inicial
	const auto & acc = ACC(gtgt);

	      // por cada arco calcular potencial e insertarlo en heap
	for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next()) 
	  {
	    auto arc = it.get_current_arc();
	    if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
	      continue;

	    auto tgt = it.get_tgt_node();
	    if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree)) 
	      continue; // causaría ciclo ==> no se mete en heap  

	    POT(arc) = acc + ARC_DIST(arc); // calcula potencial
	    heap.put_arc(arc, tgt);
	  }
      }

    uninit <Destroy_Tree_Node, Destroy_Tree_Arc> ();
  }

  /** Pinta sobre el grafo g el árbol parcial de caminos mínimos que
      comienzan desde start y se detiene cuando se encuentra el nodo
      end.

      @param[in] g el grafo.
      @param[in] start el nodo inicio del camino mínimo.
      @param[in] end el nodo final del camino mínimo.
      @throw bad_alloc si no hay suficiente memoria.
   */
  bool paint_partial_min_paths_tree(const GT & g, 
				    typename GT::Node * start, 
				    typename GT::Node * end)
  {
    bool ret_val = false;
    init<Initialize_Node, Initialize_Arc> (g, start);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start) = 0; 

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	auto arc = it.get_current_arc();
	POT(arc) = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const auto & n = g.get_num_nodes();
    size_t tn = 1; // número de nodos pintados

    while (tn < n and not heap.is_empty()) // tree no abarque g
      {
	auto garc  = heap.get_min_arc(); 
	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	auto src = g.get_src_node(garc);
	auto tgt = g.get_tgt_node(garc);

	// ¿Están los dos nodos visitados?
	if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  continue; // este arco causaría un ciclo

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

	if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  std::swap(src, tgt);

	NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
	PARENT(tgt) = src; 

	++tn; // simula que p se metió en el árbol abarcador

	if (tgt == end) // ¿está end_node en árbol abarcador? 
	  {
	    ret_val = true;
	    break; // sí ==> camino mínimo ya está pintado
	  }

	ACC(tgt) = ACC(src) + ARC_DIST(garc); // total dist nodo inicial

	const auto & acc = ACC(tgt);

	// por cada arco calcular potencial e insertarlo en heap
	for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next()) 
	  {
	    auto a = it.get_current_arc();
	    if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
	      continue;

	    auto t = it.get_tgt_node();
	    if (IS_NODE_VISITED(t, Aleph::Spanning_Tree)) 
	      continue; // causaría ciclo ==> no se mete en heap  

	    POT(a) = acc + ARC_DIST(a); // calcula potencial
	    heap.put_arc(a, t);
	  }
      }

    uninit<Destroy_Node, Destroy_Arc> ();
    painted = true;
    
    return ret_val;
  }

  /** Pinta sobre el grafo g el árbol de todos caminos mínimos que
      comienzan desde start.

      @param[in] g el grafo.
      @param[in] start el nodo inicio del camino mínimo.
      @throw bad_alloc si no hay suficiente memoria.
  */
  void paint_min_paths_tree(const GT & g, typename GT::Node * start)
  {
    init<Initialize_Node, Initialize_Arc> (g, start);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true); 
    ACC(start) = 0; 

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next()) 
      {
	auto arc = it.get_current_arc();
	POT(arc) = ARC_DIST(arc);
	heap.put_arc(arc, it.get_tgt_node());
      }

    const auto & n = g.get_num_nodes();
    size_t tn = 1; // número de nodos pintados

    while (tn < n and not heap.is_empty()) // tree no abarque g
      {
	auto garc  = heap.get_min_arc(); 
	if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
	  continue;

	auto src = g.get_src_node(garc);
	auto tgt = g.get_tgt_node(garc);

	// ¿Están los dos nodos visitados?
	if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and 
	    IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  continue; // este arco causaría un ciclo

	ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

	if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
	  std::swap(src, tgt);

	NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
	PARENT(tgt) = src; 

	++tn; // simula que p se metió en el árbol abarcador

	ACC(tgt) = ACC(src) + ARC_DIST(garc); // total dist nodo inicial
	const auto & acc = ACC(tgt);

	// por cada arco calcular potencial e insertarlo en heap
	for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next()) 
	  {
	    auto a = it.get_current_arc();
	    if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
	      continue;

	    auto t = it.get_tgt_node();
	    if (IS_NODE_VISITED(t, Aleph::Spanning_Tree)) 
	      continue; // causaría ciclo ==> no se mete en heap  

	    POT(a) = acc + ARC_DIST(a); // calcula potencial
	    heap.put_arc(a, t);
	  }
      }

    uninit<Destroy_Node, Destroy_Arc> ();
    painted = true;
  }

  /** Extrae un camino mínimo con destino en end sobre un grafo
      previamente pintado.

      get_min_path() toma un grafo g sobre el cual están
      pintados los caminos mínimos (total o parcial) mediante llamadas
      previas a paint_min_paths_tree() o paint_partial_min_paths_tree()
      y extrae el camino mínimo hacia end guardándolo en path.

      El resultado es indeterminado si end no está contenido dentro del
      grafo; es decir, se llamó previamente a
      paint_partial_min_paths_tree() con un nodo destino distinto a end
      y el resultado no contiene a end. 

      Una llamada previa a paint_min_paths_tree() o
      paint_partial_min_paths_tree() almacena el grafo y nodo origen.  
      
      @param[in] end nodo destino del camino.
      @param[out] path camino donde se guardará el mínimo.
      @throw bad_alloc sino hay suficiente memoria.
      @throw domain_error si el grafo no ha sido previamente pintado
      mediante paint_min_paths_tree() o paint_partial_min_paths_tree().
   */
      typename Distance::Distance_Type 
  get_min_path(typename GT::Node * end, Path<GT> & path)
  {
    if (ptr_g == nullptr)
      throw std::domain_error("Min path has not been computed");

    if (not painted)
      throw std::domain_error("Graph has not previously painted");
      
    return Aleph::get_min_path<GT, Distance>(s, end, path);
  }

  /** Calcula el camino mínimo entre el nodo start y end por pintado del
      grafo.

      Esta es la versión de menor consumo de memoria y probablemente
      también la más rápida. Note que cada llamada calcula el árbol
      parcial de caminos mínimos desde start.

      Use get_min_path() si ya calculó el árbol abarcador y desea
      simplemente obtener el camino mínimo.

      @param[in] g el grafo.
      @param[in] start el nodo inicio del camino.
      @param[in] end el nodo final del camino.
      @param[in] min_path el camino mínimo.
      @return el coste total del camino desde start hasta end.
      @throw bad_alloc si no hay suficiente memoria
      @see get_min_parth
   */
      typename Distance::Distance_Type 
  find_min_path(const GT & g, 
		typename GT::Node * start, typename GT::Node * end, 
		Path<GT> & min_path)
  {
    min_path.empty();
    if (paint_partial_min_paths_tree(g, start, end))
      return get_min_path(end, min_path);

    return numeric_limits<typename Distance::Distance_Type>::max(); 
  }
  
      /** Calcula el árbol abarcador de todos los caminos mínimos desde
          un nodo dado según el algoritmo de Dijkstra. 

          El árbol abarcador tree de todos los caminos mínimos resultante
          start_node está completamente mapeado con g. Por tanto, una
          búsqueda de camino en profundidad, mediante
          Find_Path_Depth_First, sobre tree encontrará y construirá
          cualquier camino mínimo desde start_node hasta un nodo
          cualquiera. 

          @param[in] g el grafo al cual se desea calcular el árbol
          abarcador mínimo.
          @param[in] s nodo inicio de los caminos mínimos.
          @param[out] tree el grafo donde se desea guardar el árbol
          abarcador resultado de todos los caminos mínimos que parten
          desde el nodo start_node. Este grafo es limpiado antes del
          comienzo del algoritmo.
          @throw bad_alloc si no hay suficiente memoria para construir
          tree; bien sea porque falla tree o por la cola interna. En este 
          caso el valor de tree es indeterminado y no está limpio.  
       */
  void operator () (const GT & g, typename GT::Node * s, GT & tree)
  {
    compute_min_paths_tree(g, s, tree);
  }

  /** Extrae el árbol de caminos mínimos (parcial o total) y lo pone en
      tree.

      copy_painted_min_paths_tree() toma un grafo g sobre el cual están
      pintados los caminos mínimos (total o parcial) mediante llamadas
      previas a paint_min_paths_tree() o paint_partial_min_paths_tree()
      y extrae una copia hacia el parámetro tree.

      @param[in] g el grafo previamente pintado donde están los caminos
      mínimos.
      @param[out] tree el grafo donde se desea copiar el árbol abarcador
      @throw bad_alloc si no hay suficiente memoria.
  */
      typename Distance::Distance_Type 
  copy_painted_min_paths_tree(const GT & g, GT & tree)
  {
    if (not painted)
      throw std::domain_error("Graph has not previously painted");

    using Paint_Filt = Painted_Min_Spanning_Tree<GT, Distance>;
    Paint_Filt painted;
    (Copy_Graph<GT, Dft_Show_Node<GT>, Paint_Filt> (painted)) (tree, g);

    return painted.dist;
  }

  /// \oveload find_min_path()
  typename Distance::Distance_Type operator () (const GT & g, 
						typename GT::Node * s, 
						typename GT::Node * e,
						Path<GT> & path)
  {
    return find_min_path (g, s, e, path);
  }

      /// Clase totalizadora de distancias.
  struct Total
  {
    typename  Distance::Distance_Type dist;
    
    Total() noexcept : dist(0) { /* empty */ }

    bool operator () (typename GT::Arc * a) noexcept
    {
      dist += ARC_DIST(a);
      return true;
    }
  };

  /** Extrae un camino mínimo con destino en end sobre un árbol de
      caminos mínimos previamente calculado.

      get_min_path() toma un árbol tree, previamente construido mediante
      compute_min_paths_tree() o compute_partial_min_paths_tree() 
      y extrae el camino mínimo hacia end guardándolo en path.

      El resultado es indeterminado si end no está contenido dentro del
      árbol; es decir, se llamó previamente a
      compute_partial_min_paths_tree() con un nodo destino distinto a end
      y el resultado no contiene a end. 

      @param[in] tree árbol de caminos mínimos el cual debe haber sido
      previamente calculado mediante compute_min_paths_tree() o
      compute_partial_min_paths_tree(). 
      @param[in] end nodo destino del camino.
      @param[out] path camino donde se guardará el mínimo.
      @throw bad_alloc sino hay suficiente memoria.
      @throw domain_error si el grafo no ha sido previamente pintado
      mediante paint_min_paths_tree() o paint_partial_min_paths_tree().
   */
      typename Distance::Distance_Type 
  get_min_path(const GT & tree, typename GT::Node * end, Path<GT> & path)
  {
    if (ptr_g == nullptr)
      throw std::domain_error("Min path has not been computed");

    auto ts = mapped_node<GT>(s);
    auto te = mapped_node<GT>(end);

    Path<GT> tree_path(tree);
    Total total;
    (Find_Path_Depth_First<GT, Itor, Total> (total)) (tree, ts, te, tree_path); 

    path.empty();
    path.init(s);
    typename Path<GT>::Iterator it(tree_path);
    for (it.next(); it.has_curr(); it.next())
      path.append(mapped_node<GT>(it.get_current_node()));

    return total.dist;
  }
};



# undef DNI
# undef PARENT
# undef TREENODE
# undef ACC
# undef HEAPNODE
# undef DAI
# undef ARC_DIST
# undef TREEARC
# undef POT
# undef GRAPHNODE
} // end namespace Aleph
# endif // DIJKSTRA_H
