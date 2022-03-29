

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

# include <iostream>

# include <quadtree.H>

using namespace std;

using Tree = QuadTree;

void write_tree(Tree::Node * root, size_t indent)
{
  for (size_t i = 0; i < indent; ++i)
    cout << "-";

  if (not root->is_leaf())
    {
      cout << endl;
      write_tree(NW_CHILD(root), indent + 2);
      write_tree(NE_CHILD(root), indent + 2);
      write_tree(SW_CHILD(root), indent + 2);
      write_tree(SE_CHILD(root), indent + 2);
      return;
    }

  root->for_each_point([](const Point & p) { cout << p.to_string(); });
  cout << endl;
}

int main()
{
  Tree tree(0, 100, 0, 100, 4);

  tree.insert(5, 5);

  tree.insert(95, 5);

  tree.insert(5, 95);

  tree.insert(95, 95);


  Tree::Node * root = tree.get_root();

  assert(root->is_leaf());
  assert(root->get_num_points() == 4);

  write_tree(root, 2);

  tree.insert(Point(5, 45));

  assert(not root->is_leaf());
  assert(root->get_num_points() == 5);

  Tree::Node * root_nw_child = NW_CHILD(root);
  assert(root_nw_child != nullptr);
  assert(root_nw_child->is_leaf());
  assert(root_nw_child->get_num_points() == 2);

  Tree::Node * root_ne_child = NE_CHILD(root);
  assert(root_ne_child != nullptr);
  assert(root_ne_child->is_leaf());
  assert(root_ne_child->get_num_points() == 1);

  Tree::Node * root_sw_child = SW_CHILD(root);
  assert(root_sw_child != nullptr);
  assert(root_sw_child->is_leaf());
  assert(root_sw_child->get_num_points() == 1);

  Tree::Node * root_se_child = SE_CHILD(root);
  assert(root_se_child != nullptr);
  assert(root_se_child->is_leaf());
  assert(root_se_child->get_num_points() == 1);

  cout << endl;
  write_tree(root, 2);

  tree.insert(Point(45, 5));

  tree.insert(Point(45, 45));

  tree.insert(Point(20, 20));

  assert(root->get_num_points() == 8);

  assert(root_nw_child->get_num_points() == 5);
  assert(root_ne_child->get_num_points() == 1);
  assert(root_sw_child->get_num_points() == 1);
  assert(root_se_child->get_num_points() == 1);

  assert(not root->is_leaf());
  assert(not root_nw_child->is_leaf());
  assert(root_ne_child->is_leaf());
  assert(root_sw_child->is_leaf());
  assert(root_se_child->is_leaf());

  Tree::Node * root_nw_child_nw_child = NW_CHILD(root_nw_child);
  assert(root_nw_child_nw_child != nullptr);
  assert(root_nw_child_nw_child->is_leaf());
  assert(root_nw_child_nw_child->get_num_points() == 2);

  Tree::Node * root_nw_child_ne_child = NE_CHILD(root_nw_child);
  assert(root_nw_child_ne_child != nullptr);
  assert(root_nw_child_ne_child->is_leaf());
  assert(root_nw_child_ne_child->get_num_points() == 1);

  Tree::Node * root_nw_child_sw_child = SW_CHILD(root_nw_child);
  assert(root_nw_child_sw_child != nullptr);
  assert(root_nw_child_sw_child->is_leaf());
  assert(root_nw_child_sw_child->get_num_points() == 1);

  Tree::Node * root_nw_child_se_child = SE_CHILD(root_nw_child);
  assert(root_nw_child_se_child != nullptr);
  assert(root_nw_child_se_child->is_leaf());
  assert(root_nw_child_se_child->get_num_points() == 1);

  cout << endl;
  write_tree(root, 2);

  tree.insert(Point(30, 30));
  tree.insert(Point(45, 30));
  tree.insert(Point(30, 45));
  tree.insert(Point(30, 40));

  assert(not root->is_leaf());
  assert(not root_nw_child->is_leaf());
  assert(root_nw_child_nw_child->is_leaf());
  assert(root_nw_child_ne_child->is_leaf());
  assert(root_nw_child_sw_child->is_leaf());
  assert(not root_nw_child_se_child->is_leaf());
  assert(root_ne_child->is_leaf());
  assert(root_sw_child->is_leaf());
  assert(root_se_child->is_leaf());

  assert(root->get_num_points() == 12);
  assert(root_nw_child->get_num_points() == 9);
  assert(root_nw_child_se_child->get_num_points() == 5);

  Tree::Node * root_nw_child_se_child_nw_child =
    NW_CHILD(root_nw_child_se_child);
  assert(root_nw_child_se_child_nw_child != nullptr);
  assert(root_nw_child_se_child_nw_child->is_leaf());
  assert(root_nw_child_se_child_nw_child->get_num_points() == 1);

  Tree::Node * root_nw_child_se_child_ne_child =
    NE_CHILD(root_nw_child_se_child);
  assert(root_nw_child_se_child_ne_child != nullptr);
  assert(root_nw_child_se_child_ne_child->is_leaf());
  assert(root_nw_child_se_child_ne_child->get_num_points() == 1);

  Tree::Node * root_nw_child_se_child_sw_child =
    SW_CHILD(root_nw_child_se_child);
  assert(root_nw_child_se_child_sw_child != nullptr);
  assert(root_nw_child_se_child_sw_child->is_leaf());
  assert(root_nw_child_se_child_sw_child->get_num_points() == 2);

  Tree::Node * root_nw_child_se_child_se_child =
    SE_CHILD(root_nw_child_se_child);
  assert(root_nw_child_se_child_se_child != nullptr);
  assert(root_nw_child_se_child_se_child->is_leaf());
  assert(root_nw_child_se_child_se_child->get_num_points() == 1);

  cout << endl;
  write_tree(root, 2);

  tree.remove(Point(20, 20));

  assert(not root->is_leaf());
  assert(not root_nw_child->is_leaf());
  assert(root_nw_child_nw_child->is_leaf());
  assert(root_nw_child_ne_child->is_leaf());
  assert(root_nw_child_sw_child->is_leaf());
  assert(not root_nw_child_se_child->is_leaf());
  assert(root_ne_child->is_leaf());
  assert(root_sw_child->is_leaf());
  assert(root_se_child->is_leaf());

  assert(root->get_num_points() == 11);
  assert(root_nw_child->get_num_points() == 8);
  assert(root_nw_child_se_child->get_num_points() == 5);
  assert(root_nw_child_se_child_nw_child->is_leaf());
  assert(root_nw_child_se_child_nw_child->get_num_points() == 1);
  assert(root_nw_child_se_child_ne_child->is_leaf());
  assert(root_nw_child_se_child_ne_child->get_num_points() == 1);
  assert(root_nw_child_se_child_sw_child->is_leaf());
  assert(root_nw_child_se_child_sw_child->get_num_points() == 2);
  assert(root_nw_child_se_child_se_child->is_leaf());
  assert(root_nw_child_se_child_se_child->get_num_points() == 1);

  cout << endl;
  write_tree(root, 2);

  tree.remove(Point(45, 45));

  assert(not root->is_leaf());
  assert(not root_nw_child->is_leaf());
  assert(root_nw_child_nw_child->is_leaf());
  assert(root_nw_child_ne_child->is_leaf());
  assert(root_nw_child_sw_child->is_leaf());
  assert(root_nw_child_se_child->is_leaf());
  assert(root_ne_child->is_leaf());
  assert(root_sw_child->is_leaf());
  assert(root_se_child->is_leaf());

  assert(root->get_num_points() == 10);
  assert(root_nw_child->get_num_points() == 7);
  assert(root_nw_child_se_child->get_num_points() == 4);

  cout << endl;
  write_tree(root, 2);

  cout << "\nQuadtree ok!\n";

  return 0;
}

