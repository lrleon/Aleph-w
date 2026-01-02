/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file tpl_rbNode.H
 *  @brief Red-Black node type alias.
 *  @ingroup Arboles
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
