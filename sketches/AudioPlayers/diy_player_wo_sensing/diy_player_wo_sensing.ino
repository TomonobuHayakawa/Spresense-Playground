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

#include <SD.h>

#include <Audio.h>

#include <Wire.h>

AudioClass *theAudio;
File theFile;

/*-------------------------------------------------------------*/
static int start_play()
{
  static int track_no = 1;
  if(track_no == 1){
    theFile = SD.open("track1.wav");
    track_no = 0;
  }else {
    theFile = SD.open("track0.wav");
    track_no = 1;    
  }

  /* Verify file open */
  if (!theFile)
    {
      printf("File open error\n");
      exit(1);
    }

  int err = theAudio->writeFrames(AudioClass::Player0, theFile);

  if (err != AUDIOLIB_ECODE_OK)
  {
    printf("File Read Error! =%d\n",err);
    theFile.close();
    exit(1);
  }
    
  theAudio->startPlayer(AudioClass::Player0);
  puts("Play!");
  ledOn(LED0);
}

/***----------------------------------------------***/
void setup()
{

  SD.begin();

  // start audio system
  theAudio = AudioClass::getInstance();

  theAudio->begin();
  puts("initialization Audio Library");

  theAudio->setRenderingClockMode(AS_CLKMODE_HIRES);
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP,AS_SP_DRV_MODE_4DRIVER);
  theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_WAV, "/mnt/sd0/BIN", AS_SAMPLINGRATE_192000,AS_BITLENGTH_24,AS_CHANNEL_STEREO);
  theAudio->setVolume(-10, 0, 0);

  start_play();

}

/*-------------------------------------------------------------*/
static bool stop_play()
{
  theAudio->stopPlayer(AudioClass::Player0);
  usleep(100000);
  ledOff(LED0);

  /* Restart? */
  return true;
}

/*-------------------------------------------------------------*/
void loop()
{
  puts("loop!!");

  static int state = 0;

  int err = theAudio->writeFrames(AudioClass::Player0, theFile);

  if (err == AUDIOLIB_ECODE_FILEEND)
    {
      printf("File End! =%d\n",err);
      sleep(1);
      theAudio->stopPlayer(AudioClass::Player0);
      /* Do next play */
      start_play();
    }
    
  usleep(20000);

}
