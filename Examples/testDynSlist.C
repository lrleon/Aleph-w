
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

# include <iostream>
# include <aleph.H>
# include <tpl_dynSlist.H>

# define Num_Items 10

using namespace std;

int main()
{
  DynSlist<int> list;
  DynSlist<int*> list_ptr;

  for (int i = 0; i < Num_Items; i++)
    {
      list.insert(i, i);
      list_ptr.insert(i, &i);
    }

  DynSlist<int>::Iterator itor(list);

  DynSlist<int>::Iterator itor_aux(list);

  itor_aux = itor;

  for (/* nothing */; itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

  for (int i = Num_Items; i < 2*Num_Items; i++)
    list.insert(i - Num_Items, i);

  for (itor.reset_first(); itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

  for (int i = 2*Num_Items; i < 3*Num_Items; i++)
    list.insert(0, i);

  for (itor.reset_first(); itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

  for (int i = 3*Num_Items; i < 4*Num_Items; i++)
    list.insert(list.size(), i);

  for (int i = 0; i < list.size(); i++)
    cout << list[i] << " ";

  cout << endl;

  for (int i = 0; i < list.size(); i++)
    cout << list[list.size() - i - 1] << " ";

  cout << endl;

  list.remove(list.size() - 1);

  list.remove(0);

  for (itor.reset_first(); itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

}
