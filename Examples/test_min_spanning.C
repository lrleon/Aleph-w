
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
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

# include <tpl_sgraph.H>
# include <tpl_agraph.H>
# include <random_graph.H>
# include <generate_graph.H>
# include <Kruskal.H>
# include <Prim.H>

//typedef List_SGraph<Graph_Snode<int>, Graph_Sarc<int>> Graph;
typedef Array_Graph<Graph_Anode<int>, Graph_Aarc<int>> Graph;
//typedef List_Graph<Graph_Node<int>, Graph_Arc<int>> Graph;

template <class GT>
struct Init_Node
{
  static int count;

  void operator () (const GT&, typename GT::Node * p)
  {
    p->get_info() = count++;
  }
};

template <class GT>
int Init_Node<GT>::count = 0;


template <class GT>
struct Init_Arc
{
  static int count;

  void operator () (const GT&, typename GT::Arc * a)
  {
    a->get_info() = count++;
  }
};

template <class GT>
struct NWnodo
{
  void operator () (const GT &, typename GT::Node * p, std::ostream & output) 
  {
    output << "label = \"" << p->get_info() << "\"";
  }
};

template <class GT>
struct NWarco
{
  void operator () (const GT & g, typename GT::Arc * a, std::ostream & output)
  {
    typename GT::Node * src = g.get_src_node(a);
    typename GT::Node * tgt = g.get_tgt_node(a);
    output << "label = \" " <<  a->get_info() << "\\n"
	   << src->get_info() << "-" << tgt->get_info() << "\"";
    if (IS_ARC_VISITED(a, Aleph::Spanning_Tree) or 
	IS_ARC_VISITED(a, Aleph::Spanning_Tree))
      output << " color = \"red\" ";

    if (ARC_COOKIE(a) != NULL)
       output << " color = \"red\" ";
  }
};

template <class GT>
int Init_Arc<GT>::count = 0;


template <class GT>
struct Total_Dist
{
  int dist;

  Total_Dist () : dist(0) { /* empty */ }

  bool operator () (typename GT::Arc * a)
  {
    if (a != NULL)
      dist += a->get_info();

    return true;
  }
};

template <class GT>
struct Kruskal_Filt
{
  bool operator () (GT &, typename GT::Arc * a) const
  {
    return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
  }

  bool operator () (typename GT::Arc * a) const
  {
    return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
  }
};

template <class GT>
struct Prim_Filt
{
  bool operator () (GT &, typename GT::Arc * a) const
  {
    return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
  }

  bool operator () (typename GT::Arc * a) const
  {
    return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
  }
};

template <class GT>
GT generate_graph(size_t num_nodes, size_t num_arcs, unsigned int seed)
{
  Random_Graph<GT, Init_Node<GT>, Init_Arc<GT>> gen(seed);

  return gen(num_nodes, num_arcs);
}

    template <class GT, 
	      template <class> class Filt = Dft_Show_Arc>
void print(GT & g)
{
  int num = 1;
  for (Arc_Iterator <GT, Filt<GT>> it(g); it.has_curr(); it.next())
    {
      typename GT::Arc * a = it.get_curr();
      typename GT::Node * src = g.get_src_node(a);
      typename GT::Node * tgt = g.get_tgt_node(a);
      cout << num++ << ": " << src->get_info() << " (" << a->get_info() << ") "
	   << tgt->get_info() << endl;
    }
}

int main(int argn, char * argc[])
{
  size_t n = 1000;
  size_t m = 4000;
  unsigned int seed = 0;

  if (argn < 5)
    {
      printf("usage: %s n m seed [algo]\n", argc[0]);
      exit(1);
    }

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    m = atoi(argc[2]);

  if (argn > 3)
    seed = atoi(argc[3]);

  if (m > 1000000)
    if (not resize_process_stack(128*1024*1024))
      cout << "Atencion: no puedo apartar suficiente pila" << endl;

  char type = 'k';
  if (argn > 4)
    type = *argc[4];

  cout << argc[0] << " " << n << " " << m << " " << seed << " " 
       << argc[4] << endl;

  cout << "Generando grafo .." << endl;

  Graph g = generate_graph<Graph>(n, m, seed);

  //  print(g);

  cout << "Generado con " << g.get_num_nodes() << " nodos y " 
       << g.get_num_arcs() << " arcos " << endl
       << endl
       << "Calculando arbol abarcador ..." << endl
       << endl;

  switch (type)
    {
    case 'k':
      {
	cout << " Kruskal pintando" << endl;
	Kruskal_Min_Spanning_Tree <Graph> () (g);

	cout << "Calculado!" << endl
	     << endl;

	std::ofstream out("test1.dot", ios::out);
	To_Graphviz <Graph, NWnodo<Graph>, NWarco<Graph>> () (g, out);

	cout << "Calculando el costo" << endl
	     << endl;

	int cost = 
	  Total_Cost<Graph, Dft_Dist<Graph>, Kruskal_Filt<Graph>>
	  () (g);

	cout << "Listo es " << cost << endl
	     << endl;
	break;
      }
    case 'K':
      {
	cout << "Kruskal por arbol separado" << endl
	     << endl;

	Graph tree;

	Kruskal_Min_Spanning_Tree <Graph> () (g, tree);

	std::ofstream out("test2.dot", ios::out);
	To_Graphviz <Graph, NWnodo<Graph>, NWarco<Graph>> () (g, out);

	cout << "Calculado! Tiene " << tree.get_num_arcs() << " arcos " << endl
	     << endl;

	cout << "Calculando el costo" << endl
	     << endl;
	int cost = Total_Cost<Graph> () (tree);

	cout << "Listo es " << cost << endl
	     << endl;
	break;
      }
      case 'P':
      {
	cout << "Prim por arbol separado" << endl
	     << endl;

	Graph tree;

	Prim_Min_Spanning_Tree <Graph> () (g, tree);

	std::ofstream out("test2.dot", ios::out);
	To_Graphviz <Graph, NWnodo<Graph>, NWarco<Graph>> () (g, out);

	cout << "Calculado! Tiene " << tree.get_num_arcs() << " arcos " << endl
	     << endl;

	cout << "Calculando el costo" << endl
	     << endl;
	int cost = Total_Cost<Graph> () (tree);

	cout << "Listo es " << cost << endl
	     << endl;
	break;
      }
    case 'p':
      {
	cout << "Prim por pintado" << endl
	     << endl;

	Graph tree;

	Prim_Min_Spanning_Tree <Graph> () (g);

	std::ofstream out("test2.dot", ios::out);
	To_Graphviz <Graph, NWnodo<Graph>, NWarco<Graph>> () (g, out);

	cout << "Calculando el costo" << endl
	     << endl;

	int cost = 
	  Total_Cost<Graph, Dft_Dist<Graph>, Prim_Filt<Graph>> () (g);

	cout << "Listo es " << cost << endl
	     << endl;
	break;
      }
    default:
      printf("Unknown algoritmh type %c\n", type);
      exit(1);
    }
}
