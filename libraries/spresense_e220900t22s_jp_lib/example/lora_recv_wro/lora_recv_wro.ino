#include <vector>
#include <LowPower.h>
#include "spresense_e220900t22s_jp_lib.h"

CLoRa lora;
struct RecvFrameE220900T22SJP_t data;

const char* boot_cause_strings[] = {
  "Power On Reset with Power Supplied",
  "System WDT expired or Self Reboot",
  "Chip WDT expired",
  "WKUPL signal detected in deep sleep",
  "WKUPS signal detected in deep sleep",
  "RTC Alarm expired in deep sleep",
  "USB Connected in deep sleep",
  "Others in deep sleep",
  "SCU Interrupt detected in cold sleep",
  "RTC Alarm0 expired in cold sleep",
  "RTC Alarm1 expired in cold sleep",
  "RTC Alarm2 expired in cold sleep",
  "RTC Alarm Error occurred in cold sleep",
  "Unknown(13)",
  "Unknown(14)",
  "Unknown(15)",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "GPIO detected in cold sleep",
  "SEN_INT signal detected in cold sleep",
  "PMIC signal detected in cold sleep",
  "USB Disconnected in cold sleep",
  "USB Connected in cold sleep",
  "Power On Reset",
};

void printBootCause(bootcause_e bc)
{
  Serial.println("--------------------------------------------------");
  Serial.print("Boot Cause: ");
  Serial.print(boot_cause_strings[bc]);
  if ((COLD_GPIO_IRQ36 <= bc) && (bc <= COLD_GPIO_IRQ47)) {
    // Wakeup by GPIO
    int pin = LowPower.getWakeupPin(bc);
    Serial.print(" <- pin ");
    Serial.print(pin);
  }
  Serial.println();
  Serial.println("--------------------------------------------------");
}

void setup() {
  // put your setup code here, to run once:
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  /* Set serial baudrate. */
  Serial.begin(115200);

  // Initialize LowPower library
  LowPower.begin();

  // Get the boot cause
  bootcause_e bc = LowPower.bootCause();

   // LoRaライブラリの初期設定
  lora.begin();
 
  if ((bc == POR_SUPPLY) || (bc == POR_NORMAL)) {
    Serial.println("Example for GPIO wakeup from cold sleep");
    printBootCause(bc);
  } else {
    Serial.println("wakeup from cold sleep");
    printBootCause(bc);
    return;
  }

  // LoRa設定値
  struct LoRaConfigItem_t config = {
    0x0000,   // own_address 0
    0b011,    // baud_rate 9600 bps
    0b10000,  // air_data_rate SF:9 BW:125
    SUBPACKET_200_BITS,
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

  // E220-900T22S(JP)へのLoRa初期設定
  if (lora.InitLoRaModule(config)) {
    SerialMon.printf("init error\n");
    return;
  } else {
    Serial.printf("init ok\n");
  }

  // WRO受信モード(M0=0,M1=1)へ移行する
  SerialMon.printf("switch to normal mode\n");
  lora.SwitchToWORReceivingMode();

}

void lora_interrupt()
{
  Serial.println("Interrupt from LoRa.");
}

void loop() {
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

  attachInterrupt(LoRa_AuxPin, lora_interrupt, FALLING);

  // Cold sleep
  Serial.print("Go to cold sleep...");
  delay(1);
  LowPower.coldSleep();

}
