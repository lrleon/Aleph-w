
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# ifndef AH_ITERATOR_H
# define AH_ITERATOR_H

template <class SetType>
struct StlIterator : public SetType::Iterator
{
  using T = typename SetType::Item_Type;
  using Itor = typename SetType::Iterator;

  using value_type = T;

  using Itor::Itor;

  Itor operator ++ ()
  {
    this->next();
    return *this;
  }

  Itor operator ++ (int)
  {
    Itor ret_val = *this;
    this->next();
    return ret_val;
  }

  bool operator == (const Itor & it) const
  {
    if (this->has_curr() and it.has_curr())
      return this->get_pos() == it.get_pos();
      // return this->get_curr() == it.get_curr();

    if (not this->has_curr() and not it.has_curr())
      return true;

    return false;
  }

  bool operator != (const Itor & it) const
  {
    return not (*this == it);
  }

  T & operator * () { return const_cast<T&>(this->get_curr()); }

  T * operator -> () { return &const_cast<T&>(this->get_curr()); }

  static StlIterator begin(const SetType & s)
  {
    return StlIterator(s);
  }

  static StlIterator end(const SetType & s)
  {
    StlIterator it(s);
    it.Itor::end();
    return it;
  }
};


template <class SetType>
struct StlConstIterator : public SetType::Iterator
{
  using T = typename SetType::Item_Type;
  using Itor = typename SetType::Iterator;

  using value_type = T;

  using Itor::Itor;

  Itor operator ++ ()
  {
    this->next();
    return *this;
  }

  Itor operator ++ (int)
  {
    Itor ret_val = *this;
    this->next();
    return ret_val;
  }

  bool operator == (const Itor & it) const
  {
    if (this->has_curr() and it.has_curr())
      return this->get_pos() == it.get_pos();

    if (not this->has_curr() and not it.has_curr())
      return true;

    return false;
  }

  bool operator != (const Itor & it) const
  {
    return not (*this == it);
  }

  const T & operator * () const { return this->get_curr(); }

  T const * operator -> () const
  { 
    return const_cast<T*>(&this->get_curr()); 
  }

  static StlConstIterator cbegin(const SetType & s)
  {
    return StlConstIterator(s);
  }

  static StlConstIterator cend(const SetType & s)
  {
    StlConstIterator it(s);
    it.Itor::end();
    return it;
  }
};


template <class SetName>
class StlAlephIterator
{
  SetName * me() { return static_cast<SetName*>(this); }

  const SetName * const_me() const { return static_cast<const SetName*>(this); }

public:

  using iterator = StlIterator<SetName>;

  using const_iterator = StlConstIterator<SetName>;

  iterator begin() noexcept { return iterator::begin(*me()); }

  iterator end() noexcept { return iterator::end(*me()); }

  const_iterator begin() const noexcept
  {
    return const_iterator::cbegin(*const_me());
  }

  const_iterator end() const noexcept
  {
    return const_iterator::cend(*const_me());
  }

  const_iterator cbegin() const noexcept
  {
    return const_iterator::cbegin(*const_me());
  }

  const_iterator cend() const noexcept
  {
    return const_iterator::cend(*const_me());
  }

  const_iterator cbegin() noexcept
  {
    return const_iterator::cbegin(*const_me());
  }

  const_iterator cend() noexcept 
  {
    return const_iterator::cend(*const_me()); 
  }

  friend const_iterator cbegin(const SetName & s) noexcept { return s.begin(); }

  friend const_iterator cend(const SetName & s) noexcept { return s.end(); }

  friend const_iterator begin(const SetName & s) noexcept { return s.begin(); }

  friend const_iterator end(const SetName & s) noexcept { return s.end(); }

  friend iterator begin(SetName & s) noexcept { return s.begin(); }

  friend iterator end(SetName & s) noexcept { return s.end(); }
};


/** Extract all the item of stl container ans save them in a DynList

    @ingroup Funcional
 */
template <class Container> inline
Aleph::DynList<typename Container::value_type> 
extract_from_stl_container(const Container & c)
{
  Aleph::DynList<typename Container::value_type> ret;
  for (const auto & i : c)
    ret.append(i);
  return ret;
}


# endif // AH_ITERATOR_H
