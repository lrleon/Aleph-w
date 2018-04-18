
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
  @file random_graph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef RANDOM_GRAPH_H
# define RANDOM_GRAPH_H

# include <gsl/gsl_rng.h> 
# include <memory>
# include <tpl_indexArc.H>
# include <tpl_graph_utils.H>
# include <tpl_components.H>
# include <single_graph.H>
# include <Tarjan.H>

namespace Aleph 
{

    template <class GT>
struct Dft_Init_Rand_Node
{
  void operator () (GT &, typename GT::Node *) const
  {
    /* Empty */
  }
};


    template <class GT>
struct Dft_Init_Rand_Arc
{
  void operator () (GT &, typename GT::Arc *) const
  {
    /* Empty */
  }
};

  /* TODO: init_node and Init_Arc eliminate classes and instead use lambdas */

  template <class GT, class Init_Node, class Init_Arc>
class Random_Graph_Base
{
protected:

  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc GT_Arc;

  gsl_rng * r;

  Init_Node & init_node;
  Init_Arc &  init_arc;

  unique_ptr<DynArray<GT_Node*>> nodes; /* Pointer to save directory if unused */

  unique_ptr<IndexArc<GT>> idx_arc; /* Pointer because the builder, it requires the graph. */
  mutable size_t num_nodes;
  mutable size_t num_arcs;
  mutable unsigned long rand_max; 

  GT g;

/** 
  @brief It indicates whether or not to keep parity relations between nodes,
  Just to build Eulerian and Hamiltonian
*/
  bool save_parity;


       virtual void 
  update_parity_after_arc_insertion(GT_Node * src, GT_Node * tgt) = 0;

  GT_Arc * insert_arc(GT_Node * src, GT_Node * tgt)
  {
    GT_Arc * a = idx_arc->insert(g.insert_arc(src, tgt));

    init_arc (g, a);

    update_parity_after_arc_insertion(src, tgt);

    return a;
  }

/** 
  @brief Randomly select a different node to excluded
*/
  GT_Node * select_random_node(GT_Node * excluded = NULL)
  {
    assert(nodes.get() != NULL);

    unsigned long idx;
    GT_Node * ret_val = NULL;
    while (true)
      {
	idx = gsl_rng_uniform_int(r, num_nodes);

	ret_val = nodes->access(idx);

	if (excluded == NULL or ret_val != excluded)
	  break;
      }

    return ret_val;
  }

  /** 
    @brief Randomly select a node from the list contained in the list
  */
  GT_Node * select_random_node(DynList<GT_Node*> & list)
  {
    const unsigned long k = gsl_rng_uniform_int(r, list.size());

    typename DynList<GT_Node*>::Iterator it(list);
    for (int i = 0; i < k; ++i, it.next()) ;

    return it.get_curr();
  }

  virtual void create_nodes_and_initialize_arc_index() = 0;

  virtual void connect() = 0;

  void initialize_and_create_nodes(const size_t & __num_nodes, 
				   const size_t & __num_arcs)
  {
    num_nodes = __num_nodes;

    const size_t num_nodes_2 = num_nodes*num_nodes;
    if (g.is_digraph())
      num_arcs = std::min(__num_arcs, num_nodes_2 - num_nodes);
    else
      num_arcs= std::min(__num_arcs, (num_nodes_2 - num_nodes)/2);
    
    create_nodes_and_initialize_arc_index();
  }

  Random_Graph_Base(unsigned long seed,
		    const Init_Node & __init_node,
		    const Init_Arc  & __init_arc) 
    : r(gsl_rng_alloc (gsl_rng_mt19937)), 
      init_node(const_cast<Init_Node&>(__init_node)), 
      init_arc(const_cast<Init_Arc&>(__init_arc)),
      num_nodes(0), num_arcs(0),
      rand_max(gsl_rng_max(r)), save_parity(false) 
  {
    if (r == NULL)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % rand_max);
  }

  ~Random_Graph_Base()
  {
    if (r != NULL)
      gsl_rng_free(r);
  }

/** 
  @brief Spread creates a random graph. 
*/
  GT create(const size_t & __num_nodes, const size_t & __num_arcs, 
	    bool connected)
  {
    initialize_and_create_nodes(__num_nodes, __num_arcs);

    /** Arches inserted randomly selecting random pairs of nodes */
    for (int i = 0; i < num_arcs; ++i)
      {
	GT_Node * src = select_random_node();
	GT_Node * tgt = select_random_node(src);

	if (idx_arc->search(src, tgt) == NULL) /* The arc is repeated? */
	  insert_arc(src, tgt);
      }

    if (connected)
      connect();
    
    return move(g);
  }

  virtual GT create_p(const size_t & __num_nodes, const double & p, 
		      bool connected) = 0;

  virtual void make_eulerian() = 0;

  virtual void make_hamiltonian () = 0;

public:

/** 
  @brief Creates a random graph is Eulerian.

  @details This version builds a random graph that spread
  guarantee is Eulerian; ie containing rings Euler.

  The process first generates a graph scattered
  random, then examines the resulting graph and make
  arches so that the output contains an Eulerian cycle.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] __num_nodes Number of arches that must be the graph.
  
  @return A pointer to random graph created.
  
  @throw bad_alloc If there is insufficient memory.
*/
  GT eulerian(const size_t & __num_nodes, const size_t & __num_arcs) 
  {
    save_parity = true;

    g = std::move(this->create(__num_nodes, __num_arcs, true));

    make_eulerian();

    return move(g);
  }

/** 
  @brief Creates a random graph is Eulerian.

  @details This version builds a random graph with Euler
  existence probability p arc between any two nodes.

  Wear this routine to generate dense graphs. In this case,
  To be used a value greater than or equal to 0.5 p.

  @warning The procedure can be extremely slow and sweet
  in memory as increase the value of __num_nodes.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] p Probability of occurrence of arcing between any
  pair of nodes.
  
  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the value of p is between 0 and 1.
*/
  GT eulerian (const size_t & __num_nodes, const double & p) 
  {
    save_parity = true;

    g = std::move(this->create_p(__num_nodes, p, true));

    make_eulerian();

    return move(g);
  }

/** 
  @brief Create a Hamiltonian random graph.

  @details This version builds a random graph Hamiltonian.

  The process first generates a random graph, then examines the
  resulting graph and make arches so that the result contains a 
  Hamiltonian cycle.

  @warning The procedure can be extremely slow and sweet
  in memory with the increase in the value of __num_nodes.
  @warning The graph is secured to be Hamiltonian by theorems
  Ore and Dirac. In this regard, note that the condition
  Hamiltonian is of sufficiency, which means that it is not
  minimal graph. In fact, the graph generated tends to be dense.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] p Probability of occurrence of arcing between any
  pair of nodes. By default this parameter value of 0.5, value
  intuitively and empirically that tends to generate lower arches and
  less time consuming.

  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the value of p is between 0 and 1.

  @ingroup Grafos
*/
  GT sufficient_hamiltonian (const size_t & __num_nodes, 
			     const double & p = 0.5) 
  {
    g = std::move(this->create_p(__num_nodes, p, true));

    make_hamiltonian();

    return move(g);
  }
};

 
/** 
  @brief Generator of random graphs.

  @details Random_Graph class is a generator of random graphs
  num_nodes and the type of operation being invoked to num_arcs.

  Class exports two methods of generating, through the operator
  parentheses, for the generation of scattered graphs and
  dense respectively.

  For the compile class must exist builders
  GT::Node_Type y GT::Arc_Type.

  The parameters of the class are:
  - GT: Building type graph.
  - Init_Node: Class initialization of the node data.
  - Init_Arc: Class initialization data arc.
  .

  La clase Init_Node have the following struct
  @code
  struct Init_Node
  {
    void operator () (GT & g, GT::Node * p) { ... }
  @endcode

  La clase Init_Arc tiene la siguiente estructura:
  @code
  struct Init_Arc
  {
    void operator () (GT & g, GT::Arc * a) { ... }
  @endcode

  Generation routines contain a logical parameter
  the graph determines whether or not to be connected. By default the
  parameter is true. Take that into consideration if it is not necessary
  that the graph is connected, then to the connected graph is
  It requires determining the unconnected components, which requires
  more time and memory consumption.
  
  @ingroup Grafos
*/
  template <class GT, 
	    class Init_Node = Dft_Init_Rand_Node<GT>,
	    class Init_Arc = Dft_Init_Rand_Arc<GT> >
class Random_Graph : public Random_Graph_Base<GT, Init_Node, Init_Arc>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc GT_Arc;

  DynSetRandTree<GT_Node*> odd_nodes;  /* Nodes with odd degree*/
  DynSetRandTree<GT_Node*> even_nodes; /* Nodes with even degree */
  
  virtual void update_parity_after_arc_insertion(GT_Node * src, GT_Node * tgt)
  {
    if (not this->save_parity)
      return;

    if (is_even(this->g.get_num_arcs(src)))
      { /* It was odd before insertion */
	this->odd_nodes.remove(src);
	this->even_nodes.insert(src);
      }
    else
      {
	this->even_nodes.remove(src);
	this->odd_nodes.insert(src);
      }

    if (is_even(this->g.get_num_arcs(tgt)))
      { /* It was odd before insertion */
	this->odd_nodes.remove(tgt);
	this->even_nodes.insert(tgt);
      }
    else
      {
	this->even_nodes.remove(tgt);
	this->odd_nodes.insert(tgt);
      }
  }

  virtual void create_nodes_and_initialize_arc_index()
  {
    this->nodes = unique_ptr<DynArray<GT_Node*>>
      (new DynArray<GT_Node*>(this->num_nodes));

    this->nodes->reserve(this->num_nodes);

    for (int i = 0; i < this->num_nodes; ++i)
      {
	typename GT::Node * p = this->g.insert_node(new GT_Node);
	this->nodes->access(i) = p;
	this->init_node(this->g, p);

	if (this->save_parity)
	  {
	    this->even_nodes.insert(p);
	    NODE_COUNTER(p) = 0;
	  }
      }

    this->idx_arc = unique_ptr<IndexArc<GT>> (new IndexArc<GT>(this->g) ); 
  }

  virtual void connect()
  {
    DynList<DynList<GT_Node*>> subgraphs; 

    Inconnected_Components <GT> () (this->g, subgraphs);

    const size_t & num_subs = subgraphs.size();

    if (num_subs == 1)
      return;

    DynArray<GT_Node*> block_nodes;

    for (typename DynList<DynList<GT_Node*>>::Iterator it(subgraphs); 
	 it.has_curr(); it.next())
      block_nodes.append(this->select_random_node(it.get_curr()));
    
    for (int i = 1; i < num_subs; ++i)
      {
	typename GT::Node * src = block_nodes.access(i - 1);
	typename GT::Node * tgt = block_nodes.access(i);

	this->insert_arc(src, tgt);
      }
  }

/** 
  @brief Creates a random graph in which the probability 
  rc between a pair of nodes is p
*/
  virtual GT create_p (const size_t & __num_nodes, const double & p, 
		       bool connected)
  {
    if (p > 1.0 or p <= 0.0)
      throw std::domain_error("Invalid value for p");

    this->initialize_and_create_nodes(__num_nodes, __num_nodes);

    for (int i = 0; i < this->num_nodes - 1; ++i)
      {
	GT_Node * src = this->nodes->access(i);
	for (int j = i + 1; j < this->num_nodes; ++j)
	  if (gsl_rng_uniform(this->r) <= p) /* Sorteo */
	    {
	      GT_Node * tgt = this->nodes->access(j);
	      assert(src != tgt);
	      this->insert_arc(src, tgt);
	    }	
      }

    if (connected)
      connect();

    return move(this->g);
  }

public:

/** 
  @brief Constructor

  @param[in] seed Seed random number generator
  @param[in] __init_node Node initialization class 
  @param[in] __init_arc Arc initialization class
*/  
  Random_Graph(unsigned long seed,
	       const Init_Node & __init_node,
	       const Init_Arc  & __init_arc)	       
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
  {
    if (this->g.is_digraph())
      throw std::domain_error("Building of random digraph through a graph");
  }

  Random_Graph(unsigned long seed = time(NULL),
	       const Init_Node && __init_node = Init_Node(),
	       const Init_Arc  && __init_arc  = Init_Arc())	       
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
  {
    if (this->g.is_digraph())
      throw std::domain_error("Building of random digraph through a graph");
  }


/** 
  @brief Create a scatter random graph.

  @details This version builds a random graph of about
  __num_arcs. The graph contains parallel arcs.

  The randomization procedure is to select at random two
  nodes and insert an arc between them if it does not
  there. The routine is linearly proportional to __num_arcs.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] __num_nodes Number of arches that must be the graph.
  @param[in] connected If the graph generated should be connected or
  no. By default it is.
  
  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
*/
  GT operator () (const size_t & __num_nodes, const size_t & __num_arcs, 
		  bool connected = true) 
  {
    return this->create(__num_nodes, __num_arcs, connected);
  }
  
public:

/** 
  @brief Creates a random graph.

  @details This version builds a random graph with probability p
  there arc between any two nodes.

  Wear this routine to generate dense graphs. In this case,
  To be used a value greater than or equal to 0.5 p.

  The procedure is quadratic. All combinations are generated
  of nodes and for each node pair (i, j)
  Lots with probability p whether there should be an arc between i and j.

  @warning El procedimiento puede ser extremadamente lento y goloso
  en memoria conforme aumento el valor de __num_nodes.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] p Existence probability arc between any two nodes.
  @param[in] connected If the graph generated should be connected or
  no. By default it is.
  
  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the value of p is between 0 and 1.
*/
  GT operator () (const size_t & __num_nodes, const double & p, 
		  bool connected = true) 
  {
    return create_p(__num_nodes, p, connected);
  }

private:

  virtual void make_eulerian()
  {
    while (this->odd_nodes.size() > 1)
      {
	GT_Node * src = NULL;
	GT_Node * tgt = NULL;

	while (true)
	  {
	    src = this->odd_nodes.select
	      (gsl_rng_uniform_int(this->r, this->odd_nodes.size()));
	    do
	      tgt = this->odd_nodes.select
		(gsl_rng_uniform_int(this->r, this->odd_nodes.size()));
	    while (tgt == src);

	    if (this->idx_arc->search(src, tgt) == NULL) 
	      break;
	    else if (this->odd_nodes.size() == 2)
	      { /* Randomize not have arc to node src or tgt */
		GT_Node * p = NULL;
		do
		  p = this->even_nodes.select
		    (gsl_rng_uniform_int(this->r, this->even_nodes.size()));
		while (this->idx_arc->search(src, p) != NULL or
		       this->idx_arc->search(tgt, p) != NULL);
		this->insert_arc(src, p);
		this->insert_arc(p, tgt);

		return;
	      }
	  }
	   
	this->insert_arc(src, tgt);
      }
    
    assert(this->odd_nodes.size() == 0);
  }

  void balance_graph_nodes_degree(GT_Node * src, GT_Node * tgt)
  {
    if (this->idx_arc->search(src, tgt) == NULL)
      this->insert_arc(src, tgt);

    const size_t & n = this->g.get_num_nodes();

    while (this->g.get_num_arcs(src) + this->g.get_num_arcs(tgt) < n)
      {
	GT_Node * p = this->nodes->access(gsl_rng_uniform_int(this->r, n));

	if (p == src or p == tgt)
	  continue;
	
	if (this->idx_arc->search(src, p) == NULL)
	  this->insert_arc(src, p);

	if (this->g.get_num_arcs(src) + this->g.get_num_arcs(tgt) == n)
	  break;

	if (this->idx_arc->search(tgt, p) == NULL)
	  this->insert_arc(tgt, p);
      }
  }

  virtual void make_hamiltonian ()
  {
    const size_t & n = this->g.get_num_nodes();

    for (int i = 0; i < n - 1; ++i)
      {
	GT_Node * src = this->nodes->access(i);

	for (int j = i + 1; j < n; ++j)
	  balance_graph_nodes_degree(src, this->nodes->access(j));
      }
  }

public:

/** 
  @brief Creates a random graph is Eulerian.

  @details This version builds a random graph that spread
  guarantee is Eulerian; ie containing rings
  Euler.

  The process first generates a graph scattered
  random, then examines the resulting graph and make
  arches so that the output contains an Eulerian cycle.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] __num_nodes Number of arches that must be the graph.

  @return A pointer to random graph created

  @throw bad_alloc If there is insufficient memory.

*/
  GT eulerian (const size_t & __num_nodes, const size_t & __num_arcs) 
  {
    this->save_parity = true;

    this->g = std::move(this->create(__num_nodes, __num_arcs, true));

    make_eulerian();

    return move(this->g);
  }

/** 
  @brief Create an Eulerian random graph.

  @details This version builds a random graph with Euler
  existence probability p arc between any two
  nodes.

  Wear this routine to generate dense graphs. In this case,
  To be used a value greater than or equal to 0.5 p.

  @warning The procedure can be extremely slow and sweet
  in memory as increase the value of __num_nodes.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] p Existence probability arc between any two nodes.

  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the value of p is between 0 and 1.
*/
  GT eulerian (const size_t & __num_nodes, const double & p) 
  {
    this->save_parity = true;

    this->g = std::move(this->create_p(__num_nodes, p, true));

    make_eulerian();
 
    return move(this->g);
  }

/** 
  @brief Create a Hamiltonian random graph.

  @details This version builds a random graph Hamiltonian.

  The process first generates a random graph, then examines the
  resulting graph and make arches so that the result contains a 
  Hamiltonian cycle.

  @warning The procedure can be extremely slow and sweet
  in memory with the increase in the value of __num_nodes.

  @warning The graph is secured to be Hamiltonian by theorems
  Ore and Dirac. In this regard, note that the condition
  Hamiltonian is of sufficiency, which means that it is not
  minimal graph. In fact, the graph generated tends to be dense.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] p Existence probability arc between any two nodes.
  By default this parameter value of 0.5, value
  intuitively and empirically that tends to generate lower 
  arches and less time consuming.
  
  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the value of p is between 0 and 1.

  @ingroup Grafos
*/
  GT sufficient_hamiltonian (const size_t & __num_nodes, 
			     const double & p = 0.5) 
  {
    this->g = std::move(this->create_p(__num_nodes, p, true));

    make_hamiltonian();

    return move(this->g);
  }
};


  template <class GT, 
	    class Init_Node = Dft_Init_Rand_Node<GT>,
	    class Init_Arc = Dft_Init_Rand_Arc<GT> >
class Random_Digraph : public Random_Graph_Base<GT, Init_Node, Init_Arc>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc GT_Arc;

  DynSetRandTree<GT_Node*> greater; /** Out nodes with degree greater than in */
  DynSetRandTree<GT_Node*> smaller; /** Out nodes with degree lower than in */
  DynSetRandTree<GT_Node*> equal;   /** Degree nodes equals out in */

  bool verify_tables()
  {
    const size_t & n = this->nodes->size();

    if (n != this->g.get_num_nodes())
      cout << "Warning num of nodes of graph does not match with array "
	   << this->g.get_num_nodes() << "!=" << n << endl;
    
    size_t total = greater.size() + smaller.size() + equal.size();
    if (total != this->g.get_num_nodes())
      cout << "Inconsistency with nodes parity" << endl
	   << "greater = " << greater.size() << endl
	   << "smaller = " << smaller.size() << endl
	   << "equal   = " << equal.size() << endl
	   << "total   = " << total << endl
	   << "|V|     = " << this->g.get_num_nodes();

    for (int i = 0; i < n; ++i)
      {
	GT_Node * p = this->nodes->access(i);
	
	const long & in_sz    = NODE_COUNTER(p);
	const size_t & out_sz = this->g.get_num_arcs(p);
	
	if (in_sz == out_sz)
	  {
	    if (smaller.search(p) != NULL)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in smaller table" << endl;
	    
	    if (greater.search(p) != NULL)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in greater table" << endl;
	    
	    if (equal.search(p) == NULL)
	      {
		cout << "node of same in/out degree is not in equal table" 
		     << endl;

		return false;
	      }
	  }
	else if (in_sz > out_sz)
	  {
	    if (greater.search(p) != NULL)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in greater table" << endl;
	    
	    if (equal.search(p) != NULL)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << endl;

	    if (smaller.search(p) == NULL)
	      {
		cout << "node with " << in_sz << "/" << out_sz << " not found "
		     << "smaller table" << endl;

		return false;
	      }	    	    
	  }
	else
	  {
	   if (smaller.search(p) != NULL)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << " in smaller table" << endl;
	    
	    if (equal.search(p) != NULL)
	      cout << "Inconsistency " << in_sz << "/" << out_sz << " found "
		   << endl;

	    if (greater.search(p) == NULL)
	      {
		cout << "node with " << in_sz << "/" << out_sz << " not found "
		     << "greater table" << endl;

		return false;
	      }	    	     
	  }
      }

    return true;
  }  
 
/** 
  @brief This call is made right after inserting a new arc src -> tgt.

  @details This implies that out (src) is updated, but in (tgt) no
*/
  virtual void update_parity_after_arc_insertion(GT_Node * src, GT_Node * tgt)
  {
    if (not this->save_parity)
      return;

    const size_t & src_out_degree = this->g.get_num_arcs(src);
    const long & src_in_degree    = NODE_COUNTER(src);

    if (src_out_degree == src_in_degree)
      { /* src is greater ==> out and put in equal */
	assert(this->smaller.search(src) != NULL);
	this->smaller.remove(src);
	this->equal.insert(src);
      }
    else if (src_out_degree > src_in_degree)
      if (src_out_degree == src_in_degree + 1)
	{
	  assert(this->equal.search(src) != NULL);
	  this->equal.remove(src);
	  this->greater.insert(src);
	}
      else
	assert(this->greater.search(src) != NULL);
    else /* src_out_degree < src_in_degree */
      assert(this->smaller.search(src) != NULL);
    
    const size_t & tgt_out_degree = this->g.get_num_arcs(tgt);
    const long tgt_in_degree      = ++NODE_COUNTER(tgt);

    if (tgt_out_degree == tgt_in_degree)
      {
	assert(this->greater.search(tgt));
	this->greater.remove(tgt);
	this->equal.insert(tgt);
      }
    else if (tgt_out_degree > tgt_in_degree)
      assert(this->greater.search(tgt));
    else /* (tgt_out_degree < tgt_in_degree) */
      {
	if (tgt_in_degree - 1 == tgt_out_degree)
	  { /* tgt is equal ==> it out */
	    assert(this->equal.search(tgt) != NULL);
	    this->smaller.insert(tgt);	  
	    this->equal.remove(tgt);
	  }
	else
	  assert(this->smaller.search(tgt) != NULL);
      } 
  }

  virtual void create_nodes_and_initialize_arc_index()
  {
    this->nodes = unique_ptr<DynArray<GT_Node*>>
      (new DynArray<GT_Node*>(this->num_nodes));

    this->nodes->reserve(this->num_nodes);

    for (int i = 0; i < this->num_nodes; ++i)
      {
	typename GT::Node * p = this->g.insert_node(new GT_Node);
	this->nodes->access(i) = p;
	this->init_node(this->g, p);

	if (this->save_parity)
	  {
	    NODE_COUNTER(p) = 0;
	    this->equal.insert(p);
	  }
      }

    this->idx_arc = unique_ptr<IndexArc<GT>> (new IndexArc<GT>(this->g) ); 
  }

  virtual void connect()
  {
    DynList<DynList<typename GT::Node*>> blk_list; /* Unconnected subgraphs */

    {     
    /* Save entry grades since the algorithm of Tarjan
    you're changing the */
      DynArray<int> in_degree; 
      in_degree.reserve(this->g.get_num_nodes());

      typename GT::Node_Iterator it(this->g);
      for (int i = 0; it.has_curr(); it.next(), ++i)
	in_degree.access(i) = NODE_COUNTER(it.get_curr());

      Tarjan_Connected_Components <GT> () (this->g, blk_list);

      it.reset_first(); /* Restore grades input */
      for (int i = 0; it.has_curr(); it.next(), ++i)
	NODE_COUNTER(it.get_curr()) = in_degree.access(i);
    }

    const size_t & num_blocks = blk_list.size();

    if (num_blocks == 1)
      return;

    /* Each node in this list is a block node i randomly selected*/ 
    DynArray<typename GT::Node*> b1; b1.reserve(num_blocks);
    DynArray<typename GT::Node*> b2; b2.reserve(num_blocks);
    {
      typename DynList<DynList<GT_Node*> >::Iterator it(blk_list);
      for (int i = 0; it.has_curr(); it.next(), ++i)
      { /* Select two nodes random current component */
	DynList<typename GT::Node*> & list = it.get_curr();
	b1.access(i) = this->select_random_node(list);
	b2.access(i) = this->select_random_node(list);
      }
    }

    for (int i = 0; i < num_blocks - 1; ++i)
      {
        GT_Node * src = b1.access(i);                    /* Node in block i */
        GT_Node * tgt = b1.access((i + 1) % num_blocks); /* Node in block i + 1 */
	
	if (this->idx_arc->search(src, tgt) == NULL)
	  this->insert_arc(src, tgt);
	
	src = b2.access(i);                    /* Node in block i */
	tgt = b2.access((i + 1) % num_blocks); /* Node in block i + 1 */

	if (this->idx_arc->search(tgt, src) == NULL)
	  this->insert_arc(tgt, src);
      }
  }

/** 
  @brief Creates a random graph in which the probability for
  arc between a pair of nodes is p
*/
  virtual GT create_p(const size_t & __num_nodes, const double & p, 
		      bool connected)
  {
    if (p > 1.0 or p <= 0.0)
      throw std::domain_error("Invalid value for p");

    this->initialize_and_create_nodes(__num_nodes, __num_nodes);

    for (int i = 0; i < this->num_nodes; ++i)
      {
	GT_Node * src = this->nodes->access(i);
	for (int j = 0; j < this->num_nodes; ++j)
	  if (i != j and gsl_rng_uniform(this->r) <= p)
	    {
	      GT_Node * tgt = this->nodes->access(j);
	      assert(this->idx_arc->search(src, tgt) == NULL);
	      this->insert_arc(src, tgt);
	    }
      }

    if (connected)
      connect();

    return move(this->g);
  }

public:

/** 
  @brief Constructor

  @param[in] seed Seed random number generator
  @param[in] __init_node Node initialization class
  @param[in] __init_arc Arc initialization class
*/
  Random_Digraph(unsigned long seed = time(NULL), 
		 const Init_Node && __init_node = Init_Node(),
		 const Init_Arc  && __init_arc  = Init_Arc())
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
  {
    if (not this->g.is_digraph())
      throw std::domain_error("Building of random graph through a digraph");
  }

  Random_Digraph(unsigned long seed, 
		 const Init_Node & __init_node,
		 const Init_Arc  & __init_arc)
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(seed, __init_node, __init_arc)
  {
    if (not this->g.is_digraph())
      throw std::domain_error("Building of random graph through a digraph");
  } 

  Random_Digraph(const Init_Node & __init_node,
		 const Init_Arc  & __init_arc)
    : Random_Graph_Base<GT, Init_Node, Init_Arc>(time(NULL), __init_node, __init_arc)
  {
    if (not this->g.is_digraph())
      throw std::domain_error("Building of random graph through a digraph");
  } 

/** 
  @brief Create a scatter random graph.

  @details This version builds a random graph of about
  __num_arcs. The graph contains parallel arcs.

  The randomization procedure is to select at random two
  nodes and insert an arc between them if it does not
  there. The routine is linearly proportional to __num_arcs.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] __num_nodes Number of arches that must be the graph.
  @param[in] connected If the graph generated should be connected or
  no. By default it is.
  
  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
*/
  GT operator () (const size_t & __num_nodes, const size_t & __num_arcs, 
		  bool connected = true) 
  {
    return this->create(__num_nodes, __num_arcs, connected);
  }

public:

/** 
  @brief Creates a random graph.

  @details This version builds a random graph with probability p
  there arc between any two nodes.

  Wear this routine to generate dense graphs. In this case,
  To be used a value greater than or equal to 0.5 p.

  The procedure is quadratic. All combinations are generated
  of nodes and for each node pair (i, j)
  Lots with probability p whether there should be an arc between i and j.

  @warning The procedure can be extremely slow and sweet
  in memory as increase the value of __num_nodes.

  @param[in] __num_nodes Number of nodes that must be the graph.
  @param[in] p Existence probability arc between any two nodes.
  @param[in] connected If the graph generated should be connected or
  no. By default it is.
  
  @return A pointer to random graph created
  
  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the value of p is between 0 and 1.
*/
  GT operator () (const size_t & __num_nodes, const double & p, 
		  bool connected = true) 
  {
    return this->create_p(__num_nodes, p, connected);
  }

private:

  virtual void make_eulerian()
  {
    GT_Node * src = NULL;
    GT_Node * tgt = NULL;

    while (this->greater.size() > 0 and this->smaller.size() > 0)
      {
	do
	  {
	    tgt = this->greater.select
	      (gsl_rng_uniform_int(this->r, this->greater.size()));
	    src = this->smaller.select
	      (gsl_rng_uniform_int(this->r, this->smaller.size()));
	  }
	while (src == tgt);
      
	if (this->idx_arc->search(src, tgt) == NULL)
	  this->insert_arc(src, tgt);
	else
	  {
	    GT_Node * mid = 
	      this->equal.select(gsl_rng_uniform_int(this->r, 
						     this->equal.size()));

	    while (this->idx_arc->search(src, mid) != NULL or 
		   this->idx_arc->search(mid, tgt) != NULL)
	      mid = this->equal.select
		(gsl_rng_uniform_int(this->r, this->equal.size()));
	    
	    this->insert_arc(src, mid);
	    this->insert_arc(mid, tgt);
	  }
      }
  }

  void balance_digraph_node(GT_Node * p)
  {
    const size_t & n = this->g.get_num_nodes();
    const size_t n2 = n/2;

    while (not (this->g.get_num_arcs(p) >= n2 and NODE_COUNTER(p) >= n2))
      {
	GT_Node * q = this->nodes->access(gsl_rng_uniform_int(this->r, n));

	if (q == p)
	  continue;

	if (this->idx_arc->search(p, q) == NULL)
	  {
	    this->insert_arc(p, q);
	    NODE_COUNTER(q)++;
	  }

	if (this->idx_arc->search(q, p) == NULL)
	  {
	    this->insert_arc(q, p);
	    NODE_COUNTER(p)++;
	  }
      }
  }

/** 
  @brief Balancing the two nodes to satisfy the condition of Hamiltonian
  if there arc src -> tgt
*/
  void balance_digraph_nodes_degree(GT_Node * src, GT_Node * tgt)
  {
    if (this->idx_arc->search(src, tgt) != NULL)
      {
	balance_digraph_node(src);
	balance_digraph_node(tgt);

	return;
      }

    const size_t & n = this->g.get_num_nodes();

    while (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) < n)
      {
	GT_Node * p = this->nodes->access(gsl_rng_uniform_int(this->r, n));

	if (p == src or p == tgt)
	  continue;
	
	if (this->idx_arc->search(src, p) == NULL)
	  {
	    this->insert_arc(src, p);
	    NODE_COUNTER(p)++;
	  
	    if (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) == n)
	      break;
	  }

	if (this->idx_arc->search(p, tgt) == NULL)
	  {
	    this->insert_arc(p, tgt);
	    NODE_COUNTER(tgt)++;
	  }
      }

    assert(this->g.get_num_arcs(src) + NODE_COUNTER(tgt) >= n);
  }

  virtual void make_hamiltonian ()
  {
    this->g.reset_counter_nodes();

    /** Counts the input level for each node */
    for (typename GT::Arc_Iterator it(this->g); it.has_curr(); it.next())
      NODE_COUNTER(it.get_tgt_node())++;

    const size_t & n = this->g.get_num_nodes();

    for (int i = 0; i < n; ++i)
      {
	GT_Node * src = this->nodes->access(i);

	for (int j = 0; j < n; ++j)
	  {
	    if (i == j)
	      continue;

	    GT_Node * tgt = this->nodes->access(j);
	    
	    balance_digraph_nodes_degree(src, tgt);
	  }
      }
  }
};

}

# endif /* RANDOM_GRAPH_H
