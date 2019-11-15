/*
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
  2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef HTLIST_H
# define HTLIST_H

# include <type_traits>
# include <cassert>
# include <stdexcept>
# include <utility>
# include <ahFunction.H>
# include <ahFunctional.H>
# include <ah-args-ctor.H>
# include <ah-iterator.H>

# define NEXT(p) (p->get_next())

using namespace Aleph;

namespace Aleph {

  template <typename T> class Snodenc; // forward declaration

  /**

     \~English
     Link of a single linked list non circular and without header node.

     @ingroup Sequencies

     \~Spanish
     Enlace a una lista simple no circular y sin nodo cabecera

     @ingroup Secuencias
  */
  class Slinknc
  {
    Slinknc * next = nullptr;

  public:

    /// Return `true` if `this` is empty
    bool is_empty() const noexcept { return next == nullptr; }

    /**  \~English Init to nullptr
         \~Spanish Inicializa en nullptr
    */
    Slinknc() noexcept : next(nullptr) { /* empty */ }

    /** \~English Dummy copy constructor. Link is set to nullptr
        \~Spanish Constructor de copia simulado. El enlace es ajustado
        como nullptr
    */
    Slinknc(const Slinknc &) noexcept : next(nullptr) { /* empty */ }

    /** \~English Reset the link to nullptr
        \~Spanish Reajusta el enlace a nullptr
    */
    void reset() noexcept
    {
      next = nullptr;
    }

    /** \~English Dummy asignation; link is set to nullptr
        \~Spanish Asignación simulada; el enlace es ajustado como nullptr
    */
    Slinknc & operator = (const Slinknc & ) noexcept
    {
      next = nullptr;
      return *this;
    }

    ///\~English getter \~Spanish Observador
    Slinknc *& get_next() noexcept
    {
      return next;
    }

    ///\~English Insert `p` after `this` \~Spanish Inserta `p` luego de `this`

    /**
       \~English

       `insert(p)` inserts the node pointed by `p` after `this`.

       @param[in] p pointer to link (or node)

       \~Spanish

       `insert(p)` inserta el nodo apuntado por `p` luego de this

       @param[in] p apuntador al enlace (o nodo)
    */
    void insert(Slinknc * p) noexcept
    {
      assert(p != nullptr);
      p->next = next;
      next = p;
    }

    /**
       \~English
       Remove for linked list the node pointed by `this`

       @return a pointer to the removed node

       \~Spanish
       Elimina de la lista enlazada el nodo apuntado por `this`

       @return un apuntador al nodo eliminado
    */
    Slinknc * remove_next() noexcept
    {
      Slinknc * ret_val = next;
      next = ret_val->next;
      ret_val->reset();
      return ret_val;
    }

    /** Convert `this` to a `Snodenc<T>.

        @note Undefined and probably buggy behavior if `this` is not a
        valid pointer
    */
    template <typename T> inline
    Snodenc<T> * to_snodenc() noexcept;

    template <typename T> inline T & to_data() noexcept;

    template <typename T> inline
    const Snodenc<T> * to_snodenc() const noexcept;

    template <typename T> inline const T & to_data() const noexcept;

    /**
       \~English
       Iterator on single links

       \ingroup Sequencies

       \~Spanish
       Iterador sobre enlaces simples

       \ingroup Secuencias
    */
    class Iterator
    {
      Slinknc * head;
      Slinknc * curr;

      void init()
      {
        curr = head->is_empty() ? nullptr : head->next;
      }

    public:

      Iterator() noexcept : head(nullptr), curr(nullptr)
      {
        // Empty
      }

      /** \~English Initialize an iterator on the first node of `list`
          \~Spanish Inicializa un iterador en el primer nodo de `list`
      */
      Iterator(Slinknc & list) noexcept : head(&list)
      {
        init();
      }

      /**
         \~English

         Initialize an iterator on the list pointed by `_head`, but on a
         node pointed by `curr`

         \~Spanish

         Inicializa un iterador sobre la lista apuntada por `_head`, pero en
         un nodo apuntado por `curr`
      */
      Iterator(Slinknc * _head, Slinknc * _curr) noexcept
        : head(_head), curr(_curr)
      {
        // Empty
      }

      /**
         \~English
         Return `true` if the iterator is positioned on a valid link

         \~Spanish Retorna `true` si el iterador es posicionado en un enlace válido
      */
      bool has_current() const noexcept
      {
        return curr != nullptr;
      }

      /// \overload has_current()
      bool has_curr() const noexcept
      {
        return has_current();
      }

      /**
         \~English
         Return the current link on which the iterator is positioned.
         Throw `overflow_error` if there is no current link.

         \~Spanish
         Retorna el enlace actual sobre el cual el iterador está posicionado.
         Lanza la excepción `overflow_error` si no hay enlace actual.
      */
      Slinknc * get_current() const
      {
        if (not has_current())
          throw std::overflow_error("Iterator is at the end of the list");
        return curr;
      }

      /// \overload get_current()
      Slinknc * get_curr() const
      {
        return get_current();
      }

      /**
         \~English
         Move the iterator one position forward. Throw `overflow_error`
         if there is no current link.

         \~Spanish
         Mueve el iterador una posición más adelante. Lanza la excepción
         `overflow_error` si no hay link actual.
      */
      void next()
      {
        if (not has_current())
          throw std::overflow_error("Iterator is at the end of the list");
        curr = curr->next;
      }

      /** \~English Reset the iterator to the first link on the list
          \~Spanish Reestablece el iterador en el primer enlace
      */
      void reset_first() noexcept
      {
        init();
      }
    };
  };


  /**
     \~English
     Node belonging to a single non-circular linked list without header
     node.

     \ingroup Sequencies

     \~Spanish
     Nodo perteneciente a una lista simple no circular sin nodo cabecera.

     \ingroup Secuencias
  */
  template <typename T>
  class Snodenc : public Slinknc
  {
    T data;

  public:

    /** \~English Return a modifiable reference to the data
        \~Spanish Retorna una referencia modificable a la clave
    */
    T & get_data() noexcept { return data; }

    /** \~English Return a constant reference to the data
        \~Spanish Retorna una referencia constante a la clave
    */
    const T & get_data() const noexcept { return data; }

    Snodenc() noexcept(std::is_nothrow_constructible<T>::value)
    {
      static_assert(std::is_default_constructible<T>::value,
                    "No default constructor for T");
    }

    /** \~English Construct with copy of `item`
        \~Spanish Constructor con copia de `item`
    */
    Snodenc(const T & item) noexcept(noexcept(T(item))) : data(item)
    {
      static_assert(std::is_copy_constructible<T>::value,
                    "No copy constructor for T");
    }

    /** \~English Costruct by moving `item`
        \~Spanish Construye moviendo `ìtem`
    */
    Snodenc(T && item) noexcept(noexcept(std::swap(data, item)))
      : data(std::forward<T>(item))
    {
      static_assert(std::is_move_constructible<T>::value,
                    "No move constructor for T");
    }

    /**
       \~English
       Remove the node following to `this`.

       `remove_next()`removes the node linked after `this` and
       returns its address.

       @return a pointer to the removed node.

       \~Spanish
       Elimina el nodo siguiente a `this`

       `remove_next()` elimina el nodo enlazado luego de `this` y
       retorna su dirección.

       @return un puntero al nodo removido.
    */
    Snodenc * remove_next() noexcept { return (Snodenc*) Slinknc::remove_next(); }

    /** \~English Return the node following to `this`
        \~Spanish Retorna el nodo siguiente a `this`
    */
    Snodenc *& get_next() noexcept { return (Snodenc*&) Slinknc::get_next(); }

    /// \overload remove_next()
    Snodenc * remove_first() noexcept{ return Snodenc::remove_next(); }

    /**
       \~English
       Return the node following to `this`.

       In the case where `this` is a pointer to the first node, returns
       the first node.

       \return a pointer to the first node.

       \~Spanish
       Retorna el nodo siguiente a `this`.

       En el caso donde `this` sea un puntero al primer nodo, retorna
       el primer nodo.

       \return un puntero al primer nodo.
    */
    Snodenc *& get_first() const noexcept { return Snodenc::get_next(); }
  };

  template <typename T> inline
  Snodenc<T> * Slinknc::to_snodenc() noexcept
  {
    return static_cast<Snodenc<T>*>(this);
  }

  template <typename T> inline
  const Snodenc<T> * Slinknc::to_snodenc() const noexcept
  {
    return static_cast<const Snodenc<T>*>(this);
  }

  template <typename T> T & Slinknc::to_data() noexcept
  {
    return this->to_snodenc<T>()->get_data();
  }

  template <typename T> const T & Slinknc::to_data() const noexcept
  {
    return this->to_snodenc<T>()->get_data();
  }

  /**

     \~English

     Single linked list of nodes.

     HTList models a list of nodes of type `Slinknc`. A `Slinknc`
     object is a link that could be contained inside any data
     structure. The possibility for belonging to a `HTList is defined
     for the owner of one o more objects of type `Slinknc`.

     A `HTlist` object maintains two references. The first one is
     called `head`" and corresponds to the first node. The second one
     is called `tail` and corresponds to last node.

     Take in account that this class does not manage memory neither
     access to the data that could be stored in the nodes.
     Slinknc y Snodenc. However, since this class is inherited by
     other more sophisticated classes, we will use the term "item"
     instead of node for referring an element of this list of nodes.

     @see Slinknc Snodenc DynList
     @author Alejandro Mujica
     @author Leandro Rabindranath León
     @ingroup Secuencias

     \~Spanish

     Lista de nodos simplemente enlazadas

     HTList modela una lista de nodos del tipo `Slinknc`. Un objeto
     `Slinknc` es un enlace que puede ser añadido en cualquier
     estructura de datos. La posibilidad de pertenecer a una
     `HTlist` es definida por el usuario de uno o más objetos del tipo
     `Slinknc`.

     Un objeto `HTlist` mantiene dos referencias. La primera es llamada
     `head` y corresponde al primer nodo. La segunda es llamda `tail` y
     corresponde al último nodo.

     Tome en cuenta que esta clase no maneja memoria ni tampoco acceso a
     la data que puede ser almacenada en los nodos.
     Slinknc y Snodenc. Sin embargo, ya que esta clase es heredada por otras
     clases más sofisticadas, usaremos el término "elemento" en vez de nodo
     para referirnos a un nodo particular.

     @see Slinknc Snodenc DynList
     @author Alejandro Mujica
     @author Leandro Rabindranath León
     @ingroup Secuencias
  */
  class HTList
  {
    Slinknc * head;
    Slinknc * tail;

  public:

    //!\~English Initialize an empty list \~Spanish Inicializa una lista vacía
    HTList() noexcept : head(nullptr), tail(nullptr) { /* empty */ }

    void reset()
    {
      head = tail = nullptr;
    }

    //!\~English Return true if list is empty \~Spanish Retorna true
    //!si la lista está vacía.
    bool is_empty() const noexcept { return head == nullptr; }

    //!\~English Return true if the list contains exactly just one
    //!element \~Spanish Retorna true si la lista contiene
    //!exactamente un solo elemento.
    bool is_unitarian() const noexcept
    {
      return head != nullptr and head == tail;
    }

    //!\~English Return true if list contains one element or is empty
    //!\~Spanish Retorna true si la lista tiene un elemento o está vacía.
    bool is_unitarian_or_empty() const noexcept { return head == tail; }

    //!\~English Return list head (first element) \~Spanish Retorna la
    //!cabeza de la lista (el primer elemento)
    Slinknc * get_head() const noexcept { return head; }

    //!\~English Return list tail (first element) \~Spanish Retorna la
    //!cola de la lista (el último elemento)
    Slinknc * get_tail() const noexcept { return tail; }

    //!\~English Return list first element \~Spanish Retorna el primer
    //!elemento de la lista.
    Slinknc * get_first() const noexcept { return get_head(); }

    /// Return the last item of the list (`nullptr` if the list is empty)
    Slinknc * get_last() const noexcept { return get_tail(); }

    //!\~English Exchange 'this' values with another list \~Spanish
    //!Intercambia los valores de this con otra lista

    /**
       \~English
       \brief  Swap in constant time (very fast) 'this' elements with 'l'
       list elements <br>
       Referenced by append(), insert(), reverse() and split_list().

       \param [in] l New list which elements will be exchanged

       \~Spanish

       \brief  Intercambia en tiempo constante (muy rápido) los elementos
       de this con los de l
       \brief Referenciado por append(), insert(), reverse() y split_list().

       \param [in] l Lista nueva con la cual se intercambiarán los elementos

    */
    HTList & swap(HTList & l) noexcept
    {
      std::swap(head, l.head);
      std::swap(tail, l.tail);
      return *this;
    }

    //!\~English Insert link as first element \~Spanish Inserta link como
    //!primer elemento.

    /**
       \~English
       \param [in] link New element that will be inserted

       \~Spanish
       \param [in] link Nuevo elemento que será insertado
    */
    void insert(Slinknc * link) noexcept
    {
      assert(NEXT(link) == nullptr);

      if (head == nullptr)
        {
          assert(tail == nullptr);
          head  = tail = link;
          return;
        }

      NEXT(link) = head;
      head = link;
    }

    //!\~English Insert link as last element \~Spanish Inserta link como
    //!último elemento.

    /**
       \~English
       \param [in] link New element that will be inserted

       \~Spanish
       \param [in] link Nuevo elemento que será insertado
    */
    void append(Slinknc * link) noexcept
    {
      assert(link != nullptr and NEXT(link) == nullptr);

      if (head == nullptr)
        {
          assert(tail == nullptr);
          head = tail = link;
          return;
        }

      NEXT(tail) = link;
      tail = link;
    }

    //!\~English Join 'this' with 'l' through list end \~Spanish Une
    //!'this' con 'l' a través del final de la lista

    /**
       \~English
       \param [in] l List that will be inserted through list end

       \~Spanish
       \param [in] l Lista que será insertada  través del final de la lista
    */
    void append(HTList & l) noexcept
    {
      if (l.is_empty())
        return;

      if (this->is_empty())
        {
          this->swap(l);
          return;
        }

      NEXT(tail) = l.head;
      tail = l.tail;
      l.head = l.tail = nullptr;
    }

    //!\~English Insert link as last element \~Spanish Inserta link como
    //!último elemento.
    void put(Slinknc * link) noexcept { append(link); }

    //!\~English Concat to 'this' all 'l' list; 'l' becomes empty
    //!\~Spanish Concatena a this toda la lista l; l deviene vacía.
    void concat(HTList & l) noexcept { append(l); }

    /// \overload concat(HTList & l)
    void concat_list(HTList & l) noexcept { append(l); }

    //!\~English Insert starting in link (contained in 'this' list) the
    //!'list' list. 'list' becomes empty \~Spanish Inserta a partir de
    //!link (contenido en la lista this) la lista list. list deviene vacía

    /**
       \~English
       \param [in] link Element where 'list' will start
       \param [in] list List that will be inserted after link

       \~Spanish
       \param [in] link Elemento donde 'list' comenzará
       \param [in] list Lista que será insertada luego de link
    */
    void insert(HTList & l) noexcept
    {
      l.append(*this);
      this->swap(l);
    }

    /**
       \~English
       Insert a `list` into `this` after one of its items.

       `insert(link, list)` assumes that `link` points to a item of
       `this` and inserts in constant time `list` just after the item
       pointed by `link`. The order of `list` is not altered. but `list`
       becomes empty after insertion.

       Suppose the following situation

       this-->t1-->t2-->t3-->t4-->t5-->t6-->t7-->t8
       ^
       |
       link

       l-->l1-->l2-->l3-->l4

       Then `insert(link, list)` produces:

       this-->t1-->t2-->t3-->t4-->l1-->l2-->l3-->l4-->t5-->t6-->t7-->t8

       l becomes empty

       \param[in] link to the item after one wants to insert `list`
       \param[in] list to be inserted after item pointed by `link`

       \~Spanish
       Inserta una `list` en `this` después de uno de sus nodos.

       `insert(link, list)` asume que `link` apunta a un elemento de
       `this` e inserta en tiempo constante `list` justo después del objeto
       apuntado por`link`. El orden de `list` no es alterado, pero `list`
       se vacía luego de la inserción.

       Supongamos la siguiente situación

       this-->t1-->t2-->t3-->t4-->t5-->t6-->t7-->t8
       ^
       |
       link

       l-->l1-->l2-->l3-->l4

       Entonces, `insert(link, list)` produce:

       this-->t1-->t2-->t3-->t4-->l1-->l2-->l3-->l4-->t5-->t6-->t7-->t8

       l se vacía

       \param[in] link enlace al elemento tras del cual se quiere insertar `list`
       \param[in] list lista a insertar tras el elemento apuntado por `link`
    */
    void insert(Slinknc * link, HTList & list) noexcept
    {
      NEXT(link) = list.head;
      tail = list.tail;
      list.head = list.tail = nullptr;
    }

    //!\~English It deletes head element (first one). Return deleted
    //!element \~Spanish Elimina el elemento en la cabeza (el
    //!primero). Retorna el elemento eliminado.
    Slinknc * remove_head()
    {
      if (is_empty())
        throw std::underflow_error("HTList is empty");

      Slinknc * ret_val = head;
      if (head == tail)
        head = tail = nullptr;
      else
        {
          head = NEXT(head);
          if (head == nullptr)
            tail = nullptr;
        }

      ret_val->reset();

      return ret_val;
    }

    /**
       \~English
       Remove the header item from the list.

       \overload remove_head()

       \return a pointer to the removed item
       \throw underflow_error if the list is empty

       \~Spanish
       Elimina el elemnto cabecera de la lista.

       \overload remove_head()

       \return un puntero al elemento eliminado
       \throw underflow_error si la lista está vacía
    */
    Slinknc * remove_first() { return remove_head(); }

    /**
       \~English
       Remove from the list the item pointed by `link`

       `remove(link)` perform a sequential traversal of `this` until find
       `link`. Then `link` is removed.

       This method has a complexity of \f$O(n)\f$ for worst and average
       case.

       \param[in] link pointer to the item to be removed.
       \return `true` if `link` was found and removed: `false` otherwise
       \throw underflow_error if the list is empty

       \~Spanish
       Elimina de la lista el elemento apuntado por `link`

       `remove(link)` realiza un recorrido secuencial sobre `this` hasta
       encontrar`link`. Luego `link` es removido.

       Este método tiene una complejidad de \f$O(n)\f$ para el peor y para
       el caso esperado.

       \param[in] link apuntador al elemento que será removido.
       \return `true` si `link` si fue encontrado y removido: `false` si no lo fue
       \throw underflow_error si la lista está vacía
    */
    bool remove(Slinknc * link)
    {
      if (is_empty())
        throw std::underflow_error("Removing from a empty list");

      if (link == head)
        {
          head = NEXT(head);
          link->reset();
          return true;
        }

      for (Slinknc * prev = head, * p = NEXT(head); p != nullptr;
           prev = p, p = NEXT(p))
        if (p == link)
          {
            NEXT(prev) = NEXT(p);
            if (link == tail)
              tail = prev;
            link->reset();
            return true;
          }

      return false;
    }


    //!\~English Insert link as first element \~Spanish Inserta link como
    //!primer elemento.
    void push(Slinknc * link) noexcept { insert(link); }

    /**
       \~English
       Remove the first item of `this`.

       \overload remove_head()

       This synonym is adequate when `this` is dealed as a stack.

       \return a pointer to the removed item.
       \throw underflow_error if the list is empty

       \~Spanish

       Elimina el primer elemento de `this`.

       \overload remove_head()

       Este sinónimo se adecúa cuando`this` es tratado como una pila.

       \return un apuntador al elemnto eliminado.
       \throw underflow_error si la lista está vacía
    */
    Slinknc * pop()
    {
      if (is_empty())
        throw std::underflow_error("HTList as stack is empty");
      return remove_head();
    }

    Slinknc * top()
    {
      if (is_empty())
        throw std::underflow_error("HTList as stack is empty");
      return get_first();
    }

    //!\~English It divides 'this' into two equal lists without modifying
    //!elements order \~Spanish Divide this en dos listas equitativas sin
    //!alterar el orden de los elementos.

    /**
       \~English
       Split the list in two.

       This method takes the first n/2 items of `this` and puts them, in
       the same order, in list `l`. The remainder items are put in list
       `r`. After operation `this` becomes empty. The order of items is
       preserved through `l` and `r`.

       \param[out] l list containg the first n/2 first items
       \param[out] r list containg the last n/2 first items
       \return the total of items that has `this`

       \~Spanish
       Divide la lista en dos.

       Este método toma los primeros n/2 elementos de `this` y los coloca,
       en el mismo orden, en la lista `l`. Los elemntos restantes son colocados
       en la lista `r`. Después de esta operación `this` se vacía. El orden de
       los elementos es preservado a través de 'r' y 'l'.

       \param[out] l una lista conteniendo los primeros n/2 elementos
       \param[out] r una lista conteniendo los últimos n/2 elementos
       \return El total de elemntos que posee `this`

    */
    size_t split_list(HTList & l, HTList & r) noexcept
    {
      assert(l.is_empty() and r.is_empty()); // l y r deben estar vacías

      if (is_empty())
        return 0;

      if (is_unitarian())
        {
          swap(l);
          return 1;
        }

      size_t count = 0;
      Slinknc * p = head;
      Slinknc * q = head;
      while (q != tail and q != nullptr)
        {
          q = NEXT(q); ++count;
          if (q == tail or q == nullptr)
            break;

          p = NEXT(p);
          q = NEXT(q); ++count;
        }

      l.head = head;
      l.tail = p;

      r.head = NEXT(p);
      r.tail = tail;

      head = tail = nullptr;

      return count;
    }

    /// \overload split_listHTList & l, HTList & r)
    size_t split(HTList & l, HTList & r) noexcept
    {
      return split_list(l, r);
    }

    //!\~English It inverts all list elements. It returns list size
    //!\~Spanish Invierte los elementos de la lista. Retorna el tamaño
    //!de la lista.
    size_t reverse() noexcept
    {
      HTList tmp;

      size_t count = 0;
      while (not is_empty())
        {
          tmp.insert(remove_first());
          ++count;
        }

      swap(tmp);

      return count;
    }

    /// \overload reverse()
    size_t reverse_list() noexcept
    {
      return reverse();
    }

    //!\~English It cuts 'this' over 'link' element and it puts all
    //!remaining elements \~Spanish Corta this por el elemento link y
    //!coloca los elementos restantes (los que están después de link) en
    //!list

    /**
       \~English
       \brief  It makes reference to is_empty(). <br>
       Referenced by cut_list().
       \param [in] link Element where 'list' will be cut.
       \param [in] list List that will be cut in link

       \~Spanish
       \brief Hace referencia a is_empty(). <br>
       Referenciado por cut_list().
       \param [in] link Elemento donde 'list' comenzará
       \param [in] list Lista que será insertada luego de link
    */
    void cut(Slinknc * link, HTList & list) noexcept
    {
      assert(list.is_empty());

      list.head = NEXT(link);
      list.tail = tail;

      tail = link;
      NEXT(link) = nullptr;
    }

    /// \overload cut(Slinknc * link, HTList & list)
    void cut_list(Slinknc * link, HTList & list) noexcept { cut(link, list); }

    /**
       \~English
       Remove and free memory for all the items of list.

       `remove_all_and_delete()` remove each item of the list and call to
       `delete` operator on the removed item. At the end of call, all the
       items were removed, all the memory freed qand the list emptied.

       \warning This method only has sense if the items of list were
       dynamically allocated with `new`. Although That is very frequently
       the case, there are some exceptions. So, be sure that the items
       were allocated with `new` operator.

       \~Spanish
       Elimina y libera la memoria de todos los elemntos de la lista.

       `remove_all_and_delete()` elimina cada elemento de la lista y llama al
       operador `delete` en el elemnto removido. Al final del llamado, todos los
       elementos fueron removidos, toda la memoria liberada y la lista vaciada.

       \warning Este método solo tiene sentido si todos los elemntos de la lista
       fueron reservados con `new`. Aunque es muy frecuente el caso, existen
       excepciones. Entonces, asegúrese de que los elemntos fueron reservados
       con el poerador `new`.

    */
    void remove_all_and_delete() noexcept
    {
      while (not is_empty())
        delete remove_head();
    }

    /**
       \~English
       Iterator on HTList.

       \ingroup Sequencies

       \~Spanish
       Iterador sobre HTList

       \ingroup Secuencias
    */
    class Iterator
    {
      HTList * lptr = nullptr;
      Slinknc * curr = nullptr;
      Slinknc * prev = nullptr;

      long pos = 0;

    public:

      //!\~English Initialize iterator on at the first item of `list`
      //!\~Spanish Inicializa un iterador sobre el primer elemento de
      //!`list` 
      Iterator(const HTList & list) noexcept
        : lptr(& (HTList&) list), curr(lptr->head), prev(curr)
      { /* empty */ }

      //!\~English Reset the iterator at the first item \~Reestablece el
      //!iterador al primer elemento 
      void reset() noexcept
      {
        prev = curr = lptr->head;
        pos = 0;
      }

      //!\~English Return the current position \~Spanish Retorna la
      //!posición actual 
      long get_pos() const noexcept { return pos; }

      /// \overload reset()
      void reset_first() noexcept { reset(); }

      //!\~English Not implemented. Put only for compilation \~Spanish
      //!No implementada. Colocada solo para compilación 
      void reset_last()
      {
        throw std::domain_error("reset_last() not implemented for DynList");
      }

      //!\~English Set the iterator to its end position, which has not
      //!current item. \~Spanish Ajusta el iterador a su posición final
      void end() noexcept
      {
        curr = nullptr;
      }

      // //!\~English Assignation \~Spanish Asignación
      // Iterator & operator = (const Iterator & it) noexcept
      // {
      //   lptr = it.lptr;
      //   curr = it.curr;
      //   prev = it.prev;
      //   return *this;
      // }

      //!\~English Return `true` if iterator has current item \~Spanish
      //!Retorna `true` si el iterador tiene un elemento actual
      bool has_curr() const noexcept { return curr != nullptr; }

      /// @overload has_curr()
      bool has_current() const noexcept { return has_curr(); }

      bool is_last() const noexcept
      {
        return lptr->is_empty() ? false : curr == lptr->tail;
      }

      ///  Return `true` if the iterator is positioned on the first item
      bool is_in_first() const noexcept
      {
        return lptr->is_empty() ? false : curr == lptr->head;
      }

      ///  Return `true` if the iterator is positioned on the last item
      bool is_in_last() const noexcept { return is_last(); }

      /**
         \~English
         Return the current node on which the iterator is positioned

         @throw overflow_error if iterator has not current node

         \~Spanish
         Retorna el nodo actual sobre el cual está posicionado el iterador

         @throw overflow_error si el iterador no tiene nodo actual
      */
      Slinknc * get_curr() const
      {
        if (curr == nullptr)
          throw std::overflow_error("Iterator overflow");

        return curr;
      }

      /// \overload get_curr()
      Slinknc * get_current() const { return get_curr(); }

      /**
         \~English
         Move the iterator one item forward. Throw overflow_error if
         iterator has not current item

         \~Spanish
         Mueve el iterador un elemento más adelante. Lanza overflow_error
         si el iterador no tiene elemento actual.
      */

      void next()
      {
        if (not has_curr())
          throw std::overflow_error("Iterator overflow");

        if (curr == lptr->head) // on the first item?
          {
            assert(prev == lptr->head);
            curr = NEXT(curr);
          }
        else if (curr == lptr->tail) // on the last item?
          {
            assert(NEXT(prev) == curr);
            curr = nullptr;
          }
        else
          {
            assert(NEXT(prev) == curr);
            prev = curr;
            curr = NEXT(curr);
            assert(NEXT(prev) == curr);
          }
        pos++;
      }

      /**
         \~English
         Remove the current item.

         `del()` removes the current item of iterator and advaces it a
         position forward.

         \return a valid pointer to the removed item
         \throw overflow_error if iterator has not current item

         \~Spanish
         Remueve el elemento actual.

         `del()` elimina el elemento actual del iterador y lo desplaza
         una posición más adelante.

         \return un puntero válido al objeto eliminado
         \throw overflow_error si el iterador no tiene elemento actual
      */
      Slinknc * del()
      {
        if (not has_curr())
          throw std::overflow_error("Iterator overflow");

        Slinknc * ret_val = nullptr;
        if (curr == lptr->head) // first item removal
          {
            ret_val = lptr->remove_first();
            prev = curr = lptr->head;
          }
        else if (curr == lptr->tail) // last item removal
          {
            assert(NEXT(prev) == curr);
            ret_val = curr;
            NEXT(prev) = NEXT(curr);
            lptr->tail = prev;
            curr = nullptr; // put in overflow
          }
        else
          {
            assert(NEXT(prev) == curr);
            ret_val = curr;
            NEXT(prev) = NEXT(curr);
            curr = NEXT(curr);
          }

        ret_val->reset();
        return ret_val;
      }
    };

    /**
       \~English

       Count the number of elements of the list.

       This method counts, it does not retrieve, the number of elements
       stored in the list.

       So it is complexity is \f$O(n)\f$. This is some polemic because one
       could maintain an internal counter and retrieve it in constant
       time. It possible that this feauture is put in next versions. We
       do not maintain this counter because it is possible to add or
       remove items from a given node. So these operations would
       require access to the full list's context, what it often not the
       case.

       \return the number of items of list

       \~Spanish

       Cuenta el número de elemntos de la lista.

       Este método cuenta, más no recupera, el número de elemntos en la lista.

       Entonces, su complejidad es \f$O(n)\f$. Esto es algo polémico porque
       uno podría mantener un contador interno y recuperarlo en un tiempo
       constante. Es posible que esta característica sea puesta en próximas
       versiones. Nosotros no mantenemos este contador porque es posible
       añadir o remover elemntos de un nodo dado. Entonces, estas operaciones
       requerirían acceso al contexto de la lista completa, lo cual no es
       el caso frecuentemente.

       \return el número de elementos de la lista
    */
    size_t size() const noexcept
    {
      size_t count = 0;
      for (Iterator it(*this); it.has_curr(); it.next())
        ++count;

      return count;
    }

    /**
       \~English
       Rotate to left the list n positions.

       `rotate_left(n)` rotates the items to left `n` positions. For
       example, if the list is as follows:

       l0, l1, l2, l3, l4, l5, l6, l7, l8, l9

       Then `rotate_left(4)` produces the following state:

       l4, l5, l6, l7, l8, l9, l0, l1, l2, l3

       \param[in] n the number of position to be rotated
       \throw domain_error if list is empty

       \~Spanish
       Rota a la izquierda la lista n posiciones.

       `rotate_left(n)` rota los elementos a la izquierda `n` posiciones.
       Por ejemplo, si la lista es así:

       l0, l1, l2, l3, l4, l5, l6, l7, l8, l9

       Entonces `rotate_left(4)` produce el siguiente estado:

       l4, l5, l6, l7, l8, l9, l0, l1, l2, l3

       \param[in] n el número de posiciones a rotar
       \throw domain_error si la lista está vacía
    */
    void rotate_left(size_t n)
    {
      if (is_empty())
        if (n == 0)
          return;
        else
          throw std::domain_error("List is empty");

      for (size_t i = 0; i < n; ++i)
        append(remove_first());
    }
  };

# include <ah-dry.H>

  /**
     \~English
     Sequence of items implemented with a single linked list.

     `DynList<T>` models list of items of generic type T.

     @see Slinknc Snodenc DynDList
     @author Alejandro Mujica
     @author Leandro Rabindranath León
     @ingroup Sequencies

     \~Spanish
     Secuencia de elementos implementada con una lista simple.

     `DynList<T>` modela una lista de elementos del tipo genérico T.

     @see Slinknc Snodenc DynDList
     @author Alejandro Mujica
     @author Leandro Rabindranath León
     @ingroup Secuencias
  */
  template <typename T = int>
  class DynList : public HTList,
                  public GenericTraverse<DynList<T>>,
                  public LocateFunctions<DynList<T>, T>,
                  public SpecialCtors<DynList<T>, T>,
                  public FunctionalMethods<DynList<T>, T>,
                  public GenericItems<DynList<T>, T>,
                  public StlAlephIterator<DynList<T>>
  {
    using CtorBase = SpecialCtors<DynList<T>, T>;

    using CtorBase::CtorBase;

  public:

    //!\~English The type of item \~Spanish el tipo de elemento
    using Item_Type = T;

    //!\~English The type of item \~Spanish el tipo de elemento
    using Key_Type = T;

    /**
       \~English
       Swap this with l. All items of each list are swapped in constant
       time

       \~Spanish
       Intercambia this con l. Todos los elementos de cada lista son
       intercambiados en tiempo constante.
    */
    DynList & swap(DynList & l) noexcept
    {
      return (DynList&) HTList::swap(l);
    }

    //!\~English Initialize an empty list \~Spanish Inicializa una lista vacía
    DynList() noexcept(std::is_nothrow_constructible<T>::value) { /* empty */ }

    //!\~English  Initialize a list with a copy of all the items of list
    //!`l` \~Spanish Inicializa una lista con una copia de todos los
    //!elementos de la lista
    DynList(const DynList & l) : HTList(), CtorBase(l) {}

  private:

    T & __insert(Snodenc<T> * p) noexcept
    {
      static_assert(std::is_copy_constructible<T>::value or
                    std::is_move_constructible<T>::value,
                    "No copy assign for T");
      HTList::insert(p);
      return p->get_data();
    }

    T & __append(Snodenc<T> * p) noexcept
    {
      static_assert(std::is_copy_constructible<T>::value or
                    std::is_move_constructible<T>::value,
                    "No copy assign for T");
      HTList::append(p);
      return p->get_data();
    }

  public:

    /**
       \~English
       Insert a new item by copy.

       Allocate memory for a new item, copy and insert into the list as
       the first item.

       \return a modifiable reference to the item in the list
       \throw  `bad_alloc` if there is no enough memory

       \~Spanish
       Inserta un nuevo elemento por copia.

       Asigna memoria para un nuevo elemento, lo copia y lo inserta como el
       primer elemento.

       \return una referencia modificable al elemento en la lista
       \throw `bad_alloc` si no hay suficiente memoria
    */
    T & insert(const T & item)
    {
      return __insert(new Snodenc<T> (item));
    }

    /**
       \~English
       Insert a new item by movement.

       Allocate memory for a new item, move the item and insert into the
       list as the first item. This operation can be faster that the
       equivalent with copy

       \return a modifiable reference to the item in the list
       \throw  `bad_alloc` if there is no enough memory

       \~Spanish
       Inserta un nuevo elemento por movimiento.

       Asigna memoria para un nuevo elemento, mueve el elemento y lo inserta
       en la lista como el primer elemento. Esta operación puede ser más
       rápida que la equivalente con copia.

       \return una referencia modificable al objeto en la lista
       \throw `bad_alloc` si no hay suficiente memoria
    */
    T & insert(T && item)
    {
      return __insert(new Snodenc<T> (std::forward<T>(item)));
    }

    /// \overload insert(const T & item)
    T & push(const T & item)
    {
      return insert(item);
    }

    /// \overload insert(T && item)
    T & push(T && item) { return insert(std::forward<T>(item)); }

    /// \overload insert(const T & item)
    T & put(const T & item) noexcept(noexcept(T(item)))
    {
      return push(item);
    }

    /// \overload insert(T && item)
    T & put(T && item) noexcept(noexcept(std::forward<T>(item)))
    {
      return push(std::forward<T>(item));
    }

    /**
       \~English
       Append a new item by copy.

       Allocate memory for a new item, copy and append it at the end of
       the list.

       \return a modifiable reference to the item in the list
       \throw  `bad_alloc` if there is no enough memory

       \~Spanish
       Mete un nuevo elemento a la lista por copia.

       Asigna memoria para un nuevo elemento, copia y lo inserta al final
       de la lista.

       \return una referencia modificable al elemento en la lista
       \throw `bad_alloc` si no hay suficiente memoria
    */
    T & append(const T & item)
    {
      return __append(new Snodenc<T> (item));
    }

    /**
       \~English
       Append a new item by movement.

       Allocate memory for a new item, move to its allocated place and
       append it at the end of the list.

       \return a modifiable reference to the item in the list
       \throw  `bad_alloc` if there is no enough memory

       \~Spanish
       Inserta un nuevo elemento por desplazamiento.

       Asigna memoria para el nuevo elemnto, lo mueve a su lugar asignado
       y lo inserta al final de la lista.

       \return una referencia modificable al elemento en la lista
       \throw `bad_alloc` si no hay suficiente memoria
    */
    T & append(T && item)
    {
      return __append(new Snodenc<T> (std::forward<T>(item)));
    }

    /** Remove the first item of the list.

        \return a copy of removed item
        \throw underflow_error if the list is empty
    */
    T remove()
    {
      Slinknc * l = this->HTList::remove_head();
      Snodenc<T> * p = static_cast<Snodenc<T>*> (l);
      T ret_val = std::move(p->get_data());
      delete p;

      return ret_val;
    }

    /// \overload remove()
    T remove_first()
    {
      return remove();
    }

    /// \overload remove()
    T pop() { return remove_first(); }

    /// \overload remove()
    T get() { return remove(); }

    /** Return the last item of the list

        \return a modifiable reference to the last item
        \throw underflow_error if the list is empty
    */
    T & get_last() const
    {
      if (is_empty())
        throw std::underflow_error("List is empty");

      Snodenc<T> * p = static_cast<Snodenc<T>*> (this->HTList::get_last());
      return p->get_data();
    }

    /** Return the first item of the list

        \return a modifiable reference to the first item
        \throw underflow_error if the list is empty
    */
    T & get_first() const
    {
      if (is_empty())
        throw std::underflow_error("List is empty");
      return static_cast<Snodenc<T>*>(this->HTList::get_first())->get_data();
    }

    /// \overload get_first()
    T & top() const
    {
      return get_first();
    }

    /// empty the list
    void empty() noexcept
    {
      if (is_empty())
        return;

      // a very fast deletion
      Snodenc<T> * last = static_cast<Snodenc<T>*> (this->HTList::get_last());
      Snodenc<T> * p = static_cast<Snodenc<T>*> (this->HTList::get_first());
      while (p != last)
        {
          Snodenc<T> * q = p;
          p = p->get_next();
          delete q;
        }
      delete p;
      reset();
    }

    ~DynList() { empty(); }

    /** Iterator on the items of list

        \ingroup Secuencias
    */
    class Iterator : public HTList::Iterator
    {
    public:

      using Item_Type = T; /// The type of elements of list

      using Iterator_Type = Iterator;

      using Set_Type = DynList; /// The type of container

      Iterator() noexcept { /* empty */ }

      /// Initiliaze an iterator on the first item of `list`
      Iterator(const DynList & list) noexcept
        : HTList::Iterator(list) { /* empty */ }

      /** Return the current item

          \return a modifiable reference to the current item
          \throw overflow_error if the iterator has not current item
      */
      T & get_curr() const
      {
        return ((Snodenc<T>*) (HTList::Iterator::get_curr()))->get_data();
      }

      /// \overload get_curr()
      T & get_current() const
      {
        return get_curr();
      }

      /** Remove the current item of the iterator.

          `del()` removes the current item and advance forward the
          iterator to the next item.

          \return a moved copy of removed item (moved because the original
          will be destroyed)
          \throw overflow_error if the iterator has not current item
      */
      T del()
      {
        Snodenc<T> * p = (Snodenc<T> *) this->HTList::Iterator::del();
        T ret_val = std::move(p->get_data());
        delete p;
        return ret_val;
      }
    };

    DynList & reverse() noexcept
    {
      reverse_list();
      return *this;
    }

    DynList & rev() noexcept { return reverse(); }

    /// Return a reversed copy of `this`. Not confuse with reverse
    /// without `const` which mutates `this`
    DynList reverse() const
    {
      DynList ret;
      for (auto it = this->get_it(); it.has_curr(); it.next())
        ret.insert(it.get_curr());

      return ret;
    }

    DynList rev() const { return reverse(); }

    /** Remove the first element matching an equality criteria.

        `remove(eq)` sequentually traverses the list and on each current
        element `curr` it calls to `eq(curr)`. If the test is `true`,
        then `curr` is removed from the list and a copy of `ciirr` is
        returned. Otherwise, if no element matches with `eq()` test,
        then exception `domain_error` is thrown.

        The `eq()` must match the following signature:

        bool eq(const T& op)

        This test would return `true` when `op` satisfies the finding
        criteria`.

        Since that this method is overloaded, you could define several
        flavors: a more sophisticated functor, function pointers or
        lambdas.

        \param[in] eq equalitiy test
        \return a copy of removed element if the item was found and
        removed; otherwise the exception `domain_error` is thrown.
    */
    template <class Equal = std::equal_to<T>>
    T remove(Equal & eq)
    {
      for (Iterator it(*this); it.has_curr(); it.next())
        {
          const T & item = it.get_curr();
          if (eq(item))
            {
              T ret = item; // performs a copy
              it.del();
              return ret;
            }
        }

      throw std::domain_error("DynList::remove(Equal &): item not found");
    }

    /// \overload remove(const T & item, Equal & eq)
    template <class Equal = std::equal_to<T>>
    T remove(Equal && eq)
    {
      return remove<Equal>(eq);
    }

    /** Initialize the list with all the items of `l` moved to `this`.

        This constructor copies (by moving) all the items of list
        `l`. Consequently, no copy is done and the construction takes
        \f$O(1)\f$.

        \note Since `l` is a rvalue reference, take care of that if you are
        interested in to avoid the copy. So, if you have a lvalue
        referencde to a list, use `std::move()`, upon your responsability,
        if and only if you are absolutely sure that the list will not be
        needed after.

        \param[in] l a rvalue containing a entire list.
    */
    DynList(DynList && l) noexcept
    {
      swap(l);
    }

    /** Assign to this a copy of `l`

        First, the list is emptied (all its elements are deleted). Then
        sequential copy of each item of `l` is inserted into `this`.

        This assignation takes \f$O(|l|)\f$ in complexity.

        \param[in] l list whose items want to be inserted into `this`
        \throw  `bad_alloc` if there is no enough memory
    */
    DynList & operator = (const DynList & l)
    {
      if (&l == this)
        return *this;

      empty();

      for (typename DynList::Iterator it(l); it.has_curr(); it.next())
        append(it.get_curr());

      return *this;
    }

    /** Assign to `this` by movement the list `l`

        This assignation swaps the content of `this` with the content of
        `l`. This swapping is done in \f$O(1)\f$.

        \note Since `l` is a rvalue reference, take care of that if you
        are interested in to avoid the copy. So, if you have a lvalue
        referencde to a list, use `std::move()`, upon your responsability,
        if and only if you are absolutely sure that the list will not be
        needed after.

        \param[in] l a rvalue reference to the list to be assigned by
        movement.
    */
    DynList & operator = (DynList && l) noexcept
    {
      return (DynList&) this->swap(l);
    }

    /** Append `list `at the end of `this` by movement.

        `append(l)` (in this rvalue version), puts in constant time the
        items of `l` at the end of `this`. After calling `l` becomes
        empty.

        \note Since `l` is a rvalue reference, take care of that if you
        are interested in to avoid the copy. So, if you have a lvalue
        reference to a list, use `std::move()`, upon your responsability,
        if and only if you are absolutely sure that the list will not be
        needed after.

        \param[in] l a rvalue reference to the list to be appended
    */
    DynList & append(DynList && list) noexcept
    {
      HTList::append(list);
      return *this;
    }

    /** Insert `list `at the beginning of `this` by movement.

        `insert(l)` (in this rvalue version), puts in constant time the
        items of `l` at the beggining of `this`. After calling `l` becomes
        empty.

        \note Since `l` is a rvalue reference, take care of that if you
        are interested in to avoid the copy. So, if you have a lvalue
        reference to a list, use `std::move()`, upon your responsability,
        if and only if you are absolutely sure that the list will not be
        needed after.

        \param[in] l a rvalue reference to the list to be inserted
    */
    DynList & insert(DynList && list) noexcept
    {
      HTList::insert(list);
      return *this;
    }

    /** Append to `this` a copy of `list`.

        This version of `append(l)` traverses the items of `l` and appends
        them (copies) into `this`.

        \param[in] l list to be copied at the end
        \throw bad_alloc if there is no enough memory
    */
    DynList & append(const DynList & list) noexcept(noexcept(DynList(list)))
    {
      DynList copy = list;
      HTList::append(copy);
      return *this;
    }

    /** Insert to `this` a copy of `list`.

        This version of `insert(l)` traverses the items of `l` and insert
        a copy into `this`.

        \param[in] l list to be copied at the end
        \throw bad_alloc if there is no enough memory
    */
    DynList & insert(const DynList & list) noexcept(noexcept(DynList(list)))
    {
      DynList tmp = list;
      HTList::insert(tmp);
      return *this;
    }

    /// Obtains a modifiable reference to the i-th item of `this`. Throws
    /// `overflow_error` if `i` is greater or equal to number of elements
    T & get(const size_t & i)
    {
      Iterator it(*this);

      for (size_t __i = 0 ; it.has_current() and __i < i; it.next(), ++__i);

      return it.get_current();
    }

    /// \overload get(const size_t & i)
    T & operator [] (const size_t & i)
    {
      return get(i);
    }
  };

  template <class Container>
  inline DynList<typename Container::Item_Type> to_dynlist(const Container & c)
  {
    return c.template maps<typename Container::Item_Type>([] (const auto & d)
                                                          {
                                                            return d;
                                                          });
  }

  template <typename T> inline
  DynList<T> get_unitarian_DynList(const T & item)
  {
    DynList<T> ret;
    ret.append(item);
    return ret;
  }

  template <typename T> inline
  DynList<T> unitarian_DynList(const T & item)
  {
    return DynList<T>({item});
  }

# undef NEXT

} // end namespace Aleph


# endif // HTLIST_H
