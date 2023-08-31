/*
 *  Main.ino - Demo application .
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
#include <SDHCI.h>

#include <FrontEnd.h>
#include <MemoryUtil.h>

#include "CoreInterface.h"

#define  CONSOLE_BAUDRATE  115200

//#define  ENABLE_DATA_COLLECTION
//#define  VIEW_RAW_DATA

FrontEnd *theFrontEnd;
SDClass theSD;

// Audio parameters.
static const int32_t channel_num  = AS_CHANNEL_MONO;
static const int32_t bit_length   = AS_BITLENGTH_16;
//static const int32_t frame_sample = 1024;
static const int32_t frame_sample = 512;
static const int32_t frame_size   = frame_sample * (bit_length / 8) * channel_num;
static const int32_t mic_gain     = 160;

static CMN_SimpleFifoHandle simple_fifo_handle;
static const int32_t fifo_size  = frame_size * 20;
static uint32_t fifo_buffer[fifo_size / sizeof(uint32_t)];

//static const int32_t proc_size  = frame_size;
static const int32_t proc_size  = 4096*2;
static uint8_t proc_buffer[proc_size];

bool isEnd = false;
bool ErrEnd = false;

/* Multi-core parameters */
const int proc_core = 1;
const int conn_core = 2;

/**
 * @brief Frontend attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

void frontend_attention_cb(const ErrorAttentionParam *param)
{
  puts("Attention!");

  if (param->error_code >= AS_ATTENTION_CODE_WARNING) {
    ErrEnd = true;
  }
}

/**
 * @brief Frontend done callback procedure
 *
 * @param [in] event        AsMicFrontendEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */

static bool frontend_done_callback(AsMicFrontendEvent ev, uint32_t result, uint32_t sub_result)
{
  UNUSED(ev);
  UNUSED(result);
  UNUSED(sub_result);
  return true;
}

/**
 * @brief Frontend pcm capture callback procedure
 *
 * @param [in] pcm          PCM data structure
 *
 * @return void
 */
static void frontend_pcm_callback(AsPcmDataParam pcm)
{
  if (pcm.is_end) {
    isEnd = true;
  }

  if (!pcm.is_valid) {
    puts("Invalid data !");
    return;
  }

  if (CMN_SimpleFifoGetVacantSize(&simple_fifo_handle) < pcm.size) {
    puts("Simple FIFO is full !");
    return;
  }

  if (CMN_SimpleFifoOffer(&simple_fifo_handle, (const void*)(pcm.mh.getPa()), pcm.size) == 0) {
    puts("Simple FIFO is full !");
    return;
  }

  return;
}

/**
 *  @brief Setup audio device to capture PCM stream
 *
 *  Select input device as microphone <br>
 *  Set PCM capture sapling rate parameters to 48 kb/s <br>
 *  Set channel number 4 to capture audio from 4 microphones simultaneously <br>
 */
char server_cid = 0;

void setup()
{
  Serial.begin(CONSOLE_BAUDRATE);

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_RECORDER);

  if (CMN_SimpleFifoInitialize(&simple_fifo_handle, fifo_buffer, fifo_size, NULL) != 0) {
    print_err("Fail to initialize simple FIFO.\n");
    exit(1);
  }

  /* Launch SubCore */
  int ret = MP.begin(proc_core);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }
  
  /* Launch SubCore */
  ret = MP.begin(conn_core);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }

  /* receive with non-blocking */
  MP.RecvTimeout(10);

#ifdef ENABLE_DATA_COLLECTION
  /* Use SD card */
  theSD.begin();
#endif /* ENABLE_DATA_COLLECTION */

  /* start audio system */
  theFrontEnd = FrontEnd::getInstance();
  theFrontEnd->begin(frontend_attention_cb);

  puts("initialization FrontEnd");

  /* Set capture clock */
  theFrontEnd->setCapturingClkMode(FRONTEND_CAPCLK_NORMAL);

  /* Activate Objects. Set output device to Microphone */
  theFrontEnd->activate(frontend_done_callback);

  usleep(100 * 1000); /* waiting for Mic startup */

  /* Initialize of capture */
  AsDataDest dst;
  dst.cb = frontend_pcm_callback;

  theFrontEnd->init(channel_num,
                    bit_length,
                    frame_sample,
                    AsDataPathCallback,
                    dst);

  theFrontEnd->setMicGain(mic_gain);

sleep(10);
  theFrontEnd->start();
  puts("Capturing Start!");

  int store = task_create("Store", 70, 0x1000, store_task, (FAR char* const*) 0);

}

#define FRAME_NUMBER 2

struct FrameInfo{
  float buffer[proc_size];
  uint16_t buffer_i[proc_size];
  int sample;
  bool renable;
  bool wenable;
  FrameInfo():sample(0),renable(false),wenable(true){}
};

FrameInfo frame_buffer[FRAME_NUMBER];

/**
 * @brief Capture a frame of PCM data into buffer for signal processing
 */
bool execute_aframe()
{
  int8_t sndid = 100; /* user-defined msgid */
  static Request request;

  size_t size = CMN_SimpleFifoGetOccupiedSize(&simple_fifo_handle);

  if (size > 0) {
    if (size > proc_size) {
      size = (size_t)proc_size;
    }

    if (CMN_SimpleFifoPoll(&simple_fifo_handle, (void*)proc_buffer, size) == 0) {
      printf("ERROR: Fail to get data from simple FIFO.\n");
      return false;
    }

    request.buffer  = proc_buffer;
    request.sample  = size / 2;

#ifndef VIEW_RAW_DATA

    MP.Send(sndid, &request, proc_core);

#else 

    for(int i=0;i<FRAME_NUMBER;i++){
      if(frame_buffer[i].wenable){
        frame_buffer[i].wenable = false;
        frame_buffer[i].sample = size/2;
        memcpy(frame_buffer[i].buffer_i,proc_buffer,size);
//        for(int j = 0; j<400; j++) frame_buffer[i].buffer_i[j] = j*100;
        frame_buffer[i].renable = true;
        break;
      }
    }
    
#endif /* WIEW_RAW_DATA */

  }

  return true;
}

const int collection_number = 500;

void send_data(byte* data,int sample)
{
  static Request request;
  int8_t sndid = NOMAL_REQUEST_ID;

  request.buffer  = data;
  request.sample  = sample;
  MP.Send(sndid, &request, conn_core);

}

int store_task(int argc, FAR char *argv[])
{
  static int gCounter = 0;
  char filename[16] = {};

  while(1){

    if(gCounter>=collection_number) while(1){sleep(1);};

    for(int i=0;i<FRAME_NUMBER;i++){
      if(frame_buffer[i].renable){
        frame_buffer[i].renable = false;

#ifdef ENABLE_DATA_COLLECTION
        sprintf(filename, "Data/data%03d.csv", gCounter++);
        puts(filename);
        if (theSD.exists(filename)) theSD.remove(filename);
        File myFile = theSD.open(filename, FILE_WRITE);
        myFile.write((uint8_t*)frame_buffer[i].buffer,proc_size);
        myFile.close();
        gCounter++;
#endif /* ENABLE_DATA_COLLECTION */

#ifdef VIEW_RAW_DATA

        send_data((uint8_t*)frame_buffer[i].buffer_i,frame_buffer[i].sample);

#else
        for(int j=0;j<frame_buffer[i].sample;j++){
          frame_buffer[i].buffer_i[j] = (uint16_t)(frame_buffer[i].buffer[j]*1000);
        }
        send_data((uint8_t*)frame_buffer[i].buffer_i,frame_buffer[i].sample);

#endif /* WIEW_RAW_DATA */

//        usleep(20000*1000);
        
        frame_buffer[i].sample = 0;
        frame_buffer[i].wenable = true;

        if(gCounter==collection_number){
          theFrontEnd->stop();
        }
      }      
    }
  }
}

/**
 * @brief Main loop
 */

void loop() {

  static Request request;
  
  /* Execute audio data */
  if (!execute_aframe()) {
    puts("Capturing Error!");
    ErrEnd = true;
  }

  int8_t   rcvid;
  Result*  result;
  int8_t   sndid = NOMAL_REQUEST_ID;

  int ret = MP.Recv(&rcvid, &result, proc_core);
  if(ret >= 0){

    for(int i=0;i<FRAME_NUMBER;i++){
      if(frame_buffer[i].wenable){
        frame_buffer[i].wenable = false;
        frame_buffer[i].sample = result->sample;
        memcpy(frame_buffer[i].buffer,result->buffer,result->sample*2);
        frame_buffer[i].renable = true;
        break;
      }
    }
  }
usleep(1);
  if (isEnd) {
    theFrontEnd->stop();
    goto exitCapturing;
  }

  if (ErrEnd) {
    puts("Error End");
    theFrontEnd->stop();
    goto exitCapturing;
  }

  return;

exitCapturing:
  theFrontEnd->deactivate();
  theFrontEnd->end();

  puts("End Capturing");
  exit(1);

}
