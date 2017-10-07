# include <tpl_agraph.H>
# include <random_graph.H>
# include <generate_graph.H>
# include <graph-traverse.H>

# include <tpl_dynSetTree.H>

unsigned long node_count = 0;
unsigned long arc_count = 0;

using Net = Array_Graph<>;

struct Init_Node
{
  void operator () (Net &, Net::Node * p) const
  {
    p->get_info() = node_count++;
  }
};

struct Init_Arc
{
  void operator () (Net &, Net::Arc * a) const
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

int main(int argc, char *argv[])
{
  if (argc != 4)
    usage();

  size_t n = atoi(argv[1]);
  double prob = atof(argv[2]);
  unsigned long seed = atoi(argv[3]);

  Net g = create_graph(n, prob, seed);

  {
    DynMapTree<unsigned long, Net::Node*> table;

    size_t n = (Graph_Traverse_DFS<Net, Node_Arc_Iterator<Net>>(g))
     		(g.get_first_node(), [&table] (Net::Node * p)
      {
     	table.insert(p->get_info(), p);
	return true;
      });

    // size_t n = (Graph_Traverse<Net, Node_Arc_Iterator<Net>>(g))
    // 		(g.get_first_node(), [&table] (Net::Node * p)
    //   {
    // 	table.insert(p->get_info(), p);
    // 	return true;
    //   });

		assert(g.vsize() == n);

     using Pair = std::pair<unsigned long, Net::Node*>;

     cout << "List = " << endl;
   
     table.for_each([] (const Pair & p)
		    {
		      cout << p.first << " " << p.second->get_info() << endl;
		    });
  }
  {
    DynMapTree<unsigned long, Net::Node*> ntable;
    DynMapTree<unsigned long, Net::Arc*> atable;
    size_t n = Graph_Traverse_DFS<Net, Node_Arc_Iterator<Net>>(g).
      exec(g.get_first_node(), [&ntable, &atable]
	   (Net::Node * p, Net::Arc * a)
      {
	if (a)
	  atable.insert(a->get_info(), a);
	else
	  cout << "Arco NULO" << endl;
	ntable.insert(p->get_info(), p);
	return true;
      });
    using Pair = std::pair<unsigned long, Net::Arc*>;
    atable.for_each([] (const Pair &p)
      {
	cout << p.first << " " << p.second->get_info() << endl;
      });
    cout << "of " << g.esize() << endl;
  }

      {
	DynMapTree<unsigned long, Net::Node*> table;

	size_t n = (Graph_Traverse<Net, Out_Iterator<Net>>(g))
		    (g.get_first_node(), [&table] (Net::Node * p)
	  {
	    table.insert(p->get_info(), p);
	    return true;
	  });

        using Pair = std::pair<unsigned long, Net::Node*>;

        cout << "List = " << endl;
        table.for_each([] (const Pair & p)
	  {
	    cout << p.first << " " << p.second->get_info() << endl;
	  });

		cout << "FIN" << endl 
		<< n << " nodes seen" << endl;
  } 

  {
    DynMapTree<unsigned long, Net::Node*> node_table;
    DynMapTree<unsigned long, Net::Arc*> arc_table;

    auto n = (Graph_Traverse<Net, Node_Arc_Iterator<Net>>(g))
     (g.get_first_node(), [&node_table] (Net::Node * p)
      {
	node_table.insert(p->get_info(), p);
	return true;
      }, [&arc_table] (Net::Arc * a)
      {
	arc_table.insert(a->get_info(), a);
	return true;
      });
	   assert(g.vsize() == get<0>(n));
	   assert(g.esize() == get<1>(n));

	      using P = pair<unsigned long, Net::Arc*>;
    arc_table.for_each([] (const P & p)
      {
	cout << p.first<< " " << p.second->get_info() << endl;
      });
  }   
}
