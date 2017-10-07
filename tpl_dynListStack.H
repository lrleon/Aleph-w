
# ifndef TPL_DYNLISTSTACK_H
# define TPL_DYNLISTSTACK_H

# include <ahDry.H>
# include <ahIterator.H>
# include <htlist.H>
# include <ah-dry.H>

namespace Aleph {

  /** Dynamic stack of generic elements of type `T`

      `DynListStack<T>` models a dynamic stack based on single linked list.

      @see ArrayStack FixedStack
      @ingroup Secuencias
   */
    template <typename T> 
using DynListStack = DynList<T>;
   

} // end namespace Aleph
# endif /* TPL_DYNLISTSTACK_H */

