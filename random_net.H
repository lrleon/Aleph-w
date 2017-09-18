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


template <class Net>
class Random_Network_Flow
{
  typedef typename Net::Node Node;
  typedef typename Net::Arc Arc;

  DynArray<DynArray<Node*>> rank;
  gsl_rng * r;
  unique_ptr<Net> net;

public:

  Random_Network_Flow(unsigned int seed = time(nullptr)) 
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
	auto src = src_rank.access(i);
	const double lambda = density*num_nodes_tgt_rank;
	size_t num_arcs = gsl_ran_exponential(r, lambda);
	num_arcs = std::min(num_arcs, num_nodes_tgt_rank);
	for (int k = 0; k < num_arcs; ++k)
	  {
	  repeat:
	    auto tgt_idx = gsl_rng_uniform_int(r, num_nodes_tgt_rank);
	    auto tgt = src_rank.access(tgt_idx);
	    if (arcs.search(src, tgt) != nullptr)
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

      // create num_ranks each one with num_nodes_by_rank according to the
      // mean of a normal with rank_sigma as deviation
  void create_ranks(size_t         num_ranks, 
		    size_t         num_nodes_by_rank, 
		    const double & rank_sigma)
  {
    if (rank_sigma < 0)
      throw std::domain_error("Rank sigma " + to_string(rank_sigma) +
			      " less than 0");
    if (rank_sigma > 1)
      throw std::domain_error("Rank sigma " + to_string(rank_sigma) +
			      " greater than 1");
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

      /// After created ranks and their nodes, this routine creates the
      /// arcs. Forward arcs betwen an rank and another are randomly
      /// generated according an exponential with parameter
      /// forward_density*total_nodes. The arc capacity is generated
      /// according to a normal with mean cap_mean and deviation cap_sigma
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

     La cantidad de nodos por rango es determinada por una distribución
     normal con media num_nodes_by_rank y desviación sigma. La
     desviación sigma debe ser un valor entre 0 y 1. El valor de sigma
     que se una para la distribución normal será
     rank_sigma*num_nodes_by_rank. 

     Las capacidades de los arcos son seleccionadas según una
     distribución normal con media cap_mean y desviación cap_sigma.

     Los arcos de adelanto entre un rango y otro se corresponden con una
     distribución exponencial con parámetro
     forward_density*num_nodes_by_rank.


     @param[in] sigma desviación estándard del número de nodos por rango
     @param[in] cap_mean media de la capacidad por arco.
     @param[in] sigma_cap desviación estándar de la capacidadde arcos
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
