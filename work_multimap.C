
# include "Vector.H"
# include <Multimap.H>

typedef std::pair<int, int> Pair;

    template <class mmap>
void print_mmap(mmap & mm)
{
  typename mmap::iterator it = mm.begin();
  const typename mmap::iterator end = mm.end();
  for (int i = 0; it != end; ++it, ++i)
    {
      Pair p = *it;
      cout << i << " = (" << p.first << "," << p.second << ") ";
    }

  cout << endl
       << "El multimapeo tiene " << mm.size() << " elementos" << endl
       << endl;
}

int main(int argn, char * arg[])
{
  int n = 10;
  int m1 = 2;
  int m2 = 3;

  if (argn >1)
    n = atoi(arg[1]);

  if (argn > 2)
    m1 = atoi(arg[2]);

  if (argn > 3)
    m2 = atoi(arg[3]);

  cout << arg[0] << " " << n << " " << m1 << " " << m2 << endl;

  Aleph::multimap<int,int> mm;

  for (int i = 0; i < n; ++i)
    for (int j = 0; j < m1; ++j)
      for (int k = 0; k < m2; ++k)
	{
	  multimap<int,int>::iterator it = mm.insert(Pair(i, j));
	  Pair p = *it;
	  cout << "(" << i << "," << j << ") == (" 
	       << p.first << "," << p.second << ") : " 
	       << (Pair(i,j) == p) << endl;
	}

  cout << endl << endl;

  Aleph::multimap<int,int>::iterator it(mm.begin()), end(mm.end());
  for (int i = 0; it != end; ++it, ++i)
    {
      Pair p = *it;
      cout << i << " = (" << p.first << "," << p.second << ")" << endl;
    }

  cout << endl
       << endl
       << "El multimapeo tiene " << mm.size() << " elementos" << endl;

  it = mm.begin();
  it += n/2;

  it = mm.end(); --it;
  for (int i = mm.size() - 1; it != mm.begin(); --it, --i)
    {
      Pair p = *it;
      cout << i << " = (" << p.first << "," << p.second << ")" << endl;
    }
  Pair p = *it;
  cout << "0 = (" << p.first << "," << p.second << ")" << endl
       << endl;
	
  it = mm.begin();
  it += n/2;

  for (int i = n/2; it != end; ++it, ++i)
    {
      Pair p = *it;
      cout << i << " = (" << p.first << "," << p.second << ")" << endl;
    }

  cout << endl
       << endl;

  it = mm.begin();
  it += n/2;
  p = *it;
  for (int i = 0; i < 30; i += 2)
    mm.insert(it, p);

  it = mm.begin();
  for (int i = 0; it != end; ++it, ++i)
    {
      Pair p = *it;
      cout << i << " = (" << p.first << "," << p.second << ")" << endl;
    }

  cout << endl
       << endl
       << "El multimapeo tiene " << mm.size() << " elementos" << endl;

  it = mm.begin();
  it += mm.size()/2;
  p = *it;
  p.second++;

  for (int i = 0; i < 10; ++i)
    mm.insert(it, p);

  print_mmap(mm);

  Aleph::vector<Pair> v;
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < m1; ++j)
      for (int k = 0; k < m2; ++k)
	{
	  v.push_back(Pair(i + n, j + 2*n));
	  Pair p = v.back();
	  cout << "(" << i + n << "," << j + 2*n << ") == (" 
	       << p.first << "," << p.second << ") : " 
	       << (Pair(i + n, j + 2*n) == p) << endl;
	}

  cout << "Insertando desde un contenedor" << endl;
  mm.insert(v.begin(), v.end());

  print_mmap(mm);

 cout << "Prueba de conteo" << endl
       << "mm.count(6) " << mm.count(6) << endl
       << endl;


  cout << "Prueba de constructor copia" << endl;

  Aleph::multimap<int,int> mm1 = mm;

  print_mmap(mm1);

  cout << "Prueba de asignacion" << endl;

  Aleph::multimap<int,int> mm2;

  mm2 = mm1;

  print_mmap(mm2);

  cout << "Prueba de clear y asignacion" << endl;

  mm2.clear();
  mm2 = mm1;

  print_mmap(mm2);

  cout << "Prueba de conteo" << endl
       << "mm.count(1) " << mm.count(1) << endl
       << endl
       << "Prueba de conteo" << endl
       << "mm.count(0) " << mm.count(0) << endl
       << endl
       << endl
       << "Prueba de busqueda ..." << endl;

  it = mm.find(1);
  cout << "find(1) = (" << it->first << "," << it->second << ")" << endl
       << "Ahora recorro y cuento desde este punto ..." << endl;
  int value = it->first; 
  ++it;
  int counter = 1;
  for (; it != mm.end(); ++counter, ++it)
    if (it->first != value)
      break;
    else
      cout << counter << " = (" << it->first << "," << it->second 
	   << ")" << endl;

  cout << "mm < mm1 = " << (mm < mm2 ? "true" : "false") << endl
       << endl;

  print_mmap(mm);

  print_mmap(mm2);


  cout << "Insercion intercalada ..." << endl;
  for (int i = 30; i < 130; i += 3)
    mm.insert(Pair(i, i + i));

  print_mmap(mm);

  const Aleph::multimap<int,int> mm3 = mm;

  typedef Aleph::multimap<int,int>::iterator Mitor;
  cout << counter << " ocurrencias recorridas" << endl
       << endl
       << "Prueba de equal_range(10) ";
  std::pair<Mitor,Mitor> pi = mm3.equal_range(10);
  for ( ; pi.first != pi.second; ++pi.first)
    cout << counter << " = (" << pi.first->first << "," << pi.first->second 
	 << ")" << endl;

  for (int i = 0; i < 5; ++i)
    {
      int v;
      cout << "Introduzca un valor a buscar lower_bound: ";
      cin >> v;
      it = mm.lower_bound(v);
	
      if (it != mm.end())
	{
	cout << endl
	     << endl
	     << "lower_bound(" << v << ") = (" <<  it->first << "," 
	     << it->second << ")" << endl;
	if (it == mm.begin())
	  cout << "el valor no se encontro pero es menor que todos" << endl;
	}
      else
	cout << "el valor no se encontro pero es mayor que todos" << endl;
      cout << endl;
    }

  for (int i = 0; i < 5; ++i)
    {
      int v;
      cout << "Introduzca un valor a buscar upper_bound: ";
      cin >> v;
      it = mm.upper_bound(v);
	
      if (it != mm.end())
	{
	  cout << endl
	       << endl
	       << "upper_bound(" << v << ") = (" <<  it->first << "," 
	       << it->second << ")" << endl;
	  if (it == mm.begin())
	    cout << "el valor no se encontro pero es menor que todos" << endl;
	}
      else
	cout << "el valor no se encontro pero es mayor que todos" << endl;
      cout << endl;
    }    

  cout << "max_size() = " << mm.max_size() << endl;

  cout << "Prueba de mm.erase(10)" << endl;
  int count = mm.erase(10);
  cout << "Borrados " << count << " elementos" << endl 
       << endl;

  cout << "Prueba de it = mm.erase(it);" << endl;
  it = mm.begin(); it++; ++it;
  it += 5;
  for (int i = 0; it != mm.end(); ++i)
    {
      Pair p = *it;
      it = mm.erase(it);
      if (it != mm.end())
	{
	  Pair d = *it;
	  cout << "eliminado " << i << " = (" << p.first << "," 
	       << p.second << ") next: ("  << d.first << "," 
	       << d.second << ")" << endl;
	}
    }
	
  print_mmap(mm);

  cout << "Prueba de conteo" << endl
       << "mm.count(6) " << mm.count(6) << endl
       << endl;
}
