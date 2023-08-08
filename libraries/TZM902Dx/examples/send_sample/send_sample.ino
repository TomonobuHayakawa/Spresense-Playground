/*
 *  send_sample.ino - Sample Application for Zeta
 *  Copyright 2023
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

#include "TZM902Dx.h"

TZM902Dx Zeta;

/**
   @brief ZETAモジュールから応答またはダウンリンクを取得する

*/
int zeta_downlink_check() {
  char packet[16];
  int n;
  int retry = 10;

  while (true) {
    switch (Zeta.tick()) {
      case WAKEUP_REASON_DOWNLINK_DATA:
        n = Zeta.get_downlink(packet, sizeof(packet));
//        downlink_proccess(packet, n);
        if (packet[4] != 0xC0) {
          puts("No control command on Result.");
          return 0;
         }

        return WAKEUP_REASON_DOWNLINK_DATA;

      case 0xFF:
        // Serial.println("zeta recv error.");
        if (--retry == 0)
        {
          return 0xFF;
        }
        delay(1000);
        break;

      default:
        break;
    }
  }

  return 0;
}

void setup() {

  pinMode(Zeta.ZETA_wakeup, OUTPUT);     // 出力に設定
  //  pinMode(21, INPUT_PULLDOWN);       // ZETA_INT

  digitalWrite(Zeta.ZETA_wakeup, HIGH);
  int resut;

  Serial.begin(115200);

  Zeta.init();
  Zeta.connect();

  /* Turn on LED0 Connect Compleate */
  ledOn(PIN_LED0);
  delay(1000);
  ledOff(PIN_LED0);

  Serial.println("初回起動UL");
  //1:データ型1byte 0x0F 2:バージョン情報2byte 0x0009
  char wakeupsensUL[3] = {0x0F, 0x02, 0x00};
  int size = sizeof(wakeupsensUL);
  //ZETAアップリンク Zeta.send("デバイス名(自動取得）"SEND_VARIABLE_LENGTH_DATA, "ペイロード"packet, "パケットの大きさ"size)
  Zeta.send(SEND_VARIABLE_LENGTH_DATA, wakeupsensUL, size);

  sleep(1);

}

void loop() {
  
  char data[128];
  static char dmy_data = 'A';
	
  for(int i=0;i<sizeof(data);i++){
  	data[i] = dmy_data+(i % 26);
  }
  data[sizeof(data)] = '\0';
  dmy_data = dmy_data % 10;

  puts(data);
  
  if (!Zeta.send(SEND_VARIABLE_LENGTH_DATA, data, strlen(data))) {
    sleep(1);
    Serial.println("one minute wait");
    Zeta.send(SEND_VARIABLE_LENGTH_DATA, data, sizeof(data));
  }

  sleep(10);

}
