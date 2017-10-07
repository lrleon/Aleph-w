
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
  @file tpl_union.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/

# ifndef TPL_UNION_H
# define TPL_UNION_H

# include <tpl_dynArray.H>
# include <tpl_dynSetTree.H>
 
# define UNION_ATTRS	       \
  DynArray<size_t> id;	       \
  DynArray<size_t> sz;         \
  size_t           num_blocks;

# define UNION_ROOT_IMPL \
  while (i != id(i))	 \
    {			 \
      id(i) = id(id(i)); \
      i = id(i);	 \
    }			 \
			 \
  return i;					

# define UNION_DEPTH		\
  size_t depth(size_t i)	\
  {				\
    const int & parent = id(i); \
				\
    if (parent == i)		\
      return 1;			\
				\
    return 1 + depth(parent);	\
  }

# define UNION_CTOR		\
  : num_blocks(n)		\
  {				\
    id.reserve(n);		\
    sz.reserve(n);		\
				\
    for (int i = 0; i < n; ++i) \
      {				\
	id(i) = i;		\
	sz(i) = 1;		\
      }				\
  }

# define UNION_PUBLIC_ROUTINES				\
  size_t size() const { return id.size(); }		\
  							\
  size_t get_num_blocks() const { return num_blocks; }	\
  							\
  bool are_connected(size_t i, size_t j) 		\
  {							\
    return root(i) == root(j);				\
  }							\
							\
  void join(size_t i, size_t j)				\
  {							\
    i = root(i);					\
    j = root(j);					\
    if (i == j)						\
      return;						\
							\
    if (sz(i) < sz(j))					\
      {							\
	id(i) = j;					\
	sz(j) += sz(i);					\
      }							\
    else						\
      {							\
	id(j) = i;					\
	sz(i) += sz(j);					\
      }							\
    --num_blocks;					\
  }									

/** 
  @brief Binary equivalence relation between integers.
  (Quick Union problem).

  @details Relation implements a binary relationship between integer elements.

  The algorithm used is called Quick weithed fast union.

  Completely inspired Sedgewick & Wayne, Algorithms, 4th edition.

  This class is very high performance. Although theoretically its 
  operations are O(lg n), in practice they tend to be O(1) time
  amortized.

  The amount of integers n is specified in the constructor of the 
  class. The elements interact must be between 0
  and n-1. The operations do not validate if the integer is valid.

  If the amount of integers is unknown, then use the "Relation" 
  class. If it required other link elements, use then Relation_T

  @see relation Relation_T
  
  @ingroups Grafos
*/
class Fixed_Relation
{
  UNION_ATTRS

  size_t root(size_t i)
  {
    UNION_ROOT_IMPL
  }

  UNION_DEPTH

public:

/** @brief Begins a relationship of n elements labeled with [0..n) */
  Fixed_Relation(size_t n) UNION_CTOR

  UNION_PUBLIC_ROUTINES
};


/** 
  @brief Binary equivalence relation between integers.
  (Quick Union problem).

  @details Relation implements a binary relationship between integer elements.

  The algorithm used is called Quick weithed fast junction.

  Completely inspired Sedgewick & Wayne, Algorithms, 4th edition.

  This class is very high performance. Although its operations are
  O(lg n), in practice tend to be O(1) time amortized.

  @ingroups Grafos
 */
class Relation
{
  UNION_ATTRS

  void verify_if_add_new_points(size_t n)
  {
    size_t l = size();
    if (n < l)
      return;
    
    id.reserve(l, n);
    sz.reserve(l, n);
    for (int i = l; i <= n; ++i)
      {
	id(i) = i;
	sz(i) = 1;
      }
    num_blocks += n - l + 1;
  }

  size_t root(size_t i)
  {
    verify_if_add_new_points(i);
    UNION_ROOT_IMPL;
  }

  UNION_DEPTH

public:

/** @brief Begins a relationship of n elements labeled with [0..n) */
  Relation(size_t n = 0) UNION_CTOR

  UNION_PUBLIC_ROUTINES
};


/**
  @brief Eqivalencia binary relationship between integers.
  (Quick Union problem).

  @details Relation_T implements a binary relation between elements
  any kind.

	The algorithm used is called Quick Fast Weigthed Union.

	Completely inspired Sedgewick & Wayne, Algorithms, 4th edition.

	This class is very high performance. Although its operations are
  O(lg n), in practice tend to be O(1) time
  amortized.

	@ingroups Grafos
*/
    template <typename T, class Compare = Aleph::less<T>>
class Relation_T : public Relation
{
  struct Pair
  {
    T      item;
    size_t i;

    Pair() {}

    Pair(const T & __item, size_t __i) 
      : item(__item), i(__i) { /* Empty */ }
  };

  struct Cmp
  {
    bool operator () (const Pair & p1, const Pair & p2) const
    {
      return Compare() (p1.item, p2.item);
    }
  };

  DynSetAvlTree<Pair, Cmp> items_tree;

/** @brief Returns the id of the "item"; either because it is or because you inserted. */
  size_t test_and_insert_new_item(const T & item)
  {
    size_t N = size();
    Pair p(item, N);
    Pair * ptr = items_tree.search_or_insert(p);
    return ptr->i;
  }

public:

/** @brief Returns true is "i" and "j" are connected. */
  bool are_connected(const T & p, const T & q)
  {
    size_t i = test_and_insert_new_item(p);
    size_t j = test_and_insert_new_item(q);

    return Relation::are_connected(i, j);
  }

/** @brief Join "p" with "q" */
  void join(const T & p, const T & q)
  {
    size_t i = test_and_insert_new_item(p);
    size_t j = test_and_insert_new_item(q);
    Relation::join(i, j);
  }
};

# endif // TPL_UNION_H
