# ifndef FLOYD_WARSHALL_H
# define FLOYD_WARSHALL_H

# include <tpl_list_graph.H>
# include <tpl_dynDlist.H>
# include <tpl_dynListStack.H>

template <class GT>
void arc_to_distance(typename GT::Arc *                arc, 
		     typename GT::Arc::Distance_Type & dist)
{
  
}

    template <class GT>
Aleph::Pair<Ady_Mat<typename GT::Arc::Distance_Type> *,
	    Ady_Mat<typename GT::Path> *>
floyd_warshall_all_min_paths(GT & g)
{
  typename Ady_Mat<typename GT::Arc::Distance_Type> w;
}


# endif
