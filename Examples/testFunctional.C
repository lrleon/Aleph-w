
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

# include <gsl/gsl_rng.h>
# include <stdlib.h>
# include <cassert>
# include <iostream>
# include <ahFunctional.H>
# include <ahSearch.H>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_dynSetTree.H>
# include <tpl_hash.H>

using namespace std;

gsl_rng * r;

template <template <class> class C, typename T>
void print_seq(const C<T> & c)
{
  for (typename C<T>::Iterator it(c); it.has_curr(); it.next())
    cout << it.get_curr() << " ";

  cout << endl;
}

int main(int argc, char *argv[])
{
  int n = argc > 1 ? atoi(argv[1]) : 100;
  unsigned long seed = argc > 2 ? atoi(argv[2]) : time(NULL);

  cout << argv[0] << " " << n << " " << seed << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);

  DynList<int> l1({1,2,3,4,5,6,7,8,9});

  cout << "** indexes(l1) = ";
  indexes(l1).for_each([] (pair<size_t, const int &> t)
	    {
	      cout << "(" << t.first << "," << t.second << ")";
	    });
  cout << endl;

  DynDlist<int> l2({1,2,3,4,5,6,7,8,9});
  HashSet<int> l3({0, 1,2,3,4,5,6,7,8,9});

  auto to_str = /* Lambda */ [&l1] (int i) -> std::string
    {					    
      return std::to_string(i);
    };

  DynList<std::string> l1_str = maps<std::string>(l1, to_str);

  print_seq(l1);
  print_seq(l2);
  print_seq(l3.keys());
  print_seq(l1_str);

  DynList<std::pair<int, int>> lz1 = zip(DynList<int>(l1), DynDlist<int>(l2));

  for (DynList<std::pair<int,int>>::Iterator it(lz1); it.has_curr(); it.next())
    cout << it.get_curr().first << "," << it.get_curr().second << endl;

  {
    cout << "Building random list" << endl;
    DynList<long> l;
    for (long i = 0; i < n; ++i)
      l.append(gsl_rng_get(r));

    cout << endl
	 << "building array copy of previous list" << endl;
    DynArray<long> a;
    l.for_each(/* Lambda */ [&a](long i)
	       {
		 //cout << i << endl;
		 a.append(i);
	       });

    cout << endl
	 << "Copying array to a tree" << endl;
    DynSetTree<long> tree;
    a.for_each(/* Lambda */ [&tree] (long i)
	       {
		 tree.append(i);
	       });

    cout << endl
	 << "Sorting array for ulterior fast searching";
    quicksort_op(a);

    cout << endl
	 << "traversing all the keys of tree and verifiying them with array" 
	 << endl;
    assert(tree.all(/* Lambda */ [&a] (long i)
		    {
		      return a(binary_search(a, i)) == i;
		    }));
    cout << "done!" << endl
	 << endl
	 << "Traversing all the keys of array and verifiying then with the tree"
	 << endl;
    assert(a.all(/* Lambda */ [&tree] (long i)
		 {
		   return tree.search(i) != NULL;
		 }));
    cout << "done!" << endl
	 << endl;
  }

  cout << "Take 1/4:";
  range(10).take(3).for_each([] (long i) { cout << " " << i; }); 
  cout << endl
       << "Drop 1/4";
  range(10).drop(3).for_each([] (long i) { cout << " " << i; }); 
  cout << endl;

  gsl_rng_free(r);
  return 0;
}
