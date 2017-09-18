
# include <stdio.h>
# include <stdlib.h>


unsigned getDate(unsigned k /* dia */,
	      unsigned m /* mes */,
	      unsigned y /* ano */)
{
  m = (m == 1 || m == 2) ? m += 10 : m -= 2;

  if (m > 10)
    y--;

  unsigned C = y / 100; // Calcula siglo
  unsigned D = y % 100;

  unsigned f = ( ((int) (2.6*m - 0.2)) + k + D + D/4 + C/4 - 2*C) % 7;

  return f;
}


int main(int argn, char *argc[])
{
  int dia = atoi(argc[1]);
  int mes = atoi(argc[2]);
  int ano = atoi(argc[3]);

  int d = getDate(dia, mes, ano);
  char *s;
  switch (d)
    {
    case 0: s = "Domingo"; break;
    case 1: s = "Lunes"; break;
    case 2: s = "Martes"; break;
    case 3: s = "Miercoles"; break;
    case 4: s = "Jueves"; break;
    case 5: s = "Viernes"; break;
    case 6: s = "Sabado"; break;
    }

  printf("%s \n\n", s);
}
