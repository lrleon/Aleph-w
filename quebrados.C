# include <stdexcept>
# include <iostream>
# include <fstream>
# include <math.h>


using namespace std;

    /// Tipo para representar quebrados mixtos (parte entera + parte fraccional)
struct Quebrado
{
  int e; // parte entera
  int n; // numerador
  int d; // denominador
};


    /// Asigna el quebrado q1 al quebrado q2. Después de la llamada q2 == q1
void asignar_quebrado(const Quebrado & q1, Quebrado & q2)
{
  q2.e = q1.e;
  q2.n = q1.n;
  q2.d = q1.d;
}


    /// retorna un quebrado cero
Quebrado quebrado_cero()
{
  Quebrado q;

  q.e = 0;
  q.n = 0;
  q.d = 1;

  return q;
}


    /// Retorna un quebrado uno
Quebrado quebrado_uno()
{
  Quebrado q;

  q.e = 1;
  q.n = 0;
  q.d = 1;

  return q;
}


    /// Retorna true si el quebrado es propio
bool es_propio(const Quebrado & q)
{
  return q.d > q.n;
}


    /// retorna true si n es un número primo; false de lo contrario
bool es_primo (int n)
{
  int max = ceil(sqrt(n));

  for (int i = 2; i < max; i += 2)
    if ((n % i) == 0)
      return false;
  
  return true;
}	


    /// Hace propio el quebrado q
void hacer_propio(Quebrado & q)
{
  if (es_propio(q))
    return;

  q.e = q.e + q.n/q.d;
  q.n    = q.n % q.d;
} 


    /// Reduce al menor denominador el quebrado q
void reducir_al_menor_denominador(Quebrado & q)
{
  while ((q.n % 2 == 0) and (q.d % 2 == 0))
    {
      q.n   = q.n/2;
      q.d = q.d/2;
    }
  
  int ultimo_divisor = ceil(sqrt(q.d));
  for (int divisor = 3; divisor <= ultimo_divisor; divisor += 2)
    if ((q.n % divisor == 0) and (q.d % divisor == 0))
      {
	q.n   = q.n/divisor;
	q.d = q.d/divisor; 
      }
}


    /// simplifica q a que sea propio y tenga el menor denominador
void simplificar_quebrado(Quebrado & q)
{
  hacer_propio(q);
  reducir_al_menor_denominador(q);
}	


void leer_quebrado_por_consola(Quebrado & q)
{
  cout << "ingrese entero, numerador y denominador ";
  cin >> q.e;
  cin >> q.n;
  cin >> q.d;
}


void escribir_quebrado_por_consola(const Quebrado & q)
{
  cout << q.e << " " << q.n << "/" << q.d <<endl;
}


    /// Retorna un quebrado resultado de la suma de q1 + q2
Quebrado suma(const Quebrado & q1, const Quebrado & q2)
{
  Quebrado r;

  r.e = q1.e + q2.e;

  if (q1.n == 0)
    {
      r.n = q2.n;
      r.d = q2.d;
    }
  else if (q2.n == 0)
    {
      r.n = q1.n;
      r.d = q1.d;
    }
  else
    {
      r.d = q1.d * q2.d;
      r.n = (r.d / q1.d)*q1.n + (r.d / q2.d)*q2.n;
    }
  
  simplificar_quebrado(r);

  return r;
}


    /// Niega el quebrado q; es decir le cambia su signo
Quebrado negado((const Quebrado & q)
{
  Quebrado r;
  asignar_quebrado(q, r);
  r.e = -r.e;

  return r;
}

    /// Retorna un quebrado resultado del la resta q1 - q2
Quebrado resta(const Quebrado & q1, const Quebrado & q2)
{
  return suma(q1, negado(q2));
}

    /// Retorna un quebrado resultado del producto q1*q2
Quebrado multiplica(const Quebrado & q1, const Quebrado & q2)
{
  Quebrado r1 = quebrado_cero();
  r1.e = q1.e*q2.e;

  Quebrado r2 = quebrado_cero();
  r2.n = q1.e*q2.n;
  r2.d = q2.d;
  hacer_propio(r2);

  Quebrado r3 = quebrado_cero();
  r3.n = q1.n*q2.e;
  r3.d = q1.d;
  hacer_propio(r3);

  Quebrado r4 = quebrado_cero();
  r4.n = q1.n*q2.n;
  r4.d = q1.d*q2.d;
  hacer_propio(r4);

  return suma(r1, suma(r2, suma(r3, r4)));
}


    /// Retorna el quebrado resultante de 1/q
Quebrado invertido(const Quebrado & q)
{
  if (q.e == 0 and q.n == 0)
    throw domain_error("División por cero");

  Quebrado r = quebrado_cero();
  r.n = q.d;
  r.d = q.e*q.d + q.n;

  hacer_propio(r);

  return r;
}


    /// Retorna el quebrado división q1/q2
Quebrado divide(const Quebrado & q1, const Quebrado & q2)
{
  return multiplica(q1, invertido(q2));
}


int main()
{
  
}
