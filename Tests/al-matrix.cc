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
  AlDomain<char> rd = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j' };
  AlDomain<string> cd = { "a", "b", "c", "d", "e", "f", "g" };
  using V1 = Vector<char, int>;
  using V2 = Vector<string, int>;
  using Mat1 = Matrix<char, string, int>;
  using Mat2 = Matrix<string, char, int>;
};

TEST_F(SmallDomains, basic)
{
  Mat1 m11 = { rd, cd };
  Mat2 m21 = { cd, rd };
  V1 v11 = { rd, range<int>(rd.size()).
	     maps([] (auto i) { return (i % 2) == 0 ? 1 : 0; }) };
  V2 v21 = { cd, range<int>(cd.size()).
	     maps([] (auto i) { return (i % 2) != 0 ? 1 : 0; }) };

  EXPECT_EQ(v11*m11, V2(cd));
  cout << v11 << endl << endl
       << v21 << endl << endl
       << m11 << endl << endl
       << m21 << endl << endl
       << v11*m11 << endl << endl;
}

TEST_F(SmallDomains, identity)
{
  using M1 = Matrix<char, char, int>;
  using M2 = Matrix<string, string, int>;
  // cout << M1(rd, rd).identity() << endl
  //      << M2(cd, cd).identity() << endl;
}
