
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath Le�n
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath Le�n. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath Le�n ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REP�BLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file tpl_snode.H
  @date 2002 - 2014
  @author Leandro Le�n (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_SNODE_H
# define TPL_SNODE_H

# include <slink.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Simple node with data type T a singly-linked list.

  @details The Snode class inherits all the Slink interface and defines
  generically the data containing the node.

  @param T Data type that stores the node.

  @see DynSlist Snode Slink

  @ingroup Secuencias
*/
template <typename T> 
class Snode : public Slink
{
private:

  T data;

public:

/** @brief Returns a reference to data contained in the node. */
  T & get_data() { return data; }

/** @brief Empty constructor. */
  Snode() { /* empty*/ }

/** @brief Constructor that copy data. */
  Snode(const T & _data) : data(_data) { /* Empty */ }

  Snode(const T && _data) : data(_data) { /* Empty */ }

/** 
  @brief Deletes the next node to "this".

  @details remove_next() removes the linked node after "this" and returns
  your memory address.
  
  @return The node removed.
*/
  Snode * remove_next() { return (Snode*) Slink::remove_next(); }

/** @brief Retorna el nodo siguiente a "this" */
  Snode *& get_next() { return (Snode*&) Slink::get_next(); }

/** 
  @brief Deletes the next node to "this".

  @details remove_next() removes the linked node after "this" and returns
  your memory address.

  @return The node removed.
*/
  Snode * remove_first() { return Snode::remove_next(); }

/** @brief Returns the next node to "this" */
  Snode *& get_first() const { return Snode::get_next(); }
};

} /* End namespace Aleph */

# endif /* TPL_SNODE_H */
