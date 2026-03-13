/**
 * @file concurrency_utils_example.cc
 * @brief Focused examples for bounded channels and synchronized wrappers.
 *
 * Usage:
 *   ./concurrency_utils_example
 */

#include <concurrency_utils.H>
#include <thread_pool.H>

#include <algorithm>
#include <iostream>
#include <vector>

using namespace Aleph;

int main()
{
  std::cout << "Example 1: bounded_channel with TaskGroup\n";
  bounded_channel<int> jobs(4);
  synchronized<std::vector<int>> squares(std::in_place);
  rw_synchronized<int> processed(0);
  ThreadPool pool(4);
  TaskGroup group(pool);

  group.launch([&] {
    for (int i = 1; i <= 8; ++i)
      jobs.send(i);
    jobs.close();
  });

  for (int worker = 0; worker < 2; ++worker)
    group.launch([&] {
      while (auto item = jobs.recv())
        {
          squares.with_lock([&](auto & out) {
            out.push_back(*item * *item);
          });
          processed.with_write_lock([](int & value) { ++value; });
        }
    });

  group.wait();

  auto sorted = squares.with_lock([](const auto & out) {
    auto copy = out;
    std::sort(copy.begin(), copy.end());
    return copy;
  });

  std::cout << "Squares:";
  for (const int value : sorted)
    std::cout << " " << value;
  std::cout << "\nProcessed count: "
            << processed.with_read_lock([](const int & value) { return value; })
            << "\n\n";

  std::cout << "Example 2: cancellation-aware recv\n";
  bounded_channel<int> canceled(1);
  CancellationSource stop;
  stop.request_cancel();
  try
    {
      (void) canceled.recv(stop.token());
      std::cout << "unexpected success\n";
    }
  catch (const operation_canceled &)
    {
      std::cout << "recv interrupted by cancellation\n";
    }

  std::cout << "\nExample 3: spsc_queue handoff\n";
  spsc_queue<int> handoff(4);
  handoff.try_push(10);
  handoff.try_push(20);
  auto first = handoff.try_pop();
  auto second = handoff.try_pop();
  std::cout << "Popped:";
  if (first.has_value())
    std::cout << " " << *first;
  if (second.has_value())
    std::cout << " " << *second;
  std::cout << "\n";

  return 0;
}
