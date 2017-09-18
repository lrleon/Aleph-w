# ifndef AH_SIGNAL_H
# define AH_SIGNAL_H

# include <signal.h>
# include <iostream>
# include <aleph.H>

using namespace std;

class Signal
{
  typedef void Sigfunc(int);

  struct sigaction oldSigaction;
  int              signalNumber;
  bool             restartCalls;

public:

  Signal(const int & signo, Sigfunc *func, const bool & _restartcalls = true)
    : restartCalls(_restartcalls)
  {
    struct sigaction    act, oact;
	  
    signalNumber = signo;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (signo == SIGALRM) 
      {
#ifdef  SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;   
#endif
      } 
    else 
      {
	if (restartCalls)
	  {
#ifdef  SA_RESTART
	    act.sa_flags |= SA_RESTART;    
#endif
	  }
      }

    if (sigaction(signo, &act, &oact) < 0)
      {
	cerr << "Cannot modify the action associated with the signal " 
	     << signo << endl;
	abort();
      }

    oldSigaction = oact;
  }

  ~Signal()
  {
    if (sigaction(signalNumber, &oldSigaction, nullptr) < 0)
      {
	cerr << "Cannot modify the action associated with the %d signal " 
	     << signalNumber << endl;
	abort();
      }
  }
};

# endif // AH_SIGNAL_H
