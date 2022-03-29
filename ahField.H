

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

# ifndef AHFIELD_H
# define AHFIELD_H

# include <iostream>
# include <limits>
# include <ahFunction.H>

namespace Aleph {

  template <typename Dtype>
class Field
{
  Dtype num;

public:

  static const Dtype Zero = 0;

  static const Dtype Min;

  static const Dtype Max;

  typedef Dtype Distance_Type;

  typedef Dtype dtype;

  Dtype & get_value() { return num; }

  const Dtype & get_value() const { return num; }

  Field(Dtype && d = 0.0) : num(d)
  {
    // empty
  }

  Field(const Dtype & d) : num(d)
  {
    // empty
  }

  Field(Dtype & d) : num(d)
  {
    // empty
  }

  Field & operator = (const Dtype & d)
  {
    num = d;
    return *this;
  }

  Field & operator = (Dtype & d)
  {
    num = d;
    return *this;
  }

  Field & operator = (const Dtype && d)
  {
    num = d;
    return *this;
  }

  Field operator + (const Field & rhs)
  {
    return Field(num + rhs.num);
  }

  Field & operator += (const Field & rhs)
  {
    num += rhs.num;
    return *this;
  }
};


  template <class T>
std::ostream & operator << (std::ostream & osObject, const Field<T> & rightOp)
{
  osObject << rightOp.get_value();

  return osObject;     
}

    template <typename Dtype> 
const Dtype Field<Dtype>::Min = std::numeric_limits<Dtype>::min();


    template <typename Dtype> 
const Dtype Field<Dtype>::Max = std::numeric_limits<Dtype>::max();


typedef Field<int> Integer;

typedef Field<short> Short;

typedef Field<float> Float;

typedef Field<double> Double;

typedef Field<long double> Ldouble;


} // end namespace Aleph

# endif // AHFIELD_H
