#ifdef SUBCORE	
#error "Core selection is wrong!!"	
#endif	

#include <MP.h>
#include <MediaRecorder.h>
#include <MemoryUtil.h>

//#define ENABLE_THROUGH

#include <arch/chip/cxd56_audio.h>  /* For set_datapath */

extern "C" {
  extern void board_external_amp_mute_control(bool);
}

MediaRecorder *theRecorder;

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

static bool app_beep(bool en = false, int16_t vol = 255, uint16_t freq = 0)
{
  if (!en)
    {
      /* Stop beep */

      if (cxd56_audio_stop_beep() != CXD56_AUDIO_ECODE_OK)
        {
          return false;
        }
    }

  if (0 != freq)
    {
      /* Set beep frequency parameter */

      if (cxd56_audio_set_beep_freq(freq) != CXD56_AUDIO_ECODE_OK)
        {
          return false;
        }
    }

  if (255 != vol)
    {
      /* Set beep volume parameter */

      if (cxd56_audio_set_beep_vol(vol) != CXD56_AUDIO_ECODE_OK)
        {
          return false;
        }
    }

  if (en)
    {
      /* Play beep */

      if (cxd56_audio_play_beep() != CXD56_AUDIO_ECODE_OK)
        {
          return false;
        }
    }

  return true;
}

static bool mediarecorder_done_callback(AsRecorderEvent event, uint32_t result, uint32_t sub_result)
{
  printf("mp cb %x %x %x\n", event, result, sub_result);

  return true;
}


void setup()
{
  int ret;

  Serial.begin(115200);
  while (!Serial);

  /* Initialize memory pools and message libs */

  initMemoryPools();
  createStaticPools(MEM_LAYOUT_RECORDER);

  Serial.println("Init Recorder Library");
  theRecorder = MediaRecorder::getInstance();
  theRecorder->begin();

  Serial.println("Init Audio Recorder");

  /* Set capture clock */

  theRecorder->setCapturingClkMode(MEDIARECORDER_CAPCLK_NORMAL);

  /* Select input device as AMIC */

  theRecorder->activate(AS_SETRECDR_STS_INPUTDEVICE_MIC, mediarecorder_done_callback);

  /* Set PCM capture */

  uint8_t channel;
  switch (mic_channel_num) {
  case 1: channel = AS_CHANNEL_MONO;   break;
  case 2: channel = AS_CHANNEL_STEREO; break;
  case 4: channel = AS_CHANNEL_4CH;    break;
  }
  theRecorder->init(AS_CODECTYPE_LPCM,
                    channel,
                    AS_SAMPLINGRATE_48000,
                    AS_BITLENGTH_16,
                    AS_BITRATE_96000,
                    "/mnt/sd0/BIN");

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

#ifdef ENABLE_THROUGH
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

#endif /* ENABLE_THROUGH */

  board_external_amp_mute_control(false);

/* ----- gokan 11-14 ----- */ 
//  Serial.println("Start!\n");

  /* Set Gain */

  theRecorder->setMicGain(180);

  /* Start Recorder */

  theRecorder->start();
}

void beep_control(uint16_t pw,uint16_t fq)
{
  int vol;
  int beep_ave = MIN(fq, 700);
  int power_ave = pw;

  if(power_ave<50){
     app_beep(0, 0, 0);
     return;
  }else if(power_ave<100){
    vol = -90;
  }else if(power_ave<300){
    vol = -60;
  }else if(power_ave<500){
    vol = -54;
  }else if(power_ave<800){
    vol = -48;
  }else if(power_ave<1200){
    vol = -42;
  }else if(power_ave<2000){
    vol = -36;      
  }else{
    vol = -28;
  }

/*  printf("power_ave =%d\n",power_ave);
  printf("ave =%d\n",beep_ave);
  printf("vol =%d\n",vol);*/

  if ( beep_ave > 100 && beep_ave < 700 ) {
     app_beep(1,vol, beep_ave);
  }else{
     app_beep(0, 0, 0);
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
  int err = theRecorder->readFrames(buffer, buffer_size, &read_size);

  if (err != MEDIARECORDER_ECODE_OK && err != MEDIARECORDER_ECODE_INSUFFICIENT_BUFFER_AREA) {
    printf("Error err = %d\n", err);
    sleep(1);
    theRecorder->stop();
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
        if(result->peak[i] > 100 && result->peak[i] < 700 ) {
          MP.Send(50, result->peak[i], 2);
        }
        beep_control(result->power[i],result->peak[i]);
        printf("main %d, %d, ", result->power[i], result->peak[i]);
      }
      printf("\n");
    }
  }

}
