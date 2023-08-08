#ifndef TZM902Dx_h
#define TZM902Dx_h

#include <Arduino.h>

#define TZM902Dx_PREAMBLE_SIZE 4
#define TZM902Dx_SEND_VARIABLE_LENGTH_SIZE 50
#define CRC16_POLY 0x1021 // x16 + x12 + x5 + 1

///////////////////////////////////
// TZM902Dx Commands             //
///////////////////////////////////
enum TZM902Dx_TR_COMMANDS
{
  SEND_STANDARD_DATA,
  SEND_VARIABLE_LENGTH_DATA,
  INQUIRE_VERSION,
  INQUIRE_MAC,
  INQUIRE_NETWORK_TIME,
  INQUIRE_NETWORK_QUALITY,
  INQUIRE_MODULE_STATUS,
  SET_SET_TIMER,
  SET_TURN_OFF_TIMER,
  SET_TEST_MODE,
  RESET_TEST_MODE
};

#define RECEIVE_DATA_FRAMES_ACK 0x01
#define RECEIVE_BUFFER_FULL 0x02
#define RECEIVE_LENGTH_ERROR 0x03
#define RECEIVE_CRC_ERROR 0x04

#define INQUIRE_VERSION 0x00
#define INQUIRE_MAC 0x10
#define INQUIRE_TIME 0x11
#define INQUIRE_NETWORK_QUALITY 0x13

#define COMMANDS_ACK_SET_SUCCEED 0x20
#define COMMANDS_ACK_SET_FAILED 0x21

#define WAKEUP_REASON_DOWNLINK_DATA 0x30

#define MODULE_STATUS_UNREGISTERED 0x43
#define MODULE_STATUS_REGISTERED 0x44
#define MODULE_STATUS_LONG_SLEEP 0x45
#define MODULE_STATUS_SHORT_SLEEP 0x46

////////////////////////////////
// TZM902Dx Class Declaration //
////////////////////////////////
class TZM902Dx
{
public:
  TZM902Dx();

  int ZETA_wakeup = 20; // ZETA_wakeupはD20（デジタルピン20、GPIO R#5）に接続
  int ZETA_intpin = 21; // ZETA_intpinはD21（デジタルピン21、GPIO R#4）に接続

  bool TZM902Dx::init();
  bool TZM902Dx::connect();

  char tick();

  int read(char *store, int size);
  bool send(TZM902Dx_TR_COMMANDS cmd, char *var, int size);
//  bool send_no_retry(TZM902Dx_TR_COMMANDS cmd, char *var, int size, int debug_retry_count);   // debug retry
  bool send_no_retry(TZM902Dx_TR_COMMANDS cmd, char *var, int size );
  bool send_raw(char *var, int size);

//  void TZM902Dx::GPS_Data_Convert(String *GPS_Raw, char *pakcet);

  int get_downlink(char *data, int size);

private:
  char downlink[64];
  char response[64];

  uint16_t downlinkWp;
  uint16_t responseWp;

  char network_status(char *rx, int size);
  bool getPreamble(TZM902Dx_TR_COMMANDS cmd, char *store, char *var, int size);
  uint16_t Crc16_CCITT_Xmodem(char *pmsg, uint16_t msg_size);
};

#endif
