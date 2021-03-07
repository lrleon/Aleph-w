
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef AH_PRIORITY_QUEUE_H
# define AH_PRIORITY_QUEUE_H

# include <tpl_dynBinHeap.H>

namespace Aleph {

    /** Implantación Aleph del contenedor estándar prority_queue<T>.

	En Aleph este contenedor estándar es implantado mediante una
	la clase DynBinHeap, la cual, a su vez está basada en árboles
	binarios. Esta implantación so se sustenta sobre un arreglo y es
	altamente dinámica. Otras estructuras o algoritmos pueden
	mantener sin riesgo punteros hacia los datos que contiene la cola de
	prioridad.

	Esta versión de Priority_Queue<T> no emplea un contenedor
	genérico como parámetro tipo. Consecuentemente, los
	constructores estándar están ligeramente modificados.

	@see DynBinHeap
	@ingroup stl
     */
    template <typename T, class Compare = Aleph::less<T> >
class priority_queue : private Aleph::DynBinHeap<T, Compare>
{
public:

      /// Tipo de valor que almacena la cola de prioridad.
  typedef T value_type;

      /// Instancia una cola de prioridad vacía
  priority_queue() { /* empty */ }

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
