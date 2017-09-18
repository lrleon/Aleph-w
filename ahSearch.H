# ifndef AH_SEARCH_H
# define AH_SEARCH_H

# include <ahFunctional.H>
# include <tpl_sort_utils.H>

namespace Aleph
{

  template <typename T, 
	    template <typename> class Container,
	    class Cmp = Aleph::less<T>>
bool contains(const Container<T> & c, const T & item)
{
  return exists(c, /* Lambda */ [&item] (const T & i)
		{
		  return are_equals<T, Cmp>(item, i);
		});
}

}

# endif // AH_SEARCH_H
