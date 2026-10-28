
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


/** @file graph_to_tree.H
 *  @brief Convert spanning tree graphs to Tree_Node structures.
 *
 *  This file provides utilities for converting spanning trees represented
 *  as adjacency-list graphs into Tree_Node<Key> structures, enabling the
 *  use of tree-specific algorithms and visualization tools.
 *
 *  ## Purpose
 *
 *  While spanning trees can be represented using graph structures like
 *  List_Graph, the Tree_Node representation offers advantages for:
 *  - Tree drawing and visualization (e.g., with ntreepic)
 *  - Tree-specific traversal algorithms
 *  - Simpler recursive tree operations
 *
 *  ## Key Limitations
 *
 *  Tree_Node<Key> is less versatile than graph representations:
 *  - No storage for arc/edge data (only node data)
 *  - Key type may differ from the graph's node type
 *  - Requires a Convert class to map graph nodes to tree keys
 *
 *  @par Example: Converting graph to DFS tree
 *  @code
 *  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
 *  GT g;
 *  
 *  auto a = g.insert_node("A");
 *  auto b = g.insert_node("B");
 *  auto c = g.insert_node("C");
 *  g.insert_arc(a, b);
 *  g.insert_arc(a, c);
 *  
 *  // Define converter from graph node to tree node
 *  struct CopyInfo {
 *    void operator()(GT::Node* gnode, Tree_Node<string>* tnode) {
 *      tnode->get_key() = gnode->get_info();
 *    }
 *  };
 *  
 *  Tree_Node<string> tree_root;
 *  DynMapTree<GT::Node*, Tree_Node<string>*> node_map;
 *  
 *  build_tree_dfs<GT, Tree_Node<string>, CopyInfo>(g, a, tree_root, node_map);
 *  // tree_root now contains the DFS spanning tree
 *  @endcode
 *
 *  @par Example: BFS tree with node mapping
 *  @code
 *  build_tree_bfs<GT, Tree_Node<string>, CopyInfo>(g, a, tree_root, node_map);
 *  
 *  // Use mapping to find tree node from graph node
 *  auto tree_node = node_map[b];  // Get tree node for graph node b
 *  @endcode
 *
 *  @see Tree_Node For the target tree structure
 *  @see generate_tree.H For tree visualization
 *  @see tpl_spanning_tree.H For spanning tree algorithms
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef GRAPH_TO_TREE_H
# define GRAPH_TO_TREE_H

# include <memory>

# include <ah-graph-concepts.H>

# include <tpl_tree_node.H>
# include <tpl_dfs_stack.H>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph {

  namespace graph_to_tree_detail
  {
    /// A node of the tree being built that waits for the one that it
    /// visits, its image in the tree, the arc through which it was reached
    /// and the next arc of it to look at
    template <class GT, typename Key, class SA>
    struct Frame
    {
      Tree_Node<Key> * troot;
      typename GT::Arc * from;
      Node_Arc_Iterator<GT, SA> it;

      template <class... Args>
      [[gnu::always_inline, gnu::flatten]]
      Frame(Tree_Node<Key> * t, typename GT::Arc * a, Args &&... args)
        : troot(t), from(a), it(std::forward<Args>(args)...) {}

      [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }

      // Moves the iterator to the next arc that is not that through which
      // the node was reached, and gives its target, or nullptr when the
      // arcs are done. In a tree, which the graph is, the other arcs go
      // to nodes not reached yet, so that the arcs need no marks.
      [[gnu::always_inline, gnu::flatten]] typename GT::Node * next_target() noexcept
      {
        for (; it.has_curr(); it.next_ne())
          if (it.get_current_arc_ne() != from)
            return it.get_tgt_node_ne();
        return nullptr;
      }
    };

    // Builds below troot the tree of what groot reaches. For each node that
    // it reaches, it gives a new tree node and the graph node to
    // convert(gnode, tnode), and puts the tree node as the rightmost child
    // of that of the node from which it was reached. A path can have as
    // many nodes as the graph, too many for recursion, so that the search
    // keeps its own stack; the order of the visits is that of a recursion.
    // If something throws, the nodes already put below troot stay there.
    template <class GT, typename Key, class SA, class Convert, class... Args>
    inline void build(Tree_Node<Key> * troot, Convert && convert,
                      typename GT::Node * groot, Args &&... args)
    {
      using Frame = graph_to_tree_detail::Frame<GT, Key, SA>;

      // Gives the tree node of gtgt, the rightmost child of that of f
      auto child_of = [&convert](Frame & f, typename GT::Node * gtgt)
        {
          // freed if convert throws
          std::unique_ptr<Tree_Node<Key>> ttgt(new Tree_Node<Key>);

          convert(gtgt, ttgt.get()); // assign its key

          f.troot->insert_rightmost_child(ttgt.get()); // insert it as a child
          return ttgt.release();
        };

      // The node being visited keeps its iterator in here until it visits
      // a node; then it waits in frames, where it goes on when it comes
      // back
      Dfs_Stack<Frame> frames;
      Frame here(troot, nullptr, groot, args...);
      while (true)
        {
          if (auto gtgt = here.next_target())
            { // the iterator stays on the arc to gtgt, as in a recursion
              Frame next(child_of(here, gtgt), here.it.get_current_arc_ne(),
                         gtgt, args...);
              frames.push(std::move(here));
              here = std::move(next);
              continue;
            }

          while (true) // back to the nodes that wait
            {
              if (frames.is_empty())
                return;

              Frame & f = frames.top();
              f.next_arc(); // past the arc to the node that was visited
              if (auto gtgt = f.next_target())
                {
                  here = Frame(child_of(f, gtgt), f.it.get_current_arc_ne(),
                               gtgt, args...);
                  break;
                }
              frames.pop();
            }
        }
    }
  } // namespace graph_to_tree_detail

  template <AlephGraph GT, typename Key, typename Convert, ArcFilter<GT> SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot);

/** @brief Convert a tree graph to a Tree_Node structure.
 *
 *  Takes a tree represented as an adjacency-list graph and converts it
 *  to a Tree_Node<Key> structure rooted at the specified node.
 *
 *  This enables the use of tree-specific algorithms and visualization
 *  tools on spanning trees extracted from graphs.
 *
 *  ## Conversion Process
 *
 *  The Convert class is invoked for each node:
 *  ```cpp
 *  void Convert::operator()(GT::Node* gnode, Tree_Node<Key>* tnode)
 *  ```
 *  It should copy/transform data from the graph node to the tree node.
 *
 *  ## Limitations
 *
 *  - Tree_Node<Key> cannot store arc data (only node data)
 *  - The Key type may differ from GT::Node's data type
 *  - Requires the input graph to be acyclic (a tree)
 *
 *  @tparam GT Graph type (derived from List_Graph)
 *  @tparam Key Data type for tree nodes
 *  @tparam Convert Converter class with operator()(gnode, tnode)
 *  @tparam SA Arc filter (default: show all arcs)
 *
 *  @param g The tree graph to convert
 *  @param groot Node to use as the tree root
 *  @return Root of the new Tree_Node<Key> tree
 *
 *  @throw std::domain_error If the graph is not acyclic
 *  @throw std::bad_alloc If memory allocation fails
 *  @throw anything that Convert throws. If something throws, the tree
 *  that was built is freed.
 *
 *  @note The arcs are not marked: the search does not go back through the
 *  arc by which it reached a node, which is enough in a tree. So the
 *  graph can be converted again.
 *  @note Performs an acyclicity check which adds O(V+E) overhead.
 *
 *  @see Tree_Node Target tree structure
 *  @see is_graph_acyclique() Acyclicity verification
 *
 *  @ingroup Graphs
 */
    template <AlephGraph GT, typename Key, 
              class Convert, ArcFilter<GT> SA = Dft_Show_Arc<GT>> inline
Tree_Node<Key> * graph_to_tree_node(GT & g, typename GT::Node * groot)
{ 
  ah_domain_error_if(not is_graph_acyclique<GT>(g))
    << "Graph is not a tree (not acyclique)";

  Tree_Node<Key> * troot = new Tree_Node<Key>; // the root
  try
    {
      Convert () (groot, troot); // convert groot into troot

      __graph_to_tree_node <GT, Key, Convert, SA> (g, groot, troot);
    }
  catch (...)
    {
      destroy_tree(troot); // what was built
      throw;
    }

  return troot;
}

/** @brief Functor class to convert a tree graph to Tree_Node structure.
 *
 *  Provides a callable interface for converting spanning trees from
 *  graph representation to Tree_Node<Key> representation.
 *
 *  This class version offers more flexibility than the free function,
 *  allowing the arc filter to be configured at construction time.
 *
 *  ## Template Parameters
 *
 *  - **GT**: Graph type (derived from List_Graph)
 *  - **Key**: Data type to store in tree nodes
 *  - **Convert**: Functor class with `operator()(gnode, tnode)` that
 *    copies data from graph nodes to tree nodes
 *  - **SA**: Arc filter class (default: show all arcs)
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  struct MyConvert {
 *    void operator()(Graph::Node* gn, Tree_Node<int>* tn) {
 *      tn->get_key() = gn->get_info().id;
 *    }
 *  };
 *
 *  Graph_To_Tree_Node<Graph, int, MyConvert> converter;
 *  auto* root = converter(g, start_node);
 *  ```
 *
 *  @see graph_to_tree_node() Free function alternative
 *  @see Tree_Node Target tree structure
 *
 *  @ingroup Graphs
 */
    template <AlephGraph GT, typename Key, 
	      class Convert, 
	      ArcFilter<GT> SA = Dft_Show_Arc<GT> > 
class Graph_To_Tree_Node
{
  SA        sa;
  Convert * convert = nullptr;

  void graph_to_tree(typename GT::Node * groot, Tree_Node<Key> * troot)
  {           
    // go through the arcs of groot and build the tree
    graph_to_tree_detail::build<GT, Key, SA>(troot, *convert, groot, sa);
  }

  Tree_Node<Key> * graph_to_tree(GT & g,
				 typename GT::Node * groot,
				 Convert & conv)
  { 
    ah_domain_error_if(not is_graph_acyclique<GT>(g))
      << "Graph is not a tree (not acyclique)";

    Tree_Node<Key> * troot = new Tree_Node<Key>; // the root

    convert = &conv;
    try
      {
        (*convert) (groot, troot); // convert groot into troot

        graph_to_tree(groot, troot);
      }
    catch (...)
      {
        destroy_tree(troot); // what was built
        throw;
      }

    return troot;
  }

public:

  Graph_To_Tree_Node(SA __sa = SA()) : sa(__sa) { /* empty */ }

/** @brief Convert a tree graph to Tree_Node structure.
   *
   *  @param g The tree graph to convert (must be acyclic)
   *  @param groot Node to use as the tree root
   *  @param conv Converter functor instance
   *  @return Root of the new Tree_Node<Key> tree
   *
   *  @throw std::domain_error If the graph is not acyclic
   *  @throw std::bad_alloc If memory allocation fails
   *  @throw anything that conv throws. If something throws, the tree
   *  that was built is freed.
   *
   *  @note The arcs are not marked, so that the graph can be converted
   *  again.
   */
      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert && conv = Convert())
  { 
    return graph_to_tree(g, groot, conv);
  }

      Tree_Node<Key> * 
  operator () (GT & g, typename GT::Node * groot, Convert & conv)
  { 
    return graph_to_tree(g, groot, conv);
  }
};


  template <AlephGraph GT, typename Key, typename Convert, ArcFilter<GT> SA> static inline
void __graph_to_tree_node(GT & g, typename GT::Node * groot,
                          Tree_Node<Key> * troot)
{           
  (void) g;

  // go through the arcs of groot and build the tree
  graph_to_tree_detail::build<GT, Key, SA>
    (troot, [](typename GT::Node * gtgt, Tree_Node<Key> * ttgt)
             {
               Convert () (gtgt, ttgt);
             }, groot);
}

} // end namespace Aleph

# endif // GRAPH_TO_TREE_H
