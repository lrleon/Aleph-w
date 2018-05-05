
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
  static constexpr size_t N = 10;
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

REGISTER_TYPED_TEST_CASE_P(Container, traverse);

typedef Types<DynList<int>, DynDlist<int>,  DynArray<int>,
	      HashSet<int, ODhashTable>, HashSet<int, OLhashTable>,
	      DynHashTable<int, LhashTable>,
	      DynHashTable<int, LinearHashTable>, DynSetHash<int>,
	      DynSetTree<int, Treap>, DynSetTree<int, Treap_Rk>,
	      DynSetTree<int, Rand_Tree>, DynSetTree<int, Splay_Tree>,
	      DynSetTree<int, Avl_Tree>, DynSetTree<int, Rb_Tree>>
  Ctypes;

INSTANTIATE_TYPED_TEST_CASE_P(traverse, Container, Ctypes);
