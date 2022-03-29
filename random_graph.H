

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

  GT operator () (const size_t & __num_nodes, const double & p, 
		  bool connected = true) 
  {
    return this->create_p(__num_nodes, p, connected);
  }

private:

  virtual void make_eulerian()
  {
    GT_Node * src = nullptr;
    GT_Node * tgt = nullptr;

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
      
	if (this->idx_arc->search_directed(src, tgt) == nullptr)
	  this->insert_arc(src, tgt);
	else
	  {
	    auto mid = 
	      this->equal.select(gsl_rng_uniform_int(this->r, 
						     this->equal.size()));

	    while (this->idx_arc->search_directed(src, mid) != nullptr or 
		   this->idx_arc->search_directed(mid, tgt) != nullptr)
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
	auto q = this->nodes->access(gsl_rng_uniform_int(this->r, n));
	if (q == p)
	  continue;

	if (this->idx_arc->search_directed(p, q) == nullptr)
	  {
	    this->insert_arc(p, q);
	    NODE_COUNTER(q)++;
	  }

	if (this->idx_arc->search_directed(q, p) == nullptr)
	  {
	    this->insert_arc(q, p);
	    NODE_COUNTER(p)++;
	  }
      }
  }

  // balancea los dos nodos para que satisfagan la condición de
  // hamiltoniano. Si existe arco src-->tgt
  void balance_digraph_nodes_degree(GT_Node * src, GT_Node * tgt)
  {
    if (this->idx_arc->search_directed(src, tgt) != nullptr)
      {
	balance_digraph_node(src);
	balance_digraph_node(tgt);

	return;
      }

    const size_t & n = this->g.get_num_nodes();

    while (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) < n)
      {
	auto p = this->nodes->access(gsl_rng_uniform_int(this->r, n));
	if (p == src or p == tgt)
	  continue;
	
	if (this->idx_arc->search_directed(src, p) == nullptr)
	  {
	    this->insert_arc(src, p);
	    NODE_COUNTER(p)++;
	  
	    if (this->g.get_num_arcs(src) + NODE_COUNTER(tgt) == n)
	      break;
	  }

	if (this->idx_arc->search_directed(p, tgt) == nullptr)
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

        // contabiliza el grado de entrada por cada nodo
    for (typename GT::Arc_Iterator it(this->g); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    const size_t & n = this->g.get_num_nodes();

    for (int i = 0; i < n; ++i)
      {
	auto src = this->nodes->access(i);
	for (int j = 0; j < n; ++j)
	  {
	    if (i == j)
	      continue;

	    auto tgt = this->nodes->access(j);	    
	    balance_digraph_nodes_degree(src, tgt);
	  }
      }
  }
};

}

# endif // RANDOM_GRAPH_H
