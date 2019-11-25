/*
 *  player_playlist.ino - Sound player example application by playlist
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

#include <SDHCI.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <stdio.h>
#include <stdlib.h>
#include <arch/board/board.h>

//#include <memutil/mem_layout.h>



SDClass theSD;
OutputMixer *theMixer;

File myFile;

bool ErrEnd = false;

static bool getFrame(AsPcmDataParam *pcm)
{
  const uint32_t readsize = 480 * 2 * 2;
//  uint16_t tmp[readsize * 2];

  /* Alloc MemHandle */
  if (pcm->mh.allocSeg(S0_REND_PCM_BUF_POOL, readsize) != ERR_OK) {
    return false;
  }

/*  pcm->size = myFile.read(tmp, readsize);

  uint16_t* wp = pcm->mh.getPa();
  uint16_t* rp = tmp;
  
  for(int i = 0; i < pcm->size; i++){
    *wp = *rp;
    wp++;
    *wp = *rp;
    wp++;rp++;
  }*/

  /* Set PCM parameters */
  pcm->identifier = 0;
  pcm->callback = 0;
  pcm->bit_length = 16;
  pcm->size = myFile.read(pcm->mh.getPa(), readsize);
  pcm->size = pcm->size;
  pcm->sample = pcm->size / 2 / 2;  
  pcm->is_end = (pcm->size < readsize);
  pcm->is_valid = true;

  return true;
}


static bool start(uint8_t no)
{
  printf("start(%d) start\n", no);

  /* Open file placed on SD card */

  const char *raw_files[] =
  {
    "clap.raw",
    "clash.raw",
    "kick.raw",
  };

  char fullpath[64] = { 0 };

  assert(no < sizeof(raw_files) / sizeof(char *));

  snprintf(fullpath, sizeof(fullpath), "AUDIO/%s", raw_files[no]);

  myFile = theSD.open(fullpath);

  if (!myFile)
    {
      printf("File open error\n");
      return false;
    }

  printf("start() complete\n");

  return true;
}

static void stop()
{
  printf("stop\n");

  AsPcmDataParam pcm_param;
  getFrame(&pcm_param);

  pcm_param.is_end = true;

  int err = theMixer->sendData(OutputMixer0,
                               outmixer_send_callback,
                               pcm_param);

  if (err != OUTPUTMIXER_ECODE_OK) {
    printf("OutputMixer send error: %d\n", err);
  }

  myFile.close();
}

/**
 * @brief Mixer done callback procedure
 *
 * @param [in] requester_dtq    MsgQueId type
 * @param [in] reply_of         MsgType type
 * @param [in,out] done_param   AsOutputMixDoneParam type pointer
 */
static void outputmixer_done_callback(MsgQueId requester_dtq,
                                      MsgType reply_of,
                                      AsOutputMixDoneParam *done_param)
{
  printf(">> %x done\n", reply_of);
  return;
}

/**
 * @brief Mixer data send callback procedure
 *
 * @param [in] identifier   Device identifier
 * @param [in] is_end       For normal request give false, for stop request give true
 */
static void outmixer_send_callback(int32_t identifier, bool is_end)
{
  //printf("send done %d %d\n", identifier, is_end);
 
  //AsPcmDataParam pcm_param;
  //fillFrame(&pcm_param);

  //theMixer->sendData(OutputMixer0,
  //                   outmixer_send_callback,
  //                   pcm_param);
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
  theSD.begin();

  /* Start audio system */
  theMixer  = OutputMixer::getInstance();
  theMixer->activateBaseband();

  /* Create Objects */
  theMixer->create(attention_cb);

  /* Set rendering clock */
  theMixer->setRenderingClkMode(OUTPUTMIXER_RNDCLK_NORMAL);

  /* Activate Mixer Object.
   * Set output device to speaker with 2nd argument.
   * If you want to change the output device to I2S,
   * specify "I2SOutputDevice" as an argument.
   */
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);

  usleep(100 * 1000);

  /* Set main volume */
  theMixer->setVolume(0, 0, 0);

  /* Unmute */
  board_external_amp_mute_control(false);

  printf("setup() complete\n");
}

static enum State {
    Ready,
    Active
} s_state = Ready;

uint8_t start_event(uint8_t playno, uint8_t eventno)
{
  if (s_state == Ready) {
    start(eventno);
    s_state = Active;
  } else {
    if(playno == eventno){
      printf("Restart\n");
      myFile.seek(0);
    }else{
      myFile.close();
      start(eventno);
    }
  }
  return eventno;
}


void loop()
{

  err_t err = OUTPUTMIXER_ECODE_OK;
  uint8_t playno = 0xff;

  /* Fatal error */

  if (ErrEnd) {
    printf("Error End\n");
    goto stop_player;
  }

  /* Menu operation */

  if (Serial.available() > 0)
    {
      switch (Serial.read()) {
        case 'p': // play
          playno = start_event(playno,0);
          break;
        case 'o': // play
          playno = start_event(playno,1);
          break;
        case 'i': // play
          playno = start_event(playno,2);
          break;
        case 's': // stop
          if (s_state == Active)
            {
              stop();
            }
          s_state = Ready;
          break;
          
          default:
            break;
        }
    }

  /* Processing in accordance with the state */

  switch (s_state) {
    case Ready:
      break;

    case Active:
      /* Send new frames to be decoded until end of file */

      for(int i = 0; i < 5; i++) {
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
          goto stop_player;
        }

        /* If file end, close file and move to Stopped state */
        if (pcm_param.is_end) {
          myFile.close();
          s_state = Ready;
          break;        
        }
      }
      break;

    default:
      break;
  }

  /* This sleep is adjusted by the time to read the audio stream file.
     Please adjust in according with the processing contents
     being processed at the same time by Application.
   */

  usleep(1 * 1000);

  return;

stop_player:
  printf("Exit player\n");
  myFile.close();
  exit(1);
}
