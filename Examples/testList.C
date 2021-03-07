
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
# include <List.H>
//# include <list.h>
# include <print_container.H>

using namespace std;

int main()
{
  unsigned int n_elem = 7;

  int valor = 4;

  list<int> list1;

  list<int> list2(5);

  list<int> list3(n_elem, valor);

  list<int> list4(list3.begin(), list3.end());
  
  cout << "size list1: " << list1.size() << endl;

  cout << "list1 esta: "; 
  if (list1.empty()) cout << "vacia" << endl;
  else cout << "llena" << endl;
 
  cout << "size list2: " << list2.size() << endl;

  cout << "list2 esta: "; 
  if (list2.empty()) 
    cout << "vacia" << endl;
  else 
    cout << "llena" << endl;

  cout << "size list3: " << list3.size() << endl;
  
  cout << "size list4: " << list4.size() << endl;
  
  cout << "Swaping list1 y list2..." << endl;

  list1.swap(list2);

  cout << "size list1: " << list1.size() << endl;
  cout << "size list2: " << list2.size() << endl;

  
  cout << "ultimo valor lista3: " << list3.back() << endl;

  cout << "prueba push_front()..." << endl;
  list3.push_front(100);
  cout << "primer valor lista3: " << list3.front() << endl;
  cout << "ultimo valor lista3: " << list3.back() << endl;
  
  cout << "prueba push_back()..." << endl;
  list3.push_back(5000);
  cout << "ultimo valor lista3: " << list3.back() << endl;
  cout << "size list3: " << list3.size() << endl;
  
  cout << "Prueba remove()..." << endl;
  list4.remove(4);
  cout << "size list4: " << list4.size() << endl;

  cout << "Prueba pop_front()..." << endl;
  list3.pop_front();
  cout << "primer valor lista3: " << list3.front() << endl;
  cout << "size list3: " << list3.size() << endl;

  cout << "Prueba pop_back()..." << endl;
  list3.pop_back();
  cout << "ultimo valor lista3: " << list3.back() << endl;
  cout << "size list3: " << list3.size() << endl;

  cout << "Prueba clear()..." << endl;
  list3.clear();
  cout << "size list3: " << list3.size() << endl;

  cout << "Llenando list3..." << endl;
  for (int i = 1; i <= 5; i++)
    {  
      for (int n = 1; n <= 10; n++)
	list3.push_back(n);
    }
  
  //list3.reverse();
  cout << "primer valor lista3: " << list3.front() << endl;
  cout << "ultimo valor lista3: " << list3.back() << endl;
  cout << "size list3: " << list3.size() << endl;

  
  cout << "Prueba unique..." << endl;
  list<int> l4((size_t) 10, (int) -35);
  list<int>::iterator it4 = l4.end(); 
  --it4;
  it4 = list3.end(); --it4;
  list3.insert(it4, l4.begin(), l4.end());
  list3.unique();
  Aleph::print_container(list3);
  cout << "size list3: " << list3.size() << endl;

  cout << "Prueba sort() " << list3.size() << endl;
  list3.sort();
  Aleph::print_container(list3);

  list<int> list5;
  cout << endl <<"Llenando list5..." << endl;
  for (int n = 1; n <= 5; n++)
    list5.push_back(n);

  cout << "primer valor lista5: " << list5.front() << endl;
  cout << "ultimo valor lista5: " << list5.back() << endl;
  cout << endl <<"Prueba reverse()..." << endl;

  list5.merge(list3);

  list5.reverse();

  cout << "primer valor lista5: " << list5.front() << endl;
  cout << "ultimo valor lista5: " << list5.back() << endl;
  Aleph::print_container(list5);
  cout << "size list5: " << list5.size() << endl;

  cout << "Prueba erase(iteratos pos)..." << endl;
  cout << *list5.erase(list5.begin()) << endl;
  cout << "size list5: " << list5.size() << endl;

  Aleph::print_container(list5);

  cout << "Prueba resize(num)..." << endl;
  list5.resize(4);
        
  Aleph::print_container(list5);

  list5.resize(2);
  
  Aleph::print_container(list5);
  
  list5.resize(10);
  
  Aleph::print_container(list5);
  
  cout << "Prueba resize(num, value)..." << endl;

  list5.resize(4,8);
   
  Aleph::print_container(list5);

  list5.resize(4,8);
  
  Aleph::print_container(list5);
  
  list5.resize(10,8);

  Aleph::print_container(list5);

}
