

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

    bool operator () (const GT & g, Path<GT> & path) const
    {
      Tarjan_Connected_Components<GT, Itor, SA> tarjan(sa);

      return tarjan.compute_cycle(g, path);
    }

    Path<GT> operator () (const GT & g) const
    {
      Path<GT> ret(g);
      Tarjan_Connected_Components<GT, Itor, SA> (sa).compute_cycle(g, ret);
      return ret;
    }

    Path<GT> operator () (const GT & g, typename GT::Node * src) const
    {
      Path<GT> ret(g);
      Tarjan_Connected_Components<GT, Itor, SA>(sa).compute_cycle(g, src, ret);
      return ret;
    }
  };

} // end namespace Aleph

# endif // TARJAN_H
