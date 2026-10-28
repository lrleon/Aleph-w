
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
 * @file allocation_failure.H
 * @brief A global operator new that a test can make fail.
 *
 * A test program that includes this header, in one of its source files,
 * replaces the global operator new: while a No_Memory lives, every
 * allocation through it throws std::bad_alloc, and while a Fail_At lives,
 * the one with the number that it was given does. allocations counts them
 * all.
 *
 * ThreadSanitizer has its own global operator new and delete, which
 * collide with these at link time, so under it nothing is replaced and
 * ALEPH_ALLOCATION_FAILURE_UNDER_TSAN is 1: the tests must skip then.
 */

#ifndef ALEPH_TESTS_ALLOCATION_FAILURE_H
#define ALEPH_TESTS_ALLOCATION_FAILURE_H

#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(__SANITIZE_THREAD__)
#  define ALEPH_ALLOCATION_FAILURE_UNDER_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define ALEPH_ALLOCATION_FAILURE_UNDER_TSAN 1
#  endif
#endif
#ifndef ALEPH_ALLOCATION_FAILURE_UNDER_TSAN
#  define ALEPH_ALLOCATION_FAILURE_UNDER_TSAN 0
#endif

namespace
{
  bool fail_allocations = false; // while true, operator new throws
  size_t refused_allocations = 0;
  size_t allocations = 0;
  size_t fail_at = 0; // the number of the allocation that throws; 0 for none

  // While it lives, every allocation through operator new fails
  struct No_Memory
  {
    No_Memory() noexcept
    {
      refused_allocations = 0;
      fail_allocations = true;
    }

    ~No_Memory() { fail_allocations = false; }
  };

  // While it lives, the allocation number n from now on, counted from 1,
  // fails
  struct Fail_At
  {
    explicit Fail_At(const size_t n) noexcept
    {
      refused_allocations = 0;
      fail_at = allocations + n;
    }

    ~Fail_At() { fail_at = 0; }
  };

#if !ALEPH_ALLOCATION_FAILURE_UNDER_TSAN
  void * allocate(const std::size_t size)
  {
    ++allocations;
    if (fail_allocations or allocations == fail_at)
      {
        ++refused_allocations;
        throw std::bad_alloc();
      }
    if (void * p = std::malloc(size == 0 ? 1 : size))
      return p;
    throw std::bad_alloc();
  }
#endif
}

#if !ALEPH_ALLOCATION_FAILURE_UNDER_TSAN

void * operator new(const std::size_t size)
{
  return allocate(size);
}

void * operator new[](const std::size_t size)
{
  return allocate(size);
}

// GoogleTest allocates with these too; under AddressSanitizer, its own
// versions would give the operator delete below memory it did not take
void * operator new(const std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void * operator new[](const std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

// They free what operator new took from malloc; GCC does not see that
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

void operator delete(void * p) noexcept
{
  std::free(p);
}

void operator delete[](void * p) noexcept
{
  std::free(p);
}

void operator delete(void * p, std::size_t) noexcept
{
  std::free(p);
}

void operator delete[](void * p, std::size_t) noexcept
{
  std::free(p);
}

void operator delete(void * p, const std::nothrow_t &) noexcept
{
  std::free(p);
}

void operator delete[](void * p, const std::nothrow_t &) noexcept
{
  std::free(p);
}

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#endif // !ALEPH_ALLOCATION_FAILURE_UNDER_TSAN

#endif // ALEPH_TESTS_ALLOCATION_FAILURE_H
