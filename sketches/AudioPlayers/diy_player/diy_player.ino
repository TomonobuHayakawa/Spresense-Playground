/*
 *  diy_player.ino - Sound player on eMMC with control by sensing
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <eMMC.h>

#include <Audio.h>

#include <Wire.h>
#include <KX122.h>
KX122 kx122(KX122_DEVICE_ADDRESS_1F);

AudioClass *theAudio;
File theFile;

/**
 *  @brief Setup audio player
 *
 *  Set output device to speaker <br>
 *  Set main player to decode stereo WAV. Stream sample rate is set to 192kHz <br>
 *  Therefore, to play 192kHz audio, set HiReso mode.
 *  System directory "/mnt/vfat/BIN" will be searched for WAV decoder (WAVDEC file) <br>
 *  This is the /BIN directory on the eMMC. <br>
 *  Volume is set to -24.0 dB
 */
void setup()
{
  /* Init kx122 */

  Wire.begin();
  int rc = kx122.init();

  /* Init eMMC */

  eMMC.begin();

  /* start audio system */

  theAudio = AudioClass::getInstance();

  theAudio->begin();
  puts("initialization Audio Library");

  /* Set audio clock mode to Hi-Reso mode, to play 192kHz audio */

  theAudio->setRenderingClockMode(AS_CLKMODE_HIRES);

  /* Transition to player mode */

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP);

  /* Init player WAV/192kHz/Stereo, WAV decoder file should be placed at eMMC */

  theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_WAV, "/mnt/vfat/BIN", AS_SAMPLINGRATE_192000,AS_CHANNEL_STEREO);

  /* Set volume -24dB */

  theAudio->setVolume(-240, 0, 0);

}

/**
 *  @brief Open audio files and start playing 
 *
 *  Open two stream files "track0.wav" and "track1.wav" <br>
 *  These should be in the root directory of the eMMC. <br>
 */
static int start_play()
{
  static int track_no = 1;

  if (track_no == 1)
    {
      theFile = eMMC.open("track1.wav");
      track_no = 0;
    }
  else
    {
      theFile = eMMC.open("track0.wav");
      track_no = 1;    
    }

  /* Verify file open */

  if (!theFile)
    {
      printf("File open error\n");
      exit(1);
    }

  /* Supply audio data in some degree before start playing */

  int err = theAudio->writeFrames(AudioClass::Player0, theFile);

  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("File Read Error! =%d\n",err);
      theFile.close();
      exit(1);
    }
    
  /* Start playing */

  theAudio->startPlayer(AudioClass::Player0);
  puts("Play!");

  ledOn(LED0);
}

/**
 *  @brief Stop playing and check restart 
 *
 *  Stop player and check sensor to determine restart playing or not.
 */
static bool stop_play()
{
  theAudio->stopPlayer(AudioClass::Player0);
  sleep(100000);
  ledOff(LED0);

  /* Restart? */

  for (int i = 0; i < 100; i++)
    {
      if (check_sense())
        {
          return true;
        }
    }

  return false;
}

/**
 *  @brief Check kx122 sensor value 
 *
 *  Get acceleration value and calc sum of vector.
 *  Return true when vector exceeds threshold.
 */
bool check_sense()
{
  byte rc;
  float acc[3];

  rc = kx122.get_val(acc);
  if (rc != 0)
    {
      return false;
    }
//  printf("KX122 (X) = %1.5f\n",acc[0]);
//  printf("KX122 (Y) = %1.5f\n",acc[1]);
//  printf("KX122 (Z) = %1.5f\n",acc[2]);
    
  float power = acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2];
//  float power = pow(acc[0],2)+pow(acc[1],2)+pow(acc[2],2);
  printf("power= %1.5f\n",power);

  if (power > 3)
    {
      return true;
    }
  else
    {
      return false;
    }
}

/**
 *  @brief Play streams
 *
 * Send new frames to decode in a loop until file ends
 * Check sensor and play next track or not.
 */
void loop()
{
  puts("loop!!");

  static int state = 0;

  bool sense = check_sense();
  if (sense)
    {
      if (state == 0)
        {
          start_play();
          state = 1;
        }
      else
        {
          if (stop_play())
            {
              start_play();
            }
          else
            {
              state = 0;
            }
        }
    }

  /* Supply audio data */

  if (state == 1)
    {
      int err = theAudio->writeFrames(AudioClass::Player0, theFile);

      if (err == AUDIOLIB_ECODE_FILEEND)
        {
          printf("File End! =%d\n",err);
          sleep(1);
          theAudio->stopPlayer(AudioClass::Player0);

          /* When file end, do next play */

          start_play();
        }
    }
  
  usleep(20000);
}

