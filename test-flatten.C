
# include <htlist.H>
# include <ahFunctional.H>

using namespace std;

template <typename T,
	  template <typename> class Container1,
	  template <typename> class Container2>
DynList<T> flaten(const Container1<Container2<T>> & c)
{
  DynList<T> ret;
  for (auto & l : c)
    for (auto & item : l)
      ret.append(item);

  return ret;
}

template <typename T,
	  template <typename> class Container1,
	  template <typename> class Container2,
	  template <typename> class Container3>
DynList<T>
flaten(const Container1<Container2<Container3<T>>> & c)
{
  DynList<T> ret;
  for (auto & l : c)
    ret.append(flaten(l));

  return ret;
}
	  

int main()
{
  DynList<DynList<DynList<DynList<int>>>> l =
    { { {{1, 2, 3}, {4, 5, 6}}, {{7, 8, 9}, {10, 11, 12}} },
      { {{13, 14, 15}, {16, 17, 18}}, {{19, 20, 21}, {22, 23, 24}} }};

  flaten<int>(l);
  
}

