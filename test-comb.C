
# include <iostream>

# include <ah-comb.H>
# include <htlist.H>
# include <tpl_dynDlist.H>

using namespace std;


void test()
{
  DynList<char> l1 = { 'a', 'b', 'c' };
  DynList<char> l2 = { 'A', 'B', 'C' };
  DynList<char> l3 = { '1', '2', '3', '4' };
  DynList<char> l4 = { '5', '6', '7'};

  DynList<DynList<char>> l = { l1, l2, l3, l4 };

  traverse_perm(l, [] (auto s)
	  {
	    cout << "s = "; s.for_each([] (auto c) { cout << c << " "; });
	    cout << endl;
	    return true;
	  });

  cout << endl
       << endl;

  transpose(l).for_each([] (const auto & row)
			{
			  row.for_each([] (auto i) { cout << i << " "; });
			  cout << endl;
			});

  auto aux = l;
  in_place_transpose(aux);
  aux.for_each([] (const auto & row)
	       {
		 row.for_each([] (auto i) { cout << i << " "; });
		 cout << endl;
	       });
}


int main()
{
  test();
  return 0;
}
