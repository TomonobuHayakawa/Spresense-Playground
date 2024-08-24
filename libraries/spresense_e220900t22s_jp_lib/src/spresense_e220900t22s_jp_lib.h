#pragma once

#include <Arduino.h>

// Set serial for debug console (to the Serial Monitor)
#define SerialMon Serial
// Set serial for LoRa (to the module)
#define SerialLoRa Serial2

// E220-900T22S(JP)へのピンアサイン
#define LoRa_ModeSettingPin_M0 PIN_D20
#define LoRa_ModeSettingPin_M1 PIN_D21
#define LoRa_AuxPin            PIN_D19

// LoRa_Header_Size
#define LORA_HEADER_SIZE (3)

// LoRa_Subblock_Size
#define MAX_SUBPACKET_SIZE (200)

#define SUBPACKET_32_BITS (0b11)
#define SUBPACKET_64_BITS (0b10)
#define SUBPACKET_128_BITS (0b01)
#define SUBPACKET_200_BITS (0b00)

// LoRa_Transmission Method
#define TRANSPARENT_MODE (0b0)
#define FIX_SIZE_MODE (0b1)

// E220-900T22S(JP)のbaud rate
#define LoRa_BaudRate 9600

// E220-900T22S(JP)の設定項目
struct LoRaConfigItem_t {
  uint16_t own_address;
  uint8_t baud_rate;
  uint8_t air_data_rate;
  uint8_t subpacket_size;
  uint8_t rssi_ambient_noise_flag;
  uint8_t transmission_pause_flag;
  uint8_t transmitting_power;
  uint8_t own_channel;
  uint8_t rssi_byte_flag;
  uint8_t transmission_method_type;
  uint8_t lbt_flag;
  uint16_t wor_cycle;
  uint16_t encryption_key;
  uint16_t target_address;
  uint8_t target_channel;
};

struct RecvFrameE220900T22SJP_t {
  uint8_t recv_data[MAX_SUBPACKET_SIZE + LORA_HEADER_SIZE];
  uint8_t recv_data_len;
  int rssi;
};

class CLoRa {
public:

  /**
   * @brief ライブラリのの初期化（pinの設定）
   * @param なし
   * @return なし
   */
  void begin();

  /**
   * @brief E220-900T22S(JP)へLoRa初期設定を行う
   * @param config LoRa設定値の格納先
   * @return 0:成功 1:失敗
   */
  int InitLoRaModule(struct LoRaConfigItem_t &config);

  /**
   * @brief LoRa受信を行う
   * @param recv_data LoRa受信データの格納先
   * @return 0:成功 1:失敗
   */
  int RecieveFrame(struct RecvFrameE220900T22SJP_t *recv_frame);

  /**
   * @brief 固定送信モードでLoRa送信を行う
   * @param config LoRa設定値の格納先
   * @param send_data 送信データ
   * @param size 送信データサイズ
   * @return 0:成功 1:失敗
   */
  int SendFrame(struct LoRaConfigItem_t &config, uint8_t *send_data, int size);

  /**
   * @brief トランスペアレントモードでLoRa送信を行う
   * @param send_data 送信データ
   * @param size 送信データサイズ
   * @return 0:成功 1:失敗
   */
  int SendFrame(uint8_t *send_data, int size);

  /**
   * @brief ノーマルモード(M0=0,M1=0)へ移行する
   */
  void SwitchToNormalMode(void);

  /**
   * @brief WOR送信モード(M0=1,M1=0)へ移行する
   */
  void SwitchToWORSendingMode(void);

  /**
   * @brief WOR受信モード(M0=0,M1=1)へ移行する
   */
  void SwitchToWORReceivingMode(void);

  /**
   * @brief コンフィグモード(M0=1,M1=1)へ移行する
   */
  void SwitchToConfigurationMode(void);

private:
  uint16_t own_address_val;
  uint32_t baud_rate_val;
  uint16_t bw_val;
  uint8_t sf_val;
  uint8_t subpacket_size_val;
  uint8_t transmitting_power_val;
  uint8_t own_channel_val;
  uint16_t wor_cycle_val;
  uint16_t encryption_key_val;
  uint16_t target_address_val;
  uint8_t target_channel_val;
  uint8_t transmission_method_val;
};
