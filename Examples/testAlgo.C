
# include <iostream>
# include <iterator>
# include <ahFunction.H>
# include <ahAlgo.H>
# include <Vector.H>
# include <Map.H>
# include <Multiset.H>
# include <Multimap.H>
# include <Set.H>
# include <List.H>
# include "print_container.H"

    template <class T>
void print (const T & t)
{
  cout << t << " ";
}


    template <class T>
struct Print_Pair
{
  void operator () (const std::pair<T, T> & p)
  {
    cout << "(" << p.first << "," << p.second << ") ";
  }
};

    template <class T>
struct Assign_Pair
{
  int counter;

  Assign_Pair() : counter(0) { /* empty */ }

  void operator () (std::pair<T, T> & p)
  {
    p.first = p.second = counter++;
  }
};

struct Assign
{
  int counter;

  Assign() : counter(0) { /* empty */ }

  Assign(int count) : counter(count) { /* empty */ }

  void operator () (int & d)
  {
    d = counter++;
  }
};

    template <class T>
struct Generate
{
  T data;

  Generate () : data() { /* empty */ }

  Generate (const T & d) : data (d) { /* empty */ }

  T operator () ()
  {
    return data++;
  }
};

    template <class T>
struct Greater_Than
{
  T data;

  Greater_Than(const T & d) : data(d) { /* empty */ }

  bool operator () (const T& d)
  {
    if (d > data)
      return true;

    return false;
  }
};

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 50;

  unsigned int t = time(0);

  int mm = 5;
  if (argn > 2)
    mm = atoi(argc[1]);

  srand(t);

  if (argn > 3)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Aleph::vector<int> v(n);

  Aleph::for_each(v.begin(), v.end(), Assign() );

  Aleph::for_each(v.begin(), v.end(), print<int>);
  cout << endl;

  cout << "Hay " << Aleph::count(v.begin(), v.end(), n/2) 
	    << " elementos de valor " << n/2 << endl;

  n = Aleph::count_if(v.begin(), v.end(), Greater_Than <int> (n/2) );
  cout << "Hay " << n << " elementos de pares " << endl;

  int min = * Aleph::min_element(v.begin(), v.end());
  cout << "El menor elemento es : " << min << endl;

  min = * Aleph::min_element(v.begin(), v.end(), Aleph::less<int>());
  cout << "El menor elemento es : " << min << endl;

  int max = * Aleph::max_element(v.begin(), v.end());
  cout << "El mayor elemento es : " << max << endl;

  max = * Aleph::max_element(v.begin(), v.end(), Aleph::greater<int>());
  cout << "El mayor elemento es : " << max << endl;

  int m = * Aleph::find(v.begin(), v.end(), n/2);
  cout << "Encontrado elemento : " << m << " (" << m << ")" << endl;

  m = * Aleph::find_if(v.begin(), v.end(), Greater_Than<int>(n/2));
  cout << "Encontrado elemento : " << m << " (" << m << ")" << endl;

  multiset<int> mset(v.begin(), v.end());
  cout << "mset: ";
  print_container(mset);

  for (int i = 0; i < m; i++)
    mset.insert(mset.begin(), n/2);

  multiset<int>::iterator it = 
    Aleph::search_n(mset.begin(), mset.end(), mm, n/2);
  if (it != mset.end())
    {
      cout << "Encontrados " << mm << " instancias de " << n/2 
		<< endl;
      while (*it == n/2)
	cout << *it++ << " ";
      cout << endl;
    }
  else
    cout << "No encontrados " << mm << " instancias de " 
	      << n/2 << endl;

  cout << "adjacent_find(mset.begin(), mset.end()) = ";
  multiset<int>::iterator its = Aleph::adjacent_find(mset.begin(), mset.end());
  if (its != mset.end())
    cout << *its;
  else
    cout << "No encontrado";
  cout << endl;


  it = Aleph::search_n(mset.begin(), mset.end(), mm, n/2, Aleph::less<int>());
  if (it != mset.end())
    {
      cout << "Encontrados " << mm << " instancias de menores que " 
		<< n/2 << endl;
      while (Aleph::less<int> () (*it, n/2) )
	cout << *it++ << " ";
      cout << endl;
    }
  else
    cout << "No encontrados " << mm << " instancias de " 
	      << n/2 << endl;

  Aleph::vector<int>::iterator itor_v_beg = v.begin(); 
  itor_v_beg += n/8;
  Aleph::vector<int>::iterator itor_v_end = itor_v_beg;
  itor_v_end += 4;

  cout << "mset:";
  print_container(mset);

  cout << "Pruebas de busquedas del rango: ";
  print_range(itor_v_beg, itor_v_end);
  cout << endl;

  {
    cout << "rit = search(mset.begin(), mset.end(), itor_v_beg, itor_v_end);"
	 << endl;
    multiset<int>::iterator rit = 
      Aleph::search(mset.begin(), mset.end(), itor_v_beg, itor_v_end);
    if (rit != mset.end() and itor_v_beg != itor_v_end)
      {
	cout << "Encontrado sub-rango con search multiset " << endl;
	while (rit != mset.end() and itor_v_beg != itor_v_end)
	  cout << *itor_v_beg++ << "==" << *rit++ << " ";
	cout << endl << endl;
      }
    else
      {
	cout << "no se encontro el sub-rango ";
	Aleph::for_each(itor_v_beg, itor_v_end, print<int>);
	cout << " mediante search en el multiset " ;
	Aleph::for_each(mset.begin(), mset.end(), print<int>);
	cout << endl;
      }
  
    itor_v_beg = v.begin(); 
    itor_v_beg += n/4;
    rit = 
      Aleph::search(mset.begin(), mset.end(), itor_v_beg, itor_v_end,
		    Aleph::equal_to<int>());
    if (rit != mset.end() and itor_v_beg != itor_v_end)
      {
	cout << "Encontrado sub-rango con search multiset" << endl;
	while (rit != mset.end() and itor_v_beg != itor_v_end)
	  cout << *itor_v_beg++ << "==" << *rit++ << " ";
	cout << endl << endl;
      }
    else
      {
	cout << "no se encontro sub-rango" ;
	Aleph::for_each(itor_v_beg, itor_v_end, print<int>);
	cout << " mediante search_n en el multiset " ;
	Aleph::for_each(mset.begin(), mset.end(), print<int>);
	cout << endl;
      }

    itor_v_beg = v.begin(); 
    itor_v_beg += n/4;
    rit = Aleph::find_end(mset.begin(), mset.end(),
			       itor_v_beg, itor_v_end);
    if (rit != mset.end() and itor_v_beg != itor_v_end)
      {
	cout << "Encontrado sub-rango con find_end multiset" << endl;
	while (rit != mset.end() and itor_v_beg != itor_v_end)
	  cout << *itor_v_beg++ << "==" << *rit++ << " "; 
	cout << endl << endl;
      }
    else
      cout << "no se encontro sub-rango con find_end multiset" 
		<< endl;
  }

  {
    itor_v_beg = v.begin(); 
    itor_v_beg += n/4;
    set<int> s(v.begin(), v.end());

    set<int>::iterator rit = 
      Aleph::find_end(s.begin(), s.end(), itor_v_beg, itor_v_end, 
		      Aleph::equal_to<int>());
    if (rit != s.end() and itor_v_beg != itor_v_end)
      {
	cout << "Encontrado sub-rango con find_end set" << endl;
	while (rit != s.end())
	  cout << *itor_v_beg++ << "==" << *rit++ << " ";
      cout << endl << endl;
    }
    else
      cout << "no se encontro sub-rango con find_end set" << endl;
  }

  {
    itor_v_beg = v.begin(); 
    itor_v_beg += n/4;
    Aleph::vector<int> v1(v.begin(), v.end());

    Aleph::vector<int>::iterator rit = 
      Aleph::find_end(v1.begin(), v1.end(), itor_v_beg, itor_v_end, 
		      Aleph::equal_to<int>());
    if (rit != v1.end() and itor_v_beg != itor_v_end)
      {
	cout << "Encontrado sub-rango con find_end vector" << endl;
	while (rit != v1.end() and itor_v_beg != itor_v_end)
	  cout << *itor_v_beg++ << "==" << *rit++ << " ";
	cout << endl << endl;
      }
    else
      cout << "no se encontro sub-rango con find_end vector" << endl;
  }

  {
    itor_v_beg = v.begin(); 
    itor_v_beg += n/4;
    list<int> l(v.begin(), v.end());

    list<int>::iterator rit = 
      Aleph::find_end(l.begin(), l.end(), itor_v_beg, itor_v_end, 
		      Aleph::equal_to<int>());
    if (rit != l.end() and itor_v_beg != itor_v_end)
      {
	cout << "Encontrado sub-rango con find_end list" << endl;
	while (rit != l.end() and itor_v_beg != itor_v_end)
	  cout << *itor_v_beg++ << "==" << *rit++ << " ";
	cout << endl << endl;
      }
    else
      cout << "no se encontro sub-rango con find_end list" << endl;

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    cout << "adjacent_find(v.begin(), v.end()) = ";
    Aleph::vector<int>::iterator itv = Aleph::adjacent_find(v.begin(), v.end());
    if (itv != v.end())
      cout << *itv;
    else
      cout << "No encontrado";
    cout << endl;

    cout << "adjacent_find(l.begin(), l.end()) = ";
    list<int>::iterator itl = Aleph::adjacent_find(l.begin(), l.end());
    if (itl != l.end())
      cout << *itl;
    else
      cout << "No encontrado";
    cout << endl;
  }

  {
    multimap<int, int> mmap1;
    for (int i = 0; i < n; ++i)
      mmap1.insert(make_pair(i, i));
    cout << "Mapping 1 ";
    Aleph::for_each(mmap1.begin(), mmap1.end(), Print_Pair<int>());
    cout << endl;

    multimap<int, int> mmap2(mmap1);
    cout << "Mapping 2 ";
    Aleph::for_each(mmap2.begin(), mmap2.end(), Print_Pair<int>());
    cout << endl;

    multimap<int, int>::iterator it1 = mmap1.begin();
    it1 += n/3;
    multimap<int, int>::iterator it2 = it1;
    it2 += 10;

    cout << "Rango ";
    Aleph::for_each(it1, it2, Print_Pair<int>());
    cout << endl;

    multimap<int, int>::iterator rit =
      Aleph::find_first_of(mmap2.begin(), mmap2.end(), it1, it2);

    cout << "find_first_of(mmap2.begin(), mmap2.end(), it1, it2) = ";
    if (rit != mmap2.end())
      cout << "(" << rit->first << "," <<  rit->second 
		<< ")" << endl;
    else
      cout << "No se encontro" << endl;

    cout << "equal(mmap1.begin(), mmap1.end(), mmap2.begin()) = ";
    if (Aleph::equal(mmap1.begin(), mmap1.end(), mmap2.begin()))
      cout << "true";
    else
      cout << "false";
    cout << endl;

    mmap1.insert(make_pair(n, n-1));

    cout << "equal(mmap1.begin(), mmap1.end(), mmap2.begin()) = ";
    if (Aleph::equal(mmap1.begin(), mmap1.end(), mmap2.begin()))
      cout << "true";
    else
      cout << "false";
    cout << endl;

    std::pair<multimap<int, int>::iterator, 
      multimap<int, int>::iterator> diff =
      Aleph::mismatch(mmap1.begin(), mmap1.end(), mmap2.begin());

    cout << "(" << diff.first->first << "," << diff.first->second 
	      << ") != (" << diff.second->first << "," << diff.second->second
	      << ")" << endl;

    cout << "Mapping 1 ";
    Aleph::for_each(mmap1.begin(), mmap1.end(), Print_Pair<int>());
    cout << endl;

    cout << "Mapping 2 ";
    Aleph::for_each(mmap2.begin(), mmap2.end(), Print_Pair<int>());
    cout << endl;
  }

  {
    Aleph::list<int> l(v.begin(), v.end());
    Aleph::for_each(l.begin(), l.end(), Assign(1024) );
    Aleph::list<int> l1(l);
    cout << "list ";
    Aleph::for_each(l.begin(), l.end(), print<int>);
    cout << endl;

    cout << "list 1 ";
    Aleph::for_each(l1.begin(), l1.end(), print<int>);
    cout << endl;

    Aleph::copy(v.begin(), v.end(), l.begin()); 
    cout << "list ";
    Aleph::for_each(l.begin(), l.end(), print<int>);
    cout << endl;

    Aleph::copy_backward(v.begin(), v.end(), l1.end()); 
    cout << "list 1 ";
    Aleph::for_each(l1.begin(), l1.end(), print<int>);
    cout << endl;
  }

  {
    Aleph::list<int> l(v.begin(), v.end());
    cout << "list ";
    Aleph::for_each(l.begin(), l.end(), print<int>);
    cout << endl;

    Aleph::set<int> s(v.begin(), v.end());
    cout << "set ";
    Aleph::for_each(s.begin(), s.end(), print<int>);
    cout << endl;

    // Aleph::transform(l.begin(), l.end(), s.begin(), Aleph::negate<int>());
    // cout << "set ";
    // Aleph::for_each(s.begin(), s.end(), print<int>);
    // cout << endl;

    Aleph::transform(s.begin(), s.end(), l.begin(), v.begin(), 
		     Aleph::plus<int>());
    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::swap_ranges(l.begin(), l.end(), v.begin());

    cout << "list ";
    Aleph::for_each(l.begin(), l.end(), print<int>);
    cout << endl;

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::fill(v.begin(), v.end(), 10);

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::fill_n(v.begin(), v.size(), 11);

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::generate(v.begin(), v.end(), Generate<int>(0));

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::generate_n(v.begin(), v.size(), Generate<int>(10));

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::replace_if(v.begin(), v.end(), Greater_Than<int>(30), 200);

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::replace(v.begin(), v.end(), 200, 300);

    cout << "vector ";
    Aleph::for_each(v.begin(), v.end(), print<int>);
    cout << endl;

    Aleph::replace_copy(v.begin(), v.end(), 
			ostream_iterator<int>(cout, " "),
			300, 400);
    cout << endl;
  }

  {
    Aleph::vector<int> vv(v);

    cout << "vector ";
    Aleph::for_each(vv.begin(), vv.end(), print<int>);
    cout << endl;

    Aleph::vector<int>::iterator mid = vv.begin();

    mid += n/2;

    Aleph::rotate(vv.begin(), mid, vv.end());

    cout << "vector ";
    Aleph::for_each(vv.begin(), vv.end(), print<int>);
    cout << endl;

  }
  {
    list<int> ll(v.begin(), v.end());

    cout << "lista ";
    Aleph::for_each(ll.begin(), ll.end(), print<int>);
    cout << endl;

    list<int>::iterator lmid = ll.begin();

    lmid += n/2;

    Aleph::rotate(ll.begin(), lmid, ll.end());

    cout << "lista ";
    Aleph::for_each(ll.begin(), ll.end(), print<int>);
    cout << endl;
  }

  {
    map<int, int> mmap;

    for (int i = 0; i < n; ++i)
      mmap.insert(make_pair(i, i));

    cout << "Mapping ";
    Aleph::for_each(mmap.begin(), mmap.end(), Print_Pair<int>() );
    cout << endl;

    map<int, int>::iterator mid = mmap.begin();

    mid++; mid++;


    //    Aleph::rotate(mmap.begin(), mid, mmap.end());

    //    cout << "Mapping ";
    //    Aleph::for_each(mmap.begin(), mmap.end(), Print_Pair<int>());
    //    cout << endl;

  }
}
