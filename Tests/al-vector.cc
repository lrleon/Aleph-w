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
# include <al-vector.H>

using namespace std;
using namespace testing;

TEST(Vector, basic)
{
  AlDomain<char> chars = { 'a', 'b', 'c', 'd', 'e'};

  Vector<char, int> v1(chars);
  Vector<char, int> v2(chars, { 0, 1, 2, 3, 4});
  auto v3 = v2;

  EXPECT_TRUE(eq(sort(v1.get_domain().to_list()), { 'a', 'b', 'c', 'd', 'e'}));
  EXPECT_EQ(v2, v3);

  auto v4 = v1;
  v4 += v2;

  EXPECT_EQ(v2, v4);


}
