# ifndef ARRAY_UTILS_H
# define ARRAY_UTILS_H

# include <cstdlib>
# include <cassert>

namespace Aleph
{

template <typename T>
void open_gap(T * ptr, size_t n, size_t pos = 0, size_t num_entries = 1)
{
  assert(ptr);
  assert(num_entries > 0);

  for (size_t i = n - 1; i > pos; --i)
    ptr[i] = ptr[i - num_entries];
}

template <typename T>
void close_gap(T * ptr, size_t n, size_t pos, size_t num_entries = 1)
{
  assert(ptr);
  assert(num_entries > 0);
  for (size_t i = pos; i < n - num_entries; ++i)
    ptr[i] = ptr[i + num_entries];
}
} // namespace Aleph

# endif
