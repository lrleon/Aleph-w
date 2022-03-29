

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
)
{

}


// 1er campo arcos vacíos, 2do arcos llenos, 3ro arcos parciales
  template <class Net>
using Feasible_Tree = tuple<DynList<typename Net::Arc*>, 
			    DynList<typename Net::Arc*>, 
			    DynList<typename Net::Arc*>>;
								

template <class Net> static inline
Feasible_Tree<Net> build_feasible_spanning_tree(const Net & net)
{
  using Arc = typename Net::Arc;
  DynList<Arc*> empty, full, partial;
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
    {
      auto a = it.get_curr_ne();
      if (a->flow == 0) // arco vacío?
	empty.append(a);
      else if (a->flow == a->cap) // arco lleno?
	full.append(a);
      else
	partial.append(a);
    }
  return make_tuple(std::move(empty), std::move(full), std::move(partial));
}


template <class Net> static inline 
DynList<typename Net::Arc*> get_partial_arcs(const Net & net)
{
  DynList<typename Net::Arc*> ret;
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
    {
      auto a = it.get_curr_ne();
      if (a->flow > 0 and a->flow < a->cap)
	ret.append(a);
    }
  return ret;
}


# define POTENTIAL(p) (NODE_COUNTER(p))


template <class Net>
typename Net::Flow_Type rcost(typename Net::Node * src, 
			      typename Net::Node * tgt)
{
  for (Out_Iterator<Net> it(src); it.has_curr(); it.next_ne())
    {
      auto a = it.get_curr_ne();
      if (a->tgt_node == tgt)
	return a->cost - (POTENTIAL(src) - POTENTIAL(tgt));
    }
  AH_ERROR("Arc not found");
}


template <class Net>
typename Net::Flow_Type rcost(typename Net::Arc * a)
{
  using Node = typename Net::Node*;
  return a->cost - 
  (POTENTIAL((Node*)a->src_node) - POTENTIAL((Node*)a->tgt_node));
}




# undef POTENTIAL

} // end namespace Aleph

# endif // TPL_MAXFLOW_MINCOST_H
