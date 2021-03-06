/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef FLOYD_WARSHALL_H
# define FLOYD_WARSHALL_H

# include <tpl_list_graph.H>
# include <tpl_dynDlist.H>
# include <tpl_dynListStack.H>

template <class GT>
void arc_to_distance(typename GT::Arc *                arc, 
		     typename GT::Arc::Distance_Type & dist)
{
  
}

    template <class GT>
Aleph::Pair<Ady_Mat<typename GT::Arc::Distance_Type> *,
	    Ady_Mat<typename GT::Path> *>
floyd_warshall_all_min_paths(GT & g)
{
  typename Ady_Mat<typename GT::Arc::Distance_Type> w;
}


# endif
