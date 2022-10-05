# ifndef AH_ARENA_H
# define AH_ARENA_H

# include <mutex>

# include <aleph.H>

using namespace std;

namespace Aleph
{

class AhArenaAllocator
{
  const char * base_addr = nullptr;
  const char * curr_addr = nullptr;
  const char * end_addr = nullptr;
  bool to_reallocate = false;
  mutex m;

public:

  AhArenaAllocator(const char * base_addr, const size_t sz)
    : base_addr(base_addr),
      curr_addr(base_addr),
      end_addr(base_addr + sz)
  {
    // empty
  }

  void * alloc(const size_t sz)
  {
    lock_guard<mutex> lock(m);
    const char * ret_addr = curr_addr + sz;
    if (ret_addr >= end_addr)
      return nullptr;

    curr_addr += sz;

    return (void*) ret_addr;
  }

  void free(void * addr, size_t sz = 0)
  {
    if (sz == 0)
      return;

    lock_guard<mutex> lock(m);
    if (curr_addr - sz == addr)
      curr_addr -= sz;

    return;
  }
    
};

}

# endif
