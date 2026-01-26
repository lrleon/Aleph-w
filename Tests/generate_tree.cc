
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
 * @file generate_tree.cc
 * @brief Tests for Generate Tree
 */
# include <gtest/gtest.h>

# include <generate_tree.H>
# include "tree-node-common.H"

using namespace std;
using namespace testing;
using namespace Aleph;

TEST_F(Simple_Tree, Generate_ntreepic)
{
  const string result =
    "Root \"0\" \n"				\
    "Node 0 \"1\" \n"				\
    "Node 0.0 \"6\" \n"				\
    "Node 0.0 \"7\" \n"				\
    "Node 0.0 \"8\" \n"				\
    "Node 0.0 \"9\" \n"				\
    "Node 0.0 \"10\" \n"			\
    "Node 0 \"2\" \n"				\
    "Node 0.1 \"11\" \n"			\
    "Node 0.1 \"12\" \n"			\
    "Node 0.1 \"13\" \n"			\
    "Node 0.1 \"14\" \n"			\
    "Node 0.1 \"15\" \n"			\
    "Node 0 \"3\" \n"				\
    "Node 0.2 \"16\" \n"			\
    "Node 0.2 \"17\" \n"			\
    "Node 0.2 \"18\" \n"			\
    "Node 0.2 \"19\" \n"			\
    "Node 0.2 \"20\" \n"			\
    "Node 0 \"4\" \n"				\
    "Node 0.3 \"21\" \n"			\
    "Node 0.3 \"22\" \n"			\
    "Node 0.3 \"23\" \n"			\
    "Node 0.3 \"24\" \n"			\
    "Node 0.3 \"25\" \n"			\
    "Node 0 \"5\" \n"				\
    "Node 0.4 \"26\" \n"			\
    "Node 0.4 \"27\" \n"			\
    "Node 0.4 \"28\" \n"			\
    "Node 0.4 \"29\" \n"			\
    "Node 0.4 \"30\" \n";

  ostringstream ss;
  generate_tree(root, ss);

  ASSERT_EQ(ss.str(), result);
}

