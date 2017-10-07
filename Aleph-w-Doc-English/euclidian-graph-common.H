
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file euclidian-graph-common.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
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
    /* Empty */
  }

  void operator () (GT &, typename GT::Node * p)
  {
    unsigned long x, y;
    while (true)
      {
	x = gsl_rng_uniform_int(rand_gen, H);
	y = gsl_rng_uniform_int(rand_gen, W);
	std::pair<int,int> q(x, y);
	if (puntos.search(q) != NULL)
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
  
  GT g = Random_Graph<GT, Init_P<GT>, Init_Arc<GT>> 
    (seed, initp, initarc) (n, m);

  gsl_rng_free(rand_gen);
  rand_gen = NULL;  

  return std::move(g);
}


} /* End namespace Aleph */

# endif //  EUCLIDIAN_GRAPH_COMMON_H
