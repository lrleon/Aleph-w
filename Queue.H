

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

       /// Instancia una cola con los elementos del contenedor cont.
      template <class Container>
  queue(Container & cont)
  {
    const typename Container::iterator end = cont.end();

    for (typename Container::iterator it(cont.begin()); it != end; put(it++));
  }
  
      /// Inserta value en la cola.
  void push(const T & value) { put(value); }

      /// Elimina un elemento del frente de la cola.
  void pop() { this->get(); }

      /// Consulta el elemento trasero de la cola.
  T & back() { return this->rear(); }

      /// Retorna true la cola está vacía.
  bool empty() const { return this->is_empty(); }
};

} // end namespace Aleph

# endif // AH_QUEUE_H

