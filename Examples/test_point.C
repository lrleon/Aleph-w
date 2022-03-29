
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
# include <cassert>
# include <iostream>

# include <point.H>

using namespace std;

int main()
{
  cout << "Testing class Point\n"
       << "===================\n";
  
  Point p1(10, 10), p2(20, 20), p3(30, 30), p4(30, 10), p5(10, 30);
  Segment s1(p1, p2), s2(p2, p3), s3(p1, p3), s4(p4, p5);

  assert(p1.is_colinear_with(p2, p3));
  assert(p2.is_colinear_with(p1, p3));
  assert(p3.is_colinear_with(p1, p2));
  assert(not p4.is_colinear_with(p1, p2));
  assert(not p5.is_colinear_with(p1, p2));

  cout << "Test collinear ok!\n";

  assert(not p1.is_between(p2, p3));
  assert(p2.is_between(p1, p3));
  assert(not p3.is_between(p1, p2));
  assert(p1.is_between(Point(0, 10), Point(20, 10)));
  assert(p1.is_between(Point(10, 0), Point(10, 20)));

  cout << "Test between ok!\n";

  assert(p4.is_to_right_from(p1, p2));
  assert(p4.is_to_right_from(p2, p3));
  assert(p4.is_to_right_from(p1, p3));
  assert(not p5.is_to_right_from(p1, p2));
  assert(not p5.is_to_right_from(p2, p3));
  assert(not p5.is_to_right_from(p1, p3));
  assert(not p1.is_to_right_from(p2, p3));
  assert(not p2.is_to_right_from(p1, p3));
  assert(not p3.is_to_right_from(p1, p2));

  assert(p4.is_to_right_from(s1));
  assert(p4.is_to_right_from(s2));
  assert(p4.is_to_right_from(s3));
  assert(not p5.is_to_right_from(s1));
  assert(not p5.is_to_right_from(s2));
  assert(not p5.is_to_right_from(s3));
  assert(not p1.is_to_right_from(s2));
  assert(not p2.is_to_right_from(s3));
  assert(not p3.is_to_right_from(s1));

  cout << "Test is_to_right_from ok!\n";

  assert(p5.is_to_left_from(p1, p2));
  assert(p5.is_to_left_from(p2, p3));
  assert(p5.is_to_left_from(p1, p3));
  assert(not p4.is_to_left_from(p1, p2));
  assert(not p4.is_to_left_from(p2, p3));
  assert(not p4.is_to_left_from(p1, p3));
  assert(not p1.is_to_left_from(p2, p3));
  assert(not p2.is_to_left_from(p1, p3));
  assert(not p3.is_to_left_from(p1, p2));

  assert(p5.is_to_left_from(s1));
  assert(p5.is_to_left_from(s2));
  assert(p5.is_to_left_from(s3));
  assert(not p4.is_to_left_from(s1));
  assert(not p4.is_to_left_from(s2));
  assert(not p4.is_to_left_from(s3));
  assert(not p1.is_to_left_from(s2));
  assert(not p2.is_to_left_from(s3));
  assert(not p3.is_to_left_from(s1));

  cout << "Test is_to_left_from ok!\n\n";
  
  cout << "Testing class Segment\n"
       << "=====================\n";

  assert(s1.contains_to(p2));
  assert(s1.contains_to(p1));
  assert(not s1.contains_to(p3));
  assert(s2.contains_to(p2));
  assert(s2.contains_to(p3));
  assert(not s2.contains_to(p1));
  assert(s3.contains_to(p1));
  assert(s3.contains_to(p2));
  assert(s3.contains_to(p3));
  assert(not s1.contains_to(p4));
  assert(not s1.contains_to(p5));
  assert(s3.contains_to(s1));
  assert(s3.contains_to(s2));
  assert(not s1.contains_to(s2));
  assert(not s2.contains_to(s1));
  assert(not s1.contains_to(s3));
  assert(not s2.contains_to(s3));
  
  cout << "Test contains_to ok!\n";

  assert(s1.is_to_right_from(p5));
  assert(s2.is_to_right_from(p5));
  assert(s3.is_to_right_from(p5));
  assert(not s1.is_to_right_from(p4));
  assert(not s2.is_to_right_from(p4));
  assert(not s3.is_to_right_from(p4));
  assert(not s2.is_to_right_from(p1));
  assert(not s3.is_to_right_from(p2));
  assert(not s1.is_to_right_from(p3));

  cout << "Test is_to_right_from ok!\n";

  assert(s1.is_to_left_from(p4));
  assert(s2.is_to_left_from(p4));
  assert(s3.is_to_left_from(p4));
  assert(not s1.is_to_left_from(p5));
  assert(not s2.is_to_left_from(p5));
  assert(not s3.is_to_left_from(p5));
  assert(not s2.is_to_left_from(p1));
  assert(not s3.is_to_left_from(p2));
  assert(not s1.is_to_left_from(p3));

  cout << "Test is_to_left_from ok!\n";

  assert(s3.intersects_properly_with(s4));
  assert(not s1.intersects_properly_with(s2));
  assert(not s1.intersects_properly_with(s3));

  Segment s5(p1, Point(50, 10)), s6(Point(30, 10), Point(30, 40));

  assert(not s5.intersects_properly_with(s6));

  cout << "Test intersects_properly_with ok!\n";

  assert(s3.intersects_with(s4));
  assert(s1.intersects_with(s2));
  assert(s1.intersects_with(s3));
  assert(s2.intersects_with(s3));
  assert(s5.intersects_with(s6));

  cout << "Test intersects_with ok!\n";
  
  return 0;
}
