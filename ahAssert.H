
# ifndef AHASSERT_H
# define AHASSERT_H

# include <cstddef>
# include <cstdlib>
# include <cstdio>

# include <cassert>

# ifdef DEBUG

extern void _Assert(const char*, const char*, unsigned);

extern void _Assert(const char*, const char*, unsigned, const char*, ...);

extern void _Warning(const char*, const char*, unsigned);

extern void _Warning(const char*, const char*, unsigned, const char*, ...);

# define COND_assert(cond, f, args...) assert((cond) ? (f) : true)

# define DEBUGINFO(p) (p)

# define PRINT_TRACE(p) (p)


# else /* DEBUG */

# define COND_ASSERT(cond, f, args...)

# define DEBUGINFO(p) 

# define PRINT_TRACE(p) (p)


# endif /* DEBUG */

# endif /* AHASSERT_H */



