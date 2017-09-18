# ifndef TPL_CUT_NODES_H
# define TPL_CUT_NODES_H

# include <tpl_graph_utils.H>

namespace Aleph {


      /** Cálculo de los puntos de corte de un grafo.

          La clase Compute_Cut_Nodes se encarga de calcular todolo
          relacionado a los nodes de corte de un grafo conexo. Para ello
          efectúa un recorrido en profundidad a partir de un nodo dado y
          añade a una lista dinámica cada nodo de corte que contenga el
          grafo.

	  La lista dinámica es provista por el usuario.

          Un nodo de corte se define como un nodo tal que al suprimirlo
          (juntos con sus arcos) el grafo deviene inconexo.

          El algoritmo utiliza el bit Depth_First para marcar los nodos y 
          arcos que han sido visitados. También, para construir copias
          mapeadas de componentes conexos en torno a un punto de corte,
          de emplea el bit Build_Subtree.

          Esta clase también provee métodos para "pintar" los diversos
          bloques que separarían los puntos de corte, así como para
          obtener copias mapeadas de los componentes conexos en torno a
          los puntos de corte, el grafo de corte y los arcos de cruce.

          Se asume que g es conexo y no se realiza ninguna verificación al
          respecto.  

	  Los parámetros tipo de la clase son:
	  - GT: el grafo.
	  - SA: el filtro de arcos para los iteradores de arcos. 

	  \note Si se requiere calcular el grafo de corte, mediante el
	  método map_cut_graph(), entonces la clase filtro de arcos
	  debe sobrecargar el operador () (GT&, GT::Arc*).

	  Esta clase exporta varias operaciones con dependencia
	  secuencial:
	  -# cut_nodes(): la cual calcula los nodos de corte y los
	  introcuce en una lista. El operador () sobre la clase es
	  sinónimo de este método.
	  -# paint_subgraphs(): "pinta" los distintos componentes del
	  grafo en torno a los puntos de corte previamente
	  calculados. La rutina retorna la cantidad de colores, uno por
	  cada componente, que tenga el grafo. Los colores son colocados
	  en los contadores asociados por cada nodo.
	  -# map_subgraph(): obtiene una copia mapeada de un componente
	  conexo asociado a un color dado.
	  -# map_cut_graph(); obtiene una copia mapeada del "grafo de
	  corte"; o sea, aquel sólo conformado por los nodos de corte
	  del grafo. Este método también calcula los arcos de cruce (los
	  que conectan a un punto de corte con un bloque conexo).

	  En caso de que se desea ejecutar todo a la vez, se provee el
	  método compute_blocks(), el cual calcula los puntos de corte,
	  pinta los componentes y calcula las copias mapeadas de los
	  componentes conexos así como los arcos de cruce.

          @ingroup Grafos
       */
      template <class GT, class SA = Dft_Show_Arc<GT> >
class Compute_Cut_Nodes
{
  SA &                            sa;
  GT *                            gptr;
  DynDlist<typename GT::Node *> * list_ptr;
  long                            curr_df;
  long                            curr_color;

  enum State { Init, Cut_Nodes_Computed, Painted } state;

  void cut_nodes(typename GT::Node * p, typename GT::Arc * a)
  {
    NODE_BITS(p).set_bit(Depth_First, true); // pinte p visitado
    low <GT> (p) = df <GT> (p) = curr_df++;  // asígnele df

        // recorrer arcos de p mientras no se abarque a g
    bool p_is_cut_node = false;
    for (Node_Arc_Iterator <GT, SA> i(p, sa); i.has_curr(); i.next())
      {
        auto arc = i.get_current_arc();
        if (arc == a) 
          continue; // a es el padre de arc ==> ignorarlo

        auto tgt = i.get_tgt_node();
        if (IS_NODE_VISITED(tgt, Depth_First)) 
          { 
            if (not IS_ARC_VISITED(arc, Depth_First)) // no abarcador?
	      low<GT>(p) = std::min(df<GT>(tgt), low<GT>(p));
            continue;
          }

        if (IS_ARC_VISITED(arc, Depth_First)) 
          continue;

        ARC_BITS(arc).set_bit(Depth_First, true); // marque arco

        cut_nodes(tgt, arc);
	low<GT>(p) = std::min(low<GT>(tgt), low<GT>(p));
        if (low<GT>(tgt) >= df<GT>(p) and df<GT>(tgt) != 0) // ¿de corte?
          p_is_cut_node = true;
      }

        // aquí, p ya fue explorado recursivamente
    if (p_is_cut_node)
      {
        NODE_BITS(p).set_bit(Cut, true);
        list_ptr->append(p);
      }
  }

public:

      /** Calcula los puntos de corte.
	
	  @param[in] start nodo inicio desde donde se desea iniciar el
          cálculo. 
          @param[out] list lista dinámica donde se guardan punteros a nodos 
          de corte del grafo.
          @throw bad_alloc si no hay memoria para insertar en la lista
          dinámica o si no hay memoria para los datos de cálculo interno
          del algoritmo.

	  La lista debe ser provista por el usario y se vacía al inicio
	  del cálculo. 
     */
  void cut_nodes(typename GT::Node *             start, 
		 DynDlist<typename GT::Node *> & list)
  {
    curr_df  = 0; // contador global de visitas 
    list_ptr = &list;

    list_ptr->empty();

    gptr->for_each_node([] (auto p) // init the nodes
			{
			  NODE_COUNTER(p) = 0;
			  NODE_BITS(p).reset();
			  low<GT>(p) = -1;
			});
    gptr->reset_arcs();

    NODE_BITS(start).set_bit(Depth_First, true); // marcar start
    df <GT> (start) = curr_df++;

    int call_counter = 0; // contador de llamadas recursivas

        // Recorra los arcos de start mientras g no haya sido abarcado
    for (Node_Arc_Iterator<GT, SA> it(start, sa); 
         it.has_curr() and curr_df < gptr->get_num_nodes(); it.next())
      {
	auto tgt = it.get_tgt_node();
	if (IS_NODE_VISITED(tgt, Depth_First)) 
	  continue; 

	auto arc = it.get_current_arc();
	if (IS_ARC_VISITED(arc, Depth_First)) 
	  continue;

	ARC_BITS(arc).set_bit(Depth_First, true);
	cut_nodes(tgt, arc); 
	++call_counter;
      }  

    if (call_counter > 1) // ¿es la raíz un punto de articulación?
      {    // sí == métala en la lista
        NODE_BITS(start).set_bit(Cut, true);
        list_ptr->append(start);
      }

    state = Cut_Nodes_Computed;
  }

private:

  void paint_subgraph(typename GT::Node * p)
  {
    assert(not is_a_cut_node <GT> (p));

    if (is_node_painted <GT> (p)) 
      return; 

    paint_node <GT> (p, curr_color);

    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next())
      {
	auto arc = it.get_current_arc();
	if (is_arc_painted <GT> (arc))
          continue;

	auto tgt = it.get_tgt_node();
	if (is_a_cut_node <GT> (tgt))
	  continue;

	paint_arc<GT>(arc, curr_color); 
	paint_subgraph(tgt); 
      }
  }

  void paint_from_cut_node(typename GT::Node * p)
  {
    assert(is_a_cut_node <GT> (p));

        // pintar recursivamente con dif colores bloques conectados a p
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next())
      {
	auto arc = it.get_current_arc();

	assert(not is_arc_painted <GT> (arc));

	auto tgt_node = it.get_tgt_node();
	if (is_a_cut_node <GT> (tgt_node)) // ¿ es un arco de corte?
	  {
	    ARC_BITS(arc).set_bit(Cut, true); // marque como de corte
	    continue; // avance a próximo arco
	  }
	else 
	  {
	    paint_arc <GT> (arc, Cross_Arc); // marque como de cruce
	    if (is_node_painted <GT> (tgt_node)) 
	      continue; 
	  }

	    // pintar recursivamente nodo conectado a arc
	paint_subgraph(tgt_node);

	curr_color++; // cambiar color (sig arco en otro bloque)

	assert(not is_arc_painted <GT> (arc));
      }
  }

  typename GT::Node * create_and_map_node(typename GT::Node * gp, 
					  const long &        color,
					  GT &                sg)
  {
    assert(get_color<GT>(gp) == color);
    assert(not IS_NODE_VISITED(gp, Build_Subtree));

    unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
    sg.insert_node(tp_auto.get());
    GT::map_nodes(gp, tp_auto.get());
    NODE_BITS(gp).set_bit(Build_Subtree, true); 

    return tp_auto.release();
  }

  void map_subgraph(GT & sg, typename GT::Node * gsrc, const long & color)
  {
    assert(get_color <GT> (gsrc) == color);

    auto tsrc = mapped_node<GT>(gsrc); // gsrc en sg

        // recorrer arcos de gsrc y añadir a sg los del color de interés
    for (Node_Arc_Iterator <GT, SA> i(gsrc, sa); i.has_curr(); i.next())
      {
        auto garc = i.get_current_arc();
        if (get_color<GT>(garc) != color or IS_ARC_VISITED(garc, Build_Subtree))
          continue; // arco es de otro color o ya está visitado 

        ARC_BITS(garc).set_bit(Build_Subtree, true); 

        auto gtgt = i.get_tgt_node(); 

        assert(get_color <GT> (gtgt) == color);

        typename GT::Node * ttgt = nullptr; // imagen gtgt en sg
        if (IS_NODE_VISITED(gtgt, Build_Subtree)) // ¿gtgt en sg?
          ttgt = mapped_node<GT> (gtgt);
        else
	  ttgt = create_and_map_node(gtgt, color, sg);

        auto tarc = sg.insert_arc(tsrc, ttgt, garc->get_info());
        GT::map_arcs(garc, tarc);

        map_subgraph(sg, gtgt, color);
      }
  }
  
public:

      /** Constructor de calculador de nodos de corte.

	  @param[in] g el grafo al cual se desea calcular los puntos de
	  corte.
	  @param[in] __sa el filtro de arcos para los oiteradores sobre
	  arcos. 
       */
  Compute_Cut_Nodes(const GT & g, SA && __sa = SA()) 
    : sa(__sa), gptr(&const_cast<GT&>(g)), state(Init)
  {
    /* empty */ 
  }

  Compute_Cut_Nodes(const GT & g, SA & __sa) 
    : sa(__sa), gptr(&const_cast<GT&>(g))
  {
    /* empty */ 
  }

      /// \overload cut_nodes()
  void operator () (DynDlist<typename GT::Node *> & list)
  {
    cut_nodes(gptr->get_first_node(), list);
  }

      /// \overload cut_nodes()
  void operator () (typename GT::Node *             start,
		    DynDlist<typename GT::Node *> & list)
  {
    cut_nodes(start, list);
  }
  
      /** Pinta los componentes conexos en torno a los puntos de corte.

  	  paint_subgraphs() pinta los componentes conexos en torno a los
  	  puntos de corte de un grafo.

	  La rutina requiere que ante se haya invocado al método cut_nodes().

	  Los "colores" de cada componente son colocados en atributo
	  NODE_COUNTER de cada nodo y arco. El primer color comienza en
	  el valor 1.

	  Úsese esta rutina si no es necesario o es suficiente con
	  obtener los componentes mediante los colores de los
	  bloques. Esto ahorra el cálculo y la memoria de tener que crear
	  nuevos grafos. 

	  @return la cantidad de componentes; que es la cantidad de colores.
	  @throw std::logic_error si el cálculo de los puntos de corte
	  no ha sido previamente invocado.
       */
  long paint_subgraphs()
  {
    if (state != Cut_Nodes_Computed)
      throw std::logic_error("Cut nodos has not been computed or the class is"
			     " another phase");

    gptr->reset_counter_nodes();
    gptr->reset_counter_arcs();
    curr_color = 1;

        // Recorrer cada nodo de corte y pintar sus bloques
    for (typename DynDlist<typename GT::Node*>::Iterator i(*list_ptr); 
	 i.has_curr(); i.next())
      paint_from_cut_node(i.get_curr());

    state = Painted;

    return curr_color;
  }

      /** Obtiene una copia mapeada del componente de color.

	  map_subgraph() extrae del grafo el componente con color color
	  y construye una grafo mapeado en sg.

	  \param[out] sg el grafo donde se desea obtener la copia mapeada.
	  \param[in] el color que se desea extraer.
	  \throw logic_error si el grafo no está pintado.
	  \throw domain_error si no hay ningún componente con el color
	  dado. 
      */
  void map_subgraph(GT & sg, const long & color)
  {
    if (state != Painted)
      throw std::logic_error("Graph is not painted");

    clear_graph(sg);

    typename GT::Node * first = nullptr; // busque primer nodo con color

    for (typename GT::Node_Iterator it(*gptr); it.has_current(); it.next())
      if (get_color <GT> (it.get_current_node()) == color)
        first = it.get_current_node();

    if (first == nullptr) // Encontró el color?
      throw std::domain_error("Color does not exist in the graph");

        // cree first, insértelo en sg y mapéelo
    create_and_map_node(first, color, sg);
    try
      {    
	map_subgraph (sg, first, color); // mapee first
      }
    catch (...)
      {
        clear_graph(sg);
      }
  }

      /** Calcula el grafo mapeado de corte de un grafo.

	 El grafo de corte de corte de un grafo es aquel sólo compuesto
	 por los nodos de corte.

	 También se calculan los arcos de cruce; es decir, aquellos que
	 conectan a puntos de corte con componentes conexos.

	 La idea de este algoritmo es que usado en combinación con
	 map_subgraph() se puedan obtener desde la copias mapeadas el
	 grafo original.

	 @param[out] cut_graph el grafo de corte
	 @param[out] cross_arc_list la lista de arcos de cruce; es
	 decir, los arcos que van desde un nodo de corte hacia un
	 componente conoxo. Estos arcos no pertenecen al grafo de corte.

	 \throw logic_error si el grafo no ha sido previamente pintado.
       */
  void map_cut_graph(GT &                          cut_graph, 
		     DynDlist<typename GT::Arc*> & cross_arc_list)
  {
    if (state != Painted)
      throw std::logic_error("Graph is not painted");

    clear_graph(cut_graph);

        // recorra lista de nodos de corte e insértelos en cut_graph
    for (typename DynDlist<typename GT::Node*>::Iterator it(*list_ptr);
	 it.has_curr(); it.next())
      {
	auto gp = it.get_current();

	assert(is_a_cut_node <GT> (gp));

	unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
	cut_graph.insert_node(tp_auto.get());
	GT::map_nodes(gp, tp_auto.release());
      }

        // recorra arcos de g ==> cut_graph = {arcos no corte} U 
        // cross_arc_list = {arcos de cruce}
    for (Arc_Iterator <GT, SA> it(*gptr, sa); it.has_curr(); it.next())
      {
	auto garc = it.get_current_arc();
	if (is_a_cross_arc <GT> (garc))
	  {
	    cross_arc_list.append(garc); 
	    continue;
	  }

	if (not is_an_cut_arc <GT> (garc)) 
	  continue;

	typename GT::Node * src = mapped_node<GT>(gptr->get_src_node(garc));
	typename GT::Node * tgt = mapped_node<GT>(gptr->get_tgt_node(garc));

	assert(src != nullptr and tgt != nullptr);

	typename GT::Arc * arc = 
	  cut_graph.insert_arc(src, tgt, garc->get_info());
	GT::map_arcs(garc, arc);
      }
  }

      /** Construye grafos mapeados en torno a los puntos de corte de un
	  grafo.

	  Esta rutina toma una lista de puntos de corte previamente
	  calculada y construye en una sola pasada sobre los nodos
	  copias mapeadas de los componentes conexos en torno a los
	  puntos de corte así como el grafo de corte. Luego, se realiza
	  una pasada sobre los arcos en la cual se determinan los arcos
	  de cruce.

	  Si se requieren calcular todos los grafos mapeados y el grafo
	  de corte, entonces el uso de esta rutina es mucho más
	  eficiente que llamadas separadas a map_subgraph() y map_cut_graph().

	  \param[out] block_list lista de grafos donde se guardarán las
	  copias mapeadas de los componentes conexos
	  \param[out] cut_graph el grafo mapeado de corte
	  \param[out] lista de arcos de cruce.
	  \throw bad_alloc si no hay suficiente memoria.
	  \throw logic_error si los nodos de corte no han sido
	  previamente calculados.
  */
  void compute_blocks(DynDlist<GT> &                block_list, 
		      GT &                          cut_graph, 
		      DynDlist<typename GT::Arc*> & cross_arc_list)
  {
    if (state < Cut_Nodes_Computed)
      throw std::logic_error("Cut nodes have not been computed");

    if (state == Cut_Nodes_Computed)
      paint_subgraphs();

    const long & num_colors = curr_color;
    
    DynArray<GT*> blocks; // bloques en un arreglo para rápido
			  // acceso. Que esté o no vacío indica si ha
			  // sido o no procesado.
    blocks.reserve(num_colors);

        // crear lista de componentes vacíos ordenador por color i
    for (int i = 0; i < num_colors; ++i)
      blocks.access(i) = &block_list.append(GT());

        // Recorrer los nodos y copiar y mapear según color
    for (typename GT::Node_Iterator it(*gptr); it.has_curr(); it.next())
      {
	auto p = it.get_curr();
	if (IS_NODE_VISITED(p, Build_Subtree))
	  continue;

	if (is_a_cut_node <GT> (p))
	  continue;

	const long color = get_color<GT>(p);

	GT & sg = *blocks.access(color - 1);

	create_and_map_node(p, color, sg);

	map_subgraph(sg, p, color);	
      }

    map_cut_graph(cut_graph, cross_arc_list);
  }
};

} // end namespace Aleph

# endif // TPL_CUT_NODES_H
