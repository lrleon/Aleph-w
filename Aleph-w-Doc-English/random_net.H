
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
  @file random_net.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef RANDOM_NET_H
# define RANDOM_NET_H

# include <memory>
# include <stdexcept>
# include <algorithm>
# include <tpl_dynArray.H>
# include <tpl_dynSetTree.H>
# include <tpl_maxflow_mincost.H>
# include <tpl_indexArc.H>

# include <autosprintf.h>
# include <gsl/gsl_rng.h> 
# include <gsl/gsl_randist.h>


template <class Net>
class Random_Network_Flow
{
  typedef typename Net::Node Node;
  typedef typename Net::Arc Arc;

  DynArray<DynArray<Node*>> rank;
  gsl_rng * r;
  unique_ptr<Net> net;

public:

  Random_Network_Flow(unsigned int seed = time(NULL)) 
    : r(gsl_rng_alloc (gsl_rng_mt19937)), net(new Net)
  {
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  ~Random_Network_Flow()
  {
    gsl_rng_free(r);
  }

private:

  void connect_ranks(size_t         src_idx_rank,
		     size_t         tgt_idx_rank,
		     const double & cap_mean,
		     const double & cap_sigma,
		     const double & density)
  {
    IndexArc<Net> arcs(*net.get());
    DynArray<Node*> src_rank = rank.access(src_idx_rank);
    DynArray<Node*> tgt_rank = rank.access(tgt_idx_rank);
    const size_t & num_nodes_src_rank = src_rank.size();
    const size_t & num_nodes_tgt_rank = tgt_rank.size();
    for (int i = 0; i < num_nodes_src_rank; ++i)
      {
	Node * src = src_rank.access(i);
	const double lambda = density*num_nodes_tgt_rank;
	size_t num_arcs = gsl_ran_exponential(r, lambda);
	num_arcs = std::min(num_arcs, num_nodes_tgt_rank);
	for (int k = 0; k < num_arcs; ++k)
	  {
	  repeat:
	    int tgt_idx = gsl_rng_uniform_int(r, num_nodes_tgt_rank);
	    Node * tgt = src_rank.access(tgt_idx);
	    if (arcs.search(src, tgt) != NULL)
	      goto repeat;

	    const double cap = gsl_ran_gaussian(r, cap_sigma) + cap_mean;
	    net->insert_arc(src, tgt, cap);
	  }
      }
  }

  void create_forward_arcs_in_rank(size_t         curr_rank_idx,
				   const double & cap_mean,
				   const double & cap_sigma,
				   const double & forward_density)
  {
    assert(curr_rank_idx < rank.size() - 1);
    connect_ranks(curr_rank_idx, curr_rank_idx + 1, 
		  cap_mean, cap_sigma, forward_density);
  }

  void create_backward_arcs_in_rank(size_t         curr_rank_idx,
				    const double & cap_mean,
				    const double & cap_sigma,
				    const double & backward_density)
  {
    assert(curr_rank_idx > 0);
    connect_ranks(curr_rank_idx, curr_rank_idx - 1, 
		  cap_mean, cap_sigma, backward_density);    
  }

/** 
  @brief Create num_ranks each one with num_nodes_by_rank according to the
  mean of a normal with rank_sigma as deviation 
*/
  void create_ranks(size_t         num_ranks, 
		    size_t         num_nodes_by_rank, 
		    const double & rank_sigma)
  {
    if (rank_sigma < 0)
      throw std::domain_error(gnu::autosprintf("Rank sigma %d less than 0",
					       rank_sigma));
    if (rank_sigma > 1)
      throw std::domain_error(gnu::autosprintf("Rankl sigma %d greater than 1",
					       rank_sigma));
    for (int i = 0; i < num_ranks; ++i)
      {
	const double mu = gsl_ran_gaussian(r, rank_sigma*num_nodes_by_rank); 
	const size_t num_nodes = num_nodes_by_rank + mu; 
	rank.touch(i).reserve(num_nodes);
	DynArray<Node*> & curr_rank = rank.access(i);
	for (int k = 0; k < num_nodes; ++k)
	  curr_rank.access(k) = net->insert_node();
      } 
  }

/**
  @note After created ranks and their nodes, this routine creates the arcs. 
  Forward arcs betwen an rank and another are randomly
  generated according an exponential with parameter
  forward_density*total_nodes. The arc capacity is generated
  according to a normal with mean cap_mean and deviation cap_sigma
*/
  void create_arcs(const double & cap_mean,
		   const double & cap_sigma,
		   const double & forward_density,
		   const double & backward_density)
  {
    if (forward_density < backward_density)
      throw std::domain_error("forward density less than backward density");
    if (forward_density <= 0 or forward_density >= 1)
      throw std::domain_error("forward density out of range");
    if (backward_density <= 0 or backward_density >= 1)
      throw std::domain_error("backward density out of range");

    const size_t & N = rank.size();
    create_forward_arcs_in_rank(0, cap_mean, cap_sigma, forward_density);
    for (int i = 1; i < N - 1; ++i)
      {
	create_forward_arcs_in_rank(i, cap_mean, cap_sigma, forward_density);
	create_backward_arcs_in_rank(i, cap_mean, cap_sigma, backward_density);
      }
    create_backward_arcs_in_rank(N - 1, cap_mean, cap_sigma, backward_density);
  }

public:

/**
  @brief The number of nodes is determined by a range distribution
  with standard deviation sigma and average num_nodes_by_rank. 

  @details The sigma deviation must be a value between 0 and 1. The value of sigma
  that is one for the normal distribution will
  rank_sigma * num_nodes_by_rank.

  The capacities of the arcs are selected according to
  normal distribution with mean deviation cap_mean and cap_sigma.

  The arches of advancement within a range and another correspond to a
  exponential distribution with parameter
  forward_density * num_nodes_by_rank.

  @param[in] sigma Standard deviation of the number of nodes per range
  @param[in] cap_mean Media arc capacity.
  @param[in] sigma_cap Standard deviation of the arches capacidadde
*/
  Net * operator () (size_t         num_ranks, 
		     size_t         num_nodes_by_rank,  
		     const double & rank_sigma       = 0.2,
		     const double & cap_mean         = 100,
		     const double & cap_sigma        = 0.9,
		     const double & forward_density  = 0.3, 
		     const double & backward_density = 0.01)
  {
    create_ranks(num_ranks, num_nodes_by_rank, rank_sigma);
    create_arcs(cap_mean, cap_sigma, forward_density, backward_density);

    return net.release();
  }

};




# endif // RANDOM_NET_H
