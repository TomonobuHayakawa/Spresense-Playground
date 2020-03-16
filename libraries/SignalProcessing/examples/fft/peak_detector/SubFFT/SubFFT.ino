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

/*-----------------------------------------------------------------*/
/*
 * FFT parameters
 */
/* Select FFT length */

//#define FFT_LEN 32
//#define FFT_LEN 64
//#define FFT_LEN 128
//#define FFT_LEN 256
//#define FFT_LEN 512
#define FFT_LEN 1024
//#define FFT_LEN 2048
//#define FFT_LEN 4096

/* Number of channels*/
//#define MAX_CHANNEL_NUM 1
//#define MAX_CHANNEL_NUM 2
#define MAX_CHANNEL_NUM 4

FFTClass<MAX_CHANNEL_NUM, FFT_LEN> FFT;

/* Allocate the larger heap size than default */

USER_HEAP_SIZE(64 * 1024);

/* MultiCore definitions */

struct Capture {
  void *buff;
  int  sample;
  int  chnum;
};

struct Result {
  float peak[MAX_CHANNEL_NUM];
  int  chnum;
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
}

void loop()
{
  int      ret;
  int8_t   sndid = 10; /* user-defined msgid */
  int8_t   rcvid;
  Capture *capture;
  static Result   result;

  static float pDst[FFT_LEN/2];

  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &capture);
  if (ret >= 0) {
      FFT.put((q15_t*)capture->buff,capture->sample);
  }

  float peak[4];
  while(FFT.empty(0) != 1){
//    printf("peak ");
      result.chnum = MAX_CHANNEL_NUM;
    for (int i = 0; i < MAX_CHANNEL_NUM; i++) {
      int cnt = FFT.get(pDst,i);
      peak[i] = get_peak_frequency(pDst, FFT_LEN);
      result.peak[i] = peak[i];
//      printf("%8.3f, ", peak[i]);
    }
//    printf("\n");

    ret = MP.Send(sndid, &result,0);
    if (ret < 0) {
      errorLoop(1);
    }
  }

}

float get_peak_frequency(float *pData, int fftLen)
{
  float g_fs = 48000.0f;
  uint32_t index;
  float maxValue;
  float delta;
  float peakFs;

  arm_max_f32(pData, fftLen / 2, &maxValue, &index);

  delta = 0.5 * (pData[index - 1] - pData[index + 1])
    / (pData[index - 1] + pData[index + 1] - (2.0f * pData[index]));
  peakFs = (index + delta) * g_fs / (fftLen - 1);

  return peakFs;
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
