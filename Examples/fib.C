

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
 
      current_activation_register.result = 
	current_activation_register.f1 + current_activation_register.result;

    exit_from_fib:
      if (stack.is_empty())
	return current_activation_register.result;

      char return_point = current_activation_register.point;
      stack.pop();
      current_activation_register = stack.lookAt();
      switch (return_point)
	{
	case P1: goto p1;
	case P2: goto p2;
	}
    }
}



int main(int argn, char * argv[])
{
  int n = atoi(argv[1]);

  cout << "fib(" << n << ") = " << fib_rec(n) << " = " << fib_it(n) 
       << " = " << fib_st(n) << endl;
}
