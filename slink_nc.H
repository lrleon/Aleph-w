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

/** @file slink_nc.H
 *  @brief Singly linked node (no check variant).
 *  @ingroup Secuencias
 *  @author Leandro Rabindranath León
 */

# ifndef SLINK_NC_H
# define SLINK_NC_H

# include <aleph.H>
# include <ah-errors.H>

namespace Aleph
{
  /** Enlace simple a lista de nodos no circular.

      Slink_Nc modeliza un puntero a un nodo dentro de una lista
      simplemente enlazada no circular. 

      @ingroup Secuencias
      @see Snode Slist DynSlist Dlink 
   */
  class Slink_Nc
  {
  protected:
    Slink_Nc * next;
  
  public:
    /// Constructor vacío.
    Slink_Nc() : next(nullptr) { /* empty */ }

    /// Constructor copia; coloca enlace a que apunte a nullptr.
    Slink_Nc(const Slink_Nc &) : next(nullptr) { /* empty */ }

    ~Slink_Nc() { /* empty */ }

    /// Asignación; coloca enlace a que apunte a nullptr.
    Slink_Nc & operator = (const Slink_Nc & link)
    {
      if (&link == this)
        return *this;

      ah_invalid_argument_if(not is_empty()) << "link is not empty";

      next = nullptr;
    }

    /// Reinicia enlace a que apunte a nullptr
    void reset()
    {
      assert(this not_eq nullptr);
      next = nullptr;
    }

    /// Retorna true si this está vacío (apunta a nullptr)
    bool is_empty() const
    {
      assert(this not_eq nullptr);
      return next == nullptr;
    }

    /// Retorna el siguiente enlace.
    Slink_Nc *& get_next()
    {
      assert(this not_eq nullptr);
      return next;
    }

    /** Inserta en enlace p después de this.

          insert(p) inserta un nodo simple cuyo enlace es p después
          del nodo cuyo enlace es this.

          @param[in] p el enlace simple a insertar.
       */
    void insert(Slink_Nc * p)
    {
      assert(this not_eq nullptr);
      assert(p not_eq nullptr);
      assert(p->is_empty());
      p->next = next;
      next = p;
    }

    /** Elimina el enlace siguiente a this.

          remove_next() elimina el nodo simple situado después
          del nodo cuyo enlace es this y retorna la dirección del enlace 
          eliminado.

          @return  el enlace eliminado.
       */
    Slink_Nc * remove_next()
    {
      assert(this not_eq nullptr);
      Slink_Nc * ret_val = next;
      next = ret_val->next;
      ret_val->reset();
      return ret_val;
    }

    class Iterator
    {
      Slink_Nc * head;
      Slink_Nc * curr;
    
    public:
      
      Iterator() : head(nullptr), curr(nullptr) { /* Empty */ }

      Iterator(Slink_Nc * head_ptr) : head(head_ptr), curr(head->get_next())
      { 
        // empty
      }

      Iterator(Slink_Nc & _head) : head(&_head), curr(head->get_next())
      { 
        // empty
      }

      Iterator(Slink_Nc * head_ptr, Slink_Nc * curr_ptr)
        : head(head_ptr), curr(curr_ptr)
      {
        // empty
      }

      void reset_first() /// Reinicia iterador a primer nodo de la lista
      {
        assert(curr != nullptr and head != nullptr);
        curr = head->get_next();
      }

      void set(Slink_Nc * new_curr) 
      {
        assert(curr != nullptr and head != nullptr);
        curr = new_curr;
      }

      void reset(Slink_Nc * new_head)
      {
        head = new_head;
        curr = head->get_next();;
      }

      bool has_curr() const 
      {
        assert(head != nullptr);
        return curr != nullptr;
      }

      /// Retorna dirección de nodo actual
      Slink_Nc * get_curr_ne() noexcept
      {
        assert(curr != nullptr and head != nullptr);
        return curr;
      }

      Slink_Nc * get_curr()
      {
        ah_overflow_error_if(not has_curr()) << "Not element in list";
        return get_curr_ne();
      }

      /// Retorna true si iterador está sobre primer elemento
      bool is_in_first() const { return curr == head->next; }

      /// Avanza iterador en una posición 
      void next_ne() noexcept
      {
        curr = curr->get_next();
      }

      void next() throw(std::exception, std::overflow_error)
      {
        ah_overflow_error_if(not has_curr()) << "Not next element in list";
        next_ne();
      }

      /// Retorna true si iteradores están sobre el mismo elemento.
      bool operator == (const Iterator & it) const { return curr == it.curr; }

      /// retorna true si iteradores tienen estados diferentes.
      bool operator != (const Iterator & it) const { return curr != it.curr; }
    
      bool verify(Slink_Nc * l) const { return head == l; }

      bool verify(const Iterator & it) const { return head == it.head; }
    };
    
  };
}

# endif

