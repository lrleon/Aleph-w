

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
  }
 
public:

      /// Tipo de elemento que retorna get_curr()
  typedef typename It::Item_Type Item_Type;

     /// El tipo de iterador.
  typedef It Iterator_Type;

  const Container & get_container() const
  {
    return *const_cast<Container*>(container_ptr);
  }

      /// Retorna el iterador de fondo.
  It & get_iterator() { return *this; }

      /// Retorna una referencia a la clase filtro. Úsese esta función
      /// si se requiere obtener el estado del filtro (recuérdese que se
      /// perderá cuando se invoque al destructor ~Filter_Iterator()
  Show_Item & get_filter() { return show_item; }

  void set_filter(Show_Item si)
  {
    show_item = si;
  }

  Filter_Iterator(Show_Item si = Show_Item()) : show_item(si)
  {
    // empty
  }

  /// Iterador sobre conjunto cont_ptr con filtro si
  Filter_Iterator(const Container & c, Show_Item si = Show_Item()) 
    : It(c), show_item(si), container_ptr(&const_cast<Container&>(c))
  {
    goto_first_valid_item();
  }

  /// Iterador sobre conjunto cont_ptr con filtro si
  Filter_Iterator(const Container & c, void * __cookie,
		  Show_Item si = Show_Item()) 
    : Filter_Iterator(c, si)
  {
    cookie = __cookie;
  }

  void set_cookie(void * __cookie) { cookie = __cookie; }

      /// Adelanta el iterador una posición.
  void next() { forward(); }

  void next_ne() noexcept
  {
    try
      {
	forward();
      }
    catch (...) {}
  }

      /// Retrocede el iterador una posición.
  void prev() { backward(); }

      /// Coloca el iterador sobre el primer elemento de la secuencia.
  void reset_first() { goto_first_valid_item(); }

      /// Coloca el iterador sobre el último  elemento de la secuencia.
  void reset_last() { goto_last_valid_item(); }
};


} // end namespace Aleph
# endif // FILTER_ITERATOR_H

