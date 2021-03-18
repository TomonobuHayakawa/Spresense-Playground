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

#define USE_KEYBOARD

#ifdef USE_KEYBOARD
#include <MP.h>
#endif /* USE_KEYBOARD */

AudioClass *theAudio;

const int keyboard_core = 1;
const unsigned int number_of_keyinfo = 6;
const unsigned int number_of_keybits = 5;
unsigned char keyinfo[number_of_keyinfo];


const uint8_t  lower_limit_tempo = 40;
const uint8_t  higher_limit_tempo = 205;

uint32_t high_fs = 264; // Hz
uint32_t low_fs = 162; // Hz
uint8_t  length = 10; // ms
uint8_t  beat = 4;
uint8_t  tempo = 100;
int32_t  volume = -40;
uint32_t interval;

static bool timer_flg = false;

unsigned int timer_isr(void) {
  timer_flg = true;
  return ((60 * 1000 * 1000) / tempo);
}

void setup()
{
  Serial.begin(115200);

#ifdef USE_KEYBOARD
  int ret = MP.begin(keyboard_core);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }
  MP.RecvTimeout(10);
#endif /* USE_KEYBOARD */

  theAudio = AudioClass::getInstance();
  theAudio->begin();
  puts("initialization Audio Library");

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, 0, 0);

  puts("Start Metronpome!");

  printf("Now tempo is : %d\nPlease input tempo!\n",tempo);

  attachTimerInterrupt(timer_isr, (60 * 1000 * 1000) / tempo);

}

void beep()
{
  static int i = 1;

  if (i >= beat) {
    theAudio->setBeep(1,volume,high_fs);
    i=1;
  } else {
    theAudio->setBeep(1,volume,low_fs);
    i++;
  }

  usleep(length * 1000);
  theAudio->setBeep(0,0,0);

}

void key_proc()
{
  // Set tempo
  if(tempo != keyinfo[5]+lower_limit_tempo+10){
    tempo = keyinfo[5]+lower_limit_tempo+10;
    printf("Tempo =%d\n",tempo);
  }

  // Set Beat
  for(int i=0; i<number_of_keybits; i++)
  {
    if(keyinfo[2] & (0x01 << i)){
      beat = i+2;
      printf("Beat =%d\n",beat);
      break;
    }
  }
}

void loop()
{
  if(timer_flg){
    beep();
    timer_flg = false;
  }

#ifdef USE_KEYBOARD
  int8_t   sndid = 100;
  int8_t   rcvid = 0;
  void*    tmp;

  int ret = MP.Send(sndid, &keyinfo, keyboard_core);
  if (ret < 0) {
    puts("Keyboad Error!");
  	return;
  }

  ret = MP.Recv(&rcvid, &tmp, keyboard_core);
  if (ret < 0) {
    puts("Keyboad Deadlock!");
  	return;
  }

#if 0 
// print keyinfo
  for(int i=0;i<number_of_keyinfo;i++) {
    printf("%x ",keyinfo[i]);
  }
  putchar('\n');
#endif

  key_proc();

#else   /* !USE_KEYBOARD */
  if (Serial.available() > 0)
    {
       String string = Serial.readStringUntil('\n');
       long tmp = string.toInt();
       if( (lower_limit_tempo <= tmp) && (tmp <= higher_limit_tempo) ){
         tempo = (0xff & tmp);
         printf("Now tempo is : %d\nPlease input tempo!\n",tempo);
       }
    }
#endif /* USE_KEYBOARD */

}
