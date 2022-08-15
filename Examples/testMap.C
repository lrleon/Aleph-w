
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

# include <iostream>
# include <Map.H>

using namespace Aleph;

void print_container(map<int,int> & m)
{
  map<int,int>::iterator it = m.begin();
  while (it not_eq m.end())
    {
      cout << "(" << it->first << "," << it->second << ") ";
      it++;
    }
  cout << endl;
}

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  map<int,int> map1;

  for (int i = 0; i < n; i++)
    {
      map1.insert(make_pair(i, i*10));
      map1.insert(make_pair(i, i*100));
    }

  cout << endl << "size map1: " << map1.size() << endl;

  print_container(map1);

  cout << endl << "Hay " << map1.count(n/2) << " valores de " << n/2
       << " en map1" << endl;

  cout << endl << "Prueba find(" << n/2 << ")..." << endl;

  std::pair<int, int> _pair;

  _pair = *map1.find(n/2);

  cout << _pair.first << endl;

  _pair = *map1.lower_bound(n/2);

  cout << endl << "lower_bound de " << n/2 << " es: " << _pair.first  << endl;

  _pair = *map1.upper_bound(n/2);

  cout << endl << "upper_bound de " << n/2 << " es: " << _pair.first  << endl;

  cout << endl << "Borrando valor " << n/2 << " ..." << endl;

  map1.erase(n/2);

  print_container(map1);

  cout << endl << "Borrando lower_bound " << n/2 << " ..." << endl;

  _pair = *map1.lower_bound(n/2);

  cout << "lower_bound de " << n/2 << " es: " << _pair.first  << endl;

  map1.erase(map1.lower_bound(n/2));

  print_container(map1);

  cout << "prueba de insercion mediante operador []" << endl;

  for (int i = 0; i < n; i++)
    map1[i] = i;

  print_container(map1);

  cout << "prueba de copia mediante operador []" << endl;

  map<int, int> map2;

  for (int i = 0; i < n; i++)
    map2[i] = map1[i];

  print_container(map2);

  cout << "prueba de lectura  mediante operador []" << endl;
  for (int i = 0; i < n; i++)
    cout << "map2[" << i << "] = " << map2[i] << " ";
  cout << endl;

  cout << "prueba de lectura fallida mediante operador []" << endl;
  try
    {
      cout << "Esta es una lectura fallida map1[" << n << "] = " 
           << map1[n] << endl;
    }
  catch (exception & e)
    {
      cout << e.what() << endl;
    }

  cout << "prueba de constructor copia " << endl;
  map<int, int> map3 = map2;
  print_container(map3);

  cout << "prueba de operador asignacion " << endl;
  map3 = map1;
  print_container(map3);

  cout << "map1 == map2: " << (map1 == map2 ? "true" : "false") << endl;

  map1[n/2] = n;

  cout << "map1 == map2: " << (map1 == map2 ? "true" : "false") << endl;

  map1[n/2] = n/2;

  cout << "map1 == map2: " << (map1 == map2 ? "true" : "false") << endl;

  map1[n] = n;

  cout << "map1 == map2: " << (map1 == map2 ? "true" : "false") << endl;

  map2[n] = n;

  cout << "map1 == map2: " << (map1 == map2 ? "true" : "false") << endl;

  map1[n] = n -1;

  cout << "map1 < map2: " << (map1 < map2 ? "true" : "false") << endl;

  map1[n] = n + 1;

  cout << "map1 < map2: " << (map1 < map2 ? "true" : "false") << endl;

  map1[n] = map1[n-1];

  print_container(map1);
  cout << "Prueba de erase(map1.begin(), map1.end())" << endl;

  map1.erase(map1.begin(), map1.end());

  print_container(map1);

}

