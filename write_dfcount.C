
# include <iostream>
# include <tpl_dynDlist.H>
# include <tpl_graph_utils.H>
# include <graph_to_tree.H>
# include <generate_tree.H>

using namespace Aleph;

struct Empty_Arc {};

struct _Node
{
  char clave;
  long df;
  long low;

  _Node() {}

  _Node(const char & c) : clave(c), df(0), low(-1) {}
};

struct node_igual
{
  bool operator () (const _Node & n1, const _Node & n2)
  {
    return n1.clave == n2.clave;
  }
};


typedef Graph_Node<_Node> Nodo;

typedef Graph_Arc<Empty_Arc> Arco;

typedef List_Graph<Nodo, Arco> Grafo;

struct _Reset_Node
{
  void operator () (Grafo&, Grafo::Node * p)
  {
    p->get_info().df = 0;
    p->get_info().low = -1;
  }
};

void reset_grafo(Grafo & g)
{
  Operate_On_Nodes <Grafo, _Reset_Node> () (g);
}

Grafo::Node * buscar_nodo(Grafo & g, char c)
{
  return g.search_node([c] (auto p) { return p->get_info().clave == c; });
}

void construir_grafo1(Grafo & gr)
{
  Grafo::Node * a = gr.insert_node(_Node('A'));
  Grafo::Node * b = gr.insert_node(_Node('B'));
  Grafo::Node * c = gr.insert_node(_Node('C'));
  Grafo::Node * d = gr.insert_node(_Node('D'));
  Grafo::Node * f = gr.insert_node(_Node('F'));
  Grafo::Node * g = gr.insert_node(_Node('G'));
  Grafo::Node * h = gr.insert_node(_Node('H'));
  Grafo::Node * i = gr.insert_node(_Node('I'));
  Grafo::Node * j = gr.insert_node(_Node('J'));
  Grafo::Node * k = gr.insert_node(_Node('K'));
  Grafo::Node * l = gr.insert_node(_Node('L'));

  gr.insert_arc(a, b, Empty_Arc());
  gr.insert_arc(a, c, Empty_Arc());
  gr.insert_arc(b, c, Empty_Arc());
  gr.insert_arc(b, d, Empty_Arc());
  gr.insert_arc(b, f, Empty_Arc());
  gr.insert_arc(c, g, Empty_Arc());
  gr.insert_arc(c, d, Empty_Arc());
  gr.insert_arc(d, f, Empty_Arc());
  gr.insert_arc(d, g, Empty_Arc());
  gr.insert_arc(f, g, Empty_Arc());
  gr.insert_arc(f, h, Empty_Arc());
  gr.insert_arc(g, h, Empty_Arc());
  gr.insert_arc(h, i, Empty_Arc());
  gr.insert_arc(h, l, Empty_Arc());
  gr.insert_arc(i, l, Empty_Arc());
  gr.insert_arc(i, j, Empty_Arc());
  gr.insert_arc(j, l, Empty_Arc());
  gr.insert_arc(j, k, Empty_Arc());
  gr.insert_arc(l, k, Empty_Arc());
}

void construir_grafo2(Grafo & gr)
{
  Grafo::Node * a = gr.insert_node(_Node('A'));
  Grafo::Node * b = gr.insert_node(_Node('B'));
  Grafo::Node * c = gr.insert_node(_Node('C'));
  Grafo::Node * d = gr.insert_node(_Node('D'));
  Grafo::Node * f = gr.insert_node(_Node('F'));
  Grafo::Node * g = gr.insert_node(_Node('G'));
  Grafo::Node * h = gr.insert_node(_Node('H'));
  Grafo::Node * i = gr.insert_node(_Node('I'));
  Grafo::Node * j = gr.insert_node(_Node('J'));
  Grafo::Node * k = gr.insert_node(_Node('K'));
  Grafo::Node * l = gr.insert_node(_Node('L'));

  gr.insert_arc(a, b, Empty_Arc());
  gr.insert_arc(a, c, Empty_Arc());
  gr.insert_arc(b, c, Empty_Arc());
  gr.insert_arc(b, d, Empty_Arc());
  gr.insert_arc(b, f, Empty_Arc());
  gr.insert_arc(c, g, Empty_Arc());
  gr.insert_arc(c, d, Empty_Arc());
  gr.insert_arc(d, f, Empty_Arc());
  gr.insert_arc(d, g, Empty_Arc());
  gr.insert_arc(f, g, Empty_Arc());
  gr.insert_arc(f, h, Empty_Arc());
  gr.insert_arc(f, i, Empty_Arc());
  gr.insert_arc(g, l, Empty_Arc());
  gr.insert_arc(g, h, Empty_Arc());
  gr.insert_arc(h, i, Empty_Arc());
  gr.insert_arc(h, l, Empty_Arc());
  gr.insert_arc(i, l, Empty_Arc());
  gr.insert_arc(i, j, Empty_Arc());
  gr.insert_arc(j, l, Empty_Arc());
  gr.insert_arc(j, k, Empty_Arc());
  gr.insert_arc(l, k, Empty_Arc());
}


static long global_counter = 0;

bool visitar_df(const Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().df = global_counter++;

  return false;
}

bool visitar_low(const Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().low = (long) NODE_COOKIE(nodo);

  return false;
}

bool visitar(const Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().df = global_counter++;

  return false;
}


struct Clave
{
  char key;
  long count;
  long low;
};


struct Clave_Igual
{
  bool operator () (const Clave & c1, const Clave & c2) const
  {
    return c1.key == c2.key;
  }  
};


struct Convertir
{
  void operator () (Grafo::Node * tnode, Tree_Node<Clave> * t)
  {
    Grafo::Node * gnode = static_cast<Grafo::Node *>(NODE_COOKIE(tnode));

    Clave & clave = t->get_key();
    clave.key   = tnode->get_info().clave;
    clave.count = gnode->get_info().df;
    clave.low   = gnode->get_info().low;
  }
};


struct Write_Node
{
  static const size_t Buf_Size = 512;

  const string operator () (Tree_Node<Clave> * p)
  {
    char str[2];

    str[0] = p->get_key().key;
    str[1] = '\0';
    return string(str);
  }
};


struct Write_Df
{
  static const size_t Buf_Size = 512;

  const string operator () (Tree_Node<Clave> * p)
  {
    char buf[Buf_Size];

    snprintf(buf, Buf_Size, "%c,%ld", p->get_key().key, p->get_key().count);
  
    return string(buf);
  }
};


struct Write_Low
{
  static const size_t Buf_Size = 512;

  const string operator () (Tree_Node<Clave> * p)
  {
    char buf[Buf_Size];

    if (p->get_key().low >= 0)
      snprintf(buf, Buf_Size, "%c,%ld,%ld", 
	       p->get_key().key, p->get_key().count, p->get_key().low);
    else
      snprintf(buf, Buf_Size, "%c,%ld,-", 
	       p->get_key().key, p->get_key().count);

    return string(buf);
  }
};


typedef std::pair<Grafo::Node *, Grafo::Node *> No_Tree_Arc;
    

    void 
generate_non_tree_arcs(const Grafo & g, DynDlist<No_Tree_Arc> & arc_list)
{
  for (Grafo::Arc_Iterator it(g); it.has_current(); it.next())
    {
      Grafo::Arc * arc = it.get_current_arc();

      Grafo::Arc * tarc = static_cast<Grafo::Arc*>(ARC_COOKIE(arc));

      if (tarc == NULL) // nodo fue mapeado en árbol abarcador?
	{
	      // obtener nodos origen y destino en el árbol abarcador
	  Grafo::Node * src = static_cast<Grafo::Node*>(g.get_src_node(arc));
	  Grafo::Node * tgt = static_cast<Grafo::Node*>(g.get_tgt_node(arc));
	  arc_list.append(No_Tree_Arc(src, tgt));
	}
    }
}

void write_deway_number(int deway [], const size_t & sz, ofstream & out)
{
  for (int i = 0; i < sz; ++i)
    {
      out << deway[i];

      if (i < sz - 1)
	out << ".";
    }
}

void write_non_tree_arcs(DynDlist<No_Tree_Arc> & list,
			 Tree_Node<Clave> *      tree,
			 ofstream &              out)
{
  const size_t Buf_Size = 512;

  DynDlist<No_Tree_Arc>::Iterator it(list);
  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      No_Tree_Arc arc = it.get_current();
      Grafo::Node * src = arc.first;
      Grafo::Node * tgt = arc.second;

      int deway[Buf_Size];
      size_t n = 0;

      Clave key; key.key = src->get_info().clave;
      
      Tree_Node<Clave> * node = search_deway <Tree_Node<Clave>, Clave_Igual> 
	(tree, key, deway, Buf_Size, n);

      assert(node != NULL);

      out << "Dashed-Connexion ";
      write_deway_number(deway, n, out);
      out << " ";

      key.key = tgt->get_info().clave;
      
      node = search_deway <Tree_Node<Clave>, Clave_Igual> 
	(tree, key, deway, Buf_Size, n);

      assert(node != NULL);

      write_deway_number(deway, n, out);

      if (i % 2 == 0)
	out << " L";
      else
	out << " R";

      out << " " << endl;
    }
}


void generate_low(DynDlist<No_Tree_Arc> & list)
{
  for (DynDlist<No_Tree_Arc>::Iterator it(list); it.has_current(); it.next())
    {
      No_Tree_Arc non_arc = it.get_current();

      Grafo::Node * gsrc = non_arc.first;
      Grafo::Node * gtgt = non_arc.second;
      
      if (gsrc->get_info().low == -1)
	gsrc->get_info().low = gtgt->get_info().df;
      else
	if (gtgt->get_info().df < gsrc->get_info().low)
	  gsrc->get_info().low = gtgt->get_info().df;

      if (gtgt->get_info().low == -1)
	gtgt->get_info().low = gsrc->get_info().df;
      else
	if (gsrc->get_info().df < gtgt->get_info().low)
	  gtgt->get_info().low = gsrc->get_info().df;
    }
}


void write_tree(const Grafo & g, Grafo::Node * src, ofstream & f)
{
  global_counter = 0;
  depth_first_traversal(g, src, &visitar);

  Grafo tree = find_depth_first_spanning_tree(g, src);

  Grafo::Node * tsrc = static_cast<Grafo::Node *>(NODE_COOKIE(src));

  Tree_Node<Clave> * r = 
    Graph_To_Tree_Node <Grafo, Clave, Convertir> () (tree, tsrc);

  generate_tree <Tree_Node<Clave>, Write_Node> (r, f);

  DynDlist<No_Tree_Arc> arc_list;

  generate_non_tree_arcs(g, arc_list);

  write_non_tree_arcs(arc_list, r, f);

  destroy_tree(r);
}

void write_df_low_tree(const Grafo & g, Grafo::Node * src, ofstream & f)
{
      // calcular puntos de corte
  DynList<Grafo::Node*> node_list = compute_cut_nodes(g, src);

  global_counter = 0;
  depth_first_traversal(g, src, &visitar_df); // copiar df

  depth_first_traversal(g, src, &visitar_low); // copiar low

  Grafo tree = find_depth_first_spanning_tree(g, src);

      // calcular arcos no abarcadores
  DynDlist<No_Tree_Arc> arc_list;
  generate_non_tree_arcs(g, arc_list);

  Grafo::Node * td =  static_cast<Grafo::Node *>(NODE_COOKIE(src));

  Tree_Node<Clave> * rd =  
    Graph_To_Tree_Node <Grafo, Clave, Convertir> () (tree, td);

  generate_tree <Tree_Node<Clave>, Write_Low> (rd, f);

  write_non_tree_arcs(arc_list, rd, f);

  destroy_tree(rd);
}



int main() 
{
  { // grafo con un punto de corte y sin números
    Grafo g; 

    construir_grafo1(g); 

    {
      ofstream f("dfcount-1-d.Tree", ios::trunc);

      write_tree(g, buscar_nodo(g, 'D'), f);
    }

    {
      ofstream f("dfcount-1-j.Tree", ios::trunc);
      write_tree(g, buscar_nodo(g, 'J'), f);
    }

    {
      ofstream f("dfcount-1-h.Tree", ios::trunc);
      write_tree(g, buscar_nodo(g, 'H'), f);
    }
  } 

  {
    Grafo g; // grafo sin punto de corte y sin números

    construir_grafo2(g); 

    {
      ofstream f("dfcount-2-d.Tree", ios::trunc);
      write_tree(g, buscar_nodo(g, 'D'), f);
    }

    {
      ofstream f("dfcount-2-j.Tree", ios::trunc);
      write_tree(g, buscar_nodo(g, 'J'), f);
    }

    {
      ofstream f("dfcount-2-h.Tree", ios::trunc);
      write_tree(g, buscar_nodo(g, 'H'), f);
    }
  }

{ // generación con números low
    Grafo g; // grafo con un punto de corte

    construir_grafo1(g); 

    {
      ofstream f("dfcount-3-d.Tree", ios::trunc);
      write_df_low_tree(g, buscar_nodo(g, 'D'), f);
    }

    { 
      ofstream f("dfcount-3-j.Tree", ios::trunc);
      write_df_low_tree(g, buscar_nodo(g, 'J'), f);
    }

    {
      ofstream f("dfcount-3-h.Tree", ios::trunc);
      write_df_low_tree(g, buscar_nodo(g, 'H'), f);
    }
  }


  { // generación con números low
    Grafo g; // grafo con un punto de corte

    construir_grafo2(g); 

    {
      ofstream f("dfcount-4-d.Tree", ios::trunc);
      write_df_low_tree(g, buscar_nodo(g, 'D'), f);
    }

    { 
      ofstream f("dfcount-4-j.Tree", ios::trunc);
      write_df_low_tree(g, buscar_nodo(g, 'J'), f);
    }

    {
      ofstream f("dfcount-4-h.Tree", ios::trunc);
      write_df_low_tree(g, buscar_nodo(g, 'H'), f);
    }
  }
}
