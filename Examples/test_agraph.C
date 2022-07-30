
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
# include <random_graph.H>
# include <tpl_agraph.H>

# define INDENT "    "

using namespace Aleph;

typedef Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>> Agraph;

struct Init_Node
{
  static int count;

  void operator () (Agraph&, Agraph::Node * p)
  {
    p->get_info() = count++;
  }
};

int Init_Node::count = 0;


struct Init_Arc
{
  static int count;

  void operator () (Agraph&, Agraph::Arc * a)
  {
    a->get_info() = count++;
  }
};

int Init_Arc::count = 0;


Agraph generate_graph(size_t num_nodes, size_t num_arcs, unsigned int seed)
{
  Random_Digraph<Agraph, Init_Node, Init_Arc> gen(seed);

  return gen(num_nodes, num_arcs);
}


int main(int argn, char * argc[])
{
  if (argn < 4)
    {
      printf("usage %s n m seed [rand-arcs-to-delete] [nodes to delete]\n", 
	     argc[0]);
      exit(1);
    }

  size_t n = 1000;
  size_t m = 4000;
  unsigned int seed = time(0);

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    m = atoi(argc[2]);

  if (argn > 3)
    seed = atoi(argc[3]);

  cout << "Generando grafo .." << endl;

  Agraph g = generate_graph(n, m, seed);

  cout << "Generado con " << g.get_num_nodes() << " y " 
       << g.get_num_arcs() << endl;

  if (argn < 5)
    return 0;

  int num_arcs = atoi(argc[4]);
  gsl_rng * r = gsl_rng_alloc (gsl_rng_taus);
  cout << "Borrando " << num_arcs << " seleccionados al azar ... " << endl;
  for (int i = 0; i < num_arcs; ++i)
    {
      int num = gsl_rng_uniform_int(r, g.get_num_arcs());
      cout << i << ": buscando arco " << num << " a eliminar ..." << endl;
      Agraph::Arc * a = g.find_arc(num);
      if (a == NULL)
	{
	  cout << "RARO: arco " << num << " no encontrado" << endl;
	  continue;
	}

      cout << "Elinando arco " << a->get_info() << endl;
      g.remove_arc(a);
      cout << "eliminado!" << endl
	   << endl;
    }
  gsl_rng_free(r);

  for (int i = 5; i < argn; ++i)
    {
      int num = atoi(argc[i]);
      cout << "Buscando nodo " << num << endl;
      Agraph::Node * p = g.find_node(num);
      if (p != NULL)
	{
	  cout << "Encontrado!" << endl
	       << endl
	       << "eliminando nodo " << num << " ... " << endl;
      
	  g.remove_node(p);

	  cout << "eliminado!" << endl;
	}
      else
	cout << "No encontrado!" << endl;
      
      cout << endl;
    }
}


