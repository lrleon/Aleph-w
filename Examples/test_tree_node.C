/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

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

#include <iostream>
#include <string>
#include <stdexcept>
#include <ah-errors.H>
#include <tpl_tree_node.H>
#include <tpl_dynArray.H>

#include <cassert>
using namespace std;
using namespace Aleph;

using Node = Tree_Node<string>;

static void print_node(Node *node, int level, int index)
{
  cout << "  [" << index << "] " << string(2 * level, ' ') << node->get_data() << '\n';
}

static Node *append_child(Node *parent, const string &name)
{
  auto *child = new Node(name);
  parent->insert_rightmost_child(child);
  return child;
}

static void draw_children(Node *node, const string &prefix)
{
  for (Node *child = node->get_left_child(); child != nullptr;
       child = child->get_right_sibling())
    {
      const bool is_last = child->get_right_sibling() == nullptr;
      cout << prefix << (is_last ? "`-- " : "|-- ") << child->get_data() << '\n';
      draw_children(child, prefix + (is_last ? "    " : "|   "));
    }
}

static void draw_tree(Node *root)
{
  cout << root->get_data() << '\n';
  draw_children(root, "");
}

/** Print every node with its Dewey number and depth indentation.

    @param[in] node current node.
    @param[in] dewey Dewey number of `node`.
    @param[in] level depth of `node`.
 */
static void print_dewey_view(Node *node, const string &dewey, size_t level)
{
  cout << string(2 * level, ' ') << dewey << "  " << node->get_data() << '\n';

  size_t child_index = 0;
  for (Node *child = node->get_left_child(); child != nullptr;
       child = child->get_right_sibling(), ++child_index)
    print_dewey_view(child, dewey + "." + to_string(child_index), level + 1);
}

int main()
{
  auto *root = new Node("Aleph-w");

  Node *containers = append_child(root, "Containers");
  Node *trees = append_child(root, "Trees");
  Node *graphs = append_child(root, "Graphs");

  append_child(containers, "Array");
  append_child(containers, "DynList");
  append_child(containers, "Hash tables");

  Node *binary_trees = append_child(trees, "Binary trees");
  append_child(trees, "General trees");
  append_child(binary_trees, "AVL");
  append_child(binary_trees, "Red-black");
  append_child(binary_trees, "Treap");

  append_child(graphs, "Traversal");
  append_child(graphs, "Shortest paths");

  //      assert(check_tree(root));

  cout << "Aleph-w data-structure tree\n"
       << "===========================\n";
  draw_tree(root);

  cout << "\nDewey-numbered view\n"
       << "--------------------\n";
  print_dewey_view(root, "0", 0);

  cout << "\nPreorder traversal\n";
  tree_preorder_traversal(root, &print_node);

  cout << "\nPostorder traversal\n";
  tree_postorder_traversal(root, &print_node);

  int treap_path[] = {0, 1, 0, 2, -1};
  Node *treap = deway_search(root, treap_path, 5);
  assert(treap != nullptr);
  cout << "\nDewey path 0.1.0.2 points to: " << treap->get_data() << '\n';

  Node *copy = clone_tree(root);
  assert(are_tree_equal(root, copy));
  cout << "Clone verification: passed\n";

  destroy_tree(root);
  destroy_tree(copy);
}
