
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
  @file tpl_balanceXt.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BALANCEXT_H
# define TPL_BALANCEXT_H

# include <tpl_binNodeXt.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Select a node of a binary tree by infix position and converts it 
  to its root.

  @details select_gotoup_root(r,i) select the node with infix position
  i and rotate it until it earns its root.

  This selection algorithm is recursive.

  @param[in] root The binary tree root with ranges.
  @param[in] i Infix position you want to access.

  @return A pointer to the node in the position i infix which after
  the call is a root of the binary tree.

  @throw out_of_range If i is greater than or equal to the total amount
  of binary tree nodes.

  @see select() select_rec()

  @ingroup Arboles
*/
    template <class Node> inline
Node * select_gotoup_root(Node * root, const size_t & i)
{

  assert(root not_eq Node::NullPtr);
  assert(COUNT(Node::NullPtr) == 0);

  if (i >= COUNT(root))
    throw std::out_of_range ("");

  if (i == COUNT(LLINK(root))) 
    return root;

  if (i < COUNT(LLINK(root)))
    {
      LLINK(root) = select_gotoup_root(LLINK(root), i);
      root = rotate_to_right_xt(root);
    }
  else
    {
      RLINK(root) = select_gotoup_root(RLINK(root), 
                                       i - COUNT(LLINK(root)) - 1);
      root = rotate_to_left_xt(root);
    }
  return root;
}
/** 
  @brief Rebalances a binary tree search.

  @details balance_tree(root) take the binary tree search with root
  "root" and rebalances "perfectly" in the sense that for
  each node the difference between the cardinality of its
  subtrees is at most one.

  The operation has a performance of \f$O(n\; \lg n)\f$.

  @param[in] root The root of the binary tree search
  to balance.

  @return The root of the balanced binary tree search.
*/
template <class Node> inline Node * balance_tree(Node * root)
{
  if (COUNT(root) <= 1) 
    return root;

  root        = select_gotoup_root(root, COUNT(root) / 2);
  LLINK(root) = balance_tree(LLINK(root));
  RLINK(root) = balance_tree(RLINK(root));

  return root;
}

} /* End namespace Aleph */

# endif // TPL_BALANCEXT_H
