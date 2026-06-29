
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

# include <cstdio>
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

  printList(list3); 
  printList(list2); 

  quicksort(list3);

  printf("Lists apparently merged and sorted\n");

  printList(list3); 

  list3.remove_all_and_delete();

  printf("Ending .... \n\n");
}
      

