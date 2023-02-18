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

#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include "AppFunc.h"
#include <MP.h>

extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;
char server_cid = 0;

#define FRAME_NUMBER 10
#define DATA_SIZE    512 /* tentative! */

struct FrameInfo{
  float buffer[DATA_SIZE];
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

  int connect = task_create("Connect", 70, 0x400, connect_task, (FAR char* const*) 0);

}

int connect_task(int argc, FAR char *argv[])
{
  puts("WiFi Start");
  Init_GS2200_SPI();

  App_InitModule();
  App_ConnectAP();

  ATCMD_NetworkStatus networkStatus;
  ATCMD_RESP_E resp = ATCMD_RESP_UNMATCH;

  puts("Start UDP Client");
  resp = AtCmd_NCUDP( (char *)UDPSRVR_IP, (char *)UDPSRVR_PORT, (char *)LocalPort, &server_cid);   // Create UDP Client; AT+NCUDP=<Dest-Address>,<Port>[<,Src-Port>]
  printf( "server_cid: %d \r\n", server_cid);

  if (resp != ATCMD_RESP_OK){
    puts("No Connect!");
    delay(2000);
    errorLoop(3);
  }

  if (server_cid == ATCMD_INVALID_CID){
    puts("No CID!");
    delay(2000);
    errorLoop(3);
  }

  do{
    resp = AtCmd_NSTAT(&networkStatus);         // AT+NSTAT=?
  } while (ATCMD_RESP_OK != resp);

  puts("Connected");
  printf("IP: %d.%d.%d.%d\r\n\r\n", networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);

  while(1){

    for(int i=0;i<FRAME_NUMBER;i++){
      if(frame_buffer[i].renable){
        frame_buffer[i].renable = false;
        WiFi_InitESCBuffer();
        AtCmd_SendBulkData(server_cid, frame_buffer[i].buffer, DATA_SIZE*sizeof(float));
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
      memcpy(frame_buffer[i].buffer,data,DATA_SIZE*sizeof(float));
      frame_buffer[i].renable =true;
      return 0;
    }
  }
  return 1;
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
