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
#include <Synthesizer.h>
#include <MediaPlayer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <MP.h>

#include "ChordController.h"
#include "KeyInfo.h"
#include "Score.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum number of channels */

#define CHANNEL_NUMBER        8

/* Oscillator parameter */
/* Organ */
/*
#define OSC_WAVE_MODE         AsSynthesizerSinWave
#define OSC_CH_NUM            CHANNEL_NUMBER
#define OSC_EFFECT_ATTACK     1
#define OSC_EFFECT_DECAY      1
#define OSC_EFFECT_SUSTAIN    100
#define OSC_EFFECT_RELEASE    1
*/

/* Piano */
#define OSC_WAVE_MODE         AsSynthesizerRectWave
#define OSC_CH_NUM            CHANNEL_NUMBER
#define OSC_EFFECT_ATTACK     20
#define OSC_EFFECT_DECAY      500
#define OSC_EFFECT_SUSTAIN    1
#define OSC_EFFECT_RELEASE    1

/* DSP file path */

#define DSPBIN_PATH           "/mnt/sd0/BIN/YUSYNTH"

/* Set volume[db] */

#define VOLUME_MASTER     -160
#define VOLUME_PLAY       -160
#define VOLUME_SYNTH      -80

/* Keyboard params */
const int keyboard_core = 1;

/* Chord clear flag */
bool clear_flg = false;

/****************************************************************************
 * Private Data
 ****************************************************************************/
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

/****************************************************************************/
SDClass theSD;

MediaPlayer *thePlayer;
OutputMixer *theMixer;
Synthesizer *theSynthesizer;

ChordController *theChordController;
KeyInfo theKeyInfo;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Audio(player1) attention callback
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
 * @brief Audio(Synth) attention callback
 *
 */
static void attention_synth_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention(synth)!");

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      play_elm.err_end = true;
   }
}

/****************************************************************************/
/**
 * @brief Player0 ad Player1 done callback procedure
 *
 * @param [in] event        AsPlayerEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */

static bool mediaplayer_done_callback(AsPlayerEvent /*event*/, uint32_t /*result*/, uint32_t /*sub_result*/)
{
  sem_post(&play_elm.sem);

  return true;
}

/**
 * @brief Mixer done callback procedure
 *
 * @param [in] requester_dtq    MsgQueId type
 * @param [in] reply_of         MsgType type
 * @param [in,out] done_param   AsOutputMixDoneParam type pointer
 */

static void outputmixer0_done_callback(MsgQueId              /*requester_dtq*/,
                                       MsgType               /*reply_of*/,
                                       AsOutputMixDoneParam* /*done_param*/)
{
  return;
}

static void outputmixer1_done_callback(MsgQueId requester_dtq,
                                       MsgType reply_of,
                                       AsOutputMixDoneParam *done_param)
{
  return;
}

/**
 * @brief Synthesizer done callback function
 *
 * @param [in] event        AsSynthesizerEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */

static void synthesizer_done_callback(AsSynthesizerEvent /*event*/,
                                      uint32_t           /*result*/,
                                      void*              /*param*/)
{
  return;
}

/****************************************************************************/
/**
 * @brief Mixer1 data send callback procedure
 *
 * @param [in] identifier   Device identifier
 * @param [in] is_end       For normal request give false, for stop request give true
 */

static void outmixer1_send_callback(int32_t /*identifier*/, bool is_end)
{
  AsRequestNextParam next;

  next.type = (!is_end) ? AsNextNormalRequest : AsNextStopResRequest;

  AS_RequestNextPlayerProcess(AS_PLAYER_ID_1, &next);

  return;
}

/****************************************************************************/
/**
 * @brief Player0 decode callback procedure
 *
 * @param [in] pcm_param    AsPcmDataParam type
 */

void mediaplayer_decode_callback(AsPcmDataParam pcm_param)
{
  theMixer->sendData(OutputMixer1,
                     outmixer1_send_callback,
                     pcm_param);
}


/**
 * @brief Audio attention callback
 *
 * When Internal error occur, this function will be called back.
 */



/****************************************************************************
 * Player process
 ****************************************************************************/
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

static int player_thread(int /*argc*/, FAR char *argv[])
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
      attachTimerInterrupt(timer_isr, (600 * 1000 ) );

      sem_wait(&elm->sem);

      printf("Start player%d!\n", elm->play_id);

      /* Running... */

      play_process(thePlayer, elm, file);

      /* Stop Player */

      printf("Stop player%d!\n", elm->play_id);

      thePlayer->stop(elm->play_id);

      sem_wait(&elm->sem);
      detachTimerInterrupt();
      clear_flg = true;

      file.close();
    }

  printf("Exit task(%d).\n", elm->play_id);

  return 0;
}

/****************************************************************************
 * Timer ISR
 ****************************************************************************/
static bool timer_flg = false;

unsigned int timer_isr(void) {
  timer_flg = true;
  return ((60 * 1000 * 1000) / tempo);
}

/****************************************************************************
 * Publuc Functions
 ****************************************************************************/

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  /* Mount SD card */
  theSD.begin();

  /* Multi-Core library */
  int ret = MP.begin(keyboard_core);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }
  MP.RecvTimeout(10);

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYER);

  /* Get static audio modules instance */
  thePlayer = MediaPlayer::getInstance();
  theMixer  = OutputMixer::getInstance();
  theSynthesizer = Synthesizer::getInstance();
  theChordController = new ChordController();

  /* Begin objects */
  thePlayer->begin();
  theMixer->activateBaseband();
  theSynthesizer->begin();
  theChordController->begin();
  
  /* Create objects */
  thePlayer->create(MediaPlayer::Player1, attention_player_cb);
  theMixer->create(attention_mixer_cb);
  theSynthesizer->create(attention_synth_cb);

  /* Activate objects */
  thePlayer->activate(MediaPlayer::Player1, mediaplayer_done_callback);
  sem_wait(&play_elm.sem);
  
  theSynthesizer->activate(synthesizer_done_callback, NULL);
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer0_done_callback);
  theMixer->activate(OutputMixer1, HPOutputDevice, outputmixer1_done_callback);

  /* Initialize synthesizer. */
  theSynthesizer->init(OSC_WAVE_MODE,
                       OSC_CH_NUM,
                       DSPBIN_PATH,
                       OSC_EFFECT_ATTACK,
                       OSC_EFFECT_DECAY,
                       OSC_EFFECT_SUSTAIN,
                       OSC_EFFECT_RELEASE);

  /* Set master volume, Player0 volume, Player1 volume */

  theMixer->setVolume(VOLUME_MASTER, VOLUME_SYNTH, VOLUME_PLAY);

  /* Set player1 paramater */

  play_elm.play_id         = MediaPlayer::Player1;
  play_elm.err_end         = false;
  play_elm.decode_callback = mediaplayer_decode_callback;
  play_elm.file_name       = PLAYBACK_FILE_NAME;
  play_elm.codec_type      = PLAYBACK_CODEC_TYPE;
  play_elm.sampling_rate   = PLAYBACK_SAMPLING_RATE;
  play_elm.channel_number  = PLAYBACK_CH_NUM;
  sem_init(&play_elm.sem, 0, 0);

  /* Initialize task parameter. */

  const char *argv[2];
  char        param[16];

  snprintf(param, 16, "%x", &play_elm);

  argv[0] = param;
  argv[1] = NULL;

  /* Start task */
  task_create("player_thread", 155, 2048, player_thread, (char* const*)argv);

  /* Start synthesizer */
  theSynthesizer->start();

}

/* ------------------------------------------------------------------------ */
void loop()
{
  static unsigned int i = 0;
  static unsigned int j = 0;
  static String current_chord = score[j][i];

  int8_t   sndid = 100;
  int8_t   rcvid = 0;
  void*    tmp;
  err_t er;

  if(clear_flg){
    clear_flg = false;
    i = 0;
    j = 0;
  }

  if(timer_flg){
    current_chord = score[j][i];
    Serial.print(current_chord);
    Serial.print(" ");

    if (i == beat-1) {
      Serial.println();
      i=0;
      if(j>=score_size-1){
        j=0;
      }else{
        j++;
      }
    } else {
      i++;
    }
    timer_flg = false;
  }

  int ret = MP.Send(sndid, theKeyInfo.adr(), keyboard_core);
  if (ret < 0) {
    puts("Keyboad Error!");
    return;
  }

  ret = MP.Recv(&rcvid, &tmp, keyboard_core);
  if (ret < 0) {
    puts("Keyboad Deadlock!");
    sleep(1);
    return;
  }

  // print keyinfo
//  theKeyInfo.print();

  // Set Beat
  for(unsigned int k=1; k<theKeyInfo.max_bits(); k++)
  {
  	if(theKeyInfo.get(0,k)){
      if(theKeyInfo.is_change(0,k)){
        er = theSynthesizer->set(k-1, theChordController->get(current_chord.c_str(),k-1));
      }
    }else{
      er = theSynthesizer->set(k-1, 0);
    }
    usleep(10*1000);
  }

  theKeyInfo.update();
}
