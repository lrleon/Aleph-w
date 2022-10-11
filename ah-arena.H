# ifndef AH_ARENA_H
# define AH_ARENA_H

# include <aleph.H>

using namespace std;

namespace Aleph
{

  class AhArenaAllocator
  {
    const char * _base_addr = nullptr;
    const char * _curr_addr = nullptr;
    const char * _end_addr = nullptr;
    bool _was_allocated = false;
    mutex _m;

  public:

    enum class TemplateType { TEMPLATE };

    AhArenaAllocator(const char * base_addr, const size_t sz)
      : _base_addr(base_addr),
        _curr_addr(base_addr),
        _end_addr(base_addr + sz)
    {
      // empty
    }

    void * alloc(const size_t sz)
    {
      if (sz == 0)
        return nullptr;

      const char * ret_addr = _curr_addr;
      if (ret_addr + sz >= _end_addr)
        return nullptr;
      
      _curr_addr += sz;

      return (void*) ret_addr;
    }

    void dealloc(void * addr, size_t sz = 0)
    {
      if (sz == 0)
        return;

      if (_curr_addr - sz == addr)
        _curr_addr -= sz;
    }

    virtual ~AhArenaAllocator()
    {
      if (_was_allocated and _base_addr == nullptr)
        free((void*) _base_addr);
    }

    size_t allocated_size() const noexcept
    {
      return _curr_addr - _base_addr;
    }

    size_t available_size() const noexcept
    {
      return _end_addr - _curr_addr;
    }

    template <typename T, typename ...Args>
    T * alloc(TemplateType, Args... args)
    {
      T * ptr = (T*) alloc(sizeof(T));
      if (ptr == nullptr)
        return nullptr;
      new (ptr) T(args...);
      return ptr;
    }

    template <typename T>
    void dealloc(TemplateType, void * ptr)
    {
      T * obj_ptr = (T*) ptr;
      obj_ptr->~T();
      this->dealloc(ptr, sizeof(T));
    }

    const void * base_addr() const noexcept
    {
      return _base_addr;
    }

    const void * next_avail_addr() const noexcept
    {
      return _curr_addr;
    }

    const void * end_addr() const noexcept
    {
      return _end_addr;
    }
  };

  template <class T, typename ...Args>
  T* allocate(AhArenaAllocator & arena, Args... args)
  {
    return arena.alloc<T>(AhArenaAllocator::TemplateType::TEMPLATE, args...);
  }

  template <class T>
  void dealloc(AhArenaAllocator & arena, T * ptr)
  {
    arena.dealloc<T>(AhArenaAllocator::TemplateType::TEMPLATE, ptr);
  }
}

# endif
