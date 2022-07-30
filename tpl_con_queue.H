
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
# ifndef TPL_CON_QUEUE_H
# define TPL_CON_QUEUE_H

# include <mutex>
# include <condition_variable>
# include <tpl_dynListQueue.H>

using namespace std;

template <typename T>
class Concurrent_Queue
{
  mutex lck;
  condition_variable cond;
  DynListQueue<T> q;

public:

  T & put(const T & item)
  {
    lock_guard<mutex> critical_section(lck);
    auto & ref = q.put(item);
    cond.notify_one();
    return ref;
  }

  T & put(T && item)
  {
    lock_guard<mutex> critical_section(lck);
    auto & ref = q.put(forward<T>(item));
    cond.notify_one();
    return ref;
  }

  T & push(const T & item) { return put(item); }

  T & push(T && item) { return put(forward<T>(item)); }

  T get()
  {
    lock_guard<mutex> critical_section(lck);
    while (q.is_empty())
      cond.wait(lck);
    return q.get();
  }

  size_t size()
  {
    lock_guard<mutex> critical_section(lck);
    return q.size();
  }

  size_t is_empty()
  {
    lock_guard<mutex> critical_section(lck);
    return q.is_empty();
  }
};



# endif // TPL_CON_QUEUE_H
