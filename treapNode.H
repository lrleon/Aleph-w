
# ifndef TREAPNODE_H
# define TREAPNODE_H

# include <limits.h> 
# include <tpl_binNode.H>

using namespace Aleph;

namespace Aleph {

const long Max_Priority = ULONG_MAX; 
const long Min_Priority = 0; 

class TreapNode_Data
{
  unsigned long priority;  

public:

  TreapNode_Data() noexcept : priority (Max_Priority) {}

  TreapNode_Data(SentinelCtor) noexcept : priority (Max_Priority) {}

  unsigned long & getPriority() noexcept { return priority; }

  void reset() noexcept { /* empty */ }
};

DECLARE_BINNODE_SENTINEL(TreapNode, 80, TreapNode_Data);

/** Return the priority of node

    @ingroup Arboles
*/
template <class Node> 
unsigned long & PRIO(Node * p) noexcept { return p->getPriority(); }

/// Return `true` if root is a consistent treap
    template <class Node>
bool is_treap(Node * root) noexcept
{
  if (root == Node::NullPtr) 
    return true;  

  if (PRIO(root) > PRIO(LLINK(root)) or PRIO(root) > PRIO(RLINK(root)))
    return false; // violación de relación ancestral

  return is_treap(LLINK(root)) and is_treap(RLINK(root));
}

} // end namespace Aleph

# endif // TREAPNODE_H

