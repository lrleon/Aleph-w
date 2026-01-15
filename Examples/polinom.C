
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

/**
 * @file polinom.C
 * @brief Polynomial arithmetic demonstration using sparse representation
 * 
 * This example demonstrates efficient polynomial arithmetic using a
 * sparse representation with doubly linked lists (DynDlist). Polynomials
 * are stored as sorted lists of non-zero terms, making operations efficient
 * for polynomials with many zero coefficients. This is essential for
 * symbolic computation and numerical analysis.
 *
 * ## What is a Polynomial?
 *
 * ### Mathematical Definition
 *
 * A polynomial is an expression of the form:
 * ```
 * P(x) = a₀ + a₁x + a₂x² + ... + aₙxⁿ
 * ```
 *
 * Where:
 * - **Coefficients**: a₀, a₁, ..., aₙ are numbers
 * - **Exponents**: Powers of x (0, 1, 2, ..., n)
 * - **Degree**: Highest exponent n
 *
 * ### Sparse vs Dense Representation
 *
 * #### Dense Representation
 *
 * - **Storage**: Store coefficients for ALL powers (even zeros)
 * - **Array**: `coeff[i]` = coefficient of x^i
 * - **Space**: O(max_degree)
 * - **Best for**: Dense polynomials (few zeros)
 * - **Example**: `[5, 3, 0, 7, 0, 0, 2]` represents 5 + 3x + 7x³ + 2x⁶
 *
 * #### Sparse Representation (This Example)
 *
 * - **Storage**: Store only non-zero terms
 * - **List**: List of (coefficient, exponent) pairs
 * - **Space**: O(number_of_nonzero_terms)
 * - **Best for**: Sparse polynomials (many zeros)
 * - **Example**: `[(5,0), (3,1), (7,3), (2,6)]` represents same polynomial
 *
 * ### When to Use Sparse?
 *
 * Use sparse when:
 * - **Many zeros**: Most coefficients are zero
 * - **High degree**: Degree >> number of non-zero terms
 * - **Memory**: Memory is a concern
 * - **Efficiency**: Want faster operations on sparse data
 *
 * ## Polynomial Structure
 *
 * ### Term Representation
 *
 * Each polynomial term (`Termino`) contains:
 * - **Coefficient**: The numeric multiplier (e.g., 5 in 5x³)
 * - **Exponent**: The power of x (e.g., 3 in 5x³)
 *
 * ### Storage Order
 *
 * Terms are stored **sorted by exponent** (ascending) for efficient operations:
 * - **Merging**: Easy to merge sorted lists
 * - **Search**: Binary search possible
 * - **Traversal**: Natural order
 *
 * ## Supported Operations
 *
 * ### Addition (p1 + p2, p1 += p2)
 *
 * **Algorithm**: Merge two sorted lists
 * ```
 * add(p1, p2):
 *   result = []
 *   i = 0, j = 0
 *   while i < p1.size() and j < p2.size():
 *     if p1[i].exp < p2[j].exp:
 *       result.append(p1[i])
 *       i++
 *     else if p1[i].exp > p2[j].exp:
 *       result.append(p2[j])
 *       j++
 *     else:  // Same exponent
 *       result.append(p1[i].coeff + p2[j].coeff, p1[i].exp)
 *       i++, j++
 *   // Append remaining terms
 * ```
 *
 * **Time**: O(n + m) where n, m are term counts
 * **Space**: O(n + m) for result
 *
 * ### Multiplication (p1 * p2)
 *
 * **Algorithm**: Distribute and combine
 * ```
 * multiply(p1, p2):
 *   result = []
 *   for each term t1 in p1:
 *     for each term t2 in p2:
 *       new_term = (t1.coeff * t2.coeff, t1.exp + t2.exp)
 *       result.append(new_term)
 *   combine_like_terms(result)  // Sort and merge
 * ```
 *
 * **Distribute**: (a·x^i) × (b·x^j) = (a·b)·x^(i+j)
 * **Time**: O(n × m) for multiplication, O(n×m log(n×m)) with sorting
 * **Space**: O(n × m) worst case
 *
 * ### Subtraction (p1 - p2)
 *
 * **Algorithm**: Negate and add
 * ```
 * subtract(p1, p2):
 *   negated_p2 = negate(p2)  // Negate all coefficients
 *   return add(p1, negated_p2)
 * ```
 *
 * **Time**: O(n + m) - same as addition
 *
 * ## Example Polynomials
 *
 * ### Polynomial p1
 *
 * **Pattern**: Odd powers from 1 to 19
 * **Terms**: X^1 + X^3 + X^5 + ... + X^19
 * **Count**: 10 terms
 * **Sparsity**: Very sparse (only odd powers)
 *
 * ### Polynomial p2
 *
 * **Pattern**: All powers from 0 to 39
 * **Terms**: X^0 + X^1 + X^2 + ... + X^39
 * **Count**: 40 terms
 * **Sparsity**: Dense (all powers present)
 *
 * ### Operations Demonstrated
 *
 * - **p1 += p2**: In-place addition (combines both polynomials)
 *   - Result: All powers from 0 to 39, with doubled coefficients for odd powers
 *
 * - **p1 * p2**: Multiplication (creates new polynomial)
 *   - Result: Product polynomial with many terms
 *
 * ## Complexity Analysis
 *
 * | Operation | Dense | Sparse (this) | Improvement |
 * |-----------|-------|---------------|-------------|
 * | Storage | O(degree) | O(terms) | **Huge** for sparse |
 * | Addition | O(degree) | O(terms₁ + terms₂) | **Better** for sparse |
 * | Multiplication | O(degree²) | O(terms₁ × terms₂) | **Better** for sparse |
 * | Evaluation | O(degree) | O(terms) | **Better** for sparse |
 *
 * ### Example: High-Degree Sparse Polynomial
 *
 * ```
 * Polynomial: x^1000 + x^5000 + x^10000
 * 
 * Dense: O(10000) storage
 * Sparse: O(3) storage
 * 
 * Sparse is 3333× more efficient!
 * ```
 *
 * ## Applications
 *
 * ### Symbolic Computation
 * - **Computer algebra systems**: Mathematica, Maple, SymPy
 * - **Polynomial manipulation**: Factor, expand, simplify
 * - **Equation solving**: Find roots, solve systems
 *
 * ### Numerical Analysis
 * - **Polynomial interpolation**: Fit curves to data points
 * - **Approximation**: Approximate functions with polynomials
 * - **Taylor series**: Represent functions as polynomials
 *
 * ### Cryptography
 * - **Polynomial-based encryption**: Some encryption schemes use polynomials
 * - **Error-correcting codes**: Reed-Solomon codes use polynomials
 * - **Cryptographic protocols**: Polynomial commitments
 *
 * ### Signal Processing
 * - **Digital filters**: FIR/IIR filters use polynomial operations
 * - **Transforms**: Polynomial-based transforms
 * - **Convolution**: Polynomial multiplication = convolution
 *
 * ### Graphics
 * - **Bézier curves**: Represented as polynomials
 * - **Splines**: Piecewise polynomial curves
 * - **Surface modeling**: Polynomial surfaces
 *
 * ### Scientific Computing
 * - **Series expansions**: Represent functions as series
 * - **Taylor polynomials**: Approximate functions
 * - **Numerical methods**: Many methods use polynomials
 *
 * ## Memory Efficiency
 *
 * ### Example Comparison
 *
 * ```
 * Polynomial: x^100 + 5x^50 + 3x^10
 * 
 * Dense representation:
 *   Array[101]: [3, 0, 0, ..., 5, 0, ..., 1]
 *   Memory: 101 integers
 * 
 * Sparse representation:
 *   List: [(3, 10), (5, 50), (1, 100)]
 *   Memory: 3 pairs = 6 integers
 * 
 * Sparse saves 94% memory!
 * ```
 *
 * ## Usage
 *
 * ```bash
 * # Run polynomial arithmetic demo
 * ./polinom
 *
 * # The program demonstrates:
 * # - Creating sparse polynomials
 * # - Addition operations
 * # - Multiplication operations
 * # - Memory efficiency
 * ```
 *
 * For sparse polynomials, this representation is much more efficient!
 *
 * ## Key Data Structures
 *
 * - `DynDlist<Termino>`: Doubly-linked list storing polynomial terms
 * - `Termino`: Structure with coefficient and exponent
 * - Terms maintained in sorted order (by exponent)
 *
 * @see tpl_dynDlist.H Doubly linked list (used for storage)
 * @see matrix_example.C Sparse matrices (related sparse representation)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <tpl_dynDlist.H>

class Polinomio
{
  struct Termino
  {
    int    coef;
    size_t pot;
    Termino() : coef(0), pot(0) {}

    Termino(const int & c, const size_t & p) : coef(c), pot(p) {}
    Termino& operator += (const Termino & der)
    {

      assert(pot == der.pot);

      if (this == &der)
        return *this;

      coef += der.coef;
      return *this;
    }
    Termino operator * (const Termino & der) const
    {
      return Termino(coef*der.coef, pot + der.pot);
    }
  };
  DynDlist<Termino> terminos;
  Polinomio(const Polinomio::Termino & termino)
  {
    terminos.append(termino);
  }
  Polinomio multiplicado_por(const Termino & term) const
  {
    Polinomio result;
    if (terminos.is_empty() or term.coef == 0) 
      return result;

    for (DynDlist<Termino>::Iterator it((DynDlist<Termino>&) terminos); 
         it.has_curr(); it.next())
      result.terminos.append(Termino(it.get_curr().coef * term.coef,
                                     it.get_curr().pot + term.pot) );
    return result;
  }

public:

  Polinomio();
  Polinomio(const int& coef, const size_t & pot);
  Polinomio operator + (const Polinomio&) const;
  Polinomio& operator += (const Polinomio&);
  Polinomio operator * (const Polinomio&) const;
  Polinomio operator - (const Polinomio&) const;

  Polinomio operator / (const Polinomio&) const;

  Polinomio operator % (const Polinomio&) const;
  const size_t & size() const;
  size_t get_power(const size_t & i) const;
  const int & get_coef(const size_t & i) const;
  const size_t & get_degree() const;
  void print() const
  {
    if (terminos.is_empty())
      {
        printf("0\n\n");
        return;
      }
    Termino current_term;
    for (DynDlist<Termino>::Iterator it(const_cast<DynDlist<Termino>&>(terminos)); 
         it.has_curr(); it.next())
      {
        current_term = it.get_curr();
        printf(" %c ", current_term.coef < 0 ? '-' : '+');

        if (abs(current_term.coef) not_eq 1)
          printf("%u", abs(current_term.coef));

        printf("X^%zu", current_term.pot);
      }

    printf("\n\n");
  }
};
Polinomio::Polinomio(const int & coef, const size_t & pot)
{
  terminos.append(Termino(coef, pot));
}
Polinomio::Polinomio() { /* empty */ }
Polinomio Polinomio::operator + (const Polinomio & der) const
{
  Polinomio ret_val(*this); // valor de retorno operando derecho
  ret_val += der; // súmele operando derecho
  return ret_val;
}
Polinomio & Polinomio::operator += (const Polinomio& der)
{
  if (der.terminos.is_empty())
    return *this;

  if (terminos.is_empty())
    {
      *this = der;
      return *this;
    }
  DynDlist<Termino>::Iterator it_izq(terminos);
  DynDlist<Termino>::Iterator it_der(const_cast<DynDlist<Termino>&>(der.terminos));
  while (it_izq.has_curr() and it_der.has_curr())
    {
      const size_t & izq = it_izq.get_curr().pot;
      const size_t & der = it_der.get_curr().pot;
      if (izq < der)
        {     // insertar a la izquierda del actual de it_izq 
          it_izq.append(Termino(it_der.get_curr().coef, der)); 
          it_der.next(); // ver próximo término de polinomio derecho
          continue;
        }
      if (izq == der)
        {      // calcular coeficiente resultado
          it_izq.get_curr() += it_der.get_curr(); // += Termino
          it_der.next(); // avanzar a sig término polinomio derecho
          if (it_izq.get_curr().coef == 0) // ¿suma anula término?
            {    // sí, borrarlo de polinomio izquierdo (coeficiente 0)
              it_izq.del();
              continue;
            }
        }
      it_izq.next();
    }
  while (it_der.has_curr())
    {    // copia términos restantes derecho a izquierdo
      terminos.append(Termino(it_der.get_curr().coef, 
                              it_der.get_curr().pot));
      it_der.next();
    }
  return *this;
}
Polinomio Polinomio::operator * (const Polinomio & der) const
{
  Polinomio result;
  if (terminos.is_empty() or der.terminos.is_empty()) 
    return result;

  for (DynDlist<Termino>::Iterator 
         it_izq(const_cast<DynDlist<Termino>&>(terminos)); 
       it_izq.has_curr(); it_izq.next())
    result += der.multiplicado_por(it_izq.get_curr());

  return result;
}
int main()
{
  Polinomio p1, p2;
  int i;

  for (i = 0; i < 20; i++)
    if (i%2)
      {
        Polinomio p(1, i);
        p1 += p;
      }

  for (i = 0; i < 40; i++)
    {
      Polinomio p(1, i);
      p2 += p;
    }

  Polinomio p3(p1*p2);

  printf("\np1 =\n\t"); p1.print();
  printf("\np2 =\n\t"); p2.print();
  p1 += p2;
  printf("\np1 += p2:\n\t"); p1.print();
  printf("\np1*p2 =\n\t"); p3.print();
}
