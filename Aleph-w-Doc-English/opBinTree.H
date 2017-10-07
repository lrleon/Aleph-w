
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
  @file opBinTree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef OPBINTREE_H 
# define OPBINTREE_H

# include <tpl_dynArray.H>

namespace Aleph {

# define COST(i,j) (cost[(i)*(n+1) + (j)])
# define TREE(i,j) (tree[(i)*(n+1) + (j)])

static inline double sum_p(double p[], const int & i, const int & j);

    static inline int 
min_index(DynArray <double>&, const int &, const int &, const int &);

    static inline 
void compute_optimal_costs(DynArray <double> & cost, double p[], 
                           const int & n, DynArray <int> & tree)
{
  int i, j, k;
  for (i = 1; i <= n; ++i)
    {
      COST(i, i - 1) = 0;
      TREE(i, i) = i;
    }
  for (i = 0; i < n; ++i)
    for (j = 1; j <= n - i; ++j)
      {
        k          = j + i;
        TREE(j, k) = min_index(cost, j, k, n);
        COST(j, k) = sum_p(p, j, k) + 
                     COST(j, TREE(j, k) - 1) + COST(TREE(j, k) + 1, k);
      }
}
static inline double sum_p(double p[], const int & i, const int & j)
{
  double sum = 0.0;
  for (int k = i - 1; k < j; ++k)
    sum += p[k];
  return sum;
}
    static inline 
int min_index(DynArray <double>& cost, 
              const int & j, const int & k, const int & n)
{
  int ret_val;
  double min_sum = 1e32;
  for (int i = j; i <= k; ++i)
    {
      const double sum = COST(j, i - 1) + COST(i + 1, k);
      if (sum < min_sum)
        {
          min_sum = sum;
          ret_val = i;
        }
    }
  return ret_val;
}
    template<class Node, typename Key> static inline
Node * compute_tree(Key keys[], DynArray<int>& tree, 
                    const int & n, const int & i, const int & j)
{
  if (i > j) 
    return Node::NullPtr;
  Node * root = new Node (keys[TREE(i, j) - 1]);
  LLINK(root) = compute_tree <Node, Key> (keys, tree, n, i, TREE(i, j) - 1);
  RLINK(root) = compute_tree <Node, Key> (keys, tree, n, TREE(i, j) + 1, j);
  return root;
}

/** 
  @brief Build a binary search tree as optimal frequencies
  estimated search.

  @details build_optimal_tree(keys,p,n) builds a binary tree
  optimal search containing n keys stored in the
  under keys [], each key frequently or likely
  search stored in the array p [], parallel keys [].

  @param[in] keys Array containing the keys.
  @param[in] p Arrangement containing probabilities appearance or search.
  @param[in] n Total number of keys; 
  This is the dimension of arrangements.
  
  @return The root of the optimal binary search tree.
  
  @throw bad_alloc If there is insufficient memory.

  @ingroup Arboles
*/
    template <class Node, typename Key>
Node * build_optimal_tree(Key keys[], double p[], const int & n)
{
  DynArray <int> tree((n + 1)*(n + 1));
  DynArray <double> cost((n + 1)*(n + 1));
  compute_optimal_costs(cost, p, n, tree);
  return compute_tree<Node, Key> (keys, tree, n, 1, n);
}


# undef COST
# undef TREE

} /* End namespace Aleph */

# endif // OPBINTREE_H
