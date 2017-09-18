
# ifndef AH_CONVERT_H
# define AH_CONVERT_H 1

# include <tpl_agraph.H>
# include <tpl_dynArray.H>

template <class T>
DynList<T> to_DynList(const Array<T> & a)
{
  return a.template maps<T>([] (const auto & item) { return item; });
}

template <class C>
Array<typename C::Item_Type> to_Array(const C & c)
{
  Array<typename C::Item_Type> ret;
  for (auto it = c.get_it(); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}

# endif // AH_CONVERT_H


