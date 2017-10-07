
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
  @file ahTypes.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef AHTYPES_H
# define AHTYPES_H


namespace Aleph {

  template <typename T>
bool is_primitive_type() { return false; }

# define DECLARE_PRIMITIVE(T)			\
  template <>					\
  bool is_primitive_type <T> ()			\
  {						\
    return true;				\
  }

  DECLARE_PRIMITIVE(char);

  DECLARE_PRIMITIVE(unsigned char);

  DECLARE_PRIMITIVE(short);

  DECLARE_PRIMITIVE(unsigned short);

  DECLARE_PRIMITIVE(int);

  DECLARE_PRIMITIVE(unsigned int);

  DECLARE_PRIMITIVE(long long);

  DECLARE_PRIMITIVE(unsigned long long);

  DECLARE_PRIMITIVE(float);

  DECLARE_PRIMITIVE(double);

  DECLARE_PRIMITIVE(long double);


  template <class T> T & swap(T & t1, T & t2)
  {
    return t1.swap(t2);
  }

# define DECLARE_SWAP(T)			\
  template <> T & swap <T> (T & t1, T & t2)	\
  {						\
    T tmp = t1;					\
    t1 = t2;					\
    t2 = tmp;					\
    return t1;
  }

  DECLARE_SWAP(char);

  DECLARE_SWAP(unsigned char);

  DECLARE_SWAP(short);

  DECLARE_SWAP(unsigned shor);

  DECLARE_SWAP(int);

  DECLARE_SWAP(unsigned int);

  DECLARE_SWAP(long);

  DECLARE_SWAP(unsigned long);

  DECLARE_SWAP(long long);

  DECLARE_SWAP(unsigned long long);

  DECLARE_SWAP(float);

  DECLARE_SWAP(double);

  DECLARE_SWAP(long double);

} /* End namespace Aleph */

# endif // AHTYPES_H
