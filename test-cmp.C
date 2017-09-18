
# include <iostream>
# include <string>

using namespace std;

struct Functor
{
  bool operator () (int i1, int i2) const
  {
    return i1 < i2;
  }
};

template <class Cmp = Functor>
struct Container
{
  Cmp & cmp;

  Container(Cmp & __cmp) : cmp(__cmp) {}

  Container(Cmp && cmp) : Container(cmp) {}

  string test(const int & op1, const int & op2) const
  {
    return to_string(cmp(op1, op2));
  }
};

int main()
{
  cout << Container<Functor>(Functor()).test(4, 5) << endl
       << Container<Functor>(Functor()).test(5, 4) << endl
       << Container(Functor()).test(4, 5) << endl;

}
