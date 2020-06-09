/*
 *  YuruSynth.ino - Synthesizer for Yuru Music
 *
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

#include <SDHCI.h>
#include <MediaPlayer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <Oscillator.h>

#include "CodeController.h"

#include "Score.h"

/* Content information definition. */

/* Set volume[db] */

#define VOLUME_MASTER     -160
#define VOLUME_PLAY       -160
#define VOLUME_SYNTH      -80

/* Play element */

static struct PLAY_ELM_T
{
  MediaPlayer::PlayerId play_id;
  sem_t                 sem;
  bool                  err_end;
  DecodeDoneCallback    decode_callback;
  const char           *file_name;
  uint8_t               codec_type;
  uint32_t              sampling_rate;
  uint8_t               channel_number;
}
play_elm;

SDClass theSD;
MediaPlayer *thePlayer;
OutputMixer *theMixer;
Oscillator  *theOscillator;
CodeController *theCodeController;


static bool getFrame(AsPcmDataParam *pcm)
{
  const uint32_t sample = 240;
  /* Alloc MemHandle */
  if (pcm->mh.allocSeg(S0_REND_PCM_SUB_BUF_POOL, (sample * 2 * 2)) != ERR_OK) {
    return false;
  }
  theOscillator->exec((q15_t*)pcm->mh.getPa(), sample);

//  q15_t* ad = (q15_t*) pcm->mh.getPa();
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


/**
 * @brief Audio(player0) attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

static void attention_player_cb(const ErrorAttentionParam *atprm)
{
  printf("Attention(player%d)!\n", atprm->module_id);

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      play_elm.err_end = true;
   }
}

/**
 * @brief Audio(mixer) attention callback
 *
 */

static void attention_mixer_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention(mixer)!");

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      play_elm.err_end = true;
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
 * @brief Mixer0 data send callback procedure
 *
 * @param [in] identifier   Device identifier
 * @param [in] is_end       For normal request give false, for stop request give true
 */

static void outmixer0_send_callback(int32_t identifier, bool is_end)
{
  AsRequestNextParam next;

  next.type = (!is_end) ? AsNextNormalRequest : AsNextStopResRequest;

  AS_RequestNextPlayerProcess(AS_PLAYER_ID_0, &next);

  return;
}

/**
 * @brief Mixer1 data send callback procedure
 *
 */

static void outmixer1_send_callback(int32_t identifier, bool is_end)
{
  /* get PCM */
  AsPcmDataParam pcm_param;
  if (!getFrame(&pcm_param)) {
      puts("Allocation error");
  }

  /* Send PCM */
  int err = theMixer->sendData(OutputMixer1,
                               outmixer1_send_callback,
                               pcm_param);

  if (err != OUTPUTMIXER_ECODE_OK) {
     printf("OutputMixer send error: %d\n", err);
     return;
  }
  return;
}

/**
 * @brief Player0 ad Player1 done callback procedure
 *
 * @param [in] event        AsPlayerEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */

static bool mediaplayer_done_callback(AsPlayerEvent event, uint32_t result, uint32_t sub_result)
{
  sem_post(&play_elm.sem);

  return true;
}

/**
 * @brief Player0 decode callback procedure
 *
 * @param [in] pcm_param    AsPcmDataParam type
 */

void mediaplayer_decode_callback(AsPcmDataParam pcm_param)
{
  theMixer->sendData(OutputMixer0,
                     outmixer0_send_callback,
                     pcm_param);
}

/**
 *  @brief Repeated transfer of data read into the decoder
 */

void play_process( MediaPlayer *thePlayer, struct PLAY_ELM_T *elm, File& file)
{
  while (1)
    {
      /* Send new frames to decode in a loop until file ends */

      err_t err = thePlayer->writeFrames(elm->play_id, file);

      /*  Tell when player file ends */

      if (err == MEDIAPLAYER_ECODE_FILEEND)
        {
          printf("Player%d File End!\n", elm->play_id);
          break;
        }

      /* Show error code from player and stop */

      else if (err != MEDIAPLAYER_ECODE_OK)
        {
          printf("Player%d error code: %d\n", elm->play_id, err);
          break;
        }

      if (elm->err_end)
        {
          printf("Error End\n");
          break;
        }

      /* This sleep is adjusted by the time to read the audio stream file.
         Please adjust in according with the processing contents
         being processed at the same time by Application.
      */

      usleep(40000);
    }
}

/**
 *  @brief Setup main audio player and sub audio player
 *
 *  Set output device to speaker <br>
 *  Open two stream files <br>
 *  These should be in the root directory of the SD card. <br>
 *  Set main player to decode stereo mp3. Stream sample rate is set to "auto detect" <br>
 *  System directory "/mnt/sd0/BIN" will be searched for MP3 decoder (MP3DEC file) <br>
 *  This is the /BIN directory on the SD card. <br>
 */

static int player_thread(int argc, FAR char *argv[])
{
  File               file;
  struct PLAY_ELM_T *elm;

  /* Get information by task */

  elm = (struct PLAY_ELM_T *)strtol(argv[1], NULL, 16);

  printf("\"%s\" task start\n", argv[0]);

  /* Continue playing the same file. */

  while (1)
    {
      /*
       * Initialize main player to decode stereo mp3 stream with 48 kb/s sample rate
       * Search for MP3 codec in "/mnt/sd0/BIN" directory
       */

      err_t err = thePlayer->init(elm->play_id, elm->codec_type, "/mnt/sd0/BIN", elm->sampling_rate, elm->channel_number);

      if (err != MEDIAPLAYER_ECODE_OK)
        {
          printf("Initialize Error!=%d\n",err);
          file.close();
          break;
        }

      sem_wait(&elm->sem);

      file = theSD.open(elm->file_name);

      /* Verify file open */

      if (!file)
        {
          printf("File open error \"%s\"\n", elm->file_name);
          break;
        }

      printf("Open! %s\n", elm->file_name);

      /* Send first frames to be decoded */

      err = thePlayer->writeFrames(elm->play_id, file);

      if (err != MEDIAPLAYER_ECODE_OK && err != MEDIAPLAYER_ECODE_FILEEND)
        {
          printf("File Read Error! =%d\n",err);
          file.close();
          break;
        }

      /* Start Player */

      thePlayer->start(elm->play_id, elm->decode_callback);

      sem_wait(&elm->sem);

      printf("Start player%d!\n", elm->play_id);

      /* Running... */

      play_process(thePlayer, elm, file);

      /* Stop Player */

      printf("Stop player%d!\n", elm->play_id);

      thePlayer->stop(elm->play_id);

      sem_wait(&elm->sem);

      file.close();
    }

  printf("Exit task(%d).\n", elm->play_id);

  return 0;
}

void setup()
{
  /* Set player0 paramater */

  play_elm.play_id         = MediaPlayer::Player0;
  play_elm.err_end         = false;
  play_elm.decode_callback = mediaplayer_decode_callback;
  play_elm.file_name       = PLAYBACK_FILE_NAME;
  play_elm.codec_type      = PLAYBACK_CODEC_TYPE;
  play_elm.sampling_rate   = PLAYBACK_SAMPLING_RATE;
  play_elm.channel_number  = PLAYBACK_CH_NUM;
  sem_init(&play_elm.sem, 0, 0);

  Serial.begin(115200);

  /* Mount SD card */
  theSD.begin();

  /* Initialize memory pools and message libs */

  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYER);

  /* Get static audio modules instance */

  thePlayer = MediaPlayer::getInstance();
  theMixer  = OutputMixer::getInstance();
  theOscillator = new Oscillator();
  theCodeController = new CodeController();

  /* start audio system */

  thePlayer->begin();

  if(!theOscillator->begin(RectWave, 2)){
      puts("begin error!");
      exit(1);
  }

  theCodeController->begin();

  puts("initialization Audio Library");

  /* Activate Baseband */

  theMixer->activateBaseband();

  /* Create Objects */

  thePlayer->create(MediaPlayer::Player0, attention_player_cb);
  thePlayer->create(MediaPlayer::Player1, attention_player_cb);

  theMixer->create(attention_mixer_cb);

  /* Set rendering clock */

  theMixer->setRenderingClkMode(OUTPUTMIXER_RNDCLK_NORMAL);

  /* Activate Player Object */

  thePlayer->activate(MediaPlayer::Player0, mediaplayer_done_callback);
  sem_wait(&play_elm.sem);

  /* Activate Mixer Object.
   * Set output device to speaker with 2nd argument.
   * If you want to change the output device to I2S,
   * specify "I2SOutputDevice" as an argument.
   */

  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);
  theMixer->activate(OutputMixer1, HPOutputDevice, outputmixer_done_callback);

  /* Set master volume, Player0 volume, Player1 volume */

  theMixer->setVolume(VOLUME_MASTER, VOLUME_PLAY, VOLUME_SYNTH);

  /* Initialize task parameter. */

  const char *argv[2];
  char        param[16];

  snprintf(param, 16, "%x", &play_elm);

  argv[0] = param;
  argv[1] = NULL;

  /* Start task */

  task_create("player_thread", 155, 2048, player_thread, (char* const*)argv);

  bool er;
  /* attack = 1000,decay = 700, sustain = 30, release = 300 */
  er = theOscillator->set(0, 10,300,0,1);
  er = theOscillator->set(1, 10,300,0,1);

  /* Mute */
  er = theOscillator->set(0, 0);
  er = theOscillator->set(1, 0);

  if(!er){
    puts("init error!");
    exit(1);
  }

  for (int i = 0; i < 3; i++) {
    /* get PCM */
    AsPcmDataParam pcm_param;
    if (!getFrame(&pcm_param)) {
      break;
    }

    /* Send PCM */
    int err = theMixer->sendData(OutputMixer1,
                                 outmixer1_send_callback,
                                 pcm_param);

    if (err != OUTPUTMIXER_ECODE_OK) {
      printf("OutputMixer send error: %d\n", err);
      exit(1);
    }
  }
}

/**
 *  @brief Play streams
 *
 * Send new frames to decode in a loop until file ends
 */

void loop()
{
  static String current_code = "C";
  
  while (Serial.available() > 0)
  {
    bool er = true;
    switch (Serial.read()) {
      case 'i': 
        er = theOscillator->set(0, theCodeController->get(current_code.c_str(),0));
        break;
      case 'o': 
        er = theOscillator->set(1, theCodeController->get(current_code.c_str(),1));
        break;
      default:
        break;
    }
  }  /* Do nothing on main task. */

//  theOscillator->set(0, 0);
//  theOscillator->set(1, 0);
  
  uint32_t interval = (60 * 1000 * 1000) / tempo;

  static int i = 0;
  static int j = 0;

  if (i == beat-1) {
    puts("*");
    i=0;
    if(j>=score_size-1){
      j=0;
    }else{
      j++;
    }
  } else {
//    puts(".");
    i++;
  }
  current_code = score[j][i];
  Serial.println(current_code);
  usleep(interval);
}
