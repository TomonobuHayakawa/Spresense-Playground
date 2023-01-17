/*
 *  Indicator.c - Demo application of keyword detector without worker on MainCore.
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

#include "Indicator.h"
#include "Adafruit_ILI9341.h"

bool Indicator::bmpDraw(char *filename)
{
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t startTime = millis();

  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = theSD->open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); Serial.println(bmpDepth);

      if((bmpDepth != 24) || (read32(bmpFile) != 0)){
        return false;
      }

      if((bmpWidth != 140) || (bmpHeight != 40)){
        return false;
      }

/*      if(bmpHeight < 0) {
        return false;
      }*/

      Serial.print("Image size: ");
      Serial.print(bmpWidth);
      Serial.print('x');
      Serial.println(bmpHeight);

      uint16_t pos = 0;
      bmpFile.seek(bmpImageoffset);
      for (int y = 1; y <= bmpHeight; y++) { // bottom to top
        bmpFile.read(line_image888, sizeof(line_image888));
        pos = (bmpHeight - y) *  bmpWidth;
        for (int x = 0; x < bmpWidth * 3; x += 3, pos++) {
          image565[pos] =  (uint16_t) (line_image888[x] & 0xf8) >> 3;
          image565[pos] |= (uint16_t) (line_image888[x+1] & 0xfC) << 3;
          image565[pos] |= (uint16_t) (line_image888[x+2] & 0xf8) << 8;
        }
      }

      Serial.print("Loaded in ");
      Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
  } else {
    return false;
  }
  bmpFile.close();
  return true;
}

uint16_t Indicator::read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t Indicator::read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void Indicator::begin()
{
  tft.begin();
  tft.fillScreen(ILI9341_WHITE);
  tft.setRotation(3);
}

void Indicator::draw(uint32_t flags)
{
  if (flags == 0) {
    if (hold_count < hold_time) {
      hold_count++;
    } else {
      tft.fillScreen(ILI9341_WHITE);
      hold_count = 0;
    }
    return;
  }
  
  for (uint32_t i = 0; i < numKeywords; i++) { 
    if (flags & (0x1 << i)) {
      if(cur_event != i){
        tft.fillScreen(ILI9341_WHITE);
        if(filename[i] != ""){
          if( bmpDraw( filename[i] ) ){
            tft.drawRGBBitmap((320-120)/2, (240-40)/2, image565, IMAGE0_WIDTH, IMAGE0_HEIGHT);
          }
        }
      puts(keywordstrings[i]);
      cur_event =i;
      hold_count = 0;
      }
    }
  }
}
