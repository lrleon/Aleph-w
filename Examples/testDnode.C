
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
# include <ctime>
# include <cstdlib>
# include <cassert>
# include <cstdio>
# include <random>

# include <iostream>
# include <string>
# include <tpl_dnode.H>

typedef Dnode<int> Test;

using namespace std;

void print_list(Test * link)
{
  Test* ptr;
  for (Test::Iterator it(link); it.has_curr(); it.next())
    {
      ptr = it.get_curr(); 
      cout << ptr->get_data() << " ";
    }
  cout << endl;
}


void create_random_list(Test * link, int n, unsigned int seed)
{
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> dist(1, 10 * n);
  Test* testPtr;
  for (int i = 0; i < n; i++)
    {
      testPtr = new Test (dist(rng));
      link->append(testPtr);
    }
}

int main(int argc, char *argv[])
{
  int n = 1000;
  unsigned int t = std::time(0);

  try 
    {
      if (argc > 1)
	n = std::stoi(argv[1]);

      if (argc > 2)
	t = std::stoi(argv[2]);
    }
  catch (...)
    {
      // ignore
    }

  if (n <= 0)
    {
      cout << "n must be positive" << endl;
      return 1;
    }

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  Test* testPtr;
  Test* headPtr;
  Test* tmpPtr;
  Test  head;

  headPtr = &head;

  unsigned long i;

  for (i = 0; i < (unsigned long) n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  cout << endl;

  for (i = 0, testPtr = headPtr->get_next(); i < (unsigned long) n; 
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

  for (i = 0; i < (unsigned long) n; i++)
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

  for (i = 0; i < (unsigned long) n; i++)
    {
      testPtr = new Test;

      testPtr->get_data() = i;
      headPtr->insert(testPtr);
      cout << i << " ";
    }

  for (Dnode<unsigned>::Iterator itor(headPtr); itor.has_curr(); )
    delete itor.del();

  assert(headPtr == headPtr->get_prev() and headPtr == headPtr->get_next());

  cout << endl << endl;

  Test list;

  create_random_list(&list, n, t);

  print_list(&list);

  print_list(&list);

  list.remove_all_and_delete();

  printf("Ending .... \n\n");
}
      

