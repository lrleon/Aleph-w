
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
  @file ahFunctional.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AH_FUNCTIONAL_H
# define AH_FUNCTIONAL_H


    /* Workaround until gmp libaries get updated */

//# include <stddef.h>
# include <stdexcept>
# include <algorithm>
# include <functional>

    //    /***************************************
    //    Definitions of Aleph functional routines 
    //    ***************************************/

namespace Aleph
{
  template <typename T> struct Found_Item 
  {
    virtual T & get_item() = 0;
  };

  template <typename T> struct None : public Found_Item<T> 
  {
    virtual T & get_item()
    {
      throw std::invalid_argument("Access from None type");
    }
  };

  template <typename T> struct Some : public Found_Item<T>
  {
    T & item;

    Some(T & i) : item(i) {}

    virtual T & get_item() { return item; }
  };

template <typename tgtT, typename srcT>
  struct Dft_Map_Op
  {
    const tgtT & operator () (const srcT & item) { return item; }
  };


template <typename T> class DynList;

  template <class Container, class Operation> inline
void for_each(Container & container, Operation & operation)
{
  container.traverse([&operation] (const typename Container::Item_Type & item)
     { 
       operation(item);
       return true;
     });
}

  template <class Container, class Operation> inline
void for_each(const Container & container, Operation & operation)
{
  container.traverse ([&operation] (const typename Container::Item_Type & item)
     { 
       operation(item);
       return true;
     });
}

  template <class Container, class Operation> inline
void for_each(Container & container, Operation && operation = Operation())
{
  for_each<Container, Operation>(container, operation);
}

  template <class Container, class Operation> inline
void for_each(const Container & container, Operation && operation = Operation())
{
  for_each<Container, Operation>(container, operation);
}


template <class Container, class Operation> inline
bool all(Container & container, Operation & operation)
{
  return container.template traverse<Operation>(operation);
}

template <class Container, class Operation> inline
bool all(const Container & container, Operation & operation)
{
  return container.template traverse<Operation>(operation);
}

template <class Container, class Operation> inline
bool all(Container & container, Operation && operation = Operation())
{
  return all<Container, Operation>(container, operation);
}

template <class Container, class Operation> inline
bool all(const Container & container, Operation && operation = Operation())
{
  return all<Container, Operation>(container, operation);
}


template <class Container, class Operation> inline
bool exists(Container & container, Operation & operation)
{
  return not container.traverse([&operation] 
				(const typename Container::Item_Type & item)
				{
				  return not operation(item);
				});
}

template <class Container, class Operation> inline
bool exists(const Container & container, Operation & operation)
{
  return not container.traverse([&operation] 
				(const typename Container::Item_Type & item)
				{
				  return not operation(item);
				});
}

template <class Container, class Operation> inline
bool exists(Container & container, Operation && operation = Operation())
{
  return exists<Container, Operation>(container, operation);
}

template <class Container, class Operation> inline
bool exists(const Container & container, Operation && operation = Operation())
{
  return exists<Container, Operation>(container, operation);
}

template <typename T>
  struct Dft_Filter_Op
  {
    bool operator () (const T &) const { return true; }
  };


template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
Container2<typename Container1::Item_Type> 
filter(Container1 & container, Operation & operation)
{
  Container2<typename Container1::Item_Type> ret_val;
  container.
  for_each([&ret_val, &operation] (const typename Container1::Item_Type & item)
	    {
	     if (operation(item))
	       ret_val.append(item);
	   });
  return ret_val;
}

template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
Container2<typename Container1::Item_Type> 
filter(const Container1 & container, Operation & operation)
{
  Container2<typename Container1::Item_Type> ret_val;
  container.for_each
  ([&ret_val, &operation] (const typename Container1::Item_Type & item)
	    {
	     if (operation(item))
	       ret_val.append(item);
	   });
  return ret_val;
}

template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
Container2<typename Container1::Item_Type> 
filter(Container1 & container, Operation && operation)
{
  return filter<Container1, Container2, Operation>(container, operation);
}


template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
Container2<typename Container1::Item_Type> 
filter(const Container1 & container, Operation && operation)
{
  return filter<Container1, Container2, Operation>(container, operation);
}

/** 
  @brief This is the classic map in a sequence

  @return Returns a new container with each element of the container
  assigned to the result of the operation

  @note The name has been changed due to clashes
  the stl , Aleph and std containers.

  @ingroup Algos
*/
template <typename                  Ta,
	  template <typename> class Ca, 
	  typename                  Tb = Ta,
	  template <typename> class Cb = Ca> inline
Cb<Tb> map_items(const Ca<Ta> & ca, std::function<Tb(const Ta &)> operation)
{
  Cb<Tb> ret_val;
  for (typename Ca<Ta>::Iterator it(ca); it.has_curr(); it.next())
    ret_val.append(operation(it.get_curr()));

  return ret_val;
}

/** 
  @brief Classic foldl

  @example Let f = operation, 
  init an initial value and container
  ={x1,x2,... xn}. 
  foldl returns the result of to execute:
  f(f(xn,..., f(x2, f(x1, init))....))

  @ingroup Algos
*/
template <template <typename> class Container, 
	  typename                  T, 
	  class                     Operation> inline
T foldl(const Container<T> & container, const T & init, Operation & operation)
{
  T ret_val = init;
  for (typename Container<T>::Iterator it(container); it.has_curr(); it.next())
    ret_val = operation(ret_val, it.get_curr());

  return ret_val;
}


template <template <typename> class Container, 
	  typename                  T, 
	  class                     Operation> inline
T foldl(const Container<T> & container, 
	const T &            init      = T(),
	Operation &&         operation = Operation())
{
  return foldl(container, operation, init) ;
}


template <class Container1, class Container2, 
	  template <typename> class Container = DynList> inline
Container<std::pair<typename Container1::Item_Type, 
		    typename Container2::Item_Type>> 
zip(const Container1 & a, const Container2 & b)
{
  typedef typename Container1::Item_Type T1;
  typedef typename Container2::Item_Type T2;
  Container<std::pair<T1, T2>> ret_val;
  
  typename Container1::Iterator it1(a);
  typename Container2::Iterator it2(b);
  for (/* Nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    ret_val.append(std::pair<T1, T2>(it1.get_curr(), it2.get_curr()));

  return ret_val;
}

template <class Container1, class Container2, 
	  template <typename> class Container = DynList> inline
Container<std::pair<typename Container1::Item_Type, 
		    typename Container2::Item_Type>> 
zipEq(const Container1 & a, const Container2 & b)
{
  typedef typename Container1::Item_Type T1;
  typedef typename Container2::Item_Type T2;
  Container<std::pair<T1, T2>> ret_val;
  
  typename Container1::Iterator it1(a);
  typename Container2::Iterator it2(b);
  for (/* Nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    ret_val.append(std::pair<T1, T2>(it1.get_curr(), it2.get_curr()));

  if (it1.has_curr() or it2.has_curr())
    throw std::length_error("Container sizes mismatch");

  return ret_val;
}

template <class Container>
DynList<std::pair<typename Container::Item_Type, size_t>>
enumerate(const Container & c)
{
  using Item = typename Container::Item_Type;
  using Pair = std::pair<Item, size_t>;
  DynList<Pair> ret;
  size_t i = 0;
  c.for_each([&i, &ret] (const Item & item) { ret.append(Pair(item, i++)); });
  return ret;
}


template <typename T, template <typename> class Container = DynList>
bool eq(const Container<T> & c1, const Container<T> & c2)
{
  typename Container<T>::Iterator it1(c1);
  typename Container<T>::Iterator it2(c2);
  for (/* Nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    if (not (it1.get_curr() == it2.get_curr()))
      return false;

  return not (it1.has_curr() or it2.has_curr());
}

template <typename T, template <typename> class Container = DynList>
bool lesser(const Container<T> & c1, const Container<T> & c2)
{
    /*
    This is an alternative implentation that could be slower on trees 
     typename Container<T>::Iterator it1(c1);
     typename Container<T>::Iterator it2(c2);
     for ( ; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
       {
         const T & item1 = it1.get_curr();
         const T & item2 = it2.get_curr();
         if (item1 == item2)
	    continue;
         else
	    return item1 < item2;
        }
      return not it1.has_curr();
    */
  try
    {
      bool ret = false;
      zipEq(c1, c2).traverse([&ret] (const std::pair<T,T> & p) 
			     {
			       if (p.first == p.second)
				 return true; /* Advance to next pair */

			       ret = p.first < p.second;
			       return false; /* Stop traverse */
			     });
      return ret;
    }
  catch (std::length_error) /* If flow reaches this point ==> all items equal
                               so untie by size */
    {                          
      return c1.size() < c2.size(); 
    }
}

template <typename T, template <typename> class Container = DynList>
bool diff(const Container<T> & c1, const Container<T> & c2)
{
  return not eq<T, Container>(c1, c2);
}

template <template <typename> class Container, typename T1, typename T2>
inline std::pair<Container<T1>, Container<T2>>
		   unzip(const Container<std::pair<T1, T2>> & l)
{
  Container<T1> l1;
  Container<T2> l2;

  for (typename Container<std::pair<T1, T2>>::Iterator it(l); it.has_curr();
	 it.next())
    {
      std::pair<T1, T2> curr = it.get_curr();
      l1.append(curr.first);
      l2.append(curr.second);
    }

  return std::pair<Container<T1>, Container<T2>>(std::move(l1), std::move(l2));
}


template <class SrcContainer, 
	  template <typename> class TgtContainer = Aleph::DynList> inline
std::pair<TgtContainer<typename SrcContainer::Item_Type>,
	  TgtContainer<typename SrcContainer::Item_Type>> partition
  (const SrcContainer & c, 
   std::function<bool(const typename SrcContainer::Item_Type &)> operation)
{
  typedef typename SrcContainer::Item_Type Type;
  typedef std::pair<TgtContainer<Type>, TgtContainer<Type>> Pair;
      
  Pair ret_val;
  for_each(c, [&ret_val, &operation] (const Type & item)
	   {
	     if (operation(item))
	       ret_val.first.append(item);
	     else
	       ret_val.second.append(item);
	   });
  return ret_val;
}


template <template <typename> class Container = DynList, typename T = int>
Container<T> range(const T start, const T end, const T step = 1)
{
  Container<T> ret_val;
  for (T i = start; i <= end; i += step)
    ret_val.append(i);
  return ret_val;
}

template <template <typename> class Container = DynList, typename T = int>
Container<T> range(const T n)
{
  Container<T> ret_val;
  for (T i = 0; i < n; ++i)
    ret_val.append(i);
  return ret_val;
}

/** 
  @brief Create a sequence of repated items

  @param[in] n Number of items
  @param[in] item The valur to be repeated
  
  @return A container with n instances of item value
      
  @ingroup Funcional
*/
  template <template <typename> class Container = DynList, typename T = int>
Container<T> rep(size_t n, const T & item)
{
  Container<T> ret_val;
  for (T i = 0; i < n; ++i)
    ret_val.append(item);
  return ret_val;
}

  template <template <typename> class Container = DynList, typename T = int>
Container<T> rep(size_t n, T && item = T())
{
  return rep<Container, T>(n, item);
}


template <typename T, template <typename> class Container>
DynList<std::pair<size_t, T>> indexes(const Container<T> & c)
{
  using Pair = std::pair<size_t, T>;
  size_t i = 0;
  return c.template map<Pair>([&i] (const T & d) { return Pair(i++, d); });
}


template <typename T, template <typename> class Container>
Container<T> reverse(const Container<T> & l)
{
  Container<T> ret_val;
  l.for_each([&ret_val] (const T & item)
	     {
	       ret_val.insert(item);
	     });
  return ret_val;
}

    /* Union */
template <typename T, template <typename> class Container>
DynList<T> join(const Container<T> & c1, const Container<T> & c2);

template <typename T, template <typename> class Container>
DynList<T> intercept(const Container<T> & c1, const Container<T> & c2);

				      
} /* End namespace Aleph */

# endif // AH_FUNCTIONAL_H
