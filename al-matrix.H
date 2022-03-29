

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

    Vector<Tcol, NumType>
    mult_vector_matrix_linear_comb(const Vector<Trow, NumType> & vec) const
    {
      if (&vec.get_domain() != &get_row_domain())
        throw std::domain_error("mult_vector_matrix_linear_comb: "
                                "domain mismatch");
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
        throw std::domain_error("mult_matrix_vector_dot_product: "
                                "domain mismatch");
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
        throw std::domain_error("mult_vector_matrix_dot_product: "
                                "domain mismatch");
      Vector<Tcol, NumType> ret_val(*cdomain_ptr);
      cdomain_ptr->for_each([&] (const Tcol & col)
                            {
                              ret_val.set_entry(col, get_col_vector(col)*vec);
                            });
      return ret_val;
    }

    Vector<Trow, NumType>
    operator * (const Vector<Tcol, NumType> & vec) const
    {
      return mult_matrix_vector_linear_comb(vec);
    }

  private:

    template <typename T2row, typename T2col>
    void validate_domains_for_mult(const Matrix<T2row, T2col, NumType> & m)
      const
    {
      if (cdomain_ptr->size() != m.rdomain_ptr->size())
        throw std::domain_error("AxB: Col domain size different from "
                                "row domain");
    }

  public:

    /// set the row row with the values of vector vec
    Matrix & set_vector_as_row(const Trow & row, const Vector<Tcol> & vec)
    {
      if (&vec.get_domain() != cdomain_ptr)
        throw std::domain_error("set_vector_as_row: domain vec mismatch");
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
        throw std::domain_error("set_vector_as_col: domain vec mismatch");
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
      static_assert(is_same<Trow, Tcol>::value,
                    "Row domain is not equal to column domain");

      Matrix<Trow, Tcol, NumType> i(*rdomain_ptr, *rdomain_ptr);
      rdomain_ptr->for_each([&i] (const Trow & row)
                            {
                              i.set_entry(row, row, 1);
                            });
      return i;
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
      Matrix<Trow, T2col> ret_val(*rdomain_ptr, m.get_col_domain());
      rdomain_ptr->
        for_each([this, &m, &ret_val] (const Trow & row)
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
      Matrix<Trow, T2col> ret_val(*rdomain_ptr, m.get_col_domain());
      m.get_col_domain().
        for_each([this, &m, &ret_val] (const Tcol & col)
                 {
                   ret_val.set_vector_as_col(col,
                                             *this * m.get_col_vector(col));
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
      DynList<std::string> collens = cols.
        template maps<std::string>([] (const ColVector & vec)
          {
            return vec.to_list().
              template foldl<std::string>
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

      DynList<std::string> longers = zip(collens, col_labels).
        template maps <std::string>([] (const StrPair & p)
                                    {
                                      return p.first.size() > p.second.size() ?
                                      p.first : p.second;
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
      return entries.all([&m, this] (const Pair & p)
                         {
                           const Trow & row = p.first.first;
                           const Tcol & col = p.first.second;
                           return are_equal(m.get_entry(row, col), p.second);
                         })
        and m.entries.all([this] (const Pair & p)
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
                                ret_val.set_entry(p1.first, p2.first,
                                                  p1.second*p2.second);
                              });
                });
    return ret_val;
  }


} // end namespace Aleph

# endif // AH_MATRIX_H
