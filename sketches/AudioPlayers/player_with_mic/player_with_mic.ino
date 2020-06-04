/*
 *  player_with_mic.ino - Simple sound player with mic-in example application
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

#include <Audio.h>
#include <SDHCI.h>
#include <arch/chip/cxd56_audio.h>  /* For mic-in */

AudioClass *theAudio;
File myFile;
SDClass theSD;

bool ErrEnd = false;

/**
 * @brief Audio attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      ErrEnd = true;
   }
}

/**
 * @brief Microphone settings
 *
 * Output microphone input to speaker.
 */

static bool set_mic_to_speaker(int gain = 0)
{
  CXD56_AUDIO_ECODE err;

  /* Set mic input */

  err = cxd56_audio_en_input();

  if (err != CXD56_AUDIO_ECODE_OK)
    {
      print_err("ERROR: cxd56_audio_en_input() code=%d\n", err);
      return false;
    }

  /* Set gsin */

  cxd56_audio_mic_gain_t  mic_gain;

  mic_gain.gain[0] = gain;
  mic_gain.gain[1] = gain;
  mic_gain.gain[2] = gain;
  mic_gain.gain[3] = gain;
  mic_gain.gain[4] = 0;
  mic_gain.gain[5] = 0;
  mic_gain.gain[6] = 0;
  mic_gain.gain[7] = 0;

  err = cxd56_audio_set_micgain(&mic_gain);

  if (err != CXD56_AUDIO_ECODE_OK)
    {
      print_err("ERROR: cxd56_audio_set_micgain() code=%d\n", err);
      return false;
    }

  /* Set through path */

  cxd56_audio_signal_t sig_id;
  cxd56_audio_sel_t    sel_info;

  sig_id = CXD56_AUDIO_SIG_MIC1;

  sel_info.au_dat_sel1 = false;
  sel_info.au_dat_sel2 = true;
  sel_info.cod_insel2  = false;
  sel_info.cod_insel3  = true;
  sel_info.src1in_sel  = false;
  sel_info.src2in_sel  = false;

  err = cxd56_audio_set_datapath(sig_id, sel_info);

  if (err != CXD56_AUDIO_ECODE_OK)
    {
      print_err("ERROR: cxd56_audio_set_datapath() code=%d\n", err);
      return false;
    }

  return true;
}

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
  theAudio = AudioClass::getInstance();
  theAudio->begin(audio_attention_cb);

  puts("initialization Audio Library");

  /* Set clock mode to normal */
  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);

  /* Set output device to speaker with first argument.
   * If you want to change the output device to I2S,
   * specify "AS_SETPLAYER_OUTPUTDEVICE_I2SOUTPUT" as an argument.
   * Set speaker driver mode to LineOut with second argument.
   * If you want to change the speaker driver mode to other,
   * specify "AS_SP_DRV_MODE_1DRIVER" or "AS_SP_DRV_MODE_2DRIVER" or "AS_SP_DRV_MODE_4DRIVER"
   * as an argument.
   */
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);

  /* Microphone settings */

  set_mic_to_speaker(160);

  /*
   * Set main player to decode stereo mp3. Stream sample rate is set to "auto detect"
   * Search for MP3 decoder in "/mnt/sd0/BIN" directory
   */
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Player0 initialize error\n");
      exit(1);
    }

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

  /* Send first frames to be decoded */
  err = theAudio->writeFrames(AudioClass::Player0, myFile);

  if ((err != AUDIOLIB_ECODE_OK) && (err != AUDIOLIB_ECODE_FILEEND))
    {
      printf("File Read Error! =%d\n",err);
      myFile.close();
      exit(1);
    }

  puts("Play!");

  /* Main volume set to -16.0 dB */
  theAudio->setVolume(-240);
  theAudio->startPlayer(AudioClass::Player0);
}

/**
 * @brief Play stream
 *
 * Send new frames to decode in a loop until file ends
 */
void loop()
{
  puts("loop!!");

  /* Send new frames to decode in a loop until file ends */
  int err = theAudio->writeFrames(AudioClass::Player0, myFile);

  /*  Tell when player file ends */
  if (err == AUDIOLIB_ECODE_FILEEND)
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
  theAudio->stopPlayer(AudioClass::Player0);
  myFile.close();
  exit(1);
}
