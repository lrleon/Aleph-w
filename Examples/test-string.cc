
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

# include <iostream>
# include <ah-string-utils.H>

const string text =
  "Esta es la historia de un programador llamado Alejandro que era admirado por Leandro por su velocidad de programacion. Pero el gran problema de Alejandro era que hablaba mucho mas rapido que la velocidad de pensamiento de Leandro. Consecuentemente, Leandro nunca lograba entender a Alejandro";

int main()
{
  split_text_into_words(text).
    for_each([] (const auto & s) { cout << s << endl; });

  cout << justify_text(text, 50, 10) << endl
       << endl
       << align_text_to_left(text, 50, 4) << endl
       << endl
       << shift_lines_to_left(justify_text(text, 60), 2) << endl
       << "FIN!" << endl;

  string str = "Hipógrifo violento que corriste parejas con el viento";
  for (size_t i = 0; i < str.size() + 1; ++i)
    {
      auto p = split_pos(str, i);
      cout << "  " << p.first << " " << i << " " << p.second << endl;
    }

  auto l = split_n(str, 7);
  for (auto it = l.get_it(); it.has_curr(); it.next())
    cout << it.get_curr() << "|";
  cout << endl;

  string s= "camelCaseString";
  split_camel_case(s).for_each([] (const auto & s) { cout << s << endl; });

  return 0;
}
