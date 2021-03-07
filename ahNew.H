
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

/*
 *	       Macro Wrappers To Memory Management Functions
 */

# ifndef AHNEW_H
# define AHNEW_H

# include <genAlloc.H>


/**@name Generic Allocation and Construction.

   {\bf Documentation}

   Lib Aleph is (was) originally designed for use in Aleph distributed
   system. A goal of this system, is (was) to research the impact on
   applications writing and systems performance of use several exclusive
   memory regions managed for independent allocators. 
   
   Allocating objects in different regions according to a specified criteria
   could result in performance and versatility gains. For example,
   contention could be reduced for the fact of having several allocators with
   separated locks. Certain applications could use their knowledge on objects
   nature in order to design special allocator adapted to their features.

   The new and delete operators of C++ poses problems for implementing the
   required features. Basically, new and delete do not allow cheap use of
   arenas, array usage is limited, and they do not allow to use memory
   allocator intended for debugging.

   All class in Aleph library needing memory management has an allocator
   parameter inside its constructor. By default, this allocator is always
   *Aleph::objectAlloc which is an allocator with the traditional features of
   malloc and free. Because the allocator is a default parameter, it is not
   necessary its explicit passing in construction time. So, the majority of
   applications do not require explicit using of an allocator.

   For allocation in Aleph::objectAlloc region, the macros Malloc, Free, New,
   NewSize, Delete and DeleteArray are provided. For allocation in other
   regions with other allocator, the macros ArrayMalloc, ArrayFree,
   ArrayNew, ArrayNewSize, ArrayDelete and ArrayDeleteArray are
   provided. Macros beginning for prefix Alloc has a reference to an
   allocator as first parameter. 

   At the time of writing of this version (1.0), Aleph library is in beta
   phase and the Aleph distributed system is in its development phase. For
   this reason, C++ new and delete operators are forbidden and their use
   will cause run-time errors. The reason is that with new and delete it is
   impossible to profit all features of debugging libraries such as
   dmalloc. This restriction could change in a future, but by now we prefer
   to be Draconian and to oblige you to use debugging libraries. 

   If DEBUG is defined, all macros pass the file name and the line number
   where the macro was instantiated. This allows to use debugging libraries,
   such as dmalloc, during the development phase. By default, Aleph::objectAlloc, the
   default allocator, uses dmalloc (if DEBUG is defined). 

   If you are a student, we highly recommend you not undefine DEBUG. You will
   need correctness instead of high performance for your course
   applications.  

   @author Leandro Leon
   @version 1.0
*/


//@{


/**
   Construct an object on a given memory address.

   The macro NewPlaced calls to constructor of object T assuming that the
   given pointer points a valid memory block. The memory block must be
   enough for holding the object.

   @param ptr Pointer to a valid memory block enough for holding the object.
 */
# define NewPlaced(T, ptr) new (ptr) T


# ifdef DEBUG

/**
   Allocate using default allocator.

   Malloc is a wrapper to a allocator which behavior is similar to
   traditional malloc c function. Default allocator (Aleph::objectAlloc) is used.

   @param size Size of requested block.
   @exception std::bad_alloc If allocator doesn't find memory
   @exception SizeFault If size == 0
   @return A pointer to allocated block
 */
# define Malloc(size) (dmalloc_malloc(__FILE__, __LINE__, size, \
                       DMALLOC_FUNC_MALLOC, ALEPH_MEM_ALIGNENT, 0))

/**
   Free a block from default allocator.

   @param p pointer to previously allocated block
   @exception AddrUnknown In certain allocators, this exception can be
   thrown when an invalid (not previously allocated) address is passed
*/
# define Free(p) (dmalloc_free(__FILE__, __LINE__, p, DMALLOC_FUNC_FREE))

/**
   Allocate a block and call to object constructor.

   Arrays can be allocated and constructed by joining their dimension
   between square brackets. Several pairs of square brackets can be used for
   specifying several dimensions. At the difference of new operator, if the
   Type of requested object is not simple, the void constructor is called
   for each entry of array.  

   @param T Type of block to be allocated (sizeof(T) bytes) and constructed 
   @param constructor_parameters Parameters can be passed to the
   constructor joining them between (); i.e. New (Foo) (bar). Here a object
   of Foo type is allocated and constructed. The parameter bar is passed to
   the constructor of Foo
   @exception std::bad_alloc If allocator doesn't find memory
   @exception Any Any exception thrown during constructor call will be
   propagated 
   @return A pointer to an allocated block containing a constructed object.
*/
# define New(T...) construct< T >(__FILE__, __LINE__)



/**
   Allocate a block plus an extra space and call to object constructor.

   The function allocates sz bytes more that sizeof(T). It could be useful
   if extra bytes are required. This situation can present in applications
   requiring allocated blocks of variable size.

   Arrays can be allocated and constructed by joining their dimension
   between square brackets. Several pairs of square brackets can be used for
   specifying several dimensions. At the difference of new operator, if the
   type of requested object is not simple, the void constructor is called
   for each entry of array. 

   @param T Type of block to be allocated (sizeof(T) bytes) and constructed 
   @param sz Extra size, respect to sizeof(T) to be allocated.
   @param constructor_parameters Parameters can be passed to the
   constructor joining them between (); i.e. New (Foo) (bar). Here a object
   of Foo type is allocated and constructed. The parameter bar is passed to
   the constructor of Foo
   @exception std::bad_alloc If allocator doesn't find memory
   @exception Any Any exception thrown during constructor call will be
   propagated 
   @return A pointer to an allocated block containing a constructed object.
*/
# define NewSize(T, sz...) construct<T>(__FILE__, __LINE__, sizeof(T) + sz)
/**
   Call object's destructor and free block.

   The macro Delete calls to destructor of an object pointed to ptr and then
   frees the memory block used by the object.

   Behavior is undefined (possibly crash) if an array is deleted through
   this macro. 

   @param ptr Pointer to previously allocated block containing an previously
   constructed object.

   @exception AddrUnknown In certain allocators, this exception can be
   thrown when an invalid (not previously allocated) address is passed
   @exception Any Any exception thrown during destructor call will be
   propagated 
*/
# define Delete(ptr) destruct(ptr, __FILE__, __LINE__)

/**
   Call object's destructor on an array and free all block used by the array.

   The macro DeleteArray calls to destructor of each object contained is the
   array of base ptr and then freed the memory block used by the array.

   Note that array dimension is not required.

   Behavior is undefined (possibly crash) if an object that is not an array
   is deleted through this macro. 

   @param ptr Pointer to base of a previously allocated array containing 
   previously constructed objects. 

   @exception AddrUnknown In certain allocators, this exception can be
   thrown when an invalid (not previously allocated) address is passed
   @exception Any Any exception thrown during destructor call will be
   propagated 
*/
# define DeleteArray(ptr) destructArray(ptr, __FILE__, __LINE__)

/**
   Allocate a block on a given allocator and call to object constructor.

   Arrays can be allocated and constructed by joining their dimension
   between square brackets. Several pairs of square brackets can be used for
   specifying several dimensions. At the difference of new operator, if the
   Type of requested object is not simple, the void constructor is called
   for each entry of array. 

   All classes of Aleph library requiring memory management use this macro
   in order to request memory on a given allocator.

   @param \_alloc Reference to memory allocator.
   @param T Type of block to be allocated (sizeof(T) bytes) and constructed 
   @param constructor_parameters Parameters can be passed to the
   constructor joining them between (); i.e. New (Foo) (bar). Here a object
   of Foo type is allocated and constructed. The parameter bar is passed to
   the constructor of Foo
   @exception std::bad_alloc If allocator doesn't find memory
   @exception Any Any exception thrown during constructor call will be
   propagated 
   @return A pointer to an allocated block containing a constructed object.
*/
# define AllocNew(_alloc, T...) construct< T >(__FILE__, __LINE__)

/**
   Allocate a block plus an extra space on a given allocator, then call to
   object constructor. 

   The function allocates sz bytes more that sizeof(T). It could be useful
   if extra bytes are required. This situation can present in applications
   requiring allocated blocks of variable size.

   Arrays can be allocated and constructed by joining their dimension
   between square brackets. Several pairs of square brackets can be used for
   specifying several dimensions. At the difference of new operator, if the
   type of requested object is not simple, the void constructor is called
   for each entry of array. 

   @param \_alloc Reference to memory allocator
   @param T Type of block to be allocated (sizeof(T) bytes) and constructed 
   @param sz Extra size, respect to sizeof(T) to be allocated.
   @param constructor_parameters Parameters can be passed to the
   constructor joining them between (); i.e. New (Foo) (bar). Here a object
   of Foo type is allocated and constructed. The parameter bar is passed to
   the constructor of Foo
   @exception std::bad_alloc If allocator doesn't find memory
   @exception Any Any exception thrown during constructor call will be
   propagated 
   @return A pointer to an allocated block containing a constructed object.
*/
# define AllocNewSize(_alloc, T, sz...) construct< T >(__FILE__,        \
                                                       __LINE__,        \
                                                       sizeof(T) + sz)

/**
   Call object's destructor and free a previously allocated block on a given
   allocator. 

   This macro calls to destructor of an object pointed to ptr and then
   frees the memory block used by the object. 

   Behavior is undefined (possibly crash) if an array is deleted through
   this macro. 

   @param ptr Pointer to previously allocated block containing an previously
   constructed object.

   @exception AddrUnknown In certain allocators, this exception can be
   thrown when an invalid (not previously allocated) address is passed. This
   error can occur too if it is used an allocator different that that used
   for allocation.
   @exception Any Any exception thrown during destructor call will be
   propagated 
*/
# define AllocDelete(alloc, ptr) destruct(ptr, __FILE__, __LINE__)

/**
   Call object's destructor on an array and free all the block used by the
   array.

   This macro calls to destructor of each object contained is the array of
   base ptr and then freed the memory block used by the array. 

   Note that array dimension is not required.

   Behavior is undefined (possibly crash) if an object that is not an array
   is deleted through this macro. 

   @param ptr Pointer to base of a previously allocated array containing 
   previously constructed objects. 

   @param \_alloc Reference to memory allocator
   @exception AddrUnknown In certain allocators, this exception can be
   thrown when an invalid (not previously allocated) address is passed
   @exception Any Any exception thrown during destructor call will be
   propagated 
*/
# define AllocDeleteArray(alloc, ptr) destructArray(ptr, __FILE__, __LINE__) 


# else /* DEBUG */


# define Malloc(size) (malloc(size))

# define Free(p) (free(p))

# define New(T...) construct< T >()

# define NewSize(T, sz...) construct<T>(sizeof(T) + sz)

# define Delete(ptr) destruct(ptr)

# define DeleteArray(ptr) destructArray(ptr)

# define AllocNew(T...) construct< T >()

# define AllocNewSize(T, sz...) construct< T >(sizeof(T) + sz)

# define AllocDelete(ptr) destruct(ptr)

# define AllocDeleteArray(ptr) destructArray(ptr)

# endif /* DEBUG */

//@}





# endif /* AHNEW_H */

