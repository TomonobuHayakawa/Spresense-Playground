/*
 *  voiceUI_player.ino - Sound player application by playlist with Voice UI
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
#include <Recognizer.h>
#include <MediaPlayer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <EEPROM.h>
#include <audio/utilities/playlist.h>
#include <stdio.h>
#include <stdlib.h>
//#include <arch/chip/cxd56_audio.h>

SDClass theSD;
FrontEnd *theFrontend;
Recognizer *theRecognizer;
MediaPlayer *thePlayer;
OutputMixer *theMixer;
Playlist thePlaylist("TRACK_DB.CSV");
Track currentTrack;

#define ANALOG_MIC_GAIN  0 /* +0dB */

/* Number of input channels
 * Set either 1, 2, or 4.
 */

static const uint8_t  recognizer_channel_number = 1;

/* Audio bit depth
 * Set 16 or 24
 */

static const uint8_t  recognizer_bit_length = 16;
char g_event_char;


int eeprom_idx = 0;
struct SavedObject {
  int saved;
  int volume;
  int random;
  int repeat;
  int autoplay;
} preset;

File myFile;

bool ErrEnd = false;

static void menu()
{
  printf("=== MENU (input voice command! ) ==============\n");
  printf("saisei kaishi:  play  \n");
  printf("ichiji teishi:  stop  \n");
  printf("mae ni tobu:    next  \n");
  printf("ushiro ni tobu: prev  \n");
  printf("=====================================\n");
}

static void show(Track *t)
{
  printf("%s | %s | %s\n", t->author, t->album, t->title);
}

static void list()
{
  Track t;
  thePlaylist.restart();
  printf("-----------------------------\n");
  while (thePlaylist.getNextTrack(&t)) {
    if (0 == strncmp(currentTrack.title, t.title, 64)) {
      printf("-> ");
    }
    show(&t);
  }
  printf("-----------------------------\n");

  /* restore the current track */
  thePlaylist.restart();
  while (thePlaylist.getNextTrack(&t)) {
    if (0 == strncmp(currentTrack.title, t.title, 64)) {
      break;
    }
  }
}

static bool next()
{
  if (thePlaylist.getNextTrack(&currentTrack)) {
    show(&currentTrack);
    return true;
  } else {
    return false;
  }
}

static bool prev()
{
  if (thePlaylist.getPrevTrack(&currentTrack)) {
    show(&currentTrack);
    return true;
  } else {
    return false;
  }
}

static err_t setPlayer(Track *t)
{
  static uint8_t   s_codec   = 0;
  static uint32_t  s_fs      = 0;
  static uint8_t   s_bitlen  = 0;
  static uint8_t   s_channel = 0;
  static uint8_t   s_clkmode = (AsClkMode)-1;
  err_t err = MEDIAPLAYER_ECODE_OK;
  uint8_t clkmode;

  if ((s_codec   != t->codec_type) ||
      (s_fs      != t->sampling_rate) ||
      (s_bitlen  != t->bit_length) ||
      (s_channel != t->channel_number)) {

    /* Set audio clock 48kHz/192kHz */

    clkmode = (t->sampling_rate <= 48000) ? OUTPUTMIXER_RNDCLK_NORMAL : OUTPUTMIXER_RNDCLK_HIRESO;

    if (s_clkmode != clkmode) {

      /* When the audio master clock will be changed, it should change the clock
       * mode once after returning the ready state. At the first start-up, it
       * doesn't need to call setReadyMode().
       */

      if (s_clkmode != (AsClkMode)-1) {
//        theAudio->setReadyMode();
          thePlayer->deactivate(MediaPlayer::Player0);
          theMixer->deactivate(OutputMixer0);
      }

      s_clkmode = clkmode;

//      theAudio->setRenderingClockMode(clkmode);
      theMixer->setRenderingClkMode(clkmode);

      /* Set output device to speaker with first argument.
       * If you want to change the output device to I2S,
       * specify "AS_SETPLAYER_OUTPUTDEVICE_I2SOUTPUT" as an argument.
       * Set speaker driver mode to LineOut with second argument.
       * If you want to change the speaker driver mode to other,
       * specify "AS_SP_DRV_MODE_1DRIVER" or "AS_SP_DRV_MODE_2DRIVER" or "AS_SP_DRV_MODE_4DRIVER"
       * as an argument.
       */

//      theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);
        thePlayer->activate(MediaPlayer::Player0, mediaplayer_done_callback);
        theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);
    }

    /* Initialize player */

    err = thePlayer->init(MediaPlayer::Player0,
                               t->codec_type,
                               "/mnt/sd0/BIN",
                               t->sampling_rate,
                               t->bit_length,
                               t->channel_number);
    if (err != MEDIAPLAYER_ECODE_OK) {
      printf("Player0 initialize error\n");
      return err;
    }

    s_codec   = t->codec_type;
    s_fs      = t->sampling_rate;
    s_bitlen  = t->bit_length;
    s_channel = t->channel_number;
  }

  return err;
}

static bool start()
{
  err_t err;
  Track *t = &currentTrack;

  /* Prepare for playback with the specified track */
  ledOn(LED1);

  err = setPlayer(t);

  if (err != MEDIAPLAYER_ECODE_OK) {
    return false;
  }

  /* Open file placed on SD card */

  char fullpath[64] = { 0 };
  snprintf(fullpath, sizeof(fullpath), "AUDIO/%s", t->title);

  myFile = theSD.open(fullpath);
 
  if (!myFile){
    printf("File open error\n");
    return false;
  }

  /* Send first frames to be decoded */
  err = thePlayer->writeFrames(MediaPlayer::Player0, myFile);

  if ((err != MEDIAPLAYER_ECODE_OK) && (err != MEDIAPLAYER_ECODE_FILEEND)) {
    printf("File Read Error! =%d\n",err);
    return false;
  }

  printf("start\n");
  thePlayer->start(MediaPlayer::Player0, mediaplayer_decode_callback);

  return true;
}

static void stop()
{
  ledOff(LED1);
  printf("stop\n");
  thePlayer->stop(MediaPlayer::Player0,AS_STOPPLAYER_NORMAL);
  myFile.close();
}

/**
 * @brief FrontEnd done callback procedure
 *
 * @param [in] event        AsRecorderEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */
static bool frontend_done_callback(AsMicFrontendEvent ev, uint32_t result, uint32_t sub_result)
{
  printf("frontend_done_callback cb\n");
  return true;
}

static void recognizer_done_callback(RecognizerResult *result)
{
  printf("recognizer cb\n");

  return;
}

static void recognizer_find_callback(AsRecognitionInfo info)
{
 static const char *keywordstrings[] = {
    "HDMI 1",
    "HDMI 2",
    "HDMI 3",
    "HDMI 4",
    "Start Play",
    "Pause",
    "Next",
    "Prev",
    "Power ON TV",
    "Power OFF TV",
    "Fast Forward",
    "Rewind",
    "Mute OFF",
    "Mute ON",
    "Volume Control"
  };

 static const char event_char[] = {
    '*',
    '*',
    '*',
    '*',
    'p',
    's',
    'n',
    'b',
    '*',
    '*',
    '*',
    '*',
    '*',
    '*',
    '*'
  };

  printf("Notify size %d byte\n", info.size);

  int8_t *param = (int8_t *)info.mh.getVa();

  for (uint32_t i = 0; i < info.size / sizeof(int8_t); i++)
    {
      if (param[i] == 1)
        {
          g_event_char = event_char[i];
          printf("%s\n", keywordstrings[i]);
        }
    }  return;
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

  next.type = (!is_end) ? AsNextNormalRequest : AsNextStopResRequest;

  AS_RequestNextPlayerProcess(AS_PLAYER_ID_0, &next);

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
    ;
  }
  
  theMixer->sendData(OutputMixer0,
                     outmixer_send_callback,
                     pcm_param);
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
  const char *playlist_dirname = "/mnt/sd0/PLAYLIST";
  bool success;

  /* Display menu */

  Serial.begin(115200);
  menu();
  
  /* Initialize memory pools and message libs */  
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYERANDRECOGNIZER);

  /* Load preset data */

  EEPROM.get(eeprom_idx, preset);
  if (!preset.saved) {
    /* If no preset data, come here */
    preset.saved = 1;
    preset.volume = -160; /* default */
    preset.random = 0;
    preset.repeat = 1;
    preset.autoplay = 0;
    EEPROM.put(eeprom_idx, preset);
  }
  printf("Volume=%d\n", preset.volume);
  printf("Random=%s\n", (preset.random) ? "On" : "Off");
  printf("Repeat=%s\n", (preset.repeat) ? "On" : "Off");
  printf("Auto=%s\n", (preset.autoplay) ? "On" : "Off");

  /* Use SD card */

  theSD.begin();


  /* Initialize playlist */

  success = thePlaylist.init(playlist_dirname);
  if (!success) {
    printf("ERROR: no exist playlist file %s/TRACK_DB.CSV\n",
           playlist_dirname);
    while (1);
  }

  /* Set random seed to use shuffle mode */

  struct timespec ts;
  clock_systimespec(&ts);
  srand((unsigned int)ts.tv_nsec);

  /* Restore preset data */

  if (preset.random) {
    thePlaylist.setPlayMode(Playlist::PlayModeShuffle);
  }

  if (preset.repeat) {
    thePlaylist.setRepeatMode(Playlist::RepeatModeOn);
  }

  thePlaylist.getNextTrack(&currentTrack);

  if (preset.autoplay) {
    show(&currentTrack);
  }

  /* Start audio system */

  theFrontend = FrontEnd::getInstance();
  theRecognizer = Recognizer::getInstance();
  thePlayer = MediaPlayer::getInstance();
  theMixer  = OutputMixer::getInstance();

  thePlayer->begin();
  theFrontend->begin();
  theRecognizer->begin(attention_cb);

  theMixer->activateBaseband();

  /* Create Objects */

  thePlayer->create(MediaPlayer::Player0, attention_cb);

  theMixer->create(attention_cb);

  /* Set rendering clock */

  theMixer->setRenderingClkMode(OUTPUTMIXER_RNDCLK_NORMAL);

  theFrontend->activate(frontend_done_callback);
  theRecognizer->activate(recognizer_done_callback);

//  CXD56_AUDIO_ECODE error_code = cxd56_audio_set_spdriver(CXD56_AUDIO_SP_DRV_4DRIVER);

  usleep(100 * 1000);
  /* Set main volume */
  theMixer->setVolume(preset.volume, 0, 0);

  theFrontend->init(recognizer_channel_number, recognizer_bit_length, 384,
                    ObjectConnector::ConnectToRecognizer, AsMicFrontendPreProcSrc, "/mnt/sd0/BIN/SRC");
//                    ObjectConnector::ConnectToRecognizer, AsMicFrontendPreProcSrc, "/mnt/spif/BIN/SRC");

  theRecognizer->init(recognizer_find_callback,
                      "/mnt/sd0/BIN/RCGPROC");
//                      "/mnt/spif/BIN/RCGPROC");

  /* Init Recognizer DSP */
  struct InitRcgProc : public CustomprocCommand::CmdBase
  {
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint32_t param4;
  };

  static InitRcgProc s_initrcgproc;
  s_initrcgproc.param1 = 500;
  s_initrcgproc.param2 = 600;
  s_initrcgproc.param3 = 0;
  s_initrcgproc.param4 = 0;

  theRecognizer->initRecognizerDsp((uint8_t *)&s_initrcgproc, sizeof(s_initrcgproc));

  /* Set Recognizer DSP */
  
  struct SetRcgProc : public CustomprocCommand::CmdBase
  {
    uint32_t param1;
    int32_t param2;
    int32_t param3;
    int32_t param4;
  };

  static SetRcgProc s_setrcgproc;
  s_setrcgproc.param1 = true;
  s_setrcgproc.param2 = -1;
  s_setrcgproc.param3 = -1;
  s_setrcgproc.param4 = -1;

  theRecognizer->setRecognizerDsp((uint8_t *)&s_setrcgproc, sizeof(s_setrcgproc));

  /* Start Recognizer */

 ledOn(LED0);

  theFrontend->start();
  theRecognizer->start();
  puts("Recognizing Start!");

}

void loop()
{
  static enum State {
    Stopped,
    Ready,
    Active
  } s_state = preset.autoplay ? Ready : Stopped;
  err_t err = MEDIAPLAYER_ECODE_OK;

  /* Fatal error */
  if (ErrEnd) {
    printf("Error End\n");
    goto stop_player;
  }

  /* Menu operation */

    switch (g_event_char) {
    case 'a': // autoplay
      if (preset.autoplay) {
        preset.autoplay = 0;
      } else {
        preset.autoplay = 1;
      }
      printf("Auto=%s\n", (preset.autoplay) ? "On" : "Off");
      EEPROM.put(eeprom_idx, preset);
      break;
    case 'p': // play
      if (s_state == Stopped) {
        s_state = Ready;
        show(&currentTrack);
      }
      g_event_char ='*';
      break;
    case 's': // stop
      theFrontend->stop();
      theRecognizer->stop();
      if (s_state == Active) {
        stop();
      }
      s_state = Stopped;
      puts("again");
      
  usleep(100*1000);

      theRecognizer->start();
      theFrontend->start();
      g_event_char ='*';
      puts("again!!");
//          s_state = Ready;
      break;
    case '+': // volume up
      preset.volume += 10;
      if (preset.volume > 120) {
        /* set max volume */
        preset.volume = 120;
      }
      printf("Volume=%d\n", preset.volume);
      theMixer->setVolume(preset.volume, 0, 0);
      EEPROM.put(eeprom_idx, preset);
      break;
    case '-': // volume down
      preset.volume -= 10;
      if (preset.volume < -1020) {
        /* set min volume */
        preset.volume = -1020;
      }
      printf("Volume=%d\n", preset.volume);
      theMixer->setVolume(preset.volume, 0, 0);
      EEPROM.put(eeprom_idx, preset);
      break;
    case 'l': // list
      if (preset.repeat) {
        thePlaylist.setRepeatMode(Playlist::RepeatModeOff);
        list();
        thePlaylist.setRepeatMode(Playlist::RepeatModeOn);
      } else {
        list();
      }
      break;
    case 'n': // next
      theFrontend->stop();
      theRecognizer->stop();
      if (s_state == Ready) {
        // do nothing
      } else { // s_state == Active or Stopped
        if (s_state == Active) {
          stop();
          s_state = Ready;
        }
        if (!next()) {
          s_state = Stopped;
        }
      }
      theFrontend->start();
      theRecognizer->start();
      g_event_char ='*';
      break;
    case 'b': // back
      theFrontend->stop();
      theRecognizer->stop();

      if (s_state == Active) {
        stop();
        s_state = Ready;
      }
      prev();
      g_event_char ='*';
      theRecognizer->start();
      theFrontend->start();
      break;
    case 'r': // repeat
      if (preset.repeat) {
        preset.repeat = 0;
        thePlaylist.setRepeatMode(Playlist::RepeatModeOff);
      } else {
        preset.repeat = 1;
        thePlaylist.setRepeatMode(Playlist::RepeatModeOn);
      }
      printf("Repeat=%s\n", (preset.repeat) ? "On" : "Off");
      EEPROM.put(eeprom_idx, preset);
      break;
    case 'R': // random
      if (preset.random) {
        preset.random = 0;
        thePlaylist.setPlayMode(Playlist::PlayModeNormal);
      } else {
        preset.random = 1;
        thePlaylist.setPlayMode(Playlist::PlayModeShuffle);
      }
      printf("Random=%s\n", (preset.random) ? "On" : "Off");
      EEPROM.put(eeprom_idx, preset);
      break;
    case 'm':
    case 'h':
    case '?':
      menu();
      break;
    default:
      break;
    }

  /* Processing in accordance with the state */

  switch (s_state) {
  case Stopped:
    break;

  case Ready:
       puts("a!!");
    theFrontend->stop();
    theRecognizer->stop();
    if (start()) {
      s_state = Active;
    } else {
      goto stop_player;
    }
    theRecognizer->start();
    theFrontend->start();
    break;

  case Active:
  
    /* Send new frames to be decoded until end of file */
    err = thePlayer->writeFrames(MediaPlayer::Player0, myFile);

    if (err == MEDIAPLAYER_ECODE_FILEEND) {
      /* Stop player after playback until end of file */
      theRecognizer->stop();
      theFrontend->stop();
      puts("here?");

      thePlayer->stop(MediaPlayer::Player0,AS_STOPPLAYER_ESEND);
      myFile.close();
      sleep(1);
      if (next()) {
        s_state = Ready;
      } else {
        /* If no next track, stop the player */
        s_state = Stopped;
      }
      theRecognizer->start();
      theFrontend->start();
    } else if (err != MEDIAPLAYER_ECODE_OK) {
      printf("Main player error code: %d\n", err);
      goto stop_player;
    }
    break;

  default:
    break;
  }

  /* This sleep is adjusted by the time to read the audio stream file.
     Please adjust in according with the processing contents
     being processed at the same time by Application.
   */

  usleep(1000);

  return;

stop_player:
  printf("Exit player\n");
  myFile.close();
  exit(1);
}
