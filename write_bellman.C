
# include <autosprintf.h>
# include <limits>
# include <iostream>
# include <tpl_graph_utils.H>
# include <Bellman_Ford.H>
# include <generate_spanning_tree_picture.H>


using namespace std;

using namespace Aleph;

# define INDENT "    "


# define Nassert(p) (static_cast<Bellman_Ford_Node_Info<GT, Distance>*>(NODE_COOKIE(p)))
# define IDX(p) (Nassert(p)->idx)
# define ACU(p) (Nassert(p)->acum)

struct Nodo
{
  string nombre;

  Nodo() {}

  Nodo(const string & str) : nombre(str) { /* empty */ }

  Nodo(char * str) : nombre(str) { /* empty */ }

  const bool operator == (const Nodo & der) const 
  {
    return nombre == der.nombre; 
  }
};


struct Arco
{
  double distancia;

  Arco() : distancia(numeric_limits<double>::infinity()) { /* empty */ }

  Arco(const double & dist) : distancia(dist) 
  {
    /* empty */ 
  }

  double & get_distance() { return distancia; }

  operator double & () { return distancia; }
}; 


const Arco Arco_Zero(0);


typedef Graph_Node<Nodo> Node_Nodo;

typedef Graph_Arc<Arco> Arco_Arco;

typedef List_Digraph<Node_Nodo, Arco_Arco> Grafo;


struct Distancia
{
  typedef double Distance_Type;

  static const double Max_Distance;

  static const double Zero_Distance;

  Distance_Type & operator () (Grafo::Arc * a) const
  {
    return a->get_info().distancia;
  }
};

const double Distancia::Max_Distance = numeric_limits<double>::infinity();

const double Distancia::Zero_Distance = 0;



void insertar_arco(Grafo &       grafo, 
		  const string & src_name, 
		  const string & tgt_name,
		  const double & distancia)
{
  Grafo::Node * n1 = grafo.search_node(Nodo(src_name));

  if (n1 == NULL)
    n1 = grafo.insert_node(src_name);

  Grafo::Node * n2 = grafo.search_node(Nodo(tgt_name));

  if (n2 == NULL)
    n2 = grafo.insert_node(tgt_name);

  grafo.insert_arc(n1, n2, Arco(distancia));
}



void build_test_graph(Grafo & g)
{
  g.insert_node(Nodo("C"));
  g.insert_node(Nodo("E"));
  g.insert_node(Nodo("I"));
  g.insert_node(Nodo("A"));
  g.insert_node(Nodo("F"));
  g.insert_node(Nodo("G"));
  g.insert_node(Nodo("B"));
  g.insert_node(Nodo("D"));
  g.insert_node(Nodo("H"));

  insertar_arco(g, "D", "F", 2);
  insertar_arco(g, "D", "F", 1);

  insertar_arco(g, "C", "A", 1);  
  insertar_arco(g, "C", "E", 4);  
  insertar_arco(g, "C", "F", 3);  
  insertar_arco(g, "D", "H", 2); 
  insertar_arco(g, "D", "B", 1); 
  insertar_arco(g, "E", "G", 2);
  insertar_arco(g, "E", "C", 1);
  insertar_arco(g, "E", "F", 1);
  insertar_arco(g, "E", "I", -2);
  insertar_arco(g, "G", "D", 3);
  insertar_arco(g, "G", "F", 1);
  insertar_arco(g, "G", "H", 2);
  insertar_arco(g, "G", "I", -2);
  insertar_arco(g, "B", "F", 1);
  insertar_arco(g, "B", "D", 3);
  insertar_arco(g, "G", "E", -1);
  insertar_arco(g, "H", "D", -2);
  insertar_arco(g, "H", "G", -1);
  insertar_arco(g, "I", "G", 4);
  insertar_arco(g, "F", "D", -1);
  insertar_arco(g, "F", "C", -1);
  insertar_arco(g, "F", "E", 2);
  insertar_arco(g, "F", "B", 2);
  insertar_arco(g, "A", "B", 2);
  insertar_arco(g, "A", "F", 5);

}



void copia_Arco_Arco(Arco_Arco * arc, const long&, const long&, double& value)
{
  value = arc->get_info().get_distance();
}

DynArray<Grafo::Node*> *  nodes_ptr;


    template <typename GT, class Distance> 
struct Shade_Bellman_Node
{
  string operator () (typename GT::Node * p) const
  {
    const long & n = nodes_ptr->size();

    int i = sequential_search<Grafo::Node*>(*nodes_ptr, p, 0, n - 1);

    return i >= 0 ? "SHADOW-NODE" : "";
  }
};

DynArray<Grafo::Arc*> *  arcs_ptr;

    template <typename GT, class Distance> 
struct Shade_Bellman_Arc
{
  string operator () (typename GT::Arc * a) const
  {
    const long & n = arcs_ptr->size();

    int i = sequential_search<Grafo::Arc*>(*arcs_ptr, a, 0, n - 1);

    return i >= 0 ?  "SHADOW-ARC" : "ARC"; 
  }
};

struct Nodo_String
{
  string operator () (Grafo::Node * p) const
  { 
    return p->get_info().nombre;
  }
};


struct Arco_String
{
  string operator () (Grafo::Arc * a) const
  { 
    int d = (int) a->get_info().distancia;
    return gnu::autosprintf("%d", d);
  }
};

    template <class GT, class Distance> static 
void write_acum(GT & g, ofstream & o)
{
  o << endl;

  typename GT::Node_Iterator it(g); 

  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      typename GT::Node * p = it.get_current_node();

      const typename Distance::Distance_Type & acu = ACU(p);

      if (acu < numeric_limits<double>::infinity())
	o << "tag " << i << gnu::autosprintf(" %d SW 0 0\n", (int) acu);
      else
	o << "tag " << i << " $\\infty$ SW 0 0" << endl;
    }
}



    template <class GT, class Distance, class Compare, class Plus> inline
bool write_bellman_ford_min_spanning_tree(GT &                g,
					  typename GT::Node * start_node, 
					  GT &                tree)
{
  if (not g.is_digraph())
    throw std::domain_error("Bellman-Ford algorithm only operates on digraphs");

  clear_graph(tree); // limpiar árbol abarcador destino 

  DynArray<typename GT::Node*> pred;
  DynArray<typename GT::Arc*> arcs;

  nodes_ptr = &pred;
  arcs_ptr  = &arcs;

  {
    typename GT::Node_Iterator it(g);

    for (int i = 0; it.has_current(); ++i, it.next())
      {
	typename GT::Node * p = it.get_current_node();

	pred[i] = NULL; 
	arcs[i] = NULL;

	g.reset_bit(p, Aleph::Min); // colocar bit en cero

          // apartar memoria para cookie (índice y acumulado en distancia)
	Bellman_Ford_Node_Info <GT, Distance> * ptr = 
	  new Bellman_Ford_Node_Info <GT, Distance>;

	NODE_BITS(p).set_bit(Min, false); 
	NODE_BITS(p).set_bit(Breadth_First, false); 
	NODE_COOKIE(p) = ptr;

    	    // inicializar campos del cookie
	ptr->idx  = i;
	ptr->acum = Distance::Max_Distance;
      }
  }

  ACU(start_node) = Distance::Zero_Distance;

  g.reset_bit_arcs(Min); 

  for (int i = 0, n = g.get_num_nodes() - 1; i < n; ++i)
    {
      string name = gnu::autosprintf("bellman-%d-aux.gra", i);
      ofstream out(name, ios::out);
    
      generate_net_graph
	<Grafo, Nodo_String, Arco_String, 
	Shade_Bellman_Node<GT, Distance>, 
	Shade_Bellman_Arc<GT, Distance> >
	(g, 3, 20, 20, out);

      write_acum<GT, Distance>(g, out);

        // recorrer los arcos para evaluar si relajamiento
      for (typename GT::Arc_Iterator it(g); it.has_current(); it.next())
	{
	  typename GT::Arc * arc = it.get_current_arc();

	  typename GT::Node * src = g.get_src_node(arc);
	  typename GT::Node * tgt = g.get_tgt_node(arc);

	  const typename Distance::Distance_Type & dist =  Distance () (arc);

	  const typename Distance::Distance_Type & acum_src = ACU(src);

	  typename Distance::Distance_Type & acum_tgt = ACU(tgt);

	  const typename Distance::Distance_Type sum = Plus () (acum_src, dist);

	  if (Compare() (sum, acum_tgt))
	    {
	      const int & idx = IDX(tgt);

	      pred[idx] = src;
	      arcs[idx] = arc;
	      acum_tgt  = sum;
	    } 
	}
    }

  string name = gnu::autosprintf("bellman-%d-aux.gra", g.get_num_nodes() -1);
  ofstream out(name, ios::out);
    
  generate_net_graph<Grafo, Nodo_String, Arco_String, 
    Shade_Bellman_Node<GT, Distance>, 
    Shade_Bellman_Arc<GT, Distance> >
  (g, 3, 20, 20, out);

  write_acum<GT, Distance>(g, out);

  bool ret_val = true;

      // recorrer todos los arcos en búsqueda de un ciclo negativo
  for (typename GT::Arc_Iterator it(g); it.has_current(); it.next())
    {
      typename GT::Arc * arc = it.get_current_arc();

      typename GT::Node * src = g.get_src_node(arc);
      typename GT::Node * tgt = g.get_tgt_node(arc);

      const typename Distance::Distance_Type & dist     = Distance () (arc);
      const typename Distance::Distance_Type & acum_src = ACU(src);
      const typename Distance::Distance_Type & acum_tgt = ACU(tgt);
      const typename Distance::Distance_Type sum = Plus ()(acum_src, dist);

      if (Compare() (sum, acum_tgt))
	{
	  ret_val = false; // se detectó un ciclo negativo
	  break;
	}
    }

  if (not ret_val) // ¿hay ciclos negativos?
    {    // liberar memoria de los cookies d elos nodos
      for (typename GT::Node_Iterator it(g); it.has_current(); it.next())
	delete Nassert(it.get_current_node());

      return false;
    }

  for (int i = 0; i < g.get_num_nodes(); ++i)
    {
      typename GT::Arc * garc = arcs[i];

      if (garc == NULL)
	continue;

      typename GT::Node * gsrc = g.get_src_node(garc);
      typename GT::Node * tsrc = NULL;

      if (IS_NODE_VISITED(gsrc, Min))
	tsrc = static_cast<typename GT::Node*>(NODE_COOKIE(gsrc));
      else
	{
	  NODE_BITS(gsrc).set_bit(Min, true); // marcar bit

	  delete Nassert(gsrc);

	  tsrc = tree.insert_node(gsrc->get_info());
	  GT::map_nodes(gsrc, tsrc);
	}

      typename GT::Node * gtgt = g.get_tgt_node(garc);
      typename GT::Node * ttgt = NULL;

      if (IS_NODE_VISITED(gtgt, Min))
	ttgt = static_cast<typename GT::Node*>(NODE_COOKIE(gtgt));
      else
	{
	  NODE_BITS(gtgt).set_bit(Min, true); // marcar bit

	  delete Nassert(gtgt);

	  ttgt = tree.insert_node(gtgt->get_info());
	  GT::map_nodes(gtgt, ttgt);
	}

      typename GT::Arc * tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
      GT::map_arcs(garc, tarc);
      ARC_BITS(garc).set_bit(Min, true);
    }

  return true; // no hay ciclos negativos
}


    template <class GT, class Distance> inline
bool write_bellman_ford_min_spanning_tree(GT &                g,
					  typename GT::Node * start_node, 
					  GT &                tree)
{
  return write_bellman_ford_min_spanning_tree
    <GT, Distance, Aleph::less<typename Distance::Distance_Type>,
     Aleph::plus<typename Distance::Distance_Type> > (g, start_node, tree);
}

    template <class GT, class Distance, class Compare, class Plus> inline
bool write_q_bellman_ford_min_spanning_tree(GT &                g,
					    typename GT::Node * start_node, 
					    GT &                tree)
{
  if (not g.is_digraph())
    throw std::domain_error("Bellman-Ford algorithm only operates on digraphs");

  clear_graph(tree); // limpiar árbol abarcador destino 

  DynArray<typename GT::Node*> pred;
  DynArray<typename GT::Arc*> arcs;

  nodes_ptr = &pred;
  arcs_ptr  = &arcs;

  {
    typename GT::Node_Iterator it(g);

    for (int i = 0; it.has_current(); ++i, it.next())
      {
	typename GT::Node * p = it.get_current_node();

	pred[i] = NULL; 
	arcs[i] = NULL;

	g.reset_bit(p, Aleph::Min); // colocar bit en cero

	    // apartar memoria para cookie (índice y acumulado en distancia)
	Bellman_Ford_Node_Info <GT, Distance> * ptr = 
	  new Bellman_Ford_Node_Info <GT, Distance>;

	NODE_BITS(p).set_bit(Min, false); 
	NODE_BITS(p).set_bit(Breadth_First, false); 
	NODE_COOKIE(p) = ptr;

    	    // inicializar campos del cookie
	ptr->idx  = i;
	ptr->acum = Distance::Max_Distance;
      }
  }

  ACU(start_node) = Distance::Zero_Distance;

  g.reset_bit_arcs(Min); 
  
  DynListQueue<typename GT::Node*> q;
  put_in_queue <GT> (q, start_node);

  int k = 0;

  {
    string name = gnu::autosprintf("bellman-q-%d-aux.gra", k++);
    ofstream out(name, ios::out);
    
    generate_net_graph
      <Grafo, Nodo_String, Arco_String, 
      Shade_Bellman_Node<GT, Distance>, 
      Shade_Bellman_Arc<GT, Distance> >
      (g, 3, 20, 20, out);

    write_acum<GT, Distance>(g, out);
  }

  for (int i = 0, n = (g.get_num_nodes() - 1)*g.get_num_arcs(); 
       (not q.is_empty()) and i < n; // cola no vacía y no más de V.E arcos
       /* nothing */)
    {
      typename GT::Node * src = get_from_queue <GT> (q);

      for (typename GT::Node_Arc_Iterator it(src); 
	   it.has_current() and i < n; // haya arco y no más de V.E arcos
	   it.next(), ++i) // avanza a siguiente arcos de nodo y cuenta arco
	{
	  typename GT::Arc * arc = it.get_current_arc();

	  typename GT::Node * tgt = it.get_tgt_node();

	  const typename Distance::Distance_Type & acum_src = ACU(src);

	  const typename Distance::Distance_Type & w = Distance () (arc);

	  const typename Distance::Distance_Type sum_src = Plus ()(acum_src, w);

	  typename Distance::Distance_Type & acum_tgt = ACU(tgt);

	    // relajar arco
	  if (Compare () (sum_src, acum_tgt)) // acum_src < acum_tgt 
	    {
	      const int & idx = IDX(tgt);

	      pred[idx] = src;
	      arcs[idx] = arc;
	      acum_tgt  = sum_src;

	      if (not is_in_queue <GT> (tgt))
		put_in_queue <GT> (q, tgt);
	    }

	  if (i % g.get_num_arcs() == 0)
	    {
	      string name = gnu::autosprintf("bellman-q-%d-aux.gra", k++);
	      ofstream out(name, ios::out);
		  
	      generate_net_graph <Grafo, Nodo_String, Arco_String, 
		Shade_Bellman_Node<GT, Distance>, 
		Shade_Bellman_Arc<GT, Distance> > (g, 3, 20, 20, out);
	  
	      write_acum<GT, Distance>(g, out);
	    }
	  
	}
    }

  string name = gnu::autosprintf("bellman-q-%d-aux.gra", k++);
  ofstream out(name, ios::out);
		  
  generate_net_graph <Grafo, Nodo_String, Arco_String, 
    Shade_Bellman_Node<GT, Distance>, 
    Shade_Bellman_Arc<GT, Distance> >
  (g, 3, 20, 20, out);

  write_acum<GT, Distance>(g, out);

  bool ret_val = true;

    // recorrer todos los arcos en búsqueda de un ciclo negativo
  for (typename GT::Arc_Iterator it(g); it.has_current(); it.next())
    {
      typename GT::Arc * arc = it.get_current_arc();

      typename GT::Node * src = g.get_src_node(arc);
      typename GT::Node * tgt = g.get_tgt_node(arc);

      const typename Distance::Distance_Type & dist     = Distance () (arc);
      const typename Distance::Distance_Type & acum_src = ACU(src);
      const typename Distance::Distance_Type & acum_tgt = ACU(tgt);
      const typename Distance::Distance_Type sum = Plus ()(acum_src, dist);

      if (Compare() (sum, acum_tgt))
	{
	  ret_val = false; // se detectó un ciclo negativo
	  break;
	}
    }

  if (not ret_val) // ¿hay ciclos negativos?
    {    // liberar memoria de los cookies d elos nodos
      for (typename GT::Node_Iterator it(g); it.has_current(); it.next())
	delete Nassert(it.get_current_node());

      return false;
    }

  for (int i = 0; i < g.get_num_nodes(); ++i)
    {
      typename GT::Arc * garc = arcs[i];

      if (garc == NULL)
	continue;

      typename GT::Node * gsrc = g.get_src_node(garc);
      typename GT::Node * tsrc = NULL;
      if (IS_NODE_VISITED(gsrc, Min))
	tsrc = static_cast<typename GT::Node*>(NODE_COOKIE(gsrc));
      else
	{
	  NODE_BITS(gsrc).set_bit(Min, true); // marcar bit

	  delete Nassert(gsrc);

	  tsrc = tree.insert_node(gsrc->get_info());
	  GT::map_nodes(gsrc, tsrc);
	}

      typename GT::Node * gtgt = g.get_tgt_node(garc);
      typename GT::Node * ttgt = NULL;
      if (IS_NODE_VISITED(gtgt, Min))
	ttgt = static_cast<typename GT::Node*>(NODE_COOKIE(gtgt));
      else
	{
	  NODE_BITS(gtgt).set_bit(Min, true); // marcar bit

	  delete Nassert(gtgt);

	  ttgt = tree.insert_node(gtgt->get_info());
	  GT::map_nodes(gtgt, ttgt);
	}

      typename GT::Arc * tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
      GT::map_arcs(garc, tarc);
      ARC_BITS(garc).set_bit(Min, true);
    }

  return true; // no hay ciclos negativos
}

    template <class GT, class Distance> inline
bool write_q_bellman_ford_min_spanning_tree(GT &                g,
					    typename GT::Node * start_node, 
					    GT &                tree)
{
  return write_q_bellman_ford_min_spanning_tree
    <GT, Distance, Aleph::less<typename Distance::Distance_Type>,
     Aleph::plus<typename Distance::Distance_Type> > (g, start_node, tree);
}


int main() 
{
  Grafo g;

  build_test_graph(g);

  Grafo tree3;

  if (not write_q_bellman_ford_min_spanning_tree <Grafo, Distancia> 
      (g, g.search_node(Nodo("A")), tree3))
    cout << "Error: hay un ciclo negativo";

  if (not write_bellman_ford_min_spanning_tree <Grafo, Distancia> 
      (g, g.search_node(Nodo("A")), tree3))
    cout << "Error: hay un ciclo negativo";

}
