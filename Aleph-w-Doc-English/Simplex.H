
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file Simplex.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef SIMPLEX_H
# define SIMPLEX_H

# include <limits>
# include <fstream>
# include <tpl_dynMat.H>
# include <tpl_dynDlist.H>
# include <format.H>

namespace Aleph {


/** 
  @brief Solver linear programs using the simplex method.

  @details Simplex<T> can express standard linear programs;
  that is, to maximize an objective function and a set of
  restrictions with positive coefficients and lower desigualdes or
  equal.

  For non-standard cases, the linear program can become one
   standard using the following criteria:
  -# If you have variable without zero lower bound,
   then you can use auxiliary variables.
  -# If a restriction is not less than or equal, then it can be
   refuse to make it less than or equal to.
  -# If you have a restrcción strict equality, then this is
   It can be modeled as two, or less than and greater than or equal to.
  .
  
  The T type must be numeric.
  It is not advisable to be whole.

  @ingroup Redes
*/
template <typename T> class Simplex
{
public:

/** 
  @enum System status 
*/
  enum State { Not_Solved, Solving, Unbounded, Solved, Unfeasible };

private:

  
    /* Select the cell of the objective function with lower value.
    Returns -1 if all cells are negative */
  int compute_pivot_col() const
  {
    T minimum = numeric_limits<T>::max();
    int p = -1;
    for (int i = 0, M = num_var + num_rest; i < M; i++)
      {
        const T & c = m->read(0, i);
        if (c < minimum)
          {
            p = i;
            minimum = c;
          }
      }
    return minimum >= 0 ? -1 : p;
  }

    /* B elements selected from the lower value of division
    col_idx between the coefficient of the objective function that is positive */
  int compute_pivot_row(int p) const
  {

    assert(p >= 0 and p < num_var + num_rest);

    int q = -1;
    T min_ratio = numeric_limits<T>::max();
    for (int i = q + 1, M = num_var + num_rest; i <= num_rest; i++)
      {
        const T val = m->read(i, M); 
        if (val < 0) 
          continue;

        const T den = m->read(i, p);
        if (den <= 0)
          continue;

        const T ratio = val / den;
        if (ratio < min_ratio)
          {
            q = i;
            min_ratio = ratio;
          }
      }
    return q;
  }
  State select_pivot(int & p, int & q) 
  {

    assert(state == Not_Solved or state == Solving);

    const int col = compute_pivot_col();
    if (col == -1) 
      return state = Solved;

    const int row = compute_pivot_row(col);
    if (row == -1)
      return state = Unbounded;

    p = row;
    q = col;

    return state = Solving;
  }
  void to_pivot(size_t p, size_t q)
  {

    assert(p <= num_rest and q <= num_var + num_rest);

    const int M = num_var + num_rest; /* Number of columns */
    const T pivot = m->read(p, q);
    for (int j = 0; j <= M; j++) /* Pivot row */
      if (j != q)
        m->write(p, j, m->read(p, j) / pivot);

    m->write(p, q, 1);

    for (int i = 0; i <= num_rest; i++) /* Other rows */
      for (int j = 0; j <= M; j++)
        if (i != p and j != q)
          m->write(i, j, m->read(i,j) - m->read(i,q)*m->read(p,j));

    for (int i = 0; i <= num_rest; i++) 
      if (i != p)
        m->write(i, q, 0);
  }
  T find_value(const size_t j) const
  {

    assert(j < num_var);

    T ret_val = 0.0;
    for (int i = 1, count = 0; i < num_rest; i++)
      {
        const T & value = m->read(i, j);
        if (value == 0.0)
          continue;

        if (value == 1.0)
          if (count++ == 0)  
            ret_val = m->read(i, num_var + num_rest);
          else
            return 0.0;
        else
          return ret_val;
      }
    return ret_val;
  }
  DynMatrix<T> * m;

  T * objetive;

  DynDlist<T*> rest_list;

  int num_var;
  int num_rest;

  T * solution;

  public:

  private:

    State state;

    void verify_var_index(int i)
    {
      if (i >= num_var)
        throw std::out_of_range
          (gnu::autosprintf("index %d out of range (%d max)", i, num_var - 1));
    }

    T * create_restriction()
    {
      T * ptr = new T [num_var + 1];
      rest_list.append(ptr);
      num_rest++;
      for (int i = 0; i <= num_var; i++)
        ptr[i] = 0;
      return ptr;
    }

    void create_matrix()
    {
      m = new DynMatrix<T> (num_rest + 1, num_var + num_rest + 1);

      int i = 0; /* Nbr_vars full coefficients of the objective function */
      for (; i < num_var; i++)
        m->write(0, i, -objetive[i]);
      
      i = 1; /* Full coefficients nbr_vars restrictions */
      for (typename DynDlist<T*>::Iterator it(rest_list); 
           it.has_current(); it.next(), i++)
        {
          T * rest = it.get_current();

          for (int j = 0; j < num_var; j++)
            m->write(i, j, rest[j]);

          m->write(i, num_var + i - 1, 1); /* Coef 1 ith varying slack */

          m->write(i, num_var + num_rest, rest[num_var]); 
        }
    }

public:

/** 
  @brief Constructor.

  @brief Initializes a linear program in standard form, without
  restrictions and with coefficients in the target fncion
  zero.

  @param[in] n Number of variables in the system.

  @throw bad_alloc If there is insufficient memory.
*/
  Simplex(int n) 

    : m(NULL), objetive(NULL), num_var(n), num_rest(0), state(Not_Solved)
  {
    objetive = new T [num_var];
    try
      {
        solution = new T [num_var];
      }
    catch (...)
      {
        delete objetive;
        throw;
      }
  }

/** 
  @brief Defines the coef coefficient of the variable i in the target APlayback,
  It is not valid if i is out of range.
*/
  void put_objetive_function_coef(int i, const T & coef)

  {
    verify_var_index(i);
    objetive[i] = coef;
  }

/** 
  @brief Define the objective function coefficients by a dynamic array;
  Just read the n - 1 coefficients.
*/
  void put_objetive_function(DynArray<T> & coefs)
  {
    for (int i = 0; i < num_var; i++)
      objetive[i] = coefs;
  }

/** 
  @brief Define the objective function coefficients by arrangement;
  Just read the n - 1 coefficients.
*/
  void put_objetive_function(T coefs[])
  {
    for (int i = 0; i < num_var; i++)
      objetive[i] = coefs;
  }
/** 
  @brief A restriction defined by arrangement.

  @details put_restriction(coefs) receives an array of n-1 coefficients more
  the corresponding limit value to a restriction in the form
  Standard type c1*x0 + c2*x_1 + ... + c(n-1)*x(n-1) <= cn.

  @param[in] coefs Settlement of coefficients to be read between 0 and n.
  If the pointer is null, then the restriction is created with
  zero coefficients, which can be defined later.

  @throw bad_alloc If a null pointer is passed and there is no
  to divide the memory array.

  @return A pointer to the array of restrictions.
*/
  T * put_restriction(T * coefs = NULL)

  {
    T * rest = create_restriction();
      
    if (coefs == NULL)
      return rest;

    for (int i = 0; i <= num_var; ++i)
      rest[i] = coefs[i];

    return rest;
  }

/** 
  @brief Returns a num_rest punero the settlement restriction. 
*/
  T * get_restriction(int rest_num)

  {
    if (rest_num >= num_rest)
      throw std::out_of_range
        (gnu::autosprintf("index %d out of range (%d max)", 
                          rest_num, num_rest - 1));
    int i = 1;
    for (typename DynDlist<T*>::Iterator it(rest_list); 
         it.has_current(); it.next())
      if (i == rest_num)
        return it.get_current();
  }

/** 
  @brief It defines a restriction by a dynamic array.

  details put_restriction(coefs) receives an array of n-1 coefficients more
   the corresponding limit value to a restriction in the form
   Standard type c1*x0 + c2*x_1 + ... + c(n-1)*x(n-1) <= cn.

  @param[in] coefs Settlement of coefficients to be read between 0 and n.
  
  @throw bad_alloc In case you do not have to divide the memory array.
  
  @return Returns a num_rest pointer the settlement restriction.
*/
  T * put_restriction(DynArray<T> & coefs)
  {
    T * rest = create_restriction();
      
    for (int i = 0; i <= num_var; ++i)
      rest[i] = coefs[i];

    return rest;
  }

  State latex_solve(char * name = NULL)
  {
    latex_matrix(gnu::autosprintf("%s-0.tex", name));
    for (int i, j, k = 1; true; k++)
      {
        const Simplex<T>::State state = select_pivot(i, j);

        string str = gnu::autosprintf("%s-%d.tex", name, k);

        if (state == Simplex<T>::Unbounded or state == Simplex<T>::Solved)
          {
            latex_matrix(str.c_str());
            return state;
          }

        latex_matrix(str.c_str(), 2, i, j);
        to_pivot(i, j);

        latex_matrix(name, i, j);
      } 
  }

/** 
  @brief Solves a linear program.

  @details Solves a linear program and completely correct
  specified.

  The state of solution are:
  -# Unbounded: The system is unlimited (design error)
  -# Solved: Found maximum solution. It should be
   careful, because the solution does not necessarily satisfy the
   restrictions.

  @return The state system solution.
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
/** 
  @brief Load the values of variables solution 
*/
  void load_solution()
  {
    for (int j = 0; j < num_var; j++)
      solution[j] = find_value(j);
  }

/** 
  @brief Returns a pointer to the array with solutions;
  It must have previously called load_solution(). 
*/
  const T & get_solution(size_t i) const
  {
    assert(i < num_var);
    return solution[i];
  }

/** 
  @brief Returns the value of the objective function. 
*/
  T objetive_value() const
  {
    T sum = 0;
    for (int i = 0; i < num_var; i++)
      sum += solution[i]*objetive[i];
    return sum;
  }

/** 
  @brief Returns true if the obtained solution satisfies the constraints; false otherwise.
*/
  bool verify_solution() const
  {
    for (typename DynDlist<T*>::Iterator it(rest_list); 
         it.has_current(); it.next())
      {
        T * rest = it.get_current();
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

  void latex_matrix(const char * name, int d = 2, int p = -1, int q = -1)
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

  void latex_linear_program(char * name)
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

    for (typename DynDlist<T*>::Iterator it(rest_list); 
         it.has_current(); it.next())
      {
        T * rest = it.get_current();

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

  size_t get_num_restrictions() const { return num_rest; }

  size_t get_num_vars() const { return num_var; }


  ~Simplex()
  {
    delete [] objetive;

    if (m != NULL)
      delete m;
  }

  T * get_objetive_function()
  {
    return objetive;
  }

/** 
  @brief Returns the coefficient idx of the restriction rest_num 
*/
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


} /* End namespace Aleph */

# endif /** SIMPLEX_H
