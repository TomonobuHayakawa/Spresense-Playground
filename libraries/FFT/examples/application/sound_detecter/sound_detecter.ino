/*
 *   - Sound Detector with FFT example application
 *  Copyright 2018 Sonsound_detecter.inoy Semiconductor Solutions Corporation
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
#include <SD.h>

/*-----------------------------------------------------------------*/
/*
 * FFT parameters
 */
#include "fft_rpc.h"

#define SAMPLING_RATE   48000 // ex.) 48000, 16000

#define DSPFFT_LIBFILE "/mnt/sd0/BIN/DSPFFT"
#define FFT_LENGTH   512 // ex.) 128, 256, 1024
#define FFT_SHIFT    256  // ex.) 0, 128, 256

/*-----------------------------------------------------------------*/
/*
 * Detector parameters
 */
#define POWER_THRESHOLD       20  // Power^2
#define LENGTH_THRESHOLD      20  // 20ms
#define INTERVAL_THRESHOLD    100 // 100ms

#define BOTTOM_SAMPLING_RATE  1000 // 1kHz
#define TOP_SAMPLING_RATE     1500 // 1.5kHz

#define FS2BAND(x)            (x*FFT_LENGTH/SAMPLING_RATE)
#define BOTTOM_BAND           (FS2BAND(BOTTOM_SAMPLING_RATE))
#define TOP_BAND              (FS2BAND(TOP_SAMPLING_RATE))

#define MS2FRAME(x)           ((x*SAMPLING_RATE/1000/(FFT_LENGTH-FFT_SHIFT))+1)
#define LENGTH_FRAME          MS2FRAME(LENGTH_THRESHOLD)
#define INTERVAL_FRAME        MS2FRAME(INTERVAL_THRESHOLD)

/*-----------------------------------------------------------------*/
/*
 * Audio parameters
 */
AudioClass *theAudio;
File myFile;

const int32_t recoding_frames = 2000;
#if(SAMPLING_RATE == 48000)
const int32_t buffer_sample = 768;
#elif(SAMPLING_RATE == 16000)
const int32_t buffer_sample = 256;
#endif
const int32_t buffer_size = buffer_sample*sizeof(int16_t);

char buffer[buffer_size];

/*-----------------------------------------------------------------*/
/*
 * Detector functions
 */
bool detect_sound(int bottom, int top, float* pdata )
{
  static int continuity = 0;
  static int interval = 0;

  if(bottom > top) return false;

  if(interval> 0){ // Do not detect in interval time. 
    interval--;
    continuity=0;
    return false;
  }
  
  for(int i=bottom;i<=top;i++){
    if(*(pdata+i) > POWER_THRESHOLD){ // find sound.
//      printf("!!%2.8f\n",*(pdata+i));
      continuity++;
//      printf("con=%d\n",continuity);
      if(continuity > LENGTH_FRAME){ // length is enough.
        interval = INTERVAL_FRAME;
        return true;
      }else{
//      puts("continue sound");
        return false;  
      }
    }
  }
  continuity=0;
  return false;  
}


/*-----------------------------------------------------------------*/
/**
 *  @brief Setup audio device to capture PCM stream
 *
 *  Select input device as analog microphone, AMIC  <br>
 *  Set PCM capture sapling rate parameters to 48 kb/s <br>
 *  Set channel number 4 to capture audio from 4 microphones simultaneously <br>
 *  System directory "/mnt/sd0/BIN" will be searched for PCM codec
 */
void setup()
{
  theAudio = AudioClass::getInstance();
  theAudio->begin();

  SD.begin();
//  myFile = SD.open("Sound.pcm", FILE_WRITE); // temtative for SD car initialize. And Debug.

  puts("initialization Audio Library");


  /* Init FFT parameter for input data */
  load_library(DSPFFT_LIBFILE);
  fft_stream_init(FFT_LENGTH, 0, TYPE_SHORT);

  /* Set FFT window */
  fft_window(FFT_WIN_HAMMING);

  puts("initialization FFT Library");

  /* Select input device as AMIC */
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC,180);

  /*
   * Set PCM capture sapling rate parameters to 48 kb/s. Set channel number 4
   * Search for PCM codec in "/mnt/sd0/BIN" directory
   */
#if(SAMPLING_RATE == 48000)
  theAudio->initRecorder(AS_CODECTYPE_PCM, "/mnt/sd0/BIN", AS_SAMPLINGRATE_48000, AS_CHANNEL_MONO);
#elif(SAMPLING_RATE == 16000)
  theAudio->initRecorder(AS_CODECTYPE_PCM, "/mnt/sd0/BIN", AS_SAMPLINGRATE_16000, AS_CHANNEL_MONO);
#else
    static_assert(0);
#endif
  puts("Init Recorder!");

  theAudio->startRecorder();
  puts("Start Recorder!");
}

/*-----------------------------------------------------------------*/
void loop() {

  static int cnt = 0;
  uint32_t read_size;

  float result_buf[FFT_LENGTH*sizeof(float)];

  /* recording end condition */
  if (cnt > recoding_frames)
    {
      puts("End Recording");
      theAudio->stopRecorder();
//      myFile.close();
      exit(1);
    }

//puts("loop!");

  /* Read frames to record in buffer */
  int err = theAudio->readFrames(buffer, buffer_size, &read_size);

  if (err != AUDIOLIB_ECODE_OK && err != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA)
    {
      printf("Error End! =%d\n",err);
      sleep(1);
      theAudio->stopRecorder();
      exit(1);
    }

    /* File write for debug. */
//    int ret = myFile.write((uint8_t*)buffer, read_size);

  /* The actual signal processing will be coding here.
     For example, prints capture data. */
     
  if ((read_size != 0) && (read_size == buffer_size)) /* Frame size is fixed as it is capture */
  {
    fft_stream_input(buffer, read_size);
  }

//  float* result_p;
  uint32_t result_p;
/*  err = fft_stream_output(1, &result_p);
  printf("Error?! =%d\n",err);*/
  
  while(fft_stream_output(1, &result_p) > 0)
  {
    if(detect_sound(BOTTOM_BAND,TOP_BAND,(float *)result_p))
    {
      puts("Find sound!");
    }
  }
  cnt++;
}
