
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file timeoutQueue.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TIMEOUTQUEUE_H
# define TIMEOUTQUEUE_H

# include <unistd.h>
# include <pthread.h>
# include <tpl_binHeap.H>
# include <useMutex.H>
# include <time.H>

     
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
	ERROR("Attempting to delete an event in queue (0x%x)", this);
    }

    Execution_Status get_execution_status() const { return execution_status; }

    Time getAbsoluteTime() { return EVENT_TIME(this); }

    void set_for_deletion() { execution_status = To_Delete; }

    virtual void EventFct() = 0;
  }; /* End class Event */

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

  ~TimeoutQueue();

  void schedule_event(const Time &, Event*);

  void schedule_event(Event*);

  bool cancel_event(Event*); 

  void reschedule_event(const Time &, Event*);

  void cancel_delete_event(Event *&);

  void shutdown();

  const pthread_t & getThreadId() const { return threadId; }
};


# endif /* TIMEOUTQUEUE_H */
