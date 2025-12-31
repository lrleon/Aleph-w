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

/** @file ah-dry.H
 *  @brief Container traversal and functional operation mixins.
 *
 *  This file provides generic traversal classes and mixins that implement
 *  functional operations (for_each, exists, all, filter, map, etc.) for
 *  containers through their iterators. These follow the DRY principle.
 *
 *  @ingroup Utils
 */

# ifndef AH_DRY_H
# define AH_DRY_H

# include "ahFunctional.H"
# include <utility>
# include <ah-errors.H>

/**  Generic traversal of the container through its iterator.

     This class implements a conditioned traversal on the container
     through its iterator.

     It is assumed that `Container` exports its iterator with

     typename Container::Iterator

     \ingroup Secuencias
*/
template <class Container>
struct GenericTraverse
{
private:
  template <class Operation>
  static constexpr bool traverse_is_noexcept() noexcept
  {
    return noexcept(std::declval<Operation&>()(
      std::declval<typename Container::Item_Type&>()));
  }

public:
  /** Traverse the container via its iterator and performs a conditioned
      operation on each item.

      `traverse(operation)` instantiates the internal iterator of the
      class and traverses each item performing `operation(item)`.

      `operation` must have the following signature:

      bool operation(const typename Container::Item_Type & item)

      If `operation(item)` returns `true` then the iterator is advanced
      and the next item processed. Otherwise. the traversal stops.

      \param[in] operation to be performed on each item
      \return `true` if all the items were visited (`operation` on each
      one always returned `true`) or `false` if the traversal was
      stoppep because there was a `false` result on an item.
      \throw anything that could throw `operation`
  */
  template <class Operation>
  bool traverse(Operation & operation) noexcept(traverse_is_noexcept<Operation>())
  {
    for (typename Container::Iterator it(*static_cast<Container*>(this));
         it.has_curr(); it.next())
      if (not operation(it.get_curr()))
        return false;
    return true;
  }

  /// \overload traverse()
  template <class Operation>
  bool traverse(Operation & operation) const noexcept(traverse_is_noexcept<Operation>())
  {
    return const_cast<GenericTraverse*>(this)->traverse<Operation>(operation);
  }

  /// \overload traverse()
  template <class Operation>
  bool traverse(Operation && operation) const noexcept(traverse_is_noexcept<Operation>())
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse()
  template <class Operation>
  bool traverse(Operation && operation) noexcept(traverse_is_noexcept<Operation>())
  {
    return traverse<Operation>(operation);
  }
};

template <class Container, class Operation>
bool traverse(const Container & c, Operation & op)
  noexcept(noexcept(c.traverse(op)))
{
  return c.traverse(op);
}

template <class Container, class Operation>
bool traverse(const Container & c, Operation && op)
  noexcept(noexcept(c.traverse(std::forward<Operation>(op))))
{
  return c.traverse(std::forward<Operation>(op));
}

/** Common sequential searching methods on containers.

    This class implements common sequential searching on containers.

    \note Take in account that any of these searches takes \f$O(n)\f$ of
    complexity for the worst case and that this is independent of type
    of container. For example, a hash table or a binary search tree,
    exports its own search that is much more faster that any of these
    primitives. As an advice: do not use this method inside loops. If
    you find yourself in this situation, then consider to revise your
    design and to use another search method. Use these primitives very
    few times, for algorithms whose complexity is by far greater that
    \f$O(n)\f$ and when you cannot index with an adequate data
    structure.

    \warning Be very careful about the fact that many of these
    primitives return pointers or references to container's data. In
    many cases, alteration of that data will corrupt the internal
    state of container.

    \ingroup Secuencias
*/
template <class Container, typename Type>
class LocateFunctions
{
  Container * me() noexcept { return static_cast<Container*>(this); }

  const Container * const_me() const noexcept
  {
    return static_cast<const Container*>(this);
  }

  LocateFunctions<Container, Type> * base() const
  {
    return const_cast<LocateFunctions*>(this);
  }

  template <class Operation>
  static constexpr bool operation_is_noexcept() noexcept
  {
    return noexcept(std::declval<Operation&>()(std::declval<Type&>()));
  }

public:

  /// Return a properly initialized iterator positioned at the first
  /// item on the container
  auto get_it() const
  {
    return typename Container::Iterator(*const_me());
  }

  // auto get_it()
  // {
  //   return typename Container::Iterator(*me());
  // }

  /// Return a properly initialized iterator positioned at the `pos`
  /// item on the container
  auto get_it(size_t pos) const
  {
    auto ret = typename Container::Iterator(*const_me());
    for (size_t i = 0; i < pos; ++i)
      ret.next();
    return ret;
  }

  /// \overload get_it()
  auto get_itor() const { return get_it(); }

  Type & nth_ne(const size_t n) noexcept
  {
    Type * ptr = nullptr;
    size_t i = 0;
    me()->traverse([&ptr, &i, &n] (Type & item)
                   {
                     if (i++ < n)
                       return true;
                     ptr = &item;
                     return false;
                   });
    return *ptr;
  }

  /// \overload nth()
  const Type & nth_ne(const size_t n) const noexcept
  {
    return base()->nth_ne(n);
  }

  /** Return the n-th item of container.

      The notion of ordinal depends of type of container. On list,
      probably will be the insertion order. On binary search trees will
      be the nth smaller item. On hash tables will be pseudo random.

      \warning Frequent use of this method will definitively degrade the
      performance. Try not to use this method inside loops. In general,
      if you falls in this situation, then consider your design and to
      use an faster approach.

      \param[in] n the nth item to find
      \return a valid reference to the item into the container.
      \throw out_of_range if n is greater or equal that the size of
      container.
   */
  Type & nth(const size_t n)
  {
    Type * ptr = nullptr;
    size_t i = 0;
    me()->traverse([&ptr, &i, &n] (Type & item)
                   {
                     if (i++ < n)
                       return true;
                     ptr = &item;
                     return false;
                   });

    ah_out_of_range_error_if(i != n + 1) << "index out of range";

    return *ptr;
  }

  /// \overload nth()
  const Type & nth(const size_t n) const
  {
    return base()->nth(n);
  }

  /** Find a pointer to an item in the container according to a
      searching criteria.

      `find_ptr(operation)` traverses the container and on each item
      perform `operation(item)`. If the result of `operation` is `true`,
      then the traversal is stopped and a pointer to the current item
      (which mathes `operation`) is returned.

      `operation` must have the following signature:

      bool operation(const typename Container::Item_Type & item)

      \warning Frequent use of this method will definitively degrade the
      performance. Try not to use this method inside loops. In general,
      if you falls in thie situation, the consider your design and to
      use a faster approach.

      \param[in] `operation` to be performed on each item for matching a
      searching criteria.
      \return a valid pointer to an item if this was found or `nullptr`
      otherwise.
   */
  template <class Operation>
  Type * find_ptr(Operation & operation)
    noexcept(operation_is_noexcept<Operation>())
  {
    Type * ptr = nullptr;
    me()->traverse([&ptr,&operation] (Type & item)
                   {
                     if (operation(item))
                       {
                         ptr = &item;
                         return false;
                       }
                     return true;
                   });
    return ptr;
  }

  /// \overload find_ptr()
  template <class Operation>
  const Type * find_ptr(Operation & operation) const
    noexcept(operation_is_noexcept<Operation>())
  {
    return base()->find_ptr(operation);
  }

  /// \overload find_ptr()
  template <class Operation>
  const Type * find_ptr(Operation && operation) const
    noexcept(operation_is_noexcept<Operation>())
  {
    return find_ptr<Operation>(operation);
  }

  /// \overload find_ptr()
  template <class Operation>
  Type * find_ptr(Operation && operation)
    noexcept(operation_is_noexcept<Operation>())
  {
    return find_ptr(operation);
  }

  /** Find the position of an item in the container according to a
      searching criteria.

      `find_index(operation)` traverses the container and on each item
      perform `operation(item)`. If the result of `operation` is `true`,
      then the traversal is stopped and the position of the current item
      (which matches `operation`) is returned.

      `operation` must have the following signature:

      bool operation(const typename Container::Item_Type & item)

      \warning Frequent use of this method will definitively degrade
      the performance. Try not to use this method inside loops. In
      general, if you falls in this situation, the consider your
      design and use a faster approach.

      \param[in] `operation` to be performed on each item for matching a
      searching criteria.
      \return the last seen position. If the item is not found, then the
      number of items is returned.
   */
  template <class Operation>
  size_t find_index(Operation & operation) const
    noexcept(operation_is_noexcept<Operation>())
  {
    size_t i = 0;
    const_me()->traverse([&i,&operation] (Type & item)
                         {
                           if (operation(item))
                             return false;
                           ++i;
                           return true;
                         });
    return i;
  }

  /// \overload find_index()
  template <class Operation>
  size_t find_index(Operation && operation) const
    noexcept(operation_is_noexcept<Operation>())
  {
    return find_index<Operation>(operation);
  }

  /** Safe sequential searching of an item matching a criteria.

      `find_item(operation)` traverses the container and on each item
      perform `operation(item)`. If the result of `operation` is `true`,
      then the traversal is stopped and duple containg a copy of found
      item is returned.

      The method is said safe because returns a copy of item.

      `operation` must have the following signature:

      bool operation(const typename Container::Item_Type & item)

      \param[in] `operation` to be used as searching criteria
      \return a duple std::tuple<bool, Type>. The first field indicates if
      the item was found and the second contains a copy of found
      item. If no item is found, then the first field is `false` and the
      second is the result of default constructor on the type stored in
      the container.
  */
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation & operation)
    noexcept(operation_is_noexcept<Operation>())
  {
    using TT = std::tuple<bool, Type>;
    auto ptr = find_ptr(operation);
    return ptr ? TT(true, *ptr) : TT(false, Type());
  }

  /// \overload find_item(Operation & operation)
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation & operation) const
    noexcept(operation_is_noexcept<Operation>())
  {
    using TT = std::tuple<bool, Type>;
    auto ptr = find_ptr(operation);
    return ptr ? TT(true, *ptr) : TT(false, Type());
  }

  /// \overload find_item(Operation & operation)
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation && operation)
    noexcept(operation_is_noexcept<Operation>())
  {
    return find_item(operation);
  }

  /// \overload find_item(Operation & operation)
  template <class Operation>
  std::tuple<bool, Type> find_item(Operation && operation) const
    noexcept(operation_is_noexcept<Operation>())
  {
    return find_item(operation);
  }
};

/** Special constructors common to `Aleph-w` (\f$\aleph_\omega\f$)
    containers.

    Basically, the constructors of this class append a sequence of items.

    \warning Note that these constructors require that the class is
    correctly initialized, what not only very often is not the case, but
    it could be impossible in most cases. The most part of situations,
    the derived class must be initialized before to start to append new
    items. But this is not the case during this construction. So, don't
    use this class unless that you are absolutely sure that the derived
    class has been initialized.

    \ingroup Secuencias
*/
template <class Container, typename T>
struct SpecialCtors
{
  SpecialCtors() {}

  SpecialCtors(const SpecialCtors&) {}

  SpecialCtors(SpecialCtors&&) noexcept {}

  SpecialCtors & operator = (const SpecialCtors&) { return *this; }

  SpecialCtors & operator = (SpecialCtors&&) noexcept { return *this; }

  /// Build the container by inserting all item of list `l`
  SpecialCtors(const DynList<T> & l)
  {
    l.for_each([this] (const T & item)
               {
                 static_cast<Container*>(this)->append(item);
               });
  }

  template <class It>
  SpecialCtors(It b, It e)
  {
    for (It it = b; it != e; ++it)
      static_cast<Container*>(this)->append(*it);
  }

  SpecialCtors(std::initializer_list<T> l)
  {
    for (const auto & item : l)
      static_cast<Container*>(this)->append(item);
  }
};


/** Common methods to the `Aleph-w` (\f$\aleph_\omega\f$) containers.

    This class contains many and practice methods that are common to any
    `Aleph-w` (\f$\aleph_\omega\f$) container.

    \ingroup Secuencias
*/
template <class Container, typename T>
class FunctionalMethods
{
  Container * me() { return static_cast<Container*>(this); }

  FunctionalMethods<Container, T> * base() const noexcept
  {
    return const_cast<FunctionalMethods<Container, T>*>(this);
  }

  const Container * const_me() const noexcept
  {
    return static_cast<const Container*>(this);
  }

public:

  /** Appends a new element into the container by constructing it
      in-place with the given args.

      `emplace(args)` tries to match a constructor `T(args)`. If this
      exists, then this is constructed in-place and directly forwarded
      to the method `append() of container. If all on the container and
      `T` is adequately done, then the object is constructed once time,
      successively forwarded and at its target place in the container is
      moved, avoiding thus unnecessary copies.

      \note The semantic of append depends of container. In general,
      this has some sense for lists and arrays and it means insertion at
      the end of sequence. On other type of container `append()` is
      equivalent to `insert()`.

      \param[in] args variadic arguments list
      \throw bad_alloc if there is no enough memory
  */
  template <typename ...Args>
  void emplace(Args && ... args)
  {
    (void) me()->append(T(std::forward<Args>(args)...));
  }

  /// \overload emplace(Args && ... args)
  template <typename ...Args>
  void emplace_end(Args && ... args)
  {
    (void) me()->append(T(std::forward<Args>(args)...));
  }

  /** Insert a new element into the container by constructing it
      in-place with the given args.

      `emplace_ins(args)` tries to match a constructor `T(args)`. If this
      exists, then this is constructed in-place and directly forwarded
      to the method `insert() of container. If all on the container and
      `T` is adequately done, then the object is constructed once time,
      successively forwarded and finally, at its target place in the
      container, is moved, avoiding thus unnecessary copies.

      \note The semantic of `insert()` depends on container. In general,
      this has some sense for lists and arrays and it means insertion at
      the beginning of sequence. On other type of container `append()`
      is equivalent to `insert()`.

      \param[in] args variadic arguments list
      \throw bad_alloc if there is no enough memory
  */
  template <typename ...Args>
  void emplace_ins(Args && ... args)
  {
    (void) me()->insert(T(std::forward<Args>(args)...));
  }

private:

  void nninsert(size_t&) {}
  void nnappend(size_t&) {}

  template <typename ... Args>
  void nninsert(size_t & n, const T & item, Args & ... args)
  {
    (void) me()->insert(item);
    ++n;
    nninsert(n, args...);
  }

  template <typename ... Args>
  void nnappend(size_t & n, const T & item, Args & ... args)
  {
    (void) me()->append(item);
    ++n;
    nnappend(n, args...);
  }

public:

  /** Insert n variadic items

      @param[in] args items to be inserted
      @return the number of inserted items
  */
  template <typename ... Args>
  size_t ninsert(Args ... args)
  {
    size_t n = 0;
    nninsert(n, args...);
    return n;
  }

  /** Append n variadic items

      @param[in] args items to be appended
      @return the number of appended items
  */
  template <typename ... Args>
  size_t nappend(Args ... args)
  {
    size_t n = 0;
    nnappend(n, args...);
    return n;
  }

  /** Traverse all the container and performs an operation on each
      element.

      `for_each(operation)` traverses the container and on each element
      `item` is performed `operation(item)`.

      `operation` must have the following signature:

      void operation(const T & item)

      Overloadings of this method allow that that the signature can be
      lightly different; for example, remove the reference or the
      `const`.

      \param[in] `operation` to be done on each element.
      \return an reference to `this`
      \throw anything that can throw `operation`
  */
  template <class Operation>
  void for_each(Operation & operation)
  {
    me()->traverse([&operation] (const T & item)
                   {
                     operation(item);
                     return true;
                   });
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void for_each(Operation & operation) const
  {
    base()->for_each(operation);
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void for_each(Operation && operation) const
  {
    for_each(operation);
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void for_each(Operation && operation)
  {
    for_each(operation);
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void each(Operation & operation)
  {
    for_each(operation);
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void each(Operation & operation) const
  {
    for_each(operation);
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void each(Operation && operation) const
  {
    for_each(operation);
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void each(Operation && operation)
  {
    for_each(operation);
  }

  /** Traverse all the container and performs a mutable operation on
      each element.

      `mutable_for_each(operation)` traverses the container and on each
      element `item` is performed `operation(item)`.

      `operation` could have the following signature:

      void operation(T & item)

      Be very careful with the fact that this method allows to modify
      the elements themselves, what could badly alter the internal
      state of container. This would be the case for heaps, binary
      trees and hash tables.

      \param pos
      \param slice
      \param[in] `operation` to be done on each element.
      \return an reference to `this`
      \throw anything that can throw `operation`
  */
  template <class Operation>
  void each(size_t pos, size_t slice, Operation & operation) const
  {
    auto it = const_me()->get_it(pos);
    while (true)
      {
        operation(it.get_curr());
        for (size_t k = 0; k < slice; ++k)
          {
            it.next();
            if (not it.has_curr())
              return;
          }
      }
  }

  /// \overload for_each(Operation & operation)
  template <class Operation>
  void each(size_t pos, size_t slice, Operation && operation) const
  {
    each(pos, slice, operation);
  }

  template <class Operation>
  void mutable_for_each(Operation & operation)
  {
    me()->traverse([&operation] (T & item)
                   {
                     operation(item);
                     return true;
                   });
  }

  /// \overload mutable_for_each(Operation & operation)
  template <class Operation>
  void mutable_for_each(Operation && operation)
  {
    mutable_for_each(operation);
  }

  /** Check if all the elements of container satisfy a condition.

      `all(operation)` checks if for each element `item` of container
      `operation(item)` returns `true`.

      This method has complexity \f$O(n)\f$ in average and worst case.

      \param[in] operation to be used as condition
      \return `true` if all the elements satisfy the criteria: `false`
      otherwise.
      \throw anything that could throw `operation`
  */
  template <class Operation>
  bool all(Operation & operation) const
  {
    return const_me()->traverse(operation);
  }

  /// \overload all(Operation & operation)
  template <class Operation>
  bool all(Operation && operation) const
  {
    return all(operation);
  }

  /** Test for existence in the container of an element satisfying a
      criteria.

      `exists(op)` returns `true` if it exists any element `item` in container
      for which `op(item)` return `true`.

      This method has complexity \f$O(n)\f$ in average and worst case.

      \param[in] op operation for testing existence
      \return `true` if it exists an item for which `op` return true;
      `false` otherwise.
      \throw anything that could throw `op`
  */
  template <class Operation>
  bool exists(Operation & op) const
  {
    return not const_me()->
      traverse([&op] (const T & i) { return not op(i); });
  }

  /// \overload exists(Operation & op)
  template <class Operation>
  bool exists(Operation && op) const
  {
    return exists(op);
  }

  /** Map the elements of the container.

      `maps(op)` produces a dynamic list resulting of mapping of each
      element of container `item` to the result of operation
      `op(item)`.

      `maps()` is a template method which receives as template
      parameters the type `__T`, which is the type of target or range
      of mapping, and the transforming operation. By default `__T` is
      the same type of the elements stored in the container.

      `operation` should have the following signature:

      __T operation(const T & item)

      So, `operation(item)` performs a transformation of `item` towards
      the type `__T`.

      If `__T == `T`, which is common and by default, then you could
      specify a mapping without need of template specification. For
      example, if the container has integer values, the a mapping of
      item multiplied by 4 could be very simply written as follows:

      c.maps([] (int item) { return 4*i; });

      In contrast, if the range type is different than the domain type,
      then it is necessary to specify the `template` keyword in the
      method call. For example, if the range is `double` and you want to
      return the elements divided by 4, the could do as follows:

          c.template maps<double>([] (int item) { return 1.0*item/4; });

      \param[in] op operation to be performed in order to do the
      transformation on an `item`
      \return a `DynList<__T> object containing the mapped items. The
      order of resulting list is the same than the order of visit of the
      iterator for the container.
      \throw anything that could throw `op` or `bad_alloc` if there is
      no enough memory
  */
  template <typename __T = T, class Operation = Dft_Map_Op<T, __T>>
  DynList<__T> maps(Operation & op) const
  {
    DynList<__T> ret_val;
    const_me()->for_each([&ret_val, &op] (const T & item)
                         {
                           ret_val.append(op(item));
                         });
    return ret_val;
  }

  /// \overload map(Operation & op)
  template <typename __T = T, class Operation = Dft_Map_Op<__T, __T>>
  DynList<__T> maps(Operation && op) const { return maps<__T, Operation>(op); }

  /** Conditional mapping of the elements of the container.

      `maps_if(prop, op)` traverses each item of container, on each
      item it tests the proposition `prop`. If this last is true, then
      the item is mapped through the function `op(item)`.

      \param[in] op operation to be perfomed in order to do the
      transformation on an `item`.
      \param[in] prop a lambda returning a `bool` which perform the
      logical test.
      \return a `DynList<__T> object containing the mapped items. The
      order of resulting list is the same than the order of visit of the
      iterator for the container.
      \throw anything that could throw `op` or `bad_alloc` if there is
      no enough memory
  */
  template <typename __T = T, class Prop, class Operation>
  DynList<__T> maps_if(Prop prop, Operation & op) const
  {
    DynList<__T> ret_val;
    const_me()->for_each([&ret_val, &prop, &op] (const T & item)
                         {
                           if (prop(item))
                             ret_val.append(op(item));
                         });
    return ret_val;
  }

  /// \overload map(Prop & prop, Operation & op)
  template <typename __T = T, class Prop, class Operation>
  DynList<__T> maps_if(Prop prop, Operation && op) const
  {
    return maps<__T, Prop, Operation>(prop, op);
  }

  DynList<T> to_dynlist() const
  {
    return maps([] (auto & item) { return item; });
  }

  /** Fold the elements of the container to a specific result.

      `foldl(init, op)` set an internal variable `acc` of type `__T` to
      `init` value. Then it traverses the container and on each `item`
      it performs:

      acc = op(acc, op(acc, item);

      So `acc` serves as a sort of accumulator.

      `op` should have the following signature:

      __T op(__T acc, const T & item);

      Since `foldl` is overloaded with several operation structures,
      there is a certain flexibility with the parameter qualifiers. You
      could, for example, to declare `acc` and/or `item` by value.

      The method is a template. The first template parameter `__T`
      specifies the final folded type. By default, this type is `T` (the
      type of elements stored in the container). The second parameter is
      the operation. If the folded type is the same than `T` (the type
      of item stored), then you can simply write a `foldl()`. For
      example, if the container stores integer, in order to determine
      the maximum of all elements you could do:

      c.foldl(std::numeric_limits<int>::min(), [] (int acc, int item)
	    {
              return std::min(acc, item);
	    });

      When the folded type is different from `T`, then you must
      specify the folded type as template parameter. For example, if
      you want to compute the sum of inverted elements, then you could
      do it as follows:

      c.template foldl<double>(0, [] (double acc, int item)
	    {
              return acu + 1.0/item;
	    });

      \param[in] init initial value of folded value (or accumulator).
      \param[in] op operation to be performed on each item and used for
	    folding.
      \return the final folded computation.
      \throw anything that could throw `op`
  */
  template <typename __T = T, class Op = Dft_Fold_Op<__T, T>>
  __T foldl(const __T & init, Op & op) const
  {
    __T ret_val = init;
    const_me()->for_each([&ret_val, &op] (const T & item)
                         {
                           ret_val = op(ret_val, item);
                         });
    return ret_val;
  }

  /// \overload foldl(const __T & init, Op & op)
  template <typename __T = T, class Op = Dft_Fold_Op<__T, T>>
    __T foldl(const __T & init, Op && op = Op()) const
  {
    return foldl(init, op);
  }

  template <typename __T = T, class Op = Dft_Fold_Op<__T, T>>
  __T fold_left(const __T & init, Op & op) const
  {
    return const_me()->template foldl<__T>(init, op);
  }

  /// \overload fold_left(Op & op, const __T & init)
  template <typename __T = T, class Op = Dft_Fold_Op<__T, T>>
  __T fold_left(const __T & init, Op && op = Op()) const
  {
    return const_me()->template foldl<__T>(init, op);
  }

  /** Simplified version of foldl() where the folded type is the same
      type of elements stored in the container.

      @see foldl(const __T & init, Op & op)
  */
  template <class Operation>
  T fold(const T & init, Operation & operation) const
  {
    auto ret_val = init;
    const_me()->for_each([&ret_val, &operation] (const T & item)
                         {
                           ret_val = operation(ret_val, item);
                         });
    return ret_val;
  }

  /// \overload fold(const T & init, Operation & operation)
  template <class Operation>
  T fold(const T & init, Operation && operation) const
  {
    return fold(init, operation);
  }

  /** Filter the elements of a container according to a matching
      criteria.

      This method builds a dynamic list with copies of items of
      container matching a criteria defined by `operation`, which
      should have the following signature:

      bool operation(const T & item)

      If `operation` return `true` then `item` matches the criteria;
      otherwise, `operation` must return `false`.

      For example, if the container has integer, then the following
      code snippet would return a list containing the items greater
      than 100:

      c.filter([] (auto item) { return item > 100; });

      \param[in] operation defining the flter criteria
      \return a `DynList<T>` with the matched elements.
      \throw anything that could throw `operation` or `bad_alloc` if
      there is no enough memory
  */
  template <class Operation>
  DynList<T> filter(Operation & operation) const
  {
    DynList<T> ret_val;
    const_me()->for_each([&ret_val, &operation] (const T & item)
                         {
                           if (operation(item))
                             ret_val.append(item);
                         });
    return ret_val;
  }

  /// \overload filter(Operation & operation)
  template <class Operation>
  DynList<T> filter(Operation && operation) const
  {
    return filter(operation);
  }

  /** Filter the elements of a container according to a matching
      criteria an return pointer to the matched items in the container.

      This method builds a dynamic list with stores pointers to the
      items of matching a criteria defined by `operation`, which
      should have the followgin signature:

      bool operation(const T & item)

      If `operation` return `true` then `item` matches the criteria;
      otherwise, `operation` must return `false`.

      For example, if the container has integer, then the following
      code snippet would return a list containing the items greater
      than 100:

      c.ptr_filter([] (auto item) { return item > 100; });

      \param[in] operation defining the flter criteria
      \return a `DynList<const T*>` with the pointers to the matched elements.
      \throw anything that could throw `operation` or `bad_alloc` if
      there is no enough memory
  */
  template <class Operation>
  DynList<const T*> ptr_filter(Operation & operation) const
  {
    DynList<const T*> ret_val;
    const_me()->for_each([&ret_val, &operation] (const T & item)
                         {
                           if (operation(item))
                             ret_val.append(&item);
                         });
    return ret_val;
  }

  template <class Operation>
  DynList<const T*> ptr_filter(Operation && operation) const
  {
    return ptr_filter(operation);
  }

  /**  Filter the elements of a container according to a matching
       criteria and determine its positions respect to the traversal
       of container.

       `pfilter(operation)` is very similar to `filter()`, but instead
       of building a list of filtered elements, it builds a list of
       pairs with form `(item, pos)`, where `item` is a copy of
       filtered element and `pos` is its position respect to the
       traversal order. The position is relative to the container type.

       The pair is defined with a tuple:

       std::tuple<T, size_t>

       \param[in] operation that defines the filter criteria
       \return a DynList
       \throw bad_alloc if there is no enough memory
       \see filter(Operation & operation)
  */
  template <class Operation>
  DynList<std::tuple<T, size_t>> pfilter(Operation & operation) const
  {
    using TT = std::tuple<T, size_t>;
    DynList<TT> ret_val;
    size_t i = 0;
    const_me()->for_each([&ret_val, &operation, &i] (const T & item)
                         {
                           if (operation(item))
                             ret_val.append(TT(item, i));
                           ++i;
                         });
    return ret_val;
  }

  /// \overload pfilter(Operation & operation)
  template <class Operation>
  DynList<std::tuple<T, size_t>> pfilter(Operation && operation) const
  {
    return pfilter(operation);
  }

  /** Exclusive partition of container according to a filter criteria.

      `partition(op)` traverses the container and filters its elements
      according to the filter criteria defined by `op`. The filtered
      elements are copied to a first list and the not filtered ones to a
      second list. When all the container is traversed, a pair
      containing these lists is returned.

      The `op` requirements are the same as for `filter()`.

      \param[in] op operation instrumenting the filter criteria
      \return a `std::pair<DynList<T>, DynList<T>>. `first` contains the
      filtered elements and `second` the non-filtered ones.
      \throw anything that could throw op  or `bad_alloc` if there is
      no enough memory
      \see filter()
  */
  template <class Operation>
  std::pair<DynList<T>, DynList<T>> partition(Operation & op) const
  {
    std::pair<DynList<T>, DynList<T>> ret_val;
    const_me()->for_each([&ret_val, &op] (const T & item)
                         {
                           if (op(item))
                             ret_val.first.append(item);
                           else
                             ret_val.second.append(item);
                         });
    return ret_val;
  }

  /// \overload partition(Operation & op
  template <class Operation>
  std::pair<DynList<T>, DynList<T>> partition(Operation && op) const
  {
    return partition(op);
  }

  /** Exclusive partition of container in the nth item

      `partition(n)` traverses the container and produces a pair of
      lists. The first one contains the first `n` elements and the
      second one the `this->size() - n` remaining elements.

      \param[in] n the first `n` items of the first list
      \throw anything that could throw op  or `bad_alloc` if there is
      no enough memory
  */
  template <class Operation>
  std::pair<DynList<T>, DynList<T>> partition(size_t n) const
  {
    size_t i = 0;
    std::pair<DynList<T>, DynList<T>> ret_val;
    const_me()->for_each([&ret_val, &i, n] (const T & item)
                         {
                           if (i++ < n)
                             ret_val.first.append(item);
                           else
                             ret_val.second.append(item);
                         });
    return ret_val;
  }

  std::pair<DynList<T>, DynList<T>> split_half() const
  {
    size_t i = 0;
    std::pair<DynList<T>, DynList<T>> ret_val;
    const_me()->for_each([&ret_val, &i] (const T & item)
                         {
                           if ((i % 2) == 0)
                             ret_val.first.append(item);
                           else
                             ret_val.second.append(item);
                           i++;
                         });
    return ret_val;
  }

  /** Exclusive partition of container according to a filter criteria.

      This method has exactly the same semantic as
      `partition(Operation & op)`, excepts than instead of returning a
      `std::pair` it returns a `std::tuple`.

      \param[in] op operation instrumenting the filter criteria
      \return a `std::tuple<DynList<T>, DynList<T>>. `first` contains the
      filteres elements and `second` the non-filtered ones.
      \throw anything that could throw op  or `bad_alloc` if there is
      no enough memory
      \see partition(Operation & op)
  */
  template <class Operation>
  std::tuple<DynList<T>, DynList<T>> tpartition(Operation & op) const
  {
    DynList<T> r1, r2;
    const_me()->for_each([&r1, &r2, &op] (const T & item)
                         {
                           if (op(item))
                             r1.append(item);
                           else
                             r2.append(item);
                         });
    return std::tuple<DynList<T>, DynList<T>>(r1, r2);
  }

  /// \overload tpartition(Operation & op)
  template <class Operation>
  std::tuple<DynList<T>, DynList<T>> tpartition(Operation && op) const
  {
    return partition(op);
  }

  /** Count the number of elements of a container.

      This method counts the number of elements stored in the container.

      \note Take in account that this method computes; it does not
      retrieve. Consequently, it always takes \f$O(n)\f$. However, for
      many containers this number is already stored and retrievable in
      \f$O(1)\f$ through the method `size()`

      \return the number of elements stored in the container.
  */
  size_t length() const noexcept
  {
    size_t count = 0;
    const_me()->for_each([&count] (const T &) { ++count; });
    return count;
  }

  /** Return a list with the elements of container in reverse order
      respect to its traversal order.

      \return a `DynList<T>` inversely ordered according to the
      traversal order.
      \throw bad_alloc if there is no enough memory
  */
  DynList<T> rev() const
  {
    DynList<T> ret;
    const_me()->for_each([&ret] (const T & i) { ret.insert(i); });
    return ret;
  }

  /** Return a list with the first n elements seen in the container
      during its traversal.

      The complexity of this method is \f$O(n)\f$ where n can be less
      than the number of elements of container.

      \return A `DynList<T>` having the first `n` elements according to
      its traversal order.
      \throw bad_alloc if there is no enough memory or `out_of_range`
      if n is greater or equal than the number of elements in the
      container.
   */
  DynList<T> take(const size_t n) const
  {
    size_t i = 0;
    DynList<T> ret;
    const_me()->traverse([&i, &ret, n] (const T & item)
                         {
                           if (i++ >= n)
                             return false;
                           ret.append(item);
                           return true;
                         });
    return ret;
  }

  /** Return a list with elements seen in the container between i and
      j position respect to its traversal.

      The complexity of this method is \f$O(n)\f$ where n can be less
      than the number of elements of container.

      \return A `DynList<T>` having the first `n` elements according to
      its traversal order.
      \throw bad_alloc if there is no enough memory or `out_of_range`
      if n is greater or equal than the number of elements in the
      container.
  */
  DynList<T> take(size_t i, size_t j, size_t step = 1) const
  {
    DynList<T> ret;
    if (step == 0)
      return ret;
    for (auto it = const_me()->get_it(i); i <= j and it.has_curr();
         it.next_ne(), i += step)
      ret.append(it.get_curr());
    return ret;
  }

  /** Drop the first n elements seen in the container during its
      traversal.

      The complexity of this method is \f$O(N)\f$ where N always is the
      number of elements of container.

      \return A `DynList<T>` having the remainder \f$N - n\f$ elements
      according to traversal order.
      \throw bad_alloc if there is no enough memory or `out_of_range`
      if n is greater or equal than `N` (the number of elements in the
      container).
   */
  DynList<T> drop(const size_t n) const
  {
    size_t i = 0;
    DynList<T> ret;
    const_me()->traverse([&i, &ret, n] (const T & item)
                         {
                           if (i++ >= n)
                             ret.append(item);
                           return true;
                         });
    return ret;
  }

  /** Drop the first n elements seen from container.

      The complexity of this method is \f$O(N)\f$ where N always is the
      number of elements of container.

      \throw out_of_range if n is greater or equal than `N` (the
      number of elements in the container).
  */
  void mutable_drop(size_t n)
  {
    for (size_t i = 0; i < n; ++i)
      me()->remove();
  }
};

/** Generic list of items stored in a container.

    \ingroup Secuencias
*/
template <class Container, typename T>
struct GenericItems
{
  /** Return a list of all the elements of a container sorted by
      traversal order.

      \return a `DynList<T>` containing all the elements of the
      container
      \throw bad_alloc if there is no enough memory
  */
  DynList<T> items() const
  {
    return static_cast<const Container*>(this)->
      Container::template maps<T> ([] (const T & key) { return key; });
  }

  /// \overload items()
  DynList<T> keys() const { return items(); }
};


/** Alias to GenricItems functor

    \ingroup Secuencias
*/
template <class Container, typename T>
using GenericKeys = GenericItems<Container, T>;


/** Equality test for containers.

    \ingroup Secuencias
*/
template <class Container>
class EqualToMethod
{
  const Container * const_me() const
  {
    return static_cast<const Container*>(this);
  }

public:

  /** Test if elements of `this` are exactly contained in another
      container.

      This method serves for testing if two containers contain the same
      elements. First, the container sizes are tested for equality. If
      they have the same size, then the testing is done by traversing
      `this`. Each seen element is searched in the another container
      with the method `search()`. So the container `r` must export the
      `search()` method, which frequently is the case for containers
      oriented to fast retrieval.

      @warning On some container, concretely `DynList`, the size is
      computed, not retrieved. So take in account this fact.

      \param[in] r container on which the searches will be performed.
      \return `true` if the container have the same size and all the
      elements of `this` are present in `r`
  */
  bool equal_to(const Container & r) const noexcept
  {
    if (this == &r)
      return true;

    if (const_me()->size() != r.size())
      return false;

    return const_me()->all([&r] (const typename Container::Key_Type & k)
                           { return r.search(k) != nullptr; });
  }

  /// \overload equal_to(const Container & r)
  bool operator == (const Container & r) const	noexcept
  {
    return equal_to(r);
  }

  /// Negation of are_equal()
  bool operator != (const Container & r) const	noexcept
  {
    return not equal_to(r);
  }
};

/** Common methods to mapping containers.

    \ingroup Secuencias
*/
template <class Container, typename Key, typename Data>
class MapSequencesMethods
{
  const Container * const_me() const
  {
    return static_cast<const Container*>(this);
  }

public:

  /** Return the domain set of container.

      This is a template method whose unique parameter is the type of
      container where the domain set will be stored. By default, this
      type is `DynList`.

      Often the elements of domain are called "keys" and those of codomain
      or range "values".

      \return A new container, according the passed template parameter,
      containing all the elements of domain of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = DynList>
  C<Key> keys() const
  {
    C<Key> ret_val;
    const_me()->for_each([&ret_val] (const Key & p)
                         {
                           ret_val.append(p.first);
                         });
    return ret_val;
  }

  /** Return the range set of container.

      This is a template method whose unique parameter is the type of
      container where the range set will be stored. By default this
      type is `DynList`.

      Often the elements of range are called "value" and those of domain
      "values".

      \return A new container, according the passed template parameter,
      containing all the elements of range of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = DynList>
  C<Data> values() const
  {
    C<Data> ret_val;
    const_me()->for_each([&ret_val] (const std::pair<Key, Data> & p)
                         {
                           ret_val.append(p.second);
                         });
    return ret_val;
  }

  /** Return a container of pointers to the elements of range set of
      container.

      This is a template method whose unique parameter is the type of
      container where the pointers set will be stored. By default this
      type is `DynList`.

      Often the elements of range are called "value" and those of domain
      "values". This method returns a container with pointers to those
      elements. In this way, the value could be modified.

      \return A new container, according the passed template parameter,
      containing all the pointers to elements of range of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = DynList>
  C<Data*> values_ptr() const
  {
    C<Data*> ret_val;
    const_me()->for_each([&ret_val] (std::pair<Key, Data> & p)
                         { ret_val.append(&p.second); });
    return ret_val;
  }

  /** Return a container with the entire mapping.

      `items()` returns a container copy of an entire mapping. Each
      element of returned mapping is a pair <key, value>.

      \return A new container, according the passed template parameter,
      containing all the pairs key,value of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = DynList>
  C<std::pair<Key, Data>> items() const
  {
    C<std::pair<Key, Data>> ret;
    const_me()->for_each([&ret] (std::pair<Key, Data> & p) { ret.append(p); });
    return ret;
  }

  /** Return a container with the entire mapping where the range could
      be modified.

      `items_ptr()` returns a container copy of an entire
      mapping. Each element of returned mapping is a pair <key,
      pointer to value>.

      \return A new container, according the passed template parameter,
      containing all the pairs key,pointer-value of mapping.
      \throw bad_alloc if there is no enough memory
  */
  template <template <typename> class C = DynList>
  C<std::pair<Key, Data*>> items_ptr() const
  {
    C<Data> ret_val;
    const_me()->for_each([&ret_val] (std::pair<Key, Data> & p)
                         {
                           ret_val.append(std::pair<Key,Data*>(p.first,
                                                               p.second));
                         });
    return ret_val;
  }

  /**  Return the image or element mapped to `key`.

       \return a modifiable reference to value associated to the key.
       \throw domain_error if key is not present in the mapping
  */
  Data & operator () (const Key & key)
  {
    return this->find(key);
  }

  /**  Return the image or element mapped to `key`.

       \return a constant reference to value associated to the key.
       \throw domain_error if key is not present in the mapping.
  */
  const Data & operator () (const Key & key) const
  {
    return this->find(key);
  }
};



# endif // AH_DRY_H
