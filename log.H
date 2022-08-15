
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

# ifndef LOG_H
# define LOG_H

# include <sys/utsname.h>
# include <errno.h>
# include <stdio.h>
# include <stdarg.h>
# include <string.h>
# include <useMutex.H>

class Log_Manager
{
  static const short int Max_Log_Line_Length  = 512;
  static const short int Max_Time_Line_Length = 61;

  FILE * file_descriptor;

  pthread_mutex_t mutex;

  char print_buffer[Max_Log_Line_Length];

  char time_buffer[Max_Time_Line_Length];

  size_t num_lines;

  size_t max_num_lines;

  void get_time()
  {
        /* classic unix time (seconds from january 1, 1970) */
    time_t unix_time;
    time(&unix_time); /* get seconds from anuary 1, 1970) */
      
          /* libc broken time */
    struct tm broken_time;
        /* convert unix time to broken time */
    localtime_r(&unix_time, &broken_time); 

    strftime(time_buffer, Max_Time_Line_Length, "%a %b %d %G %r", 
	     &broken_time);
  }

public:

  Log_Manager(FILE * _file_descriptor)
    : file_descriptor(_file_descriptor)
  {
    init_mutex(mutex);
    struct utsname site_info;
    uname(&site_info);
  }

  ~Log_Manager()
  {
    destroy_mutex(mutex);
  }

  void init_line(
# ifdef DEBUG
		  char * file, 
		  int line, 
# endif
		  const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    {
      CRITICAL_SECTION(mutex);

      get_time();

# ifdef DEBUG
      fprintf(file_descriptor, "%s (%s:%d:%ld:%d) : ", 
	      time_buffer, file, line, pthread_self(), getpid());
# else
      fprintf(file_descriptor, "%s (%ld:%d): ", 
	      time_buffer, pthread_self(), getpid()); 
# endif
      vfprintf(file_descriptor, format, ap);
    }
    va_end(ap);
  }

  void write_line(
# ifdef DEBUG
		  char * file, 
		  int line, 
# endif
		  const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    {
      CRITICAL_SECTION(mutex);

      get_time();

# ifdef DEBUG
      fprintf(file_descriptor, "%s (%s:%d:%ld:%d) : ", 
	      time_buffer, file, line, pthread_self(), getpid());
# else
      fprintf(file_descriptor, "%s (%ld:%d): ", 
	      time_buffer, pthread_self(), getpid());
# endif
      vfprintf(file_descriptor, format, ap);
      fprintf(file_descriptor, "\n");
    }
    va_end(ap);
  }


};

# ifdef DEBUG
# define WRITE_IN_LOG(log, format...) log.write_line(__FILE__, __LINE__, format)
# else
# define WRITE_IN_LOG(log, format...) log.write_line(format)
# endif

# endif // LOG_H



