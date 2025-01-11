
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
# ifndef TPL_SORT_UTILS_H
# define TPL_SORT_UTILS_H

# include <ahUtils.H>
# include <ahFunctional.H>
# include <tpl_arrayStack.H>
# include <tpl_array.H>
# include <tpl_dynArray.H>
# include <tpl_dynDlist.H>
# include <htlist.H>

namespace Aleph
{
  extern size_t Insertion_Threshold;

  extern size_t Quicksort_Threshold;

  extern const int Not_Found;

  /** Return true if the sequence is sorted

      \ingroup Algos
  */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  bool is_sorted(const Container<T> & cont, Compare cmp = Compare())
  {
    if (cont.is_empty())
      return true;

    T item = cont.get_first();
    typename Container<T>::Iterator it(cont); it.next_ne();
    for (; it.has_curr(); it.next_ne())
      {
        const T & curr = it.get_curr();
        if (not cmp(item, curr))
          return false;

        item = curr;
      }
    return true;
  }


  /** Check if container is sorted.

      @return a pair whose first element says whether the array is or
      not sorted. If the array is not sorted, then second contains the
      index where the inversion was detected. Otherwise, second
      contains the number of elements that the container has.

      \ingroup Algos
  */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  pair<bool, size_t> search_inversion(const Container<T> & cont,
                                      Compare cmp = Compare())
  {
    if (cont.is_empty())
      return make_pair(true, 0);

    T item = cont.get_first();
    typename Container<T>::Iterator it(cont); it.next_ne();
    size_t n = 0;
    for (; it.has_curr(); it.next_ne(), ++n)
      {
        const T & curr = it.get_curr();
        if (not cmp(item, curr))
          return make_pair(false, n);

        item = curr;
      }
    return make_pair(true, n);
  }



  /** Return true if the sequence is inversely sorted

      \ingroup Algos
  */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  bool is_inversely_sorted(const Container<T> & cont, Compare cmp = Compare())
  {
    if (cont.is_empty())
      return true;

    T item = cont.get_first();
    typename Container<T>::Iterator it(cont); it.next_ne();
    for (; it.has_curr(); it.next_ne())
      {
        const T & curr = it.get_curr();
        if (not cmp(curr, item))
          return false;

        item = curr;
      }
    return true;
  }

  /** Return a pair where first is `true` if the sequence is sorted. If the
      sequence is not sorted, then first is `false` and second contains
      the index where the test was failed

      \ingroup Algos
  */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  pair<bool, size_t>
  test_sorted(const Container<T> & cont, Compare cmp = Compare())
  {
    if (cont.is_empty())
      return pair<bool, size_t>(true, 0);

    T item = cont.get_first();
    typename Container<T>::Iterator it(cont); it.next_ne();
    for (size_t i = 1; it.has_curr(); it.next_ne(), ++i)
      {
        const T & curr = it.get_curr();
        if (cmp(curr, item))
          return pair<bool, size_t>(false, i);

        item = curr;
      }
    return pair<bool, size_t>(true, 0);
  }


  /** Sorts an array by the selection method.

      selection_sort(a,n) uses the selection method to sort
      the arrangement a of n elements.

      The selection method has a performance of \f$O(n^2)\f$.
      Due to its simplicity of implementation, its constant cost
      is low, so it is a good method for arrangements of
      very small dimension.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] to the array to sort.
      @param[in] n the dimension of the array.
      @see insertion_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
  template <typename T, class Compare  = Aleph::less<T>> inline
  void selection_sort(T * a, const size_t n, Compare cmp = Compare()) noexcept
  {
    for (size_t i = 0, min, j; i < n - 1; ++i)
      {
        for (min = i, j = i + 1; j < n; ++j)
          if (cmp(a[j], a[min]))
            min = j;

        if (cmp(a[min], a[i]))
          std::swap(a[min], a[i]);
      }
  }


  /** Generic search for an extreme element in a list
      of Dlink nodes.

      search_extreme(list) searches sequentially in the list of nodes
      list the extreme, minimum or element
      maximum according to the comparison criterion Compare.

      @param[in] list the list to search on.
      @return pointer to the node containing the extreme element.
      @note There is no checking if the list is empty.
      \ingroup Algos
  */
  template <class Link, class Compare> inline 
  Link * search_extreme(const Link & list, Compare & cmp)
  {
    typename Link::Iterator it(const_cast<Link&>(list));
    Link * extreme = it.get_curr();

    for (it.next(); it.has_curr(); it.next_ne())
      {
        Link * curr = it.get_curr();
        if (cmp(curr, extreme))
          extreme = curr;
      } 

    return extreme;
  }

  template <class Compare> inline 
  Dlink * search_extreme(const Dlink & list, Compare cmp = Compare())
  {
    return search_extreme<Dlink, Compare>(list, cmp);
  }

  template <class Compare> inline 
  Slinknc * search_extreme(const Slinknc & list, Compare cmp = Compare())
  {
    return search_extreme<Slinknc, Compare>(list, cmp);
  }

  /** Sorts a doubly linked list by selection method.

      selection_sort(list) on a list derived from Dlink sort
      using the list selection method.

      The routine requires the Compare type parameter that implements the
      comparison between the members of the list. Compare is invoked as
      Compare() (l1,l2), where l1 and l2 are two links of type Dlink. He
      User is responsible for properly instrumenting Compare()()
      so that the fields of interest are accessed and the
      comparison.

      @param[in,out] list list to be sorted.
      \ingroup Algos
  */
  template <class Compare> inline 
  void selection_sort(Dlink & list, Compare cmp) noexcept
  {
    Dlink aux;
    while (not list.is_empty())
      {
        Dlink * extreme = search_extreme <Dlink, Compare> (list, cmp); 
        extreme->del(); // saque extremo de list
        aux.append(extreme); // insértelo ordenado en aux;
      }

    list.swap(&aux);
  }

  template <typename Tlink, 
            template <class> class Tnode, 
            typename T, class Compare> 
  class Compare_Tnode
  {
    Compare cmp;

  public:

    Compare_Tnode(Compare __cmp = Compare()) 
      : cmp(__cmp) { /* empty */ }

    bool operator () (Tlink * l1, Tlink * l2) const noexcept
    {
      Tnode<T> * n1 = static_cast<Tnode<T>*>(l1);
      Tnode<T> * n2 = static_cast<Tnode<T>*>(l2);

      assert(n1 == l1 and n2 == l2);

      return cmp(n1->get_data(), n2->get_data());
    }

    // Para generalizar en el sequential_search
    bool operator () (Tlink * l, const T & x) const noexcept
    {
      Tnode<T> * n = static_cast<Tnode<T>*>(l);

      assert(n == l);

      return cmp(n->get_data(), x);
    }
  };

  template <typename T, class Compare> 
  struct Compare_Dnode : public Compare_Tnode<Dlink, Dnode, T, Compare>
  {
    Compare_Dnode(Compare cmp = Compare()) 
      : Compare_Tnode<Dlink, Dnode, T, Compare> (std::forward<Compare>(cmp)) 
    { /* empty */ }
  };

  template <typename T, class Compare>
  using Compare_Snodenc = Compare_Tnode<Slinknc, Snodenc, T, Compare>;


  /** Sorts a linked list of type Dnode<T> using
      the selection method.

      Sorts the list list, with nodes of type Dnode<T>
      depending on the selection method.

      The selection method has a performance of \f$O(n^2)\f$.
      Due to its simplicity of implementation and its constant cost
      is low, so it is a good method for arrangements of
      very small dimension.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] list ka lista a ser ordenada.
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> 
  inline void selection_sort(Dnode<T> & list, Compare cmp) noexcept
  {
    selection_sort<Compare_Dnode<T, Compare>>
      (list, Compare_Dnode<T, Compare>(cmp));
  }

  /** Sequential search on an array.

      sequential_search(a,x,l,r) search sequentially,
      in array a, between indices l and r, respectively,
      the first occurrence of x.

      The function handles two type parameters:
      -# T: the type of data held by the array.
      -# Equal: comparison class between elements.

      @param[in] a the array on which the search is performed.
      @param[in] x the element to search for.
      @param[in] the left index where the search begins.
      @param[in] r right index where the search ends.
      @return index of first position in containing array
      element x; -1 otherwise.
      \ingroup Algos
  */
  template <typename T, class Equal = Aleph::equal_to<T>> inline 
  int sequential_search(T * a, T && x, const int l, const int r,
                        Equal eq = Equal()) noexcept
  {
    for (int i = l; i <= r; i++)
      if (eq(a[i], x)) return i;

    return Not_Found;
  }

  /** Sequential search on a dynamic array.

      sequential_search(a,x,l,r) search sequentially,
      in array a, between indices l and r, respectively,
      the first occurrence of x.

      The function handles two type parameters:
      -# T: the type of data held by the array.
      -# Equal: comparison class between elements.

      This version of dynamic arrangements skips positions
      that have certainly not been written. However, take
      taking into consideration that, depending on the size of the block, they may
      there are entries not written but limited to addresses
      valid memory. In this sense, it is possible that within
      that class of entries an element with value x is found.

      @param[in] to the dynamic array on which it is performed
      the search.
      @param[in] x the element to search for.
      @param[in] the left index where the search begins.
      @param[in] r right index where the search ends.
      @return index of first position in containing array
      element x; -1 otherwise.
      @note Search cannot distinguish whether a valid entry has
      been written or not. Programming the Equal class can
      consider a special value that represents that the entry does not
      has been written. It is guaranteed that input that has not been
      mapped to a block will not be accessed.
      @see DynArray
      \ingroup Algos
  */
  template <typename T, class Equal = Aleph::equal_to<T>> inline
  int sequential_search(const DynArray<T> & a, const T & x, 
                        const int l, const int r, Equal eq = Equal()) noexcept
  {
    for (int i = l; i <= r; i++)
      if (a.exist(i))
        if (eq(a(i), x))
          return i;

    return -1;
  }

  template <class Link, typename T, class Equal>
  Link * sequential_search(const Link & list, const T & x, Equal & eq)
  {
    for (typename Link::Iterator it(const_cast<Link&>(list)); 
         it.has_curr(); it.next_ne())
      {
        Link * curr = it.get_curr();
        if (eq(curr, x)) 
          return curr;
      }

    return nullptr;
  }

  template <typename T, class Equal>
  Dlink * sequential_search(const Dlink & list, const T & x,
                            Equal eq = Equal()) noexcept
  {
    return sequential_search <Dlink, T, Equal>(list, x, eq);
  }

  template <typename T, class Equal>
  Slinknc * sequential_search(const Slinknc & list, const T & x,
                              Equal eq = Equal()) noexcept
  {
    return sequential_search <Slinknc, T, Equal>(list, x, eq);
  }

  /** Sequential search over a list of nodes.

      sequential_search(list,x) search sequentially,
      in the list of nodes of type Dnode<T> list the first
      occurrence of x.

      The function handles two type parameters:
      -# T: the type of data held by the array.
      -# Equal: comparison class between elements.

      @param[in] list the list on which to perform
      the search.
      @param[in] x the element to search for.
      @return pointer to the first containing node
      element x; nullptr otherwise.
      @see Dnode
      \ingroup Algos
  */
  template <typename T, class Equal = Aleph::equal_to<T>> inline
  Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, 
                               Equal & eq) noexcept
  {
    Dlink * ret =
      sequential_search<T, Compare_Dnode <T, Equal>>
      ((Dlink &) list, x, Compare_Dnode<T, Equal>(eq));

    return ret == nullptr ? nullptr : static_cast <Dnode<T> *>(ret);
  }

  template <typename T, class Equal = Aleph::equal_to<T>> inline
  Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, 
                               Equal && eq = Equal()) noexcept
  {
    return sequential_search<T, Equal>(list, x, eq);
  }

  /** Sequential search on a dynamic list DynDlist.

      sequential_search(list,x) search sequentially,
      in the dynamic list list (DynDlist<T>) the first
      occurrence of x.

      The function handles two type parameters:
      -# T: the type of data held by the array.
      -# Equal: comparison class between elements.

      @param[in] list the dynamic list on which to perform
      the search.
      @param[in] x the element to search for.
      @return pointer to the first element with value equal to x;
      nullptr otherwise.
      @see DynDlist
      \ingroup Algos
  */
  template <typename T, class Equal = Aleph::equal_to<T>> inline
  T * sequential_search(const DynDlist<T> & list, const T & x,
                        Equal eq = Equal()) noexcept
  {
    Dnode<T> * ret = sequential_search<T, Equal>((Dnode<T>&) list, x, eq);
    return ret != nullptr ? &ret->get_data() : nullptr;
  }

  template <typename T, class Equal = Aleph::equal_to<T>> inline
  T * sequential_search(const DynList<T> & list, const T & x,
                        Equal & eq = Equal())
    noexcept
  {
    Slinknc * ret =
      sequential_search<T, Compare_Snodenc<T, Equal>>
      (*list.get_head(), x, Compare_Snodenc<T, Equal>(eq));

    return 
      ret != nullptr ? &((static_cast<Snodenc<T>*>(ret))->get_data()) : nullptr;
  }


  /** Generic search in an array of an extreme element.

      search_extreme(a, l, r) searches the array sequentially
      a, between the indices l and r, the extreme, minimum or element
      maximum according to the comparison criterion Compare.

      @param[in] a the array on which to search.
      @param[in] the starting index of the search.
      @param[in] r search term index.
      @return containing index of the endpoint.
      @note There is no checking if the stream is empty.
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  int search_extreme(T * a, const int l, const int r, Compare cmp = Compare())
  {
    T extreme_index = l;
    for (int i = l + 1; i <= r; i++)
      if (cmp(a[i], a[extreme_index])) // ¿se ve un nuevo menor?
        extreme_index = i; // sí

    return extreme_index;
  }

  /** Returns the smallest element of the array a between l and r.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  int search_min(T * a, const int l, const int r, Compare cmp = Compare())
  {
    return search_extreme<T, Compare> (a, l, r, cmp);
  }

  /** Returns the maximum element of the array a between l and r.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::greater<T>> inline
  int search_max(T * a, const int l, const int r, Compare cmp = Compare())
  {
    return search_extreme<T, Compare> (a, l, r, cmp);
  }

  /** \overload search_extreme
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline 
  Dnode<T> * search_extreme(const Dnode<T> & list, Compare cmp = Compare())
  {
    Compare_Dnode<T, Compare> cmp_dnode(cmp);
    Dlink * ret = 
      search_extreme<Dlink, Compare_Dnode<T, Compare>>((Dlink&) list, cmp_dnode);

    return static_cast <Dnode<T>*> (ret);
  }

  /** Generic search for an extreme element in a list
      of nodes (Dlist<T>).

      search_extreme(list) searches sequentially in the list of nodes
      list the extreme, minimum or element
      maximum according to the comparison criterion Compare.

      @param[in] list the list to search on.
      @return pointer to the containing node of the extreme element.
      @note There is no checking if the list is empty.
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  T * search_extreme(const DynDlist<T> & list, Compare cmp = Compare())
  {
    Dnode<T> * ret = search_extreme <T, Compare> ((Dnode<T>&) list, cmp); 

    return ret != nullptr ? &(ret->get_data()) : nullptr;
  }

  template <typename T, class Compare = Aleph::less<T>> inline 
  T * search_extreme(const DynList<T> & list, Compare cmp = Compare())
  {
    Slinknc * ret =
      search_extreme <Compare_Snodenc<T, Compare>>
      (*list.get_head(), Compare_Snodenc<T, Compare>(cmp)); 

    return ret != nullptr ? &(((Snodenc<T>*)ret)->get_data()) : nullptr;
  }

  /** Returns the minimum element of the list list.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  T * search_min(const DynDlist<T> & list, Compare cmp = Compare())
  {
    return search_extreme<T, Compare> (list, cmp);
  }

  /** Returns the maximum element of the list list.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  T * search_max(const DynDlist<T> & list, Compare cmp = Compare())
  {
    return search_extreme<T, Compare> (list, cmp);
  }

  /** Sorts an array by the insert method.

      insertion_sort(a,l,r) uses the selection method to sort
      the arrangement a between the indices l and r.

      The insert method has a performance of \f$O(n^2)\f$.
      It is a simple method, so it consumes little constant time.
      On average it performs half as many exchanges as the method
      of selection. Its time tends to be linear if the arrangement is
      semi-ordered. It is a good method for small arrangements and for
      partitions made by superior methods but with
      longer constant times.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] a the array to sort.
      @param[in] l left index of the array.
      @param[in] r right index of the array.
      @see selection_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void insertion_sort(T * a, const int l, const int r,
                      Compare cmp = Compare()) noexcept
  {
    if (l >= r)
      return;
    for (int i = l, j; i <= r; ++i)
      {
        T tmp = a[i]; // memorice a[i], pues será sobre escrito
        for (j = i; j > l and cmp(tmp, a[j - 1]); --j)
          a[j] = a[j - 1]; // desplazar hacia la derecha

        a[j] = tmp; // inserte tmp en la brecha
      }
  }

  /** Inserts a node orderly into a doubly linked list.

      insert_sorted(list,p) inserts into a list based on Dlink the
      node pointed to by p.

      The ordering criterion is given by the comparison class Compare.

      @param[in,out] list the list to which p will be inserted.
      @param[in] p the node to be inserted.
      @see Dlink 
      \ingroup Algos
  */
  template <class Compare> inline 
  void insert_sorted(Dlink & list, Dlink * p, Compare & cmp)
  {
    typename Dlink::Iterator it(list); 
    while (it.has_curr() and cmp(it.get_curr(), p)) 
      it.next_ne();

    if (it.has_curr())
      it.get_curr()->append(p); // insertar antes de current
    else
      list.append(p); 
  }

  template <class Compare> inline
  void insert_sorted(HTList & list, Slinknc * p, Compare & cmp)
  {
    Slinknc * first = list.get_first();
    if (cmp(p, first) or not cmp(first, p)) // p <= first?
      {
        list.insert(p);
        return;
      }

    Slinknc * last = list.get_last();
    if (cmp(last, p) or not cmp(p, last)) // p >= last?
      {
        list.append(p);
        return;
      }

    Slinknc * prev = list.get_first();
    typename HTList::Iterator it(list); 
    for (it.next(); it.has_curr(); it.next_ne())
      {
        Slinknc * curr = it.get_curr();
        if (cmp(p, curr)) // p < curr
          {
            prev->insert(p);
            return;
          }
        prev = curr;
      }
    assert(false); // it should be unreachable
  }

  /** Sorts a list of simple nodes by insertion method

      insertion_sort(list) sorts by insertion method
      linked list list

      The insert method has a performance of \f$O(n^2)\f$.
	
      @param[in,out] list la lista a ordenar.
      \ingroup Algos
  */
  template <class ListType, class Compare> inline 
  void list_insertion_sort(ListType & list, Compare & cmp) noexcept
  {
    if (list.is_empty())
      return;

    ListType aux; aux.append(list.remove_first());
    while (not list.is_empty())
      insert_sorted<Compare>(aux, list.remove_first(), cmp);

    list.swap(aux);
  }

  template <typename T, class Compare = Aleph::less<T>> inline 
  void insertion_sort(DynList<T> & l, Compare cmp = Compare()) noexcept
  {
    using Cmp = Compare_Snodenc<T, Compare>;
    Cmp c(cmp);  
    list_insertion_sort<HTList, Cmp>(l, c);
  }

  template <typename T, class Compare = Aleph::less<T>> inline 
  DynList<T> insertion_sort(DynList<T> && l, Compare cmp = Compare()) noexcept
  {
    using Cmp = Compare_Snodenc<T, Compare>;
    Cmp c(cmp);  
    list_insertion_sort<HTList, Cmp>(l, c);
    return move(l);
  }

  /** Sorts the list by insertion method.

      Sorts the list based on Dnode<T> by the
      insertion according to comparison criterion Compare.

      The insert method has a performance of \f$O(n^2)\f$.

      @param[in,out] list the list to sort.
      @see Dnode selection_sort(Dlink * link)
  */
  template <typename T, class Compare = Aleph::less<T>> 
  inline void insertion_sort(Dnode<T> & list, Compare cmp = Compare()) noexcept
  {
    using Cmp = Compare_Dnode<T, Compare>;
    Cmp c(cmp);
    list_insertion_sort <Dlink, Cmp> (list, c);
  }

  /** Merges two ordered partitions stored in an array.

      merge() assumes as input two partitions ordered between the
      ranges [l..m] and [m+1..r] and mixes them in an ordering between
      [l..r].

      @param[in] a the array
      @param[in] l left index of the array
      @param[in] m center of the array, of the partition
      @param[in] r right index of the array

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void merge(T * a, const int l, const int m, const int r,
             Compare cmp = Compare()) noexcept
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

  /** Sorts an arrangement by the shuffle method.

      mergesort(a,l,r) sorts the array a between indices l and r
      according to the mixing method according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has a performance of \f$O(n \; \lg n)\f$, but
      a space consumption of \f$O(n)\f$.

      @param[in,out] a the array to sort.
      @param[in] l lower index.
      @param[in] r top index.

      @see selection_sort() insertion_sort() quicksort_rec() heapsort()

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void mergesort(T * a, const int l, const int r, Compare cmp = Compare())
    noexcept
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

  /** Merge two ordered lists into one.

      merge_lists(l1,l2,result) takes two sorted lists l1 and l2
      and merges them into a single ordered list result according to
      comparison criterion Compare.

      It is not checked whether lists l1 and l2 are sorted.
      Results will be incorrect if this premise is not met.

      @param[in,out] l1 an ordered list to merge.
      @param[in,out] l2 an ordered list to merge.
      @param[out] result the list where the result will be placed.
	
      \ingroup Algos
  */
  template <typename Tlist, class Compare> inline
  void merge_lists(Tlist & l1, Tlist & l2, Tlist & result,
                   Compare cmp = Compare()) noexcept
  {
    assert(result.is_empty());

    while (not l1.is_empty() and not l2.is_empty())
      if (cmp(l1.get_first_ne(), l2.get_first_ne()))
        result.append(l1.remove_first_ne());
      else
        result.append(l2.remove_first_ne());

    if (l1.is_empty())
      result.concat_list(l2);
    else
      result.concat_list(l1);

    assert(l1.is_empty() and l2.is_empty());
  }

  /** Merge two sorted lists of type Dnode into one.

      merge_lists(l1,l2,result) takes two sorted lists l1 and l2
      and merges them into a single ordered list result according to
      comparison criterion Compare.

      It is not checked whether lists l1 and l2 are sorted.
      Results will be incorrect if this premise is not met.

      @param[in,out] l1 an ordered list to merge.
      @param[in,out] l2 an ordered list to merge.
      @param[out] result the list where the result will be placed.
	
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void merge_lists(Dnode<T> & l1, Dnode<T> & l2, Dnode<T> & result,
                   Compare cmp = Compare()) noexcept
  {
    merge_lists<Dnode<T>, Compare_Dnode<T, Compare>>(l1, l2, result, cmp);
  }

  /** Sorts a list according to the mergesort method.

      Sort a list using the mergesort method
      based on Dlink, according to comparison criteria Compare.

      This method has a deterministic performance of \f$O(n \; \lg n)\f$
      and a space consumption of \f$O(1)\f$. It is a very good method
      for lists.

      @param[in,out] list the list to be sorted.

      @see mergesort(Dnode<T> & list)
      \ingroup Algos
  */
  template <typename Tlist, class Compare> inline 
  void mergesort(Tlist & list, Compare cmp = Compare()) noexcept
  {
    if (list.is_unitarian_or_empty()) 
      return;

    Tlist l, r;
    list.split_list_ne(l, r);         // dividir en dos listas

    mergesort <Tlist, Compare> (l, cmp);
    mergesort <Tlist, Compare> (r, cmp);  
  
    merge_lists <Tlist, Compare> (l, r, list, cmp); // mezclarlas 
  }

  /** Sort a list by mergesort
      combined with the insert method.

      Sort a list using the mergesort method
      dynamic (DynList or DynDlist) according to comparison criteria Compare.

      This method has a deterministic performance of \f$O(n \; \lg n)\f$
      and a space consumption of \f$O(1)\f$. It is a very good method
      for lists.

      @param[in,out] list the list to be sorted.

      @see mergesort(Dnode<T> & list) 

      \ingroup Algos
  */
  template <template <typename> class Tlist, typename T,
            class Compare = Aleph::less<T>> inline 
  void mergeinsertsort(Tlist<T> & list, Compare cmp = Compare(), 
                       const size_t lsz = Aleph::Insertion_Threshold) noexcept
  {
    if (lsz < Aleph::Insertion_Threshold)
      {
        insertion_sort<T, Compare>(list, cmp);
        return;
      }

    if (list.is_unitarian_or_empty()) 
      return;

    Tlist<T> l, r;
    list.split_list(l, r);         // dividir en dos listas

    mergeinsertsort <Tlist, T, Compare> (l, cmp);
    mergeinsertsort <Tlist, T, Compare> (r, cmp);  
  
    merge_lists <Tlist<T>, Compare> (l, r, list, cmp); // mezclarlas 
  }

  template <template <typename> class Tlist, typename T,
            class Compare = Aleph::less<T>> inline 
  void mergesort(Tlist<T> & list, Compare cmp = Compare()) noexcept
  {
    if (list.is_unitarian_or_empty()) 
      return;

    Tlist<T> l, r;
    list.split_list(l, r);         // dividir en dos listas

    mergesort <Tlist, T, Compare> (l, cmp);
    mergesort <Tlist, T, Compare> (r, cmp);  
  
    merge_lists <Tlist<T>, Compare> (l, r, list, cmp); // mezclarlas 
  }

  /** Sorts a list according to the mergesort method.

      Sort a list using the mergesort method
      based on Dnode<T>, according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has a deterministic performance of \f$O(n \; \lg n)\f$
      and a space consumption of \f$O(1)\f$. It is a very good method
      for lists.

      @param[in,out] list the list to be sorted.

      @see mergesort(Dlink & list) quicksort(Dnode<T>)

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void mergesort(Dnode<T> & list, Compare cmp = Compare()) noexcept
  {
    mergesort <Dnode<T>, Compare_Dnode<T, Compare>> (list, cmp);
  }

  /** Sorts a dynamic list based on the mergesort method.

      Sort a list using the mergesort method
      dynamics according to comparison criterion Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has a deterministic performance of \f$O(n \; \lg n)\f$
      and a space consumption of \f$O(1)\f$. It is a very good method
      for lists.

      @param[in,out] list the list to be sorted.

      @see quicksort(DynDlist<T> & list)

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void mergesort(DynDlist<T> & list, Compare cmp = Compare()) noexcept
  {
    mergesort <Dnode<T>, Compare_Dnode<T, Compare>> (list, cmp);
  }

  template <typename T, class Compare = Aleph::less<T>> inline
  void mergesort(DynList<T> & list, Compare cmp = Compare()) noexcept
  {
    mergesort<DynList<T>, Compare>(list, cmp);
  }


  template <typename T, class Compare = Aleph::less<T>> inline
  DynList<T> mergesort(DynList<T> && list, Compare cmp = Compare())
  {
    auto l = list;
    mergesort<DynList<T>, Compare>(l, cmp);
    return l;
  }

  template <typename T, class Compare = Aleph::less<T>> inline
  int select_pivot(T * a, const int l, const int r, Compare cmp = Compare())
    noexcept;


  template <typename T, class Compare = Aleph::less<T>> inline
  int partition(T * a, const int l, const int r, Compare cmp = Compare()) noexcept
  {
    const int p = select_pivot <T, Compare> (a, l, r, cmp);
    std::swap(a[p], a[r]);

    T pivot = a[r]; // pivot element
    int i = l - 1,  // index first element to the left > which pivot
      j = r;      // index first element to the right < that pivot
    while (true)
      {
        // advance while a[i] < a[pivot]
        while (cmp(a[++i], pivot)) { /* no hay cuerpo */ }

        while (cmp(pivot, a[--j])) // advance while a[pivot]< a[j]
          if (j == l) // Has the left edge been reached?
            break; // yes ==> the iteration must be finished

        if (i >= j) 
          break;

        // At this point there is an inversion a[i] > a[pivot] > a[j]
        std::swap(a[i], a[j]); //Eliminate inversion
      }

    std::swap(a[i], a[r]);

    return i;
  }

  /** Sorts an array using the quicksort method.

      quicksort_rec(a,l,r) sorts the array a between indices l and r
      according to the quicksort method according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      This version of quicksort can take up \f$O(n)\f$ space.
      Use quicksort_rec_min() if you want to guarantee consumption
      maximum space of \f$O(\lg n)\f$ at the expense of a little more
      of constant time.

      Quicksort is a probabilistic method. In a very bad case
      -very bad luck- can be degraded to
      \f$O(n^2)\f$. To alleviate, to the extent of luck, the bad
      cases, use quicksort_insertion() which runs heuristics to
      alleviate the bad cases and invoke the insert method to
      small array partitions.

      @param[in,out] a the array to sort.
      @param[in] l lower index.
      @param[in] r top index.

      @see selection_sort() insertion_sort() mergesort() heapsort()
      @see quicksort_rec_min() quicksort_insertion() quicksort()

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void quicksort_rec(T * a, const int l, const int r, Compare cmp = Compare())
    noexcept
  {
    if (l >= r) 
      return;

    const int pivot = partition <T, Compare> (a, l, r, cmp);

    quicksort_rec <T, Compare> (a, l, pivot - 1, cmp);
    quicksort_rec <T, Compare> (a, pivot + 1, r, cmp);
  }

  template <typename T, class Compare = Aleph::less<T>> inline
  void quicksort_no_tail(T * a, int l, int r, Compare cmp = Compare()) noexcept
  {
  start:
    if (l >= r) 
      return;

    const int pivot = partition <T, Compare> (a, l, r, cmp);

    quicksort_no_tail <T, Compare> (a, l, pivot - 1, cmp);

    l = pivot + 1;
    goto start;
  }

  /** Sorts an array according to the quicksort method with minimum
      space consumption.

      Purely recursive quicksort can consume proportional stack
      to the size of the arrangement to be ordered. To avoid this problem,
      at the expense of a slight execution cost, quicksort_rec_min()
      always sorts the smallest partition first. For others,
      The same considerations as for quicksort_rec() apply.

      @param[in,out] a the array to sort.
      @param[in] l initial index by which to sort.
      @param[in] r final index by which to sort.
      @see quicksort_rec() quicksort() quicksort_insertion()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void quicksort_rec_min(T * a, const int l, const int r,
                         Compare cmp = Compare()) noexcept
  {
    if (r <= l) 
      return;  

    const int pivot = partition<T, Compare>(a, l, r, cmp);
    if (pivot - l < r - pivot) // ¿cual es la partición más pequeña?
      {     // partición izquierda más pequeña
        quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
        quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
      }
    else
      {     // partición derecha más pequeña
        quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
        quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
      }
  } 

  template <typename T, class Compare> inline
  int select_pivot(T * a, const int l, const int r, Compare cmp)
    noexcept
  {
    assert(l <= r);

    if (r - l <= 2) 
      return r;

    const int m = (r + l) / 2; // índice del centro
    const int p = cmp(a[l], a[m]) ? m : l; // p=max(a[l],a[m])

    return cmp (a[r], a[m]) ? r : p; // retornar min(a[r], a[m])
  }

  /** Sorts an array by the quicksort method without recursion.

      quicksort_rec(a,l,r) sorts the array a between indices l and r
      according to the quicksort method according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      This version of quicksort can take up \f$O(n)\f$ space.
      Use quicksort_rec_min() if you want to guarantee consumption
      maximum space of \f$O(\lg n)\f$ at the expense of a little more
      of constant time.

      Quicksort is a probabilistic method. In a very bad case
      -very bad luck- can be degraded to
      \f$O(n^2)\f$. To alleviate, to the extent of luck, the bad
      cases, use quicksort_insertion() which runs heuristics to
      alleviate the bad cases and invoke the insert method to
      small array partitions.

      @param[in,out] to the array to sort.
      @param[in] the lower index.
      @param[in] r top index.

      @see selection_sort() insertion_sort() mergesort() heapsort()
      @see quicksort_rec_min() quicksort_insertion() quicksort_rec()

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void quicksort(T * a, const int l, const int r, Compare cmp = Compare())
    noexcept
  {
    if (r - l < Quicksort_Threshold) 
      {
        insertion_sort(a, l, r, cmp);
        return;
      }

    typedef typename std::pair<int, int> Partition; 
    FixedStack<Partition> stack(64);
    stack.push(Partition(l, r));

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

        if (pivot - p.first < p.second - pivot) // ¿cuál más pequeña?
          {     // partición izquierda más pequeña
            stack.push(Partition(pivot + 1, p.second));
            stack.push(Partition(p.first, pivot - 1));
          }
        else
          {     // partición derecha más pequeña
            stack.push(Partition(p.first, pivot - 1));
            stack.push(Partition(pivot + 1, p.second));
          }
      }
  }

  /** Sorts a linked list by the quicksort method.

      quicksort(list) sorts a list
      according to the quicksort method according to comparison criteria Compare.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      @param[in,out] list the list to sort.

      \ingroup Algos
  */
  template <class Compare> 
  void quicksort(Dlink & list, Compare cmp = Compare()) noexcept
  {
    if (list.is_unitarian_or_empty()) 
      return;

    Dlink * pivot = list.remove_next();
    Dlink smaller, bigger; // listas de menores y mayores que pivot

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

    list.concat_list(&smaller); // restaurar listas ordenadas en list
    list.append(pivot);
    list.concat_list(&bigger);
  } 

  /** Sorts a list by the quicksort method.

      quicksort(list) sorts a list using the method
      quicksort according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      This primitive can be used for lists of type Dlist and DynDlist.

      @param[in,out] list the list to sort.

      @see Dnode Dlist DynDlist

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>>
  void quicksort(Dnode<T> & list, Compare cmp = Compare()) noexcept
  {
    quicksort <Compare_Dnode<T, Compare>> (list, cmp);
  }

  template <typename T, class Compare> 
  void quicksort(HTList & list, Compare cmp = Compare()) noexcept
  {
    if (list.is_unitarian_or_empty()) 
      return;

    Snodenc<T> * pivot = (Snodenc<T> *) list.remove_first_ne();
    HTList smaller, bigger; // listas de menores y mayores que pivot

    while (not list.is_empty()) 
      {
        Snodenc<T> * p = (Snodenc<T> *) list.remove_first_ne();
        if (cmp(p->get_data(), pivot->get_data()))
          smaller.append(p);
        else
          bigger.append(p);
      }

    quicksort <T, Compare> (bigger, cmp);  
    quicksort <T, Compare> (smaller, cmp);

    list.concat_list(smaller); // restaurar listas ordenadas en list
    list.append(pivot);
    list.concat_list(bigger);
  } 

  template <typename T, class Compare = Aleph::less<T>>
  void quicksort(DynList<T> & list, Compare cmp = Compare()) noexcept
  {
    quicksort<T, Compare>((HTList&) list, cmp);
  }

  /** Sorts an array by the improved quicksort method.

      quicksort_insertion(a,l,r) sorts the array a among the
      indexes l and r according to the quicksort method according to criteria
      comparison Compare.

      The procedure combines various techniques to accelerate the
      ordering and at the same time avoid degradation due to bad cases.

      Bad cases are treated by pivot selection
      consisting of the median between l, r and the center of the partition.

      To ensure a maximum battery consumption of \f$O(\lg n)\f$,
      the method always proceeds recursively to sort the partition
      smaller.

      When the partition reaches a size less than or equal to the
      sim_id constant Aleph::Insertion_Threshold, then
      The partition is sorted by the insert method, which
      It is faster than quicksort for small sizes.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      @param[in,out] to the array to sort.
      @param[in] the lower index.
      @param[in] r top index.

      @see insertion_sort() mergesort() heapsort()
      @see quicksort_rec_min() quicksort_insertion() quicksort()

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void quicksort_insertion(T * a, const int l, const int r,
                           Compare cmp = Compare()) noexcept
  {
    if (r <= l) 
      return;

    const int pivot = partition<T, Compare>(a, l, r, cmp);

    const int l_size = pivot - l; // left partition size
    const int r_size = r - pivot; // right partition size
    bool left_done = false; // true if left partition is sorted
    bool right_done = false; // true if der partition is sorted

    if (l_size <= Aleph::Insertion_Threshold) 
      {     // sort left partition by insertion
        insertion_sort<T, Compare>(a, l, pivot - 1, cmp);  
        left_done = true;
      }

    if (r_size <= Aleph::Insertion_Threshold) 
      {     // sort der partition by insertion
        insertion_sort<T, Compare>(a, pivot + 1, r, cmp);
        right_done = true;
      }

    if (left_done and right_done) 
      return; //both partitions sorted by insertion

    if (left_done) // left partition sorted by insertion?
      { // Yeah; It only remains to recursively sort the right partition
        quicksort_insertion<T, Compare>(a, pivot + 1, r, cmp);
        return;
      }

    if (right_done) // der partition sorted by insertion?
      { // Yeah; It only remains to recursively sort the left partition
        quicksort_insertion<T, Compare>(a, l, pivot - 1, cmp);
        return;
      }

    // here, both partitions were not insertion sorted
    if (l_size < r_size) // sort smallest partition first
      {     //smaller left partition
        quicksort_insertion <T, Compare> (a, l, pivot - 1, cmp);
        quicksort_insertion <T, Compare> (a, pivot + 1, r, cmp);
      }
    else
      {     // smaller right partition
        quicksort_insertion <T, Compare> (a, pivot + 1, r, cmp);
        quicksort_insertion <T, Compare> (a, l, pivot - 1, cmp);
      }
  }    

  /** Random search for an element in an array.

      random_search(a,x,l,r) uses the partition algorithm
      quicksort to find element x in the array a
      between the limits l and r.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      The procedure has an expected complexity of
      \f$O(n \; \lg n)\f$, which is greater than mere search
      sequential, but with the addition that after the
      search the arrangement is partially ordered and, since
      the pivot selection is the median between l, r and the center
      of the arrangement, the search tends to be linear as
      more random searches are done.

      @param[in,out] a  array to search for; is partially modified
      after the search.
      @param[in] x element to search for.
      @param[in] l lower index of the start of the search.
      @param[in] r top index of search term.
      @return index of an array entry containing the
      value x if it is within the array;
      Not_Found (usually -1) otherwise.

      \ingroup Algos
      @see sequential_search() binary_search_rec()
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  int random_search(T * a, const T & x, const long l, const long r, 
                    Compare cmp = Compare()) noexcept
  {
    if (l > r)
      return Not_Found;

    const long pivot = partition<T, Compare>(a, l, r, cmp);

    if (cmp(x, a[pivot]))
      return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
    else if (cmp(a[pivot], x))
      return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

    return pivot; // element found at index x
  }

  template <typename T, class Compare = Aleph::less<T>> inline
  int random_search(DynArray<T> & a, const T & x, const long l, const long r,
                    Compare cmp = Compare()) noexcept
  {
    if (l > r)
      return Not_Found;

    const long pivot = partition<T, Compare>(a, l, r, cmp);

    if (cmp(x, a(pivot)))
      return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
    else if (cmp(a(pivot), x))
      return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

    return pivot; // element found at index x
  }

  /** Random search for an element in a dlink list.

      dlink_random_search(list,x) uses the algorithm
      quicksort partition to find element x
      in the list list.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      The procedure has an expected complexity of
      \f$O(n \; \lg n)\f$, which is greater than mere search
      sequential, but with the addition that after the
      search the list is partially ordered.

      @param[in,out] list list in which the search is performed;
      is partially modified after the search.
      @param[in] x element to search for.
      @return pointer to the node containing the value x if it is
      is found within the list; nullptr otherwise.

      @see random_search(Dlink&list,const T&x)

      \ingroup Algos
  */
  template <typename T, class Compare> inline
  Dnode<T> * dlink_random_search(Dlink & list, const T & x,
                                 Compare cmp = Compare()) noexcept
  {
    if (list.is_empty()) 
      return nullptr;

    Dnode<T>   item(x);
    Dnode<T> * item_ptr = &item; // puntero a celda que contiene a x

    Dlink smaller; // lista de los menores que pivot
    Dlink bigger;  // lista de los mayores que pivot

    Dnode<T> * pivot = static_cast<Dnode<T>*>(list.remove_next());
  
    while (not list.is_empty()) 
      {
        Dlink * p = list.remove_next();
        if (cmp(p, pivot))
          smaller.append(p);
        else
          bigger.append(p);
      }

    Dnode<T> * ret_val = nullptr;
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

  /** Random search for an element in a Dnode<T> list.

      random_search(list,x) uses the partition algorithm
      quicksort to find element x in the list list.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      The procedure has an expected complexity of
      \f$O(n \; \lg n)\f$, which is greater than mere search
      sequential, but with the addition that after the
      search the list is partially ordered.

      A specialization uses the "less than" relationship as
      comparison criterion and saves writing that
      class.

      @param[in,out] list list in which the search is performed;
      is partially modified after the search.
      @param[in] x element to search for.
      @return pointer to the node containing the value x if it is
      is found within the list; nullptr otherwise.

      @see dlink_random_search

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>>
  Dnode<T> * random_search(Dlink & list, const T & x, Compare cmp = Compare())
    noexcept
  {
    return dlink_random_search <T, Compare_Dnode<T, Compare>> (list, x, cmp);
  }

  /** Random search for an element in a dynamic list.

      random_search(list,x) uses the partition algorithm
      quicksort to find element x in the dynamic list list.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      The procedure has an expected complexity of
      \f$O(n \; \lg n)\f$, which is greater than mere search
      sequential, but with the addition that after the
      search the list is partially ordered.

      A specialization uses the "less than" relationship as
      comparison criterion and saves writing that
      class.

      @param[in,out] list dynamic list in which to perform
      the search; is partially modified after the search.
      @param[in] x element to search for.
      @return pointer x inside the list if it
      is found within it; nullptr otherwise.

      @see DynDlist 

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  T * random_search(DynDlist<T> & list, const T & x, Compare cmp = Compare())
    noexcept
  {
    Dnode<T> * p = 
      dlink_random_search <T, Compare_Dnode<T, Compare>> (list, x, cmp);

    return p == nullptr ? nullptr : &(p->get_data());
  }

  template <typename T, class Compare> static inline
  const T & __random_select(T * a, const long i, const long l, const long r,
                            Compare & cmp) noexcept
  {
    const long pivot = partition<T, Compare>(a, l, r, cmp);
    if (i == pivot) 
      return a[i];

    if (i < pivot) // Is it in left partition?
      return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
    else
      return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
  }

  /** Selects a pivot element of a DynArray as the median between
      the ends and the center.

      @ingrouop Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  long select_pivot_op(const DynArray<T> & a, const long l, const long r,
                       Compare cmp = Compare()) noexcept
  {
    assert(l <= r);

    if (r - l <= 5)
      return r;

    const long m = (l + r)/2; // central element

    // Access array entries only once
    T & la = a(l); 
    T & ra = a(r);
    T & ma = a(m);
  
    const T * med_ptr = Aleph::median<T, Compare>(la, ma, ra, cmp);
  
    if (med_ptr == &la)
      return l;

    if (med_ptr == &ma)
      return m;

    assert(med_ptr == &ra);

    return r;
  }

  /** Selects a pivot element of a DynArray as the median between
      the ends and the center.

      @ingrouop Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  long select_pivot_op(const Array<T> & a, const long l, const long r,
                       Compare cmp = Compare()) noexcept
  {
    assert(l <= r);

    if (r - l <= 5)
      return r;

    const long m = (l + r)/2; // central element

    // Access array entries only once
    T & la = a(l); 
    T & ra = a(r);
    T & ma = a(m);
  
    const T * med_ptr = Aleph::median<T, Compare>(la, ma, ra, cmp);
  
    if (med_ptr == &la)
      return l;

    if (med_ptr == &ma)
      return m;

    assert(med_ptr == &ra);

    return r;
  }

  template <typename T, class Compare = Aleph::less<T>> inline
  int partition_op(const DynArray<T> & a, long l, long r,
                   Compare cmp = Compare()) noexcept
  {
    if (l == r)
      return l;

    long i = l - 1;
    long j = r;

    T & pivot       = a(r);
    long pivot_idx  = select_pivot_op<T, Compare>(a, l, r, cmp);
    T & pivot_value = a(pivot_idx);
    std::swap(pivot, pivot_value);

    while (true)
      {
        while (cmp(a(++i), pivot)) ; 

        while (cmp(pivot, a(--j)))
          if (j == l)
            break;

        if (i >= j)
          break;

        std::swap(a(i), a(j));
      }

    std::swap(a(i), a(r));

    return i;
  }

  template <typename T, class Compare = Aleph::less<T>> inline
  int partition_op(const Array<T> & a, long l, long r,
                   Compare cmp = Compare()) noexcept
  {
    if (l == r)
      return l;

    long i = l - 1;
    long j = r;

    T & pivot       = a(r);
    long pivot_idx  = select_pivot_op<T, Compare>(a, l, r, cmp);
    T & pivot_value = a(pivot_idx);
    std::swap(pivot, pivot_value);

    while (true)
      {
        while (cmp(a(++i), pivot)) ; 

        while (cmp(pivot, a(--j)))
          if (j == l)
            break;

        if (i >= j)
          break;

        std::swap(a(i), a(j));
      }

    std::swap(a(i), a(r));

    return i;
  }

  template <typename T, class Compare> static inline
  const T & __random_select(const DynArray<T> & a, const long i, 
                            const long l, const long r,
                            Compare cmp = Compare()) noexcept
  {
    assert(i >= l and i <= r);

    const long pivot = partition_op<T, Compare>(a, l, r, cmp);
    if (i == pivot) 
      return a(i);

    if (i < pivot) // is it in left partition?
      return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
    else
      return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
  }

  template <typename T, class Compare> static inline
  const T & __random_select(const Array<T> & a, const long i, 
                            const long l, const long r,
                            Compare cmp = Compare()) noexcept
  {
    assert(i >= l and i <= r);

    const long pivot = partition_op<T, Compare>(a, l, r, cmp);
    if (i == pivot) 
      return a(i);

    if (i < pivot) // Is it in left partition?
      return __random_select<T, Compare>(a, i, l, pivot - 1, cmp);
    else
      return __random_select<T, Compare>(a, i, pivot + 1, r, cmp);
  }

  template <typename T, class Compare = Aleph::less<T>> 
  const T & random_select(DynArray<T> & a, const long i, Compare cmp = Compare())
  {
    const int n = a.size() - 1;
    if (i > n)
      throw std::out_of_range("index out of range");

    return __random_select<T, Compare>(a, i, 0, n, cmp);
  }

  template <typename T, class Compare = Aleph::less<T>> 
  const T & random_select(Array<T> & a, const long i, Compare cmp = Compare())
  {
    const int n = a.size() - 1;
    if (i > n)
      throw std::out_of_range("index out of range");

    return __random_select<T, Compare>(a, i, 0, n, cmp);
  }

  /** Random selection of the ith element of an array.

      random_select(a,i,n) returns the ith smallest element
      contained in the array a of dimension n.

      The routine uses the quicksort partition to
      search for position i in time \f$O(n \; \lg n)\f$, which
      It is a substantially better time than the search
      sequential (\f$O(n^2)\f$).

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] an array where the ith element will be searched.
      The arrangement remains semi-ordered through the successive
      partitions that have been made.
      @param[in] i position to be accessed.
      @param[in] n dimension of the array
      @throw out_of_range if i is greater than or equal to n.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> 
  const T & random_select(T * a, const long i, const long n,
                          Compare cmp = Compare())
  {
    if (i >= n)
      throw std::out_of_range("index out of range");

    return __random_select<T, Compare>(a, i, 0, n - 1, cmp);
  }

  template <typename T, class Compare = Aleph::less<T>> 
  const T & random_select(T * a, const long i, const long n, 
                          Compare && cmp = Compare()) noexcept
  {
    return random_select<T, Compare>(a, i, n, cmp);
  }

  /** Random selection of the ith element from a list based
      about Dlink.

      random_select(list,i) returns the ith smallest element
      content in the list list according to order criteria
      determined by the Compare class.

      The routine uses the quicksort partition to
      search for position i in time \f$O(n \; \lg n)\f$, which
      It is a substantially better time than the search
      sequential (\f$O(n^2)\f$).

      @param[in,out] list list where the ith element will be searched.
      The list is left semi-ordered through successive
      partitions that have been made.
      @param[in] i position to be accessed.
      @return pointer to Dlink corresponding to position i inside
      of the list;

      \ingroup Algos
  */
  template <class Compare>
  Dlink * dlink_random_select(Dlink & list, const size_t i,
                              Compare cmp = Compare())
  {
    if (list.is_empty()) 
      return nullptr;

    Dlink smaller; // list of minors than pivot
    Dlink bigger;  // list of majors than pivot

    size_t smaller_count = 0, // cantidad de elementos de smaller
      bigger_count  = 0; // cantidad de elementos de bigger

    Dlink * pivot = list.remove_next();
  
    while (not list.is_empty()) 
      {
        Dlink * p = list.remove_next();
        if (cmp(p, pivot)) // ¿p < pivot?
          {  smaller.append(p); ++smaller_count;  }
        else
          {  bigger.append(p); ++bigger_count;  }
      }

    if (i >= smaller_count + bigger_count + 1) 
      throw std::out_of_range("index of selection greater than list's size");

    Dlink * ret_val = nullptr;
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

  /** Random selection of the ith element from a list based
      about Dlink.

      random_select(list,i) returns the ith smallest element
      content in the list list.

      The routine uses the quicksort partition to
      search for position i in time \f$O(n \; \lg n)\f$, which
      It is a substantially better time than the search
      sequential (\f$O(n^2)\f$).

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] list list where the ith element will be searched.
      The list is left semi-ordered through successive
      partitions that have been made.
      @param[in] i position to be accessed.
      @return pointer to Dlink corresponding to position i inside
      of the list; nullptr if i is out of range.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>>
  Dnode<T> * random_select(Dlink & list, const size_t i, Compare cmp = Compare())
  {
    return (Dnode<T>*) 
      dlink_random_select<Compare_Dnode<T, Compare>> (list, i, cmp);
  }

  /** Random selection of the ith element from a dynamic list.

      random_select(list,i) returns the ith smallest element
      content in the dynamic list list.

      The routine uses the quicksort partition to
      search for position i in time \f$O(n \; \lg n)\f$, which
      It is a substantially better time than the search
      sequential (\f$O(n^2)\f$).

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] list dynamic list where the
      ith element. The list is semi-sorted through
      of the successive
      partitions that have been made.
      @param[in] i position to be accessed.
      @return pointer to the corresponding dynamic list element
      to position i within the list; nullptr if i is out of range.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>>
  T * random_select(DynDlist<T> & list, const size_t i, Compare cmp = Compare())
  {
    Dlink * link = dlink_random_select <Compare_Dnode<T, Compare>> (list, i, cmp);

    Dnode<T> * p = static_cast<Dnode<T>*>(link);

    return p != nullptr ? &(p->get_data()) : nullptr;
  }
  
  /** Sorts a dynamic array by the selection method.

      selection_sort(a) uses the selection method to sort
      the dynamic array of n elements.

      The selection method has a performance of \f$O(n^2)\f$.
      Due to its simplicity of implementation, its constant cost
      is low, so it is a good method for arrangements of
      very small dimension.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] A the array to sort.
      @see insertion_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void selection_sort(DynArray<T> & a, Compare cmp = Compare()) noexcept
  {
    const int n = a.size();

    for (int i = 0; i < n - 1; i++)
      {
        int min = i;

        for (int j = i + 1; j < n; j++)
          if (cmp(a(j), a(min)))
            min = j;

        if (cmp(a(min), a(i)))
          std::swap(a(min), a(i));
      }
  }

  /** Sorts a dynamic array by the bubble method.

      bubble_sort(a) uses the bubble method to sort
      the dynamic array of n elements.

      The bubble method has a performance of
      \f$O(n^2)\f$. It is possibly the most inefficient method of all
      the existing ones.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] A the array to sort.
      @see insertion_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void bubble_sort(DynArray<T> & a, Compare cmp = Compare()) noexcept
  {
    const int n = a.size();

    for (int i = 0; i < n - 1; i++)
      for (int j = n - 1; j > i; j--)
        if (cmp(a(j), a(j - 1)))
          std::swap(a(j - 1), a(j));
  }

  /** Sorts a dynamic array by the insert method.

      insertion_sort(a) uses the selection method to sort
      the dynamic arrangement a.

      The insert method has a performance of \f$O(n^2)\f$.
      It is a simple method, so it consumes little constant time.
      On average it performs half as many exchanges as the method
      of selection. Its time tends to be linear if the arrangement is
      semi-ordered. It is a good method for small arrangements and for
      partitions made by superior methods but with
      longer constant times.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] to the array to sort.
      @param[in] the left index of the array art to be
      would you like to order
      @param[in] r right index of the desired part of the array
      order.
      @see selection_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
  template <template <typename> class C, typename T, 
            class Compare = Aleph::less<T>> inline
  void insertion_sort(C<T> & a, long l, long r, Compare cmp = Compare()) noexcept
  {
    for (long i = l + 1; i <= r; i++)
      {
        T tmp = a(i);
        long j = i;
        for (/* nothing */; j > 0 and cmp(tmp, a(j - 1)); --j)
          a(j) = a(j - 1);

        a(j) = tmp;
      }
  }
  
  /**
     \overload insertion_sort()
     \ingroup Algos
  */
  template <template <typename> class C, typename T, 
            class Compare = Aleph::less<T>> inline
  void insertion_sort(C<T> & a, Compare cmp = Compare()) noexcept
  {
    insertion_sort(a, 0, a.size() - 1, cmp);
  }

  /** Sorts a dynamic array by the shell method.

      shellsort(a) uses the select method to sort
      the dynamic arrangement a.

      The shell method tends toward \f$O(n^2)\f$ performance, but in
      the practice is considerably less.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] to the array to sort.
      @see selection_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void shellsort(DynArray<T> & a, Compare cmp = Compare()) noexcept
  {
    const long n = a.size();
    int incs[16] = { 1391376, 463792, 198768, 86961, 33936, 13776, 
                     4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1 }; 

    for (long k = 0; k < 16; k++) 
      { 
        const long h = incs[k]; 
        for (long i = h; i < n; i++) 
          { 
            T tmp = a(i); 
            long j = i;

            while (j >= h and cmp(tmp, a(j - h)))
              {
                a(j) = a(j - h);
                j -= h;
              } 

            a(j) = tmp;
          } 
      } 
  }


  inline static long back_index(const long i) noexcept { return i - 1; }


  template <typename T, class Compare> inline
  void sift_up(DynArray<T> & table, const size_t n, Compare & cmp) noexcept
  { 
    long p;
    for (long i = n; i > 1; i = p) 
      {
        p = i >> 1;     // c = i/2 

        if (cmp(table(back_index(p)), table(back_index(i))))
          return;

        std::swap(table(back_index(p)), table(back_index(i)));
      }
  }

  template <typename T, class Compare> inline
  void sift_down(DynArray<T> & table, const size_t n, Compare & cmp) noexcept
  {
    long i = 1;

    while (true)
      {
        size_t c = i << 1; // c = 2*i

        if (c > n) 
          return;

        if (c + 1 <= n)
          if (cmp(table(back_index(c + 1)), table(back_index(c))))
            c++;

        if (cmp(table(back_index(i)), table(back_index(c))))
          return; 

        std::swap(table(back_index(c)), table(back_index(i)));
        i = c;
      }
  }

  template <typename T, class Compare>
  class Negate_Compare
  {
    Compare cmp;

  public:

    Negate_Compare(Compare __cmp = Compare()) 
      noexcept(std::is_nothrow_copy_assignable<Compare>::value)
      : cmp(__cmp) { /* Empty */ }

    bool operator () (const T & e1, const T & e2) noexcept
    {
      return not cmp(e1, e2);
    }
  };

  /** Sorts a dynamic array by the heapsort method.

      heapsort(a) uses the heapsort method to sort
      the arrangement a of n elements.

      The heapsort has a guaranteed performance of \f$O(n \; \lg n)\f$
      and it is stable.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.
      @param[in,out] a the array to be ordered.
      @see insertion_sort() quicksort_rec() mergesort() 
      @see selection_sort() faster_heapsort()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void heapsort(DynArray<T> & a, Compare cmp = Compare()) noexcept
  {
    const long n = a.size();

    long i;
    for (i = 2; i <= n; i++)
      sift_up<T, Negate_Compare<T, Compare>>(a, i, 
                                             Negate_Compare<T, Compare>(cmp));

    for (i = n; i >= 2; i--)
      {
        std::swap(a(0), a(i - 1));

        sift_down<T, Negate_Compare<T, Compare>>(a, i - 1,
                                                 Negate_Compare<T, Compare>(cmp));
      }
  }

  template <template <typename> class C, typename T, 
            class Compare = Aleph::less<T>> inline
  long partition(C<T> & a, long l, long r, Compare cmp = Compare()) noexcept
  {
    if (l == r)
      return l;

    long i = l - 1;
    long j = r;
    const T & pivot = a(r);

    while (true)
      {
        while (cmp(a(++i), pivot)) { /* Más nada */ }

        while (cmp(pivot, a(--j)))
          if (j == l)
            break;

        if (i >= j)
          break;
      
        std::swap(a(i), a(j));
      }

    std::swap(a(i), a(r));

    return i;
  }

  template <typename T, class Compare = Aleph::less<T>> inline 
  void quicksort_rec(DynArray<T> & a, const long l, const long r,
                     Compare cmp = Compare()) noexcept
  {
    if (r <= l) 
      return;

    long i = partition<T, Compare>(a, l, r, cmp);
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

# define push2(stack, a, b)  stack.push(b); stack.push(a);

  /** Sorts a dynamic array by the quicksort method without recursion.

      quicksort(a) sorts the dynamic array according to the method
      quicksort according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      This version of quicksort takes up maximum space of \f$O(\lg n)\f$.

      Quicksort is a probabilistic method. In a very bad case
      -very bad luck- can be degraded to
      \f$O(n^2)\f$. To alleviate, to the extent of luck, the bad
      cases, use quicksort_insertion() which runs heuristics to
      alleviate the bad cases and invoke the insert method to
      small array partitions.

      @param[in,out] to the array to sort.
      @see selection_sort() insertion_sort() heapsort()
      @see quicksort_rec_min() quicksort_insertion() quicksort_rec()

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  void quicksort(DynArray<T> & a, Compare cmp = Compare()) noexcept
  {
    long i, l = 0, r = a.size() -1;

    FixedStack<long> stack(40);

    push2(stack, l, r);

    while (not stack.is_empty())
      {
        l = stack.pop(); r = stack.pop();

        if (r <= l)
          continue;

        i = partition(a, l, r, cmp);

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

  /** Generic search in a dynamic array of an extreme element.

      search_extreme(a, l, r) searches the array sequentially
      a, between the indices l and r, the extreme, minimum or element
      maximum according to the comparison criterion Compare.

      @param[in] to the array on which to search.
      @param[in] the starting index of the search.
      @param[in] r search term index.
      @return containing index of the endpoint.
      @note There is no checking if the stream is empty.
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  long search_extreme(const DynArray<T>& a, const long l, const long r,
                      Compare cmp = Compare())
  {
    long extreme_index = l;

    for (long i = l + 1; i <= r; i++)
      if (cmp(a(i), a(extreme_index)))
        extreme_index = i;

    return extreme_index;
  }

  /** Returns the maximum element of the array a between l and r.

      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  long search_max(const DynArray<T>& a, const long l, const long r,
                  Compare cmp = Compare())
  {
    return search_extreme<T, Compare>(a, l, r, cmp);
  }

  /**Binary search on a dynamic ordered array.

      binary_search<T,Compare>(a,x,l,r) performs search for x
      in the arrangement a comprised between the lower limits l and
      upper r.

      The routine is generic and uses two type parameters:
      -# The type of data that the array contains.
      -# The comparison class.

      The routine uses the binary search algorithm, which requires
      that the arrangement is orderly. This condition is not verified in
      the algorithm.

      The method always returns an integer between
      [l..r]. If the element is found, then the return value
      It is the index where it is located; otherwise, it returns
      the index where x would be inserted so that the array was
      tidy.

      @warning The routine does not fail if the array is empty, but
      be careful about accessing it in this situation

      @param[in] to the dynamic array on which to perform the search.
      @param[in] x the element to search for.
      @param[in] the left search index.
      @param[in] r search right index.
      @return index of the element where x is located or the index of
      the position where x would be inserted.
      @see sequential_search()
      \ingroup Algos
  */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  long binary_search(const C<T> & a, const T & x, long l, long r,
                     Compare cmp = Compare()) noexcept
  {
    if (l > r)
      return l;

    long m;
    while (l <= r)
      {
        m = (l + r)/2;
        if (cmp(x, a(m)))
          r = m -1;
        else if (cmp(a(m), x))
          l = m + 1;
        else
          return m; // key found
      }

    return m;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  long binary_search(const C<T*> & a, const T & x, long l, long r,
                     Compare cmp = Compare()) noexcept
  {
    if (l > r)
      return l;

    long m;
    while (l <= r)
      {
        m = (l + r)/2;
        if (cmp(x, *a(m)))
          r = m - 1;
        else if (cmp(*a(m), x))
          l = m + 1;
        else
          return m; // found key
      }

    return m;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  long binary_search(const C<T*> & a, const T & x, Compare && cmp = Compare())
    noexcept
  {
    return binary_search(a, x, 0, a.size() - 1, cmp);
  }

  /** Binary search on a dynamic ordered array.

      binary_search(a,x) searches for x
      in the array a between 0 and <code>size()</code>.

      The routine uses the binary search algorithm, which requires
      that the arrangement is orderly. This condition is not verified in
      the algorithm.

      The method always returns an integer between
      [0..size()). If the element is found, then the value of
      return is the index where this is located; otherwise,
      the index where x would be inserted is returned so that the array
      was ordered.

      @warning The routine does not fail if the array is empty, but
      be careful about accessing it in this situation

      @param[in] to the dynamic array on which to perform the search.
      @param[in] x the element to search for.
      @return index of the element where x is located or the index of
      the position where x would be inserted.
      @see sequential_search()
      \ingroup Algos
  */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  long binary_search(const C<T> & a, const T & x, Compare cmp = Compare()) noexcept
  {
    return binary_search(a, x, 0, a.size() - 1, cmp);
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  DynList<size_t> binary_search_dup(const C<T> & a, const T & x,
                                    Compare cmp = Compare()) noexcept
  {
    DynList<size_t> ret;
    long idx = binary_search(a, x, 0, a.size() - 1, cmp);
    if (idx < 0)
      return ret;

    if (not are_equals(a(idx), x, cmp))
      return ret;;

    ret.append(idx);
    for (long i = idx - 1; i >= 0; --i)
      {
        if (not are_equals(a(i), x, cmp))
          break;
        ret.insert(i);
      }

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        if (not are_equals(a(i), x, cmp))
          break;
        ret.append(i);
      }

    return ret;
  }

  template <template <typename> class C, typename T, 
            class Compare = Aleph::less<T>> inline
  T * bsearch(const C<T> & a, const T & x, Compare cmp = Compare()) noexcept
  {
    long i = binary_search(a, x, cmp);
    if (i < 0)
      return nullptr;
    T * ptr = &a(i);
    return are_equals(*ptr, x, cmp) ? ptr : nullptr;
  }

  template <template <typename> class C, typename T, 
            class Compare = Aleph::less<T>> inline
  T * bsearch(const C<T*> & a, const T & x, Compare cmp = Compare()) noexcept
  {
    long i = binary_search(a, x, cmp);
    if (i < 0)
      return nullptr;
    T * ptr = a(i);
    return are_equals(*ptr, x, cmp) ? ptr : nullptr;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  DynList<T*> bsearch_dup(const C<T> & a, const T & x, Compare cmp = Compare())
    noexcept
  {
    DynList<T*> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    T * found_ptr = const_cast<T*>(&a(idx));
    if (not are_equals(*found_ptr, x, cmp))
      return ret;;

    for (long i = idx - 1; i >= 0; --i)
      {
        T * ptr = const_cast<T*>(&a(i));
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(ptr);
      }

    ret.append(found_ptr);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        T * ptr = const_cast<T*>(&a(i));
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(ptr);
      }

    return ret;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  DynList<T*> bsearch_dup(const C<T*> & a, const T & x, Compare cmp = Compare())
    noexcept
  {
    DynList<T*> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    T * found_ptr = a(idx);
    if (not are_equals(*found_ptr, x, cmp))
      return ret;;

    for (long i = idx - 1; i >= 0; --i)
      {
        T * ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(ptr);
      }

    ret.append(found_ptr);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        T * ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(ptr);
      }

    return ret;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  long binindex(const C<T> & a, const T & x, Compare cmp = Compare()) noexcept
  {
    return binary_search(a, x, cmp);
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  DynList<long> binindex_dup(const C<T> & a, const T & x, Compare cmp = Compare())
    noexcept
  {
    DynList<long> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    T * ptr = const_cast<T*>(&a(idx));
    if (not are_equals(*ptr, x, cmp))
      return ret;;

    long mid = idx;

    for (long i = idx - 1; i >= 0; --i)
      {
        ptr = const_cast<T*>(&a(i));
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(i);
      }

    ret.append(mid);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        ptr = const_cast<T*>(&a(i));
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(i);
      }

    return ret;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  DynList<long> binindex_dup(const C<T*> & a, const T & x, Compare cmp = Compare())
    noexcept
  {
    DynList<long> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    T * ptr = a(idx);
    if (not are_equals(*ptr, x, cmp))
      return ret;;

    for (long i = idx - 1; i >= 0; --i)
      {
        ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(i);
      }

    ret.append(ptr);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        T * ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(i);
      }

    return ret;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  DynArray<size_t> build_index(const C<T> & a, Compare cmp = Compare())
  {
    const size_t & n = a.size();
    DynArray<size_t> ret; ret.reserve(a.size());
    for (size_t i = 0; i < n; ++i)
      ret(i) = i;

    quicksort_op(ret,
                 [&a, &cmp] (size_t i, size_t j) { return cmp(a(i), a(j)); });

    return ret;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  DynArray<T*> build_index_ptr(const C<T> & a, Compare cmp = Compare())
  {
    const size_t & n = a.size();
    DynArray<T*> ret; ret.reserve(a.size());
    for (size_t i = 0; i < n; ++i)
      ret(i) = &a(i);

    quicksort_op(ret, [&cmp] (const T * ptr1, const T * ptr2)
                 { return cmp(*ptr1, *ptr2); });

    return ret;
  }

  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>> inline
  void quicksort_op(C<T> & a, Compare cmp = Compare(),
                    size_t threshold = Quicksort_Threshold) noexcept
  {
    size_t i, l = 0, r = a.size() -1;

    FixedStack<int> stack(40);

    push2(stack, l, r);

    while (not stack.is_empty())
      {
        l = stack.pop(); r = stack.pop();

        size_t partition_size = r - l + 1;
        if (partition_size <= 1)
          continue;

        if (partition_size <= threshold)
          {
            insertion_sort(a, l, r, cmp);
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

# undef push2

  /** Recursive binary search on an ordered array.

      binary_search_rec<T,Compare>(a,x,l,r) performs search for x
      in the arrangement a comprised between the lower limits l and
      upper r.

      The routine is generic and uses two type parameters:
      -# The type of data that the array contains.
      -# The comparison class.

      The routine uses the binary search algorithm, which requires
      that the arrangement is orderly. This condition is not verified in
      the algorithm.

      The method always returns an integer between
      [l..r]. If the element is found, then the return value
      It is the index where it is located; otherwise, it returns
      the index where x would be inserted so that the array was
      tidy.

      @param[in] to the array on which to search.
      @param[in] x the element to search for.
      @param[in] the left search index.
      @param[in] r search right index.
      @return index of the element where x is located or the index of
      the position where x would be inserted.
      @see sequential_search()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline
  int binary_search_rec(T * a, const T & x, const long l, const long r,
                        Compare cmp = Compare()) noexcept
  {
    const long m = (l + r) / 2;
    if (l > r)
      return m;

    if (cmp(x, a[m]))
      return binary_search_rec<T, Compare>(a, l, m - 1, cmp);
    else if (cmp(a[m], x))
      return binary_search_rec<T, Compare>(a, m + 1, r, cmp);

    return m; // encontrado
  }

  /** Iterative binary search over an ordered array.

      binary_search<T,Compare>(a,x,l,r) performs search for x
      in the arrangement a comprised between the lower limits l and
      upper r.

      The routine is generic and uses two type parameters:
      -# The type of data that the array contains.
      -# The comparison class.

      The routine uses the binary search algorithm, which requires
      that the arrangement is orderly. This condition is not verified in
      the algorithm.

      The method always returns an integer between
      [l..r]. If the element is found, then the return value
      It is the index where it is located; otherwise, it returns
      the index where x would be inserted so that the array was
      tidy.

      @param[in] to the array on which to search.
      @param[in] x the element to search for.
      @param[in] the left search index.
      @param[in] r search right index.
      @return index of the element where x is located or the index of
      the position where x would be inserted.
      @see sequential_search()
      \ingroup Algos
  */
  template <typename T, class Compare = Aleph::less<T>> inline 
  long binary_search(T * a, const T & x, long l, long r,
                     Compare cmp = Compare()) noexcept
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

}

# endif // TPL_SORT_UTILS_H
