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

#include <MP.h>
#include <string.h>

#include "CoreInterface.h"

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// Graphics parameters.
// For the Adafruit shield, these are the default.

#define TFT_DC 44
#define TFT_CS -1
#define TFT_RST 2

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

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
  tft.begin(4000000);
  
/*  screen_width = tft.width();
  screen_height = tft.height(); */

  tft.fillScreen(ILI9341_BLACK);

  draw_title(ILI9341_WHITE);
  Serial.println("Done!");

}

static uint16_t pitches[8][2] = {
 {48,ILI9341_BLUE},
 {48,ILI9341_BLUE},
 {48,ILI9341_BLUE},
 {48,ILI9341_BLUE},
 {60,ILI9341_BLUE},
 {60,ILI9341_BLUE},
 {60,ILI9341_BLUE},
 {60,ILI9341_BLUE}
};

static int sequence_number = 0;

void loop()
{
  int      ret;
  int8_t   rcvid;
  uint32_t data;
  
  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &data, 1);
  if(ret < 0){
    return;
  }

  if((rcvid / 10 ) == 2){
    int channel = rcvid % 10;
    if(data){
      pitches[channel][1] = ILI9341_WHITE;
    }else{
      pitches[channel][1] = ILI9341_BLUE;
    }
    draw_pitch(pitches[channel][0], channel, pitches[channel][1]);
  }
  
  if((rcvid / 10 ) == 1){
    int channel = rcvid % 10;
    pitches[channel][0] = data;
    draw_pitch(pitches[channel][0], channel, pitches[channel][1]);
  }
  
  if((rcvid / 10 ) == 3){
    int sw = rcvid % 10;
//    if(sw == 0) volume = data;
    if(sw == 1) draw_tempo(data,   ILI9341_WHITE);
//    if(sw == 2) draw_seq  (data%4, ILI9341_WHITE);
  }

  if((rcvid / 10 ) == 4){
    int sw = rcvid % 10;
    sequence_number = (sequence_number+1)%5;
    if(sw == 2) draw_seq  (sequence_number, ILI9341_WHITE);
  }

}

void draw_tempo(uint8_t val, uint16_t color)
{
  const int h_offset = 50;
  const int w_offset = 270;
  tft.fillRect(w_offset, h_offset, 25, 20, ILI9341_BLACK),
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(w_offset, h_offset);
  tft.print(50-val);
}

void draw_pitch(uint8_t val, uint8_t ch, uint16_t color)
{
  const int h_offset = 115;
  const int w_offset = 20;

  tft.fillRect(w_offset+ch*35, h_offset, 25, 20, ILI9341_BLACK),
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(w_offset+ch*35, h_offset);
  tft.print(val);
}

void draw_seq(uint8_t val, uint16_t color)
{
  const int h_offset = 140;
  const int w_offset = 160;
  tft.fillRect(w_offset, h_offset, 25, 30, ILI9341_BLACK),
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(w_offset, h_offset);
  tft.print(val);
}

void draw_title(uint16_t color)
{
  tft.setRotation(3);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(80, 3);
  tft.println("SprSynth");

  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(180, 50);
  tft.print("Tempo = ");
  draw_tempo(50,ILI9341_WHITE);

  tft.setCursor(20, 60);
  tft.print("Pitch");

  tft.setTextColor(color);
  tft.setTextSize(1);
  for(int i=0;i<8;++i){
    tft.setCursor(22+i*35, 93);
    tft.print("ch");
    tft.setCursor(37+i*35, 93);
    tft.print(i);
  }

  for(int i=0;i<8;++i){
    draw_pitch(pitches[i][0], i, pitches[i][1]);
  }

  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(20, 140);
  tft.print("Sequence = ");
  draw_seq(sequence_number,ILI9341_WHITE);

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
