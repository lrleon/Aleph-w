
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef AH_MATRIX_H
# define AH_MATRIX_H

# include <al-vector.H>

namespace Aleph 
{

template <typename Trow = int, typename Tcol = int, typename NumType = double>
class Matrix
{
public:

  using RDomain = AlDomain<Trow>;

  using CDomain = AlDomain<Tcol>;

  using RCPair = std::pair<Trow, Tcol>;

  using Map = Aleph::HashMap<RCPair, NumType, MapODhash,
			     Aleph::Dft_Pair_Cmp<RCPair, NumType>>;

  using ColVector = Vector<Trow, NumType>;

  using RowVector = Vector<Tcol, NumType>;

private:

  static const NumType default_epsilon;

  mutable RDomain * rdomain_ptr;
  mutable CDomain * cdomain_ptr;
  Map               entries;
  NumType           epsilon;

  bool is_zero(const NumType & val) const noexcept
  {
    assert(epsilon >= 0);
    return val >= -epsilon and val <= epsilon;
  }

public:

  const NumType & get_epsilon() const noexcept { return epsilon; }

  void set_epsilon(const NumType & e) 
  {
    if (e < 0)
      throw std::domain_error("epsilon must be greater or equal to zero");

    epsilon = e;
  }

  typedef std::pair<RCPair, NumType> Pair; /// the pairs (row,col) -> entry

  const RDomain & get_row_domain() const noexcept { return *rdomain_ptr; }

  const CDomain & get_col_domain() const noexcept { return *cdomain_ptr; }

  Matrix(const RDomain & rdomain, const CDomain & cdomain, 
	 const NumType & e = default_epsilon)
    : rdomain_ptr(&const_cast<RDomain&>(rdomain)), 
      cdomain_ptr(&const_cast<CDomain&>(cdomain))
  {
    set_epsilon(e);
  }

  Matrix(const RDomain & rdomain, const CDomain & cdomain,
	 std::initializer_list<std::initializer_list<NumType>> l,
	 const NumType & e = default_epsilon)
    : Matrix(rdomain, cdomain, e)
  {
    if (l.size() != rdomain_ptr->size())
      throw std::range_error("mismatch with number of rows");

    auto itr = l.begin();
    const DynList<Tcol> clist = sort(cdomain_ptr->keys());
    sort(rdomain_ptr->keys()).for_each([this, &itr, &clist] (const Trow & row)
    {
      if (itr->size() != cdomain_ptr->size())
	throw std::range_error("mismatch with number of columns");

      auto itc = itr->begin();
      clist.for_each([this, &itc, &row] (const Tcol & col)
      {
	const NumType & val = *itc++;
	if (is_zero(val))
	  return;

	entries.insert(RCPair(row, col), val);
      });
      
      itr++;
    }); 
  }

  NumType get_entry(const Trow & row, const Tcol & col)
  {
    assert(rdomain_ptr->has(row) and cdomain_ptr->has(col));

    auto * ptr = entries.search(RCPair(row, col));
    if (ptr == nullptr)
      return 0;

    if (is_zero(ptr->second))
      {
	entries.remove_ptr(ptr);
	return 0;
      }

    return ptr->second;
  }

  NumType get_entry(const Trow & row, const Tcol & col) const
  {
    assert(rdomain_ptr->has(row) and cdomain_ptr->has(col));

    auto * ptr = entries.search(RCPair(row, col));
    if (ptr == nullptr)
      return 0;

    return ptr->second;
  }

  void set_entry(const Trow & row, const Tcol & col, const NumType & val)
  {
    assert(rdomain_ptr->has(row) and cdomain_ptr->has(col));

    auto * ptr = entries.search(RCPair(row, col));
    if (is_zero(val))
      {
	if (ptr != nullptr)
	  entries.remove_by_data(ptr->second);
	return;
      }

    if (ptr == nullptr)
      entries.insert(RCPair(row, col), val);
    else
      ptr->second = val;
  }

  Matrix transpose() const
  {
    Matrix ret_val(*cdomain_ptr, *rdomain_ptr);
    entries.for_each([&ret_val] (const Pair & p)
		     {
		       const RCPair & rc = p.first;
		       const NumType & val = p.second;
		       ret_val.set_entry(rc.second, rc.first, val);
		     });
    return ret_val;
  }

  DynList<Trow> row_domain_list() const 
  {
    return sort(rdomain_ptr->keys());
  }

  DynList<Tcol> col_domain_list() const 
  { 
    return sort(cdomain_ptr->keys()); 
  }

  DynList<NumType> get_row_as_list(const Trow & row) const
  {
    if (not rdomain_ptr->has(row))
      throw std::domain_error("row is not inside the row domain");

    return 
      col_domain_list().template maps<NumType>([this, &row] (const Tcol & col) 
					       { return get_entry(row, col); });
  }

  DynList<NumType> get_col_as_list(const Tcol & col) const
  {
    if (not cdomain_ptr->has(col))
      throw std::domain_error("col is not inside the column domain");

    return row_domain_list().template maps<NumType>
      ([this, &col] (const Trow & row) { return get_entry(row, col); });
  }

  /// Return a list of vectors corresponding to the rows
  DynList<Vector<Tcol, NumType>> to_rowlist() const
  {
    DynList<Vector<Tcol, NumType>> ret_val;
    DynList<Tcol> cols = col_domain_list();
    row_domain_list().for_each([this, &ret_val, cols] (const Trow & row) 
    {
      Vector<Tcol, NumType> vec(*cdomain_ptr, epsilon);
      cols.for_each([this, &vec, &row] (const Tcol & col) 
      {
	auto * val_ptr = entries.search(RCPair(row, col));
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
    col_domain_list().for_each([this, &ret_val, &rows] (const Tcol & col)
    {
      ColVector vec(*rdomain_ptr, epsilon);
      rows.for_each([this, &vec, &col] (const Trow & row)
		    {
		      auto * ptr = entries.search(RCPair(row, col));
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
    if (not rdomain_ptr->has(row))
      throw std::domain_error("get_row_vector(): invalid row");

    Vector<Tcol, NumType> ret_val(*cdomain_ptr);
    cdomain_ptr->for_each([&] (const Tcol & col)
    {
      auto * ptr = entries.search(RCPair(row, col));
      if (ptr != nullptr)
	ret_val.set_entry(col, ptr->second);
    });

    return ret_val;
  }

  /// Given a column, build a vector corresponding to the column
  Vector<Trow, NumType> get_col_vector(const Tcol & col) const
  {
    if (not cdomain_ptr->has(col))
      throw std::domain_error("get_col_vector(): invalid col");

    Vector<Trow, NumType> ret_val(*rdomain_ptr);
    rdomain_ptr->for_each([&] (const Trow & row)
    {
      auto * ptr = entries.search(RCPair(row, col));
      if (ptr != nullptr)
       	ret_val.set_entry(row, ptr->second);
    });

    return ret_val;
  }

  /** return this*vec in terms of linear combinations

      | a b c
    ---------
    A | 1 2 3    a b c                                         A  B  C
    B | 4 5 6 * ------ = v[a]*m[a] + v[b]m[b]*v + v[c]*m[c] = --------
    C | 7 8 9    1 0 2                                         7 16 25
   */
  Vector<Trow, NumType> 
  mult_matrix_vector_linear_comb(const Vector<Tcol, NumType> & vec) const
  {
    if (&vec.get_domain() != &get_col_domain())
      throw std::domain_error("mult_matrix_vector_linear_comb:domain mismatch");

    Vector<Trow, NumType> ret_val(*rdomain_ptr);
    vec.for_each([this, &ret_val] (const std::pair<Tcol, NumType> & p)
		 {
		   const Tcol & col = p.first;
		   const NumType & val = p.second;
		   ret_val += val*get_col_vector(col);
		 });

    return ret_val;
  }

  Vector<Trow, NumType> 
  mult_matrix_vector_sparse(const Vector<Tcol, NumType> & vec) const
  {
    if (&vec.get_domain() != &get_col_domain())
      throw std::domain_error("mult_matrix_vector_sparse: domain mismatch");

    Vector<Trow, NumType> ret_val(*rdomain_ptr);
    entries.for_each([&ret_val, &vec] (const Pair & p)
		     {
		       const Trow & row = p.first.first;
		       const Tcol & col = p.first.second;
		       const NumType & entry_val = p.second;
		       NumType * vec_entry = vec.search_entry(col);

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
    if (&vec.get_domain() != &get_row_domain())
      throw std::domain_error("mult_vector_matrix_linear_comb:domain mismatch");

    Vector<Tcol, NumType> ret_val(*cdomain_ptr);
    vec.for_each([this, &ret_val] (const std::pair<Trow, NumType> & p)
		 {
		   const Trow & row = p.first;
		   const NumType & val = p.second;
		   ret_val += val*get_row_vector(row);
		 });

    return ret_val;
  }

  Vector<Trow, NumType> 
  mult_matrix_vector_dot_product(const Vector<Tcol, NumType> & vec) const
  {
    if (&vec.get_domain() != &get_col_domain())
      throw std::domain_error("mult_matrix_vector_dot_product:domain mismatch");

    Vector<Trow, NumType> ret_val(*rdomain_ptr);
    rdomain_ptr->for_each([&] (const Trow & row)
			  {
			    ret_val.set_entry(row, get_row_vector(row)*vec);
			  });

    return ret_val;
  }

  Vector<Tcol, NumType> 
  mult_vector_matrix_dot_product(const Vector<Trow, NumType> & vec) const
  {
    if (&vec.get_domain() != &get_row_domain())
      throw std::domain_error("mult_vector_matrix_dot_product:domain mismatch");

    Vector<Tcol, NumType> ret_val(*cdomain_ptr);
    cdomain_ptr->for_each([&] (const Tcol & col)
			  {
			    ret_val.set_entry(col, get_col_vector(col)*vec);
			  });

    return ret_val;
  }

  Vector<Tcol, NumType>
  operator * (const Vector<Trow, NumType> & vec) const
  {
    return mult_matrix_vector_linear_comb(vec);
  }

private:

  template <typename T2row, typename T2col>
  void validate_domains_for_mult(const Matrix<T2row, T2col, NumType> & m) const
  {
    if (cdomain_ptr->size() != m.rdomain_ptr->size())
      throw std::domain_error("AxB: Col domain size different from row domain");
  }

public:

  /// set the row row with the values of vector vec
  Matrix & set_vector_as_row(const Trow & row, const Vector<Tcol> & vec)
  {
    if (&vec.get_domain() != cdomain_ptr)
      throw std::domain_error("set_vector_as_row: domain vec mismath");

    vec.for_each([this, &row] (const std::pair<Tcol, NumType> & p)
		 {
		   const Tcol & col = p.first;
		   const NumType & val = p.second;
		   set_entry(row, col, val);
		 });

    return *this;
  }

  /// set the column col with the values of vector vec
  Matrix & set_vector_as_col(const Tcol & col, const Vector<Trow> & vec)
  {
    if (&vec.get_domain() != rdomain_ptr)
      throw std::domain_error("set_vector_as_col: domain vec mismath");

    vec.for_each([this, &col] (const std::pair<Trow, NumType> & p)
		 {
		   const Trow & row = p.first;
		   const NumType & val = p.second;
		   set_entry(row, col, val);
		 });

    return *this;
  }

  Matrix identity() const
  {
    if (rdomain_ptr != cdomain_ptr)
      throw std::domain_error("Row domain is not the column domain");

    Matrix ret_val(*rdomain_ptr, *rdomain_ptr);
    rdomain_ptr->for_each([&ret_val] (const Trow & row)
			  {
			    ret_val.set_entry(row, row, 1);
			  });

    return ret_val;
  }

  Matrix & operator += (const Matrix & m)
  {
    if (rdomain_ptr != m.rdomain_ptr or cdomain_ptr != m.cdomain_ptr)
      throw std::domain_error("Matrix addtion domain mismatch");

    m.entries.for_each([this] (const Pair & p)
		       {
			 const Trow & row = p.first.first;
			 const Tcol & col = p.first.second;
			 const RCPair rcp = RCPair(row, col);
			 auto * ptr = entries.search(rcp);
			 if (ptr == nullptr)
			   entries.insert(rcp, p.second);
			 else
			   ptr->second += p.second;
		       });

    return *this;
  }

  Matrix operator + (const Matrix & m) const
  {
    Matrix ret_val = *this;
    ret_val += m;
    return ret_val;
  }

  Matrix & mult_by_scalar(const NumType & scalar) 
  {
    entries.for_each([&scalar] (const Pair & p)
		     {
		       NumType & val = const_cast<Pair&>(p).second;
		       val *= scalar;
		     });

    return *this;
  }

  template <typename T2row, typename T2col>
  Matrix<Trow, T2col> 
  vector_matrix_mult(const Matrix<T2row, T2col, NumType> & m) const
  {
    validate_domains_for_mult(m);

    Matrix<Trow, T2col> ret_val(*rdomain_ptr, *m.cdomain_ptr);

    rdomain_ptr->for_each([this, &m, &ret_val] (const Trow & row)
    {
      ret_val.set_vector_as_row(row, get_row_vector(row)*m);
    });

    return ret_val;
  }

  template <typename T2row, typename T2col>
  Matrix<Trow, T2col> 
  matrix_vector_mult(const Matrix<T2row, T2col, NumType> & m) const
  {
    validate_domains_for_mult(m);

    Matrix<Trow, T2col> ret_val(*rdomain_ptr, *m.cdomain_ptr);
    m.cdomain_ptr->for_each([this, &m, &ret_val] (const Tcol & col)
    {
      ret_val.set_vector_as_col(col, *this * m.get_col_vector(col));
    });

    return ret_val;
  }

  void print()
  {
    cout << "Non zero entries = " << endl;
    entries.for_each([] (const Pair & p)
		     {
		       cout << "(" << Aleph::to_str(p.first.first) << "," 
			    << Aleph::to_str(p.first.second) << ") : " 
			    << Aleph::to_str(p.second) << endl;
		     });
    cout << endl;
  }

  std::string to_str() const
  {
    typedef std::pair<std::string, std::string> StrPair;

    DynList<ColVector> cols = to_collist();

        // lista de string más largos de cada columna
    DynList<std::string> collens = 
      cols.template maps<std::string>([] (const ColVector & vec)
      {
	return vec.to_list().template foldl<std::string> 
			 ("", [] (const std::string & s, const NumType & val)
        {
	  std::string val_str = Aleph::to_str(val);
	  return val_str.size() > s.size() ? val_str : s;
	});
      });

    DynList<std::string> col_labels = 
      sort(cdomain_ptr->template maps<std::string>([] (const Tcol & col)
      {
	return Aleph::to_str(col);
      }));

    DynList<std::string> longers = 
      zip(collens, col_labels).template maps <std::string> ([] (const StrPair & p)
      {
	return p.first.size() > p.second.size() ? p.first : p.second;
      });

     DynList<std::string> row_labels = 
       sort(rdomain_ptr->template maps<std::string>([] (const Trow & row)
      {
	return Aleph::to_str(row);
      }));

    std::string longer_row_label = row_labels.template foldl<std::string>
      (std::string(), [] (const std::string & s, 
			  const std::string & row_label)
       {
	 return row_label.size() > s.size() ? row_label : s;
       });

    size_t longer_row_label_sz = longer_row_label.size();

    std::string ret_val = string(longer_row_label_sz, ' ') + " |";
    ret_val = zip(col_labels, longers).template foldl<std::string> 
      (ret_val, [] (const std::string & s, const StrPair & p)
       {
	 size_t fsz = p.first.size(), ssz = p.second.size();
	 return s + string((fsz > ssz ? fsz-ssz : ssz-fsz) + 1, ' ') + p.first;
       });

    ret_val += "\n" + string(ret_val.size(), '-');

    zip(row_labels, row_domain_list()).for_each
      ([this, &ret_val, &longers, &longer_row_label_sz] 
       (const std::pair<std::string, Trow> & p)
       {
	 const std::string & label = p.first;
	 const Trow & row = p.second;
	 
	 ret_val += "\n" + 
	   string(longer_row_label_sz - label.size(), ' ') + label + " |";
	 ret_val = 
	   zip(get_row_as_list(row), longers).template foldl<std::string>
	   (ret_val, [] (const std::string & s,
			 const std::pair<NumType, std::string> & q)
	    {
	      const std::string val = Aleph::to_str(q.first);
	      const std::string & len_str = q.second;
	      assert(len_str.size() - val.size() + 1);
	      return s + string(len_str.size() - val.size() + 1, ' ') + val;
	    });
       });

    return ret_val;
  }

  bool are_equal(const NumType & n1, const NumType & n2) const
  {
    return is_zero(n1 - n2);
  }

  bool equal_to(const Matrix & m) const
  {
    if (not (rdomain_ptr == m.rdomain_ptr and cdomain_ptr == m.cdomain_ptr))
      throw std::domain_error("invalid matrixes domains");

    return entries.all([m, this] (const Pair & p)
		       {
			 const Trow & row = p.first.first;
			 const Tcol & col = p.first.second;
			 return are_equal(m.get_entry(row, col), p.second);
		       })
      and
      m.entries.all([this] (const Pair & p)
		    {
		      const Trow & row = p.first.first;
		      const Tcol & col = p.first.second;
		      return are_equal(get_entry(row, col), p.second);
		    });
  }

  bool operator == (const Matrix & m) const { return equal_to(m); }

  bool operator != (const Matrix & m) const { return not equal_to(m); }
};

template <typename Trow, typename Tcol, typename NumType> inline
Matrix<Trow, Tcol, NumType> operator * (const NumType & scalar,
					const Matrix<Trow, Tcol, NumType> & m)
{
  Matrix<Trow, Tcol, NumType> ret_val = m;
  return ret_val.mult_by_scalar(scalar); 
}

template <typename Trow, typename Tcol, typename NumType> inline
Vector<Tcol, NumType> operator * (const Vector<Trow, NumType> & vec,
				  const Matrix<Trow, Tcol, NumType> & m)
{
  return m.mult_vector_matrix_linear_comb(vec);
}

template <typename Trow, typename Tcol, typename NumType>
const NumType Matrix<Trow, Tcol, NumType>::default_epsilon = 1e-7;

template <typename Trow, typename Tcol, typename NumType> inline
ostream & operator << (ostream & s, const Matrix<Trow, Tcol, NumType> & m)
{
  return s << m.to_str();
}

template <typename Trow, typename Tcol, typename NumType = double> inline
Matrix<Trow, Tcol, NumType> outer_product(const Vector<Trow> & v1,
					  const Vector<Trow> & v2)
{
  Matrix<Trow, Tcol, NumType> ret_val(v1.get_domain(), v2.get_domain());

  v1.for_each([&ret_val, &v2] (const std::pair<Trow, NumType> & p1)
  {
    v2.for_each([&ret_val, &p1](const std::pair<Tcol, NumType> & p2)
    {
      ret_val.set_entry(p1.first, p2.first, p1.second*p2.second);
    });
  });

  return ret_val;
}


} // end namespace Aleph

# endif // AH_MATRIX_H
