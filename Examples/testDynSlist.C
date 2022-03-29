

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
; itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

  for (int i = Num_Items; i < 2*Num_Items; i++)
    list.insert(i - Num_Items, i);

  for (itor.reset_first(); itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

  for (int i = 2*Num_Items; i < 3*Num_Items; i++)
    list.insert(0, i);

  for (itor.reset_first(); itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

  for (int i = 3*Num_Items; i < 4*Num_Items; i++)
    list.insert(list.size(), i);

  for (int i = 0; i < list.size(); i++)
    cout << list[i] << " ";

  cout << endl;

  for (int i = 0; i < list.size(); i++)
    cout << list[list.size() - i - 1] << " ";

  cout << endl;

  list.remove(list.size() - 1);

  list.remove(0);

  for (itor.reset_first(); itor.has_curr(); itor.next())
    cout << itor.get_curr() << " ";

  cout << endl;

}
