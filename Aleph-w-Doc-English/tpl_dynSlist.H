 
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
  @file tpl_dynSlist.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNSLIST_H
# define TPL_DYNSLIST_H

# include <tpl_slist.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Dynamic list of elements of type T orchestrated by list
  simply linked.

  @param T Data type of the elements of the list.

  @see Slist Snode Slink 
  
  @ingroup Secuencias
*/
    template <typename T>
class DynSlist : public Slist<T>
{

private:

  size_t num_items;
  int        current_pos;
  Snode<T> * current_node;
  typename Slist<T>::Node * get_previous_to_pos(const int & pos)
  {

    if (pos > num_items)
      throw std::out_of_range ("position out of range");

    /** You have to go back?
    yes? ==> Restart current position */
    if (pos < current_pos) 
      { 
        current_pos  = 0;
        current_node = this;
      }
    while (current_pos < pos)  /* Predecessor node to advance to pos */
      {
        current_node = current_node->get_next();
        ++current_pos;
      }
    return current_node;
  }

public:

/** 
  @brief Constructor. 
*/
  DynSlist() : num_items(0), current_pos(0), current_node(this)
  { 
    /* Empty */
  } 
/** 
  @brief Access ith element of the list.

  @details The operator [i] on a list accesses the element whose position
  is i.

  Access to the i position requires travel sequentially i - 1
  previous elements.

  @param [in] i Position the item you want to access.

  @throw out_of_range_error If i is greater than the amount of
  list items.

  @return A constant reference to the element i in the list.

  @note In terms of performance, this operator is not the same as
  access on a settlement.
*/
  T & operator [] (const size_t & i) 

    throw(std::exception, std::out_of_range)

  {
    return get_previous_to_pos(i)->get_next()->get_data();
  }    

/** 
  @brief Returns the number of items you have listed. 
*/
  size_t size() const { return num_items; }

/** 
  @brief Inserts an element in the post position within a dynamic list.

  @details insert (pos, data) inserted in the position after the esta list
  element whose value is data.

  @param[in] pos Posición de inserción.
  @param[in] data Element value to insert.
  
  @throw bad_alloc If there is insufficient memory.
  @throw out_of_range Si pos está fuera de rango.
*/
  void insert(const int & pos, const T & data) 

    throw(std::exception, std::bad_alloc, std::out_of_range)

  {      
    /* Aside for new element node */
    typename Slist<T>::Node * node = new typename Slist<T>::Node (data);
    typename Slist<T>::Node * prev = get_previous_to_pos(pos); 
    prev->insert_next(node);
    ++num_items;
  }
/** 
  @brief Removes the element list position pos.

  @param [in] pos position of the element to remove.

  @throw out_of_range If pos is out of range.
*/
  void remove(const int & pos) 

    throw(std::exception, std::range_error)

  {      
    /* Get predecessor node to the new element */
    typename Slist<T>::Node * prev = get_previous_to_pos(pos);
    typename Slist<T>::Node * node_to_delete = prev->remove_next();
    delete node_to_delete;
    --num_items;
  }

/** 
  @brief Destroyer. 
*/
  ~DynSlist() 
  {      
    /** Remove node by node until the list accrues empty */
    while (not this->is_empty())
      delete this->remove_first(); /* Remove_first de clase Slink */
  }
/** 
  @brief Iterator implemented on dynamic lists with lists
  simply linked. 

  @ingroup Secuencias
*/
  class Iterator : public Slist<T>::Iterator
  {
  public:

/**
  @typedef The set type on which iterates. 
*/
    typedef Slist<T> Set_Type;
/** 
  @typedef The type of item returning get_current(). 
*/
    typedef T Item_Type;

/** 
  @brief Constructor. 
*/
    Iterator(DynSlist & list) : Slist<T>::Iterator(list) { /* Empty */ } 

/** 
  @brief Returns a reference to the current item. 
*/
    T & get_current() { return Slist<T>::Iterator::get_current()->get_data(); }
  };

};

} /* End namespace Aleph */

# endif // TPL_DYNSLIST_H
