# include <cassert>
# include <al-matrix.H>

int main(int argc, char *argv[])
{
  size_t n = argc > 1 ? atoi(argv[1]) : 5;

  const Matrix<string, string>::RDomain rd = {"A", "B", "C"};
  const Matrix<string, string>::CDomain cd = {"a", "b", "c"};

  Matrix<string, string> m1(rd, cd);
  
  m1.print();

  cout << "m1 = " << endl 
       << m1 << endl;

  Matrix<string, string> m2(rd, cd, 
			{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}});

  m2.print();

  cout << "The rows = ";
  m2.row_domain_list().for_each([&m2] (const string & str)
				{ 
				  cout << str << " ";
				  assert(m2.get_row_domain().has(str));
				});
  cout << endl
       << endl
       << "The cols = ";
  m2.col_domain_list().for_each([&m2] (const string & str)
				{ 
				  cout << str << " ";
				  assert(m2.get_col_domain().has(str));
				});
  cout << endl
       << "The row vectors = " << endl;
  m2.to_rowlist().for_each(/* Lambda */ [] (const Vector<string, double> & v)
			   {
			     cout << v.to_str() << endl << endl;
			   });
  cout << endl
       << "The column vectors = " << endl;
  m2.to_collist().for_each(/* Lambda */ [] (const Vector<string, double> & v)
			   {
			     cout << v.to_str() << endl << endl;
			   });
  cout << endl;

  Matrix<string, string>::RDomain rd3 = {"A", "Adios", "Chao"};
  Matrix<string, string>::CDomain cd3 = {"Hola, Mundo!", "B", "C"};
  Matrix<string, string> m3(rd3, cd3,
			    {{200.45, -1001.876,  -2},
			     { 3,    -10046.7261, 0.67},
                             { 87,   45,          -113}});
      
  cout << "Imprimiendo listas de filas" << endl;
  m3.row_domain_list().for_each([&m3] (const std::string & row)
  {
    m3.get_row_as_list(row).for_each([] (const double & val)
				{
				  cout << val << " ";
				});
    cout << endl;
  });
  cout << endl
       << endl
       << "Imprimiendo listas de columnas" << endl;
  m3.col_domain_list().for_each([&m3] (const std::string & col)
  {
    m3.get_col_as_list(col).for_each([] (const double & val)
				     {
				       cout << val << " ";
				     });
    cout << endl;
  });
  cout << endl
       << endl
       << "m2 = " << endl
       << m2.to_str() << endl
       << endl
       << endl
       << "m2' = " << endl
       << m2.transpose().to_str() << endl
       << endl
       << "m3 = " << endl
       << m3.to_str() << endl
       << endl
       << endl
       << "m3' = " << endl
       << m3.transpose().to_str()
       << endl
       << endl;

  Matrix<std::string, std::string> m3t = m3.transpose();

  assert(zip(m3.row_domain_list(), m3.col_domain_list()).all
	 (/* Lambda */ [&m3, &m3t] 
	  (const std::pair<std::string, std::string> & p) 
	  {
	    const std::string & row = p.first;
	    const std::string & col = p.second;
	    return m3.get_entry(row, col) == m3t.get_entry(col, row);
	  }));

  m3.get_col_domain().keys().for_each([&m3] (const string & col)
  {
    cout << m3.get_col_vector(col).to_str() << endl
	 << endl;
  });
  cout << endl;
  m3.get_row_domain().keys().for_each([&m3] (const string & row)
  {
    cout << m3.get_row_vector(row) << endl
	 << endl;
  });
  cout << endl;

  Vector<string> vec1(cd, {1, 0, 2});
  Vector<string> vec2(rd, {1.5, 0, 3});
  
  cout << "vec1 = " << endl
       << vec1 << endl
       << "m2 = " << endl
       << m2 << endl
       << endl
       << "m2*vec1 as linear combination = " << endl
       << m2.mult_matrix_vector_linear_comb(vec1) << endl
       << endl
       << "m2*vec1 as sparse product = " << endl
       << m2.mult_matrix_vector_sparse(vec1) << endl
       << endl
       << "vec2 = " << endl
       << vec2 << endl
       << endl
       << "vec2*m2 as linear combination = " << endl
       << m2.mult_vector_matrix_linear_comb(vec2) << endl
       << endl
       << "m2*vec1 as dot_product = " << endl
       << m2.mult_matrix_vector_dot_product(vec1) << endl
       << endl
       << "vec2*m2 as dot product = " << endl
       << m2.mult_vector_matrix_dot_product(vec2) << endl
       << endl
       << "v1 = " << endl
       << vec1 << endl
       << endl
       << "v2 = " << endl
       << vec2 << endl
       << endl
       << "v1*v2 (outer product) = " << endl
       << outer_product<string, string>(vec1, vec2) << endl
       << endl;
  
  assert(m2.mult_matrix_vector_linear_comb(vec1) == 
	 Vector<string>(rd, {7, 16, 25}));

  assert(m2.mult_matrix_vector_sparse(vec1) ==
	 m2.mult_matrix_vector_dot_product(vec1));

  assert(m2.mult_vector_matrix_linear_comb(vec2) ==
	 Vector<string>(cd, {22.5, 27, 31.5}));

  assert(m2.mult_matrix_vector_linear_comb(vec1) ==
	 m2.mult_matrix_vector_dot_product(vec1));

  assert(m2.mult_vector_matrix_linear_comb(vec2) ==
	 m2.mult_vector_matrix_dot_product(vec2));

  assert(m2*vec1 == Vector<string>(rd, {7, 16, 25}));

  assert(vec2*m2 == Vector<string>(cd, {22.5, 27, 31.5}));

  {
    const Matrix<string, string>::RDomain R = {"A", "B", "C", "D"};
    const Matrix<string, string>::CDomain C = {"a", "b", "c"};

    Matrix<string, string> 
      M1(R, C, {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}});

    Matrix<string, string> 
      M2(C, R, {{1, 2, 3, 4}, {5, 6, 7, 8,}, {9, 10, 11, 12}});

    cout << "M1 = " << endl
	 << M1 << endl
	 << endl
	 << "M2 = " << endl
	 << M2 << endl
	 << endl
	 << "M1*M2 (via vector_matrix_mult) =" << endl
	 << M1.vector_matrix_mult(M2) << endl
	 << endl
	 << "M1*M2 (via matrix_vector_mult) = " << endl
	 << M1.matrix_vector_mult(M2) << endl
	 << endl;

    assert(M1.vector_matrix_mult(M2) == M1.matrix_vector_mult(M2));
  }

  {
    const IntRange D(3);
    typedef Matrix<int, int> MATRIX;

    MATRIX M1(D, D, {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}});
    MATRIX M2(D, D, {{2, 1, 0}, {2, 3, -1}, {-2, 0, 1}});

    cout << "M1 = " << endl
	 << M1 << endl
	 << endl
	 << "M2 = " << endl
	 << M2 << endl
	 << endl
	 << "M1*M2 = " << endl
	 << M1.matrix_vector_mult(M2) << endl
	 << endl
	 << "M2*M1 = " << endl
	 << M2.matrix_vector_mult(M1) << endl
	 << endl
	 << "M1*I = " << endl
	 << M1.matrix_vector_mult(M1.identity()) << endl
	 << endl
	 << "I*M1 = " << endl
	 << M1.identity().matrix_vector_mult(M1) << endl
	 << endl
	 << "M1 + M2 =" << endl
	 << M1 + M2 << endl
	 << endl
	 << "2*(M1 + M2) =" << endl
	 << 2.0*(M1 + M2) << endl
	 << endl;

    assert(M1.vector_matrix_mult(M2) == M1.matrix_vector_mult(M2));
    assert(M2.vector_matrix_mult(M1) == M2.matrix_vector_mult(M1));
  }

  return 0;
}

