
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file tpl_rbNode.H
 *  @brief Red-Black node type alias.
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_RBNODE_H
# define TPL_RBNODE_H


# include <tpl_binNode.H>


# define Color        unsigned char


# ifdef DEBUG
# define RED          ('R')
# define BLACK        ('B')
# else
# define RED          (0)
# define BLACK        (1)
# endif


class RbNode_Data
{
private:
  
  Color  color; // color is either BLACK or RED 
public:

  RbNode_Data() noexcept : color(RED) { /* empty */ }

  RbNode_Data(SentinelCtor) noexcept : color(BLACK) { /* empty */ }

  Color& getColor() noexcept { return color; }

  void reset() noexcept { color = RED; }
}; 


DECLARE_BINNODE_SENTINEL(RbNode, 128, RbNode_Data);


# endif // TPL_RBNODE_H
