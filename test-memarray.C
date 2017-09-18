
# include <iostream>
# include <tpl_memArray.H>

using namespace std;
using namespace Aleph;

int count = -1;

struct Foo
{
  int * ptr;

  void swap(Foo & f)
  {
    std::swap(ptr, f.ptr);
  }

  Foo() : ptr(NULL)
  {
    ptr = new int;
    *ptr = ::count--;
  }

  Foo(int i) : ptr(NULL)
  {
    ptr = new int;
    *ptr = i;
  }

  Foo(const Foo & f) : ptr(NULL)
  {
    ptr = new int;
    *ptr = *f.ptr;
  }

  Foo(Foo && f) : ptr(NULL)
  {
    std::swap(ptr, f.ptr);
  }

  Foo & operator = (const Foo & f)
  {
    if (this == &f)
      return *this;

    *ptr = *f.ptr;

    return *this;
  }

  Foo & operator = (Foo && f)
  {
    swap(f);
    return *this;
  }

  ~Foo()
  {
    if (ptr != NULL)
      {
	delete ptr;
	ptr = NULL;
      }
  }

  // operator int () { return *ptr; }

  operator int () const { return *ptr; }
};

    template <typename T>
void print(const MemArray<T> & s)
{
  cout << "capacity = " << s.capacity() << endl
       << "size     = " << s.size() << endl;

  for (int i = 0; i < s.size(); ++i)
    {
      int val = s[i];
      cout << val << " ";
    }
  cout << endl
       << endl;
}

template <typename T>
MemArray<T> create_memarray(int n)
{
  MemArray<T> ret(n);

  for (int i = 0; i < n; ++i)
    ret.put(T(i));

  return ret;
}

int main(int, char * argc[])
{
  MemArray<int> s;

  print(s);

  int n = atoi(argc[1]);

  for (int i = 0; i < n; ++i)
    s.put(i);

  print(s);

  int m = n/4;
  if (argc[2] != NULL)
    m = atoi(argc[2]);
  
  cout << "Extracting " << m << " items" << endl;
  for (int i = 0; i < m; ++i)
    cout << s.get() << " ";
  cout << endl;

  print(s);

  MemArray<int> c(s);

  print(c);

  print(create_memarray<int>(n));

  print(create_memarray<Foo>(n));
}
