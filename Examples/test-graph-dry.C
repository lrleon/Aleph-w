

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


# include <string.h>
# include <ahSort.H>
# include <ahFunctional.H>
# include <bitArray.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>


using namespace Aleph;

using ulong = unsigned long;
using Graph = List_Graph<Graph_Node<ulong>, Graph_Arc<ulong>>;
using SGraph = List_SGraph<Graph_Snode<ulong>, Graph_Sarc<ulong>>;
using AGraph = Array_Graph<Graph_Anode<ulong>, Graph_Aarc<ulong>>;

template <class GT>
GT create_graph(size_t n)
{
  DynArray<typename GT::Node*> nodes;
  GT g;
  for (auto i = 0; i < n; ++i)
    nodes.append(g.insert_node(i));
  
  for (auto i = 0; i < g.vsize() - 1; ++i)
    {
      assert(nodes(i)->get_info() == i);
      for (auto j = i + 1; j < g.vsize(); ++j)
	{
	  assert(nodes(j)->get_info() == j);
	  g.insert_arc(nodes(i), nodes(j), i + j);
	}
    }

  return g;
}

template <class GT>
void test_basic()
{
  GT g;
  cout << "Testing basic " << typeid(g).name() << endl
       << endl;

  char * str = "This is a cookie";

  assert(g.vsize() == 0 and g.esize() == 0 and g.get_cookie() == nullptr and
	 not g.is_digraph());

  g.get_cookie() = str;
  assert(strcmp((char*) g.get_cookie(), str) == 0);
  g.get_cookie() = nullptr;
  
  auto s = g.insert_node(1);
  auto t = g.insert_node(2);
  assert(NODE_COOKIE(s) == nullptr and g.get_cookie(s) == nullptr and
	 g.get_counter(s) == 0);
  assert(NODE_COOKIE(t) == nullptr and g.get_cookie(t) == nullptr and
	 g.get_counter(t) == 0);
  assert(g.vsize() == 2);
  assert(s->get_info() == 1);
  assert(t->get_info() == 2);
  g.get_counter(s) = -1;
  g.get_cookie(s) = str;
  g.get_counter(t) = -1;
  g.get_cookie(t) = str;
  assert(strcmp((char*) g.get_cookie(s), str) == 0 and g.get_counter(s) == -1);
  assert(strcmp((char*) g.get_cookie(t), str) == 0 and g.get_counter(t) == -1);

  g.get_cookie(s) = nullptr;
  g.get_counter(s) = 0;
  g.get_cookie(t) = nullptr;
  g.get_counter(t) = 0;
  g.reset_counter(s);
  g.reset_counter(t);
  g.get_cookie(s) = g.get_cookie(t) = nullptr;
  assert(NODE_COOKIE(s) == nullptr and g.get_cookie(s) == nullptr and
	 g.get_counter(s) == 0);
  assert(NODE_COOKIE(t) == nullptr and g.get_cookie(t) == nullptr and
	 g.get_counter(t) == 0);

  auto a = g.insert_arc(s, t, 1);

  assert(g.esize() == 1);
  assert(g.get_src_node(a) == s);
  assert(g.get_tgt_node(a) == t);
  assert(a->get_info() == 1);
  assert(g.get_connected_node(a, s) == t);
  assert(g.get_connected_node(a, t) == s);

  g.get_cookie(a) = str;
  g.get_counter(a) = -1;
  assert(strcmp((char*) g.get_cookie(a), str) == 0 and g.get_counter(a) == -1);
  g.reset_counter(a);
  g.get_cookie(a) = nullptr;
  assert(g.get_cookie(a) == nullptr and g.get_counter(a) == 0);

  g.map_nodes(s, t);
  assert(g.get_cookie(s) == t and g.get_cookie(t) == s);

  auto aux = g.insert_arc(t, s);
  g.map_arcs(a, aux);
  assert(g.get_cookie(aux) == a and g.get_cookie(a) == aux);

  auto s1 = g.insert_node();
  auto s2 = g.insert_node(ulong(10));
  auto s3 = g.emplace_node();
  auto s4 = g.emplace_node(20);
  auto s5 = g.emplace_node(ulong(30));
  assert(s1->get_info() == 0);
  assert(s2->get_info() == 10);
  assert(s3->get_info() == 0);
  assert(s4->get_info() == 20);
  assert(s5->get_info() == 30);

  auto a1 = g.insert_arc(s1, s2);
  auto a2 = g.insert_arc(s3, s4, 10);
  auto a3 = g.insert_arc(s4, s5, ulong(100));
  auto a4 = g.emplace_arc(s3, s2);
  auto a5 = g.emplace_arc(s1, s5, ulong(15));
  auto a6 = g.emplace_arc(s3, s5, 25);
  assert(a1->get_info() == 0);
  assert(a2->get_info() == 10);
  assert(a3->get_info() == 100);
  assert(a4->get_info() == 0);
  assert(a5->get_info() == 15);
  assert(a6->get_info() == 25);

  g.remove_node(s1);
  assert(not g.exists_arc([a1] (auto a) { return a == a1; }));
  assert(not g.exists_arc([a5] (auto a) { return a == a5; }));
  g.remove_node(s5);
  assert(not g.exists_arc([a5, a6, a3] (auto a) { return a == a3 or a == a6 or
	  a == a5; }));
  g.remove_arc(a2);
  assert(not g.exists_arc([a2] (auto a) { return a == a2; }) and 
	 g.vsize() == 5 and g.esize() == 3);

  cout << "End testing basic " << typeid(g).name() << endl
       << endl;
}

template <class GT>
void test_functional_directed(const GT & gg)
{
  cout << "Testing functional directed " << typeid(gg).name() << endl
       << endl;

  {
    GT g = gg;

		       // testing of in|out degrees
    g.reset_counter_nodes();
    g.for_each_arc([&g] (auto a) { NODE_COUNTER(g.get_tgt_node(a))++; });
    g.for_each_node([&g] (auto p) 
		    {
		      assert(NODE_COUNTER(p) == g.in_degree(p));
		    });

    g.reset_counter_nodes();
    g.for_each_arc([&g] (auto a) { NODE_COUNTER(g.get_src_node(a))++; });
    g.for_each_node([&g] (auto p) 
		    {
		      assert(NODE_COUNTER(p) == g.out_degree(p));
		    });

    
		    // testing of is|out arcs|nodes
    g.for_each_node([] (auto p)
		    {    // out arcs list
		      NODE_COOKIE(p) = new DynList<typename GT::Arc*>();
		    });
    g.for_each_arc([&g] (auto a) // insert into out lists
      {
	auto src = g.get_src_node(a);
	auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(src);
	lptr->append(a);
      });
    g.for_each_node([&g] (auto p)
		    {
		      auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(p);
		      assert(eq(sort(*lptr), sort(g.out_arcs(p))));
		      lptr->empty();
		    });
    g.for_each_arc([&g] (auto a) // insert into in lists
      {
	auto tgt = g.get_tgt_node(a);
	auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(tgt);
	lptr->append(a);
      });
    g.for_each_node([&g] (auto p)
		    {
		      auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(p);
		      assert(eq(sort(*lptr), sort(g.in_arcs(p))));
		      lptr->empty();
		    });
    
    g.for_each_node([] (auto p)
		    {
		      delete (DynList<typename GT::Arc*>*) NODE_COOKIE(p);
		    });

		      // testing with in|out pairs
    using ArcPair = typename GT::ArcPair;
    g.for_each_node([] (auto p)
		    {    // out pair list
		      NODE_COOKIE(p) = new DynList<ArcPair>();
		    });
    g.for_each_arc([&g] (auto a)
		   {
		     auto src = g.get_src_node(a);
		     auto tgt = g.get_tgt_node(a);
		     auto lptr = (DynList<ArcPair>*) NODE_COOKIE(tgt);
		     lptr->append(std::make_tuple(a, src));
		   });
    g.for_each_node([&g] (auto p)
      {
	auto lptr = (DynList<ArcPair>*) NODE_COOKIE(p);
	auto arcs = lptr->template maps<typename GT::Arc*>([] (auto t)
          {
	    return std::get<0>(t);
	  });
	auto nodes = lptr->template maps<typename GT::Node*>([] (auto t)
          {
	    return std::get<1>(t);
	  });
	assert(eq(sort(nodes), sort(g.in_nodes(p))));
	assert(eq(sort(arcs), sort(g.in_arcs(p))));
	lptr->empty();
      });

    g.for_each_arc([&g] (auto a)
		   {
		     auto src = g.get_src_node(a);
		     auto tgt = g.get_tgt_node(a);
		     auto lptr = (DynList<ArcPair>*) NODE_COOKIE(src);
		     lptr->append(std::make_tuple(a, tgt));
		   });
    g.for_each_node([&g] (auto p)
      {
	auto lptr = (DynList<ArcPair>*) NODE_COOKIE(p);
	auto arcs = lptr->template maps<typename GT::Arc*>([] (auto t)
          {
	    return std::get<0>(t);
	  });
	auto nodes = lptr->template maps<typename GT::Node*>([] (auto t)
          {
	    return std::get<1>(t);
	  });
	assert(eq(sort(nodes), sort(g.out_nodes(p))));
	assert(eq(sort(arcs), sort(g.out_arcs(p))));
	lptr->empty();
      });

    g.for_each_node([] (auto p)
		    {    // out arcs list
		      delete (DynList<ArcPair>*) NODE_COOKIE(p);
		    });
  }

  const GT g = gg;

		      // testing of in|out degrees
  g.reset_counter_nodes();
  g.for_each_arc([&g] (auto a) { NODE_COUNTER(g.get_tgt_node(a))++; });
  g.for_each_node([&g] (auto p) 
		  {
		    assert(NODE_COUNTER(p) == g.in_degree(p));
		  });

  g.reset_counter_nodes();
  g.for_each_arc([&g] (auto a) { NODE_COUNTER(g.get_src_node(a))++; });
  g.for_each_node([&g] (auto p) 
		  {
		    assert(NODE_COUNTER(p) == g.out_degree(p));
		  });

    
		    // testing of is|out arcs|nodes
  g.for_each_node([] (auto p)
		  {    // out arcs list
		    NODE_COOKIE(p) = new DynList<typename GT::Arc*>();
		  });
  g.for_each_arc([&g] (auto a) // insert into out lists
		 {
		   auto src = g.get_src_node(a);
		   auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(src);
		   lptr->append(a);
		 });
  g.for_each_node([&g] (auto p)
		  {
		    auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(p);
		    assert(eq(sort(*lptr), sort(g.out_arcs(p))));
		    lptr->empty();
		  });
  g.for_each_arc([&g] (auto a) // insert into in lists
		 {
		   auto tgt = g.get_tgt_node(a);
		   auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(tgt);
		   lptr->append(a);
		 });
  g.for_each_node([&g] (auto p)
		  {
		    auto lptr = (DynList<typename GT::Arc*>*) NODE_COOKIE(p);
		    assert(eq(sort(*lptr), sort(g.in_arcs(p))));
		    lptr->empty();
		  });
    
  g.for_each_node([] (auto p)
		  {
		    delete (DynList<typename GT::Arc*>*) NODE_COOKIE(p);
		  });

		      // testing with in|out pairs
  using ArcPair = typename GT::ArcPair;
  g.for_each_node([] (auto p)
		  {    // out pair list
		    NODE_COOKIE(p) = new DynList<ArcPair>();
		  });
  g.for_each_arc([&g] (auto a)
		 {
		   auto src = g.get_src_node(a);
		   auto tgt = g.get_tgt_node(a);
		   auto lptr = (DynList<ArcPair>*) NODE_COOKIE(tgt);
		   lptr->append(std::make_tuple(a, src));
		 });
  g.for_each_node([&g] (auto p)
    {
      auto lptr = (DynList<ArcPair>*) NODE_COOKIE(p);
      auto arcs = lptr->template maps<typename GT::Arc*>([] (auto t)
        {
	  return std::get<0>(t);
	});
      auto nodes = lptr->template maps<typename GT::Node*>([] (auto t)
        {
	  return std::get<1>(t);
	});
      assert(eq(sort(nodes), sort(g.in_nodes(p))));
      assert(eq(sort(arcs), sort(g.in_arcs(p))));
      lptr->empty();
    });

  g.for_each_arc([&g] (auto a)
		 {
		   auto src = g.get_src_node(a);
		   auto tgt = g.get_tgt_node(a);
		   auto lptr = (DynList<ArcPair>*) NODE_COOKIE(src);
		   lptr->append(std::make_tuple(a, tgt));
		 });
  g.for_each_node([&g] (auto p)
    {
      auto lptr = (DynList<ArcPair>*) NODE_COOKIE(p);
      auto arcs = lptr->template maps<typename GT::Arc*>([] (auto t)
        {
	  return std::get<0>(t);
	});
      auto nodes = lptr->template maps<typename GT::Node*>([] (auto t)
        {
	  return std::get<1>(t);
	});
      assert(eq(sort(nodes), sort(g.out_nodes(p))));
      assert(eq(sort(arcs), sort(g.out_arcs(p))));
      lptr->empty();
    });

  g.for_each_node([] (auto p)
		  {    // out arcs list
		    delete (DynList<ArcPair>*) NODE_COOKIE(p);
		  });

  cout << "end testing functional " << typeid(gg).name() << endl
       << endl;
}

template <class GT>
void test_functional(const GT & gg)
{
  cout << "Testing functional " << typeid(gg).name() << endl
       << endl;
			 // testing of traverse
  {
    GT g = gg;
    {    
      BitArray nodes; nodes.reserve(g.vsize());
      g.traverse_nodes([&nodes] (auto p) 
		       {
			 nodes.write(p->get_info(), 1);
			 return true;
		       });
      assert(nodes.all([] (auto i) { return i; }));
    }
    assert(g.traverse_arcs([&g] (auto a) 
    {
      return a->get_info() == 
	g.get_src_node(a)->get_info() + g.get_tgt_node(a)->get_info();
    }));
  
    assert(g.traverse_nodes([&g] (auto p) 
    {
      return g.traverse_arcs(p, [&g, p] (auto a)
        {
	  return p->get_info() + g.get_connected_node(a, p)->get_info() == 
	    a->get_info();
	});
    }));

			 // testing of for_each
    g.for_each_node([&g] (auto p) { assert(p->get_info() < g.vsize()); });
    g.for_each_arc([&g] (auto a) 
		   { 
		     assert(a->get_info() == g.get_src_node(a)->get_info() +
			    g.get_tgt_node(a)->get_info()); 
		   });
    g.for_each_node([&g] (auto p) 
    {   
      g.for_each_arc(p, [p, &g] (auto a)
        {
	  assert(a->get_info() == p->get_info() +
		 g.get_connected_node(a, p)->get_info());
	});
    });
  
			   // testing of all
    assert(g.all_nodes([&g] (auto p) { return p->get_info() < g.vsize(); }));
    assert(g.all_arcs([&g] (auto a) 
    {
      return a->get_info() == 
	g.get_src_node(a)->get_info() + g.get_tgt_node(a)->get_info();
    }));
    assert(g.all_nodes([&g] (auto p) 
    {
      return g.all_arcs(p, [&g, p] (auto a)
        {
	  return p->get_info() + g.get_connected_node(a, p)->get_info() == 
	    a->get_info();
	});
    }));
  
			   // testing of map
    assert(eq(g.template nodes_map<ulong>([](auto p) { return p->get_info(); }),
	      g.nodes().template maps<ulong>([] (auto p)
					    { return p->get_info(); })));

    assert(eq(g.template arcs_map<ulong>([] (auto a) { return a->get_info(); }),
	      g.arcs().template maps<ulong>([] (auto a)
					   { return a->get_info(); })));
    g.for_each_node([&g] (auto p) 
    {
      auto l1 = 
	g.template arcs_map<ulong>(p, [] (auto a) { return a->get_info(); });
      auto l2 = g.arcs(p).template maps<ulong>([] (auto a) 
					      { return a->get_info(); });
      assert(eq(l1, l2));
    });

			   // test for exist
    assert(g.exists_node([] (auto p) { return p->get_info() > 10; }));
    assert(g.exists_arc([] (auto a) { return a->get_info() > 10; }));
    g.for_each_node([&g] (auto p)
    {
      assert(g.exists_arc(p, [] (auto a) { return a->get_info() >= 5; }));
    });

			    // test of foldl
    assert(g.template foldl_nodes<ulong>(0, [] (auto a, auto p) 
         { return a + p->get_info(); }) == 100*99/2);
    assert(g.template foldl_arcs<ulong>(0, [] (auto a, auto p) 
				      { return a + p->get_info(); }) ==
	   g.arcs().template foldl<ulong>(0 , [] (auto acc, auto a)
					  { return acc + a->get_info(); }));
    g.for_each_node([&g] (auto p)
    {
      auto s1 = 
	g.arcs(p).template foldl<ulong>(0, [] (auto acc, auto a)
					{ return acc + a->get_info(); });
      auto s2 = g.template foldl_arcs<ulong>(p, 0, [] (auto acc, auto a)
					     { return acc + a->get_info(); });
      assert(s1 == s2);
    });

			   // test of filter
    assert(eq(g.filter_nodes([] (auto p) { return p->get_info() < 5; }),
	      g.nodes().filter([] (auto p) { return p->get_info() < 5; })));
    assert(eq(g.filter_arcs([] (auto a) { return a->get_info() < 10; }), 
	      g.arcs().filter([] (auto a) { return a->get_info() < 10; })));
    g.for_each_node([&g] (auto p)
    {
      auto l1 = g.arcs(p).filter([] (auto a) { return a->get_info() < 10; });
      auto l2 = g.filter_arcs(p, [] (auto a) { return a->get_info() < 10; });
      assert(eq(l1, l2));
    });

			   // test for exist
    assert(g.exists_node([] (auto p) { return p->get_info() > 10; }));
    assert(g.exists_arc([] (auto a) { return a->get_info() > 10; }));
    g.for_each_node([&g] (auto p)
    {
      assert(g.exists_arc(p, [] (auto a) { return a->get_info() >= 5; }));
    });

			 // test for search_arc
    g.for_each_node([&g] (auto p)
    {
      assert(g.search_arc(p, [p, n = g.vsize()] (typename GT::Arc * a) 
			{ return a->get_info() == (p->get_info() + 1) % n; })
	     != nullptr);
    });

    g.for_each_arc([&g] (auto a) 
    {
      assert(g.search_arc(g.get_src_node(a), g.get_tgt_node(a)) != nullptr);
    });

    assert(g.search_node([] (auto p) { return p->get_info() == 10; }));
    assert(not g.search_node([&g] (auto p) 
			     { return p->get_info() == g.vsize(); }));
    assert(g.search_arc([] (auto a) { return a->get_info() == 12; }));
    assert(not g.search_arc([n = 2*g.vsize()] (auto a) 
			    { return a->get_info() == n; }));


		   // tests for reset family on nodes
    char * str = "This is a test";
    g.reset_nodes();
    g.for_each_node([&g, str] (auto p)
    {
      g.set_bit(p, 8, 1);
      g.set_bit(p, 9, 1);
      g.get_counter(p) = -1;
      g.get_cookie(p) = str;
    });
    assert(g.all_nodes([&g, str] (auto p)
		       {
			 return g.get_bit(p, 8) and g.get_bit(p, 9) and
			   g.get_counter(p) == -1 and g.get_cookie(p) == str;
		       }));
    g.reset_nodes();
    assert(g.all_nodes([&g] (auto p)
    {
      return g.get_bit(p, 8) == 0 and g.get_bit(p, 9) == 0 and
	g.get_counter(p) == 0 and g.get_cookie(p) == nullptr;
    }));

		    // test for reset family on arcs
    g.reset_arcs();
    g.for_each_arc([&g, str] (auto a)
		   {
		     g.set_bit(a, 8, 1);
		     g.set_bit(a, 9, 1);
		     g.get_counter(a) = -1;
		     g.get_cookie(a) = str;
		   });
    assert(g.all_arcs([&g, str] (auto a)
		      {
			return g.get_bit(a, 8) and g.get_bit(a, 9) and
			  g.get_counter(a) == -1 and g.get_cookie(a) == str;
		      }));
    g.reset_arcs();
    assert(g.all_arcs([&g] (auto a)
    {
      return g.get_bit(a, 8) == 0 and g.get_bit(a, 9) == 0 and
	g.get_counter(a) == 0 and g.get_cookie(a) == nullptr;
    }));

    g.reset_bit_nodes(4);
    g.for_each_node([&g] (auto p) { g.set_bit(p, 4, 1); });
    assert(g.all_nodes([&g] (auto p) { return g.get_bit(p, 4) == 1; }));
    g.reset_bit_nodes(4);
    assert(g.all_nodes([&g] (auto p) { return g.get_bit(p, 4) == 0; }));

    g.reset_bit_arcs(4);
    g.for_each_arc([&g] (auto a) { g.set_bit(a, 4, 1); });
    assert(g.all_arcs([&g] (auto a) { return g.get_bit(a, 4) == 1; }));
    g.reset_bit_arcs(4);
    assert(g.all_arcs([&g] (auto a) { return g.get_bit(a, 4) == 0; }));

    g.reset_node_counters();
    g.for_each_node([&g] (auto p) { g.get_counter(p) = p->get_info(); });
    assert(g.all_nodes([&g] (auto p) 
		       { return g.get_counter(p) == p->get_info(); }));
    g.reset_node_counters();
    assert(g.all_nodes([&g] (auto p) { return g.get_counter(p) == 0; }));

    g.reset_arc_counters();
    g.for_each_arc([&g] (auto a) { g.get_counter(a) = a->get_info(); });
    assert(g.all_arcs([&g] (auto a) 
		       { return g.get_counter(a) == a->get_info(); }));
    g.reset_arc_counters();
    assert(g.all_arcs([&g] (auto a) { return g.get_counter(a) == 0; }));

			  // iterators testing
    for (auto it = g.get_node_it(); it.has_curr(); it.next())
      assert(it.get_curr()->get_info() < g.vsize());

    for (auto it = g.get_arc_it(); it.has_curr(); it.next())
      {
	auto a = it.get_curr();
	assert(g.get_src_node(a)->get_info() + g.get_tgt_node(a)->get_info() ==
	       a->get_info());
      }

    for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
      for (auto it = g.get_arc_it(nit.get_curr()); it.has_curr(); it.next())
	{
	  auto a = it.get_curr();
	  assert(g.get_src_node(a)->get_info() + g.get_tgt_node(a)->get_info() 
		 == a->get_info());
	}

    for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
      for (auto it = g.get_in_it(nit.get_curr()); it.has_curr(); it.next())
	{
	  auto a = it.get_curr();
	  assert(g.get_tgt_node(a) == nit.get_curr());
	}

    for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
      for (auto it = g.get_out_it(nit.get_curr()); it.has_curr(); it.next())
	assert(g.get_src_node(it.get_curr()) == nit.get_curr());

    

  } // end for g scope

		       // test on constant graph
  const GT g = gg;
  assert(g.vsize() == gg.vsize() and g.esize() == gg.esize());
  {
    BitArray nodes; nodes.reserve(g.vsize());
    g.traverse_nodes([&nodes] (auto p) 
		      {
			nodes.write(p->get_info(), 1);
			return true;
		      });
    assert(nodes.all([] (auto i) { return i; }));
  }
  assert(g.traverse_arcs([&g] (auto a) 
    {
      return a->get_info() == 
	g.get_src_node(a)->get_info() + g.get_tgt_node(a)->get_info();
    }));
  {
    BitArray nodes; nodes.reserve(g.vsize());
    g.for_each_node([&nodes] (auto p) { nodes.write(p->get_info(), 1); });
    assert(nodes.all([] (auto i) { return i; }));
  }

  g.for_each_arc([&g] (auto a) 
		 { 
		   assert(a->get_info() == g.get_src_node(a)->get_info() +
			  g.get_tgt_node(a)->get_info()); 
		 });
  g.for_each_node([&g] (auto p) 
    {   
      g.for_each_arc(p, [p, &g] (auto a)
        {
	  assert(a->get_info() == p->get_info() +
		 g.get_connected_node(a, p)->get_info());
	});
    });

  assert(g.all_nodes([&g] (auto p) { return p->get_info() < g.vsize(); }));
  assert(g.all_arcs([&g] (auto a) 
    {
      return a->get_info() == 
	g.get_src_node(a)->get_info() + g.get_tgt_node(a)->get_info();
    }));
  assert(g.all_nodes([&g] (auto p) 
    {
      return g.all_arcs(p, [&g, p] (auto a)
        {
	  return p->get_info() + g.get_connected_node(a, p)->get_info() == 
	    a->get_info();
	});
    }));

  assert(eq(g.template nodes_map<ulong>([](auto p) { return p->get_info(); }),
	    g.nodes().template maps<ulong>([] (auto p) { return p->get_info(); })
	    ));
  assert(eq(g.template arcs_map<ulong>([] (auto a) { return a->get_info(); }),
	    g.arcs().template maps<ulong>([] (auto a) { return a->get_info(); })
	    ));
  g.for_each_node([&g] (auto p) 
  {
    auto l1 = 
      g.template arcs_map<ulong>(p, [] (auto a) { return a->get_info(); });
    auto l2 = g.arcs(p).template maps<ulong>([] (auto a)
					     { return a->get_info(); });
    assert(eq(l1, l2));
  });

  assert(g.template foldl_nodes<ulong>(0, [] (auto a, auto p) 
         { return a + p->get_info(); }) == 100*99/2);
  assert(g.template foldl_arcs<ulong>(0, [] (auto a, auto p) 
				      { return a + p->get_info(); }) ==
	 g.arcs().template foldl<ulong>(0 , [] (auto acc, auto a)
					{ return acc + a->get_info(); }));
  g.for_each_node([&g] (auto p)
    {
      auto s1 = 
	g.arcs(p).template foldl<ulong>(0, [] (auto acc, auto a)
					{ return acc + a->get_info(); });
      auto s2 = g.template foldl_arcs<ulong>(p, 0, [] (auto acc, auto a)
					     { return acc + a->get_info(); });
      assert(s1 == s2);
    });

  assert(eq(g.filter_nodes([] (auto p) { return p->get_info() < 5; }),
   	    g.nodes().filter([] (auto p) { return p->get_info() < 5; })));
  assert(eq(g.filter_arcs([] (auto a) { return a->get_info() < 10; }), 
	    g.arcs().filter([] (auto a) { return a->get_info() < 10; })));
  g.for_each_node([&g] (auto p)
    {
      auto l1 = g.arcs(p).filter([] (auto a) { return a->get_info() < 10; });
      auto l2 = g.filter_arcs(p, [] (auto a) { return a->get_info() < 10; });
      assert(eq(l1, l2));
    });

  assert(g.exists_node([] (auto p) { return p->get_info() > 10; }));
  assert(g.exists_arc([] (auto a) { return a->get_info() > 10; }));
  g.for_each_node([&g] (auto p)
    {
      assert(g.exists_arc(p, [] (auto a) { return a->get_info() >= 5; }));
    });

  g.for_each_node([&g] (auto p)
    {
      assert(g.search_arc(p, [p, n = g.vsize()] (typename GT::Arc * a) 
			{ return a->get_info() == (p->get_info() + 1) % n; })
	     != nullptr);
    });
  assert(g.search_node([] (auto p) { return p->get_info() == 10; }));
  assert(not g.search_node([&g] (auto p) 
			   { return p->get_info() == g.vsize(); }));
  assert(g.search_arc([] (auto a) { return a->get_info() == 12; }));
  assert(not g.search_arc([n = 2*g.vsize()] (auto a) 
			  { return a->get_info() == n; }));


  char * str = "hello! this is a wonderful world!";
  g.reset_nodes();
  g.for_each_node([&g, str] (auto p)
		  {
		    g.set_bit(p, 8, 1);
		    g.set_bit(p, 9, 1);
		    g.get_counter(p) = -1;
		    g.get_cookie(p) = str;
		  });
  assert(g.all_nodes([&g, str] (auto p)
		     {
		       return g.get_bit(p, 8) and g.get_bit(p, 9) and
			 g.get_counter(p) == -1 and g.get_cookie(p) == str;
		     }));
  g.reset_nodes();
  assert(g.all_nodes([&g] (auto p)
		     {
		       return g.get_bit(p, 8) == 0 and g.get_bit(p, 9) == 0 and
			 g.get_counter(p) == 0 and g.get_cookie(p) == nullptr;
		     }));

  g.reset_bit_nodes(4);
  g.for_each_node([&g] (auto p) { g.set_bit(p, 4, 1); });
  assert(g.all_nodes([&g] (auto p) { return g.get_bit(p, 4) == 1; }));
  g.reset_bit_nodes(4);
  assert(g.all_nodes([&g] (auto p) { return g.get_bit(p, 4) == 0; }));

  g.reset_bit_arcs(4);
  g.for_each_arc([&g] (auto a) { g.set_bit(a, 4, 1); });
  assert(g.all_arcs([&g] (auto a) { return g.get_bit(a, 4) == 1; }));
  g.reset_bit_arcs(4);
  assert(g.all_arcs([&g] (auto a) { return g.get_bit(a, 4) == 0; }));

  g.reset_bit_nodes(4);
  g.for_each_node([&g] (auto p) { g.set_bit(p, 4, 1); });
  assert(g.all_nodes([&g] (auto p) { return g.get_bit(p, 4) == 1; }));
  g.reset_bit_nodes(4);
  assert(g.all_nodes([&g] (auto p) { return g.get_bit(p, 4) == 0; }));

  g.reset_bit_arcs(4);
  g.for_each_arc([&g] (auto a) { g.set_bit(a, 4, 1); });
  assert(g.all_arcs([&g] (auto a) { return g.get_bit(a, 4) == 1; }));
  g.reset_bit_arcs(4);
  assert(g.all_arcs([&g] (auto a) { return g.get_bit(a, 4) == 0; }));

  g.reset_node_counters();
  g.for_each_node([&g] (auto p) { g.get_counter(p) = p->get_info(); });
  assert(g.all_nodes([&g] (auto p) 
		     { return g.get_counter(p) == p->get_info(); }));
  g.reset_node_counters();
  assert(g.all_nodes([&g] (auto p) { return g.get_counter(p) == 0; }));

  g.reset_arc_counters();
  g.for_each_arc([&g] (auto a) { g.get_counter(a) = a->get_info(); });
  assert(g.all_arcs([&g] (auto a) 
		    { return g.get_counter(a) == a->get_info(); }));
  g.reset_arc_counters();
  assert(g.all_arcs([&g] (auto a) { return g.get_counter(a) == 0; }));

  cout << "End functional " << typeid(gg).name() << endl
       << endl;
}

// template <class GT>
// void test_functional(GT && g)
// {
//   cout << "RVALUE test_functional" << endl;
//   // test_functional(g);
//   test_functional(static_cast<std::remove_reference_t<GT> const&>(g));
// }

int main()
{
  {
    cout << "List_Graph" << endl;
    test_basic<Graph>();
    auto g = create_graph<Graph>(100);
    test_functional(g);
    test_functional_directed(g);
    cout << "end List_Graph" << endl
	 << endl;
  }

  {
    cout << "List_SGraph" << endl;
    test_basic<SGraph>();
    auto g = create_graph<SGraph>(100);
    test_functional(g);
    test_functional_directed(g);
    cout << "end List_SGraph" << endl
	 << endl;
  }

  {
    cout << "Array_Graph" << endl;
    test_basic<AGraph>();
    auto g = create_graph<AGraph>(100);
    test_functional(g);
    test_functional_directed(g);
    cout << "end Array_Graph" << endl
	 << endl;
  }
  
  cout << "All Test were passed!" << endl
       << endl;
}
