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
 * @file net_float_flow_test.cc
 * @brief Tests that a float flow, deprecated, still works
 *
 * Net_Arc warns when it is instantiated with float as the type of the
 * flow. The warning comes from the header, so that a user who moves to
 * double can silence it by including the headers between the pragmas
 * below, as this test does: a pragma around the use does not.
 */

#include <gtest/gtest.h>

#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <tpl_maxflow.H>
#include <tpl_net.H>
#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(pop)
#else
#  pragma GCC diagnostic pop
#endif

using namespace Aleph;

namespace
{
  using Float_Net = Net_Graph<Net_Node<int>, Net_Arc<int, float>>;
} // anonymous namespace

// A float flow is deprecated, not rejected: on a network of capacities
// exact in binary, the maximum flows take it as before
TEST(FloatFlowTest, AFloatFlowStillWorksWhileDeprecated)
{
  for (int algo = 0; algo < 3; ++algo)
    {
      Float_Net net;
      auto s = net.insert_node(0);
      auto a = net.insert_node(1);
      auto t = net.insert_node(2);
      net.insert_arc(s, a, 3.0f);
      net.insert_arc(a, t, 2.0f);
      net.insert_arc(s, t, 1.5f);
      const float value = algo == 0 ? ford_fulkerson_maximum_flow(net)
                          : algo == 1 ? edmonds_karp_maximum_flow(net)
                                      : dinic_maximum_flow(net);
      EXPECT_EQ(value, 3.5f) << "algorithm " << algo;
      EXPECT_TRUE(net.check_network()) << "algorithm " << algo;
    }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
