
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
# include <Set.H>
//# include <set>
# include <print_container.H>

 
struct Foo
{
  struct Tata {};
};


void test()
{
  set<Foo::Tata>::iterator it;
}

using namespace std;

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  set<int> set1;

  for (int i = 1; i <= n; i++)
    set1.insert(i);

  cout << "size de set1: " << set1.size() << endl;

  cout << endl << "set1: ";

  print_container(set1);

  cout << endl << "Prueba count(value)..." << endl;

  cout << "Hay " << set1.count(11) << " valores 1" << endl;

  cout << endl << "Prueba find(value)..." << endl;

  set<int>::iterator itor1(set1.find(n));

  cout << *itor1 << endl;

  set<int> set2;

  for (int k = 1; k <= n; k++)
    {
      set2.insert(k);
      set2.insert(k);
    }

  cout << "size de set2: " << set2.size() << endl;

  cout << endl << "set2: ";

  print_container(set2);

  cout << endl << "Prueba lower_bound en set2(5)" << endl;

  cout << *set2.lower_bound(5) << endl;

  cout << endl << "Prueba upper_bound en set2(5)" << endl;

  cout << *set2.upper_bound(5) << endl;

  cout << endl << "Insertando valor 11 en set1..." << endl;

  cout << *set1.insert(set1.upper_bound(5), 11) << endl;

  {
    set<int> set3;

    cout << *set3.insert(set3.begin(), 11) << endl;
  }

  cout << "size de set1: " << set1.size() << endl;

  cout << endl << "Swaping set1 y set2..." << endl;

  //set1.swap(set2);

  cout << endl << "set1: ";

  print_container(set1);

  cout << endl << "set2: ";

   print_container(set2);

  cout << endl << "Prueba erase(value)..." << endl;
  cout << set1.erase(5) << endl;

  cout << "size de set1: " << set1.size() << endl; 

  cout << endl << "Prueba erase(pos)..." << endl;
  set1.erase(set1.begin());

  set1.erase(set1.begin(), set1.end());

  cout << "size de set1: " << set1.size() << endl;

  print_container(set1);
}
