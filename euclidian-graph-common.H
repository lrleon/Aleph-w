

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

# ifndef EUCLIDIAN_GRAPH_COMMON_H
# define  EUCLIDIAN_GRAPH_COMMON_H

# include <tpl_sgraph.H>
# include <io_graph.H>
# include <random_graph.H>

namespace Aleph {

struct My_P
{
  int x, y;
};

extern gsl_rng * rand_gen ;

  template <class GT>
struct Init_P
{
  int W, H;

  DynSetAvlTree<std::pair<int, int>> puntos;

  Init_P(unsigned long w, unsigned long h)
    : W(w), H(h)
  {
    // empty
  }

  void operator () (GT &, typename GT::Node * p)
  {
    unsigned long x, y;
    while (true)
      {
	x = gsl_rng_uniform_int(rand_gen, H);
	y = gsl_rng_uniform_int(rand_gen, W);
	std::pair<int,int> q(x, y);
	if (puntos.search(q) != nullptr)
	  continue;

	puntos.insert(q);
	break;
      }
    
    My_P & my_p = p->get_info();
    my_p.x = x;
    my_p.y = y;
  }
};

  template <class GT>
struct Init_Arc
{
  int max_offset;

  Init_Arc(int max) : max_offset(max) { }

  void operator () (GT & g, typename GT::Arc * a)
  {
    typename GT::Node * src = g.get_src_node(a);
    typename GT::Node * tgt = g.get_tgt_node(a);

    My_P & psrc = src->get_info();
    My_P & ptgt = tgt->get_info();

    float dist = hypot(psrc.x - ptgt.x, psrc.y - ptgt.y);

    int offset = gsl_rng_uniform_int(rand_gen, max_offset);

    a->get_info() = dist + offset;
  }
};

  template <class GT>
struct Wnode
{
  void operator () (ostream & output, GT &, typename GT::Node * p)
  {
    output << p->get_info().x << " " << p->get_info().y << endl;
  }
};

  template <class GT>
struct Rnode
{
  void operator () (istream & input, GT &, typename GT::Node * p)
  {
    input >> p->get_info().x;
    input >> p->get_info().y;
  }
};

  template <class GT>
struct Warc
{
  void operator () (ostream & output, GT &, typename GT::Arc * a)
  {
    output << a->get_info() << endl;
  }
};

  template <class GT>
struct Rarc
{
  void operator () (istream & input, GT &, typename GT::Arc * a)
  {
    input >> a->get_info();
  }
};


     template <class GT> inline
GT gen_random_euclidian_graph(size_t n, size_t m, int w, int h, 
				unsigned int seed)
{
  rand_gen = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(rand_gen, seed % gsl_rng_max(rand_gen));

  Init_P<GT> initp(w, h);
  Init_Arc<GT> initarc(hypot(w, h));
  
  GT g = Random_Graph<GT, Init_P<GT>, Init_Arc<GT>>(seed, initp, initarc)(n, m);

  gsl_rng_free(rand_gen);
  rand_gen = nullptr;  

  return g;
}


} // end namespace Aleph 

# endif //  EUCLIDIAN_GRAPH_COMMON_H
