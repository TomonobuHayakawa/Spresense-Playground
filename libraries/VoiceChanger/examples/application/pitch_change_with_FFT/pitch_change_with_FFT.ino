/*
 *  recording_player.ino - Object I/F based recording and play example application
 *  Copyright 2018 Sony Semiconductor Solutions Corporation
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,  MA 02110-1301  USA
 */

#include <SD.h>
#include <MediaRecorder.h>
#include <MediaPlayer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>

//SDClass theSD;

/*-----------------------------------------------------------------*/
/*
 * FFT parameters
 */
#include "changer_rpc.h"

#define SAMPLING_RATE   48000 // ex.) 48000, 16000

#define CHANGER_LIBFILE "/mnt/sd0/BIN/VOICHG"
#define FFT_LENGTH   512 // ex.) 128, 256, 1024
#define FFT_SHIFT    0  // ex.) 0, 128, 256


/*-----------------------------------------------------------------*/
#include <Wire.h>
#include <KX122.h>

KX122 kx122(KX122_DEVICE_ADDRESS_1F);

/*-----------------------------------------------------------------*/
MediaRecorder *theRecorder;
MediaPlayer *thePlayer;
OutputMixer *theMixer;

const int32_t sc_buffer_size = 6144;
uint8_t s_buffer[sc_buffer_size];

bool ErrEnd = false;

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
 * @brief Recorder done callback procedure
 *
 * @param [in] event        AsRecorderEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */

static bool mediarecorder_done_callback(AsRecorderEvent event, uint32_t result, uint32_t sub_result)
{
  return true;
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
  /* If result of "Play", restart recording (It should been stopped when "Play" requested) */

  if (event == AsPlayerEventPlay)
    {
      theRecorder->start();
    }

  return true;
}

/**
 * @brief Player decode callback procedure
 *
 * @param [in] pcm_param    AsPcmDataParam type
 */
void mediaplayer_decode_callback(AsPcmDataParam pcm_param)
{
  /* You can imprement any audio signal process */

  /* Output and sound audio data */

  theMixer->sendData(OutputMixer0,
                     outmixer_send_callback,
                     pcm_param);
}

/**
 * @brief Setup Recorder
 *
 * Set input device to Mic <br>
 * Initialize recorder to encode stereo wav stream with 48kHz sample rate <br>
 * System directory "/mnt/sd0/BIN" will be searched for SRC filter (SRC file)
 * Open "Sound.wav" file <br>
 */

void setup()
{
  SD.begin();

  /* start Sensing. */

  Wire.begin();
  kx122.init(); //TODO: Error Handling.

  /* Init FFT parameter for input data */
  load_library(CHANGER_LIBFILE);
  changer_init(FFT_LENGTH, 0, TYPE_SHORT);

  /* Set FFT window */
  fft_window(FFT_WIN_HAMMING);

  /* Initialize memory pools and message libs */

  initMemoryPools();
  createStaticPools(MEM_LAYOUT_RECORDINGPLAYER);

  /* start audio system */

  theRecorder = MediaRecorder::getInstance();
  thePlayer = MediaPlayer::getInstance();
  theMixer  = OutputMixer::getInstance();

  theRecorder->begin();
  thePlayer->begin();

  puts("initialization MediaRecorder, MediaPlayer and OutputMixer");

  theMixer->activateBaseband();

  /* Create Objects */

  thePlayer->create(MediaPlayer::Player0, attention_cb);
  theMixer->create(attention_cb);

  /* Activate Objects. Set output device to Speakers/Headphones */

  theRecorder->activate(AS_SETRECDR_STS_INPUTDEVICE_MIC, mediarecorder_done_callback);
  thePlayer->activate(MediaPlayer::Player0, AS_SETPLAYER_OUTPUTDEVICE_SPHP, mediaplayer_done_callback);
  theMixer->activate(OutputMixer0, outputmixer_done_callback);

  usleep(100 * 1000);

  /*
   * Initialize recorder to decode stereo wav stream with 48kHz sample rate
   * Search for SRC filter in "/mnt/sd0/BIN" directory
   */

  theRecorder->init(AS_CODECTYPE_LPCM, AS_CHANNEL_MONO, AS_SAMPLINGRATE_48000, AS_BITLENGTH_16, 0);
  puts("recorder init");
  thePlayer->init(MediaPlayer::Player0, AS_CODECTYPE_WAV, AS_SAMPLINGRATE_48000, AS_CHANNEL_MONO);
  puts("player init");

  /* Start Recorder */

  theMixer->setVolume(0, 0, 0);
}

/**
 * @brief Record audio frames
 */

static const int StateReady = 0;
static const int StatePrepare = 1;
static const int StateRun = 2;

void loop()
{
  static int s_cnt = 0;
  static int s_state = StateReady;
  static int s_pitch = 5;

  float acc[3];
  byte rc = kx122.get_val(acc);

  if (rc == 0)
    {
/*      printf("KX122 (X) = %1.5f [g]\n", acc[0]);
      printf("KX122 (Y) = %1.5f [g]\n", acc[1]);
      printf("KX122 (Z) = %1.5f [g]\n", acc[2]);*/
      s_pitch = (int8_t)5*acc[0];
    }


  if (s_state == StateReady)
    {
      /* Start recording */

      theRecorder->start();
      puts("recorder start");

      s_state = StatePrepare;
    }
  else if (s_state == StatePrepare)
    {
      /* Prestore recorded audio data */

      uint32_t read_size = 0;
      static uint32_t read_size_sum = 0;

      theRecorder->readFrames(s_buffer, sc_buffer_size, &read_size);

      if (read_size > 0)
        {
          changer_input(s_buffer, read_size, s_pitch);

//          int16_t result_p;
            uint32_t result_p;
        while(changer_output(1, &result_p) > 0)
          {
            thePlayer->writeFrames(MediaPlayer::Player0, (uint8_t*)result_p, FFT_LENGTH*sizeof(int16_t));
            read_size_sum += FFT_LENGTH*sizeof(int16_t);
          }
        }

      if (read_size_sum > 10000)
        {
          /* Stop recorder to avoid time lag between record and play will be get long */

          theRecorder->stop();

          thePlayer->start(MediaPlayer::Player0, mediaplayer_decode_callback);
          puts("player start");

          s_state = StateRun;
        }
    }
  else if (s_state == StateRun)
    {
      /* Get recorded data and play */

      uint32_t read_size = 0;
    
      do
        {

          theRecorder->readFrames(s_buffer, sc_buffer_size, &read_size);

          if (read_size > 0)
            {
              changer_input(s_buffer, read_size, s_pitch);
              uint32_t result_p;
              while(changer_output(1, &result_p) > 0)
              {
                thePlayer->writeFrames(MediaPlayer::Player0, (uint8_t*)result_p, FFT_LENGTH*sizeof(int16_t));
              }
            }
        }
      while (read_size != 0);

      if (s_cnt++ > 40000)
        {
          goto exitRecording;
        }
    }
  else
    {
    }

  if (ErrEnd)
    {
      printf("Error End\n");
      goto exitRecording;
    }

  usleep(1);

  return;

exitRecording:

  theRecorder->stop();
  thePlayer->stop(MediaPlayer::Player0);

  puts("Exit.");

  exit(1);
}
