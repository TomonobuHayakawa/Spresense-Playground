/*
 *  Indicator.h - Demo application of keyword detector without worker on MainCore.
 *  Copyright 2022 Sony Semiconductor Solutions Corporation
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
#ifndef __INDICATOR_H__
#define __INDICATOR_H__

#include <SDHCI.h>
#include <SPI.h>

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

class Indicator
{
public:

  Indicator(SDClass* sd) :
    theSD(sd)
  {
  }
  
  void begin();
  void draw(uint32_t flags);

private:

  #define TFT_DC 9
  #define TFT_CS 10
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

  SDClass* theSD;

  #define IMAGE0_WIDTH  140
  #define IMAGE0_HEIGHT  40

  uint16_t image565[IMAGE0_WIDTH * IMAGE0_HEIGHT];
  uint8_t  line_image888[IMAGE0_WIDTH * 3];

  int8_t     cur_event  = -1;
  int        hold_count = 0;
  const int  hold_time  = 3000;

  static const int  numKeywords =15;

  const char *keywordstrings[numKeywords] = {
    "HDMI 1",
    "HDMI 2",
    "HDMI 3",
    "HDMI 4",
    "Start Play",
    "Pause",
    "Next",
    "Prev",
    "Power ON",
    "Power OFF",
    "Fast Forward",
    "Rewind",
    "Mute OFF",
    "Mute ON",
    "Volume Control"
  };

  const char *filename[numKeywords] = {
    "",
    "",
    "",
    "",
    "start.bmp",
    "pause.bmp",
    "next.bmp",
    "back.bmp",
    "",
    "",
    "ff.bmp",
    "fb.bmp",
    "",
    "",
    ""
};

  bool bmpDraw(char *filename);
  uint16_t read16(File &f);
  uint32_t read32(File &f);

};

#endif /* __INDICATOR_H__ */
