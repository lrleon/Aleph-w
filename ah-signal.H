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
# ifndef AH_SIGNAL_H
# define AH_SIGNAL_H

# include <csignal>
# include <iostream>
# include <aleph.H>

class Signal
{
  using Sigfunc = void (int);

  struct sigaction oldSigaction;
  int              signalNumber;
  bool             restartCalls;

public:

  Signal(const int & signo, Sigfunc *func, const bool & _restartcalls = true)
    : restartCalls(_restartcalls)
  {
    struct sigaction    act;
    struct sigaction oact;

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
        std::cerr << "Cannot modify the action associated with the signal "
                  << signo << std::endl;
        abort();
      }

    oldSigaction = oact;
  }

  ~Signal()
  {
    if (sigaction(signalNumber, &oldSigaction, nullptr) < 0)
      {
        std::cerr << "Cannot modify the action associated with the signal "
                  << signalNumber << std::endl;
        abort();
      }
  }
};

# endif // AH_SIGNAL_H
