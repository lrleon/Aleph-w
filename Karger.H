
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
# ifndef KARGER_H
# define KARGER_H

# include <cmath>
# include <limits>
# include <htlist.H>
# include <tpl_sgraph.H>
# include <tpl_dynSetTree.H>
# include <generate_graph.H>


// TODO: colocar filtro para iterador de arcos

    template <class GT>
class Karger_Min_Cut
{
      // cada nodo tiene la lista de nodos comprimidos
  typedef Graph_Node<DynList<typename GT::Node*>> Knode;
  typedef Graph_Arc<typename GT::Arc*> Karc;
  typedef List_Graph<Knode, Karc> Kgraph;
  
  typedef typename GT::Node Node;
  typedef typename GT::Arc Arc;

  unsigned long seed;
  gsl_rng * r;

  void build_kgraph(GT & g, Kgraph & kg, DynSetTreapRk<Karc*> & arcs)
  {
    clear_graph(kg);
    arcs.empty();
    g.reset_nodes();
    g.reset_arcs();

    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
	auto p = it.get_curr();
	auto q = kg.insert_node();
	q->get_info().append(p);
	g.map_nodes(p, q);
      }

    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      {
	auto a = it.get_curr();
	auto s = mapped_node<GT, Kgraph>(g.get_src_node(a));
	auto t = mapped_node<GT, Kgraph>(g.get_tgt_node(a));
	auto ka = kg.insert_arc(s, t, a);
	arcs.insert(ka);
      }
  }

  void update_arcs(Kgraph & kg, Knode * p, Knode * t, Knode * cp,
		   DynSetTreapRk<Karc*> & arcs)
  {
    for (typename Kgraph::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
      {
	auto pa = it.get_curr();
	auto tgt = it.get_tgt_node_ne();
	
	arcs.remove(pa); // se elimina del índice; del grafo cuando se
	                 // eliminen los nodos
	if (tgt == t)
	  continue; // arco paralelo ==> se ignora

	auto ka = kg.insert_arc(cp, tgt, pa->get_info());
	arcs.insert(ka); 
      }
  }

  void contract(Kgraph & kg, const unsigned long & left_num_nodes,
                DynSetTreapRk<Karc*> & arcs)
  {
    while (kg.get_num_nodes() > left_num_nodes)
      {    // selecciona al azar un arco de kg
        auto num_arc = gsl_rng_uniform_int(r, kg.get_num_arcs());
        auto a = arcs.select(num_arc); // arco a eliminar
        auto s = kg.get_src_node(a); // los nodos a "contraer"
        auto t = kg.get_tgt_node(a);

        arcs.remove(a);     // elimina de kg y del índice
        kg.remove_arc(a);   

        auto cp = kg.insert_node(); // nuevo nodo contraido que representa s-t 

        update_arcs(kg, s, t, cp, arcs);
        update_arcs(kg, t, s, cp, arcs);

        cp->get_info().swap(s->get_info());
        cp->get_info().append(t->get_info());

        kg.remove_node(s);
        kg.remove_node(t);
      }
  }
  
  int karger_min_cut(GT & g,
		     DynList<typename GT::Node*> & vs, 
		     DynList<typename GT::Node*> & vt,
		     DynList<typename GT::Arc*> & cut,
		     size_t num_iter)
  {
    if (g.get_num_arcs() == 0)
      throw std::domain_error("Graph has not arcs");

    auto min_cut = numeric_limits<size_t>::max();

    for (int i = 0; i < num_iter; ++i)
      {
	Kgraph kg;
	DynSetTreapRk<Karc*> arcs; // arcos para rápida selección
	build_kgraph(g, kg, arcs);

	contract(kg, 2, arcs);

	auto cut_size = kg.get_num_arcs();

	if (cut_size >= min_cut)
	  continue;

	min_cut = cut_size; 
	
	    // actualizamos corte mìnimo
	cut.empty();

	    // recorremos los arcos de los super nodos (son los del corte)
	for (typename Kgraph::Arc_Iterator it(kg); it.has_curr(); it.next_ne())
	  {
	    auto ka = it.get_curr();
      	    assert(kg.get_src_node(ka) != kg.get_tgt_node(ka));

	    cut.append(ka->get_info());
	  }

	auto ka = kg.get_first_arc();
	auto S = kg.get_src_node(ka);
	auto T = kg.get_tgt_node(ka);

	assert(S->get_info().size() + T->get_info().size() == 
	       g.get_num_nodes());

	vs.empty();
	vt.empty();
	vs.swap(S->get_info());
	vt.swap(T->get_info());
      }
    return min_cut; 
  }

  int __fast_karger_min_cut(Kgraph & kg, DynSetTreapRk<Karc*> & arcs)
  {
    const auto & n = kg.get_num_nodes();

    if (n <= 6)
      {
        // Calcular corte mínimo por enumeración a fuerza bruta.
        return 0;
      }

    const size_t t = std::ceil(1 + 1.0 * n / std::sqrt(2));

    Kgraph h1(kg);
    DynSetTreapRk<Karc*> arcs1(arcs);
    contract(h1, t, arcs1);
    int cut1 = __fast_karger_min_cut(h1, arcs1);

    Kgraph h2(kg);
    DynSetTreapRk<Karc*> arcs2(arcs);
    contract(h2, t, arcs2);
    int cut2 = __fast_karger_min_cut(h2, arcs2);

    if (cut1 < cut2)
      {
        kg.swap(h1);
        arcs.swap(arcs1);
        return cut1;
      }

    kg.swap(h2);
    arcs.swap(arcs2);
    return cut2;
  }

  int fast_karger_min_cut(GT & g,
                          DynList<typename GT::Node*> & vs, 
                          DynList<typename GT::Node*> & vt,
                          DynList<typename GT::Arc*> & cut)
  {
    Kgraph kg;
    DynSetTreapRk<Karc*> arcs; // arcos para rápida selección
    build_kgraph(g, kg, arcs);

    int min_cut = __fast_karger_min_cut(kg, arcs);

    assert(min_cut == kg.get_num_arcs());

    for (typename Kgraph::Arc_Iterator it(kg); it.has_curr(); it.next_ne())
      {
        auto ka = it.get_curr();
	assert(kg.get_src_node(ka) != kg.get_tgt_node(ka));
        cut.append(ka->get_info());
      }

    auto ka = kg.get_first_arc();

    auto S = kg.get_src_node(ka);
    auto T = kg.get_tgt_node(ka);

    assert(S->get_info().size() + T->get_info().size() == g.get_num_nodes());

    vs.swap(S->get_info());
    vt.swap(T->get_info());
  }

public:

  Karger_Min_Cut(const unsigned long & _seed = time(nullptr))
    : seed(_seed), r(gsl_rng_alloc(gsl_rng_mt19937))
  {
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  ~Karger_Min_Cut()
  {
    gsl_rng_free(r);
  }

  int operator () (GT & g,
		   DynList<typename GT::Node*> & vs, 
		   DynList<typename GT::Node*> & vt,
		   DynList<typename GT::Arc*> & cut,
		   size_t num_iter)
  {
    return karger_min_cut(g, vs, vt, cut, num_iter);
  }

  int operator () (GT & g,
		   DynList<typename GT::Node*> & vs, 
		   DynList<typename GT::Node*> & vt,
		   DynList<typename GT::Arc*> & cut)
  {
    const size_t & n = g.get_num_nodes();
    size_t num_iter = 1.05*n*n;
    return karger_min_cut(g, vs, vt, cut, num_iter);
  }
};



# endif // KARGER_H
