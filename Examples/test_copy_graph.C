

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


# include <ahSort.H>
# include <random_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>

unsigned long node_count = 0;
unsigned long arc_count = 0;

template <class GT> struct Init_Node
{
  void operator () (GT &, typename GT::Node * p) const
  {
    p->get_info() = node_count++;
  }
};

template <class GT> struct Init_Arc
{
  void operator () (GT &, typename GT::Arc * a) const
  {
    a->get_info() = arc_count++;
  }
};


template <class GT>
bool graph_eq(const GT & g1, const GT & g2)
{
  if (g1.vsize() != g2.vsize() or g1.esize() != g2.esize())
    return false;
  
  auto cmpnode = [] (typename GT::Node *p1, typename GT::Node *p2)
    {
      return p1->get_info() < p2->get_info();
    };

  auto cmparc = [] (typename GT::Arc *a1, typename GT::Arc *a2)
    {
      return a1->get_info() < a2->get_info();
    };

  using Npair = pair<typename GT::Node*, typename GT::Node*>;
  using Apair = pair<typename GT::Arc*, typename GT::Arc*>;

  g1.reset_nodes();
  g2.reset_nodes();

  try
    {
      return zipEq(sort(g1.nodes(), cmpnode), sort(g2.nodes(), cmpnode)).
	all([] (const Npair & p) 
	    { 
	      GT::map_nodes(p.first, p.second);
	      // cout << p.first->get_info() << " == " << p.second->get_info()
	      // 	   << endl;
	      return p.first->get_info() == p.second->get_info();
	    })
	and
	zipEq(sort(g1.arcs(), cmparc), sort(g2.arcs(), cmparc)).
	all([&g1, &g2] (const Apair & p)
	    {
	      typename GT::Arc * a1 = p.first;
	      typename GT::Arc * a2 = p.second;
	      if (a1->get_info() != a2->get_info())
		return false;

	      typename GT::Node * src1 = g1.get_src_node(a1);
	      typename GT::Node * tgt1 = g1.get_tgt_node(a1);

	      typename GT::Node * src2 = mapped_node<GT>(src1);
	      typename GT::Node * tgt2 = mapped_node<GT>(tgt1);

	      if (src1->get_info() != src2->get_info() or
		  tgt1->get_info() != tgt2->get_info())
		return false;

	      bool valid =
	      (src2 == g2.get_src_node(a2) or src2 == g2.get_tgt_node(a2)) and
		(tgt2 == g2.get_tgt_node(a2) or tgt2 == g2.get_src_node(a2));

	      if (not valid)
		return false;

	      // cout << src1->get_info() << "," << a1->get_info() << ","
	      // 	   << tgt1->get_info() << endl
	      // 	   << src2->get_info() << "," << a2->get_info() << ","
	      // 	   << tgt2->get_info() << endl
	      // 	   << endl;

	      src2 = g2.get_src_node(a2);
	      tgt2 = g2.get_tgt_node(a2);

	      src1 = mapped_node<GT>(src2);
	      tgt1 = mapped_node<GT>(tgt2);

	      if (src2->get_info() != src1->get_info() or
		  tgt2->get_info() != tgt1->get_info())
		return false;

	      valid = 
		(src1 == g1.get_src_node(a1) or src1 != g1.get_tgt_node(a1)) and
		(tgt1 == g1.get_tgt_node(a1) or tgt1 == g1.get_tgt_node(a1));

	      return valid;
	    });
    }
  catch (length_error)
    {
      return false;
    }
}

template <class GT>
bool digraph_eq(const GT & g1, const GT & g2)
{
  if (g1.vsize() != g2.vsize() or g1.esize() != g2.esize())
    return false;
  
  auto cmpnode = [] (typename GT::Node *p1, typename GT::Node *p2)
    {
      return p1->get_info() < p2->get_info();
    };

  auto cmparc = [] (typename GT::Arc *a1, typename GT::Arc *a2)
    {
      return a1->get_info() < a2->get_info();
    };

  using Npair = pair<typename GT::Node*, typename GT::Node*>;
  using Apair = pair<typename GT::Arc*, typename GT::Arc*>;

  g1.reset_nodes();
  g2.reset_nodes();

  try
    {
      return zipEq(sort(g1.nodes(), cmpnode), sort(g2.nodes(), cmpnode)).
	all([] (const Npair & p) 
	    { 
	      GT::map_nodes(p.first, p.second);
	      // cout << p.first->get_info() << " == " << p.second->get_info()
	      // 	   << endl;
	      return p.first->get_info() == p.second->get_info();
	    })
	and
	zipEq(sort(g1.arcs(), cmparc), sort(g2.arcs(), cmparc)).
	all([&g1, &g2] (const Apair & p)
	    {
	      typename GT::Arc * a1 = p.first;
	      typename GT::Arc * a2 = p.second;
	      if (a1->get_info() != a2->get_info())
		return false;

	      typename GT::Node * src1 = g1.get_src_node(a1);
	      typename GT::Node * tgt1 = g1.get_tgt_node(a1);

	      typename GT::Node * src2 = mapped_node<GT>(src1);
	      typename GT::Node * tgt2 = mapped_node<GT>(tgt1);

	      if (src1->get_info() != src2->get_info() or
		  tgt1->get_info() != tgt2->get_info())
		return false;

	      if (not (src2 == g2.get_src_node(a2) and 
		       tgt2 == g2.get_tgt_node(a2)))
		return false;

	      // cout << src1->get_info() << "," << a1->get_info() << ","
	      // 	   << tgt1->get_info() << endl
	      // 	   << src2->get_info() << "," << a2->get_info() << ","
	      // 	   << tgt2->get_info() << endl
	      // 	   << endl;

	      src2 = g2.get_src_node(a2);
	      tgt2 = g2.get_tgt_node(a2);

	      src1 = mapped_node<GT>(src2);
	      tgt1 = mapped_node<GT>(tgt2);

	      if (src2->get_info() != src1->get_info() or
		  tgt2->get_info() != tgt1->get_info())
		return false;

	      return 
		src1 == g1.get_src_node(a1) and tgt1 == g1.get_tgt_node(a1);
	    });
    }
  catch (length_error)
    {
      return false;
    }
}

template <class GT> 
void test_graph(size_t n, double prob, unsigned long seed)
{
  cout << "Creating random graph" << endl;
  GT g = Random_Graph<GT, Init_Node<GT>, Init_Arc<GT>>(seed)(n, prob);
  cout << g.vsize() << " nodes and " << g.esize() << " arcs" << endl 
       << endl << endl;
  {
    cout << "Testing L copy" << endl;
    GT copy = g;
    if (graph_eq(g, copy))
      cout << "    Success" << endl;
    else
      {
	cout << "    Failure" << endl;
	exit(0);
      }

    cout << endl
	 << "Testing R copy" << endl;
    GT rcopy = move(copy);
    if (copy.vsize() > 0 or copy.esize() > 0)
      {
	cout << "    Failure" << endl;
	exit(0);
      }
    if (graph_eq(g, rcopy))
    cout << "    Success" << endl;
    else
      {
	cout << "    Failure" << endl;
	exit(0);
      }

    cout << endl
	 << "Testing L = " << endl;
    copy = g;
     if (graph_eq(g, copy))
      cout << "    Success" << endl;
    else
      {
	cout << "    Failure" << endl;
	exit(0);
      }

     cout << endl
	  << "Testing R =" << endl;
     rcopy = move(copy);
     if (graph_eq(g, rcopy))
	 cout << "    Success" << endl;
     else
      {
	cout << "    Failure" << endl;
	exit(0);
      }
  }
  cout << endl;
}

template <class GT> 
void test_digraph(size_t n, double prob, unsigned long seed)
{
  cout << "Creating random digraph" << endl;
  GT g = Random_Digraph<GT, Init_Node<GT>, Init_Arc<GT>>(seed)(n, prob);
  cout << g.vsize() << " nodes and " << g.esize() << " arcs" << endl 
       << endl << endl;
  {
    cout << "Testing L copy" << endl;
    GT copy = g;
    if (digraph_eq(g, copy))
      cout << "    Success" << endl;
    else
      {
	cout << "    Failure" << endl;
	exit(0);
      }

    cout << endl
	 << "Testing R copy" << endl;
    GT rcopy = move(copy);
    if (copy.vsize() > 0 or copy.esize() > 0)
      {
	cout << "    Failure" << endl;
	exit(0);
      }
    if (graph_eq(g, rcopy))
    cout << "    Success" << endl;
    else
      {
	cout << "    Failure" << endl;
	exit(0);
      }

    cout << endl
	 << "Testing L = " << endl;
    copy = g;
     if (graph_eq(g, copy))
      cout << "    Success" << endl;
    else
      {
	cout << "    Failure" << endl;
	exit(0);
      }

     cout << endl
	  << "Testing R =" << endl;
     rcopy = move(copy);
     if (graph_eq(g, rcopy))
	 cout << "    Success" << endl;
     else
      {
	cout << "    Failure" << endl;
	exit(0);
      }
  }
  cout << endl;
}

void usage()
{
  cout << "test n prob seed" << endl;
  exit(0);
}

int main(int argc, char *argv[])
{
  if (argc != 4)
    usage();

  size_t n = atoi(argv[1]);
  double prob = atof(argv[2]);
  unsigned long seed = atoi(argv[3]);

  test_graph<List_Graph<>>(n, prob, seed);
  test_graph<List_SGraph<>>(n, prob, seed);
  test_graph<Array_Graph<>>(n, prob, seed);

  test_digraph<List_Digraph<>>(n, prob, seed);
  test_digraph<List_SDigraph<>>(n, prob, seed);
  test_digraph<Array_Digraph<>>(n, prob, seed);
}
