/*
 *  dual_players_with_sensing.ino - Dual source sound player with sensing example application
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

#include <Audio.h>
#include <SD.h>

#include <Wire.h>
#include <KX122.h>

KX122 kx122(KX122_DEVICE_ADDRESS_1F);

AudioClass *theAudio;

File mainFile, subFile;

#define MAIN_FILE "ayaka.mp3"
#define SUB_FILE  "chicago.mp3"

/**
 *  @brief Open play file and start playing
 *
 *  Open two stream files "Sound0.mp3" and "Sound1.mp3". <br>
 *  Volume is set to -8.0 dB.
 */
static void start_players()
{
  /* Open play files */

//  mainFile = SD.open(MAIN_FILE);
//  subFile =  SD.open(SUB_FILE);
 mainFile = SD.open("ayaka.mp3");
 subFile =  SD.open("chicago.mp3");

  printf("Open! %d\n", mainFile);
  printf("Open! %d\n", subFile);

  /* Supply audio data in some degree before start playing */

 /* Send first frames to be decoded */
  err_t err = theAudio->writeFrames(AudioClass::Player0,mainFile);
  if (err)
    {
      printf("Main player: File Read Error! =%d\n",err);
      mainFile.close();
      exit(1);
    }


/*  err = theAudio->writeFrames(AudioClass::Player1,subFile);
  if (err)
    {
      printf("Sub player: File Read Error! =%d\n",err);
      subFile.close();
      exit(1);
    }*/

//while(1);

  puts("Play!");

  /* Set initial volume -8dB */

  theAudio->setVolume(-80, -80, -80);

  /* Start main/sub player */

  theAudio->startPlayer(AudioClass::Player0);
//  theAudio->startPlayer(AudioClass::Player1);

  puts("start!");  
}

/**
 *  @brief Setup main audio player and sub audio player
 *
 *  Set output device to speaker <br>
 *  Set main player to decode stereo mp3. Stream sample rate is set to 48000kHz. <br>
 *  System directory "/mnt/sd0/BIN" will be searched for MP3 decoder (MP3DEC file) <br>
 *  This is the /BIN directory on the SD card. <br>
 */
void setup()
{

//  SD.begin();
  while (!SD.begin()) {
    ; /* wait until SD card is mounted. */
  }

  /* start Sensing. */

  Wire.begin();
  kx122.init(); //TODO: Error Handling.

  /* start audio system */

  theAudio = AudioClass::getInstance();

  puts("initialization Audio Library");
  theAudio->begin();

  /* Transition to Player mode */
  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP);

  /*
   * Set main player to decode stereo mp3. Stream sample rate is set to 48000kHz
   * Search for MP3 decoder in "/mnt/sd0/BIN" directory
   */

//  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, AS_SAMPLINGRATE_48000, AS_CHANNEL_STEREO);
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
  {
    printf("Player0 initialize error\n");
    exit(1);
  }

  /*
   * Set sub player to decode stereo mp3. Stream sample rate is set to 48000kHz
   * Search for MP3 decoder in "/mnt/sd0/BIN" directory
   */

//  err = theAudio->initPlayer(AudioClass::Player1, AS_CODECTYPE_MP3, AS_SAMPLINGRATE_48000, AS_CHANNEL_STEREO);
  err = theAudio->initPlayer(AudioClass::Player1, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
  {
    printf("Player1 initialize error\n");
    exit(1);
  }


  /* Start play */

  start_players();
}

/**
 *  @brief Play streams
 *
 * Send new frames to decode in a loop until file ends.
 * Change mixing ratio by gradient of board.
 */
void loop()
{
  /* Put your main code here, to run repeatedly */

  puts("loop!!");

  /* for examle. Chack Bottom... */

//int err = 0 ;
  int err = theAudio->writeFrames(AudioClass::Player0, mainFile);
//  theAudio->writeFrames(AudioClass::Player1, subFile);

  puts("loop2!!");
 /* Stop from 1st player. */

  if (err != 0)
    {
      printf("File End! =%d\n", err);
      sleep(1);
      theAudio->stopPlayer(AudioClass::Player0);
      theAudio->stopPlayer(AudioClass::Player1);
      mainFile.close();
      subFile.close();
      start_players();
    }

  byte rc;
  float acc[3];

  /* Change mixing ratio from acc */

  rc = kx122.get_val(acc);

  if (rc == 0)
    {
      printf("KX122 (X) = %1.5f [g]\n", acc[0]);
      printf("KX122 (Y) = %1.5f [g]\n", acc[1]);
      printf("KX122 (Z) = %1.5f [g]\n", acc[2]);
    }

  int main_db = -80 + acc[0] * 80;
  printf("main db = %d \n", main_db);

  int sub_db = -80 - acc[0] * 80;
  printf("sub db = %d \n", sub_db);

  theAudio->setVolume(-80, main_db, sub_db);
}
