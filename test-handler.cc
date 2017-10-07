
# include <iostream>

using namespace std;

void foo()
{
  throw domain_error("Foo");
}

void fun1()
{
  try
    {
      cout << "Calling fun1" << endl;
      foo();
    }
  catch (...)
    {
      cout << "exception in fun1" << endl;
      throw;
    }
}

void fun2()
{
  try
    {
      cout << "Calling fun2" << endl;
      fun1();
    }
  catch (...)
    {
      cout << "Caught in fun2" << endl;
    }
}

int main()
{
  fun2();
}
