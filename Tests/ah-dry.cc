
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



/**
 * @file ah-dry.cc
 * @brief Tests for Ah Dry
 */
# include <gtest/gtest.h>

# include <limits>
# include <numeric>
# include <algorithm>
# include <map>
# include <stdexcept>
# include <string>
# include <utility>
# include <vector>
# include <ah-zip.H>
# include <ahFunctional.H>
# include <ah-string-utils.H>
# include <ahSort.H>
# include <htlist.H>
# include <tpl_arrayHeap.H>
# include <tpl_dynArrayHeap.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynDlist.H>
# include <tpl_dynSetTree.H>
# include <tpl_hash.H>
# include <tpl_dynSetHash.H>
# include <tpl_dynArray.H>
# include <tpl_arrayQueue.H>
# include <tpl_dynListStack.H>
# include <tpl_dynarray_set.H>
# include <tpl_random_queue.H>
# include <tpl_dynListQueue.H>
# include <tpl_arrayStack.H>
# include <tpl_interval_tree.H>
# include <tpl_dynMat.H>
# include <tpl_dynSkipList.H>

using namespace std;
using namespace testing;
using namespace Aleph;

template <class Ctype>
struct Container : public testing::Test
{
  static constexpr size_t N = 1000;
  Ctype c;
  DynList<int> item_list;
  Container()
  {
    for (size_t i = 0; i < N; ++i)
      {
	c.append(i);
	item_list.append(i);
      }
    item_list = sort(item_list);
  }
};

TYPED_TEST_SUITE_P(Container);

TYPED_TEST_P(Container, traverse)
{
  auto N = this->N;
  TypeParam c = this->c;
  EXPECT_EQ(c.size(), N);
  DynList<int> l;
  EXPECT_TRUE(c.traverse([&l] (auto & k) { l.append(k); return true; }));
  EXPECT_TRUE(zip_all([] (auto t) { return get<0>(t) == get<1>(t); },
		      this->item_list, sort(l)));
}

TYPED_TEST_P(Container, for_each)
{
  auto N = this->N;
  TypeParam c = this->c;
  EXPECT_EQ(c.size(), N);
  DynList<int> l;
  c.for_each([&l] (auto & k) { l.append(k); });
  EXPECT_TRUE(zip_all([] (auto t) { return get<0>(t) == get<1>(t); },
		      this->item_list, sort(l)));
}

TYPED_TEST_P(Container, find_ptr)
{
  auto N = this->N;
  TypeParam c = this->c;
  EXPECT_EQ(c.size(), N);
  auto ptr = c.find_ptr([N] (auto & k) { return k == int(N); });
  EXPECT_EQ(ptr, nullptr);
  this->item_list.for_each([&c] (auto & k)
    {
      auto ptr = c.find_ptr([k] (auto i) { return k == i; });
      ASSERT_NE(ptr, nullptr);
      ASSERT_EQ(*ptr, k);
    });
}

TYPED_TEST_P(Container, find_index_nth)
{
  auto N = this->N;
  TypeParam c = this->c;
  EXPECT_EQ(c.size(), N);

  auto idx = c.find_index([N] (auto & k) { return k == int(N); });
  ASSERT_EQ(idx, N);

  this->item_list.for_each([&c] (auto & k)
    {
      auto idx = c.find_index([k] (auto i) { return k == i; });
      ASSERT_EQ(c.nth(idx), k);
    });
}

TYPED_TEST_P(Container, find_item)
{
  auto N = this->N;
  TypeParam c = this->c;
  EXPECT_EQ(c.size(), N);

  auto t = c.find_item([N] (auto & k) { return k == int(N); });
  ASSERT_FALSE(get<0>(t));

  this->item_list.for_each([&c] (auto & k)
    {
      auto t = c.find_item([k] (auto i) { return k == i; });
      EXPECT_TRUE(get<0>(t));
      ASSERT_EQ(get<1>(t), k);
    });
}

TYPED_TEST_P(Container, iterator_operations)
{
  //auto N = this->N;
  auto c = this->c;
  const DynList<int> l = to_dynlist(c); // in the same order than iterator
  const std::vector<int> v = c.to_vector(); // test to_vector method
  const DynList<int> l2 = c.to_dynlist(); // test to_dynlist method

  ASSERT_EQ(l.size(), c.size());
  ASSERT_EQ(v.size(), c.size());
  ASSERT_EQ(l2.size(), c.size());
  
  // Verify to_vector and to_dynlist produce same content
  size_t idx = 0;
  l2.for_each([&v, &idx](int x) {
    EXPECT_EQ(x, v[idx++]);
  });

  auto itl = l.get_it();
  for (auto & item : c)
    {
      ASSERT_EQ(item, itl.get_curr_ne());
      itl.next_ne();
    }
  auto it = c.get_it();
  EXPECT_EQ(it.get_curr_ne(), l.get_first());
  it.reset_last();
  EXPECT_EQ(it.get_curr_ne(), l.get_last());
  it.reset_first();
  EXPECT_EQ(it.get_curr_ne(), l.get_first());
  it.reset_last();
  EXPECT_EQ(it.get_curr_ne(), l.get_last());
}

TYPED_TEST_P(Container, nappend)
{
  int N = this->N;
  auto c = this->c;

  c.nappend(N);
  auto ptr = c.find_ptr([N] (auto i) { return i == N; });
  EXPECT_EQ(c.size(), N + 1);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N);

  c.nappend(N + 1, N + 2, N + 3);
  EXPECT_EQ(c.size(), N + 4);

  ptr = c.find_ptr([N] (auto i) { return i == N + 1; });
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N + 1);

  ptr = c.find_ptr([N] (auto i) { return i == N + 2; });
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N + 2);

  ptr = c.find_ptr([N] (auto i) { return i == N + 3; });
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N + 3);
}

TYPED_TEST_P(Container, ninsert)
{
  int N = this->N;
  auto c = this->c;

  c.ninsert(N);
  auto ptr = c.find_ptr([N] (auto i) { return i == N; });
  EXPECT_EQ(c.size(), N + 1);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N);

  c.ninsert(N + 1, N + 2, N + 3);
  EXPECT_EQ(c.size(), N + 4);

  ptr = c.find_ptr([N] (auto i) { return i == N + 1; });
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N + 1);

  ptr = c.find_ptr([N] (auto i) { return i == N + 2; });
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N + 2);

  ptr = c.find_ptr([N] (auto i) { return i == N + 3; });
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, N + 3);
}

TYPED_TEST_P(Container, all)
{
  int N = this->N;
  auto c = this->c;
  DynSetTree<int> tbl;
  ASSERT_TRUE(c.all([&tbl] (auto i)
		    {
		      const bool ret = tbl.contains(i);
		      tbl.insert(i);
		      return not ret;
		    }));
  EXPECT_EQ(tbl.size(), N);
  EXPECT_EQ(sort(to_dynlist(c)), tbl.keys());
}

TYPED_TEST_P(Container, exists)
{
  int N = this->N;
  auto c = this->c;
  auto & l = this->item_list;
  EXPECT_TRUE(l.all([&c] (auto & i)
		    { return c.exists([i] (auto k) { return i == k; }); }));
  EXPECT_FALSE(c.exists([N] (auto i) { return i == N; }));
}

TYPED_TEST_P(Container, maps)
{
  auto c = this->c;
  auto & l = this->item_list;
  auto fct = [] (int i) { return i + 1; };
  EXPECT_TRUE(zip(sort(to_dynlist(c.maps(fct))), sort(l.maps(fct))).
	      all([] (auto & p) { return p.first == p.second; }));
  EXPECT_TRUE(zip(sort(to_dynlist(c.maps_if([] (auto i)
					    { return i < 7; }, fct))),
		  sort(l.maps_if([] (auto i)
				 { return i < 7; }, fct))).
	      all([] (auto & p) { return p.first == p.second; }));
}

TYPED_TEST_P(Container, map_synonyms)
{
  auto c = this->c;
  auto & l = this->item_list;
  auto fct = [] (int i) { return i + 1; };
  EXPECT_TRUE(zip(sort(to_dynlist(c.map(fct))), sort(l.map(fct))).
	      all([] (auto & p) { return p.first == p.second; }));
  EXPECT_TRUE(zip(sort(to_dynlist(c.map_if([] (auto i)
					   { return i < 7; }, fct))),
		  sort(l.map_if([] (auto i)
				{ return i < 7; }, fct))).
	      all([] (auto & p) { return p.first == p.second; }));
}

TYPED_TEST_P(Container, foldl)
{
  int N = this->N;
  auto c = this->c;
  auto sum = c.foldl(0, [] (auto & a, auto & i) { return a + i; });
  EXPECT_EQ(sum, N*(N-1)/2);
}

TYPED_TEST_P(Container, filter_ops)
{
  auto fct = [] (int a, int i) { return a + i; };
  auto c = this->c;
  auto sum = c.filter([] (auto i) { return i < 8; }).foldl(0, fct);
  EXPECT_EQ(sum, 28);

  auto l = c.ptr_filter([] (auto & i) { return i < 8; });
  sum = l.foldl(0, [] (auto a, auto ptr) { return a + *ptr; });
  EXPECT_EQ(sum, 28);

  int N = this->N;
  auto total = N*(N-1)/2;
  auto p = c.partition([] (auto & i) { return i < 8; });
  auto S = p.first.foldl(0, fct) + p.second.foldl(0, fct);
  EXPECT_EQ(S, total);

  auto t = c.tpartition([] (auto & i) { return i < 8; });
  S = get<0>(t).foldl(0, fct) + get<1>(t).foldl(0, fct);
  EXPECT_EQ(S, total);

  auto l1 = c.take(8);
  auto l2 = c.drop(8);
  S = l1.foldl(0, fct) + l2.foldl(0, fct);
  EXPECT_EQ(S, total);

  EXPECT_EQ(sort(c.to_dynlist()).take(8, 12),
	    build_dynlist<int>(8, 9, 10, 11, 12));
}

/// Matching criterion that cannot throw.
struct Nothrow_Match
{
  int key;
  bool operator()(const int & i) const noexcept { return i == key; }
};

TYPED_TEST_P(Container, nth_out_of_range)
{
  constexpr size_t largest = numeric_limits<size_t>::max();
  const TypeParam & c = this->c;
  EXPECT_THROW((void) c.nth(this->N), out_of_range);
  EXPECT_THROW((void) c.nth(largest), out_of_range);

  // nth(SIZE_MAX) on an empty container used to return a null reference:
  // its check compared the count with n + 1, which overflows to zero.
  TypeParam empty;
  EXPECT_THROW((void) empty.nth(0), out_of_range);
  EXPECT_THROW((void) empty.nth(largest), out_of_range);
  EXPECT_THROW((void) as_const(empty).nth(largest), out_of_range);
}

TYPED_TEST_P(Container, take_with_step)
{
  const TypeParam & c = this->c;
  vector<int> order;  // positions refer to the traversal order
  c.for_each([&order] (int i) { order.push_back(i); });
  ASSERT_EQ(order.size(), this->N);

  constexpr size_t largest = numeric_limits<size_t>::max();
  const size_t cases[][3] = { {0, 999, 1}, {0, 999, 2}, {1, 998, 3}, {5, 5, 7},
                              {0, 10, 4}, {990, largest, 3}, {0, 999, 1000},
                              {3, 900, largest}, {999, 999, 1}, {10, 5, 2},
                              {0, 4, 0}, {1000, 1005, 1}, {5000, 6000, 3},
                              {largest, largest, 1} };
  for (const auto & [i, j, step] : cases)
    {
      vector<int> expected;
      if (step != 0)
        for (size_t p = i; p <= j and p < order.size(); )
          {
            expected.push_back(order[p]);
            if (j - p < step)
              break;
            p += step;
          }
      vector<int> taken;
      c.take(i, j, step).for_each([&taken] (int x) { taken.push_back(x); });
      EXPECT_EQ(taken, expected) << "take(" << i << ", " << j << ", " << step << ")";
    }
}

TYPED_TEST_P(Container, mutable_drop_needs_remove)
{
  // mutable_drop() calls remove(); it used to be visible on containers
  // without remove(), where using it failed to compile.
  constexpr bool has_remove = requires (TypeParam & c) { c.remove(); };
  static_assert((requires (TypeParam & c) { c.mutable_drop(size_t{}); }) == has_remove);
  if constexpr (has_remove)
    {
      TypeParam c = this->c;
      const int eleventh = c.nth(10);
      c.mutable_drop(10);
      EXPECT_EQ(c.size(), this->N - 10);
      EXPECT_EQ(c.nth(0), eleventh);
    }
}

// Traversals that are plain loops over lists, arrays or hash tables cannot
// throw by themselves: the queries built on them inherit their noexcept.
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<DynList<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<DynDlist<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<DynArray<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<ArrayQueue<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<ArrayStack<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<FixedStack<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<FixedQueue<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<DynListQueue<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<DynListStack<int>, int>);
static_assert(Aleph::dry_detail::const_traversal_is_noexcept<DynArrayHeap<int>, int>);
static_assert(noexcept(declval<const DynList<int> &>().find_item(Nothrow_Match{0})));
static_assert(noexcept(declval<const DynArray<int> &>().length()));

TYPED_TEST_P(Container, find_item_noexcept_for_nothrow_items)
{
  // Copying or default-constructing an int cannot throw, so with a
  // noexcept criterion find_item() is noexcept exactly when the traversal
  // of the container cannot throw by itself (F2).
  constexpr bool nothrow_traversal =
    Aleph::dry_detail::const_traversal_is_noexcept<TypeParam, int>;
  static_assert(noexcept(declval<TypeParam &>().find_item(Nothrow_Match{0})) == nothrow_traversal);
  static_assert(noexcept(declval<const TypeParam &>().find_item(Nothrow_Match{0})) == nothrow_traversal);
  const TypeParam & c = this->c;
  const auto [found, item] = c.find_item(Nothrow_Match{7});
  EXPECT_TRUE(found);
  EXPECT_EQ(item, 7);
}


/// Operations that need to modify the item: queries must reject them.
struct Modifying_Pred
{
  bool operator()(int & x) const { x = -x; return true; }
};

struct Modifying_Act
{
  void operator()(int & x) const { x = -x; }
};

TYPED_TEST_P(Container, queries_reject_modifying_operations)
{
  // A constant container hands out its items as constant references: an
  // operation that needs `int &` does not compile with any query (G4).
  using C = TypeParam;
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.traverse(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { traverse(c, op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.all(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.exists(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.none(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.count_if(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.find_ptr(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.find_index(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.find_item(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.find_opt(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.contains_if(op); });
  static_assert(not requires (const C & c, Modifying_Pred & op) { c.filter(op); });
  static_assert(not requires (const C & c, Modifying_Act & op) { c.for_each(op); });
  static_assert(not requires (const C & c, Modifying_Act & op) { c.each(op); });
  static_assert(not requires (const C & c, Modifying_Act & op) { c.each(0, 1, op); });

  // The same operations are accepted when they only read.
  auto reads = [] (const int & x) { return x >= 0; };
  static_assert(requires (const C & c) { c.traverse(reads); c.all(reads); c.find_ptr(reads); });
}

TYPED_TEST_P(Container, each_with_position_and_slice)
{
  const TypeParam & c = this->c;
  vector<int> order;
  c.for_each([&order] (int i) { order.push_back(i); });

  constexpr size_t largest = numeric_limits<size_t>::max();
  const size_t cases[][2] = { {0, 1}, {3, 7}, {999, 1}, {1000, 1}, {5000, 2},
                              {0, largest}, {10, 0} };
  for (const auto & [pos, slice] : cases)
    {
      vector<int> expected;
      if (slice != 0)
        for (size_t p = pos; p < order.size(); )
          {
            expected.push_back(order[p]);
            if (order.size() - p <= slice)
              break;
            p += slice;
          }
      vector<int> visited;
      EXPECT_NO_THROW(c.each(pos, slice, [&visited] (int x) { visited.push_back(x); }));
      EXPECT_EQ(visited, expected) << "each(" << pos << ", " << slice << ")";
    }

  // An empty container visits nothing and does not throw (G10)
  const TypeParam empty;
  size_t calls = 0;
  EXPECT_NO_THROW(empty.each(0, 1, [&calls] (int) { ++calls; }));
  EXPECT_EQ(calls, 0u);
}

TYPED_TEST_P(Container, functional_operations)
{
  const TypeParam & c = this->c;
  vector<int> order;
  c.for_each([&order] (int i) { order.push_back(i); });
  const auto pred = [] (int x) { return x % 3 == 0; };

  EXPECT_EQ(c.count_if(pred), size_t(count_if(order.begin(), order.end(), pred)));
  EXPECT_TRUE(c.none([] (int x) { return x < 0; }));
  EXPECT_FALSE(c.none(pred));

  const auto found = c.find_opt([&order] (int x) { return x == order[17]; });
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(*found, order[17]);
  EXPECT_FALSE(c.find_opt([] (int x) { return x < 0; }).has_value());

  // take_while() and drop_while() split at the first item that fails
  const auto small = [&order] (int x) { return x != order[400]; };
  vector<int> prefix, suffix;
  c.take_while(small).for_each([&prefix] (int x) { prefix.push_back(x); });
  c.drop_while(small).for_each([&suffix] (int x) { suffix.push_back(x); });
  EXPECT_EQ(prefix, vector<int>(order.begin(), order.begin() + 400));
  EXPECT_EQ(suffix, vector<int>(order.begin() + 400, order.end()));

  // flat_map() concatenates in traversal order
  vector<int> doubled;
  c.flat_map([] (int x) { return DynList<int>({x, -x}); })
    .for_each([&doubled] (int x) { doubled.push_back(x); });
  ASSERT_EQ(doubled.size(), 2 * order.size());
  EXPECT_EQ(doubled[2 * 5], order[5]);
  EXPECT_EQ(doubled[2 * 5 + 1], -order[5]);

  // foldr() folds from the last item: building a list rebuilds the order
  const auto rebuilt = c.foldr(DynList<int>(), [] (int x, DynList<int> acc)
                                 {
                                   acc.insert(x);
                                   return acc;
                                 });
  vector<int> again;
  rebuilt.for_each([&again] (int x) { again.push_back(x); });
  EXPECT_EQ(again, order);

  // reduce() needs no initial value; an empty container gives nullopt
  const auto total = c.reduce([] (int a, int x) { return a + x; });
  ASSERT_TRUE(total.has_value());
  EXPECT_EQ(*total, accumulate(order.begin(), order.end(), 0));
  EXPECT_FALSE(TypeParam().reduce([] (int a, int x) { return a + x; }).has_value());
}

TYPED_TEST_P(Container, unchecked_iteration_matches_the_checked_one)
{
  // The _ne methods are the checked ones without the checks: in the
  // pattern `for (...; it.has_curr(); it.next_ne())` they give the same
  // sequence (G21).
  const TypeParam & c = this->c;
  vector<int> checked, unchecked;
  for (auto it = c.get_it(); it.has_curr(); it.next())
    checked.push_back(it.get_curr());
  for (auto it = c.get_it(); it.has_curr(); it.next_ne())
    unchecked.push_back(it.get_curr_ne());
  EXPECT_EQ(checked, unchecked);
  EXPECT_EQ(checked.size(), this->N);
}

REGISTER_TYPED_TEST_SUITE_P(Container, traverse, for_each, find_ptr,
                            find_index_nth, find_item, iterator_operations,
                            nappend, ninsert, all, exists, maps, map_synonyms,
                            foldl, filter_ops, nth_out_of_range, take_with_step,
                            mutable_drop_needs_remove,
                            find_item_noexcept_for_nothrow_items,
                            queries_reject_modifying_operations,
                            each_with_position_and_slice,
                            functional_operations,
                            unchecked_iteration_matches_the_checked_one);

typedef
Types< DynList<int>, DynDlist<int>,  DynArray<int>,
       HashSet<int, ODhashTable>, HashSet<int, OLhashTable>,
       DynHashTable<int, LhashTable>,
       DynHashTable<int, LinearHashTable>, DynSetHash<int>,
       DynSetTree<int, Treap>, DynSetTree<int, Treap_Rk>,
       DynSetTree<int, Rand_Tree>, DynSetTree<int, Splay_Tree>,
       DynSetTree<int, Avl_Tree>, DynSetTree<int, Rb_Tree>,
       Array<int>, ArrayQueue<int>, ArrayStack<int>, DynListQueue<int>,
       DynListStack<int>, DynArrayHeap<int>, DynBinHeap<int>,
       FixedQueue<int>, FixedStack<int>
      >
  Ctypes;

INSTANTIATE_TYPED_TEST_SUITE_P(traverses, Container, Ctypes);

template <class C>
struct CtorContainer : public ::testing::Test
{
  static constexpr size_t N = 10;
  C * ptr_1 = nullptr;
  C * ptr_2 = nullptr;
  C * ptr_3 = nullptr;
  CtorContainer()
  {
    ptr_1 = new C(range<int>(N));
    ptr_2 = new C({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 });
    ptr_3 = new C(ptr_1->begin(), ptr_1->end());  // Use same container for begin/end
  }
  ~CtorContainer()
  {
    delete ptr_1;
    delete ptr_2;
    delete ptr_3;
  }
};

TYPED_TEST_SUITE_P(CtorContainer);

TYPED_TEST_P(CtorContainer, ctor)
{
  auto N = this->N;
  auto ptr_1 = this->ptr_1;
  auto ptr_2 = this->ptr_2;
  auto ptr_3 = this->ptr_3;
  EXPECT_EQ(ptr_1->size(), N);
  EXPECT_EQ(ptr_2->size(), 10);
  EXPECT_EQ(ptr_3->size(), 10);

  auto l1 = to_dynlist(*ptr_1);
  auto l2 = to_dynlist(*ptr_2);
  auto l3 = to_dynlist(*ptr_3);

  const auto r1 = range<int>(N);
  const auto r2 = build_dynlist<int>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
  const auto & r3 = r1;

  ASSERT_EQ(sort(l1), r1);
  ASSERT_EQ(sort(l2), r2);
  ASSERT_EQ(sort(l3), r3);
}

REGISTER_TYPED_TEST_SUITE_P(CtorContainer, ctor);

INSTANTIATE_TYPED_TEST_SUITE_P(Ctors, CtorContainer, Ctypes);

TEST(EqualSequenceMethod, comparison)
{
  Array<int> a1 = {1, 2, 3, 4, 5};
  Array<int> a2 = {1, 2, 3, 4, 5};
  Array<int> a3 = {1, 2, 3, 4};
  Array<int> a4 = {5, 4, 3, 2, 1};
  Array<int> a5 = {1, 2, 2, 3, 4, 5};

  // 1. Size mismatch
  EXPECT_FALSE(a1.equal_to(a3));
  EXPECT_FALSE(a1 == a3);
  EXPECT_TRUE(a1 != a3);

  // 2. Self-comparison
  EXPECT_TRUE(a1.equal_to(a1));
  EXPECT_TRUE(a1 == a1);
  EXPECT_FALSE(a1 != a1);

  // 3. Same elements, same order
  EXPECT_TRUE(a1.equal_to(a2));
  EXPECT_TRUE(a1 == a2);
  EXPECT_FALSE(a1 != a2);

  // 4. Same elements, different order
  EXPECT_FALSE(a1.equal_to(a4));
  EXPECT_FALSE(a1 == a4);
  EXPECT_TRUE(a1 != a4);

  // 5. Multiplicity differences
  EXPECT_FALSE(a1.equal_to(a5));
  EXPECT_FALSE(a1 == a5);

  // DynArray
  DynArray<int> d1 = {1, 2, 3};
  DynArray<int> d2 = {1, 2, 3};
  DynArray<int> d3 = {3, 2, 1};

  EXPECT_TRUE(d1 == d2);
  EXPECT_FALSE(d1 == d3);
}

TEST(StdMapCoexistence, map_method_with_std_map)
{
  std::map<int, std::string> std_map;
  std_map[1] = "one";
  std_map[2] = "two";
  std_map[3] = "three";

  DynList<int> aleph_list = {1, 2, 3, 4, 5};

  auto mapped = aleph_list.map([] (int x) { return x * 2; });
  EXPECT_EQ(mapped.size(), 5);

  EXPECT_EQ(std_map.size(), 3);
  EXPECT_EQ(std_map[1], "one");

  auto filtered_mapped = aleph_list.map_if(
    [] (int x) { return x > 2; },
    [] (int x) { return x * 3; }
  );
  EXPECT_EQ(filtered_mapped.size(), 3);

  std::map<std::string, int> another_map;
  another_map["a"] = 10;
  another_map["b"] = 20;

  EXPECT_EQ(another_map.size(), 2);
  EXPECT_EQ(another_map["a"], 10);

  auto result = mapped.foldl(0, [] (int acc, int val) { return acc + val; });
  EXPECT_EQ(result, 30);
}

namespace
{
  /// Element whose copy constructor throws while `armed` is set.
  struct Copy_Throws
  {
    static inline bool armed = false;
    int value = 0;

    Copy_Throws() noexcept = default;
    explicit Copy_Throws(int v) noexcept : value(v) {}
    Copy_Throws(const Copy_Throws & other) : value(other.value)
    {
      if (armed)
        throw runtime_error("copy failed");
    }
    Copy_Throws & operator = (const Copy_Throws &) noexcept = default;
  };

  /// Element whose default constructor throws while `armed` is set.
  struct Default_Throws
  {
    static inline bool armed = false;
    int value = 0;

    Default_Throws()
    {
      if (armed)
        throw runtime_error("default construction failed");
    }
    explicit Default_Throws(int v) noexcept : value(v) {}
  };

  /// find_item() copies the found item, or default-constructs one when
  /// nothing matches. Either may throw even if the criterion cannot, and
  /// find_item() used to be noexcept anyway, so std::terminate was called.
  template <class C>
  void check_find_item_propagates()
  {
    using T = typename C::Item_Type;
    auto match = [] (const T & x) noexcept { return x.value == 2; };
    auto never = [] (const T &) noexcept { return false; };
    static_assert(not noexcept(declval<C &>().find_item(match)));
    static_assert(not noexcept(declval<const C &>().find_item(match)));

    C c;
    for (int i = 0; i < 4; ++i)
      c.append(T(i));
    const C & cc = c;

    T::armed = true;
    if constexpr (is_same_v<T, Copy_Throws>)
      {
        EXPECT_THROW((void) c.find_item(match), runtime_error);
        EXPECT_THROW((void) cc.find_item(match), runtime_error);
      }
    else
      {
        EXPECT_THROW((void) c.find_item(never), runtime_error);
        EXPECT_THROW((void) cc.find_item(never), runtime_error);
      }
    T::armed = false;

    const auto [found, item] = c.find_item(match);
    EXPECT_TRUE(found);
    EXPECT_EQ(item.value, 2);
  }
}

TEST(LocateFunctions, find_item_propagates_copy_exceptions)
{
  check_find_item_propagates<Array<Copy_Throws>>();
  check_find_item_propagates<DynList<Copy_Throws>>();
  check_find_item_propagates<DynArray<Copy_Throws>>();
}

TEST(LocateFunctions, find_item_propagates_default_construction_exceptions)
{
  check_find_item_propagates<Array<Default_Throws>>();
  check_find_item_propagates<DynList<Default_Throws>>();
  check_find_item_propagates<DynArray<Default_Throws>>();
}

namespace
{
  /// Counts its copies; moving it is free.
  struct Copy_Counted
  {
    static inline size_t copies = 0;
    vector<int> items;

    Copy_Counted() = default;
    Copy_Counted(const Copy_Counted & o) : items(o.items) { ++copies; }
    Copy_Counted(Copy_Counted && o) noexcept : items(std::move(o.items)) {}
    Copy_Counted & operator = (const Copy_Counted & o)
    {
      items = o.items;
      ++copies;
      return *this;
    }
    Copy_Counted & operator = (Copy_Counted && o) noexcept
    {
      items = std::move(o.items);
      return *this;
    }
    bool operator == (const Copy_Counted & o) const { return items == o.items; }
  };

  template <class C>
  constexpr bool foldl_without_operation = requires (const C & c) { c.foldl(0); };

  template <class C, class Arg>
  constexpr bool can_emplace = requires (C & c, Arg a) { c.emplace(a); };

  template <class C, class Op>
  constexpr bool can_find_item = requires (const C & c, Op & op) { c.find_item(op); };

  template <class C>
  constexpr bool has_equality = requires (const C & a, const C & b) { a == b; };

  template <class C>
  constexpr bool can_modify_each = requires (C & c, Modifying_Act & op) { c.mutable_for_each(op); };

  struct No_Default
  {
    int value;
    explicit No_Default(int v) : value(v) {}
    bool operator == (const No_Default & o) const { return value == o.value; }
  };

  struct Item
  {
    int key;
    bool valid;
    [[nodiscard]] bool is_valid() const noexcept { return valid; }
  };
}

TEST(FunctionalMethods, maps_deduces_the_item_type)
{
  // The item type of the result is that of the operation (G5): x / 2.0
  // used to be truncated into a DynList<int>.
  DynList<int> l = {1, 2, 3};
  auto halves = l.maps([] (int x) { return x / 2.0; });
  static_assert(is_same_v<decltype(halves), DynList<double>>);
  EXPECT_DOUBLE_EQ(halves.get_first(), 0.5);
  EXPECT_DOUBLE_EQ(halves.get_last(), 1.5);

  auto names = l.maps([] (int x) { return to_string(x); });
  static_assert(is_same_v<decltype(names), DynList<string>>);
  EXPECT_EQ(names.get_last(), "3");

  // A reference result is decayed; an explicit type is still honoured
  auto same = l.maps([] (const int & x) -> const int & { return x; });
  static_assert(is_same_v<decltype(same), DynList<int>>);
  auto longs = l.template maps<long>([] (int x) { return x; });
  static_assert(is_same_v<decltype(longs), DynList<long>>);
  auto odd_halves = l.maps_if([] (int x) { return x % 2; }, [] (int x) { return x / 2.0; });
  static_assert(is_same_v<decltype(odd_halves), DynList<double>>);
  EXPECT_EQ(odd_halves.size(), 2u);
  static_assert(is_same_v<decltype(l.map([] (int x) { return x * 1.0f; })), DynList<float>>);
  static_assert(is_same_v<decltype(l.map_if([] (int) { return true; }, [] (int x) { return x * 1.0f; })),
                          DynList<float>>);
}

TEST(FunctionalMethods, foldl_moves_the_accumulator)
{
  // Folding into a container took a copy of the accumulator per item: O(n^2).
  DynList<int> l;
  for (int i = 0; i < 1000; ++i)
    l.append(i);

  Copy_Counted::copies = 0;
  const auto by_value = l.foldl(Copy_Counted(), [] (Copy_Counted acc, int x)
                                  {
                                    acc.items.push_back(x);
                                    return acc;
                                  });
  EXPECT_EQ(by_value.items.size(), 1000u);
  EXPECT_EQ(Copy_Counted::copies, 1u);  // only the copy of init

  // An operation that returns a reference to its parameter, which is the
  // accumulator itself, must not leave it self-move-assigned
  Copy_Counted::copies = 0;
  const auto by_rvalue = l.foldl(Copy_Counted(), [] (Copy_Counted && acc, int x) -> Copy_Counted &&
                                   {
                                     acc.items.push_back(x);
                                     return std::move(acc);
                                   });
  EXPECT_EQ(by_rvalue.items.size(), 1000u);
  EXPECT_EQ(Copy_Counted::copies, 1u);

  // An operation that takes the accumulator by modifiable reference still works
  const auto by_lvalue = l.foldl(Copy_Counted(), [] (Copy_Counted & acc, int x) -> Copy_Counted &
                                   {
                                     acc.items.push_back(x);
                                     return acc;
                                   });
  EXPECT_EQ(by_lvalue.items.size(), 1000u);

  EXPECT_EQ(l.fold(0, [] (int a, int x) { return a + x; }), 499500);
  EXPECT_EQ(l.fold_left(0L, [] (long a, int x) { return a + x; }), 499500L);
}

TEST(FunctionalMethods, foldl_needs_an_operation)
{
  // foldl(init) used a default operation that returned T(), discarding
  // init and every item (G7).
  static_assert(not foldl_without_operation<DynList<int>>);
  static_assert(not foldl_without_operation<DynArray<int>>);
}

TEST(FunctionalMethods, tpartition_moves_the_lists)
{
  DynList<Copy_Counted> l;
  for (int i = 0; i < 100; ++i)
    {
      Copy_Counted c;
      c.items.push_back(i);
      l.append(std::move(c));
    }
  const auto even = [] (const Copy_Counted & c) { return c.items[0] % 2 == 0; };

  Copy_Counted::copies = 0;
  const auto p = l.partition(even);
  const size_t pair_copies = Copy_Counted::copies;
  Copy_Counted::copies = 0;
  const auto t = l.tpartition(even);
  EXPECT_EQ(pair_copies, 100u);
  EXPECT_EQ(Copy_Counted::copies, pair_copies);  // it used to copy both lists again
  EXPECT_EQ(get<0>(t).size(), p.first.size());
}

TEST(FunctionalMethods, projections_with_member_pointers)
{
  const DynList<Item> l = {Item{1, true}, Item{2, false}, Item{3, true}};
  EXPECT_EQ(l.filter(&Item::is_valid).size(), 2u);
  EXPECT_EQ(l.maps(&Item::key).get_last(), 3);
  static_assert(is_same_v<decltype(l.maps(&Item::key)), DynList<int>>);
  EXPECT_FALSE(l.all(&Item::valid));
  EXPECT_TRUE(l.exists(&Item::valid));
  EXPECT_EQ(l.count_if(&Item::is_valid), 2u);
  EXPECT_EQ(l.find_ptr(&Item::is_valid)->key, 1);
  EXPECT_EQ(l.find_index(&Item::valid), 0u);
  EXPECT_EQ(l.partition(&Item::valid).second.size(), 1u);
}

TEST(FunctionalMethods, emplace_does_not_cast)
{
  // emplace(arg) built T(arg), a C-style cast for one argument: it dropped
  // const and turned pointers into integers (G13).
  static_assert(not can_emplace<DynList<int *>, const int *>);
  static_assert(not can_emplace<DynList<long>, int *>);
  static_assert(can_emplace<DynList<int *>, int *>);
  static_assert(can_emplace<DynList<string>, const char *>);

  DynList<string> l;
  l.emplace(3, 'x');
  l.emplace_ins("first");
  EXPECT_EQ(l.get_first(), "first");
  EXPECT_EQ(l.get_last(), "xxx");
  EXPECT_EQ(l.nappend(string("a"), string("b")), 2u);
  EXPECT_EQ(l.ninsert(string("z")), 1u);
  EXPECT_EQ(l.size(), 5u);
}

TEST(LocateFunctions, find_item_needs_a_default_constructor_find_opt_does_not)
{
  auto is_two = [] (const No_Default & x) { return x.value == 2; };
  static_assert(not can_find_item<DynList<No_Default>, decltype(is_two)>);  // G12
  DynList<No_Default> l;
  l.append(No_Default(1));
  l.append(No_Default(2));
  const auto found = l.find_opt(is_two);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->value, 2);
  EXPECT_FALSE(l.find_opt([] (const No_Default & x) { return x.value == 9; }).has_value());
}

/* In the next two tests the containers are instantiated before their item
   type is complete. At that point Clang 14 checked the mixin constraints
   that depend only on the item type, and cached `false`: the members
   vanished, and the cached traits also broke later checks. These tests did
   not compile with Clang 14 until those constraints were deferred to the
   call. */
namespace
{
  // Item's default member initializer is parsed only when Nested_Items is
  // complete, so until then Item is not default-constructible.
  struct Nested_Items
  {
    struct Item
    {
      int value = 2;
    };

    Array<Item> array;
    DynArray<Item> dynarray;
    DynList<Item> list;
  };

  // The list is instantiated while Recursive_Item is incomplete, and the
  // operator == is declared after it.
  struct Recursive_Item
  {
    int value = 2;
    DynList<Recursive_Item> children;

    bool operator == (const Recursive_Item & o) const { return value == o.value; }
  };
}

TEST(IncompleteItemType, nested_type_with_default_member_initializers)
{
  // The cached default constructibility also failed the static_asserts of
  // Array and DynArray.
  Nested_Items n;
  n.array.append(Nested_Items::Item());
  n.dynarray.append(Nested_Items::Item());
  n.list.append(Nested_Items::Item());

  auto is_two = [] (const Nested_Items::Item & i) { return i.value == 2; };
  EXPECT_TRUE(get<0>(n.array.find_item(is_two)));
  EXPECT_TRUE(get<0>(n.dynarray.find_item(is_two)));
  EXPECT_TRUE(get<0>(n.list.find_item(is_two)));
}

TEST(IncompleteItemType, recursive_type)
{
  // A cached `is_nothrow_destructible` also made the item look not
  // destructible, so emplace() vanished too.
  Recursive_Item r;
  r.children.emplace();
  r.children.append(Recursive_Item());

  auto is_two = [] (const Recursive_Item & i) { return i.value == 2; };
  EXPECT_TRUE(get<0>(r.children.find_item(is_two)));
  EXPECT_TRUE(r.children.find_opt(is_two).has_value());
  EXPECT_TRUE(r.children.contains(Recursive_Item()));
  auto keep_first = [] (Recursive_Item a, const Recursive_Item &) { return a; };
  EXPECT_EQ(r.children.reduce(keep_first)->value, 2);
}

TEST(EqualityMethods, sequences_compare_order_and_repetitions)
{
  // Stacks and queues used EqualToMethod, a set equality: [1,2] == [2,1]
  // and {1,1,2} == {1,2,2} were true (G3).
  DynListStack<int> s1, s2, s3, s4;
  s1.push(1); s1.push(2);
  s2.push(2); s2.push(1);
  s3.push(1); s3.push(1); s3.push(2);
  s4.push(1); s4.push(2); s4.push(2);
  EXPECT_FALSE(s1 == s2);
  EXPECT_FALSE(s3 == s4);
  EXPECT_TRUE(s1 != s2);

  DynListQueue<int> q1, q2;
  q1.put(1); q1.put(2);
  q2.put(2); q2.put(1);
  EXPECT_FALSE(q1 == q2);
  q2.get(); q2.put(2);  // q2 is now [1, 2]
  EXPECT_TRUE(q1 == q2);

  // Stacks and queues on arrays: equality used to not even compile
  ArrayStack<int> a1, a2;
  a1.push(1); a1.push(2);
  a2.push(1); a2.push(2);
  EXPECT_TRUE(a1 == a2);
  a2.push(3);
  EXPECT_FALSE(a1 == a2);
  FixedStack<int> f1(4), f2(4);
  f1.push(1); f2.push(1);
  EXPECT_TRUE(f1 == f2);
  ArrayQueue<int> aq1, aq2;
  aq1.put(1); aq1.put(2);
  aq2.put(2); aq2.put(1);
  EXPECT_FALSE(aq1 == aq2);
  FixedQueue<int> fq1(4), fq2(4);
  fq1.put(5); fq2.put(5);
  EXPECT_TRUE(fq1 == fq2);
}

TEST(EqualityMethods, sorted_containers_count_repetitions)
{
  DynSetTree<int> t1, t2;
  t1.insert_dup(1); t1.insert_dup(1); t1.insert_dup(2);
  t2.insert_dup(1); t2.insert_dup(2); t2.insert_dup(2);
  EXPECT_FALSE(t1 == t2);
  t2.remove(2); t2.insert_dup(1);
  EXPECT_TRUE(t1 == t2);

  DynIntervalTree<int> i1, i2;
  i1.append(Interval<int>(1, 2)); i1.append(Interval<int>(1, 2)); i1.append(Interval<int>(3, 4));
  i2.append(Interval<int>(1, 2)); i2.append(Interval<int>(3, 4)); i2.append(Interval<int>(3, 4));
  EXPECT_FALSE(i1 == i2);
  DynIntervalTree<int> i3;
  i3.append(Interval<int>(3, 4)); i3.append(Interval<int>(1, 2)); i3.append(Interval<int>(1, 2));
  EXPECT_TRUE(i1 == i3);
  EXPECT_FALSE(i1 == DynIntervalTree<int>());
}

TEST(EqualityMethods, hash_sets_keep_set_equality)
{
  DynSetHash<int> h1, h2;
  for (int i : {1, 2, 3})
    h1.insert(i);
  for (int i : {3, 1, 2})
    h2.insert(i);
  EXPECT_TRUE(h1 == h2);
  h2.remove(3);
  EXPECT_FALSE(h1 == h2);
}

TEST(EqualityMethods, heaps_and_random_sets_have_no_equality)
{
  // EqualToMethod was inherited but never compiled for them (D2)
  static_assert(not has_equality<ArrayHeap<int>>);
  static_assert(not has_equality<DynBinHeap<int>>);
  static_assert(not has_equality<DynArrayHeap<int>>);
  static_assert(not has_equality<Random_Set<int>>);
  static_assert(has_equality<DynListStack<int>>);
  static_assert(has_equality<ArrayQueue<int>>);
}

TEST(RandomSet, iterator_traverses_the_items)
{
  // Random_Set::Iterator derived from DynArray<T>: a container, not an
  // iterator, so range-for, take(i, j, step) and each(pos, slice) did not
  // compile (G11).
  Random_Set<int> q;
  for (int i = 0; i < 10; ++i)
    q.put(i);
  int sum = 0;
  for (const int x : q)
    sum += x;
  EXPECT_EQ(sum, 45);
  size_t n = 0;
  for (auto it = q.get_it(); it.has_curr(); it.next_ne())
    ++n;
  EXPECT_EQ(n, 10u);
  EXPECT_EQ(q.take(0, 9, 3).size(), 4u);
  size_t visited = 0;
  q.each(1, 4, [&visited] (int) { ++visited; });
  EXPECT_EQ(visited, 3u);
}

TEST(ReadOnlyContainers, offer_queries_but_no_modifying_members)
{
  // DynMatrix, Polygon and DynIntervalTree traverse with constant
  // references: the members that modify are not offered, and the queries
  // that used to fail to compile now work (G11).
  static_assert(not can_modify_each<DynMatrix<int>>);
  static_assert(not can_modify_each<DynSkipList<int>>);
  static_assert(not can_modify_each<DynIntervalTree<int>>);
  static_assert(can_modify_each<DynList<int>>);

  DynMatrix<int> m(2, 3);
  m.allocate();
  for (size_t i = 0; i < 2; ++i)
    for (size_t j = 0; j < 3; ++j)
      m.write(i, j, int(3 * i + j));
  EXPECT_EQ(m.nth(4), 4);
  EXPECT_EQ(*m.find_ptr([] (int x) { return x == 5; }), 5);
  EXPECT_TRUE(m.contains(2));
  EXPECT_EQ(m.take(0, 5, 2).size(), 3u);
}

namespace
{
  /// A minimal map container to exercise MapSequencesMethods, which no
  /// container of the library uses.
  struct Pair_Map : public FunctionalMethods<Pair_Map, pair<int, string>>,
                    public MapSequencesMethods<Pair_Map, int, string>
  {
    DynList<pair<int, string>> items_list;

    template <class Op>
      requires Aleph::PredicateWith<Op &, pair<int, string> &>
    bool traverse(Op & op) { return items_list.traverse(op); }

    template <class Op>
      requires Aleph::PredicateWith<Op &, const pair<int, string> &>
    bool traverse(Op & op) const { return items_list.traverse(op); }

    template <class Op>
      requires Aleph::PredicateWith<Op &, pair<int, string> &>
    bool traverse(Op && op) { return items_list.traverse(op); }

    template <class Op>
      requires Aleph::PredicateWith<Op &, const pair<int, string> &>
    bool traverse(Op && op) const { return items_list.traverse(op); }

    string & find(const int & k)
    {
      auto p = items_list.find_ptr([&k] (const pair<int, string> & e) { return e.first == k; });
      ah_domain_error_if(p == nullptr) << "key not found";
      return p->second;
    }

    const string & find(const int & k) const { return const_cast<Pair_Map *>(this)->find(k); }
  };
}

TEST(MapSequencesMethods, members_work)
{
  // keys(), values_ptr(), items() and items_ptr() did not compile (G14)
  Pair_Map m;
  m.items_list.append(make_pair(1, string("one")));
  m.items_list.append(make_pair(2, string("two")));

  EXPECT_EQ(m.keys().get_last(), 2);
  EXPECT_EQ(m.values().get_first(), "one");
  EXPECT_EQ(m.items().size(), 2u);

  *m.values_ptr().get_first() = "uno";
  EXPECT_EQ(m(1), "uno");
  *m.items_ptr().get_last().second = "dos";
  EXPECT_EQ(m(2), "dos");

  const Pair_Map & cm = m;
  static_assert(is_same_v<decltype(cm.values_ptr()), DynList<const string *>>);
  EXPECT_EQ(*cm.items_ptr().get_last().second, "dos");
  EXPECT_THROW(m(3), domain_error);
}
