
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
  @file tpl_binNodeAux.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_BINNODE_H
# define TPL_BINNODE_H
 
# include <ahDefs.H>
# include <ahAssert.H>
# include <ahNew.H>

    /* TODO Documentar para que se usa esta clase. */
# define CORPUS_BINNODE(name, height, Control_Data)                   \
    template <class Key>                                              \
class name : public Control_Data                                      \
{                                                                     \
public:                                                               \
                                                                      \
  static const size_t MaxHeight = height;                             \
                                                                      \
  static name * NullPtr;                                              \
                                                                      \
private:                                                              \
                                                                      \
  Key    key;                                                         \
  name * lLink;                                                       \
  name * rLink;                                                       \
                                                                      \
public:                                                               \
                                                                      \
  Key& get_key() { return key; }                                       \
                                                                      \
  name*& getL() { return lLink; }                                     \
                                                                      \
  name*& getR() { return rLink; }                                     \
                                                                      \
  name(const Key& k) : key(k), lLink(NullPtr), rLink(NullPtr)         \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(const Control_Data & control_data, const Key& k) :             \
    Control_Data(control_data),                                       \
    key(k), lLink(NullPtr), rLink(NullPtr)                            \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(const Control_Data & control_data) :                           \
    Control_Data(control_data),                                       \
    lLink(NullPtr), rLink(NullPtr)                                    \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name() : lLink(NullPtr), rLink(NullPtr)                             \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(SentinelCtor) : Control_Data(sentinelCtor) { /* Empty */ }     \
                                                                      \
  void reset() { rLink = lLink = NullPtr; }



# define DECLARE_BINNODE(name, height, Control_Data) \
CORPUS_BINNODE(name, height, Control_Data)           \
};                                                   \
CORPUS_BINNODE(name##Vtl, height, Control_Data)      \
virtual ~name##Vtl() { /* Empty */ }                 \
}

# define SET_BINNODE_NULL_POINTER(ptr, name)                        \
template <class Key> name<Key> * name<Key>::NullPtr = ptr;          \
template <class Key> name##Vtl<Key> * name##Vtl<Key>::NullPtr = ptr


DECLARE_BINNODE(BinNode, 255, Aleph::Empty_Class);

SET_BINNODE_NULL_POINTER(NULL, BinNode);


# endif
