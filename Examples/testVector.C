

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
# include <Vector.H> 
# include <print_container.H> 

using namespace Aleph;

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  int valor = 6;

  Aleph::vector<int> vector1(n - 1);
  Aleph::vector<int> vector2(static_cast<Aleph::vector<int>::size_type>(n), valor);

  cout << "size vector1: " << vector1.size() << endl;
  cout << "size vector2: " << vector2.size() << endl;
  cout << "capacity vector2: " << vector2.capacity() << endl;

  if (vector1.empty())
    cout << "vector1: vacio" << endl;
  else
    cout << "vector1: lleno" << endl;
  
  cout << "size maximo vector2: " << vector2.max_size() << endl;

  Aleph::vector<int> vector3;
  vector3 = vector2;
  for (int i = 0; i < vector3.capacity(); i++)
    cout << vector3[i] << " ";
  cout << endl;

  cout << "size de vector3 igualado a vector 2: " << vector3.size() << endl;
  cout << "capacity de vector3 igualado a vector 2: " 
       << vector3.capacity() << endl;

  unsigned int b = 3;
  vector2.assign(b, 9);
  cout << "size de vector2 despues de assign: " << vector2.size() << endl;
  cout << "capacity de vector2 despues de assign: " << vector2.capacity() 
       << endl;

  vector1.swap(vector3);
  cout << "capacidad vector1 despues de swap con vector 3: " 
       << vector1.capacity() << endl;
  cout << "capacidad vector3 despues de swap con vector 1: " 
       << vector3.capacity() << endl;
 
  cout << "tercer valor de vector2 es: " << vector2.at(2) << endl; 
  
  vector2[4] = 100;
  
  cout << "quinto valor de vector2 (100): " << vector2[4] << endl; 

  cout << "primer valor de vector2: " << vector2.front() << endl; 
  
  cout << "ultimo valor de vector2: " << vector2.back() << endl; 

  cout << "vector2: ";
  print_container(vector2);

  Aleph::vector<int> vector4(vector2.begin(),vector2.end());

  cout << "vector4: ";
  print_container(vector4);

  //return 0;
  
  cout << "capacidad vector4: " << vector4.capacity() << endl;
 
  cout << "size vector4: " << vector4.size() << endl;
    
  vector3[2] = 5;

  cout << "Tercer elemento vector3 es (5): " << vector3[2] << endl;

  vector4.assign(vector3.begin(), vector3.end());

  cout << "capacidad vector4 despues de assign (4): " 
       << vector4.capacity() << endl;
 
  cout << "size vector4 despues de assign (4): " << vector4.size() << endl;


  cout << "capacidad vector1: " << vector1.capacity() << endl;

  cout << "size vector1: " << vector1.size() << endl;

  vector1.push_back(113);

  cout << "ultimo valor de vector1: " << vector1.back() << endl; 

  vector2.clear();
  cout << "capacidad de vector2 despues de clear(): " 
       << vector2.capacity() << endl;

  cout << "size de vector2 despues de clear(): " << vector2.size() << endl;

  vector2.insert(vector2.begin(), 20);

  cout << "size vector2 despues de insert: " << vector2.size() << endl;

  cout << "primer valor vector2: " << vector2.front() << endl;

  cout << "ultimo valor de vector2: " << vector2.back() << endl;

  vector2.insert(vector2.begin(), static_cast<Aleph::vector<int>::size_type>(n), -14);

  cout << "size vector2 despues de insert: " << vector2.size() << endl;

  cout << "primer valor vector2: " << vector2.front() << endl;

  cout << "ultimo valor de vector2: " << vector2.back() << endl;

  vector2.insert(vector2.begin(), vector1.begin(), vector1.end());

  cout << "size vector2 despues de insert: " << vector2.size() << endl;

  cout << "primer valor vector2: " << vector2.front() << endl;

  cout << "ultimo valor de vector2: " << vector2.back() << endl;


}
