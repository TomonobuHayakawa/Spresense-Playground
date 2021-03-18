/*
 *  Chocoboard.ino - The sample controller of the keyboard like chocolate
 *  Copyright 2021 Sony Semiconductor Solutions Corporation
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

#include <SoftwareSerial.h>
SoftwareSerial mySerial(21, 20); // RX, TX

const unsigned int number_of_keyinfo = 7;
const unsigned char sync_word = 0xff;

void setup()
{
  Serial.begin(115200);
  mySerial.begin(20000);
  
  sleep(1);
}

void search_sync()
{
  while (1) {
    ledOn(LED1);
    if(mySerial.available()) {
      if (mySerial.read() == sync_word) {
        ledOff(LED1);
        return;
      }
    }
  }
}

void loop()
{
  unsigned char keyinfo[number_of_keyinfo];

  // search sync word
  search_sync();

  // read keyinfo
  for(int i=0;i<number_of_keyinfo;i++) {
    ledOn(LED0);

    while (!mySerial.available()) { ledOn(LED1); }
    ledOff(LED1);
    keyinfo[i] = mySerial.read();
  }
  ledOff(LED1);

  // check sync word
  if(keyinfo[number_of_keyinfo-1] != sync_word){
    ledOn(LED3);
    return;
  }

  // print keyinfo
  for(int i=0;i<number_of_keyinfo;i++) {
    printf("%x ",keyinfo[i]);
  }
  puts("\n");
  ledOff(LED3);

  usleep(10*1000);

}
