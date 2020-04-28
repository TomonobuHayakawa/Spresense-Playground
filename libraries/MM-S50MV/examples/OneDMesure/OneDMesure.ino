#include "MM-S50MV.h"

void setup()
{
  Serial.begin(115200);
  MMS50MV.begin();

  MMS50MV.skip(256);
  delay(500);
  MMS50MV.set(0, 0xff); // sync mode.
  delay(500);
  MMS50MV.skip(256);
  MMS50MV.sync();       // sync.
  MMS50MV.set(0, 0);    // normal mode. 
  delay(500);

  MMS50MV.set(0x10, 0); // 256frames/s
  delay(500);
  MMS50MV.skip(256);
}

void loop()
{
  static  int ledr = 0;
  static  int ledg = 0;
  static  int ledb = 0;

  int32_t dis = MMS50MV.get();
  if(dis<0){
    return;
  }

// MMS50MV.led(ledr,ledg,ledb);
    
  printf("dis=%d(mm)\n",dis);

  // LED color
  int i = dis & 0xff;
  switch (dis >> 8) {
    default:
      ledr = ledg = ledb = 0;
      break;
    case    6:
      ledr = 0;
      ledg = 0;
      ledb = 255 - i;
      break;
    case    5:
      ledr = 0;
      ledg = 255 - i;
      ledb = 255;
      break;
    case    4:
      ledr = 0;
      ledg = 255;
      ledb = i;
      break;
    case    3:
      ledr = 255 - i;
      ledg = 255;
      ledb = 0;
      break;
    case    2:
      ledr = 255;
      ledg = i;
      ledb = 0;
      break;
    case    1:
      ledr = 255;
      ledg = 0;
      ledb = 255 - i;
      break;
    case    0:
      ledr = 255;
      ledg = 255 - i;
      ledb = 255;
      break;
    }

//    sleep(1);
}
