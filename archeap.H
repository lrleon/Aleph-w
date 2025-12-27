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
# ifndef ARCHEAP_H
# define ARCHEAP_H

# include <tpl_binHeap.H>
# include <tpl_fibonacci_heap.H>
# include <tpl_graph_utils.H>

template <class GT,
          class Distance,
          class Access_Heap_Node>
class ArcHeap
    : public BinHeap<typename GT::Arc *, Distance_Compare<GT, Distance>>
{
  Distance dist;
  Access_Heap_Node access_node;
  Distance_Compare<GT, Distance> dist_cmp;

public:
  /// Return the distance functor used to compare arcs in this heap.
  Distance &get_distance() { return dist; }

  /// Construct an empty arc heap with the given distance functor and
  /// heap-node access policy.
  ArcHeap(Distance __dist = Distance(),
          Access_Heap_Node acc = Access_Heap_Node())
    : dist(__dist), access_node(acc), dist_cmp(dist)
  {
    // empty
  }

  typedef
  BinHeap<typename GT::Arc *, Distance_Compare<GT, Distance>> Heap;

  typedef typename Heap::Node Node;

  using handle_type = Node *;

  /// Insert or update an arc associated with a target node, keeping the
  /// smallest-distance arc per node in the heap.
  void put_arc(typename GT::Arc *arc, typename GT::Node *tgt)
  {
    auto & handle_storage = access_node(tgt);
    handle_type & heap_node =
      reinterpret_cast<handle_type &>(handle_storage);

    if (heap_node == nullptr) // Is there already an arc inserted into the heap?
      { // No ==> create a new heap node and assign the arc
        heap_node = new Node;
        heap_node->get_key() = arc;
        this->insert(heap_node);

        return;
      }

    // two arcs with the same target ==> keep the smaller; discard the larger
    typename GT::Arc *& arc_in_heap = heap_node->get_key();

    // Does arc_in_heap have a shorter distance than arc?
    if (dist_cmp(arc_in_heap, arc))
      return; // old arc remains in the heap; new one is ignored

    // arc_in_heap will be the newly inserted arc
    arc_in_heap = arc;
    this->update(heap_node);
  }

  /// Extract the arc with minimum distance from the heap and clear its
  /// node-to-heap mapping.
  typename GT::Arc * get_min_arc()
  {
    Node *heap_node = this->getMin();
    typename GT::Arc *arc = heap_node->get_key();

    // Select the node that the access functor maps to this heap node
    auto *p = static_cast<typename GT::Node *>(arc->src_node);
    auto & handle_src =
      reinterpret_cast<handle_type &>(access_node(p));
    if (handle_src != heap_node)
      p = static_cast<typename GT::Node *>(arc->tgt_node);

    auto & handle_ref =
      reinterpret_cast<handle_type &>(access_node(p));
    assert(handle_ref == heap_node);

    handle_ref = nullptr;

    delete heap_node;

    return arc;
  }

  /// Remove all heap nodes and delete their memory.
  void empty()
  {
    this->remove_all_and_delete();
  }

  /// Destructor. Clears all remaining heap nodes.
  ~ArcHeap()
  {
    empty();
  }
};



template <class GT,
          class Distance,
          class Access_Heap_Node>
class ArcFibonacciHeap
{
  using Compare = Distance_Compare<GT, Distance>;
  using Heap = Fibonacci_Heap<typename GT::Arc *, Compare>;

public:
  using handle_type = typename Heap::handle_type;

private:
  Distance dist;
  Access_Heap_Node access_node;
  Compare dist_cmp;
  Heap heap;

  handle_type & handle_ref(typename GT::Node *p)
  {
    return reinterpret_cast<handle_type &>(access_node(p));
  }

public:
  /// Return the distance functor used to compare arcs in this heap.
  Distance &get_distance() { return dist; }

  /// Construct an empty Fibonacci arc heap with the given distance functor and
  /// heap-node access policy.
  ArcFibonacciHeap(Distance __dist = Distance(),
                   Access_Heap_Node acc = Access_Heap_Node())
    : dist(__dist), access_node(acc), dist_cmp(dist), heap(dist_cmp)
  {
    // empty
  }

  /// Insert or update an arc associated with a target node, keeping the
  /// smallest-distance arc per node in the heap.
  void put_arc(typename GT::Arc *arc, typename GT::Node *tgt)
  {
    auto & heap_node = handle_ref(tgt);

    if (heap_node == nullptr) // Is there already an arc inserted into the heap?
      { // No ==> create a new heap node and assign the arc
        heap_node = heap.insert(arc);
        return;
      }

    // two arcs with the same target ==> keep the smaller; discard the larger
    typename GT::Arc *arc_in_heap = heap_node->data;

    // Does arc_in_heap have a shorter distance than arc?
    if (dist_cmp(arc_in_heap, arc))
      return; // old arc remains in the heap; new one is ignored

    heap.decrease_key(heap_node, arc);
  }

  /// Extract the arc with minimum distance from the heap and clear its
  /// node-to-heap mapping.
  typename GT::Arc * get_min_arc()
  {
    ah_underflow_error_if(heap.is_empty()) << "ArcFibonacciHeap is empty";

    auto *heap_node = heap.get_min_node();
    auto *arc = heap_node->data;

    // Select the node that the access functor maps to this heap node
    auto *p = static_cast<typename GT::Node *>(arc->src_node);
    auto & handle_src = handle_ref(p);
    if (handle_src != heap_node)
      p = static_cast<typename GT::Node *>(arc->tgt_node);

    auto & handle_entry = handle_ref(p);
    assert(handle_entry == heap_node);

    handle_entry = nullptr;

    (void) heap.extract_min();

    return arc;
  }

  /// Return true if the heap is empty.
  bool is_empty() const noexcept { return heap.is_empty(); }

  /// Remove all heap nodes and delete their memory.
  void empty()
  {
    heap.clear();
  }

  /// Destructor. Clears all remaining heap nodes.
  ~ArcFibonacciHeap()
  {
    empty();
  }
};

# endif // ARCHEAP_H
