
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


/** @file tpl_dyn_slist_nc.H
 *  @brief Dynamic singly linked list (no check).
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYN_SLIST_NC_H
# define TPL_DYN_SLIST_NC_H

# include <tpl_snode_nc.H>
# include <ah-errors.H>

namespace Aleph
{
  /** Lista dinámica de elementos de tipo T.

      Dyn_Slist_Nc<T> define una lista dinámica, es una secuencia de elementos 
      de algún tipo T. 

      Este tipo puede emplearse como pila o cola. 

      @param T el tipo de elementos de la lista.

      @see Slink, Snode_Nc, Dyn_Slist_Nc::Iterator

      @ingroup Sequences

      @author Leandro R. León
      @author Alejandro J. Mujica
   */
  template <typename T> class Dyn_Slist_Nc : public Snode_Nc <T>
  {
  public:
    typedef Snode_Nc <T> Node;

  private:
    Node * head;
    size_t num_items;

  public:

    /// Vacía totalmente a la lista
    void empty()
    {
      while (not this->is_empty())
        delete Node::remove_next();
      this->reset();
      head = this;
      num_items = 0;
    }

    /** Inserta un elemento al principio de la lista.

        Inserta en la lista this como primer elemento una copia de data. 

        Después de la operación el primer elemento de la lista es data.

        @param[in] data el dato a insertarse.
        @return referencia al dato recién insertado.
        @throw bad_alloc si no hay memoria para el nuevo elemento.
     */
    T & insert(const T & data) throw(std::exception, std::bad_alloc)
    {
      Node * p = new Node(data);
      Node::insert(p);

      if (num_items++ == 0)
        head = p;

      return p->get_data();
    }

    /** Inserta un elemento al final de la lista.

        Inserta en la lista this como último elemento una copia de data. 

        Después de la operación el último elemento de la lista es data.

        @param[in] data el dato a insertarse.
        @return referencia al dato recién insertado.
        @throw bad_alloc si no hay memoria para el nuevo elemento.
     */
    T & append(const T & data) throw(std::exception, std::bad_alloc)
    {
      assert(head not_eq nullptr);
      Node * p = new Node(data);
      head->insert(p);

      head = p;
      ++num_items;
      return p->get_data();
    }

    /// Retorna una referencia al primer elemento de la lista.
    T & get_first()
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      return this->get_next()->get_data();
    }

    /// Retorna una referencia al último elemento de la lista.
    T & get_last()
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      return head->get_data();
    }

    /** Elimina el primer elemento de la lista: retorna una copia del
        elemento eliminado.

        @return una copia del elemento eliminado.
        @throw underflow_error si la lista está vacía.
    */
    T remove_first()
    {
      ah_underflow_error_if(this->is_empty()) << "List is empty";

      Node * p = Node::remove_next();
      T ret_val = p->get_data();
      delete p;
      if (--num_items == 0)
        head = this;
      return ret_val;
    }

    /// Si this es una cola, entonces mete el elemento item.
    T & put(const T & item) { return append(item); }

    /// Si this es una cola, entonces extrae el elemento más antiguo.
    T get() { return remove_first(); }

    /// Si this e suna cola, entonces retorna el elemento más joven.
    T & rear() { return get_last(); }

    /// Si this e suna cola, entonces retorna el elemento más antiguo.
    T & front() { return get_first(); }

    /// Si this es una pila, entonces inserta item.
    T & push(const T & item) { return insert(item); }

    /// Si this es una pila, entonces elimina el tope.
    T pop() { return remove_first(); }

    /// Si this es una pila, entonces retorna el tope.
    T & top() const { return get_first(); }

    const size_t & size() const { return num_items; }

    class Iterator : public Node::Iterator
    {
    /* TODO: Agregar atributos y métodos que hagan falta
             la implementación de este iterador la hice minimalista
             para pruebas de Dyn_Slist_Nc
    */
    public:
      Iterator(Dyn_Slist_Nc<T> & list) : Node::Iterator(list)
      {
        // empty
      }

      T & get_curr_n() const noexcept
      {
        return Node::Iterator::get_curr()->get_data();
      }

      T & get_curr() const { return Dyn_Slist_Nc::Iterator::get_curr(); }

    };

        /// Constructor vacío
    Dyn_Slist_Nc() : Node(), head(this), num_items(0)
    {
      // empty
    }

    /// Constructor de copia.
    Dyn_Slist_Nc(const Dyn_Slist_Nc & l) : Node(l), head(this), num_items(0)
    {
      for (Iterator it(const_cast <Dyn_Slist_Nc &>(l));
           it.has_curr(); it.next_ne())
        append(it.get_curr());
    } 

    ~Dyn_Slist_Nc()
    {
      empty();
    }

    /** Asignación de lista dinámica con semántica lvalue.

        La asignación elimina todos los elementos de this y luego copia en
        this los elementos contenidos en la lista list.

        @param[in] list lista a ser asignada
        @throw bad_alloc si no hay memoria.
    */
    Dyn_Slist_Nc & operator = (const Dyn_Slist_Nc & list)
    {
      if (this == &list) 
        return *this;
 
      empty();
    
      assert(this->is_empty());

      for (Iterator it(const_cast<Dyn_Slist_Nc &>(list)); 
           it.has_curr(); it.next_ne())
        append(it.get_curr()); 

      return *this;
    }

    T & operator [] (const size_t & n)
    {
      Iterator it(*this);
      for (int i = 0; i < n and it.has_curr(); ++i, it.next_ne());

      return it.get_curr();
    }
  };
}

# endif

