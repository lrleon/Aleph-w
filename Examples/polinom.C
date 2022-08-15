
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

  printf("\np1 =\n\t"); p1.print();
  printf("\np2 =\n\t"); p2.print();
  p1 += p2;
  printf("\np1 += p2:\n\t"); p1.print();
  Polinomio p3(p1*p2);
  printf("\np1*p2 =\n\t"); p3.print();
}
