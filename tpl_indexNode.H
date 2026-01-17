
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


/** @file tpl_indexNode.H
 *  @brief Node indexing for fast O(log n) lookup by key.
 *
 *  @par Example: Fast node search by value
 *  @code
 *  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
 *  GT g;
 *  IndexNode<GT> node_index(g);
 *  
 *  // Insert and index nodes
 *  auto alice = node_index.insert_in_graph("Alice");
 *  auto bob = node_index.insert_in_graph("Bob");
 *  auto charlie = node_index.insert_in_graph("Charlie");
 *  
 *  // O(log n) search by value
 *  auto found = node_index.search("Bob");
 *  if (found)
 *    cout << "Found: " << found->get_info() << endl;
 *  @endcode
 *
 *  @par Example: User database with unique IDs
 *  @code
 *  struct User { int id; string name; };
 *  using UserGraph = List_Graph<Graph_Node<User>, Graph_Arc<Empty_Class>>;
 *  
 *  // Custom comparator by ID
 *  struct UserCmp {
 *    bool operator()(UserGraph::Node* a, UserGraph::Node* b) const {
 *      return a->get_info().id < b->get_info().id;
 *    }
 *  };
 *  
 *  IndexNode<UserGraph, UserCmp> users(graph);
 *  // Fast lookup by user ID
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_INDEXNODE_H
# define TPL_INDEXNODE_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>
# include <ah-errors.H>

namespace Aleph
{

  template <class GT>
  struct Dft_Node_Cmp
  {
    bool
    operator () (typename GT::Node * p1, typename GT::Node * p2) const
    {
      return p1->get_info() < p2->get_info();
    }
  };

  /**
   * @class IndexNode
   * @brief Builds an index of nodes for fast search and retrieval.
   *
   * IndexNode indexes the nodes of a graph according to a user-defined key.
   *
   * @tparam GT The graph type, based on List_Graph.
   * @tparam Compare The comparison class for the indexing key. The contract
   *                 for this class is to implement the () operator as follows:
   * \code
   * template <class GT>
   * struct Dft_Node_Cmp
   * {
   *   bool
   *   operator () (typename GT::Node * p1, typename GT::Node * p2) const
   *   {
   *     // access nodes and compare based on the desired field
   *   }
   * };
   * \endcode
   * By default, IndexNode uses a class that compares the value returned by
   * get_info() on each node. For this, the < operator of the GT::Node_Type
   * must be implemented.
   * @tparam SN The filter class for the node iterator, if one is constructed.
   * @tparam Tree The binary search tree type used internally for indexing keys.
   *              By default, Treaps are used.
   *
   * @see IndexArc, Index_Graph
   * @ingroup Graphs
   * @author Leandro Rabindranath León (lrleon at ula dot ve)
   * @author Alejandro Mujica (aledrums at gmail dot com)
   */
  template <class GT, class Compare = Dft_Node_Cmp<GT>,
            template <class /* Key */, class /* Compare */> class Tree = Treap,
            class SN = Dft_Show_Node<GT> >
  class IndexNode
  {
    typedef typename GT::Node GT_Node;
    typedef typename GT::Node_Type GT_Node_Type;

    DynSetTree<GT_Node*, Tree, Compare> index;

    GT & g;
    SN sn;

  public:

    /**
     * @brief Inserts the node p into the index.
     * @param p Pointer to the node to be stored in the index.
     * @return A pointer to the inserted node.
     * @throw std::bad_alloc if there is not enough memory.
     */
    GT_Node * insert(GT_Node * p)
    {
      index.put(p);
      return p;
    }

    /**
     * @brief Creates a new node with generic content info, inserts it into
     *        the graph, and then into the index.
     * @param info Content associated with the new node.
     * @return A pointer to the created node.
     * @throw std::bad_alloc if there is not enough memory.
     */
    GT_Node * insert_in_graph(const GT_Node_Type & info)
    {
      GT_Node * ret_val = g.insert_node(info);

      try
        {
          insert(ret_val);
          return ret_val;
        }
      catch (...)
        {
          g.remove_node(ret_val);
          throw;
        }
    }

    /// \overload insert_in_graph(const GT_Node_Type & info)
    GT_Node * insert_in_graph(GT_Node_Type && info)
    {
      GT_Node * ret_val = g.insert_node(std::forward<GT_Node_Type>(info));

      try
        {
          insert(ret_val);
          return ret_val;
        }
      catch (...)
        {
          g.remove_node(ret_val);
          throw;
        }
    }

    /**
     * @brief Searches for a node based on the content of p.
     *
     * Note that the search criterion is determined by the Compare
     * template parameter class.
     *
     * @param p The node whose information is to be searched for.
     * @return A pointer to the node if found, or nullptr otherwise.
     */
    GT_Node * search(GT_Node * p)
    {
      GT_Node ** ret_val = index.search(p);

      if (ret_val == nullptr)
        return nullptr;

      return *ret_val;
    }

    /**
     * @brief Searches for a node based on the info.
     *
     * Note that the search criterion is determined by the Compare
     * template parameter class.
     *
     * @param info The information to search for.
     * @return A pointer to the node if found, or nullptr otherwise.
     */
    GT_Node * search(const GT_Node_Type & info)
    {
      GT_Node dummy_node(info);
      return search(&dummy_node);
    }

    /// Removes the node at address p from the index.
    void remove(GT_Node * p)
    {
      index.remove(p);
    }

    /**
     * @brief Removes the node p from the graph and the index.
     * @param p Pointer to the node to be removed.
     * @throw std::domain_error if the node is not in the index (which
     *        likely means it's not in the graph either).
     */
    void remove_from_graph(GT_Node * p)
    {
      index.find(p); // Throws an exception if p is not in the index
      index.remove(p);
      g.remove_node(p);
    }

    /// Clears the index; all nodes are removed from it.
    void clear_index()
    {
      index.empty();
    }

    /// Inserts all nodes from the graph into the index.
    void build_index()
    {
      for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
        {
          GT_Node * p = it.get_curr_ne();
          if (search(p) != p)
            insert(p);
        }
    }

    /// Removes all nodes from the graph and the index.
    void clear_graph()
    {
      clear_index();
      Aleph::clear_graph(g);
    }

  private:

    void init()
    {
      for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
        insert(it.get_curr_ne());
    }

  public:

    /**
     * @brief Creates an index from the nodes inserted in the graph.
     * @param __g The graph.
     * @param __sn A node filter object.
     * @throw std::bad_alloc if there is not enough memory.
     */
    IndexNode(GT & __g, SN __sn = SN()) : g(__g), sn(std::move(__sn))
    {
      init();
    }

    /// Returns the number of items contained in the index.
    size_t size() const { return index.size(); }
  };

}

# endif
