

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
 }

  typedef int Distance_Type;

  Distance_Type & get_distance() { return distancia; }

  static const Distance_Type Zero_Distance = 0;
};


typedef Graph_Node<__Nodo> Nodo;

typedef Graph_Arc<__Arco> Arco;

typedef List_Graph<Nodo, Arco> Mapa;


struct Distancia
{
  typedef int Distance_Type;

  static const Distance_Type Zero_Distance = 0;

  int operator () (Mapa::Arc * a) const
  {
    return a->get_info().distancia;
  }
};


//typedef List_Graph<Nodo_Ciudad, Arco_Via> Mapa;

struct Comparar_Arcos
{
  bool operator () (const __Arco::Distance_Type & d1, 
		    const __Arco::Distance_Type & d2)
  {
    return d1 < d2;
  }
};

struct Sumar_Arcos
{
  __Arco::Distance_Type operator () (const __Arco::Distance_Type & d1, 
				     const __Arco::Distance_Type & d2)
  {
    return d1 + d2;
  }
};

Mapa::Node * buscar_nodo(Mapa & mapa, const string & nombre)
{
  return mapa.search_node([nombre] (Mapa::Node * p)
			{
			  return p->get_info().nombre == nombre;
			});
}

void insertar_arco(Mapa & mapa, 
		   const string & c1, const string & c2,
		   const int & distancia)
{
  Mapa::Node * n1 = buscar_nodo(mapa, c1);

  if (n1 == NULL)
    n1 = mapa.insert_node(c1);

  Mapa::Node * n2 = buscar_nodo(mapa, c2);

  if (n2 == NULL)
    n2 = mapa.insert_node(c2);

  string nombre_arco = n1->get_info().nombre + "--" + n2->get_info().nombre;

  mapa.insert_arc(n1, n2, __Arco(nombre_arco, distancia));
}

void construir_mapa(Mapa & g)
{
  insertar_arco(g, "A", "B", 90);
  insertar_arco(g, "A", "C", 80);
  insertar_arco(g, "A", "D", 20);
  insertar_arco(g, "A", "F", 30);
  insertar_arco(g, "A", "G", 40);

  insertar_arco(g, "B", "C", 70);

  insertar_arco(g, "C", "D", 20);
  insertar_arco(g, "C", "K", 50);

  insertar_arco(g, "D", "K", 80);
  insertar_arco(g, "D", "J", 20);
  insertar_arco(g, "D", "F", 40);

  insertar_arco(g, "F", "G", 20);
  insertar_arco(g, "F", "H", 20);
  insertar_arco(g, "F", "J", 60);
  insertar_arco(g, "F", "I", 40);

  insertar_arco(g, "G", "H", 20);

  insertar_arco(g, "H", "I", 40);
  insertar_arco(g, "H", "W", 30);

  insertar_arco(g, "I", "J", 10);
  insertar_arco(g, "I", "M", 20);
  insertar_arco(g, "I", "W", 30);

  insertar_arco(g, "J", "K", 50);
  insertar_arco(g, "J", "P", 30);
  insertar_arco(g, "J", "L", 120);
  insertar_arco(g, "J", "M", 40);

  insertar_arco(g, "K", "Q", 90);
  insertar_arco(g, "K", "P", 10);

  insertar_arco(g, "L", "P", 30);
  insertar_arco(g, "L", "S", 60);
  insertar_arco(g, "L", "M", 10);
  insertar_arco(g, "L", "O", 40);

  insertar_arco(g, "M", "O", 80);
  insertar_arco(g, "M", "X", 60);
  insertar_arco(g, "M", "W", 20);

  insertar_arco(g, "O", "S", 40);
  insertar_arco(g, "O", "T", 20);
  insertar_arco(g, "O", "X", 40);

  insertar_arco(g, "P", "Q", 10);
  insertar_arco(g, "P", "R", 60);
  insertar_arco(g, "P", "S", 50);

  insertar_arco(g, "Q", "V", 40);
  insertar_arco(g, "Q", "R", 10);

  insertar_arco(g, "R", "V", 20);
  insertar_arco(g, "R", "U", 50);
  insertar_arco(g, "R", "S", 30);

  insertar_arco(g, "S", "U", 50);
  insertar_arco(g, "S", "Y", 30);
  insertar_arco(g, "S", "T", 20); 

  insertar_arco(g, "T", "X", 20);
  insertar_arco(g, "T", "Y", 100);
  insertar_arco(g, "T", "Z", 40);

  insertar_arco(g, "U", "V", 90);
  insertar_arco(g, "U", "Y", 20);

  insertar_arco(g, "W", "X", 60);

  insertar_arco(g, "X", "Z", 30);

  insertar_arco(g, "Y", "Z", 200);
}

void imprimir_mapa(Mapa & g)
{
  cout << endl
       << "Listado de nodos (" << g.get_num_nodes() << ")" << endl;

  for (Mapa::Node_Iterator node_itor(g); node_itor.has_curr(); 
       node_itor.next())
    cout << INDENT << node_itor.get_current_node()->get_info().nombre << endl;

  cout << endl
       << endl
       << "Listado de arcos (" << g.get_num_arcs() << ")"
       << endl;

  for (Mapa::Arc_Iterator arc_itor(g); arc_itor.has_curr();
       arc_itor.next())
    {
      Mapa::Arc * arc = arc_itor.get_current_arc();
      cout << arc->get_info().nombre << " " << arc->get_info().distancia 
	   << " de " << g.get_src_node(arc)->get_info().nombre 
	   << " a " << g.get_tgt_node(arc)->get_info().nombre << endl;
    }

  cout << endl
       << endl
       << "Listado del grafo por nodos y en cada nodo por arcos" 
       << endl;
  for (Mapa::Node_Iterator node_itor(g); node_itor.has_curr(); 
       node_itor.next())
    {
      Mapa::Node * src_node = node_itor.get_current_node();
      cout << src_node->get_info().nombre << endl;
      for (Mapa::Node_Arc_Iterator itor(node_itor.get_current_node());
	   itor.has_curr(); itor.next())
	{
	  Mapa::Arc * arc = itor.get_current_arc();
	  cout << INDENT << arc->get_info().distancia << " " 
	       << g.get_connected_node(arc, src_node)->get_info().nombre 
	       << endl;
	}
    }
  cout << endl;
}

int main()
{
  Mapa g;

  construir_mapa(g);

  Mapa tree3;
  Dijkstra_Min_Paths<Mapa, Distancia> () (g, buscar_nodo(g, "L"), tree3);
  cout << endl
       << "Arbol abarcador segun Dijkstra" << endl;
  imprimir_mapa(tree3);


}
