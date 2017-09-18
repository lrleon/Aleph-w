# ifndef __AH_STDC_UTILS__
# define __AH_STDC_UTILS__

# include <exception>
# include <stdexcept>

namespace Aleph
{

    template <typename Container, typename Iterator> inline
void verify_container_and_iterator(const Container & container, 
				   const Iterator & itor)
{
  if (not itor.verify(container))
    throw std::domain_error("Iterator is not set to same container");
}


    template <typename Iterator> inline
void verify_iterators(const Iterator & itor1, const Iterator & itor2)
{
  if (not itor1.verify(itor2))
    throw std::domain_error("Iterator is not set to same container");
}

    template <typename Container, typename Iterator> inline void 
verify_container_and_iterators
 (const Container & container, const Iterator & itor_container, 
  const Iterator & itor1, const Iterator & itor2)
{
  verify_container_and_iterator(container, itor_container);
  verify_container_and_iterator(itor1, itor2);

  if (itor_container.verify(itor1))
    throw std::domain_error("All iterators reference the same object");
}

    template <typename Container, typename Iterator> inline void 
verify_container_and_iterators
 (const Container & container, const Iterator & itor1, const Iterator & itor2)
{
  verify_container_and_iterator(container, itor1);
  verify_container_and_iterator(container, itor2);
}


}

# endif // __AH_STDC_UTILS__
