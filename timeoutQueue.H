/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# ifndef TIMEOUTQUEUE_H
# define TIMEOUTQUEUE_H

# include <unistd.h>
# include <pthread.h>
# include <tpl_binHeap.H>
# include <useMutex.H>
# include <ah-time.H>


class TimeoutQueue 
{
public:

  class Event : private BinHeapVtl<Time>::Node
  {
    friend class TimeoutQueue;

  public:

    enum Execution_Status 
    {
      Out_Queue, 
      In_Queue, 
      Canceled, 
      Executing, 
      Executed,
      To_Delete,
      Deleted
    };

  private:

    Execution_Status execution_status;

    void set_trigger_time(Time _t)
    {
      assert(_t.tv_nsec >= 0 and _t.tv_nsec < NSEC);
      EVENT_TIME(this) = _t;
    }

    void set_execution_status(Execution_Status status)
    {
      execution_status = status;
    }    

  protected:

    Event(const Time& t) : 
      BinHeapVtl<Time>::Node(t), execution_status(Out_Queue)
    {
      assert(EVENT_NSEC(this) >= 0 and EVENT_NSEC(this) < NSEC);
    }

    Event(long sec, long nsec) : execution_status(Out_Queue)
    {
      assert(nsec >= 0 and nsec < NSEC);
      
      EVENT_SEC(this)  = sec;
      EVENT_NSEC(this) = nsec;
    }

  public:

    virtual ~Event() 
    { 
      if (execution_status == In_Queue)
	AH_ERROR("Attempting to delete an event in queue (0x%x)", this);
    }

    Execution_Status get_execution_status() const { return execution_status; }

    Time getAbsoluteTime() { return EVENT_TIME(this); }

    void set_for_deletion() { execution_status = To_Delete; }

    virtual void EventFct() = 0;
  }; /* end class Event */

private:

  BinHeapVtl<Time> prioQueue;

  pthread_mutex_t  mutex;
  pthread_cond_t   cond;
  pthread_t        threadId;

  bool             isShutdown;

  static int       instanceCounter;
  
  void *triggerEvent();

  static void* triggerEventThread(void*);

public:

  TimeoutQueue();

  ~TimeoutQueue() throw(domain_error);

  void schedule_event(const Time &, Event*);

  void schedule_event(Event*);

  bool cancel_event(Event*); 

  void reschedule_event(const Time &, Event*);

  void cancel_delete_event(Event *&);

  void shutdown();

  const pthread_t & getThreadId() const { return threadId; }
};


# endif /* TIMEOUTQUEUE_H */

