
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


/** @file generate_df_tree.H
 *  @brief Pictures of depth-first trees with their df and low numbers.
 *
 *  write_df_low_tree() writes, for the tree drawing program `ntreepic`,
 *  the depth-first spanning tree of an undirected graph from a start node.
 *  Each node of the tree shows the label of its graph node, its
 *  depth-first number (`df`) and its `low` number, the numbers with which
 *  Compute_Cut_Nodes finds the cut nodes (see tpl_cut_nodes.H). Each arc
 *  of the graph that is not in the tree is drawn as a dashed connection
 *  between its two nodes.
 *
 *  The numbers are those of Compute_Cut_Nodes: the start node has `df` 0,
 *  each other node gets the next number when the traversal discovers it,
 *  and the `low` number of a node is the smallest `df` that the node
 *  reaches through its subtree and at most one arc that is not in the
 *  tree. The start node has no `low` number (-1), as its cut-node test
 *  does not use it.
 *
 *  build_df_low_tree() returns the same tree as a Tree_Node, together
 *  with the arcs that are not in it, for other uses or other pictures.
 *
 *  ## Example
 *
 *  @code
 *  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
 *  Graph g;
 *  // ... insert the nodes and arcs ...
 *  std::ofstream out("df-low.Tree");
 *  write_df_low_tree(g, src, out);                   // labels "key,df,low"
 *  write_df_low_tree<Write_Df_Tree_Df>(g, src, out); // labels "(key,df)"
 *  @endcode
 *
 *  These functions do not modify the graph: they use neither the cookies
 *  nor the counters nor the control bits of its nodes and arcs.
 *
 *  @see tpl_cut_nodes.H For cut nodes, bridges and biconnected components
 *  @see generate_tree.H For the format that `ntreepic` reads
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

#ifndef GENERATE_DF_TREE_H
#define GENERATE_DF_TREE_H

# include <algorithm>
# include <memory>
# include <ostream>
# include <sstream>
# include <string>
# include <utility>

# include <ah-errors.H>
# include <ah-graph-concepts.H>
# include <generate_tree.H>
# include <htlist.H>
# include <tpl_dfs_stack.H>
# include <tpl_dynMapTree.H>
# include <tpl_graph.H>
# include <tpl_tree_node.H>

namespace Aleph
{
  /** @brief Key of a node of a depth-first tree with df and low numbers.
   *
   *  build_df_low_tree() gives one to each node of the tree.
   *
   *  @ingroup Graphs
   */
  struct Df_Low_Key
  {
    std::string label; ///< Label of the graph node
    long df = -1;      ///< Depth-first number: 0 for the start node
    long low = -1;     ///< Low number; -1 for the start node, which has none
  };

  /// Node of the depth-first tree that build_df_low_tree() returns.
  using Df_Low_Tree_Node = Tree_Node<Df_Low_Key>;

  /** @brief Arc of the graph that is not in the depth-first tree.
   *
   *  The tree nodes of its two ends: first the descendant, then the
   *  ancestor.
   */
  using Df_Low_Non_Tree_Arc = std::pair<Df_Low_Tree_Node *, Df_Low_Tree_Node *>;

  /** @brief Default label of a graph node: its information, written with
   *  `operator <<`.
   *
   *  @tparam GT Graph type
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT>
  struct Dft_Df_Tree_Label
  {
    /// Return the information of @p p, written with `operator <<`.
    std::string operator () (typename GT::Node * p) const
    {
      std::ostringstream s;
      s << p->get_info();
      return s.str();
    }
  };

  /** @brief Writer for generate_tree() that shows the label of each node.
   *
   *  @ingroup Graphs
   */
  struct Write_Df_Tree_Label
  {
    /// Return the label of @p p.
    std::string operator () (Df_Low_Tree_Node * p) const
    {
      return p->get_key().label;
    }
  };

  /** @brief Writer for generate_tree() that shows the label and the df
   *  number of each node, as `(label,df)`.
   *
   *  @ingroup Graphs
   */
  struct Write_Df_Tree_Df
  {
    /// Return `(label,df)` for @p p.
    std::string operator () (Df_Low_Tree_Node * p) const
    {
      const Df_Low_Key & key = p->get_key();
      return "(" + key.label + "," + std::to_string(key.df) + ")";
    }
  };

  /** @brief Writer for generate_tree() that shows the label, the df and
   *  the low numbers of each node, as `label,df,low`.
   *
   *  For the start node, which has no low number, it shows `label,df,-`.
   *
   *  @ingroup Graphs
   */
  struct Write_Df_Tree_Low
  {
    /// Return `label,df,low` for @p p, or `label,df,-` if it has no low number.
    std::string operator () (Df_Low_Tree_Node * p) const
    {
      const Df_Low_Key & key = p->get_key();
      return key.label + "," + std::to_string(key.df) + "," +
        (key.low >= 0 ? std::to_string(key.low) : std::string("-"));
    }
  };

  namespace df_tree_detail
  {
    /// Depth-first traversal that builds the tree of build_df_low_tree().
    template <AlephGraph GT, class Node_Label, ArcFilter<GT> SA>
    class Df_Low_Builder
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;

      Node_Label & node_label;
      SA & sa;
      DynList<Df_Low_Non_Tree_Arc> & non_tree_arcs;

      // Tree node of each visited graph node; it owns them until build()
      // returns
      DynMapTree<Node *, Df_Low_Tree_Node *> tree_nodes;
      long curr_df = 0;

      // Create the tree node of p, with the next df number
      Df_Low_Tree_Node * new_tree_node(Node * p)
      {
        auto owner = std::make_unique<Df_Low_Tree_Node>
          (Df_Low_Key{node_label(p), curr_df, curr_df});
        Df_Low_Tree_Node * tp = owner.get();
        tree_nodes.insert(p, tp);
        owner.release();
        ++curr_df;
        return tp;
      }

      // A node whose arcs the traversal goes through: the graph node, its
      // tree node, the arc through which it was reached, and the next arc
      // to look at
      struct Frame
      {
        Node * p;
        Df_Low_Tree_Node * tp;
        Arc * tree_arc;
        Node_Arc_Iterator<GT, SA> it;

        Frame(Node * n, Df_Low_Tree_Node * t, Arc * a, SA & filter)
          : p(n), tp(t), tree_arc(a), it(n, filter) {}
      };

      // Traverse from src, whose tree node is root. A path can have as many
      // nodes as the graph, too many for recursion, so that the traversal
      // keeps its own stack; the order of the visits is that of a
      // recursion.
      void visit(Node * src, Df_Low_Tree_Node * root)
      {
        Dfs_Stack<Frame> frames;
        frames.push(src, root, nullptr, sa);
        while (true)
          {
            Frame & f = frames.top();
            Df_Low_Key & key = f.tp->get_key();
            Node * next = nullptr;
            for (; f.it.has_curr(); f.it.next_ne())
              {
                Arc * a = f.it.get_curr();
                if (a == f.tree_arc)
                  continue;

                Node * t = f.it.get_tgt_node();
                if (auto pair = tree_nodes.search(t); pair != nullptr)
                  { // a visited t is an ancestor of p, a descendant or p itself
                    Df_Low_Tree_Node * tt = pair->second;
                    if (tt->get_key().df < key.df)
                      {
                        key.low = std::min(key.low, tt->get_key().df);
                        non_tree_arcs.append(Df_Low_Non_Tree_Arc(f.tp, tt));
                      }
                    continue;
                  }

                next = t; // the iterator stays on the arc, as in a recursion
                break;
              }

            if (next != nullptr)
              {
                // a child is linked while it is still a leaf, as Tree_Node
                // needs
                Df_Low_Tree_Node * child = new_tree_node(next);
                f.tp->insert_rightmost_child(child);
                frames.push(next, child, f.it.get_curr(), sa); // f may move
                continue;
              }

            // the arcs of f.p are done: back to the node that reached it
            const long low = key.low;
            frames.pop();
            if (frames.is_empty())
              return;

            Frame & parent = frames.top();
            Df_Low_Key & parent_key = parent.tp->get_key();
            parent_key.low = std::min(parent_key.low, low);
            parent.it.next_ne(); // past the arc to the child
          }
      }

    public:
      /// Prepare a traversal that labels the nodes with @p label, sees only
      /// the arcs that @p filter accepts and appends to @p arcs.
      Df_Low_Builder(Node_Label & label, SA & filter,
                     DynList<Df_Low_Non_Tree_Arc> & arcs)
        : node_label(label), sa(filter), non_tree_arcs(arcs)
      {
        // empty
      }

      /// Build the tree from @p src; on an exception, free what it built.
      Df_Low_Tree_Node * build(Node * src)
      {
        try
          {
            Df_Low_Tree_Node * root = new_tree_node(src);
            visit(src, root);
            root->get_key().low = -1; // as Compute_Cut_Nodes leaves it
            return root;
          }
        catch (...)
          {
            for (auto it = tree_nodes.get_it(); it.has_curr(); it.next_ne())
              delete it.get_curr().second;
            non_tree_arcs.empty();
            throw;
          }
      }
    };

    /// Give to @p p, and to each node below it, its Dewey number, as
    /// generate_tree() writes them.
    inline void number_tree(Df_Low_Tree_Node * p, const std::string & number,
                            DynMapTree<Df_Low_Tree_Node *, std::string> & numbers)
    {
      numbers.insert(p, number);
      size_t i = 0;
      for (auto c = p->get_left_child(); c != nullptr;
           c = c->get_right_sibling(), ++i)
        number_tree(c, number + "." + std::to_string(i), numbers);
    }

    /// Deleter that frees a whole tree.
    struct Tree_Deleter
    {
      /// Free @p root and every node below it.
      void operator () (Df_Low_Tree_Node * root) const
      {
        destroy_tree(root);
      }
    };
  } // namespace df_tree_detail

  /** @brief Build the depth-first spanning tree of an undirected graph,
   *  with the df and low numbers of its nodes.
   *
   *  Traverses in depth first the nodes of @p g that @p src reaches, through
   *  the arcs that @p sa accepts, and returns their tree: the children of
   *  a node are in the order in which the traversal discovered them. Each
   *  node of the tree has the label of its graph node and its df and low
   *  numbers, as Compute_Cut_Nodes computes them (see generate_df_tree.H).
   *
   *  @p non_tree_arcs receives, in the order in which the traversal finds
   *  them, the other arcs between the nodes that @p src reaches, but for
   *  the arcs from a node to itself: for each one, the tree nodes of its
   *  descendant end and of its ancestor end.
   *
   *  @tparam GT Graph type; it must not be a digraph
   *  @tparam Node_Label Functor with
   *          `std::string operator () (typename GT::Node *)` that gives
   *          the label of each node
   *  @tparam SA Arc filter
   *
   *  @param[in] g The graph
   *  @param[in] src The start node, the root of the tree
   *  @param[out] non_tree_arcs The arcs that are not in the tree; it is
   *              emptied first
   *  @param[in] node_label Label of each node
   *  @param[in] sa Arc filter
   *
   *  @return The root of the tree, which the caller frees with
   *          destroy_tree()
   *
   *  @throw std::domain_error if @p g is a digraph
   *  @throw std::invalid_argument if @p src is null
   *  @throw std::bad_alloc if there is not enough memory
   *
   *  On an exception, nothing is left allocated and @p non_tree_arcs is
   *  empty.
   *
   *  @note O((V + E) log V) time for the V nodes and E arcs that @p src
   *        reaches. The traversal does not recurse, so that the tree can
   *        be as deep as the graph has nodes.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT, class Node_Label = Dft_Df_Tree_Label<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  [[nodiscard]] Df_Low_Tree_Node *
  build_df_low_tree(const GT & g, typename GT::Node * src,
                    DynList<Df_Low_Non_Tree_Arc> & non_tree_arcs,
                    Node_Label node_label = Node_Label(), SA sa = SA())
  {
    ah_domain_error_if(g.is_digraph())
      << "build_df_low_tree(): g is a digraph";
    ah_invalid_argument_if(src == nullptr)
      << "build_df_low_tree(): src must be non-null";

    non_tree_arcs.empty();
    return df_tree_detail::Df_Low_Builder<GT, Node_Label, SA>
      (node_label, sa, non_tree_arcs).build(src);
  }

  /** @brief Write, for `ntreepic`, the depth-first spanning tree of an
   *  undirected graph with the df and low numbers of its nodes.
   *
   *  Builds the tree with build_df_low_tree(), writes it with
   *  generate_tree() and @p Write, and then writes each arc that is not
   *  in the tree as `Dashed-Connexion <descendant> <ancestor> L`, with
   *  the Dewey numbers of its two ends in the tree, alternating `L` and
   *  `R`, so that `ntreepic` draws them as dashed curves on either side.
   *
   *  @tparam Write Writer of the label of each tree node:
   *          Write_Df_Tree_Low (`label,df,low`, the default),
   *          Write_Df_Tree_Df (`(label,df)`), Write_Df_Tree_Label
   *          (`label`) or one of the caller, with
   *          `std::string operator () (Df_Low_Tree_Node *)`
   *  @tparam GT Graph type; it must not be a digraph
   *  @tparam Node_Label Functor with
   *          `std::string operator () (typename GT::Node *)` that gives
   *          the label of each node
   *  @tparam SA Arc filter
   *
   *  @param[in] g The graph
   *  @param[in] src The start node, the root of the tree
   *  @param[out] out Stream where the picture is written
   *  @param[in] node_label Label of each node
   *  @param[in] sa Arc filter
   *
   *  @throw std::domain_error if @p g is a digraph
   *  @throw std::invalid_argument if @p src is null
   *  @throw std::bad_alloc if there is not enough memory
   *
   *  On an exception, @p out may hold part of the picture.
   *
   *  @see build_df_low_tree() For the tree and its numbers
   *
   *  @ingroup Graphs
   */
  template <class Write = Write_Df_Tree_Low, AlephGraph GT,
            class Node_Label = Dft_Df_Tree_Label<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  void write_df_low_tree(const GT & g, typename GT::Node * src,
                         std::ostream & out,
                         Node_Label node_label = Node_Label(), SA sa = SA())
  {
    DynList<Df_Low_Non_Tree_Arc> non_tree_arcs;
    std::unique_ptr<Df_Low_Tree_Node, df_tree_detail::Tree_Deleter> root
      (build_df_low_tree(g, src, non_tree_arcs, node_label, sa));

    generate_tree<Df_Low_Tree_Node, Write>(root.get(), out);

    DynMapTree<Df_Low_Tree_Node *, std::string> numbers;
    df_tree_detail::number_tree(root.get(), "0", numbers);

    size_t i = 0;
    for (auto it = non_tree_arcs.get_it(); it.has_curr(); it.next_ne(), ++i)
      {
        const Df_Low_Non_Tree_Arc & arc = it.get_curr();
        out << "Dashed-Connexion " << numbers.find(arc.first) << " "
            << numbers.find(arc.second) << (i % 2 == 0 ? " L" : " R") << '\n';
      }
  }
} // namespace Aleph

#endif // GENERATE_DF_TREE_H
