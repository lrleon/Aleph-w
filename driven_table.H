

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

public:

  virtual size_t size() const { return size_table; }
  
  Static_Event_Table(size_t num_events = 0) 
    : table(new Event_Fct [num_events]), size_table(num_events)
  {
    fill_table_to_null(num_events);
  }  

  ~Static_Event_Table()
  {
    delete [] table;
  }
};


class Dynamic_Event_Table : public Event_Table
{
private:

  DynArray<Event_Fct> table;

  virtual Event_Fct read_table(int i) const { return table[i]; }

  virtual void write_table(int i, Event_Fct event_fct) 
  {
    table[i] = event_fct;
  }

  virtual void cut_table(size_t dim) { table.cut(dim); }

public:

  virtual size_t size() const { return table.size(); }

  Dynamic_Event_Table(size_t num_events = 0) : table(num_events) 
  {
    fill_table_to_null(num_events);
  }  
};


# endif
