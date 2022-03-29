

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
);

  auto part = zip_partition([] (auto t) { return get<0>(t) < 2; }, l1, l2, l3);

  EXPECT_TRUE(eq(get<0>(part).maps<string>([] (auto & t)
    {
      return to_string(get<0>(t)) + to_string(get<1>(t)) + get<2>(t);
    }), build_dynlist<string>("000", "111")));
  EXPECT_EQ(get<1>(part), 2);
  EXPECT_TRUE(eq(get<2>(part).maps<string>([] (auto & t)
    {
      return to_string(get<0>(t)) + to_string(get<1>(t)) + get<2>(t);
    }), build_dynlist<string>("222", "333", "444")));
  EXPECT_EQ(get<3>(part), N - 2);

  auto l = zip_lists(l1, l2, l1, l2);
  for (auto it = zip_it(l, l1, l2); it.has_curr(); it.next_ne())
    {
      auto c = it.get_curr_ne();
      EXPECT_EQ(get<0>(c), build_dynlist<int>(get<1>(c), get<2>(c),
					      get<1>(c), get<2>(c)));
    }
}
