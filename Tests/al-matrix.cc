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
# include <gtest/gtest.h>

# include <ahSort.H>
# include <ah-string-utils.H>
# include <al-matrix.H>

using namespace std;
using namespace testing;

struct SmallDomains : testing::Test
{
  AlDomain<char> rd = { 'a', 'b', 'c', 'd', 'e' };
  AlDomain<string> cd = { "A", "B", "C", "D" };
  using V1 = Vector<char, int>;
  using V2 = Vector<string, int>;
  using Mat1 = Matrix<char, string, int>;
  using Mat2 = Matrix<string, char, int>;

  V1 v1_zero = { rd };
  V1 v1_one = { rd, range<int>(rd.size()) };
  V1 v1_odd = { rd, range<int>(rd.size()).
		maps([] (auto i) { return (i % 2) == 0 ? 1 : 0; }) };
  V1 v1_even = { rd, range<int>(rd.size()).
		 maps([] (auto i) { return (i % 2) == 0 ? 0 : 1; }) };

  V2 v2_zero = { cd };
  V2 v2_one = { cd, range<int>(cd.size()) };
  V2 v2_odd = { cd, range<int>(cd.size()).
		maps([] (auto i) { return (i % 2) == 0 ? 1 : 0; }) };
  V2 v2_even = { cd, range<int>(cd.size()).
		 maps([] (auto i) { return (i % 2) == 0 ? 0 : 1; }) };

  Mat1 m1_zero = { rd, cd };
  Mat1 m1_one = { rd, cd, rd.maps<DynList<int>>([&] (auto)
    { return range<int>(cd.size()); }) };
  Mat1 m1_odd = { rd, rep<V2>(rd.size(), v2_odd) };
  Mat1 m1_even = { rd, rep<V2>(rd.size(), v2_even) };

  Mat2 m2_zero = { cd, rd };
  Mat2 m2_one = { cd, rd, cd.maps<DynList<int>>([&] (auto)
    { return range<int>(cd.size()); }) };
  Mat2 m2_odd = { cd, rep<V1>(cd.size(), v1_odd) };
  Mat2 m2_even = { cd, rep<V1>(cd.size(), v1_even) };

  
};

TEST_F(SmallDomains, basic)
{
  cout << v1_zero << endl << endl
       << v1_one <<  endl << endl
       << v1_odd <<  endl << endl
       << v1_even <<  endl << endl
       << endl
       << v2_zero << endl << endl
       << v2_one <<  endl << endl
       << v2_odd <<  endl << endl
       << v2_even <<  endl << endl
       << m1_even<< endl << endl
       << m1_odd << endl << endl
       << m2_even<< endl << endl
       << m1_odd << endl << endl
       << endl;
}

TEST_F(SmallDomains, identity)
{
  // using M1 = Matrix<char, char, int>;
  // using M2 = Matrix<string, string, int>;
  // cout << M1(rd, rd).identity() << endl
  //      << M2(cd, cd).identity() << endl;
}
