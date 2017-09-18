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

} // end namespace Aleph

# endif // AHTYPES_H
