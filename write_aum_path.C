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

struct Sombra_Arco
{
  string operator () (Red::Arc * a) const
  {
    if (path_ptr == NULL)
      return "Arc";

    return path_ptr->contains_arc(a) ? "SHADOW-ARC" : "ARC"; 
  }
};

struct Nodo_String
{
  string operator () (Red::Node * p) const { return p->get_info(); }
};


struct Arco_Normal
{
  string operator () (Red::Arc * a) const
  { 
    return gnu::autosprintf("%ld/%ld", a->cap, a->flow);
  }
};



int main()
{
  typedef No_Res_Arc<Red> No_Res;
  {
    Net_Graph<Nodo, Tubo> red; 

    crear_red(red);

    Path<Red> path(red);
    path_ptr = &path;

    Find_Aumenting_Path <Red, Find_Path_Depth_First> () (red, path, 1);
    increase_flow <Red> (red, path, 1);
    {
      ofstream out1("aum-3.mf", ios::out);
      generate_cross_graph
	<Red, Nodo_String, Arco_Normal, Sombra_Nodo, Sombra_Arco, No_Res>
	(red, 5, 100, 100, out1);
    }

    Find_Aumenting_Path <Red, Find_Path_Depth_First> () (red, path, 3);
    increase_flow <Red> (red, path, 3);
    {
      ofstream out1("aum-4.mf", ios::out);
      generate_cross_graph
	<Red, Nodo_String, Arco_Normal, Sombra_Nodo, Sombra_Arco, No_Res>
	(red, 5, 100, 100, out1);
    }

 
    Find_Aumenting_Path <Red> () (red, path, 2);
    increase_flow <Red> (red, path, 2);
    {
      ofstream out1("aum-1.mf", ios::out);
      generate_cross_graph
	<Red, Nodo_String, Arco_Normal, Sombra_Nodo, Sombra_Arco, No_Res>
	(red, 5, 100, 100, out1);
    }

    
    Find_Aumenting_Path <Red> () (red, path, 4);
    increase_flow <Red> (red, path, 4);
    {
      ofstream out1("aum-2.mf", ios::out);
      generate_cross_graph
	<Red, Nodo_String, Arco_Normal, Sombra_Nodo, Sombra_Arco, No_Res>
	(red, 5, 100, 100, out1);
    }


    red.check_network();
  }

}



