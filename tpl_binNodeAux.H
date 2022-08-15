
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
# ifndef TPL_BINNODE_H
# define TPL_BINNODE_H
 
# include <ahDefs.H>
# include <ahAssert.H>
# include <ahNew.H>


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
virtual ~name##Vtl() { /* empty */ }                 \
}

# define SET_BINNODE_NULL_POINTER(ptr, name)                        \
template <class Key> name<Key> * name<Key>::NullPtr = ptr;          \
template <class Key> name##Vtl<Key> * name##Vtl<Key>::NullPtr = ptr


DECLARE_BINNODE(BinNode, 255, Aleph::Empty_Class);

SET_BINNODE_NULL_POINTER(nullptr, BinNode);


# endif

