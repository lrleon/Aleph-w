

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

      /// Instancia una cola con todos los elementos del contenedor cont.
      template <class Container>
  priority_queue(const Container & cont)
  {
    for (auto & item : cont)
      this->insert(item);
  }

      /// Instancia una cola con los elementos comprendidos entre el
      /// rango [beg..end) del contenedor asociado a los iteradores.
      template <class Itor>
  priority_queue(Itor beg, const Itor & end)
  {
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      this->insert(*beg++);
  }

      /// Inserta una copia de value en la cola de prioridad; retorna
      /// una referencia modificable a la copia insertada (esto no es
      /// parte del estándar).
  T & push(const T & value) { return this->insert(value); }

      /// Elimina el menor elemento de la cola de prioridad según el
      /// criterio de comparación especificado en la declaración.
  void pop() { this->getMin(); }

      /// Retorna el menor elemento de la cola de prioridad según el
      /// criterio de comparación especificado en la declaración.
  const T & top() { return Aleph::DynBinHeap<T, Compare>::top(); }

      /// Retorna true si la cola de prioridad está vacía.
  bool empty() const { return this->is_empty(); }

      /// Retorna la cantidad de elementos de la cola.
  size_t size() const { return Aleph::DynBinHeap<T, Compare>::size(); }
};

} // end namespace Aleph

# endif // AH_PRIORITY_QUEUE_H
