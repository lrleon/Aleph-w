# ifndef TPL_PATH_UTILS_H  
# define TPL_PATH_UTILS_H

# include <tpl_graph_utils.H>
# include <tpl_dynDlist.H>
# include <Prim.H>
using namespace Aleph;

namespace Aleph{
  
  template <class GT> inline static
  bool test_eulerian_Path(GT& g){
    
    if(not test_connectivity(g))
      return false;
    
    for(typename GT::Node_Iterator itor(g) ;itor.has_current();itor.next())
      if((itor.get_current_node()->get_num_arcs())%2){
	return false;
      } 
    return true;
  } 
  
  
  
# define VISITADOS(p) (*static_cast<int*>(NODE_COOKIE(p)))

template <class GT> inline static
bool __find_eulerian_tour(GT& g, 
			  typename GT::Node * src_node, 
			  typename GT::Node * current_node,
			  long & node_counter,
			  long & arc_counter,
			  void (*visit)(GT &, typename GT::Node *, 
					typename GT::Arc *), 
			  typename DynDlist<typename GT::Node*>::Iterator & itor_list_path)
    
{
  //cout<<"Pasó"<<endl;
  // verificar si se alcanza nodo origen
  if (src_node == current_node){
    //lista.append(current_node);
    itor_list_path.insert(current_node);
    itor_list_path.next();
    return true; // ciclo detectado

  } 
  
  if (IS_NODE_VISITED(current_node,0))
    return false; // Ya se visito este nodo
  
  NODE_COUNTER(current_node) = ++node_counter;
  NODE_BITS(current_node).set_bit(0, true);
  if (visit not_eq nullptr){
    // (*visit)(g, current_node, nullptr);
    //lista.append(current_node);
    //itor_list_path.next();
    itor_list_path.insert(current_node);
    itor_list_path.next();
    
  }
  // busque los caminos desde current_node a ver si llega a src_node 
  for (typename GT::Node::Iterator itor(g, current_node); 
       itor.has_current(); itor.next())
    {
      typename GT::Arc * arc = itor.get_current_arc();
      //(*visit)(g, current_node, nullptr);
      //(*visit)(g, arc->get_connected_node(current_node), nullptr);
      //cout<<endl<<endl;
      // verificar si se ha pasado por el arco 
      if (IS_ARC_VISITED(arc,0))
	continue; // ya pasamos por aqui. Ir al proximo 
      
      ARC_COUNTER(arc) = ++arc_counter;
      ARC_BITS(arc).set_bit(0, true);

      if(NODE_COOKIE(current_node)==nullptr)
	NODE_COOKIE(current_node) = new int(1);
      else
	NODE_COOKIE(current_node) = new int((*static_cast<int*>(NODE_COOKIE(current_node)))+1);
      
      if(NODE_COOKIE(arc->get_connected_node(current_node))==nullptr)
	NODE_COOKIE(arc->get_connected_node(current_node)) = new int(1);
      else
	NODE_COOKIE(arc->get_connected_node(current_node)) = new int((*static_cast<int*>(NODE_COOKIE(arc->get_connected_node(current_node))))+1);

      
      //cout<<VISITADOS(current_node)<<endl;
      //cout<<VISITADOS(arc->get_connected_node(current_node))<<endl;
      //      NODE_COOKIE(arc->get_connected_node(current_node)) = new int(NODE_COOKIE(arc->get_connected_node(current_node))) + 1;
      
      if (__find_eulerian_tour<GT>(g, src_node, 
				   arc->get_connected_node(current_node),
				   node_counter, arc_counter, visit, itor_list_path))
	return true; // ciclo encontrado desde el arco actual 
    }
  
  // todos los caminos desde current_node fueron recorridos sin
  // encontrar src_node ==> no existe ciclo 
  return false; 
}
  
  
  
  /*determina si existe un ciclo a partir de src_node */

  template <class GT> inline 
  bool find_eulerian_tour
  (GT& g, 
   typename GT::Node * start_node,
   void (*visit)(GT &, typename GT::Node *, typename GT::Arc *),
   DynDlist<typename GT::Node*> & lista)
  {
    typename GT::Node * aux_start_node = start_node;
    
    if(not test_eulerian_Path(g)){
      return false;
    }
    g.node_in_graph(start_node);
  
    g.reset_bit_nodes(0);
    g.reset_bit_arcs(0);
  
    long node_counter = No_Visited;
    long arc_counter  = No_Visited;
    //DynDlist<typename GT::Node*> lista1;
    lista.append(aux_start_node);
    typename DynDlist<typename GT::Node*>::Iterator itor_list_path(lista);
    //itor_list_path.next();
    
    do{
      g.reset_bit_nodes(0);

      /*cout<<"******";
      (*visit)(g, aux_start_node, nullptr);
      cout<<"******"<<endl;*/

      for (typename GT::Node::Iterator itor(g, aux_start_node); 
	   itor.has_current(); itor.next())
	{
	  typename GT::Arc * arc = itor.get_current_arc();
	  
	  if (IS_ARC_VISITED(arc,0)){
	    /*cout<<"arco de";
	    (*visit)(g, aux_start_node, nullptr);
	    cout<<" visitado"<<endl;*/
	    continue;
	  }
	  /*cout<<"arco conectado a start node";
	  (*visit)(g, arc->get_connected_node(aux_start_node), nullptr);
	  cout<<"a visitar"<<endl;*/
	
	  ARC_COUNTER(arc) = ++arc_counter;
	  ARC_BITS(arc).set_bit(0, true);

	  if(NODE_COOKIE(aux_start_node)==nullptr)
	    NODE_COOKIE(aux_start_node) = new int(1);
	  else
	    NODE_COOKIE(aux_start_node) = new int((*static_cast<int*>(NODE_COOKIE(aux_start_node)))+1);
	  
	  if(NODE_COOKIE(arc->get_connected_node(aux_start_node))==nullptr)
	    NODE_COOKIE(arc->get_connected_node(aux_start_node)) = new int(1);
	  else
	    NODE_COOKIE(arc->get_connected_node(aux_start_node)) = new int((*static_cast<int*>(NODE_COOKIE(arc->get_connected_node(aux_start_node))))+1);
	  
	  if (__find_eulerian_tour<GT>(g, aux_start_node, arc->get_connected_node(aux_start_node),
				       node_counter, arc_counter, visit, itor_list_path)){
	  //return true;
	    break;
	  }
	}

      
      typename DynDlist<typename GT::Node*>::Iterator itor_list(lista); 
      for (; itor_list.has_current(); itor_list.next()){
	if(VISITADOS(itor_list.get_current())<itor_list.get_current()->get_num_arcs()){
	  aux_start_node=itor_list.get_current();
	  break;
	}
      }
      itor_list_path=itor_list;
    }while(arc_counter < g.get_num_arcs());
    // todos los caminos desde src_node fueron recorridos sin
    // encontrar de nuevo a src_node ==> no existe ciclo 
    for (typename GT::Node_Iterator itor(g); itor.has_current(); itor.next())
       {
	 typename GT::Node * node = itor.get_current_node();
	 delete (int*) NODE_COOKIE(node);    
       }
    return true;
  }


  template <class GT> inline
  void imprimir_grafo(GT& g)
  {
    cout << endl
	 << "Total de Nodos: (" << g.get_num_nodes() << ")" << endl;
    
    for (typename GT::Node_Iterator node_itor(g); node_itor.has_current(); 
	 node_itor.next())
      cout << INDENT << node_itor.get_current_node()->valor;
    
    
    cout << endl
	 << endl 
	 << "Total de Arcos: (" << g.get_num_arcs() << ")" << endl;
    
    for (typename GT::Arc_Iterator arc_itor(g); arc_itor.has_current();
	 arc_itor.next())
      {
	typename GT::Arc * arc = arc_itor.get_current_arc();
	cout << arc->get_src_node(g)->valor << " a " <<
	  arc->get_tgt_node(g)->valor;
	cout << " con peso " << arc->peso << endl;
      }
    
    cout << endl;
    
  }
  


  template <class GT> inline static 
  void __duplicar_arcos(GT & g, GT & h,
			typename GT::Node * node, 
			typename GT::Arc *  arc,
			void (*visit)(GT & g, 
				      typename GT::Node *, 
				      typename GT::Arc *)
			)
  {
  if (IS_NODE_VISITED(node, Aleph::Depth_First))
    return; 
  
  NODE_BITS(node).set_bit(Aleph::Depth_First, true); // marca nodo visitado
  
  if (visit not_eq nullptr)
    (*visit)(g, node, arc); 
  
  
      // recorrer arcos de node y visitar recursivamente nodos
      // conectados en profundidad  
  for (typename GT::Node_Arc_Iterator itor(node); itor.has_current(); itor.next())
    {
      typename GT::Arc * arc = itor.get_current_arc();

      if (IS_ARC_VISITED(arc, Aleph::Depth_First))
	continue;

          // marca arco actual visitado
      ARC_BITS(arc).set_bit(Aleph::Depth_First, true); 
      g.insert_arc(arc->get_tgt_node(),arc->get_src_node(g),*arc);
      
  
      __duplicar_arcos(g, h, arc->get_connected_node(node), arc, visit);
    }
}



    template <class GT> inline 
void duplicar_arcos1(GT & g)
{
  DynDlist<typename GT::Arc*> list;

  for (typename GT::Arc_Iterator itor(g); itor.has_current(); itor.next())
    list.append(itor.get_current_arc());

  typename GT::Node * node = g.get_first_node();
  
  while (not list.is_empty())
    {
      typename GT::Arc * arc = list.remove_first();

      g.insert_arc(arc->get_src_node(g), arc->get_tgt_node(g), *arc);
    }
}


# ifdef nada  
  template <class GT> inline 
  void duplicar_arcos(GT& g, GT& h)
  {
     try
      {
	
	    /* asocia nodos para luego guardar arcos */ 
	DynAvlTree<typename GT::Node*, typename GT::Node*> tree; 

            /* copiar nodos */
	for (typename GT::Node_Iterator itor(g); itor.has_current(); itor.next())
	  {
	    typename GT::Node * src_node = itor.get_current_node(); 
	    auto_ptr<typename GT::Node> tgt_node ( new typename GT::Node (src_node) );

	    tree.insert(src_node, tgt_node.get());

	    h.insert_node(tgt_node.release()); 
	  }

	assert(h.get_num_nodes() == g.get_num_nodes());

	    /* copiar arcos */
	for (typename GT::Arc_Iterator itor(g); itor.has_current(); itor.next())
	  {
	    typename GT::Arc * src_arc = itor.get_current_arc();
	    typename GT::Arc * src_arc2 = itor.get_current_arc();

	    assert(tree.test(src_arc->get_src_node(g)) not_eq nullptr and
		   tree.test(src_arc->get_tgt_node(g)) not_eq nullptr);

	    typename GT::Node * src_node = tree[src_arc->get_src_node(g)];
	    typename GT::Node * tgt_node = tree[src_arc->get_tgt_node(g)];

	    h.insert_arc(src_node, tgt_node, *src_arc);
	    h.insert_arc(src_node, tgt_node, *src_arc2);
	  }

	assert(h.get_num_arcs() == 2*g.get_num_arcs());
      }
    catch (...)
      {     /* limpia el grafo para asegurar un estado consistente */
	h.clear_graph();
	throw;
      }
  }
# endif // nada


    

  template <class GT> inline 
  bool double_tree(GT& g,
		   void (*visit)(GT &, typename GT::Node *, typename GT::Arc *),
		   DynDlist<typename GT::Node*> & lista)
  {
    //Verifica que el arbol sea completo
    if(g.get_num_arcs()!=(g.get_num_nodes()*(g.get_num_nodes()-1))/2){
      return false; 
    }
    GT tree;
    prim_min_spanning_tree<GT>(g, tree);

    GT tree1;

    tree1=tree;
    duplicar_arcos1(tree);   

    imprimir_grafo(tree);

    return true;

  }
}


#endif //TPL_PATH_UTILS_H
