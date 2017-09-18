#ifndef AH_ARGS_CTOR_H
#define AH_ARGS_CTOR_H

# define Args_Ctor(Name, Type)			\
  template <typename ... Args>			\
  Name(const Type & item, Args & ... args)	\
    : Name(args...)				\
  {						\
    this->append(item);				\
  }						\
						\
  template <typename ... Args>			\
  Name(Type && item, Args & ... args)		\
    : Name(args...)				\
  {						\
    this->append(item);				\
  }

#endif
