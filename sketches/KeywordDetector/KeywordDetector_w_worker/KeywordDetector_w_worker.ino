/*
 *  KeywordDetector_w_worker.ino - Demo application of keyword detector used proprietary worker.
 *  Copyright 2021 Sony Semiconductor Solutions Corporation
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

#include <SDHCI.h>
#include <FrontEnd.h>
#include <Recognizer.h>
#include <MemoryUtil.h>
#include <stdio.h>
#include <stdlib.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

SDClass theSD;
FrontEnd *theFrontend;
Recognizer *theRecognizer;

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define ANALOG_MIC_GAIN  0 /* +0dB */

/* Number of input channels
 * Set either 1, 2, or 4.
 */

static const uint8_t  recognizer_channel_number = 1;

/* Audio bit depth
 * Set 16 or 24
 */

static const uint8_t  recognizer_bit_length = 16;
int8_t     g_cur_event  = -1;
int        g_hold_count = 0;
const int  g_hold_time  = 3000;

static const char *filename[] = {
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

#define IMAGE0_WIDTH  140
#define IMAGE0_HEIGHT  40

uint16_t image565[IMAGE0_WIDTH * IMAGE0_HEIGHT];
uint8_t  line_image888[IMAGE0_WIDTH * 3];

bool ErrEnd = false;

static void menu()
{
  printf("=== MENU (input voice command! ) ==============\n");
  printf("saisei kaishi:  play  \n");
  printf("ichiji teishi:  stop  \n");
  printf("mae ni tobu:    next  \n");
  printf("ushiro ni tobu: prev  \n");
  printf("haya okuri:     forward \n");
  printf("haya modoshi:   rewind  \n");
  printf("=====================================\n");
}

/**
 * @brief FrontEnd done callback procedure
 *
 * @param [in] event        AsRecorderEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */
static bool frontend_done_callback(AsMicFrontendEvent ev, uint32_t result, uint32_t sub_result)
{
  printf("frontend_done_callback cb\n");
  return true;
}

static void recognizer_done_callback(RecognizerResult *result)
{
  printf("recognizer cb\n");

  return;
}

static void recognizer_find_callback(AsRecognitionInfo info)
{
 static const char *keywordstrings[] = {
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

  printf("Notify size %d byte\n", info.size);

  int8_t *param = (int8_t *)info.mh.getVa();

  for (uint32_t i = 0; i < info.size / sizeof(int8_t); i++)
    {
      if (param[i] == 1)
        {
          g_cur_event = i;
          g_hold_count = 0;
          printf("%s\n", keywordstrings[i]);
        }
    }  return;
}

static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING) {
    ErrEnd = true;
  }
}

bool bmpDraw(char *filename) {

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
  if ((bmpFile = theSD.open(filename)) == NULL) {
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

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void setup()
{
  bool success;

  /* Display menu */

  Serial.begin(115200);
  menu();

  tft.begin();
  tft.fillScreen(ILI9341_WHITE);
  tft.setRotation(3);

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_RECOGNIZER);

  /* Use SD card */
  theSD.begin();

  /* Start audio system */

  theFrontend = FrontEnd::getInstance();
  theRecognizer = Recognizer::getInstance();

  theFrontend->begin();
  theRecognizer->begin(attention_cb);

  /* Create Objects */

  /* Set rendering clock */

  theFrontend->activate(frontend_done_callback);
  theRecognizer->activate(recognizer_done_callback);

  usleep(100 * 1000);
  /* Set main volume */
  theFrontend->init(recognizer_channel_number, recognizer_bit_length, 384,
                    ObjectConnector::ConnectToRecognizer, AsMicFrontendPreProcSrc, "/mnt/sd0/BIN/SRC");
//                    ObjectConnector::ConnectToRecognizer, AsMicFrontendPreProcSrc, "/mnt/spif/BIN/SRC");

  theRecognizer->init(recognizer_find_callback,
                      "/mnt/sd0/BIN/RCGPROC");
//                      "/mnt/spif/BIN/RCGPROC");

  /* Init Recognizer DSP */
  struct InitRcgProc : public CustomprocCommand::CmdBase
  {
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint32_t param4;
  };

  static InitRcgProc s_initrcgproc;
  s_initrcgproc.param1 = 500;
  s_initrcgproc.param2 = 600;
  s_initrcgproc.param3 = 0;
  s_initrcgproc.param4 = 0;

  theRecognizer->initRecognizerDsp((uint8_t *)&s_initrcgproc, sizeof(s_initrcgproc));

  /* Set Recognizer DSP */

  struct SetRcgProc : public CustomprocCommand::CmdBase
  {
    uint32_t param1;
    int32_t param2;
    int32_t param3;
    int32_t param4;
  };

  static SetRcgProc s_setrcgproc;
  s_setrcgproc.param1 = true;
  s_setrcgproc.param2 = -1;
  s_setrcgproc.param3 = -1;
  s_setrcgproc.param4 = -1;

  theRecognizer->setRecognizerDsp((uint8_t *)&s_setrcgproc, sizeof(s_setrcgproc));

  /* Start Recognizer */

 ledOn(LED0);

  theFrontend->start();
  theRecognizer->start();
  puts("Recognizing Start!");

}

void loop()
{

  /* Fatal error */
  if (ErrEnd) {
    printf("Error End\n");
    goto stop_recognizer;
  }

  static int8_t pre_event = -1;

  if(g_cur_event != pre_event){
    tft.fillScreen(ILI9341_WHITE);
    if(filename[g_cur_event] != ""){
      if( bmpDraw( filename[g_cur_event] ) ){
        tft.drawRGBBitmap((320-120)/2, (240-40)/2, image565, IMAGE0_WIDTH, IMAGE0_HEIGHT);
      }
    }
    pre_event = g_cur_event;
  }

//  g_event_char ='*';

  /* Processing in accordance with the state */

  /* This sleep is adjusted by the time to read the audio stream file.
     Please adjust in according with the processing contents
     being processed at the same time by Application.
   */

  usleep(1000);

  return;

stop_recognizer:
  printf("Exit recognizer\n");
  exit(1);
}
