/*
    Mixer4ch.ino - Mixing example of 4ch capture data.
    Copyright 2023 Sony Semiconductor Solutions Corporation

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
#include <arch/board/board.h>

OutputMixer *theMixer;
FrontEnd *theFrontend;

static const int32_t channel_number = 4;
static const int32_t bit_length     = 16;
static const int32_t frame_sample   = 240;
static const int32_t capture_size   = frame_sample * (bit_length / 8) * channel_number;
static const int32_t render_size    = frame_sample * (bit_length / 8) * 2;

/* ------------------------------------------------------------------------ */
/*
 * 4ch Mixer
 */
static void filter_sample(int16_t *in, uint32_t in_size, int16_t *out, uint32_t out_size)
{
  /* Example : A, B, C, D => A+B, C+D */

  int16_t *in0 = in;
  int16_t *in1 = in0 + 1;
  int16_t *in2 = in1 + 1;
  int16_t *in3 = in2 + 1;

  int16_t *outl = out;
  int16_t *outr = out + 1;

  uint32_t size = in_size;
  if(in_size/channel_number < out_size/2) size = out_size*channel_number/2;    

  for (uint32_t cnt = 0; cnt < size; cnt += channel_number){
    *outl = *in0 + *in1;
    *outr = *in2 + *in3;

    in0+=channel_number;
    in1+=channel_number;
    in2+=channel_number;
    in3+=channel_number;
    outl+=2;
    outr+=2;
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

static void frontend_pcm_callback(AsPcmDataParam input_pcm)
{
  AsPcmDataParam output_pcm;

  while(1){
    if(output_pcm.mh.allocSeg(S0_REND_PCM_BUF_POOL, render_size) == ERR_OK) break;
    if(output_pcm.mh.allocSeg(S0_REND_PCM_SUB_BUF_POOL, render_size) == ERR_OK) break;
    delay(1);
  }
  
  /* Set PCM parameters */
  output_pcm.identifier = OutputMixer0;
  output_pcm.callback = 0;
  output_pcm.bit_length = bit_length;
  output_pcm.size = render_size;
  output_pcm.sample = frame_sample;
  output_pcm.is_valid = true;
  output_pcm.is_end = input_pcm.is_end;

  /* You can imprement any audio signal process */

  if (input_pcm.size){
    filter_sample((int16_t*)input_pcm.mh.getPa(), input_pcm.size,(int16_t*)output_pcm.mh.getPa(), output_pcm.size);
  }

  int err = theMixer->sendData(OutputMixer0,
                               outmixer_send_callback,
                               output_pcm);

  if (err != OUTPUTMIXER_ECODE_OK) {
    printf("OutputMixer send error: %d\n", err);
    return false;
  }

  return true;

}

static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");

/*  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING) {
    ErrEnd = true;
  }*/
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

  /* Set capture clock */
  theFrontend->setCapturingClkMode(FRONTEND_CAPCLK_NORMAL);
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
  theFrontend->init(channel_number, bit_length, frame_sample, AsDataPathCallback, dst);
  
  /* Unmute */
  board_external_amp_mute_control(false);

  printf("setup() complete\n");

  theFrontend->start();

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
