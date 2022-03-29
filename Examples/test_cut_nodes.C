

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

 
void generate_low(Grafo & g, DynDlist<No_Tree_Arc> & list)
{
      // inicialmente cada low(node) = df(node)
  for (Grafo::Node_Iterator i(g); i.has_curr(); i.next())
    i.get_current_node()->get_info().low = i.get_current_node()->get_info().df;

      // revisar arcos no-abacadores en búsqueda de df menores
  for (DynDlist<No_Tree_Arc>::Iterator it(list); it.has_curr(); it.next())
    {
      No_Tree_Arc non_arc = it.get_curr();

      Grafo::Node *& gsrc = non_arc.first;
      Grafo::Node *& gtgt = non_arc.second;

      long min_low = std::min(gsrc->get_info().df,  gtgt->get_info().df);
      
      gtgt->get_info().low = std::min(gtgt->get_info().low, min_low);

      gsrc->get_info().low = std::min(gtgt->get_info().df, min_low);
    }
}


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

void imprimir_arcos(Grafo & g, const long & color)
{
  cout << "Listado de arcos con color " << color << endl;
  for (Grafo::Arc_Iterator it(g); it.has_curr(); it.next())
    {
      Grafo::Arc * arc = it.get_current_arc();

      if (g.get_counter(arc) == color)
	cout << "Arco de " << g.get_src_node(arc)->get_info().clave
	     << " a " << g.get_tgt_node(arc)->get_info().clave << endl;
    }
  cout << endl;
}

void imprimir_arcos_corte(Grafo & g)
{
  cout << "Listado de arcos de corte *** " << endl;
  for (Grafo::Arc_Iterator it(g); it.has_curr(); it.next())
    {
      Grafo::Arc * arc = it.get_current_arc();

      if (g.get_control_bits(arc).get_bit(Aleph::Cut))
	cout << "Arco de " << g.get_src_node(arc)->get_info().clave
	     << " a " << g.get_tgt_node(arc)->get_info().clave 
	     << " con color " << g.get_counter(arc) << endl;
    }
  cout << endl;
}


void write_df_low_tree(Grafo & g, Grafo::Node * src, ofstream & f)
{
      // calcular puntos de corte
  DynDlist<Grafo::Node*> node_list;
  Compute_Cut_Nodes <Grafo> comp(g);
  comp(node_list);

  depth_first_traversal(g, src, &visitar_df); // copiar df

  depth_first_traversal(g, src, &visitar_low); // copiar low

  Grafo tree = find_depth_first_spanning_tree(g, src);

      // calcular arcos no abarcadores
  DynDlist<No_Tree_Arc> arc_list;
  generate_non_tree_arcs(g, arc_list);

  Grafo::Node * td = static_cast<Grafo::Node *>(NODE_COOKIE(src));

  Convertir cuenta;
  Filtra_Arco filtro;
  Tree_Node<Clave> * rd =  
    Graph_To_Tree_Node <Grafo, Clave, Convertir, Filtra_Arco> (filtro) 
    (tree, td, cuenta);

  cout << "**** Se vieron " << filtro.count << " arcos" << endl;
  cout << "**** Se vieron " << cuenta.count << " arcos" << endl;

  generate_tree <Tree_Node<Clave>, Write_Low> (rd, f);

  write_non_tree_arcs(arc_list, rd, f);

  destroy_tree(rd);
}



int main()  
{
  {
    Grafo g; // grafo con un punto de corte

    construir_grafo(g); 

    ofstream f("test-cut.Tree", ios::trunc);

    write_df_low_tree(g, g.get_first_node(), f);

    DynDlist<Grafo::Node*> node_list;
    Filtra_Arco filtro;
    Compute_Cut_Nodes <Grafo, Filtra_Arco> cut_nodes (g, filtro);
    cut_nodes(node_list);
    cout << "Se vieron " << filtro.count << " arcos" << endl;

    cout << "Nodos de corte: ";
    Grafo::Node * p[20]; int i = 0;
    for (DynDlist<Grafo::Node*>::Iterator it(node_list); 
	 it.has_curr(); it.next())
      {
	p[i] = it.get_curr();
	cout << p[i++]->get_info().clave << " ";
      }
    cout << endl;
  }

  {
    Grafo g; // grafo con un punto de corte

    construir_grafo(g); 

    DynDlist<Grafo> blocks;
    Grafo cut_g;
    DynDlist<Grafo::Arc*> cross_list;

    DynDlist<Grafo::Node*> node_list;
    Filtra_Arco filtro;
    Compute_Cut_Nodes <Grafo, Filtra_Arco> cut_nodes (g, filtro);
    cut_nodes(node_list);

    int colors = cut_nodes.paint_subgraphs();

    imprimir_grafo(g);

    cout << "Componentes conexos ****************" << endl;
    for (int i = 1; i < colors; i++)
      {
	Grafo sub_color;
	cut_nodes.map_subgraph(sub_color, i);
	cout << "Componente " << i << endl;
	imprimir_grafo(sub_color);
      }

    cout << "Grafo de corte " << endl;
    Grafo grafo_corte;

    DynDlist<Grafo::Arc*> cross_arc_list;

    cut_nodes.map_cut_graph(grafo_corte, cross_arc_list);
    
    imprimir_grafo(grafo_corte);
  }
    
}
