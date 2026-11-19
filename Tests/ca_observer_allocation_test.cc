
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
 * @file ca_observer_allocation_test.cc
 * @brief Stationary_Detector when an allocation fails: buffering the hash
 *        of a frame throws std::bad_alloc, which reaches the caller of the
 *        callback, and leaves the hashes and their steps as they were.
 *
 * This program replaces the global operator new, so that a test can make
 * an allocation fail.
 */

# include <gtest/gtest.h>

# include "allocation_failure.H"

# include <cstddef>
# include <limits>
# include <new>
# include <string>
# include <utility>

# include <ca-observer.H>
# include <tpl_array.H>
# include <tpl_ca_lattice.H>
# include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

  // n frames that differ from each other: the frame k has k in a cell
  Array<L> distinct_frames(const int n)
  {
    Array<L> frames;
    for (int k = 0; k < n; ++k)
      {
        L frame({4, 4}, 0);
        frame.set({0, 0}, k);
        frames.append(std::move(frame));
      }
    return frames;
  }
}

// A window of SIZE_MAX keeps every frame, so that the buffers grow with the
// run. They grew inside push(), which was noexcept: a bad_alloc ended the
// program instead of reaching the caller of the callback
TEST(StationaryDetectorAllocation, AFailedBufferingReachesTheCaller)
{
  if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
    GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

  constexpr int Frames = 40;
  const Array<L> frames = distinct_frames(Frames);
  Stationary_Detector det(std::numeric_limits<std::size_t>::max());
  det.on_step_begin(0, frames(0));
  ASSERT_EQ(det.buffered_hashes(), 1u);

  size_t refused = 0;
  for (int k = 1; k < Frames; ++k)
    {
      SCOPED_TRACE("step " + std::to_string(k));
      const size_t before = det.buffered_hashes();
      bool threw = false;
      {
        const Fail_At failing(1);
        try
          {
            det.on_step_end(k, frames(k));
          }
        catch (const std::bad_alloc &)
          {
            threw = true;
          }
        refused += refused_allocations;
      }
      if (threw)
        {
          EXPECT_EQ(det.buffered_hashes(), before);
          det.on_step_end(k, frames(k));  // without the failure, it buffers
        }
      else  // completed: it must not have caught a failed allocation
        EXPECT_EQ(refused_allocations, 0u) << "no std::bad_alloc reached the caller";
      EXPECT_EQ(det.buffered_hashes(), before + 1);
      EXPECT_FALSE(det.cycle_detected());
    }
  EXPECT_GT(refused, 0u) << "the buffers did not grow under a failure";

  // The frame 5 again: each hash kept its step, so the cycle starts at 5
  det.on_step_end(Frames, frames(5));
  ASSERT_TRUE(det.cycle_detected());
  EXPECT_EQ(*det.cycle_start(), 5u);
  EXPECT_EQ(*det.cycle_length(), static_cast<size_t>(Frames - 5));
}

// A small window keeps few hashes in buffers that start with room for 32:
// dropping the oldest hash shrank the buffer of the hashes, which
// allocates, and a failure there left each hash with the step of the next
// one, so that a cycle was reported from the wrong step
TEST(StationaryDetectorAllocation, AFullSmallWindowKeepsEachHashWithItsStep)
{
  if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
    GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

  constexpr int Frames = 24;
  const Array<L> frames = distinct_frames(Frames);
  for (size_t window = 1; window <= 12; ++window)
    {
      SCOPED_TRACE("window " + std::to_string(window));
      Stationary_Detector det(window);
      det.on_step_begin(0, frames(0));
      for (int k = 1; k < Frames; ++k)
        {
          bool threw = false;
          {
            const Fail_At failing(1);
            try
              {
                det.on_step_end(k, frames(k));
              }
            catch (const std::bad_alloc &)
              {
                threw = true;
              }
          }
          if (threw)
            det.on_step_end(k, frames(k));  // without the failure, it buffers
          else  // completed: it must not have caught a failed allocation
            EXPECT_EQ(refused_allocations, 0u) << "no std::bad_alloc reached the caller";
        }
      ASSERT_EQ(det.buffered_hashes(), window + 1);
      EXPECT_FALSE(det.cycle_detected());

      // A frame of the window again: it was seen at its step
      const size_t seen = Frames - window;
      det.on_step_end(Frames, frames(seen));
      ASSERT_TRUE(det.cycle_detected());
      EXPECT_EQ(*det.cycle_start(), seen);
      EXPECT_EQ(*det.cycle_length(), window);
    }
}
