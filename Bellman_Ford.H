# ifndef BELLMAN_FORD_H
# define BELLMAN_FORD_H

# include <tpl_dynListQueue.H>
# include <tpl_dynSetTree.H>
# include <tpl_graph_utils.H>
# include <Tarjan.H>

namespace Aleph {

template <class GT, class Distance> struct Bellman_Ford_Node_Info
{
  int                              idx;
  typename Distance::Distance_Type acum;
};

/** Algorithms associated to Bellman-Ford algorithm for searching
    minimal paths in a graph.

   @ingroup Grafos
 */
    template <class GT, 
	      class Distance                     = Dft_Dist<GT>, 
	      template <class, class> class Ait  = Arc_Iterator,
	      template <class, class> class NAit = Out_Iterator,
	      class SA                           = Dft_Show_Arc<GT>> 
class Bellman_Ford
{
  typedef typename Distance::Distance_Type Distance_Type;

  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

  struct Sni
  {
    Distance_Type acum;
  };

  struct Ni : public Sni
  {
    int idx; // índice en los arreglos de predecesores
  };

  Distance_Type & acum(Node * p) noexcept
  {
    return ((Sni*) NODE_COOKIE(p))->acum;
  }

  int & idx(Node * p) noexcept
  {
    return ((Ni*) NODE_COOKIE(p))->idx;
  }

  DynArray<typename GT::Arc*>  arcs;
  const GT & g;
  const Distance_Type Inf;
  bool painted;
  Node * s;
  SA & sa;
  Distance & dist;

  // Inicializa sin usar los arreglos
  void init_simple(Node * start)
  {
    typename GT::Node_Iterator it(g);
    for (int i = 0; it.has_curr(); ++i, it.next())
      {
        auto p = it.get_curr();
        g.reset_bit(p, Aleph::Spanning_Tree); // colocar bit en cero
        auto ptr = new Sni;
        ptr->acum = Inf;
        NODE_BITS(p).set_bit(Spanning_Tree, false); 
        NODE_COOKIE(p) = ptr;
    }
    s = start;
    acum(s) = 0;
    g.reset_arcs();
  }

  void init_with_indexes(Node * start)
  {
    arcs.reserve(g.get_num_nodes());
    typename GT::Node_Iterator it(g);
    for (int i = 0; it.has_curr(); ++i, it.next())
      {
	arcs(i) = nullptr;
        auto p = it.get_curr();
        g.reset_bit(p, Aleph::Spanning_Tree); // colocar bit en cero
        auto ptr = new Ni;
        ptr->acum = Inf;
	ptr->idx = i;
        NODE_BITS(p).set_bit(Spanning_Tree, false);
	NODE_BITS(p).set_bit(Depth_First, false); // indicates if it is in queue
        NODE_COOKIE(p) = ptr;
    }

    s = start;
    acum(s) = 0;

    g.reset_arcs();
  }

  // libera la memoria asociada a los cookies
  template <class Info_Type>
  void uninit() noexcept
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	auto p = it.get_curr();
	delete (Info_Type*) NODE_COOKIE(p);
	NODE_COOKIE(p) = nullptr;
      }
  }

  bool check_painted_arcs() noexcept
  {
    auto num_arcs = 0;
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree))
	++num_arcs;

    return num_arcs == g.get_num_nodes() - 1 or num_arcs == g.get_num_nodes();
  }

public:

  Bellman_Ford(const GT & __g, Distance & d, SA & __sa) noexcept
    : g(const_cast<GT&>(__g)), Inf(std::numeric_limits<Distance_Type>::max()), 
      painted(false), sa(__sa), dist(d)
  {
    // empty
  }

  Bellman_Ford(const GT & __g, Distance && d = Distance(), SA && __sa = SA()) 
    noexcept
    : Bellman_Ford(__g, d, __sa)
  {
    // Empty
  }

  Bellman_Ford(const GT & __g, Distance && d, SA & __sa) noexcept
    : Bellman_Ford(__g, d, __sa)
  {
    // Empty
  }

  Bellman_Ford(const GT & __g, Distance & d, SA && __sa) noexcept
    : Bellman_Ford(__g, d, __sa)
  {
    // Empty
  }

private:
  
  void relax_arcs() noexcept
  {
    const size_t & n = g.vsize();
    for (int i = 0; i < n - 1; ++i)
      for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next()) 
	{
	  auto arc = it.get_curr();
	  auto src  = g.get_src_node(arc);
	  const auto & acum_src = acum(src);
	  if (acum_src == Inf)
	    continue;
	  
	  auto tgt = it.get_tgt_node();
	  auto w = dist(arc);
	  auto sum = acum_src + w;
	  auto & acum_tgt = acum(tgt);
	  if (sum < acum_tgt) // relajar arco
	    {
	      const auto & index = idx(tgt);
	      arcs(index)        = arc;
	      acum_tgt           = sum;
	    }
	}
  }

  static void 
  put_in_queue(DynListQueue<typename GT::Node*> & q, typename GT::Node * p)
  {
    if (IS_NODE_VISITED(p, Depth_First)) // is already inside the queue?
      return;
    NODE_BITS(p).set_bit(Depth_First, true);
    q.put(p);
  }

  static typename GT::Node * get_from_queue(DynListQueue<typename GT::Node*>& q)
  {
    auto ret = q.get();
    assert(IS_NODE_VISITED(ret, Depth_First));
    NODE_BITS(ret).set_bit(Depth_First, false);
    return ret;
  }

  void relax_arcs(typename GT::Node * src, DynListQueue<typename GT::Node*> & q)
  {
    for (NAit<GT, SA> it(src, sa); it.has_curr(); it.next()) 
        {
          auto arc = it.get_curr();
	  auto src  = g.get_src_node(arc);
          const auto & acum_src = acum(src);
          if (acum_src == Inf)
            continue;

          auto tgt = g.get_tgt_node(arc);
          auto w = dist(arc);
          auto sum = acum_src + w;
          auto & acum_tgt = acum(tgt);
          if (sum < acum_tgt) // relajar arco
            {
              const auto & index = idx(tgt);
              arcs(index)        = arc;
              acum_tgt           = sum;
	      put_in_queue(q, tgt);
            }
        }
  }

  void paint_tree() noexcept
  {    // pinta los nodos y arcos involucrados
    const auto & n = g.vsize();
    for (int i = 0; i < n; ++i)
      {
	auto arc = arcs(i);
	if (arc == nullptr)
	  continue;
	
	ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
	auto src = (Node*) arc->src_node;
	auto tgt = (Node*) arc->tgt_node;
	NODE_BITS(src).set_bit(Aleph::Spanning_Tree, true);
	NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
      }
    NODE_BITS(s).set_bit(Aleph::Spanning_Tree, true);

    assert(check_painted_arcs());

    painted = true;
  }

  bool last_relax_and_prepare_check_negative_cycle() noexcept
  {
    bool negative_cycle = false;
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      {
	auto arc = it.get_curr();
	auto src = g.get_src_node(arc);
	auto & acum_src = acum(src);
	if (acum_src == Inf)
	  continue;

	auto tgt        = g.get_tgt_node(arc);
	auto d          = dist(arc);
	auto & acum_tgt = acum(tgt);
	auto sum        = acum_src + d;
	if (sum < acum_tgt)
	  {
	    negative_cycle = true; 
	    const auto & index = idx(tgt);
	    arcs(index)       = arc;
	    acum_tgt          = sum;
	  }
      }
    return negative_cycle;
  }

  bool last_relax_and_test_negative_cycle() noexcept
  {
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      {
	auto arc = it.get_curr();
	auto src  = g.get_src_node(arc);
	auto & acum_src = acum(src);
	if (acum_src == Inf)
	  continue;

	auto tgt = g.get_tgt_node(arc);
	auto d = dist(arc);
	auto & acum_tgt = acum(tgt);
	auto sum = acum_src + d;
	if (sum < acum_tgt)
	  return true;
      }
    return false;
  }

  void link_cookies_and_free(typename GT::Node * s) noexcept
  {
    uninit<Ni>();

    // construye los caminos invertidos hacia el nodo origen s
    const auto & n = g.vsize();
    for (int i = 0; i < n; ++i)
      {
	auto arc = arcs(i);
	if (arc == nullptr)
	  continue;

	auto tgt = g.get_tgt_node(arc);
	NODE_COOKIE(tgt) = g.get_src_node(arc);
      }

    NODE_COOKIE(s) = nullptr; // por si acaso hay ciclo negativo 

    arcs.cut();
  }

public:

  /** Paint the minimal spaning tree for the a `start` node.
      
      @param[in] start source node from which the minimal path will be
      computed
      @return true if negative cycles are detected, which case the
      spanning minimal tree has note sense. Otherwise, `false` is
      returned and the minimal spanning tree is painted with the bit
      `Spanning_Tree`
  */
  bool paint_spanning_tree(Node * start) noexcept
  {
    init_with_indexes(start); 
    
    relax_arcs();
    bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    paint_tree();
    link_cookies_and_free(s);

    return negative_cycle;
  }

  /** faster minimal spaning tree painting from a `start` node.

      This methods executes a probably faster version of Bellman-Ford
      algorithm
      
      @param[in] start source node from which the minimal path will be
      computed
      @return true if negative cycles are detected, which case the
      spanning minimal tree has note sense. Otherwise, `false` is
      returned and the minimal spanning tree is painted with the bit
      `Spanning_Tree`
  */
  bool faster_paint_spanning_tree(Node * start) 
  {
    init_with_indexes(start);
    
    const auto & n = g.get_num_nodes();

    DynListQueue<typename GT::Node*> q;

    Node __sentinel;
    Node * sentinel = &__sentinel;

    put_in_queue(q, s);
    put_in_queue(q, sentinel);

    for (size_t i = 0; not q.is_empty(); )
      {
	auto src = get_from_queue(q);
	if (src == sentinel) // ¿se saca el centinela?
	  {
	    if (i++ > n) 
	      break;

	    put_in_queue(q, sentinel);
	  }
	else
	  relax_arcs(src, q);
      }

    bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    paint_tree();
    link_cookies_and_free(s);

    return negative_cycle;
  }

private:

  Node * create_dummy_node()
  {
    s = const_cast<GT&>(g).insert_node(typename GT::Node_Type());
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	auto p = it.get_curr();
	if (p == s)
	  continue;
	auto a = const_cast<GT&>(g).insert_arc(s, p);
	Distance::set_zero(a);
      }
    return s;
  }

  template <class Info_Type>  
  void remove_dummy_node(Node * p) 
  {
    delete (Info_Type*) NODE_COOKIE(p);
    const_cast<GT&>(g).remove_node(p);
  }

public:

  /** Returns true if it exists a negative cycle in any path starting
      from node `start`
  */
  bool has_negative_cycle(Node * start)
  {
    init_with_indexes(start);
    s = start;
    acum(s) = 0;
    
    relax_arcs();
    bool negative_cycle = last_relax_and_test_negative_cycle();
    uninit<Ni>();

    return negative_cycle;
  }

  bool has_negative_cycle()
  {
    create_dummy_node();
    auto ret = has_negative_cycle(s);
    remove_dummy_node<Ni>(s);

    return ret;
  }

private:

  Path<GT> search_negative_cycle_on_partial_graph()
  {
     GT aux = build_spanning_tree<GT>(arcs);

     // we map because Tarjan algorithm modifies cookies
     DynMapTree<Node*, Node*> table; 
     for (typename GT::Node_Iterator it(aux); it.has_curr(); it.next())
       {
	 auto p = it.get_curr();
	 table.insert(p, (Node*) NODE_COOKIE(p));
       }

     Path<GT> path(aux);
     if (Tarjan_Connected_Components<GT, NAit, SA>(sa).compute_cycle(aux, path))
       {
	 Path<GT> ret(g);
	 for (typename Path<GT>::Iterator it(path); it.has_current_node(); 
	      it.next())
	   ret.append_directed((Node*) table.find(it.get_current_node()));
	 return ret;
       }
     
     return Path<GT>(g);
  }

public:

  /** Search a negative cycle on the all possible paths starting from
      `start` node.

      If a negative cycle is found, the the Tarjan algorithm is executed
      for retrieving it. In this case the cyle is returned. Otherwise
      (tehere is no negative cycle), the returned path is empty.

      @param[in] start stating node
      @return a valid path corresponding to a negative cycle if this is
      found. Otherwise, an empty path
  */
  Path<GT> test_negative_cycle(Node * start)
  {
    init_with_indexes(start);

    relax_arcs();
    bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    if (not negative_cycle)
      {
	link_cookies_and_free(s);
	return Path<GT>(g);
      }

    Path<GT> ret = search_negative_cycle_on_partial_graph();
    if (ret.is_empty())
      WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
	      "a negative cycle, but Tarjan algorithm executed on partial\n"
	      "graph has not found such cycle\n\n"
	      "Be very careful, this is provably a bug");

    link_cookies_and_free(s);
    return ret;
  }

  /** Searches and returns a negative cycle (if it exists).
      
      @return a path containing a negative cycle (if it
      exists). Otherwise it returns an empty path.
   */
  Path<GT> test_negative_cycle()
  {
    auto start = create_dummy_node();
    auto ret_val = test_negative_cycle(start);
    remove_dummy_node<Ni>(start);
    return ret_val;
  }

  /** Searches a negative cycle using the faster version of Bellman-Ford
      algorithm and iteratively searching the cycle before finishing up.

      Normally, the Bellman-Ford algorith certainly detects a negative
      cycle if during an additional arcs scanning an arc is
      relaxed. However, very frequently the cycle appears in graph used
      for representing the partial spanning tree.

      This version could be seen as thus:

          threshold = it_factor*|V|;
          for (int i = 0; i < |V|; ++i)
            {
              for (e in Arcs) // for each arc e
                relax e;
              if (i >= threshold)
                {
                  // search a cycle in the graph representing the spanning tree
                  threshold += step;
                  if negative cylce is found
                    return it;
                }
            }

      So, from the threshold-th iteration, the algoritm try to find a
      negative cycle on the hope that if this exists, then the algorithm
      will finish too much before the normal completion.

      @warning The cycle searching is done respect a to a source
      node. So, if no negative cycle is found, then this not still
      conclusive ofr determining that the graph has not negative cycles.

      @param[in] start node from which the spanning tree is build.
      @param[in] it_factor iterative factor since then the negative
      cycle is searched.
      @param[in] step next step from which the next negative cycle will
      be done.
      @return a tuple whose `get<0>` value corresponds to the found
      negative cycle (if this one exists) and `get<1>` value is the
      external iteration when the cyle was found. The idea of this
      second field is to give feedback in order to eventually refine the
      `it_factor` value.
   */
       tuple<Path<GT>, size_t>
  search_negative_cycle(Node * start, double it_factor, const size_t step)
  {
    init_with_indexes(start);
    
    const auto & n = g.get_num_nodes();
    DynListQueue<typename GT::Node*> q;
    Node __sentinel;
    Node * sentinel = &__sentinel;
    put_in_queue(q, s); 
    put_in_queue(q, sentinel); 

    double threshold = it_factor*n;
    Path<GT> ret(g);

    size_t i = 0;
    while (not q.is_empty())
      {
	auto src = get_from_queue(q);
	if (src == sentinel)
	  {
	    if (i++ > n)
	      break;

	    put_in_queue(q, sentinel);
	    if (i >= threshold) // must I search negative cycles?
	      {
		ret = search_negative_cycle_on_partial_graph();
		if (not ret.is_empty()) // negative cycle found?
		  {
		    link_cookies_and_free(s);
		    return make_tuple(std::forward<Path<GT>>(ret), i);
		  }
		threshold += step;
	      }
	  }
	else
	  relax_arcs(src, q);
      }

    bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    if (negative_cycle)
      {
	ret = search_negative_cycle_on_partial_graph();
	if (ret.is_empty())
	  WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
		  "a negative cycle, but Tarjan algorithm executed on partial\n"
		  "graph has not found such cycle\n\n"
		  "Be very careful, this provably is a bug");
      }

    link_cookies_and_free(s);
    return make_tuple(std::forward<Path<GT>>(ret), i);
  }

  /** Searches a negative cycle using the faster version of Bellman-Ford
      algorithm.

      @param[in] start node from which the spanning tree is build.
      @param[in] it_factor iterative factor since then the negative
      cycle is searched.
      @param[in] step next step from which the next negative cycle will
      be done.
      @return a tuple whose `get<0>` value corresponds to the found
      negative cycle (if this one exists) and `get<1>` value is the
      external iteration when the cyle was found. The idea of this
      second field is to give feedback in order to eventually refine the
      `it_factor` value.
   */
  Path<GT> search_negative_cycle(Node * start)
  {
    init_with_indexes(start);
    
    const auto & n = g.get_num_nodes();
    DynListQueue<typename GT::Node*> q;
    Node __sentinel;
    Node * sentinel = &__sentinel;
    put_in_queue(q, s); 
    put_in_queue(q, sentinel); 

    Path<GT> ret(g);
    for (size_t i = 0; not q.is_empty(); /* nothing */)
      {
	auto src = get_from_queue(q);
	if (src == sentinel)
	  {
	    if (i++ > n)
	      break;

	    put_in_queue(q, sentinel);
	  }
	else
	  relax_arcs(src, q);
      }

    bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    if (negative_cycle)
      {
	ret = search_negative_cycle_on_partial_graph();
	if (ret.is_empty())
	  WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
		  "a negative cycle, but Tarjan algorithm executed on partial\n"
		  "graph has not found such cycle\n\n"
		  "Be very careful, this provably is a bug");
      }

    link_cookies_and_free(s);

    return ret;
  }

  /** Searches a negative cycle using the faster version of Bellman-Ford
      algorithm and iteratively searching the cycle before finishing up.

      \overload search_negative_cycle(Node * start, double it_factor, const size_t step)

      This overloaded version searches for negatives cycles in all the
      graph. For that, a dummy and temporal node is inserted into the
      graph and the search is perfomed using the dummy node as starting
      node. 
   */
  tuple<Path<GT>, size_t> search_negative_cycle(double it_factor,
						const size_t step)
  {
    auto start = create_dummy_node();
    auto ret_val = search_negative_cycle(start, it_factor, step);
    remove_dummy_node<Ni>(start);
    return ret_val;
  }

   /**
      \overload search_negative_cycle(Node * start)
   */
  Path<GT> search_negative_cycle()
  {
    auto start = create_dummy_node();
    auto ret_val = search_negative_cycle(start);
    remove_dummy_node<Ni>(start);
    return ret_val;
  }
  

  /** Extract the minimal spanning tree in a compressed form.

      On a previously painted minimal spanning tree of shortest paths
      starting on source node, this method extract the tree and loads it
      in an array of arcs.

      @return an array of arcs belonging to the minimal spanning tree.
      @throw domain_error if the minimal spanning tree has not
      previously built
  */
  DynArray<Arc*> extract_mim_spanning_tree()
  {
    return arcs;
  }

  /** Extract from the graph a previously painted minimal spanning tree
      of minimal paths.

      This method requires that a minimal spanning tree has been
      executed with a `paint()` method.

      @param[in] tree a graph where the minimal spanning tree will be
      put.
      @param[in] with_map if true the the extracted tree is mapped
      through the cookies with the graph.
   */
  void build_tree(GT & tree, bool with_map = true)
  { 
    if (not painted and with_map)
      throw std::domain_error("Spanning tree has not been painted");

    clear_graph(tree);

    DynMapTree<Node*, Node*> table;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	auto gp = it.get_curr();
	auto tp = tree.insert_node(gp->get_info());
	table.insert(gp, tp);
      }

    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	auto gtgt = it.get_curr();
	auto gsrc = (Node*) NODE_COOKIE(gtgt);
	if (gsrc == nullptr)
	  continue; // Se trata del nodo origen del árbol abarcador
	
	auto garc_ptr = g.arcs(gsrc).find_ptr([gsrc, gtgt] (Arc * a)
		{
		  return a->src_node == gsrc and a->tgt_node == gtgt;
		});
	assert(garc_ptr and IS_ARC_VISITED(*garc_ptr, Aleph::Spanning_Tree));
	auto garc = *garc_ptr;
	assert(garc->src_node == gsrc and garc->tgt_node == gtgt);
	auto tsrc_ptr = table.search(gsrc);
	auto ttgt_ptr = table.search(gtgt);
	assert(tsrc_ptr and ttgt_ptr);
	auto tarc = tree.insert_arc(tsrc_ptr->second, ttgt_ptr->second, 
				    garc->get_info());
	if (with_map)
	  GT::map_arcs(garc, tarc);
      }

    if (with_map)
      table.for_each([] (const auto & p) { GT::map_nodes(p.first, p.second); });
  }
   
  bool test_negative_cycle(typename GT::Node * s, Path<GT> & cycle)
  {
    cycle = test_negative_cycle(s);
    return not cycle.is_empty();
  }

  bool test_negative_cycle(Path<GT> & cycle)
  {
    cycle = test_negative_cycle();
    return not cycle.is_empty();
  }

  Distance_Type get_min_path(typename GT::Node * end, Path<GT> & path)
  {
    if (not painted)
      throw std::domain_error("Graph has not previously painted");
      
    return Aleph::get_min_path<GT, Distance>(s, end, path);
  }

 /** Retorna en el mapeo avl los pesos asociados a cada nodo obtenidos
      luego de ejecutar el algoritmo de Bellman-Ford a partir de un nodo
      ficticio. Esta rutina está concebida específicamente para el
      algoritmo de Jhonson de cálculo de todos los caminos mínimos a
      partir de un nodo dado.

      @param[in] m un mapeo avl dinámico de la forma Node*-->w de
      puntero a nodo a peso asociado al nodo.
      @return true si el grafo no tiene un ciclo negativo, en cuyo caso
      los valores de en m pueden usarse para el algoritmo de
      Jhonson. Retorna false si se detecta un ciclo negativo.
  */
  DynMapTree<Node*, Distance_Type> compute_nodes_weights()
  {
    create_dummy_node();

    init_with_indexes(s);
    
    const auto & n = g.get_num_nodes();

    DynListQueue<typename GT::Node*> q;

    Node __sentinel;
    Node * sentinel = &__sentinel;

    put_in_queue(q, s);
    put_in_queue(q, sentinel);

    bool negative_cycle = false;
    for (int i = 0; not q.is_empty() and not negative_cycle; /* nada */)
      {
	auto src = get_from_queue(q); 
	if (src == sentinel) // ¿se saca el centinela?
	  {
	    if (i++ > n) 
	      break;
	    else
	      put_in_queue(q, sentinel);
	  }
	else
	  relax_arcs(src, q);
      }

    negative_cycle = last_relax_and_prepare_check_negative_cycle();
    remove_dummy_node<Ni>(s);

    DynMapTree<Node*, long> ret;

    // build mapping if there are no negative cycles
    if (not negative_cycle)
      for (auto it = g.get_node_it(); it.has_curr(); it.next())
	{
	  auto p = it.get_curr();
	  ret.insert(p, acum(p));
	}

    uninit<Ni>();
    arcs.cut();

    if (negative_cycle)
      throw domain_error("negative cycles detected");

    return ret;
  }
};

      /** Detecta si existe un ciclo negativo y eventualmente lo calcula.

          Esta clase toma un digrafo y un nodo destino, ejecuta el
          algoritmo de Bellman-Ford y si se detecta un ciclo negativo
          entonces lo almacena en un camino.

          El procedimiento es parametrizado con las siguientes
          especificaciones:
          -# GT: el tipo de grafo basado en List_Graph.
          -# Distance<GT>: La clase de lectura del peso del arco  que debe
          exportar los siguientes atributos:
            -# typedef Distance<GT>::Distance_Type: el tipo de dato que
            representa un peso en un arco.
            -# Distance<GT>::Distance_Type operator()(typename GT::Arc *a):
            que retorna el valor del peso en el arco a.
            .
          -# SA: filtro de arcos.

          @ingroup Grafos
       */
 template <class GT, 
	   class Distance                     = Dft_Dist<GT>, 
	   template <class, class> class Ait  = Arc_Iterator,
	   template <class, class> class NAit = Node_Arc_Iterator,
           class SA                           = Dft_Show_Arc<GT>> 
struct Bellman_Ford_Negative_Cycle
{ 
  /** Invoca a la detección y cálculo de ciclo negativo.

      @param[in] g el grafo.
      @param[out] path el camino donde se guarda el ciclo en caso de
      que se detecte ciclo negativo.
      @return true si existe un ciclo negativo; false de lo
      contrario.
      @throw bad_alloc si no hay suficiente memoria.
   */
  bool operator () (const GT & g, Path<GT> & path, 
		    Distance & d, SA & sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(path);
  }

  bool operator () (const GT & g, Path<GT> & path, 
		    Distance && d = Distance(), SA && sa = SA()) const
  {
    return Bellman_Ford<GT,Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(path);
  }  

  /** Invoca a la detección y cálculo de ciclo negativo.

      @param[in] g el grafo.
      @param[in] s el nodo inicio.
      @param[out] path el camino donde se guarda el ciclo en caso de
      que se detecte ciclo negativo.
      @return true si existe un ciclo negativo; false de lo
      contrario.
      @throw bad_alloc si no hay suficiente memoria.
   */
  bool operator () (const GT & g, typename GT::Node * s,
		    Path<GT> & path, Distance & d, SA & sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(s, path);
  }

  bool operator () (const GT & g, typename GT::Node * s,
		    Path<GT> & path, Distance && d = Distance(), 
		    SA && sa = SA()) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      test_negative_cycle(s, path);
  }

  Path<GT> operator () (const GT & g, typename GT::Node * s, 
			Distance & d, SA & sa, double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      search_negative_cycle(s, it_factor);
  }

  Path<GT> operator () (const GT & g, typename GT::Node * s, 
			Distance && d = Distance(), SA && sa = SA(), 
			double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
      search_negative_cycle(s, it_factor);
  }

  Path<GT> operator () (const GT & g, Distance & d, SA & sa, 
			double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
    search_negative_cycle(it_factor);
  }

  Path<GT> 
  operator () (const GT & g, Distance && d = Distance(), SA && sa = SA(), 
	       double it_factor = 0.7) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
    search_negative_cycle(it_factor);
  }
};


# undef NI
# undef IDX
# undef ACU
} // end namespace Aleph

# endif // BELLMAN_FORD_H
