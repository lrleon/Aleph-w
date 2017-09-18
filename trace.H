
# ifndef TRACE_H
# define TRACE_H

# include <stdio.h>


class Trace
{
  char * file;
  int line;

  char * name;

public:

  Trace(char * _file, int _line, char * _name) :
    file(_file), line(_line), name(_name)
  {
    printf("****ENTER %s in %s:%d\n", name, file, line);
  }

  ~Trace()
  {
    printf("****LEAVE %s\n", name);
  }
};

# define TRACE(name) Trace __trace__##__LINE__(__FILE__, __LINE__, name)

# endif // TRACE_H
