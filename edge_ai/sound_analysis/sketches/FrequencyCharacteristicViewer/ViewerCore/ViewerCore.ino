/*
 *  ViewCore.ino - Demo application.
 *  Copyright 2023 T.Hayakawa
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>

#include <SPI.h>

#include "CoreInterface.h"

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// Graphics parameters.
// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

/* Multi-core parameters */
const int subcore = 2;

/*int screen_width;
int screen_height; */

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define LEFT_SPACE   30
#define RIGHT_SPACE  10
#define TOP_SPACE    40
#define BOTTOM_SPACE 10

#define GRAPH_WIDTH  (SCREEN_WIDTH - LEFT_SPACE - RIGHT_SPACE)
#define GRAPH_HEIGHT (SCREEN_HEIGHT - TOP_SPACE - BOTTOM_SPACE)

#define FRAME_NUMBER 2


struct FrameInfo{
  float buffer[GRAPH_WIDTH];
  bool renable;
  bool wenable;
  FrameInfo():renable(false),wenable(true){}
};

FrameInfo frame_buffer[FRAME_NUMBER];



/**
 *  @brief Setup audio device to capture PCM stream
 *
 *  Select input device as microphone <br>
 *  Set PCM capture sapling rate parameters to 48 kb/s <br>
 *  Set channel number 4 to capture audio from 4 microphones simultaneously <br>
 */
void setup()
{
  Serial.begin(115200);

  /* Initialize MP library */
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }
  
  /* receive with non-blocking */
  MP.RecvTimeout(1);

  puts("Graph Start");
  tft.begin(40000000);
  
/*  screen_width = tft.width();
  screen_height = tft.height(); */

  tft.fillScreen(ILI9341_BLACK);

  draw_frame(ILI9341_WHITE);
  Serial.println("Done!");

  int proter = task_create("Proter", 70, 0x400, proter_task, (FAR char* const*) 0);

}

int proter_task(int argc, FAR char *argv[])
{
  while(1){
    for(int i=0;i<FRAME_NUMBER;i++){
      if(frame_buffer[i].renable){
        frame_buffer[i].renable = false;
        prot_data(frame_buffer[i].buffer,ILI9341_RED);
        frame_buffer[i].wenable = true;
      }
    }
  }
}

void loop()
{
  int      ret;
  int8_t   rcvid;
  Request  *request;
  
  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &request);
  if (ret >= 0) {
     write_buffer(request->buffer);
  }

}

int write_buffer(float* data)
{
  for(int i=0;i<FRAME_NUMBER;i++){
    if(frame_buffer[i].wenable){
      frame_buffer[i].wenable = false;
      memcpy(frame_buffer[i].buffer,data,GRAPH_WIDTH*sizeof(float)/2);
      frame_buffer[i].renable =true;
      return 0;
    }
  }
  return 1;
}

void draw_frame(uint16_t color)
{
  tft.drawRect(BOTTOM_SPACE, LEFT_SPACE, GRAPH_HEIGHT, GRAPH_WIDTH, color);
}

inline float f_max(float x, float y) { return((x>y)?x:y); }
inline float f_min(float x, float y) { return((x<y)?x:y); }

void prot_data(float* data, uint16_t color)
{
  tft.fillRect(BOTTOM_SPACE+1, LEFT_SPACE+1, GRAPH_HEIGHT-2, GRAPH_WIDTH-2, ILI9341_BLACK);
  for(int i=1;i<GRAPH_WIDTH/2-2;i++){
    tft.drawLine(BOTTOM_SPACE+f_min((*data)+1,(GRAPH_HEIGHT-2)),LEFT_SPACE+i*2,BOTTOM_SPACE+f_min(*(data+1)+1,GRAPH_HEIGHT-2),LEFT_SPACE+(i+1)*2, color);
    data++;
  }
}

void errorLoop(int num)
{
  int i;

  while (1) {
    for (i = 0; i < num; i++) {
      ledOn(LED0);
      delay(300);
      ledOff(LED0);
      delay(300);
    }
    delay(1000);
  }
}
