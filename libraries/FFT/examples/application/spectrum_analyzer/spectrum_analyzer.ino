/*
 *  spectrum_analyzer.ino - PCM capture and FFT example application
 *  Copyright 2018 Sony Semiconductor Solutions Corporation
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

#include <Audio.h>

AudioClass *theAudio;

/*-----------------------------------------------------------------*/
/*
 * FFT parameters
 */
#include "fft_rpc.h"

#define DSPFFT_LIBFILE "/mnt/sd0/BIN/DSPFFT"
#define FFT_LENGTH   512 // ex.) 128, 256, 1024

/*-----------------------------------------------------------------*/
/*
 * LCD display
 */
#define HAVE_LCD

#ifdef HAVE_LCD

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define _cs -1
#define _dc 9
#define _rst 8

Adafruit_ILI9341 tft = Adafruit_ILI9341(_cs, _dc, _rst);

/* Text position (rotated coordinate) */
#define TX 35
#define TY 210

/* Graph area */
#define GRAPH_WIDTH  128 // Spectrum value
#define GRAPH_HEIGHT 256 // Sample step

/* Graph position */
#define GX ((240 - GRAPH_WIDTH) / 2)
#define GY ((320 - GRAPH_HEIGHT) / 2)

int range = 80;

/*-----------------------------------------------------------------*/
unsigned long showSpectrum(float *pResult)
{
  static uint16_t frameBuf[GRAPH_HEIGHT][GRAPH_WIDTH];
  unsigned long start;
  int val;
  int i, j;

  start = micros();

  for (i = 0; i < GRAPH_HEIGHT; i++) {
    //printf("%1.3f\n", pResult[i]);
    val = (int)(pResult[i] * range);
    val = (val > GRAPH_WIDTH) ? GRAPH_WIDTH : val;
    for (j = 0; j < GRAPH_WIDTH; j++) {
      frameBuf[i][j] = (j <= val) ? ILI9341_GREEN : ILI9341_BLACK;
    }
  }
  //tft.fillImage(GX, GY, GRAPH_WIDTH, GRAPH_HEIGHT, (uint16_t*)frameBuf);
  tft.drawRGBBitmap(GX, GY, (uint16_t*)frameBuf, GRAPH_WIDTH, GRAPH_HEIGHT);
  return micros() - start;
}
#else
unsigned long showSpectrum(float *pResult) { (void)pResult; return 0; }
#endif

/*-----------------------------------------------------------------*/
static int fft_monitor(int argc, FAR char *argv[])
{
  int ret;
  uint32_t addr;

  (void)argc;
  (void)argv;

  for (; ; ) {
    ret = fft_stream_output(0, &addr);
    if (ret < 0) {
      printf("ERROR: ret=%d\n", ret);
      break;
    }

    //printf("FFT execution result: ret=%d addr=0x%08x\n", ret, addr);
    showSpectrum((float *)addr);
  }

  return ret;
}

/*-----------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);
  while (!Serial);

#ifdef HAVE_LCD
  Serial.println("LCD settings");
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);
  tft.setCursor(TX, TY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("FFT Spectrum Analyzer");
  tft.setRotation(0);
  tft.fillRect(GX - 5, GY - 5, GRAPH_WIDTH + 10, GRAPH_HEIGHT + 10, ILI9341_BLUE);
#endif

  Serial.println("Init Audio Library");
  theAudio = AudioClass::getInstance();
  theAudio->begin();

  Serial.println("Init Audio Recorder");
  /* Select input device as AMIC */
  //theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC, 180);
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);

  /* Set PCM capture */
  theAudio->initRecorder(AS_CODECTYPE_PCM, "/mnt/sd0/BIN", AS_SAMPLINGRATE_48000, AS_CHANNEL_MONO);
  //theAudio->initRecorder(AS_CODECTYPE_PCM, "/mnt/sd0/BIN", AS_SAMPLINGRATE_16000, AS_CHANNEL_MONO);

  /* Load FFT dedicated DSP */
  Serial.println("FFT settings");
  sleep(2); // workaround to wait until SD card is mounted
  load_library(DSPFFT_LIBFILE);

  /* Init FFT parameter for input data */
  fft_stream_init(FFT_LENGTH, 0, TYPE_SHORT);

  /* Set FFT window */
  fft_window(FFT_WIN_HAMMING);

  /* Create FFT monitor task */
  task_create("fft_monitor", 110, 1024, fft_monitor, NULL);
  //task_create("fft_monitor", 180, 1024, fft_monitor, NULL);

  Serial.println("Rec start!");
  theAudio->startRecorder();

  Serial.println(String("range: ") + range);
  Serial.println("range? ");
}

/*-----------------------------------------------------------------*/
void loop()
{
  static const int32_t buffer_sample = 768; // ex.) 256, 1024
  static const int32_t buffer_size = buffer_sample * sizeof(int16_t);
  static char  buffer[buffer_size];
  uint32_t read_size;

  /* Read frames to record in buffer */
  int err = theAudio->readFrames(buffer, buffer_size, &read_size);

  if (err != AUDIOLIB_ECODE_OK && err != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
      printf("Error err = %d\n", err);
      sleep(1);
      theAudio->stopRecorder();
      exit(1);
    }

  if ((read_size != 0) && (read_size == buffer_size)) {
      fft_stream_input(buffer, read_size);
  } else {
      usleep(1);
  }
  
  String str;
  bool hasInput = false;

  while (Serial.available() > 0) {
      hasInput = true;
      str += (char)Serial.read();
  }
  if (hasInput) {
    int tmp = str.toInt();
    if ((0 < tmp) && (tmp < 500)) {
      range = tmp;
      Serial.println(String("range: ") + range);
      Serial.println("range? ");
    }
  }
}
