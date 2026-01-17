
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
 * @file ah_signal_test.cc
 * @brief Unit tests for ah-signal.H
 *
 * Tests cover:
 * - Signal class: installation, restoration, RAII behavior
 * - SignalSet class: add, remove, contains operations
 * - SignalBlocker class: blocking and unblocking signals
 * - Move semantics for Signal and SignalBlocker
 * - Error handling and edge cases
 * - Backward compatibility with original API
 */

#include <gtest/gtest.h>

#include <ah-signal.H>

#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>
#include <unistd.h>

namespace
{

// Global flag for signal handlers (must be volatile sig_atomic_t for safety)
volatile sig_atomic_t g_signal_received = 0;
std::atomic<int> g_signal_count{0};

void test_handler(int signo)
{
  g_signal_received = signo;
  g_signal_count.fetch_add(1, std::memory_order_relaxed);
}

void another_handler(int signo)
{
  g_signal_received = signo + 1000;
}

// Reset global state before each test
class SignalTestFixture : public ::testing::Test
{
protected:
  void SetUp() override
  {
    g_signal_received = 0;
    g_signal_count.store(0, std::memory_order_relaxed);
  }
};

} // namespace

// ============================================================================
// SignalSet Tests
// ============================================================================

TEST(SignalSet, DefaultConstructorCreatesEmptySet)
{
  SignalSet set;
  EXPECT_FALSE(set.contains(SIGUSR1));
  EXPECT_FALSE(set.contains(SIGUSR2));
  EXPECT_FALSE(set.contains(SIGINT));
}

TEST(SignalSet, InitializerListConstructor)
{
  SignalSet set({SIGUSR1, SIGUSR2, SIGTERM});

  EXPECT_TRUE(set.contains(SIGUSR1));
  EXPECT_TRUE(set.contains(SIGUSR2));
  EXPECT_TRUE(set.contains(SIGTERM));
  EXPECT_FALSE(set.contains(SIGINT));
}

TEST(SignalSet, AddAndRemove)
{
  SignalSet set;

  set.add(SIGUSR1);
  EXPECT_TRUE(set.contains(SIGUSR1));

  set.add(SIGUSR2);
  EXPECT_TRUE(set.contains(SIGUSR1));
  EXPECT_TRUE(set.contains(SIGUSR2));

  set.remove(SIGUSR1);
  EXPECT_FALSE(set.contains(SIGUSR1));
  EXPECT_TRUE(set.contains(SIGUSR2));
}

TEST(SignalSet, FluentInterface)
{
  SignalSet set;
  set.add(SIGUSR1).add(SIGUSR2).remove(SIGUSR1);

  EXPECT_FALSE(set.contains(SIGUSR1));
  EXPECT_TRUE(set.contains(SIGUSR2));
}

TEST(SignalSet, ClearAndFill)
{
  SignalSet set({SIGUSR1, SIGUSR2});

  set.clear();
  EXPECT_FALSE(set.contains(SIGUSR1));
  EXPECT_FALSE(set.contains(SIGUSR2));

  set.fill();
  EXPECT_TRUE(set.contains(SIGUSR1));
  EXPECT_TRUE(set.contains(SIGUSR2));
  EXPECT_TRUE(set.contains(SIGINT));
}

TEST(SignalSet, StaticFactoryMethods)
{
  auto empty = SignalSet::empty();
  EXPECT_FALSE(empty.contains(SIGUSR1));

  auto full = SignalSet::full();
  EXPECT_TRUE(full.contains(SIGUSR1));
  EXPECT_TRUE(full.contains(SIGINT));
}

TEST(SignalSet, GetReturnsValidPointer)
{
  SignalSet set({SIGUSR1});

  sigset_t * ptr = set.get();
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(sigismember(ptr, SIGUSR1), 1);
}

// ============================================================================
// Signal Tests
// ============================================================================

TEST_F(SignalTestFixture, SignalInstallsHandler)
{
  {
    Signal sig(SIGUSR1, test_handler);

    // Send signal to self
    EXPECT_EQ(raise(SIGUSR1), 0);

    // Give time for signal delivery
    usleep(1000);

    EXPECT_EQ(g_signal_received, SIGUSR1);
  }
}

TEST_F(SignalTestFixture, SignalRestoresPreviousHandler)
{
  // First, install a known handler
  Signal outer(SIGUSR1, test_handler);

  {
    // Install a different handler in inner scope
    Signal inner(SIGUSR1, another_handler);

    raise(SIGUSR1);
    usleep(1000);
    EXPECT_EQ(g_signal_received, SIGUSR1 + 1000);

    g_signal_received = 0;
  }
  // Inner scope ended, original handler should be restored

  raise(SIGUSR1);
  usleep(1000);
  EXPECT_EQ(g_signal_received, SIGUSR1);
}

TEST_F(SignalTestFixture, SignalWithSigIgn)
{
  {
    Signal sig(SIGUSR1, SIG_IGN);

    // Signal should be ignored
    raise(SIGUSR1);
    usleep(1000);
    // No crash means SIG_IGN worked
  }
  SUCCEED();
}

TEST_F(SignalTestFixture, SignalGetters)
{
  Signal sig(SIGUSR1, test_handler, false);

  EXPECT_EQ(sig.signal_number(), SIGUSR1);
  EXPECT_FALSE(sig.restarts_calls());
  EXPECT_TRUE(sig.is_active());
}

TEST_F(SignalTestFixture, SignalRelease)
{
  {
    Signal sig(SIGUSR1, test_handler);
    sig.release();
    EXPECT_FALSE(sig.is_active());
  }
  // Handler should NOT be restored because release() was called

  // The handler should still be test_handler
  raise(SIGUSR1);
  usleep(1000);
  EXPECT_EQ(g_signal_received, SIGUSR1);

  // Clean up: restore default handler
  signal(SIGUSR1, SIG_DFL);
}

TEST_F(SignalTestFixture, SignalMoveConstruction)
{
  Signal sig1(SIGUSR1, test_handler);
  EXPECT_TRUE(sig1.is_active());

  Signal sig2(std::move(sig1));

  EXPECT_FALSE(sig1.is_active());
  EXPECT_TRUE(sig2.is_active());
  EXPECT_EQ(sig2.signal_number(), SIGUSR1);
}

TEST_F(SignalTestFixture, SignalMoveAssignment)
{
  Signal sig1(SIGUSR1, test_handler);
  Signal sig2(SIGUSR2, test_handler);

  sig2 = std::move(sig1);

  EXPECT_FALSE(sig1.is_active());
  EXPECT_TRUE(sig2.is_active());
  EXPECT_EQ(sig2.signal_number(), SIGUSR1);
}

TEST_F(SignalTestFixture, SignalCreateThrowsOnInvalidSignal)
{
  // Signal 0 is invalid for sigaction
  EXPECT_THROW(Signal::create(0, test_handler), SignalError);
}

TEST_F(SignalTestFixture, SignalTryCreateReturnsError)
{
  int error = 0;
  auto sig = Signal::try_create(0, test_handler, true, error);

  EXPECT_NE(error, 0);
  EXPECT_FALSE(sig.is_active());
}

TEST_F(SignalTestFixture, SignalCreateSucceeds)
{
  auto sig = Signal::create(SIGUSR1, test_handler);

  EXPECT_TRUE(sig.is_active());
  EXPECT_EQ(sig.signal_number(), SIGUSR1);

  raise(SIGUSR1);
  usleep(1000);
  EXPECT_EQ(g_signal_received, SIGUSR1);
}

TEST_F(SignalTestFixture, SignalPreviousHandler)
{
  Signal outer(SIGUSR1, test_handler);

  {
    Signal inner(SIGUSR1, another_handler);
    EXPECT_EQ(inner.previous_handler(), test_handler);
  }
}

TEST(SignalTypeTraits, NonCopyable)
{
  static_assert(!std::is_copy_constructible_v<Signal>);
  static_assert(!std::is_copy_assignable_v<Signal>);
}

TEST(SignalTypeTraits, Movable)
{
  static_assert(std::is_move_constructible_v<Signal>);
  static_assert(std::is_move_assignable_v<Signal>);
}

// ============================================================================
// SignalBlocker Tests
// ============================================================================

TEST_F(SignalTestFixture, SignalBlockerBlocksSignal)
{
  Signal sig(SIGUSR1, test_handler);

  {
    SignalBlocker blocker(SIGUSR1);

    // Send signal - it should be blocked
    raise(SIGUSR1);
    usleep(1000);

    // Signal should not have been delivered yet
    EXPECT_EQ(g_signal_received, 0);
  }
  // Blocker destroyed, signal should now be delivered
  usleep(1000);
  EXPECT_EQ(g_signal_received, SIGUSR1);
}

TEST_F(SignalTestFixture, SignalBlockerWithInitializerList)
{
  Signal sig1(SIGUSR1, test_handler);
  Signal sig2(SIGUSR2, test_handler);

  {
    SignalBlocker blocker({SIGUSR1, SIGUSR2});

    raise(SIGUSR1);
    raise(SIGUSR2);
    usleep(1000);

    EXPECT_EQ(g_signal_count.load(), 0);
  }

  usleep(1000);
  // Both signals should now be delivered
  EXPECT_GE(g_signal_count.load(), 1);  // At least one delivered
}

TEST_F(SignalTestFixture, SignalBlockerWithSignalSet)
{
  Signal sig(SIGUSR1, test_handler);

  SignalSet set;
  set.add(SIGUSR1);

  {
    SignalBlocker blocker(set);

    raise(SIGUSR1);
    usleep(1000);

    EXPECT_EQ(g_signal_received, 0);
  }

  usleep(1000);
  EXPECT_EQ(g_signal_received, SIGUSR1);
}

TEST_F(SignalTestFixture, SignalBlockerRelease)
{
  Signal sig(SIGUSR1, test_handler);

  {
    SignalBlocker blocker(SIGUSR1);
    blocker.release();

    raise(SIGUSR1);
    usleep(1000);

    // Signal still blocked (release only affects destructor)
    EXPECT_EQ(g_signal_received, 0);
  }
  // After destruction, mask should NOT be restored because release() was called
  // The signal might still be blocked

  // Manually unblock to clean up
  SignalSet empty;
  sigprocmask(SIG_SETMASK, empty.get(), nullptr);
}

TEST_F(SignalTestFixture, SignalBlockerMoveConstruction)
{
  Signal sig(SIGUSR1, test_handler);

  SignalBlocker blocker1(SIGUSR1);
  SignalBlocker blocker2(std::move(blocker1));

  // blocker1 should no longer be active
  raise(SIGUSR1);
  usleep(1000);
  EXPECT_EQ(g_signal_received, 0);  // Still blocked by blocker2
}

TEST(SignalBlockerTypeTraits, NonCopyable)
{
  static_assert(!std::is_copy_constructible_v<SignalBlocker>);
  static_assert(!std::is_copy_assignable_v<SignalBlocker>);
}

TEST(SignalBlockerTypeTraits, Movable)
{
  static_assert(std::is_move_constructible_v<SignalBlocker>);
  static_assert(std::is_move_assignable_v<SignalBlocker>);
}

// ============================================================================
// SignalError Tests
// ============================================================================

TEST(SignalError, ContainsSignalInfo)
{
  SignalError err("Test error", SIGUSR1, EINVAL);

  EXPECT_EQ(err.signal_number(), SIGUSR1);
  EXPECT_EQ(err.error_code(), EINVAL);
  EXPECT_STREQ(err.what(), "Test error");
}

// ============================================================================
// Utility Function Tests
// ============================================================================

TEST(SignalUtilities, SignalName)
{
  EXPECT_EQ(signal_name(SIGINT), "SIGINT");
  EXPECT_EQ(signal_name(SIGTERM), "SIGTERM");
  EXPECT_EQ(signal_name(SIGUSR1), "SIGUSR1");
  EXPECT_EQ(signal_name(SIGUSR2), "SIGUSR2");
  EXPECT_EQ(signal_name(SIGKILL), "SIGKILL");
}

TEST(SignalUtilities, SendSignalToSelf)
{
  g_signal_received = 0;
  Signal sig(SIGUSR1, test_handler);

  EXPECT_TRUE(send_signal_to_self(SIGUSR1));
  usleep(1000);
  EXPECT_EQ(g_signal_received, SIGUSR1);
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

TEST_F(SignalTestFixture, BackwardCompatibleConstructor)
{
  // Original constructor signature used const int & and const bool &
  const int signo = SIGUSR1;
  const bool restart = true;

  Signal sig(signo, test_handler, restart);

  EXPECT_EQ(sig.signal_number(), SIGUSR1);
  EXPECT_TRUE(sig.restarts_calls());

  raise(SIGUSR1);
  usleep(1000);
  EXPECT_EQ(g_signal_received, SIGUSR1);
}

TEST_F(SignalTestFixture, BackwardCompatibleDefaultRestartCalls)
{
  // Default for restart_calls should be true
  Signal sig(SIGUSR1, test_handler);

  EXPECT_TRUE(sig.restarts_calls());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(SignalTestFixture, NestedSignalHandlers)
{
  std::vector<int> received_order;

  auto handler1 = [](int) { g_signal_received = 1; };
  auto handler2 = [](int) { g_signal_received = 2; };
  auto handler3 = [](int) { g_signal_received = 3; };

  {
    Signal sig1(SIGUSR1, handler1);
    raise(SIGUSR1);
    usleep(1000);
    EXPECT_EQ(g_signal_received, 1);

    {
      Signal sig2(SIGUSR1, handler2);
      raise(SIGUSR1);
      usleep(1000);
      EXPECT_EQ(g_signal_received, 2);

      {
        Signal sig3(SIGUSR1, handler3);
        raise(SIGUSR1);
        usleep(1000);
        EXPECT_EQ(g_signal_received, 3);
      }

      raise(SIGUSR1);
      usleep(1000);
      EXPECT_EQ(g_signal_received, 2);  // Back to handler2
    }

    raise(SIGUSR1);
    usleep(1000);
    EXPECT_EQ(g_signal_received, 1);  // Back to handler1
  }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
