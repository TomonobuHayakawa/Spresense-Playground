#include "MM-S50MV.h"

void setup()
{
  Serial.begin(115200);
  MMS50MV.begin();

  MMS50MV.skip(256);
  delay(500);
  MMS50MV.set(MMS50MV_CMD_MODE, MMS50MV_MODE_SYNC);  // sync mode.
  delay(500);
  MMS50MV.skip(256);
  MMS50MV.sync();                                    // sync.
  MMS50MV.set(MMS50MV_CMD_MODE, MMS50MV_MODE_NOMAL); // normal mode. 
  delay(500);

//  MMS50MV.set(MMS50MV_RATE_LOW, 0x20); // 32frames/s
  MMS50MV.set(MMS50MV_RATE_LOW, 0x10); // 8frames/s
  delay(500);
  MMS50MV.set(MMS50MV_CMD_DISTANCE, MMS50MV_DIST_LONG); // long distance
  delay(500);
  MMS50MV.skip(256);
}

void loop()
{
  static  int32_t data[8][4];
  static  int ledr = 0;
  static  int ledg = 0;
  static  int ledb = 0;

//  printf("range=%d\n",MMS50MV.get1d());

  int32_t* ptr = (int32_t*)data;
  if(!MMS50MV.get3d(ptr)){
    puts("error!");
    return;
  }

  for(int j=0;j<8;j++){
    for(int i=0;i<4;i++){
//      if(data[j][i]<300){
        printf("range[%d][%d]=%d\n",j,i,data[j][i]);
//      }
    }
  }
  
// MMS50MV.led(ledr,ledg,ledb);
   printf("\n\n\n");
   usleep(10*1000);
}
