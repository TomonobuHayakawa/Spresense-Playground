/*
 *  SubCore.ino - Demo application of keyword detector without worker on SubCore.
 *  Copyright 2022 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <MP.h>

#include "rcgproc.h"

/*-----------------------------------------------------------------*/
/* Number of channels*/
#define MAX_CHANNEL_NUM 1

/* Allocate the larger heap size than default */
USER_HEAP_SIZE(64 * 1024);

RcgProc theRcgProc;

/* MultiCore definitions */
struct Request {
  void *buffer;
  int  sample;
};

void setup()
{
  /* Initialize MP library */
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }

  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

  theRcgProc.init(&initrcgparam);
}

void loop()
{
  int      ret;
  int8_t   sndid = 10; /* user-defined msgid */
  int8_t   rcvid;
  Request *request;

  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &request);
  if (ret >= 0) {

    ExecRcgParam execrcgparam;
    execrcgparam.addr = request->buffer;
    execrcgparam.size = request->sample;

    uint32_t result = theRcgProc.exec(&execrcgparam);

    if(result != 0){
      ret = MP.Send(sndid, result, 0);
    }
  } 
}

void errorLoop(int num)
{
  int i;

  while (1) {
    for (i = 0; i < num; i++) {
      ledOn(LED0);
      delay(300);
      ledOff(LED0);
      delay(300);
    }
    delay(1000);
  }
}
