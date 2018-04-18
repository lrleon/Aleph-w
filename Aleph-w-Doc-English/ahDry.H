
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
  @file ahDry.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AHDRY_H
# define AHDRY_H

# include <functional>
# include <sstream>

# include <ahFunctional.H>

namespace Aleph 
{

  template <typename T> class DynList;
  
# define Generic_Traverse(Type)						\
  template <class Operation>						\
  bool traverse(Operation & operation) const				\
  {									\
    for (Iterator it(*this); it.has_curr(); it.next())			\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
  									\
  template <class Operation>						\
  bool traverse(Operation & operation)					\
  {									\
    for (Iterator it(*this); it.has_curr(); it.next())			\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
									\
  template <class Operation>						\
  bool traverse(Operation && operation = Operation()) const		\
  {									\
    return traverse<Operation>(operation);				\
  }									\
  									\
  template <class Operation>						\
  bool traverse(Operation && operation = Operation())			\
  {									\
    return traverse<Operation>(operation);				\
  }

# define Functional_Methods(Type)					\
  template <class Operation>						\
  void for_each(Operation & operation) const				\
  {									\
    this->traverse([&operation] (const Type & item)			\
		   {							\
		     operation(item);					\
		     return true;					\
		   });							\
  }									\
									\
  template <class Operation>						\
  void for_each(Operation & operation)					\
  {									\
    this->traverse([&operation] (const Type & item)			\
		   {							\
		     operation(item);					\
		     return true;					\
		   });							\
  }									\
									\
  template <class Operation>						\
  void for_each(Operation && operation = Operation()) const		\
  {									\
    this->for_each<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  void for_each(Operation && operation = Operation())			\
  {									\
    this->for_each<Operation>(operation);				\
  }									\
  									\
  template <class Operation>						\
  void mutable_for_each(Operation & operation) const			\
  {									\
    this->traverse([&operation] (Type & item)				\
		   {							\
		     operation(item);					\
		     return true;					\
		   });							\
  }									\
									\
  template <class Operation>						\
  void mutable_for_each(Operation & operation)				\
  {									\
    this->traverse([&operation] (Type & item)				\
		   {							\
		     operation(item);					\
		     return true;					\
		   });							\
  }									\
									\
  template <class Operation>						\
  void mutable_for_each(Operation && operation = Operation()) const	\
  {									\
    this->mutable_for_each<Operation>(operation);			\
  }									\
									\
  template <class Operation>						\
  void mutable_for_each(Operation && operation = Operation())		\
  {									\
    this->mutable_for_each<Operation>(operation);			\
  }									\
									\
  template <class Operation>						\
  bool all(Operation & operation) const					\
  {									\
    return this->traverse<Operation>(operation);			\
  }									\
									\
  template <class Operation>						\
  bool all(Operation & operation)					\
  {									\
    return this->traverse<Operation>(operation);			\
  }									\
									\
  template <class Operation>						\
  bool all(Operation && operation = Operation()) const			\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool all(Operation && operation = Operation())			\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall(Operation & operation) const				\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall(Operation & operation)					\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall(Operation && operation = Operation()) const		\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall(Operation && operation = Operation())			\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool exists(Operation & operation) const				\
  {									\
    return not this->traverse([&operation] (const Type & item)		\
			      {						\
				return not operation(item);		\
			      });					\
  }									\
									\
  template <class Operation>						\
  bool exists(Operation & operation)					\
  {									\
    return not this->traverse([&operation] (const Type & item)		\
			      {						\
				return not operation(item);		\
			      });					\
  }									\
									\
  template <class Operation>						\
  bool exists(Operation && operation = Operation()) const		\
  {									\
    return exists<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  bool exists(Operation && operation = Operation())			\
  {									\
    return exists<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  Type * find_ptr(Operation & operation)				\
  {									\
    Type * ptr = NULL;							\
    this->traverse([&ptr,&operation] (Type & item)			\
		   {							\
		     if (operation(item))				\
		       {						\
			 ptr = &item;					\
			 return false;					\
		       }						\
		     return true;					\
		   });							\
    return ptr;								\
  }									\
									\
  template <class Operation>						\
  Type * find_ptr(Operation & operation) const				\
  {									\
    Type * ptr = NULL;							\
    this->traverse([&ptr,&operation] (Type & item)			\
		   {							\
		     if (operation(item))				\
		       {						\
			 ptr = &item;					\
			 return false;					\
		       }						\
		     return true;					\
		   });							\
    return ptr;								\
  }									\
									\
  template <class Operation>						\
  Type * find_ptr(Operation && operation = Operation()) const		\
  {									\
    return find_ptr<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  Type * find_ptr(Operation && operation = Operation())			\
  {									\
    return find_ptr<Operation>(operation);				\
  }									\
									\
  template <typename __T                        = Type,			\
	    template <typename> class Container = Aleph::DynList,	\
	    class Operation = Dft_Map_Op<Type, __T>>			\
  Container<__T> map(Operation & operation) const			\
  {									\
    Container<__T> ret_val;						\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     ret_val.append(operation(item));			\
		   });							\
    return ret_val;							\
  }									\
									\
  template < typename __T                        = Type,		\
	     template <typename> class Container = Aleph::DynList,	\
	     class Operation = Dft_Map_Op<__T, __T>>			\
  Container<__T> map(Operation && operation = Operation()) const	\
  {									\
    return map<__T, Container, Operation>(operation);			\
  }									\
									\
									\
  template <typename __T = Type>					\
  __T foldl(const __T & init,						\
	    std::function<__T(const __T&, const Type &)> operation) const \
  {									\
    __T ret_val = init;							\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     ret_val = operation(ret_val, item);		\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  DynList<Type> filter(Operation & operation) const			\
  {									\
    DynList<Type> ret_val;						\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     if (operation(item))				\
		       ret_val.append(item);				\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  DynList<Type> filter(Operation & operation)				\
  {									\
    DynList<Type> ret_val;						\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     if (operation(item))				\
		       ret_val.append(item);				\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  DynList<Type> filter(Operation && operation = Operation()) const	\
  {									\
    return filter<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  DynList<Type> filter(Operation && operation = Operation())		\
  {									\
    return filter<Operation>(operation);				\
  }									\
  									\
  template <class Operation>						\
  std::pair<DynList<Type>, DynList<Type>> partition(Operation & op) const \
  {									\
    std::pair<DynList<Type>, DynList<Type>> ret_val;			\
    this->for_each([&ret_val, &op] (const Type & item)			\
		   {							\
		     if (op(item))					\
		       ret_val.first.append(item);			\
		     else						\
		       ret_val.second.append(item);			\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  std::pair<DynList<Type>, DynList<Type>> partition(Operation & op)	\
  {									\
    std::pair<DynList<Type>, DynList<Type>> ret_val;			\
    this->for_each([&ret_val, &op] (const Type & item)			\
		   {							\
		     if (op(item))					\
		       ret_val.first.append(item);			\
		     else						\
		       ret_val.second.append(item);			\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  std::pair<DynList<Type>, DynList<Type>>				\
    partition(Operation && op = Operation()) const			\
  {									\
    return partition<Operation>(op);					\
  }									\
    									\
  template <class Operation>						\
  std::pair<DynList<Type>, DynList<Type>>				\
    partition(Operation && op = Operation())				\
  {									\
    return partition<Operation>(op);					\
  }									\
  									\
  size_t length() const							\
  {									\
    size_t count = 0;							\
    this->for_each([&count] (const Type &) { ++count; });		\
    return count;							\
  }									\
									\
  Type & nth(const size_t n) const					\
  {									\
    Type * ptr = NULL;							\
    size_t i = 0;							\
    this->traverse([&ptr, &i, &n] (Type & item)				\
		   {							\
		     if (i++ < n)					\
		       return true;					\
		     ptr = &item;					\
		     return false;					\
		   });							\
									\
    if (i != n + 1)							\
      throw std::out_of_range("nth");					\
    									\
    return *ptr;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Type> rev()	const						\
  {									\
    Container<Type> ret_val;						\
    for_each([&ret_val] (const Type & item)				\
	     {								\
	       ret_val.insert(item);					\
	     });							\
    return ret_val;							\
  }


# define Generic_Keys(Type)				   \
  template <template <typename> class Container = DynList> \
  Container<Type> keys() const				   \
  {							   \
    return this->template map<Type, Container>([] (const Type & key)	\
					       { return key; });	\
  }

# define Generic_Items(Type)					\
  template <template <typename> class Container = DynList>	\
  Container<Type> items() const					\
  {								\
    return this->template map<Type, Container> ([] (const Type & key)	\
						{ return key; });	\
  }


# define Equal_To_Method(class_name)			\
  bool equal_to(const class_name & r) const		\
  {							\
    if (this == &r)					\
      return true;					\
							\
    if (this->size() != r.size())			\
      return false;					\
							\
    return this->all(/* Lambda */ [&r] (const Key & k)	\
		     { return r.search(k) != NULL; });	\
  }							\
							\
  bool operator == (const class_name & r) const		\
  {							\
    return equal_to(r);					\
  }							\
							\
  bool operator != (const class_name & r) const		\
  {							\
    return not equal_to(r);				\
  }
  

# define Map_Sequences_Methods() \
  template <template <typename> class Container = ::DynList>		\
  Container<Key> keys()	const						\
  {									\
    Container<Key> ret_val;						\
    this->for_each([&ret_val] (const std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(p.first);					\
	     });							\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<Data> values() const					\
  {									\
    Container<Data> ret_val;						\
    this->for_each([&ret_val] (const std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(p.second);				\
	     });							\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<Data*> values_ptr() const					\
  {									\
    Container<Data*> ret_val;						\
    this->for_each([&ret_val] (std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(&p.second);				\
	     });							\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<std::pair<Key, Data>> items() const				\
  {									\
    return this->Base::keys();						\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<std::pair<Key, Data*>> items_ptr() const			\
  {									\
    Container<Data> ret_val;						\
    this->for_each([&ret_val] (std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(std::pair<Key,Data*>(p.first, p.second));	\
	     });							\
    return ret_val;							\
  }									\
									\
  Data & operator () (const Key & key)					\
  {									\
    return this->find(key);						\
  }

# define Generate_Proxy_Operator(Class_Name)				\
  class Proxy								\
  {									\
    Class_Name & container;						\
    const Key  & key;							\
    Data       * data_ptr;						\
									\
  public:								\
									\
    Proxy(Class_Name & c, const Key &_key) : container(c), key(_key)	\
    {									\
      data_ptr = container.search(key);					\
    }									\
									\
    Proxy & operator = (const Data & data)				\
      {									\
	if (data_ptr == NULL)						\
	  container.insert(key, data);					\
	else								\
	  *data_ptr = data;						\
									\
	return *this;							\
      }									\
									\
    Proxy & operator = (const Proxy & proxy)				\
      {									\
	if (this == &proxy)						\
	  return *this;							\
									\
	if (proxy.data_ptr == NULL)					\
	  throw std::domain_error("key not found");			\
									\
	if (&container == &proxy.tree and key == proxy.key)		\
	  return *this;							\
									\
	if (data_ptr == NULL)						\
	  container.insert(key, *proxy.data_ptr);			\
	else								\
	  *data_ptr = proxy.data_ptr;					\
									\
	return *this;							\
      }									\
									\
    operator Data & () const						\
    {									\
      if (data_ptr == NULL)						\
	throw std::domain_error("key not found");			\
  									\
      return *data_ptr;							\
    }									\
  };									\
									\
  Proxy operator [] (const Key & key) const				\
    Exception_Prototypes(std::domain_error)				\
  {									\
    return Proxy(*this, key);						\
  }									\
									\
  Proxy operator [] (const Key & key)					\
    Exception_Prototypes(std::domain_error)				\
  {									\
    return Proxy(*this, key);						\
  }
  
  template <typename Type> inline 
std::string to_str(const Type & d)  
{
  std::ostringstream os;
  os << d;
  return os.str();
}

/**
  @brief This class wrappes the compare class 
  passed to DynMapLinHash which has form cpm<Key>
*/
  template <typename Key, typename Data, class Cmp = std::equal_to<Key>>
	struct Dft_Pair_Cmp 
	{
	  bool operator () (const std::pair<Key, Data> & p1, 
			    const std::pair<Key, Data> & p2) const
	  {
	    return Cmp () (p1.first, p2.first);
	  }
	};

  template <typename Key, typename Data>
  std::pair<Key, Data> * key_to_pair(Key * ptr)
  {
    return (std::pair<Key, Data>*) ptr; 
  }

  template <typename Key, typename Data>
  std::pair<Key, Data> * data_to_pair(Data * ptr)
  {
    std::pair<Key, Data> * zero = 0;
    return (std::pair<Key, Data>*) ((long) ptr - (long) &zero->second); 
  }


} /* End namespace Aleph */

# endif // AHDRY_H
  