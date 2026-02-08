
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


/** @file ahFunction.H
 *  @brief Standard functor implementations and comparison objects.
 *
 *  This file provides STL-compatible function objects for common operations
 *  including arithmetic operations, comparisons, logical operations, and
 *  hash functions. These functors are used throughout Aleph-w for generic
 *  algorithm implementations.
 *
 *  @ingroup Functors
 *  @author Leandro Rabindranath León
 */

// Functor implementations -*- C++ -*-

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996-1998
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

# ifndef AHFUNCTION_H
# define AHFUNCTION_H

# include <utility>

namespace Aleph
{

      template <class _Arg, class _Result>
  struct unary_function
  {
    typedef _Arg argument_type;
    typedef _Result result_type;
  };

      template <class _Arg1, class _Arg2, class _Result>
  struct binary_function
  {
    typedef _Arg1 first_argument_type;   // the type of the first argument
    typedef _Arg2 second_argument_type;  // the type of the second argument
    typedef _Result result_type;         // type of the return type
  };


      template <class _Tp>
  struct plus : public binary_function<_Tp, _Tp, _Tp>
  {
    _Tp operator () (const _Tp& __x, const _Tp& __y) const noexcept
    {
      return __x + __y; 
    }
  };


      template <class _Tp>
  struct minus : public binary_function<_Tp, _Tp, _Tp>
  {
    _Tp operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x - __y; 
    }
  };


      template <class _Tp>
  struct multiplies : public binary_function<_Tp, _Tp, _Tp>
  {
    _Tp operator () (const _Tp& __x, const _Tp& __y) const noexcept
    {
      return __x * __y; 
    }
  };


      template <class _Tp>
  struct divides : public binary_function<_Tp, _Tp, _Tp>
  {
    _Tp operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x / __y; 
    }
  };


      template <class _Tp>
  struct modulus : public binary_function<_Tp, _Tp, _Tp>
  {
    _Tp operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x % __y; 
    }
  };


      template <class _Tp>
  struct negate : public unary_function<_Tp, _Tp>
  {
    _Tp operator () (const _Tp& __x) const noexcept
    { 
      return -__x; 
    }
  };


      template <class _Tp>
  struct equal_to : public binary_function<_Tp, _Tp, bool>
  {
    bool operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x == __y; 
    }
  };


      template <class _Tp>
  struct not_equal_to : public binary_function<_Tp, _Tp, bool>
  {
    bool operator()(const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x != __y; 
    }
  };


      template <class _Tp>
  struct greater : public binary_function<_Tp, _Tp, bool>
  {
    bool operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x > __y; 
    }
  };


      template <class _Tp>
  struct less : public binary_function<_Tp, _Tp, bool>
  {
    bool operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x < __y; 
    }
  };


      template <class _Tp>
  struct greater_equal : public binary_function<_Tp, _Tp, bool>
  {
    bool operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x >= __y; 
    }
  };


      template <class _Tp>
  struct less_equal : public binary_function<_Tp, _Tp, bool>
  {
    bool operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x <= __y; 
    }
  };


      template <class _Tp>
  struct logical_and : public binary_function<_Tp, _Tp, bool>
  {
    bool operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x and __y; 
    }
  };


      template <class _Tp>
  struct logical_or : public binary_function<_Tp, _Tp, bool>
  {
    bool operator () (const _Tp& __x, const _Tp& __y) const noexcept
    { 
      return __x or __y; 
    }
  };


      template <class _Tp>
  struct logical_not : public unary_function<_Tp, bool>
  {
    bool operator () (const _Tp& __x) const noexcept
    { 
      return not __x; 
    }
  };


      template <class _Predicate>
  class unary_negate
    : public unary_function <typename _Predicate::argument_type, bool>
  {
    protected:

      _Predicate _M_pred;

    public:

    explicit unary_negate (const _Predicate& __x) : _M_pred(__x) {}

    bool operator () (const typename _Predicate::argument_type& __x)
      const noexcept
    { 
      return not _M_pred(__x); 
    }
  };


      template <class _Predicate> inline 
  unary_negate<_Predicate> not1 (const _Predicate& __pred)
  { 
    return unary_negate<_Predicate>(__pred); 
  }


      template <class _Predicate>
  class binary_negate
    : public binary_function<typename _Predicate::first_argument_type,
			     typename _Predicate::second_argument_type,
			     bool>
  {
  protected:

    _Predicate _M_pred;

  public:

    explicit binary_negate (const _Predicate& __x) : _M_pred(__x) { }

        bool 
    operator () (const typename _Predicate::first_argument_type& __x,
		 const typename _Predicate::second_argument_type& __y) const
      { 
	return not _M_pred(__x, __y); 
      }
  };


      template <class _Predicate> inline 
  binary_negate<_Predicate> not2 (const _Predicate& __pred)
  { 
    return binary_negate<_Predicate>(__pred); 
  }


      template <class _Operation>
  class binder1st
    : public unary_function<typename _Operation::second_argument_type,
			    typename _Operation::result_type>
  {
  protected:

    _Operation op;

    typename _Operation::first_argument_type value;

    public:
    
    binder1st(const _Operation& __x,
	      const typename _Operation::first_argument_type& __y)
      : op(__x), value(__y) {}

        typename _Operation::result_type
    operator () (const typename _Operation::second_argument_type& __x) const
    { 
      return op (value, __x); 
    }

        typename _Operation::result_type
    operator () (typename _Operation::second_argument_type& __x) const
    { 
      return op(value, __x); 
    }
  };


      template <class _Operation, class _Tp> inline 
  binder1st<_Operation> bind1st(const _Operation& __fn, const _Tp& __x)
  {
    typedef typename _Operation::first_argument_type _Arg1_type;

    return binder1st<_Operation> (__fn, _Arg1_type(__x));
  }

      template <class _Operation>
  class binder2nd
    : public unary_function<typename _Operation::first_argument_type,
			    typename _Operation::result_type>
  {
  protected:

      _Operation op;

      typename _Operation::second_argument_type value;

    public:

    binder2nd(const _Operation& __x,
	      const typename _Operation::second_argument_type& __y)
      : op(__x), value (__y) {}

        typename _Operation::result_type
    operator () (const typename _Operation::first_argument_type& __x) const
    { 
      return op (__x, value); 
    }
    
        typename _Operation::result_type
    operator () (typename _Operation::first_argument_type& __x) const
    { 
      return op (__x, value); 
    }
  };


      template <class _Operation, class _Tp> inline
  binder2nd<_Operation> bind2nd (const _Operation& __fn, const _Tp& __x)
  {
    typedef typename _Operation::second_argument_type _Arg2_type;

    return binder2nd<_Operation>(__fn, _Arg2_type(__x));
  }


      template <class _Arg, class _Result>
  class pointer_to_unary_function : public unary_function<_Arg, _Result>
  {
  protected:

    _Result (*_M_ptr) (_Arg);

  public:

    pointer_to_unary_function () {}

    explicit pointer_to_unary_function(_Result (*__x) (_Arg))
      : _M_ptr (__x) {}

    _Result operator () (_Arg __x) const
    {
      return _M_ptr (__x); 
    }
  };


      template <class _Arg, class _Result> inline 
  pointer_to_unary_function<_Arg, _Result> ptr_fun(_Result (*__x)(_Arg))
  { 
    return pointer_to_unary_function<_Arg, _Result>(__x); 
  }
  

      template <class _Arg1, class _Arg2, class _Result>
  class pointer_to_binary_function
    : public binary_function<_Arg1, _Arg2, _Result>
  {
  protected:

    _Result (*_M_ptr) (_Arg1, _Arg2);

  public:

    pointer_to_binary_function() {}

    explicit pointer_to_binary_function(_Result (*__x) (_Arg1, _Arg2))
      : _M_ptr(__x) {}

    _Result operator () (_Arg1 __x, _Arg2 __y) const
    {
      return _M_ptr(__x, __y); 
    }
  };


      template <class _Arg1, class _Arg2, class _Result> inline
  pointer_to_binary_function<_Arg1, _Arg2, _Result> 
  ptr_fun (_Result (*__x) (_Arg1, _Arg2) )
  {
    return pointer_to_binary_function<_Arg1, _Arg2, _Result>(__x); 
  }

      template <class _Tp>
  struct _Identity : public unary_function<_Tp,_Tp>
  {
    _Tp & operator () (_Tp& __x) const
    { 
      return __x;
    }

    const _Tp& operator () (const _Tp& __x) const
    { 
      return __x; 
    }
  };

      template <class _Pair> 
  struct _Select1st : public unary_function<_Pair, typename _Pair::first_type>
  {
    typename _Pair::first_type& operator () (_Pair& __x) const
    { 
      return __x.first; 
    }

    const typename _Pair::first_type& operator () (const _Pair& __x) const
    { 
      return __x.first; 
    }
  };

      template <class _Pair>
  struct _Select2nd : public unary_function<_Pair, typename _Pair::second_type>
  {
    typename _Pair::second_type& operator () (_Pair& __x) const
    {
      return __x.second; 
    }

    const typename _Pair::second_type& operator () (const _Pair& __x) const
    { 
      return __x.second; 
    }
  };


      template <class _Ret, class _Tp>
  class mem_fun_t : public unary_function<_Tp*, _Ret>
  {
  public:

    explicit mem_fun_t (_Ret (_Tp::*__pf) () )
      : _M_f(__pf) {}

    _Ret operator ()(_Tp* __p) const
    { 
      return (__p->*_M_f)(); 
    }

    private:

      _Ret (_Tp::*_M_f) ();
  };


      template <class _Ret, class _Tp>
  class const_mem_fun_t : public unary_function<const _Tp*, _Ret>
  {
  public:

    explicit const_mem_fun_t (_Ret (_Tp::*__pf) () const)
      : _M_f (__pf) {}

    _Ret operator () (const _Tp* __p) const
    { 
      return (__p->*_M_f) (); 
    }

    private:

    _Ret (_Tp::*_M_f) () const;
  };


      template <class _Ret, class _Tp>
  class mem_fun_ref_t : public unary_function<_Tp, _Ret>
  {
  public:

    explicit mem_fun_ref_t (_Ret (_Tp::*__pf)())
      : _M_f (__pf) {}

    _Ret operator () (_Tp& __r) const
    { 
      return (__r.*_M_f)(); 
    }

    private:

    _Ret (_Tp::*_M_f) ();
  };


      template <class _Ret, class _Tp>
  class const_mem_fun_ref_t : public unary_function<_Tp, _Ret>
  {
  public:

    explicit const_mem_fun_ref_t (_Ret (_Tp::*__pf)() const)
      : _M_f (__pf) {}

    _Ret operator () (const _Tp& __r) const
    { 
      return (__r.*_M_f)(); 
    }

    private:

    _Ret (_Tp::*_M_f) () const;
  };


      template <class _Ret, class _Tp, class _Arg>
  class mem_fun1_t : public binary_function<_Tp*, _Arg, _Ret>
  {
  public:

    explicit mem_fun1_t (_Ret (_Tp::*__pf) (_Arg))
      : _M_f(__pf) {}

    _Ret operator () (_Tp* __p, _Arg __x) const
    { 
      return (__p->*_M_f) (__x); 
    }

    private:
    
    _Ret (_Tp::*_M_f) (_Arg);
  };


      template <class _Ret, class _Tp, class _Arg>
  class const_mem_fun1_t : public binary_function<const _Tp*, _Arg, _Ret>
  {
  public:

    explicit const_mem_fun1_t (_Ret (_Tp::*__pf) (_Arg) const)
      : _M_f (__pf) {}

    _Ret operator () (const _Tp* __p, _Arg __x) const
    { 
      return (__p->*_M_f)(__x); 
    }

    private:

    _Ret (_Tp::*_M_f) (_Arg) const;
  };


      template <class _Ret, class _Tp, class _Arg>
  class mem_fun1_ref_t : public binary_function<_Tp, _Arg, _Ret>
  {
  public:
    
    explicit mem_fun1_ref_t (_Ret (_Tp::*__pf) (_Arg))
      : _M_f (__pf) {}

    _Ret operator () (_Tp& __r, _Arg __x) const
    { 
      return (__r.*_M_f)(__x); 
    }

    private:

    _Ret (_Tp::*_M_f) (_Arg);
  };


      template <class _Ret, class _Tp, class _Arg>
  class const_mem_fun1_ref_t : public binary_function<_Tp, _Arg, _Ret>
  {
  public:
  
    explicit const_mem_fun1_ref_t(_Ret (_Tp::*__pf)(_Arg) const)
      : _M_f(__pf) {}

    _Ret operator () (const _Tp& __r, _Arg __x) const
    { 
      return (__r.*_M_f)(__x); 
    }

    private:

      _Ret (_Tp::*_M_f) (_Arg) const;
  };


      template <class _Tp>
  class mem_fun_t<void, _Tp> : public unary_function<_Tp*, void>
  {
  public:
  
    explicit mem_fun_t (void (_Tp::*__pf)())
      : _M_f (__pf) {}

    void operator () (_Tp* __p) const
    {
      (__p->*_M_f)(); 
    }

    private:

    void (_Tp::*_M_f)();
  };


      template <class _Tp>
  class const_mem_fun_t<void, _Tp> : public unary_function<const _Tp*, void>
  {
  public:
  
    explicit const_mem_fun_t (void (_Tp::*__pf)() const)
      : _M_f (__pf) {}

    void operator () (const _Tp* __p) const
    { 
      (__p->*_M_f)(); 
    }

    private:

      void (_Tp::*_M_f)() const;
  };


      template <class _Tp>
  class mem_fun_ref_t<void, _Tp> : public unary_function<_Tp, void>
  {
  public:

    explicit mem_fun_ref_t (void (_Tp::*__pf)()) : _M_f (__pf) {}

    void operator () (_Tp& __r) const
    { 
      (__r.*_M_f)(); 
    }

    private:

      void (_Tp::*_M_f)();
  };


      template <class _Tp>
  class const_mem_fun_ref_t<void, _Tp> : public unary_function<_Tp, void>
  {
  public:

    explicit const_mem_fun_ref_t (void (_Tp::*__pf) () const)
      : _M_f (__pf) {}

    void operator () (const _Tp& __r) const
    {
      (__r.*_M_f) (); 
    }

    private:

    void (_Tp::*_M_f) () const;
  };


      template <class _Tp, class _Arg>
  class mem_fun1_t<void, _Tp, _Arg> : public binary_function<_Tp*, _Arg, void>
  {
  public:
    
    explicit mem_fun1_t(void (_Tp::*__pf) (_Arg)) : _M_f (__pf) {}

    void operator () (_Tp* __p, _Arg __x) const
    {
      (__p->*_M_f)(__x); 
    }

    private:

    void (_Tp::*_M_f) (_Arg);
  };


      template <class _Tp, class _Arg>
  class const_mem_fun1_t<void, _Tp, _Arg>
    : public binary_function<const _Tp*, _Arg, void>
  {
  public:

    explicit const_mem_fun1_t (void (_Tp::*__pf) (_Arg) const) : _M_f (__pf) {}

    void operator () (const _Tp* __p, _Arg __x) const
    { 
      (__p->*_M_f)(__x); 
    }

    private:

    void (_Tp::*_M_f)(_Arg) const;
  };


      template <class _Tp, class _Arg>
  class mem_fun1_ref_t<void, _Tp, _Arg>
    : public binary_function<_Tp, _Arg, void>
  {
  public:

    explicit mem_fun1_ref_t (void (_Tp::*__pf) (_Arg)) : _M_f (__pf) {}

    void operator () (_Tp& __r, _Arg __x) const
    { 
      (__r.*_M_f)(__x); 
    }

    private:

    void (_Tp::*_M_f) (_Arg);
  };


      template <class _Tp, class _Arg>
  class const_mem_fun1_ref_t<void, _Tp, _Arg>
    : public binary_function<_Tp, _Arg, void>
  {
  public:

    explicit const_mem_fun1_ref_t (void (_Tp::*__pf) (_Arg) const) 
      : _M_f (__pf) {}

    void operator () (const _Tp& __r, _Arg __x) const
    {
      (__r.*_M_f) (__x); 
    }

  private:

    void (_Tp::*_M_f)(_Arg) const;
  };


      template <class _Ret, class _Tp> inline
  mem_fun_t<_Ret, _Tp> mem_fun (_Ret (_Tp::*__f) () )
  {
    return mem_fun_t<_Ret, _Tp> (__f); 
  }

      template <class _Ret, class _Tp> inline 
  const_mem_fun_t<_Ret, _Tp> mem_fun (_Ret (_Tp::*__f) () const)
  { 
    return const_mem_fun_t<_Ret, _Tp> (__f); 
  }

      template <class _Ret, class _Tp> inline
  mem_fun_ref_t<_Ret, _Tp> mem_fun_ref (_Ret (_Tp::*__f) ())
  {
    return mem_fun_ref_t<_Ret, _Tp> (__f); 
  }

      template <class _Ret, class _Tp> inline 
  const_mem_fun_ref_t<_Ret, _Tp> mem_fun_ref (_Ret (_Tp::*__f) () const)
  { 
    return const_mem_fun_ref_t<_Ret, _Tp> (__f); 
  }

      template <class _Ret, class _Tp, class _Arg> inline
  mem_fun1_t<_Ret, _Tp, _Arg> mem_fun (_Ret (_Tp::*__f) (_Arg))
  {
    return mem_fun1_t<_Ret, _Tp, _Arg> (__f); 
  }

      template <class _Ret, class _Tp, class _Arg> inline
  const_mem_fun1_t<_Ret, _Tp, _Arg> mem_fun (_Ret (_Tp::*__f) (_Arg) const)
  { 
    return const_mem_fun1_t<_Ret, _Tp, _Arg>(__f); 
  }

      template <class _Ret, class _Tp, class _Arg> inline
  mem_fun1_ref_t<_Ret, _Tp, _Arg> mem_fun_ref (_Ret (_Tp::*__f) (_Arg))
  { 
    return mem_fun1_ref_t<_Ret, _Tp, _Arg>(__f); 
  }

      template <class _Ret, class _Tp, class _Arg> inline
  const_mem_fun1_ref_t<_Ret, _Tp, _Arg> 
      mem_fun_ref (_Ret (_Tp::*__f) (_Arg) const)
  { 
    return const_mem_fun1_ref_t<_Ret, _Tp, _Arg>(__f); 
  }


  /** Determines if op1 is less than op2 using a comparison operator.
   *
   *  Uses an exclusive comparison operator (< or >). For use with generic
   *  programs that need comparison functionality.
   *
   *  @param op1 First operand
   *  @param op2 Second operand
   *  @param cmp Comparison functor
   *  @return true if op1 < op2 according to cmp
   *  @ingroup Functors
   */
    template <class T, class Compare> inline
bool less_than(const T & op1, const T & op2, Compare & cmp)
{
  return cmp (op1, op2);
}

 
  /**
     @ingroup Functions
     \overload less_than()
  */
    template <class T, class Compare> inline
bool less_than(const T & op1, const T & op2, Compare && cmp = Compare())
{
  return less_than <T, Compare> (op1, op2, cmp);
}


  /** Determines if op1 is less than or equal to op2 using a comparison operator.
   *
   *  Uses an exclusive comparison operator (< or >). For use with generic
   *  programs that need comparison functionality.
   *
   *  @param op1 First operand
   *  @param op2 Second operand
   *  @param cmp Comparison functor
   *  @return true if op1 <= op2 according to cmp
   *  @ingroup Functors
   */
    template <class T, class Compare> inline
bool less_or_equal_than(const T& op1, const T& op2, Compare & cmp)
{
  if (cmp(op1, op2))
    return true;
  
  if (cmp(op2, op1))
    return false;

  return true;
}


  /**
     @ingroup Functions
     \overload less_or_equal_than()
  */
    template <class T, class Compare> inline
bool less_or_equal_than(const T& op1, const T& op2, Compare && cmp = Compare())
{
  return less_or_equal_than<T, Compare>(op1, op2, cmp);
}


  /** Determines if op1 is greater than op2 using a comparison operator.
   *
   *  @param op1 First operand
   *  @param op2 Second operand
   *  @param cmp Comparison functor
   *  @return true if op1 > op2 according to cmp
   *  @ingroup Functors
   */
    template <class T, class Compare> inline
bool greater_than(const T& op1, const T& op2, Compare && cmp = Compare())
{
  return not less_or_equal_than<T, Compare>(op1, op2, 
					    std::forward<Compare>(cmp));
}

  /** \overload greater_than()

      @ingroup Functions
  */
  template <class T, class Compare> inline
bool greater_than(const T& op1, const T& op2, Compare & cmp)
{
  return not less_or_equal_than<T, Compare> (op1, op2, cmp);
}

  /** Determines if op1 is greater than or equal to op2 using a comparison operator.
   *
   *  @param op1 First operand
   *  @param op2 Second operand
   *  @param cmp Comparison functor
   *  @return true if op1 >= op2 according to cmp
   *  @ingroup Functors
   */
    template <class T, class Compare> inline
bool greater_or_equal_than(const T& op1, const T & op2,
			   Compare && cmp = Compare())
{
  return not less_than<T, Compare> (op1, op2, std::forward<Compare>(cmp));
}

  /** \overload greater_or_equal_than()

      @ingroup Functions
  */
    template <class T, class Compare> inline
bool greater_or_equal_than(const T& op1, const T& op2, Compare & cmp)
{
  return not less_than<T, Compare> (op1, op2, cmp);
}

  /** Determines if operands are not equal using a comparison operator.
   *
   *  @param op1 First operand
   *  @param op2 Second operand
   *  @param cmp Comparison functor
   *  @return true if op1 != op2 according to cmp
   *  @ingroup Functors
   */
    template <class T, class Compare> inline
bool no_equals(const T & op1, const T & op2, Compare && cmp = Compare())
{
  return cmp(op1, op2) or cmp(op2, op1);
}

  /** \overload no_equals()

      @ingroup Functions
  */
    template <class T, class Compare> inline
bool no_equals(const T & op1, const T & op2, Compare & cmp)
{
  return cmp(op1, op2) or cmp(op2, op1);
}

  /** Determines if operands are equal using a comparison operator.
   *
   *  @param op1 First operand
   *  @param op2 Second operand
   *  @param cmp Comparison functor
   *  @return true if op1 == op2 according to cmp
   *  @ingroup Functors
   */
    template <class T, class Compare> inline
bool are_equals(const T & op1, const T & op2, Compare && cmp = Compare())
{
  return not no_equals <T, Compare> (op1, op2, std::forward<Compare>(cmp));
}

  /** \overload are_equals()

      @ingroup Functions
  */
    template <class T, class Compare> inline
bool are_equals(const T & op1, const T & op2, Compare & cmp) 
{
  return not no_equals <T, Compare> (op1, op2, cmp);
}

  /** Performs order reversal of Compare by swapping operands.
   *
   *  Inversed_Compare evaluates `cmp(op2, op1)`. This is a semantic
   *  inversion (argument swap), not logical negation of `cmp(op1, op2)`.
   *  For example: `<` becomes `>`, and `<=` becomes `>=`.
   *
   *  @tparam T Type of values to compare
   *  @tparam Compare Comparison functor type
   *  @ingroup Functors
   */
    template <class T, class Compare>
class Inversed_Compare
{
  const Compare & cmp;

public:

  Inversed_Compare(const Compare & __cmp) noexcept : cmp(__cmp) { /* empty */ }

  Inversed_Compare(Compare && cmp = Compare()) noexcept
  : Inversed_Compare(cmp)
  { /* empty */ }

  bool operator () (const T & op1, const T & op2) const noexcept
  {
    return cmp(op2, op1);
  }
};

   /**

     @ingroup Functions
   */
    template <class T, class Compare>
struct Compare_Dup 
{
  bool operator () (const T & op1, const T & op2) const
  {
    if (Compare () (op1, op2))
      return true;
    
    if (Compare () (op2, op1))
      return false;

    return true; 
  }
};

} // namespace Aleph

#endif /* AHFUNCTION_H */

