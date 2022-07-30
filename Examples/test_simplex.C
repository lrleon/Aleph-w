
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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

# include <Simplex.H>

int main()
{
  Simplex<float> s(4);

  s.put_objetive_function_coef(0, 40);
  s.put_objetive_function_coef(1, 50);
  s.put_objetive_function_coef(2, 60);
  s.put_objetive_function_coef(3, 30);

  float * r1 = s.put_restriction();
  float * r2 = s.put_restriction();
  float * r3 = s.put_restriction();
  float * r4 = s.put_restriction();

  r1[0] = 2; r1[1] = 1; r1[2] = 2; r1[3] = 2; r1[4] = 205;

  r2[0] = 1; r2[1] = 1; r2[2] = 3; r2[3] = 1; r2[4] = 205;

  r3[0] = 1; r3[1] = 3; r3[2] = 4; r3[4] = 255;

  r4[0] = 3; r4[1] = 2; r4[2] = 2; r4[3] = 2; r4[4] = 250;

  s.latex_linear_program("sistema-simplex.tex");

  s.prepare_linear_program();

  Simplex<float>::State  state = s.latex_solve("mat");

  if (state == Simplex<float>::Unbounded)
    cout << "No se puede resolver el sistema" << endl;

  s.load_solution();

  s.print_matrix();

  for (int i = 0; i < 4; i++)
    cout << "x" << i << " = " << s.get_solution(i) << endl;

  assert(s.verify_solution());
}
