#include "spresense_e220900t22s_jp_lib.h"
#include <vector>

template<typename T>
bool ConfRange(T target, T min, T max);

int CLoRa::InitLoRaModule(struct LoRaConfigItem_t &config) {
  int ret = 0;

  // コンフィグモード(M0=1,M1=1)へ移行する
  SerialMon.printf("switch to configuration mode\n");
  SwitchToConfigurationMode();

  SerialLoRa.begin(LoRa_BaudRate);
  delay(100);

  // Configuration
  std::vector<uint8_t> command = { 0xc0, 0x00, 0x08 };
  std::vector<uint8_t> response = {};

  // Register Address 00H, 01H
  uint8_t ADDH = config.own_address >> 8;
  uint8_t ADDL = config.own_address & 0xff;
  command.push_back(ADDH);
  command.push_back(ADDL);

  // Register Address 02H
  uint8_t REG0 = 0;
  REG0 = REG0 | (config.baud_rate << 5);
  REG0 = REG0 | (config.air_data_rate);
  command.push_back(REG0);

  // Register Address 03H
  uint8_t REG1 = 0;
  REG1 = REG1 | (config.subpacket_size << 6);
  REG1 = REG1 | (config.rssi_ambient_noise_flag << 5);
  REG1 = REG1 | (config.transmission_pause_flag << 4);
  REG1 = REG1 | (config.transmitting_power);
  command.push_back(REG1);

  // Register Address 04H
  uint8_t REG2 = config.own_channel;
  command.push_back(REG2);

  // Register Address 05H
  uint8_t REG3 = 0;
  REG3 = REG3 | (config.rssi_byte_flag << 7);
  REG3 = REG3 | (config.transmission_method_type << 6);
  REG3 = REG3 | (config.lbt_flag << 4);
  REG3 = REG3 | (config.wor_cycle);
  command.push_back(REG3);

  // Register Address 06H, 07H
  uint8_t CRYPT_H = config.encryption_key >> 8;
  uint8_t CRYPT_L = config.encryption_key & 0xff;
  command.push_back(CRYPT_H);
  command.push_back(CRYPT_L);

  SerialMon.printf("# Command Request\n");
  for (auto i : command) {
    SerialMon.printf("0x%02x ", i);
  }
  SerialMon.printf("\n");

  for (auto i : command) {
    SerialLoRa.write(i);
  }
  SerialLoRa.flush();

  delay(100);

  while (SerialLoRa.available()) {
    uint8_t data = SerialLoRa.read();
    response.push_back(data);
  }

  SerialMon.printf("# Command Response\n");
  for (auto i : response) {
    SerialMon.printf("0x%02x ", i);
  }
  SerialMon.printf("\n");

  if (response.size() != command.size()) {
    ret = 1;
  }

  return ret;
}

int CLoRa::RecieveFrame(struct RecvFrameE220900T22SJP_t *recv_frame) {
  int len = 0;
  uint8_t *start_p = recv_frame->recv_data;

  memset(recv_frame->recv_data, 0x00,
         sizeof(recv_frame->recv_data) / sizeof(recv_frame->recv_data[0]));

  while (1) {
    while (SerialLoRa.available()) {
      uint8_t ch = SerialLoRa.read();
      *(start_p + len) = ch;
      len++;

      if (len > 200) {
        return 1;
      }
    }

    if ((SerialLoRa.available() == 0) && (len > 0)) {
      delay(10);
      if (SerialLoRa.available() == 0) {
        recv_frame->recv_data_len = len - 1;
        recv_frame->rssi = recv_frame->recv_data[len - 1] - 256;
        break;
      }
    }
    delay(100);
  }

  return 0;
}

int CLoRa::SendFrame(struct LoRaConfigItem_t &config, uint8_t *send_data,
                     int size) {
  uint8_t subpacket_size = 0;
  switch (config.subpacket_size) {
    case 0b00:
      subpacket_size = 200;
      break;
    case 0b01:
      subpacket_size = 128;
      break;
    case 0b10:
      subpacket_size = 64;
      break;
    case 0b11:
      subpacket_size = 32;
      break;
    default:
      subpacket_size = 200;
      break;
  }
  if (size > subpacket_size) {
    SerialMon.printf("send data length too long\n");
    return 1;
  }
  uint8_t target_address_H = config.target_address >> 8;
  uint8_t target_address_L = config.target_address & 0xff;
  uint8_t target_channel = config.target_channel;

  uint8_t frame[3 + size] = { target_address_H, target_address_L,
                              target_channel };

  memmove(frame + 3, send_data, size);

#if 1 /* print debug */
  for (int i = 0; i < 3 + size; i++) {
    if (i < 3) {
      SerialMon.printf("%02x", frame[i]);
    } else {
      SerialMon.printf("%c", frame[i]);
    }
  }
  SerialMon.printf("\n");
#endif

  for (auto i : frame) {
    SerialLoRa.write(i);
  }
  SerialLoRa.flush();
  delay(100);
  while (SerialLoRa.available()) {
    while (SerialLoRa.available()) {
      SerialLoRa.read();
    }
    delay(100);
  }

  return 0;
}

void CLoRa::SwitchToNormalMode(void) {
  pinMode(LoRa_ModeSettingPin_M0, OUTPUT);
  pinMode(LoRa_ModeSettingPin_M1, OUTPUT);

  digitalWrite(LoRa_ModeSettingPin_M0, 0);
  digitalWrite(LoRa_ModeSettingPin_M1, 0);
  delay(100);
}

void CLoRa::SwitchToWORSendingMode(void) {
  pinMode(LoRa_ModeSettingPin_M0, OUTPUT);
  pinMode(LoRa_ModeSettingPin_M1, OUTPUT);

  digitalWrite(LoRa_ModeSettingPin_M0, 1);
  digitalWrite(LoRa_ModeSettingPin_M1, 0);
  delay(100);
}

void CLoRa::SwitchToWORReceivingMode(void) {
  pinMode(LoRa_ModeSettingPin_M0, OUTPUT);
  pinMode(LoRa_ModeSettingPin_M1, OUTPUT);

  digitalWrite(LoRa_ModeSettingPin_M0, 0);
  digitalWrite(LoRa_ModeSettingPin_M1, 1);
  delay(100);
}

void CLoRa::SwitchToConfigurationMode(void) {
  pinMode(LoRa_ModeSettingPin_M0, OUTPUT);
  pinMode(LoRa_ModeSettingPin_M1, OUTPUT);

  digitalWrite(LoRa_ModeSettingPin_M0, 1);
  digitalWrite(LoRa_ModeSettingPin_M1, 1);
  delay(100);
}

// コンフィグ値が設定範囲内か否か
template<typename T>
bool ConfRange(T target, T min, T max) {
  if (target >= min && target <= max) {
    return true;
  } else {
    return false;
  }
}
