# include <autosprintf.h>
# include <generate_graph.H>
# include <tpl_netgraph.H>

typedef Net_Node<string, long> Nodo;

typedef Net_Arc<Empty_Class, long> Tubo; 


typedef Net_Graph<Nodo, Tubo> Red;


void crear_tubo(Red & red, const string & src_name, const string & tgt_name, 
		const Red::Flow_Type & cap)
{
  Red::Node * src = red.search_node(src_name);
  if (src == NULL)
    src = red.insert_node(src_name);

  Red::Node * tgt = red.search_node(tgt_name);
  if (tgt == NULL)
    tgt = red.insert_node(tgt_name);

  red.insert_arc(src, tgt, cap);
}


void crear_red(Red & red)
{
  red.insert_node("C");
  red.insert_node("G");
  red.insert_node("J");
  red.insert_node("L");
  red.insert_node("A");
  red.insert_node("B");
  red.insert_node("F");
  red.insert_node("H");
  red.insert_node("M");
  red.insert_node("D");
  red.insert_node("E");
  red.insert_node("I");
  red.insert_node("K");

  crear_tubo(red, "A", "C", 5);
  crear_tubo(red, "A", "B", 7);
  crear_tubo(red, "A", "E", 3);
  crear_tubo(red, "A", "D", 6);

  crear_tubo(red, "B", "C", 5);
  crear_tubo(red, "B", "F", 6);

  crear_tubo(red, "C", "F", 3);

  crear_tubo(red, "D", "E", 4);
  //  crear_tubo(red, "D", "I", 5);

  crear_tubo(red, "E", "B", 4);
  crear_tubo(red, "E", "F", 5);
  crear_tubo(red, "E", "I", 8);

  crear_tubo(red, "F", "G", 5);
  crear_tubo(red, "F", "J", 7);
  crear_tubo(red, "F", "L", 6);
  crear_tubo(red, "F", "H", 4);

  crear_tubo(red, "G", "C", 4);

  crear_tubo(red, "J", "G", 6);
  crear_tubo(red, "J", "L", 5);

  crear_tubo(red, "H", "E", 3);
  crear_tubo(red, "H", "I", 4);
  crear_tubo(red, "H", "M", 5);

  crear_tubo(red, "I", "K", 4);

  crear_tubo(red, "K", "H", 3);
  crear_tubo(red, "K", "M", 4);

  crear_tubo(red, "L", "M", 6);
  crear_tubo(red, "L", "H", 4);
}


void crear_otra_red(Red & red)
{
  red.insert_node("C");
  red.insert_node("G");
  red.insert_node("J");
  red.insert_node("L");
  red.insert_node("A");
  red.insert_node("B");
  red.insert_node("F");
  red.insert_node("H");
  red.insert_node("M");
  red.insert_node("D");
  red.insert_node("E");
  red.insert_node("I");
  red.insert_node("K");

  crear_tubo(red, "A", "C", 5);
  crear_tubo(red, "A", "B", 7);
  crear_tubo(red, "A", "E", 3);
  crear_tubo(red, "A", "D", 6);

  crear_tubo(red, "B", "C", 5);
  crear_tubo(red, "B", "F", 3);

  crear_tubo(red, "C", "F", 3);

  crear_tubo(red, "D", "E", 4);
  //  crear_tubo(red, "D", "I", 5);

  crear_tubo(red, "E", "B", 4);
  crear_tubo(red, "E", "F", 3);
  crear_tubo(red, "E", "I", 1);

  crear_tubo(red, "F", "G", 5);
  crear_tubo(red, "F", "B", 1);
  crear_tubo(red, "F", "J", 7);
  crear_tubo(red, "F", "L", 3);
  crear_tubo(red, "F", "H", 4);

  crear_tubo(red, "G", "C", 4);

  crear_tubo(red, "J", "G", 6);
  crear_tubo(red, "J", "L", 5);

  crear_tubo(red, "H", "E", 3);
  crear_tubo(red, "H", "I", 4);
  crear_tubo(red, "H", "M", 5);

  crear_tubo(red, "I", "K", 4);

  crear_tubo(red, "K", "H", 3);
  crear_tubo(red, "K", "M", 4);

  crear_tubo(red, "L", "M", 6);
  crear_tubo(red, "L", "H", 4);
}


Path<Red> * path_ptr = NULL;

Red * red_ptr = NULL;

struct Sombra_Nodo
{
  string operator () (Red::Node * p) const
  {
    if (path_ptr == NULL)
      return "";

    return path_ptr->contains_node(p) ? "SHADOW-NODE" : "";
  }
};

struct Sombra_Arco_Res
{
  string operator () (Red::Arc * a) const
  {
    if (path_ptr == NULL)
      return "Arc";

    return path_ptr->contains_arc(a) ? "SHADOW-ARC" : "ARC"; 
  }
};


struct Sombra_Arco_Red
{
  string operator () (Red::Arc * a) const
  {
    if (path_ptr == NULL)
      return "Arc";
    
    Red::Arc * img = (Red::Arc*) ARC_COOKIE(a);

    if (path_ptr->contains_arc(img))
      return "SHADOW-ARC";

    if (not img->is_residual)
      {
	Red::Arc * res_arc = img->img_arc;

	return path_ptr->contains_arc(res_arc) ? "SHADOW-ARC" : "ARC";
      }

    return "ARC"; 
  }
};

struct Nodo_String
{
  string operator () (Red::Node * p) const { return p->get_info(); }
};


struct Arco_Red
{
  string operator () (Red::Arc * a) const
  { 
    Red::Arc * img = (Red::Arc*) ARC_COOKIE(a);

    return gnu::autosprintf("%ld/%ld", red_ptr->get_cap(img),
			    red_ptr->get_flow(img));
  }
};

struct Arco_Normal
{
  string operator () (Red::Arc * a) const
  { 
    return gnu::autosprintf("%ld/%ld", a->cap, a->flow);
  }
};

struct Arco_Residual
{
  string operator () (Red::Arc * a) const
  { 
    return gnu::autosprintf("%ld", red_ptr->get_cap(a) - red_ptr->get_flow(a));
  }
};


template <class Net, class Node_to_String, 
	  class Net_Arc_to_String, class Res_Arc_to_String,
	  class Shade_Node, class Net_Shade_Arc, class Res_Shade_Arc>
void write_ford_fulkerson(Net & net, const string & name)
{
  typedef Res_F<Net> RFA;
  
  Net bak;
  copy_graph(bak, net, true);

  net.make_super_nodes();
  net.make_residual_net();

  typename Net::Node * source = *(net.get_src_nodes().begin());
  typename Net::Node * sink   = *net.get_sink_nodes().begin();

  red_ptr = &net;

  {
    string nn = gnu::autosprintf("%s-%d-a.mf", name.c_str(), 0);
    ofstream out1(nn, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Net_Arc_to_String, Shade_Node, Net_Shade_Arc, RFA>
      (bak, 5, 100, 100, out1);

    string nn2 = gnu::autosprintf("%s-%d-b.mf", name.c_str(), 0);
    ofstream out2(nn2, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Res_Arc_to_String, Shade_Node, Res_Shade_Arc, RFA>
      (net, 5, 100, 100, out2); 
  }

  int i = 1;
  Path<Net> path(net);
  while (true)
    {
      if (not find_path_depth_first<Net, RFA>(net, source, sink, path)) break;

      path_ptr = &path;

      string nn = gnu::autosprintf("%s-%d-a.mf", name.c_str(), i);
      ofstream out1(nn, ios::out);
      generate_cross_graph
	<Net, Node_to_String, Net_Arc_to_String, Shade_Node, Net_Shade_Arc, RFA>
	(bak, 5, 100, 100, out1);
      
      string nn2 = gnu::autosprintf("%s-%d-b.mf", name.c_str(), i);
      ofstream out2(nn2, ios::out);
      generate_cross_graph
	<Net, Node_to_String, Res_Arc_to_String, Shade_Node, Res_Shade_Arc, RFA>
	(net, 5, 100, 100, out2);

      increase_flow(net, path);

      ++i; 
    }

  {
    string nn = gnu::autosprintf("%s-%d-a.mf", name.c_str(), i);
    ofstream out1(nn, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Net_Arc_to_String, Shade_Node, Net_Shade_Arc, RFA>
      (bak, 5, 100, 100, out1);

    string nn2 = gnu::autosprintf("%s-%d-b.mf", name.c_str(), i);
    ofstream out2(nn2, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Res_Arc_to_String, Shade_Node, Res_Shade_Arc, RFA>
      (net, 5, 100, 100, out2); 
  }
   
  net.unmake_residual_net();
  net.unmake_super_nodes();
}

template <class N> struct Filtra_Res  
{
  bool operator () (typename N::Node *, typename N::Arc * a) const
  {
    return not a->is_residual;
  }
  bool operator () (N&, typename N::Arc * a) const
  {
    return not a->is_residual;
  }
};


template <class Net, class Node_to_String, 
	  class Net_Arc_to_String, class Res_Arc_to_String,
	  class Shade_Node, class Net_Shade_Arc, class Res_Shade_Arc>
void write_ford_fulkerson_edmond_karp(Net & net, const string & name)
{
  typedef Res_F<Net> RFA;

  Net bak;
  copy_graph(bak, net, true);

  net.make_super_nodes();
  net.make_residual_net();

  typename Net::Node * source = *(net.get_src_nodes().begin());
  typename Net::Node * sink   = *net.get_sink_nodes().begin();  

  path_ptr = NULL;
  red_ptr = &net;

  {
    string nn = gnu::autosprintf("%s-%d-a.mf", name.c_str(), 0);
    ofstream out1(nn, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Net_Arc_to_String, Shade_Node, Net_Shade_Arc, RFA>
      (bak, 5, 100, 100, out1);

    string nn2 = gnu::autosprintf("%s-%d-b.mf", name.c_str(), 0);
    ofstream out2(nn2, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Res_Arc_to_String, Shade_Node, Res_Shade_Arc, RFA>
      (net, 5, 100, 100, out2); 
  }

  int i = 1;
  Path<Net> path(net);
  while (true)
    {
      if (not find_path_breadth_first<Net, RFA>(net, source, sink, path)) break;

      path_ptr = &path;

      string nn = gnu::autosprintf("%s-%d-a.mf", name.c_str(), i);
      ofstream out1(nn, ios::out);
      generate_cross_graph
	<Net, Node_to_String, Net_Arc_to_String, Shade_Node, Net_Shade_Arc, RFA>
	(bak, 5, 100, 100, out1);
      
      string nn2 = gnu::autosprintf("%s-%d-b.mf", name.c_str(), i);
      ofstream out2(nn2, ios::out);
      generate_cross_graph
	<Net, Node_to_String, Res_Arc_to_String, Shade_Node, Res_Shade_Arc, RFA>
	(net, 5, 100, 100, out2);

      increase_flow(net, path);

      ++i; 
    }

  {
    string nn = gnu::autosprintf("%s-%d-a.mf", name.c_str(), i);
    ofstream out1(nn, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Net_Arc_to_String, Shade_Node, Net_Shade_Arc, RFA>
      (bak, 5, 100, 100, out1);

    string nn2 = gnu::autosprintf("%s-%d-b.mf", name.c_str(), i);
    ofstream out2(nn2, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Res_Arc_to_String, Shade_Node, Res_Shade_Arc, RFA>
      (net, 5, 100, 100, out2); 
  }
   
  net.unmake_residual_net();
  net.unmake_super_nodes();
}

Red::Node * active_node = NULL; 

struct Nodo_Activo
{
  string operator () (Red::Node * p) const
  {
    if (active_node == NULL)
      return "";

    return active_node == p ? "SHADOW-NODE" : "";
  }
};

struct Arco_Red_Push
{
  string operator () (Red::Arc * a) const
  { 
    if (a->is_residual)
      return gnu::autosprintf("%ld", a->cap - a->flow);

    return gnu::autosprintf("%ld/%ld", a->cap, a->flow);
  }
};

struct Arco_Push
{
  string operator () (Red::Arc * a) const
  {
    if (active_node == NULL)
      return "Arc";
    
    if (active_node == a->src_node)
      {
	Red::Node * src = (Red::Node *) a->src_node;
	Red::Node * tgt = (Red::Node *) a->tgt_node;

	if (node_height<Red>(src) == node_height<Red>(tgt) + 1)
	  return "SHADOW-ARC";
      }

    return "ARC"; 
  }
};

    template <class Net> static 
void write_node_attrs(Net & net, ofstream & o)
{
  o << endl;

  typename Net::Node_Iterator it(net); 

  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      typename Net::Node * p = it.get_current_node();

      o << "tag " << i << gnu::autosprintf(" %ld SW 0 0\n", NODE_COUNTER(p));
      o << "tag " << i << gnu::autosprintf(" %ld NE 0 0\n", p->in_flow);
      o << "tag " << i << gnu::autosprintf(" %ld SE 0 0\n", p->out_flow);
    }
}


template <class Net, class Q_Type,
	  class Node_to_String, 
	  class Net_Arc_to_String, class Res_Arc_to_String,
	  class Shade_Node, class Net_Shade_Arc, class Res_Shade_Arc>
void write_preflow_push_maximum_flow(Net & net, const string & name)
{
  typedef Res_F<Net> RFA;
  typedef Filtra_Res<Net> RF;

  net.make_super_nodes();

  net.make_residual_net();
  
  net.reset_nodes();
  net.reset_arcs();

  init_height_in_nodes(net);

  typename Net::Node * source = *net.get_src_nodes().begin();
  typename Net::Node * sink = *net.get_sink_nodes().begin();
  Q_Type q;

       // inundar arcos de nodo fuente y crear conjunto inicial de nodos activos
  for (Node_Arc_Iterator<Net, Res_F<Net> > it(source); it.has_current(); it.next())
    {
      typename Net::Arc * arc  = it.get_current_arc();
      typename Net::Node * tgt = net.get_tgt_node(arc);

      arc->flow = tgt->in_flow = arc->cap;
      arc->img_arc->flow = 0;
      put_in_active_queue(q, tgt);
    }
  source->out_flow = source->out_cap; 

  {
    active_node = source;
    string nn = gnu::autosprintf("%s-%d.mf", name.c_str(), 0);
    ofstream out(nn, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Net_Arc_to_String, Shade_Node, Res_Shade_Arc, RF>
      (net, 5, 100, 100, out); 
    write_node_attrs(net, out);
  }
  
  int n = 0;
  int i = 1;
  red_ptr = &net;
  while (not q.is_empty()) // mientras haya nodos activos en la cola
    {
      typename Net::Node * src = get_from_active_queue(q);
      n++;

      typename Net::Flow_Type excess_flow = src->in_flow - src->out_flow;

      assert(excess_flow >= 0);

      int count = 0; // contador de arcos elegibles

      for (Node_Arc_Iterator<Net, Res_F<Net> > it(src); 
	   it.has_current() and excess_flow > 0; it.next()) 
	{
	  typename Net::Arc * arc = it.get_current_arc();

	  typename Net::Node * tgt = net.get_tgt_node(arc);

	  if (node_height<Net>(src) != node_height<Net>(tgt) + 1) // ¿elegible?
	    continue;

	  const typename Net::Flow_Type flow_avail_in_arc = 
	    arc->cap - arc->flow;

	  typename Net::Flow_Type flow_to_push = 
	    std::min(flow_avail_in_arc, excess_flow);

	  if (count == 0)
	    {
	      active_node = src;
	      string nn = gnu::autosprintf("%s-%d.mf", name.c_str(), i++);
	      ofstream out(nn, ios::out);
	      generate_cross_graph
		<Net, Node_to_String, Net_Arc_to_String, Shade_Node, 
		Res_Shade_Arc, RFA> 
		(net, 5, 100, 100, out); 
	      write_node_attrs(net, out);
	      count++;
	    }

	  arc->flow          += flow_to_push;
	  arc->img_arc->flow -= flow_to_push;

	  if (arc->is_residual)
	    {
	      tgt->out_flow -= flow_to_push;
	      src->in_flow  -= flow_to_push;
	    }
	  else
	    {
	      src->out_flow += flow_to_push;
	      tgt->in_flow  += flow_to_push;	    
	    }

	  if (is_node_active<Net>(tgt) and tgt != sink and tgt != source) 
	    put_in_active_queue<Q_Type>(q, tgt);
	  
	  excess_flow -= flow_to_push;
	}

      if (excess_flow > 0) // 
	{ // No ==> 
	  node_height<Net>(src)++;
	  put_in_active_queue<Q_Type>(q, src);
	}
    }
  
  {
    active_node = NULL;
    string nn = gnu::autosprintf("%s-%d.mf", name.c_str(), i);
    ofstream out(nn, ios::out); 
    generate_cross_graph
      <Net, Node_to_String, Net_Arc_to_String, Shade_Node, Res_Shade_Arc, RF>
      (net, 5, 100, 100, out); 
    write_node_attrs(net, out);
  }

  cout << n << " iteraciones en total" << endl;

  net.unmake_residual_net();
  net.unmake_super_nodes();
}

# include <tpl_random_queue.H>

Red::Arc * arco_actual = NULL;

struct Sombra_Arco
{
  string operator () (Red::Arc * a) const
  {
    if (arco_actual == a)
      return "Shadow-Arc";

    return "ARC"; 
  }
};

typedef Nodo_String Escribe_Nodo;

struct Escribe_Cap
{
  string operator () (Red::Arc * a) const
  {
    return gnu::autosprintf("%ld", a->cap - a->flow);
  }
};

struct Escribe_Cap_Flow
{
  string operator () (Red::Arc * a) const
  {
    return gnu::autosprintf("%ld/%ld", a->cap, a->flow);
  } 
};

template <class Net, class QN_Type, class QA_Type,
	  class Node_to_String, 
	  class Net_Arc_to_String, class Res_Arc_to_String,
	  class Shade_Node, class Net_Shade_Arc, class Res_Shade_Arc>
void write_generic_preflow_edge_push_maximum_flow(Net & net)
{
  typedef Res_F<Net> RFA;
  typedef Filtra_Res<Net> RF;
  net.make_residual_net();
  net.reset_nodes();
  net.reset_arcs();

  init_height_in_nodes(net); 

  typename Net::Node * source = *net.get_src_nodes().begin();
  typename Net::Node * sink   = *net.get_sink_nodes().begin();
  QA_Type q;  // instancia conjunto de arcos elegibles
  QN_Type p;  // instancia conjunto de nodos activos

  for (Node_Arc_Iterator<Net, Res_F<Net> > it(source); it.has_current(); it.next())
    {
      typename Net::Arc * arc  = it.get_current_arc();  
      typename Net::Node * tgt = net.get_tgt_node(arc); 
      arc->flow = tgt->in_flow = arc->cap;  // inunde arco actual
      arc->img_arc->flow       = 0;         // arco residual sin capacidad
        // meter arcos elegibles de tgt
      for (Node_Arc_Iterator<Net, Res_F<Net> > it(tgt); 
	   it.has_current(); it.next())
	{
	  typename Net::Arc * a  = it.get_current_arc(); 
	  if (node_height<Net>(tgt) == 
	      node_height<Net>(net.get_tgt_node(a)) + 1)
	    put_in_active_queue(q, a);  // mete en cola de arcos elegibles
	}
      put_in_active_queue(p, tgt); // mete en cola de nodos activos
    }
  source->out_flow = source->out_cap;

  int i = 0;
  red_ptr = &net;

 process_active_arcs:

  while (not q.is_empty()) // mientras haya arcos elegibles
    {
      typename Net::Arc * arc  = get_from_active_queue(q);
      typename Net::Node * src = net.get_src_node(arc);
      typename Net::Node * tgt = net.get_tgt_node(arc);

      if (node_height<Net>(src) != node_height<Net>(tgt) + 1) 
	continue;	

      typename Net::Flow_Type excess = src->in_flow - src->out_flow;

      if (excess == 0) 
	{
	  remove_from_active_queue(p, src); // TODO: verificar si hace falta 
	  continue;
	}

      {
	active_node = src;
	arco_actual = arc;
	string nn = gnu::autosprintf("test-%d.mf", i++);
	ofstream out(nn, ios::out);
	generate_cross_graph
	  <Net, Node_to_String, Res_Arc_to_String, 
	  Shade_Node, Res_Shade_Arc, RFA>
	  (net, 5, 100, 100, out); 
	write_node_attrs(net, out);
      }

      const typename Net::Flow_Type push = 
	std::min(excess, arc->cap - arc->flow);
      arc->flow          += push;
      arc->img_arc->flow -= push;
      if (arc->is_residual)
	{
	  tgt->out_flow -= push; 
	  src->in_flow  -= push;
	}
      else
	{
	  tgt->in_flow  += push;
	  src->out_flow += push; 
	}
      excess -= push;

      if (is_node_active<Net>(tgt) and tgt != source and tgt != sink) 
	{
	  for (Node_Arc_Iterator<Net, Res_F<Net> > it(tgt); 
	       it.has_current(); it.next()) 
	    {
	      typename Net::Arc * a  = it.get_current_arc();

	      if (node_height<Net>(tgt) == 
		  node_height<Net>(net.get_tgt_node(a)) + 1)
		put_in_active_queue(q, a);
	    }
	  put_in_active_queue(p, tgt);
	}

      if (excess == 0)
	{
	  remove_from_active_queue(p, src);
	  continue;
	}

      if (src != source and src != sink and
	  not has_arcs_in_active_queue<Net>(src)) 
	{ // sí ==> incremente h(active) y re-inserte arc en q
	  remove_from_active_queue(p, src);
	  node_height<Net>(src)++;
	  put_in_active_queue(p, src);
	  
	  for (Node_Arc_Iterator<Net> it(src); it.has_current(); it.next()) 
	    {
	      typename Net::Arc * a = it.get_current_arc();

	      if (node_height<Net>(src) == 
		  node_height<Net>(net.get_tgt_node(a)) + 1 
		  and a->cap - a->flow > 0) 
		put_in_active_queue(q, a);

	      typename Net::Arc * i = a->img_arc;
    
	      assert(net.get_src_node(i) != src and net.get_tgt_node(i) == src);

	      if (i->cap - i->flow > 0 and // ¿i es elegible?
		  node_height<Net>(net.get_src_node(i)) == 
		  node_height<Net>(src) + 1)
		put_in_active_queue(q, i);
	    }
	}
    }

  while (not p.is_empty()) // mientras haya nodos elegibles
    {
      typename Net::Node * src = get_from_active_queue(p);
      remove_from_active_queue(p, src);
      node_height<Net>(src)++;
      put_in_active_queue(p, src);

      for (Node_Arc_Iterator<Net> it(src); it.has_current(); it.next()) 
	{
	  typename Net::Arc * a = it.get_current_arc();

	  if (node_height<Net>(src) == 
	      node_height<Net>(net.get_tgt_node(a)) + 1 
	      and a->cap - a->flow > 0) 
	    put_in_active_queue(q, a);

	  typename Net::Arc * i = a->img_arc;
    
	  assert(net.get_src_node(i) != src and net.get_tgt_node(i) == src);

	  if (i->cap - i->flow > 0 and // ¿i es elegible?
	      node_height<Net>(net.get_src_node(i)) == 
	      node_height<Net>(src) + 1)
	    put_in_active_queue(q, i);
	}
      
      goto process_active_arcs;
    }

  cout << i << " figuras " << endl;

  {
    arco_actual = NULL;
    active_node = NULL;
    string nn = gnu::autosprintf("test-%d.mf", i++);
    ofstream out(nn, ios::out);
    generate_cross_graph
      <Net, Node_to_String, Net_Arc_to_String, 
      Shade_Node, Net_Shade_Arc, RF>
      (net, 5, 100, 100, out); 
    write_node_attrs(net, out);
  }
  net.unmake_residual_net();

  assert(source->out_flow == sink->in_flow);
}

struct Compara_Arco
{
  bool operator () (Red::Arc * a1, Red::Arc * a2) const
  {
    Red::Node * src1 = (Red::Node *) a1->src_node;
    Red::Node * src2 = (Red::Node *) a2->src_node;

    if (src1->counter == src2->counter)
      return a1->cap - a1->flow > a2->cap - a2->flow;

    return src1->counter > src2->counter;
  }
};



Aleph::set<Red::Node*> * __vs_ptr = NULL;
Aleph::set<Red::Node*> * __vt_ptr = NULL;


struct Arco_Cruce
{
  string operator () (Red::Arc * a) const
  {
    if (red_ptr == NULL)
      return "Arc";

    if (__vs_ptr == NULL or __vt_ptr == NULL)
      return "Arc";

    Red::Node * src = red_ptr->get_src_node(a);
    Red::Node * tgt = red_ptr->get_tgt_node(a);

    if (__vs_ptr->count(src) > 0 and __vt_ptr->count(tgt) > 0)
      return "Shadow-Arc";

    if (__vt_ptr->count(src) > 0 and __vs_ptr->count(tgt) > 0)
      return "Dashed-Arc";

    return "ARC"; 
  }
};


struct Nodo_Cruce
{

};


int main()
{
  {
    Net_Graph<Nodo, Tubo> red; 

    crear_red(red);

    write_ford_fulkerson
    <Red, Nodo_String, Arco_Red, Arco_Residual, Sombra_Nodo, Sombra_Arco_Red, Sombra_Arco_Res>
    (red, "ff");

    red.check_network();
  }

  {
    Net_Graph<Nodo, Tubo> red; 

    crear_red(red);

    write_ford_fulkerson_edmond_karp
    <Red, Nodo_String, Arco_Red, Arco_Residual, Sombra_Nodo, Sombra_Arco_Red, Sombra_Arco_Res>
    (red, "ffek");
  }

  {
    Net_Graph<Nodo, Tubo> red; 

    crear_red(red);

    write_preflow_push_maximum_flow
      <Red, DynListQueue<Red::Node *>,
      Nodo_String, Arco_Red_Push, Arco_Residual, Nodo_Activo, 
      Arco_Push, Arco_Push>
      (red, "pfq");
  }

  {
    Net_Graph<Nodo, Tubo> red; 

    crear_red(red);

    write_preflow_push_maximum_flow
      <Red, DynBinHeap<Red::Node *, Compare_Height<Red> >,
      Nodo_String, Arco_Red_Push, Arco_Residual, Nodo_Activo, 
      Arco_Push, Arco_Push>
      (red, "pfh");
  }

//    {
//      Net_Graph<Nodo, Tubo> red; 

//      crear_red(red);

//       write_generic_preflow_edge_push_maximum_flow
//         <Red, DynSetTreap<Red::Node*>, DynListStack<Red::Arc*>,
//          Escribe_Nodo, Escribe_Cap_Flow, Escribe_Cap, 
//          Nodo_Activo, Sombra_Arco, Sombra_Arco> (red);
//    }
   {
     Net_Graph<Nodo, Tubo> red; 

     crear_red(red);
     write_generic_preflow_edge_push_maximum_flow
       <Red, DynSetTreap<Red::Node*>, DynBinHeap<Red::Arc*, Compara_Arco>,
        Escribe_Nodo, Escribe_Cap_Flow, Escribe_Cap, 
        Nodo_Activo, Sombra_Arco, Sombra_Arco> (red);
   }

   {
     Net_Graph<Nodo, Tubo> red; 
     Aleph::set<Red::Node*> vs, vt;

     red_ptr = &red;
     __vs_ptr = &vs;
     __vt_ptr = &vt;

     DynDlist<Red::Arc *> cuts, cutt;

     crear_otra_red(red);
     
     min_cut <Red, Edmonds_Karp_Maximum_Flow> (red, vs, vt, cuts, cutt);

     {
       arco_actual = NULL;
       active_node = NULL;
       ofstream out("corte-minimo.mf", ios::out);
       generate_cross_graph
	 <Red, Escribe_Nodo, Escribe_Cap_Flow, Nodo_Activo, Arco_Cruce, 
	 No_Res_Arc<Red> >  (red, 5, 100, 100, out); 
     }

     cout << "Vs=";
     for (Aleph::set<Red::Node*>::iterator it(vs.begin()); it != vs.end(); ++it)
       cout << (*it)->get_info() << ",";

     cout << endl 
	  << "Vt=";

     for (Aleph::set<Red::Node*>::iterator it(vt.begin()); it != vt.end(); ++it)
       cout << (*it)->get_info() << ",";
     
     cout << endl
	  << "Arcos hacia Vt:" << endl;
     for (DynDlist<Red::Arc *>::Iterator it(cuts); it.has_current(); it.next())
       {
	 Red::Arc * a =  it.get_current();
	 cout << "    " << red.get_src_node(a)->get_info() << "-->"
	      << red.get_tgt_node(a)->get_info() << endl;
       }

     cout << endl
	  << "Arcos hacia Vs:" << endl;
     for (DynDlist<Red::Arc *>::Iterator it(cutt); it.has_current(); it.next())
       {
	 Red::Arc * a =  it.get_current();
	 cout << "    " << red.get_src_node(a)->get_info() << "-->"
	      << red.get_tgt_node(a)->get_info() << endl;
       }
   }
# ifdef nada
{
     Net_Graph<Nodo, Tubo> red; 
     Aleph::set<Red::Node*> vs, vt;
     DynDlist<Red::Arc *> cuts, cutt;

     

     crear_otra_red(red);
     
     min_cut <Red, Depth_First_Preflow_Maximum_Flow> (red, vs, vt, cuts, cutt);

     {
       arco_actual = NULL;
       active_node = NULL;
       ofstream out("corte-minimo-1.mf", ios::out);
       generate_cross_graph
	 <Red, Escribe_Nodo, Escribe_Cap_Flow, Nodo_Activo, Sombra_Arco, 
	 No_Res_Arc<Red> >  (red, 5, 100, 100, out); 
     }

     cout << "Vs=";
     for (Aleph::set<Red::Node*>::iterator it(vs.begin()); it != vs.end(); ++it)
       cout << (*it)->get_info() << ",";

     cout << endl 
	  << "Vt=";

     for (Aleph::set<Red::Node*>::iterator it(vt.begin()); it != vt.end(); ++it)
       cout << (*it)->get_info() << ",";
     
     cout << endl
	  << "Arcos hacia Vt:" << endl;
     for (DynDlist<Red::Arc *>::Iterator it(cuts); it.has_current(); it.next())
       {
	 Red::Arc * a =  it.get_current();
	 cout << "    " << red.get_src_node(a)->get_info() << "-->"
	      << red.get_tgt_node(a)->get_info() << endl;
       }

     cout << endl
	  << "Arcos hacia Vs:" << endl;
     for (DynDlist<Red::Arc *>::Iterator it(cutt); it.has_current(); it.next())
       {
	 Red::Arc * a =  it.get_current();
	 cout << "    " << red.get_src_node(a)->get_info() << "-->"
	      << red.get_tgt_node(a)->get_info() << endl;
       }
   }
# endif
//    {
//      Net_Graph<Nodo, Tubo> red; 

//      crear_red(red);
//       write_generic_preflow_edge_push_maximum_flow
//         <Red, DynSetTreap<Red::Node*>, DynDlist<Red::Arc*>,
//          Escribe_Nodo, Escribe_Cap_Flow, Escribe_Cap, 
//          Nodo_Activo, Sombra_Arco, Sombra_Arco> (red);
//   }

//   {
//     Net_Graph<Nodo, Tubo> red; 

//     crear_red(red);

//     write_preflow_push_maximum_flow
//       <Red, Random_Set<Red::Node*>,
//       Nodo_String, Arco_Red_Push, Arco_Residual, Nodo_Activo, 
//       Arco_Push, Arco_Push>
//       (red, "pfr");

}



