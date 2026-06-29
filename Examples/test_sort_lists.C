
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

# include <gsl/gsl_rng.h>
# include <cassert>
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <iostream>
# include <ahSort.H>
# include <tpl_sort_utils.H>

# include <ctime>
using namespace std;

gsl_rng * r = NULL;

template <template <typename T> class List,
	  typename T = long>
List<T> build_int_list(int n)
{
  List<T> ret_val;
  for (int i = 0; i < n; ++i)
    ret_val.append(gsl_rng_get(r));
  
  return ret_val;
}

template <template <typename T> class List,
	  typename T>
bool verify_sort(const List<T> & list)
{
  T value = numeric_limits<T>::min();
  return list.all([&value] (const T & item)
		  {
		    bool ret = value <= item;
		    value = item;
		    return ret;
		  });    
}

template <template <typename T> class List,
	  typename T = long>
List<T*> build_ptr_list(int n)
{
  List<T*> ret_val;
  for (int i = 0; i < n; ++i)
    ret_val.append(new T(gsl_rng_get(r)));
  
  return ret_val;
}

template <template <typename T> class List,
	  typename T >
bool verify_sort(const List<T*> & list)
{
  T value = numeric_limits<T>::min();
  return list.all([&value] (T * ptr)
		  {
		    bool ret = value <= *ptr;
		    value = *ptr;
		    return ret;
		  });    
}

template <template <typename T> class List, typename T>
void free_ptr_list(List<T*> & list)
{
  list.for_each([] (T * ptr)
		{
		  delete ptr;
		});
}

int main(int argc, char *argv[])
{
  unsigned long n = 1000;
  if (argc > 1)
    {
      errno = 0;
      char * endptr = nullptr;
      const unsigned long parsed = strtoul(argv[1], &endptr, 10);
      if (errno != 0 or endptr == argv[1] or *endptr != '\0' or
          parsed > static_cast<unsigned long>(INT_MAX))
        {
          cerr << "Invalid n: must be a non-negative integer <= "
               << INT_MAX << endl;
          return 1;
        }
      n = parsed;
    }

  unsigned int t = std::time(NULL);
  if (argc > 2)
    {
      errno = 0;
      char * endptr = nullptr;
      const unsigned long parsed_t = strtoul(argv[2], &endptr, 10);
      if (errno != 0 or endptr == argv[2] or *endptr != '\0' or
          parsed_t > UINT_MAX)
        {
          cerr << "Invalid t: " << argv[2] << endl;
          return 1;
        }
      t = static_cast<unsigned int>(parsed_t);
    }

  cout << argv[0] << " " << n << " " << t << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, t % gsl_rng_max(r));

  {
    cout << "Testing quicksort on single lists" << endl
	 << "Building list ... " << endl;
    DynList<long> l = build_int_list<DynList>(n);
    cout << "sorting it ..." << endl;
    quicksort(l);
    cout << "done! " << endl
	 << "Verifying ... " << endl;
    assert(verify_sort(l));
    assert(l.length() == n);
    cout << "done!" << endl
	 << endl;
  }

  {
    cout << "Testing quicksort on single lists of pointers" << endl
	 << "Building list ... " << endl;
    DynList<long*> l = build_ptr_list<DynList>(n);
    cout << "sorting it ..." << endl;
    quicksort(l, [] (long * x, long * y)
	      {
		return *x < *y;
	      });
    cout << "done! " << endl
	 << "Verifying ... " << endl;
    assert(verify_sort(l));
    assert(l.length() == n);
    cout << "done!" << endl
	 << endl;
    free_ptr_list(l);
  }

 {
    cout << "Testing mergesort on single lists" << endl
	 << "Building list ... " << endl;
    DynList<long> l = build_int_list<DynList>(n);
    cout << "sorting it ..." << endl;
    mergesort(l);
    cout << "done! " << endl
	 << "Verifying ... " << endl;
    assert(verify_sort(l));
    assert(l.length() == n);
    cout << "done!" << endl
	 << endl;
  }

 {
   cout << "Testing mergesort on single lists of pointers" << endl
	<< "Building list ... " << endl;
   DynList<long*> l = build_ptr_list<DynList>(n);
   cout << "sorting it ..." << endl;
   mergesort(l, [] (long * x, long * y)
	     {
	       return *x < *y;
	     });
   cout << "done! " << endl
	<< "Verifying ... " << endl;
   assert(verify_sort(l));
   assert(l.length() == n);
   cout << "done!" << endl
	<< endl;
   free_ptr_list(l);
 }

 {
   cout << "Testing default sort method on single lists" << endl
	<< "Building list ... " << endl;
   DynList<long> l = build_int_list<DynList>(n);
   cout << "sorting it ..." << endl;
   auto sorted = sort(l);
   cout << "done! " << endl
	<< "Verifying ... " << endl;
   assert(verify_sort(sorted));
   assert(sorted.length() == n);
   cout << "done!" << endl
	<< endl;
 }

  gsl_rng_free(r); 
  return 0;
}
