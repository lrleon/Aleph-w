
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


/** @file driven_table.H
 *  @brief Event-driven table abstraction.
 *
 *  Provides abstract base classes for static and dynamic event tables,
 *  supporting read, write, cut, and size operations for event-driven
 *  simulations.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef DRIVEN_TABLE_H
# define DRIVEN_TABLE_H

# include <aleph.H>
# include <tpl_dynArray.H>
 # include <ah-errors.H>

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

  void fill_table_to_null(const size_t num_events)
  {
    for (int i = 0; i < num_events; i++)
      write_table(i, nullptr);
  }

public:

  void register_event(const size_t index, Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    ah_domain_error_if(index >= size())
        << "Index out of range";

    ah_domain_error_if(read_table(index) not_eq nullptr) << "Index is already used";

    write_table(index, fct);
  }

      /* append a new event and return its index */
  size_t register_event(Event_Fct fct)
  {
    assert(fct not_eq nullptr);

    append_event_to_table(fct);

    return size() - 1;
  }

  void unregister_event(const size_t index)
  {
    ah_domain_error_if(index >= size())
        << "Index out of range";

    ah_domain_error_if(read_table(index) == nullptr) << "Index is not used";

    write_table(index, nullptr);

    if (index == size())
      cut_table(index - 1);
  }

  bool check(const size_t index, Event_Fct fct) const
  {
    return index < size() and read_table(index) == fct;
  }

  void * execute_event(const size_t index, void * input = nullptr)
  {
    if (read_table(index) == nullptr)
      ah_domain_error_if(true) << "Index is not used";

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
      ah_range_error_if(true) << "index out of range";
  }

  Event_Fct read_table(int i) const override
  { 
    verify_index(i);
    return table[i]; 
  }

  void write_table(int i, Event_Fct event_fct) override
  {
    verify_index(i);
    table[i] = event_fct;
  }

  void cut_table(size_t) override { /* empty */ }

public:

  size_t size() const override { return size_table; }
  
  Static_Event_Table(size_t num_events = 0) 
    : table(new Event_Fct [num_events]), size_table(num_events)
  {
    fill_table_to_null(num_events);
  }

  /// Copying disabled (raw pointer management)
  Static_Event_Table(const Static_Event_Table &) = delete;
  Static_Event_Table & operator=(const Static_Event_Table &) = delete;

  /// Move construction transfers ownership
  Static_Event_Table(Static_Event_Table && other) noexcept
    : table(other.table), size_table(other.size_table)
  {
    other.table = nullptr;
    other.size_table = 0;
  }

  /// Move assignment transfers ownership
  Static_Event_Table & operator=(Static_Event_Table && other) noexcept
  {
    if (this != &other)
      {
        delete [] table;
        table = other.table;
        size_table = other.size_table;
        other.table = nullptr;
        other.size_table = 0;
      }
    return *this;
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

  Event_Fct read_table(int i) const override { return table[i]; }

  void write_table(int i, Event_Fct event_fct) override
  {
    table[i] = event_fct;
  }

  void cut_table(size_t dim) override { table.cut(dim); }

public:

  size_t size() const override { return table.size(); }

  Dynamic_Event_Table(size_t num_events = 0) : table(num_events) 
  {
    fill_table_to_null(num_events);
  }  
};


# endif
