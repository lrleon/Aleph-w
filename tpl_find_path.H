

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

  template <class GT,
	    template <typename, class> class Itor = Out_Iterator,
	    class SA = Dft_Show_Arc<GT>>
class Directed_Find_Path
{
  const GT & g;
  SA & sa;

  template <template <typename T> class Q, class Op>
  Path<GT> find(typename GT::Node * start, Op & op)
  {
    g.reset_nodes();
    g.reset_arcs();

    start->set_state(Processed);

    Q<typename GT::Arc*> q;
    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next())
      {
	auto a = it.get_curr();
	it.get_node(a)->set_state(Processing);
	a->set_state(Processing);
	q.put(a);
      }

    typename GT::Node * end = nullptr, * curr = nullptr;
    while (not q.is_empty())
      {
	auto arc = q.get();
	assert(arc->state() == Processing);
	arc->set_state(Processed);

	curr = g.get_tgt_node(arc);
	if (curr->state() == Processed)
	  continue;

	curr->set_state(Processed);
	NODE_COOKIE(curr) = g.get_src_node(arc);

	if (op(curr))
	  {
	    end = curr;
	    break;
	  }

	for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next()) 
	  {
	    auto a  = it.get_curr();
	    a->set_state(Processing);

	    auto tgt = it.get_node(a);
	    if (tgt->state() == Processed)
	      continue;

	    if (tgt->state() != Processed)
	    {
	      q.put(a);
	      tgt->set_state(Processing);
	    }
	  else
	    a->set_state(Processed);
	}
      } // end while

    Path<GT> ret(g);
    if (not end)
      return ret;

    assert(curr == end);

    while (curr != start)
      {
	ret.insert(curr);
	curr = (typename GT::Node*) NODE_COOKIE(curr);
      }
    ret.insert(start);

    return ret;
  }

public:

  Directed_Find_Path(const GT & __g, SA & __sa) : g(__g), sa(__sa) {}

  Directed_Find_Path(const GT & __g, SA && __sa = SA()) : g(__g), sa(__sa) {}

  template <class Op> 
  Path<GT> dfs(typename GT::Node * start, Op & op) 
  {
    return find<DynListStack, Op>(start, op);
  }

  template <class Op> 
  Path<GT> dfs(typename GT::Node * start, Op && op) 
  {
    return find<DynListStack, Op>(start, op);
  }

  template <class Op> 
  Path<GT> bfs(typename GT::Node * start, Op & op) 
  {
    return find<DynListQueue, Op>(start, op);
  }

  template <class Op> 
  Path<GT> bfs(typename GT::Node * start, Op && op) 
  {
    return find<DynListQueue, Op>(start, op);
  }

  Path<GT> dfs(typename GT::Node * start, typename GT::Node * end) 
  {
    return dfs(start, [end] (auto p) { return p == end; });
  }

  Path<GT> bfs(typename GT::Node * start, typename GT::Node * end) 
  {
    return bfs(start, [end] (auto p) { return p == end; });
  }
};




} // namespace Aleph

# endif // TPL_FIND_PATH_H
