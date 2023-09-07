#include <MP.h>
#include "Adafruit_ILI9341.h"

#define TFT_DC 9
#define TFT_RST 8
#define TFT_CS -1

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

#include <MPMutex.h>

MPMutex mutex(MP_MUTEX_ID0);

void setup() {
  Serial.begin(115200);
  int ret = 0;
  ret = MP.begin();

  /* Initialize MP library */

  if (ret < 0) {
    printf("Subcore initialization error\n");
  }
  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

//  tft.begin();
  tft.begin(40000000);  
  tft.setRotation(3);
}

void loop() {
  int8_t msgid;
  uint16_t* imgbuf = NULL;

  int ret = MP.Recv(&msgid, &imgbuf);

  int lock;
  do {
    lock = mutex.Trylock();
  } while (lock != 0);

  if (ret > 0) {
    // Serial.println("start drawing");
    tft.drawRGBBitmap(0, 0, imgbuf, 320, 240);
    // Serial.println("end drawing");
    mutex.Unlock();
  } else {
    mutex.Unlock();
  }
}