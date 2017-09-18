#ifndef AH_DISPATCHER_H
#define AH_DISPATCHER_H

# include <iostream>
# include <tpl_dynMapTree.H>
# include <tpl_odhash.H>

template <typename Key, class Operation>
class AHDispatcher
{
  DynMapTree<Key, Operation> tbl;

public:

  AHDispatcher() {}

  template <typename ... Pairs>
  AHDispatcher(const Key & key, Operation op, Pairs ... pairs)
    : AHDispatcher(pairs...)
  {
    tbl.insert(key, op);
  }  

  void insert(const Key & key, Operation op)
  {
    tbl.insert(key, op);
  }

  template <typename ... Args>
  auto run(const Key & key, Args ... args) const
  {
    auto ptr = tbl.search(key);
    if (ptr == nullptr)
      {
	cerr << "Tried to run dispatcher with a key" 
	     << " which has not been registered between" << endl;
	abort();
      }
    return (*ptr->second)(args...);
  }

  void remove(const Key & key)
  {
    tbl.remove(key);
  }

  bool valid_key(const Key & key) { return tbl.has(key); }
};


template <typename Key, class Operation>
class AhHashDispatcher
{
  using P = pair<Key, Operation>;

  struct Equal
  {
    bool operator () (const P & p1, const P & p2) const
    {
      return p1.first == p2.first;
    }
  };

  static size_t fst_hash(const P & p) { return dft_hash_fct(p.first); }
  static size_t snd_hash(const P & p) { return snd_hash_fct(p.first); }

  ODhashTable<P, Equal> tbl;

public:

  void insert(const Key & key, Operation op)
  {
    tbl.insert(P(key, op));
  }

  template <typename ... Args>
  auto run(const Key & key, Args ... args) const
  {
    P p; p.first = key;
    return tbl.find(p).second (args...);
  }

  void remove(const Key & key)
  {
    tbl.remove(key);
  }
};




#endif
