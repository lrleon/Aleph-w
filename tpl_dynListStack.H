
# ifndef TPL_DYNLISTSTACK_H
# define TPL_DYNLISTSTACK_H

# include <ahDry.H>
# include <ahIterator.H>
# include <htlist.H>
# include <ah-dry.H>

namespace Aleph {

  /** Dynamic stack of element of generic type `T`

      `DynListStack<T>` models a dynamic stack bases on single linked list.
      tipo genérico T.

      @see ArrayStack FixedStack
      @ingroup Secuencias
   */
    template <typename T> 
using DynListStack = DynList<T>;
   

} // end namespace Aleph
# endif /* TPL_DYNLISTSTACK_H */

