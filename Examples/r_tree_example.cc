/*
                          Aleph_w

  Data structures & Algorithms
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/**
 * @file r_tree_example.cc
 * @brief Dynamic spatial indexing with Aleph::RTree.
 *
 * Usage:
 *   ./r_tree_example
 */

#include <cassert>
#include <iostream>
#include <string>

#include <tpl_r_tree.H>
#include <tpl_r_star_tree.H>

using namespace Aleph;
using namespace std;

static Rectangle box(const int x1, const int y1, const int x2, const int y2)
{
  return Rectangle(Geom_Number(x1), Geom_Number(y1),
                   Geom_Number(x2), Geom_Number(y2));
}

int main()
{
  cout << "Aleph-w RTree Example" << endl;
  cout << "=====================" << endl;

  // A dynamic spatial index mapping bounding boxes to named regions.
  RTree<string> map;

  map.insert(box(0, 0, 10, 10), "downtown");
  map.insert(box(8, 8, 20, 20), "midtown");
  map.insert(box(30, 30, 40, 40), "airport");
  map.insert(box(5, 5, 6, 6), "central-park");

  cout << "Indexed " << map.size() << " regions, tree height "
       << map.height() << "." << endl;

  // Which regions overlap the query window (7,7)-(9,9)?
  cout << "Regions intersecting (7,7)-(9,9):";
  map.for_each_intersecting(box(7, 7, 9, 9),
    [](const Rectangle &, const string &name) { cout << ' ' << name; });
  cout << endl;

  // Which regions contain the point (35,35)?
  Array<string> here = map.search_contains(Point(Geom_Number(35), Geom_Number(35)));
  cout << "Region(s) containing point (35,35):";
  for (size_t i = 0; i < here.size(); ++i)
    cout << ' ' << here(i);
  cout << endl;
  assert(here.size() == 1);

  // A far-away query returns nothing.
  assert(map.search_intersects(box(100, 100, 110, 110)).is_empty());

  // Remove a region; the index stays consistent and queryable.
  const bool removed = map.erase(box(30, 30, 40, 40), "airport");
  assert(removed);
  assert(map.size() == 3);
  assert(map.search_contains(Point(Geom_Number(35), Geom_Number(35))).is_empty());
  assert(map.verify());

  cout << "After removing 'airport': " << map.size()
       << " regions, all invariants valid." << endl;

  // RStarTree has the exact same API but uses the R*-tree heuristics
  // (overlap-minimizing subtree choice, margin/overlap split and forced
  // reinsertion), which typically yield less node overlap and faster queries.
  RStarTree<string> rstar;
  rstar.insert(box(0, 0, 10, 10), "a");
  rstar.insert(box(8, 8, 20, 20), "b");
  rstar.insert(box(30, 30, 40, 40), "c");
  assert(rstar.search_intersects(box(7, 7, 9, 9)).size() == 2);
  assert(rstar.verify());
  cout << "RStarTree indexed " << rstar.size()
       << " regions with the same API." << endl;

  return 0;
}