
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
  @file tpl_autoPtr.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_AUTO_PTR_H
# define TPL_AUTO_PTR_H

# include <ahExceptions.H>

    /* TODO Documentar para que se usa esta clase. */
    template <class T>
class Auto_Ptr
{
private:

  T *          ptr;
  mutable bool committed;

# ifdef DEBUG
  const char*       file;
  int               line;
# endif

public:

  Auto_Ptr(
# ifdef DEBUG
	   const char* _file, 
	   int         _line, 
# endif
	   T*        p       = NULL
	   ) :
    ptr(p), committed(false)
# ifdef DEBUG
    , 
    file(_file), line(_line)
# endif
  {
    /* Empty */ 
  }

  Auto_Ptr & operator = (T * p)
  {
    if (ptr not_eq NULL)
      throw std::domain_error ("Assignent of non null pointer");

    ptr = p;
    assert(not committed);

    return *this;
  }

  ~Auto_Ptr()
  {
    if (ptr == NULL)
      return;

    if (not committed)
      {
# ifdef DEBUG
	WARNING("Auto pointer declared in %s:%d\n"
		"was not committed and it has been freed", 
		file, line);
# endif
	delete ptr;
      }
  }

  void commit() { committed = true; }

  operator T* () 
  { 
    return ptr;
  }

  Auto_Ptr<T>& operator () () { return *this; }

  T* operator -> () 
  { 
    if (ptr == NULL)
      throw std::domain_error ("Null pointer");

    return ptr;
  }

  T& operator * ()
  { 
    if (ptr == NULL)
      throw std::domain_error ("Null pointer");
    
    return *ptr;
  }
};


# ifdef DEBUG
# define AUTO_PTR(type, name) Auto_Ptr<type> name(__FILE__, __LINE__)
# else
# define AUTO_PTR(type, name) Auto_Ptr<type> name
# endif

# endif // TPL_AUTO_PTR_H
