
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
  @file arcHeap.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef ARCHEAP_H
# define ARCHEAP_H

# include <tpl_binHeap.H>
# include <tpl_graph_utils.H>

    template <class GT, 
	      class Distance, 
	      class Access_Heap_Node>
class ArcHeap 
 : public BinHeap<typename GT::Arc*, Distance_Compare<GT, Distance>>
{
  Distance &                     dist;
  Access_Heap_Node &             access_node;
  Distance_Compare<GT, Distance> dist_cmp;

public:

  Distance & get_distance() { return dist; }

  ArcHeap(Distance & __dist, Access_Heap_Node && acc = Access_Heap_Node())
    : dist(__dist), access_node(acc), dist_cmp(dist)
  {
    /* Empty */
  }

      typedef 
  BinHeap<typename GT::Arc*, Distance_Compare<GT, Distance> > Heap;

  typedef typename Heap::Node Node;

  void put_arc(typename GT::Arc * arc, typename GT::Node * tgt)
  {
    Node *& heap_node = access_node(tgt);

    /** Is there arc inserted into the heap ?
    If the answer is No, then create a new node for the
    heap and assign bow */
    if (heap_node == NULL) 
      {     
	assert(heap_node == NULL); /* To be consistent */

	heap_node = new Node;
	heap_node->get_key() = arc;
	this->insert(heap_node);

	return;
      }

    /* Two arcs with the same fate == > take less ; discard more */
    typename GT::Arc *& arc_in_heap = heap_node->get_key();

    /**¿ Arc_in_heap have less distance to arc ?
    If the answer is YES, then the ancient arch remains in the
    and the new heap is ignored * /
    if (dist_cmp(arc_in_heap, arc)) 
      return; /* Antiguo arco permanece en el heap; nuevo se ignora */

    /* Arc_in_heap ¿ Will the newly inserted arc arc * /
    arc_in_heap = arc; /* Change the arc */
    this->update(heap_node); /* Update the heap with the new weight of arc */
  }

  typename GT::Arc * get_min_arc()  
  {
    Node * heap_node = this->getMin();
    typename GT::Arc * arc = heap_node->get_key();

    /* Select node that returns the kind of access */
    typename GT::Node * p = (typename GT::Node*) arc->src_node;
    if (access_node(p) != heap_node)
      p = (typename GT::Node*) arc->tgt_node;

    assert(access_node(p) == heap_node); 

    access_node(p) = NULL; 

    delete heap_node;

    return arc;
  }

  void empty()
  {
    this->remove_all_and_delete();
  }

  ~ArcHeap() 
  {
    empty();
  }  
};

# endif // ARCHEAP_H
