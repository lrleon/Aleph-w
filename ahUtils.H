
# ifndef AHUTILS_H
# define AHUTILS_H

# include <ahAssert.H>
# include <ahFunction.H>

namespace Aleph {


    /** Retorna un puntero a la mediana entre tres datos.

	Se retorna un puntero a efectos de ahorrar la copia y, muy
	especialmente, para emplear la igualdad por punteros.

	@ingroup fcts
     */
  template <class T, class Compare = Aleph::less<T>> inline
const T * median(const T & a, const T & b, const T & c, Compare & cmp)
{
  if (cmp(a, b))
    if (cmp(b, c))
      return &b;
    else if (cmp(a, c))
      return &c;
    else
      return &a;
  else if (cmp(a, c))
    return &a;
  else if (cmp(b, c))
    return &c;
  else
    return &b;
}


  /** Retorna <code>true</code> si n es par

     @ingroup fcts
   */
inline bool is_even(long n)
{
  return (n % 2) == 0;
}

  /** Retorna <code>true</code> si n es impar

     @ingroup fcts
   */
inline bool is_odd(long n)
{
  return not is_even(n);
}

  /** Convierte una nibla almaceneda en un <code>int</code> a un
      <code<>char/code>.  

      @ingroup fcts
   */
inline char nibble_to_char(const int & i)
{
  assert(i >= 0 and i <= 15);

  const char ret = i < 10 ? i + '0' : i - 10 + 'A';

  assert((ret >= '0' and ret <= '9') or (ret >= 'A' and ret <= 'F'));

  return ret;
}

  /** Convierte una nibla almaceneda en un <code<>char/code> a un
      <code>int</code>.

      @ingroup fcts
   */
inline int char_to_nibble(const char & c)
{
  assert((c >= '0' and c <= '9') or (c >= 'A' and c <= 'F'));

  const int ret = c < 'A' ? c - '0' : c - 'A' + 10; 

  assert(ret >= 0 and ret <= 15);

  return ret;
} 

# define DERIVATE_ITERATOR(container_name,  base_it_name, it_name) \
  struct it_name : public base_it_name				   \
  {								   \
    it_name() { /* empty */ }					   \
								   \
    it_name(container_name & c) : base_it_name(c)		   \
      {								   \
	/* empty */						   \
      }								   \
								   \
    it_name(const it_name & it) : base_it_name(it)		   \
      {								   \
	/* empty */						   \
      }								   \
								   \
    it_name & operator = (const it_name & it)			   \
      {								   \
	return base_it_name::operator = (it);			   \
      }								   \
  }; 


/** Reajusta el tamaño de la pila sistema a new_size. Retorna true si
    tiene éxito; false de lo contrario

   @ingroup fcts
 */
extern bool resize_process_stack(size_t new_size);

inline size_t u_index(const size_t & i)
{
  return i >> 1; // divide i entre 2
}

inline size_t l_index(const size_t & i)
{
  return i << 1; // multiplica i por 2
}

/** 

    Taken from http://stackoverflow.com/questions/3638431/determine-if-an-int-is-a-power-of-2-or-not-in-a-single-line

    @ingroup utils
*/
inline bool is_power_of_2(unsigned long x) 
{
  return x && !(x & (x - 1));
}


}


# endif // AHUTILS_H

