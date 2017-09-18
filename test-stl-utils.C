
# include <iostream>
# include <ah-stl-utils.H>

int main()
{
  auto t = make_tuple(1,2,3,4,5,6);

  auto l = tuple_to_dynlist(t);

  l.for_each([] (auto i) { cout << i << " "; }); cout << endl;

  auto ll = tuple_to_array(t); 

  ll.for_each([] (auto i) { cout << i << " "; }); cout << endl;
}

