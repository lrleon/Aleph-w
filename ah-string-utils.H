
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# ifndef AH_STRING_UTILS_H
# define AH_STRING_UTILS_H

# include <type_traits>
# include <cstring>
# include <memory>
# include <string>
# include <sstream>
# include <iterator>
# include <vector>
# include <htlist.H>
# include <tpl_array.H>
# include <ah-zip.H>

namespace Aleph
{

  // left trim
  inline void ltrim(string & s)
  {
    s.erase(s.begin(), find_if(s.begin(), s.end(),
                               [](int ch)
                               { return not isspace(ch); }));
  }

  // trim from end
  inline void rtrim(string & s)
  {
    s.erase(find_if(s.rbegin(), s.rend(),
                    [](int ch)
                    { return not isspace(ch); }).base(), s.end());
  }

  // fast concatenation of several strings
  template <class ... Args>
  inline string concat(string str, const Args & ... args)
  {
    size_t sz = str.size();
    auto add_size = [&sz](const string_view & s)
    { sz += s.size(); };
    (add_size(args), ...); // put in sz the final size of the string

    str.reserve(sz); // thus, we avoid additional reallocations

    auto add = [&str](const string_view & s)
    { str += s; };
    (add(args), ...);

    return str;
  }

  // trim from both ends (in place)
  inline string trim(const string & s)
  {
    string ret = s;
    ltrim(ret);
    rtrim(ret);
    return ret;
  }

  inline string & trim_in_place(string & s)
  {
    ltrim(s);
    rtrim(s);
    return s;
  }

  inline bool contains(const string_view & str, const string_view & substr)
  {
    return str.find(substr) != std::string::npos;
  }

  inline string to_string(double d, size_t precision)
  {
    ostringstream s;
    s.precision(precision);
    s << fixed << d;
    return s.str();
  }

  inline string to_str(double d)
  {
    return to_string(d, numeric_limits<double>::max_digits10);
  }

  // inline bool read_line(istream & in, istringstream & s)
  // {
  //   string line;
  //   if (not getline(in, line))
  //     return false;
  //   s = istringstream(line);
  //   return true;
  // }

  inline string tolower(const string & str)
  {
    string ret;
    ret.reserve(str.size());
    for (auto & c: str)
      ret.push_back(::tolower(c));
    return ret;
  }

  inline string toupper(const string & str)
  {
    string ret;
    ret.reserve(str.size());
    for (auto & c: str)
      ret.push_back(::toupper(c));
    return ret;
  }

  inline string & mutable_tolower(string & str)
  {
    for (auto & c: str)
      c = ::tolower(c);
    return str;
  }

  inline string & mutable_toupper(string & str)
  {
    for (auto & c: str)
      c = ::toupper(c);
    return str;
  }

  inline std::string only_alpha(const std::string & str)
  {
    std::string ret;
    ret.reserve(str.size() + 1);
    for (size_t i = 0, n = str.size(); i < n; ++i)
      {
        auto c = str[i];
        switch (c)
          {
            case '0'...'9':
            case 'a'...'z':
              ret.push_back(c);
            break;
            case 'A'...'Z':
              ret.push_back(::tolower(c));
            break;
            default:
              break;
          }
      }
    return ret;
  }

  inline string remove_spaces(const string & str)
  {
    const size_t n = str.size();
    std::string ret;
    ret.reserve(n + 1);
    for (auto c: str)
      if (not isspace(c))
        ret.push_back(c);
    return ret;
  }

  inline string remove_symbols(const string & str, const string & symbols)
  {
    const size_t n = str.size();
    std::string ret;
    ret.reserve(n + 1);
    for (auto c: str)
      {
        string s;
        s.push_back(c);
        if (not contains(symbols, s))
          ret.push_back(c);
      }
    return ret;
  }

  template <class C>
  inline ostream & join(const C & c, const string & sep, ostream & out)
  {
    if (c.is_empty())
      return out;

    auto & last = c.get_last();
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        out << curr;
        if (&curr != &last)
          out << sep;
      }
    return out;
  }

  template <class C>
  inline string join(const C & c, const string & sep = " ")
  {
    ostringstream s;
    join(c, sep, s);
    return s.str();
  }

  // return true if str can be converted to a double
  inline bool is_double(const string & str)
  {
    char* endptr = 0;
    long n = strtod(str.data(), &endptr);
    if(*endptr != '\0' or endptr == str.data())
      return false;
    return n >= 0;
  }

  inline bool is_long(const string & str)
  {
    char *endptr = 0;
    strtol(str.data(), &endptr, 10);
    if (*endptr != '\0' or endptr == str.data())
      return false;
    return true;
  }

  inline bool is_size_t(const string & str)
  {
    char *endptr = 0;
    strtol(str.data(), &endptr, 10);
    if (*endptr != '\0' or endptr == str.data())
      return false;
    return true;
  }

  inline long atol(const string & s)
  {
    return ::atol(s.c_str());
  }

  inline long atoi(const string & s)
  {
    return ::atoi(s.c_str());
  }

  inline double atof(const string & s)
  {
    return ::atof(s.c_str());
  }

  inline bool is_prefix(const string & str, const string & prefix)
  {
    return strncmp(str.data(), prefix.data(), prefix.size()) == 0;
  }

  inline string remove_prefix(string & str, const string & prefix)
  {
    if (is_prefix(str, prefix))
      str = str.substr(prefix.size());
    return str;
  }

  inline std::string to_lower(const std::string & str)
  {
    string ret;
    for (char c: str)
      ret.push_back(::tolower(c));
    return ret;
  }

  inline std::string to_upper(const std::string & str)
  {
    string ret;
    for (char c: str)
      ret.push_back(::toupper(c));
    return ret;
  }

  inline std::string to_name(const std::string & str)
  {
    if (str.size() == 0)
      return str;

    string ret;
    ret.push_back(::toupper(str[0]));

    for (size_t i = 1; i < str.size(); ++i)
      ret.push_back(str[i]);

    return ret;
  }

  inline DynList<string> split_camel_case(const char *const str)
  {
    DynList<string> ret;
    const char *ptr = str;
    string curr(1, *ptr);
    while (*++ptr)
      if (isupper(*ptr))
        {
          ret.append(curr);
          curr = string(1, *ptr);
        }
      else
        curr.push_back(*ptr);

    if (not curr.empty())
      ret.append(curr);

    return ret;
  }

  inline DynList<string> split_camel_case(const string & str)
  {
    return split_camel_case(str.data());
  }

  /** Fill all the content of string with a defined char.

      Use for deleting sensitive data; for example, passwords

  */
  inline void fill_string(std::string & str, char sym)
  {
    for (char & c: str)
      c = sym;
  }

  inline std::vector<std::string> &
  split(const std::string & s, char delim, std::vector<std::string> & elems)
  {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim))
      elems.push_back(item);

    return elems;
  }

  inline std::vector<std::string> split(const std::string & s, char delim)
  {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }

  template <template <typename> class Container = DynList>
  inline Container<string> split_string(const string & s, const string & delim)
  {
    Container<string> elems;
    if (s.empty())
      return elems;

    char *buf = new char[s.size() + 1];
    char *ptr = buf;
    char **p = &buf;

    string data = s;
    const char *d = delim.data();
    char *str = (char *) data.data();

    try
      {
        for (char *token = strtok_r(str, d, p); token;
             token = strtok_r(NULL, d, p))
          elems.append(token);
        delete[] ptr;
      }
    catch (...)
      {
        delete[] ptr;
        throw;
      }

    return elems;
  }

  inline DynList<std::string> split_to_list(const string & s,
                                            const string & delim)
  {
    return split_string<DynList>(s, delim);
  }

  inline std::string to_Pascalcase(const string & str)
  {
    DynList<string> tokens = split_to_list(str, " _-");

    tokens.mutable_for_each([](string & token)
                            {
                              token[0] = ::toupper(token[0]);
                            });

    return tokens.foldl<string>("", [](const string & acu, const string & token)
    {
      return acu + token;
    });
  }

  inline Array<std::string> split_to_array(const string & s,
                                           const string & delim)
  {
    return split_string<Array>(s, delim);
  }

  inline pair<string, string> split_pos(const string & str, size_t pos)
  {
    if (pos > str.size())
      {
        ostringstream s;
        s << "split_pos(" << str << ", " << pos << "): position " << pos
          << " is larger than string size " << str.size();
        throw range_error(s.str());
      }

    const char *ptr = str.data();
    const char *end1 = ptr + pos;

    string s1;
    while (ptr != end1)
      s1.push_back(*ptr++);

    string s2;
    while (*ptr)
      s2.push_back(*ptr++);

    return pair<string, string>(s1, s2);
  }

  inline DynList<string> split_n(const string & str, size_t n)
  {
    if (n > str.size())
      {
        ostringstream s;
        s << "split_n(" << str << ", " << n << "): number of parts " << n
          << " is larger than string size " << str.size();
        throw range_error(s.str());
      }

    size_t sz = str.size() / n;

    const char *ptr = str.data();

    DynList<string> ret;

    for (size_t i = 0; i < n; ++i)
      {
        string s;
        for (size_t k = 0; k < sz and *ptr; ++k)
          s.push_back(*ptr++);
        ret.append(s);
      }

    return ret;
  }

  template <typename T>
  inline DynList<DynList<T>> complete_rows(DynList<DynList<T>> & m)
  {
    size_t max_sz = 0;
    DynList<size_t> sizes =
      m.template maps<size_t>([&max_sz](auto & l)
                              {
                                const size_t sz = l.size();
                                max_sz = max(max_sz, sz);
                                return sz;
                              });

    DynList<DynList<T>> ret;
    for (auto it = zip_it(m, sizes); it.has_curr(); it.next_ne())
      {
        auto t = it.get_curr();
        DynList<T> & l = get<0>(t);
        size_t sz = get<1>(t);
        long n = max_sz - sz;
        for (auto i = 0; i < n; ++i)
          l.append(T());
        ret.append(move(l));
      }

    return ret;
  }

  /**
     @param[in] lens parallel row containing the maximum lenght og each colunm
  */
  inline DynList<DynList<string>>
  format_string(const DynList<size_t> & lens,
                const DynList<DynList<string>> & mat)
  {
    return mat.maps<DynList<string>>([&lens](const DynList<string> & l)
                                     {
                                       return zip(lens, l).template maps<string>([](auto p)
                                                                                 {
                                                                                   const string blanks(
                                                                                     p.first - p.second.size(), ' ');
                                                                                   return blanks + p.second + " ";
                                                                                 });
                                     });
  }

  inline ostream & format_string(ostream & out, const DynList<size_t> & lens,
                                 const DynList<DynList<string>> & mat)
  {
    for (auto it = mat.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & row = it.get_curr();
        auto line = zip(lens, row).
          template maps<string>([](auto p)
                                {
                                  const string blanks(p.first - p.second.size(), ' ');
                                  return blanks + p.second + " ";
                                });
        line.for_each([&out](const auto & s)
                      { out << s; });
        out << endl;
      }

    return out;
  }

  inline DynList<DynList<string>>
  format_string(const DynList<DynList<string>> & mat)
  {
    if (mat.is_empty())
      return mat;

    DynList<size_t> ilens = rep<size_t>(mat.nth(0).size(), 0);
    DynList<size_t> maxs =
      mat.foldl(ilens, [](const DynList<size_t> & acu,
                          const DynList<string> & l)
      {
        return zip(acu, l).
          maps<size_t>([](auto p)
                       { return max(p.first, p.second.size()); });
      });

    return format_string(maxs, mat);
  }

  inline DynList<DynList<string>>
  format_string_csv(const DynList<DynList<string>> & mat)
  {
    DynList<DynList<string>> ret;
    for (auto row_it = mat.get_it(); row_it.has_curr(); row_it.next_ne())
      {
        const DynList<string> & curr_row = row_it.get_curr();
        const string & last = curr_row.get_last();
        DynList<string> row;
        for (auto it = curr_row.get_it(); it.has_curr(); it.next_ne())
          {
            const string & s = it.get_curr();
            if (&s == &last)
              row.append(s);
            else
              row.append(s + ",");
          }
        ret.append(row);
      }

    return ret;
  }

  inline string to_string(const DynList<DynList<string>> & mat)
  {
    ostringstream s;
    mat.for_each([&s](const auto & row)
                 {
                   row.for_each([&s](const string & str)
                                { s << str; });
                   s << endl;
                 });
    return s.str();
  }

  inline DynList<string> split_text_into_words(const string & text)
  {
    DynList<string> words;
    istringstream in(text);
    for (string word; in >> word;)
      words.append(word);
    return words;
  }

  inline DynList<string> split_text_into_lines(const string & text)
  {
    return split_to_list(text, "\n");
  }

  inline string to_string(const DynList<string> & lines)
  {
    ostringstream s;
    for (auto it = lines.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & line = it.get_curr();
        s << line;
        if (line != lines.get_last())
          s << endl;
      }

    return s.str();
  }

  inline string justify_line(string line, const size_t page_width)
  {
    size_t pos = line.find_first_of(' ');
    if (pos != std::string::npos)
      {
        while (line.size() < page_width)
          {
            pos = line.find_first_not_of(' ', pos);
            line.insert(pos, " ");
            pos = line.find_first_of(' ', pos + 1);
            if (pos == string::npos)
              pos = line.find_first_of(' ');
          }
      }
    return line;
  }

  inline string justify_text(const string & text,
                             const size_t width,
                             const size_t left_margin = 0)
  {
    auto words = split_text_into_words(text);

    ostringstream s;
    string line;
    for (const string & word: words)
      if (line.size() + word.size() + 1 > width)
        {    // next word doesn't fit into the line
          s << string(left_margin, ' ') << justify_line(line, width) << endl;
          line.clear();
          line = word;
        }
      else
        {
          if (not line.empty())
            line.append(" ");
          line.append(word);
        }

    s << string(left_margin, ' ') << line;

    return s.str();
  }

  inline string justify_line_except_first(const string & text,
                                          const size_t width,
                                          const size_t left_margin = 0)
  {
    auto formatted = justify_text(text, width);
    auto lines = split_text_into_lines(formatted);
    ostringstream s;
    s << lines.remove_first() << endl;
    s << justify_text(to_string(lines), width, left_margin);
    return s.str();
  }

  inline string align_text_to_left(const string & text,
                                   const size_t page_width,
                                   const size_t left_margin = 0)
  {
    auto words = split_text_into_words(text);

    const string margin(left_margin, ' ');
    ostringstream s;
    string line;
    for (const string & word: words)
      if (line.size() + word.size() + 1 > page_width)
        {    // next word doesn't fit into the line
          s << margin << line << endl;
          line.clear();
          line = word;
        }
      else
        {
          if (not line.empty())
            line.append(" ");
          line.append(word);
        }

    s << margin << line;

    return s.str();
  }

  inline string align_text_to_left_except_first(const string & text,
                                                const size_t width,
                                                const size_t left_margin = 0)
  {
    auto formatted = align_text_to_left(text, width);
    auto lines = split_text_into_lines(formatted);
    ostringstream s;
    s << lines.remove_first() << endl;
    s << align_text_to_left(to_string(lines), width, left_margin);
    return s.str();
  }

  inline string shift_lines_to_left(const string & str, const size_t n)
  {
    ostringstream s;
    string fill(n, ' ');
    auto lines = split_to_list(str, "\n");
    for (auto it = lines.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & line = it.get_curr();
        s << fill << line;
        if (line != lines.get_last())
          s << endl;
      }
    return s.str();
  }

  inline void build_pars_list(string &)
  {}

  template <typename T>
  inline void build_pars_list(string & str, const T & item)
  {
    ostringstream s;
    s << ", " << item;
    str.append(s.str());
  }

  template <class T, class ... Args>
  inline void build_pars_list(string & str, const T & item, Args ... args)
  {
    ostringstream s;
    s << item;
    str.append(s.str());
    build_pars_list(str, args...);
  }

  /** Build a sringficated parameters' lists

   */
  template <class T, class ... Args>
  inline string build_pars_list(Args ... args)
  {
    string ret;
    build_pars_list(ret, args...);
    return ret;
  }

  template <class C>
  inline string build_pars_list(const C & c, const string & sep = ",")
  {
    if (c.is_empty())
      return "";

    auto & last = c.get_last();
    ostringstream s;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      {
        auto & curr = it.get_curr();
        s << curr;
        if (&curr != &last)
          s << sep;
      }
    return s.str();
  }

}

# endif // AH_STRING_UTILS_H

