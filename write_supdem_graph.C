
# include <autosprintf.h>
# include <generate_graph.H>
# include <tpl_net_sup_dem.H>

typedef Net_Sup_Dem_Node<string, long> Nodo;

typedef Net_Arc<Empty_Class, long> Tubo;


typedef Net_Sup_Dem_Graph<Nodo, Tubo> Red;


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
  red.insert_node("G", -4);
  red.insert_node("J");
  red.insert_node("L", -3);
  red.insert_node("A");
  red.insert_node("B");
  red.insert_node("F", -5);
  red.insert_node("H");
  red.insert_node("M", -15);
  red.insert_node("D", -6);
  red.insert_node("E");
  red.insert_node("I", -4);
  red.insert_node("K");

  crear_tubo(red, "A", "C", 5);
  crear_tubo(red, "A", "B", 7);
  crear_tubo(red, "A", "E", 3);
  crear_tubo(red, "A", "D", 6);

  crear_tubo(red, "B", "C", 5);
  crear_tubo(red, "B", "F", 1);

  crear_tubo(red, "C", "F", 3);

  crear_tubo(red, "D", "E", 6);

  crear_tubo(red, "E", "B", 4);
  crear_tubo(red, "E", "F", 5);
  crear_tubo(red, "E", "I", 8);

  crear_tubo(red, "F", "G", 5);
  crear_tubo(red, "F", "J", 7);
  crear_tubo(red, "F", "L", 6);
  crear_tubo(red, "F", "H", 1);

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
  red.insert_node("G", -4);
  red.insert_node("J");
  red.insert_node("L", -3);
  red.insert_node("A");
  red.insert_node("B");
  red.insert_node("F", -5);
  red.insert_node("H");
  red.insert_node("M", -15);
  red.insert_node("D", -6);
  red.insert_node("E");
  red.insert_node("I", -4);
  red.insert_node("K");

  crear_tubo(red, "A", "C", 5);
  crear_tubo(red, "A", "B", 7);
  crear_tubo(red, "A", "E", 9); // 3 - 9
  crear_tubo(red, "A", "D", 9); // 6

  crear_tubo(red, "B", "C", 5);
  crear_tubo(red, "B", "F", 8); // 1

  crear_tubo(red, "C", "F", 11); // 3

  crear_tubo(red, "D", "E", 6);

  crear_tubo(red, "E", "B", 4);
  crear_tubo(red, "E", "F", 5);
  crear_tubo(red, "E", "I", 8);

  crear_tubo(red, "F", "G", 5);
  crear_tubo(red, "F", "J", 7);
  crear_tubo(red, "F", "L", 6);
  crear_tubo(red, "F", "H", 1);

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
  
    template <class Net> static 
void write_node_attrs(Net & net, ofstream & o)
{
  o << endl;

  typename Net::Node_Iterator it(net); 

  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      typename Net::Node * p = it.get_current_node();

      o << "tag " << i << gnu::autosprintf(" %ld N 0 10\n", p->supply_flow);
    }
}

struct Contenido_Nodo
{
  string operator () (Red::Node * p) const 
  {
    return p->get_info();
  }
};


struct Contenido_Arco
{
  string operator () (Red::Arc * a) const
  { 
    return gnu::autosprintf("%ld/%ld", a->cap, a->flow);
  }
};

struct Sombra_Nodo
{
  string operator () (Red::Node *) const
  {
    return ""; 
  }
};

struct Sombra_Arco
{
  string operator () (Red::Arc *) const
  {
    return "ARC"; 
  }
};


Red::Aux_Net * red_ptr = NULL;
Aleph::set<Red::Aux_Net::Node*> * __vs_ptr = NULL;
Aleph::set<Red::Aux_Net::Node*> * __vt_ptr = NULL;


DynDlist<Red::Node*> * supply_ptr = NULL;
DynDlist<Red::Node*> * demand_ptr = NULL;

struct Nodo_Cruce
{
 string operator () (Red::Node * p) const
  {
    if (red_ptr == NULL or __vs_ptr == NULL or __vt_ptr == NULL)
      return "";

    if (sequential_search(*demand_ptr, p) != NULL)
      return "Shadow-node";

    return ""; 
  }
};

struct Arco_Cruce
{
  string operator () (Red::Arc * a) const
  {
    if (red_ptr == NULL or __vs_ptr == NULL or __vt_ptr == NULL)
      return "Arc";

    Red::Aux_Net::Node * src = (Red::Aux_Net::Node*) red_ptr->get_src_node(a);
    Red::Aux_Net::Node * tgt = (Red::Aux_Net::Node*) red_ptr->get_tgt_node(a);

    if (__vs_ptr->count(src) > 0 and __vt_ptr->count(tgt) > 0)
      return "Shadow-Arc";

    if (__vt_ptr->count(src) > 0 and __vs_ptr->count(tgt) > 0)
      return "Dashed-Arc";

    return "ARC"; 
  }
};


int main()
{
  {
    Net_Sup_Dem_Graph<Nodo, Tubo> red; 

    crear_red(red);

    Net_Sup_Dem_Graph<Nodo, Tubo> red_c = red;

    {
      ofstream out("red-sup-dem-0.mf", ios::out);
      generate_cross_graph
	<Red, Contenido_Nodo, Contenido_Arco, Sombra_Nodo, Sombra_Arco>
	(red, 5, 100, 100, out); 
      write_node_attrs(red, out);
    }

    Red::Aux_Net * aux = red.compute_aux_net();

    typedef Red::Aux_Net Aux_Type;

    Aleph::set<Aux_Type::Node*> vs, vt; 

    red_ptr = aux;
    __vs_ptr = &vs;
    __vt_ptr = &vt;

    DynDlist<Aux_Type::Arc *> cuts, cutt;

    min_cut <Aux_Type, Heap_Preflow_Maximum_Flow> (*aux, vs, vt, cuts, cutt);

    DynDlist<Red::Node*> supply;
    DynDlist<Red::Node*> demand;

    supply_ptr = &supply;
    demand_ptr = &demand;

    red.non_feasible_nodes(supply, demand);

    cout << "Supply ";
    for (DynDlist<Aux_Type::Node*>::Iterator it(supply); it.has_current(); 
	 it.next())
      cout << it.get_current()->get_info() << " ";
    cout << endl
	 << "Demand ";

    for (DynDlist<Aux_Type::Node*>::Iterator it(demand); it.has_current(); 
	 it.next())
      cout << it.get_current()->get_info() << " ";
    cout << endl;

    red.free_aux_net();

    {
      ofstream out("red-sup-dem-1.mf", ios::out);
      generate_cross_graph
	<Red, Contenido_Nodo, Contenido_Arco, Nodo_Cruce, Arco_Cruce,
	No_Res_Arc<Red> >
 	(red, 5, 100, 100, out); 
      write_node_attrs(red, out);
    }

    red_ptr = &red_c;
    vs.clear();
    vt.clear();
    cuts.empty();
    cutt.empty();
    supply.empty();
    demand.empty(); 

    min_cut <Red, Heap_Preflow_Maximum_Flow> (red_c, vs, vt, cuts, cutt);
    red_c.non_feasible_nodes(supply, demand);

    {
      ofstream out("red-sup-dem-2.mf", ios::out);
      generate_cross_graph
	<Red, Contenido_Nodo, Contenido_Arco, Nodo_Cruce, Arco_Cruce, 
	No_Res_Arc<Red> > (red_c, 5, 100, 100, out); 
      write_node_attrs(red, out);
    }
  }

  {
    red_ptr = NULL;
    __vs_ptr = NULL;
    __vt_ptr = NULL;
    Net_Sup_Dem_Graph<Nodo, Tubo> red; 

    crear_otra_red(red); 

    Net_Sup_Dem_Graph<Nodo, Tubo> red_c = red;

    {
      ofstream out("red-sup-dem-3.mf", ios::out);
      generate_cross_graph
	<Red, Contenido_Nodo, Contenido_Arco, Sombra_Nodo, Sombra_Arco>
	(red, 5, 100, 100, out); 
      write_node_attrs(red, out);
    }

    Red::Aux_Net * aux = red.compute_aux_net();

    typedef Red::Aux_Net Aux_Type;

    Aleph::set<Aux_Type::Node*> vs, vt; 
    DynDlist<Aux_Type::Arc *> cuts, cutt;

    red_ptr = aux;
    __vs_ptr = &vs;
    __vt_ptr = &vt;

    min_cut <Aux_Type, Heap_Preflow_Maximum_Flow> (*aux, vs, vt, cuts, cutt);

    DynDlist<Red::Node*> supply;
    DynDlist<Red::Node*> demand;

    supply_ptr = &supply;
    demand_ptr = &demand;

    red.non_feasible_nodes(supply, demand);

    cout << "Supply ";
    for (DynDlist<Aux_Type::Node*>::Iterator it(supply); it.has_current(); it.next())
      cout << it.get_current()->get_info() << " ";
    cout << endl
	 << "Demand ";

    for (DynDlist<Aux_Type::Node*>::Iterator it(demand); it.has_current(); it.next())
      cout << it.get_current()->get_info() << " ";
    cout << endl;

    red.free_aux_net(); 

    {
       ofstream out("red-sup-dem-4.mf", ios::out);
       generate_cross_graph
	 <Red, Contenido_Nodo, Contenido_Arco, Nodo_Cruce, Arco_Cruce ,
	 No_Res_Arc<Red> >
 	(red, 5, 100, 100, out); 
       write_node_attrs(red, out);
     }

    red_ptr = &red_c;
    cuts.empty();
    cutt.empty();
    supply.empty();
    demand.empty(); 

    min_cut <Red, Heap_Preflow_Maximum_Flow> (red_c, vs, vt, cuts, cutt);

    red_c.non_feasible_nodes(supply, demand);

    {
      ofstream out("red-sup-dem-5.mf", ios::out);
      generate_cross_graph
	<Red, Contenido_Nodo, Contenido_Arco, Nodo_Cruce, Arco_Cruce, 
	No_Res_Arc<Red> >
 	(red_c, 5, 100, 100, out); 
      write_node_attrs(red, out);
    }
  }
}



