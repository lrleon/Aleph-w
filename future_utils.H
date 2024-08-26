# ifndef FUTURE_UTILS_H
# define FUTURE_UTILS_H

# include <future>
# include <tpl_dynList.H>

template <typename T> inline DynList<T>
get_futures(DynList<std::future<T>> & future_list)
{
  DynList<T> ret;
  while (not future_list.is_empty())
    ret.append(future_list.remove_first().get());

  return ret;
}

template <typename T> inline DynList<T>
get_futures(DynList<std::future<T>> && future_list)
{
  return get_futures(static_cast<DynList<std::future<T>>&>(future_list));
}

inline void get_futures(DynList<std::future<void>> & future_list)
{
  while (not future_list.is_empty())
    future_list.remove_first().get();
}

inline void get_futures(DynList<std::future<void>> && future_list)
{
  DynList<std::future<void>> & futures = future_list;
  get_futures(futures);
}

# endif
