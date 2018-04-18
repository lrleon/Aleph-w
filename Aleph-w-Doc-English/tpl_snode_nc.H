
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
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

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
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
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file tpl_snode_nc.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_SNODE_NC_H
# define TPL_SNODE_NC_H

# include <slink_nc.H>

namespace Aleph
{
  template <typename T> class Snode_Nc : public Slink_Nc
  {
    T data;

  public:

/** @brief Empty constructor. */
    Snode_Nc() : Slink_Nc() { /* empty*/ }

/** @brief Constructor que copia dato. */
    Snode_Nc(const T & _data) : Slink_Nc(), data(_data) { /* Empty */ }

    ~Snode_Nc() { /* Empty */ }

/** @brief Returns a reference to data contained in the node. */
    T & get_data() { return data; }
  
/** 
  @brief Deletes the next node to "this".

  @details remove_next() removes the node bound after this and returns
  your memory.

  @return The node removed.
*/
    Snode_Nc * remove_next()
    { 
      return (Snode_Nc *) Slink_Nc::remove_next();
    }
    
/** @brief Retorna el nodo siguiente a this */
    Snode_Nc *& get_next()
    {
      return (Snode_Nc *&) Slink_Nc::get_next();
    }

    class Iterator : public Slink_Nc::Iterator
    {
    public:
      Iterator() { /* Empty */ }

/** @brief Iterator instance in list whose head node is head_ptr */
      Iterator(Snode_Nc<T> * head_ptr)
        : Slink_Nc::Iterator(head_ptr)
      {
        /* Empty */
      }

/** @brief Iterator instance in list whose head node is head_ptr */
      Iterator(Snode_Nc<T> & head) : Slink_Nc::Iterator(&head) { /* Empty */ }

/** @brief Iterator instance in list whose head node is head_ptr y
coloca el elemento actual en el apuntado por curr_ptr */
      Iterator(Snode_Nc<T> * head_ptr, Snode_Nc<T> * curr_ptr) 
        : Slink_Nc::Iterator(head_ptr, curr_ptr)
      {
        /* Empty */
      }

/** @brief Returns pointer to the current node. */ 
      Snode_Nc * get_current() 
      {
        return (Snode_Nc *) this->Slink_Nc::Iterator::get_current(); 
      }

/** @brief Returns pointer to the current node. */
      Snode_Nc * get_curr() 
      {
        return Snode_Nc::Iterator::get_current(); 
      }
    };
  };
}

# endif
