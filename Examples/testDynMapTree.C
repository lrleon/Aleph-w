

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
# include <btreepic_avl.H>
# include <tpl_dynMapTree.H>

# include <stdlib.h>
# include <autosprintf.h>

using namespace std;
using namespace Aleph;


struct Print_Key
{
  string operator () (const int & i)
  {
    gnu::autosprintf ret("%d", i);
    return ret;
  }
};


template <template <class, class> class Tree>
void test_tree(int n)
{
  {
    DynMapTree<int, int, Tree> s;

    for (int i = 0; i < n; ++i)
      s.insert(i, i);

    for (int i = 0; i < n; ++i)
      {
        assert(i == s[i]);
        s.remove(i);
      }

    assert(s.is_empty() and s.size() == 0);
  }

  {
    DynMapTree<int, int, Tree> s;

    for (int i = 0; i < n; ++i)
      {
        int value = rand();
        s.insert(i, value);
      }

    for (auto p : s)
      assert(p.first == p.second);

    cout << s.size() << " nodes " << endl;
  }

  {
    DynMapTree<int, int, Tree> s;

    for (int i = 0; i < n; ++i)
      {
        s[i]++;
        s[i] += 2;
      }

    assert(s.all([] (auto p) { return p.second == 3; }));

    cout << s.size() << " nodes " << endl;
  }
}

int main(int argn, char *argv[])
{
  int n = argn > 1 ? atoi(argv[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argv[2]);

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  test_tree <Splay_Tree> (n);

  test_tree <Avl_Tree> (n);

  test_tree <Rb_Tree> (n);

  //test_tree <BinTree> (n);

  test_tree <Rand_Tree> (n);
  
  test_tree <Treap> (n);

  test_tree <Treap_Rk> (n);

}
