
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file mat_latex.H
 *  @brief Matrix to LaTeX table conversion utilities.
 *
 *  This file provides utilities for generating LaTeX tabular representations
 *  of matrices. It is particularly useful for visualizing adjacency matrices,
 *  distance matrices from Floyd-Warshall, and path matrices.
 *
 *  ## Features
 *
 *  - Customizable row and column index formatting
 *  - Customizable cell content formatting
 *  - Support for prefix and suffix LaTeX content
 *  - Works with any matrix-like container
 *
 *  ## Usage Example
 *
 *  @code{.cpp}
 *  #include <mat_latex.H>
 *  #include <fstream>
 *
 *  // Formatter for row indices
 *  struct RowFormatter {
 *    std::string operator()(const MyMatrix& m, long i) const {
 *      return std::to_string(i);
 *    }
 *  };
 *
 *  // Formatter for column indices
 *  struct ColFormatter {
 *    std::string operator()(const MyMatrix& m, long j) const {
 *      return std::to_string(j);
 *    }
 *  };
 *
 *  // Formatter for matrix entries
 *  struct EntryFormatter {
 *    std::string operator()(const MyMatrix& m, long i, long j) const {
 *      return std::to_string(m(i, j));
 *    }
 *  };
 *
 *  std::ofstream out("matrix.tex");
 *  mat_to_latex<MyMatrix, RowFormatter, ColFormatter, EntryFormatter>(
 *      matrix, n, m, out, "\\begin{center}\n", "\\end{center}\n");
 *  @endcode
 *
 *  ## Generated LaTeX Format
 *
 *  The function generates a tabular environment with:
 *  - Column headers from the column formatter
 *  - Row headers from the row formatter
 *  - Cell contents from the entry formatter
 *  - Horizontal lines at top and bottom
 *
 *  @see latex_floyd.H For Floyd-Warshall specific LaTeX output
 *  @see tpl_matgraph.H For matrix-based graph representations
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

#ifndef MAT_LATEX_H
#define MAT_LATEX_H

#include <fstream>
#include <ostream>
#include <string>

namespace Aleph
{

/** @brief Generate LaTeX tabular representation of a matrix.
 *
 *  Converts a matrix to a LaTeX tabular environment with customizable
 *  formatting for row indices, column indices, and matrix entries.
 *
 *  @tparam Mat Matrix type (must support operator()(long, long) for entry access)
 *  @tparam RowIndexFormatter Functor type for formatting row indices.
 *         Must provide: `std::string operator()(Mat&, long) const`
 *  @tparam ColIndexFormatter Functor type for formatting column indices.
 *         Must provide: `std::string operator()(Mat&, long) const`
 *  @tparam EntryFormatter Functor type for formatting matrix entries.
 *         Must provide: `std::string operator()(Mat&, long, long) const`
 *
 *  @param[in] mat The matrix to convert (passed by reference for efficiency)
 *  @param[in] n Number of rows
 *  @param[in] m Number of columns
 *  @param[out] out Output stream to write LaTeX content
 *  @param[in] prefix Optional LaTeX content to prepend (e.g., "\\begin{center}")
 *  @param[in] suffix Optional LaTeX content to append (e.g., "\\end{center}")
 *
 *  @note The column width is set to 1mm by default. For larger content,
 *        consider modifying the generated LaTeX or using a different approach.
 *
 *  @par Example Output
 *  @code{.tex}
 *  \begin{tabular}{|c|p{1mm}p{1mm}p{1mm}|} \hline
 *  & 0 & 1 & 2\\ \hline
 *  0 & 0 & 5 & 3\\
 *  1 & 5 & 0 & 2\\
 *  2 & 3 & 2 & 0\\
 *  \hline\end{tabular}
 *  @endcode
 *
 *  @ingroup Utilities
 */
template <typename Mat,
          class RowIndexFormatter,
          class ColIndexFormatter,
          class EntryFormatter>
void mat_to_latex(Mat & mat,
                  long n,
                  long m,
                  std::ostream & out,
                  const std::string & prefix = "",
                  const std::string & suffix = "")
{
  out << prefix;
  out << "\\begin{tabular}{|c|";
  
  for (long j = 0; j < m; ++j)
    out << "p{1mm}";
  
  out << "|} \\hline\n& ";

  // Column headers
  for (long j = 0; j < m; ++j)
    {
      out << ColIndexFormatter()(mat, j);
      if (j < m - 1)
        out << " & ";
      else
        out << "\\\\ \\hline\n";
    }

  // Matrix rows
  for (long i = 0; i < n; ++i)
    {
      out << RowIndexFormatter()(mat, i) << " & ";
      
      for (long j = 0; j < m; ++j)
        {
          out << EntryFormatter()(mat, i, j);
          if (j < m - 1)
            out << " & ";
        }

      out << "\\\\\n";
    }

  out << "\\hline\\end{tabular}\n" << suffix;
}

/** @brief Overload for std::ofstream output.
 *
 *  Convenience overload that accepts std::ofstream directly.
 *
 *  @see mat_to_latex(Mat&, long, long, std::ostream&, const std::string&, const std::string&)
 *  @ingroup Utilities
 */
template <typename Mat,
          class RowIndexFormatter,
          class ColIndexFormatter,
          class EntryFormatter>
void mat_to_latex(Mat & mat,
                  long n,
                  long m,
                  std::ofstream & out,
                  const std::string & prefix = "",
                  const std::string & suffix = "")
{
  mat_to_latex<Mat, RowIndexFormatter, ColIndexFormatter, EntryFormatter>(
      mat, n, m, static_cast<std::ostream&>(out), prefix, suffix);
}

/** @brief Default row index formatter.
 *
 *  Formats row indices as simple integers.
 *
 *  @tparam Mat Matrix type
 *  @ingroup Utilities
 */
template <typename Mat>
struct Default_Row_Formatter
{
  std::string operator()(Mat &, long i) const
  {
    return std::to_string(i);
  }
};

/** @brief Default column index formatter.
 *
 *  Formats column indices as simple integers.
 *
 *  @tparam Mat Matrix type
 *  @ingroup Utilities
 */
template <typename Mat>
struct Default_Col_Formatter
{
  std::string operator()(Mat &, long j) const
  {
    return std::to_string(j);
  }
};

/** @brief Default entry formatter.
 *
 *  Formats matrix entries using stream insertion operator.
 *  Requires Mat::Entry_Type to be streamable.
 *
 *  @tparam Mat Matrix type with Entry_Type typedef
 *  @ingroup Utilities
 */
template <typename Mat>
struct Default_Entry_Formatter
{
  std::string operator()(Mat & mat, long i, long j) const
  {
    std::ostringstream ss;
    ss << mat(i, j);
    return ss.str();
  }
};

/** @brief Generate LaTeX table with default formatters.
 *
 *  Simplified version using default integer formatters for indices
 *  and stream-based formatting for entries.
 *
 *  @tparam Mat Matrix type
 *  @param mat The matrix to convert
 *  @param n Number of rows
 *  @param m Number of columns
 *  @param out Output stream
 *  @param prefix Optional LaTeX prefix
 *  @param suffix Optional LaTeX suffix
 *
 *  @ingroup Utilities
 */
template <typename Mat>
void mat_to_latex_simple(Mat & mat,
                         long n,
                         long m,
                         std::ostream & out,
                         const std::string & prefix = "",
                         const std::string & suffix = "")
{
  mat_to_latex<Mat,
               Default_Row_Formatter<Mat>,
               Default_Col_Formatter<Mat>,
               Default_Entry_Formatter<Mat>>(mat, n, m, out, prefix, suffix);
}

} // namespace Aleph

// Global namespace compatibility
using Aleph::mat_to_latex;
using Aleph::mat_to_latex_simple;
using Aleph::Default_Row_Formatter;
using Aleph::Default_Col_Formatter;
using Aleph::Default_Entry_Formatter;

#endif // MAT_LATEX_H
