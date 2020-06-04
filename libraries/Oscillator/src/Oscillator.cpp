/*
 *  Oscillator.h - Oscillator Library for Synthesizer
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

#include <stdio.h>
#include "Oscillator.h"

/*--------------------------------------------------------------------*/
/*   Wave Generator Base                                              */
/*--------------------------------------------------------------------*/
void GeneratorBase::init(uint8_t bits/* only 16bits*/ , uint32_t rate, uint8_t channels)
{
  (void) bits;
  m_theta = 0;
  m_omega = 0;
  m_sampling_rate = rate;
  m_channels = channels;

}
/*--------------------------------------------------------------------*/
void GeneratorBase::set(uint32_t frequency)
{
  if (frequency != 0)
    {
      m_omega = frequency * 0x7fff / m_sampling_rate;
    }
}

/*--------------------------------------------------------------------*/
q15_t* GeneratorBase::update_sample(q15_t* ptr, q15_t val)
{
  *ptr = val;
  if (m_channels < 2)
    {
      *(ptr+1) = val;
    }
  m_theta = (m_theta + m_omega) & 0x7fff;
  ptr += (m_channels < 2) ? 2 : m_channels;
  return ptr;
}

/*--------------------------------------------------------------------*/
/*  Sin Wave Generator                                                */
/*--------------------------------------------------------------------*/
void SinGenerator::exec(q15_t* ptr, uint16_t samples)
{
  for (int i = 0; i < samples ; i++)
    {
      q15_t val = arm_sin_q15(m_theta);
      ptr = update_sample(ptr,val);
    }
}

/*--------------------------------------------------------------------*/
/*   Rectangle Wave Generator                                         */
/*--------------------------------------------------------------------*/
void RectGenerator::exec(q15_t* ptr, uint16_t samples)
{
  for (int i = 0; i < samples ; i++)
    {
     q15_t val = (m_theta > 0x3fff) ?  0x7fff: 0x8000;
     ptr = update_sample(ptr,val);
    }
}

/*--------------------------------------------------------------------*/
/*  Saw Wave Generator                                                */
/*--------------------------------------------------------------------*/

void SawGenerator::exec(q15_t* ptr, uint16_t samples)
{
  for (int i = 0; i < samples ; i++)
    {
     q15_t val = m_theta;
     ptr = update_sample(ptr,val);
    }
}

/*--------------------------------------------------------------------*/
/*    Envelope Generator                                              */
/*--------------------------------------------------------------------*/
void EnvelopeGenerator::init(uint8_t bits, uint8_t channels)
{
  m_channels = channels;
  m_bits = bits;
  m_cur_coef = 0;
  set(1,1,100,1);

  m_cur_state = Ready_state;
}

/*--------------------------------------------------------------------*/
void EnvelopeGenerator::set(uint16_t attack, uint16_t decay, uint16_t sustain, uint16_t release)
{
  m_a_delta = (float) 0x7fff/(48*attack);  // only 48kHz sampling
  m_s_level = 0x7fff * (uint32_t) sustain / 100;
  m_d_delta = (float) (0x7fff- m_s_level)/(48*decay);  // only 48kHz sampling
  m_r_delta = (float) (m_s_level)/(48*release);  // only 48kHz sampling

  m_cur_state = Attack_state;
}

/*--------------------------------------------------------------------*/
void EnvelopeGenerator::start(void)
{
    m_cur_state = Attack_state;
}

/*--------------------------------------------------------------------*/
void EnvelopeGenerator::stop(void)
{
    m_cur_state = Release_state;
}

/*--------------------------------------------------------------------*/
q15_t* EnvelopeGenerator::update_sample(q15_t* ptr, q15_t val)
{
  if (m_channels < 2)
    {
      *(ptr+1) = *ptr;
    }
  ptr += (m_channels < 2) ? 2 : m_channels;
  return ptr;
}

/*--------------------------------------------------------------------*/
void EnvelopeGenerator::exec(q15_t* ptr, uint16_t samples)
{
  while (samples > 0)
    {
      switch (m_cur_state)
        {
          case Attack_state:
            samples = attack(&ptr, samples);
            break;
          case Decay_state:
            samples = decay(&ptr, samples);
            break;
          case Sustain_state:
            samples = sustain(&ptr, samples);
            break;
          case Release_state:
            samples = release(&ptr, samples);
            break;
          case Ready_state:
            samples = ready(&ptr, samples);
            break;
          default:
            break;
        }
    }
}

/*--------------------------------------------------------------------*/
uint16_t EnvelopeGenerator::attack(q15_t** top, uint16_t samples)
{
  q15_t *ptr = *top;

  while (samples>0)
    {
      float data = *ptr;
      *ptr = (q15_t)((data * m_cur_coef) / 0x7fff);
      m_cur_coef += m_a_delta;
      ptr = update_sample(ptr,*ptr);
      samples--;

      if (m_cur_coef > (float)0x7fff)
        {
          m_cur_coef = 0x7fff;
          m_cur_state = Decay_state;
          break;
        }
    }

  *top = ptr;

  return samples;
}
/*--------------------------------------------------------------------*/
uint16_t EnvelopeGenerator::decay(q15_t** top, uint16_t samples)
{
  q15_t *ptr = *top;

  while (samples>0)
    {
      float data = *ptr;
      *ptr = (q15_t)((data * m_cur_coef) / 0x7fff);
      m_cur_coef -= m_d_delta;
      ptr = update_sample(ptr,*ptr);
      samples--;

      if (m_cur_coef < m_s_level)
        {
          m_cur_state = Sustain_state;
          break;
        }
    }

  *top = ptr;

  return samples;
}
/*--------------------------------------------------------------------*/
uint16_t EnvelopeGenerator::sustain(q15_t** top, uint16_t samples)
{
  q15_t *ptr = *top;

  while (samples>0)
    {
      float data = *ptr;
      *ptr = (q15_t)((data * m_cur_coef) / 0x7fff);
      ptr = update_sample(ptr,*ptr);
      samples--;
    }

  *top = ptr;

  return samples;

}
/*--------------------------------------------------------------------*/
uint16_t EnvelopeGenerator::release(q15_t** top, uint16_t samples)
{
  q15_t *ptr = *top;

  while (samples>0)
    {
      float data = *ptr;
      *ptr = (q15_t)((data * m_cur_coef) / 0x7fff);
      m_cur_coef -= m_r_delta;
      ptr = update_sample(ptr,*ptr);
      samples--;

      if (m_cur_coef < 0)
        {
          m_cur_coef = 0;
          m_cur_state = Ready_state;
        }
    }

  *top = ptr;

  return samples;
}
/*--------------------------------------------------------------------*/
uint16_t EnvelopeGenerator::ready(q15_t** top, uint16_t samples)
{
  q15_t *ptr = *top;

  while (samples > 0)
    {
      *ptr = 0;
      ptr = update_sample(ptr,*ptr);
      samples--;
    }

  *top = ptr;

  return 0;
}

/*--------------------------------------------------------------------*/
/*  Oscillator                                                        */
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
bool Oscillator::init(WaveMode type, uint8_t channel_num)
{
  /* Init signal process. */

  /* Data check */
  m_type        = type;
  m_bit_length  = 16;
  m_channel_num = channel_num;

  switch (m_type)
    {
      case SinWave:
        for (int i = 0; i < m_channel_num; i++)
          {
            m_wave[i] = &m_sin[i];
          }
        break;
      case RectWave:
        for (int i = 0; i < m_channel_num; i++)
          {
            m_wave[i] = &m_rect[i];
          }
        break;
      case SawWave:
        for (int i = 0; i < m_channel_num; i++)
          {
            m_wave[i] = &m_saw[i];
          }
        break;
      default:
        return false;
    }

  for (int i = 0; i < m_channel_num; i++)
    {
      m_wave[i]->init(m_bit_length,
                      m_sampling_rate,
                      m_channel_num);
      m_envlop[i].init(m_bit_length, m_channel_num);

      /* Initial value */
      m_envlop[i].set(1,1,100,1);
    }

  return true;

}

/*--------------------------------------------------------------------*/
void Oscillator::exec(q15_t* ptr, uint16_t sample)
{

  /* Byte size per sample.
   * If ch num is 1, but need to extend mono data to L and R ch.
   */
  uint16_t samples = sample;

  for (int i = 0; i < m_channel_num; i++)
    {
      m_wave[i]->exec((ptr + i), samples);
      m_envlop[i].exec((ptr + i), samples);
    }
}

/*--------------------------------------------------------------------*/
void Oscillator::flush()
{

}
/*--------------------------------------------------------------------*/
bool Oscillator::set(uint8_t ch, WaveMode type)
{
  switch (type)
    {
      case SinWave:
        m_sin[ch].set(*m_wave[ch]);
        m_wave[ch] = &m_sin[ch];
        break;
      case RectWave:
        m_rect[ch].set(*m_wave[ch]);
        m_wave[ch] = &m_rect[ch];
        break;
      case SawWave:
        m_saw[ch].set(*m_wave[ch]);
        m_wave[ch] = &m_saw[ch];
        break;
      default:
        return false;
    }
  return true;
}

/*--------------------------------------------------------------------*/
bool Oscillator::set(uint8_t ch, uint16_t frequency)
{
  if(ch>=m_channel_num) return false;

  m_wave[ch]->set(frequency);

  if (frequency == 0)
    {
      m_envlop[ch].stop();
    }
  else
    {
      m_envlop[ch].start();
    }

  return true;
}

/*--------------------------------------------------------------------*/
bool Oscillator::set(uint8_t ch, float a, float d, q15_t s, float r)
{
  if(ch>=m_channel_num) return false;

  m_envlop[ch].set(a,d,s,r);

  return true;
}
