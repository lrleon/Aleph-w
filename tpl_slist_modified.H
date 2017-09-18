
/*
  This file is part of Aleph system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  UNIVERSITY LOS ANDES (ULA) Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA  

  - Center of Studies in Microelectronics & Distributed Systems (CEMISID) 
  - ULA Computer Science Department

  PERMISSION TO USE, COPY, MODIFY AND DISTRIBUTE THIS SOFTWARE AND ITS
  DOCUMENTATION IS HEREBY GRANTED, PROVIDED THAT BOTH THE COPYRIGHT
  NOTICE AND THIS PERMISSION NOTICE APPEAR IN ALL COPIES OF THE
  SOFTWARE, DERIVATIVE WORKS OR MODIFIED VERSIONS, AND ANY PORTIONS
  THEREOF, AND THAT BOTH NOTICES APPEAR IN SUPPORTING DOCUMENTATION.

  Aleph is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  UNIVERSIDAD DE LOS ANDES requests users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  lrleon@ula.ve
  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant Universidad 
  de Los Andes (ULA) the rights to redistribute these changes. 
*/


# ifndef TPL_SLIST_H
# define TPL_SLIST_H

# include <ahDefs.H>
# include <tpl_snode.H>

using namespace Aleph;

namespace Aleph
{
  
template <typename T>
class Slist_Node : public Snode<T>
{
public:

  Slist_Node() : Snode<T>()
  {
    /* Empty */
  }
    
  Slist_Node(const T & _data) : Snode<T>(_data)
  {
    /* Empty */
  }
};
  
template <typename T>
class Slist_Node_Vtl : public Snode<T>
{
public:
  Slist_Node_Vtl() : Snode<T>()
  {
    /* Empty */
  }
    
  Slist_Node_Vtl(const T & _data) : Snode<T>(_data)
  {
    /* Empty */
  }
    
  virtual ~Slist_Node_Vtl()
  {
    /* Empty */
  }
};
  
template <template <typename> class Node, typename T>
class MetaSlistNode : public Node<T>
{

public:

  MetaSlistNode() : Node<T>()
  {
    /* Empty */
  }
    
  MetaSlistNode(const T & _data) : Node<T>(_data)
  {
    /* Empty */
  }
    
  MetaSlistNode * get_next()
  {
    return static_cast<MetaSlistNode*>(Node<T>::get_next());
  }
    
  MetaSlistNode * remove_next()
  {
    return static_cast<MetaSlistNode*>(Node<T>::remove_next());
  }
};
  
  template <template <typename> class Node_Type, typename T>
class GenSlist : public MetaSlistNode<Node_Type, T>
{
    
public:

  typedef MetaSlistNode<Node_Type, T> Node;
    
  GenSlist() { /* empty */ }
    
  void insert_first(Node * node)
  {
    assert(node not_eq nullptr);
    assert(node->is_empty());
    this->insert_next(node);
  }
    
  Node * remove_first() throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("list is empty");

    return this->remove_next();
  }
    
  Node * get_first() throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("list is empty");

    return this->get_next();
  }
    
  class Iterator
  {
      
  private:

    GenSlist * list;
    Node  * current;
      
  public:

    typedef MetaSlistNode<Node_Type, T> Set_Type;
    typedef MetaSlistNode<Node_Type, T> * Item_Type;
      
    Iterator(GenSlist & _list) 
      : list(&_list), current(list->get_first()) { /* Empty */ }
      
    bool has_current() const { return current != list; }
      
    Node * get_current() throw(std::exception, std::overflow_error)
    {
      if (not this->has_current())
	throw std::overflow_error ("");

      return current;
    }
      
    void next() throw(std::exception, std::overflow_error)
    {
      if (not this->has_current())
	throw std::overflow_error ("");

      current = current->get_next();
    }
      
    void reset_first() { current = list->get_next(); }

    Iterator & operator = (Node * node)
    {
      if (this == node)
	return *this;

      current = node;
      return *this;
    }
  };
};

template <typename T>
class Slist : public GenSlist<Slist_Node, T>
{

public:

  typedef typename GenSlist<Slist_Node, T>::Node Node;
  typedef typename GenSlist<Slist_Node, T>::Iterator Iterator;
};

template <typename T>
class SlistVtl : public GenSlist<Slist_Node_Vtl, T>
{

public:

  typedef typename GenSlist<Slist_Node_Vtl, T>::Node Node;
  typedef typename GenSlist<Slist_Node_Vtl, T>::Iterator Iterator;
};

} // end namespace Aleph
# endif /* TPL_SLIST_H */
