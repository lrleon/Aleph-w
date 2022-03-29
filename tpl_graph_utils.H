

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

  template <class GT, 
            class Distance = Dft_Dist<GT>, 
            class SA       = Dft_Show_Arc<GT>>
  class Total_Cost
  {
    Distance dist;
    SA       sa;

  public:

    Total_Cost(Distance __dist = Distance(), SA __sa = SA()) 
      : dist(__dist), sa(__sa)
    {
      // empty
    }

    /// Compute the total cost
    typename Distance::Distance_Type total_cost(GT & g)
    {
      typename Distance::Distance_Type sum = 0;
  
      // recorrer todos los arcos y sumar su peso
      for (Arc_Iterator <GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        sum += dist(it.get_current_arc_ne());

      return sum;
    }

    /// \overload total_cost
    typename Distance::Distance_Type operator () (GT & g)
    {
      return total_cost (g);
    }

    bool operator () (typename GT::Arc * a)
    {
      if (not sa(a))
        return false;

      dist += dist(a);
      return true;
    }
  };



} // end namespace Aleph

# endif // TPL_GRAPH_UTILS_H
