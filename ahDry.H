
# ifndef AHDRY_H
# define AHDRY_H

# include <tuple>
# include <functional>
# include <sstream>
# include <initializer_list>

# include <ahFunctional.H>

namespace Aleph 
{

  template <typename T> class DynList;
  
# define Generic_Traverse(Type) \
  template <class Operation>						\
  bool traverse(Operation & operation) const noexcept(noexcept(operation)) \
  {									\
    for (Iterator it(*this); it.has_curr(); it.next())			\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
  									\
  template <class Operation>						\
  bool traverse(Operation & operation) noexcept(noexcept(operation))	\
  {									\
    for (Iterator it(*this); it.has_curr(); it.next())			\
      if (not operation(it.get_curr()))					\
	return false;							\
    return true;							\
  }									\
									\
  template <class Operation>						\
  bool traverse(Operation && operation = Operation()) const		\
    noexcept(noexcept(operation))					\
  {									\
    return traverse<Operation>(operation);				\
  }									\
  									\
  template <class Operation>						\
  bool traverse(Operation && operation = Operation())			\
    noexcept(noexcept(operation))					\
  {									\
    return traverse<Operation>(operation);				\
  }

# define Special_Ctors(Set_Type, Type)					\
  template <template <typename> class List>				\
  Set_Type(const List<Type> & l) : Set_Type()				\
  {									\
    l.for_each([this] (const Type & item) { this->append(item); });	\
  }									\
									\
  template <class It>							\
  Set_Type(It b, It e) : Set_Type()					\
  {									\
    for (It it = b; it != e; ++it)					\
      this->append(*it);						\
  }									\
									\
  Set_Type(std::initializer_list<Type> l) : Set_Type()			\
  {									\
    for (const auto & item : l)						\
      this->append(item);						\
  }


# define Locate_Functions(Type) \
  Type & nth(const size_t n) const					\
  {									\
    Type * ptr = nullptr;						\
    size_t i = 0;							\
    this->traverse([&ptr, &i, &n] (Type & item)				\
		   {							\
		     if (i++ < n)					\
		       return true;					\
		     ptr = &item;					\
		     return false;					\
		   });							\
									\
    if (i != n + 1)							\
      throw std::out_of_range("index out of range");			\
    									\
    return *ptr;							\
  }									\
  									\
  template <class Operation>						\
  Type * find_ptr(Operation & operation) noexcept(noexcept(operation)) \
  {									\
    Type * ptr = nullptr;						\
    this->traverse([&ptr,&operation] (Type & item)			\
		   {							\
		     if (operation(item))				\
		       {						\
			 ptr = &item;					\
			 return false;					\
		       }						\
		     return true;					\
		   });							\
    return ptr;								\
  }									\
									\
  template <class Operation>						\
  Type * find_ptr(Operation & operation) const noexcept(noexcept(operation)) \
  {									\
    Type * ptr = nullptr;						\
    this->traverse([&ptr, &operation] (Type & item)			\
		   {							\
		     if (operation(item))				\
		       {						\
			 ptr = &item;					\
			 return false;					\
		       }						\
		     return true;					\
		   });							\
    return ptr;								\
  }									\
									\
  template <class Operation>						\
  Type * find_ptr(Operation && operation = Operation()) const		\
    noexcept(noexcept(operation))					\
  {									\
    return find_ptr<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  Type * find_ptr(Operation && operation = Operation())		\
    noexcept(noexcept(operation))					\
  {									\
    return find_ptr<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  std::tuple<bool, Type> find_item(Operation & operation)		\
    noexcept(noexcept(operation))					\
  {									\
    auto ptr = find_ptr(operation);					\
    return ptr ? std::make_tuple(true, *ptr) : std::make_tuple(false, Type()); \
  }									\
									\
  template <class Operation>						\
  std::tuple<bool, Type> find_item(Operation & operation) const \
    noexcept(noexcept(operation))					\
  {									\
    auto ptr = find_ptr(operation);					\
    return ptr ? std::make_tuple(true, *ptr) : std::make_tuple(false, Type()); \
  }									\
									\
  template <class Operation>						\
  std::tuple<bool, Type> find_item(Operation && operation = Operation()) \
    noexcept(noexcept(operation))					\
  {									\
    return find_item(operation);					\
  }									\
									\
  template <class Operation>						\
  std::tuple<bool, Type> find_item(Operation && operation = Operation()) const \
    noexcept(noexcept(operation))					\
  {									\
    return find_item(operation);					\
  }

# define Functional_Methods(Type)					\
  template <class Operation>						\
  auto for_each(Operation & operation) const				\
    noexcept(noexcept(operation))-> decltype(*this)			\
  {									\
    this->traverse([&operation] (const Type & item)			\
		   {							\
		     operation(item);					\
		     return true;					\
		   });							\
    return *this;							\
  }									\
									\
  template <class Operation>						\
  auto for_each(Operation & operation) noexcept(noexcept(operation))	\
    -> decltype(*this)							\
  {									\
    this->traverse([&operation] (const Type & item)			\
		   {							\
		     operation(item);					\
		     return true;					\
		   });							\
    return *this;							\
  }									\
									\
  template <class Operation>						\
  auto for_each(Operation && operation = Operation()) const		\
    noexcept(noexcept(operation)) -> decltype(*this)			\
  {									\
    return this->for_each<Operation>(operation);			\
  }									\
									\
  template <class Operation>						\
  auto for_each(Operation && operation = Operation())			\
    noexcept(noexcept(operation)) -> decltype(*this)			\
  {									\
    return this->for_each<Operation>(operation);			\
  }									\
  									\
  template <class Operation>						\
  auto mutable_for_each(Operation & operation)				\
    noexcept(noexcept(operation)) -> decltype(*this)			\
  {									\
    this->traverse([&operation] (Type & item)				\
		   {							\
		     operation(item);					\
		     return true;					\
		   });							\
    return *this;							\
  }									\
									\
  template <class Operation>						\
  auto mutable_for_each(Operation && operation = Operation())		\
    noexcept(noexcept(operation)) -> decltype(*this)			\
  {									\
    return this->mutable_for_each<Operation>(operation);		\
  }									\
									\
  template <class Operation>						\
  bool all(Operation & operation) const					\
    noexcept(noexcept(operation))					\
  {									\
    return this->template traverse<Operation>(operation);		\
  }									\
									\
  template <class Operation>						\
  bool all(Operation && operation = Operation()) const			\
    noexcept(noexcept(operation))					\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall(Operation & operation) const				\
    noexcept(noexcept(operation))					\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool forall(Operation && operation = Operation()) const		\
    noexcept(noexcept(operation))					\
  {									\
    return all<Operation>(operation);					\
  }									\
									\
  template <class Operation>						\
  bool exists(Operation & operation) const				\
    noexcept(noexcept(operation))					\
  {									\
    return not this->traverse([&operation] (const Type & item)		\
			      {						\
				return not operation(item);		\
			      });					\
  }									\
									\
  template <class Operation>						\
  bool exists(Operation && operation = Operation()) const		\
    noexcept(noexcept(operation))					\
  {									\
    return exists<Operation>(operation);				\
  }									\
									\
  template <typename __Type                        = Type,			\
	    template <typename> class Container = Aleph::DynList,	\
	    class Operation = Dft_Map_Op<Type, __Type>>			\
  Container<__Type> maps(Operation & operation) const			\
  {									\
    Container<__Type> ret_val;						\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     ret_val.append(operation(item));			\
		   });							\
    return ret_val;							\
  }									\
									\
  template < typename __Type                        = Type,		\
	     template <typename> class Container = Aleph::DynList,	\
	     class Operation = Dft_Map_Op<__Type, __Type>>			\
  Container<__Type> maps(Operation && operation = Operation()) const	\
  {									\
    return maps<__Type, Container, Operation>(operation);			\
  }									\
									\
  template <typename __Type = Type>					\
  __Type foldl(const __Type & init,						\
	    std::function<__Type(const __Type&, const Type &)> operation) const \
    noexcept(noexcept(operation))					\
  {									\
    auto ret_val = init;						\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     ret_val = operation(ret_val, item);		\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  Type fold(const Type & init, Operation & operation) const		\
    noexcept(noexcept(operation))					\
  {									\
    auto ret_val = init;						\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     ret_val = operation(ret_val, item);		\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  Type fold(const Type & init, Operation && operation = Operation()) const \
    noexcept(noexcept(operation))					\
  {									\
    return fold<Operation>(init, operation);				\
  }									\
									\
  template <class Operation>						\
  DynList<Type> filter(Operation & operation) const			\
  {									\
    DynList<Type> ret_val;						\
    this->for_each([&ret_val, &operation] (const Type & item)		\
		   {							\
		     if (operation(item))				\
		       ret_val.append(item);				\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  DynList<Type> filter(Operation && operation = Operation()) const	\
  {									\
    return filter<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  DynList<std::tuple<Type, size_t>> pfilter(Operation & operation) const \
  {									\
    DynList<std::tuple<Type, size_t>> ret_val;				\
    size_t i = 0;							\
    this->for_each([&ret_val, &operation, &i] (const Type & item)	\
		   {							\
		     if (operation(item))				\
		       ret_val.append(std::make_tuple(item, i));	\
		     ++i;						\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  DynList<std::tuple<Type, size_t>>					\
  pfilter(Operation && operation = Operation()) const			\
  {									\
    return pfilter<Operation>(operation);				\
  }									\
									\
  template <class Operation>						\
  std::pair<DynList<Type>, DynList<Type>> partition(Operation & op) const \
  {									\
    std::pair<DynList<Type>, DynList<Type>> ret_val;			\
    this->for_each([&ret_val, &op] (const Type & item)			\
		   {							\
		     if (op(item))					\
		       ret_val.first.append(item);			\
		     else						\
		       ret_val.second.append(item);			\
		   });							\
    return ret_val;							\
  }									\
									\
  template <class Operation>						\
  std::pair<DynList<Type>, DynList<Type>>				\
    partition(Operation && op = Operation()) const			\
  {									\
    return partition<Operation>(op);					\
  }									\
    									\
  template <class Operation>						\
  std::tuple<DynList<Type>, DynList<Type>> tpartition(Operation & op) const \
  {									\
    DynList<Type> r1, r2;						\
    this->for_each([&r1, &r2, &op] (const Type & item)			\
		   {							\
		     if (op(item))					\
		       r1.append(item);					\
		     else						\
		       r2.append(item);					\
		   });							\
    return std::make_tuple(r1, r2);					\
  }									\
									\
  template <class Operation>						\
  std::tuple<DynList<Type>, DynList<Type>>				\
    tpartition(Operation && op = Operation()) const			\
  {									\
    return partition<Operation>(op);					\
  }									\
    									\
  size_t length() const noexcept					\
  {									\
    size_t count = 0;							\
    this->for_each([&count] (const Type &) { ++count; });		\
    return count;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Type> rev()	const						\
  {									\
    Container<Type> ret_val;						\
    for_each([&ret_val] (const Type & item)				\
	     {								\
	       ret_val.insert(item);					\
	     });							\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Type> take(const size_t n) const				\
  {									\
    size_t i = 0;							\
    Container<Type> ret;						\
    this->traverse([&i, &ret, n] (const Type & item)			\
	     {								\
	       if (i++ >= n)						\
		 return false;						\
	       ret.append(item);					\
	       return true;						\
	     });							\
    return ret;								\
  }									\
									\
  template <template <typename> class Container = Aleph::DynList>	\
  Container<Type> drop(const size_t n) const				\
  {									\
    size_t i = 0;							\
    Container<Type> ret;						\
    this->traverse([&i, &ret, n] (const Type & item)			\
	     {								\
	       if (i++ >= n)						\
		 ret.append(item);					\
	       return true;						\
	     });							\
    return ret;								\
  }

# define Generic_Keys(Type)						\
  template <template <typename> class Container = DynList>		\
  Container<Type> keys() const						\
  {									\
    return this->template maps<Type, Container>([] (const Type & key)	\
						{ return key; });	\
  }

# define Generic_Items(Type)						\
  template <template <typename> class Container = DynList>		\
  Container<Type> items() const						\
  {									\
    return this->template maps<Type, Container> ([] (const Type & key)	\
						 { return key; });	\
  }


# define Equal_To_Method(class_name)			\
  bool equal_to(const class_name & r) const noexcept	\
  {							\
    if (this == &r)					\
      return true;					\
							\
    if (this->size() != r.size())			\
      return false;					\
							\
    return this->all(/* Lambda */ [&r] (const Key & k)	\
		     { return r.search(k) != nullptr; });	\
  }							\
							\
  bool operator == (const class_name & r) const	noexcept	\
  {							\
    return equal_to(r);					\
  }							\
							\
  bool operator != (const class_name & r) const	noexcept	\
  {							\
    return not equal_to(r);				\
  }
  

# define Map_Sequences_Methods()					\
  template <template <typename> class Container = ::DynList>		\
  Container<Key> keys()	const						\
  {									\
    Container<Key> ret_val;						\
    this->for_each([&ret_val] (const std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(p.first);					\
	     });							\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<Data> values() const					\
  {									\
    Container<Data> ret_val;						\
    this->for_each([&ret_val] (const std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(p.second);				\
	     });							\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<Data*> values_ptr() 					\
  {									\
    Container<Data*> ret_val;						\
    this->for_each([&ret_val] (std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(&p.second);				\
	     });							\
    return ret_val;							\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<std::pair<Key, Data>> items() const				\
  {									\
    return this->Base::keys();						\
  }									\
									\
  template <template <typename> class Container = ::DynList>		\
  Container<std::pair<Key, Data*>> items_ptr() const			\
  {									\
    Container<std::pair<Key, Data*>> ret_val;				\
    this->for_each([&ret_val] (std::pair<Key, Data> & p)		\
	     {								\
	       ret_val.append(std::pair<Key,Data*>(p.first, p.second));	\
	     });							\
    return ret_val;							\
  }									\
									\
  Data & operator () (const Key & key)					\
  {									\
    return this->find(key);						\
  }									\
									\
  const Data & operator () (const Key & key) const			\
  {									\
    return this->find(key);						\
  }									\

# define Generate_Proxy_Operator(Class_Name)				\
  const Data & operator [] (const Key & key) const			\
  {									\
    return find(key);							\
  }									\
									\
  Data & operator [] (const Key & key)					\
  {									\
    return find(key);							\
  }
  
  template <typename Type> inline 
std::string to_str(const Type & d)  
{
  std::ostringstream os;
  os << d;
  return os.str();
}


  // This class wrappes the compare class passed to DynMapLinHash which
  // has form cpm<Key>
  template <typename Key, typename Data, class Cmp = std::equal_to<Key>>
	struct Dft_Pair_Cmp 
	{
	  bool operator () (const std::pair<Key, Data> & p1, 
			    const std::pair<Key, Data> & p2) const
	  {
	    return Cmp () (p1.first, p2.first);
	  }
	};

  template <typename Key, typename Data>
  std::pair<Key, Data> * key_to_pair(Key * ptr)
  {
    return (std::pair<Key, Data>*) ptr; 
  }

  template <typename Key, typename Data>
  std::pair<Key, Data> * data_to_pair(Data * ptr)
  {
    std::pair<Key, Data> * zero = 0;
    return (std::pair<Key, Data>*) ((long) ptr - (long) &zero->second); 
  }


} // end namespace Aleph 

# endif // AHDRY_H
