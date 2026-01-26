
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


/** @file ahField.H
 *  @brief Field abstraction for numeric types with bounds.
 *
 *  Provides Field template class representing numeric fields with
 *  minimum, maximum, and invalid value sentinels. Useful for
 *  domain-specific numeric constraints.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
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
