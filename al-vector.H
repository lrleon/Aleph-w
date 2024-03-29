
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
# include <ah-zip.H>
# include <tpl_hash.H>
# include <al-domain.H>

namespace Aleph
{

  template <typename Trow, typename Tcol, typename NumType>
  class Matrix;

  /**

     @author Leandro Rabindranath Leon
  */
  template <typename T = int, typename NumType = double>
  class Vector
  {
  public:

    using Domain = AlDomain<T>;
    using Map = Aleph::HashMap<T, NumType, MapODhash>;

  private:

    static const NumType default_epsilon;

    const Domain * domain_ptr = nullptr;
    NumType        epsilon    = default_epsilon;
    Map            entries;

    static NumType abs(const NumType & val) noexcept
    {
      return val < 0 ? -val : val;
    }

    bool is_zero(const NumType & val) const noexcept
    {
      assert(epsilon >= 0);
      return abs(val) <= epsilon;
    }

    static bool test_epsilon(const NumType & e) noexcept { return e >= 0; }

  public:

    const NumType & get_epsilon() const noexcept { return epsilon; }

    void set_epsilon(const NumType & e) noexcept
    {
      epsilon = e;
    }

    using Pair = std::pair<T, NumType>;

    const Domain & get_domain() const noexcept { return *domain_ptr; }

    Vector(const Domain & d, const NumType & zero = default_epsilon)
      : domain_ptr(&const_cast<Domain&>(d)), epsilon(zero)
    {
      //Empty
    }

    Vector(const Vector & v)
      : domain_ptr(v.domain_ptr), epsilon(v.epsilon), entries(v.entries)
    {
      // empty
    }

    Vector(Vector && v)
      : domain_ptr(v.domain_ptr), epsilon(v.epsilon),
        entries(std::forward<Map>(v.entries))
    {
      // empty
    }

    Vector(const Domain & d, const DynList<NumType> & l,
           const NumType & zero = default_epsilon)
      : domain_ptr(&d), epsilon(zero)
    {
      if (l.size() != domain_ptr->size())
        throw std::length_error("Vector(DynList): list sizes does not match");

      for (auto it = zip_it(domain_ptr->keys(), l); it.has_curr(); it.next_ne())
        {
          auto t = it.get_curr();
          set_entry(get<0>(t), get<1>(t));
        }
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

      auto * ptr = const_cast<Pair*>(entries.search(i));
      if (is_zero(value))
        {
          if (ptr != nullptr)
            entries.remove_ptr(ptr);
          return;
        }

      if (ptr == nullptr)
        entries.insert(i, value);
      else
        ptr->second = value;
    }

    void set_entries(std::initializer_list<T>     ld,
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
      c.for_each([this, &itr] (const T & key)
                 {
                   if (not domain_ptr->has(key))
                     throw std::domain_error("Key does not belong to domain");

                   set_entry(key, *itr++);
                 });
    }

    NumType get_entry(const T & i)
    {
      assert(domain_ptr->has(i));

      auto * ptr = entries.search(i);
      if (ptr == nullptr)
        return 0;

      if (is_zero(ptr->second))
        {
          entries.remove_ptr(ptr);
          return 0;
        }

      return ptr->second;
    }

    NumType get_entry(const T & i) const noexcept
    {
      assert(domain_ptr->has(i));

      auto * ptr = entries.search(i);
      if (ptr == nullptr)
        return 0;

      return ptr->second;
    }

    NumType * search_entry(const T & i) const noexcept
    {
      assert(domain_ptr->has(i));

      auto * ptr = entries.search(i);
      if (ptr == nullptr)
        return nullptr;

      return &ptr->second;
    }

    bool are_equal(const NumType & n1, const NumType & n2) const noexcept
    {
      return is_zero(n1 - n2);
    }

    bool equal_to(const Vector & other) const noexcept
    {
      assert(domain_ptr == other.domain_ptr);

      return entries.all([&other, this] (const Pair & p)
                         {
                           return are_equal(other.get_entry(p.first), p.second);
                         }) and
        other.entries.all([this] (const Pair & p)
                          {
                            return are_equal(get_entry(p.first), p.second);
                          });
    }

    bool operator == (const Vector & v) const noexcept { return equal_to(v); }

    bool operator != (const Vector & v) const noexcept
    {
      return not equal_to(v);
    }

    Vector & operator += (const Vector & v)
    {
      assert(domain_ptr == v.domain_ptr);

      v.entries.for_each([this] (const Pair & p)
                         {
                           set_entry(p.first, get_entry(p.first) + p.second);
                         });
      return *this;
    }

    Vector & operator -= (const Vector & v)
    {
      assert(domain_ptr == v.domain_ptr);

      v.entries.for_each([this] (const Pair & p)
                         {
                           set_entry(p.first, get_entry(p.first) - p.second);
                         });
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

    Vector & product_by_scalar(const NumType & scalar) noexcept
    {
      if (is_zero(scalar))
        {
          entries.empty();
          return *this;
        }

      if (scalar == 1)
        return *this;

      entries.for_each([&scalar] (const Pair & p)
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

      entries.for_each([&scalar] (const Pair & p)
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

    // negation
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
        return entries.
          template foldl<NumType>(0, [&v] (const NumType & acc, const Pair & p)
                                  {
                                    return acc + p.second*v.get_entry(p.first);
                                  });

      return v.entries.
        template foldl<NumType>(0, [this] (const NumType & acc, const Pair & p)
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
      return sort(domain_ptr->template maps<NumType>([this](const T & i)
                                                     {
                                                       return get_entry(i);
                                                     }));
    }

    void print() const
    {
      domain_ptr->for_each([&] (const T & k)
                           {
                             if (entries.has(k) != 0)
                               std::cout << "(" << k << "," << entries(k) << ") ";
                           });
      std::cout << std::endl;
    }

    std::string to_str() const
    {
      // Elementos del dominio en string
      DynList<std::string> keys_str =
        domain_ptr->keys().template maps<std::string>([] (const T & k)
                                                      {
                                                        ostringstream s;
                                                        s << k;
                                                        return s.str();
                                                      });

      // elementos del rango en string
      DynList<std::string> entries_str =
        domain_ptr->keys().template maps<std::string>([this] (const T & d)
                                                      {
                                                        return std::to_string(get_entry(d));
                                                      });

      typedef std::pair<std::string, std::string> Pair;
      DynList<Pair> pairs = zip(keys_str, entries_str);

      DynList<Pair> format = pairs.
        template maps<Pair>([] (const Pair & p)
                            {
                              size_t fsz = p.first.size(),
                              ssz = p.second.size();
                              if (fsz > ssz)
                                return Pair(" " + p.first, string(fsz - ssz + 1, ' ') + p.second);
                              else
                                return Pair(string(ssz - fsz + 1, ' ') + p.first, " " + p.second);
                            });

      std::pair<DynList<std::string>, DynList<std::string>> ret = unzip(format);

      auto concatenate = [] (const std::string & s1, const std::string & s2)
        {
          return s1 + s2;
        };

      std::string dstr = ret.first.template foldl<std::string>("", concatenate);

      std::string estr = ret.second.template foldl<std::string>("", concatenate);

      return dstr + "\n" + string(dstr.size(), '-') + "\n" + estr;
    }

    class Proxy
    {
      Vector *    v_ptr   = nullptr;
      T * key_ptr         = nullptr;
      NumType * entry_ptr = nullptr;

    public:

      Proxy(Vector & v, const T & k) noexcept
        : v_ptr(&v), key_ptr(&const_cast<T&>(k))
      {
        auto * ptr = v_ptr->entries.search(k);
        entry_ptr = ptr ? &ptr->second : nullptr;
      }

      Proxy & operator = (const Proxy & proxy)
      {
        if (proxy.entry_ptr == nullptr)
          return *this; // zero assigment

        if (entry_ptr == nullptr)
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
            catch (domain_error) { /* nothing */ }
            return * this;
          }

        if (entry_ptr == nullptr)
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
            catch (domain_error) { /* nothing */ }
            return * this;
          }

        if (entry_ptr == nullptr)
          v_ptr->entries.insert(*key_ptr, std::forward<NumType>(item));
        else
          std::swap(*entry_ptr, item);

        return *this;
      }

      operator NumType () noexcept
      {
        if (entry_ptr == nullptr)
          return 0;

        return *entry_ptr;
      }
    };

    Proxy operator [] (const T & k) const noexcept
    {
      return Proxy(*this, k);
    }

    Proxy operator [] (const T & k) noexcept
    {
      return Proxy(*this, k);
    }

    Proxy operator () (const T & k) const noexcept
    {
      return Proxy(*this, k);
    }

    Proxy operator () (const T & k) noexcept
    {
      return Proxy(*this, k);
    }

    struct Iterator : public Map::Iterator
    {
      Iterator(const Vector & vec) : Map::Iterator(vec.entries) {}
    };

    Iterator get_itor() const noexcept { return Iterator(*this); }
    Iterator get_it() const noexcept { return Iterator(*this); }

    Generic_Traverse(Pair);

    Functional_Methods(Pair);
    Locate_Functions(Pair);
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

} // end namespace Aleph

# endif // AL_VECTOR
