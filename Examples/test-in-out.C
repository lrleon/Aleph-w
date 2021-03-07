
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# include <tpl_agraph.H>
# include <random_graph.H>
# include <generate_graph.H>

unsigned long node_count = 0;
unsigned long arc_count = 0;

using Net = Array_Graph<>;


struct Init_Node
{
  void operator () (const Net &, Net::Node * p) const
  {
    p->get_info() = node_count++;
  }
};

struct Init_Arc
{
  void operator () (const Net &, Net::Arc * a) const
  {
    a->get_info() = arc_count++;
  }
};

Net create_graph(size_t n, double prob, unsigned long seed)
{
  return Random_Graph<Net, Init_Node, Init_Arc>(seed)(n, prob);
}

void usage()
{
  cout << "test n prob seed" << endl;
  exit(0);
}

inline ostream & operator << (ostream & s, const Path<Net> & path)
{
  if (path.is_empty())
    return s << "Path is Empty";

  const Net & net = path.get_graph();
  Path<Net>::Iterator it(path);
  s << it.get_current_node()->get_info();
  for (; it.has_current_arc(); it.next())
    {
      Net::Arc * a = it.get_current_arc();
      s << "(" << a->get_info() << ")" 
	<< net.get_connected_node(a, it.get_current_node())-> get_info();
    }
  return s;
}



int main(int argc, char *argv[])
{
  if (argc != 4)
    usage();

  size_t n = atoi(argv[1]);
  double prob = atof(argv[2]);
  unsigned long seed = atoi(argv[3]);

  Net g = create_graph(n, prob, seed);

  To_Graphviz<Net> ().digraph(g, cout);

  for (Net::Node_Iterator it(g); it.has_curr(); it.next())
    {
      cout << "ARCS(" << it.get_curr()->get_info() << ") =";
      arcs<Net>(it.get_curr()).for_each([&g] (Net::Arc * a)
        {
	  cout << " " << g.get_src_node(a)->get_info() << "(" 
	       << a->get_info() << ")" << g.get_tgt_node(a)->get_info();
	});
      cout << endl
	   << endl
	   << "IN(" << it.get_curr()->get_info() << ") =";
      in_arcs<Net>(it.get_curr()).for_each([&g] (Net::Arc * a)
        {
	  cout << " " << g.get_src_node(a)->get_info() << "(" 
	       << a->get_info() << ")" << g.get_tgt_node(a)->get_info();
	});
      cout << endl
	   << endl
	   << "in_nodes(" << it.get_curr()->get_info() << ") =";
      in_nodes<Net>(it.get_curr()).for_each([] (Net::Node * p)
        {
	  cout << " " << p->get_info();
	});
      cout << endl
	   << endl
	   << "Out(" << it.get_curr()->get_info() << ") =";
      out_arcs<Net>(it.get_curr()).for_each([&g] (Net::Arc * a)
        {
	  cout << " " << g.get_src_node(a)->get_info() << "(" 
	       << a->get_info() << ")" << g.get_tgt_node(a)->get_info();
	});
      cout << endl
	   << endl
	   << "out_nodes(" << it.get_curr()->get_info() << ") =";
      out_nodes<Net>(it.get_curr()).for_each([] (Net::Node * p)
        {
	  cout << " " << p->get_info();
	});
      cout << endl
	   << endl;
    }

  gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));
  DynArray<Net::Node*> nodes = g.nodes();
  long src_idx, tgt_idx;
  while (cin >> src_idx >> tgt_idx)
    {
      cout << "src = " << src_idx << endl
	   << "tgt = " << tgt_idx << endl;

      cout << "DFS" << endl
	   << Directed_Find_Path<Net>(g).dfs(nodes(src_idx), nodes(tgt_idx)) 
	   << endl
	   << "BFS" << endl
	   << Directed_Find_Path<Net>(g).bfs(nodes(src_idx), nodes(tgt_idx)) 
	   << endl
	   << endl;
    }

  gsl_rng_free(r);
}
