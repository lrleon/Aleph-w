

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
# include <tpl_dynMat.H>

struct Entry
{
  int i, j;
};

int main()
{
# ifdef nada
  {
  DynMatrix<int> mat(100000, 100000);

  for (int i = 1230; i < 3000; i++)
    for (int j = 3000; j < 4000; j++)
      mat(i, j) = 1000; 


  for (int i = 1230; i < 3000; i++)
    for (int j = 3000; j < 4000; j++)
      cout << mat(i, j) << " ";
  cout << endl;

  mat(10, 10) = 100;

  cout << mat(10,10) << endl;

  DynMatrix<int> mat1 = mat;

  for (int i = 1229; i < 3001; i++)
    for (int j = 2999; j < 4001; j++)
      cout << mat(i, j) << " ";
  }
# endif

  {
    DynMatrix<Entry> mat(10000, 10000);
    for (int i = 0; i < 10000; i++)
      for (int j = 0; j < 10000; j++)
	{
	  Entry entry; entry.i = i; entry.j = j;
	  mat.write(i, j, entry);
	}

    cout << "verficando" << endl;

    for (int i = 0; i < 10000; i++)
      for (int j = 0; j < 10000; j++)
	assert(mat.read(i, j).i == i and mat.read(i, j).j == j);
  }
  
}
