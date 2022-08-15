
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

# include <cassert>
# include <tpl_dynSetTree.H>

template <template <typename, class> class Tree = Rand_Tree> 
inline ostream & operator << (ostream & s, const DynSetTree<long, Tree> & t)
{
  t.for_each([&s] (long i) { s << i << " "; });
  cout << "(" << t.size() << ")";
  return s;
}

template <template <typename, class> class Tree = Rand_Tree> 
void test_split_pos(size_t n)
{
  DynSetTree<long, Tree> t;
  for (int i = 0; i < n; ++i)
    t.insert(i);

  DynSetTree<long, Tree> t1, t2;

  try
    {
      t.split_pos(n, t1, t2);
      cout << "ERROR" << endl;
    }
  catch (out_of_range)
    {
      cout << "Exception ok" << endl;
    }

  t.split_pos((n-1)/2, t1, t2);

  assert(t.is_empty());
  assert(t1.size() == (n-1)/2 or t1.size() == (n-1)/2 + 1);
  assert(t2.size() == (n-1)/2 or t2.size() - 1);
  assert(check_rank_tree(t1.get_root_node()));
  assert(check_rank_tree(t2.get_root_node()));

  t1.join_dup(t2);
  t.swap(t1);
  
  assert(t1.is_empty() and t2.is_empty());
  assert(t.size() == n);
  assert(check_rank_tree(t.get_root_node()));

  t.split_pos(0, t1, t2);
  assert(t1.min() == t1.max() and t1.min() == 0);
  t.join_dup(t1.join_dup(t2));

  t.split_pos(n - 1, t1, t2);
  cout << t1 << endl
       << t2 << endl;
}

template <template <typename, class> class Tree = Rand_Tree> 
void test_split_key(size_t n)
{
  DynSetTree<long, Tree> t;
  for (int i = 0; i < n; ++i)
    t.insert(i);

  DynSetTree<long, Tree> t1, t2;

  t.split_key_dup(n, t1, t2);
  assert(t1.size() == n and t2.is_empty());

  t.join_dup(t1);
  assert(t.size() == n and t1.is_empty());

  t.split_key_dup(n/2, t1, t2);
  
  cout << "split_key(" << n/2 << ")" << endl
       << t1 << endl
       << t2 << endl;

  assert(t.is_empty());
  assert(t1.size() == (n-1)/2 or t1.size() == (n-1)/2 + 1);
  assert(t2.size() == (n-1)/2 or t2.size() - 1);
  assert(check_rank_tree(t1.get_root_node()));
  assert(check_rank_tree(t2.get_root_node()));

  t1.join_dup(t2);
  t.swap(t1);
  
  assert(t1.is_empty() and t2.is_empty());
  assert(t.size() == n);
  assert(check_rank_tree(t.get_root_node()));


  t.split_key_dup(0, t1, t2);

  cout << "split_key(0)" << endl
       << t1 << endl
       << t2 << endl;

  assert(t1.is_empty() and t2.size() == n);
  t.join_dup(t1.join_dup(t2));

  t.split_key_dup(n - 1, t1, t2);
  cout << "split_key(" << n - 1 << ")" << endl
       << t1 << endl
       << t2 << endl;

  t.join_dup(t1.join_dup(t2));

  t.split_key_dup(n, t1, t2);
  cout << "split_key(" << n << ")" << endl
       << t1 << endl
       << t2 << endl;
}


int main(int, char *argv[])
{
  size_t n = atoi(argv[1]);

  // test_split_pos<Rand_Tree>(n);

  // test_split_pos<Treap_Rk>(n); 

  test_split_key<Rand_Tree>(n);

  return 0;
}
