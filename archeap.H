
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


/** @file archeap.H
 *  @brief Arc heap for graph algorithms.
 *
 *  Provides ArcHeap template for maintaining a priority queue of
 *  graph arcs ordered by distance/weight. Used by Prim, Dijkstra,
 *  and other graph algorithms.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef ARCHEAP_H
# define ARCHEAP_H

# include <memory>

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
    : Heap(Distance_Compare<GT, Distance>(__dist)), // orders the heap
      dist(__dist), access_node(acc), dist_cmp(dist)
  {
    // empty
  }

  /// Copying is disabled (heap manages internal node memory)
  ArcHeap(const ArcHeap &) = delete;
  ArcHeap & operator=(const ArcHeap &) = delete;

  /// Move construction transfers ownership of heap nodes. (The base heap
  /// cannot be moved: it points to its own header, so it is swapped.)
  ArcHeap(ArcHeap && other) noexcept
    : Heap(other.get_compare()), dist(std::move(other.dist)),
      access_node(std::move(other.access_node)), dist_cmp(dist)
  {
    this->swap(other);
  }

  /// Move assignment transfers ownership of heap nodes, after emptying this
  /// heap as empty() does
  ArcHeap & operator=(ArcHeap && other) noexcept
  {
    if (this != &other)
      {
        empty();
        this->swap(other);
        dist = std::move(other.dist);
        access_node = std::move(other.access_node);
        dist_cmp = Distance_Compare<GT, Distance>(dist);
      }
    return *this;
  }

  typedef
  BinHeap<typename GT::Arc *, Distance_Compare<GT, Distance>> Heap;

  typedef typename Heap::Node Node;

  using handle_type = Node *;

private:
  // The handle that the access functor keeps for heap_node: that of the
  // node of its arc whose handle it is
  handle_type & handle_of(Node *heap_node)
  {
    typename GT::Arc *arc = heap_node->get_key();
    auto *p = static_cast<typename GT::Node *>(arc->src_node);
    auto & handle_src = reinterpret_cast<handle_type &>(access_node(p));
    if (handle_src == heap_node)
      return handle_src;

    p = static_cast<typename GT::Node *>(arc->tgt_node);
    auto & handle_tgt = reinterpret_cast<handle_type &>(access_node(p));
    assert(handle_tgt == heap_node);
    return handle_tgt;
  }

public:

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
  /// node-to-heap mapping. If the distance throws while the other arcs
  /// are put back in order, the arc is already out of the heap, its heap
  /// node freed and its mapping cleared.
  typename GT::Arc * get_min_arc()
  {
    Node *heap_node = this->top();
    typename GT::Arc *arc = heap_node->get_key();
    handle_of(heap_node) = nullptr;

    std::unique_ptr<Node> owner(heap_node); // freed even if getMin_ne() throws
    [[maybe_unused]] Node *removed = this->getMin_ne();
    assert(removed == heap_node);

    return arc;
  }

  /// Remove all heap nodes and delete their memory, after clearing the
  /// handles that the access functor keeps for them, so that their nodes
  /// can be put in a heap again. No arc is compared. The storage of the
  /// handles must still be alive, and the access functor, called for both
  /// nodes of each arc, must not throw: the destructor and the move
  /// assignment, which do not throw, empty the heap too. With the handles
  /// in a map, operator[] of std::unordered_map, for one, inserts the
  /// nodes that it does not find, and may throw std::bad_alloc.
  void empty()
  {
    this->for_each_in_preorder([this](Node *p) { handle_of(p) = nullptr; });
    this->remove_all_and_delete();
  }

  /** @brief Empties the container.

      @throws none
      @see empty()
  */
  void clear() { empty(); }

  /// Destructor. Clears all remaining heap nodes, as empty() does.
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

  // The handle that the access functor keeps for heap_node: that of the
  // node of its arc whose handle it is
  handle_type & handle_of(handle_type heap_node)
  {
    auto *arc = heap_node->data;
    auto & handle_src =
      handle_ref(static_cast<typename GT::Node *>(arc->src_node));
    if (handle_src == heap_node)
      return handle_src;

    auto & handle_tgt =
      handle_ref(static_cast<typename GT::Node *>(arc->tgt_node));
    assert(handle_tgt == heap_node);
    return handle_tgt;
  }

  // Clears the handles of the heap nodes in the circular list of first and
  // in the lists of their children. It goes down by child and up by parent,
  // without recursion, since a tree can be as high as the heap is large
  void clear_handles(handle_type first)
  {
    handle_type p = first;
    while (p != nullptr)
      {
        handle_of(p) = nullptr;
        if (p->child != nullptr)
          {
            p = p->child;
            continue;
          }

        // p has no children: go to the next node of its list or, if p is
        // the last one, to the next one of the list of its parent
        while (p != nullptr)
          {
            handle_type head = p->parent == nullptr ? first : p->parent->child;
            if (p->right != head)
              {
                p = p->right;
                break;
              }
            p = p->parent;
          }
      }
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

  /// Copying is disabled (heap manages internal node memory)
  ArcFibonacciHeap(const ArcFibonacciHeap &) = delete;
  ArcFibonacciHeap & operator=(const ArcFibonacciHeap &) = delete;

  /// Move construction transfers ownership of heap nodes
  ArcFibonacciHeap(ArcFibonacciHeap && other) noexcept
    : dist(std::move(other.dist)), access_node(std::move(other.access_node)),
      dist_cmp(dist), heap(std::move(other.heap))
  {}

  /// Move assignment transfers ownership of heap nodes, after emptying this
  /// heap as empty() does
  ArcFibonacciHeap & operator=(ArcFibonacciHeap && other) noexcept
  {
    if (this != &other)
      {
        empty();
        dist = std::move(other.dist);
        access_node = std::move(other.access_node);
        dist_cmp = Compare(dist);
        heap = std::move(other.heap);
      }
    return *this;
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
    handle_of(heap_node) = nullptr;

    (void) heap.extract_min();

    return arc;
  }

  /// Return true if the heap is empty.
  bool is_empty() const noexcept { return heap.is_empty(); }

  /// Remove all heap nodes and delete their memory, after clearing the
  /// handles that the access functor keeps for them, so that their nodes
  /// can be put in a heap again. No arc is compared. The storage of the
  /// handles must still be alive, and the access functor, called for both
  /// nodes of each arc, must not throw: the destructor and the move
  /// assignment, which do not throw, empty the heap too. With the handles
  /// in a map, operator[] of std::unordered_map, for one, inserts the
  /// nodes that it does not find, and may throw std::bad_alloc.
  void empty()
  {
    clear_handles(heap.get_min_node());
    heap.clear();
  }

  /** @brief Empties the container.

      @throws none
      @see empty()
  */
  void clear() { empty(); }

  /// Destructor. Clears all remaining heap nodes, as empty() does.
  ~ArcFibonacciHeap()
  {
    empty();
  }
};

# endif // ARCHEAP_H
