
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

# ifndef SLINK_H
# define SLINK_H

# include <aleph.H>

using namespace Aleph;

namespace Aleph {

  /** Enlace simple a lista de nodos.

      Slink modeliza un puntero a un nodo dentro de una lista
      simplemente enlazada. 

      @ingroup Secuencias
      @see Snode Slist DynSlist Dlink 
  */
  class Slink
  {
  protected:

    Slink * next;

  public:

    /// Constructor vacío.
    Slink() : next(this) { /* Empty */ }

    /// Reinicia enlace a que apunte a sí mismo
    void reset() 
    {
      next = this; 
    }
    /// Retorna true si this está vacío (apunta a sí mismo)
    bool is_empty() const 
    {
      return next == this; 
    }

    /// Retorna el siguiente enlace.
    Slink *& get_next() 
    {
      return next; 
    }

    /** Inserta en enlace p después de this.

        insert_next(p) inserta un nodo simple cuyo enlace es p después
        del nodo cuyo enlace es this.

        @param[in] p el enlace simple a insertar.
    */
    void insert_next(Slink * p)
    {
      assert(p not_eq nullptr);
      assert(p->is_empty());

      p->next = next;
      next = p;
    }

    /** Elimina el enlace siguiente a this.

        remove_next() elimina el nodo simple situado después
        del nodo cuyo enlace es this y retorna la dirección del enlace 
        eliminado.

        @return  el enlace eliminado.
    */
    Slink * remove_next()
    {
      Slink * ret_val = next;
      next = ret_val->next;
      ret_val->reset();
      return ret_val;
    }    
  };
  /** Genera función de conversión de nombre de enlace simple a 
      estructura que lo contenga. El nombre de la función es literalmente
      el parámetro que se instancie como link_name 

      Este macro se utiliza cuando se tiene dos o más Slink que son
      parte de una estructura y se desea obtener un apuntador a la
      estructura desde algunos de los enlaces. 

      Si tenemos, por ejemplo:
      struct Registro
      {
      ...
      Slink l1;
      Slink l2;
      ...
      };

      Entonces slink_TO_TYPE(Registro, l1) y
      SLINK_TO_TYPE(Registro, l2) generará las funciones:

      -# Registro * l1_to_type(Slink * link), la cual recibe un
      apuntador al campo l1 del registro y retorna el
      puntero al registro.

      -# Registro * l2_to_type(Slink * link), la cual recibe un
      apuntador al campo l2 del registro y retorna el
      puntero al registro.

      La idea es disponer de esquemas de nombramiento que prmitan
      hacer la distición entre los campos.

      @param type_name el tipo de la estructura (struct o class) que
      contiene al Slink.
      @param link_name el nombre del campo del enlace doble dentro de
      la estructura.
      @ingroup Secuencias
  */
# define SLINK_TO_TYPE(type_name, link_name)                            \
  static type_name * slink_to_type(Slink * link)                        \
  {                                                                     \
    type_name * ptr_zero = 0;                                           \
    size_t offset_link = (size_t) &(ptr_zero->link_name);               \
    unsigned long address_type = ((unsigned long) link) - offset_link;	\
    return (type_name *) address_type;                                  \
  } 

} // end namespace Aleph
# endif /* SLINK_H */

