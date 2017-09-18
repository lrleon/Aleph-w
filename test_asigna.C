
# include <iostream>

struct X
{
  int a;

  X() : a(-1) {}

  X& operator = (const X & x)
  {
    std::cout << "X::operator =" << std::endl;
  }
};


struct Y : public X
{
  Y& operator = (const Y & x)
  {
    std::cout << "Y::operator =" << std::endl;
  }
};


int main()
{
  Y a,b;

  a = b;
}
