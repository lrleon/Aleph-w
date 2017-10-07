
# include <htlist.H>
# include <filter_iterator.H>

struct Filt
{
  bool operator () (long i) const { return i > 10 and i < 50; }
};

int main(int argc, char *argv[])
{
  DynList<long> l = range(1000l);

  auto F = [] (long i) { return i < 100; };

  for (Filter_Iterator<DynList<long>, DynList<long>::Iterator, Filt> it(l); 
       it.has_curr(); it.next())
    ;

}

