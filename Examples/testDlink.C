
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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

# include <stdio.h>
# include <iostream>
# include <dlink.H>

using namespace std;


struct Test //: public Dlink
{
  unsigned n;
  Dlink link;

  Test(unsigned __n = 0) : n(__n) {}
};


DLINK_TO_TYPE(Test, link);

void print_list(Dlink* link)
{
  Test* ptr;
  for (Dlink::Iterator it(link); it.has_curr(); it.next())
    {
      ptr = dlink_to_Test(it.get_curr()); 
      cout << ptr->n << " ";
    }
  cout << endl;
}

void delete_list(Dlink* link)
{
  Test* ptr;
  while (not link->is_empty())
    {
      ptr = dlink_to_Test(link->remove_next()); 
      delete ptr;
    }
}

void create_random_list(Dlink* link, int n)
{
  Test* testPtr;
  for (int i = 0; i < n; i++)
    {
      testPtr = new Test;
      testPtr->n = 1 + (int) (10.0*n*rand()/(RAND_MAX+1.0));
      link->append(&testPtr->link);
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
  Dlink head;

  unsigned i;

  for (i = 0; i < n; i++)
    {
      testPtr = new Test;

      testPtr->n = i;
      head.append(&testPtr->link);
      cout << testPtr->n << " ";
    }

  cout << endl;  

  Dlink l1, l2;

  head.split_list(l1, l2);

  print_list(&l1);

  head.swap(&l2);

  head.reverse_list();
  print_list(&head);

  Dlink * ptr = head.get_next();
  for (i = 0; i < n/4; i++)
    ptr = ptr->get_next();

  ptr->insert_list(&l1);

  assert(l1.is_empty());

  print_list(&l1);

  print_list(&head);

  for (ptr = head.get_next(), i = 0; i < n/4; i++)
    ptr = ptr->get_next();

  delete_list(&l1);

  l1 = head.cut_list(ptr);

  print_list(&head);

  print_list(&l1);

  delete_list(&head);

  head = l1.cut_list(l1.get_prev());

  cout << "head: "; print_list(&head);

  cout << "l1: "; print_list(&l1);

  {
    Dlink list;
    for (int i = 1; i < 10; ++i)
      list.append(&(new Test(i))->link);

    Dlink split = list.cut_list(list.get_next());
    assert(list.is_empty());
    Dlink::Iterator it(split); int i = 0;
    for (; it.has_curr(); i++, it.next())
      assert(dlink_to_Test(it.get_curr())->n == i);
    assert(i == 10);
  }

  printf("Ending .... \n\n");
}
      


