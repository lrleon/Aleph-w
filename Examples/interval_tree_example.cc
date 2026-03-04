/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
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
 * @file interval_tree_example.cc
 * @brief Example: meeting scheduler and 1D segment intersection.
 */

# include <iostream>
# include <tpl_interval_tree.H>

using namespace std;
using namespace Aleph;

/// Scenario 1: Meeting scheduler — detect conflicts
void meeting_scheduler()
{
  cout << "=== Meeting Scheduler ===" << endl;

  DynIntervalTree<int> calendar;

  // Insert existing meetings (time in minutes from midnight)
  calendar.insert(540, 600);   // 9:00 - 10:00
  calendar.insert(660, 720);   // 11:00 - 12:00
  calendar.insert(780, 840);   // 13:00 - 14:00
  calendar.insert(870, 930);   // 14:30 - 15:30
  calendar.insert(960, 1020);  // 16:00 - 17:00

  cout << "Existing meetings:" << endl;
  calendar.for_each([](const Interval<int> & iv) {
    cout << "  [" << iv.low / 60 << ":"
         << (iv.low % 60 < 10 ? "0" : "") << iv.low % 60
         << " - " << iv.high / 60 << ":"
         << (iv.high % 60 < 10 ? "0" : "") << iv.high % 60
         << "]" << endl;
  });

  // Try to schedule a new meeting 14:00-15:00 (840-900)
  Interval<int> proposed(840, 900);
  cout << "\nProposed meeting: 14:00 - 15:00" << endl;

  auto conflicts = calendar.find_all_overlaps(proposed);
  if (conflicts.is_empty())
    cout << "  No conflicts! Meeting can be scheduled." << endl;
  else
    {
      cout << "  Conflicts with:" << endl;
      conflicts.for_each([](const Interval<int> & iv) {
        cout << "    [" << iv.low / 60 << ":"
             << (iv.low % 60 < 10 ? "0" : "") << iv.low % 60
             << " - " << iv.high / 60 << ":"
             << (iv.high % 60 < 10 ? "0" : "") << iv.high % 60
             << "]" << endl;
      });
    }

  // Stabbing query: what's happening at 14:30 (870)?
  cout << "\nStabbing query: what meetings are at 14:30?" << endl;
  auto at_1430 = calendar.find_stab(870);
  at_1430.for_each([](const Interval<int> & iv) {
    cout << "  [" << iv.low / 60 << ":"
         << (iv.low % 60 < 10 ? "0" : "") << iv.low % 60
         << " - " << iv.high / 60 << ":"
         << (iv.high % 60 < 10 ? "0" : "") << iv.high % 60
         << "]" << endl;
  });
}

/// Scenario 2: 1D segment intersection
void segment_intersection()
{
  cout << "\n=== 1D Segment Intersection ===" << endl;

  // Given horizontal segments, find pairs sharing x-range
  // Segment A: y=1, x in [1, 5]
  // Segment B: y=2, x in [3, 8]
  // Segment C: y=3, x in [7, 10]
  // Segment D: y=4, x in [12, 15]

  struct Segment
  {
    int y;
    Interval<int> x_range;
  };

  Segment segments[] = {
    {1, {1, 5}},
    {2, {3, 8}},
    {3, {7, 10}},
    {4, {12, 15}}
  };

  cout << "Segments:" << endl;
  for (const auto & s : segments)
    cout << "  y=" << s.y
         << "  x=[" << s.x_range.low << ", " << s.x_range.high << "]"
         << endl;

  // Insert x-ranges, check each new one against existing
  DynIntervalTree<int> active;

  for (const auto & seg : segments)
    {
      auto overlaps = active.find_all_overlaps(seg.x_range);
      if (not overlaps.is_empty())
        {
          cout << "\nSegment y=" << seg.y
               << " x=[" << seg.x_range.low << "," << seg.x_range.high
               << "] shares x-range with:" << endl;
          overlaps.for_each([](const Interval<int> & iv) {
            cout << "  x=[" << iv.low << ", " << iv.high << "]" << endl;
          });
        }
      active.insert_dup(seg.x_range);
    }

  cout << "\nTree verified: " << (active.verify() ? "yes" : "no") << endl;
}

int main()
{
  meeting_scheduler();
  segment_intersection();
  return 0;
}
