# ifndef AH_ARENA_H
# define AH_ARENA_H

# include <aleph.H>

class AhArenaAllocator
{
  const char * base_addr = nullptr;
  const char * curr_addr = nullptr;
  const char * end_addr = nullptr;
  size_t chunk_size = 0;
  bool to_reallocate = false;

public:

  AhArenaAllocator(const char * base_addr, const size_t sz)
    : base_addr(base_addr), curr_addr(base_addr), chunk_size(sz) {}

  void * alloc(const size_t sz)
  {
    const char * ret_addr = curr_addr;
    
  }
    
};

# endif
