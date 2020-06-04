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

#include <FrontEnd.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <stdio.h>
#include <stdlib.h>
#include <arch/board/board.h>

#include <Oscillator.h>

#include <stdio.h>

OutputMixer *theMixer;
FrontEnd *theFrontend;

bool ErrEnd = false;

static const int channel_number = 2;
static const int bit_length     = 16;

/* ------------------------------------------------------------------------ */
/*
 * Filter sample
 */
static void filter_sample(int16_t *ptr, uint32_t size)
{
  /* Example : L, R => L+R, 0 */

  int16_t *ls = ptr;
  int16_t *rs = ls + 1;

  for (uint32_t cnt = 0; cnt < size; cnt += channel_number){
    *ls = *ls + *rs;
    *rs = 0;
    ls+=channel_number;
    rs+=channel_number;
  }
}

/* ------------------------------------------------------------------------ */
/*
 * Callbacks
 */
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

static bool frontend_done_callback(AsMicFrontendEvent ev, uint32_t result, uint32_t sub_result)
{
  printf("frontend_done_callback cb\n");
  return true;
}

static void frontend_pcm_callback(AsPcmDataParam pcm_param)
{
  /* You can imprement any audio signal process */

  if (pcm_param.size){
    filter_sample((int16_t*)pcm_param.mh.getPa(), pcm_param.size);
  }

  int err = theMixer->sendData(OutputMixer0,
                               outmixer_send_callback,
                               pcm_param);

  if (err != OUTPUTMIXER_ECODE_OK) {
    printf("OutputMixer send error: %d\n", err);
    return;
  }
}

static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING) {
    ErrEnd = true;
  }
}

/* ------------------------------------------------------------------------ */
/*
 * Setup
 */
void setup()
{
  printf("setup() start\n");

  /* Display menu */

  Serial.begin(115200);

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_RECORDINGPLAYER);

  /* Start audio system */
  theFrontend = FrontEnd::getInstance();
  theFrontend->begin();
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
  theFrontend->activate(frontend_done_callback);
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);

  usleep(100 * 1000);

  /* Set main volume */
  theMixer->setVolume(-320, 0, 0);

  AsDataDest dst;
  dst.cb = frontend_pcm_callback;
  theFrontend->init(channel_number, bit_length, 384, AsDataPathCallback, dst);
  
  /* Unmute */
  board_external_amp_mute_control(false);

  printf("setup() complete\n");

}

/* ------------------------------------------------------------------------ */
/*
 * Loop
 */
void loop()
{
  
  /* Menu operation */

  if (Serial.available() > 0)
  {
    switch (Serial.read()) {
      case 's': // start
        theFrontend->start();
        break;
      case 'p': // stop
        theFrontend->stop();
        break;
      default:
        break;
    }
  }

  usleep(1 * 1000);
  
  return;
}
