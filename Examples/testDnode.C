
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

# include <iostream>
# include <tpl_dnode.H>

typedef Dnode<int> Test;

using namespace std;

void print_list(Test * link)
{
  Test* ptr;
  for (Test::Iterator it(link); it.has_current(); it.next())
    {
      ptr = it.get_current(); 
      cout << ptr->get_data() << " ";
    }
  cout << endl;
}


void create_random_list(Test * link, int n)
{
  Test* testPtr;
  for (int i = 0; i < n; i++)
    {
      testPtr = new Test (1 + (int) (10.0*n*rand()/(RAND_MAX+1.0)));
      link->append(testPtr);
    }
}

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Test* testPtr;
  Test* headPtr;
  Test* tmpPtr;
  Test  head;

  headPtr = &head;

  unsigned long i;

  for (i = 0; i < n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  cout << endl;

  for (i = 0, testPtr = headPtr->get_next(); i < n; 
       i++, testPtr = testPtr->get_next())
    cout << testPtr->get_data() << " ";

  cout << endl;

  for (testPtr = headPtr->get_prev(); testPtr != headPtr; 
       testPtr = testPtr->get_prev())
    cout << testPtr->get_data() << " ";

  cout << endl;

  while (not headPtr->is_empty())
    {
      testPtr = headPtr->get_next();
      cout << testPtr->get_data() << " ";
      headPtr->remove_next();
      delete testPtr;
    }

  cout << endl;

  for (i = 0; i < n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  cout << endl;

  for (testPtr = headPtr->get_next(); testPtr not_eq headPtr;)
    {
      tmpPtr = testPtr;
      testPtr = testPtr->get_next();
      cout << testPtr->get_data() << " ";
      tmpPtr->del();
      delete tmpPtr;
    }

  cout << endl;

  for (i = 0; i < n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  for (Dnode<unsigned>::Iterator itor(headPtr); itor.has_current(); )
    delete itor.del();

  assert(headPtr == headPtr->get_prev() && headPtr == headPtr->get_next());

  cout << endl << endl;

  Test list;

  create_random_list(&list, n);

  print_list(&list);

  print_list(&list);

  list.remove_all_and_delete();

  printf("Ending .... \n\n");
}
      

