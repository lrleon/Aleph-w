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
