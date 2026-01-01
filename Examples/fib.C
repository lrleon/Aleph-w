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

# include <cstdlib>
# include <iostream>
# include <tpl_arrayStack.H>

using namespace std;
using namespace Aleph;

int fib_it(const int n)
{
  if (n <= 1)
    return 1;

  int fi_1 = 1, fi_2 = 1, fi = 0;

  for (int i = 2; i <= n; i++, fi_2 = fi_1, fi_1 = fi)
    fi = fi_1 + fi_2;

  return fi;
}

int fib_rec(const int n)
{
  if (n <= 1)
    return 1;

  const int f1 = fib_rec(n - 1);
  const int f2 = fib_rec(n - 2);
  return f1 + f2;
}


# define P0 0
# define P1 1
# define P2 2

struct Item
{
  int n;
  int f1;
  int result;
  char return_point;
};

# define N  (stack.top().n)
# define F1 (stack.top().f1)
# define RESULT (stack.top().result)
# define RETURN_POINT (stack.top().return_point)

int fib_st(const int n)
{
  ArrayStack<Item> stack;
  Item first_activation_register;
  Item current_activation_register;

  first_activation_register.n = n;

  stack.push(first_activation_register);

  while (true)
    {
      if (N <= 1)
        {
          current_activation_register = stack.pop();
          current_activation_register.result = 1;
          stack.push(current_activation_register);
          goto exit_from_fib;
        } { /* Esta es la llamada a fib(n - 1) */
        Item new_activation_register;
        new_activation_register.n = N - 1;
        new_activation_register.return_point = P1;
        stack.push(new_activation_register);
        continue;
      }

    p1: /* Esta es la salida de fib(n - 1) */
      current_activation_register = stack.pop();
      current_activation_register.f1 = current_activation_register.result; /* int f1 = fib(n - 1) */
      stack.push(current_activation_register); { /* Esta es la llamada a fib(n - 2) */
        Item new_activation_register;
        new_activation_register.n = N - 2;
        new_activation_register.return_point = P2;
        stack.push(new_activation_register);
        continue;
      }

    p2: /* Esta es la salida de fib(n - 2) */
      current_activation_register = stack.pop();
      current_activation_register.result =
          current_activation_register.f1 + current_activation_register.result;
      stack.push(current_activation_register);

    exit_from_fib:
      if (stack.size() == 1)
        return RESULT;

      current_activation_register = stack.pop();
      char return_point = current_activation_register.return_point;

      // Propagate result to caller
      int res = current_activation_register.result;
      Item caller = stack.pop();
      caller.result = res;
      stack.push(caller);

      switch (return_point)
        {
        case P1: goto p1;
        case P2: goto p2;
        }
    }
}


int main(int argn, char *argv[])
{
  if (argn != 2)
    {
      cout << "usage: " << argv[0] << " n" << endl;
      return 1;
    }
  int n = atoi(argv[1]);

  cout << "fib(" << n << ") = " << fib_rec(n) << " = " << fib_it(n)
      << " = " << fib_st(n) << endl;
}
