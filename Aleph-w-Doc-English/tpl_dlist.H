
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
  @file tpl_dlist.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DLIST_H
# define TPL_DLIST_H

# include <tpl_dnode.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Node without virtual destructor of doubly linked list.

  @param T Data type that contains the node.

  @see Dlist GenDlist

  @ingroup Secuencias
*/
    template <typename T>
class Dlist_Node : public Dnode<T>
{
public:

  Dlist_Node() : Dnode<T>() { /* Empty */ }

  Dlist_Node(const T & _data) : Dnode<T>(_data) { /* Empty */ }
};
/** 
  @brief Node with virtual destructor of doubly linked list.

  @param T Data type that contains the node.

  @see DlistVtl GenDlist

  @ingroup Secuencias
*/
    template <typename T>
class Dlist_Node_Vtl : public Dlist_Node<T>
{
public:

  Dlist_Node_Vtl() : Dlist_Node<T>() { /* Empty */ }

  virtual ~Dlist_Node_Vtl() { /* Empty */ }

  Dlist_Node_Vtl(const T & _data) : Dlist_Node<T>(_data) { /* Empty */ }
};  
    template <template <class> class Node, typename T>
class MetaDlistNode : public Node<T>
{
public:

  MetaDlistNode() { /* Empty */ }

  MetaDlistNode(const T& _data) : Node<T>(_data) { /* Empty */ }

  MetaDlistNode(const MetaDlistNode<Node, T> & node) 
    : Node<T>(node) { /* Empty */ }

  MetaDlistNode<Node, T> *& get_next() 
  {
    return (MetaDlistNode<Node, T> *&) Node<T>::get_next(); 
  }
  MetaDlistNode<Node, T> *& get_prev()  
  {
    return (MetaDlistNode<Node, T> *&) Node<T>::get_prev(); 
  }
};

/** 
  @brief Generic doubly linked list of nodes.

  @details GenDlist<Node_Type, T> implements a doubly linked list,
  circular, of nodes.

  @param Node_Type Node type used.
  It can be of two types:
  -# Dlist_Node: Node without virtual destructor.
  -# Dlist_Node_Vtl: Node with virtual destructor. This class
  node should be used if required invoke the destroyer
  a derived class type node that contains the list.
  @param T Data type that houses the node.

  @ingroup Secuencias
*/ 
    template <template <class> class Node_Type, typename T>
class GenDlist : public MetaDlistNode<Node_Type, T>
{
public:

/** @brief List Node Type
  typedef MetaDlistNode<Node_Type,T> Node;
/** 
  @brief Return address of the first node.

  @details Check the first node of the list.

  @return A pointer to the first node of the list. 

  @throw underflow_error If the list is empty.
*/
  Node * get_first() 
    throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("List is empty");

    return this->get_next();
  }
/** 
  @brief Return address of the last node.

  @details Check the last node of the list.

  @return A pointer to the last node of the list.

  @throw underflow_error If the list is empty.
*/
  Node * get_last() 
    throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("List is empty");
    return this->get_prev();
  }
/** 
  @brief Removes first node and returns its address.

  @details Removes the first node in the list and returns a pointer to the node
  removed.

  @return A pointer to the node removed.

  @throw underflow_error If the list is empty.
*/
  Node * remove_first() 
    throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("List is empty");

    Node* retValue = this->get_next();
    retValue->del();
    return retValue;
  }
/** 
  @brief Removes last node and returns its address.

  @details Removes the last node in the list and returns a pointer to the node
  removed.

  @return A pointer to the node removed.
  
  @throw underflow_error If the list is empty.
*/
  Node * remove_last() 
    throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("List is empty");
    Node* retValue = this->get_prev();
    retValue->del();
    return retValue;
  }
/** 
  @brief Iterator over the nodes in a doubly linked list.

  @see GenDlist Dlist DlistVtl
      
  @ingroup Secuencias
*/
  class Iterator : public Node::Iterator
  {
  public:
/** @brief Set type which is iterated. */
    typedef GenDlist<Node_Type, T> Set_Type;

/** @brief Type returning get_current(). */
    typedef Node * Item_Type;

/** @brief Constructor. */
    Iterator(GenDlist<Node_Type, T> & list) : Node::Iterator(&list) { }

/** @brief Constructor with current on curr node. */
    Iterator(GenDlist<Node_Type, T> & list, GenDlist<Node_Type, T> * curr) 
      : Node::Iterator(&list, curr) 
    {
      /* Empty */
    }

/** @brief Empty constructor. */
    Iterator() : Node::Iterator() { /* Empty */ }

/** @brief Copy constructor. */
    Iterator(const Iterator & it) : Node::Iterator(it) { /* Empty */ }

/** @brief Iterator assignment. */
    Iterator & operator = (const Iterator & it) 
    {
      if (this == &it)
        return *this;

      Node::Iterator::operator = (it);
      return *this;
    }
/** @brief Returns the current node */
    Node * get_current() { return (Node*) Node::Iterator::get_current(); }

/** @brief Deletes the current node and advances the iterator forward. */
    Node * del() { return (Node*) Node::Iterator::del(); }
  };
};
    
/** 
  @brief Doubly linked list of nodes without virtual destructor.

  @see DlistVtl
        
  @ingroup Secuencias
*/ 
    template <typename T> 
class Dlist : public GenDlist<Dlist_Node, T> 
{
public:

/** @brief Type node that manages the list. */
  typedef typename GenDlist<Dlist_Node, T>::Node Node;

/** @brief Iterator type that manages the list. */
  typedef typename GenDlist<Dlist_Node, T>::Iterator Iterator;
};
    
/** 
  @brief Doubly linked list of nodes with virtual destructor.

  @see Dlist
        
  @ingroup Secuencias
*/ 
    template <typename T> 
class DlistVtl : public GenDlist<Dlist_Node_Vtl, T> 
{
public:

/** @brief Type node that manages the list. */
  typedef typename GenDlist<Dlist_Node_Vtl, T>::Node Node;

/** @brief Iterator type that manages the list. */
  typedef typename GenDlist<Dlist_Node_Vtl, T>::Iterator Iterator;
};

} /* End namespace Aleph */

# endif // TPL_DLIST_H
