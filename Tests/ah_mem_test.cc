
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
