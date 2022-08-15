
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
# include <tpl_find_path.H>
# include <random_graph.H>
# include <tpl_sgraph.H>

//typedef List_SGraph<Graph_Snode<int>, Graph_Sarc<long>> Grafo;
typedef List_Graph<Graph_Node<int>, Graph_Arc<long>> Grafo;


int counter = 0;

unsigned int t = 0;

struct Inicia_Nodo
{
  void operator () (Grafo &, Grafo::Node * p)
  {
    p->get_info() = counter++;
  }
};

struct Inicia_Arco
{
  gsl_rng * r;

  Inicia_Arco()
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);
    gsl_rng_set(r, t % gsl_rng_max(r));
  }

  Inicia_Arco(const Inicia_Arco & a) 
    : r(a.r)
  {
    const_cast<Inicia_Arco&>(a).r = NULL;
  }

  ~Inicia_Arco()
  {
    if (r != NULL)
      gsl_rng_free(r);
    r = NULL;
  }

  void operator () (Grafo &, Grafo::Arc * a) const
  {
    unsigned long rand_offset = 1 + gsl_rng_uniform_int(r, 99);

    a->get_info() = rand_offset;
  }
};

Grafo generar_grafo(size_t num_nodes, size_t num_arcs, unsigned int s)
{
  Grafo ret =
    Random_Graph <Grafo, Inicia_Nodo, Inicia_Arco> (s) (num_nodes, num_arcs);
  
  return ret;
}

Grafo::Node * buscar(Grafo & g, int num)
{
  return g.find_node(num);
}

// usage: test_find_path n m s t seed

int main(int argn, char * argc[])
{
  if (argn < 5)
    {
      printf("usage: test_find_path n m s t [seed]\n");
      exit(0);
    }

  size_t n = atoi(argc[1]);
  size_t m = atoi(argc[2]);
  
  int s = atoi(argc[3]);
  int t = atoi(argc[4]);

  unsigned int seed = 0;
  if (argn >= 6)
    seed = atoi(argc[5]);

  Grafo g = generar_grafo(n, m, seed);

  Grafo::Node * src = buscar(g, s);
  if (src == NULL)
    {
      printf("Node number %d not found\n", s);
      exit(1);
    }

  Grafo::Node * tgt = buscar(g, t);
  if (tgt == NULL)
    {
      printf("Node number %d not found\n", t);
      exit(1);
    }

  {
    Path<Grafo> p = Find_Path_Depth_First<Grafo> () (g, src, tgt);
    p.for_each_node([] (auto p) { cout << p->get_info() << " "; });
    cout << endl;
  }
  
  {
    Path<Grafo> p1 = find_path_breadth_first(g, src, tgt);
    Path<Grafo> p2(g);

    Find_Path_Breadth_First<Grafo> () (g, src, tgt, p2);

    Path<Grafo>::Iterator it1(p1);
    Path<Grafo>::Iterator it2(p2);

    while (it1.has_curr() and it2.has_curr())
      {
	if (it1.get_current_node() != it2.get_current_node())
	  {
	    printf("Distints paths!\n");
	    exit(1);
	  }
	
	it1.next();
	it2.next();
      }
  }
}
