
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# include <gtest/gtest.h>

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

using namespace std;
using namespace testing;

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
    sort(item_list);
  }
};

TYPED_TEST_CASE_P(Container);

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

  ASSERT_EQ(l.size(), c.size());

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
}

REGISTER_TYPED_TEST_CASE_P(Container, traverse, for_each, find_ptr,
			   find_index_nth, find_item, iterator_operations);

typedef
Types< DynList<int>, DynDlist<int>,  DynArray<int>,
       HashSet<int, ODhashTable>, HashSet<int, OLhashTable>,
       DynHashTable<int, LhashTable>,
       DynHashTable<int, LinearHashTable>, DynSetHash<int>,
       DynSetTree<int, Treap>, DynSetTree<int, Treap_Rk>,
       DynSetTree<int, Rand_Tree>, DynSetTree<int, Splay_Tree>,
       DynSetTree<int, Avl_Tree>, DynSetTree<int, Rb_Tree>,
       Array<int>, ArrayQueue<int>, ArrayStack<int>, DynListQueue<int>,
       DynListStack<int>, DynArrayHeap<int>, DynBinHeap<int>
      >
  Ctypes;

INSTANTIATE_TYPED_TEST_CASE_P(traverses, Container, Ctypes);

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
    ptr_3 = new C(ptr_1->begin(), ptr_2->end());
  }
  ~CtorContainer()
  {
    delete ptr_1;
    delete ptr_2;
    delete ptr_3;
  }
};

TYPED_TEST_CASE_P(CtorContainer);

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

REGISTER_TYPED_TEST_CASE_P(CtorContainer, ctor);

INSTANTIATE_TYPED_TEST_CASE_P(Ctors, CtorContainer, Ctypes);
