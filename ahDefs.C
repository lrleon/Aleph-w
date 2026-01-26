
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


# include <ahDefs.H>
# include <pthread.h>
# include <useMutex.H>

# define STR_LOG_LEN 256

namespace Aleph
{
  bool daemonized = false;

  const IPv4_Address Null_IPv4_Address = 0;

  bool verbose = false;

  double max_time = 300; // seconds
}

/* void Aleph::syslog(const char * format, va_list ap) */
/* { */
/*   int str_log_len = STR_LOG_LEN; */

/*  repeat: */
/*   //TODO: apartar el arreglo con obstack */
/*   char log_str[str_log_len + 1]; */
/*   int written_bytes = vsnprintf(log_str, str_log_len, format, ap); */
/*   if (written_bytes < str_log_len && written_bytes > -1) */
/*     { */
/*       ::syslog(LOG_NOTICE, "%s", log_str); */
/*       return; */
/*     } */

/*   str_log_len = written_bytes; */
  
/*   goto repeat; */
/* } */


# ifdef _PTHREAD_H
static pthread_mutex_t aleph_message_mutex = PTHREAD_MUTEX_INITIALIZER; 
# endif

# ifdef MESSAGES
void Aleph::message(const char* file, int line, const char* format, ...)
{
# ifdef _PTHREAD_H
  CRITICAL_SECTION(aleph_message_mutex);
# endif

  va_list ap;
  
  va_start(ap, format);

  /* if (Aleph::daemonized) */
  /*   { */
  /*     ::syslog(LOG_NOTICE, "%s: %u\n", file, line);  */
  /*     Aleph::syslog(format, ap); */
  /*   } */
  /* else */
  /*   { */
      printf("%s:%u\n", file, line);
      vfprintf(stdout, format, ap);
      printf("\n");
    /* } */
  va_end(ap);
}
# else

void Aleph::message(const char*, int, const char*, ...) { }

# endif


void Aleph::error(const char* file, int line, const char* format, ...)
{
# ifdef _PTHREAD_H
  CRITICAL_SECTION(aleph_message_mutex);
# endif 

  va_list ap;
  
  va_start(ap, format);

  /* if (Aleph::daemonized)   */
  /*   { */
  /*     ::syslog(LOG_ERR, "Fatal error detected at %s %u\n", file, line); */
  /*     Aleph::syslog(format, ap); */
  /*   } */
  /* else */
  /*   { */
      printf("Fatal error detected in %s %u\n", file, line); 
      vfprintf(stdout, format, ap);
      printf("\n");
    /* } */

  va_end(ap);

  abort();
}

void Aleph::exit(const char* file, int line, const char* format, ...) 
{
# ifdef _PTHREAD_H
  CRITICAL_SECTION(aleph_message_mutex);
# endif

  va_list ap;
  
  va_start(ap, format);

  /* if (Aleph::daemonized)   */
  /*   { */
  /*     ::syslog(LOG_NOTICE, "%s: %u\n", file, line);  */
  /*     Aleph::syslog(format, ap); */
  /*   } */
  /* else */
  /*   { */
      vfprintf(stdout, format, ap);
      printf("%s:%u\n", file, line);
      printf("\n");
    /* } */
  
  va_end(ap);

  ::exit(0);
}


void Aleph::warning(const char* file, int line, const char* format, ...)
{
# ifdef _PTHREAD_H
  CRITICAL_SECTION(aleph_message_mutex);
# endif

  va_list ap;
  
  va_start(ap, format);
  
  /* if (daemonized)   */
  /*   { */
  /*     ::syslog(LOG_WARNING, "Warning detected in %s %u\n", file, line);  */
  /*     Aleph::syslog(format, ap); */
  /*   } */
  /* else */
  /*   { */
      printf("Warning detected in %s %u\n", file, line); 
      vfprintf(stdout, format, ap);
      printf("\n");
    /* } */
  
  va_end(ap);
}
