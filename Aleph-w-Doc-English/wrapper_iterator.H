
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
  @file wrapper_iterator.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


#line 5097 "grafos.nw"
# ifndef WRAPPER_ITERATOR_H
# define WRAPPER_ITERATOR_H

/** 
  @brief Iterator wrapper over another iterator.

 	@details An iterator is an iterator wrapper receiving rates
  the following types parameterized:
  
  -# Container: Container elements among most of as defined in Aleph. 
  -# It: Iterator class must belong to the iterator for
  the Container class. Since there might be several kinds of
  associated iterator(Container::It1, Container::It2, ...).
  -# Show_Item: Class determines whether an item
  should not be set or displayed by the iterator. the
  above determination is made by logic called (bool)
  Show_Item::operator()(Container&, It, T) which should return
  true if the item should be displayed or false otherwise.
  .
  
  Wrapper_Iterator is to export a generic iterator
  filter elements of the set according to some criterion
  Show_Item defined by the class. This allows algorithms
  using generic Wrapper_Iterator have different
  Show_Item behaviors depending on the filter.

  Aleph containers exported two major types in
  Wrapper_Iterator context: SET_TYPE and ITEM_TYPE, the
  which correspond to the type of joint on which
  iterates and type of item that returns the iterator It.

  @note This class is designed to Aleph, it does not work for the
  std standard library.

  @ingroup Secuencias
*/
    template <class Container, class It, class Show_Item>
class Wrapper_Iterator : public It
{

private:

  
#line 5175 "grafos.nw"
Container * cont;
#line 5182 "grafos.nw"
void goto_first_valid_item()
{
  try
    { 
    /** Placed in the first element that accepts Show_Item */
      for (It::reset_first(); true; It::next())
	if (not It::has_current() or Show_Item () (*cont, It::get_current()))
	  return;
    }
  catch (std::overflow_error) { /* It stays in overflow without spreading */ }
}
void forward()
{
  It::next(); 
  try 
    {     
    /** Advance to the next item that accepts Show_Item */
      for (;true; It::next()) 
	if (not It::has_current() or Show_Item () (*cont, It::get_current()))
	  return;
    }
  catch (std::overflow_error) { /* It stays in overflow without spreading */ }
}
#line 5220 "grafos.nw"
void goto_last_valid_item()
{
  for (It::reset_last(); true; It::prev())
    if (not It::has_current() or Show_Item () (*cont, It::get_current()))
      return;
}
void backward()
{
  It::prev();
  try 
    {     
      for (;true; It::prev()) 
	if (not It::has_current() or Show_Item() (*cont, It::get_current()))
	  return;
    }
  catch (std::underflow_error) { /* It stays in overflow without spreading */ }
}

#line 5139 "grafos.nw"
public:

  
#line 5245 "grafos.nw"

/** 
  @typedef Type element returns get_current()
*/
typedef typename It::Item_Type Item_Type;

Wrapper_Iterator() : cont(NULL) { /* Empty */ }

/** 
  @brief Iterator on set conj. 
*/
Wrapper_Iterator(Container & cont) : It(cont), cont(&cont)
{
  goto_first_valid_item();
}

/** 
  @brief Copy Constructor. 
*/
Wrapper_Iterator(const Wrapper_Iterator & it) : It(it), cont(it.cont) { /* Empty */ }
Wrapper_Iterator & operator = (const  Wrapper_Iterator & it) 
{
  *((It*) this) = it;
  cont = it.cont;
  return *this;
}

/** 
  @brief Advances the iterator position. 
*/
void next() { forward(); }

/** 
  @brief Back the iterator position. 
*/
void prev() { backward(); }

/** 
  @brief Place the iterator to the first element of the sequence. 
*/
void reset_first() { goto_first_valid_item(); }

/** 
  @brief Place the iterator on the last item in the sequence. 
*/
void reset_last() { goto_last_valid_item(); }
#line 5142 "grafos.nw"
};


# endif // WRAPPER_ITERATOR_H
