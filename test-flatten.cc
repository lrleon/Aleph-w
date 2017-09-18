
# include <memory>
# include <iostream>
# include <htlist.H>
# include <ahFunctional.H>

using namespace std;

int main()
{
  DynList<DynList<DynList<DynList<int>>>> l =
    { { {{1, 2, 3}, {4, 5, 6}}, {{7, 8, 9}, {10, 11, 12}} },
      { {{13, 14, 15}, {16, 17, 18}}, {{19, 20, 21}, {22, 23, 24}} }};

  // DynList<DynList<DynList<int>>> l =
  //   {{{1,2,3}, {4,5}}, {{6,7,8}, {9, 0}}};

  auto f = flatten(l);

  for (auto & i : f)
    cout << i << " ";
  cout << endl
       << endl;
}

