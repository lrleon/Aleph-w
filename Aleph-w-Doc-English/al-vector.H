
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
  @file al-vector.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AL_VECTOR
# define AL_VECTOR

# include <memory>
# include <sstream>
# include <iostream>
# include <string>
# include <ahFunctional.H>
# include <ahDry.H>
# include <ahSort.H>
# include <htlist.H>
# include <tpl_hash.H>
# include <al-domain.H>

namespace Aleph
{

template <typename Trow, typename Tcol, typename NumType>
class Matrix;

    template <typename T = int, typename NumType = double>
class Vector
{
public:

  typedef AlDomain<T> Domain;

  typedef Aleph::HashMap<T, NumType, DynMapODHash ,
			 Aleph::Dft_Pair_Cmp<T, NumType>> Map;

private:

  static const NumType default_epsilon;

  const Domain * domain_ptr = NULL;
  NumType        epsilon    = default_epsilon;
  Map            entries;

  static NumType abs(const NumType & val)
  {
    return val < 0 ? -val : val;
  }

  bool is_zero(const NumType & val) const
  {
    assert(epsilon >= 0);
    return abs(val) <= epsilon;
  }

  static bool test_epsilon(const NumType & e) { return e >= 0; }

public:

  const NumType & get_epsilon() const { return epsilon; }

  void set_epsilon(const NumType & e) 
  {
    test_epsilon(e);
    epsilon = e;
  }

  typedef std::pair<T, NumType> Pair;

  const Domain & get_domain() const { return *domain_ptr; }

  Vector(const Domain & d, const NumType & zero = default_epsilon)
    : domain_ptr(&const_cast<Domain&>(d)), epsilon(zero)
  {
    test_epsilon(epsilon);
  }

  Vector(const Vector & v)
    : domain_ptr(v.domain_ptr), epsilon(v.epsilon), entries(v.entries)
  {
    /* Empty */
  }

  Vector(Vector && v)
    : domain_ptr(v.domain_ptr), epsilon(v.epsilon), 
      entries(std::move(v.entries))
  {
    /* Empty */
  }

  Vector(const Domain & d, const DynList<NumType> & l,
	 const NumType & zero = default_epsilon)
    : domain_ptr(&d), epsilon(zero)
  {
    test_epsilon(epsilon);
    if (l.size() != domain_ptr->size())
      throw std::range_error("Vector(DynList): list sizes does'n match");

    Aleph::zip(sort(domain_ptr->keys()), l).
      for_each(/* Lambda */ [this] (const std::pair<T, NumType> & p)
	       {
		 set_entry(p.first, p.second);
	       });
  }

  Vector & operator = (const Vector & v)
  {
    if (this == &v)
      return *this;

    if (domain_ptr != v.domain_ptr)
      throw std::domain_error("Unmatch domain in Vector assignation");

    epsilon = v.epsilon;
    entries = v.entries;

    return *this;
  }

  Vector & operator = (Vector && v)
  {
    if (domain_ptr != v.domain_ptr)
      throw std::domain_error("Unmatch domain in Vector assignation");

    epsilon = v.epsilon;
    entries.swap(v.entries);

    return *this;
  }

  void set_entry(const T & i, const NumType & value)
  {
    assert(domain_ptr->has(i));

    NumType * entry_ptr = const_cast<NumType*>(entries.search(i));

    if (is_zero(value))
      {
	if (entry_ptr != NULL)
	  entries.remove_by_data(*entry_ptr);
	
	return;
      }

    if (entry_ptr == NULL)
      entries.insert(i, value);
    else
      *entry_ptr = value;
  }

  void set_entries(std::initializer_list<T>       ld, 
		   std::initializer_list<NumType> lr)
  {
    if (ld.size() != lr.size())
      throw std::range_error("size mismatch between domain and range");

    auto itd = ld.begin();
    auto itr = lr.begin();
    for (; itd != ld.end(); itd++, itr++)
      {
	if (not domain_ptr->has(*itd))
	  std::domain_error("An item of first list doesn't belong to domain");
	set_entry(*itd, *itr);
      }
  }

  template <template <typename> class Container = DynList>
  void set_entries(const Container<T> & c, std::initializer_list<NumType> lr)
  {
    if (c.size() != lr.size())
      throw std::range_error("size mismatch between domain and range");

    auto itr = lr.begin();
    c.for_each(/* Lambda */ [this, &itr] (const T & key)
	       {
		 if (not domain_ptr->has(key))
		   throw std::domain_error("Key does not belong to domain");

		 set_entry(key, *itr++);
	       });
  }

  NumType get_entry(const T & i)
  {
    assert(domain_ptr->has(i));

    NumType * entry_ptr = entries.search(i);
    if (entry_ptr == NULL)
      return 0;

    if (is_zero(*entry_ptr))
      {
	entries.remove_by_data(*entry_ptr);
	return 0;
      }

    return *entry_ptr;
  }

  NumType get_entry(const T & i) const
  {
    assert(domain_ptr->has(i));

    NumType * entry_ptr = entries.search(i);
    if (entry_ptr == NULL)
      return 0;

    return *entry_ptr;
  }

  NumType * search_entry(const T & i) const
  {
    assert(domain_ptr->has(i));

    NumType * entry_ptr = entries.search(i);
    if (entry_ptr == NULL)
      return NULL;

    return entry_ptr;
  }

  bool are_equal(const NumType & n1, const NumType & n2) const
  {
    return is_zero(n1 - n2);
  }

  bool equal_to(const Vector & other) const
  {
    assert(domain_ptr == other.domain_ptr);

    return entries.all(/* Lamdba */ [&other, this] (const Pair & p)
		       {
			 return are_equal(other.get_entry(p.first), p.second);
		       }
		       ) 
      and
      other.entries.all(/* Lamdba */ [this] (const Pair & p)
			{
			  return are_equal(get_entry(p.first), p.second);
			});
  }

  bool operator == (const Vector & v) const { return equal_to(v); }

  bool operator != (const Vector & v) const { return not equal_to(v); }

  Vector & operator += (const Vector & v)
  {
    assert(domain_ptr == v.domain_ptr);

    /* Traverse all non-zero entries of v and add them to this */
    v.entries.for_each(/* Lambda */ [this] (const Pair & p)
		       {
			 set_entry(p.first, get_entry(p.first) + p.second);
		       }
		       );
    return *this;
  }

  Vector & operator -= (const Vector & v)
  {
    assert(domain_ptr == v.domain_ptr);

    v.entries.for_each(/* Lambda */ [this] (const Pair & p)
		       {
			 set_entry(p.first, get_entry(p.first) - p.second);
		       }
		       );
    return *this;
  }

  Vector operator + (const Vector & r) const
  {
    Vector ret_val = *this;
    ret_val += r;
    return ret_val;
  }

  Vector operator - (const Vector & r) const
  {
    Vector ret_val = *this;
    ret_val -= r;
    return ret_val;
  }

  Vector & product_by_scalar(const NumType & scalar) 
  {
    if (is_zero(scalar))
      {
	entries.empty();
	return *this;
      }

    if (scalar == 1)
      return *this;

    entries.for_each(/* Lambda */ [this, &scalar] (const Pair & p)
		     {
		       const_cast<Pair&>(p).second *= scalar;
		     });
    return *this;
  }

  Vector operator * (const NumType & scalar) const
  {
    Vector ret_val = *this;
    return ret_val.product_by_scalar(scalar);
  }

  Vector & divide_by_scalar(const NumType & scalar) 
  {
    if (is_zero(scalar))
      throw std::domain_error("Zero division");

    if (scalar == 1)
      return *this;

    entries.for_each(/* Lambda */ [this, &scalar] (const Pair & p)
		     {
		       const_cast<Pair&>(p).second /= scalar;
		     });
    return *this;
  }

  Vector operator / (const NumType & scalar) const
  {
    Vector ret_val = *this;
    return ret_val.divide_by_scalar(scalar);
  }

    /* Negation */
  Vector operator - () const
  {
    Vector ret_val = *this;
    return ret_val.product_by_scalar(-1.0);
  }

  NumType scalar_product(const Vector & v)
  {
    if (domain_ptr != v.domain_ptr)
      throw std::domain_error("scalar_product: different domains");

    if (entries.size() < v.entries.size())
      return entries.template foldl<NumType>(0, 
					     /* Lambda */ [this, &v] 
			   (const NumType & acc, const Pair & p) -> NumType
			   {
			     return acc + p.second*v.get_entry(p.first);
			   });

    return v.entries.template foldl<NumType>(0, /* Lambda */ [this] 
					     (const NumType & acc, 
					      const Pair & p)
		   {
		     return acc + get_entry(p.first)*p.second;
		   });
  }

  NumType operator * (const Vector & v) const
  {
    Vector ret_val = *this;
    return ret_val.scalar_product(v);
  }

  DynList<NumType> to_list() const
  {
    return sort(domain_ptr->template map<NumType>([this](const T & i) 
      {
	return get_entry(i); 
      }));
  }

  void print() const
  {
    domain_ptr->for_each(/* Lambda */ [&] (const T & k)
			 {
			   if (entries.has(k) != 0)
			     std::cout << "(" << k << "," << entries(k) << ") ";
			 });
    std::cout << std::endl;
  }

  std::string to_str() const
  {
    DynList<T> keys = Aleph::sort(domain_ptr->keys());

    /* Elements dof domain in string */
    DynList<std::string> keys_str = 
      keys.template map <std::string> (/* Lambda */ [] (const T & k)
				       { 
					 return Aleph::to_str(k);
				       });

    /* Elements of the rank in string */
    DynList<std::string> entries_str =
      keys.template map <std::string> (/* Lambda */ [this] (const T & d)
      {
	return Aleph::to_str(get_entry(d));
      });

    typedef std::pair<std::string, std::string> Pair;
    DynList<Pair> pairs = zip(keys_str, entries_str);

    DynList<Pair> format = /* Formatted pairs */
      pairs.template map <Pair> (/* Lambda */ [] (const Pair & p)
      {
	size_t fsz = p.first.size(), ssz = p.second.size();
	if (fsz > ssz)
	  return Pair(" " + p.first, string(fsz - ssz + 1, ' ') + p.second);
	else
	  return Pair(string(ssz - fsz + 1, ' ') + p.first, " " + p.second);
      });

    std::pair<DynList<std::string>, DynList<std::string>> ret = unzip(format);

    auto concatenate = /* Lambda */ [] 
      (const std::string & s1, const std::string & s2)
      {
	return s1 + s2;
      };

    std::string dstr = ret.first.template foldl<std::string>("", concatenate);

    std::string estr = ret.second.template foldl<std::string>("", concatenate);

    return dstr + "\n" + string(dstr.size(), '-') + "\n" + estr;
  }

  class Proxy
  {
    Vector *    v_ptr   = NULL;
    T * key_ptr         = NULL;
    NumType * entry_ptr = NULL;

  public:

    Proxy(Vector & v, const T & k) : v_ptr(&v), key_ptr(&const_cast<T&>(k))
    {
      entry_ptr = v_ptr->entries.search(k);
    }

    Proxy & operator = (const Proxy & proxy)
    {
      if (proxy.entry_ptr == NULL)
	return *this; /* Zero assigment */

      if (entry_ptr == NULL)
	v_ptr->entries.insert(*key_ptr, *proxy.entry_ptr);
      else
	*entry_ptr = *proxy.entry_ptr;

      return *this;
    }

    Proxy & operator = (const NumType & item)
    {
      if (v_ptr->is_zero(item))
	{
	  try { v_ptr->entries.remove(key_ptr); }
	  catch (domain_error) { /* Nothing */ }
	  return * this;
	}
	 
      if (entry_ptr == NULL)
	v_ptr->entries.insert(*key_ptr, item);
      else
	*entry_ptr = item;

      return *this;
    }

    Proxy & operator = (NumType && item)
    {
      if (v_ptr->is_zero(item))
	{
	  try { v_ptr->entries.remove(*key_ptr); }
	  catch (domain_error) { /* Nothing */ }
	  return * this;
	}
	 
      if (entry_ptr == NULL)
	v_ptr->entries.insert(*key_ptr, std::move(item));
      else
	std::swap(*entry_ptr, item);

      return *this;
    }

    operator NumType () 
    {
      if (entry_ptr == NULL)
	return 0;
      
      return *entry_ptr;
    }
  };

  Proxy operator [] (const T & k) const
  {
    return Proxy(*this, k);
  }

  Proxy operator [] (const T & k)
  {
    return Proxy(*this, k);
  }

  Proxy operator () (const T & k) const
  {
    return Proxy(*this, k);
  }

  Proxy operator () (const T & k)
  {
    return Proxy(*this, k);
  }

  struct Iterator : public Map::Iterator 
  {
    Iterator(const Vector & vec) : Map::Iterator(vec.entries) { }
  };

  Generic_Traverse(Pair);

  Functional_Methods(Pair);
};

template <typename T, typename NumType>
const NumType Vector<T, NumType>::default_epsilon = 1e-7;


template <typename T, typename NumType> inline
Vector<T, NumType> operator * (const NumType & scalar, 
			       const Vector<T, NumType> & v)
{
  Vector<T, NumType> ret_val = v;
  return ret_val.product_by_scalar(scalar);
}

template <typename T, typename NumType> inline
ostream & operator << (ostream & s, const Vector<T, NumType> & vec)
{
  return s << vec.to_str();
}

} /* End namespace Aleph */

# endif // AL_VECTOR
