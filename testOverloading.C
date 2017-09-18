
# include <iostream>

using namespace std;

template <class T> void foo(const T & t) {}

template <class T> void foo(T & t) {}

template <class T> class Foo
{
public:
  template <class T1, class T2, class T3>
T* foo (T1& t1, T2& t2, T3& t3)
{
  void *p;
  try                                  
    {      
      T * ptr = new (p) T (t1, t2, t3);

      return ptr;
    }                                  
  catch (...)                          
    {                                  

      throw;                           
    }

}
};

class X 
{
public:
  X(int, int, int) {}

};

int  main() 
{
  Foo<X> f;

  int i;

    f.foo(i, i, i);

  cout << "hello word" << endl;
}
