
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# include <iostream>

# include <ah-zip.H>
# include <tpl_dynSetTree.H>

using namespace std;
using namespace Aleph;

DynList<int> l1 = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
DynList<int> l2 = { 0, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
DynList<char> l3 = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k' };
DynSetTree<char> l5 = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K' };

int main()
{
  auto it = get_zip_it(l1, l2, l3, l5);
  for (; it.has_curr(); it.next())
    {
      auto curr = it.get_curr();
      cout << "curr = (" << get<0>(curr) << ", " << get<1>(curr) << ", "
	   << get<2>(curr) << ")" << endl;
    }

  zip_for_each([] (auto t) {}, l1, l2);

  assert(not it.completed());

  auto l = t_enum_zip(l1, l2, l3);

  auto lu = t_unzip(l);

  for (auto it = get<3>(lu).get_it(); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;

  auto l4 = l1.maps([] (auto i) { return 10*i; });

  for (auto it = get_zip_it(l1, l2, l4); it.has_curr(); it.next())
    {
      auto l = it.get_curr_list();
      l.for_each([] (auto i) { cout << i << " "; });
      cout << endl;
    }

  zip_lists(l1, l2, l4).for_each([] (auto & l)
				 {
				   l.for_each([] (auto i) { cout << i << " "; });
				   cout << endl;
				 });

  zip_maps<int>
    ([] (tuple<int, int, int> t) { return get<0>(t) + get<1>(t) + get<2>(t); },
     l1, l2, l4).for_each([] (auto i) { cout << i << endl; });

  auto filter = zip_filter([] (auto t) { return get<0>(t) < 5; }, l1, l2, l3, l5);

  for (auto it = filter.get_it(); it.has_curr(); it.next())
    {
      auto & t = it.get_curr();
      cout << get<0>(t) << " " << get<1>(t) << " " << get<2>(t) << " " << get<3>(t)
	   << endl;
    }
}
