
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

# include <gtest/gtest.h>

# include <al-domain.H>

using namespace std;
using namespace testing;

TEST(Domain, basic)
{
  AlDomain<int> domain;

  EXPECT_TRUE(domain.is_empty());

  for (size_t i = 0; i < 10; ++i)
    domain.insert(i);

  EXPECT_FALSE(domain.contains(-1));
  EXPECT_FALSE(domain.contains(10));

  EXPECT_TRUE(domain.all([] (auto i) { return i >= 0 and i < 10; }));
}
