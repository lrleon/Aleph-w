/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef TPL_SKIPLIST_H
# define TPL_SKIPLIST_H

# include <aleph.H>
# include <limits.h>
# include <ran_array.h>

/*
  This is an ordered list, whose nodes are implemented with an array of
  pointers, that it's the meaning of "Skip List"  
 
  SkipList is a data structure proposed by William Pugh, in which  every node 
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
  static const   int            maxLevel =  32; /* 2^5 :  Maximum level of a
						   node */

  static const double defaultProbability; /* Probability used in
					     randomLevel function */   

  typedef double (*RandomFct)();
 
  class Node
  {
  protected:
  
    int level;

    static Node nodeSentinel;
  
    /* 
       Public methods work in function of nullptr pointer in order to indicate
       unsuccessful conditions. We put NullPtr as private in order to avoid
       that to accidentally put NullPtr instead of nullptr
    */
    static Node* NullPtr;

  private:

    Key  key;
    Type data = Type();
    
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
      /* EMPTY */
    }

    Node(const Key& _key, int  n)
      : level   ( n    ),
        key     ( _key )
    {
      /* EMPTY */
    }

    Node(int n)
      : level( n )
    {
      /* EMPTY */
    }
    
    Node() 
      : level( 0              ),
        key ( computeMaxKey() )
    {
      /* EMPTY */
    }

    ~Node()
    {
      /* EMPTY */
    }

    Node* get_next()
    {
      return ( getForward(0) == NullPtr ) ? nullptr : getForward( 0 );
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

  /*
    Headenode : it's used for header node, 
    it's an static array with maxLevel elements 
  */ 
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

  HeaderNode  header;               // SkipList's header
  HeaderNode* headerPtr;            // Pointer to header
  double probability;               // probability for randomLevel
  int level;                        // Current Maximum level of list linked  
  RandomFct randomFct;              // random function  


public:

  SkipList(RandomFct fct, double p = defaultProbability)
    : headerPtr    ( &header ),
      probability  ( p       ),
      level        ( 0       )
  {
    assert( probability>=0 && probability<=1);
    assert( fct != nullptr );

    randomFct = fct;
  }

  ~SkipList()
  {
    /* EMPTY */
  }
  
  /**
     Searches a key in a Skip list.

     @return a pointer to the node containing the key if the key is found;
     nullptr otherwise.
     @param key key to search
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
      
    if ( x == Node::NullPtr )  // highest key is  nullptr or NIL 
      return nullptr;

    assert( x->get_key() == searchKey );
    
    return x; 
  }

  /**
     Inserts an item at orderer list (skip list).  

     @param item node to be inserted at orderer list.
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

    x = x->getForward( 0 ); // go to next node     
       
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

  /* 
     Remove a key from a Skip List.

     Searches a key in a Skip List  and remove the containing the key if this
     is found.

     @return a pointer to node containing the removed key.
     @param key to search
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

	while( level > 0  && headerPtr->getForward(level) == nullptr )
	  level--;
	  
      }
    else 
      return nullptr;
	
    assert( checkSkipList() == true );

    return x;
  }

  /* random fuction used for generate every node's level  */

  int generateRandomLevel()
  {
    int l = 1;

    assert( probability > 0 && probability < 1 );
     
    while( (*randomFct)() < probability && l < maxLevel )
      l++;
    return l;
  }


  /* check the ordered list, all keys must be in ascendant order */
  bool checkSkipList()
  {
    Node* node = header.get_next();
    while( node != nullptr )
      {
	if ( node->get_next() == nullptr ) 
	  break;

	if ( node->get_key() > node->get_next()->get_key() ) 
	  return false;
	node = node->get_next(); 
      }
    return true;
  }

  Node * new_node() { return nullptr; }
};

/*
  BEWARE: Ensure that this instantiation to be unique
*/
    template <class Key, class Type>
typename SkipList<Key, Type>::Node SkipList<Key, Type>::Node::nodeSentinel;

    template <class Key, class Type> 
typename SkipList<Key, Type>::Node * SkipList<Key, Type>::Node::NullPtr = 
  &SkipList<Key, Type>::Node::nodeSentinel;


    template <class Key, class Type>
const double SkipList<Key, Type>::defaultProbability = 0.5;
    


# endif /* TPL_SKIPLIST_H */




