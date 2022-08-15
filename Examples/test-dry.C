
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

# include <iostream>
# include <vector>
# include <ahSort.H>
# include <htlist.H>
# include <tpl_arrayHeap.H>
# include <tpl_dynArrayHeap.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynDlist.H>
# include <tpl_dynSetTree.H>
# include <tpl_olhash.H>
# include <tpl_odhash.H>
# include <tpl_dynSetHash.H>
# include <tpl_dynArray.H>
# include <tpl_arrayQueue.H>
# include <tpl_dynListStack.H>
# include <tpl_dynarray_set.H>
# include <tpl_random_queue.H>

using namespace std;

using namespace Aleph;

template <class C>
void find_test()
{
  C c = { 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9 };

  const C a = c;

  c.traverse([] (auto i) { cout << " " << i; return true; }); cout << endl;

  a.traverse([] (auto i) { cout << " " << i; return true; }); cout << endl;

  assert(c.all([] (auto i) { return i >= 0; }));
  assert(a.all([] (auto i) { return i >= 0; }));

  int vals[11]; int k = 0;
  c.for_each([&k, &vals] (int i) { vals[k++] = i; });

  c.for_each([&c, vals] (int i) { assert(c.nth_ne(i) == vals[i]); });

  k = 0;
  a.for_each([&k, &vals] (int i) { vals[k++] = i; });
  a.for_each([&a, vals] (int i) { assert(a.nth_ne(i) == vals[i]); });

  assert(c.find_ptr([] (int i) { return i == 5; }));
  assert(a.find_ptr([] (int i) { return i == 5; }));
  assert(not c.find_ptr([] (int i) { return i == 15; }));
  assert(not a.find_ptr([] (int i) { return i == 15; }));
  assert(get<0>(c.find_item([] (int i) { return i == 5; })));
  assert(get<0>(c.find_item([] (int i) { return i == 5; })) and
         get<1>(c.find_item([] (int i) { return i == 5; })) == 5);
  assert(get<0>(a.find_item([] (int i) { return i == 5; })));
  assert(get<0>(a.find_item([] (int i) { return i == 5; })) and
         get<1>(a.find_item([] (int i) { return i == 5; })) == 5);

}

template <class C>
void ctors_test()
{
  C c = { 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9 };
  const C a = c;
  c.traverse([] (auto i) { cout << " " << i; return true; }); cout << endl;
  a.traverse([] (auto i) { cout << " " << i; return true; }); cout << endl;

  C c1 = DynList<typename C::Item_Type>({ 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9 });
  c1.for_each([] (int i) { cout << " " << i; }); cout << endl;

  const C c2 = DynList<typename C::Item_Type>({ 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9 });
  c2.for_each([] (int i) { cout << " " << i; }); cout << endl;

  vector<int> v = { 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9 };

  C c3(v.begin(), v.end());
  c3.for_each([] (int i) { cout << " " << i; }); cout << endl;

  const C c4(v.begin(), v.end());
  c4.for_each([] (int i) { cout << " " << i; }); cout << endl;
}

template <class C>
void functional_test()
{
  C c = { 0, 1, 2, 3, 4, 5, 6, 7, 8 ,9 };
  const C a = c;

  c.for_each([] (int i) { cout << " " << i; }); cout << endl;
  a.for_each([] (int i) { cout << " " << i; }); cout << endl;

  assert(c.all([&a] (int i) { return a.exists([i] (int k) 
                                              { return k == i; }); }));
  assert(a.all([&c] (int i) { return c.exists([i] (int k)
                                              { return k == i; }); }));

  assert(c.exists([] (int i) { return i == 9; }));
  assert(a.exists([] (int i) { return i == 9; }));

  assert(c.all([&c] (int i) 
               { return c.exists([i] (int k) { return i == k; }); }));
  assert(a.all([&a] (int i) 
               { return a.exists([i] (int k) { return i == k; }); }));

  C cm = c.maps([] (int i) { return 10*i; });
  assert(cm.all([&c] (int k) 
                {
                  return c.exists([k] (int i) { return 10*i == k; });
                }));
		
  const C ccm = a.maps([] (int i) { return 10*i; });
  assert(ccm.all([a] (int k) 
                 {
                   return a.exists([k] (int i) { return 10*i == k; });
                 }));

  {
    auto m = c.template maps<string>([] (int i) { return to_string(i); });
  }

  cout << "S1 = " 
       << c.template foldl<int>(0, [] (auto a, auto i) { return a + i; }) 
       << endl
       << "S2 = " 
       << ccm.template foldl<int>(0, [] (int a, int i) { return a + i; }) 
       << endl
       << "S3 = " << c.fold(0, [] (auto a, auto i) { return a + i; }) 
       << endl
       << "S4 = " << a.fold(0, [] (auto a, auto i) { return a + i; }) 
       << endl
       << endl;
  
  assert(eq(build_dynlist<int>(0, 1, 2, 3, 4, 5),
            sort(c.filter([] (int i) { return i < 6; }))));
  assert(eq(build_dynlist<int>(0, 1, 2, 3, 4, 5),
            sort(a.filter([] (int i) { return i < 6; }))));
  
  c.pfilter([] (int i) { return i < 6; }).for_each([] (auto p)
                                                   {
                                                     cout << "(" << get<0>(p) << "," << get<1>(p) << ")";
                                                   });
  cout << endl;
  a.pfilter([] (int i) { return i < 6; }).for_each([] (auto p)
                                                   {
                                                     cout << "(" << get<0>(p) << "," << get<1>(p) << ")";
                                                   });
  cout << endl
       << endl;

  auto cmp_tup = [] (std::tuple<size_t,size_t> t1, std::tuple<size_t,size_t> t2)
                 {
                   return get<0>(t1) < get<0>(t2);
                 };

  auto l1 = sort(c.pfilter([] (int i) { return i < 6; }), cmp_tup);
  auto l2 = sort(a.pfilter([] (int i) { return i < 6; }), cmp_tup);

  l1.for_each([] (auto p)
              {
                cout << "(" << get<0>(p) << "," << get<1>(p) << ")";
              });
  cout << endl;
  l2.for_each([] (auto p)
              {
                cout << "(" << get<0>(p) << "," << get<1>(p) << ")";
              });
  cout << endl
       << endl;

  auto eq_tup = [] (std::tuple<size_t,size_t> t1, std::tuple<size_t,size_t> t2)
                {
                  return get<0>(t1) == get<0>(t2);
                };
  assert(eq(l1 ,l2, eq_tup));

  auto p = c.partition([] (int i) { return i < 6; });
  assert(eq(sort(p.first), build_dynlist<int>(0, 1, 2, 3, 4, 5)) and 
         eq(sort(p.second), build_dynlist<int>(6, 7, 8, 9)));
  p = a.partition([] (int i) { return i < 6; });
  assert(eq(sort(p.first), build_dynlist<int>(0, 1, 2, 3, 4, 5)) and
         eq(sort(p.second), build_dynlist<int>(6, 7, 8, 9)));

  auto t = c.tpartition([] (int i) { return i < 6; });
  assert(eq(sort(get<0>(t)), build_dynlist<int>(0, 1, 2, 3, 4, 5)) and
         eq(sort(get<1>(t)), build_dynlist<int>(6, 7, 8, 9)));
  t = a.tpartition([] (int i) { return i < 6; });
  assert(eq(sort(get<0>(t)), build_dynlist<int>(0, 1, 2, 3, 4, 5)) and
         eq(sort(get<1>(t)), build_dynlist<int>(6, 7, 8, 9)));

  assert(c.length() == 10);
  assert(a.length() == 10);

  c.take(3).for_each([] (auto i) { cout << i << " "; });
  cout << endl;
  a.take(3).for_each([] (auto i) { cout << i << " "; });
  cout << endl
       << endl;

  auto cc = c;
  auto ca = a;

  c.take(3).for_each([] (auto i) { cout << i << " "; });
  cout << endl;
  cc.take(3).for_each([] (auto i) { cout << i << " "; });
  cout << endl
       << endl;

  assert(eq(sort(join(c.take(3), c.drop(3))), sort(c.items())));
  assert(eq(sort(join(a.take(3), a.drop(3))), sort(a.items())));

  cout << "All test were passed!" << endl
       << endl
       << endl;
}

template <class C>
void tests()
{
  cout << "Testing for " << typeid(C).name() << endl
       << endl;

  find_test<C>();
  ctors_test<C>();
  functional_test<C>();

  cout << "Ended tests for " << typeid(C).name() << endl
       << endl;
}

int main()
{
  DynList<int> c = {0, 1, 2, 3};
  DynDlist<int> cc = c;
  tests<DynList<int>>();
  tests<DynDlist<int>>();
  tests<DynSetTree<int>>();
  tests<OLhashTable<int>>();
  tests<ODhashTable<int>>();
  tests<DynSetHash<int>>();
  tests<ArrayHeap<int>>();
  tests<DynArray<int>>();
  tests<DynArrayHeap<int>>();
  tests<DynBinHeap<int>>();
  tests<ArrayQueue<int>>();
  tests<FixedQueue<int>>();
  tests<ArrayStack<int>>();
  tests<FixedStack<int>>();
  tests<DynListStack<int>>();
  tests<DynListQueue<int>>();
  tests<DynArray_Set<int>>();
  tests<Random_Set<int>>();
}




