
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


/** @file tpl_dynBinHeap.H
 *  @brief Dynamic binary heap with node-based storage.
 *
 *  Binary heap using linked nodes instead of array. Supports
 *  priority updates and arbitrary node removal.
 *
 *  ## Features
 *  - O(log n) insert, extract-min
 *  - O(log n) decrease-key
 *  - Node handles for updates
 *
 *  @see tpl_binHeap.H Array-based heap
 *  @see tpl_dynArrayHeap.H Array-based dynamic heap
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNBINHEAP_H
# define TPL_DYNBINHEAP_H

# include <memory>

# include <ah-concepts.H>
# include <ahDry.H>
# include <ah-args-ctor.H>
# include <htlist.H>
# include <ah-dry.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binHeap.H>

using namespace Aleph;

namespace Aleph {

  /** Dynamic heap of elements of type @p T ordered by a comparison functor.

      `DynBinHeap` stores items in a binary heap whose nodes are allocated
      dynamically (no backing array), so the structure can grow or shrink as
      needed.

      @ingroup Trees
      @author Leandro Rabindranath León
  */
  template <class T, class Compare = Aleph::less<T>>
    requires StrictWeakOrder<Compare, T>
  class DynBinHeap : public BinHeap<T, Compare>,
                     public LocateFunctions<DynBinHeap<T, Compare>, T>,
                     public FunctionalMethods<DynBinHeap<T, Compare>, T>,
                     public GenericKeys<DynBinHeap<T, Compare>, T>,
                     public StlAlephIterator<DynBinHeap<T, Compare>>
  {
  public:

    using Set_Type = DynBinHeap;

    using Item_Type = T;

    using Key_Type = T;

  private:

    using Base = BinHeap<T, Compare>;

    using Node = typename BinHeap<T, Compare>::Node;

    // What the comparison throws leaves the node in the heap, which may then
    // be out of order
    T & __insert(Node * p) noexcept(Base::nothrow_cmp)
    {
      return BinHeap<T, Compare>::insert(p)->get_key();
    }

    void copy(const DynBinHeap & src)
    {
      src.for_each_in_preorder([this] (Node * p)
                               {
                                 __insert(new Node (p->get_key()));
                               });
    }

  public:

    DynBinHeap(Compare & cmp) noexcept : Base(cmp) { /* empty */ }

    DynBinHeap(Compare && cmp = Compare()) noexcept : BinHeap<T, Compare>(cmp)
    { /* empty */ }

    DynBinHeap(const DynBinHeap & h) : Base(h.cmp)
    {
      try
        {
          copy(h);
        }
      catch (...)
        { // the destructor does not run for an object that is not built
          empty();
          throw;
        }
    }

    DynBinHeap(DynBinHeap && h) : Base(h.cmp)
    {
      this->swap(h);
    }

    Special_Ctors(DynBinHeap, T);

    Args_Ctor(DynBinHeap, T);

    DynBinHeap & operator = (const DynBinHeap & h)
    {
      if (this == &h)
        return *this;

      empty();

      copy(h);

      return *this;
    }

    DynBinHeap & operator = (DynBinHeap && h) noexcept
    {
      this->swap(h);
      return *this;
    }

    /** Insert a copy of @p item into the heap.

        @param[in] item value to insert.
        @return reference to the stored element.
        @throw bad_alloc if allocation fails.
        @throw anything that the comparison throws. Then the item is in the
        heap, but the heap may be out of order.
    */
    T & insert(const T & item)
    {
      return __insert(new Node (item));
    }

    T & insert(T && item)
    {
      return __insert(new Node (std::forward<T>(item)));
    }

    T & append(const T & item)
    {
      return __insert(new Node (item));
    }

    T & append(T && item)
    {
      return __insert(new Node (std::forward<T>(item)));
    }

    /** Synonym of insert(). */
    T & put(const T & item)
    {
      return insert(item);
    }

    T & put(T && item)
    {
      return insert(std::forward<T>(item));
    }

    /** Remove the minimum element (according to @p Compare) and return it.

        @return copy of the removed element.
        @throw underflow_error if the heap is empty.
        @throw anything that the comparison throws. Then the element is out
        of the heap and destroyed, and the heap may be out of order.
    */
    T getMin()
    {
      // The root leaves the heap before the keys are compared, so it is
      // freed even if the comparison throws; it is null in an empty heap,
      // for which Base::getMin() throws
      std::unique_ptr<Node> node(Base::getRoot());
      Base::getMin();
      return std::move(node->get_key());
    }

    /// \overload getMin() to use if the order of priority is reversed
    T getMax()
    {
      return getMin();
    }

    /** Alias for getMin(). */
    T get()
    {
      return getMin();
    }

    /** Adjust the position of an element after mutating its priority.

        @param[in] data reference previously returned by insert().
        @throw anything that the comparison throws. Then the heap may be out
        of order.
        @note Behavior is undefined if @p data does not belong to this heap.
    */
    void update(T & data) noexcept(Base::nothrow_cmp)
    {
      Node * node = Node::key_to_node(data);
      BinHeap<T, Compare>::update(node);
    }

    /** Remove an arbitrary element belonging to the heap.

        The reference must have been obtained from insert(). The underlying
        node is freed.

        @param data element to erase.
        @return true if a node was removed.
        @throw anything that the comparison throws. Then the element is out
        of the heap and destroyed, and the heap may be out of order.
    */
    bool remove(T & data) noexcept(Base::nothrow_cmp)
    {
      if (this->is_empty())
        return false;

      // The node leaves the heap before the keys are compared, so it is
      // freed even if the comparison throws
      std::unique_ptr<Node> node(Node::key_to_node(data));
      Base::remove(node.get());
      return true;
    }

    /// Alias for remove().
    bool erase(T & data) noexcept(Base::nothrow_cmp)
    {
      return remove(data);
    }

    /// Return a reference to the smallest element.
    const T & top() const
    {
      return BinHeap<T, Compare>::top()->get_key();
    }

    /// Remove every element.
    void empty() noexcept
    {
      this->remove_all_and_delete();
    }

    /** @brief Removes all elements from the heap.

        @throws none

        @note This method delegates to empty() and is noexcept.
    */
    void clear() noexcept { empty(); }

    /// Destructor.
    ~DynBinHeap()
    {
      empty();
    }

    template <class Operation>
      requires Aleph::PredicateWith<Operation &, T &>
    bool traverse(Operation & op)
    {
      return this->preorder_traverse([&op] (Node * p)
                                     { return op(p->get_key()); });
    }

    template <class Operation>
      requires Aleph::PredicateWith<Operation &, T &>
    bool traverse(Operation && op = Operation())
    {
      return traverse(op);
    }

    template <class Operation>
      requires Aleph::PredicateWith<Operation &, const T &>
    bool traverse(Operation & op) const
    {
      return this->preorder_traverse([&op] (Node * p)
                                     { return op(static_cast<const T &>(p->get_key())); });
    }

    template <class Operation>
      requires Aleph::PredicateWith<Operation &, const T &>
    bool traverse(Operation && op = Operation()) const
    {
      return traverse<Operation>(op);
    }

    struct Iterator : public Base::Iterator
    {
      using Item_Type = T;
      /// Default constructor creates an "end" iterator
      Iterator() noexcept = default;
      Iterator(const DynBinHeap & h)
        : Base::Iterator(h) {}
      const T & get_curr_ne() const noexcept
      {
        return KEY(Base::Iterator::get_curr_ne());
      }
      const T & get_curr() const { return KEY(Base::Iterator::get_curr()); }
    };
  };

} // end namespace Aleph

# endif // TPL_DYNBINHEAP_H



