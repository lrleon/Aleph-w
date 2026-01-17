
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
 * @file write_tree.C
 * @brief Generates tree structure files for visualization and analysis
 * 
 * This utility program creates `.Tree` files containing binary search tree
 * structures in preorder traversal format. These files are designed for
 * visualization tools like `btreepic` or for algorithm testing and analysis.
 * This tool is essential for understanding how different BST implementations
 * structure the same data differently.
 *
 * ## Purpose
 *
 * ### Use Cases
 *
 * This tool is useful for:
 * - **Visualization**: Generating tree structures for LaTeX/eepic diagrams
 * - **Algorithm testing**: Creating test cases for tree algorithms
 * - **Comparison**: Generating the same data with different tree types
 * - **Education**: Understanding how different BST implementations structure data
 * - **Research**: Studying tree structure properties
 * - **Documentation**: Creating tree diagrams for papers/presentations
 *
 * ## Supported Tree Types
 *
 * ### Balanced Trees
 *
 * #### AVL Tree
 * - **Balance**: Strictly balanced (height difference ≤ 1)
 * - **Structure**: Most balanced possible
 * - **Use**: When balance is critical
 *
 * #### Red-Black Tree
 * - **Balance**: Relaxed balance (no path > 2× shortest)
 * - **Structure**: Good balance, efficient operations
 * - **Use**: General-purpose balanced tree
 *
 * ### Self-Adjusting Trees
 *
 * #### Splay Tree
 * - **Strategy**: Moves accessed nodes to root
 * - **Structure**: Adapts to access patterns
 * - **Use**: When temporal locality matters
 *
 * ### Randomized Trees
 *
 * #### Treap
 * - **Strategy**: Randomized BST with heap priorities
 * - **Structure**: Probabilistically balanced
 * - **Use**: Simple, good average case
 *
 * #### Rand Tree
 * - **Strategy**: Randomized BST variant
 * - **Structure**: Different randomization approach
 * - **Use**: Alternative randomized structure
 *
 * ### Unbalanced Tree
 *
 * #### Binary Tree
 * - **Strategy**: No balancing
 * - **Structure**: Can degrade to linked list
 * - **Use**: Baseline comparison, educational
 *
 * ### Summary Table
 *
 * | Type | Description | Balance Strategy | Best For |
 * |------|-------------|------------------|----------|
 * | **AVL** | Strictly balanced | Height difference ≤ 1 | Read-heavy |
 * | **Red-Black** | Relaxed balance | No path > 2× shortest | General purpose |
 * | **Splay** | Self-adjusting | Moves accessed nodes to root | Temporal locality |
 * | **Treap** | Randomized | Heap priorities for balance | Simple, average case |
 * | **Rand** | Randomized BST | Random insertion order | Alternative random |
 * | **Binary** | Unbalanced | No balancing | Baseline, educational |
 *
 * ## Output Format
 *
 * ### File Structure
 *
 * Each `.Tree` file contains:
 * - **Preorder traversal**: Tree structure in preorder format
 * - **Node values**: Keys stored in nodes
 * - **Format**: Compatible with `btreepic` visualization tool
 *
 * ### File Naming
 *
 * Files are named: `<prefix><tree_type>.Tree`
 * - Examples: `test_avl.Tree` (use `-o test_`), `rb.Tree` (default prefix), `mytree_splay.Tree` (use `-o mytree_`)
 *
 * ### Format Details
 *
 * The `.Tree` format includes:
 * - Node keys in preorder
 * - Tree structure information
 * - Metadata for visualization
 *
 * ## Comparison Capabilities
 *
 * ### Same Data, Different Structures
 *
 * Generate same data with different tree types to compare:
 * - **Structure differences**: See how trees differ
 * - **Balance**: Compare balance quality
 * - **Height**: Compare tree heights
 * - **Visualization**: Visual comparison
 *
 * ### Example
 *
 * ```bash
 * # Generate same data with different trees
 * write_tree -n 50 -s 42 -t avl   -o compare_
 * write_tree -n 50 -s 42 -t rb    -o compare_
 * write_tree -n 50 -s 42 -t splay -o compare_
 *
 * # Visualize and compare
 * btreepic compare_avl.Tree
 * btreepic compare_rb.Tree
 * btreepic compare_splay.Tree
 * ```
 *
 * ## Usage Examples
 *
 * ```bash
 * # Generate all tree types with 100 nodes
 * write_tree -n 100
 *
 * # Generate only AVL tree with specific seed
 * write_tree -n 50 -t avl -s 12345 -o mytree_
 *
 * # Generate Red-Black and Splay trees
 * write_tree -n 200 -t rb
 * write_tree -n 200 -t splay
 *
 * # Generate multiple types for comparison
 * write_tree -n 100 -t avl -s 42 -o compare_
 * write_tree -n 100 -t rb  -s 42 -o compare_
 * write_tree -n 100 -t splay -s 42 -o compare_
 * ```
 *
 * ## Parameters
 *
 * | Parameter | Short | Description | Default |
 * |-----------|-------|-------------|---------|
 * | `--count` | `-n` | Number of nodes to insert | 30 |
 * | `--seed` | `-s` | Random seed for reproducibility | Time-based |
 * | `--type` | `-t` | Tree type (avl, rb, splay, treap, rand, bin, all) | all |
 * | `--output` | `-o` | Output file prefix (concatenated literally; use a trailing `_` if you want a separator) | empty |
 *
 * ## Integration with Visualization
 *
 * ### btreepic Tool
 *
 * Generated `.Tree` files can be visualized:
 * ```bash
 * btreepic output_avl.Tree > avl.tex
 * pdflatex avl.tex
 * ```
 *
 * ### LaTeX Output
 *
 * The visualization generates LaTeX files suitable for:
 * - **Papers**: Include in academic papers
 * - **Presentations**: Use in slides
 * - **Documentation**: Document tree structures
 *
 * ## Applications
 *
 * ### Algorithm Development
 * - **Test cases**: Generate test trees for algorithms
 * - **Debugging**: Visualize tree structures during debugging
 * - **Validation**: Verify algorithm correctness
 *
 * ### Education
 * - **Teaching**: Demonstrate tree structures visually
 * - **Learning**: Understand how trees differ
 * - **Comparison**: Compare different implementations
 *
 * ### Research
 * - **Experiments**: Generate trees for experiments
 * - **Analysis**: Analyze tree properties
 * - **Publications**: Create figures for papers
 *
 * @see btreepic.C Visualization tool for binary trees
 * @see timeAllTree.C Performance comparison (related)
 * @see dynset_trees.C Practical comparison
 * @see tpl_avl.H AVL tree implementation
 * @see tpl_rb_tree.H Red-Black tree implementation
 * @see tpl_splay_tree.H Splay tree implementation
 * @see tpl_treap.H Treap implementation
 * @author Leandro Rabindranath León
 * @ingroup Examples
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
