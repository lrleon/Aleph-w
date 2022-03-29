

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

  State solve()
  {
    if (state != Not_Solved)
      throw std::logic_error("solve() has already been called");
    if (num_rest == 0)
      throw std::logic_error("linear program without restrictions");

    for (int i, j; true;)
      {
        const Simplex<T>::State state = select_pivot(i, j);
        if (state == Simplex<T>::Unbounded or state == Simplex<T>::Solved)
          return state;

        to_pivot(i, j);
      } 
  }

    /// Carga los valores de las variables solución
  void load_solution() noexcept
  {
    for (int j = 0; j < num_var; j++)
      solution[j] = find_value(j);
  }

     /// Retorna un puntero al arreglo con las soluciones. Previamente
     /// debe haberse llamado a load_solution().
  const T & get_solution(size_t i) const noexcept
  {
    assert(i < num_var);
    return solution[i];
  }

      /// Retorna el valor de la función objetivo.
  T objetive_value() const noexcept
  {
    T sum = 0;
    for (int i = 0; i < num_var; i++)
      sum += solution[i]*objetive[i];
    return sum;
  }

      /// Retorna true si la solución obtenida satisface las
      /// restricciones; false de lo contrario.
  bool verify_solution() const
  {
    for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne())
      {
        T * rest = it.get_curr_ne();
        T sum = 0;
        for (int j = 0; j < num_var; j++)
          sum += rest[j]*solution[j];
        
        if (sum > rest[num_var])
          return false;
      }
    return true;
  }

  void print_matrix()
  {
    for (int i = 0; i <= num_rest; ++i)
      {
        for (int j = 0; j <= num_var + num_rest; j++)
          cout << float_f(m->read(i, j), 2) << " ";

        cout << endl;
      }
  }

  void latex_matrix(const string & name, int d = 2, int p = -1, int q = -1)
  {
    ofstream out(name, ios::out);

    const int cols = num_var + num_rest;

    out << "$\\left(\\begin{array}{c";
    for (int i = 0; i < cols; i++)
      out << "c";
    out << "}" << endl;

    for (int i = 0; i <= num_rest; ++i)
      {
        for (int j = 0; j <= cols; j++)
          {
            if (i == p and j == q)
              out << "\\circled{" << float_f(m->read(i, j), d) << "}" << " ";
            else
              out << float_f(m->read(i, j), d) << " ";
            if (j != cols)
              out << "& ";
          }

        if (i != num_rest)
          out << "\\\\";

        out << endl;
      }
    out << "\\end{array}\\right)$" << endl;
  }

  void latex_linear_program(const string & name)
  {
    ofstream out(name, ios::out);
    out << "\\begin{equation*}" << endl
        << "Z = " ;
    for (int i = 0; i < num_var; i++)
      {
        if (objetive[i] == 0.0)
          continue;

        if (i > 0)
          out << " + ";

        if (objetive[i] != 1.0)
          out << objetive[i];
        out << "x_" << i; 
      }
    out << endl
        << "\\end{equation*}" << endl
        << "Sujeto a:" << endl
        << "\\begin{eqnarray*}" << endl;

    for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne())
      {
        T * rest = it.get_curr_ne();

        for (int i = 0; i < num_rest; i++)
          {
            if (rest[i] == 0.0)
              continue;

            if (i > 0)
              out << " + ";

            if (rest[i] != 1.0)
              out << rest[i];
            out << " x_" << i;
          }

        out << " & \\leq & " << rest[num_rest];

        if (not it.is_in_last())
          out << " \\\\";
              
        out << endl;
      }
    out << "\\end{eqnarray*}" << endl;
  }

  size_t get_num_restrictions() const noexcept { return num_rest; }

  size_t get_num_vars() const noexcept { return num_var; }

  T * get_objetive_function() noexcept
  {
    return objetive;
  }

      /// retorna el coeficiente idx de la restricci{on rest_num
  T & get_restriction_coef(int rest_num, int idx) 
  {
    verify_var_index(idx);

    return get_restriction(rest_num)[idx];
  } 

  void put_restriction_coef(int rest_num, int idx, const T & coef)
  {
    get_restriction_coef(rest_num, idx) = coef;
  }

  void prepare_linear_program()
  {
    create_matrix();
  }
};


} // end namespace Aleph

# endif // SIMPLEX_H

