
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


/** @file tpl_dynMat.H
 *  @brief Dynamic matrix with lazy allocation.
 *
 *  2D matrix using DynArray for flexible, sparse storage.
 *  Only allocates cells that are actually accessed.
 *
 *  ## Features
 *  - Sparse matrix support
 *  - Row/column resizing
 *  - Matrix arithmetic operations
 *
 *  @see tpl_dynArray.H Underlying storage
 *  @see tpl_matgraph.H Graph adjacency matrices
 *  @ingroup Matrices
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_DYNMAT_H
#define TPL_DYNMAT_H

#include <ah-errors.H>
#include <ahDry.H>
#include <tpl_dynArray.H>

namespace Aleph {

/** @brief Dynamic matrix with sparse storage.
 *
 *  DynMatrix<T> implements a two-dimensional matrix of generic type T with
 *  dynamic, sparse storage. Only entries that are explicitly written consume
 *  memory; unwritten entries return the default "zero" value.
 *
 *  ## Features
 *
 *  - **Sparse storage**: Only written entries consume memory
 *  - **Dynamic dimensions**: Can be resized at runtime
 *  - **Row-major order**: Elements are stored in row-major order for cache efficiency
 *  - **Functional interface**: Supports traverse, maps, filter, foldl, etc.
 *  - **Customizable zero value**: Default value for unwritten entries
 *
 *  ## Example Usage
 *
 *  @code
 *  // Create a 3x4 matrix with default value 0.0
 *  DynMatrix<double> mat(3, 4, 0.0);
 *
 *  // Write some entries
 *  mat.write(0, 0, 1.0);
 *  mat.write(1, 2, 5.0);
 *
 *  // Read entries (unwritten returns 0.0)
 *  double a = mat.read(0, 0);  // 1.0
 *  double b = mat.read(0, 1);  // 0.0 (default)
 *
 *  // Using operator() for allocated entries
 *  mat.allocate();  // Allocate all entries
 *  mat(2, 3) = 7.0;
 *
 *  // Traverse all written entries
 *  mat.traverse([](double& val) {
 *    std::cout << val << " ";
 *    return true;
 *  });
 *  @endcode
 *
 *  ## Memory Model
 *
 *  The matrix uses a DynArray<T> internally, which provides:
 *  - O(1) access to any entry (amortized)
 *  - Memory allocation only for written entries
 *  - Efficient traversal of all written entries
 *
 *  ## Thread Safety
 *
 *  This class is not thread-safe. External synchronization is required
 *  for concurrent access from multiple threads.
 *
 *  @tparam T Type of elements stored in the matrix. Must be default-constructible.
 *
 *  @see DynArray The underlying storage container
 *
 *  @ingroup Sequences
 */
template <typename T>
class DynMatrix
    : public LocateFunctions<DynMatrix<T>, T>,
      public FunctionalMethods<DynMatrix<T>, T>
{
  size_t n = 0;  ///< Number of rows
  size_t m = 0;  ///< Number of columns

  DynArray<T> * array_ptr = nullptr;

  T default_value = T();

  [[nodiscard]] const T & read_array_entry(const size_t i) const noexcept
  {
    T * entry_ptr = array_ptr->test(i);
    return entry_ptr == nullptr ? default_value : *entry_ptr;
  }

  T & write_array_entry(const size_t i, const T & data)
  {
    T & ref = array_ptr->touch(i) = data;
    return ref;
  }

  T & write_array_entry(const size_t i, T && data)
  {
    T & ref = array_ptr->touch(i) = std::move(data);
    return ref;
  }

  [[nodiscard]] constexpr size_t get_index(const size_t i,
                                           const size_t j) const noexcept
  {
    return i * m + j;
  }

  void check_indices(const size_t i, const size_t j) const
  {
    ah_out_of_range_error_if(i >= n)
      << "DynMatrix: row index " << i << " out of range [0, " << n << ")";
    ah_out_of_range_error_if(j >= m)
      << "DynMatrix: column index " << j << " out of range [0, " << m << ")";
  }

public:

  /// @brief The container type (self-reference for generic programming).
  using Set_Type = DynMatrix<T>;

  /// @brief The type of elements stored in the matrix.
  using Item_Type = T;

  /// @brief Alias for Item_Type, required by functional mixins.
  using Key_Type = T;

  /** @brief Set the default value for unwritten entries.
   *
   *  Changes the value returned for entries that have not been explicitly
   *  written. This also affects new entries created by touch().
   *
   *  @param[in] value The new default value.
   *
   *  @note This does not modify existing entries; it only affects
   *        reads of unwritten positions and newly allocated entries.
   */
  void set_default_initial_value(const T & value)
  {
    default_value = value;
    if (array_ptr)
      array_ptr->set_default_initial_value(value);
  }

  /** @brief Get the default value for unwritten entries.
   *
   *  @return The current default value.
   */
  [[nodiscard]] const T & get_default_value() const noexcept
  {
    return default_value;
  }

  /** @brief Swap the contents of this matrix with another.
   *
   *  Exchanges the contents of this matrix with those of @p mat in
   *  constant time O(1). No elements are copied or moved individually.
   *
   *  @param[in,out] mat The matrix to swap with.
   */
  void swap(DynMatrix & mat) noexcept
  {
    std::swap(n, mat.n);
    std::swap(m, mat.m);
    std::swap(default_value, mat.default_value);
    std::swap(array_ptr, mat.array_ptr);
  }

  /** @brief Change the matrix dimensions.
   *
   *  Resizes the matrix to have @p num_rows rows and @p num_cols columns.
   *  All existing data is discarded.
   *
   *  @param[in] num_rows The new number of rows.
   *  @param[in] num_cols The new number of columns.
   *
   *  @throw std::bad_alloc If memory allocation fails.
   *
   *  @warning This operation discards all existing data. If you need to
   *           preserve data, copy it before resizing.
   */
  void set_dimension(const size_t num_rows, const size_t num_cols)
  {
    delete array_ptr;

    n = num_rows;
    m = num_cols;
    size_t d, s, b;
    const size_t N = n * m;
    DynArray<T>::compute_sizes(N, d, s, b);

    array_ptr = new DynArray<T>(d, s, b);
    array_ptr->set_default_initial_value(default_value);
  }

  /** @brief Pre-allocate memory for the entire matrix.
   *
   *  Allocates memory for all n*m entries, allowing O(1) access via
   *  operator() without triggering additional allocations.
   *
   *  @throw std::bad_alloc If there is not enough memory.
   *
   *  @note After calling allocate(), operator() can be used safely
   *        for any valid index without checking for allocation.
   */
  void allocate()
  {
    array_ptr->reserve(n * m);
  }

  /** @brief Construct a matrix with specified dimensions.
   *
   *  Creates a matrix with @p num_rows rows and @p num_cols columns. All entries
   *  are initialized to @p zero (default: T()).
   *
   *  @param[in] num_rows Number of rows (must be > 0).
   *  @param[in] num_cols Number of columns (must be > 0).
   *  @param[in] zero Default value for unwritten entries.
   *
   *  @throw std::bad_alloc If memory allocation fails.
   *  @throw std::invalid_argument If num_rows or num_cols is zero.
   */
  DynMatrix(const size_t num_rows, const size_t num_cols, const T & zero = T())
    : n(num_rows), m(num_cols), array_ptr(nullptr), default_value(zero)
  {
    ah_invalid_argument_if(num_rows == 0 || num_cols == 0)
      << "DynMatrix dimensions must be > 0";
    set_dimension(n, m);
  }

  /** @brief Default constructor - creates an invalid matrix.
   *
   *  Creates a matrix with zero dimensions. The matrix must be
   *  initialized with set_dimension() before use.
   *
   *  @note This constructor is provided primarily for container compatibility.
   *        Most operations will fail on an uninitialized matrix.
   */
  DynMatrix() noexcept = default;

  /** @brief Destructor - releases all allocated memory.
   */
  ~DynMatrix()
  {
    delete array_ptr;
  }

  /** @brief Copy constructor.
   *
   *  Creates a deep copy of @p mat.
   *
   *  @param[in] mat The matrix to copy.
   *
   *  @throw std::bad_alloc If memory allocation fails.
   */
  DynMatrix(const DynMatrix<T> & mat)
    : n(mat.n), m(mat.m), array_ptr(nullptr), default_value(mat.default_value)
  {
    if (mat.array_ptr)
      {
        set_dimension(n, m);
        *array_ptr = *mat.array_ptr;
      }
  }

  /** @brief Move constructor.
   *
   *  Transfers ownership of resources from @p mat to this matrix.
   *  After this operation, @p mat is left in a valid but unspecified state.
   *
   *  @param[in,out] mat The matrix to move from.
   */
  DynMatrix(DynMatrix<T> && mat) noexcept
    : n(mat.n),
      m(mat.m),
      array_ptr(mat.array_ptr),
      default_value(std::move(mat.default_value))
  {
    mat.n = mat.m = 0;
    mat.array_ptr = nullptr;
    mat.default_value = T();
  }

  /** @brief Copy assignment operator.
   *
   *  Replaces the contents of this matrix with a copy of @p mat.
   *
   *  @param[in] mat The matrix to copy.
   *  @return Reference to this matrix.
   *
   *  @throw std::bad_alloc If memory allocation fails.
   */
  DynMatrix<T> & operator = (const DynMatrix<T> & mat)
  {
    if (this == &mat)
      return *this;

    default_value = mat.default_value;

    if (n != mat.n || m != mat.m)
      set_dimension(mat.n, mat.m);

    if (mat.array_ptr)
      array_ptr->copy_array(*mat.array_ptr);

    return *this;
  }

  /** @brief Move assignment operator.
   *
   *  Transfers ownership of resources from @p mat to this matrix.
   *
   *  @param[in,out] mat The matrix to move from.
   *  @return Reference to this matrix.
   */
  DynMatrix<T> & operator = (DynMatrix<T> && mat) noexcept
  {
    if (this == &mat)
      return *this;

    delete array_ptr;

    n = mat.n;
    m = mat.m;
    default_value = std::move(mat.default_value);
    array_ptr = mat.array_ptr;

    mat.n = mat.m = 0;
    mat.array_ptr = nullptr;
    mat.default_value = T();

    return *this;
  }

  /** @brief Equality comparison.
   *
   *  Returns true if this matrix has the same dimensions as @p mat and
   *  all corresponding entries are equal (using T::operator==).
   *
   *  @param[in] mat The matrix to compare with.
   *  @return true if matrices are equal, false otherwise.
   */
  [[nodiscard]] bool operator == (const DynMatrix<T> & mat) const
  {
    if (n != mat.n || m != mat.m)
      return false;

    const size_t N = n * m;
    for (size_t i = 0; i < N; ++i)
      if (read_array_entry(i) != mat.read_array_entry(i))
        return false;

    return true;
  }

  /** @brief Inequality comparison.
   *
   *  @param[in] mat The matrix to compare with.
   *  @return true if matrices are not equal, false otherwise.
   */
  [[nodiscard]] bool operator != (const DynMatrix<T> & mat) const
  {
    return !(*this == mat);
  }

  /** @brief Get the number of rows.
   *
   *  @return The number of rows in the matrix.
   */
  [[nodiscard]] constexpr size_t rows() const noexcept { return n; }

  /** @brief Get the number of columns.
   *
   *  @return The number of columns in the matrix.
   */
  [[nodiscard]] constexpr size_t cols() const noexcept { return m; }

  /** @brief Get the total number of entries.
   *
   *  @return rows() * cols()
   */
  [[nodiscard]] constexpr size_t size() const noexcept { return n * m; }

  /** @brief Check if the matrix is square.
   *
   *  @return true if rows() == cols(), false otherwise.
   */
  [[nodiscard]] constexpr bool is_square() const noexcept { return n == m; }

  /** @brief Check if the matrix is empty (uninitialized).
   *
   *  @return true if dimensions are zero, false otherwise.
   */
  [[nodiscard]] constexpr bool is_empty() const noexcept { return n == 0 || m == 0; }

  /** @brief Read the entry at position (i, j).
   *
   *  Returns the value at row @p i and column @p j. If the entry has not
   *  been written, returns the default value.
   *
   *  Time complexity: O(1) amortized.
   *
   *  @param[in] i Row index (0-based).
   *  @param[in] j Column index (0-based).
   *  @return The value at position (i, j).
   *
   *  @throw std::out_of_range If i >= rows() or j >= cols().
   */
  [[nodiscard]] const T & read(const size_t i, const size_t j) const
  {
    check_indices(i, j);
    return read_array_entry(get_index(i, j));
  }

  /** @brief Read entry without bounds checking.
   *
   *  @param[in] i Row index.
   *  @param[in] j Column index.
   *  @return The value at position (i, j).
   *
   *  @warning Behavior is undefined if indices are out of bounds.
   */
  [[nodiscard]] const T & read_ne(const size_t i, const size_t j) const noexcept
  {
    return read_array_entry(get_index(i, j));
  }

  /** @brief Write a value to position (i, j).
   *
   *  Sets the value at row @p i and column @p j. Allocates memory for
   *  the entry if not already allocated.
   *
   *  Time complexity: O(1) amortized.
   *
   *  @param[in] i Row index (0-based).
   *  @param[in] j Column index (0-based).
   *  @param[in] data The value to write.
   *  @return A modifiable reference to the written value.
   *
   *  @throw std::out_of_range If i >= rows() or j >= cols().
   *  @throw std::bad_alloc If memory allocation fails.
   */
  T & write(const size_t i, const size_t j, const T & data)
  {
    check_indices(i, j);
    return write_array_entry(get_index(i, j), data);
  }

  /// @overload write(const size_t i, const size_t j, const T & data)
  T & write(const size_t i, const size_t j, T && data)
  {
    check_indices(i, j);
    return write_array_entry(get_index(i, j), std::move(data));
  }

  /** @brief Direct access to an allocated entry.
   *
   *  Returns a reference to the entry at position (i, j). The entry must
   *  have been previously written or the matrix must have been allocated
   *  with allocate().
   *
   *  Time complexity: O(1).
   *
   *  @param[in] i Row index (0-based).
   *  @param[in] j Column index (0-based).
   *  @return A modifiable reference to the entry.
   *
   *  @warning Behavior is undefined if the entry has not been allocated.
   *           Call allocate() first or use write() for safe access.
   */
  [[nodiscard]] T & access(const size_t i, const size_t j)
  {
    return array_ptr->access(get_index(i, j));
  }

  /** @brief Direct const access to an allocated entry.
   *
   *  @param[in] i Row index.
   *  @param[in] j Column index.
   *  @return A const reference to the entry.
   */
  [[nodiscard]] const T & access(const size_t i, const size_t j) const
  {
    return array_ptr->access(get_index(i, j));
  }

  /** @brief Subscript operator for allocated entries.
   *
   *  Equivalent to access(i, j).
   *
   *  @param[in] i Row index.
   *  @param[in] j Column index.
   *  @return A modifiable reference to the entry.
   */
  [[nodiscard]] T & operator () (const size_t i, const size_t j)
  {
    return access(i, j);
  }

  /** @brief Const subscript operator for allocated entries.
   *
   *  @param[in] i Row index.
   *  @param[in] j Column index.
   *  @return A const reference to the entry.
   */
  [[nodiscard]] const T & operator () (const size_t i, const size_t j) const
  {
    return access(i, j);
  }

  /** @brief Fill all entries with a value.
   *
   *  Sets all entries to @p value. This allocates memory for all entries.
   *
   *  @param[in] value The value to fill with.
   *
   *  @throw std::bad_alloc If memory allocation fails.
   */
  void fill(const T & value)
  {
    allocate();
    for (size_t idx = 0; idx < n * m; ++idx)
      array_ptr->access(idx) = value;
  }

  /** @brief Create a transposed copy of the matrix.
   *
   *  Returns a new matrix where entry (i,j) contains this->read(j,i).
   *
   *  @return The transposed matrix.
   *
   *  @throw std::bad_alloc If memory allocation fails.
   */
  [[nodiscard]] DynMatrix<T> transpose() const
  {
    DynMatrix<T> result(m, n, default_value);
    for (size_t i = 0; i < n; ++i)
      for (size_t j = 0; j < m; ++j)
        {
          const T & val = read_array_entry(get_index(i, j));
          if (&val != &default_value)  // Only copy non-default entries
            result.write(j, i, val);
        }
    return result;
  }

  /** @brief Iterator for traversing matrix elements.
   *
   *  Provides forward iteration over all matrix positions in row-major order.
   *  The iterator visits positions (0,0), (0,1), ..., (0,m-1), (1,0), ...
   *
   *  @note This iterator visits all logical positions, not just written entries.
   *        Use the underlying DynArray iterator for sparse traversal.
   */
  class Iterator
  {
    const DynMatrix * mat_ptr = nullptr;
    size_t curr_idx = 0;

  public:

    /// @brief The container type this iterator operates on.
    using Set_Type = DynMatrix<T>;

    /** @brief Construct an iterator for a matrix.
     *
     *  @param[in] mat The matrix to iterate over.
     */
    Iterator(const DynMatrix<T> & mat) noexcept
      : mat_ptr(&mat), curr_idx(0) {}

    /** @brief Check if iterator has a current element.
     *
     *  @return true if positioned at a valid element, false otherwise.
     */
    [[nodiscard]] bool has_curr() const noexcept
    {
      return curr_idx < mat_ptr->size();
    }

    /** @brief Get the current element without bounds checking.
     *
     *  @return Reference to the current element.
     */
    [[nodiscard]] const T & get_curr_ne() const noexcept
    {
      return mat_ptr->read_array_entry(curr_idx);
    }

    /** @brief Get the current element.
     *
     *  @return Reference to the current element.
     *  @throw std::overflow_error If iterator is exhausted.
     */
    [[nodiscard]] const T & get_curr() const
    {
      ah_overflow_error_unless(has_curr())
        << "DynMatrix::Iterator: no current element";
      return get_curr_ne();
    }

    /** @brief Advance to the next element without bounds checking.
     */
    void next_ne() noexcept { ++curr_idx; }

    /** @brief Advance to the next element.
     *
     *  @throw std::overflow_error If iterator is already exhausted.
     */
    void next()
    {
      ah_overflow_error_unless(has_curr())
        << "DynMatrix::Iterator: no more elements";
      next_ne();
    }

    /** @brief Get the current row index.
     *
     *  @return The row index of the current position.
     */
    [[nodiscard]] size_t get_row() const noexcept
    {
      return curr_idx / mat_ptr->m;
    }

    /** @brief Get the current column index.
     *
     *  @return The column index of the current position.
     */
    [[nodiscard]] size_t get_col() const noexcept
    {
      return curr_idx % mat_ptr->m;
    }

    /** @brief Reset iterator to the beginning.
     */
    void reset() noexcept { curr_idx = 0; }
  };

  /** @brief Get an iterator positioned at the first element.
   *
   *  @return An iterator at position (0, 0).
   */
  [[nodiscard]] Iterator get_it() const { return Iterator(*this); }

  /** @brief Traverse all elements in row-major order.
   *
   *  Executes @p operation on each element. The traversal stops early
   *  if @p operation returns false.
   *
   *  @tparam Operation A callable with signature `bool(const T&)`.
   *  @param[in] operation The operation to perform.
   *  @return true if all elements were visited, false if stopped early.
   */
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    for (size_t i = 0; i < n * m; ++i)
      if (!operation(read_array_entry(i)))
        return false;
    return true;
  }

  /// @overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation)
  {
    return const_cast<const DynMatrix*>(this)->traverse(operation);
  }

  /// @overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation) const
  {
    return traverse<Operation>(operation);
  }

  /// @overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation)
  {
    return traverse<Operation>(operation);
  }

  /** @brief Traverse entries in allocated memory blocks.
   *
   *  Visits all entries within allocated memory blocks. This is more
   *  efficient than traverse() when the matrix is very sparse, but note
   *  that it visits all entries in each allocated block, not just those
   *  that were explicitly written.
   *
   *  @tparam Operation A callable with signature `bool(T&)`.
   *  @param[in] operation The operation to perform.
   *  @return true if all entries were visited, false if stopped early.
   *
   *  @note Due to the block-based allocation of DynArray, this function
   *        may visit entries that contain the default value but happen
   *        to be in the same block as a written entry.
   */
  template <class Operation>
  bool traverse_allocated(Operation && operation)
  {
    if (array_ptr == nullptr)
      return true;

    const size_t total = n * m;
    for (size_t idx = 0; idx < total; ++idx)
      if (T *entry = array_ptr->test(idx))
        {
          if (!operation(*entry))
            return false;
        }

    return true;
  }

  /// \overload traverse_allocated(Operation && operation)
  template <class Operation>
  bool traverse_allocated(Operation && operation) const
  {
    if (array_ptr == nullptr)
      return true;

    const size_t total = n * m;
    for (size_t idx = 0; idx < total; ++idx)
      if (const T *entry = array_ptr->test(idx))
        {
          if (!operation(*entry))
            return false;
        }

    return true;
  }
};

} // end namespace Aleph

#endif // TPL_DYNMAT_H