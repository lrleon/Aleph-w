
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
  @file tpl_sort_utils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_SORT_UTILS_H
# define TPL_SORT_UTILS_H

# include <ahUtils.H>
# include <ahFunctional.H>
# include <tpl_arrayStack.H>
# include <tpl_dynArray.H>
# include <tpl_dynDlist.H>
# include <htlist.H>

namespace Aleph 
{
    /* These values must be long (not unsigned) in order to satisfyingly
    check conditions in boolean expresion */
  extern long Insertion_Threshold;

  extern long Quicksort_Threshold;

  extern const int Not_Found;

/** 
  @return true if the sequence is sorted

  @ingroup Algos
*/
  template <template <typename> class Container, typename T, class Compare>
bool is_sorted(const Container<T> & cont, Compare & cmp)
{
  if (cont.is_empty())
    return true;

  T item = cont.get_first();
  typename Container<T>::Iterator it(cont); 
  for (it.next(); it.has_curr(); it.next())
    {
      const T curr = it.get_curr();
      if (cmp(curr, item))
	return false;

      item = curr;
    }
  return true;
}

  template <template <typename> class Container, typename T, class Compare = Aleph::less<T>>
bool is_sorted(const Container<T> & cont, Compare && cmp = Compare())
{
  return is_sorted<Container, T, Compare>(cont, cmp);
}

/** 
  @brief Sorts an array by the method of selection.

  @details selection_sort(a,n) the selection method used to order
  the array "a" of n elements.

The selection method has a performance of \f$O(n^2)\f$.
  Because of its simplicity of implementation, its constant cost
  it is low, so it is a good method to arrays of 
  very small dimension.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] a Array to order.
  @param[in] n Dimension of the array.

  @see insertion_sort() quicksort_rec() mergesort() heapsort()

  @ingroup Algos
*/
    template <typename T, class Compare> inline
void selection_sort(T * a, const size_t n, Compare & cmp)
{
  for (int i = 0, min, j; i < n - 1; ++i)
    {
      for (min = i, j = i + 1; j < n; ++j)
	if (cmp(a[j], a[min]))
	  min = j;

      if (cmp(a[min], a[i]))
        std::swap(a[min], a[i]);
    }
}


    template <typename T, class Compare = Aleph::less<T>> inline
void selection_sort(T * a, const size_t n, Compare && cmp = Compare())
{
  selection_sort<T, Compare>(a, n, cmp);
}


/** 
  @brief Generic search of an end item in a list
  Dlink node.
      		
  @details search_extreme(list) sequentially searches the list of nodes
  "list" end member, minimum or
  maximum at the discretion of comparison Compare.
  @param[in] list List on which to search.

  @return A pointer to the node contentivo of extreme element.

  @note Do not check if the list is empty.

  @ingroup Algos
*/
  template <class Link, class Compare> inline 
Link * search_extreme(const Link & list, Compare & cmp)
{
  typename Link::Iterator it(const_cast<Link&>(list));
  Link * extreme = it.get_current();

  for (it.next(); it.has_curr(); it.next())
    {
      Link * curr = it.get_curr();
      if (cmp(curr, extreme))
	extreme = curr;
    } 

  return extreme;
}

/** @brief Wrapper to call to search_extreme with r-value */
    template <class Compare> inline 
Dlink * search_extreme(const Dlink & list, Compare & cmp)
{
  return search_extreme<Dlink, Compare>(list, cmp);
}

/** @brief Wrapper to call to search_extreme with r-value */
    template <class Compare> inline 
Dlink * search_extreme(const Dlink & list, Compare && cmp = Compare())
{
  return search_extreme<Dlink, Compare>(list, cmp);
}

/** @brief Wrapper to call to search_extreme with r-value */
    template <class Compare> inline 
Slinknc * search_extreme(const Slinknc & list, Compare & cmp)
{
  return search_extreme<Slinknc, Compare>(list, cmp);
}

/** @brief Wrapper to call to search_extreme with r-value */
    template <class Compare> inline 
Slinknc * search_extreme(const Slinknc & list, Compare && cmp = Compare())
{
  return search_extreme<Compare>(list, cmp);
}

/** 
  @brief Order by the method of selecting a doubly linked list.

  @details selection_sort(list) on a list derived from Dlink orders
  by the method of selecting the "list" list.

  The routine requires parameter type Compare that implements the
  comparison between list members. Compare invoked as
  Compare()(l1,l2), where "l1" and "l2" are two links Dlink type. The
  user are responsible for properly implementing Compare()()
  so that access areas of interest and make the
  comparison.

  @param[in,out] list List to be sorted.

  @ingroup Algos
*/
    template <class Compare> inline 
void selection_sort(Dlink & list, Compare & cmp)
{
  Dlink aux;
  while (not list.is_empty())
    {
      Dlink * extreme = search_extreme <Dlink, Compare> (list, cmp); 
      extreme->del(); /* Saque extremo de list */
      aux.append(extreme); /* Insértelo ordenado en aux; */
    }

  list.swap(&aux);
}

    template <class Compare> inline 
void selection_sort(Dlink & list, Compare && cmp = Compare())
{
  selection_sort<Compare>(list, cmp);
}


template <class Tlink, 
	  template <class> class Tnode, 
	  typename T, class Compare> 
class Compare_Tnode
{
  Compare & cmp;

public:

  Compare_Tnode(Compare && __cmp = Compare()) : cmp(__cmp) { /* Empty */ }

  Compare_Tnode(Compare & __cmp) : cmp(__cmp) { /* Empty */ }

  bool operator () (Tlink * l1, Tlink * l2) const
  {
    Tnode<T> * n1 = static_cast<Tnode<T>*>(l1);
    Tnode<T> * n2 = static_cast<Tnode<T>*>(l2);

    assert(n1 == l1 and n2 == l2);

    return cmp(n1->get_data(), n2->get_data());
  }

/** @brief Para generalizar en el sequential_search */
  bool operator () (Tlink * l, const T & x) const
  {
    Tnode<T> * n = static_cast<Tnode<T>*>(l);

    assert(n == l);

    return cmp(n->get_data(), x);
  }
};

    template <typename T, class Compare> 
struct Compare_Dnode : public Compare_Tnode<Dlink, Dnode, T, Compare>
{
  Compare_Dnode(Compare && cmp = Compare()) 
    : Compare_Tnode<Dlink, Dnode, T, Compare> (std::move(cmp)) 
  { /* Empty */ }

  Compare_Dnode(Compare & cmp) 
    : Compare_Tnode<Dlink, Dnode, T, Compare> (cmp) 
  { /* Empty */ }
};

template <typename T, class Compare>
using Compare_Snodenc = Compare_Tnode<Slinknc, Snodenc, T, Compare>;


/** 
  @brief Order a linked list of Dnode<T> type using
  the selection method.

  @details Order the "list" list, with nodes of Dnode<T> type
  according to the method of selection.

  The selection method has a performance of \f$O(n^2)\f$.
  Because of its simplicity of implementation, its constant cost
  it is low, so it is a good method to arrays of 
  very small dimension.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] list ka list to be sorted.

  @ingroup Algos
*/
    template <typename T, class Compare> 
inline void selection_sort(Dnode<T> & list, Compare & cmp)
{
  selection_sort <Compare_Dnode<T, Compare>>
    (list, Compare_Dnode<T, Compare>(cmp));
}

    template <typename T, class Compare = Aleph::less<T>> 
inline void selection_sort(Dnode<T> & list, Compare && cmp = Compare())
{
  selection_sort<T, Compare>(list, cmp);
}

/**
  @brief Sequential search on an array.

  @details sequential_search(a,x,l,r) sequentially searches,
  on the "a" array, among the indices "l" and "r" respectively
  the first occurrence of "x".

  The function manages two type parameters:
  -# T: Data type that housing the array.
  -# Equal: Class comparison between the elements.

  @param[in] a Array upon which the search is performed.
  @param[in] x Element to search.
  @param[in] l Left index where the search begins.
  @param[in] r Right index where the search ends.

  @return The index of the first position in array containing
  the element x; -1 otherwise.

  @ingroup Algos
*/
      template <typename T, class Equal> inline 
int sequential_search(T * a, const T & x, const int l, const int r, Equal & eq)
{
  for (int i = l; i <= r; i++)
    if (eq(a[i], x)) return i;

  return Not_Found;
}

      template <typename T, class Equal = Aleph::equal_to<T>> inline 
int sequential_search(T * a, const T & x, const int l, const int r,
		      Equal && eq = Equal())
{
  return sequential_search<T, Equal>(a, x, l, r, eq);
}


/** 
  @brief Sequential search on a dynamic array.

  @details sequential_search(a,x,l,r) sequentially searches,
  on the "a" array, among the indices "l" and "r" respectively
  the first occurrence of "x".

  The function manages two type parameters:
  -# T: Data type that housing the array.
  -# Equal: Class comparison between the elements.

  This version of dynamic arrays jumps positions
  they certainly have not been written. However, take
  into account that, according to the block size can
  tickets have not written but circumscribed in directions
  valid memory. In this sense, it is possible that within
  Entries that kind of an element with value "x" are.

  @param[in] a Dynamic array upon which the search is performed.
  @param[in] x Element to search.
  @param[in] l Left index where the search begins.
  @param[in] r Right index where the search ends.

  @return The index of the first position in array containing
  the element x; -1 otherwise.

  @note The search can not tell if a valid entry has
  It has been written or not. the Class programming "Equal" can
  consider a special value that represents the input is not
  it has been written. It ensures that input has not been
  mapped a block will not be accessed.

  @see DynArray

  @ingroup Algos
*/
    template <typename T, class Equal = Aleph::equal_to<T>> inline
int sequential_search(const DynArray<T> & a, const T & x, 
		      const int l, const int r, Equal & eq)
{
  for (int i = l; i <= r; i++)
    if (a.exist(i))
      if (eq(a.access(i), x))
	return i;

  return -1;
}

    template <typename T, class Equal = Aleph::equal_to<T>> inline
int sequential_search(const DynArray<T> & a, 
		      const T & x, const int l, const int r,
		      Equal && eq = Equal())
{
  return sequential_search<T, Equal>(a, x, l, r, eq);
}


    template <class Link, typename T, class Equal>
Link * sequential_search(const Link & list, const T & x, Equal & eq)
{
  for (typename Link::Iterator it(const_cast<Link&>(list)); 
       it.has_curr(); it.next())
    {
      Link * curr = it.get_curr();
      if (eq(curr, x)) 
	return curr;
    }

  return NULL;
}

    template <typename T, class Equal>
Dlink * sequential_search(const Dlink & list, const T & x, Equal & eq)
{
  return sequential_search <Dlink, T, Equal>(list, x, eq);
}

template <typename T, class Equal>
Dlink * sequential_search(const Dlink & list, const T & x, 
			  Equal && eq = Equal())
{
  return sequential_search <T, Equal>(list, x, eq);
}

    template <typename T, class Equal>
Slinknc * sequential_search(const Slinknc & list, const T & x, Equal & eq)
{
  return sequential_search <Slinknc, T, Equal>(list, x, eq);
}

template <typename T, class Equal>
Slinknc * sequential_search(const Slinknc & list, const T & x, 
			    Equal && eq = Equal())
{
  return sequential_search <T, Equal>(list, x, eq);
}

/** 
  @brief Sequential search on a list of nodes.

	@details sequential_search(list,x) sequentially searches,
  in the nodes list of Dnode<T> type "list" the first
  occurrence of "x".

	The function manages two type parameters:
	-# T: Data type that housing the array.
	-# Equal: Class comparison between the elements.

	@param[in] list List in which the search is performed;
	@param[in] x Element to search.
	
  @return A pointer to the first node containing
  the "x" element; NULL otherwise.
	
  @see Dnode
	
  @ingroup Algos
*/
    template <typename T, class Equal = Aleph::equal_to<T>> inline
Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, 
			     Equal & eq)
{
  Dlink * ret =
   sequential_search<T, Compare_Dnode <T, Equal>>
      ((Dlink &) list, x, Compare_Dnode<T, Equal>(eq));

  return ret == NULL ? NULL : static_cast <Dnode<T> *>(ret);
}

    template <typename T, class Equal = Aleph::equal_to<T>> inline
Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, 
			     Equal && eq = Equal())
{
  return sequential_search<T, Equal>(list, x, eq);
}

/** 
  @brief Sequential search of a dynamic list DynDlist.t.

  @details sequential_search(list,x) sequentially searches,
  in the "list" dynamic list (DynDlist<T>) the first
  occurrence of "x".

  The function manages two type parameters:
  -# T: Data type that housing the array.
  -# Equal: Class comparison between the elements.

  @param[in] list Dynamic list in which the search is performed;
  @param[in] x Element to search.

  @return A pointer to the first element equal to "x";
  NULL otherwise.

  @see DynDlist

  @ingroup Algos
*/
    template <typename T, class Equal> inline
T * sequential_search(const DynDlist<T> & list, const T & x, Equal & eq)
{
  Dnode<T> * ret = sequential_search<T, Equal>((Dnode<T>&) list, x, eq);
  return ret != NULL ? &ret->get_data() : NULL;
}

    template <typename T, class Equal = Aleph::equal_to<T>> inline
T * sequential_search(const DynDlist<T> & list, const T & x, 
		      Equal && eq = Equal())
{
  return sequential_search<T, Equal>(list, x, eq);
}

    template <typename T, class Equal> inline
T * sequential_search(const DynList<T> & list, const T & x, Equal & eq)
{
  Slinknc * ret =
    sequential_search<T, Compare_Snodenc<T, Equal>>
     (*list.get_head(), x, Compare_Snodenc<T, Equal>(eq));
  return ret != NULL ? &((static_cast<Snodenc<T>*>(ret))->get_data()) : NULL;
}

    template <typename T, class Equal = Aleph::equal_to<T>> inline
T * sequential_search(const DynList<T> & list, const T & x, 
		      Equal && eq = Equal())
{
  return sequential_search<T, Equal>(list, x, eq);
}

/** 
  @brief Generic search in an array of one end element.
	
  @details search_extreme(a, l, r) Search sequentially in the array
  to, among indexes "l" and "r", the end member, minimum or
  maximum at the discretion of comparison Compare.

  @param[in] a Array on which to search.
  @param[in] l Index beginning of the search.
  @param[in] r Index search term.

  @return The index contentivo of extreme.

  @note It does not check if the sequence is empty.

  @ingroup Algos
*/
    template <typename T, class Compare = Aleph::less<T>> inline
int search_extreme(T * a, const int l, const int r, Compare & cmp)
{
  T extreme_index = l;
  for (int i = l + 1; i <= r; i++)
    if (cmp(a[i], a[extreme_index])) /* ¿Se ve un nuevo menor? */
      extreme_index = i; /* sí */

  return extreme_index;
}

    template <typename T, class Compare = Aleph::less<T>> inline
int search_extreme(T * a, const int l, const int r, 
		   Compare && cmp = Compare())
{
  return search_extreme<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Returns the minimum element of the array to between l and r.

  @ingroup Algos
*/
    template <typename T, class Compare = Aleph::less<T>> inline
int search_min(T * a, const int l, const int r, Compare & cmp)
{
  return search_extreme<T, Compare> (a, l, r, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
int search_min(T * a, const int l, const int r, Compare && cmp = Compare())
{
  return search_min<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Returns the maximum element of the array to between l and r.

  @ingroup Algos
*/
    template <typename T, class Compare = Aleph::greater<T>> inline
int search_max(T * a, const int l, const int r, Compare & cmp)
{
  return search_extreme<T, Compare> (a, l, r, cmp);
}

    template <typename T, class Compare = Aleph::greater<T>> inline
int search_max(T * a, const int l, const int r, Compare && cmp = Compare())
{
  return search_max<T, Compare>(a, l, r, cmp);
}

/** 
  @overload search_extreme

  @ingroup Algos
*/
    template <typename T, class Compare> inline 
Dnode<T> * search_extreme(const Dnode<T> & list, Compare & cmp)
{
  Compare_Dnode<T, Compare> cmp_dnode(cmp);
  Dlink * ret = 
    search_extreme<Dlink, Compare_Dnode<T, Compare>>((Dlink&) list, cmp_dnode);

  return static_cast <Dnode<T>*> (ret);
}

/** 
  @brief Generic search of an end item in a list
  node (Dlist<T>).
    		
	@details search_extreme(list) sequentially searches the list of nodes
  "list" element end, minimum or
  maximum at the discretion of comparison Compare.

	@param[in] list List on which to search.

	@return A pointer to the node contentivo of extreme element.
	
  @note Do not check if the list is empty.
	
  @ingroup Algos
*/
    template <typename T, class Compare> inline
T * search_extreme(const DynDlist<T> & list, Compare & cmp)
{
  Dnode<T> * ret = search_extreme <T, Compare> ((Dnode<T>&) list, cmp); 

  return ret != NULL ? &(ret->get_data()) : NULL;
}

    template <typename T, class Compare = Aleph::less<T>> inline 
Dnode<T> * search_extreme(const Dnode<T> & list, Compare && cmp = Compare())
{
  return search_extreme<T, Compare>(list, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
T * search_extreme(const DynDlist<T> & list, Compare && cmp = Compare())
{
  return search_extreme<T, Compare>(list, cmp);
}

    template <typename T, class Compare> inline 
T * search_extreme(const DynList<T> & list, Compare & cmp)
{
  Slinknc * ret =
    search_extreme <Compare_Snodenc<T, Compare>>
      (*list.get_head(), Compare_Snodenc<T, Compare>(cmp)); 

  return ret != NULL ? &(((Snodenc<T>*)ret)->get_data()) : NULL;
}

    template <typename T, class Compare> inline 
T * search_extreme(const DynList<T> & list, Compare && cmp = Compare())
{
  return search_extreme<T, Compare>(list, cmp);
}

/** 
  @brief Returns the minimum element of "list" list.

	@ingroup Algos
*/
    template <typename T, class Compare> inline
T * search_min(const DynDlist<T> & list, Compare & cmp)
{
  return search_extreme<T, Compare> (list, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
T * search_min(const DynDlist<T> & list, Compare && cmp = Compare())
{
  return search_min<T, Compare>(list, cmp);
}

/** 
  @brief Returns the maximum element of the "list" list.

	@ingroup Algos
*/
    template <typename T, class Compare> inline
T * search_max(const DynDlist<T> & list, Compare & cmp)
{
  return search_extreme<T, Compare> (list, cmp);
}


    template <typename T, class Compare = Aleph::greater<T>> inline
T * search_max(const DynDlist<T> & list, Compare && cmp = Compare())
{
  return search_max<T, Compare>(list, cmp);
}

/**
  @brief Sorts an array by the method of insertion.

	@details insertion_sort(a,l,r) it selection method used to sort 
  the array to indices between "l" and "r"

	The insertion method has a performance of \f$O(n^2)\f$. 
	It is a simple method, which consumes little time constant.
  On average accounts for half of the exchanges that the method
  selection. Your time tends to be linear if the array is
  semi-ordered. It is a good method for small arrays and
  partitions made by methods above but with
  larger time constants.

	The method uses two type parameters:
	-# T: Type elements having the array.
	-# Compare: Class of comparison.

	@param[in,out] Array to order.
	@param[in] l Left index of the array.
	@param[in] r Right index of the array.
	
  @see selection_sort() quicksort_rec() mergesort() heapsort()
	
  @ingroup Algos
*/
    template <typename T, class Compare> inline
void insertion_sort(T * a, const size_t l, const size_t r, Compare & cmp)
{
  for (int i = l, j; i <= r; ++i)
    {
      T tmp = a[i]; /* Memorice a[i], pues será sobre escrito */
      
      for (j = i; j > l and cmp(tmp, a[j - 1]); --j)
	a[j] = a[j - 1]; /* Desplazar hacia la derecha */

      a[j] = tmp; /* Inserte tmp en la brecha */
    }
}

    template <typename T, class Compare = Aleph::less<T>> inline
void insertion_sort(T * a, const size_t l, const size_t r,
		    Compare && cmp = Compare())
{
  insertion_sort<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Inserts a node neatly in a doubly linked list.

  @details insert_sorted(list,p) inserted in a list based on Dlink the 
  node pointed to by "p".

  The sorting order is given by the class of comparison Compare.

  @param[in,out] list List that will be inserted "p".
  @param[in] p the node to be inserted.

  @see Dlink 

  @ingroup Algos
*/
    template <class Compare> inline 
void insert_sorted(Dlink & list, Dlink * p, Compare & cmp)
{
  typename Dlink::Iterator it(list); 
  while (it.has_curr() and cmp(it.get_curr(), p)) 
    it.next();

  if (it.has_curr())
    it.get_curr()->append(p); /* Insertar antes de current */
  else
    list.append(p); 
}


    template <class Compare> inline 
void insert_sorted(Dlink & list, Dlink * p, Compare && cmp = Compare())
{
  insert_sorted<Compare>(list, p, cmp);
}

template <class Compare> inline
void insert_sorted(HTList & list, Slinknc * p, Compare & cmp)
{
  Slinknc * first = list.get_first();
  if (cmp(p, first) or not cmp(first, p)) /* p <= first? */
    {
      list.insert(p);
      return;
    }

  Slinknc * last = list.get_last();
  if (cmp(last, p) or not cmp(p, last)) /* p >= last? */
    {
      list.append(p);
      return;
    }

  Slinknc * prev = list.get_first();
  typename HTList::Iterator it(list); 
  for (it.next(); it.has_curr(); it.next())
    {
      Slinknc * curr = it.get_curr();
      if (cmp(p, curr)) /* p < curr */
	{
	  prev->insert(p);
	  return;
	}
      prev = curr;
    }
  assert(false); /* It should be unreachable */
}

template <class Compare> inline
void insert_sorted(HTList & list, Slinknc * p, Compare && cmp = Compare())
{
  insert_sorted<Compare>(list, p, cmp);
}

/** 
  @brief Ordered according to the method of inserting a list of simple nodes

	@details insertion_sort(list) ordered according to the method of inserting the
  linked list "list"

	The insertion method has a performance of \f$O(n^2)\f$. 
	
	@param[in,out] list Sort list.
	
  @ingroup Algos
*/
template <class ListType, class Compare> inline 
void list_insertion_sort(ListType & list, Compare & cmp)
{
  if (list.is_empty())
    return;

  ListType aux; aux.append(list.remove_first());
  while (not list.is_empty())
    insert_sorted<Compare>(aux, list.remove_first(), cmp);

  list.swap(aux);
}


template <typename T, class Compare> inline 
void insertion_sort(DynList<T> & l, Compare & cmp)
{
  using Cmp = Compare_Snodenc<T, Compare>;
  Cmp c(cmp);  
  list_insertion_sort<HTList, Cmp>(l, c);
}

template <typename T, class Compare = Aleph::less<T>> inline 
void insertion_sort(DynList<T> & l, Compare && cmp = Compare())
{
  insertion_sort<T, Compare>(l, cmp);
}



/** 
  @brief Order the "list" list by the method of insertion.

	@details Order the "list" list based on Dnode<T> method by
  insertion by comparison criterion Compare.

	The insertion method has a performance of \f$O(n^2)\f$. 

	@param[in,out] list Sort list.

	@see Dnode selection_sort(Dlink * link)
*/
    template <typename T, class Compare> 
inline void insertion_sort(Dnode<T> & list, Compare & cmp)
{
  using Cmp = Compare_Dnode<T, Compare>;
  Cmp c(cmp);
  list_insertion_sort <Dlink, Cmp> (list, c);
}


    template <typename T, class Compare = Aleph::less<T>> 
inline void insertion_sort(Dnode<T> & list, Compare && cmp = Compare())
{
  insertion_sort<T, Compare>(list, cmp);
}


/** 
  @brief Mix two ordered partitions stored in an array.

  @details merge() it takes as input two ordered partitions between
  ranges [l..m] and [m + 1..r] and the mixture in a system between
  [l..r].

  @param[in] a Array
  @param[in] l Array left index
  @param[in] m Array center, of the partition
  @param[in] r Array right index

  \ingroup Algos
*/
    template <typename T, class Compare> inline
void merge(T * a, const int l, const int m, const int r, Compare & cmp)
{
  int i, j, k, s;
  s = r - l + 1;
  T b[s];
 
  for (i = l, k = 0; i <= m; ++i, ++k)
    b[k] = a[i];

  for (j = r; j > m; ++k, --j)
    b[k] = a[j];

  for (k = l, i = 0, j = s - 1; k <= r; ++k)
    if (cmp(b[i], b[j]))
      a[k] = b[i++];
    else
      a[k] = b[j--];
}

    template <typename T, class Compare = Aleph::less<T>> inline
void merge(T * a, const int l, const int m, const int r, 
	   Compare && cmp = Compare())
{
  merge<T, Compare>(a, l, m, r, cmp);
}

/** 
  @brief Sorts an array by the method of mixing.

  @details mergesort(a,l,r) orders the indices array "a" between "l" and "r"
  according to the method for mixing as comparison criteria "Compare".

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  This method has a deterministic performance of \f$O(n \; \lg n)\f$, but 
  a space consumption of \f$O(1)\f$. 

  @param[in,out] a Array to order.
  @param[in] l Lower index.
  @param[in] r Top index.

  @see selection_sort() insertion_sort() quicksort_rec() heapsort()

  @ingroup Algos
*/
    template <typename T, class Compare> inline
void mergesort(T * a, const int l, const int r, Compare & cmp)
{
  if (l >= r) 
    return;

  const int m = (l + r)/2;

  mergesort<T, Compare>(a, l, m, cmp);
  mergesort<T, Compare>(a, m + 1, r, cmp);

  if (cmp(a[m], a[m + 1]))
    return;

  merge<T, Compare>(a, l, m, r, cmp);
}


    template <typename T, class Compare = Aleph::less<T>> inline
void mergesort(T * a, const int l, const int r, Compare && cmp = Compare())
{
  mergesort<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Mix two ordered lists into one.

	@details merge_lists(l1,l2,result) It takes two sorted lists "l1" and "l2"
  and merges them into a single ordered list "result" as
  Compare comparison criteria.

  It does not check whether the l1 and l2 lists are sorted.
  Results will be incorrect if this assumption is not met.

	@param[in,out] l1 Ordered list to merge.
	@param[in,out] l2 Ordered list to merge.
	@param[out] result List where the result will be placed.

  @ingroup Algos
*/
template <class Tlist, class Compare> inline
void merge_lists(Tlist & l1, Tlist & l2, Tlist & result, Compare & cmp)
{
  assert(result.is_empty());

  while (not l1.is_empty() and not l2.is_empty())
    if (cmp(l1.get_first(), l2.get_first()))
      result.append(l1.remove_first());
    else
      result.append(l2.remove_first());

  if (l1.is_empty())
    result.concat_list(l2);
  else
    result.concat_list(l1);

  assert(l1.is_empty() and l2.is_empty());
}

    template <class Tlist, class Compare> inline
void merge_lists(Tlist & l1, Tlist & l2, Tlist & result,
		 Compare && cmp = Compare())
{
  merge_lists<Tlist, Compare>(l1, l2, result, cmp);
}

/** 
  @brief Mix two ordered lists of Dnode type into one.

  @details merge_lists(l1,l2,result) It takes two sorted lists "l1" and "l2"
  and merges them into a single ordered list "result" as
  Compare comparison criteria.

  It does not check whether the l1 and l2 lists are sorted.
  Results will be incorrect if this assumption is not met.

  @param[in,out] l1 Ordered list to merge.
  @param[in,out] l2 Ordered list to merge.
  @param[out] result List where the result will be placed.

  @ingroup Algos
*/
template <class T, class Compare> inline
void merge_lists(Dnode<T> & l1, Dnode<T> & l2, Dnode<T> & result,
		 Compare & cmp)
{
  merge_lists<Dnode<T>, Compare_Dnode<T, Compare>>(l1, l2, result, cmp);
}

     template <class T, class Compare = Aleph::less<T>> inline
void merge_lists(Dnode<T> & l1, Dnode<T> & l2, Dnode<T> & result,
		 Compare && cmp = Compare())
{
  merge_lists<Dnode<T>, Compare_Dnode<T, Compare>>(l1, l2, result, cmp);
}


/** 
  @brief Order a list according to mixing method (mergesort).

  @details Order by mixing method (mergesort) a list
  based on Dlink, by comparison criterion Compare.

  This method has a deterministic performance of \f$O(n \; \lg n)\f$
  and space consumption of \f$O(1)\f$. It is a very good method
  for lists.

  @param[in,out] list List to be sorted.

  @see mergesort(Dnode<T> & list) 

  @ingroup Algos
*/
    template <class Tlist, class Compare> inline 
void mergesort(Tlist & list, Compare & cmp)
{
  if (list.is_unitarian_or_empty()) 
    return;

  Tlist l, r;
  list.split_list(l, r);  /* Dividir en dos listas */

  mergesort <Tlist, Compare> (l, cmp);
  mergesort <Tlist, Compare> (r, cmp);  
  
  merge_lists <Tlist, Compare> (l, r, list, cmp); /* Mezclarlas */
}

    template <class Tlist, class Compare> inline 
void mergesort(Tlist & list, Compare && cmp = Compare())
{
  mergesort<Tlist, Compare>(list, cmp);
}


/** 
  @brief Order a list according to mixing method (mergesort)
  combined with the method of insertion.

  @details Order by mixing method (mergesort) a list
  dynamic (DynList o DynDlist) by comparison criterion Compare.

  This method has a deterministic performance of \f$O(n \; \lg n)\f$
  and space consumption of \f$O(1)\f$. It is a very good method
  for lists.

  @param[in,out] list List to be sorted.

  @see mergesort(Dnode<T> & list) 

  @ingroup Algos
*/
template <template <typename> class Tlist, typename T, class Compare> inline 
void mergeinsertsort(Tlist<T> & list, Compare & cmp, 
		     const size_t lsz = Aleph::Insertion_Threshold)
{
  if (lsz < Aleph::Insertion_Threshold)
    {
      insertion_sort<T, Compare>(list, cmp);
      return;
    }

  if (list.is_unitarian_or_empty()) 
    return;

  Tlist<T> l, r;
  list.split_list(l, r);         /* Dividir en dos listas */

  mergeinsertsort <Tlist, T, Compare> (l, cmp);
  mergeinsertsort <Tlist, T, Compare> (r, cmp);  
  
  merge_lists <Tlist<T>, Compare> (l, r, list, cmp); /* Mezclarlas */
}

template <template <typename> class Tlist, typename T, class Compare> inline 
void mergeinsertsort(Tlist<T> & list, Compare && cmp = Compare())
{
  mergeinsertsort<Tlist, T, Compare>(list, cmp);
}



/** 
 @brief Order a list according to mixing method (mergesort).

 @details Order by mixing method (mergesort) a list
 based on Dnode<T>, by comparison criterion Compare.

 The method uses two type parameters:
 -# T: Type elements having the array.
 -# Compare: Class of comparison.

 This method has a deterministic performance of \f$O(n \; \lg n)\f$
 and space consumption of \f$O(1)\f$. It is a very good method
 for lists.
 
 @param[in,out] list List to be sorted.
 
 @see mergesort(Dlink & list) quicksort(Dnode<T>)
 
 @ingroup Algos
*/
    template <typename T, class Compare> inline
void mergesort(Dnode<T> & list, Compare & cmp)
{
  mergesort <Dnode<T>, Compare_Dnode<T, Compare>> (list, cmp);
}


    template <typename T, class Compare = Aleph::less<T>> inline
void mergesort(Dnode<T> & list, Compare && cmp = Compare())
{
  mergesort<Dnode<T>, Compare_Dnode<T, Compare>>(list, cmp);
}

/** 
  @brief Order a dynamic list according to mixing method (mergesort).

  @details Order by mixing method (mergesort) a list
  dynamic by comparison criterion Compare.

  The method uses two type parameters:
  -# T: the type of items that have the array.
  -# Compare: class of comparison.

  This method has a deterministic performance of \f$O(n \; \lg n)\f$
  and space consumption of \f$O(1)\f$. It is a very good method
  for lists.

  @param[in,out] list List to be sorted.

  @see quicksort(DynDlist<T> & list)

  @ingroup Algos
*/
    template <typename T, class Compare> inline
void mergesort(DynDlist<T> & list, Compare & cmp)
{
  mergesort <Dnode<T>, Compare_Dnode<T, Compare>> (list, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
void mergesort(DynDlist<T> & list, Compare && cmp = Compare())
{
  mergesort<T, Compare>(list, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
void mergesort(DynList<T> & list, Compare & cmp)
{
  mergesort<DynList<T>, Compare>(list, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
void mergesort(DynList<T> & list, Compare && cmp = Compare())
{
  mergesort<DynList<T>, Compare>(list, cmp);
}

    template <typename T, class Compare> inline
int select_pivot(T * a, const int l, const int r, Compare & cmp);

    template <typename T, class Compare = Aleph::less<T>> inline
int select_pivot(T * a, const int l, const int r, Compare && cmp = Compare());  


    template <typename T, class Compare> inline
int partition(T * a, const int l, const int r, Compare & cmp)
{
  const int p = select_pivot <T, Compare> (a, l, r, cmp);
  std::swap(a[p], a[r]);

  T pivot = a[r]; /* Elemento pivot */
  int i = l - 1,  /* Índice primer elemento a la izquierda > que pivot */
      j = r;      /* Índice primer elemento a la derecha < que pivot */
  while (true)
    {
    
    /** Progress while a[i] < a[pivot] */
      while (cmp(a[++i], pivot)) { /* Nothing */ }

    /** Progress while a[pivot]< a[j] */
      while (cmp(pivot, a[--j])) 

    /** Is the left edge is reached?
    If the answer is YES, then you have to finish the iteration */
	if (j == l) 
	  break; 

      if (i >= j) 
        break;

        /* En este punto hay una inversión a[i] > a[pivot] > a[j] */
      std::swap(a[i], a[j]); /* Eliminar la inversión */
    }

  std::swap(a[i], a[r]);

  return i;
}

    template <typename T, class Compare> inline
int partition(T * a, const int l, const int r, Compare && cmp = Compare())
{
  return partition<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Sorts an array by the method quicksort.

  @details quicksort_rec(a,l,r) orders the indices array "a" between "l" and "r"
  according to the method "quicksort" as comparison criteria "Compare".

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  This method has an expected performance of \f$O(n \; \lg n)\f$
  and it is considered the method of faster system.

  This version of quicksort can occupy space \f$O(n)\f$.
  Use quicksort_rec_min() if you want to ensure consumer
  maximum space \f$O(\lg n)\f$ at the expense of a little more
  constant time.

  Quicksort is a probabilistic method. In a very bad case
  -very bad luck- can degrade \f$O(n^2)\f$. 
  To alleviate the extent of luck, bad cases, 
  use quicksort_insertion() running heuristics to
  mitigate the bad cases and invokes the method of insertion
  partitions small settlement.

  @param[in,out] Array to order.
  @param[in] l Lower index.
  @param[in] r Top index.

  @see selection_sort() insertion_sort() mergesort() heapsort()
  @see quicksort_rec_min() quicksort_insertion() quicksort()

  @ingroup Algos
*/
    template <typename T, class Compare> inline
void quicksort_rec(T * a, const int l, const int r, Compare & cmp)
{
  if (l >= r) 
    return;

  const int pivot = partition <T, Compare> (a, l, r, cmp);

  quicksort_rec <T, Compare> (a, l, pivot - 1, cmp);
  quicksort_rec <T, Compare> (a, pivot + 1, r, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_rec(T * a, const int l, const int r, 
		   Compare && cmp = Compare())
{
  quicksort_rec<T, Compare>(a, l, r, cmp);
}

    template <typename T, class Compare> inline
void quicksort_no_tail(T * a, int l, int r, 
		       Compare & cmp)
{
 start:
  if (l >= r) 
    return;

  const int pivot = partition <T, Compare> (a, l, r, cmp);

  quicksort_no_tail <T, Compare> (a, l, pivot - 1, cmp);

  l = pivot + 1;
  goto start;
}

    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_no_tail(T * a, int l, int r, 
		       Compare && cmp = Compare())
{
  quicksort_no_tail<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Orders an array under quicksort method with minimal
  space consumption.

  @details The purely recursive "quicksort" can consume proportional stack
  the array size to order. To avoid this problem,
  at the expense of a slight cost of performance, quicksort_rec_min()
  first ever order from the smallest part. For others,
  the same considerations as for quicksort_rec() apply.

  @param[in,out] a Array to order.
  @param[in] l Index initial where is ordered.
  @param[in] r Index final where is ordered.

  @see quicksort_rec() quicksort() quicksort_insertion()

  @ingroup Algos
*/
    template <typename T, class Compare> inline
void quicksort_rec_min(T * a, const int l, const int r, Compare & cmp)
{
  if (r <= l) 
    return;  

  const int pivot = partition<T, Compare>(a, l, r, cmp);

    /* ¿Cual es la partición más pequeña? */
  if (pivot - l < r - pivot)
    {     
    /* Partición izquierda más pequeña */
      quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
      quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
    }
  else
    {     
    /* Partición derecha más pequeña */
      quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
      quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
    }
} 


    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_rec_min(T * a, const int l, const int r, 
		       Compare && cmp = Compare())
{
  quicksort_rec_min<T, Compare>(a, l, r, cmp);  
}


    template <typename T, class Compare> inline
int select_pivot(T * a, const int l, const int r, Compare & cmp)
{
  assert(l <= r);

  if (r - l <= 2) 
    return r;

  const int m = (r + l) / 2; /* Índice del centro */
  const int p = cmp(a[l], a[m]) ? m : l; /* p=max(a[l],a[m]) */

  return cmp (a[r], a[m]) ? r : p; /* retornar min(a[r], a[m]) */
}


    template <typename T, class Compare> inline
int select_pivot(T * a, const int l, const int r, Compare && cmp)
{
  return select_pivot<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Sorts an array by the quicksort method without recursion.

  @details quicksort_rec(a,l,r) orders the indices array "a" between "l" and "r"
  according to the method "quicksort" as comparison criteria "Compare".

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  This method has an expected performance of \f$O(n \; \lg n)\f$
  and it is considered the method of faster system.

  This version of quicksort can occupy space \f$O(n)\f$.
  Use quicksort_rec_min() if you want to ensure consumer
  maximum space \f$O(\lg n)\f$ at the expense of a little more
  constant time.

  Quicksort is a probabilistic method. In a very bad case
  -very bad luck- can degrade to 
  \f$O(n^2)\f$. To alleviate the extent of luck, bad
  cases, use quicksort_insertion() running heuristics to
  mitigate the bad cases and invokes the method of insertion
  partitions small settlement.

  @param[in,out] a Array to order.
  @param[in] l Lower index.
  @param[in] r Top index.

  @see selection_sort() insertion_sort() mergesort() heapsort()
  @see quicksort_rec_min() quicksort_insertion() quicksort_rec()

  @ingroup Algos
*/
    template <typename T, class Compare> inline
void quicksort(T * a, const int l, const int r, Compare & cmp)
{
  if (r - l < Quicksort_Threshold) 
    {
      insertion_sort(a, l, r, cmp);
      return;
    }

  typedef typename std::pair<int, int> Partition; 
  FixedStack<Partition> stack(64);
  stack.push(Partition(l, r)); /* Todo el arreglo como partición inicial */

  while (stack.size() > 0)
    {
      const Partition p = stack.pop();
      const long diff = p.second - p.first;
      
      if (diff <= 1)
	continue;

      if (diff < Quicksort_Threshold)
	{
	  insertion_sort(a, p.first, p.second, cmp);
	  continue;
	}

      const int pivot = partition <T, Compare>(a, p.first, p.second, cmp);

    /* ¿Cuál más pequeña? */
      if (pivot - p.first < p.second - pivot) 
	{     
    /* Partición izquierda más pequeña */
	  stack.push(Partition(pivot + 1, p.second));
	  stack.push(Partition(p.first, pivot - 1));
	}
      else
	{     
    /* Partición derecha más pequeña */
	  stack.push(Partition(p.first, pivot - 1));
	  stack.push(Partition(pivot + 1, p.second));
	}
    }
}

    template <typename T, class Compare = Aleph::less<T>> inline
void quicksort(T * a, const int l, const int r, Compare && cmp = Compare())
{
  quicksort<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Sorts a linked list by the quicksort method.

	@details quicksort(list) sorts a list by the method
  quicksort discretion of comparison Compare.

	This method has an expected performance of \f$O(n \; \lg n)\f$
  and it is considered the method of faster system.

	@param[in,out] list Sort list.

	@ingroup Algos
*/
  template <class Compare> 
void quicksort(Dlink & list, Compare & cmp) 
{
  if (list.is_unitarian_or_empty()) 
    return;

  Dlink * pivot = list.remove_next();
  Dlink smaller, bigger; /* Listas de menores y mayores que pivot */

  while (not list.is_empty()) 
    {
      Dlink * p = list.remove_next();
      if (cmp(p, pivot))
	smaller.append(p);
      else
	bigger.append(p);
    }

  quicksort <Compare> (bigger, cmp);  
  quicksort <Compare> (smaller, cmp);

  list.concat_list(&smaller); /* Restaurar listas ordenadas en list */
  list.append(pivot);
  list.concat_list(&bigger);
} 

    template <class Compare> 
void quicksort(Dlink & list, Compare && cmp = Compare()) 
{
  quicksort<Compare>(list, cmp);
}

/**
  @brief Sorts a list by the quicksort method.

	@details quicksort(list) sorts a list by the method
  quicksort discretion of comparison Compare.

	The method uses two type parameters:
	-# T: Type elements having the array.
	-# Compare: Class of comparison.

	This method has an expected performance of \f$O(n \; \lg n)\f$
  and it is considered the method of faster system.

	This primitive can be used to lists Dlist and DynDlist type.

	@param[in,out] list Sort list.

	@see Dnode Dlist DynDlist

	@ingroup Algos
*/
template <typename T, class Compare>
void quicksort(Dnode<T> & list, Compare & cmp)
{
  quicksort <Compare_Dnode<T, Compare>> (list, cmp);
}


    template <typename T, class Compare = Aleph::less<T>>
void quicksort(Dnode<T> & list, Compare && cmp = Compare())
{
  quicksort<T, Compare>(list, cmp);
}


template <typename T, class Compare> 
void quicksort(HTList & list, Compare & cmp) 
{
  if (list.is_unitarian_or_empty()) 
    return;

  Snodenc<T> * pivot = (Snodenc<T> *) list.remove_first();
  HTList smaller, bigger; /* Listas de menores y mayores que pivot */

  while (not list.is_empty()) 
    {
      Snodenc<T> * p = (Snodenc<T> *) list.remove_first();
      if (cmp(p->get_data(), pivot->get_data()))
	smaller.append(p);
      else
	bigger.append(p);
    }

  quicksort <T, Compare> (bigger, cmp);  
  quicksort <T, Compare> (smaller, cmp);

  list.concat_list(smaller); /* Restaurar listas ordenadas en list */
  list.append(pivot);
  list.concat_list(bigger);
} 


    template <typename T, class Compare = Aleph::less<T>>
void quicksort(DynList<T> & list, Compare && cmp = Compare())
{
  quicksort<T, Compare>((HTList&) list, cmp);
}

/** 
  @brief Sorts an array by the improved method quicksort.

	@details quicksort_insertion(a,l,r) ordered array between the
  index "l" and "r" as the quicksort method according to criteria
  Compare comparison.

	The procedure combines several techniques to accelerate
  system while avoiding degradation by bad cases.

	The bad cases are treated by selectively pivot
  consisting of the median between "l", "r" and the center of the partition.

	To ensure a maximum battery consumption \f$O(\lg n)\f$,
  the method always proceeds recursively to sort the partition
  smaller.

	When the partition reaches a size less than or equal to
  global Aleph::Insertion_Threshold constant, then
  the partition is ordered by the insertion method, which
  it is faster than quicksort for small sizes.

	 The method uses two type parameters:
	-# T: Type elements having the array.
	-# Compare: Class of comparison.

	This method has an expected performance of \f$O(n \; \lg n)\f$
  and it is considered the method of faster system.

	@param[in,out] a Array to order.
	@param[in] l Lower index.
	@param[in] r Top index.

	@see insertion_sort() mergesort() heapsort()
	@see quicksort_rec_min() quicksort_insertion() quicksort()

	@ingroup Algos
*/
 template <typename T, class Compare> inline
void quicksort_insertion(T * a, const int l, const int r, Compare & cmp)
{
  if (r <= l) 
    return;

  const int pivot = partition<T, Compare>(a, l, r, cmp);

  const int l_size = pivot - l; /* Tamaño partición izquierda */
  const int r_size = r - pivot; /* Tamaño partición derecha */
  bool left_done  = false; /* True si partición izq está ordenada */
  bool right_done = false; /* True si partición der está ordenada */

  if (l_size <= Aleph::Insertion_Threshold) 
    {     
    /* Ordene partición izq por inserción */
      insertion_sort<T, Compare>(a, l, pivot - 1, cmp);  
      left_done = true;
    }

  if (r_size <= Aleph::Insertion_Threshold) 
    {     
    /* Ordene partición der  por inserción */
      insertion_sort<T, Compare>(a, pivot + 1, r, cmp);
      right_done = true;
    }

  if (left_done and right_done) 
    return; /* Ambas particiones ordenadas por inserción */

    /** Partition "izq" orderly by insertion?
    If the answer is YES only remaining order
    right partition recursively */
  if (left_done) 
    {
      quicksort_insertion<T, Compare>(a, pivot + 1, r, cmp);
      return;
    }

    /** Partition "der" orderly by insertion?
    If the answer is YES only remaining order
    left partition recursively */
  if (right_done)
    { 
      quicksort_insertion<T, Compare>(a, l, pivot - 1, cmp);
      return;
    }

    /* Aquí, ambas particiones no fueron ordenadas por inserción */
  if (l_size < r_size) /* Ordenar primero partición más pequeña */
    {     
    /* Partición izquierda más pequeña */
      quicksort_insertion <T, Compare> (a, l, pivot - 1, cmp);
      quicksort_insertion <T, Compare> (a, pivot + 1, r, cmp);
    }
  else
    {     
    /* Partición derecha más pequeña */
      quicksort_insertion <T, Compare> (a, pivot + 1, r, cmp);
      quicksort_insertion <T, Compare> (a, l, pivot - 1, cmp);
    }
}    

     template <typename T, class Compare = Aleph::less<T>> inline
void quicksort_insertion(T * a, const int l, const int r,
			 Compare && cmp = Compare())
{
  quicksort_insertion<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Random search for an item in an array.

  @details random_search(a,x,l,r) it uses the partitioning algorithm of 
  quicksort to search for the "x" element in the array to understand
  the boundaries between "l" and "r".

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  The method has an expected complexity of
  \f$O(n \; \lg n)\f$, which is greater than the mere search
  sequentially, but with the addition that after the
  search the array is partially ordered and, since
  the selection of the pivot is the median between "l", "r" and the center
  the array, the search tends to be linear as
  more random searches are done.


  @param[in,out] a array to search; It is partially modified
  after the search.
  @param[in] x Element to search.
  @param[in] l Lower index of start of the search.
  @param[in] r Higher index of term of the search.

  @return The index of input of array contentiva of
  value "x" if it is within the array;
  Not_Found (typically -1) otherwise.

  @ingroup Algos

  @see sequential_search() binary_search_rec()
*/
     template <typename T, class Compare> inline
int random_search(T * a, const T & x, const int l, const int r,
		  Compare & cmp)
{
  if (l > r)
    return Not_Found;

  const int pivot = partition<T, Compare>(a, l, r, cmp);

  if (cmp(x, a[pivot]))
    return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
  else if (cmp(a[pivot], x))
    return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

  return pivot; /* Elemento encontrado en el índice x */
}

     template <typename T, class Compare = Aleph::less<T>> inline
int random_search(T * a, const T & x, const int l, const int r,
		  Compare && cmp = Compare())
{
  return random_search<T, Compare>(a, x, l, r, cmp);
}


    template <typename T, class Compare> inline
int random_search(DynArray<T> & a, const T & x, const int l, const int r,
		  Compare & cmp)
{
  if (l > r)
    return Not_Found;

  const int pivot = partition<T, Compare>(a, l, r, cmp);

  if (cmp(x, a(pivot)))
    return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
  else if (cmp(a(pivot), x))
    return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

  return pivot; /* Elemento encontrado en el índice x */
}

template <typename T, class Compare = Aleph::less<T>> inline
int random_search(DynArray<T> & a, const T & x, const int l, const int r,
		  Compare && cmp = Compare())
{
  return random_search<T, Compare>(a, x, l, r, cmp);
}

/** 
  @brief Random search for an item in a dlink list.

  @details dlink_random_search(list,x) it uses the partitioning algorithm 
  of quicksort to search for the "x" element 
  in the "list" list.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  The method has an expected complexity of
  \f$O(n \; \lg n)\f$, which is greater than the mere pursuit
  sequentially, but with the addition that after the
  search the list is partially ordered.

  @param[in,out] list the list in which the search is performed;
  it is partially modified after the search.
  @param[in] x element to search.

  @return A pointer to contentivo node of value "x" if
  it is within the list; NULL otherwise.

  @see random_search(Dlink&list,const T&x)

  @ingroup Algos
*/
    template <typename T, class Compare> inline
Dnode<T> * dlink_random_search(Dlink & list, const T & x, Compare & cmp)
{
  if (list.is_empty()) 
    return NULL;

  Dnode<T>   item(x);
  Dnode<T> * item_ptr = &item; /* Puntero a celda que contiene a x */

  Dlink smaller; /* Lista de los menores que pivot */
  Dlink bigger;  /* Lista de los mayores que pivot */

  Dnode<T> * pivot = static_cast<Dnode<T>*>(list.remove_next());
  
  while (not list.is_empty()) 
    {
      Dlink * p = list.remove_next();
      if (cmp(p, pivot))
	smaller.append(p);
      else
	bigger.append(p);
    }

  Dnode<T> * ret_val = NULL;
  if (cmp(item_ptr, pivot))
    ret_val = dlink_random_search <T, Compare> (smaller, x, cmp);
  else if (cmp(pivot, item_ptr))
    ret_val = dlink_random_search <T, Compare> (bigger, x, cmp);
  else
    ret_val = pivot;

  assert(list.is_empty());

  list.swap(&smaller); 
  list.append(pivot);  
  list.concat_list(&bigger);

  return ret_val;
}

    template <typename T, class Compare> inline
Dnode<T> * dlink_random_search(Dlink & list, const T & x,
			       Compare && cmp = Compare())
{
  return dlink_random_search<T, Compare>(list, x, cmp);
}

/** 
  @brief Random search for an item in a Dnode<T> list.

	@details random_search(list,x) it uses the partitioning algorithm of
  quicksort to search for the "x" element in the "list" list.

	The method uses two type parameters:
	-# T: Type elements having the array.
	-# Compare: Class of comparison.

	The method has an expected complexity of
  \f$O(n \; \lg n)\f$, which is greater than the mere pursuit
  sequentially, but with the addition that after the
  search the list is partially ordered.

  A specialization use the "less than" relationship as
  criteria for comparison and saves the write of that
  class.

	@param[in,out] list List in which the search is performed;
  it is partially modified after the search.
	@param[in] x Element to search.
	
  @return A pointer to contentivo node of value "x" if
  it is within the list; NULL otherwise.

	@see dlink_random_search

	@ingroup Algos
*/
template <typename T, class Compare>
Dnode<T> * random_search(Dlink & list, const T & x, Compare & cmp)
{
  return dlink_random_search <T, Compare_Dnode<T, Compare>> (list, x, cmp);
}

    template <typename T, class Compare = Aleph::less<T>>
Dnode<T> * random_search(Dlink & list, const T & x, Compare && cmp = Compare())
{
  return random_search<T, Compare>(list, x, cmp);
}


/** 
  @brief Random search for an item in a dynamic list.

  @details random_search(list,x) it uses the partitioning algorithm of
  quicksort to search for the "x" element in the "list" dynamic list.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare:Class of comparison.

  The method has an expected complexity of
  \f$O(n \; \lg n)\f$, which is greater than the mere pursuit
  sequentially, but with the addition that after the
  search the list is partially ordered.

  A specialization use the "less than" relationship as
  criteria for comparison and saves the write of that
  class.

  @param[in,out] list Lista dinámica en la cual se realiza 
  la búsqueda; es parcialmente modificada luego de la búsqueda.
  @param[in] x Element to search.

  @return puntero x dentro de la lista si ésta
  se encuentra dentro de ella; NULL de lo contrario.

  @see DynDlist 

  @ingroup Algos
*/
    template <typename T, class Compare> inline
T * random_search(DynDlist<T> & list, const T & x, Compare & cmp)
{
  Dnode<T> * p = 
    dlink_random_search <T, Compare_Dnode<T, Compare>> (list, x, cmp);

  return p == NULL ? NULL : &(p->get_data());
}

    template <typename T, class Compare = Aleph::less<T>> inline
T * random_search(DynDlist<T> & list, const T & x, Compare && cmp = Compare())
{
  return random_search<T, Compare>(list, x, cmp);
}

    template <typename T, class Compare> static inline
const T & __random_select(T * a, const int i, 
			  const int l, const int r,
			  Compare & cmp)
{
  const int pivot = partition<T, Compare>(a, l, r, cmp);
  if (i == pivot) 
    return a[i];

  if (i < pivot) /* ¿Está en partición izquierda? */
    return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
  else
    return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
}

    template <typename T, class Compare> static inline
const T & __random_select(T * a, const int i, 
			  const int l, const int r,
			  Compare && cmp = Compare())
{
  return __random_select<T, Compare>(a, i, l, r, cmp);
}


/** 
  @brief Select an item pivot of a DynArray as the median between
  the ends and the center.

  @ingrouop Algos
*/
    template <typename T, class Compare> inline
int select_pivot_op(DynArray<T> & a, const int l, const int r, Compare & cmp)
{
  assert(l <= r);

  if (r - l <= 5)
    return r;

  const int m = (l + r)/2; /* Elemento central */

    /* Accede las entradas del arreglo una sola vez */
  T & la = a.access(l); 
  T & ra = a.access(r);
  T & ma = a.access(m);
  
  const T * med_ptr = Aleph::median<T, Compare>(la, ma, ra, cmp);
  
  if (med_ptr == &la)
    return l;

  if (med_ptr == &ma)
    return m;

  assert(med_ptr == &ra);

  return r;
}

    template <typename T, class Compare> inline
int select_pivot_op(DynArray<T> & a, const int l, const int r,
		    Compare && cmp = Compare())
{
  return select_pivot_op<T, Compare>(a, l, r, cmp);
}


    template <typename T, class Compare> inline
int partition_op(DynArray<T> & a, int l, int r, Compare & cmp)
{
  if (l == r)
    return l;

  int i = l - 1, 
      j = r;

  T & pivot       = a.access(r);
  int pivot_idx   = select_pivot_op<T, Compare>(a, l, r, cmp);
  T & pivot_value = a.access(pivot_idx);
  std::swap(pivot, pivot_value);

  while (true)
    {
      while (cmp(a.access(++i), pivot)) ; /* Nada */

      while (cmp(pivot, a.access(--j)))
	if (j == l)
	  break;

      if (i >= j)
	break;

      std::swap(a.access(i), a.access(j));
    }

  std::swap(a.access(i), a.access(r));

  return i;
}

    template <typename T, class Compare> inline
int partition_op(DynArray<T> & a, int l, int r, Compare && cmp = Compare())
{
  return partition_op<T, Compare>(a, l, r, cmp);
}


    template <typename T, class Compare> static inline
const T & __random_select(DynArray<T> & a, const int i, 
			  const int l, const int r,
			  Compare & cmp)
{
  assert(i >= l and i <= r);

  const int pivot = partition_op<T, Compare>(a, l, r, cmp);
  if (i == pivot) 
    return a(i);

  if (i < pivot) /* ¿Está en partición izquierda? */
    return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
  else
    return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
}

    template <typename T, class Compare> static inline
const T & __random_select(DynArray<T> & a, const int i, 
			  const int l, const int r,
			  Compare && cmp = Compare())
{
  return __random_select<T, Compare>(a, i, l, r, cmp);
}


template <typename T, class Compare> 
const T & random_select(DynArray<T> & a, const int i, Compare & cmp)
{
  const int n = a.size() - 1;
  if (i > n)
    throw std::out_of_range("index out of range");

  return __random_select<T, Compare>(a, i, 0, n, cmp);
}

  template <typename T, class Compare = Aleph::less<T>> 
const T & random_select(DynArray<T> & a, const int i, 
			Compare && cmp = Compare())
{
  return random_select<T, Compare>(a, i, cmp);
}


/** 
  @brief Random selection of the ith element of an array.

  @details random_select(a,i,n) returns the i-th smallest element
  content in the "a" array with dimension n.

  The routine uses the partition for quicksort to
  look for the "i" in time \f$O(n \; \lg n)\f$, which
  it is a substantially time better than the search
  sequential (\f$O(n^2)\f$).

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] a Array where it seeks the i-th element.
  The array is semi-ordered through successive
  partitions have been made.
  @param[in] i Access desired position.
  @param[in] n Dimension of the array.

  @throw out_of_range If "i" is greater than or equal to n.

  @ingroup Algos
*/
  template <typename T, class Compare> 
const T & random_select(T * a, const int i, const int n, Compare & cmp)
{
  if (i >= n)
    throw std::out_of_range("index out of range");

  return __random_select<T, Compare>(a, i, 0, n - 1, cmp);
}

  template <typename T, class Compare = Aleph::less<T>> 
const T & random_select(T * a, const int i, const int n, 
			Compare && cmp = Compare())
{
  return random_select<T, Compare>(a, i, n, cmp);
}

/** 
  @brief Random selection of the i-th element of a list based
  on Dlink.

  @details random_select(list,i) returns the i-th smallest element
  contained in the "list" list at the discretion of order
  determined by the Compare class.

  The routine uses the partition for quicksort to
  look for the "i" in time \f$O(n \; \lg n)\f$, which
  it is a substantially time better than the search
  sequential (\f$O(n^2)\f$).

  @param[in,out] list List where you look for the i-th element.
  The list is semi-ordered through successive
  partitions have been made.
  @param[in] i Access desired position.

  @return pointer to Dlink corresponding to "i" position in
  of the list; 

  @ingroup Algos
*/
    template <class Compare>
Dlink * dlink_random_select(Dlink & list, const size_t i, Compare & cmp)
{
  if (list.is_empty()) 
    return NULL;

  Dlink smaller; /* Lista de los menores que pivot */
  Dlink bigger;  /* Lista de los mayores que pivot */

  size_t smaller_count = 0, /* Cantidad de elementos de smaller */
         bigger_count  = 0; /* Cantidad de elementos de bigger */

  Dlink * pivot = list.remove_next();
  
  while (not list.is_empty()) 
    {
      Dlink * p = list.remove_next();
      if (cmp(p, pivot)) /* ¿p < pivot? */
	{  smaller.append(p); ++smaller_count;  }
      else
	{  bigger.append(p); ++bigger_count;  }
    }

  if (i >= smaller_count + bigger_count + 1) 
    throw std::out_of_range("index of selection greater than list's size");

  Dlink * ret_val = NULL;
  if (i == smaller_count)
    ret_val = pivot;
  else if (i < smaller_count)
    ret_val = dlink_random_select<Compare>(smaller, i, cmp);
  else
    ret_val = dlink_random_select<Compare>(bigger, i - (smaller_count+1), cmp);

  list.concat_list(&smaller);
  list.append(pivot);
  list.concat_list(&bigger);

  return ret_val;
}


    template <class Compare>
Dlink * dlink_random_select(Dlink & list, const size_t i, 
			    Compare && cmp = Compare())
{
  return dlink_random_select<Compare>(list, i, cmp);
}

/** 
  @brief Random selection of the i-th element of a list based
  on Dlink.

  @details random_select(list,i) returns the i-th smallest element
  contained in the "list" list.

  The routine uses the partition for quicksort to
  look for the "i" in time \f$O(n \; \lg n)\f$, which
  it is a substantially time better than the search
  sequential (\f$O(n^2)\f$).

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] list List where you look for the i-th element.
  The list is semi-ordered through successive
  partitions have been made.
  @param[in] i Access desired position.

  @return pointer to Dlink corresponding to "i" position in
  of the list; NULL if "i" is out of range.

  @ingroup Algos
*/
    template <typename T, class Compare>
Dnode<T> * random_select(Dlink & list, const size_t i, Compare & cmp)
{
  return (Dnode<T>*) 
    dlink_random_select<Compare_Dnode<T, Compare>> (list, i, cmp);
}


    template <typename T, class Compare = Aleph::less<T>>
Dnode<T> * random_select(Dlink & list, const size_t i, 
			 Compare && cmp = Compare())
{
  return random_select<T, Compare> (list, i, cmp);
}

/** 
  @brief Random selection of the i-th element of a dynamic list.

  @details random_select(list,i) returns the i-th smallest element
  content in the "list" dynamic list.

  The routine uses the partition for quicksort to
  look for the "i" in time \f$O(n \; \lg n)\f$, which
  it is a substantially time better than the search
  sequential (\f$O(n^2)\f$).

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] list Dynamic list where you seek the
  ith element. The list is semi-ordered through
  of successive
  partitions have been made.
  @param[in] i Access desired position.

  @return A pointer to the element of the corresponding dynamic list
  to the "i" position in the list; NULL if "i" is out of range.

  @ingroup Algos
*/
    template <typename T, class Compare>
T * random_select(DynDlist<T> & list, const size_t i, Compare & cmp)
{
  Dlink * link = dlink_random_select <Compare_Dnode<T, Compare>> (list, i, cmp);

  Dnode<T> * p = static_cast<Dnode<T>*>(link);

  return p != NULL ? &(p->get_data()) : NULL;
}

    template <typename T, class Compare = Aleph::less<T>>
T * random_select(DynDlist<T> & list, const size_t i, 
		  Compare && cmp = Compare())
{
  return random_select<T, Compare>(list, i, cmp);
}
  
/** 
  @brief Order a dynamic array by the method of selection.

  @details selection_sort(a) the selection method used to order
  the dynamic array of n elements.

  The selection method has a performance of \f$O(n^2)\f$.
  Because of its simplicity of implementation, its constant cost
  it is low, so it is a good method to arrays of 
  very small dimension.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] a The array to the order.

  @see insertion_sort() quicksort_rec() mergesort() heapsort()

  @ingroup Algos
*/
    template <class T, class Compare> inline
void selection_sort(DynArray<T> & a, Compare & cmp)
{
  const int n = a.size();

  for (int i = 0; i < n - 1; i++)
    {
      int min = i;

      for (int j = i + 1; j < n; j++)
	if (cmp(a.access(j), a.access(min)))
	  min = j;

      if (cmp(a.access(min), a.access(i)))
	std::swap(a.access(min), a.access(i));
    }
}

    template <class T, class Compare = Aleph::less<T>> inline
void selection_sort(DynArray<T> & a, Compare && cmp = Compare())
{
  selection_sort<T, Compare>(a, cmp);
}

/** 
  @brief Order a dynamic array by the method of the bubble.

  @details bubble_sort(a) employs the bubble method to order
  the dynamic array of n elements

  The bubble method has a performance
  \f$O(n^2)\f$. Possibly the most inefficient method of all
  existing.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] a Array to order.

  @see insertion_sort() quicksort_rec() mergesort() heapsort()

  @ingroup Algos
*/
    template <class T, class Compare> inline
void bubble_sort(DynArray<T> & a, Compare & cmp)
{
  const int n = a.size();

  for (int i = 0; i < n - 1; i++)
    for (int j = n - 1; j > i; j--)
      if (cmp(a.access(j), a.access(j - 1)))
	std::swap(a.access(j - 1), a.access(j));
}

    template <class T, class Compare = Aleph::less<T>> inline
void bubble_sort(DynArray<T> & a, Compare && cmp = Compare())
{
  bubble_sort<T, Compare>(a, cmp);
}

/** 
  @brief Order a dynamic array by the method of insertion.

  @details insertion_sort(a) the selection method used to order
  dynamic arrangement "a".

  The insertion method has a performance of \f$O(n^2)\f$. 
  It is a simple method, which consumes little time constant.
  On average accounts for half of the exchanges that the method
  selection. Your time tends to be linear if the array is
  semi-ordered. It is a good method for small arrays and
  partitions made by methods above but with
  larger time constants.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] a Array to order.
  @param[in] l The left index of the array part that you want to order
  @param[in] r The right index of the array part that you want to order

  @see selection_sort() quicksort_rec() mergesort() heapsort()

  @ingroup Algos
*/
    template <class T, class Compare> inline
void insertion_sort(DynArray<T> & a, int l, int r, Compare & cmp)
{
  for (int i = l + 1; i <= r; i++)
    {
      T tmp = a.access(i);
      int j = i;
      for (/* Nothing */; j > 0 and cmp(tmp, a.access(j - 1)); --j)
	a.access(j) = a.access(j - 1);

      a.access(j) = tmp;
    }
}

    template <class T, class Compare = Aleph::less<T>> inline
void insertion_sort(DynArray<T> & a, int l, int r, Compare && cmp = Compare())
{
  insertion_sort<T, Compare>(a, l, r, cmp);
}

/**
  @overload insertion_sort()
  
  @ingroup Algos
*/
    template <class T, class Compare> inline
void insertion_sort(DynArray<T> & a, Compare & cmp)
{
  insertion_sort<T, Compare> (a, 0, a.size() - 1, cmp);
}

    template <class T, class Compare = Aleph::less<T>> inline
void insertion_sort(DynArray<T> & a, Compare && cmp = Compare())
{
  insertion_sort<T, Compare> (a, cmp);
}

/** 
  @brief Order a dynamic array by the method of shell.

  @details shellsort(a) the selection method used to order
  dynamic arrangement "a".

  The shell method tends to a performance of \f$O(n^2)\f$, but
  practice is considerably less.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] a Array to order.

  @see selection_sort() quicksort_rec() mergesort() heapsort()

  @ingroup Algos
*/
    template <class T, class Compare> inline
void shellsort(DynArray<T> & a, Compare & cmp)
{
  const int n = a.size();
  int incs[16] = { 1391376, 463792, 198768, 86961, 33936, 13776, 
		   4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1 }; 

  for (int k = 0; k < 16; k++) 
    { 
      const int h = incs[k]; 
      for (int i = h; i < n; i++) 
      { 
	T tmp = a.access(i); 
	int j = i;

	while (j >= h and cmp(tmp, a.access(j - h)))
	  {
	    a.access(j) = a.access(j - h);
	    j -= h;
	  } 

	a.access(j) = tmp;
      } 
    } 
}

    template <class T, class Compare = Aleph::less<T>> inline
void shellsort(DynArray<T> & a, Compare && cmp = Compare())
{
 shellsort<T, Compare>(a, cmp);
}
    inline static 
int back_index(const int & i) { return i - 1; }


    template <class T, class Compare> inline
void sift_up(DynArray<T> & table, const size_t n, Compare & cmp) 
{ 
  int p;
  for (int i = n; i > 1; i = p) 
    {
      p = i >> 1;     /* c = i/2 */

      if (cmp(table.access(back_index(p)), table.access(back_index(i))))
	return;

      std::swap(table.access(back_index(p)), table.access(back_index(i)));
    }
}


    template <class T, class Compare = Aleph::less<T>> inline
void sift_up(DynArray<T> & table, const size_t n, Compare && cmp = Compare()) 
{
  sift_up<T, Compare>(table, n, cmp);
}

    template <class T, class Compare> inline
void sift_down(DynArray<T> & table, const size_t n, Compare & cmp) 
{
  unsigned i = 1;

  while (true)
    {
      int c = i << 1; /* c = 2*i */

      if (c > n) 
	return;

      if (c + 1 <= n)
	if (cmp(table.access(back_index(c + 1)), table.access(back_index(c))))
	  c++;

      if (cmp(table.access(back_index(i)), table.access(back_index(c))))
	return; 

      std::swap(table.access(back_index(c)), table.access(back_index(i)));
      i = c;
    }
}

    template <class T, class Compare = Aleph::less<T>> inline
void sift_down(DynArray<T> & table, const size_t n, 
	      Compare && cmp = Compare()) 
{
  sift_down<T, Compare>(table, n, cmp);
}

    template <class T, class Compare>
class Negate_Compare
{
  Compare & cmp;

public:
  Negate_Compare(Compare & _cmp) : cmp(_cmp) { /* Empty */ }

  Negate_Compare(Compare && _cmp = Compare()) : cmp(_cmp) { /* Empty */ }

  bool operator () (const T & e1, const T & e2)
  {
    return not cmp(e1, e2);
  }
};

/** 
  @brief Order a dynamic array by heapsort method.

  @details heapsort(a) uses the method of "heapsort" to sort
  the array "a" of n elements.

  The heapsort has a guaranteed performance of \f$O(n \; \lg n)\f$
  and it is stable.

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  @param[in,out] a Array to order.

  @see insertion_sort() quicksort_rec() mergesort() 
  @see selection_sort() faster_heapsort()

  @ingroup Algos
*/
    template <class T, class Compare> inline
void heapsort(DynArray<T> & a, Compare & cmp)
{
  const int n = a.size();

  int i;
  for (i = 2; i <= n; i++)
    sift_up<T, Negate_Compare<T, Compare>>(a, i, 
                                Negate_Compare<T, Compare>(cmp));

  for (i = n; i >= 2; i--)
    {
      std::swap(a.access(0), a.access(i - 1));

      sift_down<T, Negate_Compare<T, Compare>>(a, i - 1,
                                 Negate_Compare<T, Compare>(cmp));
    }
}

    template <class T, class Compare> inline
void heapsort(DynArray<T> & a, Compare && cmp = Compare())
{
  heapsort<T, Compare>(a, cmp);
}

    template <class T, class Compare> inline
int partition(DynArray<T> & a, int l, int r, Compare & cmp)
{
  if (l == r)
    return l;

  int i = l - 1, 
      j = r;
  const T & pivot = a.access(r);

  while (true)
    {
      while (cmp(a.access(++i), pivot)) { /* Más nada */ }

      while (cmp(pivot, a.access(--j)))
	if (j == l)
	  break;

      if (i >= j)
	break;
      
      std::swap(a.access(i), a.access(j));
    }

    std::swap(a.access(i), a.access(r));

  return i;
}


    template <class T, class Compare> inline
int partition(DynArray<T> & a, const int l, const int r,
	      Compare && cmp = Compare())
{
  return partition<T, Compare>(a, l, r, cmp);
}


    template <class T, class Compare> inline 
void quicksort_rec(DynArray<T> & a, const int l, const int r, Compare & cmp)
{
  if (r <= l) 
    return;

  int i = partition<T, Compare>(a, l, r, cmp);

  if (i - l < r - i)
    {
      quicksort_rec<T, Compare>(a, l, i - 1, cmp);
      quicksort_rec<T, Compare>(a, i + 1, r, cmp);
    }
  else
    {
      quicksort_rec<T, Compare>(a, i + 1, r, cmp);
      quicksort_rec<T, Compare>(a, l, i - 1, cmp);
    }
}


    template <class T, class Compare = Aleph::less<T>> inline 
void quicksort_rec(DynArray<T> & a, const int l, const int r,
		   Compare && cmp = Compare())
{
  quicksort_rec<T, Compare>(a, l, r, cmp);
}

# define push2(stack, a, b)  stack.push(b); stack.push(a);

/** 
  @brief Order a dynamic array by the "quicksort" method without recursion.

  @details quicksort(a) orders the dynamic array according to the method
  "quicksort" as comparison criteria "Compare".

  The method uses two type parameters:
  -# T: Type elements having the array.
  -# Compare: Class of comparison.

  This method has an expected performance of \f$O(n \; \lg n)\f$
  and it is considered the method of faster system.

  This version of quicksort takes up maximum space of \f$O(\lg n)\f$.

  Quicksort is a probabilistic method. In a very bad case
  -very bad luck- can degrade \f$O(n^2)\f$. 
  To alleviate the extent of luck, bad cases, 
  use quicksort_insertion() running heuristics to
  mitigate the bad cases and invokes the method of insertion
  partitions small settlement.

  @param[in,out] a Array to order.

  @see selection_sort() insertion_sort() heapsort()
  @see quicksort_rec_min() quicksort_insertion() quicksort_rec()

  @ingroup Algos
*/
    template <class T, class Compare> inline
void quicksort(DynArray<T> & a, Compare & cmp)
{
  int i, l = 0, r = a.size() -1;

  FixedStack<int> stack(40);

  push2(stack, l, r);

  while (not stack.is_empty())
    {
      l = stack.pop(); r = stack.pop();

      if (r <= l)
	continue;

      i = partition<T, Compare>(a, l, r, cmp);

      if (i - l > r - i)
	{
	  push2(stack, l, i - 1); push2(stack, i + 1, r);
	}
      else 
	{
	  push2(stack, i + 1, r); push2(stack, l, i - 1); 
	} 
    }
}

    template <class T, class Compare = Aleph::less<T>> inline
void quicksort(DynArray<T> & a, Compare && cmp = Compare())
{
  quicksort<T, Compare>(a, cmp);
}

/** 
  @brief Generic search in a dynamic array of one end element.

  @details search_extreme(a, l, r) search sequentially in the array
  to, among indexes "l" and "r", the end member, minimum or
  maximum at the discretion of comparison "Compare".

  @param[in] a Array on which to search.
  @param[in] l Index beginning of the search.
  @param[in] r Index search term.

  @return The index contentivo of extreme.

  @note It does not check if the sequence is empty.

  @ingroup Algos
*/
    template <class T, class Compare> inline
int search_extreme(const DynArray<T>& a, const int l, const int r, 
		   Compare & cmp)
{
  int extreme_index = l;

  for (int i = l + 1; i <= r; i++)
    if (cmp(a.access(i), a.access(extreme_index)))
      extreme_index = i;

  return extreme_index;
}

    template <class T, class Compare = Aleph::less<T>> inline
int search_extreme(const DynArray<T>& a, const int l, const int r,
		   Compare && cmp = Compare())
{
  return search_extreme<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Returns the maximum element of the array between "l" and "r".

  @ingroup Algos
*/
    template <class T, class Compare> inline
int search_max(const DynArray<T>& a, const int & l, const int & r,
	       Compare & cmp)
{
  return search_extreme<T, Compare>(a, l, r, cmp);
}  

    template <class T, class Compare = Aleph::greater<T>> inline
int search_max(const DynArray<T>& a, const int l, const int r,
	       Compare && cmp = Compare())
{
  return search_max<T, Compare>(a, l, r, cmp);
}

/** 
  @brief Binary search on a sorted dynamic array.

  @details binary_search<T,Compare>(a,x,l,r) performance the searches for "x"
  in the array to understood between the lower limits "l" and
  superior "r". 

  The routine is generic and uses two parameters type:
  -# Data type housing the array.
  -# Comparison class.

  The routine uses the binary search algorithm, which requires
  the array is ordered. This condition is not checked in algorithm.

  The method always returns an integer between [l..r]. If the 
  element is found, then the return value is index where it is 
  located; otherwise, it returns the index where to insert x for 
  the array were ordered.  

  @warning The routine does not fail if the array is empty, but
  beware access it in this situation

  @param[in] a Dynamic array on which to search.
  @param[in] x Element to search.
  @param[in] l Left index of search.
  @param[in] r Right index of search.

  @return The index of the item which is "x" or index
  the position where "x" is inserted.

  @see sequential_search()

  @ingroup Algos
*/
    template <class T, class Compare> inline
int binary_search(const DynArray<T> & a, const T & x, int l, int r, 
		  Compare & cmp)
{
  if (l > r) 
    return l;

  int m;
  while (l <= r)
    {
      m = (l + r)/2;

      if (cmp(x, a.access(m)))
	r = m -1;
      else if (cmp(a.access(m), x))
	l = m + 1;
      else
	return m; /* Clave encontrada */
    }

  return m;
} 

    template <class T, class Compare = Aleph::less<T>> inline
int binary_search(const DynArray<T> & a, const T & x, 
		  int l, int r, 
		  Compare && cmp = Compare())
{
  return binary_search<T, Compare>(a, x, l, r, cmp);
}

/** 
  @brief Binary search on a sorted dynamic array.

  @details binary_search(a,x) searches for "x"
  on the basis of between 0 and <code>size()</code>.

  The routine uses the binary search algorithm, which requires
  the array is ordered. This condition is not checked in algorithm.

  The method always returns an integer between
  [0..size()). If the element is found, then the value of
  return the index where it is; otherwise,
  the rate where it is inserted returns "x" to the array
  it was ordered.

  @warning The routine does not fail if the array is empty, but
  beware access it in this situation

  @param[in] a Dynamic array on which to search.
  @param[in] x Element to search.
  @return The index of the item which is "x" or index
  the position where "x" is inserted.

  @see sequential_search()

  @ingroup Algos
*/
    template <class T, class Compare> inline
int binary_search(const DynArray<T> & a, const T & x, Compare & cmp)
{
  return binary_search<T, Compare>(a, x, 0, a.size() - 1, cmp);
}


    template <class T, class Compare = Aleph::less<T>> inline
int binary_search(const DynArray<T> & a, const T & x, 
		  Compare && cmp = Compare())
{
  return binary_search<T, Compare>(a, x, 0, (int) a.size() - 1, cmp);
}



    template <class T, class Compare> inline
void quicksort_op(DynArray<T> & a, 
		  Compare & cmp,
		  size_t threshold = Quicksort_Threshold)
{
  int i, l = 0, r = a.size() -1;

  FixedStack<int> stack(40);

  push2(stack, l, r);

  while (not stack.is_empty())
    {
      l = stack.pop(); r = stack.pop();

      int partition_size = r - l + 1;
      if (partition_size <= 1)  
	continue;

      if (partition_size <= threshold)
	{
	  insertion_sort<T, Compare>(a, l, r, cmp);
	  continue;
	}

      i = partition_op<T, Compare>(a, l, r, cmp);

      if (i - l > r - i)
	{
	  push2(stack, l, i - 1); push2(stack, i + 1, r);
	}
      else 
	{
	  push2(stack, i + 1, r); push2(stack, l, i - 1); 
	} 
    }
}



    template <class T, class Compare = Aleph::less<T>> inline
void quicksort_op(DynArray<T> & a, 
		  Compare && cmp = Compare(),
		  size_t threshold = Quicksort_Threshold)
{
  quicksort_op<T, Compare>(a, cmp, threshold);
}

# undef push2


/** 
  @brief Recursive binary search on a sorted array.

  @details binary_search_rec<T,Compare>(a,x,l,r) performance the searches for "x"
  in the array to understood between the lower limits "l" and
  superior "r". 

  The routine is generic and uses two parameters type:
  -# El Tipo de dato que alberga el arreglo.
  -# La Class of comparison.

  The routine uses the binary search algorithm, which requires
  the array is ordered. This condition is not checked in algorithm.

  The method always returns an integer between [l..r]. If the 
  element is found, then the return value is index where it is 
  located; otherwise, it returns the index where to insert x for 
  the array were ordered.  

  @param[in] a Array on which to search.
  @param[in] x Element to search.
  @param[in] l Left index of search.
  @param[in] r Right index of search.

  @return The index of the item which is "x" or index
  the position where "x" is inserted.

  @see sequential_search()

  @ingroup Algos
*/
    template <typename T, class Compare> inline 
int binary_search_rec(T * a, const T & x, 
		      const int & l, const int & r,
		      Compare & cmp)
{
  const int m = (l + r) / 2;
  if (l > r) 
    return m;

  if (cmp(x, a[m]))
    return binary_search_rec<T, Compare>(a, l, m - 1, cmp);
  else if (cmp(a[m], x))
    return binary_search_rec<T, Compare>(a, m + 1, r, cmp);

  return m; /* encontrado
}

    template <typename T, class Compare = Aleph::less<T>> inline 
int binary_search_rec(T * a, const T & x, 
		      const int & l, const int & r,
		      Compare && cmp = Compare())
{
  return binary_search_rec<T, Compare>(a, x, l, r, cmp);
}


/** 
  @brief Iterative binary search on a sorted array.

  @details binary_search<T,Compare>(a,x,l,r) performance the searches for "x"
  in the array to understood between the lower limits "l" and
  superior "r". 

  The routine is generic and uses two parameters type:
  -# Data type housing the array.
  -# Comparison class.

  The routine uses the binary search algorithm, which requires
  the array is ordered. This condition is not checked in algorithm.

  The method always returns an integer between [l..r]. If the 
  element is found, then the return value is index where it is 
  located; otherwise, it returns the index where to insert x for 
  the array were ordered.  

  @param[in] a Array on which to search.
  @param[in] x Element to search.
  @param[in] l Left index of search.
  @param[in] r Right index of search.

  @return The index of the item which is "x" or index
  the position where "x" is inserted.

  @see sequential_search()

  @ingroup Algos
*/
    template <typename T, class Compare> inline 
long binary_search(T * a, const T & x, long l, long r, Compare & cmp)
{
  long m;
  while (l <= r)
    {
      m = (l + r) / 2;
      if (cmp(x, a[m]))
	r = m - 1;
      else if (cmp(a[m], x))
	l = m + 1;
      else 
	break;
    }
  return m; 
}

template <typename T, class Compare> inline 
long binary_search(T * a, T && x, long l, long r, Compare & cmp)
{
  return binary_search<T, Compare>(a, x, l, r, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline 
long binary_search(T * a, const T & x, long l, long r, 
		   Compare && cmp = Compare())
{
  return binary_search<T, Compare>(a, x, l, r, cmp);
}

    template <typename T, class Compare = Aleph::less<T>> inline 
long binary_search(T * a, T && x, long l, long r, Compare && cmp = Compare())
{
  return binary_search<T, Compare>(a, x, l, r, cmp);
}


}

# endif /* TPL_SORT_UTILS_H
