

# include <tclap/CmdLine.h>

# include <tpl_dynMapOhash.H>

using namespace std;
using namespace Aleph;
using namespace TCLAP;

CmdLine cmd = { "hash_resize", ' ', "0" };

ValueArg<size_t> n =
  { "n", "n", "num of keys", false, 1000, "num of keys", cmd };

struct Foo
{
  string val;
  int i;

  bool operator == (const Foo & foo) const
  {
    return i == foo.i and val == foo.val;
  }
};

MapOLhash<int, Foo> tbl;
DynArray<Foo> backup;

void fill(size_t n)
{
  for (size_t i = 0; i < n; ++i)
    {
      cout << "i = " << i << endl;
      Foo foo;
      foo.i = i;
      foo.val = to_string(i);
      auto ptr = tbl.insert(i, move(foo));
      assert(ptr != nullptr);
      backup.append(move(foo));
    }
}

void verify()
{
  for (auto it = backup.get_it(); it.has_curr(); it.next())
    {
      const Foo & foo = it.get_curr();
      auto ptr = tbl.search(foo.i);
      assert(ptr != nullptr);
      assert(ptr->first == foo.i);
    }
}

int main(int argc, char **argv)
{
  cmd.parse(argc, argv);

  fill(n);
  verify();
}
