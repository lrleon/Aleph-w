# ifndef AL_DOMAIN_H
# define AL_DOMAIN_H

# include <tpl_hash.H>

namespace Aleph
{

  template <typename T = int>
struct AlDomain : public Aleph::HashSet<T, SetODhash>
{
  using Base = Aleph::HashSet<T, SetODhash>;
  using Base::Base;

  template <template <typename> class Container = DynList>
  Container<T> to_list() const
  {
    Container<T> ret_val;
    this->for_each(/* Lambda */ [&ret_val] (const T & item)
	     {
	       ret_val.append(item);
	     });
    return ret_val;    
  }

  std::string to_str() const
  {
    return sort(to_list()).template foldl<std::string> 
      ("", /* Lambda */ [] (const std::string & s, const T & item)
       {
	 return s + " " + Aleph::to_str(item);
       });
  }
};

template <typename T> inline
ostream & operator << (ostream & s, const AlDomain<T> & dom)
{
  return s << dom.to_str();
}

struct IntRange : public AlDomain<int>
{
  IntRange() = delete; 

  IntRange(int start, int end,  int step = 1)
  {
    if (step < 0)
      throw std::domain_error("negative step");

    cout << "step = " << step << endl;
    for (long i = start; i <= end; i += step)
      insert(i);
  }

  IntRange(size_t n)
  {
    for (int i = 0; i < n; ++i)
      insert(i);
  }

};

} // end namespace Aleph

# endif // AL_DOMAIN_H
