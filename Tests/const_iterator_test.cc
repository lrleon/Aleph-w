
/*
                          Aleph_w

                           / \  | | ___ _ __ | |__      __      __
                          / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / /
                         / ___ \| |  __/ |_) | | | |_____\ V  V /
                        /_/   \_\_|\___| .__/|_| |_|      \_/\_/
                                       |_|

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

/**
 * @file const_iterator_test.cc
 * @brief A constant container is read-only: its iterator is Const_Iterator, its
 *        accessors hand out references to const, and the members that modify
 *        the container are not available through it.
 */

#include <gtest/gtest.h>

#include <ah-iterator.H>
#include <ah-zip.H>
#include <ahFunctional.H>
#include <filter_iterator.H>
#include <htlist.H>
#include <tpl_array.H>
#include <tpl_arrayHeap.H>
#include <tpl_arrayQueue.H>
#include <tpl_arrayStack.H>
#include <tpl_dynArray.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynBinHeap.H>
#include <tpl_dynDlist.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynListStack.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetHash.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynSkipList.H>
#include <tpl_hash.H>
#include <tpl_odhash.H>
#include <tpl_olhash.H>
#include <tpl_memArray.H>
#include <tpl_random_queue.H>
#include <tpl_small_vector.H>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>

using namespace Aleph;

namespace
{
  template <class C>
  constexpr bool iterator_rejects_a_constant_container =
    not std::is_constructible_v<typename C::Iterator, const C &>
    and std::is_constructible_v<typename C::Iterator, C &>
    and std::is_constructible_v<typename C::Const_Iterator, const C &>;

  // What get_curr() hands out on the iterator of C and on its read-only view.
  template <class C>
  using curr_t = decltype(std::declval<const typename C::Iterator &>().get_curr());

  template <class C>
  using const_curr_t = decltype(std::declval<const const_iterator_t<C> &>().get_curr());

  // The expressions that a client must not be able to write. They are
  // templates so that an ill-formed expression makes the requires-expression
  // false instead of making the program ill-formed.
  template <class C>
  constexpr bool writes_through_call = requires (const C & c) { c(0) = 7; };

  template <class It>
  constexpr bool writes_through_curr = requires (It it) { it.get_curr() = 8; };

  template <class It>
  constexpr bool removes_through_it = requires (It it) { it.del(); };

  template <class C>
  constexpr bool const_traverse_takes_a_modifying_operation = requires (const C & c)
    {
      c.traverse([] (typename C::Item_Type & x) { x = 9; return true; });
    };

  template <class C>
  constexpr bool const_traverse_takes_a_reading_operation = requires (const C & c)
    {
      c.traverse([] (const typename C::Item_Type & x) { return x == 9; });
    };
}

TEST(ConstIterator, the_iterators_that_hand_out_modifiable_references_reject_constant_containers)
{
  // The families whose Iterator::get_curr() returns T &: they are built from
  // a modifiable container only, and a constant one yields a Const_Iterator.
  static_assert(iterator_rejects_a_constant_container<MemArray<int>>);
  static_assert(iterator_rejects_a_constant_container<Array<int>>);
  static_assert(iterator_rejects_a_constant_container<ArrayStack<int>>);
  static_assert(iterator_rejects_a_constant_container<FixedStack<int>>);
  static_assert(iterator_rejects_a_constant_container<ArrayQueue<int>>);
  static_assert(iterator_rejects_a_constant_container<FixedQueue<int>>);
  static_assert(iterator_rejects_a_constant_container<Random_Set<int>>);
  static_assert(iterator_rejects_a_constant_container<DynArray<int>>);
  static_assert(iterator_rejects_a_constant_container<DynList<int>>);
  static_assert(iterator_rejects_a_constant_container<DynDlist<int>>);
  static_assert(iterator_rejects_a_constant_container<DynListQueue<int>>);
  static_assert(iterator_rejects_a_constant_container<DynListStack<int>>);
  static_assert(iterator_rejects_a_constant_container<DynSetTree<int>>);
  static_assert(iterator_rejects_a_constant_container<DynMapTree<int, int>>);
  static_assert(iterator_rejects_a_constant_container<DynSetHash<int>>);
  static_assert(iterator_rejects_a_constant_container<DynMapHash<int, int>>);
  static_assert(iterator_rejects_a_constant_container<ODhashTable<int>>);
  static_assert(iterator_rejects_a_constant_container<OLhashTable<int>>);

  static_assert(std::is_same_v<curr_t<DynList<int>>, int &>);
  static_assert(std::is_same_v<const_curr_t<DynList<int>>, const int &>);
  static_assert(std::is_same_v<curr_t<Array<int>>, int &>);
  static_assert(std::is_same_v<const_curr_t<Array<int>>, const int &>);
  static_assert(std::is_same_v<const_curr_t<DynArray<int>>, const int &>);

  // The heaps are read-only through their iterators: the heap order must not
  // be broken.
  static_assert(std::is_same_v<curr_t<ArrayHeap<int>>, const int &>);
  static_assert(std::is_same_v<curr_t<DynArrayHeap<int>>, const int &>);
  static_assert(std::is_same_v<curr_t<DynBinHeap<int>>, const int &>);

  // The skip list keeps its keys sorted: its iterator is read-only.
  static_assert(std::is_constructible_v<DynSkipList<int>::Iterator, const DynSkipList<int> &>);
  static_assert(std::is_same_v<decltype(std::declval<DynSkipList<int>::Iterator &>().get_curr()),
                               const int &>);

  // A constant iterator object of a set hands out a reference to const; the
  // view keeps the underlying iterator and its typedefs.
  static_assert(std::is_same_v<curr_t<DynSetTree<int>>, const int &>);
  static_assert(std::is_same_v<decltype(std::declval<DynMapTree<int, int>::Iterator &>().get_curr()),
                               std::pair<int, int> &>);
  static_assert(std::is_same_v<const_curr_t<DynMapTree<int, int>>, const std::pair<int, int> &>);
  static_assert(std::is_same_v<const_iterator_t<DynSetTree<int>>::Itor, DynSetTree<int>::Iterator>);
  static_assert(std::is_same_v<const_iterator_t<DynList<int>>::Item_Type, int>);
  static_assert(std::is_same_v<const_iterator_t<DynList<int>>::Set_Type, DynList<int>>);
  static_assert(std::is_same_v<const_curr_t<HashSet<int>>, const int &>);
}

TEST(ConstIterator, get_it_depends_on_the_constness_of_the_container)
{
  DynList<int> l = {1, 2, 3};
  const DynList<int> & cl = l;
  static_assert(std::is_same_v<decltype(l.get_it()), DynList<int>::Iterator>);
  static_assert(std::is_same_v<decltype(cl.get_it()), DynList<int>::Const_Iterator>);
  static_assert(std::is_same_v<decltype(cl.get_it(1)), DynList<int>::Const_Iterator>);
  static_assert(std::is_same_v<decltype(iterator_for(l)), DynList<int>::Iterator>);
  static_assert(std::is_same_v<decltype(iterator_for(cl)), DynList<int>::Const_Iterator>);

  l.get_it().get_curr() = 10;  // modifiable through the modifiable container
  EXPECT_EQ(cl.get_first(), 10);

  int sum = 0;
  for (auto it = cl.get_it(); it.has_curr(); it.next_ne())
    sum += it.get_curr();
  EXPECT_EQ(sum, 15);
  EXPECT_EQ(cl.get_it(2).get_curr(), 3);
}

TEST(ConstIterator, reads_the_container_like_the_iterator)
{
  DynDlist<int> l;
  for (int i = 0; i < 5; ++i)
    l.append(i);
  const DynDlist<int> & cl = l;

  DynDlist<int>::Const_Iterator it(cl);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 0);
  EXPECT_EQ(it.get_pos(), 0);
  it.next();
  EXPECT_EQ(it.get_curr_ne(), 1);
  it.reset_last();
  EXPECT_EQ(it.get_curr(), 4);
  EXPECT_TRUE(it.is_in_last());
  it.prev();
  EXPECT_EQ(it.get_curr(), 3);
  it.reset_first();
  EXPECT_EQ(it.get_curr(), 0);

  // Copies and exchanges, as the iterator.
  DynDlist<int>::Const_Iterator copy(it);
  copy.next();
  EXPECT_EQ(it.get_curr(), 0);
  EXPECT_EQ(copy.get_curr(), 1);
  it.swap(copy);
  EXPECT_EQ(it.get_curr(), 1);
  EXPECT_EQ(copy.get_curr(), 0);

  // A view over an existing iterator starts at its position.
  auto mit = l.get_it(3);
  DynDlist<int>::Const_Iterator view(mit);
  EXPECT_EQ(view.get_curr(), 3);

  // The end of the sequence.
  it.end();
  EXPECT_FALSE(it.has_curr());
}

TEST(ConstIterator, the_stl_const_iterator_is_read_only_and_random_access_on_arrays)
{
  Array<int> a;
  for (int i = 0; i < 10; ++i)
    a.append(i);
  const Array<int> & ca = a;

  static_assert(std::random_access_iterator<decltype(ca.begin())>);
  static_assert(std::is_same_v<decltype(*ca.begin()), const int &>);
  static_assert(std::is_same_v<decltype(*a.begin()), int &>);

  EXPECT_EQ(std::count_if(ca.begin(), ca.end(), [] (int x) { return x % 2 == 0; }), 5);
  EXPECT_EQ(ca.end() - ca.begin(), 10);
  EXPECT_EQ(ca.begin()[7], 7);
  EXPECT_TRUE(std::is_sorted(ca.begin(), ca.end()));

  int sum = 0;
  for (const int & x : ca)
    sum += x;
  EXPECT_EQ(sum, 45);

  const DynList<int> cl = {3, 1, 2};
  EXPECT_EQ(*std::max_element(cl.begin(), cl.end()), 3);
}

TEST(ConstIterator, a_constant_dynarray_is_read_without_allocating)
{
  DynArray<int> a(16);  // sixteen entries never written
  a.set_default_initial_value(7);
  const DynArray<int> & ca = a;

  int sum = 0;
  for (auto it = ca.get_it(); it.has_curr(); it.next_ne())
    sum += it.get_curr();
  EXPECT_EQ(sum, 16 * 7);
  EXPECT_EQ(ca.get_first(), 7);
  EXPECT_EQ(ca.get_last(), 7);
  EXPECT_EQ(ca.read(3), 7);
  for (size_t i = 0; i < 16; ++i)
    EXPECT_FALSE(ca.exist(i));  // nothing was allocated by reading

  // The modifiable iterator allocates (the whole block of the entry), as
  // a[i] does.
  DynArray<int>::Iterator it(a);
  it.get_curr() = 1;
  EXPECT_TRUE(ca.exist(0));
  EXPECT_EQ(ca(0), 1);
  EXPECT_EQ(ca.get_first(), 1);
  EXPECT_EQ(ca.get_last(), 7);
}

TEST(ConstIterator, the_accessors_of_a_constant_container_hand_out_references_to_const)
{
  using CA = const Array<int> &;
  static_assert(std::is_same_v<decltype(std::declval<CA>()[0]), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CA>()(0)), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CA>().get_first()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CA>().get_last()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<Array<int> &>()[0]), int &>);

  using CQ = const ArrayQueue<int> &;
  static_assert(std::is_same_v<decltype(std::declval<CQ>().front()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CQ>().rear()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CQ>()[0]), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CQ>().top()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<ArrayQueue<int> &>().front()), int &>);

  using CL = const DynList<int> &;
  static_assert(std::is_same_v<decltype(std::declval<CL>().get_first()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CL>().get_last_ne()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CL>().top()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<DynList<int> &>().get_first()), int &>);

  using CD = const DynDlist<int> &;
  static_assert(std::is_same_v<decltype(std::declval<CD>().get_first()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CD>()[0]), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<DynDlist<int> &>()[0]), int &>);

  using CDA = const DynArray<int> &;
  static_assert(std::is_same_v<decltype(std::declval<CDA>()(0)), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CDA>().access(0)), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<CDA>().top()), const int &>);

  static_assert(std::is_same_v<decltype(std::declval<const DynSetTree<int> &>().find(0)), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<const DynSetTree<int> &>().search(0)), const int *>);
  static_assert(std::is_same_v<decltype(std::declval<DynSetTree<int> &>().find(0)), int &>);
  static_assert(std::is_same_v<decltype(std::declval<const DynMapTree<int, int> &>().search(0)),
                               const std::pair<int, int> *>);
  static_assert(std::is_same_v<decltype(std::declval<const HashSet<int> &>().get_first()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<const DynSetHash<int> &>().search(0)), const int *>);
  static_assert(std::is_same_v<decltype(std::declval<DynSetHash<int> &>().search(0)), int *>);
  static_assert(std::is_same_v<decltype(std::declval<const DynMapHash<int, int> &>().search(0)),
                               const std::pair<int, int> *>);
  static_assert(std::is_same_v<decltype(std::declval<const ODhashTable<int> &>().search(0)), const int *>);
  static_assert(std::is_same_v<decltype(std::declval<const OLhashTable<int> &>().search(0)), const int *>);
  static_assert(std::is_same_v<decltype(std::declval<const DynSkipList<int> &>().search(0)), const int *>);
  static_assert(std::is_same_v<decltype(std::declval<DynSkipList<int> &>().search(0)), int *>);
  static_assert(std::is_same_v<decltype(std::declval<const DynBinHeap<int> &>().top()), const int &>);
  static_assert(std::is_same_v<decltype(std::declval<const DynList<int> &>().nth(0)), const int &>);

  // The modifiable path still writes.
  DynList<int> l = {1, 2};
  l.get_first() = 5;
  l.nth(1) = 6;
  EXPECT_EQ(l.get_first(), 5);
  EXPECT_EQ(l.get_last(), 6);
  DynMapTree<int, int> m;
  m.insert(1, 10);
  m.find(1) = 11;
  EXPECT_EQ(m.find(1), 11);
  EXPECT_EQ(std::as_const(m).search(1)->second, 11);
}

TEST(ConstIterator, the_leaks_found_from_a_client_no_longer_compile)
{
  // DynArray::operator()(size_t) const handed out T &.
  static_assert(not writes_through_call<DynArray<int>>);
  // The iterator of a constant Array handed out T & from get_curr().
  static_assert(not std::is_constructible_v<Array<int>::Iterator, const Array<int> &>);
  static_assert(not writes_through_curr<Array<int>::Const_Iterator>);
  static_assert(writes_through_curr<Array<int>::Iterator>);
  // SmallVector::traverse() const passed T & to the operation.
  static_assert(not const_traverse_takes_a_modifying_operation<SmallVector<int, 4>>);
  static_assert(const_traverse_takes_a_reading_operation<SmallVector<int, 4>>);
  static_assert(not const_traverse_takes_a_modifying_operation<DynList<int>>);
  static_assert(const_traverse_takes_a_reading_operation<DynList<int>>);
  // Through a constant DynArray, the shared default value cannot be written.
  static_assert(not writes_through_curr<DynArray<int>::Const_Iterator>);
  // A Const_Iterator has no modifying members.
  static_assert(not removes_through_it<DynList<int>::Const_Iterator>);
  static_assert(removes_through_it<DynList<int>::Iterator>);
}

TEST(ConstIterator, the_generic_code_reads_constant_containers)
{
  const DynList<int> a = {1, 2, 3};
  const DynList<int> b = {4, 5, 6};

  // zip, pairs and filters.
  int sum = 0;
  for (auto it = get_pair_it(a, b); it.has_curr(); it.next_ne())
    {
      const auto [x, y] = it.get_curr();
      sum += x * y;
    }
  EXPECT_EQ(sum, 4 + 10 + 18);
  EXPECT_EQ(zip(a, b).size(), 3u);

  Filter_Iterator<DynList<int>, DynList<int>::Const_Iterator, bool (*)(const int &)>
    odd(a, [] (const int & x) { return x % 2 == 1; });
  static_assert(std::is_same_v<decltype(odd)::Item_Type, int>);
  int count = 0;
  for (; odd.has_curr(); odd.next())
    ++count;
  EXPECT_EQ(count, 2);

  // The mixins and the free functions.
  EXPECT_TRUE(a.all([] (const int & x) { return x > 0; }));
  EXPECT_EQ(*a.find_ptr([] (const int & x) { return x == 2; }), 2);
  static_assert(std::is_same_v<decltype(a.find_ptr([] (const int & x) { return x == 2; })), const int *>);
  EXPECT_EQ(a.nth(2), 3);
  EXPECT_TRUE(a == DynList<int>({1, 2, 3}));
  EXPECT_FALSE(a == b);
  ASSERT_NE(min_ptr(a), nullptr);
  EXPECT_EQ(*min_ptr(a), 1);
}
