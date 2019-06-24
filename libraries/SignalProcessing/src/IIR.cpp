/*
 *  IIR.cpp - IIR(biquad cascade) Library
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
 
#include "IIR.h"

RingBuff ringbuf[MAX_CHANNEL_NUM](INPUT_BUFFER);

bool IIRClass::begin(filterType_t type, int channel, int cutoff, float q)
{
  if(channel > MAX_CHANNEL_NUM) return false;

  m_channel = channel;

  create_coef(type, cutoff, q);

  arm_biquad_cascade_df1_init_f32(&S,1,coef,buffer);

  return true;
}

void IIRClass::create_coef(filterType_t type, int cutoff, float q)
{
  float w = 2.0f * PI * cutoff/48000;

  float a0 =  1.0f + sin(w) / (2.0f * sqrt(2));
  float a1 = -2.0f * cos(w);
  float a2 =  1.0f - sin(w) / (2.0f * sqrt(2));
  float b0 = (1.0f - cos(w)) / 2.0f;
  float b1;
  float b2 = (1.0f - cos(w)) / 2.0f;

  switch(type){
  case LPF:
    b1 =  1.0f - cos(w);
    break;
  case HPF:
  	b1 =  -(1.0f - cos(w));
    break;
  default:
    exit(1);
  }    

  coef[0] = b0/a0;
  coef[1] = b1/a0;
  coef[2] = b2/a0;
  coef[3] = -(a1/a0);
  coef[4] = -(a2/a0);

}


bool IIRClass::put(q15_t* pSrc, int sample)
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

bool IIRClass::empty(int channel)
{
   return (ringbuf[channel].stored() < FRAMSIZE);
}

int IIRClass::get(q15_t* pDst, int channel)
{

  if(channel >= m_channel) return false;
  if (ringbuf[channel].stored() < 1024) return 0;

  /* Read from the ring buffer */
  ringbuf[channel].get(tmpInBuf, FRAMSIZE);

  arm_biquad_cascade_df1_f32(&S, tmpInBuf, tmpOutBuf, FRAMSIZE);
  arm_float_to_q15(tmpOutBuf, pDst, FRAMSIZE);

  return FRAMSIZE;
}
