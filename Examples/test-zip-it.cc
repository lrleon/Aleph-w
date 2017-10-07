
# include <iostream>

# include <ah-zip.H>
# include <tpl_dynSetTree.H>

using namespace std;

DynList<int> l1 = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
DynList<int> l2 = { 0, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
DynList<char> l3 = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k' };
DynSetTree<char> l5 = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k' };

int main()
{
  auto it = get_zip_it(l1, l2, l3, l5);
  for (; it.has_curr(); it.next())
    {
      auto curr = it.get_curr();
      cout << "curr = (" << get<0>(curr) << ", " << get<1>(curr) << ", "
	   << get<2>(curr) << ")" << endl;
    }

  zip_for_each([] (auto t) {}, l1, l2);

  assert(not it.completed());

  auto l = t_enum_zip(l1, l2, l3);

  auto lu = t_unzip(l);

  for (auto it = get<3>(lu).get_it(); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;

  auto l4 = l1.maps([] (auto i) { return 10*i; });

  for (auto it = get_zip_it(l1, l2, l4); it.has_curr(); it.next())
    {
      auto l = it.get_curr_list();
      l.for_each([] (auto i) { cout << i << " "; });
      cout << endl;
    }

  zip_lists(l1, l2, l4).for_each([] (auto & l)
				 {
				   l.for_each([] (auto i) { cout << i << " "; });
				   cout << endl;
				 });

  zip_maps<int>
    ([] (tuple<int, int, int> t) { return get<0>(t) + get<1>(t) + get<2>(t); },
     l1, l2, l4).for_each([] (auto i) { cout << i << endl; });
}
