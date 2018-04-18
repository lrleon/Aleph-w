
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
  @file staticAlloc.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


/** 
  @brief Simpler arena allocator. 

  @details It is generally used for static permanent assignment static data
  (in the data segment through a static array)
  or crush object mobility
*/

# ifndef STATICALLOC_H
# define STATICALLOC_H

# include <genAlloc.H>


class StaticAlloc : public GenAlloc
{
private:

  char *baseAddr;
  char *currAddr;
  char *maxAddr;

public:

  StaticAlloc(void* base, size_t size) 
    : GenAlloc(true), /* It is an arena */
      baseAddr(static_cast<char*> (base)),
      currAddr(baseAddr),
      maxAddr(baseAddr + size)
    {
      /* Empty */
    }

  size_t getAllocatedSize() const { return currAddr - baseAddr; }

  size_t getAvailableSize() const { return maxAddr - currAddr; }

  size_t getCapacity() const { return maxAddr - baseAddr; }

  virtual Pointer alloc(size_t size) 
    Exception_Prototypes(std::bad_alloc, SizeFault)
    {
      if (size == 0)
	Throw (SizeFault) (size);

      size_t neededSize = roundAlign(size);

      if (currAddr + neededSize > maxAddr)
	throw std::bad_alloc();

      Pointer retVal = currAddr;

      currAddr += neededSize;

      return retVal;
    }

  virtual void dealloc(Pointer) throw ()
    {
      /* Empty */
    }

  virtual void dealloc(Pointer, size_t) throw ()
    {
      /* Empty */
    }

  virtual Pointer ralloc(Pointer, size_t) 
    throw ()
    {
      return NULL;
    }

  virtual Pointer alloc(size_t size, 
			const char* //__FILE__, 
			int         //__LINE__) 
    Exception_Prototypes(std::bad_alloc, SizeFault)
    {
      return alloc(size);
    }

  virtual void dealloc(Pointer,
		       const char* //__FILE__,
		       int         //__LINE__) 
    throw ()
    {
      /* Empty */
    }

  virtual void dealloc(Pointer,
		       size_t,
		       const char* //__FILE__,
		       int         //__LINE__) 
    throw ()
    {
      /* Empty */
    }

  virtual Pointer ralloc(Pointer, 
			 size_t, 
			 const char* //__FILE__, 
			 int         //__LINE__) 
    throw ()
    {
      return NULL;
    }

};

# endif /* STATICALLOC_H */
