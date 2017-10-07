
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
  @file Priority_Queue.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AH_PRIORITY_QUEUE_H
# define AH_PRIORITY_QUEUE_H

# include <tpl_dynBinHeap.H>

namespace Aleph {

/** 
  @brief Aleph implementation of standard container prority_queue<T>.

	@details In this standard container Aleph is implanted through a
  the DynBinHeap class, which in turn is based on trees
  binaries. So this implementation is based on an arrangement and is
  highly dynamic. Other structures or algorithms can
  pointers maintained without risk to the data contained in the tail
  priority.

  This version of priority_queue <T> does not use a container
  generic type parameter. Consequently, builders are slightly modified standard.

	@see DynBinHeap

	@ingroup stl
*/
    template <typename T, class Compare = Aleph::less<T> >
class priority_queue : private Aleph::DynBinHeap<T, Compare>
{
public:

/** 
  @typedef Value type that stores the priority queue. 
*/
  typedef T value_type;

/** 
  @brief Instance a priority queue empty
*/
  priority_queue() { /* Empty */ }

/** 
  @brief Instance queue with all elements of the container cont.
*/
      template <class Container>
  priority_queue(Container & cont)
  {
    const typename Container::iterator end = cont.end();

    for (typename Container::iterator it(cont.begin()); 
	 it != end; this->insert(*it++));
  }

/** 
  @brief Instance a queue with elements between the
  range [beg..end) associated with the container iteradores. 
*/
      template <class Itor>
  priority_queue(Itor beg, const Itor & end)
  {
    Aleph::verify_iterators (beg, end);

    while (beg != end)
      this->insert(*beg++);
  }

/** 
  @brief Inserts a copy of value in the priority queue; 
  returns a modifiable reference to the inserted copy 
  (this is not part of the standard).
*/
  T & push(const T & value) { return this->insert(value); }

/** 
  @brief Removes the smallest element of the priority queue according to the
  comparison criteria specified in the declaration. 
*/
  void pop() { this->getMin(); }

/** 
  @brief Returns the smallest element of the priority queue according to the
  comparison criteria specified in the declaration. 
*/
  const T & top() { return Aleph::DynBinHeap<T, Compare>::top(); }

/** 
  @brief Returns true if the priority queue is empty.
*/
  bool empty() const { return this->is_empty(); }

/** 
  @brief Returns the number of items in the queue. 
*/
  size_t size() const { return Aleph::DynBinHeap<T, Compare>::size(); }
};

} /* End namespace Aleph */

# endif // AH_PRIORITY_QUEUE_H
