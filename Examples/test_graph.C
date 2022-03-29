

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
 [] (Mapa::Arc * a)
	   {
	     cout << "    " << a->get_info().nombre << endl;
	   });
       });
   }
}


int main()
{
  Mapa g;

  construir_mapa(g);

  test_functional(g);

  {
    cout << "Recorrido en profundidad " << endl;
    size_t n = Depth_First_Traversal <Mapa, Visitar> () (g, Visitar());
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en profundidad " << endl;
    n = Depth_First_Traversal <Mapa, Visitar> () 
      (g, Visitar(buscar_ciudad(g, "Paraguaipos")));
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en profundidad " << endl;
    n = Depth_First_Traversal <Mapa> () (g);
    cout << n << " nodos visitados" << endl;
  }

  {
    cout << "Recorrido en amplitud " << endl;
    size_t n = breadth_first_traversal (g, &visitar);
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en amplitud " << endl;
    n = Breadth_First_Traversal<Mapa, Visitar> () (g, Visitar());
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en amplitud " << endl;
    n = Breadth_First_Traversal<Mapa, Visitar> () 
      (g, Visitar(buscar_ciudad(g, "Carora")));
    cout << n << " nodos visitados" << endl;
  }

  {
    if (Test_Connectivity <Mapa> () (g))
      cout << "El grafo es conectado";
    else
      cout << "El grafo no es conectado";
    cout << endl;
  }

  {
    Contar_Arcos contar;
    if (Test_Connectivity <Mapa, Contar_Arcos> () (g, contar))
      cout << "El grafo es conectado";
    else
      cout << "El grafo no es conectado";
    cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    if (Test_For_Cycle <Mapa> () (g, buscar_ciudad(g, "San Cristobal")))
      cout << "Hay un ciclo desde San Cristobal" << endl;
    else
      cout << "No hay un ciclo desde San Cristobal" << endl;
  }

  {
    Contar_Arcos contar;
    if (Test_For_Cycle <Mapa, Contar_Arcos> (contar) 
	(g, buscar_ciudad(g, "San Cristobal")))
      cout << "Hay un ciclo desde San Cristobal" << endl;
    else
      cout << "No hay un ciclo desde San Cristobal" << endl;
    cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    if (Is_Graph_Acyclique <Mapa> () (g))
      cout << "El grafo es aciclico (es un arbol)" << endl;
    else
      cout << "El grafo tiene ciclos" << endl;
  }

  {
    if (Is_Graph_Acyclique <Mapa> () (g))
      cout << "El grafo es aciclico (es un arbol)" << endl;
    else
      cout << "El grafo tiene ciclos" << endl;
  }

  {
    Contar_Arcos contar;
    if (Is_Graph_Acyclique <Mapa, Contar_Arcos> (contar) (g))
      cout << "El grafo es aciclico (es un arbol)" << endl;
    else
      cout << "El grafo tiene ciclos" << endl;
    cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    if (Test_For_Path <Mapa> () (g, buscar_ciudad(g, "El Vigia"), 
				 buscar_ciudad(g, "Carora")))
      cout << "Existe un camino desde El Vigia hacia Carora" << endl;
  }

  {
     Contar_Arcos contar;
     if (Test_For_Path <Mapa, Contar_Arcos> (contar) 
	 (g, buscar_ciudad(g, "El Vigia"), buscar_ciudad(g, "Carora")))
       cout << "Existe un camino desde El Vigia hacia Carora" << endl;
     cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    auto cmp = [] (const Mapa::Node * p1, const Mapa::Node * p2)
      {
	return p1->get_info().nombre < p2->get_info().nombre; 
      };	
    DynList<Mapa> subgrafos;

    Inconnected_Components <Mapa> () (g, subgrafos);
    subgrafos.mutable_for_each([cmp] (Mapa & m) { m.sort_nodes(cmp); });

    subgrafos.for_each([] (const Mapa & m)
    {
      m.nodes().for_each([] (auto p) { cout << p->get_info().nombre << " "; });
      cout << endl;
    });
    cout << endl;

    DynList<Mapa> subs = inconnected_components(g);
    subs.mutable_for_each([cmp] (Mapa & m) { m.sort_nodes(cmp); });
    
    subs.for_each([] (const Mapa & m)
    {
      m.nodes().for_each([] (auto p) { cout << p->get_info().nombre << " "; });
      cout << endl;
    });
    cout << endl;

    assert(eq(subgrafos, subs, [] (const Mapa & m1, const Mapa & m2)
    	      {
    		return eq(m1.nodes(), m2.nodes(), [] (auto p1, auto p2)
                  {
		    return p1->get_info().nombre == p2->get_info().nombre; 
		  });
    	      }));

    zipEq(subgrafos, subs).for_each([] (auto & p)
    {
      zipEq(p.first.nodes(), p.second.nodes()).for_each([] (auto p) 
      {
	assert(p.first->get_info().nombre == p.second->get_info().nombre);
      });
    });
  }

  {
    Mapa tree;
    Find_Depth_First_Spanning_Tree <Mapa> () (g, tree);
  }

  {
    Mapa tree;
    Total_Dist total;
    (Find_Depth_First_Spanning_Tree <Mapa, Total_Dist> (total)) (g, tree);
    cout << "La distancia total del arbol es : " << total.dist << endl;
  }


  Path<Mapa> path_b = find_path_breadth_first(g, buscar_ciudad(g, "El Vigia"), 
					      buscar_ciudad(g, "Carora")); 
  cout << "Camino por amplitud : ";
  imprimir_camino(path_b);

  Find_Path_Breadth_First<Mapa> ()  (g, buscar_ciudad(g, "El Vigia"), 
				     buscar_ciudad(g, "Carora"), path_b); 
  cout << "Camino por amplitud : ";
  imprimir_camino(path_b);

  //  return 0;

  Path<Mapa> path =
    Find_Path_Depth_First <Mapa> () (g, buscar_ciudad(g, "El Vigia"), 
				     buscar_ciudad(g, "Carora")); 
  cout << "Camino por profundidad : ";
  imprimir_camino(path);

  //  Find_Path_Depth_First <Mapa>()(g, g.search_node("El Vigia"), 
  //			  g.search_node("Carora"), path); 

  //  return 0;

  imprimir_mapa(g);

  Mapa tree1;
  Kruskal_Min_Spanning_Tree <Mapa, Distancia_Via> () (g, tree1);
  int sum1 = Total_Cost<Mapa, Distancia_Via> () (tree1);
  cout << endl
       << "Arbol abarcador segun Kruskal (" << sum1 << ")" << endl;
  imprimir_mapa(tree1);

  Mapa tree2;
  Prim_Min_Spanning_Tree <Mapa, Distancia_Via> () (g, tree2);
  int sum2 = Total_Cost<Mapa, Distancia_Via>() (tree2);
  cout << endl
       << "Arbol abarcador segun Prim (" << sum2 << ")" << endl;
  imprimir_mapa(tree2);

  if (sum1 != sum2)
    EXIT("Costes totales de arboles difieren %d %d", sum1, sum2);

  Mapa tree3;

  Dijkstra_Min_Paths<Mapa, Distancia_Via> () (g, g.get_first_node(), tree3);
  cout << endl
       << "Arbol abarcador segun Dijkstra" << endl;
  imprimir_mapa(tree3);

  Path<Mapa> min_path1 = find_path_depth_first(tree3, tree3.get_first_node(), 
					       buscar_ciudad(tree3, "Carora"));
  if (not min_path1.is_empty())
    {
      cout << "Camino encontrado en Dijkstra: " 
	   << tree3.get_first_node()->get_info().nombre << " - Carora" << endl;
      imprimir_camino(min_path1);
      cout << endl << endl;
    }
  else
    EXIT("Error no se encontro camino!");

   
  // TODO: hacer una copia hacia un digrafo yt verificar resultados con
  // los dos algoritmos dijkstra y bellman

# ifdef nada

  Mapa tree4;

  Bellman_Ford_Min_Spanning_Tree<Mapa, Distancia_Via> ()
    (g, g.get_first_node(), tree4);
  cout << endl
       << "Arbol abarcador segun Bellman-Ford" << endl;
  imprimir_mapa(tree4);

  Path<Mapa> min_path2 = 
    find_path_depth_first(tree4, tree4.get_first_node(), 
			  buscar_ciudad(tree4, "Carora"));
  if (not min_path2.is_empty())
    {
      cout << "Camino encontrado en Dijkstra: " 
	   << tree4.get_first_node()->get_info().nombre << " - Carora" << endl;
      imprimir_camino(min_path2);
      cout << endl << endl;
    }
  else
    EXIT("Error no se encontró camino!");

# endif 

  Path<Mapa> aux_path = min_path1;

  min_path1 = aux_path;

  DynDlist<Path<Mapa> > test;

  Path<Mapa> path2(g);
  Dijkstra_Min_Paths<Mapa, Distancia_Via> ().find_min_path
    (g, buscar_ciudad(g, "Guanare"), buscar_ciudad(g, "San Fernando"), path2);


  cout << "Camino encontrado:" << endl;
  imprimir_camino(path2);
  cout << endl << endl;

  (Dijkstra_Min_Paths <Mapa, Distancia_Via> ()).find_min_path 
    (g, buscar_ciudad(g, "Guanare"), buscar_ciudad(g, "San Fernando"), path2);

  cout << "Camino encontrado:" << endl;
  imprimir_camino(path2);
  cout << endl << endl;

# ifdef nada

  Mat_Mapa matriz(g);

  Ady_Mat<Mat_Mapa, int> distancias1(matriz, &via_a_distancia);

  Ady_Mat<Mat_Mapa, int> distancias2(distancias1);

  Mapa gc1 = g;

  Mapa gc2;

  gc2 = g;

  g = gc1;

  imprimir_mapa(g);

  Dimapa dimapa;

  gc2 = dimapa;

  EXIT("");

# endif

}


