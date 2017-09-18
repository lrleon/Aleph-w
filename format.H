
# ifndef FORMAT_H
# define FORMAT_H

# include <iomanip>
# include <string>
# include <iostream>

std::string float_f(const float & r, size_t d)
{
  stringstream s;
  s << std::setprecision(d) << std::fixed << r;
  return s.str();
}


# endif // FORMAT_H
