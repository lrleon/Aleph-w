

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
 }
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
