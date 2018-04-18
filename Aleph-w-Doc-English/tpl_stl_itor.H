
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
  @file tpl_stl_itor.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_STD_ITRO_H
# define TPL_STD_ITRO_H

namespace Aleph {


/** 
  @brief Iterator over a set of C ++ Standard Library
  implanted with Aleph.

	@ingroup Arboles Contenedores
*/
  template <class Tree_Type>
class Tree_Iterator
{
private:

  friend class Set_Type;
    
  typedef typename Tree_Type::Iterator Iterator;
    
  Iterator itor;
  bool underflow;
  bool overflow;

  void init_flags ()
  {
    if (itor.has_current () )
      underflow = overflow = false;
    else
      underflow = overflow = true;
  }

  void goto_begin ()
  {
    itor.reset_first ();
    init_flags ();
  }

  void goto_last ()
  {
    itor.reset_last ();
    init_flags ();
  }

  void goto_end ()
  {
    itor.reset_last ();
    init_flags ();
    overflow = true;
    if (not size() != 0)
      itor.next ();
  }

  void forward ()
  {
    if (underflow)
      {
	goto_begin ();
	return;
      }
      
    itor.next ();

    if (not itor.has_current () )
      overflow = true;
  }

  void backward ()
  {
    if (overflow)
      {
	goto_last ();
	return;
      }
      
    itor.prev ();

    if (not itor.has_current () )
      underflow = true;
  }

  iterator (Tree_Type & _tree, Node * node) 
    : itor (_tree, node), underflow (false), overflow (false)
  {
    /* Empty */
  }

public:

  typedef typename map::value_type    value_type;
  typedef typename map::size_type     difference_type;
  typedef typename map::value_type *   pointer;
  typedef typename map::reference     reference;

/** @brief Empty constructor; It does not apply if a group is not associated. */
  iterator () { /* Empty */ }

  iterator (Tree_Type & tree) : itor (tree)
  {
    init_flags ();
  }

/** @brief It provides a reference to the current item. */
  Pair & operator * ()
  {
    return KEY (itor.get_current () );
  }

/** @brief "Dereference" a pointer to the current item. */
  Pair * operator -> ()
  {
    return &KEY (itor.get_current () );
  }

/** @brief Advances the iterator a forward position,
then loads the current item. */
  Pair & operator ++ ()
  {
    forward ();

    return KEY (itor.get_current () );
  }

  Pair & operator ++ (int)
  {
    Pair & retPair = KEY (itor.get_current () );
    forward ();

    return retPair;
  }

  Pair & operator -- ()
  {
    backward ();

    return KEY (itor.get_current () );
  }

  Pair & operator -- (int)
  {
    Pair & retPair = KEY (itor.get_current () );
    backward ();

    return retPair;
  }
    
  Pair & operator += (const size_type & n)
  {
    itor.reset_to_pos (itor.get_current_position () + n);

    return KEY (itor.get_current () );
  } 

  Pair & operator -= (const size_type & n)
  {
    itor.reset_to_pos (itor.get_current_position () - n);

    return KEY (itor.get_current () );
  } 

  bool operator == (const iterator & _itor) const
  {
    return itor == _itor.itor;
  }

  bool operator != (const iterator & _itor) const
  {
    return not (itor == _itor.itor);
  }

  bool verify (const map & _map) const
  {
    return itor.verify ( (Tree_Type*) &_map.tree); 
  }

  bool verify (const iterator & it) const
  {
    return itor.verify (it.itor);
  }
};



} /* End namespace Aleph */

# endif // TPL_STD_ITRO_H
