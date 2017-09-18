# include <autosprintf.h>
# include <tpl_dynBinHeap.H>
# include <Dijkstra.H>
# include <generate_spanning_tree_picture.H>

struct Nodo 
{
  string str;

  Nodo(const int & c)
  {
    str = c;
  }

  Nodo(const string & s) : str(s)
  {
    // empty
  }

  bool operator == (const Nodo & c) const
  {
    return c.str == str;
  }
};


struct Arco
{
  int w;

  Arco(const int & __w) : w(__w) { /* empty */ }

  Arco() { /* empty */ }
};


typedef List_Graph<Graph_Node<Nodo>, Graph_Arc<Arco> > Grafo;


struct Distancia
{
  typedef int Distance_Type;

  static const Distance_Type Zero_Distance = 0;

  int operator () (Grafo::Arc * a) const
  {
    return a->get_info().w;
  }
};


struct Nodo_String
{
  string operator () (Grafo::Node * p) const
  { 
    return p->get_info().str;
  }
};


struct Arco_String
{
  string operator () (Grafo::Arc * a) const
  { 
    return gnu::autosprintf("%d", a->get_info().w);
  }
};


Grafo::Node * bn(Grafo * g, const string & str)
{
  return g->search_node(Nodo(str));
}


void insertar_arco(Grafo *        g,
		   const string & s1, 
		   const string & s2, 
		   const int &    i)
{
  Grafo::Node * src = bn(g, s1);
  Grafo::Node * tgt = bn(g, s2);

  g->insert_arc(src, tgt, i); 
}


Grafo * build_graph()
{
  Grafo * g = new Grafo;

  for (int i = 'A'; i <= 'P'; ++i)
    g->insert_node(new Grafo::Node(i));

  insertar_arco(g, "A", "B", 3);
  //  insertar_arco(g, "B", "A", 3);
  insertar_arco(g, "B", "C", 4);
  insertar_arco(g, "C", "D", 3);
  insertar_arco(g, "D", "E", 4);
  insertar_arco(g, "E", "K", 2);
  insertar_arco(g, "K", "P", 7);
  insertar_arco(g, "P", "J", 1);
  insertar_arco(g, "J", "K", 4);
  insertar_arco(g, "K", "D", 5);
  insertar_arco(g, "D", "J", 2);
  insertar_arco(g, "J", "I", 2);
  //  insertar_arco(g, "I", "J", 2);
  insertar_arco(g, "I", "D", 1);
  insertar_arco(g, "I", "C", 2);
  insertar_arco(g, "I", "H", 2);
  insertar_arco(g, "H", "C", 3);
  insertar_arco(g, "H", "B", 7);
  insertar_arco(g, "B", "G", 1);
  insertar_arco(g, "B", "M", 15);
  //  insertar_arco(g, "M", "B", 3);
  insertar_arco(g, "M", "G", 10);
  insertar_arco(g, "G", "A", 4);
  insertar_arco(g, "A", "F", 9);
  insertar_arco(g, "F", "G", 5);
  insertar_arco(g, "F", "L", 10);
  insertar_arco(g, "F", "M", 12);
  insertar_arco(g, "H", "M", 8);
  insertar_arco(g, "L", "M", 4);
  insertar_arco(g, "M", "N", 2);
  insertar_arco(g, "N", "H", 3);
  insertar_arco(g, "N", "I", 1);
  insertar_arco(g, "N", "O", 3);
  insertar_arco(g, "O", "I", 3);
  insertar_arco(g, "O", "J", 1);
  insertar_arco(g, "O", "P", 6);

  return g;    
}


    // conversión de cookie a Node_Info 
# define DNassert(p) (static_cast<Dijkstra_Node_Info<GT, Distance>*>(NODE_COOKIE((p))))

    // Acceso al nodo del árbol en el grafo
# define TREENODE(p) (DNassert(p)->tree_node)

    // Acceso a la distancia acumulada
# define ACC(p) (DNassert(p)->dist)
# define DAassert(p) (static_cast<Dijkstra_Arc_Info<GT, Distance>*>(ARC_COOKIE(p)))
# define ARC_DIST(p) (Distance() (p))
# define TREEARC(p) (DAassert(p)->tree_arc)
# define POT(p) (DAassert(p)->pot)

template <typename GT, class Distance> 
struct Shade_Dijkstra_Node
{
  string operator () (typename GT::Node * p) 
  {
    return TREENODE(p) == NULL ? "" : "SHADOW-NODE";
  }
};


template <typename GT, class Distance> 
struct Shade_Dijkstra_Arc
{
  string operator () (typename GT::Arc * a) const
  {
    return TREEARC(a) == NULL ? "ARC" : "SHADOW-ARC"; 
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
void write_dijkstra_min_spanning_tree(GT &                g,
				      typename GT::Node * start_node, 
				      GT &                tree)
{
  clear_graph(tree); // limpiar árbol abarcador destino 

  Operate_On_Nodes <GT,Initialize_Dijkstra_Node <GT, Distance> > () (g);

  Operate_On_Arcs <GT, Initialize_Dijkstra_Arc<GT, Distance> > () (g);

  ACC(start_node) = Distance::Zero_Distance;

        // inserte start_node en árbol abarcador y mapee
  TREENODE(start_node) = tree.insert_node(start_node->get_info());
  NODE_BITS(start_node).set_bit(Aleph::Dijkstra, true); 
  NODE_COOKIE(TREENODE(start_node)) = start_node;

  {
    // bloque adicional para asegurar que el heap se destruya antes que
    // los bloques almacenados en los cookies
    DynBinHeap<typename GT::Arc*, Compare_Arc_Data<GT,Compare,Distance> > heap;

    for (typename GT::Node_Arc_Iterator it(start_node); 
	 it.has_current(); it.next())
      {
        typename GT::Arc * arc = it.get_current_arc();

        ARC_BITS(arc).set_bit(Aleph::Dijkstra, true); 

        POT(arc) = ARC_DIST(arc);

        heap.insert(arc);
      }

    int i = 0, j = 0;

    while (tree.get_num_nodes() < g.get_num_nodes()) 
      {
        typename GT::Arc * garc = heap.getMin(); // obtener arco menor potencial

            // nodos conectados por el arco
        typename GT::Node * gsrc = g.get_src_node(garc);
        typename GT::Node * gtgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(gsrc, Aleph::Dijkstra) and 
            IS_NODE_VISITED(gtgt, Aleph::Dijkstra))
          continue; // ambos visitados ==> insertar este arco causaría ciclo
            
        typename GT::Node * new_node = // seleccionar nodo no visitado
          IS_NODE_VISITED(gsrc, Aleph::Dijkstra) ? gtgt : gsrc;

	assert(TREENODE(g.get_connected_node(garc, new_node)) != NULL);

            // insertar nuevo nodo en árbol y mapear
        typename GT::Node * ttgt = tree.insert_node(new_node->get_info());
        NODE_BITS(new_node).set_bit(Aleph::Dijkstra, true);
	TREENODE(new_node) = ttgt;

	assert(TREENODE(gsrc) != NULL and TREENODE(gtgt) != NULL);

        typename GT::Arc * tarc = // inserte nuevo arco en tree y mapear
          tree.insert_arc(TREENODE(gsrc), TREENODE(gtgt), garc->get_info());
	TREEARC(garc) = tarc;

	if (i++ % 3 == 0)
	  {
	    string name = gnu::autosprintf("dijkstra-%d-aux.gra", j++);
	    ofstream out(name, ios::out);

	    generate_cross_graph
	      <Grafo, Nodo_String, Arco_String, 
	      Shade_Dijkstra_Node<GT, Distance>, Shade_Dijkstra_Arc<GT, Distance> >
	        (g, 6, 20, 20, out);

	    write_acum<GT, Distance>(g, out);
	  }

            // actualizar distancia recorrida desde nodo inicial 
        ACC(new_node) = POT(garc);
        const typename Distance::Distance_Type & acc = ACC(new_node);

            // por cada arco calcular potencial e insertarlo en heap
        for (typename GT::Node_Arc_Iterator it(new_node); 
	     it.has_current(); it.next())
          {
            typename GT::Arc * arc = it.get_current_arc();

            if (IS_ARC_VISITED(arc, Aleph::Dijkstra))
              continue;

            ARC_BITS(arc).set_bit(Aleph::Dijkstra, true);

            POT(arc) = Plus () (acc, ARC_DIST(arc)); // calcula potencial

            heap.insert(arc);
          }
      }

    string name = gnu::autosprintf("dijkstra-%d-aux.gra", j++);
    ofstream out(name, ios::out);

    generate_cross_graph
      <Grafo, Nodo_String, Arco_String, 
	  Shade_Dijkstra_Node<GT, Distance>, Shade_Dijkstra_Arc<GT, Distance> >
        (g, 6, 20, 20, out);
    write_acum<GT, Distance>(g, out);
}

  Operate_On_Arcs <GT, Destroy_Dijkstra_Arc<GT, Distance> > () (g);

  Operate_On_Nodes <GT, Destroy_Dijkstra_Node <GT, Distance> > () (g);
}

    template <class GT, class Distance> inline
void write_dijkstra_min_spanning_tree(GT &                g,
                                typename GT::Node * start_node, 
                                GT &                tree)
{
  write_dijkstra_min_spanning_tree<GT, Distance, 
    Aleph::less<typename Distance::Distance_Type>,
    Aleph::plus<typename Distance::Distance_Type> > 
  (g, start_node, tree);
}


# undef DNI
# undef TREENODE
# undef ACC
# undef DAI
# undef ARC_DIST
# undef TREEARC
# undef POT





int main()
{
  Grafo * g = build_graph();

  Grafo tree1;

  write_dijkstra_min_spanning_tree<Grafo, Distancia>(*g, 
						     g->get_first_node(),
						     tree1);
}

