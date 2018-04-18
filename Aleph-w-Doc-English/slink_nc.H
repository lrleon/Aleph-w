
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
  @file slink_nc.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef SLINK_NC_H
# define SLINK_NC_H

# include <aleph.H>

namespace Aleph
{
/** 
  @brief Just a list of non-circular link nodes.

  @details Slink_Nc modeled a pointer to a node in a list
  single linked noncircular.

  @ingroup Secuencias

  @see Snode Slist DynSlist Dlink 
*/
  class Slink_Nc
  {
  protected:
    Slink_Nc * next;
  
  public:
/** 
  @brief Empty constructor. 
*/
    Slink_Nc() : next(NULL) { /* Empty */ }

/** 
  @brief Copy constructor, placed link to point toNULL. 
*/
    Slink_Nc(const Slink_Nc &) : next(NULL) { /* Empty */ }

    ~Slink_Nc() { /* Empty */ }

/** 
  @brief Assignment, placed link to point to NULL. 
*/
    Slink_Nc & operator = (const Slink_Nc & link)
    {
      if (&link == this)
        return *this;

      if (not is_empty())
        throw std::invalid_argument("link is not empty");

      next = NULL; 
    }

/** 
  @brief Restart link to point to NULL 
*/
    void reset()
    {
      assert(this not_eq NULL);
      next = NULL;
    }

/** 
  @brief Returns true if this rate is empty (point to NULL) 
*/
    bool is_empty() const
    {
      assert(this not_eq NULL);
      return next == NULL;
    }

/** 
  @brief Returns the following link.
*/
    Slink_Nc *& get_next()
    {
      assert(this not_eq NULL);
      return next;
    }

/** 
  @brief Bond inserted after p this.

  @details insert(p) insert a single node whose link is p then
  the node whose link is this.

  @param[in] p Simple link to insert.
*/
    void insert(Slink_Nc * p)
    {
      assert(this not_eq NULL);
      assert(p not_eq NULL);
      assert(p->is_empty());
      p->next = next;
      next = p;
    }

/** 
  @brief It deletes the following link to this.

  @details remove_next() eliminating the single node located after
  the node whose link is esta and returns the link address
  removed.

  @return The removed link 
*/
    Slink_Nc * remove_next()
    {
      assert(this not_eq NULL);
      Slink_Nc * ret_val = next;
      next = ret_val->next;
      ret_val->reset();
      return ret_val;
    }

    class Iterator
    {
      Slink_Nc * head;
      Slink_Nc * curr;
    
    public:
      Iterator() : head(NULL), curr(NULL) { /* Empty */ }

      Iterator(Slink_Nc * head_ptr) : head(head_ptr), curr(head->get_next())
      { 
        /* Empty */
      }

      Iterator(Slink_Nc & _head) : head(&_head), curr(head->get_next())
      { 
        /* Empty */
      }

      Iterator(Slink_Nc * head_ptr, Slink_Nc * curr_ptr)
        : head(head_ptr), curr(curr_ptr)
      {
        /* Empty */
      }

/** 
  @brief Iterator restarts first node in the list
*/
      void reset_first() 
      {
        assert(curr != NULL and head != NULL);
        curr = head->get_next();
      }

      void set(Slink_Nc * new_curr) 
      {
        assert(curr != NULL and head != NULL);
        curr = new_curr;
      }

      void reset(Slink_Nc * new_head)
      {
        head = new_head;
        curr = head->get_next();;
      }

      bool has_current() const 
      {
        assert(head != NULL);
        return curr != NULL;
      }

/** 
  @brief Synonymous has_current() 
*/
      bool has_curr() const { return has_current(); }

/** 
  @brief Returns current node address
*/
      Slink_Nc * get_current()  
      {
        assert(curr != NULL and head != NULL);

        if (not has_current())
          throw std::overflow_error("Not element in list");

        return curr;
      }

/** 
  @brief Synonymous get_current() 
*/
      Slink_Nc * get_curr() { return get_current(); }

/** 
  @brief Returns true if the iterator is on first element
*/
      bool is_in_first() const { return curr == head->next; }

/** 
  @brief Iterator advances in a position
*/
      void next() throw(std::exception, std::overflow_error)
      {
        if (not has_current())
          throw std::overflow_error("Not next element in list");

        curr = curr->get_next();
      }

/**
  @brief Returns true if iterators are on the same element.
*/
      bool operator == (const Iterator & it) const { return curr == it.curr; }

/** 
  @brief Returns true if iterator have different states.
*/
      bool operator != (const Iterator & it) const { return curr != it.curr; }
    
      bool verify(Slink_Nc * l) const { return head == l; }

      bool verify(const Iterator & it) const { return head == it.head; }
    };
    
  };
}

# endif
