
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
 * @file mpsc_queue_example.cc
 * @brief Several producer threads feeding one consumer through
 * `Aleph::MpscQueue`.
 *
 * Usage:
 *   ./mpsc_queue_example
 */

#include <print_rule.H>
#include <tpl_mpsc_queue.H>

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

using namespace Aleph;

namespace
{
struct Job
{
  int producer;
  int seq;
};

void demo_basic_usage()
{
  std::cout << "[1] Single-threaded push/try_pop walkthrough\n";
  print_rule();

  MpscQueue<int> q;
  q.push(1);
  q.push(2);
  q.emplace(3);

  int out = 0;
  while (q.try_pop(out))
    std::cout << "popped " << out << "\n";
  std::cout << "queue empty: " << std::boolalpha << q.is_empty() << "\n\n";
}

void demo_multiple_producers()
{
  std::cout << "[2] Four producers feeding one consumer\n";
  print_rule();

  constexpr int producer_count = 4;
  constexpr int items_per_producer = 5000;

  MpscQueue<Job> q;
  std::atomic<bool> start{false};
  std::vector<std::thread> producers;

  for (int p = 0; p < producer_count; ++p)
    producers.emplace_back([&, p]
    {
      while (not start.load(std::memory_order_acquire))
        ; // spin until every producer thread has been created
      for (int i = 0; i < items_per_producer; ++i)
        q.push(Job{p, i});
    });

  std::vector<int> per_producer_count(producer_count, 0);
  const int total = producer_count * items_per_producer;
  int consumed = 0;

  start.store(true, std::memory_order_release);

  // The consumer runs on the main thread, matching MpscQueue's
  // single-consumer contract.
  while (consumed < total)
    {
      Job job{};
      if (not q.try_pop(job))
        continue;  // transient: a producer's push is mid-flight, or done
      ++per_producer_count[static_cast<size_t>(job.producer)];
      ++consumed;
    }

  for (auto & t : producers)
    t.join();

  std::cout << "Consumed " << consumed << " jobs from " << producer_count
            << " producers:\n";
  for (int p = 0; p < producer_count; ++p)
    std::cout << "  producer " << p << ": " << per_producer_count[static_cast<size_t>(p)]
              << " jobs (expected " << items_per_producer << ")\n";
  std::cout << "queue empty after drain: " << std::boolalpha << q.is_empty()
            << "\n\n";
}
}  // namespace

int main()
{
  std::cout << "\n=== Aleph::MpscQueue: multi-producer/single-consumer queue ===\n\n";

  demo_basic_usage();
  demo_multiple_producers();

  std::cout << "Done.\n";
  return 0;
}