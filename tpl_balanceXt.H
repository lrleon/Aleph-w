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

/** @file tpl_balanceXt.H
 *  @brief Tree balancing with extended nodes.
 *
 *  Utilities for balancing trees with subtree counts.
 *
 *  @see tpl_binNodeXt.H Extended nodes
 *  @ingroup Arboles
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BALANCEXT_H
# define TPL_BALANCEXT_H

# include <tpl_binNodeXt.H>
 # include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

    /** Selecciona un nodo de un árbol binario según su posición infija
        y lo convierte en su raíz.

        select_gotoup_root(r,i) selecciona  el nodo con posición infija
        i y lo rota hasta que éste devenga su raíz.

        Este algoritmo de selección es recursivo.

        @param[in] root raíz del árbol binario con rangos.
        @param[in] i posición infija que se desea acceder.
        @return puntero al nodo en la posición infija i el cual luego de
        la llamada es raíz del árbol binario.
        @throw out_of_range si i es mayor o igual que la cantidad total
        de nodos del árbol binario.
        @see select() select_rec()

        @ingroup Arboles
     */
    template <class Node> inline
Node * select_gotoup_root(Node * root, const size_t & i) 
{
  assert(root not_eq Node::NullPtr);
  assert(COUNT(Node::NullPtr) == 0);

  if (i >= COUNT(root))
    ah_out_of_range_error_if(true) << "";

  if (i == COUNT(LLINK(root))) 
    return root;

  if (i < COUNT(LLINK(root)))
    {
      LLINK(root) = select_gotoup_root(LLINK(root), i);
      root = rotate_to_right_xt(root);
    }
  else
    {
      RLINK(root) = select_gotoup_root(RLINK(root), 
                                       i - COUNT(LLINK(root)) - 1);
      root = rotate_to_left_xt(root);
    }
  return root;
}

    /** Reequilibra un árbol binario de búsqueda.

        balance_tree(root) toma el árbol binario de búsqueda con raíz
        root y lo reequilibra "perfectamente" en el sentido de que para
        cada nodo la diferencia entre las cardinalidades de sus
        subárboles es a lo sumo uno.

        La operación tiene un desempeño de \f$O(n\; \lg n)\f$.

        @param[in] root la raíz del árbol binario de búsqueda a
        equilibrar. 
        @return raíz del árbol binario de búsqueda equilibrado.
     */
template <class Node> inline Node * balance_tree(Node * root)
{
  if (COUNT(root) <= 1) 
    return root;

  root        = select_gotoup_root(root, COUNT(root) / 2);
  LLINK(root) = balance_tree(LLINK(root));
  RLINK(root) = balance_tree(RLINK(root));

  return root;
}

} // end namespace Aleph

# endif // TPL_BALANCEXT_H
