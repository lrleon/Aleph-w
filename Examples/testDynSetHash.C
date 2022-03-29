

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
 [&table] 
             (std::pair<unsigned long, long> p)
                                 { return table.find(p.first) == p.second; } ));
  cout << "done!" << endl
       << endl
       << "Testing remove by data pointer ...." << endl;
  removed_counter = 0;
  for_each(keys, [&table, &removed_counter] (const unsigned long & k)
                 {
                   auto ptr = table.search(k);
                   if (ptr == nullptr)
                     return;

                   table.remove_by_data(ptr->second);
                   ++removed_counter;
                 });
  assert(table.is_empty());

  cout << endl
       << "Reinserting keys for doing other tests ...." << endl;
  for (int i = 0; i < n; ++i)
    table.insert(keys(i), i);
  assert(table.size() == removed_counter);
  cout << "done!" << endl
       << endl;

}

int main(int argn, char *argc[])
{
  assert(Primes::check_primes_database());
  assert(next_prime(5) == 5);

  unsigned long n = argn > 1 ? atoi(argc[1]) : NumItems;
  unsigned int  t = argn > 2 ? atoi(argc[2]) : time(NULL);

  cout << argc[0] << " " << n << " " << t << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, t % gsl_rng_max(r));

  test_DynSetLinHash<DynSetLhash>(n);
  test_DynMapLinHash<DynMapHash>(n);

  test_DynSetLinHash<DynSetLinHash>(n);
  test_DynMapLinHash<DynMapLinHash>(n);

  cout << "testing of ODhash based set ...." << endl
       << endl;
  test_DynSetLinHash<SetODhash>(n);
  cout << endl
       << "Done all test of ODhash based set!" << endl
       << endl
       << endl
       << "testing of OLhash based set ...." << endl
       << endl;
  test_DynSetLinHash<SetOLhash>(n);
  cout << endl
       << "Done all test of OLhash based set!" << endl
       << endl
       << endl
       << "Testing all tests of OLhash based map" << endl
       << endl;
  // test_DynMapLinHash<DynMapODHash>(n);
  // cout << "Done all tests of OD hash based map" << endl
  //      << endl
  //      << endl
  //      << "Testing of OLhash based map" << endl
  //      << endl;
  // test_DynMapLinHash<DynMapOLHash>(n);
  // cout << "Done all tests of OL hash based map" << endl
  //      << endl
  //      << endl;
  gsl_rng_free(r);
}
