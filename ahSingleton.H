
# ifndef AHSINGLETON_H
# define AHSINGLETON_H


class Singleton
{
public:

  static Singleton & get_instance()
  {
    static Singleton instance;
    return instance;
  }
  
private:
  
  Singleton() {};

public:
  
  Singleton(Singleton const&) = delete;
  void operator = (Singleton const&) = delete;
};

# endif // AHSINGLETON_H
