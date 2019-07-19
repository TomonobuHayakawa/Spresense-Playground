/*
 *  FFT.cpp - FFT Library
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

#ifndef _FFT_H_
#define _FFT_H_

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>

#include "RingBuff.h"

/*------------------------------------------------------------------*/
/* Configurations                                                   */
/*------------------------------------------------------------------*/
/* Select FFT length */

//#define FFTLEN 32
//#define FFTLEN 64
//#define FFTLEN 128
//#define FFTLEN 256
//#define FFTLEN 512
#define FFTLEN 1024
//#define FFTLEN 2048
//#define FFTLEN 4096

/* Number of channels*/
//#define MAX_CHANNEL_NUM 1
//#define MAX_CHANNEL_NUM 2
#define MAX_CHANNEL_NUM 4

#define INPUT_BUFFER (FFTLEN*sizeof(q15_t)*2)

/*------------------------------------------------------------------*/
/* Type Definition                                                  */
/*------------------------------------------------------------------*/
/* WINDOW TYPE */
typedef enum e_windowType {
	WindoHamming,
	WindoHanning,
	WindowRectangle
} windowType_t;

/*------------------------------------------------------------------*/
/* Input buffer                                                      */
/*------------------------------------------------------------------*/
class FFTClass
{
public:
  void begin(){
      begin(WindoHamming, MAX_CHANNEL_NUM, (FFTLEN/2));
  }

  bool begin(windowType_t type, int channel, int overlap);
  bool put(q15_t* pSrc, int size);

  int  get_raw(float* out, int channel, int raw);
  int  get(float* out, int channel){
    return get_raw(out, channel, false);
  }

  void clear();
  void end(){}
  bool empty(int channel);
	
private:

  int m_channel;
  int m_overlap;
  arm_rfft_fast_instance_f32 S;

  /* Temporary buffer */
  float tmpInBuf[MAX_CHANNEL_NUM][FFTLEN];
  float coef[FFTLEN];
  float tmpOutBuf[FFTLEN];

  void create_coef(windowType_t);
  void fft_init();
  void fft(float *pSrc, float *pDst, int fftLen);
  void fft_amp(float *pSrc, float *pDst, int fftLen);

};

extern FFTClass FFT;

#endif /*_FFT_H_*/
