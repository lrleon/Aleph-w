# ifndef TPL_TEST_CONNECTIVITY_H
# define TPL_TEST_CONNECTIVITY_H

# include <tpl_graph_utils.H>


       /** Determina si un grafo g es conexo.

           Esta clase realiza una prueba de conectividad del grafo g. La
           prueba apela a un recorrido en profundidad.

           La clase verifica la
           cantidad de arcos. Si esta cantidad es menor que el número de
           nodos, entonces el grafo se considera inconexo.

           La función toma dos parámetros tipo:
           -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
           -# SA: clase que se encarga de mostrar el arco. Internamente, la
           función emplea el iterador filtro Node_Arc_Iterator (basado
           en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
           el la clase que determina si el arco debe o no mostrarse al
           recorrido. 

           @ingroup Grafos
           @see depth_first_traversal() is_reachable()
        */
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Test_Connectivity
{
public:

        /** Invoca a la prueba de conectividad.

	    Esta versión se emplea cuando se requiera que el filtro de arcos
	    mantenga un estado.

            @param[in] g el grafo o digrafo a verificar.
	    \param[in] sa el filtro de arcos del grafo.
            @return true si el grafo es conexo; false de lo contrario.
            @note Debido a la prueba con el número de arcos, esta función
            es incorrecta para multigrafos.
            @throw domain_error si la rutina es invocada sobre un digrafo.
         */
  bool operator () (GT & g, SA && sa = SA()) const
  {
    if (g.is_digraph()) // sólo es válida para grafos, no digrafos
      throw std::domain_error("test_connectivity() does not work on digraphs");

    if (g.get_num_arcs() < g.get_num_nodes() - 1) 
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);
 
    return traversal(g) == g.get_num_nodes();
  }

/** Invoca a la prueba de conectividad.

	    Esta versión se emplea cuando se requiera que el filtro de arcos
	    mantenga un estado.

            @param[in] g el grafo o digrafo a verificar.
	    \param[in] sa el filtro de arcos del grafo.
            @return true si el grafo es conexo; false de lo contrario.
            @note Debido a la prueba con el número de arcos, esta función
            es incorrecta para multigrafos.
            @throw domain_error si la rutina es invocada sobre un digrafo.
         */
  bool operator () (GT & g, SA & sa) const
  {
    if (g.is_digraph()) // sólo es válida para grafos, no digrafos
      throw std::domain_error("test_connectivity() does not work on digraphs");

    if (g.get_num_arcs() < g.get_num_nodes() - 1) 
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);
 
    return traversal(g) == g.get_num_nodes();
  }
};



# endif // TPL_TEST_CONNECTIVITY_H
