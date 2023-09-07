/*
 *  ViewCore.ino - Demo application.
 *  Copyright 2023 T.Hayakawa
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
#include "CoreInterface.h"

#include <USBSerial.h>

USBSerial UsbSerial;

#define GRAPH_WIDTH 100
#define FRAME_NUMBER 4

// Please change the serial setting for user environment
#define SERIAL_OBJECT   UsbSerial
#define SERIAL_BAUDRATE 921600

struct FrameInfo{
  uint8_t buffer[GRAPH_WIDTH];
  bool renable;
  bool wenable;
  FrameInfo():renable(false),wenable(true){}
};

FrameInfo frame_buffer[FRAME_NUMBER];

/**
 *  @brief Setup audio device to capture PCM stream
 *
 *  Select input device as microphone <br>
 *  Set PCM capture sapling rate parameters to 48 kb/s <br>
 *  Set channel number 4 to capture audio from 4 microphones simultaneously <br>
 */
void setup()
{
  Serial.begin(115200);

  /* Initialize MP library */
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }
  
  /* receive with non-blocking */
  MP.RecvTimeout(1);

  puts("Graph Start");
  
/*  screen_width = tft.width();
  screen_height = tft.height(); */
  SERIAL_OBJECT.begin(SERIAL_BAUDRATE);
  Serial.println("Done!");

  int proter = task_create("Sender", 70, 0x400, sender_task, (FAR char* const*) 0);

}

int sender_task(int argc, FAR char *argv[])
{
  while(1){
    for(int i=0;i<FRAME_NUMBER;i++){
      if(frame_buffer[i].renable){
        frame_buffer[i].renable = false;
        send_data(frame_buffer[i].buffer,GRAPH_WIDTH);
        frame_buffer[i].wenable = true;
      }
    }
  }
}

void loop()
{
  int      ret;
  int8_t   rcvid;
  Request  *request;
  
  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &request);
  if (ret >= 0) {
     write_buffer(request->buffer);
  }
}

int write_buffer(float* data)
{
  for(int i=0;i<FRAME_NUMBER;i++){
    if(frame_buffer[i].wenable){
      frame_buffer[i].wenable = false;
      for(int j=0;j<GRAPH_WIDTH;j++){
        frame_buffer[i].buffer[j] = (uint8_t)*data;
        data++;
      }
      frame_buffer[i].renable =true;
      return 0;
    }
  }
  return 1;
}

void send_data(byte* data,int size)
{
  if (wait_char('>', 3000)) { // 3sec
    puts("send data");

    SERIAL_OBJECT.write('S'); // Payload
    // Send a binary data size in 4byte
    SERIAL_OBJECT.write((size >> 24) & 0xFF);
    SERIAL_OBJECT.write((size >> 16) & 0xFF);
    SERIAL_OBJECT.write((size >>  8) & 0xFF);
    SERIAL_OBJECT.write((size >>  0) & 0xFF);

    // Send binary data
    SERIAL_OBJECT.write(data,size);  
  }
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
