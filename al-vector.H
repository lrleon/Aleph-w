

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
 }
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
