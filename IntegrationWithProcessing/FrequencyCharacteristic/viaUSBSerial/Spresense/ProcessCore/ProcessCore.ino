/*
 *  ProcessCore.ino - Demo application.
 *  Copyright 2023 T.Hayakawa
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
#include "CoreInterface.h"

#include <FFT.h>

/*-----------------------------------------------------------------*/
/*
 * FFT parameters
 */
/* Select FFT length */

/* Number of channels*/
#define MAX_CHANNEL_NUM 1
//#define MAX_CHANNEL_NUM 2
//#define MAX_CHANNEL_NUM 4

#define SAMPLING_RATE   48000 // ex.) 48000, 16000

#define FFT_LEN         2048 // ex.) 128, 256, 1024
#define OVERLAP         (FFT_LEN/2)  // ex.) 0, 128, 256

FFTClass<MAX_CHANNEL_NUM, FFT_LEN> FFT;

#define RESULT_FRAME_NUMBER 4

static float  OutBuffer[RESULT_FRAME_NUMBER][MAX_CHANNEL_NUM][FFT_LEN*2] = {0};
static Result ResultBuffer[RESULT_FRAME_NUMBER];
static int    buffer_pos = 0;

/* Allocate the larger heap size than default */
USER_HEAP_SIZE(64 * 1024);

void setup()
{
  int ret = 0;

  /* Initialize MP library */
  ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }

  /* receive with non-blocking */
  MP.RecvTimeout(100000);

  /* begin FFT */
  FFT.begin();
}

void loop()
{
  int      ret;
  int8_t   sndid = NOMAL_RESULT_ID; /* user-defined msgid */
  int8_t   rcvid;
  Request  *request;
  static Result result[RESULT_FRAME_NUMBER];
  static float pDst[FFT_LEN];

  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &request);
  if (ret >= 0) {
      FFT.put((q15_t*)request->buffer,request->sample);
  }

  while (!FFT.empty(0)) {
    for (int i = 0; i < MAX_CHANNEL_NUM; i++) {
      FFT.get(OutBuffer[buffer_pos][i],i);

      result[buffer_pos].buffer = (void*)MP.Virt2Phys(&OutBuffer[buffer_pos][i]);
      result[buffer_pos].sample = FFT_LEN/2;
      uint16_t *data = result[buffer_pos].buffer;
      ret = MP.Send(sndid, &result[buffer_pos],0);
      if (ret < 0) {
        errorLoop(11);
      }
    }
    buffer_pos = (buffer_pos + 1) % RESULT_FRAME_NUMBER;
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
