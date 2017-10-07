
# include <memory>
# include <iostream>
# include "tpl_odhash.H"

using namespace std;

struct Foo
{
  string a = "hello word";
  Foo() {}
  Foo(const string & str) : a(str) {}
};

struct FooPairEqual
{
  bool operator () (const pair<pair<const Foo*, const Foo*>, string> & p1,
		    const pair<pair<const Foo*, const Foo*>, string> & p2) const
  {
    return p1.first == p2.first;
  }
};

using FooMap = ODhashTable<pair<pair<const Foo*, const Foo*>, string>,
			   FooPairEqual>;

static size_t fst_unit_pair_hash
(const pair<pair<const Foo*, const Foo*>, string> & p)
{
  return dft_hash_fct(p.first);
}

static size_t snd_unit_pair_hash
(const pair<pair<const Foo*, const Foo*>, string> & p)
{
  return snd_hash_fct(p.first);
}

FooMap m(5, fst_unit_pair_hash, snd_unit_pair_hash);

int main()
{
  DynArray<shared_ptr<Foo>> foos;
  for (size_t i = 0; i < 1000; ++i)
    foos.append(shared_ptr<Foo>(new Foo(to_string(i))));

  shared_ptr<Foo> f1(new Foo("Hola"));
  shared_ptr<Foo> f2(new Foo("Mundo"));

  m.insert(make_pair(make_pair(f1.get(), f2.get()), f1->a + " " + f2->a));

  for (auto it = foos.get_it(); it.has_curr(); it.next())
    {
      auto f1 = it.get_curr().get();
      for (auto it = foos.get_it(); it.has_curr(); it.next())
	m.insert(make_pair(make_pair(f1, it.get_curr().get()),
			   f1->a + " " + it.get_curr()->a));
    }

  for (auto it = foos.get_it(); it.has_curr(); it.next())
    {
      auto f1 = it.get_curr().get();
      for (auto it = foos.get_it(); it.has_curr(); it.next())
	m.remove(make_pair(make_pair(f1, it.get_curr().get()), ""));
    }

  for (auto it = m.get_it(); it.has_curr(); it.next())
    {
      auto p = it.get_curr();
      cout << p.second << endl;
    }
}
