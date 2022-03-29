
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

# ifndef TPL_DYNARRAY_SET_H
# define TPL_DYNARRAY_SET_H

# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>

namespace Aleph
{

    /** Conjunto de elementos implantado mediante un arreglo dinámico.
    
        DynArray_Set instrumenta un conjunto de elementos de tipo T. 

        El conjunto está internamente representado mediante un arreglo
        dinámico del tipo DynArray. Consecuentemente, el consumo en
        memoria es proporcional al número de elementos. 

        La inserción es sumamente rápida, la búsqueda es lineal y la
        eliminación de un elementpo ya encontrado también es muy rápida.

        La clase recibe dos parámetros tipo: el tipo de elemento del
        conjunto y una clase de comparación cuyo único rol es determinar
        si un elemento es igual a no a otro.

        Por razones de rapidez, se permite duplicar elementos.
        
        @see DynArray
        \ingroup Secuencias
    */
template <typename T, class Equal = Aleph::equal_to<T>>
  using DynArray_Set = DynArray<T>;

};

# endif // TPL_DYNARRAY_SET_H

