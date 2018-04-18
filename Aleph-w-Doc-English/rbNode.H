
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
  @file rbNode.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_RBNODE_H
# define TPL_RBNODE_H

# include <tpl_binNode.H>

typedef unsigned char Color;
# define COLOR(p) ((p)->getColor())
# define RED      (0)
# define BLACK    (1)


class RbNode_Data
{
private:  

  Color  color; /* RED o BLACK */

public:

  RbNode_Data() : color(RED) {}
  RbNode_Data(SentinelCtor) : color(BLACK) {}

  Color & getColor() { return color; }

  void reset() { color = RED; }

}; 

DECLARE_BINNODE_SENTINEL(RbNode, 128, RbNode_Data);

    template <class Node>
bool test_black_condition(Node *p, int &max, int bh = 0)
{
  if (p == Node::NullPtr)
    return true;

  if (COLOR(p) == BLACK)
    bh++; /* Another black node was seen */

  if (LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr)
    {
      if (max == -1) /* true when the first leaf is acomplish */
        max = bh;
      
      return bh == max;
    }
    
  return test_black_condition(LLINK(p), max, bh) and 
         test_black_condition(RLINK(p), max, bh);
}

    template <class Node>
bool is_red_black(Node * node)
{
  if (node == Node::NullPtr)
    return true;

  if (not (COLOR(node) == RED or COLOR(node) == BLACK))
    return false;

  if (COLOR(node) == RED)
    if (COLOR(LLINK(node)) == RED or COLOR(RLINK(node)) == RED)
      return false; 

  int max = -1;
  const int bh  = 0;
  
  return test_black_condition(node, max, bh);
}

    template <class Node>
bool is_red_black_tree(Node * node)
{
  if (is_red_black(node))
    return is_red_black(LLINK(node)) and is_red_black(RLINK(node));

  return false;
}
# endif // TPL_RBNODE_H
