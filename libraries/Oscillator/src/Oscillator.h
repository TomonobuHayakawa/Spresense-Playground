/*
 *  Oscillator.h - Oscillator Library Header for Synthesizer
 *  Copyright 2020 Sony Semiconductor Solutions Corporation
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

#ifndef __OSCILLATOR_H__
#define __OSCILLATOR_H__

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>

#define MAX_CHANNEL_NUMBER 8


typedef enum
{
  SinWave = 0,
  RectWave, /* 1 */
  SawWave,  /* 2 */
  WaveModeNum

} WaveMode;

/*--------------------------------------------------------------------*/
/*  Wave Generator                                                */
/*--------------------------------------------------------------------*/
class GeneratorBase
{
public:
  void init(uint8_t, uint32_t, uint8_t);
  virtual void exec(q15_t*, uint16_t) = 0;
  void set(uint32_t);

protected:
  q15_t* update_sample(q15_t* ptr, q15_t val);

  uint32_t  m_sampling_rate;  /**< Sampling rate of data */
  q15_t     m_theta;
  q15_t     m_omega;
  uint8_t   m_channels;
};

class SinGenerator : public GeneratorBase
{
public:
  void exec(q15_t*, uint16_t);
};

class RectGenerator : public GeneratorBase
{
public:
  void exec(q15_t*, uint16_t);

};

class SawGenerator : public GeneratorBase
{
public:
  void exec(q15_t*, uint16_t);
};

/*--------------------------------------------------------------------*/
/*  Envelope Generator                                                */
/*--------------------------------------------------------------------*/
class EnvelopeGenerator
{
public:
  void init(uint8_t, /* bit_length */
            uint8_t  /* channel_num */);

  void set(uint16_t, /* Attack :ms */
           uint16_t, /* Decay  :ms */
           uint16_t, /* Sustain:%  */
           uint16_t  /* Release:ms */
           );

  void exec(q15_t*, uint16_t);

  void start(void);

  void stop(void);

private:

  enum EgState
  {
    Attack_state = 0,
    Decay_state,
    Sustain_state,
    Release_state,
    Ready_state,
    EgStateNum
  };

  float   m_cur_coef;
  EgState m_cur_state;

  float m_a_delta;
  float m_d_delta;
  float m_r_delta;
  q15_t m_s_level;

  uint8_t m_channels;
  uint8_t m_bits;

  uint16_t attack(q15_t**, uint16_t);
  uint16_t decay(q15_t**, uint16_t);
  uint16_t sustain(q15_t**, uint16_t);
  uint16_t release(q15_t**, uint16_t);
  uint16_t ready(q15_t**, uint16_t);
};

/*--------------------------------------------------------------------*/
/*  Oscillator                                                        */
/*--------------------------------------------------------------------*/
class OscillatorClass
{
public:

  /* For Arduino */
  void begin(WaveMode type, uint8_t channel_num){
    init(type, channel_num);
  }

  void end(){
    flush();
  }

  void init(WaveMode type, uint8_t channel_num);
  void exec(q15_t* ptr, uint16_t sample);
  void flush();
  void set(uint8_t ch, uint16_t frequency);
  void set(uint8_t ch, float attack, float decay, q15_t sustain, float release);

private:
  WaveMode m_type;
  uint8_t m_channel_num;
  uint8_t m_bit_length;
  const uint32_t m_sampling_rate = 48000;

  GeneratorBase* m_wave[MAX_CHANNEL_NUMBER];
  SinGenerator   m_sin[MAX_CHANNEL_NUMBER];
  RectGenerator  m_rect[MAX_CHANNEL_NUMBER];
  SawGenerator   m_saw[MAX_CHANNEL_NUMBER];
  EnvelopeGenerator m_envlop[MAX_CHANNEL_NUMBER];

};

extern OscillatorClass Oscillator;

#endif /* __OSCILLATOR_H__ */
