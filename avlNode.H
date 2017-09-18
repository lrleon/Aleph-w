
# ifndef AVLNODE_H
# define AVLNODE_H

# include <tpl_binNodeUtils.H>

class AvlNode_Data
{

private:

  signed char diff; // diferencia de altura

public:

  AvlNode_Data() noexcept : diff(0) { /* EMPTY */ }

  signed char & getDiff() noexcept { return diff; }

  void reset() noexcept { diff = 0; }

};

/// \class AvlNode
DECLARE_BINNODE(AvlNode, 40, AvlNode_Data);


# define DIFF(p) ((p)->getDiff())

    template <class Node>
bool is_avl(Node* p)
{
  if (p == Node::NullPtr) 
    return true;

  if (DIFF(p) < -1 or DIFF(p) > 1)
    return false;
      
  const int hL = computeHeightRec(LLINK(p));
  const int hR = computeHeightRec(RLINK(p));

  int diff = hR - hL;

  if (diff > 1 or diff < -1)
    return false;

  if (((int) DIFF(p)) != hR - hL)
    return false;

  if (not is_avl(LLINK(p)))
    return false;

  return is_avl(RLINK(p));
}

# endif // AVLNODE_H

