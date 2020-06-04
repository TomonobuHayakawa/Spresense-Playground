/*
 *  Metronome.ino - Metronome with beep
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

AudioClass *theAudio;

uint32_t fs = 162; // Hz
uint8_t  length = 5; // ms
uint8_t  tempo = 100;
uint32_t interval;

void setup()
{
  Serial.begin(115200);

  theAudio = AudioClass::getInstance();
  theAudio->begin();
  puts("initialization Audio Library");

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, 0, 0);
  
  puts("Start Metronpome!");

  printf("Now tempo is : %d\tPlease input tempo!\n",tempo);
  
}

void loop()
{
  if (Serial.available() > 0)
    {
       String string = Serial.readStringUntil('\n');
       long tmp = string.toInt();
       if( (40 <= tmp) && (tmp <=205) ){
         tempo = (0xff & tmp);
         printf("Now tempo is : %d\tPlease input tempo!\n",tempo);
       }
    }

  interval = ((60 * 1000 * 1000) / tempo) - (length * 1000);

  theAudio->setBeep(1,-40,fs);
  usleep(length * 1000);
  theAudio->setBeep(0,0,0);
  usleep(interval);

}
