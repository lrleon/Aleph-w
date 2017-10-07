
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
  @file tpl_skipList.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_SKIPLIST_H
# define TPL_SKIPLIST_H

# include <aleph.H>
# include <limits.h>
# include <ran_array.h>

/**
  @brief This is an ordered list, whose nodes are implemented with an array of
  pointers, that it's the meaning of "Skip List"  
 
  @details SkipList is a data structure proposed by William Pugh, in which  every node 
  has an array of pointers  to its next nodes (simple link), every node has
  a level ( integer value ), so the node's level is the array's length. 

  There is a list's head,  the head's level  is  equal to maxLevel. 
  There is a sentinel node (Null Node ) with the highest key that might
  improve all algoritms. The node's level is a random number that is
  generated at insertion time. 

  It's necessary alloc nodes with operator (NewSize), and calculate the
  forward's size as follow: 

  sizeof(SkipList<Key,Type>::Node)*level, 

  it's very important don't forget this condition, if you use 
  operator (New) instead of NewSize then will crash your code.
*/

    template <class Key, class Type> 
class SkipList
{

public:
  static const   int            maxLevel =  32; /** 2^5 :  Maximum level of a node */

  static const double defaultProbability; /** Probability used in random level function */   

  typedef double (*RandomFct)();
 
  class Node
  {
  protected:
  
    int level;

    static Node nodeSentinel;
  
/** @brief Public methods work in function of NULL pointer in order to indicate
unsuccessful conditions. We put NullPtr as private in order to avoid
that to accidentally put NullPtr instead of NULL */
    static Node* NullPtr;

  private:

    Key  key;
    Type data;
    
    void fillForwardNULL()
    {
      for(int i = 0; i < level; i++)
	forward [ i ] = const_cast<Node*>(&nodeSentinel);
    }
       
  public:
       
    friend class SkipList<Key, Type>;

    Node(const Key& _key, const Type& _data, int n)
      : level   ( n     ),
        key     ( _key  ),
	data    ( _data )
    {
      /* Empty */
    }

    Node(const Key& _key, int  n)
      : level   ( n    ),
        key     ( _key )
    {
      /* Empty */
    }

    Node(int n)
      : level( n )
    {
      /* Empty */
    }
    
    Node() 
      : level( 0              ),
        key ( computeMaxKey() )
    {
      /* Empty */
    }

    ~Node()
    {
      /* Empty */
    }

    Node* get_next()
    {
      return ( getForward(0) == NullPtr ) ? NULL : getForward( 0 );
    }
 
    Node*& getForward(int i) 
    {
      assert( i < level );
      return forward[i];
    }
     
    Node** getForward() 
    {
      return forward;
    }

    Key  get_key() const { return key; }

    Key& get_key() { return key; }

    Type get_data() const { return data; }


    Type& get_data() { return data; }

    int getLevel() const { return level; }

    Key computeMaxKey();

  protected:
    
    Node* forward[0];
  };

    
private:

    /* Headenode : it's used for header node, 
    it's an static array with maxLevel elements */ 
  class HeaderNode : public SkipList<Key, Type>::Node
  {
  private:
  
    typename SkipList<Key, Type>::Node * forward[maxLevel];
    
    friend class SkipList<Key,Type>;
 
  public:

    HeaderNode() 
    {
      this->level = maxLevel;

      for(int i = 0; i < maxLevel; i++ )
	HeaderNode::forward[i] = Node::NullPtr;
    }

    Node*& getForward(int i) 
    {
      assert( i < this->getLevel() );
      assert( this->getLevel() <= maxLevel );

      return HeaderNode::forward[ i ];
    }

  };

  /*
    Node*& getForward(Node* p, int i)
    {
    if ( p == headerPtr )
    return  ((HeaderNode*) p)->HeaderNode::forward[ i ];
    else
    return p->Node::forward[ i ];   
    }
  */

  HeaderNode  header;         /* SkipList's header */
  HeaderNode* headerPtr;      /* Pointer to header */
  double probability;         /* Probability for randomLevel */
  int level;                  /* Current Maximum level of list linked */ 
  RandomFct randomFct;        /* Random function */


public:

  SkipList(RandomFct fct, double p = defaultProbability)
    : headerPtr    ( &header ),
      probability  ( p       ),
      level        ( 0       )
  {
    assert( probability>=0 && probability<=1);
    assert( fct != NULL );

    randomFct = fct;
  }

  ~SkipList()
  {
    /* Empty */
  }
  
/**
  @brief Searches a key in a Skip list.

  @return A pointer to the node containing the key if the key is found;
  NULL otherwise.

  @param key Key to search
*/

  Node* search(const Key& searchKey)
  {
    Node *x = headerPtr;

    for(int i = level-1; i >= 0; i-- )
      {
	while( x->getForward(i)->get_key() < searchKey )
	  x = x->getForward( i );
      }
    x = x->getForward( 0 );
      
    if ( x == Node::NullPtr )  /* Highest key is  NULL or NIL */
      return NULL;

    assert( x->get_key() == searchKey );
    
    return x; 
  }

/**
  @brief Inserts an item at orderer list (skip list).  

  @param item Node to be inserted at orderer list. 
*/

  Node* insert(Node* p)
  {
    int i;
    Node* update[ maxLevel ];
    Node  *x =  headerPtr;

    assert(p->getLevel() > 0 && p->getLevel() <= maxLevel );

    p->fillForwardNULL();

    for(i = level - 1; i >= 0; i-- )
      {
	
	while( x->getForward(i)->get_key() < p->get_key() )
	  x = x->getForward(i);
	    
	update[ i ] = x;
      }

    x = x->getForward( 0 ); /* Go to next node */ 
       
    if ( p->getLevel() > level )
      {
	for(i = level; i < p->getLevel(); i++)
	  update[i] = headerPtr;
	level = p->getLevel();
      }

		  
    for( i = 0; i < p->getLevel() ; i++ )
      {
	p->getForward(i)           = update[i]->getForward(i);
	update[i]->getForward(i) = p;
      }
	
    assert( checkSkipList() == true );
    return p;
  }

  Node* get_first() 
  {
    return headerPtr->HeaderNode::forward[0];
  }

/** 
  @brief Remove a key from a Skip List.

  @details Searches a key in a Skip List  and remove the containing the key if this
  is found.

  @return A pointer to node containing the removed key.

  @param searchKey Key to search
*/ 

  Node*  remove(const Key& searchKey)
  {
    Node* update[ maxLevel ];
    Node* x = headerPtr;      
    int i;

    for(i = level - 1; i >= 0; i--)
      {
	while( x->getForward(i)->get_key() < searchKey )
	  x = x->getForward(i);
	update[ i ] = x;
      }

    x = x->getForward( 0 );
      
    if ( x->get_key() == searchKey )
      {
	for( i = 0; i < level; i++ )
	  {
	    if ( update[i]->getForward(i) != x )
	      break;
	    update[i]->getForward(i) = x->getForward(i);
	  }

	while( level > 0  && headerPtr->getForward(level) == NULL )
	  level--;
	  
      }
    else 
      return NULL;
	
    assert( checkSkipList() == true );

    return x;
  }

/** @brief random fuction used for generate every node's level */
  int generateRandomLevel()
  {
    int l = 1;

    assert( probability > 0 && probability < 1 );
     
    while( (*randomFct)() < probability && l < maxLevel )
      l++;
    return l;
  }


/** @brief check the ordered list, all keys must be in ascendant order */
  bool checkSkipList()
  {
    Node* node = header.get_next();
    while( node != NULL )
      {
	if ( node->get_next() == NULL ) 
	  break;

	if ( node->get_key() > node->get_next()->get_key() ) 
	  return false;
	node = node->get_next(); 
      }
    return true;
  }

  Node * new_node() { return NULL; }
};

    
    /*  BEWARE: Ensure that this instantiation to be unique */
    
    template <class Key, class Type>
typename SkipList<Key, Type>::Node SkipList<Key, Type>::Node::nodeSentinel;

    template <class Key, class Type> 
typename SkipList<Key, Type>::Node * SkipList<Key, Type>::Node::NullPtr = 
  &SkipList<Key, Type>::Node::nodeSentinel;


    template <class Key, class Type>
const double SkipList<Key, Type>::defaultProbability = 0.5;
    


# endif /* TPL_SKIPLIST_H */
