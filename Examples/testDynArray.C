
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
# include <stdlib.h>
# include <time.h>
# include <math.h>
# include <iostream>

# include <tpl_dynarray_set.H>


using namespace std;

# define RAND( x  ) (unsigned long) ( x * ( rand() / (RAND_MAX+1.0) ) ) 


using namespace Aleph;
using namespace std;

struct Test
{
  char a1;
  char a2;
};


int main(int argc,char *argv[])
{
  unsigned long seed,index;
  unsigned long i, value, val;

  const unsigned long NUM_ITE = argc > 1 ? atoi(argv[1]) : 1000;
  
  if (argc > 2)
    seed = atol(argv[1]);
  else
    seed = time(NULL);

  srand (seed);

  DynArray<Test> t;

  t[10]->a1 = 'a';
  t[10]->a2 = 'b';

  cout << t[10]->a1 << t[10000]->a1 << endl;

  cout << "./testDynArray " << NUM_ITE << " " << seed << endl;

  try
  {
    {
      DynArray<unsigned long> v1(8,8,8), v2(NUM_ITE);

      for(i = 0; i < NUM_ITE; i++)
	{
	  value =  RAND(NUM_ITE);
	  index =  RAND(NUM_ITE);
	  v1[index] = value;
	  v2[index] = v1[index];
	  val = v1[index];
	  value = v2[index];
	  cout << "(" << val << ") (" << value << ") ";
	  assert(v2[index] == v1[index]);
	}

      {
	DynArray<unsigned long> v3(v2);

	DynArray<unsigned long> v4(v3);

	v4 = v1;

	v4.swap(v2);
      }

      v2.reserve(0, 2*NUM_ITE);

    }
    DynArray<unsigned long> v(10, 10, 6);

    v.reserve(0, 2*NUM_ITE);

    v.reserve(NUM_ITE, 4*NUM_ITE);
  }
  catch(std::overflow_error)
    {
      cout << "Overflow!" << endl;
    }
  catch(std::bad_alloc)
    {
      cout << "Not enough memory!" << endl;
    }
  catch(std::invalid_argument)
    {
      cout << "invalid_argument!" << endl;  
    }
  catch (std::exception & e)
    {
      cout << e.what() << endl;
    }
  cout << endl;

  {
    DynArray<int> s(12,10,4);

    s.append(10);

    cout << s.access(0) << " ****************" << endl;

    cout << s.access(0) << " ****************" << endl;

    s.append(16);

    cout << s[1] << " ****************" << endl;

  }

  {
    DynArray_Set<int> s(12,10,4);

    s.append(10);

    cout << s[0] << " ****************" << endl;

    cout << s.access(0) << " ****************" << endl;

    s.append(16);

    cout << s[1] << " ****************" << endl;

  }
 
  DynArray<int> a;
  for (int i = 0; i < NUM_ITE; ++i)
    a.touch(i) = i;

  for (DynArray<int>::Iterator it(a); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;

  DynArray<int>::Iterator it(a);
  for (it.reset_last(); it.has_curr(); it.prev())
    cout << it.get_curr() << " ";
  cout << endl;
}












