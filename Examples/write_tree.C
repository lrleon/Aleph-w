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
 * @file write_tree.C
 * @brief Generates tree structure files for visualization
 * 
 * This program creates .Tree files containing tree structures in preorder
 * traversal format, suitable for visualization tools like btreepic.
 * 
 * It supports multiple BST implementations:
 * - AVL Tree (balanced, height-based)
 * - Red-Black Tree (balanced, color-based)
 * - Splay Tree (self-adjusting)
 * - Treap (randomized heap-ordered)
 * - Rand Tree (randomized BST)
 * - Binary Tree (unbalanced BST)
 * 
 * Output files are named: <tree_type>.Tree (e.g., avl.Tree, rb.Tree)
 * 
 * Usage: write_tree [-n <count>] [-s <seed>] [-t <type>] [-o <prefix>]
 *        -t all     : Generate all tree types (default)
 *        -t avl     : AVL tree only
 *        -t rb      : Red-Black tree only
 *        -t splay   : Splay tree only
 *        -t treap   : Treap only
 *        -t rand    : Randomized tree only
 *        -t bin     : Binary tree only
 */

# include <iostream>
# include <fstream>
# include <cstdlib>
# include <ctime>
# include <vector>
# include <set>
# include <tclap/CmdLine.h>
# include <aleph.H>
# include <tpl_avl.H>
# include <tpl_rb_tree.H>
# include <tpl_splay_tree.H>
# include <tpl_treap.H>
# include <tpl_rand_tree.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Helper to generate unique random values
// =============================================================================

vector<int> generate_random_values(size_t n, unsigned int seed)
{
  srand(seed);
  vector<int> values;
  values.reserve(n);
  
  set<int> used;  // Standard STL set for uniqueness
  
  while (values.size() < n)
    {
      int val = 1 + rand() % (n * 10);
      if (used.find(val) == used.end())
        {
          used.insert(val);
          values.push_back(val);
        }
    }
  
  return values;
}

// =============================================================================
// Output stream pointer for print functions
// =============================================================================

static ofstream* current_output = nullptr;

// =============================================================================
// AVL Tree Writer
// =============================================================================

static void avl_print_key(Avl_Tree<int>::Node* p, int, int)
{
  *current_output << p->get_key() << " ";
}

void write_avl(const vector<int>& values, const string& prefix)
{
  using Node = Avl_Tree<int>::Node;
  
  ofstream output(prefix + "avl.Tree", ios::out);
  if (!output.is_open())
    {
      cerr << "Error: cannot open " << prefix << "avl.Tree" << endl;
      return;
    }
  
  Avl_Tree<int> tree;
  
  for (int val : values)
    {
      if (tree.search(val) == nullptr)
        {
          Node* node = new Node(val);
          (void) tree.insert(node);  // Ignore nodiscard
        }
    }
  
  current_output = &output;
  int count = preOrderRec(tree.getRoot(), avl_print_key);
  output << endl;
  
  cout << "  Height: " << computeHeightRec(tree.getRoot()) 
       << ", Nodes: " << count << endl;
  cout << "  Written: " << prefix << "avl.Tree" << endl;
  
  destroyRec(tree.getRoot());
}

// =============================================================================
// Red-Black Tree Writer (with color info)
// =============================================================================

static void rb_print_key(Rb_Tree<int>::Node* p, int, int)
{
  *current_output << p->get_key() << " ";
}

static int rb_pos = 0;
static void rb_write_colors(Rb_Tree<int>::Node* p, int, int)
{
  if (COLOR(p) == RED)
    *current_output << rb_pos << " ";
  rb_pos++;
}

void write_rb(const vector<int>& values, const string& prefix)
{
  using Node = Rb_Tree<int>::Node;
  
  ofstream output(prefix + "rb.Tree", ios::out);
  if (!output.is_open())
    {
      cerr << "Error: cannot open " << prefix << "rb.Tree" << endl;
      return;
    }
  
  Rb_Tree<int> tree;
  
  for (int val : values)
    {
      if (tree.search(val) == nullptr)
        {
          Node* node = new Node(val);
          tree.insert(node);
        }
    }
  
  current_output = &output;
  
  // Write keys in preorder
  int count = preOrderRec(tree.getRoot(), rb_print_key);
  
  // Write red node positions for visualization
  output << endl << "START-SHADOW ";
  rb_pos = 0;
  inOrderRec(tree.getRoot(), rb_write_colors);
  output << endl;
  
  cout << "  Height: " << computeHeightRec(tree.getRoot()) 
       << ", Nodes: " << count << endl;
  cout << "  Written: " << prefix << "rb.Tree (with color info)" << endl;
  
  destroyRec(tree.getRoot());
}

// =============================================================================
// Splay Tree Writer
// =============================================================================

static void splay_print_key(Splay_Tree<int>::Node* p, int, int)
{
  *current_output << p->get_key() << " ";
}

void write_splay(const vector<int>& values, const string& prefix)
{
  using Node = Splay_Tree<int>::Node;
  
  ofstream output(prefix + "splay.Tree", ios::out);
  if (!output.is_open())
    {
      cerr << "Error: cannot open " << prefix << "splay.Tree" << endl;
      return;
    }
  
  Splay_Tree<int> tree;
  
  for (int val : values)
    {
      if (tree.search(val) == nullptr)
        {
          Node* node = new Node(val);
          tree.insert(node);
        }
    }
  
  current_output = &output;
  int count = preOrderRec(tree.getRoot(), splay_print_key);
  output << endl;
  
  cout << "  Height: " << computeHeightRec(tree.getRoot()) 
       << ", Nodes: " << count << endl;
  cout << "  Written: " << prefix << "splay.Tree" << endl;
  
  destroyRec(tree.getRoot());
}

// =============================================================================
// Treap Writer
// =============================================================================

static void treap_print_key(Treap<int>::Node* p, int, int)
{
  *current_output << p->get_key() << " ";
}

void write_treap(const vector<int>& values, const string& prefix)
{
  using Node = Treap<int>::Node;
  
  ofstream output(prefix + "treap.Tree", ios::out);
  if (!output.is_open())
    {
      cerr << "Error: cannot open " << prefix << "treap.Tree" << endl;
      return;
    }
  
  Treap<int> tree;
  
  for (int val : values)
    {
      if (tree.search(val) == nullptr)
        {
          Node* node = new Node(val);
          tree.insert(node);
        }
    }
  
  current_output = &output;
  int count = preOrderRec(tree.getRoot(), treap_print_key);
  output << endl;
  
  cout << "  Height: " << computeHeightRec(tree.getRoot()) 
       << ", Nodes: " << count << endl;
  cout << "  Written: " << prefix << "treap.Tree" << endl;
  
  destroyRec(tree.getRoot());
}

// =============================================================================
// Rand Tree Writer
// =============================================================================

static void rand_print_key(Rand_Tree<int>::Node* p, int, int)
{
  *current_output << p->get_key() << " ";
}

void write_rand(const vector<int>& values, const string& prefix)
{
  using Node = Rand_Tree<int>::Node;
  
  ofstream output(prefix + "rand.Tree", ios::out);
  if (!output.is_open())
    {
      cerr << "Error: cannot open " << prefix << "rand.Tree" << endl;
      return;
    }
  
  Rand_Tree<int> tree;
  
  for (int val : values)
    {
      if (tree.search(val) == nullptr)
        {
          Node* node = new Node(val);
          tree.insert(node);
        }
    }
  
  current_output = &output;
  preOrderRec(tree.getRoot(), rand_print_key);
  output << endl;
  
  cout << "  Height: " << computeHeightRec(tree.getRoot()) 
       << ", Nodes: " << tree.size() << endl;
  cout << "  Written: " << prefix << "rand.Tree" << endl;
  
  destroyRec(tree.getRoot());
}

// =============================================================================
// Binary Tree Writer (unbalanced)
// =============================================================================

static void bin_print_key(BinTree<int>::Node* p, int, int)
{
  *current_output << p->get_key() << " ";
}

void write_bin(const vector<int>& values, const string& prefix)
{
  using Node = BinTree<int>::Node;
  
  ofstream output(prefix + "bin.Tree", ios::out);
  if (!output.is_open())
    {
      cerr << "Error: cannot open " << prefix << "bin.Tree" << endl;
      return;
    }
  
  BinTree<int> tree;
  
  for (int val : values)
    {
      if (tree.search(val) == nullptr)
        {
          Node* node = new Node(val);
          tree.insert(node);
        }
    }
  
  current_output = &output;
  int count = preOrderRec(tree.getRoot(), bin_print_key);
  output << endl;
  
  cout << "  Height: " << computeHeightRec(tree.getRoot()) 
       << ", Nodes: " << count << endl;
  cout << "  Written: " << prefix << "bin.Tree" << endl;
  
  destroyRec(tree.getRoot());
}

// =============================================================================
// Main program
// =============================================================================

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Generate tree structure files for visualization.\n"
        "Creates .Tree files with preorder traversal for btreepic.",
        ' ', "1.0");

      TCLAP::ValueArg<size_t> nArg("n", "count",
                                    "Number of elements",
                                    false, 30, "size_t");
      cmd.add(nArg);

      TCLAP::ValueArg<unsigned int> seedArg("s", "seed",
                                             "Random seed (0 = use time)",
                                             false, 0, "unsigned int");
      cmd.add(seedArg);

      vector<string> allowedTypes = {"all", "avl", "rb", "splay", "treap", "rand", "bin"};
      TCLAP::ValuesConstraint<string> typeConstraint(allowedTypes);
      TCLAP::ValueArg<string> typeArg("t", "type",
                                       "Tree type to generate",
                                       false, "all", &typeConstraint);
      cmd.add(typeArg);

      TCLAP::ValueArg<string> prefixArg("o", "output",
                                         "Output file prefix",
                                         false, "", "prefix");
      cmd.add(prefixArg);

      cmd.parse(argc, argv);

      size_t n = nArg.getValue();
      unsigned int seed = seedArg.getValue();
      string type = typeArg.getValue();
      string prefix = prefixArg.getValue();

      if (seed == 0)
        seed = time(nullptr);

      cout << "=== Tree Structure Generator ===" << endl;
      cout << "Elements: " << n << ", Seed: " << seed << endl;
      cout << "Type: " << type << endl << endl;

      // Generate random values
      vector<int> values = generate_random_values(n, seed);
      
      cout << "Generated " << values.size() << " unique values" << endl << endl;

      // Generate requested trees
      if (type == "all" || type == "avl")
        {
          cout << "AVL Tree:" << endl;
          write_avl(values, prefix);
        }

      if (type == "all" || type == "rb")
        {
          cout << "Red-Black Tree:" << endl;
          write_rb(values, prefix);
        }

      if (type == "all" || type == "splay")
        {
          cout << "Splay Tree:" << endl;
          write_splay(values, prefix);
        }

      if (type == "all" || type == "treap")
        {
          cout << "Treap:" << endl;
          write_treap(values, prefix);
        }

      if (type == "all" || type == "rand")
        {
          cout << "Rand Tree:" << endl;
          write_rand(values, prefix);
        }

      if (type == "all" || type == "bin")
        {
          cout << "Binary Tree (unbalanced):" << endl;
          write_bin(values, prefix);
        }

      cout << endl << "Done." << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
