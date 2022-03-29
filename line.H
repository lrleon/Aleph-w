

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

struct LineEq
{
  mutable double y0 = 0;
  mutable double m = 1;

  /// Build y = x
  LineEq() {}

  /// Build a line given cut with y axis and its slope
  LineEq(const double __y0, const double __m) : y0(__y0), m(__m) {}

  /// Build a line given a point (x1,y1) and its slope
  LineEq(const double x1, const double y1, const double __m) : m(__m)
  {
    y0 = y1 - m*x1;
  }

  /// Build a line given two points
  LineEq(const double x1, const double y1, const double x2, const double y2)
  {
    if (x1 >= x2)
      ALEPHTHROW(OutOfRange, "value x1 = " + ::to_string(x1) +
	       " is greater or equal than x2 = " + ::to_string(x2));
    if (y1 == y2)
      ALEPHTHROW(OutOfRange, "value of y1 = " + ::to_string(y1) + " == y2 = " +
	       ::to_string(y2));
    m = (y2 - y1)/(x2 - x1);
    y0 = y1 - m*x1;
  }

  /// Return the line value at x value
  double operator() (const double x) const noexcept { return y0 + m*x; }

  /// Return a pair with the intersection point of this with
  /// `l`. Throws `OutOfRange` if lines are parallel
  pair<double, double> intersection(const LineEq & l) const
  {
    if (m == l.m)
      ALEPHTHROW(OutOfRange, "parallel lines");

    double x = (y0 - l.y0)/(l.m - m);

    return pair<double, double>(x, (*this)(x));
  }

  bool operator == (const LineEq & l) const noexcept
  {
    return y0 == l.y0 and m == l.m;
  }

  bool operator != (const LineEq & l) const noexcept
  {
    return not (*this == l);
  }

  string to_string() const
  {
    ostringstream s;
    s << "y = " << y0 << " + " << m << " * x";
    return s.str();
  }

  friend ostream & operator << (ostream & out, const LineEq & l)
  {
    return out << l.to_string();
  }
};

#endif
 
