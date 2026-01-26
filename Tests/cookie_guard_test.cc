
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
 * @file cookie_guard_test.cc
 * @brief Tests for Cookie Guard
 */
#include <gtest/gtest.h>
#include <cookie_guard.H>
#include <tpl_graph.H>

using namespace Aleph;

using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = Graph::Node;
using Arc = Graph::Arc;

class CookieGuardTest : public testing::Test
{
protected:
  Graph g;
  Node *n1, *n2, *n3;
  Arc *a1, *a2;

  void SetUp() override
  {
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);
    a1 = g.insert_arc(n1, n2, 10);
    a2 = g.insert_arc(n2, n3, 20);
  }

  bool all_node_cookies_null()
  {
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      if (NODE_COOKIE(it.get_curr()) != nullptr)
        return false;
    return true;
  }

  bool all_arc_cookies_null()
  {
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      if (ARC_COOKIE(it.get_curr()) != nullptr)
        return false;
    return true;
  }
};

// =============================================================================
// Cookie_Guard Tests
// =============================================================================

TEST_F(CookieGuardTest, BasicCleanup)
{
  // Set some cookies
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);
  NODE_COOKIE(n2) = reinterpret_cast<void*>(0x2);
  ARC_COOKIE(a1) = reinterpret_cast<void*>(0x3);

  EXPECT_FALSE(all_node_cookies_null());
  EXPECT_FALSE(all_arc_cookies_null());

  {
    Cookie_Guard<Graph> guard(g);
    // Cookies should still be set inside the block
    EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));
  }

  // After guard destructor, all cookies should be null
  EXPECT_TRUE(all_node_cookies_null());
  EXPECT_TRUE(all_arc_cookies_null());
}

TEST_F(CookieGuardTest, SelectiveCleanup)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);
  ARC_COOKIE(a1) = reinterpret_cast<void*>(0x2);

  {
    Cookie_Guard<Graph> guard(g, true, false);  // only clear nodes
  }

  EXPECT_TRUE(all_node_cookies_null());
  EXPECT_FALSE(all_arc_cookies_null());  // arcs should still have cookies
  EXPECT_EQ(ARC_COOKIE(a1), reinterpret_cast<void*>(0x2));
}

TEST_F(CookieGuardTest, Release)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  {
    Cookie_Guard<Graph> guard(g);
    guard.release();  // Don't clean on destruction
  }

  EXPECT_FALSE(all_node_cookies_null());
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));
}

TEST_F(CookieGuardTest, ClearNow)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  Cookie_Guard<Graph> guard(g);
  EXPECT_FALSE(all_node_cookies_null());
  
  guard.clear_now();
  EXPECT_TRUE(all_node_cookies_null());

  // Set again - guard should not clear again (already inactive)
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x2);
  // guard goes out of scope but won't clear
}

TEST_F(CookieGuardTest, CustomDeleter)
{
  static int delete_count = 0;
  delete_count = 0;

  // Allocate some data
  NODE_COOKIE(n1) = new int(100);
  NODE_COOKIE(n2) = new int(200);

  {
    Cookie_Guard<Graph> guard(g, 
      [](Node* p) { 
        delete static_cast<int*>(NODE_COOKIE(p)); 
        delete_count++;
      },
      nullptr);
  }

  EXPECT_EQ(delete_count, 3);  // n1, n2, n3 (n3 has nullptr but deleter still called)
  EXPECT_TRUE(all_node_cookies_null());
}

TEST_F(CookieGuardTest, ExceptionSafety)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);
  NODE_COOKIE(n2) = reinterpret_cast<void*>(0x2);

  try
    {
      Cookie_Guard<Graph> guard(g);
      throw std::runtime_error("test exception");
    }
  catch (...)
    {
      // Exception caught
    }

  // Despite exception, cookies should be cleaned
  EXPECT_TRUE(all_node_cookies_null());
}

TEST_F(CookieGuardTest, MoveConstruction)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  Cookie_Guard<Graph> guard1(g);
  Cookie_Guard<Graph> guard2(std::move(guard1));

  // guard1 is now inactive, guard2 is active
  // Only guard2's destructor should clear cookies
}

// =============================================================================
// Cookie_Saver Tests
// =============================================================================

TEST_F(CookieGuardTest, BasicSaveRestore)
{
  // Set original cookies
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);
  NODE_COOKIE(n2) = reinterpret_cast<void*>(0x2);
  ARC_COOKIE(a1) = reinterpret_cast<void*>(0x3);

  {
    Cookie_Saver<Graph> saver(g);

    // Modify cookies
    NODE_COOKIE(n1) = reinterpret_cast<void*>(0xA);
    NODE_COOKIE(n2) = reinterpret_cast<void*>(0xB);
    ARC_COOKIE(a1) = reinterpret_cast<void*>(0xC);

    // Verify modification
    EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0xA));
  }

  // After saver destructor, original cookies should be restored
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));
  EXPECT_EQ(NODE_COOKIE(n2), reinterpret_cast<void*>(0x2));
  EXPECT_EQ(ARC_COOKIE(a1), reinterpret_cast<void*>(0x3));
}

TEST_F(CookieGuardTest, SaveRestoreWithNullOriginal)
{
  // Start with null cookies
  EXPECT_TRUE(all_node_cookies_null());

  {
    Cookie_Saver<Graph> saver(g);

    // Use cookies
    NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);
  }

  // Should be restored to null
  EXPECT_TRUE(all_node_cookies_null());
}

TEST_F(CookieGuardTest, Discard)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  {
    Cookie_Saver<Graph> saver(g);
    NODE_COOKIE(n1) = reinterpret_cast<void*>(0xA);
    saver.discard();  // Don't restore
  }

  // New value should be kept
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0xA));
}

TEST_F(CookieGuardTest, RestoreNow)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  Cookie_Saver<Graph> saver(g);
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0xA);
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0xA));

  saver.restore_now();
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));

  // Modify again - saver should not restore again
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0xB);
  // saver goes out of scope but won't restore
}

TEST_F(CookieGuardTest, SaverWithCleanup)
{
  static int cleanup_count = 0;
  cleanup_count = 0;

  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  {
    Cookie_Saver<Graph> saver(g,
      [](Node* node) { 
        cleanup_count++;
        // Free the allocated memory if it's a valid heap pointer
        // (not the sentinel value 0x1 or nullptr)
        void* cookie = NODE_COOKIE(node);
        if (cookie != nullptr && cookie != reinterpret_cast<void*>(0x1))
          delete static_cast<int*>(cookie);
      },
      nullptr);

    // Allocate temporary data
    NODE_COOKIE(n1) = new int(100);
    NODE_COOKIE(n2) = new int(200);
  }

  // Cleanup should have been called
  EXPECT_EQ(cleanup_count, 3);
  // Original should be restored
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));
}

TEST_F(CookieGuardTest, SaverExceptionSafety)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  try
    {
      Cookie_Saver<Graph> saver(g);
      NODE_COOKIE(n1) = reinterpret_cast<void*>(0xA);
      throw std::runtime_error("test exception");
    }
  catch (...)
    {
      // Exception caught
    }

  // Despite exception, original cookies should be restored
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));
}

TEST_F(CookieGuardTest, GetSavedCookies)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);
  NODE_COOKIE(n2) = reinterpret_cast<void*>(0x2);

  Cookie_Saver<Graph> saver(g);

  auto & saved = saver.get_saved_node_cookies();
  EXPECT_EQ(saved.size(), 3);  // 3 nodes

  // Find n1's saved cookie
  auto it = std::find_if(saved.begin(), saved.end(),
    [this](const auto & p) { return p.first == n1; });
  ASSERT_NE(it, saved.end());
  EXPECT_EQ(it->second, reinterpret_cast<void*>(0x1));
}

// =============================================================================
// Convenience Function Tests
// =============================================================================

TEST_F(CookieGuardTest, WithCleanCookies)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  int result = with_clean_cookies(g, [&]() {
    EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));
    NODE_COOKIE(n1) = reinterpret_cast<void*>(0xA);
    return 42;
  });

  EXPECT_EQ(result, 42);
  EXPECT_TRUE(all_node_cookies_null());
}

TEST_F(CookieGuardTest, WithSavedCookies)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  int result = with_saved_cookies(g, [&]() {
    NODE_COOKIE(n1) = reinterpret_cast<void*>(0xA);
    return 42;
  });

  EXPECT_EQ(result, 42);
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));  // restored
}

TEST_F(CookieGuardTest, NestedSavers)
{
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1);

  {
    Cookie_Saver<Graph> outer(g);
    NODE_COOKIE(n1) = reinterpret_cast<void*>(0xA);

    {
      Cookie_Saver<Graph> inner(g);
      NODE_COOKIE(n1) = reinterpret_cast<void*>(0xB);
      EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0xB));
    }

    // Inner restored to 0xA
    EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0xA));
  }

  // Outer restored to 0x1
  EXPECT_EQ(NODE_COOKIE(n1), reinterpret_cast<void*>(0x1));
}

// ============================================================================
// Scope_Guard Tests
// ============================================================================

TEST_F(CookieGuardTest, ScopeGuardBasicCleanup)
{
  bool cleanup_called = false;
  
  {
    Scope_Guard guard(g, [&cleanup_called](const Graph &) {
      cleanup_called = true;
    });
    EXPECT_TRUE(guard.is_active());
    EXPECT_FALSE(cleanup_called);
  }
  
  EXPECT_TRUE(cleanup_called);
}

TEST_F(CookieGuardTest, ScopeGuardRelease)
{
  bool cleanup_called = false;
  
  {
    Scope_Guard guard(g, [&cleanup_called](const Graph &) {
      cleanup_called = true;
    });
    guard.release();
    EXPECT_FALSE(guard.is_active());
  }
  
  EXPECT_FALSE(cleanup_called);
}

TEST_F(CookieGuardTest, ScopeGuardCleanupNow)
{
  int cleanup_count = 0;
  
  {
    Scope_Guard guard(g, [&cleanup_count](const Graph &) {
      cleanup_count++;
    });
    
    guard.cleanup_now();
    EXPECT_EQ(cleanup_count, 1);
    EXPECT_FALSE(guard.is_active());
    
    // Second call should not execute cleanup
    guard.cleanup_now();
    EXPECT_EQ(cleanup_count, 1);
  }
  
  // Destructor should not call cleanup again
  EXPECT_EQ(cleanup_count, 1);
}

TEST_F(CookieGuardTest, ScopeGuardMoveConstruction)
{
  bool cleanup_called = false;
  
  {
    Scope_Guard guard1(g, [&cleanup_called](const Graph &) {
      cleanup_called = true;
    });
    
    Scope_Guard guard2(std::move(guard1));
    EXPECT_FALSE(guard1.is_active());
    EXPECT_TRUE(guard2.is_active());
  }
  
  // Only guard2's destructor should have called cleanup
  EXPECT_TRUE(cleanup_called);
}

TEST_F(CookieGuardTest, ScopeGuardExceptionSafety)
{
  bool cleanup_called = false;
  
  try
  {
    Scope_Guard guard(g, [&cleanup_called](const Graph &) {
      cleanup_called = true;
    });
    throw std::runtime_error("test");
  }
  catch (...)
  {
    // Exception caught
  }
  
  EXPECT_TRUE(cleanup_called);
}

TEST_F(CookieGuardTest, ScopeGuardWithRealCleanup)
{
  // Simulate real usage: set cookies, use guard to clean them
  NODE_COOKIE(n1) = reinterpret_cast<void*>(0x1234);
  NODE_COOKIE(n2) = reinterpret_cast<void*>(0x5678);
  
  {
    Scope_Guard guard(g, [](const Graph & graph) {
      for (Node_Iterator<Graph> it(graph); it.has_curr(); it.next_ne())
        NODE_COOKIE(it.get_curr()) = nullptr;
    });
    
    // Cookies are set during scope
    EXPECT_NE(NODE_COOKIE(n1), nullptr);
    EXPECT_NE(NODE_COOKIE(n2), nullptr);
  }
  
  // After scope, cookies should be cleaned
  EXPECT_EQ(NODE_COOKIE(n1), nullptr);
  EXPECT_EQ(NODE_COOKIE(n2), nullptr);
}

