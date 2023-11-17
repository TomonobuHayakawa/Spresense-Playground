/*
 *  FilterCore.ino - Mixing example of 4ch capture data
 *  Copyright 2023 Sony Semiconductor Solutions Corporation
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

#define CHANNEL_NUM 4
#define MAX_FRAME_SIZE  1024

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
  MP.RecvTimeout(100000);
}

/* ------------------------------------------------------------------------ */
/*
 * 4ch Mixer
 */
static void filter_sample(int16_t *in, int16_t *out, uint32_t sample)
{
  puts("in");
  /* Example : A, B, C, D => A+B, C+D */

  int16_t *in0 = in;
  int16_t *in1 = in0 + 1;
  int16_t *in2 = in1 + 1;
  int16_t *in3 = in2 + 1;

  int16_t *outl = out;
  int16_t *outr = out + 1;


  for (uint32_t cnt = 0; cnt < sample; cnt++){
    *outl = *in0 + *in1;
    *outr = *in2 + *in3;

    in0+=CHANNEL_NUM;
    in1+=CHANNEL_NUM;
    in2+=CHANNEL_NUM;
    in3+=CHANNEL_NUM;
    outl+=2;
    outr+=2;
  }
}

#define RESULT_NUM  4

void loop()
{
  int      ret;
  int8_t   sndid = 10; /* user-defined msgid */
  int8_t   rcvid;
  Request  *request;
  static   Result result[RESULT_NUM];

  static uint16_t pOut[RESULT_NUM][MAX_FRAME_SIZE];
  static int pos = 0;

  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &request);
  if (ret >= 0) {

    filter_sample((uint16_t*)request->buffer,pOut[pos],request->sample);
    
    result[pos].buffer = (void*)MP.Virt2Phys(&pOut[pos][0]);
    result[pos].sample = request->sample;
    ret = MP.Send(sndid, &result[pos],0);
    pos = (pos + 1) % RESULT_NUM;
 
    if (ret < 0) {
      errorLoop(1);
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
