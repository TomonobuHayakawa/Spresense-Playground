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
#include <KX122.h>

KX122 kx122(KX122_DEVICE_ADDRESS_1F);

AudioClass *theAudio;

File mainFile, subFile;

#define MAIN_FILE "sound0.mp3"
#define SUB_FILE  "sound1.mp3"

/***----------------------------------------------***/
static void start_players()
{
  mainFile = SD.open(MAIN_FILE);
  subFile =  SD.open(SUB_FILE);

  printf("Open! %d\n", mainFile);
  printf("Open! %d\n", subFile);

  int err = theAudio->writeFrames(AudioClass::Player0, mainFile);
  err = theAudio->writeFrames(AudioClass::Player1, subFile);

  if (err != 0)
  {
    printf("File Read Error! =%d\n", err);
  }

  puts("Play!");

  theAudio->setVolume(-80, -80, -80);

  theAudio->startPlayer(AudioClass::Player0);
  theAudio->startPlayer(AudioClass::Player1);

  puts("start!");  
}

/***----------------------------------------------***/
void setup()
{

  SD.begin();

  // start Sensing.
  Wire.begin();
  kx122.init(); //TODO: Error Handling.

  // start audio system
  theAudio = AudioClass::getInstance();

  puts("initialization Audio Library");
  theAudio->begin();

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP);
  theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, AS_SAMPLINGRATE_48000, AS_CHANNEL_STEREO);
  theAudio->initPlayer(AudioClass::Player1, AS_CODECTYPE_MP3, AS_SAMPLINGRATE_48000, AS_CHANNEL_STEREO);

  start_players();
}

/***----------------------------------------------***/
void loop()
{

  // put your main code here, to run repeatedly:

  puts("loop!!");
  // for examle. Chack Bottom...

  int err = theAudio->writeFrames(AudioClass::Player0, mainFile);
  theAudio->writeFrames(AudioClass::Player1, subFile);

 // Stop from 1st player.
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

  if (rc == 0) {
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
