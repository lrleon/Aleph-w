
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
#ifndef TPL_DYNMAT_H
#define TPL_DYNMAT_H

# include <ahDry.H>
# include <tpl_dynArray.H>

namespace Aleph {

    /** Dynamic matrix.

	This class implents a rectangular dynamic matrix of generic type `T`.

	By default the entries contain the value `DynMatrix<T>::Zero`,
	which is the default constructed value.

	@see DynArray
        @ingroup Secuencias
     */
    template <typename T> 
class DynMatrix 
      : public LocateFunctions<DynMatrix<T>, T>,
	public FunctionalMethods<DynMatrix<T>, T>
{
  size_t n = 0;
  size_t m = 0;

  DynArray<T> * array_ptr = nullptr;

  mutable T Zero = T();

  const T & read_array_entry(const size_t i) const noexcept
  {
    T * entry_ptr = array_ptr->test(i);

    return entry_ptr == nullptr ? Zero : *entry_ptr;
  }

  T & write_array_entry(const size_t i, const T & data)
  {
    T & ref = array_ptr->touch(i) = data;

    return ref;
  }

  size_t get_index(const size_t i, const size_t j) const noexcept
  {
    return i*m + j;
  }

public:

  /// Set the initial or zero value to `value`
  void set_default_initial_value(const T & value)
  {
    array_ptr->set_default_initial_value(value);
  }

  /// Swap `mat` with `this`
  void swap(DynMatrix & mat)
  {
    std::swap(n, mat.n);
    std::swap(m, mat.m);
    std::swap(Zero, mat.Zero);
    std::swap(array_ptr, mat.array_ptr);
  }

      /// Change the matrix dimensions
  void set_dimension(const size_t __n, const size_t __m)
  {
    if (array_ptr != nullptr) // se verifica porque puede llamarse desde el ctor
      delete array_ptr;

    n = __n;
    m = __m;
    size_t d, s, b;
    const size_t N = n*m;
    DynArray<T>::compute_sizes(N, d, s, b);

    array_ptr = new DynArray<T> (d, s, b);
    array_ptr->set_default_initial_value(Zero);
  }

  /// Allocate all the memory needed for storing the full matrix. Throw
  /// `bad_alloc` if there is no enough memory
  void allocate()
  {
    array_ptr->reserve(n*m);
  }

      /** Construct a matrix

	  @param[in] __n number of rows
	  @param[in] __m number of columns
	  @param[in] zero default value of each entry
	  @throw bad_alloc if there is no enough memory
       */
  DynMatrix(const size_t __n, const size_t __m, const T & zero = T())
    : n(__n), m(__m), array_ptr(nullptr), Zero(zero)
  {
    set_dimension(n, m);
  }

  ~DynMatrix()
  {
    if (array_ptr)
      delete array_ptr;
  }

  ///  Copy constructor
  DynMatrix(const DynMatrix<T> & mat) 
    : DynMatrix(mat.n, mat.m, mat.Zero)
  {
    *array_ptr = *mat.array_ptr;
  }

  /// Move constructor
  DynMatrix(DynMatrix<T> && mat)
  {
    swap(mat);
  }

  /// Copy assign
  DynMatrix<T> & operator = (const DynMatrix<T> & mat) 
  {
    if (this == &mat)
      return *this;

    n = mat.n;
    m = mat.m;
    array_ptr->copy_array(*mat.array_ptr);

    return *this;
  }

  /// Move assign
  DynMatrix<T> operator = (DynMatrix<T> && mat) 
  {
    swap(mat);
    return *this;
  }

  /// Return `true` if `this` is equal to `mat`. It is based on `T::operator ==`
  bool operator == (const DynMatrix<T> & mat) const
  {
    const size_t N = n*m;
    for (int i = 0; i < N; ++i)
      if (read_array_entry(i) != mat.read_array_entry(i))
	return false;

    return true;
  }

  /// Return the number of rows
  const size_t & rows() const noexcept { return n; }

      /// Retorna la cantidad de columnas de la matriz
  const size_t & cols() const noexcept { return m; }

      /// Read the entry at the row `i` and column `j`
  const T & read(const size_t i, const size_t j) const
  {
    assert(i < n and j < m);

    return read_array_entry(get_index(i, j));
  }

      /** Write the entry at the row `i` and column `j`
	  
	  @param[in] i row index
	  @param[in] j column index
	  @param[in] data to be written
	  @return a modifiable reference to the data in the matrix
	  @throw bad_alloc if there is no enough memory
       */
  T & write(const size_t i, const size_t j, const T & data)
  {
    assert(i < n and j < m);

    return write_array_entry(get_index(i, j), data);
  }

  /// Fast access to i-th row j-th column
  T & access(const size_t i, const size_t j) const
  {
    return array_ptr->access(get_index(i, j));
  }

  /// \overload access(const size_t i, const size_t j) 
  T & operator () (const size_t i, const size_t j) const
  {
    return access(i, j);
  }
  
  /** Traverse all the items of matrix.
      
      The visit order is by row starting from the row 0 until the
      last. Each column is visited from the 0 index to the last.
  */
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return array_ptr->traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) 
  {
    return array_ptr->traverse(operation); 
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation) const
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation) 
  {
    return traverse<Operation>(operation);
  }
};

} // end namespace Aleph

#endif // TPL_DYNMAT_H
