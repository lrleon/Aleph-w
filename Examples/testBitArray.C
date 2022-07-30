
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

# include <iostream>
# include <bitArray.H>

using namespace std;
using namespace Aleph;

void print(BitArray & array)
{
  cout << "size = " << array.size() << endl;

  for (int i = 0; i < array.size(); i++)
    cout << array[i] << " ";
  cout << endl;
}

void foo(int& i) { i = 5; }

int main()
{
  BitArray array(20);

  for (int i = 0; i < 20; i++)
    {
      array[i] = 0;
    }

  array[20] = 1;
  array[27] = 1;
  array[22] = 1;

  for (int i = 30; i < 40; i++)
    {
      array[i] = array[i];
    }

  print(array);

  cout << endl;  

  ofstream out("test.bits");
  array.save(out);

  ifstream in("test-aux.bits");
  array.load(in);

  print(array);
}
