
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file random_net.H
 *  @brief Random network flow graph generation.
 *
 *  Generates random capacitated networks for flow algorithm testing. The
 *  numbers come from a Gsl_Compatible_Rng, which gives those of the
 *  `gsl_rng_mt19937` of GSL and its draws: a seed gives the network that
 *  it gave when they came from GSL.
 *
 *  @see random_graph.H Random graph generation
 *  @see tpl_net.H Network structures
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef RANDOM_NET_H
# define RANDOM_NET_H

# include <memory>
# include <stdexcept>
# include <algorithm>
# include <tpl_dynArray.H>
# include <tpl_dynSetTree.H>
# include <tpl_netcost.H>
# include <tpl_indexArc.H>
# include <ah-errors.H>
# include <ah-random.H>


template <class Net>
class Random_Network_Flow
{
  typedef typename Net::Node Node;
  typedef typename Net::Arc Arc;

  DynArray<DynArray<Node *>> rank;
  Aleph::Gsl_Compatible_Rng_Owner r; // the generator, in dynamic memory
  std::unique_ptr<Net> net;

public:
  Random_Network_Flow(const unsigned int seed = time(nullptr))
    : r(seed % Aleph::Gsl_Compatible_Rng::max()), net(new Net)
  {
    // empty
  }

  /// Not copyable, nor movable: it owns its generator and its network.
  Random_Network_Flow(const Random_Network_Flow &) = delete;

  /// Not assignable, as it is not copyable.
  Random_Network_Flow & operator = (const Random_Network_Flow &) = delete;

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
        size_t num_arcs = r->exponential(lambda);
        num_arcs = std::min(num_arcs, num_nodes_tgt_rank);
        for (int k = 0; k < num_arcs; ++k)
          {
          repeat:
            auto tgt_idx = r->uniform_int(num_nodes_tgt_rank);
            auto tgt = tgt_rank.access(tgt_idx);
            if (arcs.search(src, tgt) != nullptr)
              goto repeat;

            const double cap = r->gaussian(cap_sigma) + cap_mean;
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
        const double mu = r->gaussian(rank_sigma * num_nodes_by_rank);
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
     @param[in] cap_mean Mean of the capacity of an arc
     @param[in] cap_sigma Standard deviation of the capacity of an arc
     @param[in] forward_density Density of the arcs from a rank to the next
                one, in (0, 1), and not less than backward_density
     @param[in] backward_density Density of the arcs from a rank to the
                previous one, in (0, 1)
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
