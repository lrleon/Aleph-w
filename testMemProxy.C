
# include <memProxy.H>

struct Foo
{
  int i, j, k;

  Foo() {}

  Foo(int a, int b, int c)
    : i(a), j(b), k(c)
    {}
};

struct Bar
{
  int i, j, k;

  Bar() {}

  Bar(int a, int b, int c)
    : i(a), j(b), k(c)
    {}
};

void testProxies()
{
  Foo *f1;
  Bar *f2, *f3;

  MemProxy<Foo> px1 = CtorMemProxy(f1) (1, 2, 3);
  MemProxy<Bar> px2 = CtorMemProxy(f2) (1, 2, 3);
  MemProxy<Bar> px3 = CtorMemProxy(f3) (1, 2, 3);

  closeMemProxy(px1, px2);
  
}


int main()
{
  testProxies();
}
