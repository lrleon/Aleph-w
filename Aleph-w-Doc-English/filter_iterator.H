
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
  @file filter_iterator.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef FILTER_ITERATOR_H
# define FILTER_ITERATOR_H

# include <stdexcept>

namespace Aleph {


/** 
  @brief iterator filter over another iterator.

  @details An iterator is an iterator filter receiving rates
  parameterized the following types:
   - # Container: Container elements among most of those defined in Aleph.
   - # It: iterator must belong to the iterator for the Container class.
  Since there might be various kinds associated iterator
  (Container :: It1, Container :: It2,...).
   - # Show_Item: Class that determines if an element of the set must either
  not be shown by the iterator. The above determination is made by
  call logic (bool) Show_Item :: operator () (Container &, It, T) which must return
  true if the item should be displayed or false otherwise.
   .

  Filter_Iterator is to export a generic iterator
  filter elements of the set according to some criterion
  Show_Item defined by the class. This allows algorithms
  using generic Filter_Iterator have different
  Show_Item behaviors depending on the filter.

  Aleph containers exported two major types in
  Filter_Iterator context: SET_TYPE and ITEM_TYPE, the
  which correspond to the type of joint on which
  iterates and type of item that returns the iterator It.

  @note This class is designed to Aleph, it does not work for the
  std standard library.
        
  @ingroup Sequences
*/
    template <class Container, class It, class Show_Item>
class Filter_Iterator : public It
{
  Show_Item & show_item; 

  Container * container_ptr = NULL;

  void * cookie = NULL;

  void goto_first_valid_item() 
  {
    try
      {     

/** 
  @brief Routine to be placed in the first element that accepts show_item 
*/
        for (this->It::reset_first(); true; this->It::next())
          if (not this->It::has_curr() or show_item(this->It::get_curr()))
            return;
      }
    catch (std::overflow_error) { /* Continue to overflow; not spread */ }
  }

  void forward()
  {
    this->It::next(); 
    try 
      {     

/** 
  @brief Routine to advance to the next item that accepts show_item
*/
        for (;true; this->It::next()) 
	  {
            if (not this->It::has_curr() or show_item(this->It::get_curr()))
	      return ;
	  }
      }
    catch (std::overflow_error) { /* Proceed to overflow; not spread */ }
  }

  void goto_last_valid_item()
  {
    for (this->It::reset_last(); true; this->It::prev())
      if (not this->It::has_curr() or show_item(this->It::get_curr()))
        return;
  }

  void backward()
  {
    this->It::prev();
    try 
      {     
        for (;true; this->It::prev()) 
          if (not this->It::has_curr() or show_item(this->It::get_curr()))
            return;
      }
    catch (std::underflow_error) { /* Proceed without spreading underflow */ }
  }
 
public:

/** @typedef Item Type returning get_current() */
  typedef typename It::Item_Type Item_Type;

/** @typedef Type of iterator. */
  typedef It Iterator_Type;

  const Container & get_container() const
  {
    return *const_cast<Container*>(container_ptr);
  }

/** 
  @brief Returns the iterator background. 
*/
  It & get_iterator() { return *this; }

/** 
  @brief Returns a reference to the filter class.

  @details Wear this function if required to obtain the status of the filter
  (remember that was lost when the destructor is invoked ~ Filter_Iterator() 
*/
  Show_Item & get_filter() { return show_item; }

  void set_filter(Show_Item && si)
  {
    show_item = si;
  }

  void set_filter(Show_Item & si)
  {
    show_item = si;
  }

  Filter_Iterator(Show_Item & si) 
    : show_item(si)
  {
    /* Empty */
  }

  Filter_Iterator(Show_Item && si = Show_Item()) : Filter_Iterator(si)
  {
    /* Empty */
  }

/** 
  @brief Cont_ptr set iterator over filter if. 
*/
  Filter_Iterator(const Container & c, Show_Item & si) 
    : It(c), show_item(si), container_ptr(&const_cast<Container&>(c))
  {
    goto_first_valid_item();
  }

/** 
  @brief Iterator over cont_ptr filter assembly itself. 
*/
  Filter_Iterator(const Container & c, Show_Item && si = Show_Item()) 
    : Filter_Iterator(c, si)
  {
    /* Empty */
  }

/** 
  @brief Iterator over cont_ptr filter assembly itself. 
*/
  Filter_Iterator(const Container & c, void * __cookie, Show_Item & si) 
    : Filter_Iterator(c, si)
  {
    cookie = __cookie;
  }

  void set_cookie(void * __cookie) { cookie = __cookie; }

/** 
  @brief Advances the iterator position. 
*/
  void next() { forward(); }

/** 
  @brief The iterator back one position. 
*/
  void prev() { backward(); }

/** 
  @brief  Place the iterator to the first element of the sequence.
*/
  void reset_first() { goto_first_valid_item(); }

/** @brief Place the iterator on the last item in the sequence. */
  void reset_last() { goto_last_valid_item(); }
};


} /* End namespace Aleph */

# endif // FILTER_ITERATOR_H
