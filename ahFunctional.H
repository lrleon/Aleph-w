# ifndef AH_FUNCTIONAL_H
# define AH_FUNCTIONAL_H

# include <stdexcept>
# include <utility>
# include <tuple>
# include <functional>
# include <algorithm>

/* Definitions of Aleph functional routines */

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
    const tgtT & operator () (const srcT & item) noexcept { return item; }
  };

template <typename TR, typename TD>
  struct Dft_Fold_Op
  {
    const TR operator () (const TR & /*acc */, const TD & /* val */) 
      const noexcept { return TR(); }
  };


template <typename T> class DynList;

template <class Container>
DynList<typename Container::Item_Type*> pointers_list(Container & c)
{
  DynList<typename Container::Item_Type*> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next())
    ret.append(&it.get_curr());
  return ret;
}

template <class Container>
DynList<const typename Container::Item_Type * const>
pointers_list(const Container & c)
{
  using T = const typename  Container::Item_Type * const;
  DynList<T> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next())
    ret.append(const_cast<T>(&it.get_curr()));
  return ret;
}

template <class Container>
DynList<typename Container::Item_Type>
sublist(const Container & c, size_t stride)
{
  DynList<typename Container::Item_Type> ret;
  try
    {
      for (auto it = c.get_it(stride - 1); it.has_curr(); )
	{
	  ret.append(it.get_curr());
	  for (size_t i = 0; i < stride; ++i)
	    it.next();
	}
    }
  catch (std::overflow_error) { /* nothing */ }

  return ret;
}

  template <class Op>
void each(size_t start, size_t end, Op & op)
{
  for (size_t i = start; i <= end; ++i)
    op();
}

    template <class Op>
void each(size_t start, size_t end, Op && op)
{
  each(start, end, op);
}

    template <class Op>
void each(size_t n, Op & op)
{
  each(0, n - 1, op);
}

    template <class Op>
void each(size_t n, Op && op)
{
  each(n, op);
}

    template <class Container, class Operation> inline
Container & for_each(Container & container, Operation & operation)
    noexcept(noexcept(operation))
{
  container.traverse([&operation] (const auto & item)
     { 
       operation(item);
       return true;
     });
  return container;
}

  template <class Container, class Operation> inline
const Container & for_each(const Container & container, Operation & operation)
    noexcept(noexcept(operation))
{
  container.traverse([&operation] (const auto & item)
     { 
       operation(item);
       return true;
     });
  return container;
}

  template <class Container, class Operation> inline
Container& for_each(Container & container, Operation && operation = Operation())
    noexcept(noexcept(operation))
{
  return for_each<Container, Operation>(container, operation);
}

  template <class Container, class Operation> inline const Container &
for_each(const Container & container, Operation && operation = Operation())
    noexcept(noexcept(operation))
{
  return for_each<Container, Operation>(container, operation);
}

  template <class Container, class Operation> inline
void enum_for_each(const Container & container, Operation & operation)
    noexcept(noexcept(operation))
{
  size_t i = 0;
  for (auto it = container.get_it(); it.has_curr(); it.next(), ++i)
    operation(it.get_curr(), i);
}

  template <class Container, class Operation> inline
void enum_for_each(const Container & container, Operation && operation)
    noexcept(noexcept(operation))
{
  enum_for_each(container, operation);
}

template <class Container, class Operation> inline
bool all(Container & container, Operation & operation)
  noexcept(noexcept(operation))
{
  return container.template traverse(operation);
}

template <class Container, class Operation> inline
bool all(const Container & container, Operation & operation)
  noexcept(noexcept(operation))
{
  return container.template traverse<Operation>(operation);
}

template <class Container, class Operation> inline
bool all(Container & container, Operation && operation = Operation())
  noexcept(noexcept(operation))
{
  return all<Container, Operation>(container, operation);
}

template <class Container, class Operation> inline
bool all(const Container & container, Operation && operation = Operation())
  noexcept(noexcept(operation))
{
  return all<Container, Operation>(container, operation);
}


template <class Container, class Operation> inline
bool exists(Container & container, Operation & operation)
  noexcept(noexcept(operation))
{
  return not 
    container.traverse([&operation] (const auto & item)
		       {
			 return not operation(item);
		       });
}

template <class Container, class Operation> inline
bool exists(const Container & container, Operation & operation)
  noexcept(noexcept(operation))
{
  return not 
    container.traverse([&operation] (const auto & item)
		       {
			 return not operation(item);
		       });
}

template <class Container, class Operation> inline
bool exists(Container & container, Operation && operation = Operation())
  noexcept(noexcept(operation))
{
  return exists<Container, Operation>(container, operation);
}

template <class Container, class Operation> inline
bool exists(const Container & container, Operation && operation = Operation())
  noexcept(noexcept(operation))
{
  return exists<Container, Operation>(container, operation);
}

template <typename T>
  struct Dft_Filter_Op
  {
    bool operator () (const T &) const noexcept { return true; }
  };


template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
inline Container2<typename Container1::Item_Type> 
filter(Container1 & container, Operation & operation)
{
  Container2<typename Container1::Item_Type> ret_val;
  container.
  for_each([&ret_val, &operation] (const auto & item)
	    {
	      if (operation(item))
		ret_val.append(item);
	   });
  return ret_val;
}

template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
inline Container2<typename Container1::Item_Type> 
filter(const Container1 & container, Operation & operation)
{
  Container2<typename Container1::Item_Type> ret_val;
  container.for_each
  ([&ret_val, &operation] (const auto & item)
	    {
	      if (operation(item))
		ret_val.append(item);
	   });
  return ret_val;
}

template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
inline Container2<typename Container1::Item_Type> 
filter(Container1 & container, Operation && operation)
{
  return filter<Container1, Container2, Operation>(container, operation);
}


template <class Container1,
	  template <typename> class Container2 = Aleph::DynList,
	  class Operation = Dft_Filter_Op<typename Container1::Item_Type>>
Container2<typename Container1::Item_Type> 
inline filter(const Container1 & container, Operation && operation)
{
  return filter<Container1, Container2, Operation>(container, operation);
}

/** This is the classic map on a sequence. 

    Return a new container with each element of container mapped to the
    result of operation(item)

    @note The name has been changed because clashes with the stl and
    Aleph and std map containers. 
    @ingroup Algos
*/
template <typename                  Ta,
	  template <typename> class Ca, 
	  typename                  Tb = Ta,
	  template <typename> class Cb = Ca> inline
Cb<Tb> maps(const Ca<Ta> & ca, std::function<Tb(const Ta &)> operation)
{
  Cb<Tb> ret_val;
  for (typename Ca<Ta>::Iterator it(ca); it.has_curr(); it.next())
    ret_val.append(operation(it.get_curr()));

  return ret_val;
}

/** Classic foldl

    Let f = operation, init an initial value and container={x1,x2,
    ... xn}. foldl returns the result of to execute:

    f(f(xn, ..., f(x2, f(x1, init))....))

    @ingroup Algos
 */
template <template <typename> class Container, 
	  typename                  T, 
	  class                     Operation> inline
T foldl(const Container<T> & container, const T & init, Operation & operation)
  noexcept(noexcept(operation) and std::is_nothrow_copy_assignable<T>::value)
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
  noexcept(noexcept(operation) and std::is_nothrow_copy_assignable<T>::value)
{
  return foldl(container, operation, init) ;
}


  template <class Container1, class Container2> inline
DynList<std::pair<typename Container1::Item_Type, 
		  typename Container2::Item_Type>> 
zip(const Container1 & a, const Container2 & b)
{
  typedef typename Container1::Item_Type T1;
  typedef typename Container2::Item_Type T2;
  DynList<std::pair<T1, T2>> ret_val;
  
  typename Container1::Iterator it1(a);
  typename Container2::Iterator it2(b);
  for (/* nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    ret_val.append(std::pair<T1, T2>(it1.get_curr(), it2.get_curr()));

  return ret_val;
}

template <class Container1, class Container2> inline
DynList<std::tuple<typename Container1::Item_Type, 
		   typename Container2::Item_Type>> 
tzip(const Container1 & a, const Container2 & b)
{
  typedef typename Container1::Item_Type T1;
  typedef typename Container2::Item_Type T2;
  using Tuple = std::tuple<T1, T2>;
  DynList<Tuple> ret_val;
  
  typename Container1::Iterator it1(a);
  typename Container2::Iterator it2(b);
  for (/* nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    ret_val.append(Tuple(it1.get_curr(), it2.get_curr()));

  return ret_val;
}

template <class Container1, class Container2> inline
DynList<std::pair<typename Container1::Item_Type, 
		  typename Container2::Item_Type>> 
zipEq(const Container1 & a, const Container2 & b)
{

  typedef typename Container1::Item_Type T1;
  typedef typename Container2::Item_Type T2;
  DynList<std::pair<T1, T2>> ret_val;
  
  auto it1 = a.get_it();
  auto it2 = b.get_it();
  for (/* nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    ret_val.append(std::pair<T1, T2>(it1.get_curr(), it2.get_curr()));

  if (it1.has_curr() or it2.has_curr())
    throw std::length_error("Container sizes mismatch");

  return ret_val;
}

template <class Container1, class Container2> inline
DynList<std::tuple<typename Container1::Item_Type, 
		   typename Container2::Item_Type>> 
tzipEq(const Container1 & a, const Container2 & b)
{
  typedef typename Container1::Item_Type T1;
  typedef typename Container2::Item_Type T2;
  using Tuple = std::tuple<T1, T2>;
  DynList<Tuple> ret_val;
  
  typename Container1::Iterator it1(a);
  typename Container2::Iterator it2(b);
  for (/* nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    ret_val.append(Tuple(it1.get_curr(), it2.get_curr()));

  if (it1.has_curr() or it2.has_curr())
    throw std::length_error("Container sizes mismatch");

  return ret_val;
}

  template <class C1, class C2, class C3> inline
DynList<std::tuple<typename C1::Item_Type, 
		   typename C2::Item_Type,
		   typename C3::Item_Type>> 
  tzip(const C1 & c1, const C2 & c2, const C3 & c3)
{
  typedef typename C1::Item_Type T1;
  typedef typename C2::Item_Type T2;
  typedef typename C3::Item_Type T3;
  DynList<std::tuple<T1, T2, T3>> ret_val;
  
  typename C1::Iterator it1(c1);
  typename C2::Iterator it2(c2);
  typename C3::Iterator it3(c3);
  for (; it1.has_curr() and it2.has_curr() and it3.has_curr();
       it1.next(), it2.next(), it3.next())
    ret_val.append(std::make_tuple(it1.get_curr(),
				   it2.get_curr(),
				   it3.get_curr()));

  return ret_val;
}

  template <class C1, class C2, class C3> inline
DynList<std::tuple<typename C1::Item_Type, 
		   typename C2::Item_Type,
		   typename C3::Item_Type>> 
    tzipEq(const C1 & c1, const C2 & c2, const C3 & c3)
{
  typedef typename C1::Item_Type T1;
  typedef typename C2::Item_Type T2;
  typedef typename C3::Item_Type T3;
  DynList<std::tuple<T1, T2, T3>> ret_val;
  
  typename C1::Iterator it1(c1);
  typename C2::Iterator it2(c2);
  typename C2::Iterator it3(c3);
  for (; it1.has_curr() and it2.has_curr() and it3.has_curr();
       it1.next(), it2.next(), it3.next())
    ret_val.append(std::make_tuple(it1.get_curr(), it2.get_curr(),
				   it3.get_curr()));

  if (it1.has_curr() or it2.has_curr() or it3.has_curr())
    throw std::length_error("Container sizes mismatch");

  return ret_val;
}


template <class Container>
DynList<std::pair<typename Container::Item_Type, size_t>> inline
enumerate(const Container & c)
{
  using Item = typename Container::Item_Type;
  using Pair = std::pair<Item, size_t>;
  DynList<Pair> ret;
  size_t i = 0;
  c.for_each([&i, &ret] (const Item & item) { ret.append(Pair(item, i++)); });
  return ret;
}


  template <typename T, template <typename> class Container = DynList,
	    class Eq = std::equal_to<T>> inline 
bool eq(const Container<T> & c1, const Container<T> & c2, Eq & e)
{
  typename Container<T>::Iterator it1(c1), it2(c2);
  for (/* nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    if (not (e(it1.get_curr(), it2.get_curr())))
      return false;

  return not (it1.has_curr() or it2.has_curr());
}

  template <typename T, template <typename> class Container = DynList,
	    class Eq = std::equal_to<T>> inline 
bool eq(const Container<T> & c1, const Container<T> & c2, Eq && e = Eq())
{
  return eq<T, Container, Eq>(c1, c2, e);
}

  template <class Container, class Eq> inline 
bool containers_eq(const Container & c1, const Container & c2, Eq && e)
{
  typename Container::Iterator it1(c1), it2(c2);
  for (/* nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    if (not (e(it1.get_curr(), it2.get_curr())))
      return false;

  return not (it1.has_curr() or it2.has_curr());
}


  template <typename T, template <typename> class Container = DynList,
	    class Eq = std::equal_to<T>> inline 
    std::tuple<bool, size_t, T, T>
are_eq(const Container<T> & c1, const Container<T> & c2, Eq && e = Eq())
{
  typename Container<T>::Iterator it1(c1), it2(c2);
  size_t n = 0;
  for (; it1.has_curr() and it2.has_curr(); it1.next(), it2.next(), n++)
    {
      auto i1 = it1.get_curr();
      auto i2 = it2.get_curr();
      if (not (e(i1, i2)))
	return std::make_tuple(false, n, i1, i2);
    }

  return std::make_tuple(c1.is_empty() == c2.is_empty(), n, T(), T());
}


  template <typename T, template <typename> class Container = Aleph::DynList, 
	    class Cmp = std::less<T>> inline
bool lesser(const Container<T> & c1, const Container<T> & c2, Cmp cmp = Cmp())
{
  typename Container<T>::Iterator it1(c1), it2(c2);
  for (/* nothing */; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
    if (cmp(it1.get_curr(), it2.get_curr()))
      return true;
    else if (cmp(it2.get_curr(), it1.get_curr()))
      return false;

  if (not it1.has_curr() and not it2.has_curr())
    return false;

  return it2.has_curr();
}

template <typename T, template <typename> class Container = DynList,
	  class Eq = std::equal_to<T>> inline
bool diff(const Container<T> & c1, const Container<T> & c2, Eq & e)
{
  return not eq<T, Container, Eq>(c1, c2, e);
}

template <typename T, template <typename> class Container = DynList,
	  class Eq = std::equal_to<T>> inline
bool diff(const Container<T> & c1, const Container<T> & c2, Eq && e = Eq())
{
  return diff<T, Container, Eq>(c1, c2, e);
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

template <template <typename> class Container, typename T1, typename T2>
inline std::tuple<Container<T1>, Container<T2>> 
tunzip(const Container<std::tuple<T1, T2>> & l)
{
  Container<T1> l1;
  Container<T2> l2;
  for (typename Container<std::pair<T1, T2>>::Iterator it(l); it.has_curr();
       it.next())
    {
      auto curr = it.get_curr();
      l1.append(curr.first);
      l2.append(curr.second);
    }

  return std::make_tuple(std::move(l1), std::move(l2));
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

  template <typename T = int, template <typename> class Container = DynList>
inline Container<T> range(const T start, const T end, const T step = 1)
{
  Container<T> ret_val;
  for (T i = start; i <= end; i += step)
    ret_val.append(i);
  return ret_val;
}

    template <typename T = int, template <typename> class Container = DynList>
inline Container<T> nrange(const T start, const T end, const size_t n)
{
  if (n < 2)
    throw std::domain_error("nrange: n must be greater than 1");
  
  const auto step = (end - start)/(n - 1);
  Container<T> ret_val;
  T val = start;
  for (size_t i = 0; i < n; ++i, val += step)
    ret_val.append(val);
  return ret_val;
}

  template <typename T = int, template <typename> class Container = DynList>
inline Container<T> contiguous_range(T start, const size_t n)
{
  Container<T> ret_val;
  for (size_t i = 0; i < n; ++i)
    ret_val.append(start++);
  return ret_val;
}


  /** Return a range 0, 1, ... n - 1

      @param[in] n size of range
      @return a container with the range (according to the template
      parameter, which is DynList

     @ingroup Funcional
   */
template <typename T = int, template <typename> class Container = DynList>
inline Container<T> range(const T n)
{
  Container<T> ret_val;
  for (T i = 0; i < n; ++i)
    ret_val.append(i);
  return ret_val;
}


  /** Create a sequence of repeated items

      @param[in] n number of items
      @param[in] item the valur to be repeated
      @return a container with n instances of item value
      
     @ingroup Funcional
   */
template <typename T = int> inline
DynList<T> rep(size_t n, const T & item)
{
  DynList<T> ret_val;
  for (size_t i = 0; i < n; ++i)
    ret_val.append(item);
  return ret_val;
}

  template <typename T = int> inline
DynList<T> rep(size_t n, T && item = T())
{
  return rep<T>(n, item);
}


template <class Container> inline
DynList<std::pair<size_t, typename Container::Key_Type>>
indexes(const Container & c)
{
  using T = typename Container::Key_Type;
  using Pair = std::pair<size_t, T>;
  size_t i = 0;
  
  return c.Container:: template maps<Pair>([&i] (const T & d) 
					   { return Pair(i++, d); });
}


template <class Container> inline
DynList<std::tuple<size_t, typename Container::Key_Type>> 
tindexes(const Container & c)
{
  using T = typename Container::Key_Type;
  using Tuple = std::tuple<size_t, typename Container::Key_Type>;
  size_t i = 0;
  return c.Container::template maps<std::tuple<size_t, T>>([&i] (const T & d) 
    {
      return Tuple(i++, d); 
    });
}


template <typename T, template <typename> class Container>
inline Container<T> reverse(const Container<T> & l)
{
  Container<T> ret_val;
  l.for_each([&ret_val] (const T & item)
	     {
	       ret_val.insert(item);
	     });
  return ret_val;
}

/** Genera todas las tuplas secuenciales de tamaño contenidas en el
    contenedor c.

    Si por ejemplo c = { 1, 2, 3, 4, 5, 6 ..., n}

    Entonces gen_seq_list_tuples(c, 4) = { (1,2,3,4), (2,3,4,5), ... }

    @param[in] c contenedor a partir del cual, secuencialzado, se desea
    generar las tuplas 
    
    @param[in] n tamaño de cada tupla.
    @return Listas de listas. Cada lista tiene tamañao n y contiene una
    tupla. 

   @ingroup Algos
 */
template <typename T, template <typename> class Container>
DynList<DynList<T>> gen_seq_list_tuples(const Container<T> & c, size_t n)
{
  typename Container<T>::Iterator it(c);
  DynList<T> l;
  for (size_t i = 0; i < n; ++i, it.next())
    l.append(it.get_curr());
	 
  DynList<DynList<T>> ret;
  ret.append(l);
  for (; it.has_curr(); it.next())
    {
      l.remove_first();
      l.append(it.get_curr());
      ret.append(l);
    }

  return ret;
}

  /** Returns the different groups contained in the container `c`.

      For example, if container `c` is `{1, 1, 2, 2, 2, 4, 4, 0, 0, 1}`,
      then 
          
          sequential_groups(c)

      will return a pair whose first item is a list of list as follows:

      `{ {1, 1}, {2, 2, 2}, {4, 4}, {0, 0}, {1} }`

      and the second item is the number of found groups 5.

      @Note In order 

      @ingroup Algos
   */
template <typename T, template <typename> class Container, class Equal>
std::pair<DynList<DynList<T>>, size_t> 
sequential_groups(const Container<T> & c, Equal & eq)
{
  using P = std::pair<DynList<DynList<T>>, size_t>;
  if (c.is_empty())
    return P(DynList<DynList<T> >(), 0);

  DynList<DynList<T>> ret; // this will be the result

  DynList<T> & group = ret.append(DynList<T>()); // creates a first group
  
  auto it = c.get_it();                    // put the firstitem into the group
  auto & curr_item = it.get_curr();
  group.append(curr_item);
  
  size_t count = 1; // count the number of groups

  for (it.next(); it.has_curr(); it.next())
    {
      auto & curr = it.get_curr();
      if (not eq(curr, curr_item)) // group change?
        {
          curr_item = curr;
          group = ret.append(DynList<T>()); // create new group and insert it
          ++count; // increase the number of groups
        }

      group.append(curr);
    }
  
  return P(ret, count);
}

/** \overload sequential_groups(const Container<T> & c, Equal & eq)

      @ingroup Algos
   */
template <typename T, template <typename> class Container, 
	  class Equal = std::equal_to<T>>
std::pair<DynList<DynList<T>>, size_t> 
sequential_groups(const Container<T> & c, Equal && eq = Equal())
{
  return sequential_groups(c, eq);
}

template <typename T, template <typename> class Container, class Equal>
std::pair<DynList<T>, size_t> 
unique_sequential(const Container<T> & c, Equal & eq)
{
  using P = std::pair<DynList<T>, size_t>;
  if (c.is_empty())
    return P(DynList<T>(), 0);

  DynList<T> ret;

  auto it = c.get_it();                    // put the first item
  auto & curr_item = it.get_curr();
  ret.append(curr_item);
  
  size_t count = 1; // count the number of groups

  for (it.next(); it.has_curr(); it.next())
    {
      auto & curr = it.get_curr();
      if (not eq(curr, curr_item)) // group change?
        {
          curr_item = curr;
          ret.append(curr_item);
          ++count; // increase the number of groups
        }
    }
  
  return P(ret, count);
}

template <typename T, template <typename> class Container, 
	  class Equal = std::equal_to<T>>
std::pair<DynList<T>, size_t> 
unique_sequential(const Container<T> & c, Equal && eq = Equal())
{
  return unique_sequential(c, eq);
}

    template <class Itor1, class Itor2 = Itor1>
class Pair_Iterator
{
  Itor1 it1;
  Itor2 it2;

public:

  Pair_Iterator(Itor1 i1, Itor2 i2) : it1(i1), it2(i2) {}

  template <class C1, class C2>
  Pair_Iterator(const C1 & c1, const C2 & c2)
    : Pair_Iterator(c1.get_it(), c2.get.get_it()) {}
  
  bool has_curr() const noexcept { return it1.has_curr() and it2.has_curr(); }

  auto get_curr() const
  {
    return std::make_pair(it1.get_curr(), it2.get_curr());
  }

  void next()
  {
    it1.next();
    it2.next();
  }

  /// return true if the two iterator were completely traversed 
  bool was_traversed() const noexcept
  {
    return not (it1.has_curr() or it2.has_curr());
  }
};

  template <class C1, class C2>
Pair_Iterator<typename C1::Iterator, typename C2::Iterator> inline
get_pair_it(const C1 & c1, const C2 & c2)
{
  using I1 = typename C1::Iterator;
  using I2 = typename C2::Iterator;
  I1 i1(c1);
  I2 i2(c2);
  return Pair_Iterator<I1, I2>(i1, i2);
}

    template <class C1, class C2>
Pair_Iterator<typename C1::Iterator, typename C2::Iterator> inline
    get_pair_it(const C1 & c1, const C2 & c2, size_t pos)
{
  using I1 = typename C1::Iterator;
  using I2 = typename C2::Iterator;
  I1 i1(c1);
  I2 i2(c2);
  for (size_t i = 0; i < pos; ++i)
    {
      i1.next();
      i2.next();
    }
  return Pair_Iterator<I1, I2>(i1, i2);
}

template <class C> inline void insert_in_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
void insert_in_container(C & c, size_t & n, const T & item, Args & ... args)
{
  c.insert(item);
  ++n;
  insert_in_container(c, n, args...);
}

    template <class C, typename ... Args> inline
size_t insert_in_container(C & c, Args ... args)
{
  size_t n = 0;
  insert_in_container(c, n, args...);
  return n;
}

  template <class C, typename ... Args>
C build_container(Args ... args)
{
  C c;
  append_in_container(c, args...);
  return c;
}

  template <class SrcC, class TgtC>
TgtC assign_container(const SrcC & srcc)
{
  TgtC ret;
  for (auto it = srcc.get_it(); it.has_curr(); it.next())
    ret.append(it.get_curr());

  return ret;
}

  template <typename T, typename ... Args>
DynList<T> build_dynlist(Args ... args)
{
  return build_container<DynList<T>>(args...);
}

template <class C> inline void remove_from_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
void remove_from_container(C & c, size_t & n, const T & item, Args & ... args)
{
  c.remove(item);
  ++n;
  remove_from_container(c, n, args...);
}

    template <class C, typename ... Args> inline
size_t remove_from_container(C & c, Args ... args)
{
  size_t n = 0;
  remove_from_container(c, n, args...);
  return n;
}

template <class C> inline void append_in_container(C &, size_t&) {}

  template <class C, typename T, typename ... Args> inline
void append_in_container(C & c, size_t & n, const T & item, Args & ... args)
{
  c.append(item);
  ++n;
  append_in_container(c, n, args...);
}

    template <class C, typename ... Args> inline
size_t append_in_container(C & c, Args ... args)
{
  size_t n = 0;
  append_in_container(c, n, args...);
  return n;
}

// These function are defined in tpl_dynSetHash.H

// union 
template <typename T, template <typename> class Container> inline
DynList<T> join(const Container<T> & c1, const Container<T> & c2);

template <typename T, template <typename> class Container> inline
DynList<T> intercept(const Container<T> & c1, const Container<T> & c2);

template <typename T, template <typename> class Container> inline
DynList<T> unique(const Container<T> & c);

template <typename T, template <typename> class Container> inline
DynList<T> repeated(const Container<T> & c);

template <typename T, template <typename> class Container> inline
DynList<std::pair<T, size_t>> repeated_with_index(const Container<T> & c);

  template <typename T,
	    template <typename> class C1,
	    template <typename> class C2>
DynList<T> flatten(const C2<C1<T>> & c)
{
  DynList<T> ret;
  for (auto it_c = c.get_it(); it_c.has_curr(); it_c.next())
    {
      auto & curr_c = it_c.get_curr();
      for (auto it = curr_c.get_it(); it.has_curr(); it.next())
	ret.append(it.get_curr());
    }
  return ret;
}

  template <typename T,
	    template <typename> class C1,
	    template <typename> class C2,
	    template <typename> class C3>
DynList<T> flatten(const C3<C2<C1<T>>> & c)
{
  DynList<T> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next())
    ret.append(flatten(it.get_curr()));
  return ret;
}

  template <typename T,
	    template <typename> class C1,
	    template <typename> class C2,
	    template <typename> class C3,
	    template <typename> class C4>
DynList<T> flatten(const C4<C3<C2<C1<T>>>> & c)
{
  DynList<T> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next())
    ret.append(flatten(it.get_curr()));
  return ret;
}


} // end namespace Aleph

# endif // AH_FUNCTIONAL_H
