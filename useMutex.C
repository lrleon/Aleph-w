
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


# include <string>
# include <useMutex.H>
 # include <ah-errors.H>

using namespace std;

void init_mutex(pthread_mutex_t * m)
{
  const int status = pthread_mutex_init(m, nullptr);

  ah_domain_error_if(status != 0)
    << "cannot init mutex; error code " << to_string(status);
}

void init_mutex(pthread_mutex_t & m)
{
  init_mutex(&m);
}

void destroy_mutex(pthread_mutex_t * m)
{
  const int status = pthread_mutex_destroy(m);

  ah_domain_error_if(status != 0)
    << "cannot init mutex; error code " << to_string(status);
}

void destroy_mutex(pthread_mutex_t & m)
{
  destroy_mutex(&m);
} 
