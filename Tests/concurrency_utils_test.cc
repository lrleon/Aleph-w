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

#include <gtest/gtest.h>

#include <concurrency_utils.H>
#include <thread_pool.H>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace Aleph;
using namespace std::chrono_literals;

namespace
{
  template <typename Sync>
  concept SupportsValueReturningWithLock =
      requires(Sync sync)
      {
        sync.with_lock([](auto & value) { return value; });
      };

  template <typename Sync>
  concept SupportsValueReturningWithReadLock =
      requires(Sync sync)
      {
        sync.with_read_lock([](const auto & value) { return value; });
      };

  template <typename Sync>
  concept SupportsValueReturningWithWriteLock =
      requires(Sync sync)
      {
        sync.with_write_lock([](auto & value) { return value; });
      };

  struct TokenPayload
  {
    CancellationToken token;
    int value;

    TokenPayload(CancellationToken token_arg, int value_arg)
      : token(std::move(token_arg)), value(value_arg) {}
  };

  struct ExceptionSafePayload
  {
    static inline bool throw_on_move_construction = false;
    static inline bool throw_on_move_assignment = false;

    int value = 0;

    ExceptionSafePayload() = default;

    explicit ExceptionSafePayload(int value_arg)
      : value(value_arg) {}

    ExceptionSafePayload(const ExceptionSafePayload &) = default;
    ExceptionSafePayload & operator = (const ExceptionSafePayload &) = default;

    ExceptionSafePayload(ExceptionSafePayload && other)
      : value(other.value)
    {
      other.value = -1;
      if (throw_on_move_construction)
        throw std::runtime_error("move construction failed");
    }

    ExceptionSafePayload & operator = (ExceptionSafePayload && other)
    {
      value = other.value;
      other.value = -1;
      if (throw_on_move_assignment)
        throw std::runtime_error("move assignment failed");
      return *this;
    }

    static void reset_failures() noexcept
    {
      throw_on_move_construction = false;
      throw_on_move_assignment = false;
    }
  };
}

static_assert(SupportsValueReturningWithLock<synchronized<int>>);
static_assert(SupportsValueReturningWithReadLock<rw_synchronized<int>>);
static_assert(SupportsValueReturningWithWriteLock<rw_synchronized<int>>);

TEST(ConcurrencyUtils, BoundedChannelCloseDrainsQueuedItems)
{
  bounded_channel<int> ch(2);

  ASSERT_TRUE(ch.send(10));
  ASSERT_TRUE(ch.send(20));
  ch.close();

  auto first = ch.recv();
  auto second = ch.recv();
  auto third = ch.recv();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(*first, 10);
  EXPECT_EQ(*second, 20);
  EXPECT_FALSE(third.has_value());
  EXPECT_FALSE(ch.send(30));
}

TEST(ConcurrencyUtils, BoundedChannelTrySendAndTryRecv)
{
  bounded_channel<int> ch(1);

  EXPECT_TRUE(ch.try_send(7));
  EXPECT_FALSE(ch.try_send(8));

  auto first = ch.try_recv();
  auto second = ch.try_recv();

  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(*first, 7);
  EXPECT_FALSE(second.has_value());
}

TEST(ConcurrencyUtils, BoundedChannelCloseUnblocksSenderAndReceiver)
{
  bounded_channel<int> full_ch(1);
  ASSERT_TRUE(full_ch.send(1));

  std::atomic<bool> sender_entered{false};
  std::promise<bool> sender_result;
  auto sender_future = sender_result.get_future();
  std::thread sender([&] {
    sender_entered.store(true, std::memory_order_release);
    sender_result.set_value(full_ch.send(2));
  });

  while (not sender_entered.load(std::memory_order_acquire))
    std::this_thread::yield();
  full_ch.close();
  EXPECT_FALSE(sender_future.get());
  sender.join();

  bounded_channel<int> empty_ch(1);
  std::atomic<bool> receiver_entered{false};
  std::promise<std::optional<int>> receiver_result;
  auto receiver_future = receiver_result.get_future();
  std::thread receiver([&] {
    receiver_entered.store(true, std::memory_order_release);
    receiver_result.set_value(empty_ch.recv());
  });

  while (not receiver_entered.load(std::memory_order_acquire))
    std::this_thread::yield();
  empty_ch.close();
  EXPECT_FALSE(receiver_future.get().has_value());
  receiver.join();
}

TEST(ConcurrencyUtils, BoundedChannelProducerConsumerPreservesOrder)
{
  bounded_channel<int> ch(8);
  std::vector<int> received;
  received.reserve(2000);
  std::atomic<bool> producer_ok{true};

  std::thread producer([&] {
    for (int i = 0; i < 2000; ++i)
      if (not ch.send(i))
        producer_ok.store(false, std::memory_order_relaxed);
    ch.close();
  });

  std::thread consumer([&] {
    while (auto item = ch.recv())
      received.push_back(*item);
  });

  producer.join();
  consumer.join();

  EXPECT_TRUE(producer_ok.load(std::memory_order_relaxed));
  ASSERT_EQ(received.size(), 2000u);
  for (size_t i = 0; i < received.size(); ++i)
    EXPECT_EQ(received[i], static_cast<int>(i));
}

TEST(ConcurrencyUtils, BoundedChannelSupportsMoveOnlyPayloads)
{
  bounded_channel<std::unique_ptr<int>> ch(1);

  ASSERT_TRUE(ch.send(std::make_unique<int>(42)));
  auto item = ch.recv();

  ASSERT_TRUE(item.has_value());
  ASSERT_TRUE(item->get() != nullptr);
  EXPECT_EQ(**item, 42);
}

TEST(ConcurrencyUtils, BoundedChannelCancellationInterruptsBlockedSend)
{
  bounded_channel<int> ch(1);
  CancellationSource source;
  ASSERT_TRUE(ch.send(1));

  std::atomic<bool> started{false};
  std::promise<bool> canceled;
  auto future = canceled.get_future();

  std::thread sender([&] {
    started.store(true, std::memory_order_release);
    try
      {
        (void) ch.send(2, source.token());
        canceled.set_value(false);
      }
    catch (const operation_canceled &)
      {
        canceled.set_value(true);
      }
  });

  while (not started.load(std::memory_order_acquire))
    std::this_thread::yield();
  std::this_thread::sleep_for(5ms);
  source.request_cancel();

  EXPECT_TRUE(future.get());
  sender.join();
}

TEST(ConcurrencyUtils, BoundedChannelCancellationInterruptsBlockedRecv)
{
  bounded_channel<int> ch(1);
  CancellationSource source;

  std::atomic<bool> started{false};
  std::promise<bool> canceled;
  auto future = canceled.get_future();

  std::thread receiver([&] {
    started.store(true, std::memory_order_release);
    try
      {
        (void) ch.recv(source.token());
        canceled.set_value(false);
      }
    catch (const operation_canceled &)
      {
        canceled.set_value(true);
      }
  });

  while (not started.load(std::memory_order_acquire))
    std::this_thread::yield();
  std::this_thread::sleep_for(5ms);
  source.request_cancel();

  EXPECT_TRUE(future.get());
  receiver.join();
}

TEST(ConcurrencyUtils, BoundedChannelCloseWinsIfOperationCanProceed)
{
  bounded_channel<int> ch(1);
  CancellationSource source;

  ASSERT_TRUE(ch.send(11));
  ch.close();
  source.request_cancel();

  auto first = ch.recv(source.token());
  auto second = ch.recv(source.token());

  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(*first, 11);
  EXPECT_FALSE(second.has_value());
}

TEST(ConcurrencyUtils, BoundedChannelCancellationAppliesToEmplace)
{
  bounded_channel<int> ch(1);
  CancellationSource source;
  ASSERT_TRUE(ch.send(1));

  std::atomic<bool> started{false};
  std::promise<bool> canceled;
  auto future = canceled.get_future();

  std::thread producer([&] {
    started.store(true, std::memory_order_release);
    try
      {
        (void) ch.emplace(source.token(), 2);
        canceled.set_value(false);
      }
    catch (const operation_canceled &)
      {
        canceled.set_value(true);
      }
  });

  while (not started.load(std::memory_order_acquire))
    std::this_thread::yield();
  std::this_thread::sleep_for(5ms);
  source.request_cancel();

  EXPECT_TRUE(future.get());
  producer.join();
}

TEST(ConcurrencyUtils, BoundedChannelEmplaceAcceptsCancellationTokenPayloads)
{
  bounded_channel<TokenPayload> ch(1);
  CancellationSource source;
  auto token = source.token();

  ASSERT_TRUE(ch.emplace(token, 42));

  auto received = ch.recv();
  ASSERT_TRUE(received.has_value());
  EXPECT_EQ(received->value, 42);
  EXPECT_FALSE(received->token.stop_requested());

  source.request_cancel();

  EXPECT_TRUE(received->token.stop_requested());
}

TEST(ConcurrencyUtils, BoundedChannelRecvUsesCopyFallbackForThrowingMoves)
{
  ExceptionSafePayload::reset_failures();

  bounded_channel<ExceptionSafePayload> ch(1);
  ASSERT_TRUE(ch.send(ExceptionSafePayload(23)));

  ExceptionSafePayload::throw_on_move_construction = true;
  auto received = ch.recv();
  ExceptionSafePayload::reset_failures();

  ASSERT_TRUE(received.has_value());
  EXPECT_EQ(received->value, 23);
}

TEST(ConcurrencyUtils, BoundedChannelTryRecvPreservesQueuedValueOnAssignmentFailure)
{
  ExceptionSafePayload::reset_failures();

  bounded_channel<ExceptionSafePayload> ch(1);
  ASSERT_TRUE(ch.send(ExceptionSafePayload(17)));

  ExceptionSafePayload out;
  ExceptionSafePayload::throw_on_move_assignment = true;
  EXPECT_THROW((void) ch.try_recv(out), std::runtime_error);
  ExceptionSafePayload::reset_failures();

  auto recovered = ch.recv();
  ASSERT_TRUE(recovered.has_value());
  EXPECT_EQ(recovered->value, 17);
}

TEST(ConcurrencyUtils, SynchronizedWithLockAndGuard)
{
  synchronized<std::vector<int>> values(std::in_place);

  values.with_lock([](auto & v) {
    v.push_back(1);
    v.push_back(2);
  });

  {
    auto locked = values.lock();
    locked->push_back(3);
  }

  const auto copy = values.with_lock([](const auto & v) {
    return v;
  });

  EXPECT_EQ(copy, (std::vector<int>{1, 2, 3}));
}

TEST(ConcurrencyUtils, SynchronizedCounterUnderContention)
{
  ThreadPool pool(4);
  TaskGroup group(pool);
  synchronized<int> counter(0);

  for (int worker = 0; worker < 8; ++worker)
    group.launch([&] {
      for (int i = 0; i < 2000; ++i)
        counter.with_lock([](int & value) { ++value; });
    });

  group.wait();

  EXPECT_EQ(counter.with_lock([](const int & value) { return value; }), 16000);
}

TEST(ConcurrencyUtils, RwSynchronizedSupportsReadAndWriteAccess)
{
  rw_synchronized<std::vector<int>> values(std::in_place);

  values.with_write_lock([](auto & v) {
    v.push_back(4);
    v.push_back(5);
  });

  {
    auto write_locked = values.write();
    write_locked->push_back(6);
  }

  {
    auto read_locked = values.read();
    ASSERT_EQ(read_locked->size(), 3u);
    EXPECT_EQ((*read_locked)[0], 4);
    EXPECT_EQ((*read_locked)[2], 6);
  }

  EXPECT_EQ(values.with_read_lock([](const auto & v) {
    return std::accumulate(v.begin(), v.end(), 0);
  }), 15);
}

TEST(ConcurrencyUtils, SynchronizedSupportsNonDefaultConstructibleValues)
{
  struct Payload
  {
    explicit Payload(int x) : value(x) {}
    int value;
  };

  synchronized<Payload> payload(std::in_place, 7);
  payload.with_lock([](Payload & p) { p.value += 5; });
  EXPECT_EQ(payload.with_lock([](const Payload & p) { return p.value; }), 12);
}

TEST(ConcurrencyUtils, RwSynchronizedSupportsNonDefaultConstructibleValues)
{
  struct Payload
  {
    explicit Payload(int x) : value(x) {}
    int value;
  };

  rw_synchronized<Payload> payload(std::in_place, 9);
  payload.with_write_lock([](Payload & p) { p.value *= 2; });
  EXPECT_EQ(payload.with_read_lock([](const Payload & p) { return p.value; }), 18);
}

TEST(ConcurrencyUtils, SpscQueueBasicPushPop)
{
  spsc_queue<int> queue(3);

  EXPECT_TRUE(queue.try_push(1));
  EXPECT_TRUE(queue.try_push(2));
  EXPECT_TRUE(queue.try_push(3));
  EXPECT_FALSE(queue.try_push(4));
  EXPECT_TRUE(queue.full());

  auto first = queue.try_pop();
  auto second = queue.try_pop();
  auto third = queue.try_pop();
  auto fourth = queue.try_pop();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  ASSERT_TRUE(third.has_value());
  EXPECT_EQ(*first, 1);
  EXPECT_EQ(*second, 2);
  EXPECT_EQ(*third, 3);
  EXPECT_FALSE(fourth.has_value());
  EXPECT_TRUE(queue.empty());
}

TEST(ConcurrencyUtils, SpscQueueProducerConsumerRoundTrip)
{
  spsc_queue<int> queue(64);
  std::vector<int> received;
  received.reserve(5000);

  std::thread producer([&] {
    for (int i = 0; i < 5000; ++i)
      while (not queue.try_push(i))
        std::this_thread::yield();
  });

  std::thread consumer([&] {
    while (received.size() < 5000)
      {
        auto item = queue.try_pop();
        if (item.has_value())
          received.push_back(*item);
        else
          std::this_thread::yield();
      }
  });

  producer.join();
  consumer.join();

  ASSERT_EQ(received.size(), 5000u);
  for (size_t i = 0; i < received.size(); ++i)
    EXPECT_EQ(received[i], static_cast<int>(i));
}

TEST(ConcurrencyUtils, SpscQueueSupportsMoveOnlyValues)
{
  spsc_queue<std::unique_ptr<int>> queue(2);

  ASSERT_TRUE(queue.try_push(std::make_unique<int>(42)));
  auto item = queue.try_pop();

  ASSERT_TRUE(item.has_value());
  ASSERT_TRUE(item->get() != nullptr);
  EXPECT_EQ(**item, 42);
}

TEST(ConcurrencyUtils, SpscQueueTryPopUsesCopyFallbackForThrowingMoves)
{
  ExceptionSafePayload::reset_failures();

  spsc_queue<ExceptionSafePayload> queue(1);
  ASSERT_TRUE(queue.try_push(ExceptionSafePayload(31)));

  ExceptionSafePayload::throw_on_move_construction = true;
  auto item = queue.try_pop();
  ExceptionSafePayload::reset_failures();

  ASSERT_TRUE(item.has_value());
  EXPECT_EQ(item->value, 31);
}
