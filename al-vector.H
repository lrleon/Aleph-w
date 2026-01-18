/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file al-vector.H
 *  @brief Sparse vector with named elements.
 *
 *  Provides Vector class for sparse vectors where elements are
 *  indexed by arbitrary types. Efficient for vectors with many
 *  zero/default elements using hash-based storage.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef AL_VECTOR
# define AL_VECTOR

# include <memory>
# include <sstream>
# include <iostream>
# include <string>
# include <ah-errors.H>
# include <ahFunctional.H>
# include <ahDry.H>
# include <ah-dry-mixin.H>
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
     @brief Sparse vector implementation with domain-based indexing

     This class implements a mathematical vector with sparse storage,
     where only non-zero entries are stored. The vector is defined over
     a domain of type T, and entries have values of type NumType.

     The vector uses epsilon-based comparison for numerical tolerance,
     automatically removing entries that become close to zero.

     @tparam T Type of domain elements (default: int)
     @tparam NumType Numeric type for vector entries (default: double)

     @author Leandro Rabindranath Leon
  */
  template <typename T = int, typename NumType = double>
  class Vector; // Forward declaration for CRTP

  template <typename T, typename NumType>
  class Vector : public TraverseMixin<Vector<T, NumType>, std::pair<T, NumType>>,
                 public LocateMixin<Vector<T, NumType>, std::pair<T, NumType>>,
                 public FunctionalMixin<Vector<T, NumType>, std::pair<T, NumType>>
  {
  public:
    using Domain = AlDomain<T>;
    using DomainPtr = std::shared_ptr<Domain>;
    using Map = Aleph::HashMap<T, NumType, MapODhash>;
    using Item_Type = std::pair<T, NumType>; // Required by mixins

  private:
    static const NumType default_epsilon;

    DomainPtr domain_ptr = nullptr;
    NumType epsilon = default_epsilon;
    Map entries;

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
    /**
       @brief Get the epsilon value used for zero comparisons
       @return Current epsilon value
    */
    const NumType &get_epsilon() const noexcept { return epsilon; }

    /**
       @brief Set the epsilon value for zero comparisons
       @param e New epsilon value (must be >= 0)
       @note Values with absolute value <= epsilon are considered zero
    */
    void set_epsilon(const NumType & e) noexcept
    {
      epsilon = e;
    }

    using Pair = std::pair<T, NumType>;

    /**
       @brief Get the domain over which this vector is defined
       @return Reference to the domain
    */
    const Domain &get_domain() const noexcept { return *domain_ptr; }

    /// Return the shared pointer to the domain.
    const DomainPtr &get_domain_ptr() const noexcept { return domain_ptr; }

    /**
       @brief Construct a vector over a given domain (shared_ptr version)
       @param d Shared pointer to domain for the vector indices
       @param zero Epsilon value for zero comparisons (default: 1e-7)
    */
    Vector(DomainPtr d, const NumType & zero = default_epsilon)
      : domain_ptr(std::move(d)), epsilon(zero)
    {
      //Empty
    }

    /**
       @brief Construct a vector over a given domain (reference version)
       @param d Domain for the vector indices (will be wrapped in shared_ptr)
       @param zero Epsilon value for zero comparisons (default: 1e-7)
       @note Creates a non-owning shared_ptr; caller must ensure domain outlives vector
    */
    Vector(const Domain & d, const NumType & zero = default_epsilon)
      : domain_ptr(DomainPtr(&const_cast<Domain &>(d), [](Domain *) {})), epsilon(zero)
    {
      //Empty
    }

    /**
       @brief Copy constructor
       @param v Vector to copy from
    */
    Vector(const Vector & v)
      : domain_ptr(v.domain_ptr), epsilon(v.epsilon), entries(v.entries)
    {
      // empty
    }

    /**
       @brief Move constructor
       @param v Vector to move from
    */
    Vector(Vector && v) noexcept
      : domain_ptr(v.domain_ptr), epsilon(v.epsilon),
        entries(std::forward<Map>(v.entries))
    {
      // empty
    }

    /**
       @brief Construct vector from domain and list of values (shared_ptr version)
       @param d Shared pointer to domain for the vector indices
       @param l List of values (must match domain size)
       @param zero Epsilon value for zero comparisons (default: 1e-7)
       @throw ah_length_error if list size doesn't match domain size
    */
    Vector(DomainPtr d, const DynList<NumType> & l,
           const NumType & zero = default_epsilon)
      : domain_ptr(std::move(d)), epsilon(zero)
    {
      ah_length_error_if(l.size() != domain_ptr->size())
        << "Vector(DynList): list sizes does not match";

      for (auto it = zip_it(domain_ptr->keys(), l); it.has_curr(); it.next_ne())
        {
          auto t = it.get_curr();
          set_entry(get<0>(t), get<1>(t));
        }
    }

    /**
       @brief Construct vector from domain and list of values (reference version)
       @param d Domain for the vector indices (will be wrapped in shared_ptr)
       @param l List of values (must match domain size)
       @param zero Epsilon value for zero comparisons (default: 1e-7)
       @throw ah_length_error if list size doesn't match domain size
       @note Creates a non-owning shared_ptr; caller must ensure domain outlives vector
    */
    Vector(const Domain & d, const DynList<NumType> & l,
           const NumType & zero = default_epsilon)
      : Vector(DomainPtr(&const_cast<Domain &>(d), [](Domain *) {}), l, zero)
    {}

    /**
       @brief Copy assignment operator
       @param v Vector to copy from
       @return Reference to this vector
       @throw std::domain_error if domains are different (not the same object)
       @note Vectors must share the same domain (pointer equality) for assignment
    */
    Vector &operator =(const Vector & v)
    {
      if (this == &v)
        return *this;

      ah_domain_error_if(domain_ptr.get() != v.domain_ptr.get())
        << "Vector assignment: domains must be the same";

      epsilon = v.epsilon;
      entries = v.entries;

      return *this;
    }

    /**
       @brief Move assignment operator
       @param v Vector to move from
       @return Reference to this vector
       @throw std::domain_error if domains are different (not the same object)
       @note Vectors must share the same domain (pointer equality) for assignment
    */
    Vector &operator =(Vector && v)
    {
      if (this == &v)
        return *this;

      ah_domain_error_if(domain_ptr.get() != v.domain_ptr.get())
        << "Vector assignment: domains must be the same";

      epsilon = v.epsilon;
      entries.swap(v.entries);

      return *this;
    }

    /**
       @brief Set a vector entry at given index
       @param i Index (must be in domain)
       @param value Value to set (automatically removed if within epsilon of zero)
       @note If value is zero (within epsilon), the entry is removed from storage
    */
    void set_entry(const T & i, const NumType & value)
    {
      assert(domain_ptr->has(i));

      auto *ptr = const_cast<Pair *>(entries.search(i));
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

    /**
       @brief Set multiple entries using initializer lists
       @param ld List of domain indices
       @param lr List of corresponding values
       @throw ah_range_error if lists have different sizes
       @throw ah_domain_error if any index not in domain
    */
    void set_entries(std::initializer_list<T> ld,
                     std::initializer_list<NumType> lr)
    {
      ah_range_error_if(ld.size() != lr.size())
        << "size mismatch between domain and range";

      auto itd = ld.begin();
      auto itr = lr.begin();
      for (; itd != ld.end(); ++itd, ++itr)
        {
          ah_domain_error_if(not domain_ptr->has(*itd))
            << "An item of first list doesn't belong to domain";
          set_entry(*itd, *itr);
        }
    }

    /**
       @brief Set multiple entries using container and initializer list
       @tparam Container Container type (default: DynList)
       @param c Container of domain indices
       @param lr List of corresponding values
       @throw ah_range_error if container and list have different sizes
       @throw ah_domain_error if any index not in domain
    */
    template <template <typename> class Container = DynList>
    void set_entries(const Container<T> & c, std::initializer_list<NumType> lr)
    {
      ah_range_error_if(c.size() != lr.size())
        << "size mismatch between domain and range";

      auto itr = lr.begin();
      c.for_each([this, &itr](const T & key)
                   {
                     ah_domain_error_if(not domain_ptr->has(key))
                     << "Key does not belong to domain";

                     set_entry(key, *itr++);
                   });
    }

    /**
       @brief Get vector entry at given index (non-const version)
       @param i Index (must be in domain)
       @return Value at index (0 if not set or within epsilon of zero)
       @warning This non-const version may modify internal state by removing
                near-zero entries
    */
    NumType get_entry(const T & i)
    {
      assert(domain_ptr->has(i));

      auto *ptr = entries.search(i);
      if (ptr == nullptr)
        return 0;

      if (is_zero(ptr->second))
        {
          entries.remove_ptr(ptr);
          return 0;
        }

      return ptr->second;
    }

    /**
       @brief Get vector entry at given index (const version)
       @param i Index (must be in domain)
       @return Value at index (0 if not set)
    */
    NumType get_entry(const T & i) const noexcept
    {
      assert(domain_ptr->has(i));

      auto *ptr = entries.search(i);
      if (ptr == nullptr)
        return 0;

      return ptr->second;
    }

    /**
       @brief Search for an entry and return pointer to its value
       @param i Index (must be in domain)
       @return Pointer to value if exists, nullptr otherwise
    */
    NumType * search_entry(const T & i) const noexcept
    {
      assert(domain_ptr->has(i));

      auto *ptr = entries.search(i);
      if (ptr == nullptr)
        return nullptr;

      return &ptr->second;
    }

    /**
       @brief Check if two numeric values are equal within epsilon
       @param n1 First value
       @param n2 Second value
       @return true if |n1 - n2| <= epsilon
    */
    bool are_equal(const NumType & n1, const NumType & n2) const noexcept
    {
      return is_zero(n1 - n2);
    }

    /**
       @brief Check if this vector equals another within epsilon tolerance
       @param other Vector to compare with (must have same domain)
       @return true if all corresponding entries are within epsilon
    */
    bool equal_to(const Vector & other) const noexcept
    {
      assert(domain_ptr == other.domain_ptr);

      return entries.all([&other, this](const Pair & p)
                           {
                             return are_equal(other.get_entry(p.first), p.second);
                           }) and
             other.entries.all([this](const Pair & p)
                                 {
                                   return are_equal(get_entry(p.first), p.second);
                                 });
    }

    bool operator ==(const Vector & v) const noexcept { return equal_to(v); }

    bool operator !=(const Vector & v) const noexcept
    {
      return not equal_to(v);
    }

    Vector &operator +=(const Vector & v)
    {
      assert(domain_ptr == v.domain_ptr);

      v.entries.for_each([this](const Pair & p)
                           {
                             set_entry(p.first, get_entry(p.first) + p.second);
                           });
      return *this;
    }

    Vector &operator -=(const Vector & v)
    {
      assert(domain_ptr == v.domain_ptr);

      v.entries.for_each([this](const Pair & p)
                           {
                             set_entry(p.first, get_entry(p.first) - p.second);
                           });
      return *this;
    }

    Vector operator +(const Vector & r) const
    {
      Vector ret_val = *this;
      ret_val += r;
      return ret_val;
    }

    Vector operator -(const Vector & r) const
    {
      Vector ret_val = *this;
      ret_val -= r;
      return ret_val;
    }

    Vector &product_by_scalar(const NumType & scalar) noexcept
    {
      if (is_zero(scalar))
        {
          entries = Map(); // Clear by assigning empty map
          return *this;
        }

      if (scalar == 1)
        return *this;

      entries.for_each([&scalar](const Pair & p)
                         {
                           const_cast<Pair &>(p).second *= scalar;
                         });
      return *this;
    }

    Vector operator *(const NumType & scalar) const
    {
      Vector ret_val = *this;
      return ret_val.product_by_scalar(scalar);
    }

    Vector &divide_by_scalar(const NumType & scalar)
    {
      ah_domain_error_if(is_zero(scalar))
        << "Zero division";

      if (scalar == 1)
        return *this;

      entries.for_each([&scalar](const Pair & p)
                         {
                           const_cast<Pair &>(p).second /= scalar;
                         });
      return *this;
    }

    Vector operator /(const NumType & scalar) const
    {
      Vector ret_val = *this;
      return ret_val.divide_by_scalar(scalar);
    }

    // negation
    Vector operator -() const
    {
      Vector ret_val = *this;
      return ret_val.product_by_scalar(-1.0);
    }

    NumType scalar_product(const Vector & v) const
    {
      ah_domain_error_if(domain_ptr != v.domain_ptr)
        << "scalar_product: different domains";

      if (entries.size() < v.entries.size())
        return entries.
            template foldl<NumType>(0, [&v](const NumType & acc, const Pair & p)
                                      {
                                        return acc + p.second * v.get_entry(p.first);
                                      });

      return v.entries.
               template foldl<NumType>(0, [this](const NumType & acc, const Pair & p)
                                         {
                                           return acc + get_entry(p.first) * p.second;
                                         });
    }

    NumType operator *(const Vector & v) const
    {
      return scalar_product(v);
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
      domain_ptr->for_each([&](const T & k)
                             {
                               if (entries.has(k) != 0)
                                 std::cout << "(" << k << "," << entries(k) << ") ";
                             });
      std::cout << '\n';
    }

    [[nodiscard]] std::string to_str() const
    {
      // Convert domain elements to strings
      const DynList<std::string> keys_str =
          domain_ptr->keys().template maps<std::string>([](const T & k)
                                                          {
                                                            std::ostringstream s;
                                                            s << k;
                                                            return s.str();
                                                          });

      // Convert range elements to strings
      const DynList<std::string> entries_str =
          domain_ptr->keys().template maps<std::string>([this](const T & d)
                                                          {
                                                            return std::to_string(get_entry(d));
                                                          });

      typedef std::pair<std::string, std::string> Pair;
      DynList<Pair> pairs = zip(keys_str, entries_str);

      const DynList<Pair> format = pairs.
          template maps<Pair>([](const Pair & p)
                                {
                                  const size_t fsz = p.first.size();
                                  const size_t ssz = p.second.size();
                                  if (fsz > ssz)
                                    return Pair(" " + p.first, std::string(fsz - ssz + 1, ' ') + p.second);
                                  return Pair(std::string(ssz - fsz + 1, ' ') + p.first, " " + p.second);
                                });

      std::pair<DynList<std::string>, DynList<std::string>> ret = unzip(format);

      auto concatenate = [](const std::string & s1, const std::string & s2)
        {
          return s1 + s2;
        };

      const std::string dstr = ret.first.template foldl<std::string>("", concatenate);

      const std::string estr = ret.second.template foldl<std::string>("", concatenate);

      return dstr + "\n" + std::string(dstr.size(), '-') + "\n" + estr;
    }

    class Proxy
    {
      Vector *v_ptr = nullptr;
      T *key_ptr = nullptr;
      NumType *entry_ptr = nullptr;

    public:
      Proxy(Vector & v, const T & k) noexcept
        : v_ptr(&v), key_ptr(&const_cast<T &>(k))
      {
        auto *ptr = v_ptr->entries.search(k);
        entry_ptr = ptr ? &ptr->second : nullptr;
      }

      Proxy &operator =(const Proxy & proxy)
      {
        if (this == &proxy)
          return *this; // self-assignment

        if (proxy.entry_ptr == nullptr)
          return *this; // zero assigment

        if (entry_ptr == nullptr)
          v_ptr->entries.insert(*key_ptr, *proxy.entry_ptr);
        else
          *entry_ptr = *proxy.entry_ptr;

        return *this;
      }

      Proxy &operator =(const NumType & item)
      {
        if (v_ptr->is_zero(item))
          {
            try { v_ptr->entries.remove(key_ptr); }
            catch (...) { /* Key not found, already zero */ }
            return *this;
          }

        if (entry_ptr == nullptr)
          v_ptr->entries.insert(*key_ptr, item);
        else
          *entry_ptr = item;

        return *this;
      }

      Proxy &operator =(NumType && item)
      {
        if (v_ptr->is_zero(item))
          {
            try { v_ptr->entries.remove(*key_ptr); }
            catch (...) { /* Key not found, already zero */ }
            return *this;
          }

        if (entry_ptr == nullptr)
          v_ptr->entries.insert(*key_ptr, std::forward<NumType>(item));
        else
          std::swap(*entry_ptr, item);

        return *this;
      }

      operator NumType() noexcept
      {
        if (entry_ptr == nullptr)
          return 0;

        return *entry_ptr;
      }
    };

    Proxy operator [](const T & k) const noexcept
    {
      return Proxy(*this, k);
    }

    Proxy operator [](const T & k) noexcept
    {
      return Proxy(*this, k);
    }

    Proxy operator ()(const T & k) const noexcept
    {
      return Proxy(*this, k);
    }

    Proxy operator ()(const T & k) noexcept
    {
      return Proxy(*this, k);
    }

    struct Iterator : public Map::Iterator
    {
      Iterator(const Vector & vec) : Map::Iterator(vec.entries) {}
    };

    Iterator get_itor() const noexcept { return Iterator(*this); }
    Iterator get_it() const noexcept { return Iterator(*this); }

    // Note: traverse(), for_each(), all(), exists(), maps(), filter(),
    // foldl(), fold(), partition(), take(), drop(), rev(), length(),
    // nth(), nth_ne(), find_ptr(), find_item() are now provided
    // by the CRTP mixins: TraverseMixin, FunctionalMixin, LocateMixin
  };

  template <typename T, typename NumType>
  const NumType Vector<T, NumType>::default_epsilon = 1e-7;


  template <typename T, typename NumType>
  inline
  Vector<T, NumType> operator *(const NumType & scalar,
                                const Vector<T, NumType> & v)
  {
    Vector<T, NumType> ret_val = v;
    return ret_val.product_by_scalar(scalar);
  }

  template <typename T, typename NumType>
  inline
  std::ostream &operator <<(std::ostream & s, const Vector<T, NumType> & vec)
  {
    return s << vec.to_str();
  }
} // end namespace Aleph

# endif // AL_VECTOR
