
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

# ifndef DRIVEN_TABLE_H
# define DRIVEN_TABLE_H

# include <aleph.H>
# include <tpl_dynArray.H>

using namespace std;

typedef void * (*Event_Fct)(void*);

class Event_Table
{
private:

  virtual Event_Fct read_table(int i) const = 0;

  virtual void write_table(int i, Event_Fct event_fct) = 0;

  virtual void cut_table(size_t dim) = 0;

public:

  virtual size_t size() const = 0;

  virtual ~Event_Table() { /* empty */ }

private:

  void append_event_to_table(Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    write_table(size(), fct);
  }

protected:

  void fill_table_to_null(size_t num_events)
  {
    for (int i = 0; i < num_events; i++)
      write_table(i, nullptr);
  }

public:

  void register_event(size_t index, Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    if (read_table(index) not_eq nullptr)
      throw domain_error("Index is already used");

    write_table(index, fct);
  }

      /* append a new event and return its index */
  size_t register_event(Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    append_event_to_table(fct);

    return size() - 1;
  }

  void unregister_event(size_t index)
  {
    if (read_table(index) == nullptr)
      throw domain_error("Index is not used");

    write_table(index, nullptr);

    if (index == size())
      cut_table(index - 1);
  }

  bool check(size_t index, Event_Fct fct) const
  {
    return index < size() and read_table(index) == fct;
  }

  void * execute_event(size_t index, void * input = nullptr)
  {
    if (read_table(index) == nullptr)
      throw domain_error("Index is not used");

    return (*read_table(index))(input);
  }
};


class Static_Event_Table : public Event_Table
{
private:

  Event_Fct * table;
  size_t      size_table;

  void verify_index(int i) const
  { 
    if (i < 0 or i >= size_table)
      throw range_error("index out of range");
  }

  virtual Event_Fct read_table(int i) const
  { 
    verify_index(i);
    return table[i]; 
  }

  virtual void write_table(int i, Event_Fct event_fct) 
  {
    verify_index(i);
    table[i] = event_fct;
  }

  virtual void cut_table(size_t) { /* empty */ }

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
