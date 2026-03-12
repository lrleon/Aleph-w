
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
# include <cerrno>
# include <q-consumer-threads.H>

using namespace std;

struct Event1 : QueueTheadsPool<int>::Event
{
  void run() override
  {
    // cout << "Event 1 " << item << endl;
    for (volatile size_t i = 0; i < 10000000; ++i);
    increment_count(); 
  }
};

struct Event2 : QueueTheadsPool<int>::Event
{
  void run() override
  {
    // cout << "Event 2 " << item << endl;
    for (volatile size_t i = 0; i < 10000000; ++i);
    increment_count(); 
  }
};

int main(int argc, char *argv[])
{
  if (argc < 4)
    {
      cerr << "Usage: " << argv[0] << " <num_threads> <num_items> <secs>" << endl;
      return 1;
    }

  char * endptr = nullptr;
  errno = 0;
  const size_t num_threads = strtoul(argv[1], &endptr, 10);
  if (errno != 0 or endptr == argv[1] or *endptr != '\0' or num_threads == 0)
    {
      cerr << "Invalid num_threads: " << argv[1] << endl;
      return 1;
    }

  const size_t num_items = strtoul(argv[2], &endptr, 10);
  if (errno != 0 or endptr == argv[2] or *endptr != '\0')
    {
      cerr << "Invalid num_items: " << argv[2] << endl;
      return 1;
    }

  const size_t secs = strtoul(argv[3], &endptr, 10);
  if (errno != 0 or endptr == argv[3] or *endptr != '\0')
    {
      cerr << "Invalid secs: " << argv[3] << endl;
      return 1;
    }

  QueueTheadsPool<int> qpool;
  DynList<QueueTheadsPool<int>::Event*> event_list;
  for (size_t i = 0; i < num_threads; ++i)
    {
      QueueTheadsPool<int>::Event * ptr = nullptr;
      if (i % 2 == 0)
	qpool.create_thread(ptr = new Event1);
      else
	qpool.create_thread(ptr = new Event2);
      event_list.append(ptr);
    }

  cout << "Inserting " << num_items << " to the queue" << endl;
  for (size_t i = 0; i < num_items; ++i)
    {
      cout << "Inserting " << i << endl;
      qpool.put(i);
    }
  cout << "Done" << endl
       << endl
       << "Resuming ... " << endl;
  qpool.resume();
  
  cout << endl
       << "Sleeping for " << secs << " seconds" << endl;
  this_thread::sleep_for(chrono::seconds(secs));
  cout << endl 
       << "Done" << endl
       << endl
       << "List of event counters" << endl;
  event_list.for_each([] (auto e) 
    {
      cout << "    Event count = " << e->get_count() << endl; 
    });

  cout << "done" << endl
       << endl
       << "Suspending" << endl;
  qpool.suspend();
  cout << "Done" << endl
       << endl
       << "Sleeping for 5 secs" << endl;
  this_thread::sleep_for(chrono::seconds(5));
  cout << "done" << endl
       << endl
       << "Resuming while other " << secs << " seconds" << endl
       << endl;
  qpool.resume();
    this_thread::sleep_for(chrono::seconds(secs));
  cout << endl 
       << "Done" << endl
       << endl
       << "List of event counters" << endl;
  event_list.for_each([] (auto e) 
    {
      cout << "    Event count = " << e->get_count() << endl; 
    });

  cout << "done" << endl
       << endl
       << "Shutdown" << endl;
  qpool.shutdown();
  cout << "Done" << endl;

  return 0;
}
