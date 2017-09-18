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
