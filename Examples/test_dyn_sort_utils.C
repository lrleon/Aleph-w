
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

# include <stdlib.h>
# include <iostream>
# include <tpl_sort_utils.H>


void verify(DynArray<int>& a)
{
  int i;
  
  for (i = 0; i < a.size() - 1; i++)
    if (a.access(i) > a.access(i + 1))
      EXIT("Error i = %d : not %d < %d\n", i, a.access(i), a.access(i + 1));
}

void fill_array(DynArray<int>& a, unsigned t)
{
  int i;

  srand(t);
  
  for (i = 0; i < a.size(); i++)
    a.access(i) = rand();
}

int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int t = time(0);

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  DynArray<int> a;
  a.reserve(0, n - 1);

  cout << "quicksort optimo ... " << endl;
  cout << "filling" << endl;
  fill_array(a, t);
  cout << "sorting ..." << endl;
  quicksort_op(a);
  cout << "verifying ..." << endl;
  verify(a);
  cout << " done " << endl << endl;


# ifdef nada
  cout << "selection ... " << endl;
  cout << "filling" << endl;
  fill_array(a, t);
  cout << "sorting ..." << endl;
  selection_sort(a);
  cout << "verifying ..." << endl;
  verify(a);
  cout << " done " << endl << endl;


  cout << "bubble ... " << endl;
  cout << "filling" << endl;
  fill_array(a, t);
  cout << "sorting ..." << endl;
  bubble_sort(a);
  cout << "verifying ..." << endl;
  verify(a);
  cout << " done " << endl << endl;




  cout << "insertion ... " << endl;
  cout << "filling" << endl;
  fill_array(a, t);
  cout << "sorting ..." << endl;
  insertion_sort(a);
  cout << "verifying ..." << endl;
  verify(a);
  cout << " done " << endl << endl;


  cout << "shell sort ... " << endl;
  cout << "filling" << endl;
  fill_array(a, t);
  cout << "sorting ..." << endl;
  shellsort(a);
  cout << "verifying ..." << endl;
  verify(a);
  cout << " done " << endl << endl;

  cout << "quicksort ... " << endl;
  cout << "filling" << endl;
  fill_array(a, t);
  cout << "sorting ..." << endl;
  quicksort(a);
  cout << "verifying ..." << endl;
  verify(a);
  cout << " done " << endl << endl;


  cout << "heapsort ... " << endl;
  cout << "filling" << endl;
  fill_array(a, t);
  cout << "sorting ..." << endl;
  heapsort(a);
  cout << "verifying ..." << endl;
  verify(a);
  cout << " done " << endl << endl;

# endif


  printf("Ending .... \n\n");
}
      

