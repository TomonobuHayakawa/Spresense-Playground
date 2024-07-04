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

#include <TelitWiFi.h>
#include "config.h"

#define CONSOLE_BAUDRATE 115200

/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/
TelitWiFi gs2200;
TWIFI_Params gsparams;

/* Multi-core parameters */
const int subcore = 2;

#define FRAME_NUMBER 8
#define DATA_SIZE 2048                      /* tentative! */
#define HEADER_SIZE (12 / sizeof(uint16_t)) /* tentative! */

struct FrameInfo {
  uint16_t buffer[HEADER_SIZE + DATA_SIZE + 1];
  uint16_t sample;
  uint32_t frame_no;
  uint16_t channel;
  bool renable;
  bool wenable;
  FrameInfo()
    : renable(false), wenable(true) {}
};

FrameInfo frame_buffer[FRAME_NUMBER];


/*-------------------------------------------------------------------------*
 * the setup function runs once when you press reset or power the board 
 */
char server_cid = 0;

void setup() {
  Serial.begin(115200);

  /* Initialize MP library */
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }

  /* receive with non-blocking */
  MP.RecvTimeout(1);

  puts("WiFi Start");

  /* Initialize SPI access of GS2200 */
  Init_GS2200_SPI_type(iS110B_TypeA);
  /* Initialize AT Command Library Buffer */
  gsparams.mode = ATCMD_MODE_STATION;
  gsparams.psave = ATCMD_PSAVE_DEFAULT;
  if (gs2200.begin(gsparams)) {
    ConsoleLog("GS2200 Initilization Fails");
    while (1)
      ;
  }
  /* GS2200 Association to AP */
  if (gs2200.activate_station(AP_SSID, PASSPHRASE)) {
    ConsoleLog("Association Fails");
    while (1)
      ;
  }
  ConsoleLog("Start UDP Client");
  // Create UDP Client
  while (1) {
    server_cid = gs2200.connectUDP(UDPSRVR_IP, UDPSRVR_PORT, LocalPort);
    ConsolePrintf("server_cid: %d \r\n", server_cid);
    if (server_cid != ATCMD_INVALID_CID) {
      puts("OK!");
      break;
    }
  }

  ConsoleLog("Start to send UDP Data");
  // Prepare for the next chunck of incoming data
  WiFi_InitESCBuffer();
  ConsolePrintf("\r\n");

  int connect = task_create("Connect", 70, 0x2000, connect_task, (FAR char* const*)0);
}

int connect_task(int argc, FAR char* argv[]) {
  while (1) {
    for (int i = 0; i < FRAME_NUMBER; i++) {
      if (frame_buffer[i].renable) {
        if (frame_buffer[i].sample > 700) frame_buffer[i].sample = 700;
        if (!send_data(frame_buffer[i])) {
          puts("send miss!");
          gs2200.stop(server_cid);
          puts("stoped!");
          sleep(1);
          server_cid = gs2200.connectUDP(UDPSRVR_IP, UDPSRVR_PORT, LocalPort);
          puts("connect again!");
          usleep(100 * 1000);
        }
      }
    }
  }
}

#define SPI_MAX_SIZE 1400
#define TXBUFFER_SIZE SPI_MAX_SIZE

bool send_data(FrameInfo& frame) {
  frame.renable = false;

  memcpy(frame.buffer, "SPRS", 4);
  memcpy(&frame.buffer[2], &frame.channel, 2);
  memcpy(&frame.buffer[3], &frame.sample, 2);
  memcpy(&frame.buffer[4], &frame.frame_no, 4);
  /*      printf("frame.channel = %d\n",frame.buffer[2]);
      printf("frame.sample = %d\n",frame.buffer[3]);
      printf("frame no = %d\n",frame.frame_no);*/
  frame.buffer[HEADER_SIZE+frame.sample] = '\0';
  frame.sample += HEADER_SIZE+1;
  uint8_t* ptr = (uint8_t*)frame.buffer;
  while(frame.sample>0){
    int write_sample = ((frame.sample > TXBUFFER_SIZE) ? TXBUFFER_SIZE : frame.sample);
    printf("write_sample = %d\n",write_sample);
    bool ret = gs2200.write(server_cid, (const uint8_t*)ptr, write_sample*sizeof(uint16_t));
    if(!ret) return ret;
    ptr += write_sample*sizeof(uint16_t);
    frame.sample -= write_sample;
  }
  frame.wenable = true;
  return true;
}

void loop()
{
  int      ret;
  int8_t   rcvid;
  Request  *request;
  
  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &request);
  if (ret >= 0) {
    write_buffer(request->buffer, request->sample, request->frame_no, request->channel);
  }
}

int write_buffer(uint16_t* data, int sample, uint32_t frame_no, int channel) {
//if(channel==0){return 0;}
  for (int i = 0; i < FRAME_NUMBER; i++) {
    if (frame_buffer[i].wenable) {
      frame_buffer[i].wenable = false;
      frame_buffer[i].sample = ((DATA_SIZE > sample) ? sample : DATA_SIZE);
      memcpy(&frame_buffer[i].buffer[HEADER_SIZE], data, sample * sizeof(uint16_t));
      frame_buffer[i].frame_no = frame_no;
      frame_buffer[i].channel = channel;
      frame_buffer[i].renable = true;
      return 0;
    }
  }
  return 1;
}

void errorLoop(int num) {
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
