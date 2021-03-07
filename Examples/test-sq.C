
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
# include <iostream>
# include <ahSort.H>
# include <tpl_arrayStack.H>
# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>
# include <tpl_arrayQueue.H>

template <template <typename __T> class Container, typename T> 
Container<T> copy(const Container<T> & s)
{
  Container<T> ret(s);
  return ret;
}

template <template <typename __T> class Container, typename T> 
void print(const Container<T> & s)
{
  cout << "(" << s.size() << ")";
  s.items().for_each([] (const T & d) { cout << " " << d; });
  cout << endl;
}

template <template <typename __T> class Stack, typename T> 
void check_lifo()
{
  Stack<T> s;
  for (int i = 0; i < 10; ++i)
    s.push(i);

  assert(s.top() == 9);

  const Stack<T> aux = s;
  assert(aux.top() == 9);
  assert(eq(s.items(), aux.items()));

  for (int i = 9; i >= 0; --i)
    {
      assert(s.top() == i);
      assert(s.pop() == i);
    }
  assert(s.size() == 0);
  assert(s.is_empty());
}

template <template <typename __T> class Stack, typename T> 
void test_stack()
{
  check_lifo<Stack, T>();

  Stack<T> s1;
  for (int i = 0; i < 10; ++i)
    s1.push(i);

  const Stack<T> s2 = copy(s1);

  Stack<T> s3;
  Stack<T> s4;

  s3 = s2;
  s4 = copy(s1);

  print(s1);
  print(s2);
  print(s3);
  print(s4);

  assert(eq(sort(s1.items()), sort(s2.items())));
  assert(eq(sort(s3.items()), sort(s4.items())));
}

template <template <typename __T> class Queue, typename T> 
void check_fifo()
{
  Queue<T> q;
  for (int i = 0; i < 10; ++i)
    q.put(i);

  assert(q.front() == 0);
  assert(q.rear() == 9);

  const Queue<T> aux = q;
  assert(aux.front() == 0);
  assert(aux.rear() == 9);
  assert(eq(q.items(), aux.items()));


  for (int i = 0; i < 10; ++i)
    {
      assert(q.front() == i);
      assert(q.get() == i);
    }
  assert(q.size() == 0);
  assert(q.is_empty());
}

template <template <typename __T> class Queue, typename T> 
void test_queue()
{
  check_fifo<Queue, T>();

  Queue<T> q1;
  for (int i = 0; i < 10; ++i)
    q1.put(i);

  Queue<T> q2 = q1;

  print(q1);
  print(q2);

  assert(eq(q1.items(), q2.items()));

  Queue<int> q3 = copy(q1);
  q3 = q2;
  Queue<int> q4 = q1;
  q4 = copy(q1);
  
  assert(eq(q3.items(), q3.items()));

  print(q3);
  print(q4);
}

template <template <typename T> class C>
void test_rank(size_t n, unsigned long seed)
{
  auto r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));

  C<unsigned long> l;

  cout << "Test rank for " << typeid(l).name() << endl
       << endl;

  for (auto i = 0; i < n; ++i)
    l.append(gsl_rng_get(r));

  auto ranks = Aleph::ranks(l);
  DynArray<unsigned long> s = sort(l);

  size_t i = 0;
  assert(l.all([&i, &s, &ranks] (auto item)
  	       {
  		 return s(ranks(i++)) == item;
  	       }));

  cout << "Original sequence" << endl;
  l.for_each([] (auto p) { cout << p << " "; });
  cout << endl
       << endl
       << "Ranks" << endl;

  // pair_ranks(l).for_each([] (auto p) 
  // 			 {
  // 			   cout << "(" << p.first << "," << p.second << ")";
  // 			 });
  // cout << endl
  //      << endl
  //      << "sorted" << endl;
  // s.for_each([] (auto p) { cout << p << " "; });
  // cout << endl
  //      << endl;

  // This line causes an assertion violation in llvm 3.6.1
  assert(pair_ranks(l).all([&s] (auto p) { return p.first == s(p.second); }));

  gsl_rng_free(r);

  cout << "end for " << typeid(l).name() << endl
       << endl;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    {
      cout << "usage" << endl
	   << "    " << argv[0] << " n seed" << endl;
      exit(0);
    }

  size_t n = atoi(argv[1]);
  unsigned long seed = atoi(argv[2]);

  cout << "DynListStack" << endl;
  test_stack<DynListStack, int>();

  cout << endl
       << "ArrayStack" << endl;
  test_stack<ArrayStack, int>();

  cout << endl
       << "FixedStack" << endl;
  test_stack<FixedStack, int>();

  cout << endl
       << "DynListQueue" << endl;
  test_queue<DynListQueue, int>();

  cout << endl
       << "ArrayQueue" << endl;
  test_queue<ArrayQueue, int>();

  cout << endl
       << "FixedQueue" << endl;
  test_queue<FixedQueue, int>();

  cout << endl;

  test_rank<DynArray>(n, seed);

  test_rank<DynList>(n, seed);

  test_rank<DynDlist>(n, seed);
}

