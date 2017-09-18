
# include <gsl/gsl_rng.h> 
# include <ahNow.H>
# include <ahSort.H>
# include <tpl_dynArray.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynSetTree.H>
# include <tpl_dynSetHash.H>

unsigned long ctor_move_count = 0;
unsigned long ctor_copy_count = 0;
unsigned long ass_copy_count = 0; // I write ass on purpose ;-)
unsigned long ass_move_count = 0;


void reset_counters()
{
  ctor_move_count = ctor_copy_count = ass_copy_count = ass_move_count = 0;
}

void print_counters()
{
  cout << "ctor_move_count = " << ctor_move_count << endl
       << "ctor_copy_count = " << ctor_copy_count << endl
       << "ass_copy_count  = " << ass_copy_count << endl
       << "ass_move_count  = " << ass_move_count << endl;
}

template <class C>
void print_pairs(const C & c)
{
  cout << "Pairs = ";
  c.for_each([] (auto p) { cout << "[" << p.first << "," << p.second << "]"; });
  cout << endl;
}

struct Foo
{
  unsigned long k = -1;
  string d = "";

  Foo() {}

  Foo(unsigned long __k) : k(__k), d(to_string(k)) {  }

  Foo(const Foo & f) : k(f.k), d(f.d) { ctor_copy_count++; }

  Foo(Foo && f) : k(f.k), d(move(f.d)) { ctor_move_count++; }

  Foo & operator = (const Foo & f)
  {
    k = f.k;
    d = f.d;
    ass_copy_count++;
    return *this;
  }

  Foo & operator = (Foo && f)
  {
    std::swap(k, f.k);
    std::swap(d, f.d);
    ass_move_count++;
    return *this;
  }

  bool operator == (const Foo & f) const { return k == f.k; }

  bool operator < (const Foo & f) const { return k < f.k; }

  friend ostream & operator << (ostream & s, const Foo & f) 
  {
    s << "(" << f.k << "," << f.d << ")";
    return s;
  }
};

size_t hash_foo_pair(const std::pair<Foo, Foo> & p) 
{
  return dft_hash_fct(p.first.k); 
}

size_t hash_foo_pair2(const std::pair<Foo, Foo> & p) 
{
  return snd_hash_fct(p.first.k); 
}

template <class C> 
void test_map(C & table, size_t n, unsigned long seed)
{
  cout << "================================================================"
       << endl
       << endl
       << typeid(table).name() << endl
       << endl;
  reset_counters();
  auto r = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));
  DynArray<std::pair<Foo, Foo>> pairs;

  cout << "Testing simple copy search and insertion" << endl;

  Now now;
  
  auto tp = now.start();
  for (auto i = 0; i < n; ++i)
    {
      Foo k = gsl_rng_get(r);
      Foo d = gsl_rng_get(r);
      if (table.search(k))
	continue;
      assert(table.insert(k, d));
      pairs.append(make_pair(k, d));
    }
  auto t = now.delta(tp);
  cout << "done in " << t << " ms" << endl;

  print_counters();
  reset_counters();

  cout << "Testing Iterator" << endl
       << endl;
  for (auto it = table.get_itor(); it.has_curr(); it.next())
    {
      auto p = it.get_curr();
      auto ptr = table.search(p.first);
      assert(ptr and ptr->first == p.first and ptr->second == p.second);
    }
  cout << "done!" << endl;

  cout << endl
       << endl
       << "Testing copy search" << endl;
  assert(pairs.all([&table] (auto p) 
		   {
		    auto ptr = table.search(p.first);
		    return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   }));
  print_counters();
  reset_counters();
  cout << "Done!" << endl
       << endl
       << endl
       << "Testing remove" << endl
       << endl;
  pairs.for_each([&table] (const auto p) { table.remove(p.first); });
  assert(table.is_empty());
  cout << "done! " << endl
       << endl
       << "Testing copy/move insert" << endl
       << endl;
  pairs.for_each([&table] (auto p)
		 {
		   auto c = p;
		   assert(table.insert(p.first, move(p.second)));
		 });
  assert(table.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   }));
  print_counters();
  reset_counters();
  table.empty();
  
  cout << "done" << endl
       << endl
       << endl
       << "Testing move/move insert" << endl;
  pairs.all([&table] (auto p) 
		   {
		     auto ptr = table.insert(move(p.first), move(p.second));
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   });
  assert(table.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   }));
  cout << "Done!" << endl
       << endl;
  print_counters();
  reset_counters();
  table.empty();
  
  cout << endl
       << "Testing move/copy insert" << endl;
  pairs.all([&table] (auto p) 
		   {
		     auto ptr = table.insert(move(p.first), p.second);
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   });
  assert(table.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   }));
  cout << "Done!" << endl
       << endl;
  print_counters();
  reset_counters();
  table.empty();
  
  cout << endl
       << "Testing copy/move insert" << endl;
  pairs.all([&table] (auto p) 
		   {
		     auto ptr = table.insert(p.first, move(p.second));
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   });
  assert(table.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   }));
  cout << "Done!" << endl
       << endl;
  print_counters();
  reset_counters();
  assert(table.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and 
		       ptr->second == p.second;
		   }));
  {
    auto c = pairs;
    reset_counters();
    cout << "Testing move search" << endl
	 << endl;

    print_counters();
    reset_counters();
  }

  {
    assert(table.keys().all([&table] (auto k) { return table.has(k); }));
    assert(table.values_ptr().all([&table] (auto ptr)
			   {
			     auto k = table.get_key(ptr);
			     return table.contains(k);
			   }));
    assert(table.items().all([&table] (auto p) 
      {
	auto ptr = table.search(p.first);
	return ptr and ptr->first == p.first and ptr->second == p.second;
      }));
    assert(table.items_ptr().all([&table] (auto p)
      {
	auto ptr = table.search(p->first);
	return ptr and ptr->first == p->first and ptr->second == p->second;
      }));

    auto vals = 
      table.values_ptr().template maps<Foo>([] (auto p) { return *p; });
    assert(vals.size() == table.size());
    assert(eq(sort(table.values()), sort(vals)));
  }

  table.empty();
  reset_counters();
  cout << endl
       << "Testing copy base insertion" << endl;
  pairs.for_each([&table] (const auto & p) { table.insert(p); });
  cout << "done!" << endl
       << endl;
  print_counters();
  assert(pairs.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and
		       ptr->second == p.second;
		   }));

  table.empty();
  reset_counters();
  cout << "Testing move base insertion" << endl;
  pairs.for_each([&table] (auto p) { table.insert(move(p)); });
  cout << "done!" << endl
       << endl;
  print_counters();
  assert(pairs.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and
		       ptr->second == p.second;
		   }));

  table.empty();
  reset_counters();
  cout << "Testing operator insertion via copy [] operator" << endl;
  pairs.for_each([&table] (auto & p) { table[p.first] = p.second; });
  cout << "done!" << endl
       << endl;
  print_counters();
  assert(pairs.all([&table] (auto p) 
		   {
		     auto ptr = table.search(p.first);
		     return ptr and ptr->first == p.first and
		       ptr->second == p.second;
		   }));
  cout << "done!" << endl
       << endl;
  print_counters();
  reset_counters();
  assert(pairs.all([&table] (const auto & p) 
		   { 
		     return table[p.first] == p.second;
		   }));
  cout << "done!" << endl
       << endl;
  print_counters();

  reset_counters();
  cout << endl
       << "Testing operator insertion via move [] operator" << endl;
  pairs.for_each([&table] (auto p) { table[move(p.first)] = move(p.second); });
  cout << "done!" << endl
       << endl;  
  print_counters();
  assert(pairs.all([&table] (const auto & p) 
		   { 
		     return table[p.first] == p.second;
		   }));

  gsl_rng_free(r);

  cout << endl
       << endl
       << typeid(table).name() << endl
       << endl;
}

// template <template <typename K, typename D, 
// 		    template <typename, class> class Tree,
// 		    class Cmp> class MapTree>
template <template <typename, class> class Tree>
void test_map_tree(size_t n, unsigned long seed)
{
  cout << "Testing map tree" << endl
       << endl;
  DynMapTree<Foo, Foo, Tree, std::less<Foo>> table;
  test_map(table, n, seed);
}

void test_map_olhash(size_t n, unsigned long seed)
{
  cout << "Testing map OLhash" << endl
       << endl;
  MapOLhash<Foo, Foo> table(n, &hash_foo_pair);
  test_map(table, n, seed);
}

void test_map_odhash(size_t n, unsigned long seed)
{
  cout << "Testing map ODhash" << endl
       << endl;
  MapODhash<Foo, Foo> table(n, hash_foo_pair, hash_foo_pair2);
  test_map(table, n, seed);
}

template <template <typename K, typename D, class Cmp> class MapTable>
void test_map_hash(size_t n, unsigned long seed)
{
  cout << "Testing map ODhash" << endl
       << endl;

  MapTable<Foo, Foo, Dft_Pair_Cmp<Foo, Foo, std::equal_to<Foo>>> table((size_t)n);

  // MapTable<Foo, Foo, Dft_Pair_Cmp<Foo, Foo, std::equal_to<Foo>>> table;
  test_map(table, n, seed);

  const auto c = table;
}

void usage()
{
  cout << "./test-map n [seed]" << endl
       << endl;
  exit(0);
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    usage();

  size_t n = atoi(argv[1]);
  unsigned long seed = argc > 2 ? atoi(argv[2]) : time(0);

  // test_map_tree(n, seed);

  test_map_olhash(n, seed);

  test_map_odhash(n, seed);

  test_map_hash<DynMapHash>(n, seed);

  test_map_hash<DynMapLinHash>(n, seed);

  test_map_tree<BinTree>(n, seed);
}
