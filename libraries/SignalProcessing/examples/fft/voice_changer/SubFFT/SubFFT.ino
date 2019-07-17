/*
 *  SubFFT.ino - MP Example for Audio FFT 
 *  Copyright 2019 Sony Semiconductor Solutions Corporation
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

#include "FFT.h"

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>

arm_rfft_fast_instance_f32 iS;

/* Select FFT Offset */
const int g_channel = 1;

/* Allocate the larger heap size than default */

USER_HEAP_SIZE(64 * 1024);

/* MultiCore definitions */

struct Request {
  void *buffer;
  int  sample;
};

struct Result {
  void *buffer;
  int  sample;
};

void setup()
{
  int ret = 0;

  /* Initialize MP library */
  ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }
  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

  FFT.begin();

  arm_rfft_1024_fast_init_f32(&iS);

}

#define RESULT_SIZE 4
void loop()
{
  int      ret;
  int8_t   sndid = 10; /* user-defined msgid */
  int8_t   rcvid;
  Request *request;
  Result   result[RESULT_SIZE];
     
  static float pDst[FFTLEN+10];
  static float pTmp[FFTLEN];
  static q15_t pOut[RESULT_SIZE][FFTLEN];
  static int pos=0;

 
  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &request);
  if (ret >= 0) {
      FFT.put((q15_t*)request->buffer,request->sample);
  }

  int cnt = 0;
  while(FFT.empty(0) != 1){
//      result.chnum = g_channel;
    for (int i = 0; i < g_channel; i++) {
      int cnt = FFT.get(&pDst[5],i);

      memset(pDst,0,5);
      arm_rfft_fast_f32(&iS, pDst, pTmp, 1);
      arm_float_to_q15(pTmp,&pOut[pos][0],FFTLEN);

      result[pos].buffer = MP.Virt2Phys(&pOut[pos][0]);
      result[pos].sample = FFTLEN;

      ret = MP.Send(sndid, &result[pos],0);
      pos = (pos+1)%RESULT_SIZE;
      if (ret < 0) {
        errorLoop(1);
      }
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
