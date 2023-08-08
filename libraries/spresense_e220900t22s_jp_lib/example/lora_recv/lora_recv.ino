#include <Arduino.h>
#include <vector>
#include "spresense_e220900t22s_jp_lib.h"

CLoRa lora;
struct RecvFrameE220900T22SJP_t data;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  /* Set serial baudrate. */
  Serial.begin(115200);

  /* Wait HW initialization done. */
  sleep(1);
  Serial.printf("program start\n");

  // LoRa設定値
  struct LoRaConfigItem_t config = {
    0x0000,   // own_address 0
    0b011,    // baud_rate 9600 bps
    0b10000,  // air_data_rate SF:9 BW:125
    0b00,     // subpacket_size 200
    0b1,      // rssi_ambient_noise_flag 有効
    0b0,      // transmission_pause_flag 有効
    0b01,     // transmitting_power 13 dBm
    0x00,     // own_channel 0
    0b1,      // rssi_byte_flag 有効
    0b1,      // transmission_method_type 固定送信モード
    0b0,      // lbt_flag 有効
    0b011,    // wor_cycle 2000 ms
    0x0000,   // encryption_key 0
    0x0000,   // target_address 0
    0x00      // target_channel 0
  };

  // E220-900T22S(JP)へのLoRa初期設定
  if (lora.InitLoRaModule(config)) {
    SerialMon.printf("init error\n");
    return;
  } else {
    Serial.printf("init ok\n");
  }

  // ノーマルモード(M0=0,M1=0)へ移行する
  SerialMon.printf("switch to normal mode\n");
  lora.SwitchToNormalMode();

  // LoRa受信
  while (1) {
    if (lora.RecieveFrame(&data) == 0) {
      SerialMon.printf("recv data:\n");
      for (int i = 0; i < data.recv_data_len; i++) {
        SerialMon.printf("%c", data.recv_data[i]);
      }
      SerialMon.printf("\n");
      SerialMon.printf("hex dump:\n");
      for (int i = 0; i < data.recv_data_len; i++) {
        SerialMon.printf("%02x ", data.recv_data[i]);
      }
      SerialMon.printf("\n");
      SerialMon.printf("RSSI: %d dBm\n", data.rssi);
      SerialMon.printf("\n");

      SerialMon.flush();
    }

    delay(1);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED0, HIGH);
  delay(100);
  digitalWrite(LED1, HIGH);
  delay(100);
  digitalWrite(LED2, HIGH);
  delay(100);
  digitalWrite(LED3, HIGH);
  delay(1000);

  digitalWrite(LED0, LOW);
  delay(100);
  digitalWrite(LED1, LOW);
  delay(100);
  digitalWrite(LED2, LOW);
  delay(100);
  digitalWrite(LED3, LOW);
  delay(1000);
}
