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

/** @file Stack.H
 *  @brief STL-compatible stack adapter (legacy).
 *
 *  @deprecated This is a legacy Aleph implementation of std::stack.
 *  It will be deprecated in future versions. For new code, prefer using
 *  `std::stack` directly or `Aleph::DynListStack` for Aleph-native features.
 *
 *  Aleph implementation of std::stack using DynListStack for LIFO
 *  operations with STL-compatible interface.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef AH_STACK_H
# define AH_STACK_H

# include <tpl_dynListStack.H>


namespace Aleph {

    /** Implantación Aleph del contenedor estándar stack<T>.

	En Aleph este contenedor estándar es implantado mediante una
	lista circular simplemente enlazada. Consecuentemente, esta
	versión de stack<T> no emplea un contenedor genérico como
	parámetro.

	@ingroup stl
     */
    template <typename T>
class stack : public Aleph::DynListStack<T>
{
public:

      /// El tipo de valor que guarda la pila.
  typedef T value_type;

      /// Tipo numérico para tamaño de pila.
  typedef size_t size_type;

      /// Instancia una pila vacía. 
  stack() { /* empty */ }

      /// Instancia una pila con los elementos del contenedor cont.
      template <class Container>
  stack(Container & cont)
  {
    const typename Container::iterator end = cont.end();

    for (typename Container::iterator it(cont.begin()); it != end; push(it++));
  }
  
      /// Retorna true si la pila está vacía.
  bool empty() const { return this->is_empty(); }
};

} // end namespace Aleph

# endif // AH_STACK_H
