#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

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
  board_external_amp_mute_control(false);

/* ----- gokan 11-14 ----- */ 
//  Serial.println("Start!\n");
  theAudio->startRecorder();
}

void beep_control(int fq)
{
  static int beep_fq[3] = {0,0,0};
  static int beep_pw[3] = {0,0,0};
  int vol;
  beep_pw[2] = 0xffff & fq;
  beep_fq[2] = MIN((fq >> 16), 650);
  int beep_ave = (beep_fq[2] + beep_fq[1] + beep_fq[0])/3;
  int power_ave = (beep_pw[2] + beep_pw[1] + beep_pw[0])/3;
  beep_fq[1] = beep_fq[2];
  beep_fq[0] = beep_fq[1];
  beep_pw[1] = beep_pw[2];
  beep_pw[0] = beep_pw[1];

  if(power_ave<30){
    vol = -120;
  }else if(power_ave<100){
    vol = -60;    
  }else if(power_ave<300){
    vol = -40; 
  }else if(power_ave<500){
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
  Capture  capture;

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
    usleep(1);
  }

/* ----- gokan 11-14 ----- */
    int       ret_fq;
    int8_t    id_fq;
    int       peak_fq;
    ret_fq = MP.Recv( &id_fq, &peak_fq, subcore );
    if ( ret_fq > 0 ) {
      if((peak_fq >> 16) > 100 && (peak_fq >> 16) < 650 ) {
        int peakFsi = (int) (peak_fq >> 16);
        MP.Send(50, peakFsi, 2);
      }
      beep_control(peak_fq);
    }
}
