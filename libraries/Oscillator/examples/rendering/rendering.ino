/*
    rendering.ino - Rendering example for Oscillator
    Copyright 2020 Sony Semiconductor Solutions Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <stdio.h>
#include <stdlib.h>
#include <arch/board/board.h>

#include <Oscillator.h>

#include <stdio.h>

OutputMixer *theMixer;
bool ErrEnd = false;

static bool getFrame(AsPcmDataParam *pcm)
{
  const uint32_t sample = 480;

  /* Alloc MemHandle */
  if (pcm->mh.allocSeg(S0_REND_PCM_BUF_POOL, (sample * 2 * 2)) != ERR_OK) {
    return false;
  }

  Oscillator.exec((q15_t*)pcm->mh.getPa(), sample);

  q15_t* ad = pcm->mh.getPa();

//  printf("p0=%x.p1=%x.p2=%x.p3=%x\n",*ad,*(ad+1),*(ad+2),*(ad+3));

  /* Set PCM parameters */
  pcm->identifier = 0;
  pcm->callback = 0;
  pcm->bit_length = 16;
  pcm->size = sample * 2 * 2;
  pcm->sample = sample;
  pcm->is_end = false;
  pcm->is_valid = true;

  return true;
}


static bool active()
{
  for (int i = 0; i < 5; i++) {
    /* get PCM */
    AsPcmDataParam pcm_param;
    if (!getFrame(&pcm_param)) {
      break;
    }

    /* Send PCM */
    int err = theMixer->sendData(OutputMixer0,
                                 outmixer_send_callback,
                                 pcm_param);

    if (err != OUTPUTMIXER_ECODE_OK) {
      printf("OutputMixer send error: %d\n", err);
      return false;
    }
  }
  return true;
}

/**
   @brief Mixer done callback procedure

   @param [in] requester_dtq    MsgQueId type
   @param [in] reply_of         MsgType type
   @param [in,out] done_param   AsOutputMixDoneParam type pointer
*/
static void outputmixer_done_callback(MsgQueId requester_dtq,
                                      MsgType reply_of,
                                      AsOutputMixDoneParam *done_param)
{
  printf(">> %x done\n", reply_of);
  return;
}

/**
   @brief Mixer data send callback procedure

   @param [in] identifier   Device identifier
   @param [in] is_end       For normal request give false, for stop request give true
*/
static void outmixer_send_callback(int32_t identifier, bool is_end)
{
  return;
}


static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING) {
    ErrEnd = true;
  }
}

void setup()
{
  printf("setup() start\n");

  /* Display menu */

  Serial.begin(115200);

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYER);

  /* Use SD card */
  Oscillator.begin(SinWave, 2);
//  Oscillator.begin(SawWave, 2);

  /* Start audio system */
  theMixer  = OutputMixer::getInstance();
  theMixer->activateBaseband();

  /* Create Objects */
  theMixer->create(attention_cb);

  /* Set rendering clock */
  theMixer->setRenderingClkMode(OUTPUTMIXER_RNDCLK_NORMAL);

  /* Activate Mixer Object.
     Set output device to speaker with 2nd argument.
     If you want to change the output device to I2S,
     specify "I2SOutputDevice" as an argument.
  */
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);

  usleep(100 * 1000);

  /* Set main volume */
  theMixer->setVolume(-32, 0, 0);

  /* Unmute */
  board_external_amp_mute_control(false);

  printf("setup() complete\n");

  Oscillator.set(0, 0);
  Oscillator.set(1, 0);
}

#define C_NOTE  262
#define D_NOTE  294
#define E_NOTE  330
#define F_NOTE  349
#define G_NOTE  392

void loop()
{

  err_t err = OUTPUTMIXER_ECODE_OK;
  uint16_t note = 0xff;

  /* Fatal error */

  if (ErrEnd) {
    printf("Error End\n");
    goto stop_rendering;
  }
  
  /* Menu operation */

  if (Serial.available() > 0)
  {
    switch (Serial.read()) {
      case 'c': // C
        Oscillator.set(0, C_NOTE);
        Oscillator.set(1, C_NOTE);
        break;
      case 'd': // D
        Oscillator.set(0, D_NOTE);
        Oscillator.set(1, D_NOTE);
        break;
      case 'e': // E
        Oscillator.set(0, E_NOTE);
        Oscillator.set(1, E_NOTE);
        break;
      case 'f': // F
        Oscillator.set(0, F_NOTE);
        Oscillator.set(1, F_NOTE);
        break;
      case 'g': // G
        Oscillator.set(0, G_NOTE);
        Oscillator.set(1, G_NOTE);
        break;
      case 's': // Stop
        Oscillator.set(0, 0);
        Oscillator.set(1, 0);
        break;
      default:
        break;
    }
  }

  active();

  usleep(1 * 1000);

  return;

stop_rendering:
  printf("Exit rendering\n");
  exit(1);
}
