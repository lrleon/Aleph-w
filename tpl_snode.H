
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

# ifndef TPL_SNODE_H
# define TPL_SNODE_H

# include <slink.H>

using namespace Aleph;

namespace Aleph {

  /** Nodo simple con dato de tipo de T de una lista simplemente enlazada.

      La clase Snode hereda toda la interfaz de Slink y define
      genéricamente el dato que contiene el nodo.

      @param T el tipo dato que guarda el nodo.

      @see DynSlist Snode Slink
      @ingroup Secuencias
   */
template <typename T> 
class Snode : public Slink
{
private:

  T data;

public:

      /// Retorna una referencia al dato contenido en el nodo.
  T & get_data() { return data; }

      /// Constructor vacío. 
  Snode() { /* empty*/ }

      /// Constructor que copia dato.
  Snode(const T & _data) : data(_data) { /* empty */ }

  Snode(const T && _data) : data(_data) { /* empty */ }

      /** Elimina el nodo siguiente a this.

          remove_next() elimina el nodo enlazado después de this y retorna
          su dirección de memoria.
          
          @return el nodo eliminado.
       */
  Snode * remove_next() { return (Snode*) Slink::remove_next(); }

      /// Retorna el nodo siguiente a this
  Snode *& get_next() { return (Snode*&) Slink::get_next(); }

      /** Elimina el nodo siguiente a this.

          remove_next() elimina el nodo enlazado después de this y retorna
          su dirección de memoria.
          
          @return el nodo eliminado.
       */
  Snode * remove_first() { return Snode::remove_next(); }

      /// Retorna el nodo siguiente a this
  Snode *& get_first() const { return Snode::get_next(); }
};

} // end namespace Aleph

# endif /* TPL_SNODE_H */

