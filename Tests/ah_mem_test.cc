
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/**
 * @file ah_mem_test.cc
 * @brief Tests for Ah Mem
 */

#include <gtest/gtest.h>

#include <ahMem.H>

#include <cstring>
#include <stdexcept>

namespace
{

constexpr std::size_t kBlockSize = 64;

} // namespace

TEST(AhMemTest, AllocateReturnsValidBuffer)
{
  void * ptr = Aleph::allocate(kBlockSize);
  ASSERT_NE(ptr, nullptr);
  std::memset(ptr, 0xAB, kBlockSize);
  Aleph::deallocate(ptr);
}

TEST(AhMemTest, AllocateRejectsZeroSize)
{
  EXPECT_THROW(static_cast<void>(Aleph::allocate(0)), std::invalid_argument);
}

TEST(AhMemTest, AllocateZeroedClearsMemory)
{
  void * ptr = Aleph::allocate_zeroed(kBlockSize);
  ASSERT_NE(ptr, nullptr);
  const unsigned char * bytes =
    static_cast<const unsigned char *>(ptr);
  for (std::size_t i = 0; i < kBlockSize; ++i)
    EXPECT_EQ(bytes[i], 0u);
  Aleph::deallocate(ptr);
}

TEST(AhMemTest, AllocateZeroedRejectsZeroSize)
{
  EXPECT_THROW(static_cast<void>(Aleph::allocate_zeroed(0)),
               std::invalid_argument);
}
