/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

