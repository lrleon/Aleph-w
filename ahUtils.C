
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


# include <unistd.h>
# include <sys/resource.h>
# include <algorithm>
# include <ahUtils.H>


bool Aleph::resize_process_stack(size_t new_size)
{
  const rlim_t kStackSize = new_size;  
  struct rlimit rl;
  int result;

  result = getrlimit(RLIMIT_STACK, &rl);
  if (result == 0)
    if (rl.rlim_cur < kStackSize)
      {
	rl.rlim_cur = kStackSize;
	result = setrlimit(RLIMIT_STACK, &rl);

	return result == 0;
      }

  return false;
}
