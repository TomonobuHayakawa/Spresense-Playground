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
 
#include "FFT.h"

RingBuff ringbuf[MAX_CHANNEL_NUM](INPUT_BUFFER);

bool FFTClass::begin(windowType_t type, int channel, int overlap)
{
  if(channel > MAX_CHANNEL_NUM) return false;
  if(overlap > (FFTLEN/2)) return false;

  m_overlap = overlap;
  m_channel = channel;
  
  clear();
  create_coef(type);
  fft_init();

  return true;
}

void FFTClass::clear()
{
  for (int i = 0; i < MAX_CHANNEL_NUM; i++) {
    memset(tmpInBuf[i], 0, FFTLEN);
  }
}

void FFTClass::fft_init()
{
#if (FFTLEN == 32)
  arm_rfft_32_fast_init_f32(&S);
#elif (FFTLEN == 64)
  arm_rfft_64_fast_init_f32(&S);
#elif (FFTLEN == 128)
  arm_rfft_128_fast_init_f32(&S);
#elif (FFTLEN == 256)
  arm_rfft_256_fast_init_f32(&S);
#elif (FFTLEN == 512)
  arm_rfft_512_fast_init_f32(&S);
#elif (FFTLEN == 1024)
  arm_rfft_1024_fast_init_f32(&S);
#elif (FFTLEN == 2048)
  arm_rfft_2048_fast_init_f32(&S);
#elif (FFTLEN == 4096)
  arm_rfft_4096_fast_init_f32(&S);
#endif
}

void FFTClass::create_coef(windowType_t type)
{

  for (int i = 0; i < FFTLEN / 2; i++)
  {
    if(type == WindoHamming){
      coef[i] = 0.54f - (0.46f * arm_cos_f32(2 * PI * (float)i / (FFTLEN - 1)));
    }else if(type == WindoHanning){
      coef[i] = 0.54f - (1.0f * arm_cos_f32(2 * PI * (float)i / (FFTLEN - 1)));
    }else{
      coef[i] = 1;
    }
    coef[FFTLEN-1-i] = coef[i];
  }
}

bool FFTClass::put(q15_t* pSrc, int sample)
{
  /* Ringbuf size check */
  if(m_channel > MAX_CHANNEL_NUM) return false;
  if(sample > ringbuf[0].remain()) return false;

  if (m_channel == 1) {
  	/* the faster optimization */
  	ringbuf[0].put((q15_t*)pSrc, sample);
  } else {
  	for (int i = 0; i < m_channel; i++) {
  	  ringbuf[i].put(pSrc, sample, m_channel, i);
    }
  }
  return  true;
}

bool FFTClass::empty(int channel)
{
   return (ringbuf[channel].stored() < FFTLEN);
}

int FFTClass::get(float* out, int channel, bool raw)
{
  static float tmpFft[FFTLEN];  

  if(channel >= m_channel) return false;
  if (ringbuf[channel].stored() < FFTLEN) return 0;

  for(int i=0;i<m_overlap;i++){
    tmpInBuf[channel][i]=tmpInBuf[channel][FFTLEN-m_overlap+i];
  }

  /* Read from the ring buffer */
  ringbuf[channel].get(&tmpInBuf[channel][m_overlap], FFTLEN-m_overlap);

  for(int i=0;i<FFTLEN;i++){
    tmpFft[i] = tmpInBuf[channel][i]*coef[i];
  }

  if(raw){
    /* Calculate only FFT */
    fft(tmpFft, out, FFTLEN);
  }else{
    /* Calculate FFT for convert to amplitude */
    fft_amp(tmpFft, out, FFTLEN);
  }
  return (FFTLEN-m_overlap);
}

void FFTClass::fft(float *tmpInBuf, float *pDst, int fftLen)
{
  /* calculation */
  arm_rfft_fast_f32(&S, tmpInBuf, pDst, 0);
}

void FFTClass::fft_amp(float *tmpInBuf, float *pDst, int fftLen)
{
  /* calculation */
  arm_rfft_fast_f32(&S, tmpInBuf, tmpOutBuf, 0);

  arm_cmplx_mag_f32(&tmpOutBuf[2], &pDst[1], fftLen / 2 - 1);
  pDst[0] = tmpOutBuf[0];
  pDst[fftLen / 2] = tmpOutBuf[1];
}

/* Pre-instantiated Object for this class */
FFTClass FFT;
