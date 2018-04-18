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

# include <stdlib.h>
# include <iostream>
# include <time.h>
# include <tpl_dynBinHeap.H>

using namespace std;

int keys [] = { 36, 32, 4, 12, 52, 59, 2, 2, 26, 1 };

# include <string.h>

struct Record
{
  char buf[10];
  int i;

  Record (int n) : i(n) 
  { 
    bzero(buf, sizeof(buf[10]));
  }

  virtual ~Record() { }

  virtual void foo() = 0;
};


struct Rec : public Record
{
  Rec(int j) : Record(j) {}

  virtual void foo() 
  {

  }

  virtual ~Rec() {}
};

struct Cmp_Rec
{
  bool operator () (const Record & l, const Record & r) const
  {
    return l.i < r.i; 
  }

  bool operator () (Record * l, Record * r) const
  {
    return l->i < r->i; 
  }
};

struct Cmp_Ptr
{
  bool operator () (unsigned int* p1, unsigned int* p2) const
  {
    return *p1 < *p2;
  }
};


int main(int argn, char* argc[])
{
  MESSAGE("main()");
  srand(time(0));
  unsigned int n = 10;

  if (argn > 1)
    n = atoi(argc[1]);

  {
    DynBinHeap<int> heap;
    unsigned int i, value;

    for (i = 0; i < n; i++)
      {
	value = (int) ((10.0*n*rand())/(RAND_MAX+1.0));
	cout << value << ", ";
	heap.insert(value);
      }

    assert(heap.verify_heap());

    cout << "\n\n"
	 << "Sum = " << heap.fold(0, [] (const int acu, const int i)
				  {
				    return acu + i;
				  });
    cout << endl
	 << endl;
  }
  
  { 
    cout << "testing with pointers" << endl;

    DynBinHeap<unsigned int*, Cmp_Ptr> heap;
    unsigned int i, value;
    unsigned int *ptr;

    for (i = 0; i < n; i++)
      {
	value = (int) ((10.0*n*rand())/(RAND_MAX+1.0));
	ptr = new unsigned int;
	*ptr = value;
	cout << value << ", ";
	heap.insert(ptr);
      }

    cout << "\n\n"
	 << "Extracting from heap" << endl;

    for (i = 0; i < n; i++)
      {
	ptr = heap.getMin();
	value = *ptr;
	cout << value << " ";
	delete ptr;
      }

    cout << "\n\n";

    heap.size();
  }

  { 
    cout << "testing with pointers to record" << endl;

    DynBinHeap<Record*, Cmp_Rec> heap;
    unsigned int i, value;
    Rec *ptr;

    for (i = 0; i < n; i++)
      {
	value = (int) ((10.0*n*rand())/(RAND_MAX+1.0));
	ptr = new Rec(value);
	cout << value << ", ";
	heap.insert(ptr);
      }

    cout << "\n\n"
	 << "Extracting from heap" << endl;

    for (i = 0; i < n; i++)
      {
	ptr = static_cast<Rec*>(heap.getMin());
	value = ptr->i;
	cout << value << " ";
	delete ptr;
      }

    cout << "\n\n";
  }

}
