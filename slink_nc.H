

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
 }

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
        if (not has_curr())
          throw std::overflow_error("Not element in list");
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
        if (not has_curr())
          throw std::overflow_error("Not next element in list");
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

