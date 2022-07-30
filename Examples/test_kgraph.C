
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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
# include <iostream>
# include <tpl_graph_utils.H>
# include <tpl_kgraph.H>


typedef List_Graph<Graph_Node<int>, Graph_Arc<Empty_Class> > Grafo;


void crear_grafo(Grafo & g, int n, int m)
{
  long max_arcs = n*(n-1)/2;
  if (m > max_arcs)
    throw std::domain_error("cannot generate graph with more of " +
			    to_string(max_arcs) + " arcs");
  DynArray<Grafo::Node*> nodes;

  for (int i = 0; i < n; i++)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < m; i++)
    {
      int n2, n1 = (int) (1.0 * n * (rand() / (RAND_MAX + 1.0)));
      while (g.get_num_arcs(nodes.access(n1)) == n - 1)
	n1 = (int) (1.0 * n * (rand() / (RAND_MAX + 1.0)));
      do 
	n2 = (int) (1.0 * n * (rand() / (RAND_MAX + 1.0)));
      while (n2 == n1 or g.get_num_arcs(nodes.access(n2)) == n - 1 or
	     search_arc(g, nodes.access(n1), nodes.access(n2)) != NULL);
      g.insert_arc(nodes[n1], nodes[n2]); 
    }

  for (int i = 0; i < n; i++)
    {
      Grafo::Node * p = nodes.access(i);

      if (g.get_num_arcs(p) != 0)
	continue;

    repeat:
      int idx = (int) (1.0 * n * (rand() / (RAND_MAX + 1.0)));
      if (idx == i) 
	goto repeat;

      g.insert_arc(nodes[i], nodes[idx]);
    }

  assert(test_connectivity(g));

  if (g.get_num_arcs() >= n*n/2)
    throw std::domain_error("cannot generate random graph");
} 

void crear_arco(Grafo & g, int isrc, int itgt)
{
  Grafo::Node * src = g.find_node(isrc);
  if (src == NULL)
    src = g.insert_node(isrc);

  Grafo::Node * tgt = g.find_node(itgt);
  if (tgt == NULL)
    tgt = g.insert_node(itgt);

  if (search_arc<Grafo>(g, src, tgt) != NULL)
    throw std::invalid_argument("Duplicated arc");

  g.insert_arc(src, tgt);
}


void crear_grafo(Grafo & g)
{
  g.insert_node(1);
  g.insert_node(2);
  g.insert_node(3);
  g.insert_node(4);
  g.insert_node(5);
  g.insert_node(6);
  g.insert_node(7);
  g.insert_node(8);
  g.insert_node(9);
  g.insert_node(10);
  g.insert_node(11);
  g.insert_node(12);
  g.insert_node(13);
  g.insert_node(14);
  g.insert_node(15);
  g.insert_node(16);
  g.insert_node(17);
  g.insert_node(18);
  g.insert_node(19);
  g.insert_node(20);
  g.insert_node(21);
  g.insert_node(22);
  g.insert_node(23);
  g.insert_node(24);
  g.insert_node(25);
  g.insert_node(26);
  g.insert_node(27);

  crear_arco(g, 17, 6);
  crear_arco(g, 17, 12);
  crear_arco(g, 17, 18);
  crear_arco(g, 17, 23);
  crear_arco(g, 23, 12);
  crear_arco(g, 23, 18);
  crear_arco(g, 23, 24);
  crear_arco(g, 12, 6);
  crear_arco(g, 12, 7);
  crear_arco(g, 18, 12);
  crear_arco(g, 24, 18);
  crear_arco(g, 24, 19);
  crear_arco(g, 24, 25);
  crear_arco(g, 25, 18);
  crear_arco(g, 25, 12);
  crear_arco(g, 25, 19);
  crear_arco(g, 26, 15);
  crear_arco(g, 19, 12);
  crear_arco(g, 19, 13);
  crear_arco(g, 13, 12);
  crear_arco(g, 13, 6);
  crear_arco(g, 13, 7);
  crear_arco(g, 7, 6);
  crear_arco(g, 7, 1);
  crear_arco(g, 13, 8);
  crear_arco(g, 8, 7);
  crear_arco(g, 8, 1);
  crear_arco(g, 8,2 );
  crear_arco(g, 8, 3);
  crear_arco(g, 8, 9);
  crear_arco(g, 9, 4);
  crear_arco(g, 14, 13);
  crear_arco(g, 14, 8);
  crear_arco(g, 14, 3);
  crear_arco(g, 14, 19);
  crear_arco(g, 14, 25);
  crear_arco(g, 14, 20);
  crear_arco(g, 14, 15);
  crear_arco(g, 14, 9);
  crear_arco(g, 9, 3);
  crear_arco(g, 15, 9);
  crear_arco(g, 20, 26 );
  crear_arco(g, 20, 25);
  crear_arco(g, 20, 27);
  crear_arco(g, 20, 21);
  crear_arco(g, 20, 15);
  crear_arco(g, 15, 21);
  crear_arco(g, 21, 27);
  crear_arco(g, 21, 16);
  crear_arco(g, 21, 22);
  crear_arco(g, 15, 16);
  crear_arco(g, 15, 10);
  crear_arco(g, 10, 9);
  crear_arco(g, 10, 4);
  crear_arco(g, 10, 5);
  crear_arco(g, 10, 11);
  crear_arco(g, 10, 16);
  crear_arco(g, 16, 11);
  crear_arco(g, 16, 22);
  crear_arco(g, 16, 27);
  crear_arco(g, 27, 22);
  crear_arco(g, 22, 11);
  crear_arco(g, 11, 5);
  crear_arco(g, 5, 4);
  crear_arco(g, 5, 9);
  crear_arco(g, 4, 3);
  crear_arco(g, 3, 2);
  crear_arco(g, 2, 1);
  crear_arco(g, 2, 13);
  crear_arco(g, 2, 15);
  crear_arco(g, 2, 19);
  crear_arco(g, 2, 7);
  crear_arco(g, 1, 6);
  crear_arco(g, 25, 26);
  crear_arco(g, 26, 27);
  crear_arco(g, 26, 21);
}


void crear_otro_grafo(Grafo & g)
{
  g.insert_node(1);
  g.insert_node(2);
  g.insert_node(3);
  g.insert_node(4);
  g.insert_node(5);
  g.insert_node(6);
  g.insert_node(7);
  g.insert_node(8);
  g.insert_node(9);
  g.insert_node(10);
  g.insert_node(11);
  g.insert_node(12);
  g.insert_node(13);
  g.insert_node(14);
  g.insert_node(15);
  g.insert_node(16);
  g.insert_node(17);
  g.insert_node(18);
  g.insert_node(19);
  g.insert_node(20);
  g.insert_node(21);
  g.insert_node(22);
  g.insert_node(23);
  g.insert_node(24);
  g.insert_node(25);
  g.insert_node(26);
  g.insert_node(27);

  crear_arco(g, 17, 6);
  crear_arco(g, 17, 12);
  crear_arco(g, 17, 18);
  crear_arco(g, 17, 23);
  crear_arco(g, 23, 12);
  crear_arco(g, 23, 18);
  crear_arco(g, 23, 24);
  crear_arco(g, 12, 6);
  crear_arco(g, 12, 7);
  crear_arco(g, 18, 12);
  crear_arco(g, 24, 18);
  crear_arco(g, 24, 19);
  crear_arco(g, 24, 25);
  crear_arco(g, 25, 18);
  crear_arco(g, 25, 12);
  crear_arco(g, 25, 19);
  crear_arco(g, 26, 15);
  crear_arco(g, 19, 12);
  crear_arco(g, 19, 13);
  crear_arco(g, 13, 12);
  crear_arco(g, 13, 6);
  crear_arco(g, 13, 7);
  crear_arco(g, 7, 6);
  //  crear_arco(g, 7, 8); // ojo
  crear_arco(g, 7, 1);
  crear_arco(g, 7, 2);
  crear_arco(g, 13, 8);
  crear_arco(g, 8, 1);
  crear_arco(g, 8, 2);
  crear_arco(g, 8, 3);

  crear_arco(g, 9, 4);

  crear_arco(g, 14, 19);

  crear_arco(g, 14, 20);
  crear_arco(g, 14, 15);
  crear_arco(g, 14, 9);
  crear_arco(g, 9, 3);
  crear_arco(g, 15, 9);
  crear_arco(g, 20, 26 );

  crear_arco(g, 20, 27);
  crear_arco(g, 20, 21);
  crear_arco(g, 20, 15);
  crear_arco(g, 15, 21);
  crear_arco(g, 21, 27);
  crear_arco(g, 21, 16);
  crear_arco(g, 21, 22);
  crear_arco(g, 15, 16);
  crear_arco(g, 15, 10);
  crear_arco(g, 10, 9);
  crear_arco(g, 10, 4);
  crear_arco(g, 10, 5);
  crear_arco(g, 10, 11);
  crear_arco(g, 10, 16);
  crear_arco(g, 16, 11);
  crear_arco(g, 16, 22);
  crear_arco(g, 16, 27);
  crear_arco(g, 27, 22);
  crear_arco(g, 22, 11);
  crear_arco(g, 11, 5);
  crear_arco(g, 5, 4);
  crear_arco(g, 5, 9);
  crear_arco(g, 4, 3);
  crear_arco(g, 3, 2);
  crear_arco(g, 2, 1);
  crear_arco(g, 2, 13);
  crear_arco(g, 2, 19);
  crear_arco(g, 1, 6);

  crear_arco(g, 26, 27);
  crear_arco(g, 26, 21);
}

void escribir_grafo(Grafo & g)
{
  for (Node_Iterator<Grafo> it(g); it.has_curr(); it.next())
    {
      Grafo::Node * p = it.get_curr();
      cout << p->get_info() << endl;
      for (Node_Arc_Iterator<Grafo> i(p); i.has_curr(); i.next())
	cout << "    " << i.get_tgt_node()->get_info() << endl;
    }
}


int main(int argn, char *argc[])
{
  int num_nodes = argc[1] ? atoi(argc[1]) : 1000;

  int num_arcs = argc[1] ? atoi(argc[2]) : 100000;

  unsigned int t = time(0);

  if (argn > 3)
    t = atoi(argc[3]);

  srand(t);

  cout << argc[0] << " " << num_nodes << " " << num_arcs << " " << t << endl;


  {
    Grafo g;

    crear_grafo(g, num_nodes, num_arcs);

    //    crear_otro_grafo(g);

    //    escribir_grafo(g);

    cout << "...." << endl;
    //    int k3 = Edge_Connectivity <Grafo, Heap_Preflow_Maximum_Flow> () (g);

    
    int k = vertex_connectivity<Grafo, Heap_Preflow_Maximum_Flow> (g);
    

 //    int k4 = Edge_Connectivity <Grafo, Fifo_Preflow_Maximum_Flow> () (g);

    //    cout << "Ke(G) = " << k1 << endl;
    cout << "Kv(G) = " << k << endl;

    //    cout << "Ke(G) = " << k4 << endl;
  }

  {
    Grafo g;
    crear_otro_grafo(g);

    int k3 = Edge_Connectivity <Grafo, Heap_Preflow_Maximum_Flow> () (g);
    int k4 = Edge_Connectivity <Grafo, Fifo_Preflow_Maximum_Flow> () (g);
    int k1 = Edge_Connectivity <Grafo, Ford_Fulkerson_Maximum_Flow> () (g);
    int k2 = Edge_Connectivity <Grafo, Edmonds_Karp_Maximum_Flow> () (g);

    cout << "Ke(G) = " << k1 << endl;
    cout << "Ke(G) = " << k2 << endl;
    cout << "Ke(G) = " << k3 << endl;
    cout << "Ke(G) = " << k4 << endl;
  }
}
