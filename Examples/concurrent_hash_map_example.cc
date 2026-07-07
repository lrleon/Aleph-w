
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
 * @file concurrent_hash_map_example.cc
 * @brief Several worker threads sharing one `Aleph::ConcurrentHashMap`.
 *
 * Usage:
 *   ./concurrent_hash_map_example
 */

#include <print_rule.H>
#include <tpl_concurrent_hash_map.H>

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

using namespace Aleph;

namespace
{
void demo_basic_usage()
{
  std::cout << "[1] Single-threaded insert/find/erase walkthrough\n";
  print_rule();

  ConcurrentHashMap<std::string, int> counters;
  counters.insert("visits", 0);
  counters.insert("errors", 0);

  counters.with_value_mut("visits", [](int &v) { v += 3; });
  counters.insert_or_assign("errors", 1);

  std::cout << "visits: " << *counters.find_copy("visits") << " (expect 3)\n";
  std::cout << "errors: " << *counters.find_copy("errors") << " (expect 1)\n";
  std::cout << "contains 'timeouts': " << std::boolalpha
            << counters.contains("timeouts") << " (expect false)\n\n";
}

void demo_concurrent_workers()
{
  std::cout << "[2] Eight worker threads sharing one sharded map\n";
  print_rule();

  constexpr int worker_count = 8;
  constexpr int keys_per_worker = 4000;

  // 16 shards: each worker's keys are spread across shards by hash, so
  // most concurrent inserts from different workers proceed without
  // contending on the same shard lock.
  ConcurrentHashMap<int, int, Aleph::equal_to<int>, 16> map;
  std::atomic<bool> start{false};
  std::vector<std::thread> workers;

  for (int w = 0; w < worker_count; ++w)
    workers.emplace_back([&, w]
    {
      while (not start.load(std::memory_order_acquire))
        ; // wait until every worker thread has been created
      const int base = w * keys_per_worker;
      for (int i = 0; i < keys_per_worker; ++i)
        map.insert(base + i, i);
    });

  start.store(true, std::memory_order_release);
  for (auto &w : workers)
    w.join();

  std::cout << "Total entries: " << map.size() << " (expect "
            << worker_count * keys_per_worker << ")\n";

  auto snapshot = map.snapshot();
  std::cout << "Snapshot size: " << snapshot.size()
            << " (an independent copy, unaffected by later mutations)\n\n";
}
}  // namespace

int main()
{
  std::cout << "\n=== Aleph::ConcurrentHashMap: sharded concurrent map ===\n\n";

  demo_basic_usage();
  demo_concurrent_workers();

  std::cout << "Done.\n";
  return 0;
}