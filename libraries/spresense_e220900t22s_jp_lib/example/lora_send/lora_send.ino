#include <Arduino.h>
#include <vector>
#include "spresense_e220900t22s_jp_lib.h"

/* Send data size is lora subblock size - lora heaser size. */
#define SEND_DATA_SIZE (29) // 32 - 3
//#define SEND_DATA_SIZE (61) // 64 - 3
//#define SEND_DATA_SIZE (125) // 128 - 3
//#define SEND_DATA_SIZE (197) // 200 - 3

CLoRa lora;

// LoRa設定値
struct LoRaConfigItem_t config = {
  0x0000,   // own_address 0
  0b011,    // baud_rate 9600 bps
  0b10000,  // air_data_rate SF:9 BW:125
#if (SEND_DATA_SIZE == 29)
  SUBPACKET_32_BITS,
#elif (SEND_DATA_SIZE == 61)
  SUBPACKET_64_BITS,
#elif (SEND_DATA_SIZE == 125)
  SUBPACKET_128_BITS,
#else
  SUBPACKET_200_BITS,
#endif
  0b1,      // rssi_ambient_noise_flag 有効
  0b0,      // transmission_pause_flag 有効
  0b01,     // transmitting_power 13 dBm
  0x00,     // own_channel 0
  0b1,      // rssi_byte_flag 有効
  FIX_SIZE_MODE,      // transmission_method_type 固定送信モード
  0b0,      // lbt_flag 有効
  0b011,    // wor_cycle 2000 ms
  0x0000,   // encryption_key 0
  0x0000,   // target_address 0
  0x00      // target_channel 0
};

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

   // LoRaライブラリの初期設定
  lora.begin();

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
}

void loop() {
  char msg[SEND_DATA_SIZE] = { 0 };
  static char dmy_data = '0';

  for (int i = 0; i < SEND_DATA_SIZE; i++) msg[i] = dmy_data + (i % 40);
  dmy_data > 'S' ? dmy_data = '0' : dmy_data++;

  if (lora.SendFrame(config, (uint8_t *)msg, sizeof(msg)) == 0) {
    SerialMon.printf("send succeeded.\n");
    SerialMon.printf("\n");
  } else {
    SerialMon.printf("send failed.\n");
    SerialMon.printf("\n");
  }
  SerialMon.flush();

  sleep(10);
}
