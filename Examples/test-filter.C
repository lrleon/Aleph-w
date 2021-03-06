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

# include <htlist.H>
# include <filter_iterator.H>

struct Filt
{
  bool operator () (long i) const { return i > 10 and i < 50; }
};

int main(int argc, char *argv[])
{
  DynList<long> l = range(1000l);

  auto F = [] (long i) { return i < 100; };

  for (Filter_Iterator<DynList<long>, DynList<long>::Iterator, Filt> it(l); 
       it.has_curr(); it.next())
    ;

}

