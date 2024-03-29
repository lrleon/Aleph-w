
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

# include <gsl/gsl_rng.h> 
# include <iostream>
# include <tpl_union.H>

void usage(int argn, char * argc[])
{
  if (argn < 3)
    {
      cout << "usage is " << argc[0] << " n num-pairs [seed]" << endl;
      exit(0);
    }
}

int main(int argn, char * argc[])
{
  if (argn < 3 or argn > 4)
    usage(argn, argc);
  int n = atoi(argc[1]);

  int num_pairs = atoi(argc[2]);

  unsigned int seed = 0;
  if (argn > 3)
    seed = atoi(argc[3]);

  gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));

# ifdef NADA
  {
    Relation rel;

    cout << "Insertando " << num_pairs << " pares " << "(" 
	 << rel.get_num_blocks() << ")" << endl;
    int ins_count = 0;
    for (int i = 0; i < num_pairs; ++i)
      {
	int i = gsl_rng_uniform_int(r, n - 1);
	int j = gsl_rng_uniform_int(r, n - 1);

	if (i == j)
	  continue;

	if (rel.are_connected(i, j))
	  continue;

	cout << i << "-" << j << ", ";
	rel.join(i, j);
	++ins_count;
      }
    cout << ins_count << " pairs inserted" << endl
	 << endl;

    for (int i = 0; i < n; ++i)
      {
	for (int j = 0; j < n; ++j)
	  {
	    if (i == j)
	      continue;

	    //cout << "(" << i << "," << j << ")";
	    if (rel.are_connected(i, j))
	      cout << i << "-" << j << ", ";
	  }
	cout << i << endl;
      }
  
    cout << endl
	 << rel.size() << " items " << rel.get_num_blocks() 
	 << " blocks" << endl;
  }
# endif
  {
    Relation_T<int> rel;

    cout << "Insertando " << num_pairs << " pares " << "(" 
	 << rel.get_num_blocks() << ")" << endl;
    int ins_count = 0;
    for (int k = 0; k < num_pairs; ++k)
      {
	int i = gsl_rng_uniform_int(r, n - 1);
	int j = gsl_rng_uniform_int(r, n - 1);

	if (i == j)
	  continue;

	if (rel.are_connected(i, j))
	  continue;

	cout << i << "-" << j << ", ";
	rel.join(i, j);
	++ins_count;
      }
    cout << ins_count << " pairs inserted" << endl
	 << endl;

    for (int i = 0; i < n; ++i)
      {
	for (int j = 0; j < n; ++j)
	  {
	    if (i == j)
	      continue;

	    //cout << "(" << i << "," << j << ")";
	    if (rel.are_connected(i, j))
	      cout << i << "-" << j << ", ";
	  }
	cout << i << endl;
      }
  
    cout << endl
	 << rel.size() << " items " << rel.get_num_blocks() 
	 << " blocks" << endl;
  }
}
