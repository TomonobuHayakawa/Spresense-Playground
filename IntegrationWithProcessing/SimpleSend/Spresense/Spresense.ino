/*
 *  Draw Processing from Spresense
 *  Copyright 2023 Tomonobu Hayakawa
 *
 *  Orignal from https://github.com/baggio63446333/SpresenseCameraApps/tree/main/UsbLiveStreaming
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
 
#include <USBSerial.h>

USBSerial UsbSerial;

// Please change the serial setting for user environment
#define SERIAL_OBJECT   UsbSerial
#define SERIAL_BAUDRATE 921600

void setup()
{
  puts("setup");
  Serial.begin(115200);
  SERIAL_OBJECT.begin(SERIAL_BAUDRATE);
}

void loop()
{
  puts("loop");
  // Receive a start trigger '>'
  if (wait_char('>', 3000)) { // 3sec
    puts("send_i");
    send_data(4);
  }
  sleep(1);
}

int send_data(size_t size)
{
  SERIAL_OBJECT.write('S'); // Payload
  // Send a binary data size in 4byte
  SERIAL_OBJECT.write((size >> 24) & 0xFF);
  SERIAL_OBJECT.write((size >> 16) & 0xFF);
  SERIAL_OBJECT.write((size >>  8) & 0xFF);
  SERIAL_OBJECT.write((size >>  0) & 0xFF);

  static int   data_i = 0;
  static float data_f = 0;
  // Send binary data
  SERIAL_OBJECT.write(data_i);
  data_i+=2;
  SERIAL_OBJECT.write(data_i);
  data_i-=1;

  SERIAL_OBJECT.write((uint8_t)((uint16_t)(data_f*100)>>8));
  SERIAL_OBJECT.write((uint8_t)((uint16_t)(data_f*100)&0xff));
  printf("%f\n",data_f);
  printf("%d\n",(uint16_t)(data_f*100));
  printf("%d\n",(uint8_t)((uint16_t)(data_f*100)>>8));
  printf("%d\n",(uint8_t)((uint16_t)(data_f*100)&0xff));
  data_f+=0.3;

//  SERIAL_OBJECT.write('\0');

  return 0;
}

//
// Wait for a specified character with timeout
//
int wait_char(char code, int timeout)
{
  uint64_t expire = millis() + timeout;

  while (1) {
    if (SERIAL_OBJECT.available() > 0) {
      uint8_t cmd = SERIAL_OBJECT.read();
      if (cmd == code) {
        return 1;
      }
    }
    if (timeout > 0) {
      if (millis() > expire) {
        return 0;
      }
    }
  }
}
