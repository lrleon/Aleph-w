# include <string>
# include <iostream>
# include <fstream>

using namespace std;

void search_noascii_in_stream(ifstream & input_stream)
{
  int line_number = 1;
  int col_number = 0;
  char c;

  while (true)
    {
      col_number++;
      c = input_stream.get();

      if (c == '\n')
	{
	  line_number++;
	  col_number = 1;
	}

      if (c == EOF)
	return;

      switch (c)
	{
	case -38: case -45: case -51: case -55: case -63: case -47: 
	case -15: case -6: case -13: case -19: case -23: case -31: 
	case '\n': case '\\': case '[': case ']': case '{': case '}': 
	  cout << c;
	  break;
	default:
	  if ((c > 'a' and c < 'z') or (c > 'A' and c < 'Z') or
	      (c > '0' and c < '9') or (c > 30 and c < 128))
	    cout << c;
	  break;
	  cout << "Caracter " << c << " (" << (int)c 
	       << ") es problematico en columna " 
	       << col_number << " en línea " << line_number << endl;
	}
    }
}


int main(int argn, char * argc[])
{
  ifstream input(argc[1]);

  search_noascii_in_stream(input);
}
