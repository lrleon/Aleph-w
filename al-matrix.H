/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file al-matrix.H
 *  @brief Sparse matrix with generic domains.
 *
 *  Provides Matrix class for sparse matrices where rows and columns
 *  can have arbitrary domains. Efficient for matrices with many
 *  zero elements using hash-based storage.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef AH_MATRIX_H
# define AH_MATRIX_H

# include <al-vector.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Sparse matrix with generic row and column domains.

      This class implements a sparse matrix where row and column indices
      are elements of arbitrary domains (AlDomain). Internally, only non-zero
      entries are stored in a hash table keyed by (row, col) pairs.

      - Absence of an entry in the hash table means the value is zero.
      - Setting an entry to zero removes it from the hash table.

      @tparam Trow Type of row domain elements (default: int)
      @tparam Tcol Type of column domain elements (default: int)
      @tparam NumType Numeric type for matrix entries (default: double)
  */
  template <typename Trow = int, typename Tcol = int, typename NumType = double>
  class Matrix
  {
  public:
    using RDomain = AlDomain<Trow>;
    using CDomain = AlDomain<Tcol>;
    using RDomainPtr = std::shared_ptr<RDomain>;
    using CDomainPtr = std::shared_ptr<CDomain>;

    using RCPair = std::pair<Trow, Tcol>;

    using Map = Aleph::HashMap<RCPair, NumType, MapODhash>;

    using ColVector = Vector<Trow, NumType>;

    using RowVector = Vector<Tcol, NumType>;

  private:
    static const NumType default_epsilon;

    RDomainPtr rdomain_ptr = nullptr;
    CDomainPtr cdomain_ptr = nullptr;
    Map entries;
    NumType epsilon;

    bool is_zero(const NumType & val) const noexcept
    {
      assert(epsilon >= 0);
      return val >= -epsilon and val <= epsilon;
    }

  public:
    /// Return the epsilon value used for zero comparisons.
    const NumType &get_epsilon() const noexcept { return epsilon; }

    /** @brief Set the epsilon value for zero comparisons.
        @param e New epsilon value (must be >= 0)
        @throw ah_domain_error if e < 0
    */
    void set_epsilon(const NumType & e)
    {
      ah_domain_error_if(e < 0)
        << "epsilon must be greater or equal to zero";
      epsilon = e;
    }

    /// Type alias for entries stored in the hash table: ((row,col), value).
    typedef std::pair<RCPair, NumType> Pair;

    /// Return the row domain.
    const RDomain &get_row_domain() const noexcept { return *rdomain_ptr; }

    /// Return the column domain.
    const CDomain &get_col_domain() const noexcept { return *cdomain_ptr; }

    /// Return the shared pointer to the row domain.
    const RDomainPtr &get_row_domain_ptr() const noexcept { return rdomain_ptr; }

    /// Return the shared pointer to the column domain.
    const CDomainPtr &get_col_domain_ptr() const noexcept { return cdomain_ptr; }

    /** @brief Construct an empty sparse matrix over the given domains (shared_ptr version).
        @param rdomain Shared pointer to row domain
        @param cdomain Shared pointer to column domain
        @param e Epsilon for zero comparisons (default: 1e-7)
    */
    Matrix(RDomainPtr rdomain, CDomainPtr cdomain,
           const NumType & e = default_epsilon)
      : rdomain_ptr(std::move(rdomain)),
        cdomain_ptr(std::move(cdomain)),
        entries(10, Aleph::pair_dft_hash_fct<Trow, Tcol>,
                Aleph::pair_snd_hash_fct<Trow, Tcol>)
    {
      set_epsilon(e);
    }

    /** @brief Construct an empty sparse matrix over the given domains (reference version).
        @param rdomain Row domain (will be wrapped in non-owning shared_ptr)
        @param cdomain Column domain (will be wrapped in non-owning shared_ptr)
        @param e Epsilon for zero comparisons (default: 1e-7)
        @note Creates non-owning shared_ptrs; caller must ensure domains outlive matrix
    */
    Matrix(const RDomain & rdomain, const CDomain & cdomain,
           const NumType & e = default_epsilon)
      : rdomain_ptr(RDomainPtr(&const_cast<RDomain &>(rdomain), [](RDomain *) {})),
        cdomain_ptr(CDomainPtr(&const_cast<CDomain &>(cdomain), [](CDomain *) {})),
        entries(10, Aleph::pair_dft_hash_fct<Trow, Tcol>,
                Aleph::pair_snd_hash_fct<Trow, Tcol>)
    {
      set_epsilon(e);
    }

    /** @brief Construct a matrix from a 2D initializer list (shared_ptr version).
        @param rdomain Shared pointer to row domain
        @param cdomain Shared pointer to column domain
        @param l Initializer list of rows (each row is a list of values)
        @param e Epsilon for zero comparisons
        @throw ah_range_error if dimensions don't match domain sizes
    */
    Matrix(RDomainPtr rdomain, CDomainPtr cdomain,
           std::initializer_list<std::initializer_list<NumType>> l,
           const NumType & e = default_epsilon)
      : Matrix(std::move(rdomain), std::move(cdomain), e)
    {
      ah_range_error_if(l.size() != rdomain_ptr->size()) << "mismatch with number of rows";

      auto itr = l.begin();
      const DynList<Tcol> clist = sort(cdomain_ptr->keys());
      sort(rdomain_ptr->keys()).
          for_each([this, &itr, &clist](const Trow & row)
                     {
                       ah_range_error_if(itr->size() != cdomain_ptr->size())
                          << "mismatch with number of columns";
                       auto itc = itr->begin();
                       clist.for_each([this, &itc, &row](const Tcol & col)
                                        {
                                          const NumType & val = *itc++;
                                          if (is_zero(val))
                                            return;

                                          entries.insert(RCPair(row, col), val);
                                        });
                       ++itr;
                     });
    }

    /** @brief Construct a matrix from a 2D initializer list (reference version).
        @param rdomain Row domain (will be wrapped in non-owning shared_ptr)
        @param cdomain Column domain (will be wrapped in non-owning shared_ptr)
        @param l Initializer list of rows
        @param e Epsilon for zero comparisons
        @note Creates non-owning shared_ptrs; caller must ensure domains outlive matrix
    */
    Matrix(const RDomain & rdomain, const CDomain & cdomain,
           std::initializer_list<std::initializer_list<NumType>> l,
           const NumType & e = default_epsilon)
      : Matrix(RDomainPtr(&const_cast<RDomain &>(rdomain), [](RDomain *) {}),
               CDomainPtr(&const_cast<CDomain &>(cdomain), [](CDomain *) {}), l, e)
    {}

    /** @brief Construct a matrix from a list of lists (shared_ptr version).
        @param rdomain Shared pointer to row domain
        @param cdomain Shared pointer to column domain
        @param l List of rows (each row is a DynList of values)
        @param e Epsilon for zero comparisons
        @throw ah_range_error if dimensions don't match domain sizes
    */
    Matrix(RDomainPtr rdomain, CDomainPtr cdomain,
           const DynList<DynList<NumType>> & l,
           const NumType & e = default_epsilon)
      : Matrix(std::move(rdomain), std::move(cdomain), e)
    {
      ah_range_error_if(l.size() != rdomain_ptr->size()) << "mismatch with number of rows";

      for (auto it = zip_it(rdomain_ptr->keys(), l); it.has_curr(); it.next())
        {
          auto t = it.get_curr();
          const Trow & row_idx = get<0>(t);
          const DynList<NumType> & row = get<1>(t);
          for (auto it = zip_it(cdomain_ptr->keys(), row); it.has_curr(); it.next())
            {
              auto t = it.get_curr();
              const NumType & val = get<1>(t);
              if (is_zero(val))
                continue;
              const Tcol & col_idx = get<0>(t);
              entries.insert(RCPair(row_idx, col_idx), val);
            }
        }
    }

    /** @brief Construct a matrix from a list of lists (reference version).
        @param rdomain Row domain (will be wrapped in non-owning shared_ptr)
        @param cdomain Column domain (will be wrapped in non-owning shared_ptr)
        @param l List of rows
        @param e Epsilon for zero comparisons
        @note Creates non-owning shared_ptrs; caller must ensure domains outlive matrix
    */
    Matrix(const RDomain & rdomain, const CDomain & cdomain,
           const DynList<DynList<NumType>> & l,
           const NumType & e = default_epsilon)
      : Matrix(RDomainPtr(&const_cast<RDomain &>(rdomain), [](RDomain *) {}),
               CDomainPtr(&const_cast<CDomain &>(cdomain), [](CDomain *) {}), l, e)
    {}

    /// Copy constructor.
    Matrix(const Matrix & other)
      : rdomain_ptr(other.rdomain_ptr),
        cdomain_ptr(other.cdomain_ptr),
        entries(other.entries),
        epsilon(other.epsilon)
    {
      // empty
    }

    /// Move constructor.
    Matrix(Matrix && other) noexcept
      : rdomain_ptr(other.rdomain_ptr),
        cdomain_ptr(other.cdomain_ptr),
        entries(std::move(other.entries)),
        epsilon(other.epsilon)
    {
      // empty
    }

    /// Copy assignment operator.
    Matrix &operator =(const Matrix & other)
    {
      if (this != &other)
        {
          rdomain_ptr = other.rdomain_ptr;
          cdomain_ptr = other.cdomain_ptr;
          entries = other.entries;
          epsilon = other.epsilon;
        }
      return *this;
    }

    /// Move assignment operator.
    Matrix &operator =(Matrix && other) noexcept
    {
      if (this != &other)
        {
          rdomain_ptr = other.rdomain_ptr;
          cdomain_ptr = other.cdomain_ptr;
          entries = std::move(other.entries);
          epsilon = other.epsilon;
        }
      return *this;
    }

    /** @brief Create a matrix from a list of row vectors.
        @param rdomain Row domain
        @param rows List of vectors, one per row
        @param e Epsilon for zero comparisons
        @return New matrix constructed from the row vectors
    */
    static Matrix create_by_rows(const RDomain & rdomain,
                                 const DynList<Vector<Tcol, NumType>> & rows,
                                 const NumType & e = default_epsilon)
    {
      Matrix ret(rdomain, rows.get_first().get_domain(), e);

      for (auto it = zip_it(rdomain.keys(), rows); it.has_curr(); it.next())
        {
          auto t = it.get_curr();
          const Trow & row_idx = get<0>(t);
          const Vector<Tcol, NumType> & row = get<1>(t);
          for (auto it_col = row.get_it(); it_col.has_curr(); it_col.next())
            {
              auto & p = it_col.get_curr();
              const NumType & val = p.second;
              if (ret.is_zero(val))
                continue;
              const Tcol & col_idx = p.first;
              ret.entries.insert(RCPair(row_idx, col_idx), val);
            }
        }

      return ret;
    }

    /** @brief Create a matrix from a list of column vectors.
        @param cdomain Column domain
        @param cols List of vectors, one per column
        @param e Epsilon for zero comparisons
        @return New matrix constructed from the column vectors
    */
    static Matrix create_by_columns(const CDomain & cdomain,
                                    const DynList<Vector<Trow, NumType>> & cols,
                                    const NumType & e = default_epsilon)
    {
      Matrix ret(cols.get_first().get_domain(), cdomain, e);

      for (auto it = zip_it(cdomain.keys(), cols); it.has_curr(); it.next())
        {
          auto t = it.get_curr();
          const Tcol & col_idx = get<0>(t);
          const Vector<Trow, NumType> & col = get<1>(t);
          for (auto it_col = col.get_it(); it_col.has_curr(); it_col.next())
            {
              auto & p = it_col.get_curr();
              const NumType & val = p.second;
              if (ret.is_zero(val))
                continue;
              const Trow & row_idx = p.first;
              ret.entries.insert(RCPair(row_idx, col_idx), val);
            }
        }

      return ret;
    }

    /** @brief Get an entry value (non-const version with lazy cleanup).

        If the stored value is within epsilon of zero, the entry is removed
        from the hash table (lazy cleanup) and zero is returned.

        @param row Row index
        @param col Column index
        @return Entry value, or 0 if not present
    */
    NumType get_entry(const Trow & row, const Tcol & col)
    {
      assert(rdomain_ptr->has(row) and cdomain_ptr->has(col));
      auto *ptr = entries.search(RCPair(row, col));
      if (ptr == nullptr)
        return 0;
      if (is_zero(ptr->second))
        {
          entries.remove_ptr(ptr);
          return 0;
        }
      return ptr->second;
    }

    /** @brief Get an entry value (const version, no cleanup).
        @param row Row index
        @param col Column index
        @return Entry value, or 0 if not present
    */
    NumType get_entry(const Trow & row, const Tcol & col) const noexcept
    {
      assert(rdomain_ptr->has(row) and cdomain_ptr->has(col));
      auto *ptr = entries.search(RCPair(row, col));
      if (ptr == nullptr)
        return 0;
      return ptr->second;
    }

    /** @brief Set an entry value.

        If the value is within epsilon of zero, the entry is removed from
        the hash table (sparse storage optimization).

        @param row Row index
        @param col Column index
        @param val Value to set
    */
    void set_entry(const Trow & row, const Tcol & col, const NumType & val)
    {
      assert(rdomain_ptr->has(row) and cdomain_ptr->has(col));
      auto *ptr = entries.search(RCPair(row, col));
      if (is_zero(val))
        {
          if (ptr != nullptr)
            entries.remove_ptr(ptr);
          return;
        }
      if (ptr == nullptr)
        entries.insert(RCPair(row, col), val);
      else
        ptr->second = val;
    }

    /** @brief Compute the transpose of this matrix.
        @return New matrix with rows and columns swapped
    */
    Matrix transpose() const
    {
      Matrix ret_val(*cdomain_ptr, *rdomain_ptr);
      entries.for_each([&ret_val](const Pair & p)
                         {
                           const RCPair & rc = p.first;
                           const NumType & val = p.second;
                           ret_val.set_entry(rc.second, rc.first, val);
                         });
      return ret_val;
    }

    /// Return a sorted list of row domain keys.
    DynList<Trow> row_domain_list() const
    {
      return sort(rdomain_ptr->keys());
    }

    /// Return a sorted list of column domain keys.
    DynList<Tcol> col_domain_list() const
    {
      return sort(cdomain_ptr->keys());
    }

    /** @brief Get a row as a list of values (in column order).
        @param row Row index
        @return List of values in that row
        @throw ah_domain_error if row not in domain
    */
    DynList<NumType> get_row_as_list(const Trow & row) const
    {
      ah_domain_error_if(not rdomain_ptr->has(row)) << "row is not inside the row domain";
      return col_domain_list().
          template maps<NumType>([this, &row](const Tcol & col)
                                   {
                                     return get_entry(row, col);
                                   });
    }

    /** @brief Get a column as a list of values (in row order).
        @param col Column index
        @return List of values in that column
        @throw ah_domain_error if col not in domain
    */
    DynList<NumType> get_col_as_list(const Tcol & col) const
    {
      ah_domain_error_if(not cdomain_ptr->has(col)) << "col is not inside the column domain";
      return row_domain_list().template maps<NumType>
          ([this, &col](const Trow & row) { return get_entry(row, col); });
    }

    /// Return a list of vectors corresponding to the rows
    DynList<Vector<Tcol, NumType>> to_rowlist() const
    {
      DynList<Vector<Tcol, NumType>> ret_val;
      DynList<Tcol> cols = col_domain_list();
      row_domain_list().
          for_each([this, &ret_val, cols](const Trow & row)
                     {
                       Vector<Tcol, NumType> vec(*cdomain_ptr, epsilon);
                       cols.for_each([this, &vec, &row](const Tcol & col)
                                       {
                                         auto *val_ptr =
                                             entries.search(RCPair(row, col));
                                         if (val_ptr != nullptr)
                                           vec.set_entry(col, val_ptr->second);
                                       });
                       ret_val.append(vec);
                     });
      return ret_val;
    }

    /// Return a list of vectors corresponding to the columns
    DynList<ColVector> to_collist() const
    {
      DynList<ColVector> ret_val;
      DynList<Trow> rows = row_domain_list();
      col_domain_list().
          for_each([this, &ret_val, &rows](const Tcol & col)
                     {
                       ColVector vec(*rdomain_ptr, epsilon);
                       rows.for_each([this, &vec, &col](const Trow & row)
                                       {
                                         auto *ptr = entries.search(RCPair(row, col));
                                         if (ptr != nullptr)
                                           vec.set_entry(row, ptr->second);
                                       });
                       ret_val.append(vec);
                     });
      return ret_val;
    }

    /// Given a row, build a vector corresponding to the row
    Vector<Tcol, NumType> get_row_vector(const Trow & row) const
    {
      ah_domain_error_if(not rdomain_ptr->has(row))
        << "get_row_vector(): invalid row";
      Vector<Tcol, NumType> ret_val(*cdomain_ptr);
      cdomain_ptr->for_each([&](const Tcol & col)
                              {
                                auto *ptr = entries.search(RCPair(row, col));
                                if (ptr != nullptr)
                                  ret_val.set_entry(col, ptr->second);
                              });
      return ret_val;
    }

    /// Given a column, build a vector corresponding to the column
    Vector<Trow, NumType> get_col_vector(const Tcol & col) const
    {
      ah_domain_error_if(not cdomain_ptr->has(col))
        << "get_col_vector(): invalid col";
      Vector<Trow, NumType> ret_val(*rdomain_ptr);
      rdomain_ptr->for_each([&](const Trow & row)
                              {
                                auto *ptr = entries.search(RCPair(row, col));
                                if (ptr != nullptr)
                                  ret_val.set_entry(row, ptr->second);
                              });
      return ret_val;
    }

    /** return this*vec in terms of linear combinations

        | a b c
        ---------
        A | 1 2 3    a b c                                         A  B  C
        B | 4 5 6 * ------ = v[a]*m[a] + v[b]*m[b] + v[c]*m[c] = --------
        C | 7 8 9    1 0 2                                         7 16 25
    */
    Vector<Trow, NumType>
    mult_matrix_vector_linear_comb(const Vector<Tcol, NumType> & vec) const
    {
      ah_domain_error_if(&vec.get_domain() != &get_col_domain())
        << "mult_matrix_vector_linear_comb: domain mismatch";
      Vector<Trow, NumType> ret_val(*rdomain_ptr);
      vec.for_each([this, &ret_val](const std::pair<Tcol, NumType> & p)
                     {
                       const Tcol & col = p.first;
                       const NumType & val = p.second;
                       ret_val += val * get_col_vector(col);
                     });
      return ret_val;
    }

    /** @brief Matrix-vector multiplication using sparse iteration.

        Iterates only over non-zero matrix entries for efficiency.

        @param vec Column vector to multiply
        @return Result vector
        @throw ah_domain_error if domain mismatch
    */
    Vector<Trow, NumType>
    mult_matrix_vector_sparse(const Vector<Tcol, NumType> & vec) const
    {
      ah_domain_error_if(&vec.get_domain() != &get_col_domain())
        << "mult_matrix_vector_sparse: domain mismatch";
      Vector<Trow, NumType> ret_val(*rdomain_ptr);
      entries.for_each([&ret_val, &vec](const Pair & p)
                         {
                           const Trow & row = p.first.first;
                           const Tcol & col = p.first.second;
                           const NumType & entry_val = p.second;
                           NumType *vec_entry = vec.search_entry(col);
                           if (vec_entry != nullptr)
                             ret_val.set_entry(row, ret_val.get_entry(row) +
                                                    entry_val * *vec_entry);
                         });
      return ret_val;
    }

    /** return vec*this in terms of linear combinations

        | a b c
        ---------
        A B C   A | 1 2 3                   a  b    c
        -------- * B | 4 5 6 = v[A]*M[A] = -------------
        1.5 0 3   C | 7 8 9                22.5 27 31.5

    */
    Vector<Tcol, NumType>
    mult_vector_matrix_linear_comb(const Vector<Trow, NumType> & vec) const
    {
      ah_domain_error_if(&vec.get_domain() != &get_row_domain())
        << "mult_vector_matrix_linear_comb: domain mismatch";
      Vector<Tcol, NumType> ret_val(*cdomain_ptr);
      vec.for_each([this, &ret_val](const std::pair<Trow, NumType> & p)
                     {
                       const Trow & row = p.first;
                       const NumType & val = p.second;
                       ret_val += val * get_row_vector(row);
                     });
      return ret_val;
    }

    /** @brief Matrix-vector multiplication using dot products.

        Computes each result entry as dot product of row vector and input.

        @param vec Column vector to multiply
        @return Result vector
        @throw ah_domain_error if domain mismatch
    */
    Vector<Trow, NumType>
    mult_matrix_vector_dot_product(const Vector<Tcol, NumType> & vec) const
    {
      ah_domain_error_if(&vec.get_domain() != &get_col_domain())
        << "mult_matrix_vector_dot_product: domain mismatch";
      Vector<Trow, NumType> ret_val(*rdomain_ptr);
      rdomain_ptr->for_each([&](const Trow & row)
                              {
                                ret_val.set_entry(row, vec * get_row_vector(row));
                              });
      return ret_val;
    }

    /** @brief Vector-matrix multiplication using dot products.

        Computes each result entry as dot product of input and column vector.

        @param vec Row vector to multiply
        @return Result vector
        @throw ah_domain_error if domain mismatch
    */
    Vector<Tcol, NumType>
    mult_vector_matrix_dot_product(const Vector<Trow, NumType> & vec) const
    {
      ah_domain_error_if(&vec.get_domain() != &get_row_domain())
        << "mult_vector_matrix_dot_product: domain mismatch";
      Vector<Tcol, NumType> ret_val(*cdomain_ptr);
      cdomain_ptr->for_each([&](const Tcol & col)
                              {
                                ret_val.set_entry(col, vec * get_col_vector(col));
                              });
      return ret_val;
    }

    /// Matrix-vector multiplication operator (uses linear combination method).
    Vector<Trow, NumType>
    operator *(const Vector<Tcol, NumType> & vec) const
    {
      return mult_matrix_vector_linear_comb(vec);
    }

  private:
    /** @brief Validate domain compatibility for matrix multiplication.
    
        For A×B to be valid, the column domain of A must be identical
        to the row domain of B (same shared_ptr).
        
        @param m Matrix to multiply with
        @throw ah_domain_error if domains are not identical
    */
    template <typename T2row, typename T2col>
    void validate_domains_for_mult(const Matrix<T2row, T2col, NumType> & m)
    const
    {
      ah_domain_error_if(cdomain_ptr != m.rdomain_ptr)
        << "AxB: Column domain of A must be identical to row domain of B";
    }

  public:
    /** @brief Set a row from a vector.
        @param row Row index
        @param vec Vector with column domain
        @return Reference to this matrix
        @throw ah_domain_error if vector domain doesn't match column domain
    */
    Matrix &set_vector_as_row(const Trow & row, const Vector<Tcol, NumType> & vec)
    {
      ah_domain_error_if(&vec.get_domain() != cdomain_ptr.get())
        << "set_vector_as_row: domain vec mismatch";
      vec.for_each([this, &row](const std::pair<Tcol, NumType> & p)
                     {
                       const Tcol & col = p.first;
                       const NumType & val = p.second;
                       set_entry(row, col, val);
                     });
      return *this;
    }

    /** @brief Set a column from a vector.
        @param col Column index
        @param vec Vector with row domain
        @return Reference to this matrix
        @throw ah_domain_error if vector domain doesn't match row domain
    */
    Matrix &set_vector_as_col(const Tcol & col, const Vector<Trow, NumType> & vec)
    {
      ah_domain_error_if(&vec.get_domain() != rdomain_ptr.get())
        << "set_vector_as_col: domain vec mismatch";
      vec.for_each([this, &col](const std::pair<Trow, NumType> & p)
                     {
                       const Trow & row = p.first;
                       const NumType & val = p.second;
                       set_entry(row, col, val);
                     });
      return *this;
    }

    /** @brief Create an identity matrix (only for square matrices).

        Requires that Trow == Tcol (enforced by static_assert).

        @return Identity matrix over the row domain
    */
    Matrix identity() const
    {
      static_assert(std::is_same_v<Trow, Tcol>,
                    "Row domain is not equal to column domain");

      Matrix<Trow, Tcol, NumType> i(*rdomain_ptr, *rdomain_ptr);
      rdomain_ptr->for_each([&i](const Trow & row)
                              {
                                i.set_entry(row, row, 1);
                              });
      return i;
    }

    /** @brief Add another matrix to this one.
        @param m Matrix to add (must share same domains)
        @return Reference to this matrix
        @throw ah_domain_error if domains don't match
    */
    Matrix &operator +=(const Matrix & m)
    {
      ah_domain_error_if(rdomain_ptr != m.rdomain_ptr or cdomain_ptr != m.cdomain_ptr)
        << "Matrix addition domain mismatch";
      m.entries.for_each([this](const Pair & p)
                           {
                             const Trow & row = p.first.first;
                             const Tcol & col = p.first.second;
                             const RCPair rcp = RCPair(row, col);
                             auto *ptr = entries.search(rcp);
                             if (ptr == nullptr)
                               entries.insert(rcp, p.second);
                             else
                               ptr->second += p.second;
                           });
      return *this;
    }

    /// Matrix addition operator.
    Matrix operator +(const Matrix & m) const
    {
      Matrix ret_val = *this;
      ret_val += m;
      return ret_val;
    }

    /** @brief Subtract another matrix from this one.
        @param m Matrix to subtract (must share same domains)
        @return Reference to this matrix
        @throw ah_domain_error if domains don't match
    */
    Matrix &operator -=(const Matrix & m)
    {
      ah_domain_error_if(rdomain_ptr != m.rdomain_ptr or cdomain_ptr != m.cdomain_ptr)
        << "Matrix subtraction domain mismatch";
      m.entries.for_each([this](const Pair & p)
                           {
                             const Trow & row = p.first.first;
                             const Tcol & col = p.first.second;
                             const RCPair rcp = RCPair(row, col);
                             auto *ptr = entries.search(rcp);
                             if (ptr == nullptr)
                               entries.insert(rcp, -p.second);
                             else
                               ptr->second -= p.second;
                           });
      return *this;
    }

    /// Matrix subtraction operator.
    Matrix operator -(const Matrix & m) const
    {
      Matrix ret_val = *this;
      ret_val -= m;
      return ret_val;
    }

    /** @brief Multiply all entries by a scalar (in-place).
        @param scalar Value to multiply by
        @return Reference to this matrix
    */
    Matrix &mult_by_scalar(const NumType & scalar)
    {
      entries.for_each([&scalar](const Pair & p)
                         {
                           NumType & val = const_cast<Pair &>(p).second;
                           val *= scalar;
                         });
      return *this;
    }

    /** @brief Matrix multiplication using row-vector times matrix approach.
        @tparam T2row Row type of second matrix
        @tparam T2col Column type of second matrix
        @param m Matrix to multiply with
        @return Result matrix
        @throw ah_domain_error if column domain size != row domain size of m
    */
    template <typename T2row, typename T2col>
    Matrix<Trow, T2col>
    vector_matrix_mult(const Matrix<T2row, T2col, NumType> & m) const
    {
      validate_domains_for_mult(m);
      Matrix<Trow, T2col> ret_val(*rdomain_ptr, m.get_col_domain());
      rdomain_ptr->
          for_each([this, &m, &ret_val](const Trow & row)
                     {
                       ret_val.set_vector_as_row(row, get_row_vector(row) * m);
                     });
      return ret_val;
    }

    /** @brief Matrix multiplication using matrix times column-vector approach.
        @tparam T2row Row type of second matrix
        @tparam T2col Column type of second matrix
        @param m Matrix to multiply with
        @return Result matrix
        @throw ah_domain_error if column domain size != row domain size of m
    */
    template <typename T2row, typename T2col>
    Matrix<Trow, T2col>
    matrix_vector_mult(const Matrix<T2row, T2col, NumType> & m) const
    {
      validate_domains_for_mult(m);
      Matrix<Trow, T2col> ret_val(*rdomain_ptr, m.get_col_domain());
      m.get_col_domain().
        for_each([this, &m, &ret_val](const Tcol & col)
                   {
                     ret_val.set_vector_as_col(col,
                                               *this * m.get_col_vector(col));
                   });
      return ret_val;
    }

    /// Print non-zero entries to stdout.
    void print() const
    {
      std::cout << "Non zero entries = " << '\n';
      entries.for_each([](const Pair & p)
                         {
                           std::cout << "(" << Aleph::to_str(p.first.first) << ","
                               << Aleph::to_str(p.first.second) << ") : "
                               << Aleph::to_str(p.second) << '\n';
                         });
      std::cout << '\n';
    }

    /// Convert matrix to a formatted string representation.
    [[nodiscard]] std::string to_str() const
    {
      typedef std::pair<std::string, std::string> StrPair;
      DynList<ColVector> cols = to_collist();
      // list of longest strings for each column
      const DynList<std::string> collens = cols.
          template maps<std::string>([](const ColVector & vec)
                                       {
                                         return vec.to_list().
                                                    template foldl<std::string>
                                                    ("", [](const std::string & s, const NumType & val)
                                                       {
                                                         std::string val_str = Aleph::to_str(val);
                                                         return val_str.size() > s.size() ? val_str : s;
                                                       });
                                       });

      const DynList<std::string> col_labels =
          sort(cdomain_ptr->template maps<std::string>([](const Tcol & col)
                                                         {
                                                           return Aleph::to_str(col);
                                                         }));

      DynList<std::string> longers = zip(collens, col_labels).
          template maps<std::string>([](const StrPair & p)
                                       {
                                         return p.first.size() > p.second.size() ?
                                                  p.first :
                                                  p.second;
                                       });

      DynList<std::string> row_labels =
          sort(rdomain_ptr->template maps<std::string>([](const Trow & row)
                                                         {
                                                           return Aleph::to_str(row);
                                                         }));

      const std::string longer_row_label = row_labels.template foldl<std::string>
          (std::string(), [](const std::string & s,
                             const std::string & row_label)
             {
               return row_label.size() > s.size() ? row_label : s;
             });

      size_t longer_row_label_sz = longer_row_label.size();

      std::string ret_val = std::string(longer_row_label_sz, ' ') + " |";
      ret_val = zip(col_labels, longers).template foldl<std::string>
          (ret_val, [](const std::string & s, const StrPair & p)
             {
               const size_t fsz = p.first.size();
               const size_t ssz = p.second.size();
               return s + std::string((fsz > ssz ? fsz - ssz : ssz - fsz) + 1, ' ') + p.first;
             });

      ret_val += "\n" + std::string(ret_val.size(), '-');

      zip(row_labels, row_domain_list()).for_each
          ([this, &ret_val, &longers, &longer_row_label_sz]
       (const std::pair<std::string, Trow> & p)
             {
               const std::string & label = p.first;
               const Trow & row = p.second;

               ret_val += "\n" +
                   std::string(longer_row_label_sz - label.size(), ' ') + label + " |";
               ret_val =
                   zip(get_row_as_list(row), longers).template foldl<std::string>
                   (ret_val, [](const std::string & s,
                                const std::pair<NumType, std::string> & q)
                      {
                        const std::string val = Aleph::to_str(q.first);
                        const std::string & len_str = q.second;
                        assert(len_str.size() - val.size() + 1);
                        return s + std::string(len_str.size() - val.size() + 1, ' ') + val;
                      });
             });

      return ret_val;
    }

    /// Check if two values are equal within epsilon tolerance.
    bool are_equal(const NumType & n1, const NumType & n2) const
    {
      return is_zero(n1 - n2);
    }

    /** @brief Check if this matrix equals another (within epsilon).
        @param m Matrix to compare with
        @return true if all entries are equal within epsilon
        @throw ah_domain_error if domains don't match
    */
    bool equal_to(const Matrix & m) const
    {
      ah_domain_error_if(not (rdomain_ptr == m.rdomain_ptr and cdomain_ptr == m.cdomain_ptr))
        << "invalid matrix's domains";
      return entries.all([&m, this](const Pair & p)
                           {
                             const Trow & row = p.first.first;
                             const Tcol & col = p.first.second;
                             return are_equal(m.get_entry(row, col), p.second);
                           }) and
             m.entries.all([this](const Pair & p)
                             {
                               const Trow & row = p.first.first;
                               const Tcol & col = p.first.second;
                               return are_equal(get_entry(row, col), p.second);
                             });
    }

    /// Equality operator (uses equal_to).
    bool operator ==(const Matrix & m) const { return equal_to(m); }

    /// Inequality operator.
    bool operator !=(const Matrix & m) const { return not equal_to(m); }
  };

  /// Scalar-matrix multiplication (scalar * matrix).
  template <typename Trow, typename Tcol, typename NumType>
  inline
  Matrix<Trow, Tcol, NumType> operator *(const NumType & scalar,
                                         const Matrix<Trow, Tcol, NumType> & m)
  {
    Matrix<Trow, Tcol, NumType> ret_val = m;
    return ret_val.mult_by_scalar(scalar);
  }

  /// Vector-matrix multiplication (vector * matrix).
  template <typename Trow, typename Tcol, typename NumType>
  inline
  Vector<Tcol, NumType> operator *(const Vector<Trow, NumType> & vec,
                                   const Matrix<Trow, Tcol, NumType> & m)
  {
    return m.mult_vector_matrix_linear_comb(vec);
  }

  template <typename Trow, typename Tcol, typename NumType>
  const NumType Matrix<Trow, Tcol, NumType>::default_epsilon = 1e-7;

  /// Stream output operator for Matrix.
  template <typename Trow, typename Tcol, typename NumType>
  inline
  std::ostream &operator <<(std::ostream & s, const Matrix<Trow, Tcol, NumType> & m)
  {
    return s << m.to_str();
  }

  /** @brief Compute the outer product of two vectors.
      @tparam Trow Row domain type (from first vector)
      @tparam Tcol Column domain type (from second vector)
      @tparam NumType Numeric type
      @param v1 First vector (becomes rows)
      @param v2 Second vector (becomes columns)
      @return Matrix where entry (i,j) = v1[i] * v2[j]
  */
  template <typename Trow, typename Tcol, typename NumType = double>
  inline
  Matrix<Trow, Tcol, NumType> outer_product(const Vector<Trow, NumType> & v1,
                                            const Vector<Tcol, NumType> & v2)
  {
    Matrix<Trow, Tcol, NumType> ret_val(v1.get_domain(), v2.get_domain());

    v1.for_each([&ret_val, &v2](const std::pair<Trow, NumType> & p1)
                  {
                    v2.for_each([&ret_val, &p1](const std::pair<Tcol, NumType> & p2)
                                  {
                                    ret_val.set_entry(p1.first, p2.first,
                                                      p1.second * p2.second);
                                  });
                  });
    return ret_val;
  }
} // end namespace Aleph

# endif // AH_MATRIX_H
