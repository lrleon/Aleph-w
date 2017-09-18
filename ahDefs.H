# ifndef AHDEFS_H
# define AHDEFS_H

# include <unistd.h>
# include <time.h>
# include <stdint.h>
# include <cstdlib>
# include <cstdio>
# include <cstdarg>

# include <stdexcept>
# include <utility>

enum EmptyCtor { emptyCtor };

enum SentinelCtor { sentinelCtor };

namespace Aleph
{
  typedef uint32_t IPv4_Address; 

  extern const IPv4_Address Null_IPv4_Address;

  struct Empty_Class 
  { 
    Empty_Class() { /* empty */ }

    Empty_Class(EmptyCtor) { /* empty */ }

    Empty_Class(SentinelCtor) { /* empty */ }

    Empty_Class(const Empty_Class &) {}

    bool operator == (const Empty_Class &) const
    {
      return true;
    }

    bool operator != (const Empty_Class &) const
    {
      return false;
    }
  };

  const int UnknownSize = -1;
  const int UnknownLine = -1;
  const int UnknownLoad = -1;

  extern bool daemonized;

  extern bool verbose;

  extern double max_time;

  extern void message(const char * file, int line, const char *format, ...);

  extern void error(const char * file, int line, const char *format, ...);

  extern void exit(const char * file, int line, const char *format, ...);

  extern void warning(const char * file, int line, const char *format, ...);

  extern void syslog(const char * format, va_list ap);
}

# ifdef MESSAGES

# define MESSAGE(format, args...) \
         Aleph::message(__FILE__, __LINE__, format , ##args)
# define WARNING(format, args...) \
         Aleph::warning(__FILE__, __LINE__, format , ##args)

# else

# define MESSAGE(format, args...)
# define WARNING(format, args...) 

# endif

# define AH_ERROR(format, args...) \
         Aleph::error(__FILE__, __LINE__, format , ##args)

# define EXIT(format, args...) \
 	 Aleph::exit(__FILE__, __LINE__, format , ##args)

# define POINTER_HAS_BEEN_FOUND(ptr) ((ptr) not_eq nullptr)
# define POINTER_HAS_NOT_BEEN_FOUND(ptr) ((ptr) == nullptr)

# define Exception_Prototypes(list...) throw(std::exception, ##list)

#endif // AHDEFS_H




