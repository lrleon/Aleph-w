

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

    void remove(T & data) noexcept
    {
      Node * node = BinHeap<T, Compare>::Node::key_to_node(data);
      BinHeap<T, Compare>::remove(node);
    }

    /// Sinónimo de remove.
    void erase(T & data) noexcept
    {
      remove(data);
    }

    /// Retorna una referencia modificable al menor elemento dentro
    /// del heap dinámico según el orden determinado por el criterio
    /// de comparación dado en la declaración.
    T & top() const
    {
      return BinHeap<T, Compare>::top()->get_key();
    }

    /// Vacía todos los elementos del heap dinámico.
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
    bool traverse(Operation & op) noexcept(noexcept(op))
    {
      return this->preorder_traverse([&op] (Node * p)
                                     { return op(p->get_key()); });
    }

    template <class Operation>
    bool traverse(Operation && op = Operation()) noexcept(noexcept(op))
    {
      return traverse(op);
    }

    template <class Operation>
    bool traverse(Operation & op) const noexcept(noexcept(op))
    {
      return this->preorder_traverse([&op] (Node * p)
                                     { return op(p->get_key()); });
    }

    template <class Operation>
    bool traverse(Operation && op = Operation()) const noexcept(noexcept(op))
    {
      return traverse<Operation>(op);
    }

    struct Iterator : public Base::Iterator
    {
      using Item_Type = T;
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



