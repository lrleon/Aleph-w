
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
# include <time.h>
# include <gsl/gsl_rng.h>
# include <cassert>
# include <al-vector.H>

gsl_rng * r;

const size_t N = 1000;

Vector<int> create_domain(size_t n)
{
  Vector<int>::Domain domain;
  for (int i = 0; i < n; ++i)
    domain.insert(i);

  return std::move(domain);
}

Vector<std::string> create_string_domain(size_t n)
{
  int m = 'Z' - 'A';
  int num = n/m;
  int rem = n%m;

  std::string str = "";
  Vector<string>::Domain domain;
  
  int k;
  for (int i = 0; i < num; i += m)
    {
      for (k = 0; k < m; ++k)
	domain.insert(str + string('A' + k, 1));

      str += string('A' + k, 1);
    }

  for (int i = 0; i < rem; ++i)
    domain.insert(str + string('A' + k + i, 1));

  return std::move(domain);
}

int main(int argn, char * argc[])
{
  size_t n = argn > 1 ? atoi(argc[1]) : N;
  unsigned long seed = argn > 2 ? atoi(argc[2]) : time(NULL);

  cout << argc[0] << " " << n << " " << seed << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));

  Vector<int>::Domain d ({0,1,2,3,4,5,6,7,8,9});
    
  Vector<int> v1(d, {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9});
  Vector<int> v2(d, {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9});
  Vector<int> v3(d, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
 
  cout << "v1 = " << endl
       << v1.to_str() << endl
       << endl
       << "v2 = " << endl
       << v2.to_str() << endl
       << endl
       << "(v1 + v2) = " << endl
       << (v1 + v2).to_str() << endl
       << endl
       << "2.0*v1 = " << endl
       << (2.0*v1).to_str() << endl
       << endl
       << "v2*-3 = " << endl
       << (v2*-3.0).to_str() << endl
       << endl
       << "v1*v2 = " << v1*v2 << endl
       << endl
       << "v3*v3 = " << v3*v3 << endl
       << endl
       << "Domain from list = ";
  v1.to_list().for_each(/* Lambda */ [] (const double & val)
			{
			  cout << val << ", ";
			});
  cout << endl;  

  assert(v1 + v2 == 
	 Vector<int>(d, {1, 1.2, 1.4, 1.6, 1.8, 2, 2.2, 2.4, 2.6, 2.8}));

  assert(2.0*v1 == 
	 Vector<int>(d, {0, 0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4, 1.6, 1.8}));
  
  assert(v2*-3.0 ==
	 Vector<int>(d, {-3, -3.3, -3.6, -3.9, -4.2, -4.5, -4.8, -5.1, -5.4, -5.7}));

  assert(v1 - v1 == Vector<int>(d, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));

  assert(v3*v3 == 385.0);

  assert((2.0*v1)/2.0 == v1);
  assert((2.0*v2)/2.0 == v2);

  cout << "Testing iterator on v2 ";
  for (Vector<int>::Iterator it(v2); it.has_curr(); it.next())
    cout << "(" << it.get_curr().first << "," << it.get_curr().second << ") ";
  cout << endl;

  cout << "Testing for_each for v2 vector ";
  v2.for_each([] (const std::pair<int, double> & p)
	      {
		cout << "(" << p.first << "," << p.second << ") ";
	      });

  Vector<int> v4(d, {0,1,2,3,4,5,6,7,8,9});
  for_each(d, [&v4] (const int & i)
	   {
	     assert(v4[i] == i); 
	   });

  Vector<int> zero(d);
  assert(all(d, [&zero] (const int i)
	     {
	       return zero[i] == 0;
	     }));

  Vector<int> v5(d);
  for_each(d, [&v5] (int i)
	   {
	     v5[i] = i;
	   });
  assert(all(d, [&v5] (int i)
	     {
	       return v5[i] == i;
	     }));

  for_each(d, [&v5,&v1] (int i)
	   {
	     v5[i] = v1[i];
	   });
  assert(all(d, [&v5,&v1] (int i)
	{
	  return v5[i] == v1[i];
	}));

  gsl_rng_free(r);
}
