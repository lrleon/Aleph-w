
# include <gsl/gsl_rng.h>
# include <cassert>
# include <iostream>
# include <ahSort.H>
# include <tpl_sort_utils.H>

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
  
  return std::move(ret_val);
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
  unsigned long n = argc > 1 ? atoi(argv[1]) : 1000;
  unsigned int  t = argc > 2 ? atoi(argv[2]) : time(NULL);

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
   cout << "done! " << endl
	<< "Verifying ... " << endl;
   assert(verify_sort(sort(DynList<long>(l))));
   assert(sort(l).length() == n);
   cout << "done!" << endl
	<< endl;
 }

  gsl_rng_free(r); 
  return 0;
}
