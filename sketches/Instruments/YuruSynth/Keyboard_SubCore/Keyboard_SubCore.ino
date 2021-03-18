/*
 *  Keyboard_SubCore.ino - Keyboard sketch for SubCore
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

#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

#include <SoftwareSerial.h>
SoftwareSerial mySerial(21, 20); // RX, TX

#include <MP.h>

const unsigned int number_of_keyinfo = 7;
const unsigned char sync_word = 0xff;

void setup()
{
  Serial.begin(115200);
  mySerial.begin(20000);

  MP.begin();
  MP.RecvTimeout(MP_RECV_POLLING);

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
    while (!mySerial.available()) { ledOn(LED1); }
    keyinfo[i] = mySerial.read();
  }

  // check sync word
  if(keyinfo[number_of_keyinfo-1] != sync_word){
    return;
  }

  int8_t        msgid;
  unsigned char *data;

  int ret = MP.Recv(&msgid, &data);
  if (ret < 0) { return; }

  memcpy(data,keyinfo,number_of_keyinfo-1);

  ret = MP.Send(msgid, data, 0);
  if (ret < 0) { return; }

}
