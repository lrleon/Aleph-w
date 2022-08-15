
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
# ifndef MAT_LATEX_H
# define MAT_LATEX_H

# include <fstream>


namespace Aleph
{

  template <typename Mat, 
	    class    _W_i,  // formato de índice de fila
	    class    _W_j,  // formato de índice de columna
	    class    _W_ij> // formato de entrada de a matriz
void mat_to_latex(Mat &          mat, 
		  const long &   n, 
		  const long &   m, 
		  ofstream &     out, 
		  const string & prefix = "", 
		  const string & suffix = "")
{
  out << prefix;
  out << "\\begin{tabular}{|c|";
  for (int i = 0; i < m; ++i)
    out << "p{1mm}";
  out << "|} \\hline" << endl
       << "& ";

  for (int i = 0; i < m; ++i)
    {
      out << _W_j () (mat, i);

      if (i < m - 1)
	out << " & ";
      else
	out << "\\\\ \\hline" << endl;
    }

  for (int i = 0; i < n; ++i)
    {
      out << _W_i () (mat, i) << " & ";
      for (int j = 0; j < m; ++j)
	{
	  out << _W_ij () (mat, i, j);

	  if (j < m -1)
	    out << " & ";
	}

      out << "\\\\";
      
      out << endl;
    }

  out << "\\hline" 
    << "\\end{tabular}" << endl
      << suffix;
}

} // end namespace Aleph


# endif // MAT_LATEX_H
