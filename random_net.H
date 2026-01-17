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

/** @file random_net.H
 *  @brief Random network flow graph generation.
 *
 *  Generates random capacitated networks for flow algorithm testing.
 *
 *  @see random_graph.H Random graph generation
 *  @see tpl_net.H Network structures
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef RANDOM_NET_H
# define RANDOM_NET_H

# include <gsl/gsl_rng.h>
# include <gsl/gsl_randist.h>

# include <memory>
# include <stdexcept>
# include <algorithm>
# include <tpl_dynArray.H>
# include <tpl_dynSetTree.H>
# include <tpl_netcost.H>
# include <tpl_indexArc.H>
# include <ah-errors.H>


template <class Net>
class Random_Network_Flow
{
  typedef typename Net::Node Node;
  typedef typename Net::Arc Arc;

  DynArray<DynArray<Node *>> rank;
  gsl_rng *r;
  std::unique_ptr<Net> net;

public:
  Random_Network_Flow(const unsigned int seed = time(nullptr))
    : r(gsl_rng_alloc(gsl_rng_mt19937)), net(new Net)
  {
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  ~Random_Network_Flow()
  {
    gsl_rng_free(r);
  }

private:
  void connect_ranks(size_t src_idx_rank,
                     size_t tgt_idx_rank,
                     const double & cap_mean,
                     const double & cap_sigma,
                     const double & density)
  {
    IndexArc<Net> arcs(*this->net.get());
    DynArray<Node *> src_rank = rank.access(src_idx_rank);
    DynArray<Node *> tgt_rank = rank.access(tgt_idx_rank);
    const size_t & num_nodes_src_rank = src_rank.size();
    const size_t & num_nodes_tgt_rank = tgt_rank.size();
    for (int i = 0; i < num_nodes_src_rank; ++i)
      {
        auto src = src_rank.access(i);
        const double lambda = density * num_nodes_tgt_rank;
        size_t num_arcs = gsl_ran_exponential(r, lambda);
        num_arcs = std::min(num_arcs, num_nodes_tgt_rank);
        for (int k = 0; k < num_arcs; ++k)
          {
          repeat:
            auto tgt_idx = gsl_rng_uniform_int(r, num_nodes_tgt_rank);
            auto tgt = tgt_rank.access(tgt_idx);
            if (arcs.search(src, tgt) != nullptr)
              goto repeat;

            const double cap = gsl_ran_gaussian(r, cap_sigma) + cap_mean;
            this->net->insert_arc(src, tgt, cap);
          }
      }
  }

  void create_forward_arcs_in_rank(size_t curr_rank_idx,
                                   const double & cap_mean,
                                   const double & cap_sigma,
                                   const double & forward_density)
  {
    assert(curr_rank_idx < rank.size() - 1);
    connect_ranks(curr_rank_idx, curr_rank_idx + 1,
                  cap_mean, cap_sigma, forward_density);
  }

  void create_backward_arcs_in_rank(const size_t curr_rank_idx,
                                    const double & cap_mean,
                                    const double & cap_sigma,
                                    const double & backward_density)
  {
    assert(curr_rank_idx > 0);
    connect_ranks(curr_rank_idx, curr_rank_idx - 1,
                  cap_mean, cap_sigma, backward_density);
  }

  // create num_ranks each one with num_nodes_by_rank according to the
  // mean of a normal with rank_sigma as deviation
  void create_ranks(const size_t num_ranks,
                    const size_t num_nodes_by_rank,
                    const double & rank_sigma)
  {
    ah_domain_error_if(rank_sigma < 0)
      << "Rank sigma " + std::to_string(rank_sigma) + " less than 0";
    ah_domain_error_if(rank_sigma > 1)
      << "Rank sigma " + std::to_string(rank_sigma) + " greater than 1";
    for (int i = 0; i < num_ranks; ++i)
      {
        const double mu = gsl_ran_gaussian(r, rank_sigma * num_nodes_by_rank);
        const size_t num_nodes = num_nodes_by_rank + mu;
        rank.touch(i).reserve(num_nodes);
        DynArray<Node *> & curr_rank = rank.access(i);
        for (int k = 0; k < num_nodes; ++k)
          curr_rank.access(k) = this->net->insert_node();
      }
  }

  /// After created ranks and their nodes, this routine creates the
  /// arcs. Forward arcs between a rank and another are randomly
  /// generated according an exponential with parameter
  /// forward_density*total_nodes. The arc capacity is generated
  /// according to a normal with mean cap_mean and deviation cap_sigma
  void create_arcs(const double & cap_mean,
                   const double & cap_sigma,
                   const double & forward_density,
                   const double & backward_density)
  {
    ah_domain_error_if(forward_density < backward_density)
      << "forward density less than backward density";
    ah_domain_error_if(forward_density <= 0 or forward_density >= 1)
      << "forward density out of range";
    ah_domain_error_if(backward_density <= 0 or backward_density >= 1)
      << "backward density out of range";

    const size_t & N = rank.size();
    
    // Need at least 2 ranks to create inter-rank arcs
    if (N < 2)
      return;
      
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

    The number of nodes per rank is determined by a distribution
     normal with half num_nodes_by_rank and sigma deviation. The
     sigma deviation should be a value between 0 and 1. The value of sigma
     that joins for normal distribution will be
     rank_sigma*num_nodes_by_rank.

    The capacities of the arches are selected according to a
     Normal distribution with mean cap_mean and deviation cap_sigma.

     The arcs of advancement between one rank and another correspond to a
     Exponential distribution with parameter
     forward_density*num_nodes_by_rank.


     @param[in] num_ranks Number of ranks in the network
     @param[in] num_nodes_by_rank Average number of nodes per rank
     @param[in] rank_sigma Standard deviation of the number of nodes per rank (0-1)
     @param[in] cap_mean media de la capacidad por arco
     @param[in] cap_sigma desviación estándar de la capacidad de arcos
     @param[in] forward_density densidad de arcos hacia adelante (0-1)
     @param[in] backward_density densidad de arcos hacia atrás (0-1)
     @return Pointer to the generated network
   */
  Net * operator ()(const size_t num_ranks,
                    const size_t num_nodes_by_rank,
                    const double & rank_sigma = 0.2,
                    const double & cap_mean = 100,
                    const double & cap_sigma = 0.9,
                    const double & forward_density = 0.3,
                    const double & backward_density = 0.01)
  {
    // Create fresh network for this generation
    this->net.reset(new Net);
    rank.empty();
    
    create_ranks(num_ranks, num_nodes_by_rank, rank_sigma);
    create_arcs(cap_mean, cap_sigma, forward_density, backward_density);

    return this->net.release();
  }
};


# endif // RANDOM_NET_H
