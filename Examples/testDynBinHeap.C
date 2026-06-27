
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# include <cstdlib>
# include <ctime>
# include <cstring>
# include <iostream>
# include <tpl_dynBinHeap.H>

# include <cassert>
using namespace std;

int keys [] = { 36, 32, 4, 12, 52, 59, 2, 2, 26, 1 };

struct Record
{
  char buf[10];
  int i;

  Record (int n) : i(n) 
  { 
    std::memset(buf, 0, sizeof(buf));
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


int main(int argc, char* argv[])
{
  MESSAGE("main()");
  srand(time(nullptr));
  unsigned int n = 10;

  if (argc > 1)
    n = static_cast<unsigned int>(atoi(argv[1]));

  {
    cout << "Testing with predefined keys array" << endl;
    DynBinHeap<int> heap;
    for (int key : keys)
      heap.insert(key);

    assert(heap.verify_heap());
    cout << "Predefined keys heap size: " << heap.size() << endl;
  }

  {
    DynBinHeap<int> heap;
    unsigned int i, value;

    for (i = 0; i < n; i++)
      {
	value = (unsigned int) ((10.0*n*rand())/(RAND_MAX+1.0));
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
	value = (unsigned int) ((10.0*n*rand())/(RAND_MAX+1.0));
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
  }

  { 
    cout << "testing with pointers to record" << endl;

    DynBinHeap<Record*, Cmp_Rec> heap;
    unsigned int i, value;
    Rec *ptr;

    for (i = 0; i < n; i++)
      {
	value = (unsigned int) ((10.0*n*rand())/(RAND_MAX+1.0));
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
