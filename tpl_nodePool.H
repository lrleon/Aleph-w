
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef TPL_NODEPOOL_H
# define TPL_NODEPOOL_H

# include <tpl_arrayStack.H>

    /** Conjunto de nodos a reusarse.

	La clase Node_Pool se destina como un buffer de nodos de
	algún tipo de dato (listas, árboles, etc) disponibles para
	rápido uso evitando llamadas a los operadores new y delete.

	La clase maneja un parámetro n que representa la máxima cantidad
	de nodos de reserva. 

       @author Leandro Rabindranath León
       @ingroup utils
     */
    template <class Node>
class Node_Pool
{

  FixedStack<Node*> node_pool;
  const size_t n;

public:

  Node_Pool(size_t __n = 100) noexcept : node_pool(__n), n(__n) { /* empty */ }

      ///  \overload allocate()
  Node * allocate()
  {
    return node_pool.is_empty() ? new Node : node_pool.pop();
  }

  /** Aparta memoria para un nodo y le asigna un valor de clave de tipo
      Key. 

      allocate_node() revisa si hay un nodo ya previamente apartado
      entre los n posibles. Si tal es el caso, entonces se escoge un
      nodo entre los existentes del conjunto. De lo contrario, se
      reserva memoria para un nuevo nodo mediante new. En ambos casos la
      clave key es asignada al resultado que dé get_key() invocada sobre
      el nodo.

      @param[in] key clave a asignar al nodo.
      @throw bad_alloc si no hay suficiente memoria.
   */
  Node * allocate(const typename Node::key_type & key)
  {
    return new (allocate()) Node (key);
  }

  Node * allocate(typename Node::key_type && key)
  {
    return new (allocate()) Node (std::forward<typename Node::key_type>(key));
  }  

      /** Libera el nodo p.

	  deallocate_node(p) entrega el nodo p para eventual posterior
	  reutilización. 

	  Cada vez que se libera un nodo por este método, se guarda en
	  un conjunto para posterior reutilización de hasta un mácimo de
	  n nodos. Si el conjunto no está lleno, entonces el nodo puede
	  ser reutilizado. De lo contrario, si el conjunto está lleno,
	  entonces el nodo es liberado mediante delete.

	  @note Tenga muy en cuenta que en caso de que el nodo se
	  guarde para uso posterior no se llama al destructor. Por
	  tanto, es muy importante que p refiera a un nodo recién
	  apartado.

	  @param[in] p el nodo a liberar.
       */
  void deallocate(Node * p) noexcept
  {
    if (node_pool.size() == n)
      delete p;
    else
      node_pool.push(p);
  }

  ~Node_Pool()
  {
    while (not node_pool.is_empty())
      delete node_pool.pop();
  }
};

# endif // TPL_NODEPOOL_H

