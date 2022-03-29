

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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



