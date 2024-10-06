/*
 *  LimitedAP.ino - Demo application for WiFi Access Point with LTE.
 *  Copyright 2024 T.Hayakawa
 *
 *  This work is free software; you can redistribute it and/or modify it under the terms 
 *  of the GNU Lesser General Public License as published by the Free Software Foundation; 
 *  either version 2.1 of the License, or (at your option) any later version.
 *
 *  This work is distributed in the hope that it will be useful, but without any warranty; 
 *  without even the implied warranty of merchantability or fitness for a particular 
 *  purpose. See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with 
 *  this work; if not, write to the Free Software Foundation, 
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <TelitWiFi.h>
#include "CoreInterface.h"
#include "config.h"

#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

#define  CONSOLE_BAUDRATE  115200


const uint16_t RECEIVE_PACKET_SIZE = 1500;
uint8_t Receive_Data[RECEIVE_PACKET_SIZE] = {0};

TelitWiFi gs2200;
TWIFI_Params gsparams;


// the setup function runs once when you press reset or power the board
void setup() {

  /* initialize digital pin of LEDs as an output. */
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  MP.begin();
  MP.RecvTimeout(MP_RECV_BLOCKING);

  digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
  Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

  /* Initialize SPI access of GS2200 */
  Init_GS2200_SPI_type(iS110B_TypeA);

  /* Initialize AT Command Library Buffer */
  gsparams.mode = ATCMD_MODE_LIMITED_AP;
  gsparams.psave = ATCMD_PSAVE_DEFAULT;
  if( gs2200.begin( gsparams ) ){
    ConsoleLog( "GS2200 Initilization Fails" );
    while(1);
  }

  /* GS2200 runs as AP */
  if( gs2200.activate_ap( AP_SSID, PASSPHRASE, AP_CHANNEL ) ){
    ConsoleLog( "WiFi Network Fails" );
    while(1);
  }

  digitalWrite( LED0, HIGH ); // turn on LED
  
}

// the loop function runs over and over again forever
void loop() {

  char remote_cid = 0;
  char server_cid = 0;
  uint32_t timer = 5000;

  ConsoleLog( "Start TCP Server");
  server_cid = gs2200.start_tcp_server((char*)TCPSRVR_PORT);
  if (server_cid == ATCMD_INVALID_CID) {
    delay(2000);
    return;
  }
  int8_t  msgid;
  Request packet;

  int ret = MP.Recv(&msgid, &packet);
  if (ret < 0) {
    puts("MP recieve error!");
    exit(1);
  }

  if(!packet.enable) return;

  while(1) {
    ConsoleLog("Waiting for TCP Client");
    if (true != gs2200.wait_connection(&remote_cid, timer)) {
      continue;
    }

    ConsoleLog( "TCP Client Connected");
    
    // Start the echo server
    while (1) {

      #define PACKET_NUM 4
      static Result packet[PACKET_NUM];
      static int index = 0;

      if (gs2200.available()) {
        if (0 < gs2200.read(remote_cid, Receive_Data, RECEIVE_PACKET_SIZE)) {
          ConsolePrintf("Received : %s\r\n", Receive_Data);

          packet[index].device = 1;
          packet[index].size = strlen(Receive_Data)+1;
          memcpy(packet[index].buffer,Receive_Data,packet[index].size);
          packet[index].buffer[packet[index].size-1]=0;

          int ret = MP.Send(NOMAL_RESULT_ID, (void*)&packet[index]);
          if (ret < 0) { puts("MP send error!"); }

          index=(index+1)%PACKET_NUM;
          WiFi_InitESCBuffer();
        }
      }
    }
  }
}
