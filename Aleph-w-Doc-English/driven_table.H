
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file driven_table.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
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

  virtual ~Event_Table() { /* Empty */ }

private:

  void append_event_to_table(Event_Fct fct)
  {
    assert(fct not_eq NULL);

    write_table(size(), fct);
  }

protected:

  void fill_table_to_null(size_t num_events)
  {
    for (int i = 0; i < num_events; i++)
      write_table(i, NULL);
  }

public:

  void register_event(size_t index, Event_Fct fct)
  {
    assert(fct not_eq NULL);

    if (read_table(index) not_eq NULL)
      throw domain_error("Index is already used");

    write_table(index, fct);
  }

/** @brief Append a new event and return its index */
  size_t register_event(Event_Fct fct)
  {
    assert(fct not_eq NULL);

    append_event_to_table(fct);

    return size() - 1;
  }

  void unregister_event(size_t index)
  {
    if (read_table(index) == NULL)
      throw domain_error("Index is not used");

    write_table(index, NULL);

    if (index == size())
      cut_table(index - 1);
  }

  bool check(size_t index, Event_Fct fct) const
  {
    return index < size() and read_table(index) == fct;
  }

  void * execute_event(size_t index, void * input = NULL)
  {
    if (read_table(index) == NULL)
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

  virtual void cut_table(size_t) { /* Empty */ }

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
