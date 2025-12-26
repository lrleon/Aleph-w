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
/*
  Aleph Distributed System License & Copyright Note

  Copyright (c) 1999, 2000
  UNIVERSIDAD DE LOS ANDES (ULA) Merida - VENEZUELA
  All rights reserved.

  - Centro de Estudios en Microcomputacion y Sistemas Distribuidos (CEMISID)
  - Centro Nacional de Calculo Cientifico de la ULA (CECALCULA)

  PERMISSION TO USE, COPY, MODIFY AND DISTRIBUTE THIS SOFTWARE AND ITS
  DOCUMENTATION IS HEREBY GRANTED, PROVIDED THAT BOTH THE COPYRIGHT
  NOTICE AND THIS PERMISSION NOTICE APPEAR IN ALL COPIES OF THE
  SOFTWARE, DERIVATIVE WORKS OR MODIFIED VERSIONS, AND ANY PORTIONS
  THEREOF, AND THAT BOTH NOTICES APPEAR IN SUPPORTING DOCUMENTATION.

  LibAleph and Aleph are distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  UNIVERSIDAD DE LOS ANDES requests users of this software to return to

  Leandro Leon  ( lrleon@cemisid.ing.ula.ve )
  CEMISID
  Ed La Hechicera
  3er piso, ala Norte
  Facultad de Ingenieria
  Univerisdad de Los Andes
  Merida - VENEZUELA

  or to     software@cemisid.ing.ula.ve

  any improvements or extensions that they make and grant Universidad
  de Los Andes (ULA) the rights to redistribute these changes.

  Aleph is (or was) granted by:
  - Consejo de Desarrollo Cientifico, Humanistico, Tecnico de la ULA  (CDCHT)
  - Consejo Nacional de Investigaciones Cientificas y Tecnologicas (CONICIT)

  This file is part of Alephlib, the container of data structures and
  algorithms used in the Aleph distributed system
*/

#ifndef USECONDVAR_H
#define USECONDVAR_H

#include <pthread.h>

class UseCondVar 
{
private:

  pthread_cond_t *cond;
  pthread_mutex_t *mutex;

public:

  UseCondVar(pthread_cond_t *c, pthread_mutex_t *m)
  {
    mutex = m;
    cond = c;
  }

  int wait()
  {
    int retVal = pthread_cond_wait(cond, mutex);
    return retVal;
  }

  int signal()
  {
    int retVal = pthread_cond_signal(cond);
    return retVal;
  }
	
  int broadcast()
  {
    int retVal = pthread_cond_broadcast(cond);
    return retVal;
  }

  ~UseCondVar()
  {
    //nullptr;
  }
};
#endif

