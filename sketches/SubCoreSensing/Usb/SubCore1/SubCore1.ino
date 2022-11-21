/*
 *  Sub1.ino - Subcore Sensing Example on SubCore
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

#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

#include <MP.h>

#include <Usb.h>


USB     Usb;

void setup()
{
  int ret = 0;

  ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }

  Serial.begin( 115200 );
  while (!Serial);

  Serial.println("Start SubCore1");

  if (Usb.Init() == -1)
    {
      Serial.println("OSC did not start.");
    }

  Serial.println("Usb.Init() done");

  delay( 200 );

}

uint32_t read_sensor()
{

}

void loop()
{
  int      ret;
  int8_t   msgid = 10;
  uint32_t msgdata;

  msgdata = read_sensor();

  ret = MP.Send(msgid, msgdata);
  if (ret < 0) {
    errorLoop(4);
  }

  Usb.Task();

}

void errorLoop(int num)
{
  int i;

  while (1) {
    for (i = 0; i < num; i++) {
      ledOn(LED0);
      delay(300);
      ledOff(LED0);
      delay(300);
    }
    delay(1000);
  }
}
