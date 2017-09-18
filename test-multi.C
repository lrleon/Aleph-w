
struct F1
{
  void m1();
};


template <typename T>
struct F2
{
  void m1(int) {}
};

template <typename T1, typename T2>
struct F3
{
  void m1(int, int) {}
};


template <typename T1, typename T2, typename T3>
struct F : public F1, public F2<T1>, public F3<T2, T3>
{
  using F1::m1;
  using F2<T1>::m1;
  using F3<T2, T3>::m1;
};

int main()
{
  F<int, int, int> f;

  f.m1(5);
  f.m1(5,6);
  f.m1();
}
