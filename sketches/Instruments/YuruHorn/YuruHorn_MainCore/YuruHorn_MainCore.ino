#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <SDHCI.h>
#include <MP.h>
#include <MediaRecorder.h>
#include <Synthesizer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>

#include <memutils/s_stl/queue.h>

//#define ENABLE_THROUGH

#include <arch/chip/cxd56_audio.h>  /* For set_datapath */

extern "C" {
  extern void board_external_amp_mute_control(bool);
}

FrontEnd *theFrontEnd;
OutputMixer *theMixer;
Synthesizer *theSynthesizer;

SDClass SD;  /**< SDClass object */

using namespace s_std;

Queue<AsPcmDataParam,30>  thePcmQue;

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


struct Result {
  uint16_t peak[mic_channel_num];
  uint16_t power[mic_channel_num];
  int  chnum;
};

#define EFFECT_ATTACK     300
#define EFFECT_DECAY      1
#define EFFECT_SUSTAIN    100
#define EFFECT_RELEASE    500

/*static bool mediarecorder_done_callback(AsRecorderEvent event, uint32_t result, uint32_t sub_result)
{
  printf("mr cb %x %x %x\n", event, result, sub_result);

  return true;
}*/

static void frontend_done_callback(AsPcmDataParam pcm_param)
{
  if(thePcmQue.full()){
    puts("error que full");
    exit(1);
  }

  thePcmQue.push(pcm_param);
  
}

static void outputmixer_done_callback(MsgQueId requester_dtq,
                                      MsgType reply_of,
                                      AsOutputMixDoneParam *done_param)
{
  printf("om cb\n");
  return;
}

static void outmixer_send_callback(int32_t identifier, bool is_end)
{
  return;
}

static void synthesizer_done_callback(AsSynthesizerEvent event, uint32_t result, void *param)
{
//  printf("ss cb %x %x \n", event, result);
  return true;
}

static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
}

void setup()
{
  int ret;

  Serial.begin(115200);
  while (!Serial);

  /* Initialize memory pools and message libs */

  initMemoryPools();
  createStaticPools(MEM_LAYOUT_RECORDINGPLAYER);

  SD.begin();

  ret = MP.begin(subcore);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }

/*  ret = MP.begin(2);
  if (ret < 0) {
    puts("MP.begin error = 2\n");
  }*/


  Serial.println("Init Recorder Library");
  theFrontEnd = FrontEnd::getInstance();
  theFrontEnd->begin();

  theSynthesizer = Synthesizer::getInstance();
  theSynthesizer->begin();

  theMixer       = OutputMixer::getInstance();

  Serial.println("Init Audio Recorder");

  /* Activate Baseband */

  theMixer->activateBaseband();
//  theFrontEnd->activate(NULL, AsMicFrontendDeviceMic, false);
  theFrontEnd->activate(NULL, false);

  /* Set capture clock */

//  theRecorder->setCapturingClkMode(MEDIARECORDER_CAPCLK_NORMAL);

  /* Select input device as AMIC */

  theMixer->create(attention_cb);
  theSynthesizer->create(attention_cb);

  theSynthesizer->activate(synthesizer_done_callback, NULL);
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);

//  usleep(100 * 1000);
  sleep(3);
  /* Set PCM capture */

  uint8_t channel;
  switch (mic_channel_num) {
    case 1: channel = AS_CHANNEL_MONO;   break;
    case 2: channel = AS_CHANNEL_STEREO; break;
    case 4: channel = AS_CHANNEL_4CH;    break;
  }

  AsDataDest  dest;
  dest.cb = frontend_done_callback;

  theFrontEnd->init(AS_CHANNEL_MONO,
                    AS_BITLENGTH_16,
                    240*3,
                    AsDataPathCallback,
                    dest,
//                    AsMicFrontendPreProcThrough,
                    AsMicFrontendPreProcSrc,
//                    AsMicFrontendPreProcVad,
                      "/mnt/sd0/BIN/SRC");
//                    "/mnt/sd0/BIN/VAD");

  theSynthesizer->init(AsSynthesizerSinWave, 2, "/mnt/sd0/BIN/OSCPROC",EFFECT_ATTACK,EFFECT_DECAY,EFFECT_SUSTAIN,EFFECT_RELEASE);
  /* Launch SubCore */

  /* ----- gokan 11-14 ----- */
  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

  /* ----- hayakawa 11-24 ----- */
#ifdef ENABLE_THROUGH
  cxd56_audio_signal_t sig_id;
  cxd56_audio_sel_t    sel_info;

  sig_id = CXD56_AUDIO_SIG_MIC1;

  sel_info.au_dat_sel1 = false;
  sel_info.au_dat_sel2 = true;
  sel_info.cod_insel2  = false;
  sel_info.cod_insel3  = true;
  sel_info.src1in_sel  = false;
  sel_info.src2in_sel  = false;

  cxd56_audio_set_datapath(sig_id, sel_info);
  cxd56_audio_set_vol(CXD56_AUDIO_VOLID_MIXER_IN2, 0);

#endif /* ENABLE_THROUGH */



  /* ----- gokan 11-14 ----- */
  //  Serial.println("Start!\n");

  /* Set Gain */

  /* Main volume set to -16.0 dB, Main player and sub player set to 0 dB */

  theMixer->setVolume(-160, 0, 0);

  /* Start Recorder */
  theSynthesizer->start();
  sleep(1); 
  theFrontEnd->start();
}

void beep_control(uint16_t pw, uint16_t fq)
{
  int vol;
  int beep_ave = MIN(fq, 2000);
  int power_ave = pw;
  const int k=12;
  static int prev_ave =0;

  if (power_ave < 50) {
    theSynthesizer->set(0, 0);
    return;
  } else if (power_ave < 100) {
    vol = -90+k;
  } else if (power_ave < 300) {
    vol = -60+k;
  } else if (power_ave < 500) {
    vol = -54+k;
  } else if (power_ave < 800) {
    vol = -48+k;
  } else if (power_ave < 1200) {
    vol = -42+k;
  } else if (power_ave < 2000) {
    vol = -36+k;
  } else {
    vol = -28+k;
  }

  /*printf("power_ave =%d\n",power_ave);
    printf("ave =%d\n",beep_ave);
    printf("vol =%d\n",vol);*/

  if ( beep_ave > 100 && beep_ave < 700 ) {
    if(prev_ave != beep_ave){
      printf("beep %d\n",beep_ave);
      theSynthesizer->set(0, beep_ave);
      prev_ave = beep_ave;
    }
//    theMixer->setVolume(0, (vol * 10), 0);
  } else {
    theSynthesizer->set(0, 0);
  }
}

void loop()
{
  int8_t   sndid = 100; /* user-defined msgid */
  int8_t   rcvid = 0;
  Capture  capture;
  Result*  result;

  static const int32_t buffer_sample = 240 * mic_channel_num;
  static const int32_t buffer_size = buffer_sample * sizeof(int16_t);
  static char  buffer[buffer_size];
  uint32_t read_size;

  /* Read frames to record in buffer */
  if(!thePcmQue.empty()){
    read_size = thePcmQue.top().size;
//    printf("pop %d\n",read_size);
//    memcpy(buffer,thePcmQue.top().mh.getPa(),thePcmQue.top().size);
    memcpy(buffer,thePcmQue.top().mh.getPa(),read_size);
    thePcmQue.pop();
  }else{
    read_size = 0;
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
      for (int i = 0; i < mic_channel_num; i++) {
        if (result->peak[i] > 100 && result->peak[i] < 700 ) {
//          MP.Send(50, result->peak[i], 2);
        }
        beep_control(result->power[i], result->peak[i]);
//        printf("main %d, %d, ", result->power[i], result->peak[i]);
      }
//      printf("\n");
    }
  }
}
