
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

  /** Heap dinámico de elementos de tipo genérico T con criterio de
      comparación Compare.

      DynBinHeap instrumenta un heap dinámico de elementos de tipo T
      ordenados según criterio de comparación Compare()().

      Por dinámico se entiende que el heap puede crecer o disminuir en
      función de los recursos. Dicho de otro modo, no está supeditado
      por el uso de un arreglo interno.

      @ingroup Arboles
      @author Leandro Rabindranath León (lrleon en ula punto ve)
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

    /// El tipo de conjunto sobre el cual se itera.
    using Set_Type = DynBinHeap;

    /// El tipo de elemento que retorna get_curr().
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

    /** Inserta en el heap una copia de item.

        Inserta una copia de item en el heap dinámico.

        @param[in] item elemento a insertar en el heap.
        @return una referencia modificable al elemento copiado e
        insertado en el heap.
        @throw bad_alloc si no hay suficiente memoria.
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

    /** Inserta en el heap una copia de item.

        Sinónimo de insert();

        @param[in] item elemento a insertar en el heap.
        @return una referencia modificable al elemento copiado e
        insertado en el heap.
        @throw bad_alloc si no hay suficiente memoria.
    */
    T & put(const T & item)
    {
      return insert(item);
    }

    T & put(T && item)
    {
      return insert(std::forward<T>(item));
    }

    /** Elimina el menor elemento de un heap dinámico y retorna una
        copia.

        @return una copia del elemento eliminado
        @throw underflow_error si el heap está vacío.
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

    /** Elimina el menor elemento de un heap dinámico y retorna una
        copia.

        @return una copia del elemento eliminado
        @throw underflow_error si el heap está vacío.
    */
    T get()
    {
      return getMin();
    }

    /** Actualiza una prioridad en el heap.

        update(data) toma una referencia a un elemento del heap, cuya
        prioridad ha sido presumiblemente modificada, y actualiza su
        posición en el heap.

        @param[in] data referencia modificable a un elemento dentro del
        heap.

        @note La referencia debe ser válida en el sentido de que debe
        tratarse de una referencia retornada por insert(). No se hacen
        verificaciones al respecto.
    */
    void update(T & data) noexcept
    {
      Node * node = Node::key_to_node(data);
      BinHeap<T, Compare>::update(node);
    }

    /** Elimina un elemento arbitrario del heap.

        remove(data) elimina un elemento cualquiera que esté dentro
        del heap.

        `data` debe pertenecer al heap.

        Esta operación es útil cuando de alguna manera, según la
        aplicación, el elemento data pierda vigencia dentro del
        heap. Por ejemplo, la cancelación de un evento.

        @param data el elemento a eliminar.

        @note Resultados inesperados si data no pertenece al heap.
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



