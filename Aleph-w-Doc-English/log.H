
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file log.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
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
    
    /* get seconds from anuary 1, 1970) */
    time(&unix_time); 
      
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
		  const char *format,...)
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
		  const char *format,...)
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
