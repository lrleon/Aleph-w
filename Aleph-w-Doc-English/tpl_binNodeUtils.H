
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file tpl_binNodeUtils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BINNODEUTILS_H
# define TPL_BINNODEUTILS_H 

# include <ahFunction.H>
# include <tpl_arrayStack.H> 
# include <tpl_arrayQueue.H>
# include <tpl_dynListQueue.H>
# include <bitArray.H>
# include <tpl_dynDlist.H>
# include <tpl_binNode.H>

using namespace Aleph;
namespace Aleph {

    template <class Node> inline static
void __inorder_rec(Node * node, const int& level, int & position, 
                   void (*visitFct)(Node *, int, int))
{
  if (node == Node::NullPtr) 
    return;

  __inorder_rec((Node*) LLINK(node), level + 1, position, visitFct);
 
 (*visitFct)(node, level, position);
  ++position;

  __inorder_rec((Node*) RLINK(node), level + 1, position, visitFct); 
}

/** 
  @brief Recursively browse in infix a binary tree.

  @details inOrderRec(root,visit) takes a tour infix on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:
  
  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.

  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.
  
  @see preOrderRec() postOrderRec()
  
  @ingroup Arboles
*/
    template <class Node> inline
int inOrderRec(Node * root, void (*visitFct)(Node*, int, int))
{
  int position = 0;
  __inorder_rec(root, 0, position, visitFct); 
  return position;
}

    template <class Node> inline static
void __preorder_rec (Node * p, const int & level, int & position,
                     void (*visitFct)(Node*, int, int))
{
  if (p == Node::NullPtr) 
    return;

  (*visitFct)(p, level, position);
  ++position;

  __preorder_rec((Node*) LLINK(p), level + 1, position, visitFct);
  __preorder_rec((Node*) RLINK(p), level + 1, position, visitFct);
} 

/** 
  @brief Browse recursively in prefix a binary tree.

  @details preOrderRec(root,visit) takes a tour prefix on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.
  
  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.
  
  @see inOrderRec() postOrderRec()

  @ingroup Arboles
*/
    template <class Node> inline 
int preOrderRec(Node * root, void (*visitFct)(Node*, int, int))
{
  int position = 0;
  __preorder_rec(root, 0, position, visitFct); 
  return position;
}

    template <class Node> inline static
void __postorder_rec(Node * node, const int & level, int & position, 
                     void (*visitFct)(Node*, int, int))
{
  if (node == Node::NullPtr) 
    return;

  __postorder_rec((Node*) LLINK(node), level + 1, position, visitFct);
  __postorder_rec((Node*) RLINK(node), level + 1, position, visitFct); 

  (*visitFct)(node, level, position);
  ++position;
}

/** 
  @brief Browse recursively in suffix a binary tree.

  @details postOrderRec(root,visit) takes a infix tour on
  binary tree with "root" root. If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.
  
  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.
  
  @see inOrderRec() preOrderRec()
  
  @ingroup Arboles
*/
    template <class Node> inline
int postOrderRec(Node * root, void (*visitFct)(Node*, int, int))
{
  int position = 0;
  __postorder_rec(root, 0, position, visitFct); 
  return position;
}

/** 
  @brief Generic infix route on a binary tree.
          
  @details For_Each_In_Order traverses a binary tree in inorder.
  The class handles the following type parameters:
  -# Node: Binary node type by of binary tree is handled.
  -# Op: Type of operation to be executed on each node. The
     shaped class <code>Op(p)</code> where <code>p</code>
     the current node is visited.
     Traditionally, <code>Op(p)</code> has the following
     structure:
  
     @code
     struct Op
     {
       // State attributes that you wish to keep

         // Optional builder if necessary to initialize
       Op(...) // Passing information builder initial state
       {
         // Initialization
       }

       void operator () (Node * p)
       {
         // Operation on node p
       }
     };
     @endcode

  @ingroup arboles
*/
    template <class Node, class Op> 
class For_Each_In_Order
{
  static void for_each_inorder(Node * root, Op & op) 
  {
    if (root == Node::NullPtr)
      return;

    for_each_inorder((Node*) LLINK(root), op);
    op(root);
    for_each_inorder((Node*) RLINK(root), op);
  }

public:

/** @brief Operation on each node. */
  void operator () (Node * root, Op & op) 
  {
    for_each_inorder(root, op);
  }

/** @brief Operation on each node. */
  void operator () (Node * root, Op && op = Op()) 
  {
    for_each_inorder(root, op);
  }
};

/** 
  @brief Infix o inorder traversal condioned to operation result.

  @details If operation return true, then the traversal continues. 
  Otherwise, the traverse stops.  
*/
  template <class Node, class Operation> inline
bool traverse(Node * root, Operation & operation)
{
  if (root == Node::NullPtr)
    return true;

  return traverse<Node, Operation>((Node*) LLINK(root), operation) and
    operation(root) and 
    traverse<Node, Operation>((Node*) RLINK(root), operation);
}

  template <class Node, class Operation> inline
bool traverse(Node * root, Operation && operation = Operation())
{
  return traverse<Node, Operation>(root, operation);
}

/** 
  @brief Infix generic route on a binary tree with the possibility of output.
          
  @details Goto_In_Order traverses a binary tree in inorder.
  The class handles the following type parameters:
  -# Node: Binary node type by of binary tree is handled.
  -# Op: Type of operation to be executed on each node. The
     shaped class <code>Op(p)</code> where <code>p</code>
     the current node is visited.
     Traditionally, <code>Op(p)</code> has the following
     structure:

     @code
     struct Op
     {
       // State attributes that you wish to keep

         // Optional builder if necessary to initialize
       Op(...) // Passing information builder initial state
       {
         // Initialization
       }

       bool operator () (Node * p)
       {
         // Operation on node p

         return // If you desired or not to continue the travel
       }
     };
     @endcode

     To stop the tour before reaching the last node,
     usually when it has completed a calculation,
     operation should return <code>Op(p)false</code>.

  @ingroup arboles
*/
    template <class Node, class Op> 
class Goto_In_Order
{
  bool to_leave;

  void inorder(Node * root, Op & op) 
  {
    if (root == Node::NullPtr or to_leave)
      return;

    for_each_inorder((Node*) LLINK(root), op);
    to_leave = op(root);
    for_each_inorder((Node*) RLINK(root), op);
  }

public:

  Goto_In_Order() : to_leave(false) { /* Empty */ }

  void leave() { to_leave = true; }

/** @brief Operation on each node. */
  void operator () (Node * root, Op & op) 
  {
    inorder(root, op);
  }

/** @brief Operation on each node. */
  void operator () (Node * root, Op && op = Op()) const 
  {
    inorder(root, op);
  }
};

/** 
  @brief Generic prefix route on a binary tree.
          
  @details For_Each_Preorder traverses a binary tree in order prefix.
  The class handles the following type parameters:
  -# Node: Binary node type by of binary tree is handled.
  -# Op: Type of operation to be executed on each node. The
     shaped class <code>Op(p)</code> where <code>p</code>
     the current node is visited.
     Traditionally, <code>Op(p)</code> has the following
     structure:

     @code
     struct Op
     {
       // State attributes that you wish to keep

         // Optional builder if necessary to initialize
       Op(...) // Passing information builder initial state
       {
         // Initialization
       }

       void operator () (Node * p)
       {
         // Operation on node p
       }
     };
     @endcode

  @ingroup arboles
*/
    template <class Node, class Op> 
class For_Each_Preorder
{
  static void preorder(Node * root, Op & op) 
  {
    if (root == Node::NullPtr)
      return;

    op(root);
    preorder((Node*) LLINK(root), op);
    preorder((Node*) RLINK(root), op);
  }

public:

/** @brief Operation on each node. */
  void operator () (Node * root, Op & op) 
  {
    preorder(root, op);
  }

/** @brief Operation on each node.  */
  void operator () (Node * root, Op && op = Op()) 
  {
    preorder(root, op);
  }
};


/** 
  @brief Generic suffix route on a binary tree.
          
  @details For_Each_Postorder traverses a binary tree in order prefix.
  The class handles the following type parameters:
  -# Node: Binary node type by of binary tree is handled.
  -# Op: Type of operation to be executed on each node. The
     shaped class <code>Op(p)</code> where <code>p</code>
     the current node is visited.
     Traditionally, <code>Op(p)</code> has the following
     structure:

     @code
     struct Op
     {
       // State attributes that you wish to keep

         // Optional builder if necessary to initialize
       Op(...) // Passing information builder initial state
       {
         // Initialization
       }

       void operator () (Node * p)
       {
         // operación sobre nodo p
       }
     };
     @endcode

  @ingroup arboles
*/
    template <class Node, class Op> 
class For_Each_Postorder
{
  static void postorder(Node * root, Op & op) 
  {
    if (root == Node::NullPtr)
      return;

    postorder((Node*) LLINK(root), op);
    postorder((Node*) RLINK(root), op);
    op(root);
  }

public:
/** @brief Operation on each node */
  void operator () (Node * root, Op & op) 
  {
    postorder(root, op);
  }

/** @brief Operation on each node */
  void operator () (Node * root, Op && op = Op())
  {
    postorder(root, op);
  }
};

/** 
  @brief Walking on prefix a binary tree.

  @details simple_preOrderRec(root,visit) takes a tour prefix on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.

  Instead of appealing to the recursion, this version of the prefix 
  route uses a queue internal, vector, of maximum capacity 
  Node::MaxHeight.

  @param[in] node The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.
  
  @see preOrderStack() 

  @ingroup Arboles
*/
    template <class Node> inline
size_t simple_preOrderStack(Node * node, void (*visitFct)(Node *, int, int))
{
  if (node == Node::NullPtr) 
    return 0;

  ArrayStack<Node *> stack(Node::MaxHeight);
  stack.push(node); 

  Node * p;
  size_t count = 0;
  while (not stack.is_empty())
    {
      p = stack.pop();

      (*visitFct) (p, stack.size(), count++); 

      if (RLINK(p) != Node::NullPtr)
        stack.push(RLINK(p));

      if (LLINK(p) != Node::NullPtr)
        stack.push(LLINK(p));
    }
  return count;
} 

/** 
  @brief Walking on prefix a binary tree.

  @details preOrderRec(root,visit) takes a tour prefix on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.

  Instead of appealing to the recursion, this version of the prefix 
  route uses a queue internal, vector, of maximum capacity 
  Node::MaxHeight.

  @param[in] node The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.
  
  @see simple_preOrderStack() 

  @ingroup Arboles
*/
    template <class Node> inline
size_t preOrderStack(Node * node, void (*visitFct)(Node *, int, int))
{
  if (node == Node::NullPtr) 
    return 0;

  ArrayStack<Node *> stack(Node::MaxHeight);
  Node *p = node;
  size_t count = 0;

  while (true)
    {
      (*visitFct)(p, stack.size(), count++);

      if (LLINK(p) != Node::NullPtr)
        {
          stack.push(p); /* p and RLINK(p) have yet to visit */
          p = LLINK(p);  /* Move to the left */
          continue; /* Go to visit the left branch root */
        }
      while (true) 
        {
          if (RLINK(p) != Node::NullPtr)
            {
              p = RLINK(p); /* Move to the right */
              break;        /* Go to visit the right branch root */
            }
          if (stack.is_empty()) 
            return count; /* End */

          p = stack.pop(); /* Get to go right branch */
        }
    }
}

/** 
  @brief Infix walking in a binary tree.

  @details inOrderStack(root,visit) takes a infix tour
  on the binary tree with "root" root. If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.

  Instead of appealing to the recursion, this version of the prefix 
  route uses a queue internal, vector, of maximum capacity 
  Node::MaxHeight.

  @param[in] node The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.

  @return The number of nodes visited.

  @ingroup Arboles
*/
    template <class Node> inline
size_t inOrderStack(Node * node, void (*visitFct)(Node *, int, int))
{
  if (node == Node::NullPtr) 
    return 0;

  ArrayStack<Node *> stack(Node::MaxHeight);
  Node *p = node;
  size_t count = 0;

  while (true)
    {
      if (LLINK(p) != Node::NullPtr)
        {
          stack.push(p); /* p and RLINK(p) have yet to visit */
          p = LLINK(p);  /* Move to the left */
          continue; /* Continue down by the left branch */
        }
      while (true)
        {
          (*visitFct)(p, stack.size(), count++);

          if (RLINK(p) != Node::NullPtr)
            {
              p = RLINK(p); /* Move to the right */
              break;        /* Go to visit the right branch root */
            }
          if (stack.is_empty()) 
            return count;

          p = stack.pop(); /* Get to go right branch */
        }
    }
}

/** 
  @brief Walking in suffix a binary tree without recursion

  @details inOrderStack(root,visit) takes a infix tour
  on the binary tree with "root" root. If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.

  Instead of appealing to the recursion, this version of the prefix 
  route uses a queue internal, vector, of maximum capacity 
  Node::MaxHeight.

  @param[in] node The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.

  @ingroup Arboles
*/
    template <class Node> inline
size_t postOrderStack(Node * root, void (*visitFct)(Node *, int, int))
{    
  if (root == Node::NullPtr)
    return 0;

  typedef std::pair<Node*, char> Postorder_Pair;
  ArrayStack<Postorder_Pair> stack(Node::MaxHeight);

  Postorder_Pair __pair;
  Node *& p   = __pair.first  = root;
  char & side = __pair.second = 'i';
  stack.push(__pair);
  size_t count = 0;

  while (not stack.is_empty())
    {
      __pair = stack.pop();
      switch (side)
        {
        case 'i': 
          if (LLINK(p) != Node::NullPtr)
            stack.push(Postorder_Pair(LLINK(p), 'i'));

          stack.push(Postorder_Pair(p, 'l'));
          break;

        case 'l':
          if (RLINK(p) != Node::NullPtr)
            stack.push(Postorder_Pair(RLINK(p), 'i'));

          stack.push(Postorder_Pair(p, 'r'));
          break;

        case 'r':
          (*visitFct) (p, stack.size(), count++); 
          break;
        }
    }

  return count;
}

template <class Node>
static void prefix(Node * root, DynList<Node*> & acc)
{
  if (root == Node::NullPtr)
    return;

  acc.append(root);
  prefix((Node*) LLINK(root), acc);
  prefix((Node*) RLINK(root), acc);
}

template <class Node>
static void infix(Node * root, DynList<Node*> & acc)
{
  if (root == Node::NullPtr)
    return;

  infix((Node*) LLINK(root), acc);
  acc.append(root);
  infix((Node*) RLINK(root), acc);
}

template <class Node>
static void suffix(Node * root, DynList<Node*> & acc)
{
  if (root == Node::NullPtr)
    return;

  sufffix((Node*) LLINK(root), acc);
  siffix((Node*) RLINK(root), acc);
  acc.append(root);
}

/** 
  @brief Returns a list with the path prefix of a binary tree.

  @param[in] root The root of tree

  @return A list of pointers to the corresponding node to the prefix path

  @ingroup Arboles
*/
template <class Node>
DynList<Node*> prefix(Node * root)
{
  DynList<Node*> ret_val;
  prefix(root, ret_val);
  return ret_val;
}

/** 
  @brief Returns a list with the infix path of a binary tree.

  @param[in] root The root of tree
    
  @return A list of pointers to the corresponding node to the infix path
    
  @ingroup Arboles
*/
template <class Node>
DynList<Node*> infix(Node * root)
{
  DynList<Node*> ret_val;
  infix(root, ret_val);
  return ret_val;
}

/** 
  @brief Returns a list with the suffix path of a binary tree.

  @param[in] root The root of tree

  @return A list of pointers to the corresponding node to the suffix path

  @ingroup Arboles
*/
template <class Node>
DynList<Node*> suffix(Node * root)
{
  DynList<Node*> ret_val;
  suffix(root, ret_val);
  return ret_val;
}


/** 
  @brief Count the number of nodes in a binary tree.

  @details compute_cardinality_rec(node) all the tree with root node and
  counts the number of nodes.

  @param[in] node Tree root to calculate the cardinality.

  @return The number of nodes having the tree.

  @ingroup Arboles
*/
    template <class Node> inline 
size_t compute_cardinality_rec(Node * node)
{
  if (node == Node::NullPtr) 
    return 0;

  return (compute_cardinality_rec(LLINK(node)) + 1 + 
          compute_cardinality_rec(RLINK(node)));
}

/** 
  @brief Calculate the height of a binary tree.

  @details computeHeightRec(node) calculates the height of the entire tree
  root node.

  @param[in] node Tree root to calculate altitude.
  
  @return The tree height.
  
  @ingroup Arboles
*/
template <class Node> inline size_t computeHeightRec(Node * node)
{
  if (node == Node::NullPtr) 
    return 0;

  const size_t left_height  = computeHeightRec(LLINK(node));
  const size_t right_height = computeHeightRec(RLINK(node));

  return 1 + std::max(left_height, right_height);
}

/** 
  @brief Destruction of a binary tree.

  @details destroyRec(root) destroys (releases all memory) binary
  tree whose root is root.

  @param[in] root The root of tree to destroy.
  
  @ingroup Arboles
*/
    template <class Node> inline 
void destroyRec(Node *& root)
{
  if (root == Node::NullPtr) 
    return;

  destroyRec((Node*&) LLINK(root));
  destroyRec((Node*&) RLINK(root));
  delete root;

  root = (Node*) Node::NullPtr;
}

/** 
  @brief Copia recursiva de un árbol binario.

  @details copyRec(src_root) return a full copy of binary tree with the 
  root node.

  @param[in] src_root The root of tree to copy.
  
  @return A tree copy with root src_root.
  
  @throw bad_alloc If there is insufficient memory.
  
  @ingroup Arboles
*/
    template <class Node> inline 
Node * copyRec(Node * src_root) 
  throw(std::exception, std::bad_alloc) 
{
  if (src_root == Node::NullPtr) 
    return (Node*) Node::NullPtr;

  Node * tgt_root = new Node(*src_root); 

  try 
    {
      LLINK(tgt_root) = copyRec<Node>((Node*) LLINK(src_root));
      RLINK(tgt_root) = copyRec<Node>((Node*) RLINK(src_root));

    }
  catch (...)
    {
      assert(RLINK(tgt_root) == Node::NullPtr);

      if (LLINK(tgt_root) != Node::NullPtr) 
        destroyRec((Node*&) LLINK(tgt_root)); /* TODO: diff de Node*& */

      delete tgt_root;

      throw;
    }

  return tgt_root;
}      

/** 
  @brief Returns true if the trees are similar.

  @details Two binary trees t1 and t2 are similar if they have
  exactly the same way.

  @param[in] t1 One of the trees to compare similarity.
  @param[in] t2 The other tree to compare similarity.

  @return true sf t1 and t2 are similar; false otherwise.

  @see areEquivalents()
*/
    template <class Node> inline 
bool areSimilar(Node * t1, Node * t2)
{
  if (t1 == Node::NullPtr and t2 == Node::NullPtr) 
    return true;

  if (t1 == Node::NullPtr or t2 == Node::NullPtr) 
    return false;

  return (areSimilar(LLINK(t1), LLINK(t2)) and 
          areSimilar(RLINK(t1), RLINK(t2)));
}

/** 
  @brief Returns true if the trees are equivalent.

  @details Two binary trees t1 and t2 are equivalent if they
  are similar to each other and their nodes have exactly the
  same content as the standard of equality Equal()().

  The routine uses two parameters:
  -# Node: Binary node type. It must be derived from BinNode.
  -# Equal: Class that implements the relationship "equal that".

  A specialized version assumes equality criteria
  T::operator=().

  @param[in] t1 One of the trees to compare similarity.
  @param[in] t2 The other tree to compare similarity.

  @return true if t1 and t2 are equivalent; false otherwise.

  @see areEquivalents()
*/
    template <class Node, class Equal> inline
bool areEquivalents(Node * t1, Node * t2)
{
  if (t1 == Node::NullPtr and t2 == Node::NullPtr) 
    return true;

  if (t1 == Node::NullPtr or t2 == Node::NullPtr) 
    return false;

  if (not Equal () (KEY(t1), KEY(t2))) 
    return false;

  return (areEquivalents<Node, Equal>(LLINK(t1), LLINK(t2)) and 
          areEquivalents<Node, Equal>(RLINK(t1), RLINK(t2)));
}

    template <class Node> inline
bool areEquivalents(Node * t1, Node * t2)
{
  return areEquivalents<Node, Aleph::equal_to<typename Node::key_type> >(t1, t2);
}

/** 
  @brief Walking by levels a binary tree.

  @details levelOrder(root,visit) takes a infix tour through levels
  on the binary tree with "root" root. If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, bool is_left)

  Where:
  -# p: Pointer to the node currently visited.
  -# pos: Ordinal of visit.
  -# is_left: true if the node is a left child, false
  otherwise.

  This algorithm uses a queue, vector, of maximum capacity
  queue_size.

  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  @param[in] queue_size Size of the internal queue.

  @return The number of nodes visited.

  @note The internal queue is a contiguous memory array than
  is separated at the beginning of the routine and released at the end. 
  This makes this routine is rather limited in memory. Use care.

  @ingroup Arboles
*/
    template <class Node> inline
void levelOrder(Node * root, 
                void (*visitFct)(Node*, int, bool), size_t queue_size)
{
  if (root == Node::NullPtr)
    return;

  const size_t two_power = 
     (size_t) (std::log((float)queue_size)/std::log(2.0) + 1);
  ArrayQueue<std::pair<Node*, bool> > queue(two_power); 
  queue.put(std::pair<Node*, bool>(root, bool()));

  for (int pos = 0; not queue.is_empty(); pos++)
    {
      std::pair<Node*, bool> pr = queue.get();
      Node *& p = pr.first;

      (*visitFct) (p, pos, pr.second);

      if (LLINK(p) != Node::NullPtr)
        queue.put(std::pair <Node*, bool> (LLINK(p), true));

      if (RLINK(p) != Node::NullPtr)
        queue.put(std::pair <Node*, bool> (RLINK(p), false));
    }
}


/** 
  @brief Walking by levels a binary tree.

  @details levelOrder(root, op) takes a tour through levels
  on the binary tree with "root" root.

  The op function has the following specification:

  bool op(Node* p)

  Where p is pointer to the node currently visited. If the function
  returns true, the tour continues. Otherwise
  tour stops

  @param[in] root The root of the tree to visit.
  @param[in] op Operation to be performed on each node
  
  @return true if all nodes are traversed; false otherwise
  
  @ingroup Arboles
*/
  template <class Node, class Operation> inline
bool level_traverse(Node * root, Operation & operation)
{
  if (root == Node::NullPtr)
    return true;

  DynListQueue<Node*> queue; 
  queue.put(root);

  while (not queue.is_empty())
    {
      Node * p = queue.get();

      if (not operation(p))
	return false;

      if (LLINK(p) != Node::NullPtr)
        queue.put((Node*) LLINK(p));

      if (RLINK(p) != Node::NullPtr)
        queue.put((Node*) RLINK(p));
    }
  return true;
}

  template <class Node, class Operation> inline
bool level_traverse(Node * root, Operation && operation = Operation())
{
  return level_traverse<Node, Operation>(root, operation);
}

/** 
  @brief Rebuild a binary tree from his travels prefix and infix.

  @details build_tree() takes two dynamic contentivos arrays of 
  tours prefix and infix of a tree and rebuild the tree causing 
  the routes in question.

  @param[in] preorder Contentivo travel array prefix.
  @param[in] l_p Prefix route begins on the preorder array.
  @param[in] r_p End of the prefix route in the preorder array.
  @param[in] inorder Array contentivo of infix tour.
  @param[in] l_i Start of infix tour in the inorder array.
  @param[in] r_i End of infix tour in the inorder array.

  @return The root of a binary tree, single, causing the
  routes given in parameters.

  @throw bad_alloc If there is insufficient memory.

  @ingroup Arboles
*/
    template <template <class> class Node, typename Key> inline
Node<Key> * build_tree(DynArray<Key> & preorder, 
                       const int & l_p, const int & r_p,
                       DynArray<Key> & inorder, 
                       const int & l_i, const int & r_i)
{
    /** Are you empty the tour?
    If the answer is YES, then return empty tree */
  if (l_p > r_p) 
    { 
      assert(l_i > r_i);
      return Node <Key> ::NullPtr;
    }

  assert(r_p - l_p == r_i - l_i); 

  Node<Key> * root = new Node <Key> (preorder[l_p]); /* Crear la raíz */
  if (r_p == l_p) 
    return root; /* Recorrido de longitud 1 */
  
  assert(l_i <= r_i);

  int i = 0;
  for (int j = l_i; j <= r_i; ++j)
    if (inorder[j] == preorder[l_p])
      {
        i = j - l_i;
        break;
      }

  assert(i <= r_i);

  LLINK(root) = build_tree <Node, Key> (preorder, l_p + 1, l_p + i, 
                                        inorder, l_i, l_i + (i - 1));
  RLINK(root) = build_tree <Node, Key> (preorder, l_p + i + 1, r_p,
                                        inorder, l_i + i + 1, r_i);
  return root;
}

    template <class Node> inline static void 
__compute_nodes_in_level(Node * root, const int & level, 
                         const int & current_level, 
                         DynDlist<Node*> & level_list)
{
  if (root == Node::NullPtr) 
    return;
  if (current_level == level)
    {
      level_list.append(root);
      return; /* Not worth descend */
    }
  __compute_nodes_in_level(LLINK(root), level, current_level + 1, level_list);
  __compute_nodes_in_level(RLINK(root), level, current_level + 1, level_list);
}

/** 
  @brief Calcule the nodes amount than have a level.

  @details compute_nodes_in_level(root,level,list) save to the list
  dynamic "list" the level nodes "level" in the tree whose root is
  root.

  @param[in] root The root of the binary tree.
  @param[in] level Nivel que se desea contabilizar.
  @param[out] list Dynamic list of nodes located in level
  level.
  
  @throw bad_alloc If there is insufficient memory.
  
  @ingroup Arboles
*/
    template <class Node> inline
void compute_nodes_in_level(Node * root, const int & level, 
                            DynDlist<Node*>& list)
{
  __compute_nodes_in_level(root, level, 0, list);
}

/** 
  @brief Infix walking in a binary tree without recursion or use explicit stack.

  @details inOrderThreaded(root,visit) takes a infix tour
  on the binary tree with "root" root. If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.

  This version of infix tour not consume stack space.

  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.
  
  @see preOrderThreaded() 
  
  @note The routine is not reentrant.
  
  @ingroup Arboles
*/
    template <class Node> inline
void inOrderThreaded(Node * root, void (*visitFct)(Node*))
{
  if (root == Node::NullPtr) 
    return;

  Node *p = root, *r = Node::NullPtr, *q;
  while (p != Node::NullPtr)
    {
      q = LLINK(p); 
      if (q == Node::NullPtr) 
        { /* No branch left ==> p visit */
          (*visitFct)(p);  
          r = p;          
          p = RLINK(p);   
          continue;       
        }

    /* Moving towards the right node of the left branch */
      while (q != r and RLINK(q) != Node::NullPtr)
        q = RLINK(q);
    /** P is a predecessor?
    If the answer is YES, then stop a thread and then up to visit p */
      if (q != r) 
        {  
          RLINK(q) = p; /* Here is placed the thread */
          p = LLINK(p); /* Continue down by the left */
          continue;     
        }

      (*visitFct)(p);

      RLINK (q) = Node::NullPtr; /* Delete thread */
      r = p;            
      p = RLINK(p); /* Move to the right branch */
    }
}

/** 
  @brief Walking in a binary prefix tree without using recursion or
  explicit stack.

  @details prerderThreaded(root,visit) takes a tour prefix on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level de p within the entire tree.
  -# pos: Ordinal of visit.

  This version of the prefix tour does not consume stack space.

  @param[in] node The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.
  
  @return The number of nodes visited.
  
  @see inOrderThreaded() 
  
  @note The routine is not reentrant.
  
  @ingroup Arboles
*/
    template <class Node> inline
void preOrderThreaded(Node * node, void (*visitFct)(Node*))
{
  if (node == Node::NullPtr) 
    return;

  Node * p = node, * r = Node::NullPtr, *q;
  while (p != Node::NullPtr)
  {
    q = LLINK(p); 

    if (q == Node::NullPtr) 
      { 
        (*visitFct)(p);  
        r = p;
        p = RLINK(p);
        continue;    
      }

    /* Moving towards the right node of the left branch */
    while (q != r and RLINK(q) != Node::NullPtr)
      q = RLINK(q);

    if (q != r) 
      { 
        RLINK(q) = p;
        (*visitFct)(p);
        p = LLINK(p);
        continue;    
      }

    RLINK(q) = Node::NullPtr; /* Delete thread */
    r = p;                  
    p = RLINK(p); /* Advance to right branch */
  }
} 

    template <class Node> inline  static
size_t __internal_path_length(Node * p, const size_t & level)
{
  if (p == Node::NullPtr) 
    return 0;

  return level + __internal_path_length(LLINK(p), level + 1) +
         __internal_path_length(RLINK(p), level + 1);
}

/** 
  @brief Computes the internal path length of a binary tree.

  @param[in] p Root of the binary tree to calculate the ipl.
  
  @return The internal path length of the tree with root p.

  @ingroup Arboles
*/
    template <class Node> inline 
size_t internal_path_length(Node * p) 
{
  return __internal_path_length(p, 0);
}

/** 
  @brief Keeps the structure of a binary tree in an array of bits.

  @details tree_to_bits(root, array) takes a binary tree of root "root", 
  generates the bit prefix code (word Lukasiewicz) and stored in the 
  "array" bit array.

  The prefix string is concatenated to the array of bits. This
  thus, it is possible to store multiple trees in a single array.

  @param[in] root The root of tree a guardar.
  @param[out] array Settlement of bits where the code is saved
  prefix binary tree.
  
  @throw std::bad_Alloc If there is insufficient memory for the
  dynamic array of bits.

  @see bits_to_tree() BitArray save_tree_keys_in_prefix() load_tree_keys_in_prefix()

  @ingroup Arboles
*/
    template <class Node> inline 
void tree_to_bits(Node * root, BitArray & array)
{
  if (root == Node::NullPtr)
    {
      array.push(1);
      return;
    }

  array.push(0);
  tree_to_bits((Node*) LLINK(root), array);
  tree_to_bits((Node*) RLINK(root), array);
}

/** 
  @brief Returns an bits array of corresponding in the code tree.

  @details tree_to_bits(root) takes a binary tree of root "root", 
  generates the bit prefix code (word Lukasiewicz) and return into a 
  bit array.

  @param[in] root The root of binary tree to save.

  @return array Settlement of bits where the code is saved
  prefix binary tree.

  @throw std::bad_Alloc If there is insufficient memory for the
  dynamic array of bits.

  @see bits_to_tree() BitArray save_tree_keys_in_prefix() load_tree_keys_in_prefix()

  @ingroup Arboles
*/
template <class Node> inline 
BitArray tree_to_bits(Node * root)
{
  BitArray ret_val;
  tree_to_bits(root, ret_val);
  return ret_val;
}

     template <class Node> static inline 
Node * __bits_to_tree(BitArray & array, int & i)
{
  int bit = array.read_bit(i++);
  if (bit == 1) /* ¿Es un nodo externo? */
    return Node::NullPtr;

  Node * p = new Node;  /* Crear nodo interno actual */
  LLINK(p) = (Node*) __bits_to_tree<Node>(array, i); /* Subárbol izq. */
  RLINK(p) = (Node*) __bits_to_tree<Node>(array, i); /* Subárbol der. */

  return p;
}

/** 
  @brief Build a binary tree from a code prefix stored in an array of bits.

  @details bits_to_tree(array, idx) takes an array of bits "array", in which
  IDX index begins a prefix of a binary tree code, and built the original binary tree.

  @param[in] array Contentivo array of bits of the code prefix
  binary tree to be built.
  @param[in] idx Bit index in array where the code begins prefix.

  @return A pointer to the root of the binary tree corresponding to the
  word prefix stored in the array of bits.

  @throw std::bad_Alloc If there is insufficient memory for the
  build the tree. In that case, the tree is in a state incoherent.

  @see tree_to_bits() BitArray save_tree_keys_in_prefix() load_tree_keys_in_prefix()
  
  @ingroup Arboles
*/
template <class Node> inline Node * bits_to_tree(BitArray & array, int idx = 0)
{
  return __bits_to_tree <Node> (array, idx);
}

/** 
  @brief Save in a file the keys in order prefix of a binary tree.

  @details save_tree_keys_in_prefix(root, output) runs recursively in
  prefix root tree "root". Covered by each key is 
  invokes the Get_Key()(root, output) class whose function is
  extract from node the key to save and return it in
  string. The key stream is stored in the output stream.

  @param[in] root The root of tree binary sequence whose prefix
  you want to be kept on file.
  @param[out] output Stream file with the sequence prefix stored.
  
  @ingroup Arboles
  
  @see load_tree_keys_in_prefix()
*/
  template <class Node, class Get_Key> inline 
void save_tree_keys_in_prefix(Node * root, ofstream & output)
{
  if (root == Node::NullPtr)
    return;

  output << Get_Key () (root) << " ";

  save_tree_keys_in_prefix<Node,Get_Key>((Node*)LLINK(root), output);
  save_tree_keys_in_prefix<Node,Get_Key>((Node*)RLINK(root), output);
}

/** 
  @brief Loaded from a file to a binary tree a sequence of key in prefix.

  @details load_tree_keys_in_prefix(root, input) runs recursively in
  binary prefix the tree root "root". For each route node,
  it invokes the Load_Key()(root, input) class; where input is
  stream containing the sequence of keys in prefix. The role
  of Load_Key is read from stream the key, previously saved
  by save_tree_keys_in_prefix(), and store its value in the node
  current root.

  At the end the call the tree containing the keys read from
  input.

  @param[in] root The root of binary tree on which to load the key.
  @param[in] input File where the keys are in prefixed sequence.
  
  @ingroup Arboles
*/
  template <class Node, class Load_Key> inline 
void load_tree_keys_in_prefix(Node * root, ifstream & input)
{
  if (root == Node::NullPtr)
    return;

  Load_Key () (root, input);

  load_tree_keys_in_prefix<Node,Load_Key>((Node*)LLINK(root), input);
  load_tree_keys_in_prefix<Node,Load_Key>((Node*)RLINK(root), input);
}

/** 
  @brief Save a binary tree in file.

  @details save_tree(root, output) stores the root binary tree "root"
  in the file indicated by output. The tree can be recovered
  by load_tree().

  The Get_Key()(root) class is responsible for extracting the key to a
  node and return it to type string; this return value is
  that is saved in the file.

  The storage method is to store the code prefix tree in bits, 
  called word of Lukasiewicz, Then the prefix key sequence obtained 
  is saved for calls to Get_Key()(root) on each node prefix. This 
  technique is considered one of the most compact and it can be 
  reduced by compression

  @param[in] root The root of binary tree to store.
  @param[out] output Reference to file handle; should be properly opened.
  
  @throw std::bad_alloc If there is not enough memory to
  construct array intermediate bits.
  
  @see load_tree()
  
  @ingroup Arboles
*/
  template <class Node, class Get_Key> inline 
void save_tree(Node * root, ofstream & output)
{
  BitArray prefix;
  tree_to_bits(root, prefix); 
  prefix.save(output);
  save_tree_keys_in_prefix <Node, Get_Key> (root, output);
}

/** 
  @brief Loading and builds a binary tree from a file.

  @details load_tree(input) reads the input file referenced, which
  contains a binary tree previously saved save_tree()
  and restores memory.

  @param[in] input Reference to the file is located
  saved the binary tree.
  
  @return Binary tree root file read and built in memory. 
  
  @throw std::bad_alloc If there is not enough to build
  array of bits intermediate or to build the binary tree.
  
  @see save_tree()
  
  @ingroup Arboles
*/
  template <class Node, class Load_Key> inline 
Node * load_tree(ifstream & input)
{
  BitArray prefix;
  prefix.load(input);
  Node * root = bits_to_tree <Node> (prefix);
  load_tree_keys_in_prefix <Node, Load_Key> (root, input); 
  return root;
}

   
   template <class Node, class Get_Key> inline 
void put_tree_keys_in_array(Node * root, ofstream & out)
{
  if (root == Node::NullPtr)
    return;

  const string str = Get_Key () (root);

  if (str == "\t")
    out << "\"\t\"";
  else if (str == "\n")
    out << "\"\\n\""; 
  else
    out << "\"" << str << "\"";
  out << ", ";

  put_tree_keys_in_array <Node, Get_Key> ((Node *) LLINK(root), out);
  put_tree_keys_in_array <Node, Get_Key> ((Node *) RLINK(root), out);
}


    template <class Node, class Load_Key> inline 
void load_tree_keys_from_array(Node * root, const char * keys[], int & idx)
{
  if (root == Node::NullPtr)
    return;

  if (Load_Key()(root, keys[idx]))
    ++idx;

  load_tree_keys_from_array <Node, Load_Key> ((Node *) LLINK(root), keys, idx);
  load_tree_keys_from_array <Node, Load_Key> ((Node *) RLINK(root), keys, idx);
}

/** 
  @brief Generates statements and bit arrays of keys for a binary tree.

  @details save_tree_in_array_of_chars(root, array_name, output) generates
  statements for two arrays with which you can define a
  binary tree. The statements are written to a file
  text, already opened, addressed by the output stream. The
  resulting statement has the general form:

  const unsigned char array_name_cdp[n] = { unsigned char list };

  const char * array_name_k[] = { list of keys in prefix };

  The first array is bits and stores the tree topology
  under a code prefix bits (a word of Lukasiewicz). The
  second array saves the contents of nodes and is
  ordained in the prefix sequence. The key values are
  given by Get_Key()(root, str) class which should return
  a constant char* representing the contents of the node.

  The Get_Key()() class, observing the contents of the node, you must
  be very carefully planned. Its overall structure is the
  following:

  string operator() (Node * p) const;

  Where p is the prefix of the current node in the route
  binary tree. The result of this call is placed as
  array_name constant value of settlement.

  The value of this routine is to incorporate binary trees and
  defined in other programs. The resulting statement
  it included in the program of interest and charged with the routine
  counterpart load_tree_from_array().

  @param[in] root The root of the binary tree.
  @param[in] array_name Name of the array of content tree nodes
  @param[out] output Stream open where write the
  arrays statements and their values.

  @see load_tree_from_array() BitArray

  @ingroup Arboles
*/
   template <class Node, class Get_Key> inline 
void save_tree_in_array_of_chars(Node *         root, 
                                 const string & array_name, 
                                 ofstream &     output)
{
  BitArray prefix;
  tree_to_bits(root, prefix); 
  prefix.save_in_array_of_chars(array_name + "_cdp", output);
  output << "const char * " << array_name << "_k[] = { " << endl;
  put_tree_keys_in_array <Node, Get_Key> (root, output);
  output << "NULL };" << endl;
}


/** 
  @brief Read a binary tree two constants generated arrays.

  @details load_tree_from_array(bits, num_bits, keys) takes an array bits,
  of num_bits dimension, the values of type unsigned char
  containing code prefix bits of a binary tree. The
  array of bits is read and the binary tree is built. Then
  the tree is traversed in prefix for each node travel, the
  Load_Key()(p, str) operator it is invoked on the current node and
  an input key array keys. The Load_Key() function
  key is to take the keys[i] and eventually, according as it has been
  saved the tree, load the key in the node. If the key is
  loaded on the node, then Load_Key() should return true for
  tell the function to be read the following key
  keys[i+1] in the array. Conversely, if the return value
  is false, then the prefix travel advances to the next node
  with the same value keys[i].

  Load_Key() it must be carefully specified under following 
  interface:

  bool Load_Key::operator () (Node * p, const char * str) const

  p is the current node in the prefix tree traversal
  recovered by the code bits. str is the current value
  keys[i] is being read. If the operation returns true,
  then it is assumed that the key was loaded and advanced to the
  next; otherwise, the key is kept available str
  to the next node in the route prefix.

  @param[in] bits Unsigned char array where they are stored
  the bits. This array must have been generated by
  BitArray::save_in_array_of_chars() (after obtaining the
  bit binary prefix tree) code. 
  @param[in] num_bits Number of bits in the array.
  @param[in] keys The keys array containing the binary tree
  in order prefix.

  @return The root of the binary tree read.

  @throw std::bad_alloc If there is not enough memory to
  construct the tree.

  The meaning of this routine is incorporated into a program, a
  binary tree that somehow or other was generated by another
  program by save_tree_in_array_of_chars().

  @see save_tree_in_array_of_chars() BitArray
 
  @ingroup Arboles
*/
    template <class Node, class Load_Key> inline 
Node * load_tree_from_array(const unsigned char bits[], 
                            const size_t &      num_bits, 
                            const char *        keys[])
{
  BitArray prefix;
  prefix.load_from_array_of_chars(bits, num_bits);
  Node * root = bits_to_tree <Node> (prefix);
  int i = 0;
  load_tree_keys_from_array <Node, Load_Key> (root, keys, i);
  return root;
}


/** 
  @brief Check consistency of a binary tree search.

  @details check_binary_search_tree(p) checks whether the binary tree
  with root node satisfies the order property of a tree
  binary search for the criterion of comparison Compare()().

  @param[in] p Root of the binary tree to verify.
  
  @return true if the binary tree satisfies the order condition;
  false otherwise. 

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
bool check_binary_search_tree(Node * p)
{
  typedef typename Node::key_type Key_Type;

  if (p == Node::NullPtr) 
    return true;

  if (LLINK(p) != Node::NullPtr)
    {
      if (less_or_equal_than<Key_Type, Compare> (KEY(LLINK(p)), KEY(p)))
        return check_binary_search_tree<Node, Compare>(LLINK(p));
      else 
        return false;
    }

  if (RLINK(p) != Node::NullPtr)
    {
      if (less_or_equal_than<Key_Type, Compare> (KEY(p), KEY(RLINK(p))))
        return check_binary_search_tree<Node, Compare>(RLINK(p));
      else
        return false;
    }
  return true;
}

/** 
  @brief Rebuild a binary tree search from its route prefix.

  @param[in] preorder Dynamic array contentivo of travel 
  prefix of the keys of a binary tree search.
  @param[in] l Index beginning of the sequence prefix
  in the preorder dynamic array.
  @param[in] r Index term of the sequence prefix
  in the preorder dynamic array.
  
  @return The root of the binary tree search that produces
  the prefix given sequence.
  
  @throw bad_alloc If there is insufficient memory.
  
  @ingroup Arboles
*/
    template <class Node> inline Node * 
preorder_to_binary_search_tree (DynArray<typename Node::key_type> & preorder, 
				const int & l, const int & r)
{
  if (l > r) 
    return Node::NullPtr;

  Node * root = new Node(preorder[l]);

  if (l == r) 
    return root;

  int first_greater = l + 1;
  while ((first_greater <= r) and (preorder[first_greater] < preorder[l]))
    ++first_greater; 

  LLINK(root) = 
    preorder_to_binary_search_tree<Node>(preorder, l + 1, first_greater - 1);
  RLINK(root) = 
    preorder_to_binary_search_tree<Node>(preorder, first_greater, r);

  return root;
}

/** 
  @brief Find a key in a binary tree search.

  @details searchInBinTree(root,key) searches a binary tree search
  contentivo key node "key".

  The binary tree search is ordered with criteria comparison 
  Compare()().

  @param[in] root The root of the binary tree.
  @param[in] key The key to look for.
  @param[in] cmp Comparison operation.

  @return A pointer to node contentivo of key this is in binary tree search; 
  NULL otherwise.

  @ingroup Arboles
*/
    template <class Node, 
	      class Compare = Aleph::less<typename Node::key_type>> 
  inline 
Node * searchInBinTree(Node * root, 
		       const typename Node::key_type & key,
		       Compare & cmp)
{
  while (root != Node::NullPtr)
    if (cmp(key, KEY(root)))       /* ¿En rama izquierda? */
      root = (Node*) LLINK(root);  /* Baje a la izquierda */
    else if(cmp (KEY(root), key))  /* ¿En rama derecha? */
      root = (Node*) RLINK(root);  /* Baje a la derecha */
    else
      break;                       /* Se encontró! */

  return root;
}

  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline 
Node * searchInBinTree(Node * root,
		       const typename Node::key_type & key,
		       Compare && cmp = Compare())
{  
  return searchInBinTree <Node, Compare> (root, key, cmp);
}

/** 
  @brief Find the minimum element in a binary tree search.

  @details find_min(root) returns the minimum element of the binary tree
  search whose root is "root".

  @param[in] root The root the binary tree search.

  @return A pointer to the node containing the minimum element.

  @note No check is done empty tree.

  @ingroup Arboles

  @see find_max()
*/
    template <class Node> inline 
Node * find_min(Node * root)
{
  while (LLINK(root) != Node::NullPtr)
    root = static_cast<Node*>(LLINK(root));

  return root;
}

/** 
  @brief Find the maximum element in a binary tree search.

  @details find_max(root) returns the maximum element of the binary tree
  search whose root is "root".

  @param[in] root The root the binary tree search.
  
  @return A pointer to the node containing the maximum element.
  
  @note No check is done empty tree.
  
  @ingroup Arboles
  
  @see find_min()
*/
    template <class Node> inline 
Node * find_max(Node * root)
{
  while (RLINK(root) != Node::NullPtr)
    root = static_cast<Node*>(RLINK(root));

  return root;
}
/** 
  @brief p is the infix successor node.

  @details find_successor(p,pp) searches the successor node within the
  infix sequence of node p.

  @param[in] p Node pointer to which you want to know his successor infix.
  @param[out] pp Pointer to parent of successor.

  @return A pointer to the successor node infix p.

  @see find_predecessor()

  @ingroup Arboles
*/
    template <class Node> inline 
Node * find_successor(Node * p, Node *& pp)
{ 
  assert(p != Node::NullPtr);
  assert(RLINK(p) != Node::NullPtr);

  pp = p;
  p  = (Node*) RLINK(p);
  while (LLINK(p) != Node::NullPtr)
    {
      pp = p;
      p  = (Node*) LLINK(p);
    }

  return p;
}

/** 
  @brief p is the infix predecessor node.

  @details find_predecessor(p,pp) looking predecessor node within the
  infix sequence of node p.

  @param[in] p Node pointer to which you want to know your infix predecessor.
  @param[out] pp Pointer to parent of predecessor.

  @return A pointer to the predecessor node infix p.

  @see find_successor()

  @ingroup Arboles
*/
    template <class Node> inline
Node* find_predecessor(Node * p, Node *& pp)
{
  assert(p != Node::NullPtr);
  assert(LLINK(pp) != Node::NullPtr);

  pp = p;
  p  = static_cast<Node*>(LLINK(p));

  while (RLINK(p) != Node::NullPtr)
    {
      pp = p;
      p  = static_cast<Node*>(RLINK(p));
    }

  return p;
}

/** 
  @brief Keyword Search binary tree search with determination 
  of the parent.

  @details search_parent(root,key,parent) searches for the key in the 
  key binary tree search whose root is "root" and determines,
  contentivo addition the key node, the parent node.

  The routine used as comparison criteria Compare()().

  @param[in] root The root the binary search tree where
  search.
  @param[in] root The root the binary tree search where search.
  @param[in] key The search key.
  @param[out] parent Pointer to the parent node containing the
  key (if it was found).
  @param[in] cmp Comparison operation.
  
  @return A pointer to the key document containing key node if it
  is in the binary tree search; NULL than otherwise.
  
  @see searchInBinTree()
  
  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
Node * search_parent(Node * root, const typename Node::key_type & key, 
                     Node *& parent, Compare && cmp = Compare())
{
  assert((LLINK(parent) == root) or (RLINK(parent) == root));
  assert(root != Node::NullPtr);

  while (true)
    if (cmp(key, KEY(root)))
      { 
        if (LLINK(root) == Node::NullPtr) 
          return root;

        parent = root;
        root = static_cast<Node*>(LLINK(root));
      }
    else if (cmp(KEY(root), key))
      {
        if (RLINK(root) == Node::NullPtr) 
          return root;

        parent = root;
        root = static_cast<Node*>(RLINK(root));
      }
    else
      return root;
}

/** 
  @brief Search failed node and determination of the parent.

  @details search_rank_parent(root,key) search the binary tree search
  its root is the key "key". If the key is in the 
  binary tree, then returns the document containing key node;
  otherwise, the node returns to the key would parent
  if it it were inserted into the binary tree search.

  @param[in] root The root the binary tree search.
  @param[in] key The key to look for.
  @param[in] cmp Comparison operation.

  @return A pointer to contentivo node of the key if it is
  in the binary tree; otherwise, it returns a pointer
  the node would parent of key if this key was inserted in
  binary tree search.

  @note Do not check if the tree is empty.

  @ingroup Arboles
*/
    template <class Node, 
	      class Compare = Aleph::less<typename Node::key_type> > 
    inline Node * 
search_rank_parent(Node * root, 
		   const typename Node::key_type & key,
		   Compare && cmp = Compare())
{
  assert(root != Node::NullPtr);

  while (true)
    if (cmp(key, KEY(root)))
      { 
        if (LLINK(root) == Node::NullPtr) 
          return root; 
        
        root = static_cast<Node*>(LLINK(root));
      }
    else if (cmp(KEY(root), key))
      {
        if (RLINK(root) == Node::NullPtr) 
          return root;

        root = static_cast<Node*>(RLINK(root));
      }
    else
      return root;
}

/** 
  @brief Inserts a node in a binary tree search by substitution
  an external node.

  @details insert_in_binary_search_tree(root, p) inserts the p node in the
  binary tree search whose root is "root" at the discretion of
  comparison Compare()().

  @param[in,out] root The root the binary tree search.
  @param[in] p Node to insert.
  
  @return The direction of the inserted node (p) if the key
  of p is not contained within the tree; Node::NullPtr than
  otherwise.

  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_in_binary_search_tree(Node *& root, Node * p)
{
  if (root == Node::NullPtr) 
    return root = p;

  if (Compare () (KEY(p), KEY(root)))        /* p < root? */
    return insert_in_binary_search_tree<Node,Compare>((Node*&) LLINK(root), p);
  else if (Compare () (KEY(root), KEY(p)))   /* ¿p > root? */
    return insert_in_binary_search_tree<Node,Compare>((Node*&) RLINK(root), p);

  return Node::NullPtr; /* Clave repetida ==> no hay inserción */
}

# define INSERT_DUP insert_dup_in_binary_search_tree<Node,Compare>

/** 
  @brief Inserts a node in a binary tree search by substitution
  an external node.

  @details insert_dup_in_binary_search_tree(root, p) inserts the 
  p node in the binary tree search whose root is root at the 
  discretion of comparison Compare()().

  Repeated keys are allowed.

  @param[in,out] root The root the binary tree search.
  @param[in] p Node to insert.
  
  @return The direction of the inserted node (p) if the key
  of p is not contained within the tree; Node::NullPtr than
  otherwise.

  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_dup_in_binary_search_tree(Node *& root, Node * p)
{
  if (root == Node::NullPtr) 
    return root = p;

  if (Compare () (KEY(p), KEY(root)))        /* p < root? */
    return INSERT_DUP ((Node*&) LLINK(root), p);
  
  return INSERT_DUP((Node*&) RLINK(root), p);
}

# undef INSERT_DUP

/** 
  @brief Find a key in a binary tree search and inserted into
  if you do not find it.

  @details search_or_insert_in_binary_search_tree(root, p) searches for the key
  <code>KEY(p)</code> binary tree search whose root is "root"
  according to the comparison criteria <code>Compare()()</code>. If the
  key is found, then it returns a pointer to the node
  the houses; otherwise, the <code>p</code> node is inserted
  in the tree and it returns <code>p</code>.

  @param[in,out] r The root of binary tree search.
  @param[in] p Node to find or insert.
  
  @return The address of the node that contains a <code>KEY(p)</code>
  if your key is already in the tree or <code>p</code> if the key does not
  it is, in which case inserts.

  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
Node * search_or_insert_in_binary_search_tree(Node *& r, Node * p)
{
  if (r == Node::NullPtr) 
    return r = p;

  if (Compare () (KEY(p), KEY(r)))        /* p < root? */
    return 
      search_or_insert_in_binary_search_tree<Node,Compare>((Node*&)LLINK(r), p);
  else if (Compare () (KEY(r), KEY(p)))   /* ¿p > root? */
    return 
      search_or_insert_in_binary_search_tree<Node,Compare>((Node*&)RLINK(r), p);

  return r; 
}

# define SPLIT split_key_rec<Node, Compare>

/** 
  @brief Recursively partitioned a binary tree search as a key.

  @details split_key_rec(root,key,ts,tg) partitions, according to the key 
  "key" a binary tree search in two trees tg and ts. After the
  operation tree, tree root becomes empty, ts contains all lower keys than 
  key and tg the majors.

  @param[in,out] root A pointer to the root of the binary tree to partition. 
  @param[in] key Partition key.
  @param[out] ts Tree with smaller keys that key.
  @param[out] tg Tree with the biggest keys that key.
  @return true if key is not within the binary tree; in which
  if the partition was performed successfully. Otherwise, if
  key is inside the tree, the tree is not partitioned and the
  result is false.

  @see split_key()

  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
bool split_key_rec(Node * root, const typename Node::key_type & key, 
                   Node *& ts, Node *& tg)
{
  if (root == Node::NullPtr)
    {
    /* key is not in tree ==> split will succeed */
      ts = tg = Node::NullPtr;
      return true;
    }

  if ( Compare() (key, KEY(root)) ) /* key < KEY(root)? */
    if (SPLIT((Node*&) LLINK(root), key, ts, (Node*&) LLINK(root)))
      {
        tg = root;
        return true;
      }
    else
      return false;

  if (Compare() (KEY(root), key)) /* key < KEY(root)? */
    if (SPLIT((Node*&) RLINK(root), key, (Node*&) RLINK(root), tg))
      {
        ts = root;
        return true;
      }
    else
      return false;

  return false; /* Key exists in tree ==> is left intact */
}

# undef SPLIT

# define SPLIT split_key_dup_rec<Node, Compare>

/** 
  @brief Recursively partitioned a binary tree search as a key.

  @details split_key_dup_rec(root,key,ts,tg) partitions, according to the key 
  "key" a binary tree search in two trees tg and ts. After the
  operation tree, tree root becomes empty, ts contains all lower keys than 
  key and tg the majors.

  With this approach, the infix traversal of tree will be ordered
  by keys and the duplication by insertion order.

  @param[in,out] root A pointer to the root of the binary tree to partition. 
  @param[in] key Partition key.
  @param[out] ts Tree with smaller keys that key.
  @param[out] tg Tree with the biggest keys that key.

  @see split_key()

  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
void split_key_dup_rec(Node * root, const typename Node::key_type & key, 
                       Node *& ts, Node *& tg)
{
  if (root == Node::NullPtr)
    {    
    /* key is not in tree ==> split will succeed */
      ts = tg = Node::NullPtr;
      return;
    }

  if (Compare () (KEY(root), key)) /* key < KEY(root)? */
    SPLIT((Node*&) RLINK(root), key, (Node*&) RLINK(root), tg);
  else /* key <= KEY(root) */
    SPLIT((Node*&) LLINK(root), key, ts, (Node*&) LLINK(root));
}

# undef SPLIT

/** 
  @brief Exclusive union of two binary trees search.

  @details join_exclusive(ts,tg) joins two binary trees search into one.
  By unique is meant that all keys "ts" they are smaller than all keys "tg".

  @param[in] ts Binary tree search of minor keys.
  @param[in] tg Binary tree search for higher keys.
  
  @return The resulting tree of the exclusive union.
  
  @note The routine does not validate that tg and ts are binary trees
  search nor that their ranges are exclusive.

  @ingroup Arboles

  @see remove_from_search_binary_tree()
*/
template <class Node> inline Node * join_exclusive(Node *& ts, Node *& tg)
{
  if (ts == Node::NullPtr) 
    return tg;

  if (tg == Node::NullPtr) 
    return ts;

  LLINK(tg) = join_exclusive(RLINK(ts), LLINK(tg));

  RLINK(ts) = tg;
  Node * ret_val = ts;
  ts = tg = Node::NullPtr; /* They may stay empty after join */

  return ret_val;
}  

# define REMOVE remove_from_search_binary_tree<Node, Compare>

/** 
  @brief Deletes a key from a binary tree search by the exclusive union.

  @details remove_from_search_binary_tree(root,key) search in the binary tree
  searches with "root" root node containing the key
  key. If the node is found, then it is replaced in the
  general binary tree by the result of the exclusive union of
  hives removed node.

  @param[in,out] root The root the binary tree search.
  @param[in] key Key of node to delete.
  
  @return A pointer to the node removed if the key is in the
  tree; Node::NullPtr otherwise.

  @see join_exclusive()
  
  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline 
Node * remove_from_search_binary_tree(Node *& root, 
                                      const typename Node::key_type & key)
{
  if (root == Node::NullPtr) 
    return Node::NullPtr;

  if (Compare () (key, KEY(root)))       /* key < KEY(root)? */
    return REMOVE((Node*&) LLINK(root), key);
  else if (Compare () (KEY(root), key))  /* key < KEY(root)? */
    return REMOVE((Node*&) RLINK(root), key);

  Node * ret_val = root; /* Support root to be deleted */
  root = join_exclusive((Node*&) LLINK(root), (Node*&) RLINK(root));

  ret_val->reset();

  return ret_val;
}

# undef REMOVE

/** 
  @brief Insert a node as root in a binary tree search by the 
  partitioning algorithm.

  @details insert_root(root,p) inserts the binary tree search with
  root "root" the node p. After the operation, p becomes the root from 
  binary tree search.

  The routine partitione root into two trees as the key
  contained in p and then attached to the partitions to 
  the branch left and right of p.

  @param[in,out] root The root the binary tree search. After of
  the root operation acquires the value of the parameter p.
  @param[in] p Node to insert as a root.
  
  @return A pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.
  
  @see insert_root_rec()
  
  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_root(Node *& root, Node * p)
{
  Node * l = Node::NullPtr, * r = Node::NullPtr;

  if (not split_key_rec<Node, Compare>(root, KEY(p), l, r)) 
    return Node::NullPtr; 

  LLINK(p) = l;
  RLINK(p) = r;
  root = p;

  return root; 
}

/** 
  @brief Inserts a node with the possibility of doubling as a root in a
  binary tree search by partitioning algorithm.

  @details insert_dup_root(root,p) inserts the binary tree search with
  root "root" the p node. After the operation, p becomes the root of 
  binary tree search.

  The keys equal to KEY(p) are on the left.

  The routine partitione root into two trees as the key
  contained in p and then attached to the partitions to 
  the branch left and right of p.

  @param[in,out] root The root the binary tree search. After of
  the root operation acquires the value of the parameter p.
  @param[in] p Node to insert as a root.
  
  @return A pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.
  
  @see insert_root_rec()
  
  @ingroup Arboles
*/
  template <class Node, 
            class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_dup_root(Node *& root, Node * p)
{
  split_key_dup_rec<Node, Compare>(root, KEY(p), LLINK(p), RLINK(p));
  return root = p;
}

/** 
  @brief Union of two binary trees search as prefix algorithm.

  @details join_preorder(t1,t2,dup) runs the prefix tree t2. Every
  t2 key is inserted into t1. If the key is duplicated,
  then inserted into dup.

  @param[in,out] root A pointer to the root of the binary tree
  @param[in] t1 Binary tree search.
  @param[in] t2 Binary tree search.
  @param[out] dup Binary tree search with the key duplicate of t2.

  @return A pointer to the root of the binary search tree
  resulting from the union of t1 and t2.

  @see join()
  
  @note After the trees calls t1 and t2 become empty;
  however the values of t1 and t2 are unchanged.

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * join_preorder(Node * t1, Node * t2, Node *& dup)
{
  if (t2 == Node::NullPtr) 
    return t1;

  Node * l = (Node*) LLINK(t2); /* t2 branches backups */
  Node * r = (Node*) RLINK(t2);

  if (insert_in_binary_search_tree <Node, Compare> (t1, t2) == Node::NullPtr)
    /* Insertion failed ==> duplicate ==> element inserted into dup */
    insert_in_binary_search_tree<Node, Compare>(dup, t2);

  join_preorder(t1, l, dup);
  join_preorder(t1, r, dup);

  return t1;
}

/** 
  @brief Union of two binary trees search.

  @details join(t1,t2,dup) build a binary tree search corresponding 
  to the union of t1 to t2. Duplicate keys is inserted in dup.

  @param[in,out] root A pointer to the root of the binary tree
  @param[in] t1 Binary tree search.
  @param[in] t2 Binary tree search.
  @param[out] dup Binary tree search with the key duplicate of t2.

  @return A pointer to the root of the binary search tree
  resulting from the union of t1 and t2.

  @see join_preorder()

  @note After the trees calls t1 and t2 become empty;
  however the values of t1 and t2 are unchanged.

  @ingroup Arboles
*/
    template <class Node, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * join(Node * t1, Node * t2, Node *& dup)
{
  if (t1 == Node::NullPtr) 
    return t2;

  if (t2 == Node::NullPtr) 
    return t1;

  Node * l = (Node*) LLINK(t1); /* t1 branches backups */
  Node * r = (Node*) RLINK(t1);

  t1->reset();

    /* Mientras la clave raíz de t1 esté contenida en t2 */
  while (t1 != Node::NullPtr and 
	 insert_root<Node, Compare>(t2, t1) == Node::NullPtr) 
    {     
    /* Sí ==> sáquelo de t1 e insértelo en dup */
      Node * p = remove_from_search_binary_tree(t1, KEY(t1));

      l = (Node*) LLINK(t1);
      r = (Node*) RLINK(t1);

      assert(p != Node::NullPtr);

      insert_in_binary_search_tree<Node, Compare>(dup, p);
    }

  LLINK(t2) = join<Node, Compare>(l, (Node*) LLINK(t2), dup);
  RLINK(t2) = join<Node, Compare>(r, (Node*) RLINK(t2), dup);

  return t2;
}

/** 
  @brief Rotates to the right the binary tree with root p.

  @ingroup Arboles
*/
    template <class Node> inline 
Node * rotate_to_right(Node * p)
{
  assert(p != Node::NullPtr);

  Node * q  = static_cast<Node*>(LLINK(p));
  LLINK(p) = RLINK(q);
  RLINK(q) = p;

  return q;           
} 

/** 
  @brief Rotates to the right with to update of parent.

  @details rotate_to_right(p, pp) rotates to the right the binary tree with
  root p y updated in pp the parent of p, the pointing to the branch
  resulting binary tree rotated.

  @param[in] p Tree root to rotate.
  @param[in,out] pp Parent of p. After the operation, the branch of pp
  than pointing to p is updated to the resulting binary tree rotation.

  @return The resulting binary tree of the rotation.
*/
    template <class Node> inline 
Node * rotate_to_right(Node * p, Node * pp)
{
  assert(p != Node::NullPtr);
  assert(pp != Node::NullPtr);
  assert( ((Node*) LLINK(pp)) == p or ((Node*) RLINK(pp)) == p);

  Node *q = static_cast<Node*>(LLINK(p));
  LLINK(p) = RLINK(q);
  RLINK(q) = p;

  if (static_cast<Node*>(LLINK(pp)) == p) /* Actualización del padre */
    LLINK(pp) = q;
  else
    RLINK(pp) = q;

  return q;           
} 

/** 
  @brief Rota hacia la izquierda el árbol binario con raíz p.

  @ingroup Arboles
*/
    template <class Node> inline
Node* rotate_to_left(Node * p)
{
  assert(p != Node::NullPtr);

  Node *q  = static_cast<Node*>(RLINK(p));
  RLINK(p) = LLINK(q);
  LLINK(q) = p;

  return q;                  
} 

/** 
  @brief Rotates to the left with to update of parent.

  @details rotate_to_left(p, pp) rotates to the left the binary tree with
  root p y updated in pp the parent of p, the pointing to the branch
  resulting binary tree rotated.

  @param[in] p Tree root to rotate.
  @param[in,out] pp Parent of p. After the operation, the branch of pp
  than pointing to p is updated to the resulting binary tree rotation.
  
  @return The resulting binary tree of the rotation.

  @ingroup Arboles
*/
    template <class Node> inline
Node* rotate_to_left(Node * p, Node * pp)
{
  assert(p != Node::NullPtr);
  assert(pp != Node::NullPtr);
  assert(static_cast<Node*>(LLINK(pp)) == p or static_cast<Node*>(RLINK(pp)) == p);

  Node *q = static_cast<Node*>(RLINK(p));
  RLINK(p) = LLINK(q);
  LLINK(q) = p;

    /* Actualización del padre */
  if (LLINK(pp) == p)
    LLINK(pp) = q;
  else
    RLINK(pp) = q;        
      
  return q;                  
} 

/** 
  @brief Iteratively partitions a binary tree search as a key.

  @details split_key(root,key,l,r) iteratively partitioned according to the scale
  key, a binary tree search into two trees l and r. Then of the 
  operation tree, tree root becomes empty, l contains
  all keys under key and r greater or equal.

  The iterative partition is faster than the recursive.

  @param[in,out] root A pointer to the root of the binary tree partition.
  @param[in] key Partition key.
  @param[out] l Tree with smaller keys that key.
  @param[out] r Tree with the biggest keys that key.

  @return true if key is not within the binary tree; in which
  if the partition was performed successfully. Otherwise, if
  key is inside the tree, the tree is not partitioned and the
  result is false.

  @see split_key_rec()

  @note Unlike split_key_rec() this routine includes the 
  partition key if it is already within the binary tree.

  @ingroup Arboles
*/
    template <class Node, class Key, 
              class Compare = Aleph::less<typename Node::key_type> > inline
void split_key(Node * root, const Key & key, Node *& l, Node *& r)
{
  if (root == Node::NullPtr) 
    {
      l = r = (Node*) Node::NullPtr;
      return;
    }

  Node * current;
  Node ** current_parent = NULL;
  Node ** pending_child  = NULL;
  char current_is_right;
  Compare cmp;

  if (cmp (key, KEY(root)))
    {
      r = root;
      pending_child    = &l;
      current_is_right = true;
    }
  else
    {
      l = root;
      pending_child    = &r;
      current_is_right = false;
    }

  current = root;

  while (current != Node::NullPtr) 
    {
      if (cmp (key, KEY(current)))
        { /* Current must be in right side */
          if (not current_is_right)
            { 
              current_is_right = not current_is_right;
              *pending_child   = *current_parent; /* Change of side */
              pending_child    = current_parent;
            }

          current_parent = static_cast<Node**>(&LLINK(current));
        }
      else
        { /* Current must be in left side */
          if (current_is_right)
            { 
              current_is_right = not current_is_right;
              *pending_child   = *current_parent; /* Change of side */
              pending_child    = current_parent;
            }
          current_parent = static_cast<Node**>(&RLINK(current));
        }

      current = *current_parent;
    }

  *pending_child = static_cast<Node*>(Node::NullPtr);
}

/** 
  @brief Link-level exchanges a node with its successor.

  @details swap_node_with_successor(p,pp,q,pq) exchanges at the level of their
  links in a binary tree, the node p with their successor node
  infix q. Links parents p and q are the parameters
  pp and pq required to perform the update.

  The routine is essentially intended for use in
  disposal in a binary tree search.
  
  @param[in] p Node to exchange with his successor.
  @param[in,out] pp Parent of node p.
  @param[in] q Successor of node p.
  @param[in,out] pq Parent of successor q. 
  
  @see swap_node_with_predecessor()
  
  @ingroup Arboles
*/
    template <class Node> inline
void swap_node_with_successor(Node *  p,  /* Node for swapping */
                              Node *& pp, /* Parent of p */
                              Node *  q,  /* Successor inorder of p */
                              Node *& pq) /* Parent of q */
{
  assert(p != Node::NullPtr and q != Node::NullPtr and 
    pp != Node::NullPtr and pq != Node::NullPtr);
  assert(LLINK(pp) == p or RLINK(pp) == p); 
  assert(LLINK(pq) == q or RLINK(pq) == q); 
  assert(LLINK(q) == Node::NullPtr);        
     
    /* Set of pp to its new son q */ 
  if (LLINK(pp) == p)
    LLINK(pp) = q;
  else
    RLINK(pp) = q;
     
  LLINK(q) = LLINK(p);
  LLINK(p) = Node::NullPtr; 

/** @brief Checks if successor is right child of p. In this case, p will become q's son. 
This situation happens when p's son does not have a left branch */   
  if (RLINK(p) == q) 
    {
      RLINK(p) = RLINK(q);
      RLINK(q) = p;
      pq        = pp;
      pp        = q;
      return;
    }

/** @brief In this case, successor is the leftmost node 
descending from right son of p */ 
  Node *qr   = RLINK(q); 
  RLINK(q)  = RLINK(p);
  LLINK(pq) = p;
  RLINK(p)  = qr;

  std::swap(pp, pq);
}

/** 
  @brief Level exchanges links a node to its predecessor.

  @details swap_node_with_predecessor(p,pp,q,pq) exchanges at the level of their
  links in a binary tree, the node p with their predecessor node
  infix q. Links parents p and q are the parameters
  pp and pq required to perform the update.

  The routine is essentially intended for use in the 
  disposal in a binary tree search.

  @param[in] p Node to exchange with his predecessor.
  @param[in,out] pp Parent of node p.
  @param[in] q Predecessor of node p.
  @param[in,out] pq Parent of successor q.

  @see swap_node_with_predecessor()

  @ingroup Arboles
*/
    template <class Node> inline
void swap_node_with_predecessor(Node *  p,  /* Node for swapping */
                                Node *& pp, /* Parent of p */
                                Node *  q,  /* Predecessor inorder of p */
                                Node *& pq) /* Parent of q */
{
  assert((p != Node::NullPtr) and (q != Node::NullPtr) and 
    (pp != Node::NullPtr) and (pq != Node::NullPtr));
  assert((RLINK(pp) == p) or (LLINK(pp) == p)); /* assert pp is parent of p */
  assert((RLINK(pq) == q) or (LLINK(pq) == q)); /* assert pq is parent of q */
  assert(RLINK(q) == Node::NullPtr);
     
    /* Set of pp to its new son q */ 
  if (RLINK(pp) == p)
    RLINK(pp) = q;
  else
    LLINK(pp) = q;
     
  RLINK(q) = RLINK(p);
  RLINK(p) = Node::NullPtr; 

/** @if Checks if predecessor is left child of p. In this case, p will become q's son. 
This situation happens when p's son does not have a right branch */   
  if (LLINK(p) == q) 
    {
      LLINK(p) = LLINK(q);
      LLINK(q) = p;
      pq       = pp;
      pp       = q;
      return;
    }

/** @brief In this case, predecessor is the rightmost node descending from
right son of p */ 
  Node *ql  = LLINK(q); 
  LLINK(q)  = LLINK(p);
  RLINK(pq) = p;
  LLINK(p)  = ql;
  std::swap(pp, pq);
}

/** 
  @brief Insert a node as root in a binary tree search by the 
  recursive reverse and rotations to the root.

  @details insert_root_rec(root,p) inserts the binary tree search with
  root "root" the p node. After the operation, p becomes the root from 
  binary tree search.

  The routine first inserted p in the classic insertion, then
  p rotates until it earns the root.

  @param[in] root The root the binary tree search.
  @param[in] p Node to insert as a root.

  @return A pointer to the inserted node (the new root) if
  p key is not contained within the binary tree
  search; Node::NullPtr otherwise.

  @see insert_root()

  @ingroup Arboles
*/
    template <class Node, class Key, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * insert_root_rec(Node * root, Node * p)
{
  if (root == Node::NullPtr)
    return p; /* Insertion in empty tree */

  if (Compare () (KEY(p), KEY(root)))
    { /* Insert in left subtree */
      Node *left_branch = 
        insert_root_rec<Node, Key, Compare>(static_cast<Node*>(LLINK(root)), p);
      if (left_branch == Node::NullPtr)
        return (Node*) Node::NullPtr;

      LLINK(root) = left_branch;
      root        = rotate_to_right(root);
    }
  else if (Compare () (KEY(root), KEY(p) ))
    { /* Insert in right subtree */
      Node *right_branch = 
        insert_root_rec<Node, Key, Compare>(static_cast<Node*>(RLINK(root)), p);
      if (right_branch == Node::NullPtr)
        return (Node*) Node::NullPtr;

      RLINK(root) = right_branch;
      root        = rotate_to_left(root);
    }
  else
    return (Node*) Node::NullPtr; /* Duplicated key */

  return root;
}

/** 
  @brief Search the p key in the binary tree search or inserts 
  in case if not found.

  @details search_or_insert_root_rec(p) search the binary tree node
  whose key is <code>KEY(p)</code>.
  If the key is found, then it returns a pointer to the node that hosts it.
  Otherwise p is inserted into the binary tree search <code>this</code>.

  @param[in] r Root of tree.
  @param[in] p Node to find or insert.
  
  @return A pointer to the inserted node if the key p is not
  contained within the tree; Otherwise, it returns a pointer 
  to the node in the tree containing <code>KEY(p)</code>.

  @ingroup Arboles
*/
    template <class Node, class Key, 
              class Compare = Aleph::less<typename Node::key_type> > inline
Node * search_or_insert_root_rec(Node * root, Node * p)
{
  if (root == Node::NullPtr)
    return p; /* Insertion in empty tree */

  if (Compare () (KEY(p), KEY(root)))
    { /* Insert in left subtree */
      Node * left_branch = 
        search_or_insert_root_rec<Node, Key, Compare>((Node*) LLINK(root), p);
      if (left_branch == p) /* ¿Hubo inserción? */
        {
          LLINK(root) = left_branch;
          root        = rotate_to_right(root);
          return p;
        }

        return left_branch; /* No la hubo */
    }
  else if (Compare () (KEY(root), KEY(p)))
    { /* Insert in right subtree */
      Node * right_branch = 
        search_or_insert_root_rec<Node, Key, Compare>((Node*) RLINK(root), p);
      if (right_branch == p) /* ¿Hubo inserción? */
        {
          RLINK(root) = right_branch;
          root        = rotate_to_left(root);
          return p;
        }

      return right_branch;  /* No la hubo */
    }

  return root;
}


} /* End Aleph */

# endif // TPL_BINNODEUTILS_H 
