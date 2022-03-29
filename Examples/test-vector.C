

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
 [] (const double & val)
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
