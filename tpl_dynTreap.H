
# ifndef TPL_DYNTREAP_H
# define TPL_DYNTREAP_H

# include <tpl_dynMapTree.H>
# include <tpl_treap.H>

namespace Aleph {

/**
   Dynamic mapping implemented with treap trees.

 */
    template <typename Key, typename Type, class Compare = Aleph::less<Key> >
struct DynTreapTree : public DynMapTree<Key, Type, Treap, Compare>
{
  
};

} // end namespace Aleph

# endif /* TPL_DYNTREAP_H */



