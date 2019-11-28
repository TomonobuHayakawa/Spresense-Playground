/*
 *  MainAudio.ino - MP Example for Audio FFT 
 *  Copyright 2019 Sony Semiconductor Solutions Corporation
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
#include <Audio.h>

#include <arch/chip/cxd56_audio.h>  /* For set_datapath */

extern "C" {
  extern void board_external_amp_mute_control(bool);
}

AudioClass *theAudio;

/* Select mic channel number */
const int mic_channel_num = 1;
//const int mic_channel_num = 2;
//const int mic_channel_num = 4;

const int subcore = 1;

struct Capture {
  void *buff;
  int  sample;
  int  chnum;
};

/*struct Result {
  float peak[mic_channel_num];
  int  chnum;
};*/

struct Result {
  uint16_t peak[mic_channel_num];
  uint16_t power[mic_channel_num];
  int  chnum;
};


void setup()
{
  int ret;

  Serial.begin(115200);
  while (!Serial);

  Serial.println("Init Audio Library");
  theAudio = AudioClass::getInstance();
  theAudio->begin();

  Serial.println("Init Audio Recorder");
  /* Select input device as AMIC */
  //theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC, 210);
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);

  /* Set PCM capture */
  uint8_t channel;
  switch (mic_channel_num) {
  case 1: channel = AS_CHANNEL_MONO;   break;
  case 2: channel = AS_CHANNEL_STEREO; break;
  case 4: channel = AS_CHANNEL_4CH;    break;
  }
  theAudio->initRecorder(AS_CODECTYPE_PCM, "/mnt/sd0/BIN", AS_SAMPLINGRATE_48000, channel);

  /* Launch SubCore */
  ret = MP.begin(subcore);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }

  ret = MP.begin(2);
  if (ret < 0) {
    puts("MP.begin error = 2\n");
  }

/* ----- gokan 11-14 ----- */ 
  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

/* ----- hayakawa 11-24 ----- */ 
  cxd56_audio_set_spout(true);
  cxd56_audio_en_output();

  cxd56_audio_signal_t sig_id;
  cxd56_audio_sel_t    sel_info;

  sig_id = CXD56_AUDIO_SIG_MIC1;

  sel_info.au_dat_sel1 = true;
  sel_info.au_dat_sel2 = false;
  sel_info.cod_insel2  = true;
  sel_info.cod_insel3  = false;
  sel_info.src1in_sel  = false;
  sel_info.src2in_sel  = false;

  cxd56_audio_set_datapath(sig_id, sel_info);
  cxd56_audio_set_vol(CXD56_AUDIO_VOLID_MIXER_IN1, 0);  
  cxd56_audio_set_vol(CXD56_AUDIO_VOLID_MIXER_OUT, -20/*db*/);

  board_external_amp_mute_control(false);

/* ----- gokan 11-14 ----- */ 
//  Serial.println("Start!\n");
  theAudio->startRecorder();
}

void beep_control(uint16_t pw,uint16_t fq)
{
  static int beep_fq[3] = {0,0,0};
  static int beep_pw[3] = {0,0,0};
  int vol;
  beep_pw[2] = pw;
  beep_fq[2] = MIN(fq, 650);
  int beep_ave = (beep_fq[2] + beep_fq[1] + beep_fq[0])/3;
  int power_ave = (beep_pw[2] + beep_pw[1] + beep_pw[0])/3;
  beep_fq[1] = beep_fq[2];
  beep_fq[0] = beep_fq[1];
  beep_pw[1] = beep_pw[2];
  beep_pw[0] = beep_pw[1];

  if(power_ave<50){
    vol = -90;
  }else if(power_ave<200){
    vol = -60;
  }else if(power_ave<500){
    vol = -52;
  }else if(power_ave<800){
    vol = -46;
  }else if(power_ave<1200){
    vol = -40;
  }else if(power_ave<2000){
    vol = -32;      
  }else{
    vol = -24;
  }

/*  printf("power_ave =%d\n",power_ave);
  printf("ave =%d\n",beep_ave);
  printf("vol =%d\n",vol);*/

  if ( beep_ave > 100 && beep_ave < 650 ) {
     theAudio->setBeep(1,vol, beep_ave);
  }else{
     theAudio->setBeep(0, 0, 0);
  }
}

void loop()
{
  int8_t   sndid = 100; /* user-defined msgid */
  int8_t   rcvid = 0;
  Capture  capture;
  Result*  result;
  
  static const int32_t buffer_sample = 768 * mic_channel_num;
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
    capture.buff   = buffer;
    capture.sample = buffer_sample / mic_channel_num;
    capture.chnum  = mic_channel_num;
    MP.Send(sndid, &capture, subcore);
  } else {
    /* Receive PCM captured buffer from MainCore */
    int ret = MP.Recv(&rcvid, &result, subcore);
    if (ret >= 0) {
      for(int i=0;i<mic_channel_num;i++){
        if(result->peak[i] > 100 && result->peak[i] < 650 ) {
          MP.Send(50, result->peak[i], 2);
        }
        beep_control(result->power[i],result->peak[i]);
        printf("main %d, %d, ", result->power[i], result->peak[i]);
      }
      printf("\n");
    }
  }

}
