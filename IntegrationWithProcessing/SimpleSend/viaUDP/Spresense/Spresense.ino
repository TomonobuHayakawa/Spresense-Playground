/*
 *  Draw Processing from Spresense via UDP on WiFi
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
 
#include <TelitWiFi.h>
#include "config.h"

#define  CONSOLE_BAUDRATE  115200
/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/
TelitWiFi gs2200;
TWIFI_Params gsparams;

const uint16_t UDP_PACKET_SIZE = 1500; 
uint8_t UDP_Data[UDP_PACKET_SIZE] = {0};

/*-------------------------------------------------------------------------*
 * the setup function runs once when you press reset or power the board 
 */
char server_cid = 0;

void setup()
{
  puts("setup");
  Serial.begin(CONSOLE_BAUDRATE);

  /* Initialize AT Command Library Buffer */
  AtCmd_Init();
  /* Initialize SPI access of GS2200 */
  Init_GS2200_SPI_type(iS110B_TypeA);
  /* Initialize AT Command Library Buffer */
  gsparams.mode = ATCMD_MODE_STATION;
  gsparams.psave = ATCMD_PSAVE_DEFAULT;
  if (gs2200.begin(gsparams)) {
    ConsoleLog("GS2200 Initilization Fails");
    while(1);
  }

  /* GS2200 Association to AP */
  if (gs2200.activate_station(AP_SSID, PASSPHRASE)) {
    ConsoleLog("Association Fails");
    while(1);
  }

  ConsoleLog("Start UDP Client");
  // Create UDP Client
  while(1){
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
}

void loop()
{
  puts("loop");
  puts("send");
  send_data(4);
  sleep(1);
}

int send_data(size_t size)
{
  memcpy(UDP_Data,"SPRS",4);
  memcpy(&UDP_Data[4],&size,4);

  static int   data_i = 0;
  static float data_f = 0;
  // Send binary data
  UDP_Data[8] = data_i;
  data_i+=2;
  UDP_Data[9] = data_i;
  data_i-=1;

  UDP_Data[10] = (uint8_t)((uint16_t)(data_f*100)>>8);
  UDP_Data[11] = (uint8_t)((uint16_t)(data_f*100)&0xff);
  printf("%d\n",UDP_Data[8]);
  printf("%d\n",UDP_Data[9]);
  printf("%f\n",data_f);
  printf("%d\n",(uint16_t)(data_f*100));
  printf("%d\n",(uint8_t)((uint16_t)(data_f*100)>>8));
  printf("%d\n",(uint8_t)((uint16_t)(data_f*100)&0xff));
  data_f+=0.31;

  UDP_Data[12] = '\0';

//  SERIAL_OBJECT.write('\0');

  gs2200.write(server_cid, UDP_Data, (4+4+4+1));

  return 0;
}
