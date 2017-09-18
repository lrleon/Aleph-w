
# include <ahAlloc.H>
# include <ahAssert.H>


# ifdef DEBUG
static DebugAlloc allocator;
# else
static ObjectAlloc allocator;
# endif


GenAlloc * Aleph::objectAlloc = &allocator;


