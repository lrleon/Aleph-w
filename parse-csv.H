# ifndef PARSE_CSV_H
# define PARSE_CSV_H

# include <sstream>
# include <istream>
# include <string>
# include <tpl_array.H>

using namespace std;

inline Array<string> csv_read_row(istream & in, char delimiter = ',')
{
  stringstream ss;
  bool inquotes = false;
  Array<string> row; // relying on RVO
  while (in.good())
    {
      char c = in.get();
      if (not inquotes and c == '"') // begin quote char
	inquotes = true;
      else if (inquotes and c == '"') // quote char
	{
	  if (in.peek() == '"') // 2 consecutive quotes resolve to 1
	    ss << (char) in.get();
	  else // endquotechar
	    inquotes = false;
	}
      else if (not inquotes and c == delimiter) // end of field
	{
	  row.append(ss.str());
	  ss.str("");
	}
      else if (not inquotes and (c == '\r' or c == '\n') )
	{
	  if (in.peek() == '\n') 
	    in.get(); 
	  row.append(ss.str());
	  return row;
	}
      else
	ss << c;
    }
  return row;
}

inline Array<string> csv_read_row(string & line, char delimiter)
{
  stringstream ss(line);
  return csv_read_row(ss, delimiter);
}


# endif // PARSE_CSV_H
