
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

# include <stdio.h>
# include <tpl_sort_utils.H>


# define NumItems 200

typedef Dnode<unsigned> List;

void printList(List & list)
{
  printf("\n");

  List::Iterator itor(list);
  Dnode<unsigned>* node;

  for (itor.reset_first(); itor.has_curr(); itor.next())
    {
      node = itor.get_curr();
      printf("%u ", node->get_data());
    }

  printf("\n");
}      


struct Menor_Que
{
  bool operator() (Dlink * l1, Dlink * l2) const
  {
    return static_cast<Dnode<unsigned>*>(l1)->get_data() <
      static_cast<Dnode<unsigned>*>(l2)->get_data();
  }
};

int main()
{
  printf("Starting ........\n\n");

  List list1;
  List list2;

  Dnode<unsigned>* node;

  unsigned long i;

  for (i = 0; i < NumItems; i++)
    {
      node = new Dnode<unsigned>;
      node->get_data() = i;
      list1.insert(node); 
      printf("%lu ", i);
    }

  printf("\n");

  for (i = 0; i < NumItems; i++)
    {
      node = new Dnode<unsigned>;
      node->get_data() = i;
      list2.append(node); 
      printf("%lu ", i);
    }

  List list3;

  merge_lists<List, Menor_Que>(list1, list2, list3);

  printList(list1); 
  printList(list2); 

  quicksort(list1);

  printf("Lists apparently merged and sorted\n");

  printList(list1); 

  list1.remove_all_and_delete();

  printf("Ending .... \n\n");
}
      

