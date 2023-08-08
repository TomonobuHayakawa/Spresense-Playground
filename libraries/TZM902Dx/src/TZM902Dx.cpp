// ver1.1 retry send 2023/04/12
#include "TZM902Dx.h"
#include <Arduino.h>
#include <string.h>

/**
 * @brief Construct a new TZM902Dx::TZM902Dx object
 *
 */
TZM902Dx::TZM902Dx()
{
}

/**
 * @brief 初期化
 *
 * @return bool
 */
bool TZM902Dx::init()
{
  Serial2.begin(115200, SERIAL_8N1);
  Serial2.setTimeout(10000);

  pinMode(ZETA_wakeup, OUTPUT);
  pinMode(ZETA_intpin, INPUT_PULLDOWN);

  digitalWrite(ZETA_wakeup, HIGH);

  downlinkWp = 0;
  responseWp = 0;

  return true;
}

/**
 * @brief 接続
 *
 * @return bool
 */
bool TZM902Dx::connect()
{
//  int debug_retry_count = 3;
  // 待機
  Serial.println("waiting for TZM902Dx connect to ZETA server");
  for (int i = 0; i < 10; i++)
  {
    delay(1000);
    Serial.println(i);
  }

  // 登録待ち
  char rx[8];
  while (true)
  {
//    Serial.printf("debug_retry_count=%d \n", debug_retry_count);
//    if (debug_retry_count == 0){         // debug_for_retry
//      digitalWrite(ZETA_wakeup, LOW);  // debug_for_retry
//    }
//      debug_retry_count--;              // debug_for_retry
    char sts = network_status(rx, sizeof(rx));
    uint32_t sleep_time = 10000;

    if (sts == MODULE_STATUS_REGISTERED)
    {
      Serial.println("registered");
      delay(100);
      break;
    }
    else if (sts == MODULE_STATUS_SHORT_SLEEP)
    {
      sleep_time = rx[4] << 8 | rx[5];
      Serial.printf("short sleep %d sec\n", sleep_time);
      sleep_time *= 1000;
    }
    else if (sts == MODULE_STATUS_LONG_SLEEP)
    {
      sleep_time = (rx[4] << 8 | rx[5]) * 60;
      Serial.printf("long sleep %d sec\n", sleep_time);
      sleep_time *= 1000;
    }
    delay(sleep_time);
  }

  return true;
}

/**
 * @brief モジュールステータス確認
 *
 * @param rx 受信バッファ
 * @param size 受信バッファサイズ
 * @return char ステータス
 */
char TZM902Dx::network_status(char *rx, int size)
{
  if (size < 4)
  {
    return -1;
  }

  // SEND
//  send_no_retry(INQUIRE_MODULE_STATUS, NULL, 0, 2);       // debug retry
  send_no_retry(INQUIRE_MODULE_STATUS, NULL, 0); 

  // RECV
  read(rx, size);

  return rx[3];
}

/**
 * @brief 受信データ判定
 *
 * @return char データタイプ
 */
char TZM902Dx::tick()
{
  if (!Serial2.available())
  {
    return -1;
  }

  //  Serial.println("get UART data");   //debug

  char rx[64];
  int n = read(rx, sizeof(rx));
  if (n <= 0)
  {
    return -1;
  }

  if (rx[3] == WAKEUP_REASON_DOWNLINK_DATA)
  {
    memcpy(downlink, rx, n);
    downlinkWp = n;
  }
  else
  {
    memcpy(response, rx, n);
    responseWp = n;
  }

  return rx[3];
}

/**
 * @brief 受信
 *
 * @param rx 受信バッファ
 * @param size 受信バッファサイズ
 * @return int 受信データ数
 */
int TZM902Dx::read(char *rx, int size)
{
  int n = 0;
  int len = 0;
  n = Serial2.readBytes(rx, 3);
  if (n != 3)
  {
    return 0;
  }
  len += n;

  n = Serial2.readBytes(&rx[3], rx[2]);
  if (n != rx[2])
  {
    return 0;
  }
  len += n;

  uint16_t crc1 = Crc16_CCITT_Xmodem(&rx[2], len - 4);
  uint16_t crc2 = rx[len - 2] << 8 | rx[len - 1];
  if (crc1 != crc2)
  {
    return -1;
  }

  Serial.print("rx : ");
  for (size_t i = 0; i < len; i++)
  {
    Serial.printf("%02X ", rx[i]);
  }
  Serial.println();

  return len;
}

/**
 * @brief 送信
 *
 * @param cmd 送信コマンド
 * @param var 送信データ
 * @param size 送信データサイズ
 * @return bool
 */
bool TZM902Dx::send(TZM902Dx_TR_COMMANDS cmd, char *var, int size)
{
  // // Read Buff Clear
  // while(Serial2.available()) {
  //   Serial2.read();
  // }

  bool ret = false;
  char rx[16];

  unsigned long tmout = Serial2.getTimeout();
  Serial2.setTimeout(3000);

  for (size_t i = 0; i < 3; i++)
  {
//    send_no_retry(cmd, var, size, i);   // debug  retry
    send_no_retry(cmd, var, size);

    int n = read(rx, sizeof(rx));

    if (n > 0)
    {
      // FA F5 03 01 45 72 /* データ送信成功 */
      // FA F5 03 02 75 11 /* バッファフル; 送信失敗*/
      // FA F5 03 03 65 30 /* データ長エラー */
      if (rx[3] == 0x01)
      {
        ret = true;
        break;
      }
    }

    delay(60 * 1000);
  }

  Serial2.setTimeout(tmout);

  return ret;
}

/**
 * @brief 送信
 *
 * @param cmd 送信コマンド
 * @param var 送信データ
 * @param size 送信データサイズ
 * @return bool
 */
//bool TZM902Dx::send_no_retry(TZM902Dx_TR_COMMANDS cmd, char *var, int size, int debug_retry_count)    // debug retry
bool TZM902Dx::send_no_retry(TZM902Dx_TR_COMMANDS cmd, char *var, int size)
{
  // // Read Buff Clear
  // while(Serial2.available()) {
  //   Serial2.read();
  // }

  char pkt[4 + size + 2];

  // Send TZM902Dx's Preanble
  if (getPreamble(cmd, pkt, var, size) == false)
  {
    return false;
  }
  memcpy(&pkt[4], var, size);
  uint16_t CRC_check_bytes = Crc16_CCITT_Xmodem(&pkt[2], size + 2);

  pkt[4 + size + 0] = (CRC_check_bytes & 0xFF00) >> 8;
  pkt[4 + size + 1] = CRC_check_bytes & 0x00FF;

  bool ret = false;
  char rx[16];

  digitalWrite(ZETA_wakeup, LOW);

//    Serial.printf("debug_retry_count=%d \n", debug_retry_count);
//    if (debug_retry_count == 2){         // debug_for_retry
//      digitalWrite(ZETA_wakeup, LOW);  // debug_for_retry
//    }

  delay(20);

  Serial2.write(pkt, sizeof(pkt));

  Serial.print("tx : ");
  for (size_t i = 0; i < sizeof(pkt); i++)
  {
    Serial.printf("%02X ", pkt[i]);
  }
  Serial.println();

  delay(20);
  digitalWrite(ZETA_wakeup, HIGH);

  Serial2.flush();

  return ret;
}

/**
 * @brief 送信(生データ)
 *
 * @param var 送信データ
 * @param size 送信データサイズ
 * @return bool
 */
bool TZM902Dx::send_raw(char *var, int size)
{
  digitalWrite(ZETA_wakeup, LOW);
  delay(20);

  Serial2.write(var, size);

  Serial.print("tx : ");
  for (size_t i = 0; i < size; i++)
  {
    Serial.printf("%02X ", var[i]);
  }
  Serial.println();

  delay(20);
  digitalWrite(ZETA_wakeup, HIGH);

  Serial2.flush();

  return true;
}

/**
 * @brief ヘッダー取得
 *
 * @param cmd 送信コマンド
 * @param store ヘッダー格納バッファ
 * @param var 送信データ
 * @param size 送信データサイズ
 * @return bool
 */
bool TZM902Dx::getPreamble(TZM902Dx_TR_COMMANDS cmd, char *store, char *var, int size)
{
  // error
  if ((cmd == SEND_VARIABLE_LENGTH_DATA) && (var != NULL && sizeof(var) == 0))
  {
    return false;
  }

  char temp[TZM902Dx_PREAMBLE_SIZE] = {0xfa, 0xf5, 0x00, 0x00};
  switch (cmd)
  {
  case SEND_VARIABLE_LENGTH_DATA:
    temp[2] = 0x03 + size;
    temp[3] = 0x02;
    break;

  case INQUIRE_VERSION:
    temp[2] = 0x03;
    temp[3] = 0x00;
    break;

  case INQUIRE_MAC:
    temp[2] = 0x03;
    temp[3] = 0x10;
    break;

  case INQUIRE_NETWORK_TIME:
    temp[2] = 0x03;
    temp[3] = 0x11;
    break;

  case INQUIRE_NETWORK_QUALITY:
    temp[2] = 0x03;
    temp[3] = 0x13;
    break;

  case INQUIRE_MODULE_STATUS:
    temp[2] = 0x03;
    temp[3] = 0x14;
    break;

  case SET_TEST_MODE:
    temp[2] = 0x04;
    temp[3] = 0x22;
    break;
  }
  memcpy(store, temp, sizeof(temp));
  return true;
}

/**
 * @brief CRC計算
 *
 * @param pmsg 計算対象バッファ
 * @param msg_size 計算対象データサイズ
 * @return uint16_t CRC計算結果
 */
uint16_t TZM902Dx::Crc16_CCITT_Xmodem(char *pmsg, uint16_t msg_size)
{
  uint16_t i = 0, j = 0;
  uint16_t msg = 0;
  uint16_t crc = 0x0000;
  for (i = 0; i < msg_size; i++)
  {
    msg = *pmsg;
    msg <<= 8;
    pmsg++;
    for (j = 0; j < 8; j++)
    {
      if ((msg ^ crc) >> 15)
      {
        crc <<= 1;
        crc ^= CRC16_POLY;
      }
      else
        crc <<= 1;
      msg <<= 1;
    }
  }
  return (crc);
}


/**
 * @brief ダウンリンクデータ取得
 *
 * @param data 格納バッファ
 * @param size 格納バッファサイズ
 * @return int 格納データ数
 */
int TZM902Dx::get_downlink(char *data, int size)
{
  int n = size < downlinkWp ? size : downlinkWp;
  memcpy(data, downlink, n);

  Serial.print("downlink : ");
  for (size_t i = 0; i < n; i++)
  {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();

  return n;
}
