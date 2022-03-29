

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
  const DynList<Grafo::Node*> node_list = compute_cut_nodes(g);

  auto n = paint_subgraphs<Grafo>(g, node_list);

  depth_first_traversal(g, src, &visitar_df); // copiar df

  depth_first_traversal(g, src, &visitar_low); // copiar low

  Grafo tree = find_depth_first_spanning_tree (g, src);

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
  {
    Grafo g; // grafo con un punto de corte

    construir_grafo(g); 

    ofstream f("test-cut.Tree", ios::trunc);

    write_df_low_tree(g, g.get_first_node(), f);

  }
}
