
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file tpl_snode_nc.H
 *  @brief Typed singly linked node (no check).
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_SNODE_NC_H
# define TPL_SNODE_NC_H

# include <slink_nc.H>

namespace Aleph
{
  template <typename T> class Snode_Nc : public Slink_Nc
  {
    T data;

  public:
    /// Empty constructor.
    Snode_Nc() : Slink_Nc() { /* empty*/ }

    /// Constructor that copies data.
    Snode_Nc(const T & _data) : Slink_Nc(), data(_data) { /* empty */ }

    ~Snode_Nc() { /* empty */ }

    /// Returns a reference to the data contained in the node.
    T & get_data() { return data; }

    /** @brief Returns a reference to the data contained in the node (constant version).
        @return constant reference to the data.
        @throws none
    */
    const T & get_data() const { return data; }
  
    /** Removes the node following this.

        remove_next() removes the node linked after this and returns
        its memory address.
          
        @return the removed node.
    */
    Snode_Nc * remove_next()
    { 
      return (Snode_Nc *) Slink_Nc::remove_next();
    }
    
    /// Returns the node following this.
    Snode_Nc *& get_next()
    {
      return (Snode_Nc *&) Slink_Nc::get_next();
    }

    /** @brief Returns the node following this (constant version).
        @return a constant pointer to the next node.
        @throws none
    */
    const Snode_Nc * get_next() const
    {
      return (const Snode_Nc *) Slink_Nc::get_next();
    }

    class Iterator : public Slink_Nc::Iterator
    {
    public:
      Iterator() { /* empty */ }

      /// Instantiates iterator in list whose head node is head_ptr.
      Iterator(Snode_Nc<T> * head_ptr)
        : Slink_Nc::Iterator(head_ptr)
      {
        // empty
      }

      /// Instantiates iterator in list whose head node is head.
      Iterator(Snode_Nc<T> & head) : Slink_Nc::Iterator(&head) { /* empty */ }

      // Instantiates iterator in list whose head node is head_ptr and
      // sets the current element to the one pointed by curr_ptr.
      Iterator(Snode_Nc<T> * head_ptr, Snode_Nc<T> * curr_ptr) 
        : Slink_Nc::Iterator(head_ptr, curr_ptr)
      {
        // Empty
      }

      Snode_Nc * get_curr() const /// Returns pointer to the current node.
      {
        return (Snode_Nc *) this->Slink_Nc::Iterator::get_curr(); 
      }
    };
  };
}

# endif
