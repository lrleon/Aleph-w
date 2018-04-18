
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
  @file ahDefs.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AHDEFS_H
# define AHDEFS_H

# include <unistd.h>
# include <time.h>
# include <stdint.h>
# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>

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
    Empty_Class() { /* Empty */ }

    Empty_Class(EmptyCtor) { /* Empty */ }

    Empty_Class(SentinelCtor) { /* Empty */ }

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

  extern void message(const char * file, int line, const char *format,...);

  extern void error(const char * file, int line, const char *format,...);

  extern void exit(const char * file, int line, const char *format,...);

  extern void warning(const char * file, int line, const char *format,...);

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

# define ERROR(format, args...) \
         Aleph::error(__FILE__, __LINE__, format , ##args)

# define EXIT(format, args...) \
 	 Aleph::exit(__FILE__, __LINE__, format , ##args)

# define POINTER_HAS_BEEN_FOUND(ptr) ((ptr) not_eq NULL)
# define POINTER_HAS_NOT_BEEN_FOUND(ptr) ((ptr) == NULL)

# define Exception_Prototypes(list...) throw(std::exception, ##list)

#endif // AHDEFS_H
   