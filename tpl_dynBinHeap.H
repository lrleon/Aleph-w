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
# ifndef TPL_DYNBINHEAP_H
# define TPL_DYNBINHEAP_H

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

      @ingroup Arboles
      @author Leandro Rabindranath León
  */
  template <class T, class Compare = Aleph::less<T>>
  class DynBinHeap : public BinHeap<T, Compare>,
                     public LocateFunctions<DynBinHeap<T, Compare>, T>,
                     public FunctionalMethods<DynBinHeap<T, Compare>, T>,
                     public GenericKeys<DynBinHeap<T, Compare>, T>,
                     public EqualToMethod<DynBinHeap<T, Compare>>,
                     public StlAlephIterator<DynBinHeap<T, Compare>>
  {
  public:

    using Set_Type = DynBinHeap;

    using Item_Type = T;

    using Key_Type = T;

  private:

    using Base = BinHeap<T, Compare>;

    using Node = typename BinHeap<T, Compare>::Node;

    T & __insert(Node * p) noexcept
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
      copy(h);
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
    */
    T getMin()
    {
      Node * node = BinHeap<T, Compare>::getMin();

      T return_value = std::move(node->get_key());

      delete node;

      return return_value;
    }

    /// \overload getMin() para usar si se invierte el orden de prioridad
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
        @note Behavior is undefined if @p data does not belong to this heap.
    */
    void update(T & data) noexcept
    {
      Node * node = Node::key_to_node(data);
      BinHeap<T, Compare>::update(node);
    }

    /** Remove an arbitrary element belonging to the heap.

        The reference must have been obtained from insert(). The underlying
        node is freed.

        @param data element to erase.
        @return true if a node was removed.
    */
    bool remove(T & data) noexcept
    {
      if (this->is_empty())
        return false;

      Node * node = BinHeap<T, Compare>::Node::key_to_node(data);
      Node * removed = BinHeap<T, Compare>::remove(node);
      if (removed == nullptr)
        return false;

      delete removed;
      return true;
    }

    /// Alias for remove().
    bool erase(T & data) noexcept
    {
      return remove(data);
    }

    /// Return a reference to the smallest element.
    T & top() const
    {
      return BinHeap<T, Compare>::top()->get_key();
    }

    /// Remove every element.
    void empty() noexcept
    {
      this->remove_all_and_delete();
    }

    /// Destructor.
    ~DynBinHeap()
    {
      empty();
    }

    template <class Operation>
    bool traverse(Operation & op)
    {
      return this->preorder_traverse([&op] (Node * p)
                                     { return op(p->get_key()); });
    }

    template <class Operation>
    bool traverse(Operation && op = Operation())
    {
      return traverse(op);
    }

    template <class Operation>
    bool traverse(Operation & op) const
    {
      return this->preorder_traverse([&op] (Node * p)
                                     { return op(p->get_key()); });
    }

    template <class Operation>
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



