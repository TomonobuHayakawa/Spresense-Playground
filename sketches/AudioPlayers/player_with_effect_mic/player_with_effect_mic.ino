/*
 *  player_with_effect_mic.ino - Simple sound player with mic-in example application
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

#include <FrontEnd.h>
#include <MediaPlayer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>

FrontEnd *theFrontend;
MediaPlayer *thePlayer;
OutputMixer *theMixer;

File myFile;
SDClass theSD;

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

  for (uint32_t cnt = 0; cnt < size; cnt += channel_number*sizeof(uint16_t)){
    *ls = *ls + *rs;
    *rs = 0;
    ls+=channel_number;
    rs+=channel_number;
  }
}


/* ------------------------------------------------------------------------ */
/**
 * @brief Audio attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      ErrEnd = true;
   }
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
  AsRequestNextParam next;

  if(identifier == AS_PLAYER_ID_0){
    next.type = (!is_end) ? AsNextNormalRequest : AsNextStopResRequest;
    AS_RequestNextPlayerProcess(identifier, &next);
  }

  return;
}

/**
 * @brief Player done callback procedure
 *
 * @param [in] event        AsPlayerEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */
static bool mediaplayer_done_callback(AsPlayerEvent event, uint32_t result, uint32_t sub_result)
{
  printf("mp cb %x %x %x\n", event, result, sub_result);

  return true;
}

/**
 * @brief Player decode callback procedure
 *
 * @param [in] pcm_param    AsPcmDataParam type
 */
void mediaplayer_decode_callback(AsPcmDataParam pcm_param)
{
  {
    /* You can process a data here. */
    
/*    signed short *ptr = (signed short *)pcm_param.mh.getPa();

    for (unsigned int cnt = 0; cnt < pcm_param.size; cnt += 4)
      {
        *ptr = *ptr + 0;
        ptr++;
        *ptr = *ptr + 0;
        ptr++;
      }*/
  }
  
  theMixer->sendData(OutputMixer0,
                     outmixer_send_callback,
                     pcm_param);
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

  int err = theMixer->sendData(OutputMixer1,
                               outmixer_send_callback,
                               pcm_param);

  if (err != OUTPUTMIXER_ECODE_OK) {
    printf("OutputMixer send error: %d\n", err);
    return;
  }
}


/* ------------------------------------------------------------------------ */
/**
 * @brief Setup audio player to play mp3 file
 *
 * Set clock mode to normal <br>
 * Set output device to speaker <br>
 * Set main player to decode stereo mp3. Stream sample rate is set to "auto detect" <br>
 * System directory "/mnt/sd0/BIN" will be searched for MP3 decoder (MP3DEC file)
 * Open "Sound.mp3" file <br>
 * Set master volume to -16.0 dB
 */
void setup()
{
  /* Initialize SD Card */
  while (!theSD.begin()) {
    ; /* wait until SD card is mounted. */
  }

  // start audio system
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_RECORDINGPLAYER);
  
  /* start audio system */
  theFrontend = FrontEnd::getInstance();
  theFrontend->begin();
  
  thePlayer = MediaPlayer::getInstance();
  theMixer  = OutputMixer::getInstance();
  thePlayer->begin();

  puts("initialization Audio Library");
  /* Activate Baseband */

  theMixer->activateBaseband();

  /* Create Objects */

  thePlayer->create(MediaPlayer::Player0, attention_cb);
  theMixer->create(attention_cb);

  /* Set rendering clock */

  theMixer->setRenderingClkMode(OUTPUTMIXER_RNDCLK_NORMAL);

  /* Activate FrontEnd Object */
  /* Activate Player Object */
  /* Activate Mixer Object.
   * Set output device to speaker with 2nd argument.
   * If you want to change the output device to I2S,
   * specify "I2SOutputDevice" as an argument.
   */

  theFrontend->activate(frontend_done_callback);
  thePlayer->activate(MediaPlayer::Player0, mediaplayer_done_callback);
  theMixer->activate(OutputMixer1, HPOutputDevice, outputmixer_done_callback);
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);

  usleep(100 * 1000);

  /*
   * Set main player to decode stereo mp3. Stream sample rate is set to "auto detect"
   * Search for MP3 decoder in "/mnt/sd0/BIN" directory
   */
  err_t err = thePlayer->init(MediaPlayer::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_48000, AS_CHANNEL_STEREO);
  
  /* Verify player initialize */
  if (err != MEDIAPLAYER_ECODE_OK)
    {
      printf("Player0 initialize error\n");
      exit(1);
    }

  /* FrontEnd init */
  AsDataDest dst;
  dst.cb = frontend_pcm_callback;
  theFrontend->init(channel_number, bit_length, 384, AsDataPathCallback, dst);

  /* Open the file. Note that only one file can be open at a time,
     so you have to close this one before opening another. */
  myFile = theSD.open("Sound.mp3");

  /* Verify file open */
  if (!myFile)
    {
      printf("File open error\n");
      exit(1);
    }
  printf("Open! %d\n",myFile);

  err = thePlayer->writeFrames(MediaPlayer::Player0, myFile);

  if (err != MEDIAPLAYER_ECODE_OK)
    {
      printf("File Read Error! =%d\n",err);
      myFile.close();
      exit(1);
    }

  puts("Play!");

  theFrontend->start();

  /* Main volume set to -16.0 dB, Main player and sub player set to 0 dB */
  theMixer->setVolume(-160, 0, 0);

  // Start Player
  thePlayer->start(MediaPlayer::Player0, mediaplayer_decode_callback);

}

/**
 * @brief Play stream
 *
 * Send new frames to decode in a loop until file ends
 */
void loop()
{
  /* Send new frames to decode in a loop until file ends */
  err_t err = thePlayer->writeFrames(MediaPlayer::Player0, myFile);

  /*  Tell when player file ends */
  if (err == MEDIAPLAYER_ECODE_FILEEND)
    {
      printf("Main player File End!\n");
    }

  /* Show error code from player and stop */
  if (err)
    {
      printf("Main player error code: %d\n", err);
      goto stop_player;
    }

  if (ErrEnd)
    {
      printf("Error End\n");
      goto stop_player;
    }

  /* This sleep is adjusted by the time to read the audio stream file.
     Please adjust in according with the processing contents
     being processed at the same time by Application.
  */

  usleep(40000);


  /* Don't go further and continue play */
  return;

stop_player:
  sleep(1);
  thePlayer->stop(MediaPlayer::Player0);
  myFile.close();
  exit(1);
}
