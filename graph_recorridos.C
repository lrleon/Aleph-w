
#include <tpl_graph_utils.H>
#include <iostream>

using namespace Aleph;
using namespace std;

//************************************PROFUNDIDAD********************


template <class GT> inline static 
void __Profundidad_Arcos(GT & g,
			 typename GT::Node * node,
			 typename GT::Arc * arc,
			 void (*visit)(GT &,
				       typename GT::Node *,
				       typename GT::Arc *),
			 long & node_counter,
			 long & arc_counter
			 )
{
  if (IS_ARC_VISITED(arc))
    return;
	
  ARC_COUNTER(arc) = ++arc_counter;
		
  typename GT::Node * aux_node;
  if (IS_NODE_VISITED(node))
    aux_node = NULL;
  else {
    aux_node = node;
    NODE_COUNTER(node) = ++node_counter;
  }

  if (visit not_eq NULL)
    (*visit)(g, aux_node, arc);
	
  if (aux_node == NULL)
    return;

  for (typename GT::Node::Iterator itor(g, node); itor.has_current(); itor.next())
    {
      typename GT::Arc * arc = itor.get_current_arc();
      typename GT::Node * node = arc -> get_connected_node(node);
		
      __Profundidad_Arcos<GT>(g,
			      node,
			      arc,
			      visit,
			      node_counter,
			      arc_counter
			      );
    }
}

//*****************************INTERFAZ 1 DE PROFUNDIDAD***************

template <class GT> inline
void Profundidad_Arcos(GT & g,
		       void (*visit)(GT &,
				     typename GT::Node *,
				     typename GT::Arc *)
		       )
{
  g.reset_nodes();
  g.reset_arcs();

  long node_counter = No_Visited;
  long arc_counter = No_Visited;

  typename GT::Arc * arc = g.get_first_arc();
  if (arc == NULL)
    return;
	
  (*visit)(g, arc -> get_src_node(), NULL);

  __Profundidad_Arcos<GT>(g, 
			  arc->get_tgt_node(), arc, 
			  visit,
			  node_counter, arc_counter);
}

//*****************************INTERFAZ 2 DE PROFUNDIDAD****************
template <class GT> inline
void Profundidad_Arcos(GT & g,
		       typename GT::Arc * arc_start,
		       void (*visit)(GT &,
				     typename GT::Node *,
				     typename GT::Arc *)
		       )
{
  assert(arc_start not_eq NULL);

  g.reset_nodes();
  g.reset_arcs();

  long node_counter = No_Visited;
  long arc_counter = No_Visited;

  (*visit)(g, arc_start -> get_node_src(), NULL);

  __Profundidad_Arcos<GT>(g, arc -> get_node_tgt(), arc_start, visit, node_counter, arc_counter);
}


//********************************AMPLITUD 1***************************

/*	template <class GT> inline static
	void __Amplitud_Arcos(GT & g,
	typename GT::Arc * arc_start,
	void (*visit)(GT &,
	typename GT::Node *,
	typename GT::Arc  *)
	)
	{
	ASSERT(arc_start not_eq NULL);

	NODE_COUNTER(arc -> get_src_node())++;
	(*visit)(g, arc_start -> get_src_node(), NULL);
	ARC_COUNTER(arc)++;
	NODE_COUNTER(arc -> get_tgt_node())++;
	(*visit)(g, arc -> get_tgt_node(), arc);
	
	DinListQueue<typename GT::Node *> queue;
	queue.put(arc_start -> get_tgt_node());

	while(not queue.is_empty()){

	typename GT::Node * node = queue.get();
	for (typename GT::Node::Iterator itor(g, node), itor.has_current(); itor.next()){

	typename GT::Arc * arc = itor.get_current_arc();
	if (IS_ARC_VISITED(arc))
	continue;

	typename GT::Node * aux_node;
	if (IS_NODE_VISITED(arc -> get_connected_node(node)))
	aux_node = NULL;
	else {
	aux_node = arc -> get_connected_node(node);
	queue.put(aux_node);
	NODE_COUNTER(aux_node)++;
	}

	ARC_COUNTER(arc)++;
	(*visit)(g, arc, aux_node);
	}
	}
	}*/


//******************************AMPLITUD 2*****************************

template <class GT> inline static
void __Amplitud_Arcos(GT & g,
		      typename GT::Arc * arc_start,
		      void (*visit)(GT &,
				    typename GT::Node *,
				    typename GT::Arc  *)
		      )
{
  assert(arc_start not_eq NULL);

  NODE_COUNTER(arc_start -> get_src_node())++;
  ARC_COUNTER(arc_start)++;
  (*visit)(g, arc_start -> get_src_node(), arc_start);

  DynListQueue<typename GT::Node *> queue;
  queue.put(arc_start -> get_src_node());

  while (not queue.is_empty()){

    typename GT::Node * node = queue.get();
    if (IS_NODE_VISITED(node))
      continue;
		
    for (typename GT::Node::Iterator itor(g, node); itor.has_current(); itor.next()){

      typename GT::Arc * arc = itor.get_current_arc();
      if (IS_ARC_VISITED(arc))
	continue;

      typename GT::Node * aux_node;
      if (IS_NODE_VISITED(node))
	aux_node = NULL;
      else {
	aux_node = node;
	NODE_COUNTER(aux_node)++;
      }
			
      if (not IS_NODE_VISITED(arc -> get_connected_node(node)))
	queue.put(arc -> get_connected_node(node));

      (*visit)(g, aux_node, arc);
    }
  }
}

//*******************************INTERFAZ 1 DE AMPLITUD******************

template <class GT> inline
void Amplitud_Arcos(GT & g,
		    void (*visit)(GT &,
				  typename GT::Node *,
				  typename GT::Arc *)
		    )
{
	
  g.reset_nodes();
  g.reset_arcs();

  __Amplitud_Arcos<GT>(g, g.get_first_arc(), visit);
}

//******************************INTERFAZ 2 DE AMPLITUD*******************

template <class GT> inline
void Amplitud_Arcos(GT & g,
		    typename GT::Arc * arc_start,
		    void (*visit)(GT &,
				  typename GT::Node*,
				  typename GT::Arc *)
		    )
{

  g.reset_nodes();
  g.reset_arcs();
	
  __Amplitud_Arcos<GT>(g, arc_start, visit);
}

//******************************INICIO DE LA APLICACION********************

struct nodo {
  char valor;
  nodo() : valor('A'){}
  nodo(char v) : valor(v){}
};

struct arco {
  int distancia;
  arco() : distancia(0){}
  arco(int d) : distancia(d){}
};

typedef List_Graph<nodo, arco> Grafo;

void Visitar(Grafo &, Grafo::Node * node, Grafo::Arc * arc){
  if (node != NULL)
    cout << "Nodo: " << node -> valor << endl;
  if (arc != NULL)
    cout << "Arco: " << arc -> distancia << endl;
}

int main(){
  Grafo g;
  Grafo::Node * n1, *n2, *n3;
  n1 = g.insert_node(nodo('A'));
  n2 = g.insert_node(nodo('B'));
  g.insert_arc(n1, n2, arco(20));
  n3 = g.insert_node(nodo('C'));
  g.insert_arc(n1, n3, arco(30));
  g.insert_arc(n2, n3, arco(10));
  Profundidad_Arcos(g, Visitar);
  Amplitud_Arcos(g, Visitar);
  return 0;
}
