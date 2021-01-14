/*
 *  rendering.ino - Rendering example for AudioOscillator
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

#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <stdio.h>
#include <stdlib.h>
#include <arch/board/board.h>

#include <AudioOscillator.h>

Oscillator  *theOscillator;
OutputMixer *theMixer;
bool ErrEnd = false;

static void menu()
{
  printf("=== MENU (input key ?) ==============\n");
  printf("c: Sound \"Do\"\n");
  printf("d: Sound \"Ro\"\n");
  printf("e: Sound \"Mi\"\n");
  printf("f: Sound \"Fa\"\n");
  printf("g: Sound \"So\"\n");
  printf("s: Stop sound\n");
  printf("=====================================\n");
}

static bool getFrame(AsPcmDataParam *pcm)
{
  const uint32_t sample = 480;
  /* Alloc MemHandle */
  if (pcm->mh.allocSeg(S0_REND_PCM_BUF_POOL, (sample * 2 * 2)) != ERR_OK) {
    return false;
  }
  theOscillator->exec((q15_t*)pcm->mh.getPa(), sample);

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
  (void)done_param;
  printf(">> %x done from %x\n", reply_of, requester_dtq);
  return;
}

/**
   @brief Mixer data send callback procedure

   @param [in] identifier   Device identifier
   @param [in] is_end       For normal request give false, for stop request give true
*/
static void outmixer_send_callback(int32_t identifier, bool is_end)
{
  (void)identifier;
  (void)is_end;
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
  Serial.begin(115200);

  printf("setup() start\n");

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYER);

  /* Create Objects */
  theOscillator = new Oscillator();

//  if(!theOscillator->begin(SinWave, 2)){
//  if(!theOscillator->begin(RectWave, 2)){
  if(!theOscillator->begin(SawWave, 2)){
      puts("begin error!");
      exit(1);
  }

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
  theMixer->setVolume(-160, 0, 0);

  /* Unmute */
  board_external_amp_mute_control(false);

  printf("setup() complete\n");
  menu();

  bool er;
  er = theOscillator->set(0, 0);
  er = theOscillator->set(1, 0);

  /* attack = 1000,decay = 700, sustain = 30, release = 300 */
  er = theOscillator->set(0, 700,200,50,400);
  er = theOscillator->set(1, 700,200,50,400);

  er = theOscillator->lfo(0, 4, 2);
  er = theOscillator->lfo(1, 4, 2);

  if(!er){
    puts("init error!");
    exit(1);
  }
}

#define C_NOTE  262
#define D_NOTE  294
#define E_NOTE  330
#define F_NOTE  349
#define G_NOTE  392

void loop()
{
  /* Menu operation */

  if (Serial.available() > 0)
  {
    bool er = true;
    switch (Serial.read()) {
      case 'i': // Sin
        er = theOscillator->set(0,SinWave);
        break;
      case 'r': // Rect
        er = theOscillator->set(0,RectWave);
        break;
      case 'a': // Saw
        er = theOscillator->set(0,SawWave);
        break;
      case 'c': // C
        er = theOscillator->set(0, C_NOTE);
        er = theOscillator->set(1, C_NOTE*2);
        break;
      case 'd': // D
        er = theOscillator->set(0, D_NOTE);
        er = theOscillator->set(1, D_NOTE*2);
        break;
      case 'e': // E
        er = theOscillator->set(0, E_NOTE);
        er = theOscillator->set(1, E_NOTE*2);
        break;
      case 'f': // F
        er = theOscillator->set(0, F_NOTE);
        er = theOscillator->set(1, F_NOTE*2);
        break;
      case 'g': // G
        er = theOscillator->set(0, G_NOTE);
        er = theOscillator->set(1, G_NOTE*2);
        break;
      case 's': // Stop
        er = theOscillator->set(0, 0);
        er = theOscillator->set(1, 0);
        break;
      default:
        break;
    }
    if(!er){
      puts("set error!");
      goto stop_rendering;
    }
  }

  active();

  usleep(1 * 1000);

  return;

stop_rendering:
  printf("Exit rendering\n");
  exit(1);
}
