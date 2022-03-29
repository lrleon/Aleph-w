

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

            break;
          }
        case End:
          {
            while (op_stack.top() != '$')
              apply(val_stack, op_stack);
            op_stack.pop(); // debe ser '$'
            const int ret_val = val_stack.pop();

            if ((val_stack.size() != 0) or (op_stack.size() != 0))
              AH_ERROR("Bad expression");

            return ret_val;
          }

        case  Error:
        default: AH_ERROR("Bad token detected");

        } 
    }
}

int main(int, char** argc)
{
  printf("\t%d\n\n", eval(argc[1]));
}
