
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
  @file slink.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef SLINK_H
# define SLINK_H

# include <aleph.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Simple link to node list.

  @details Slink modeled a pointer to a node in a list
  simply linked.

  @ingroup Secuencias

  @see Snode Slist DynSlist Dlink 
*/
class Slink
{
protected:

    Slink * next;

public:

/** 
  @brief Empty constructor. 
*/
  Slink() : next(this) { /* Empty */ }

/** 
  @brief Restart link to point to itself
*/
  void reset() 
  {
    assert(this not_eq NULL);

    next = this; 
  }
/** 
  @brief Returns true if this rate is empty (pointing to himself)
*/
  bool is_empty() const 
  {
    assert(this not_eq NULL);

    return next == this; 
  }

/** 
  @brief Returns the following link.
*/
  Slink *& get_next() 
  {
    assert(this not_eq NULL);
    return next; 
  }

/** 
  @brief Bond inserted after esta p.

  @details insert_next(p) insert a single node whose link is p then
  the node whose link is esta.

  @param[in] p Simple link to insert.
*/
  void insert_next(Slink * p)
  {

    assert(this not_eq NULL);
    assert(p not_eq NULL);
    assert(p->is_empty());

    p->next = next;
    next = p;
  }

/** 
  @brief It deletes the following link to this.

  @details remove_next() 
  eliminating the single node located after
  the node whose link is esta and returns the link address
  removed.

  @return The link removed.
*/
  Slink * remove_next()
  {
    assert(this not_eq NULL);

    Slink * ret_val = next;
    next = ret_val->next;
    ret_val->reset();
    return ret_val;
  }    
};
/** 
  @brief Genera conversion function name simple link to
  structure that contains it; The function name is literally
  the parameter to be instantiated as link_name

  @details This macro is used when you have two or more Slink are
  part of a structure and want to get a pointer to the
  structure since some of the links.

  Example:
  struct Registro
  {
      ...
      Slink l1;
      Slink l2;
      ...
  };

  So slink_TO_TYPE(Registro, l1) and
  SLINK_TO_TYPE(Registro, l2) generate functions:

  -# Registro * l1_to_type(Slink * link), which receives
   l1 pointer to field registration and returns
   pointer to register.
    
  -# Registro * l2_to_type(Slink * link), which receives
   l2 pointer to the field of registration and returns
   pointer to register.

   The idea is to have naming schemes that prmitan
   distición make between fields.

  @param type_name Type structure (struct or class) that
  It contains at Slink.
  @param link_name Name field of the double bond within
  the structure.

  @ingroup Secuencias
*/
# define SLINK_TO_TYPE(type_name, link_name)				\
  static type_name * slink_to_type(Slink * link)			\
  {									\
    type_name * ptr_zero = 0;						\
    size_t offset_link = (size_t) &(ptr_zero->link_name);		\
    unsigned long address_type = ((unsigned long) link) - offset_link;	\
    return (type_name *) address_type;					\
  } 

} /* End namespace Aleph */
  
# endif /* SLINK_H */
