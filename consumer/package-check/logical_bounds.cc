/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file logical_bounds.cc
 * @brief Installed-package check: logical bounds of the arrays under the
 *        sanitizer configuration that the package publishes.
 *
 * Built with nothing but `Aleph::Aleph`. Without arguments it runs every
 * legitimate access and, when the package annotates `MemArray` for
 * AddressSanitizer, checks after each operation that exactly the reserved
 * slots beyond the logical size are poisoned. With the name of a case as
 * argument it reads the first slot beyond the logical size, which
 * AddressSanitizer must report as a container-overflow in any build type.
 *
 * The storage of `ArrayQueue` is circular: its logical sequence is not a
 * prefix of the buffer, so it is only required to work without reports.
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include <aleph-version.H>
#include <tpl_array.H>
#include <tpl_arrayQueue.H>
#include <tpl_memArray.H>

#ifdef ALEPH_MEMARRAY_ANNOTATIONS
# include <sanitizer/asan_interface.h>
#endif

using Aleph::Array;
using Aleph::ArrayQueue;
using Aleph::MemArray;

// What the package declares is what this translation unit got from the
// usage requirements of Aleph::Aleph.
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
static_assert(EXPECT_CONTAINER_ANNOTATIONS == 1,
              "MemArray is annotated but the package does not declare it");
static_assert(Aleph::Build_Info::container_annotations);
#else
static_assert(EXPECT_CONTAINER_ANNOTATIONS == 0,
              "the package declares container annotations but MemArray is not annotated");
static_assert(not Aleph::Build_Info::container_annotations);
#endif

namespace
{
  int failures = 0;

  void check(const bool condition, const char *step, const char *what)
  {
    if (condition)
      return;
    ++failures;
    std::cerr << "FAILED after " << step << ": " << what << '\n';
  }

  /// Live slots are addressable; with annotations, the reserved ones are not.
  void check_bounds(const MemArray<std::string> & a, const char *step)
  {
    check(a.size() <= a.capacity(), step, "size() <= capacity()");
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    const std::string *raw = a.get_ptr();
    for (size_t i = 0; i < a.capacity(); ++i)
      {
        const char *first = reinterpret_cast<const char *>(raw + i);
        const char *last = reinterpret_cast<const char *>(raw + i + 1) - 1;
        const bool poisoned = __asan_address_is_poisoned(first) != 0
                              and __asan_address_is_poisoned(last) != 0;
        const bool addressable = __asan_address_is_poisoned(first) == 0
                                 and __asan_address_is_poisoned(last) == 0;
        if (i < a.size())
          check(addressable, step, "a live slot is addressable");
        else
          check(poisoned, step, "a reserved slot beyond size() is poisoned");
      }
#endif
  }

  /// Reads the first byte of the first slot beyond the logical size.
  [[noreturn]] void read_beyond_size(const MemArray<std::string> & a)
  {
    if (a.size() >= a.capacity())
      {
        std::cout << "no reserved slot to read: the read was not reported\n";
        std::exit(EXIT_FAILURE);
      }
    const volatile char *byte = reinterpret_cast<const volatile char *>(a.get_ptr() + a.size());
    volatile char sink = *byte;
    (void) sink;
    std::cout << "the read was not reported\n";
    std::exit(EXIT_FAILURE);
  }

  /// Runs the sequence of operations; `stop_at` names where to overflow.
  void memarray_sequence(const char *stop_at)
  {
    const auto at = [stop_at] (const char *step) { return std::strcmp(stop_at, step) == 0; };

    MemArray<std::string> a(16);
    check_bounds(a, "construction");

    for (int i = 0; i < 5; ++i)
      a.put(std::to_string(i));
    check(a.size() == 5 and a(4) == "4", "put", "the items are stored");
    check_bounds(a, "put");
    if (at("after_put"))
      read_beyond_size(a);

    for (int i = 5; i < 40; ++i)
      a.append(std::to_string(i));
    check(a.size() == 40 and a(39) == "39", "growth", "the items are stored");
    check_bounds(a, "growth");
    if (at("after_growth"))
      read_beyond_size(a);

    for (int i = 0; i < 30; ++i)
      (void) a.get();
    check(a.size() == 10 and a(9) == "9", "shrink", "the first items remain");
    check_bounds(a, "shrink");
    if (at("after_shrink"))
      read_beyond_size(a);

    a.putn(3);
    a(12) = "putn";
    check_bounds(a, "putn");

    a.push("front");
    check(a(0) == "front", "push", "the item is first");
    check(a.remove_first() == "front", "remove_first", "the first item is returned");
    check_bounds(a, "push and remove_first");

    a.reserve(256);
    check(a.capacity() >= 256 and a(12) == "putn", "reserve", "the items are kept");
    check_bounds(a, "reserve");

    MemArray<std::string> copy(a);
    check(copy.size() == a.size() and copy(12) == "putn", "copy", "the items are copied");
    check_bounds(copy, "copy construction");

    MemArray<std::string> assigned(4);
    assigned = a;
    check_bounds(assigned, "copy assignment");

    MemArray<std::string> moved(std::move(copy));
    check(moved.size() == a.size() and moved(12) == "putn", "move", "the items are moved");
    check_bounds(moved, "move construction");
    check_bounds(copy, "move construction (source)");
    if (at("after_move"))
      read_beyond_size(moved);

    MemArray<std::string> small(8);
    small.put("small");
    small.swap(moved);
    check(small.size() == a.size() and moved.size() == 1 and moved(0) == "small",
          "swap", "the contents are exchanged");
    check_bounds(small, "swap");
    check_bounds(moved, "swap (other)");
    if (at("after_swap"))
      read_beyond_size(moved);

    // A moved-from array is reused.
    copy.put("reused");
    check(copy.size() == 1 and copy(0) == "reused", "reuse", "a moved-from array is reused");
    check_bounds(copy, "reuse of a moved-from array");

    a.empty();
    check(a.size() == 0, "empty", "the array is empty");
    check_bounds(a, "empty");

    a.empty_and_release();
    check_bounds(a, "empty_and_release");
    a.put("again");
    check(a.size() == 1 and a(0) == "again", "reuse", "a released array is reused");
    check_bounds(a, "reuse of a released array");
    if (at("after_reuse"))
      read_beyond_size(a);
  }

  void array_sequence()
  {
    Array<int> a = Array<int>::create_reserved(8);
    for (int i = 0; i < 100; ++i)
      a.append(i);
    long sum = 0;
    for (auto it = a.get_it(); it.has_curr(); it.next_ne())
      sum += it.get_curr();
    check(a.size() == 100 and sum == 4950, "Array", "append and iteration");

    Array<int> moved(std::move(a));
    a.append(7);
    check(moved.size() == 100 and a.size() == 1 and a(0) == 7, "Array", "moved-from reuse");
  }

  void queue_sequence()
  {
    ArrayQueue<int> q(8);
    for (int i = 0; i < 8; ++i)
      q.put(i);
    for (int i = 0; i < 5; ++i)
      check(q.get() == i, "ArrayQueue", "FIFO order");
    for (int i = 8; i < 13; ++i)
      q.put(i);  // the storage wraps around
    check(q.size() == 8 and q.front() == 5 and q.rear() == 12, "ArrayQueue wrap-around",
          "front and rear");
    for (size_t i = 0; i < q.size(); ++i)
      {
        check(q.front(i) == static_cast<int>(5 + i), "ArrayQueue wrap-around", "front(i)");
        check(q.rear(i) == static_cast<int>(12 - i), "ArrayQueue wrap-around", "rear(i)");
      }

    ArrayQueue<int> copy(q);
    check(copy.size() == 8 and copy.front() == 5 and copy.rear() == 12, "ArrayQueue copy",
          "a wrapped queue is copied in order");

    q.reserve(64);
    check(q.size() == 8 and q.front() == 5 and q.rear() == 12, "ArrayQueue reserve",
          "a wrapped queue is reserved in order");
    for (int i = 13; i < 60; ++i)
      q.put(i);
    for (int i = 5; i < 60; ++i)
      check(q.get() == i, "ArrayQueue growth", "FIFO order");
    check(q.size() == 0, "ArrayQueue", "empty after draining");

    ArrayQueue<int> moved(std::move(copy));
    check(moved.size() == 8 and moved.front() == 5, "ArrayQueue move", "the items are moved");
    copy.put(1);
    check(copy.size() == 1 and copy.front() == 1, "ArrayQueue move", "moved-from reuse");

    moved.empty();
    moved.put(2);
    check(moved.size() == 1 and moved.front() == 2, "ArrayQueue empty", "reuse");
    moved.empty_and_release();
    moved.put(3);
    moved.put(4);
    check(moved.size() == 2 and moved.front() == 3 and moved.rear() == 4,
          "ArrayQueue empty_and_release", "reuse");
  }
}

int main(int argc, char **argv)
{
  const char *stop_at = argc > 1 ? argv[1] : "";

  memarray_sequence(stop_at);
  if (stop_at[0] != '\0')
    {
      std::cout << "unknown case '" << stop_at << "': the read was not reported\n";
      return EXIT_FAILURE;
    }

  array_sequence();
  queue_sequence();

  if (failures != 0)
    {
      std::cerr << failures << " check(s) failed\n";
      return EXIT_FAILURE;
    }

  std::cout << "logical bounds: ok (container annotations "
            << (Aleph::Build_Info::container_annotations ? "on" : "off") << ")\n";
  return EXIT_SUCCESS;
}
