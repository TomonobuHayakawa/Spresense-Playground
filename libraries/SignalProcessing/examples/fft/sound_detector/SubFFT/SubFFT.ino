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
#define SAMPLING_RATE   48000 // ex.) 48000, 16000

#define FFTLEN          1024 // ex.) 128, 256, 1024
#define OVERLAP         (FFTLEN/2)  // ex.) 0, 128, 256

/*-----------------------------------------------------------------*/
/*
 * Detector parameters
 */
#define POWER_THRESHOLD       30  // Power
#define LENGTH_THRESHOLD      30  // 20ms
#define INTERVAL_THRESHOLD    100 // 100ms

#define BOTTOM_SAMPLING_RATE  1000 // 1kHz
#define TOP_SAMPLING_RATE     1500 // 1.5kHz

#define FS2BAND(x)            (x*FFTLEN/SAMPLING_RATE)
#define BOTTOM_BAND           (FS2BAND(BOTTOM_SAMPLING_RATE))
#define TOP_BAND              (FS2BAND(TOP_SAMPLING_RATE))

#define MS2FRAME(x)           ((x*SAMPLING_RATE/1000/(FFTLEN-OVERLAP))+1)
#define LENGTH_FRAME          MS2FRAME(LENGTH_THRESHOLD)
#define INTERVAL_FRAME        MS2FRAME(INTERVAL_THRESHOLD)

/*-----------------------------------------------------------------*/
/* Select FFT Offset */
const int g_channel = 4;

/* Allocate the larger heap size than default */

USER_HEAP_SIZE(64 * 1024);

/* MultiCore definitions */

struct Capture {
  void *buff;
  int  sample;
  int  channel;
};

struct Result {
  bool found[g_channel] = {false,false,false,false};
  int  channel;
  void clear(){ found[0]=false;found[1]=false;found[2]=false;found[3]=false;}
};

void setup()
{
  /* Initialize MP library */
  int ret = MP.begin();  if (ret < 0) {
    errorLoop(2);
  }
  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

  FFT.begin();
}

#define RESULT_SIZE 4
void loop()
{
  int      ret;
  int8_t   sndid = 10; /* user-defined msgid */
  int8_t   rcvid;
  Capture *capture;
  static Result result[RESULT_SIZE];
  static int pos=0;

  result[pos].clear();
  
  static float pDst[FFTLEN/2];
 
  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &capture);
  if (ret >= 0) {
      FFT.put((q15_t*)capture->buff,capture->sample);
  }

  while(FFT.empty(0) != 1){
      result[pos].channel = g_channel;
    for (int i = 0; i < g_channel; i++) {
      int cnt = FFT.get(pDst,i);
      result[pos].found[i] = detect_sound(BOTTOM_BAND,TOP_BAND,pDst,i);
//      if(result[pos].found[i]){ printf("Sub channel %d\n",i); }
    }
    ret = MP.Send(sndid, &result[pos],0);
    pos = (pos+1)%RESULT_SIZE;
    if (ret < 0) {
      errorLoop(1);
    }
  }

}

/*-----------------------------------------------------------------*/
/*
 * Detector functions
 */
bool detect_sound(int bottom, int top, float* pdata, int channel )
{
  static int continuity[g_channel] = {0,0,0,0};
  static int interval[g_channel] = {0,0,0,0};

  if(bottom > top) return false;

  if(interval[channel]> 0){ // Do not detect in interval time. 
    interval[channel]--;
    continuity[channel]=0;
    return false;
  }
  
  for(int i=bottom;i<=top;i++){
//     printf("!!%2.8f\n",*(pdata+i));
    if(*(pdata+i) > POWER_THRESHOLD){ // find sound.
//      printf("!!%2.8f\n",*(pdata+i));
      continuity[channel]++;
//      printf("con=%d\n",continuity);
      if(continuity[channel] > LENGTH_FRAME){ // length is enough.
        interval[channel] = INTERVAL_FRAME;
        return true;
      }else{
//      puts("continue sound");
        return false;  
      }
    }
  }
  continuity[channel]=0;
  return false;  
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
